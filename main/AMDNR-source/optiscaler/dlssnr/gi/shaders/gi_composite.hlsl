// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// P6 Upsample + composite (render grid; design 3.2 P6 and 2.1 item 11), in linear light:
//   - joint bilateral upsample (Kopf et al. 2007): the 4 trace texels of the bilinear footprint, each weighted by how
//     far this pixel lies from the texel's tangent plane (the full-res depth is linearised inline); no valid tap:
//     the tap nearest in depth;
//   - albedo estimate (EstimateAlbedo, AlbedoMode), Bounce colour (saturation);
//   - AO with the GTAO multi-bounce fit, faded toward 1 on pixels much brighter than their neighbourhood (direct
//     light, emissives: AoLitProtect), scaled by Occlusion;
//   - added light = albedo x (E / pi x Bounce light + visibility x sky radiance x Sky), the sky radiance being the
//     mean colour of this frame's sky texels (GI_STATE_SKYSUM, summed by P2; Sky defaults to 0);
//   - masks: reactive (soft), NearFade, DistanceFade (values > 0 in view-depth units; auto = no fade in this preview);
//     sky pixels pass through;
//   - out = C x AO + added, re-encoded for display-referred colour, clamped to [0, 1] for UNORM out. A pixel the GI
//     does not change (AO factor exactly 1 and nothing added) is copied bit for bit, and so is any pixel whose
//     result is not finite (a NaN handed to FSR/XeSS would stay in their history).
// Threads inside the trace grid also write bounceHalf: the light this frame adds at that texel, the next frame's
// multi-bounce source.
//
// t0 gi (final spatial result)  t1 zHalf[cur]  t2 nHalf[cur]  t3 depth  t4 colour  t5 reactive  t6 radMip
// t7 exposure  t8 histLen[cur]
// u0 out  u1 bounceHalf (R11G11B10_FLOAT, trace grid; the next frame's multi-bounce input)  u2 state
#include "gi_common.hlsli"

Texture2D<float4> tGi : register(t0);
Texture2D<float2> tZHalf : register(t1);
Texture2D<float2> tNHalf : register(t2);
Texture2D<float> tDepth : register(t3);
Texture2D<float4> tColour : register(t4);
Texture2D<float> tReactive : register(t5);
Texture2D<float4> tRadMip : register(t6);
Texture2D<float> tExposure : register(t7);
Texture2D<uint> tHistLen : register(t8);
RWTexture2D<float4> uOut : register(u0);
RWTexture2D<float3> uBounce : register(u1);
RWByteAddressBuffer uState : register(u2);


float Reactive(uint2 px) { return HasReactive() ? saturate(tReactive.Load(int3(px, 0))) : 0.0f; }

// Fades by view depth (composite1.y NearFade, composite1.z DistanceFade): -1 auto (no fade in this preview), 0 off,
// > 0 the view depth where the fade ends.
float DepthMask(float z)
{
    float w = 1.0f;
    const float nearV = g.composite1.y, farV = g.composite1.z;
    if (nearV > 0.0f)
        w *= saturate((z - 0.5f * nearV) / (0.5f * nearV));
    if (farV > 0.0f)
        w *= 1.0f - saturate((z - farV) / farV);
    return w;
}

// Sky / ambient (composite0.w, default 0: the game's colour already holds its own sky light). This preview uses
// the mean colour of this frame's sky texels as the radiance of every open sector; no sky on screen = no sky term.
float3 SkyRadiance()
{
    if (g.composite0.w <= 0.0f)
        return float3(0.0f, 0.0f, 0.0f);
    const uint4 s = uState.Load4(GI_STATE_SKYSUM);
    if (s.w < 16u)
        return float3(0.0f, 0.0f, 0.0f);
    return float3(s.xyz) / (GI_SKY_FIXED * float(s.w));
}

// Incident light the composite adds through the albedo (E / pi): the bounce x Bounce light plus the open share x
// the sky radiance x Sky.
float3 IncidentLight(float4 gi, float3 skyL)
{
    return max(gi.rgb, 0.0f) * g.composite0.x + saturate(gi.a) * skyL * g.composite0.w;
}

// Added light for the albedo estimate of colour c and the GI value gi.
float3 AddedLight(float3 c, float4 gi, float localMeanLum, float3 skyL)
{
    const float3 rho = EstimateAlbedo(c, localMeanLum, g.spatial.y);
    return rho * IncidentLight(gi, skyL);
}

