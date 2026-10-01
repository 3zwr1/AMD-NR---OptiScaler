// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// P3 Trace (trace grid; design 3.2 P3 and 2.1 items 1-2). Visibility-bitmask horizon tracing (the idea of Therrien,
// Levesque and Gilet 2023) on GTAO's slice parametrisation (Jimenez et al. 2016), written for this module.
//
// Per texel and slice: a screen direction (interleaved gradient noise + the frame's R2 offset rotate it), the slice
// plane through the camera, the view vector V and that direction. Directions in the slice are angles theta from V
// (positive toward the slice direction). 32 sectors cover the whole part of the slice in front of the surface,
// [gamma - pi/2, gamma + pi/2] (gamma = angle of the projected normal), including the directions that point away from
// the camera (theta beyond pi/2: a floor seen at a grazing angle finds the wall behind it there). The sectors sit at
// equal steps of the cosine-weighted slice measure w(theta) = cos(theta - gamma) |sin theta| (the projected-normal
// kernel with its Jacobian), so a sector count is a cosine-weighted share directly.
//
// Each depth sample occludes the sectors between its front point and its back point (the front pushed away from the
// camera by the thickness, a fraction of view depth). Only sectors not occluded by nearer samples add the sample's
// radiance (bounce passes behind thin objects). AO = open share, bounce = sum over newly occluded sectors of L / 32.
// Under uniform radiance L the two add up to exactly L (open x L_open + occluded x L), the furnace property.
//
// Output: rgb = cosine-weighted mean incident radiance from the occluders (E / pi of the bounce), a = visibility.
// Every loop has a fixed maximum (GI_MAX_SLICES, GI_MAX_STEPS) whatever the constants say.
//
// t0 zHalf[cur]  t1 nHalf[cur]  t2 zMip (all levels)  t3 radMip (all levels)
// u0 trace (R16G16B16A16_FLOAT)  u1 state
#include "gi_common.hlsli"

Texture2D<float2> tZHalf : register(t0);
Texture2D<float2> tNHalf : register(t1);
Texture2D<float2> tZMip : register(t2);
Texture2D<float4> tRadMip : register(t3);
RWTexture2D<float4> uTrace : register(u0);
RWByteAddressBuffer uState : register(u1);

// Antiderivative of cos(t - gamma) sin t: 1/4 (2 t sin gamma - cos(2t - gamma)).
float SliceG(float t, float gamma, float sinGamma)
{
    return 0.25f * (2.0f * t * sinGamma - cos(2.0f * t - gamma));
}

struct SliceMeasure
{
    float gamma, sinGamma, lo, hi, gLo, g0, invW;
};

SliceMeasure MakeMeasure(float gamma)
{
    SliceMeasure m;
    m.gamma = gamma;
    m.sinGamma = sin(gamma);
    // The whole front hemisphere of the slice (verify pass, 2026-09-26: the earlier [-pi/2, pi/2] cap dropped the
    // away-from-camera half of it, about half the cosine weight of a grazing floor; gilab High, WARP: Cornell bleed
    // red/green 0.094/0.069 -> 0.127/0.090, AO contrast 0.345 -> 0.404, furnace closure 0.997 -> 0.9999, s7 bounce
    // 0.033 -> 0.053 of the reference, no halo or flicker change). |sin t| keeps SliceCdf's split at 0 valid on
    // [-pi, pi], and cos(t - gamma) >= 0 on this range.
    m.lo = gamma - kGiHalfPi;
    m.hi = gamma + kGiHalfPi;
    m.gLo = SliceG(m.lo, gamma, m.sinGamma);
    m.g0 = SliceG(0.0f, gamma, m.sinGamma);
    const float w = m.gLo - 2.0f * m.g0 + SliceG(m.hi, gamma, m.sinGamma);
    m.invW = w > 1.0e-5f ? 1.0f / w : 0.0f;
    return m;
}

