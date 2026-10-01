// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// AMDNR Screen GI: the seam (see GiSeam.h). Everything OptiScaler-specific lives here; AmdnrGi.cpp stays free of
// Config, State and NGX so the lab links it unchanged.

#include "pch.h"

#include "GiSeam.h"
#include "GiShared.h"

#include <Config.h>
#include <State.h>
#include <OptiTypes.h>
#include <dlssnr/amd/AmdBridge.h>
#include <hooks/Streamline_Hooks.h>

#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>

namespace AmdnrGi::Seam
{
namespace
{
constexpr float kDegToRad = 0.017453292519943295f;
constexpr float kRadToDeg = 57.29577951308232f;

std::mutex g_mutex; // one Record at a time (EvaluateInternal can run on more than one thread)
// Never destroyed: its D3D12 objects must not be released from DLL_PROCESS_DETACH (the process is ending anyway).
Effect* g_effect = nullptr;
ID3D12Device* g_device = nullptr; // the first device GI ran on (the AmdDeviceGate rule: others pass through)
// The game's device vendor, checked once per device (GpuSupportInfo is the primary adapter until NR's backend is
// built, which on a hybrid laptop can be the Ryzen iGPU while the game runs on another vendor's GPU).
ID3D12Device* g_vendorDevice = nullptr;
bool g_vendorAmd = false;
std::atomic<bool> g_chainWired { false };
std::atomic<bool> g_resetRequested { false };

// GI's replacement in this thread's parameter block (the upscaler evaluates and restores on the caller's thread).
thread_local NVSDK_NGX_Parameter* t_params = nullptr;
thread_local ID3D12Resource* t_original = nullptr;
thread_local ID3D12Resource* t_replacement = nullptr;

std::mutex g_statusMutex;
std::string g_seamStatus; // a refusal before the effect ran; empty when the effect's own status applies

// One stream per presented frame (the AmdOneStreamPerFrame idea): GI runs on the largest stream, others pass through.
unsigned long long g_runEpoch = ~0ull;
uint64_t g_runArea = 0;
uint64_t g_mainArea = 0;
unsigned long long g_mainEpoch = 0;

void SinkToLog(int level, const char* text)
{
    if (level >= 2)
        LOG_ERROR("{}", text);
    else if (level == 1)
        LOG_WARN("{}", text);
    else
        LOG_INFO("{}", text);
}

void SetSeamStatus(const std::string& s)
{
    std::lock_guard<std::mutex> lock(g_statusMutex);
    if (s != g_seamStatus && !s.empty())
        LOG_INFO("AMDNR Screen GI: {}", s);
    g_seamStatus = s;
}

ID3D12Resource* Resource(NVSDK_NGX_Parameter* p, const char* name)
{
    ID3D12Resource* r = nullptr;
    if (p->Get(name, &r) != NVSDK_NGX_Result_Success)
        p->Get(name, reinterpret_cast<void**>(&r));
    return r;
}

float Clampf(float v, float lo, float hi, float fallback)
{
    return std::isfinite(v) ? std::clamp(v, lo, hi) : fallback;
}

Settings ReadSettings(const Config& cfg)
{
    Settings s;
    int q = cfg.AmdGiQuality.value_or_default();
    // APUs default to Low (design 6.1); an explicit ini value wins.
    if (!cfg.AmdGiQuality.has_value() && DlssNr::AmdBridge::GpuSupportInfo().apu)
        q = 0;
    s.quality = q == 4 ? 2 : std::clamp(q, 0, 3); // Auto = High's trace until the tier controller lands
    s.intensity = Clampf(cfg.AmdGiIntensity.value_or_default(), 0.0f, 3.0f, 1.0f);
    s.occlusion = Clampf(cfg.AmdGiOcclusion.value_or_default(), 0.0f, 2.0f, 1.0f);
    s.radius = Clampf(cfg.AmdGiRadius.value_or_default(), 0.25f, 4.0f, 1.0f);
    const float t = cfg.AmdGiThickness.value_or_default();
    s.thickness = std::isfinite(t) && t > 0.0f ? std::clamp(t, 0.01f, 1.0f) : 0.0f;
    s.saturation = Clampf(cfg.AmdGiSaturation.value_or_default(), 0.0f, 2.0f, 1.0f);
    s.sky = Clampf(cfg.AmdGiSky.value_or_default(), 0.0f, 2.0f, 0.0f);
    s.feedback = Clampf(cfg.AmdGiFeedback.value_or_default(), 0.0f, 0.8f, 0.5f);
    s.nearFade = Clampf(cfg.AmdGiNearFade.value_or_default(), -1.0f, 1.0e6f, -1.0f);
    s.distanceFade = Clampf(cfg.AmdGiDistanceFade.value_or_default(), -1.0f, 1.0e6f, -1.0f);
    s.encoding = std::clamp(cfg.AmdGiEncoding.value_or_default(), -1, 2);
    s.debugView = std::clamp(cfg.AmdGiDebugView.value_or_default(), 0, 10);
    s.depthConvention = std::clamp(cfg.AmdGiDepthConvention.value_or_default(), -1, 2);
    s.traceCap = Clampf(cfg.AmdGiTraceCap.value_or_default(), 0.0f, 8.0f, 0.0f);
    s.albedoMode = std::clamp(cfg.AmdGiAlbedoMode.value_or_default(), 0, 2);
    s.aoLitProtect = Clampf(cfg.AmdGiAoLitProtect.value_or_default(), 0.0f, 1.0f, 1.0f);
    s.translucency = std::clamp(cfg.AmdGiTranslucency.value_or_default(), 0, 1);
    return s;
}

bool FovOk(float degrees)
{
    return std::isfinite(degrees) && degrees >= 20.0f && degrees <= 140.0f;
}

float VerticalFromHorizontal(float fovXRad, float aspect)
{
    return 2.0f * std::atan(std::tan(fovXRad * 0.5f) / std::max(aspect, 0.1f));
}

// Design 5.2: FSR parameters, Streamline constants, DLSS-FG camera, (auto-FOV: later), manual [AmdGi] Fov, the
// [FSR] VerticalFov/HorizontalFov when set in the ini, then 70 degrees vertical. Any source outside 20-140 is skipped.
Camera ReadCamera(NVSDK_NGX_Parameter* params, const Config& cfg, uint32_t rw, uint32_t rh)
{
    Camera cam;
    const float aspect = float(rw) / float(std::max(rh, 1u));

    float v = 0.0f;
    if (params->Get(OptiKeys::FSR_CameraFovVertical, &v) == NVSDK_NGX_Result_Success && FovOk(v * kRadToDeg))
    {
        cam.fovY = v;
        cam.source = GI_CAM_FSR;
        float n = 0.0f, f = 0.0f;
        if (params->Get(OptiKeys::FSR_NearPlane, &n) == NVSDK_NGX_Result_Success &&
            params->Get(OptiKeys::FSR_FarPlane, &f) == NVSDK_NGX_Result_Success && std::isfinite(n) &&
            std::isfinite(f))
        {
            // FSR allows reversed near/far; GI wants near < far.
            cam.nearZ = std::min(n, f);
            cam.farZ = std::max(n, f);
        }
        return cam;
    }

    // Streamline: the last slSetConstants, only while fresh (at most 2 presents old). Near/far from UE matrices
    // are not trusted (design 5.2), so only the FOV and the matrices are taken.
    const SLSetConstantsDiagnostics sl = StreamlineHooks::getSetConstantsDiagnostics();
    const uint64_t now = State::Instance().frameCount;
    if (sl.calls > 0 && sl.lastPresent != UINT64_MAX && now >= sl.lastPresent && now - sl.lastPresent <= 2)
    {
        const SLConstantsSnapshot snap = StreamlineHooks::getSLConstantsSnapshot();
        const sl::Constants& c = snap.constants;
        float fov = c.cameraFOV;
        if (!FovOk(fov * kRadToDeg))
        {
            const float p11 = c.cameraViewToClip.row[1].y;
            fov = std::isfinite(p11) && std::abs(p11) > 1.0e-4f ? 2.0f * std::atan(1.0f / std::abs(p11)) : 0.0f;
        }
        if (FovOk(fov * kRadToDeg))
        {
            cam.fovY = fov;
            cam.source = GI_CAM_SL;
            cam.hasViewToClip = true;
            cam.hasClipToPrevClip = true;
            for (int r = 0; r < 4; ++r)
            {
                const sl::float4& a = c.cameraViewToClip.row[r];
                const sl::float4& b = c.clipToPrevClip.row[r];
                const float ra[4] = { a.x, a.y, a.z, a.w }, rb[4] = { b.x, b.y, b.z, b.w };
                for (int k = 0; k < 4; ++k)
                {
                    cam.viewToClip[r * 4 + k] = ra[k];
                    cam.clipToPrevClip[r * 4 + k] = rb[k];
                }
            }
            return cam;
        }
    }

    // DLSS-FG camera, when the title also put it on the SR block (the FOV may be degrees or radians).
    float dlssgFov = 0.0f;
    if (params->Get("DLSSG.CameraFOV", &dlssgFov) == NVSDK_NGX_Result_Success && std::isfinite(dlssgFov) &&
        dlssgFov > 0.0f)
    {
        const float rad = dlssgFov > 3.2f ? dlssgFov * kDegToRad : dlssgFov;
        if (FovOk(rad * kRadToDeg))
        {
            cam.fovY = rad;
            cam.source = GI_CAM_DLSSG;
            float n = 0.0f, f = 0.0f;
            if (params->Get("DLSSG.CameraNear", &n) == NVSDK_NGX_Result_Success &&
                params->Get("DLSSG.CameraFar", &f) == NVSDK_NGX_Result_Success && std::isfinite(n) && std::isfinite(f))
            {
                cam.nearZ = std::min(n, f);
                cam.farZ = std::max(n, f);
            }
            return cam;
        }
    }

    const float manual = cfg.AmdGiFov.value_or_default();
    if (FovOk(manual))
    {
        cam.fovY = cfg.AmdGiFovAxis.value_or_default() == 1 ? VerticalFromHorizontal(manual * kDegToRad, aspect)
                                                             : manual * kDegToRad;
        cam.source = GI_CAM_MANUAL;
        return cam;
    }
    if (cfg.FsrVerticalFov.has_value() && FovOk(cfg.FsrVerticalFov.value()))
    {
        cam.fovY = cfg.FsrVerticalFov.value() * kDegToRad;
        cam.source = GI_CAM_FSR_INI;
        return cam;
    }
    if (cfg.FsrHorizontalFov.has_value() && FovOk(cfg.FsrHorizontalFov.value()))
    {
        cam.fovY = VerticalFromHorizontal(cfg.FsrHorizontalFov.value() * kDegToRad, aspect);
        cam.source = GI_CAM_FSR_INI;
        return cam;
    }
    cam.fovY = 70.0f * kDegToRad;
    cam.source = GI_CAM_DEFAULT;
    return cam;
}

const char* CameraName(int source)
{
    switch (source)
    {
    case GI_CAM_FSR: return "from FSR";
    case GI_CAM_SL: return "from Streamline";
    case GI_CAM_DLSSG: return "from DLSS-FG";
    case GI_CAM_MANUAL: return "manual";
    case GI_CAM_FSR_INI: return "from [FSR] ini";
    default: return "default";
    }
}
} // namespace

bool Wanted()
{
    return Config::Instance()->AmdGiEnabled.value_or_default() && DlssNr::AmdBridge::GpuSupportInfo().amd;
}

void NoteSkipped(const char* reason)
{
    SetSeamStatus(reason ? reason : "skipped");
}

void BeforeNr(ID3D12GraphicsCommandList* cmdList, NVSDK_NGX_Parameter* params, ID3D12CommandQueue* timingQueue,
              unsigned long long epoch)
{
    if (!cmdList || !params)
        return;
    std::lock_guard<std::mutex> lock(g_mutex);
    const Config& cfg = *Config::Instance();

    // A replacement left in this block by an earlier call that nobody restored: put the game's colour back first,
    // or GI would read its own output.
    if (t_params == params && t_original)
    {
        if (Resource(params, NVSDK_NGX_Parameter_Color) == t_replacement)
            params->Set(NVSDK_NGX_Parameter_Color, t_original);
        t_params = nullptr;
        t_original = t_replacement = nullptr;
    }

    // The colour chain is the host side (AmdBridge::Restore/Replacement/HasReplacement and NR's colour state):
    // until AmdBridge calls Restore, a replacement would stay in the title's block and the upscalers would barrier it
    // as the title's texture. So: no replacement and no passes (nothing recorded, the frame as before).
    if (!g_chainWired.load(std::memory_order_acquire))
    {
        SetSeamStatus("waiting for the colour chain (AmdBridge) - GI does not run in this build");
        return;
    }
    // [AmdGi] Placement=1 (after NR) needs the bridge's chain too (host side); until then GI runs before NR.

    // Inputs (the AmdBridge::Run reading rules, design 4.3 item 3).
    Inputs in;
    in.colour = Resource(params, NVSDK_NGX_Parameter_Color);
    in.depth = Resource(params, NVSDK_NGX_Parameter_Depth);
    in.motion = Resource(params, NVSDK_NGX_Parameter_MotionVectors);
    in.exposure = Resource(params, NVSDK_NGX_Parameter_ExposureTexture);
    in.reactive = Resource(params, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask);
    if (!in.reactive)
        in.reactive = Resource(params, OptiKeys::FSR_Reactive);
    if (!in.colour || !in.depth)
    {
        SetSeamStatus(in.colour ? "no depth" : "no colour");
        return;
    }
    UINT x = 0, y = 0;
    params->Get(NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X, &x);
    params->Get(NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y, &y);
    if (x || y)
    {
        SetSeamStatus("nonzero colour subrect origin unsupported");
        return;
    }
    UINT rw = 0, rh = 0;
    params->Get(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width, &rw);
    params->Get(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, &rh);
    const D3D12_RESOURCE_DESC cd = in.colour->GetDesc();
    if (!rw || !rh)
    {
        rw = UINT(cd.Width);
        rh = cd.Height;
    }
    in.renderWidth = rw;
    in.renderHeight = rh;

    UINT flags = 0;
    const bool haveFlags = params->Get(NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, &flags) == NVSDK_NGX_Result_Success;
    in.isHdr = (flags & NVSDK_NGX_DLSS_Feature_Flags_IsHDR) != 0;
    in.depthInverted = (flags & NVSDK_NGX_DLSS_Feature_Flags_DepthInverted) != 0;
    in.mvJittered = (flags & NVSDK_NGX_DLSS_Feature_Flags_MVJittered) != 0;
    in.mvLowRes = !haveFlags || (flags & NVSDK_NGX_DLSS_Feature_Flags_MVLowRes) != 0;
    if (in.motion && !in.mvLowRes)
    {
        // Display-sized vectors: sampled with a UV scale (design 5.1).
        UINT ow = 0, oh = 0;
        params->Get(NVSDK_NGX_Parameter_OutWidth, &ow);
        params->Get(NVSDK_NGX_Parameter_OutHeight, &oh);
        in.motionWidth = ow ? ow : UINT(in.motion->GetDesc().Width);
        in.motionHeight = oh ? oh : in.motion->GetDesc().Height;
    }
    params->Get(NVSDK_NGX_Parameter_MV_Scale_X, &in.mvScaleX);
    params->Get(NVSDK_NGX_Parameter_MV_Scale_Y, &in.mvScaleY);
    if (!std::isfinite(in.mvScaleX) || !std::isfinite(in.mvScaleY))
        in.mvScaleX = in.mvScaleY = 1.0f;
    params->Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &in.jitterX);
    params->Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &in.jitterY);
    if (!std::isfinite(in.jitterX) || !std::isfinite(in.jitterY))
        in.jitterX = in.jitterY = 0.0f;
    params->Get(NVSDK_NGX_Parameter_DLSS_Pre_Exposure, &in.preExposure);
    params->Get(NVSDK_NGX_Parameter_DLSS_Exposure_Scale, &in.exposureScale);
    if (!std::isfinite(in.preExposure))
        in.preExposure = 0.0f;
    if (!std::isfinite(in.exposureScale))
        in.exposureScale = 0.0f;
    UINT reset = 0;
    params->Get(NVSDK_NGX_Parameter_Reset, &reset);
    in.reset = reset != 0 || g_resetRequested.exchange(false);

    // Resource states: NGX inputs arrive readable unless the ini says otherwise (the NR path's rule).
    if (cfg.ColorResourceBarrier.has_value())
        in.colourState = static_cast<D3D12_RESOURCE_STATES>(cfg.ColorResourceBarrier.value());
    if (cfg.MVResourceBarrier.has_value())
        in.motionState = static_cast<D3D12_RESOURCE_STATES>(cfg.MVResourceBarrier.value());
    if (cfg.DepthResourceBarrier.has_value())
        in.depthState = static_cast<D3D12_RESOURCE_STATES>(cfg.DepthResourceBarrier.value());
    if (cfg.ExposureResourceBarrier.has_value())
        in.exposureState = static_cast<D3D12_RESOURCE_STATES>(cfg.ExposureResourceBarrier.value());
    in.timingQueue = timingQueue ? timingQueue : State::Instance().currentCommandQueue;

    // One stream per presented frame: the largest runs; a smaller second entry, or a stream under half the main
    // stream's area seen in the last 120 frames (scopes, picture-in-picture), passes through.
    const uint64_t area = uint64_t(rw) * rh;
    if (epoch == g_runEpoch && area <= g_runArea)
    {
        SetSeamStatus("");
        return;
    }
    if (area >= g_mainArea || epoch - g_mainEpoch > 120)
    {
        g_mainArea = area;
        g_mainEpoch = epoch;
    }
    else if (area * 2 < g_mainArea)
        return;

    // One effect, on the first device (the AmdDeviceGate rule).
    ID3D12Device* device = nullptr;
    if (FAILED(cmdList->GetDevice(IID_PPV_ARGS(&device))) || !device)
    {
        SetSeamStatus("no device");
        return;
    }
    device->Release(); // the command list keeps it alive; the effect holds its own reference
    if (device != g_vendorDevice)
    {
        g_vendorDevice = device;
        g_vendorAmd = DlssNr::AmdBridge::DeviceIsAmd(device);
    }
    if (!g_vendorAmd)
    {
        SetSeamStatus("needs an AMD GPU (the game's device is not AMD)");
        return;
    }
    if (!g_effect)
    {
        SetLogSink(SinkToLog);
        g_effect = Effect::Create(device).release();
        g_device = device;
        if (!g_effect)
        {
            SetSeamStatus("could not create the effect");
            return;
        }
        LOG_INFO("AMDNR Screen GI (preview) enabled: [AmdGi] Enabled=true");
    }
    if (device != g_device)
    {
        SetSeamStatus("second device: GI runs only on the first one");
        return;
    }

    in.camera = ReadCamera(params, cfg, rw, rh);
    const Settings s = ReadSettings(cfg);
    ID3D12Resource* out = g_effect->Record(cmdList, in, s);
    SetSeamStatus("");
    if (!out)
        return;
    g_runEpoch = epoch;
    g_runArea = area;

    // Hand the composited colour to NR and the upscaler through the parameter block; Restore puts the game's back.
    t_params = params;
    t_original = in.colour;
    t_replacement = out;
    params->Set(NVSDK_NGX_Parameter_Color, out);
}