// Joint bilateral upsample of the trace grid at render pixel px (view position P, view depth z).
float4 Upsample(uint2 px, float3 P, float z)
{
    const float2 tp = (float2(px) + 0.5f) * g.traceRatio.zw - 0.5f;
    const int2 b = int2(floor(tp));
    const float2 f = tp - float2(b);
    float4 sum = 0.0f;
    float wSum = 0.0f, bestDz = 3.0e38f;
    float4 best = float4(0.0f, 0.0f, 0.0f, 1.0f);
    [unroll] for (uint k = 0; k < 4; ++k)
    {
        const int2 q = clamp(b + int2(k & 1u, k >> 1), int2(0, 0), int2(TraceSize()) - 1);
        const float2 zq = tZHalf.Load(int3(q, 0));
        if (IsSkyZ(zq.x))
            continue;
        const float4 v = tGi.Load(int3(q, 0));
        if (!AllFinite(v))
            continue;
        const float dz = abs(zq.x - z);
        if (dz < bestDz)
        {
            bestDz = dz;
            best = v;
        }
        const float3 Q = ViewPosition(RepCentre(uint2(q), zq.y), zq.x);
        const float3 Nq = DecodeNormal(tNHalf.Load(int3(q, 0)));
        const float plane = abs(dot(Nq, P - Q)) / max(z, 1.0e-6f);
        const float bw = ((k & 1u) ? f.x : 1.0f - f.x) * ((k >> 1) ? f.y : 1.0f - f.y);
        const float w = max(bw, 1.0e-3f) * exp(-plane * 50.0f);
        sum += w * v;
        wSum += w;
    }
    return wSum > 1.0e-4f ? sum / wSum : best;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    // bounceHalf (trace grid, never larger than the render grid): this frame's added light at each texel.
    if (all(id.xy < TraceSize()))
    {
        float3 bounce = float3(0.0f, 0.0f, 0.0f);
        const float2 zs = tZHalf.Load(int3(id.xy, 0));
        if (!IsSkyZ(zs.x) && g.composite1.x > 0.0f)
        {
            const uint2 rp = RepPixel(id.xy, uint(zs.y + 0.5f));
            const float3 c = DecodeColour(tColour.Load(int3(rp, 0)).rgb);
            const float4 gi = tGi.Load(int3(id.xy, 0));
            const float lm = Luminance(LocalStats(tRadMip, float2(id.xy) + 0.5f).rgb);
            if (AllFinite3(c) && AllFinite(gi))
                bounce = AddedLight(max(c, 0.0f), gi, lm, SkyRadiance()) * (1.0f - Reactive(rp)) * DepthMask(zs.x);
        }
        uBounce[id.xy] = AllFinite3(bounce) ? max(bounce, 0.0f) : float3(0.0f, 0.0f, 0.0f);
    }
    if (any(id.xy >= RenderSize()))
        return;

    const float4 raw = tColour.Load(int3(id.xy, 0));
    const float d = tDepth.Load(int3(id.xy, 0));
    const float z = ViewZ(d);
    const float3 c = DecodeColour(raw.rgb);
    if (IsSkyZ(z) || !AllFinite3(c))
    {
        uOut[id.xy] = raw;
        return;
    }
    const float3 P = ViewPosition(float2(id.xy) + 0.5f, z);
    const float4 gi = Upsample(id.xy, P, z);
    const float2 tracePx = (float2(id.xy) + 0.5f) * g.traceRatio.zw;
    const float4 stats = LocalStats(tRadMip, tracePx);
    const float meanLum = Luminance(stats.rgb);
    const float3 cp = max(c, 0.0f);

    const float mask = (1.0f - Reactive(id.xy)) * DepthMask(z);
    const float3 rho = EstimateAlbedo(cp, meanLum, g.spatial.y);
    // AO spares direct light and emissives: fade it out where the pixel is far brighter than its neighbourhood.
    const float litProtect = LitProtect(cp, meanLum);
    const float aoMb = MultiBounceAo(saturate(gi.a), Luminance(rho));
    const float occlusion = g.composite0.y * (1.0f - litProtect) * mask;
    const float aoFactor = max(lerp(1.0f, aoMb, occlusion), 0.0f);
    const float3 added = rho * IncidentLight(gi, SkyRadiance()) * mask;

    if (aoFactor == 1.0f && all(added == 0.0f))
    {
        uOut[id.xy] = raw; // unchanged: bit for bit
        return;
    }
    float3 o = c * aoFactor + added; // c, not cp: scRGB may carry negative (wide-gamut) components
    if (g.colourInfo.x != GI_ENC_LINEAR)
        o = EncodeColour(o);
    if (g.colourInfo.z != 0)
        o = saturate(o);
    const float4 result = float4(o, raw.a);
    uOut[id.xy] = AllFinite(result) ? result : raw;
}
