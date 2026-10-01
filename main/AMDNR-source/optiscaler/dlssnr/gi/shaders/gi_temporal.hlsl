// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// P4 Temporal (trace grid; design 3.2 P4 and 2.1 item 6). Written for this module from the published ideas: history
// length per pixel and luminance moments (SVGF, Schied et al. 2017), a fast/slow history pair where the slow history
// is clamped to the fast one's band rather than to the raw noisy neighbourhood (after Karis 2014 / Salvi 2016), and
// an antilag rule.
//
//   - Reprojection with the game's MVs (design 3.1: the MV is added, prev = p + mv; no jitter term), bilinear over
//     the four previous texels, each kept only when its depth lies in the current 3x3 neighbourhood's depth range
//     (widened by a relative tolerance that grows with the MV length) and its normal is not turned away. Nothing valid
//     = disocclusion: the history starts over.
//   - Every history read is rescaled by the pre-exposure ratio (g.exposure.y); AO (alpha) is not.
//   - alpha = max(1 / len, 1 / max history); fast history alpha >= 1/4; antilag: when the slow history leaves the
//     fast band, the length drops to 4; reactive pixels keep at most g.temporal.w frames.
//   - No MVs or a history reset: this frame's trace only (spatial filtering does the rest).
//
// t0 trace  t1 histGI[prev]  t2 histFast[prev]  t3 histMom[prev]  t4 histLen[prev]
// t5 zHalf[cur]  t6 zHalf[prev]  t7 nHalf[cur]  t8 nHalf[prev]  t9 motion (MV grid, see g.motion)  t10 reactive
// u0 histGI[cur]  u1 histFast[cur]  u2 histMom[cur] (R16G16_FLOAT)  u3 histLen[cur] (R8_UINT)  u4 state
#include "gi_common.hlsli"

Texture2D<float4> tTrace : register(t0);
Texture2D<float4> tHistGI : register(t1);
Texture2D<float4> tHistFast : register(t2);
Texture2D<float2> tHistMom : register(t3);
Texture2D<uint> tHistLen : register(t4);
Texture2D<float2> tZHalf : register(t5);
Texture2D<float2> tZHalfPrev : register(t6);
Texture2D<float2> tNHalf : register(t7);
Texture2D<float2> tNHalfPrev : register(t8);
Texture2D<float2> tMotion : register(t9);
Texture2D<float> tReactive : register(t10);
RWTexture2D<float4> uHistGI : register(u0);
RWTexture2D<float4> uHistFast : register(u1);
RWTexture2D<float2> uHistMom : register(u2);
RWTexture2D<uint> uHistLen : register(u3);
RWByteAddressBuffer uState : register(u4);

