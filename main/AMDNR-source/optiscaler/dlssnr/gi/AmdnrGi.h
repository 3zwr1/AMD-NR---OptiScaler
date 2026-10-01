// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// AMDNR Screen GI: the D3D12 compute module (design: the AMDNR SSGI design note, sections 3-5).
//
// Effect is self-contained: it knows nothing about OptiScaler's Config, State or NGX parameter blocks, so the
// offline lab (dlssnr/gi/lab) links the same source. GiSeam.cpp is the adapter that reads the ini and the parameter
// block and calls Record.
//
// Contract of Record:
//   - Returns the composited colour, a texture of the game colour's extent, left in
//     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; or nullptr, and then nothing at all was recorded (pass-through:
//     the caller leaves the game's colour untouched).
//   - Every game input is read in the state the caller names in Inputs and is left in that state.
//   - It binds its own descriptor heap and root signature; the caller restores the game's compute state
//     (DlssNr_Dx12.cpp wraps the call in ScopedNrStateEnvelope).
//   - Nothing is allocated before the first Record, and nothing at all while the seam does not call it
//     ([AmdGi] Enabled=false).
//   - Any failure (a throw, a failed allocation or PSO) turns the effect off for the session; Status() says why.
#pragma once

#include <d3d12.h>
#include <cstdint>
#include <memory>
#include <string>

namespace AmdnrGi
{
// Log sink: level 0 info, 1 warning, 2 error. The seam routes it to OptiScaler.log; the lab to stdout.
using LogSink = void (*)(int level, const char* text);
void SetLogSink(LogSink sink);

// [AmdGi] values, already clamped to their ranges by the seam (design 6.1). Defaults are the ini defaults.
struct Settings
{
    int quality = 2;          // 0 Low, 1 Medium, 2 High, 3 Ultra (4 Auto is resolved by the seam)
    float intensity = 1.0f;   // Bounce light 0..3
    float occlusion = 1.0f;   // Ambient occlusion 0..2
    float radius = 1.0f;      // multiplier on the tier's screen radius, 0.25..4
    float thickness = 0.0f;   // 0 = auto per tier, else 0.01..1 (fraction of view depth)
    float saturation = 1.0f;  // Bounce colour 0..2
    float sky = 0.0f;         // Sky light 0..2
    float feedback = 0.5f;    // Multi-bounce 0..0.8 (the effective value is clamped, 2.1 item 9)
    float nearFade = -1.0f;   // -1 auto, 0 off, > 0 value
    float distanceFade = -1.0f;
    int encoding = -1;        // -1 auto (from the format and IsHDR), 0 linear, 1 sRGB, 2 gamma 2.2
    int debugView = 0;        // GI_DEBUG_*
    // hidden experiment keys (ini only, not saved)
    int depthConvention = -1; // -1 auto (flag + probe), 0 standard, 1 reversed, 2 linear
    float traceCap = 0.0f;    // Mpx of the trace grid, 0 = tier
    int albedoMode = 0;       // 0..2, the estimators of 3.2 P6
    float aoLitProtect = 1.0f;// 0..1
    int translucency = 0;     // 0 off, 1 heuristic
};

// Camera, resolved by the seam from the chain of design 5.2.
struct Camera
{
    float fovY = 1.2217305f;  // radians (70 degrees)
    int source = 0;           // GI_CAM_*
    float nearZ = 0.0f;       // 0 = unknown
    float farZ = 0.0f;        // 0 = unknown or infinite
    bool hasViewToClip = false;
    float viewToClip[16] = {}; // row-major, Streamline convention
    bool hasClipToPrevClip = false;
    float clipToPrevClip[16] = {};
};

// One frame's inputs at the pre-upscale point. Render subrect origin is 0,0 (the seam refuses other origins).
struct Inputs
{
    ID3D12Resource* colour = nullptr;   // required
    ID3D12Resource* depth = nullptr;    // required
    ID3D12Resource* motion = nullptr;   // optional: without it the history is off (spatial only)
    ID3D12Resource* reactive = nullptr; // optional
    ID3D12Resource* exposure = nullptr; // optional (1x1)
    D3D12_RESOURCE_STATES colourState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES depthState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES motionState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES reactiveState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES exposureState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    uint32_t renderWidth = 0, renderHeight = 0; // active subrect; 0 = the colour texture's extent
    uint32_t motionWidth = 0, motionHeight = 0; // extent the MVs cover; 0 = render size
    float mvScaleX = 1.0f, mvScaleY = 1.0f;
    bool mvJittered = false, mvLowRes = true;
    float jitterX = 0.0f, jitterY = 0.0f;
    float preExposure = 0.0f;   // 0 = not sent
    float exposureScale = 0.0f; // 0 = not sent
    bool depthInverted = false;
    bool isHdr = false;
    bool reset = false;         // NGX reset
    Camera camera;
    ID3D12CommandQueue* timingQueue = nullptr; // optional: timestamp frequency for the ms readout
};

// Menu / log readout. Plain values, copied out under a lock.
struct Stats
{
    bool ready = false;          // pipelines built
    bool failed = false;         // turned off for the session
    bool ranLastFrame = false;
    uint32_t traceWidth = 0, traceHeight = 0;
    uint32_t renderWidth = 0, renderHeight = 0;
    float gpuMs = -1.0f;         // -1 = no measurement
    int depthConvention = -1;    // GI_DEPTH_* in use, -1 before the first frame
    int depthSource = 0;         // 0 flag, 1 probe, 2 ini
    int cameraSource = 0;        // GI_CAM_*
    float fovYDegrees = 0.0f;
    int quality = 2;
    uint64_t frames = 0;         // Records that ran the passes
    uint64_t historyResets = 0;
    uint32_t probe[16] = {};     // last probe read back (GI_PROBE_*)
    std::string status;          // one line, the reason when it passes through
};

class Effect
{
  public:
    // Creates no GPU resource. The pipelines are built on a worker thread, started by the first Record.
    static std::unique_ptr<Effect> Create(ID3D12Device* device);
    ~Effect();
    Effect(const Effect&) = delete;
    Effect& operator=(const Effect&) = delete;

    ID3D12Resource* Record(ID3D12GraphicsCommandList* cmdList, const Inputs& in, const Settings& s);

    void ResetHistory();
    Stats GetStats() const;
    bool Failed() const;
    ID3D12Device* Device() const;

    struct Impl;

  private:
    explicit Effect(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

// Tier table (design 6.3). Exposed for the lab and the menu readout.
struct TierInfo
{
    float capMpx;       // trace-grid pixel cap
    int minRatio;       // render px per trace px, at least
    int slices;         // slices per pixel and frame
    int stepsPerSide;
    bool sampleNormals;
    int maxHistory;
    int atrousIterations;
    float radiusFraction; // max screen radius as a fraction of the render height
    float thicknessAuto;  // fraction of view z
};
const TierInfo& Tier(int quality);
// The trace grid for a render size (pixel cap per tier, or traceCap Mpx when > 0).
void TraceSize(uint32_t renderW, uint32_t renderH, int quality, float traceCapMpx, uint32_t& traceW, uint32_t& traceH);
const char* QualityName(int quality);
const char* DepthName(int convention);
} // namespace AmdnrGi
