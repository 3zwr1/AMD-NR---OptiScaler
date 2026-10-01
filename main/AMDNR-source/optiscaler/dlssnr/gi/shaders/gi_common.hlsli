// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// AMDNR Screen GI: shared HLSL. Bindings of every pass: root signature = b0 frame constants (root CBV), b1 eight
// root constants, one table t0-t11 + u0-u15, static samplers s0 point-clamp and s1 linear-clamp. Unused slots hold
// null descriptors. The slot map per pass is in AmdnrGi.cpp (the pass table) and the internal design note for it.
#ifndef AMDNR_GI_COMMON_HLSLI
#define AMDNR_GI_COMMON_HLSLI

#include "../GiShared.h"

cbuffer GiFrame : register(b0)
{
    GiFrameConstants g;
};

cbuffer GiPass : register(b1)
{
    GiPassConstants p;
};

SamplerState sPoint : register(s0);
SamplerState sLinear : register(s1);

// Every loop in a pass has a fixed upper bound whatever the constants say (design 3.5: a hang is not a throw).
#define GI_MAX_SLICES 4
#define GI_MAX_STEPS 32
#define GI_MAX_ATROUS 2

static const float kGiPi = 3.14159265f;
static const float kGiHalfPi = 1.57079633f;

uint2 RenderSize() { return g.renderSize.xy; }
uint2 TraceSize() { return g.renderSize.zw; }
bool HistoryReset() { return g.frame.y != 0; }
bool HasMotion() { return g.motionInfo.x != 0; }
bool HasReactive() { return g.colourInfo.w != 0; }

// The render pixel a trace-grid texel represents (its centre, clamped into the subrect).
uint2 TraceToRender(uint2 t)
{
    const float2 c = (float2(t) + 0.5f) * g.traceRatio.xy;
    return min(uint2(c), RenderSize() - 1);
}

// Sky (no geometry) is stored in zHalf as this sentinel, far beyond any real 1/d (d >= eps gives <= 1/eps = 1e6).
// Squares of it still fit a float; every pass tests IsSkyZ before it builds a position from a depth.
#define GI_SKY_Z 1.0e9f
bool IsSkyZ(float z) { return z >= 0.5f * GI_SKY_Z; }

// The far plane / cleared depth of the convention in use.
bool IsSkyDepth(float d)
{
    if (g.depthInfo.x == GI_DEPTH_REVERSED)
        return d <= 0.0f;
    if (g.depthInfo.x == GI_DEPTH_LINEAR)
        return d <= 0.0f;
    return d >= 1.0f;
}

// Scale-free view depth from the raw depth value (design 3.1). Near/far are not needed: the passes work in ratios.
float LinearViewZ(float d)
{
    const float eps = g.depthParams.x;
    if (g.depthInfo.x == GI_DEPTH_REVERSED)
        return 1.0f / max(d, eps);
    if (g.depthInfo.x == GI_DEPTH_LINEAR)
        return max(d * g.depthParams.y, eps);
    return 1.0f / max(1.0f - d, eps);
}

// LinearViewZ with sky mapped to GI_SKY_Z (and a non-finite depth treated as sky).
float ViewZ(float d)
{
    if (!(d == d) || IsSkyDepth(d))
        return GI_SKY_Z;
    return LinearViewZ(d);
}

// View-space position of a render pixel centre at view depth z (x right, y up, z forward).
float3 ViewPosition(float2 renderPixelCentre, float z)
{
    const float2 ndc = renderPixelCentre * g.invSizes.xy * 2.0f - 1.0f;
    return float3(ndc.x * g.camera.x * z, -ndc.y * g.camera.y * z, z);
}

float Luminance(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }

// Exponent-bit test, not a float compare: the shaders are built without -Gis, so DXC marks float compares 'fast'
// (no-NaN semantics) and a v == v / abs(v) <= max test may be folded away. An all-ones exponent is Inf or NaN.
bool AllFinite(float4 v) { return all((asuint(v) & 0x7F800000u) != 0x7F800000u); }
bool AllFinite3(float3 v) { return all((asuint(v) & 0x7F800000u) != 0x7F800000u); }

// ---- representative pixel of a trace texel (design 3.2 P1) ----
// With a trace ratio of about 2 or more, a trace texel covers the 2x2 render footprint nearest its centre and keeps
// one of those four pixels (the checkerboard min/max choice of P1); zHalf.y stores which one (0..3). Below a ratio of
// 1.5 the texel is its centre pixel and the offset is 0.
bool HasFootprint() { return g.traceRatio.x >= 1.5f && g.traceRatio.y >= 1.5f; }

int2 FootprintBase(uint2 t)
{
    if (!HasFootprint())
        return int2(TraceToRender(t));
    const float2 c = (float2(t) + 0.5f) * g.traceRatio.xy;
    return clamp(int2(floor(c - 0.5f)), int2(0, 0), int2(RenderSize()) - 2);
}

uint2 RepPixel(uint2 t, uint k)
{
    const int2 b = FootprintBase(t) + int2(k & 1u, (k >> 1) & 1u);
    return uint2(min(b, int2(RenderSize()) - 1));
}

// The render-space position (pixel centre) of the texel's representative; k = zHalf.y.
float2 RepCentre(uint2 t, float k)
{
    return float2(RepPixel(t, uint(clamp(k, 0.0f, 3.0f) + 0.5f))) + 0.5f;
}