void Restore(NVSDK_NGX_Parameter* params)
{
    g_chainWired.store(true, std::memory_order_release);
    if (params && t_params == params)
    {
        params->Set(NVSDK_NGX_Parameter_Color, t_original);
        t_params = nullptr;
        t_original = t_replacement = nullptr;
    }
}

ID3D12Resource* Replacement(const NVSDK_NGX_Parameter* params)
{
    return params && t_params == params ? t_replacement : nullptr;
}

ID3D12Resource* Original(const NVSDK_NGX_Parameter* params)
{
    return params && t_params == params ? t_original : nullptr;
}

bool ChainWired()
{
    return g_chainWired.load(std::memory_order_acquire);
}

Stats GetStats()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_effect ? g_effect->GetStats() : Stats {};
}

std::string StatusLine()
{
    if (!Config::Instance()->AmdGiEnabled.value_or_default())
        return {};
    if (!DlssNr::AmdBridge::GpuSupportInfo().amd)
        return "GI: needs an AMD GPU";
    {
        std::lock_guard<std::mutex> lock(g_statusMutex);
        if (!g_seamStatus.empty())
            return "GI: " + g_seamStatus;
    }
    const Stats s = GetStats();
    if (!s.status.empty())
        return "GI: " + s.status;
    if (!s.ready)
        return "GI: waiting for the first frame";
    std::string line = std::string("GI: ") + QualityName(s.quality);
    if (s.gpuMs >= 0.0f)
    {
        char ms[32];
        std::snprintf(ms, sizeof(ms), " - %.2f ms", s.gpuMs);
        line += ms;
    }
    line += " - trace " + std::to_string(s.traceWidth) + "x" + std::to_string(s.traceHeight);
    line += std::string(" - depth ") + DepthName(s.depthConvention) +
            (s.depthSource == 0 ? " (flag)" : s.depthSource == 1 ? " (from the data)" : " (ini)");
    char fov[48];
    std::snprintf(fov, sizeof(fov), " - FOV %.0f deg %s", s.fovYDegrees, CameraName(s.cameraSource));
    line += fov;
    return line;
}

void ResetHistory()
{
    g_resetRequested.store(true, std::memory_order_release);
}
} // namespace AmdnrGi::Seam
