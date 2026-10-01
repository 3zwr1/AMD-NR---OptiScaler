// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// AMDNR Screen GI: the constant-buffer layouts shared by the C++ host (AmdnrGi.cpp) and the HLSL passes
// (shaders/gi_common.hlsli includes this file). Every member is a 16-byte vector, so the C++ struct and the HLSL
// cbuffer have the same layout without packing rules; AmdnrGi.cpp static_asserts the size.
//
// Change a field here, and both sides change together. Rebuild the shaders (shaders/build_gi_shaders.py) after any
// edit, or the precompiled headers read the old layout.
#ifndef AMDNR_GI_SHARED_H
#define AMDNR_GI_SHARED_H

#ifdef __cplusplus
#include <cstdint>
namespace AmdnrGi::Hlsl
{
struct float4 { float x, y, z, w; };
struct uint4 { uint32_t x, y, z, w; };
struct int4 { int32_t x, y, z, w; };
} // namespace AmdnrGi::Hlsl
#define GI_STRUCT_BEGIN(name)                                                                                         \
    namespace AmdnrGi                                                                                                 \
    {                                                                                                                 \
    using namespace AmdnrGi::Hlsl;                                                                                    \
    struct name                                                                                                       \
    {
#define GI_STRUCT_END                                                                                                 \
    }                                                                                                                 \
    ;                                                                                                                 \
    }
#else
#define GI_STRUCT_BEGIN(name)                                                                                         \
    struct name                                                                                                       \
    {
#define GI_STRUCT_END                                                                                                 \
    }                                                                                                                 \
    ;
#endif

// Depth conventions (depthInfo.x)
#define GI_DEPTH_STANDARD 0 // near 0, far 1: view z ~ 1 / max(1 - d, eps)
#define GI_DEPTH_REVERSED 1 // near 1, far 0 (infinite or very far plane): view z ~ 1 / max(d, eps)
#define GI_DEPTH_LINEAR 2   // the title stores view depth itself: z = d * depthParams.y

// Colour encodings (colourInfo.x)
#define GI_ENC_LINEAR 0
#define GI_ENC_SRGB 1
#define GI_ENC_GAMMA22 2

// Camera sources (cameraInfo.z), for the readout
#define GI_CAM_DEFAULT 0   // 70 degrees vertical, nothing better known
#define GI_CAM_FSR 1       // FSR.cameraFovAngleVertical in the parameter block
#define GI_CAM_SL 2        // Streamline sl::Constants
#define GI_CAM_DLSSG 3     // DLSS-FG camera
#define GI_CAM_MANUAL 4    // [AmdGi] Fov
#define GI_CAM_FSR_INI 5   // [FSR] VerticalFov / HorizontalFov set in the ini

// Debug views (frame.w), 3.2 P7
#define GI_DEBUG_OFF 0
#define GI_DEBUG_GI 1
#define GI_DEBUG_AO 2
#define GI_DEBUG_NORMALS 3
#define GI_DEBUG_HISTORY 4
#define GI_DEBUG_DISOCCLUSION 5
#define GI_DEBUG_DEPTH 6
#define GI_DEBUG_RADIANCE 7
#define GI_DEBUG_THICKNESS 8
#define GI_DEBUG_TRANSLUCENCY 9
#define GI_DEBUG_LITPROTECT 10

// The state buffer (RWByteAddressBuffer, GI_STATE_BYTES). Byte offsets.
#define GI_STATE_BYTES 4096
#define GI_STATE_PROBE 0        // 16 uints, cleared by the host before P1 every frame (ClearUnorderedAccessViewUint)
#define GI_PROBE_ZERO 0         //   uint: pixels with d == 0 exactly
#define GI_PROBE_ONE 1          //   uint: pixels with d == 1 exactly
#define GI_PROBE_ABOVE_ONE 2    //   uint: pixels with d > 1 (linear depth)
#define GI_PROBE_TOTAL 3        //   uint: pixels probed
#define GI_PROBE_MIN_BITS 4     //   uint: asuint(min d) of d in (0, 1) (InterlockedMin on the bits of a positive float)
#define GI_PROBE_MAX_BITS 5     //   uint: asuint(max d) (InterlockedMax); cleared to 0
#define GI_PROBE_HIST 6         //   8 uints: histogram of d in [0, 1], 8 even bins
#define GI_PROBE_WORDS 16
#define GI_STATE_STATS 64       // 48 uints for the passes (local means, frame mean for the readout); kept across frames
#define GI_STATE_SKY 256        // 16 floats: L1 SH RGB (12) + weight + seconds since the last far pixel + 2 spare
#define GI_STATE_SKYSUM 320     // 4 uints: this frame's sky colour sums (fixed point x GI_SKY_FIXED) r, g, b and the
                                //   sky texel count; zeroed by P1, summed by P2 level 0, read by P6 (the Sky term)