void Write(uint2 t, float4 slow, float4 fast, float2 mom, uint len)
{
    uHistGI[t] = slow;
    uHistFast[t] = fast;
    uHistMom[t] = mom;
    uHistLen[t] = len;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= TraceSize()))
        return;
    float4 cur = tTrace.Load(int3(id.xy, 0));
    if (!AllFinite(cur))
        cur = float4(0.0f, 0.0f, 0.0f, 1.0f);
    const float lc = Luminance(cur.rgb);
    const float2 momCur = float2(lc, lc * lc);
    const float2 zs = tZHalf.Load(int3(id.xy, 0));
    if (IsSkyZ(zs.x) || HistoryReset() || !HasMotion())
    {
        Write(id.xy, cur, cur, momCur, 1u);
        return;
    }

    const float2 rep = RepCentre(id.xy, zs.y);
    const float2 mv = MotionAt(tMotion, rep);
    const float3 n = DecodeNormal(tNHalf.Load(int3(id.xy, 0)));
    const float mvTrace = length(mv * g.traceRatio.zw);
    const float tolerance = 0.04f + 0.08f * saturate(mvTrace / 8.0f);
    // Accepted depth range: the current 3x3 neighbourhood's (non-sky) min/max, widened by the tolerance. On a sloped
    // or grazing surface the sub-pixel jitter alone moves a texel's depth by more than a fixed tolerance; testing
    // against the neighbourhood's range keeps that history while a real disocclusion (a depth outside the whole
    // neighbourhood) still resets.
    float zMin = zs.x, zMax = zs.x;
    [unroll] for (int ny = -1; ny <= 1; ++ny)
    {
        [unroll] for (int nx = -1; nx <= 1; ++nx)
        {
            const int2 nq = clamp(int2(id.xy) + int2(nx, ny), int2(0, 0), int2(TraceSize()) - 1);
            const float zn = tZHalf.Load(int3(nq, 0)).x;
            if (!IsSkyZ(zn))
            {
                zMin = min(zMin, zn);
                zMax = max(zMax, zn);
            }
        }
    }
    zMin *= 1.0f - tolerance;
    zMax *= 1.0f + tolerance;

    // Bilinear over the previous trace grid, with a depth and normal test per tap. The displacement is applied to
    // the texel centre (not to the representative), so a still camera reads exactly its own texel back: resampling
    // from the representative's quarter-texel offset every frame would blur the history a little more each frame.
    const float2 tp = float2(id.xy) + mv * g.traceRatio.zw;
    const int2 b = int2(floor(tp));
    const float2 f = tp - float2(b);
    float4 slow = 0.0f, fast = 0.0f;
    float2 mom = 0.0f;
    float lenAcc = 0.0f, wSum = 0.0f;
    [unroll] for (uint k = 0; k < 4; ++k)
    {
        const int2 q = b + int2(k & 1u, k >> 1);
        if (any(q < 0) || any(q >= int2(TraceSize())))
            continue;
        const float bw = ((k & 1u) ? f.x : 1.0f - f.x) * ((k >> 1) ? f.y : 1.0f - f.y);
        const float zq = tZHalfPrev.Load(int3(q, 0)).x;
        if (IsSkyZ(zq) || zq < zMin || zq > zMax)
            continue;
        // Loose normal test (design 3.2 P4: view normals rotate with the camera): only a previous surface turned more
        // than about 90 degrees away is dropped. At a crease the depth-derived normal of a texel flips between the two
        // faces with the sub-pixel jitter, and the earlier cos > 0.5 test threw that texel's history away every few
        // frames (gilab WARP, still camera with jitter, High: static-pixel flicker CV Cornell 2.25 % -> 1.82 %,
        // furnace 3.4 % -> 2.4 %, s7 2.9 % -> 2.2 %, pillar 1.7 % -> 1.45 %; moving camera 0.28 % -> 0.25 %).
        const float nd = dot(DecodeNormal(tNHalfPrev.Load(int3(q, 0))), n);
        const float w = bw * saturate(nd * 4.0f + 1.0f);
        if (w <= 1.0e-4f)
            continue;
        const float4 hs = tHistGI.Load(int3(q, 0));
        const float4 hf = tHistFast.Load(int3(q, 0));
        const float2 hm = tHistMom.Load(int3(q, 0));
        if (!AllFinite(hs) || !AllFinite(hf) || !AllFinite(float4(hm, 0.0f, 0.0f)))
            continue;
        slow += w * hs;
        fast += w * hf;
        mom += w * hm;
        lenAcc += w * float(tHistLen.Load(int3(q, 0)));
        wSum += w;
    }
    if (wSum < 0.05f)
    {
        Write(id.xy, cur, cur, momCur, 1u); // disocclusion
        return;
    }
    const float inv = 1.0f / wSum;
    const float ex = g.exposure.y;
    slow *= inv;
    fast *= inv;
    mom *= inv;
    slow.rgb *= ex;
    fast.rgb *= ex;
    mom *= float2(ex, ex * ex);

    const float maxLen = max(g.temporal.x, 1.0f);
    float len = min(floor(lenAcc * inv + 0.5f) + 1.0f, maxLen);
    if (HasReactive())
    {
        const float r = saturate(tReactive.Load(int3(RepPixel(id.xy, uint(zs.y + 0.5f)), 0)));
        if (r >= 0.5f)
            len = min(len, max(g.temporal.w, 1.0f));
    }

    // Fast history (about 4 frames) and the band the slow history must stay in.
    const float4 fastNew = lerp(fast, cur, max(1.0f / len, 0.25f));
    const float2 momNew = lerp(mom, momCur, max(1.0f / len, g.temporal.y));
    const float sigma = sqrt(max(momNew.y - momNew.x * momNew.x, 0.0f));
    const float lFast = Luminance(fastNew.rgb);
    const float band = g.temporal.z * sigma + 0.1f * lFast + 1.0e-4f;
    // Antilag: the slow history is far from the fast one (a light turned on, an MV-less mover): shorten it.
    if (abs(Luminance(slow.rgb) - lFast) > band || abs(slow.a - fastNew.a) > 0.2f)
        len = min(len, 4.0f);
    slow.rgb = clamp(slow.rgb, fastNew.rgb - band, fastNew.rgb + band);
    slow.a = clamp(slow.a, fastNew.a - 0.25f, fastNew.a + 0.25f);

    const float alpha = max(1.0f / len, g.temporal.y);
    const float4 slowNew = lerp(slow, cur, alpha);
    Write(id.xy, max(slowNew, 0.0f), max(fastNew, 0.0f), momNew, uint(len));
}
