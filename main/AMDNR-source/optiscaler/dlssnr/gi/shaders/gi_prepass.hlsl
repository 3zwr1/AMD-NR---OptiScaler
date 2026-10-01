// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// P1 Prepass (trace grid; design 3.2 P1). Per trace texel:
//   - the representative: of the 2x2 render footprint, the nearest depth on "white" checkerboard texels and the
//     farthest on "black" ones, so both sides of a depth edge survive at half resolution; its index (0..3) goes to
//     zHalf.y so later passes use the exact position (RepCentre in gi_common.hlsli);
//   - the view-space normal at the representative, from the full-res depth: per axis, the neighbour pair with the
//     smaller depth jump (the idea of Turanszki 2019 / Wu 2020), oriented toward the camera;
//   - the depth probe (counts, min/max, histogram) the host reads 4 frames late for the depth-convention verdict.
//
// t0 colour   t1 depth   t2 reactive
// u0 zHalf[cur] (R32G32_FLOAT: x view z or GI_SKY_Z, y representative offset 0..3)
// u1 nHalf[cur] (R16G16_SNORM: octahedral view normal)
// u2 state (RWByteAddressBuffer; probe words at GI_STATE_PROBE, cleared by the host before this pass)
#include "gi_common.hlsli"

Texture2D<float4> tColour : register(t0);
Texture2D<float> tDepth : register(t1);
Texture2D<float> tReactive : register(t2);
RWTexture2D<float2> uZHalf : register(u0);
RWTexture2D<float2> uNHalf : register(u1);
RWByteAddressBuffer uState : register(u2);

float ZAt(int2 p)
{
    p = clamp(p, int2(0, 0), int2(RenderSize()) - 1);
    return ViewZ(tDepth.Load(int3(p, 0)));
}

// One axis of the tangent frame: the neighbour (left/right or up/down) whose depth is closer to the centre's, as a
// forward difference so both choices point the same way. `hasLo`/`hasHi` exclude neighbours outside the subrect.
float3 Tangent(float3 pc, float zc, int2 p, int2 axis, bool hasLo, bool hasHi, out bool ok)
{
    const float zl = hasLo ? ZAt(p - axis) : GI_SKY_Z;
    const float zh = hasHi ? ZAt(p + axis) : GI_SKY_Z;
    const bool lo = hasLo && !IsSkyZ(zl);
    const bool hi = hasHi && !IsSkyZ(zh);
    ok = lo || hi;
    const bool useHi = hi && (!lo || abs(zh - zc) <= abs(zl - zc));
    if (useHi)
        return ViewPosition(float2(p + axis) + 0.5f, zh) - pc;
    if (lo)
        return pc - ViewPosition(float2(p - axis) + 0.5f, zl);
    return float3(0.0f, 0.0f, 0.0f);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= TraceSize()))
        return;
    if (all(id.xy == 0u))
        uState.Store4(GI_STATE_SKYSUM, uint4(0u, 0u, 0u, 0u)); // P2 sums this frame's sky colour into these words

    // Depth probe (design 3.2 P1): the host reads it 4 frames late and decides the convention with evidence. The
    // counts are reduced per wave first (one atomic per wave and word): a per-texel atomic on these few words cost
    // about 0.4 ms at 960x540 on the RX 9070 XT.
    {
        const float d = tDepth.Load(int3(TraceToRender(id.xy), 0));
        const uint total = WaveActiveCountBits(true);
        const uint zeros = WaveActiveCountBits(d == 0.0f);
        const uint ones = WaveActiveCountBits(d == 1.0f);
        const uint above = WaveActiveCountBits(d > 1.0f);
        const uint minBits = WaveActiveMin((d > 0.0f && d < 1.0f) ? asuint(d) : 0xFFFFFFFFu);
        const uint maxBits = WaveActiveMax(d > 0.0f ? asuint(d) : 0u);
        const bool inRange = d >= 0.0f && d <= 1.0f;
        const uint bin = inRange ? min(uint(d * 8.0f), 7u) : 8u;
        uint hist[8];
        [unroll] for (uint b = 0; b < 8; ++b)
            hist[b] = WaveActiveCountBits(bin == b);
        if (WaveIsFirstLane())
        {
            uState.InterlockedAdd(GI_STATE_PROBE + 4 * GI_PROBE_TOTAL, total);
            if (zeros)
                uState.InterlockedAdd(GI_STATE_PROBE + 4 * GI_PROBE_ZERO, zeros);
            if (ones)
                uState.InterlockedAdd(GI_STATE_PROBE + 4 * GI_PROBE_ONE, ones);
            if (above)
                uState.InterlockedAdd(GI_STATE_PROBE + 4 * GI_PROBE_ABOVE_ONE, above);
            if (minBits != 0xFFFFFFFFu)
                uState.InterlockedMin(GI_STATE_PROBE + 4 * GI_PROBE_MIN_BITS, minBits);
            if (maxBits != 0u)
                uState.InterlockedMax(GI_STATE_PROBE + 4 * GI_PROBE_MAX_BITS, maxBits);
            [unroll] for (uint b2 = 0; b2 < 8; ++b2)
                if (hist[b2])
                    uState.InterlockedAdd(GI_STATE_PROBE + 4 * (GI_PROBE_HIST + b2), hist[b2]);
        }
    }

    // Representative of the footprint: checkerboard min/max (sky counts as the farthest).
    uint k = 0;
    float z = ZAt(FootprintBase(id.xy));
    if (HasFootprint())
    {
        const bool nearest = ((id.x + id.y) & 1u) == 0u;
        [unroll] for (uint i = 1; i < 4; ++i)
        {
            const float zi = ZAt(int2(RepPixel(id.xy, i)));
            if (nearest ? zi < z : zi > z)
            {
                z = zi;
                k = i;
            }
        }
    }
    uZHalf[id.xy] = float2(z, float(k));

    float3 n = float3(0.0f, 0.0f, -1.0f);
    if (!IsSkyZ(z))
    {
        const int2 p = int2(RepPixel(id.xy, k));
        const int2 last = int2(RenderSize()) - 1;
        const float3 pc = ViewPosition(float2(p) + 0.5f, z);
        bool okX, okY;
        const float3 tx = Tangent(pc, z, p, int2(1, 0), p.x > 0, p.x < last.x, okX);
        const float3 ty = Tangent(pc, z, p, int2(0, 1), p.y > 0, p.y < last.y, okY);
        const float3 c = cross(tx, ty);
        const float l2 = dot(c, c);
        if (okX && okY && l2 > 1.0e-20f * dot(pc, pc) * dot(pc, pc))
        {
            n = c * rsqrt(l2);
            if (dot(n, pc) > 0.0f) // face the camera (the camera is at the origin, pc points away from it)
                n = -n;
        }
        else
            n = -normalize(pc);
    }
    uNHalf[id.xy] = EncodeNormal(n);
}
