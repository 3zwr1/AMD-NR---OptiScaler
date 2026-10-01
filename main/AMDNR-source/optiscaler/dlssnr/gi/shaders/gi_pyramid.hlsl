// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// P2 Pyramid, one dispatch per level (b1 a.x = level, over LevelSize(level) = the trace grid divided by 2^level,
// rounded up). Design 3.2 P2.
//   Level 0: the radiance source of the trace, L = C x (1 - reactive) + feedback x last frame's added light
//            (bounceHalf, reprojected with the MVs and a depth test, rescaled by the pre-exposure ratio). Sky texels
//            are not emitters (L = 0); with Sky > 0 their colour is summed into GI_STATE_SKYSUM instead. zMip level
//            0 = the trace-grid depth.
//   Level 1-5: 2x2 reduction: depth min/max, radiance mean, max luminance (a) for the energy guard.
//
// t0 colour  t1 depth  t2 reactive  t3 motion  t4 zHalf[cur]  t5 zHalf[prev]  t6 bounceHalf (last frame's)
// t7 nHalf[cur]
// u0-u5  zMip levels 0-5 (R32G32_FLOAT: x min view z, y max view z)
// u6-u11 radMip levels 0-5 (R16G16B16A16_FLOAT: rgb radiance, a max luminance)
// u12 state
#include "gi_common.hlsli"

Texture2D<float4> tColour : register(t0);
Texture2D<float> tDepth : register(t1);
Texture2D<float> tReactive : register(t2);
Texture2D<float2> tMotion : register(t3);
Texture2D<float2> tZHalf : register(t4);
Texture2D<float2> tZHalfPrev : register(t5);
Texture2D<float3> tBounce : register(t6);
Texture2D<float2> tNHalf : register(t7);
RWTexture2D<float2> uZMip0 : register(u0);
RWTexture2D<float2> uZMip1 : register(u1);
RWTexture2D<float2> uZMip2 : register(u2);
RWTexture2D<float2> uZMip3 : register(u3);
RWTexture2D<float2> uZMip4 : register(u4);
RWTexture2D<float2> uZMip5 : register(u5);
RWTexture2D<float4> uRadMip0 : register(u6);
RWTexture2D<float4> uRadMip1 : register(u7);
RWTexture2D<float4> uRadMip2 : register(u8);
RWTexture2D<float4> uRadMip3 : register(u9);
RWTexture2D<float4> uRadMip4 : register(u10);
RWTexture2D<float4> uRadMip5 : register(u11);
RWByteAddressBuffer uState : register(u12);

// The level is a root constant (uniform), so plain switches select the mip views.
float2 ZAt(uint level, uint2 xy)
{
    switch (level)
    {
    case 0: return uZMip0[xy];
    case 1: return uZMip1[xy];
    case 2: return uZMip2[xy];
    case 3: return uZMip3[xy];
    default: return uZMip4[xy];
    }
}
float4 RadAt(uint level, uint2 xy)
{
    switch (level)
    {
    case 0: return uRadMip0[xy];
    case 1: return uRadMip1[xy];
    case 2: return uRadMip2[xy];
    case 3: return uRadMip3[xy];
    default: return uRadMip4[xy];
    }
}
void Store(uint level, uint2 xy, float2 z, float4 r)
{
    switch (level)
    {
    case 0: uZMip0[xy] = z; uRadMip0[xy] = r; break;
    case 1: uZMip1[xy] = z; uRadMip1[xy] = r; break;
    case 2: uZMip2[xy] = z; uRadMip2[xy] = r; break;
    case 3: uZMip3[xy] = z; uRadMip3[xy] = r; break;
    case 4: uZMip4[xy] = z; uRadMip4[xy] = r; break;
    default: uZMip5[xy] = z; uRadMip5[xy] = r; break;
    }
}

