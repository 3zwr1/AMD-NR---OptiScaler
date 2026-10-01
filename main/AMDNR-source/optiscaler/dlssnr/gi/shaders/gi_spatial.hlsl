// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// P5 Spatial, one dispatch per a-trous iteration (b1 a.x = iteration, a.y = step width in trace px). Design 3.2 P5:
// a 3x3 edge-avoiding a-trous step (Dammertz et al. 2010) with the SVGF weights (Schied et al. 2017): the distance
// of the neighbour to this texel's tangent plane, the normal agreement, and the luminance difference scaled by the
// temporal standard deviation from the moments. Texels with fewer than 4 frames of history use twice the step and
// no luminance weight (the SVGF rule for short histories). The history keeps the unfiltered value (no feedback).
//
// t0 source (iteration 0: histGI[cur]; 1: gi)  t1 histMom[cur]  t2 histLen[cur]  t3 zHalf[cur]  t4 nHalf[cur]
// u0 destination (iteration 0: gi; 1: trace)
#include "gi_common.hlsli"

Texture2D<float4> tSrc : register(t0);
Texture2D<float2> tHistMom : register(t1);
Texture2D<uint> tHistLen : register(t2);
Texture2D<float2> tZHalf : register(t3);
Texture2D<float2> tNHalf : register(t4);
RWTexture2D<float4> uDst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= TraceSize()))
        return;
    const float4 c = tSrc.Load(int3(id.xy, 0));
    const float2 zs = tZHalf.Load(int3(id.xy, 0));
    if (IsSkyZ(zs.x) || !AllFinite(c))
    {
        uDst[id.xy] = AllFinite(c) ? c : float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }
    const float3 P = ViewPosition(RepCentre(id.xy, zs.y), zs.x);
    const float3 N = DecodeNormal(tNHalf.Load(int3(id.xy, 0)));
    const uint len = tHistLen.Load(int3(id.xy, 0));
    const bool young = len < 4u;
    const int step = int(max(p.a.y, 1u)) * (young ? 2 : 1);
    const float2 m = tHistMom.Load(int3(id.xy, 0));
    const float sigma = sqrt(max(m.y - m.x * m.x, 0.0f));
    const float lc = Luminance(c.rgb);
    const float lumScale = 1.0f / (4.0f * sigma + 0.05f * lc + 1.0e-4f);
    const float planeScale = 1.0f / (0.01f * float(step) * zs.x);

    // 3x3 taps (1/4, 1/2, 1/4 per axis). A 5x5 B3-spline kernel was measured (gilab s0/s3/s1/s6, 2026-09-26) and did
    // not lower flicker or motion change, so the cheaper kernel of the design stays.
    static const float kTap[3] = { 0.25f, 0.5f, 0.25f };
    float4 sum = kTap[1] * kTap[1] * c;
    float wSum = kTap[1] * kTap[1];
    [unroll] for (int dy = -1; dy <= 1; ++dy)
    {
        [unroll] for (int dx = -1; dx <= 1; ++dx)
        {
            if (dx == 0 && dy == 0)
                continue;
            const int2 q = int2(id.xy) + int2(dx, dy) * step;
            if (any(q < 0) || any(q >= int2(TraceSize())))
                continue;
            const float2 zq = tZHalf.Load(int3(q, 0));
            if (IsSkyZ(zq.x))
                continue;
            const float4 s = tSrc.Load(int3(q, 0));
            if (!AllFinite(s))
                continue;
            const float3 Q = ViewPosition(RepCentre(uint2(q), zq.y), zq.x);
            const float wPlane = exp(-abs(dot(N, Q - P)) * planeScale);
            const float nd = saturate(dot(N, DecodeNormal(tNHalf.Load(int3(q, 0)))));
            const float nd2 = nd * nd, nd4 = nd2 * nd2;
            const float wNormal = nd4 * nd4; // cos^8
            const float wLum = young ? 1.0f : exp(-abs(Luminance(s.rgb) - lc) * lumScale);
            const float kernel = kTap[dx + 1] * kTap[dy + 1];
            const float w = kernel * wPlane * wNormal * wLum;
            sum += w * s;
            wSum += w;
        }
    }
    uDst[id.xy] = sum / wSum;
}