#define GI_SKY_FIXED 64.0       // fixed-point scale of the sky sums (a texel adds at most 64 x 64 per channel)
#define GI_STATE_FREE 336       // up to GI_STATE_BYTES: free
#define GI_READBACK_BYTES 256   // bytes 0..255 are copied to the CPU every frame (read 3 frames late)

// Pyramid levels (zMip and radMip, level 0 = trace grid)
#define GI_PYRAMID_LEVELS 6

GI_STRUCT_BEGIN(GiFrameConstants)
    uint4 renderSize;    // x,y active render subrect (origin 0,0), in colour pixels; z,w active trace grid
    float4 invSizes;     // 1/render.x, 1/render.y, 1/trace.x, 1/trace.y
    uint4 allocSize;     // x,y colour texture extent (= out); z,w trace-grid allocation (>= trace grid)
    float4 traceRatio;   // x,y render px per trace px; z,w trace px per render px
    float4 camera;       // x tan(fovX/2), y tan(fovY/2), z near (0 = unknown), w far (0 = unknown or infinite)
    float4 cameraInfo;   // x fovY (radians), y aspect (render w / h), z camera source (GI_CAM_*), w 1 = near/far known
    int4 depthInfo;      // x convention (GI_DEPTH_*), y the NGX DepthInverted flag, z verdict source (0 flag, 1 probe, 2 ini), w 0
    float4 depthParams;  // x eps for the 1/d linearisation, y scale for GI_DEPTH_LINEAR (1), z,w 0
    float4 motion;       // x,y NGX MV_Scale_X/Y; z,w MV texels per render px (motion extent / render size)
    int4 motionInfo;     // x has MVs, y MVJittered create flag, z MVs at render res (MVLowRes), w 0
    float4 jitter;       // x,y this frame's NGX jitter (render px, NGX sign); z,w last frame's
    float4 exposure;     // x pre-exposure (1 when not sent), y history rescale preExp_cur / preExp_prev,
                         // z exposure scale (1 when not sent), w 1 = exposure texture bound
    int4 colourInfo;     // x encoding (GI_ENC_*), y IsHDR create flag, z out is UNORM (clamp to [0,1]), w reactive bound
    uint4 frame;         // x frame index, y history reset (1 = ignore every *prev* resource and bounceHalf),
                         // z random seed, w debug view (GI_DEBUG_*)
    float4 noise;        // x,y R2 offset for this frame in [0,1); z,w 0
    int4 tier;           // x quality 0..3, y slices per pixel, z steps per side, w sample normals (0/1)
    float4 trace;        // x max screen radius in trace px, y Radius dial, z thickness (fraction of view z), w max mip
    float4 temporal;     // x max history (frames), y minimum blend alpha, z antilag k, w history cap for reactive px
    int4 spatial;        // x a-trous iterations (0..2), y AlbedoMode, z Translucency mode, w 0
    float4 composite0;   // x Intensity (bounce), y Occlusion, z Saturation, w Sky
    float4 composite1;   // x effective feedback, y NearFade, z DistanceFade (-1 auto, 0 off, > 0 value), w AoLitProtect
    float4 viewToClip[4];     // rows (row-major, Streamline convention); valid when matrixInfo.x
    float4 clipToPrevClip[4]; // rows; valid when matrixInfo.y
    int4 matrixInfo;     // x viewToClip valid, y clipToPrevClip valid, z,w 0
GI_STRUCT_END

// b1: eight root constants per dispatch. Meaning per pass (see AmdnrGi.cpp, the pass table):
//   P2 pyramid: a.x = level (0..GI_PYRAMID_LEVELS-1)
//   P5 spatial: a.x = iteration, a.y = step width in trace px (1, 2)
//   P7 debug:   a.x = view (GI_DEBUG_*)
GI_STRUCT_BEGIN(GiPassConstants)
    uint4 a;
    uint4 b;
GI_STRUCT_END

#endif // AMDNR_GI_SHARED_H