// Last frame's added light at this texel's surface, or 0 when it cannot be found (reset, no MVs, off screen,
// disoccluded).
float3 PreviousBounce(uint2 t, float z, float2 rep)
{
    if (g.composite1.x <= 0.0f || HistoryReset() || !HasMotion())
        return float3(0.0f, 0.0f, 0.0f);
    const float2 q = float2(t) + 0.5f + MotionAt(tMotion, rep) * g.traceRatio.zw; // texel centre + displacement
    if (any(q < 0.0f) || any(q >= float2(TraceSize())))
        return float3(0.0f, 0.0f, 0.0f);
    const uint2 qi = uint2(q);
    const float zp = tZHalfPrev.Load(int3(qi, 0)).x;
    if (IsSkyZ(zp) || abs(zp - z) > 0.1f * min(zp, z))
        return float3(0.0f, 0.0f, 0.0f);
    const float3 b = tBounce.Load(int3(qi, 0)) * g.exposure.y;
    return AllFinite3(b) ? max(b, 0.0f) : float3(0.0f, 0.0f, 0.0f);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint level = min(p.a.x, GI_PYRAMID_LEVELS - 1);
    const uint2 size = LevelSize(level);
    if (any(id.xy >= size))
        return;
    if (level == 0)
    {
        const float2 zs = tZHalf.Load(int3(id.xy, 0));
        float3 L = float3(0.0f, 0.0f, 0.0f);
        // Sky colour for the Sky term (P6): per-wave sums, one atomic per wave and channel.
        uint4 sky = uint4(0u, 0u, 0u, 0u);
        if (IsSkyZ(zs.x) && g.composite0.w > 0.0f)
        {
            const float3 c = DecodeColour(tColour.Load(int3(RepPixel(id.xy, uint(zs.y + 0.5f)), 0)).rgb);
            if (AllFinite3(c))
                sky = uint4(uint3(clamp(c, 0.0f, 64.0f) * GI_SKY_FIXED + 0.5f), 1u);
        }
        const uint4 waveSky = uint4(WaveActiveSum(sky.x), WaveActiveSum(sky.y), WaveActiveSum(sky.z), WaveActiveSum(sky.w));
        if (WaveIsFirstLane() && waveSky.w > 0u)
        {
            uState.InterlockedAdd(GI_STATE_SKYSUM + 0, waveSky.x);
            uState.InterlockedAdd(GI_STATE_SKYSUM + 4, waveSky.y);
            uState.InterlockedAdd(GI_STATE_SKYSUM + 8, waveSky.z);
            uState.InterlockedAdd(GI_STATE_SKYSUM + 12, waveSky.w);
        }
        if (!IsSkyZ(zs.x))
        {
            const uint2 rp = RepPixel(id.xy, uint(zs.y + 0.5f));
            float3 c = DecodeColour(tColour.Load(int3(rp, 0)).rgb);
            c = AllFinite3(c) ? max(c, 0.0f) : float3(0.0f, 0.0f, 0.0f);
            const float r = HasReactive() ? saturate(tReactive.Load(int3(rp, 0))) : 0.0f;
            L = c * (1.0f - r) + g.composite1.x * PreviousBounce(id.xy, zs.x, float2(rp) + 0.5f);
        }
        Store(0, id.xy, zs.xx, float4(L, Luminance(L)));
        return;
    }
    const uint2 src = LevelSize(level - 1);
    float2 z = float2(3.0e38f, 0.0f);
    float4 r = 0.0f;
    [unroll] for (uint k = 0; k < 4; ++k)
    {
        const uint2 s = min(id.xy * 2 + uint2(k & 1, k >> 1), src - 1);
        const float2 zs = ZAt(level - 1, s);
        const float4 rs = RadAt(level - 1, s);
        z = float2(min(z.x, zs.x), max(z.y, zs.y));
        r.rgb += rs.rgb * 0.25f;
        r.a = max(r.a, rs.a);
    }
    Store(level, id.xy, z, r);
}