// Cosine-weighted share of [lo, theta] in the slice: |sin t| splits the integral at t = 0.
float SliceCdf(float theta, SliceMeasure m)
{
    const float t = clamp(theta, m.lo, m.hi);
    const float gt = SliceG(t, m.gamma, m.sinGamma);
    const float i = t <= 0.0f ? m.gLo - gt : m.gLo - 2.0f * m.g0 + gt;
    return saturate(i * m.invW);
}

// Sectors [round(32 f0), round(32 f1)) of the 32-bit mask. Rounding both ends is unbiased on average (an occluder
// narrower than half a sector does not count this frame, a wider one may count a whole sector; the per-frame noise
// moves the boundaries).
uint SectorBits(float f0, float f1)
{
    const uint a = uint(saturate(min(f0, f1)) * 32.0f + 0.5f);
    const uint b = uint(saturate(max(f0, f1)) * 32.0f + 0.5f);
    if (b <= a)
        return 0u;
    const uint n = b - a;
    return (n >= 32u ? 0xFFFFFFFFu : ((1u << n) - 1u)) << a;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= TraceSize()))
        return;
    const float2 zs = tZHalf.Load(int3(id.xy, 0));
    if (IsSkyZ(zs.x))
    {
        uTrace[id.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    const float3 P = ViewPosition(RepCentre(id.xy, zs.y), zs.x);
    const float3 V = -normalize(P);
    const float3 N = DecodeNormal(tNHalf.Load(int3(id.xy, 0)));
    const float thickness = max(g.trace.z, 0.001f);
    const float radius = max(g.trace.x, 2.0f);
    const uint slices = clamp(uint(g.tier.y), 1u, GI_MAX_SLICES);
    const uint steps = clamp(uint(g.tier.z), 1u, GI_MAX_STEPS);
    const bool sampleNormals = g.tier.w != 0;
    const float2 origin = float2(id.xy) + 0.5f;
    const float2 traceMax = float2(TraceSize());

    // Firefly clamp against the local mean radiance (the coarsest level around this texel; design 3.2 P2).
    const uint topMip = min(uint(g.trace.w), GI_PYRAMID_LEVELS - 1u);
    const float localMean = Luminance(tRadMip.SampleLevel(sLinear, MipUv(origin, topMip), topMip).rgb);
    const float fireflyCap = 10.0f * max(localMean, 1.0e-4f);

    const float noiseA = frac(Ign(float2(id.xy)) + g.noise.x);
    const float noiseS = frac(Ign(float2(id.xy) + float2(47.0f, 17.0f)) + g.noise.y);

    float3 bounceSum = 0.0f;
    float visSum = 0.0f, weightSum = 0.0f;

    [loop] for (uint s = 0; s < GI_MAX_SLICES; ++s)
    {
        if (s >= slices)
            break;
        const float phi = (float(s) + noiseA) * (kGiPi / float(slices));
        const float2 dir = float2(cos(phi), sin(phi)); // trace px, screen y down
        const float3 d = float3(dir.x, -dir.y, 0.0f);   // the same direction in view space (y up)
        const float3 O = normalize(d - dot(d, V) * V);  // in the slice, perpendicular to V, toward +dir

        const float nv = dot(N, V), no = dot(N, O);
        const float projLen = sqrt(nv * nv + no * no);
        const SliceMeasure m = MakeMeasure(clamp(atan2(no, nv), -kGiHalfPi, kGiHalfPi));
        if (m.invW <= 0.0f || projLen < 1.0e-2f)
            continue;
        const float sliceWeight = projLen / m.invW; // |Np| x W
        // Samples sit up to a texel off the slice plane (texel snapping, the representative offsets). Dropping the
        // off-plane part would lift points of this very surface above its tangent line, so a flat floor would occlude
        // itself. Instead slide each point into the slice plane along the tangent plane: its height above the
        // surface (N . delta) is kept, and a point of the surface lands exactly on the horizon of the slice.
        // The slide is exact (not clamped): only the height above the tangent line depends on it, which is
        // N . delta / projLen, and projLen >= 1e-2 here. A clamp (it was +-16) left grazing surfaces a residual
        // height on slices nearly parallel to them, so a floor or box top seen at a grazing angle occluded itself on
        // every other texel of the checkerboard representatives (a static 2-3 % AO/GI checkerboard; gilab Cornell box
        // top AO 0.97/1.00 alternating -> 1.00 everywhere).
        const float3 A = cross(V, O); // slice-plane normal
        const float slide = dot(N, A) / (projLen * projLen);

        uint mask = 0u;
        float3 bounce = 0.0f;
        [loop] for (uint side = 0; side < 2; ++side)
        {
            const float sgn = side == 0 ? 1.0f : -1.0f;
            [loop] for (uint i = 0; i < GI_MAX_STEPS; ++i)
            {
                if (i >= steps || mask == 0xFFFFFFFFu)
                    break;
                const float u = (float(i) + noiseS) / float(steps);
                const float dist = 1.0f + (radius - 1.0f) * u * u; // quadratic spacing, trace px
                const float2 tp = origin + sgn * dir * dist;
                if (any(tp < 0.0f) || any(tp >= traceMax))
                    break; // off screen: the rest of this side stays open
                const uint2 q = uint2(tp);
                if (all(q == id.xy))
                    continue;
                const float2 zq = tZHalf.Load(int3(q, 0));
                if (IsSkyZ(zq.x))
                    continue;
                const float3 S = ViewPosition(RepCentre(q, zq.y), zq.x);
                const float3 dF = S - P;
                // On or below this texel's tangent plane (the common case on flat ground), and so is the back point
                // (it moves along the camera ray, away from a camera-facing plane): it occludes nothing. Tested
                // explicitly because the slide below can turn such a point's in-slice coordinate negative, and the
                // clamp to [0, pi] would then place it straight toward the camera (spurious sectors).
                const bool below = dot(N, dF) <= 0.0f && dot(N, S) < 0.0f;
                const float3 dB = S * (1.0f + thickness) - P;
                // Angles from V toward this side (after the slide into the slice plane), clamped to [0, pi] so a
                // sample can never wrap to the other side.
                const float kF = dot(dF, A) * slide, kB = dot(dB, A) * slide;
                const float hF = clamp(atan2(sgn * (dot(dF, O) + kF * no), dot(dF, V) + kF * nv), 0.0f, kGiPi);
                const float hB = clamp(atan2(sgn * (dot(dB, O) + kB * no), dot(dB, V) + kB * nv), 0.0f, kGiPi);
                const uint bits = below ? 0u : SectorBits(SliceCdf(sgn * hF, m), SliceCdf(sgn * hB, m));
                const uint fresh = bits & ~mask;
                if (fresh != 0u)
                {
                    const uint lvl = min(uint(max(log2(dist) - 2.0f, 0.0f)), topMip);
                    float3 L = tRadMip.SampleLevel(sLinear, MipUv(tp, lvl), lvl).rgb;
                    const float lL = Luminance(L);
                    if (lL > fireflyCap)
                        L *= fireflyCap / lL;
                    float emit = 1.0f;
                    if (sampleNormals)
                    {
                        // Radiance is invariant along the ray, so no emitter cosine; a surface that faces away
                        // from this texel shows us its back, whose light the screen does not have.
                        const float3 Nq = DecodeNormal(tNHalf.Load(int3(q, 0)));
                        const float c = dot(Nq, -dF) * rsqrt(max(dot(dF, dF), 1.0e-20f));
                        emit = saturate(c * 10.0f + 1.0f); // front (c >= 0): 1; clearly back (c <= -0.1): 0
                    }
                    bounce += float(countbits(fresh)) * emit * L;
                }
                mask |= bits;
            }
        }
        const float open = 1.0f - float(countbits(mask)) / 32.0f;
        bounceSum += sliceWeight * bounce / 32.0f;
        visSum += sliceWeight * open;
        weightSum += sliceWeight;
    }

    float4 result = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (weightSum > 0.0f)
        result = float4(bounceSum / weightSum, visSum / weightSum);
    uTrace[id.xy] = AllFinite(result) ? result : float4(0.0f, 0.0f, 0.0f, 1.0f);
}
