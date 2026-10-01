// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// P7 Debug (render grid), only when [AmdGi] DebugView > 0 (b1 a.x = view, GI_DEBUG_*). Replaces P6's output with
// one of GI's buffers, point-sampled from the trace grid (design 3.2 P7):
//   1 GI only (the denoised E / pi, linear)      2 AO only (denoised visibility)
//   3 normals (view space, 0.5 n + 0.5)          4 history length (0 .. max history -> black .. white)
//   5 disocclusion (red where the history restarted this frame, over the dimmed image)
//   6 depth (bands of log2 view depth)          7 radiance (the raw trace output of this frame, before denoise;
//                                                  overwritten by the second a-trous iteration at High/Ultra)
//   10 AO lit protection (orange where AO is spared on a pixel much brighter than its neighbourhood, full-res,
//      the composite's own formula; the dimmed image elsewhere and on sky pixels)
//   others (8 thickness, 9 translucency: drawn once those exist, M3): the game's image, unchanged.
// Sky pixels show black in every buffer view.
//
// t0 gi (final)  t1 zHalf[cur]  t2 nHalf[cur]  t3 histLen[cur]  t4 colour  t5 depth  t6 trace (raw P3 output)
// t7 motion  t8 radMip (all levels)
// u0 out
#include "gi_common.hlsli"

Texture2D<float4> tGi : register(t0);
Texture2D<float2> tZHalf : register(t1);
Texture2D<float2> tNHalf : register(t2);
Texture2D<uint> tHistLen : register(t3);
Texture2D<float4> tColour : register(t4);
Texture2D<float> tDepth : register(t5);
Texture2D<float4> tTrace : register(t6);
Texture2D<float2> tMotion : register(t7);
Texture2D<float4> tRadMip : register(t8);
RWTexture2D<float4> uOut : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= RenderSize()))
        return;
    const uint2 t = min(uint2((float2(id.xy) + 0.5f) * g.traceRatio.zw), TraceSize() - 1);
    const float4 raw = tColour.Load(int3(id.xy, 0));
    const bool sky = IsSkyZ(tZHalf.Load(int3(t, 0)).x);
    if (p.a.x < GI_DEBUG_GI || (p.a.x > GI_DEBUG_RADIANCE && p.a.x != GI_DEBUG_LITPROTECT))
    {
        uOut[id.xy] = raw; // views not drawn in this preview: the game's image
        return;
    }
    float3 v = 0.0f;
    switch (p.a.x)
    {
    case GI_DEBUG_GI: v = tGi.Load(int3(t, 0)).rgb; break;
    case GI_DEBUG_AO: v = tGi.Load(int3(t, 0)).aaa; break;
    case GI_DEBUG_NORMALS: v = DecodeNormal(tNHalf.Load(int3(t, 0))) * 0.5f + 0.5f; break;
    case GI_DEBUG_HISTORY: v = saturate(float(tHistLen.Load(int3(t, 0))) / max(g.temporal.x, 1.0f)).xxx; break;
    case GI_DEBUG_DISOCCLUSION:
        v = tHistLen.Load(int3(t, 0)) <= 1u ? float3(1.0f, 0.0f, 0.0f)
                                             : 0.25f * saturate(DecodeColour(raw.rgb));
        break;
    case GI_DEBUG_DEPTH: v = frac(log2(max(tZHalf.Load(int3(t, 0)).x, 1.0e-6f))).xxx; break;
    case GI_DEBUG_RADIANCE: v = tTrace.Load(int3(t, 0)).rgb; break;
    case GI_DEBUG_LITPROTECT:
    {
        // Per full-res pixel, like P6: sky pixels (P6 passes them through) show the dimmed image only.
        const float3 c = max(DecodeColour(raw.rgb), 0.0f);
        const float2 tracePx = (float2(id.xy) + 0.5f) * g.traceRatio.zw;
        const bool skyPx = IsSkyZ(ViewZ(tDepth.Load(int3(id.xy, 0))));
        const float protect = skyPx ? 0.0f : LitProtect(c, Luminance(LocalStats(tRadMip, tracePx).rgb));
        v = lerp(0.25f * saturate(c), float3(1.0f, 0.5f, 0.0f), saturate(protect));
        break;
    }
    default: break;
    }
    if (sky && p.a.x != GI_DEBUG_LITPROTECT)
        v = 0.0f;
    if (!AllFinite3(v))
        v = float3(1.0f, 0.0f, 1.0f); // magenta: a non-finite value in the buffer
    if (g.colourInfo.x != GI_ENC_LINEAR)
        v = EncodeColour(v);
    if (g.colourInfo.z != 0)
        v = saturate(v);
    uOut[id.xy] = float4(v, raw.a);
}