// Motion vector at a render position, in render pixels (design 3.1: the MV is added, prev = p + mv).
float2 MotionAt(Texture2D<float2> mvTex, float2 renderPos)
{
    const float2 mvTexel = renderPos * g.motion.zw;
    const float2 mv = mvTex.Load(int3(int2(mvTexel), 0));
    const float2 px = mv * g.motion.xy / max(g.motion.zw, 1.0e-6f);
    return AllFinite(float4(px, 0.0f, 0.0f)) ? px : float2(0.0f, 0.0f);
}

// Level-m UV of a trace-grid position (trace px), clamped half a texel inside the written part of that level (the
// allocation is larger than the trace grid, and the rest of it is never written).
uint2 LevelSize(uint m) { return max((TraceSize() + (1u << m) - 1u) >> m, 1u); }
float2 MipUv(float2 tracePx, uint m)
{
    const float2 valid = float2(LevelSize(m));
    const float2 alloc = float2(max(g.allocSize.zw >> m, 1u));
    const float2 p = clamp(tracePx / float(1u << m), 0.5f, valid - 0.5f);
    return p / alloc;
}

// Interleaved gradient noise (Jimenez 2014), the formula from the talk.
float Ign(float2 px) { return frac(52.9829189f * frac(dot(px, float2(0.06711056f, 0.00583715f)))); }

// Transfer functions (IEC 61966-2-1 sRGB; pure 2.2 power), written for this module.
float3 SrgbToLinear(float3 c)
{
    c = saturate(c);
    return select(c <= 0.04045f, c / 12.92f, pow((c + 0.055f) / 1.055f, 2.4f));
}
float3 LinearToSrgb(float3 c)
{
    c = saturate(c);
    return select(c <= 0.0031308f, c * 12.92f, 1.055f * pow(c, 1.0f / 2.4f) - 0.055f);
}
float3 DecodeColour(float3 c)
{
    if (g.colourInfo.x == GI_ENC_SRGB)
        return SrgbToLinear(c);
    if (g.colourInfo.x == GI_ENC_GAMMA22)
        return pow(saturate(c), 2.2f);
    return c;
}
float3 EncodeColour(float3 c)
{
    if (g.colourInfo.x == GI_ENC_SRGB)
        return LinearToSrgb(c);
    if (g.colourInfo.x == GI_ENC_GAMMA22)
        return pow(saturate(c), 1.0f / 2.2f);
    return c;
}

// Octahedral normal encoding (view space), for nHalf (R16G16_SNORM).
float2 OctWrap(float2 v) { return (1.0f - abs(v.yx)) * select(v >= 0.0f, 1.0f, -1.0f); }
float2 EncodeNormal(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0f ? n.xy : OctWrap(n.xy);
    return n.xy;
}
float3 DecodeNormal(float2 e)
{
    float3 n = float3(e.x, e.y, 1.0f - abs(e.x) - abs(e.y));
    const float t = saturate(-n.z);
    n.xy += select(n.xy >= 0.0f, -t, t);
    const float l2 = dot(n, n);
    return l2 > 1.0e-12f ? n * rsqrt(l2) : float3(0.0f, 0.0f, -1.0f);
}

// ---- composite helpers shared by P6 (composite, bounceHalf) and P7 (debug) ----
static const float kGiRhoAverage = 0.45f; // assumed mean albedo of the scene (M2/M4 tune it)

// Albedo estimate (design 3.2 P6). Mode 0 (default): chroma of the colour with a dark-surface guard (a surface
// more than 4x darker than its neighbourhood gets proportionally less albedo; a shadowed floor keeps it). Mode 1:
// retinex with the local mean (design (a)). Mode 2: chroma only (design (c)).
float3 EstimateAlbedo(float3 c, float localMeanLum, int mode)
{
    const float lc = Luminance(c);
    float3 rho;
    if (mode == 1)
        rho = c / max(localMeanLum, 1.0e-6f) * kGiRhoAverage;
    else if (mode == 2)
        rho = kGiRhoAverage * c / max(lc, 1.0e-6f);
    else
        rho = kGiRhoAverage * c / max(max(lc, 0.25f * localMeanLum), 1.0e-6f);
    rho = clamp(rho, 0.02f, 0.9f);
    const float s = g.composite0.z; // Bounce colour
    return max(lerp(Luminance(rho).xxx, rho, s), 0.0f);
}

// Neighbourhood statistics for the albedo estimate and the lit protection: radMip at level 3 (8 x 8 trace texels),
// bilinear. rgb = mean radiance, a = max luminance.
static const uint kGiStatsMip = 3;
float4 LocalStats(Texture2D<float4> radMip, float2 tracePx)
{
    const uint m = min(kGiStatsMip, uint(g.trace.w));
    return radMip.SampleLevel(sLinear, MipUv(tracePx, m), m);
}

// AO spares direct light and emissives (design 2.1 item 11): 0 = AO fully applied, 1 = AO spared. The pixel is
// compared with its neighbourhood mean; AoLitProtect (composite1.w) scales it. P6 applies it, P7 view 10 shows it.
float LitProtect(float3 c, float meanLum)
{
    const float ratio = Luminance(c) / max(meanLum, 1.0e-6f);
    return g.composite1.w * smoothstep(2.0f, 8.0f, ratio);
}

// GTAO multi-bounce fit (Jimenez et al. 2016): visibility and albedo -> occlusion with interreflections.
float MultiBounceAo(float v, float albedo)
{
    const float a = 2.0404f * albedo - 0.3324f;
    const float b = -4.7951f * albedo + 0.6417f;
    const float c = 2.7552f * albedo + 0.6903f;
    return max(v, ((v * a + b) * v + c) * v);
}

#endif // AMDNR_GI_COMMON_HLSLI
