// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#include <pch.h>

#include "Streamline_Hooks.h"

#include <Util.h>
#include <Config.h>

#include <nvapi/fakenvapi.h>
#include <misc/IdentifyGpu.h>
#include <hooks/Reflex_Hooks.h>
#include <menu/menu_overlay_base.h>
#include <framegen/nvngx/Nvngx_FG.h>
#include <proxies/KernelBase_Proxy.h>
#include <proxies/FfxApi_Proxy.h>
#include <imgui/ImGuiNotify.hpp>

#include <json.hpp>
#include <magic_enum.hpp>
#include "detours/detours.h"
#include <atomic>
#include <chrono>
#include <format>
#include <sl1_reflex.h>
#include <NVNGX_Parameter.h>

std::mutex StreamlineHooks::rrSignalTagMutex {};
RRSignalTagDiagnostics StreamlineHooks::rrSignalTagDiagnostics {};
std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,
           static_cast<size_t>(RRTaggedSignal::Count)> StreamlineHooks::rrTaggedD3D12Resources {};
SLTagInventoryDiagnostics StreamlineHooks::slTagInventoryDiagnostics {};
RRNGXPointerDiagnostics StreamlineHooks::rrNGXPointerDiagnostics {};

// AMDNR 0.3.4.1: the last EvaluateFeature-local tag of each RR signal that [RR_TAG_DIAG] logged. The local overlay is
// restored when slEvaluateFeature returns, so the global diagnostic cannot serve as "previous" for these tags: a
// title that passes its RR inputs to slEvaluateFeature (Control Resonant) got 2 identical lines on every frame.
// Guarded by rrSignalTagMutex; cleared with the global diagnostics so a new RR session logs its inputs once again.
static std::array<RRTaggedResourceDiagnostic, static_cast<size_t>(RRTaggedSignal::Count)> rrEvaluateTagLastLogged {};

namespace
{
// [Hotfix] DiagNoStreamlineHooks (crash triage): no detours into any Streamline module. Logged once.
bool DiagNoStreamlineHooks()
{
    static const bool disabled = []
    {
        const bool value = Config::Instance()->DiagNoStreamlineHooks.value_or_default();

        if (value)
        {
            LOG_INFO("[Diag] DiagNoStreamlineHooks=true: sl.interposer/sl.dlss/sl.dlss_d/sl.dlss_g/sl.reflex/sl.pcl/"
                     "sl.common are left unhooked (no NVIDIA spoof inside Streamline, no DLSS/RR through Streamline)");
        }

        return value;
    }();

    return disabled;
}

thread_local uint32_t g_rrActiveEvaluationFrame = UINT32_MAX;
thread_local uint32_t g_rrActiveEvaluationViewport = UINT32_MAX;

// B9 (AMDNR 0.3.4): FSR_RR features alive (rrTagConsumerAdded/Removed), and a release of the RR tag references
// waiting for the end of an outermost slEvaluateFeature. Plain atomics: the FSR_RR destructor also runs at
// process shutdown, where it must not take a lock.
std::atomic<int> g_rrTagConsumers { 0 };
std::atomic<bool> g_rrTagReleasePending { false };

// SAT-P0-3 (AMDNR 0.3.4): slSetConstants calls this session (SL2 and SL1) and State::frameCount at the last SL2 call,
// for FSR Ray Regeneration's one-shot [RR_CAM] streamline line (StreamlineHooks::getSetConstantsDiagnostics).
// Counting only; the hooks behave as before.
std::atomic<uint64_t> g_slSetConstantsCalls { 0 };
std::atomic<uint64_t> g_slSetConstantsSl1Calls { 0 };
std::atomic<uint64_t> g_slSetConstantsLastPresent { UINT64_MAX };

// SAT-P1 (AMDNR 0.3.4): the last four slSetConstants calls, recorded only with [FSR-RR]
// FfxDenoiserNgxDirectSLConstants=true, for an NGX-direct Ray Reconstruction evaluate to find its own frame's
// camera by jitter (StreamlineHooks::findSLConstantsForJitter). Guarded by setConstantsMutex.
struct SLConstantsRingEntry
{
    sl::Constants constants {};
    uint32_t frameIndex = UINT32_MAX;
    uint32_t viewport = UINT32_MAX;
    uint64_t present = 0;
    bool valid = false;
};
std::array<SLConstantsRingEntry, 4> g_slConstantsRing {};
uint32_t g_slConstantsRingNext = 0;

class ScopedRRActiveEvaluationFrame
{
  public:
    ScopedRRActiveEvaluationFrame(uint32_t frame, uint32_t viewport) noexcept
        : m_previousFrame(g_rrActiveEvaluationFrame),
          m_previousViewport(g_rrActiveEvaluationViewport)
    {
        g_rrActiveEvaluationFrame = frame;
        g_rrActiveEvaluationViewport = viewport;
    }

    ~ScopedRRActiveEvaluationFrame()
    {
        g_rrActiveEvaluationFrame = m_previousFrame;
        g_rrActiveEvaluationViewport = m_previousViewport;
    }

  private:
    uint32_t m_previousFrame;
    uint32_t m_previousViewport;
};
}







static bool TryGetRRTaggedSignal(sl::BufferType type, RRTaggedSignal& signal)
{
    switch (type)
    {
    case sl::kBufferTypeNormalRoughness:
        signal = RRTaggedSignal::NormalRoughness;
        return true;
    case sl::kBufferTypeEmissive:
        signal = RRTaggedSignal::Emissive;
        return true;
    case sl::kBufferTypeSpecularMotionVectors:
        signal = RRTaggedSignal::SpecularMotionVectors;
        return true;
    case sl::kBufferTypeReflectionMotionVectors:
        signal = RRTaggedSignal::ReflectionMotionVectors;
        return true;
    case sl::kBufferTypeSpecularHitDistance:
        signal = RRTaggedSignal::SpecularHitDistance;
        return true;
    case sl::kBufferTypeSpecularRayDirectionHitDistance:
        signal = RRTaggedSignal::SpecularRayDirectionHitDistance;
        return true;
    case sl::kBufferTypeLinearDepth:
        signal = RRTaggedSignal::LinearDepth;
        return true;
    case sl::kBufferTypeDiffuseHitNoisy:
        signal = RRTaggedSignal::DiffuseNoisy;
        return true;
    case sl::kBufferTypeDiffuseHitDenoised:
        signal = RRTaggedSignal::DiffuseDenoised;
        return true;
    case sl::kBufferTypeSpecularHitNoisy:
        signal = RRTaggedSignal::SpecularNoisy;
        return true;
    case sl::kBufferTypeSpecularHitDenoised:
        signal = RRTaggedSignal::SpecularDenoised;
        return true;
    case sl::kBufferTypeShadowNoisy:
        signal = RRTaggedSignal::ShadowNoisy;
        return true;
    case sl::kBufferTypeShadowDenoised:
        signal = RRTaggedSignal::ShadowDenoised;
        return true;
    case sl::kBufferTypeAmbientOcclusionNoisy:
        signal = RRTaggedSignal::AmbientOcclusionNoisy;
        return true;
    case sl::kBufferTypeAmbientOcclusionDenoised:
        signal = RRTaggedSignal::AmbientOcclusionDenoised;
        return true;
    case sl::kBufferTypeShadowHint:
        signal = RRTaggedSignal::ShadowHint;
        return true;
    case sl::kBufferTypeReflectionHint:
        signal = RRTaggedSignal::ReflectionHint;
        return true;
    default:
        return false;
    }
}

static bool IsRRCheckerboardInput(RRTaggedSignal signal)
{
    return signal == RRTaggedSignal::DiffuseNoisy ||
           signal == RRTaggedSignal::SpecularNoisy ||
           signal == RRTaggedSignal::ShadowNoisy ||
           signal == RRTaggedSignal::AmbientOcclusionNoisy;
}

static const char* GetRRTagSourceName(RRTagSource source)
{
    switch (source)
    {
    case RRTagSource::SetTag:
        return "slSetTag";
    case RRTagSource::SetTagForFrame:
        return "slSetTagForFrame";
    case RRTagSource::EvaluateFeature:
        return "slEvaluateFeature";
    default:
        return "unknown";
    }
}

const char* StreamlineHooks::getSLBufferTypeName(sl::BufferType type)
{
    static constexpr std::array<const char*, 68> names {
        "Depth",
        "MotionVectors",
        "HUDLessColor",
        "ScalingInputColor",
        "ScalingOutputColor",
        "Normals",
        "Roughness",
        "Albedo",
        "SpecularAlbedo",
        "IndirectAlbedo",
        "SpecularMotionVectors",
        "DisocclusionMask",
        "Emissive",
        "Exposure",
        "NormalRoughness",
        "DiffuseHitNoisy",
        "DiffuseHitDenoised",
        "SpecularHitNoisy",
        "SpecularHitDenoised",
        "ShadowNoisy",
        "ShadowDenoised",
        "AmbientOcclusionNoisy",
        "AmbientOcclusionDenoised",
        "UIColorAndAlpha",
        "ShadowHint",
        "ReflectionHint",
        "ParticleHint",
        "TransparencyHint",
        "AnimatedTextureHint",
        "BiasCurrentColorHint",
        "RaytracingDistance",
        "ReflectionMotionVectors",
        "Position",
        "InvalidDepthMotionHint",
        "Alpha",
        "OpaqueColor",
        "ReactiveMaskHint",
        "TransparencyAndCompositionMaskHint",
        "ReflectedAlbedo",
        "ColorBeforeParticles",
        "ColorBeforeTransparency",
        "ColorBeforeFog",
        "SpecularHitDistance",
        "SpecularRayDirectionHitDistance",
        "SpecularRayDirection",
        "DiffuseHitDistance",
        "DiffuseRayDirectionHitDistance",
        "DiffuseRayDirection",
        "HiResDepth",
        "LinearDepth",
        "BidirectionalDistortionField",
        "TransparencyLayer",
        "TransparencyLayerOpacity",
        "Backbuffer",
        "NoWarpMask",
        "ColorAfterParticles",
        "ColorAfterTransparency",
        "ColorAfterFog",
        "ScreenSpaceSubsurfaceScatteringGuide",
        "ColorBeforeScreenSpaceSubsurfaceScattering",
        "ColorAfterScreenSpaceSubsurfaceScattering",
        "ScreenSpaceRefractionGuide",
        "ColorBeforeScreenSpaceRefraction",
        "ColorAfterScreenSpaceRefraction",
        "DepthOfFieldGuide",
        "ColorBeforeDepthOfField",
        "ColorAfterDepthOfField",
        "ScalingOutputAlpha"
    };

    return type < names.size() ? names[type] : "Unknown/custom";
}

const char* StreamlineHooks::getRRTaggedSignalName(RRTaggedSignal signal)
{
    switch (signal)
    {
    case RRTaggedSignal::NormalRoughness:
        return "NormalRoughness";
    case RRTaggedSignal::Emissive:
        return "Emissive";
    case RRTaggedSignal::SpecularMotionVectors:
        return "SpecularMotionVectors";
    case RRTaggedSignal::ReflectionMotionVectors:
        return "ReflectionMotionVectors";
    case RRTaggedSignal::SpecularHitDistance:
        return "SpecularHitDistance";
    case RRTaggedSignal::SpecularRayDirectionHitDistance:
        return "SpecularRayDirectionHitDistance";
    case RRTaggedSignal::LinearDepth:
        return "LinearDepth";
    case RRTaggedSignal::DiffuseNoisy:
        return "Diffuse.Noisy";
    case RRTaggedSignal::DiffuseDenoised:
        return "Diffuse.Denoised";
    case RRTaggedSignal::SpecularNoisy:
        return "Specular.Noisy";
    case RRTaggedSignal::SpecularDenoised:
        return "Specular.Denoised";
    case RRTaggedSignal::ShadowNoisy:
        return "Shadow.Noisy";
    case RRTaggedSignal::ShadowDenoised:
        return "Shadow.Denoised";
    case RRTaggedSignal::AmbientOcclusionNoisy:
        return "AmbientOcclusion.Noisy";
    case RRTaggedSignal::AmbientOcclusionDenoised:
        return "AmbientOcclusion.Denoised";
    case RRTaggedSignal::ShadowHint:
        return "ShadowHint";
    case RRTaggedSignal::ReflectionHint:
        return "ReflectionHint";
    default:
        return "Unknown";
    }
}

const char* StreamlineHooks::getRRPreferredTagFormat(RRTaggedSignal signal)
{
    switch (signal)
    {
    case RRTaggedSignal::NormalRoughness:
        return "game-defined";
    case RRTaggedSignal::Emissive:
        return "shader-readable color texture";
    case RRTaggedSignal::SpecularMotionVectors:
    case RRTaggedSignal::ReflectionMotionVectors:
        return "DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R32G32_FLOAT, "
               "DXGI_FORMAT_R16G16B16A16_FLOAT, or DXGI_FORMAT_R32G32B32A32_FLOAT";
    case RRTaggedSignal::SpecularHitDistance:
        return "DXGI_FORMAT_R16_FLOAT or DXGI_FORMAT_R32_FLOAT";
    case RRTaggedSignal::SpecularRayDirectionHitDistance:
        return "DXGI_FORMAT_R16G16B16A16_FLOAT or DXGI_FORMAT_R32G32B32A32_FLOAT";
    case RRTaggedSignal::LinearDepth:
        return "DXGI_FORMAT_R32_FLOAT or DXGI_FORMAT_R16_FLOAT";
    case RRTaggedSignal::DiffuseNoisy:
    case RRTaggedSignal::DiffuseDenoised:
    case RRTaggedSignal::SpecularNoisy:
    case RRTaggedSignal::SpecularDenoised:
        return "DXGI_FORMAT_R16G16B16A16_FLOAT";
    case RRTaggedSignal::ShadowNoisy:
        return "DXGI_FORMAT_R16_FLOAT";
    case RRTaggedSignal::ShadowDenoised:
    case RRTaggedSignal::AmbientOcclusionNoisy:
    case RRTaggedSignal::AmbientOcclusionDenoised:
        return "DXGI_FORMAT_R8_UNORM";
    case RRTaggedSignal::ShadowHint:
    case RRTaggedSignal::ReflectionHint:
        return "hint only; not an RR signal";
    default:
        return "unknown";
    }
}

bool StreamlineHooks::isRRPreferredTagFormat(RRTaggedSignal signal, DXGI_FORMAT format)
{
    switch (signal)
    {
    case RRTaggedSignal::NormalRoughness:
        return format != DXGI_FORMAT_UNKNOWN;
    case RRTaggedSignal::Emissive:
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return true;
        default:
            return false;
        }
    case RRTaggedSignal::SpecularMotionVectors:
    case RRTaggedSignal::ReflectionMotionVectors:
        switch (format)
        {
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return true;
        default:
            return false;
        }
    case RRTaggedSignal::SpecularHitDistance:
        return format == DXGI_FORMAT_R16_FLOAT || format == DXGI_FORMAT_R32_FLOAT;
    case RRTaggedSignal::SpecularRayDirectionHitDistance:
        return format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
               format == DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RRTaggedSignal::LinearDepth:
        return format == DXGI_FORMAT_R32_FLOAT || format == DXGI_FORMAT_R16_FLOAT;
    case RRTaggedSignal::DiffuseNoisy:
    case RRTaggedSignal::DiffuseDenoised:
    case RRTaggedSignal::SpecularNoisy:
    case RRTaggedSignal::SpecularDenoised:
        return format == DXGI_FORMAT_R16G16B16A16_FLOAT;
    case RRTaggedSignal::ShadowNoisy:
        return format == DXGI_FORMAT_R16_FLOAT;
    case RRTaggedSignal::ShadowDenoised:
    case RRTaggedSignal::AmbientOcclusionNoisy:
    case RRTaggedSignal::AmbientOcclusionDenoised:
        return format == DXGI_FORMAT_R8_UNORM;
    default:
        return false;
    }
}

const char* StreamlineHooks::getRRCheckerboardAssessment(
    RRTaggedSignal signal, const RRTaggedResourceDiagnostic& diagnostic,
    uint32_t renderWidth, uint32_t renderHeight)
{
    if (!diagnostic.observed)
        return "not-observed";
    if (!diagnostic.present)
        return "cleared";
    if (!IsRRCheckerboardInput(signal))
        return "not-applicable";
    if (renderWidth == 0 || renderHeight == 0)
        return "render-size-unknown";
    if (diagnostic.effectiveHeight != renderHeight)
        return "resolution-mismatch";
    if (diagnostic.effectiveWidth == renderWidth)
        return "full-resolution";

    const uint64_t doubledWidth = static_cast<uint64_t>(diagnostic.effectiveWidth) * 2u;
    const uint64_t renderWidth64 = renderWidth;
    if (doubledWidth + 1u >= renderWidth64 && doubledWidth <= renderWidth64 + 1u)
        return "half-width-candidate";

    return "resolution-mismatch";
}

RRSignalTagDiagnostics StreamlineHooks::getRRSignalTagDiagnostics()
{
    std::scoped_lock lock(rrSignalTagMutex);
    return rrSignalTagDiagnostics;
}

Microsoft::WRL::ComPtr<ID3D12Resource> StreamlineHooks::getRRTaggedD3D12Resource(RRTaggedSignal signal)
{
    std::scoped_lock lock(rrSignalTagMutex);
    return rrTaggedD3D12Resources[static_cast<size_t>(signal)];
}

RRD3D12SignalTagSnapshot StreamlineHooks::getRRD3D12SignalTagSnapshot()
{
    std::scoped_lock lock(rrSignalTagMutex);
    RRD3D12SignalTagSnapshot snapshot {};
    snapshot.generation = rrSignalTagDiagnostics.generation;
    snapshot.activeEvaluationFrame = g_rrActiveEvaluationFrame;
    snapshot.activeEvaluationViewport = g_rrActiveEvaluationViewport;
    for (size_t i = 0; i < snapshot.resources.size(); ++i)
    {
        snapshot.resources[i].diagnostic = rrSignalTagDiagnostics.resources[i];
        snapshot.resources[i].resource = rrTaggedD3D12Resources[i];
    }
    return snapshot;
}

SLConstantsSnapshot StreamlineHooks::getSLConstantsSnapshot()
{
    std::scoped_lock lock(setConstantsMutex);
    const auto& state = State::Instance();
    return {
        .constants = state.slLastConstants,
        .frameIndex = state.slLastConstantsFrame,
        .viewport = state.slLastConstantsViewport
    };
}

SLSetConstantsDiagnostics StreamlineHooks::getSetConstantsDiagnostics()
{
    return {
        .hooked = o_slSetConstants != nullptr,
        .sl1Hooked = o_slSetConstants_interposer_sl1 != nullptr,
        .calls = g_slSetConstantsCalls.load(std::memory_order_relaxed),
        .sl1Calls = g_slSetConstantsSl1Calls.load(std::memory_order_relaxed),
        .lastPresent = g_slSetConstantsLastPresent.load(std::memory_order_relaxed),
    };
}

bool StreamlineHooks::findSLConstantsForJitter(float ngxJitterX, float ngxJitterY, uint64_t maxAgePresents,
                                               SLConstantsJitterMatch& out)
{
    // A zero jitter matches every zero-jitter frame, so it proves nothing about which frame this is.
    constexpr float kTolerance = 1e-3f;
    if (!std::isfinite(ngxJitterX) || !std::isfinite(ngxJitterY) ||
        (std::abs(ngxJitterX) < kTolerance && std::abs(ngxJitterY) < kTolerance))
        return false;

    const uint64_t now = State::Instance().frameCount;
    std::scoped_lock lock(setConstantsMutex);

    // Newest first.
    const uint32_t count = static_cast<uint32_t>(g_slConstantsRing.size());
    for (uint32_t i = 1; i <= count; ++i)
    {
        const SLConstantsRingEntry& entry = g_slConstantsRing[(g_slConstantsRingNext + count - i) % count];
        if (!entry.valid || now < entry.present || now - entry.present > maxAgePresents)
            continue;

        const float slX = entry.constants.jitterOffset.x;
        const float slY = entry.constants.jitterOffset.y;
        const float same = std::max(std::abs(slX - ngxJitterX), std::abs(slY - ngxJitterY));
        const float flipped = std::max(std::abs(slX + ngxJitterX), std::abs(slY + ngxJitterY));
        if (!(same < kTolerance) && !(flipped < kTolerance))
            continue;

        out.constants = entry.constants;
        out.frameIndex = entry.frameIndex;
        out.viewport = entry.viewport;
        out.agePresents = now - entry.present;
        out.signFlipped = !(same < kTolerance);
        out.jitterDelta = out.signFlipped ? flipped : same;
        return true;
    }

    return false;
}

void StreamlineHooks::resetRRSignalTagDiagnostics()
{
    std::scoped_lock lock(rrSignalTagMutex);
    rrSignalTagDiagnostics = {};
    rrTaggedD3D12Resources = {};
    rrEvaluateTagLastLogged = {};
}

void StreamlineHooks::rrTagConsumerAdded()
{
    g_rrTagConsumers.fetch_add(1, std::memory_order_acq_rel);
    // Ray Regeneration is on again: nothing to release. hkslEvaluateFeature re-checks the count in any case.
    g_rrTagReleasePending.store(false, std::memory_order_release);
}

void StreamlineHooks::rrTagConsumerRemoved()
{
    // Only marks the release, and logs nothing (this runs at process shutdown too). A title that re-creates RR
    // inside slEvaluateFeature (resize, quality change) destroys the old feature after it tagged this frame's
    // inputs and before the new feature reads them; releasing here would hand the new feature an empty tag set.
    if (g_rrTagConsumers.fetch_sub(1, std::memory_order_acq_rel) == 1)
        g_rrTagReleasePending.store(true, std::memory_order_release);
}

static bool HasResourceMetadataChanged(const RRTaggedResourceDiagnostic& previous,
                                       const RRTaggedResourceDiagnostic& next)
{
    return
        !previous.observed ||
        previous.present != next.present ||
        previous.debugName != next.debugName ||
        previous.nativeWidth != next.nativeWidth ||
        previous.nativeHeight != next.nativeHeight ||
        previous.effectiveWidth != next.effectiveWidth ||
        previous.effectiveHeight != next.effectiveHeight ||
        previous.extentLeft != next.extentLeft ||
        previous.extentTop != next.extentTop ||
        previous.format != next.format ||
        previous.dimension != next.dimension ||
        previous.resourceFlags != next.resourceFlags ||
        previous.mipLevels != next.mipLevels ||
        previous.arraySize != next.arraySize ||
        previous.sampleCount != next.sampleCount ||
        previous.state != next.state ||
        previous.lifecycle != next.lifecycle ||
        previous.viewport != next.viewport;
}

static std::string GetD3D12DebugObjectName(ID3D12Object* object)
{
    if (object == nullptr)
        return {};

    UINT nameSize = 0;
    object->GetPrivateData(WKPDID_D3DDebugObjectName, &nameSize, nullptr);
    if (nameSize > 1)
    {
        std::string name(nameSize, '\0');
        if (SUCCEEDED(object->GetPrivateData(WKPDID_D3DDebugObjectName, &nameSize, name.data())))
        {
            name.resize(strnlen(name.c_str(), name.size()));
            return name;
        }
    }

    nameSize = 0;
    object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &nameSize, nullptr);
    if (nameSize > sizeof(wchar_t))
    {
        std::vector<wchar_t> name(nameSize / sizeof(wchar_t), L'\0');
        if (SUCCEEDED(object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &nameSize, name.data())))
            return wstring_to_string(name.data());
    }

    return {};
}

static RRTaggedResourceDiagnostic CaptureSLResourceTag(
    const sl::ResourceTag& tag, uint32_t frameIndex, uint32_t viewport,
    RRTagSource source)
{
    RRTaggedResourceDiagnostic result {};
    result.observed = true;
    result.present = tag.resource != nullptr && tag.resource->native != nullptr;
    result.frameIndex = frameIndex;
    result.viewport = viewport;
    result.source = source;
    result.lifecycle = tag.lifecycle;

    if (!result.present)
        return result;

    result.resourceAddress = tag.resource->native;
    result.state = tag.resource->state;

    auto* resource = reinterpret_cast<ID3D12Resource*>(tag.resource->native);
    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    result.debugName = GetD3D12DebugObjectName(resource);
    result.nativeWidth = desc.Width;
    result.nativeHeight = desc.Height;
    result.format = desc.Format;
    result.dimension = desc.Dimension;
    result.resourceFlags = desc.Flags;
    result.mipLevels = desc.MipLevels;
    result.arraySize = desc.DepthOrArraySize;
    result.sampleCount = desc.SampleDesc.Count;

    result.usesExtent = static_cast<bool>(tag.extent);
    result.extentLeft = tag.extent.left;
    result.extentTop = tag.extent.top;
    result.effectiveWidth = result.usesExtent
        ? tag.extent.width
        : static_cast<uint32_t>(std::min<uint64_t>(desc.Width, UINT32_MAX));
    result.effectiveHeight = result.usesExtent ? tag.extent.height : desc.Height;
    return result;
}

SLTagInventoryDiagnostics StreamlineHooks::getSLTagInventoryDiagnostics()
{
    std::scoped_lock lock(rrSignalTagMutex);
    return slTagInventoryDiagnostics;
}

// Ray Regeneration cannot do anything without its denoiser DLL, and the DLL is absent in
// every installation that is not deliberately set up for RR. Probing the title's resource
// tags anyway means walking every tag, QueryInterface-ing every resource and reading its
// descriptor, on every SetTag call, in every game - to fill a diagnostic nobody will read.
//
// It is also a real risk rather than only a waste. These hooks now attach in every game
// (they used to attach only under frame generation), including titles on Streamline 1.x
// whose tag structures are not the ones this code was written against. The cheapest
// correct answer is to not touch them at all unless RR is actually available.
//
// One atomic load per call, and it is the first thing either probe does.
static bool RRProbingWanted()
{
    // Cached: IsDenoiserReady walks module state, and this sits on a per-tag path.
    static const bool wanted = FfxApiProxy::IsDenoiserReady(false);
    return wanted;
}

void StreamlineHooks::probeSLResourceTag(
    const sl::ResourceTag& tag, uint32_t frameIndex, uint32_t viewport,
    RRTagSource source)
{
    if (!RRProbingWanted())
        return;

    RRTaggedResourceDiagnostic next =
        CaptureSLResourceTag(tag, frameIndex, viewport, source);
    bool shouldLog = false;

    {
        std::scoped_lock lock(rrSignalTagMutex);
        auto entry = std::find_if(
            slTagInventoryDiagnostics.resources.begin(), slTagInventoryDiagnostics.resources.end(),
            [&tag](const SLTaggedResourceInventoryEntry& candidate) { return candidate.type == tag.type; });

        if (entry == slTagInventoryDiagnostics.resources.end())
        {
            slTagInventoryDiagnostics.resources.push_back({ tag.type, next });
            entry = std::prev(slTagInventoryDiagnostics.resources.end());
            shouldLog = true;
        }
        else
        {
            next.updateCount = entry->resource.updateCount + 1;
            shouldLog = HasResourceMetadataChanged(entry->resource, next);
            entry->resource = next;
        }

        if (entry->resource.updateCount == 0)
            entry->resource.updateCount = 1;

        std::sort(
            slTagInventoryDiagnostics.resources.begin(), slTagInventoryDiagnostics.resources.end(),
            [](const SLTaggedResourceInventoryEntry& left, const SLTaggedResourceInventoryEntry& right) {
                return left.type < right.type;
            });
        ++slTagInventoryDiagnostics.generation;
    }

    if (!shouldLog)
        return;

    if (!next.present)
    {
        LOG_INFO("[SL_TAG_INVENTORY] {} type={} ({}) frame={} cleared",
                 GetRRTagSourceName(source), tag.type, getSLBufferTypeName(tag.type), frameIndex);
        return;
    }

    const auto formatName = magic_enum::enum_name(next.format);
    LOG_INFO(
        "[SL_TAG_INVENTORY] {} type={} ({}) frame={} ptr={}, debugName='{}', native={}x{}, effective={}x{}, "
        "extent=[{},{}], format={}({}), dimension={}, mips={}, arrays={}, samples={}, "
        "resourceFlags={:#x}, state={:#x}",
        GetRRTagSourceName(source), tag.type, getSLBufferTypeName(tag.type), frameIndex,
        next.resourceAddress, next.debugName, next.nativeWidth, next.nativeHeight,
        next.effectiveWidth, next.effectiveHeight, next.extentLeft, next.extentTop,
        formatName.empty() ? "UNKNOWN" : formatName, static_cast<uint32_t>(next.format),
        static_cast<uint32_t>(next.dimension), next.mipLevels, next.arraySize, next.sampleCount,
        static_cast<uint32_t>(next.resourceFlags), next.state);
}

void StreamlineHooks::logSLTagInventoryDiagnostics(uint32_t renderWidth, uint32_t renderHeight)
{
    const auto diagnostics = getSLTagInventoryDiagnostics();
    LOG_INFO("[SL_TAG_INVENTORY] snapshot: generation={}, observedTypes={}, render={}x{}",
             diagnostics.generation, diagnostics.resources.size(), renderWidth, renderHeight);

    for (const auto& entry : diagnostics.resources)
    {
        const auto& diagnostic = entry.resource;
        if (!diagnostic.present)
        {
            LOG_INFO("[SL_TAG_INVENTORY] snapshot type={} ({}): cleared, updates={}",
                     entry.type, getSLBufferTypeName(entry.type), diagnostic.updateCount);
            continue;
        }

        const auto formatName = magic_enum::enum_name(diagnostic.format);
        const bool fullResolution =
            renderWidth != 0 && renderHeight != 0 &&
            diagnostic.effectiveWidth == renderWidth && diagnostic.effectiveHeight == renderHeight;
        const bool halfWidthCandidate =
            renderWidth != 0 && renderHeight != 0 &&
            diagnostic.effectiveHeight == renderHeight &&
            static_cast<uint64_t>(diagnostic.effectiveWidth) * 2u + 1u >= renderWidth &&
            static_cast<uint64_t>(diagnostic.effectiveWidth) * 2u <= static_cast<uint64_t>(renderWidth) + 1u;

        LOG_INFO(
            "[SL_TAG_INVENTORY] snapshot type={} ({}): ptr={}, debugName='{}', effective={}x{}, native={}x{}, "
            "format={}({}), state={:#x}, updates={}, resolutionClass={}",
            entry.type, getSLBufferTypeName(entry.type), diagnostic.resourceAddress,
            diagnostic.debugName,
            diagnostic.effectiveWidth, diagnostic.effectiveHeight,
            diagnostic.nativeWidth, diagnostic.nativeHeight,
            formatName.empty() ? "UNKNOWN" : formatName, static_cast<uint32_t>(diagnostic.format),
            diagnostic.state, diagnostic.updateCount,
            fullResolution ? "full-resolution" : halfWidthCandidate ? "half-width-candidate" : "other");
    }
}

RRNGXPointerDiagnostics StreamlineHooks::getRRNGXPointerDiagnostics()
{
    std::scoped_lock lock(rrSignalTagMutex);
    return rrNGXPointerDiagnostics;
}

static const char* GetNGXPointerKindName(RRNGXPointerKind kind)
{
    switch (kind)
    {
    case RRNGXPointerKind::OpaquePointer:
        return "opaque-pointer";
    case RRNGXPointerKind::D3D11Resource:
        return "D3D11-resource";
    case RRNGXPointerKind::D3D12Resource:
        return "D3D12-resource";
    default:
        return "unknown";
    }
}

void StreamlineHooks::probeRRNGXPointerParameters(const NVSDK_NGX_Parameter& parameters)
{
    static std::atomic_uint64_t probeCalls = 0;
    const uint64_t probeCall = probeCalls.fetch_add(1, std::memory_order_relaxed);
    {
        std::scoped_lock lock(rrSignalTagMutex);
        if (rrNGXPointerDiagnostics.generation != 0 && probeCall % 120u != 0)
            return;
    }

    uint32_t allocationType = NGX_AllocTypes::Unknown;
    if (parameters.Get(NGX_AllocTypes::AllocKey.data(), &allocationType) != NVSDK_NGX_Result_Success ||
        (allocationType != NGX_AllocTypes::InternDynamic &&
         allocationType != NGX_AllocTypes::InternPersistent))
    {
        std::scoped_lock lock(rrSignalTagMutex);
        rrNGXPointerDiagnostics.tableInspectable = false;
        return;
    }

    const auto& internalParameters = static_cast<const NVNGX_Parameters&>(parameters);
    const auto pointers = internalParameters.enumeratePointerParameters();

    std::vector<RRNGXPointerDiagnostic> nextEntries;
    nextEntries.reserve(pointers.size());
    for (const auto& pointer : pointers)
    {
        RRNGXPointerDiagnostic next {};
        next.name = pointer.name;
        next.present = pointer.address != nullptr;
        next.address = pointer.address;

        switch (pointer.kind)
        {
        case NGXPointerParameterKind::D3D11Resource:
            next.kind = RRNGXPointerKind::D3D11Resource;
            break;
        case NGXPointerParameterKind::D3D12Resource:
            next.kind = RRNGXPointerKind::D3D12Resource;
            break;
        default:
            next.kind = RRNGXPointerKind::OpaquePointer;
            break;
        }

        // Only exact D3D12 resource values are dereferenced. NGX void pointers
        // can also contain matrices, callbacks, or backend-specific objects.
        if (next.present && next.kind == RRNGXPointerKind::D3D12Resource)
        {
            const auto desc = static_cast<ID3D12Resource*>(pointer.address)->GetDesc();
            next.nativeWidth = desc.Width;
            next.nativeHeight = desc.Height;
            next.format = desc.Format;
            next.dimension = desc.Dimension;
            next.resourceFlags = desc.Flags;
            next.mipLevels = desc.MipLevels;
            next.arraySize = desc.DepthOrArraySize;
            next.sampleCount = desc.SampleDesc.Count;
        }

        nextEntries.push_back(std::move(next));
    }

    std::sort(nextEntries.begin(), nextEntries.end(),
              [](const RRNGXPointerDiagnostic& left, const RRNGXPointerDiagnostic& right) {
                  return left.name < right.name;
              });

    std::vector<RRNGXPointerDiagnostic> changedEntries;
    {
        std::scoped_lock lock(rrSignalTagMutex);
        for (auto& next : nextEntries)
        {
            const auto previous = std::find_if(
                rrNGXPointerDiagnostics.parameters.begin(), rrNGXPointerDiagnostics.parameters.end(),
                [&next](const RRNGXPointerDiagnostic& candidate) { return candidate.name == next.name; });

            if (previous != rrNGXPointerDiagnostics.parameters.end())
            {
                next.updateCount = previous->updateCount + 1;
                const bool changed =
                    previous->kind != next.kind ||
                    previous->present != next.present ||
                    previous->nativeWidth != next.nativeWidth ||
                    previous->nativeHeight != next.nativeHeight ||
                    previous->format != next.format ||
                    previous->dimension != next.dimension ||
                    previous->resourceFlags != next.resourceFlags ||
                    previous->mipLevels != next.mipLevels ||
                    previous->arraySize != next.arraySize ||
                    previous->sampleCount != next.sampleCount;
                if (changed)
                    changedEntries.push_back(next);
            }
            else
            {
                next.updateCount = 1;
                changedEntries.push_back(next);
            }
        }

        rrNGXPointerDiagnostics.tableInspectable = true;
        rrNGXPointerDiagnostics.parameters = nextEntries;
        ++rrNGXPointerDiagnostics.generation;
    }

    for (const auto& entry : changedEntries)
    {
        if (entry.kind != RRNGXPointerKind::D3D12Resource || !entry.present)
        {
            LOG_INFO("[RR_NGX_INVENTORY] key='{}', kind={}, ptr={}, present={}",
                     entry.name, GetNGXPointerKindName(entry.kind), entry.address, entry.present);
            continue;
        }

        const auto formatName = magic_enum::enum_name(entry.format);
        LOG_INFO(
            "[RR_NGX_INVENTORY] key='{}', kind={}, ptr={}, size={}x{}, format={}({}), "
            "dimension={}, mips={}, arrays={}, samples={}, resourceFlags={:#x}",
            entry.name, GetNGXPointerKindName(entry.kind), entry.address,
            entry.nativeWidth, entry.nativeHeight,
            formatName.empty() ? "UNKNOWN" : formatName, static_cast<uint32_t>(entry.format),
            static_cast<uint32_t>(entry.dimension), entry.mipLevels, entry.arraySize,
            entry.sampleCount, static_cast<uint32_t>(entry.resourceFlags));
    }
}

void StreamlineHooks::logRRNGXPointerDiagnostics()
{
    const auto diagnostics = getRRNGXPointerDiagnostics();
    LOG_INFO("[RR_NGX_INVENTORY] snapshot: inspectable={}, generation={}, pointerParameters={}",
             diagnostics.tableInspectable, diagnostics.generation, diagnostics.parameters.size());

    for (const auto& entry : diagnostics.parameters)
    {
        if (entry.kind != RRNGXPointerKind::D3D12Resource || !entry.present)
        {
            LOG_INFO("[RR_NGX_INVENTORY] snapshot key='{}': kind={}, ptr={}, present={}, updates={}",
                     entry.name, GetNGXPointerKindName(entry.kind), entry.address,
                     entry.present, entry.updateCount);
            continue;
        }

        const auto formatName = magic_enum::enum_name(entry.format);
        LOG_INFO(
            "[RR_NGX_INVENTORY] snapshot key='{}': kind={}, ptr={}, size={}x{}, "
            "format={}({}), dimension={}, updates={}",
            entry.name, GetNGXPointerKindName(entry.kind), entry.address,
            entry.nativeWidth, entry.nativeHeight,
            formatName.empty() ? "UNKNOWN" : formatName, static_cast<uint32_t>(entry.format),
            static_cast<uint32_t>(entry.dimension), entry.updateCount);
    }
}

void StreamlineHooks::resetRRInputInventoryDiagnostics()
{
    std::scoped_lock lock(rrSignalTagMutex);
    slTagInventoryDiagnostics = {};
    rrNGXPointerDiagnostics = {};
}

void StreamlineHooks::probeRRResourceTag(
    const sl::ResourceTag& tag, uint32_t frameIndex, uint32_t viewport,
    RRTagSource source)
{
    // See probeSLResourceTag: nothing here is reachable without the RR denoiser.
    if (!RRProbingWanted())
        return;

    RRTaggedSignal signal;
    if (!TryGetRRTaggedSignal(tag.type, signal))
        return;

    RRTaggedResourceDiagnostic next =
        CaptureSLResourceTag(tag, frameIndex, viewport, source);

    RRTaggedResourceDiagnostic previous {};
    bool shouldLog = false;
    {
        std::scoped_lock lock(rrSignalTagMutex);
        auto& stored = rrSignalTagDiagnostics.resources[static_cast<size_t>(signal)];
        previous = stored;
        next.updateCount = previous.updateCount + 1;

        if (source == RRTagSource::EvaluateFeature)
        {
            // Compare with the last logged local tag, not the restored global one (see rrEvaluateTagLastLogged).
            auto& lastLogged = rrEvaluateTagLastLogged[static_cast<size_t>(signal)];
            shouldLog = HasResourceMetadataChanged(lastLogged, next);
            if (shouldLog)
                lastLogged = next;
        }
        else
        {
            shouldLog = HasResourceMetadataChanged(previous, next);
        }

        stored = next;
        auto& retainedResource = rrTaggedD3D12Resources[static_cast<size_t>(signal)];
        // eOnlyValidNow submitted through SetTag cannot be cached after that call
        // returns. A local ResourceTag passed directly to EvaluateFeature remains
        // inside its declaring call, however, and is retained only until the local
        // overlay is restored when that evaluation returns.
        retainedResource = next.present &&
                (next.lifecycle != sl::ResourceLifecycle::eOnlyValidNow ||
                 source == RRTagSource::EvaluateFeature)
            ? reinterpret_cast<ID3D12Resource*>(next.resourceAddress)
            : nullptr;
        ++rrSignalTagDiagnostics.generation;
    }

    if (!shouldLog)
        return;

    if (!next.present)
    {
        LOG_INFO("[RR_TAG_DIAG] {} {} frame={} cleared",
                 GetRRTagSourceName(source), getRRTaggedSignalName(signal), frameIndex);
        return;
    }

    const auto formatName = magic_enum::enum_name(next.format);
    const auto lifecycleName = magic_enum::enum_name(next.lifecycle);
    LOG_INFO(
        "[RR_TAG_DIAG] {} {} frame={} ptr={}, debugName='{}', native={}x{}, effective={}x{}, "
        "extent=[{},{}], format={}({}), dimension={}, mips={}, arrays={}, samples={}, "
        "resourceFlags={:#x}, state={:#x}, lifecycle={}, preferredFormat={} ({})",
        GetRRTagSourceName(source), getRRTaggedSignalName(signal), frameIndex,
        next.resourceAddress, next.debugName, next.nativeWidth, next.nativeHeight,
        next.effectiveWidth, next.effectiveHeight, next.extentLeft, next.extentTop,
        formatName.empty() ? "UNKNOWN" : formatName, static_cast<uint32_t>(next.format),
        static_cast<uint32_t>(next.dimension), next.mipLevels, next.arraySize, next.sampleCount,
        static_cast<uint32_t>(next.resourceFlags), next.state,
        lifecycleName.empty() ? "UNKNOWN" : lifecycleName,
        isRRPreferredTagFormat(signal, next.format), getRRPreferredTagFormat(signal));
}

void StreamlineHooks::logRRSignalTagDiagnostics(uint32_t renderWidth, uint32_t renderHeight)
{
    const RRSignalTagDiagnostics diagnostics = getRRSignalTagDiagnostics();
    LOG_INFO("[RR_TAG_DIAG] snapshot: generation={}, render={}x{}",
             diagnostics.generation, renderWidth, renderHeight);

    for (size_t i = 0; i < diagnostics.resources.size(); ++i)
    {
        const auto signal = static_cast<RRTaggedSignal>(i);
        const auto& diagnostic = diagnostics.resources[i];
        if (!diagnostic.observed)
        {
            LOG_INFO("[RR_TAG_DIAG] snapshot {}: not observed",
                     getRRTaggedSignalName(signal));
            continue;
        }

        if (!diagnostic.present)
        {
            LOG_INFO("[RR_TAG_DIAG] snapshot {}: cleared, updates={}",
                     getRRTaggedSignalName(signal), diagnostic.updateCount);
            continue;
        }

        const auto formatName = magic_enum::enum_name(diagnostic.format);
        LOG_INFO(
            "[RR_TAG_DIAG] snapshot {}: ptr={}, debugName='{}', effective={}x{}, native={}x{}, "
            "format={}({}), state={:#x}, updates={}, checkerboard={}, preferredFormat={} ({})",
            getRRTaggedSignalName(signal), diagnostic.resourceAddress,
            diagnostic.debugName,
            diagnostic.effectiveWidth, diagnostic.effectiveHeight,
            diagnostic.nativeWidth, diagnostic.nativeHeight,
            formatName.empty() ? "UNKNOWN" : formatName, static_cast<uint32_t>(diagnostic.format),
            diagnostic.state, diagnostic.updateCount,
            getRRCheckerboardAssessment(signal, diagnostic, renderWidth, renderHeight),
            isRRPreferredTagFormat(signal, diagnostic.format), getRRPreferredTagFormat(signal));
    }
}


static bool IsSL1AndDLSSGActive()
{
    return State::Instance().streamlineVersion.major == 1 && State::Instance().activeFgInput == FGInput::DLSSG &&
           (State::Instance().activeFgOutput == FGOutput::FSRFG || State::Instance().activeFgOutput == FGOutput::XeFG);
}

static bool IsSL1AndFGActive()
{
    const auto& state = State::Instance();

    return state.streamlineVersion.major == 1 && state.activeFgInput == FGInput::DLSSG;
}

static void PatchSL1PluginJson(nlohmann::json& configJson)
{
    if (!IsSL1AndFGActive())
        return;

    LOG_DEBUG("Patching SL1 plugin JSON for external FG management");

    if (configJson.contains("/hooks"_json_pointer))
        configJson["hooks"].clear();

    if (configJson.contains("/exclusive_hooks"_json_pointer))
        configJson["exclusive_hooks"].clear();

    if (configJson.contains("/external/feature/tags"_json_pointer))
        configJson["external"]["feature"]["tags"].clear();

    if (configJson.contains("/vsync/supported"_json_pointer))
        configJson["vsync"]["supported"] = true;

    if (configJson.contains("/external/hws/required"_json_pointer))
        configJson["external"]["hws"]["required"] = false;
}

// ---------------------------------------------------------------------------------------------
// slInit: the plugin-JSON guard and the [SLINIT] diagnostics (0.3.3, NBA 2K27 report)
// ---------------------------------------------------------------------------------------------
//
// NBA 2K27 on an AMD card stops with "slInit error 0x18". 0x18 is sl::Result::eErrorExceptionHandler
// (24): Streamline runs slInit inside SEH, and an exception anywhere in it - in a plugin, in a
// loader hook, in one of our slOnPluginLoad wrappers - comes back as that code, with a minidump in
// %ProgramData%\NVIDIA\Streamline\<exe>\<id>\. Which step faulted cannot be read from the code, so
// the log has to name it. Every [SLINIT] line is INFO (the shipped LogLevel=2 keeps it) and is
// written only while the game's slInit runs under hkslInit, so nothing changes outside that window.

static bool SlInitDiagnostics() { return State::Instance().slInitInProgress.load(std::memory_order_relaxed); }

namespace
{
// Opens the diagnostics window for exactly the original slInit call and puts the previous state
// back on the way out, so a nested slInit cannot close the outer one's window early.
class ScopedSlInitWindow
{
  public:
    ScopedSlInitWindow() : m_previous(State::Instance().slInitInProgress.exchange(true)) {}
    ~ScopedSlInitWindow() { State::Instance().slInitInProgress.store(m_previous); }
    ScopedSlInitWindow(const ScopedSlInitWindow&) = delete;
    ScopedSlInitWindow& operator=(const ScopedSlInitWindow&) = delete;

  private:
    bool m_previous;
};

// Skips OptiScaler's loader checks for one DLL name for the lifetime of the object - the pattern
// hookInterposer uses around its version read - and always re-enables them, even on a throw.
class ScopedDllChecksSkip
{
  public:
    ScopedDllChecksSkip(UINT owner, std::string dllName) : m_owner(owner)
    {
        State::DisableChecks(owner, std::move(dllName));
    }
    ~ScopedDllChecksSkip() { State::EnableChecks(m_owner); }
    ScopedDllChecksSkip(const ScopedDllChecksSkip&) = delete;
    ScopedDllChecksSkip& operator=(const ScopedDllChecksSkip&) = delete;

  private:
    UINT m_owner;
};
} // namespace

// Set once per slInit, so the adapter list is logged once.
static std::atomic<bool> g_slInitGpusLogged { false };

// sl::Result by name, for the log line and the menu. A switch rather than magic_enum: State keeps
// the pointer for the menu, so it has to be a string literal.
static const char* SlResultName(sl::Result result)
{
    switch (result)
    {
    case sl::Result::eOk:
        return "eOk";
    case sl::Result::eErrorIO:
        return "eErrorIO";
    case sl::Result::eErrorDriverOutOfDate:
        return "eErrorDriverOutOfDate";
    case sl::Result::eErrorOSOutOfDate:
        return "eErrorOSOutOfDate";
    case sl::Result::eErrorOSDisabledHWS:
        return "eErrorOSDisabledHWS";
    case sl::Result::eErrorDeviceNotCreated:
        return "eErrorDeviceNotCreated";
    case sl::Result::eErrorNoSupportedAdapterFound:
        return "eErrorNoSupportedAdapterFound";
    case sl::Result::eErrorAdapterNotSupported:
        return "eErrorAdapterNotSupported";
    case sl::Result::eErrorNoPlugins:
        return "eErrorNoPlugins";
    case sl::Result::eErrorVulkanAPI:
        return "eErrorVulkanAPI";
    case sl::Result::eErrorDXGIAPI:
        return "eErrorDXGIAPI";
    case sl::Result::eErrorD3DAPI:
        return "eErrorD3DAPI";
    case sl::Result::eErrorNRDAPI:
        return "eErrorNRDAPI";
    case sl::Result::eErrorNVAPI:
        return "eErrorNVAPI";
    case sl::Result::eErrorReflexAPI:
        return "eErrorReflexAPI";
    case sl::Result::eErrorNGXFailed:
        return "eErrorNGXFailed";
    case sl::Result::eErrorJSONParsing:
        return "eErrorJSONParsing";
    case sl::Result::eErrorMissingProxy:
        return "eErrorMissingProxy";
    case sl::Result::eErrorMissingResourceState:
        return "eErrorMissingResourceState";
    case sl::Result::eErrorInvalidIntegration:
        return "eErrorInvalidIntegration";
    case sl::Result::eErrorMissingInputParameter:
        return "eErrorMissingInputParameter";
    case sl::Result::eErrorNotInitialized:
        return "eErrorNotInitialized";
    case sl::Result::eErrorComputeFailed:
        return "eErrorComputeFailed";
    case sl::Result::eErrorInitNotCalled:
        return "eErrorInitNotCalled";
    case sl::Result::eErrorExceptionHandler:
        return "eErrorExceptionHandler";
    case sl::Result::eErrorInvalidParameter:
        return "eErrorInvalidParameter";
    case sl::Result::eErrorMissingConstants:
        return "eErrorMissingConstants";
    case sl::Result::eErrorDuplicatedConstants:
        return "eErrorDuplicatedConstants";
    case sl::Result::eErrorMissingOrInvalidAPI:
        return "eErrorMissingOrInvalidAPI";
    case sl::Result::eErrorCommonConstantsMissing:
        return "eErrorCommonConstantsMissing";
    case sl::Result::eErrorUnsupportedInterface:
        return "eErrorUnsupportedInterface";
    case sl::Result::eErrorFeatureMissing:
        return "eErrorFeatureMissing";
    case sl::Result::eErrorFeatureNotSupported:
        return "eErrorFeatureNotSupported";
    case sl::Result::eErrorFeatureMissingHooks:
        return "eErrorFeatureMissingHooks";
    case sl::Result::eErrorFeatureFailedToLoad:
        return "eErrorFeatureFailedToLoad";
    case sl::Result::eErrorFeatureWrongPriority:
        return "eErrorFeatureWrongPriority";
    case sl::Result::eErrorFeatureMissingDependency:
        return "eErrorFeatureMissingDependency";
    case sl::Result::eErrorFeatureManagerInvalidState:
        return "eErrorFeatureManagerInvalidState";
    case sl::Result::eErrorInvalidState:
        return "eErrorInvalidState";
    case sl::Result::eWarnOutOfVRAM:
        return "eWarnOutOfVRAM";
    default:
        return "unknown result (newer Streamline)";
    }
}

static const char* SlFeatureName(sl::Feature feature)
{
    switch (feature)
    {
    case sl::kFeatureDLSS:
        return "DLSS";
    case sl::kFeatureNRD_INVALID:
        return "NRD";
    case sl::kFeatureNIS:
        return "NIS";
    case sl::kFeatureReflex:
        return "Reflex";
    case sl::kFeaturePCL:
        return "PCL";
    case sl::kFeatureDeepDVC:
        return "DeepDVC";
    case sl::kFeatureLatewarp:
        return "Latewarp";
    case sl::kFeatureDLSS_G:
        return "DLSS_G";
    case sl::kFeatureDLSS_RR:
        return "DLSS_RR";
    case sl::kFeatureNvPerf:
        return "NvPerf";
    case sl::kFeatureDirectSR:
        return "DirectSR";
    case 1004: // kFeatureDLSS_NR in public Streamline main; not in the headers this tree carries
        return "DLSS_NR";
    case sl::kFeatureImGUI:
        return "ImGUI";
    case sl::kFeatureCommon:
        return "Common";
    default:
        return "?";
    }
}

static void RecordSlInitResult(int32_t result, const char* name, uint32_t ms, bool sl1)
{
    auto& state = State::Instance();
    state.slInitResultName.store(name);
    state.slInitMs.store(ms);
    state.slInitIsSl1.store(sl1);
    state.slInitResult.store(result); // last: a reader that sees the result sees the rest
}

// D2, first half: what the game asked slInit for. Memory reads only - no file, loader or DXGI call -
// because it runs before the original slInit, outside Streamline's SEH, where a throw of ours would
// end the game rather than return 0x18. So it also catches everything it could throw.
static void LogSlInitPreferences(const sl::Preferences& gamePref, const sl::Preferences& localPref,
                                 uint64_t sdkVersion)
{
    try
    {
        const auto wide = [](const wchar_t* text)
        { return text != nullptr ? wstring_to_string(std::wstring(text)) : std::string("(none)"); };

        std::string features;
        if (localPref.featuresToLoad != nullptr)
        {
            const uint32_t count = std::min<uint32_t>(localPref.numFeaturesToLoad, 64u);
            for (uint32_t i = 0; i < count; i++)
            {
                const sl::Feature feature = localPref.featuresToLoad[i];
                features += std::format("{}{} ({})", i == 0 ? "" : ", ", SlFeatureName(feature), feature);
            }
        }

        const uint64_t flags = static_cast<uint64_t>(localPref.flags);
        LOG_INFO("[SLINIT] slInit called: sdkVersion 0x{:X}, flags 0x{:X} (OTA {}, downloaded plugins {}), engine {} "
                 "'{}', applicationId {}, renderAPI {}, features to load ({}): {}",
                 sdkVersion, flags, (flags & static_cast<uint64_t>(sl::PreferenceFlags::eAllowOTA)) != 0,
                 (flags & static_cast<uint64_t>(sl::PreferenceFlags::eLoadDownloadedPlugins)) != 0,
                 magic_enum::enum_name(localPref.engine),
                 localPref.engineVersion != nullptr ? localPref.engineVersion : "", localPref.applicationId,
                 magic_enum::enum_name(localPref.renderAPI), localPref.numFeaturesToLoad,
                 features.empty() ? std::string("none") : features);

        // Where the game told Streamline to write sl.log: with a folder set, Streamline also copies
        // sl.log next to the minidump it writes for a 0x18.
        LOG_INFO("[SLINIT] slInit log folder: {}", wide(gamePref.pathToLogsAndData));

        if (localPref.pathsToPlugins == nullptr || localPref.numPathsToPlugins == 0)
        {
            LOG_INFO("[SLINIT] slInit plugin paths: none given, Streamline loads its plugins from beside the exe");
        }
        else
        {
            const uint32_t count = std::min<uint32_t>(localPref.numPathsToPlugins, 16u);
            for (uint32_t i = 0; i < count; i++)
                LOG_INFO("[SLINIT] slInit plugin path {} of {}: {}", i + 1, localPref.numPathsToPlugins,
                         wide(localPref.pathsToPlugins[i]));
        }
    }
    catch (const std::exception& e)
    {
        LOG_WARN("[SLINIT] slInit preferences summary skipped: {}", e.what());
    }
    catch (...)
    {
        LOG_WARN("[SLINIT] slInit preferences summary skipped");
    }
}

// D2, second half: the sl.*.dll files where Streamline looked, with their file versions - an updater
// that replaced some plugins and not others leaves mixed versions (the report's first candidate).
// hkslInit calls it only when slInit failed.
//
// Built AFTER the original slInit, on purpose. Reading a DLL's version goes through LoadLibraryExW
// (as a data file), which our loader hooks see, and for an sl.* name LoadLibraryCheckW loads and
// hooks the real module itself - which, before slInit, would be ahead of Streamline and change what
// we are trying to observe. The checks are skipped for exactly the file being read: per file,
// because the skip matches the END of the loaded name ("sl.dlss" matches ...\sl.dlss.dll), so a bare
// "sl." prefix would match nothing.
static void LogStreamlinePluginFiles(const sl::Preferences& localPref)
{
    constexpr size_t maxFiles = 64;

    try
    {
        std::vector<std::filesystem::path> folders;
        if (localPref.pathsToPlugins != nullptr)
        {
            const uint32_t count = std::min<uint32_t>(localPref.numPathsToPlugins, 16u);
            for (uint32_t i = 0; i < count; i++)
            {
                if (localPref.pathsToPlugins[i] != nullptr)
                    folders.emplace_back(localPref.pathsToPlugins[i]);
            }
        }

        if (folders.empty())
            folders.push_back(Util::ExePath().parent_path());

        const auto owner = State::GetOwner();
        size_t listed = 0;

        for (const auto& folder : folders)
        {
            const auto folderName = wstring_to_string(folder.wstring());

            std::error_code ec;
            std::filesystem::directory_iterator it(folder, std::filesystem::directory_options::skip_permission_denied,
                                                   ec);
            if (ec)
            {
                LOG_INFO("[SLINIT] Streamline plugin files in {}: cannot list ({})", folderName, ec.message());
                continue;
            }

            size_t inFolder = 0;
            for (; !ec && it != std::filesystem::directory_iterator(); it.increment(ec))
            {
                std::error_code fileEc;
                if (!it->is_regular_file(fileEc))
                    continue;

                const std::wstring fileName = it->path().filename().wstring();
                std::wstring lower = fileName;
                to_lower_in_place(lower);

                if (!lower.starts_with(L"sl.") || !lower.ends_with(L".dll"))
                    continue;

                if (++listed > maxFiles)
                {
                    LOG_INFO("[SLINIT] more than {} Streamline plugin files; the rest are not listed", maxFiles);
                    return;
                }

                inFolder++;

                version_t fileVersion {};
                bool gotVersion = false;
                {
                    ScopedDllChecksSkip skip(owner, wstring_to_string(lower.substr(0, lower.size() - 4)));
                    gotVersion = Util::GetFileVersion(it->path().wstring(), &fileVersion);
                }

                std::error_code sizeEc;
                const auto size = it->file_size(sizeEc);

                if (gotVersion)
                    LOG_INFO("[SLINIT] Streamline plugin file {}: {}.{}.{}.{}, {} bytes, in {}",
                             wstring_to_string(fileName), fileVersion.major, fileVersion.minor, fileVersion.patch,
                             fileVersion.reserved, sizeEc ? 0 : size, folderName);
                else
                    LOG_INFO("[SLINIT] Streamline plugin file {}: no version resource, {} bytes, in {}",
                             wstring_to_string(fileName), sizeEc ? 0 : size, folderName);
            }

            if (inFolder == 0)
                LOG_INFO("[SLINIT] Streamline plugin files in {}: none", folderName);
        }
    }
    catch (const std::exception& e)
    {
        LOG_WARN("[SLINIT] Streamline plugin file listing stopped: {}", e.what());
    }
    catch (...)
    {
        LOG_WARN("[SLINIT] Streamline plugin file listing stopped");
    }
}

// D4, entry half: the last line before a fault inside a plugin's own onLoad names that plugin.
static void LogPluginLoadEntry(const char* plugin, bool spoofing)
{
    if (SlInitDiagnostics())
        LOG_INFO("[SLINIT] {} slOnPluginLoad: calling the plugin (architecture spoof {})", plugin,
                 spoofing ? "on" : "off");
}

// F1: ONE PATH FOR EVERY slOnPluginLoad JSON PATCH (sl.dlss, sl.dlss_d, sl.dlss_g, the local
// sl.dlss_g, sl.reflex, sl.common). The six wrappers used to parse *pluginJSON unconditionally:
// when a plugin's onLoad returns false it may leave the JSON unset, strlen(nullptr) faults, and
// Streamline turns the access violation into slInit error 0x18. A throw from the parse or the
// patch reached Streamline's catch and it dropped the plugin.
//
// The outcomes are Streamline's own, without the fault:
//   - onLoad returned false: return false, JSON untouched (Streamline skips the plugin and never
//     reads its JSON).
//   - onLoad returned true with no JSON (every wrapper, sl.common too): return false, so Streamline
//     skips the plugin; handing the null back would only move the fault into Streamline.
//   - SkipPlugin (fail closed; five wrappers): the parse / patch / dump threw -> return false, so
//     Streamline skips the plugin exactly as it did after the exception. Loading it unpatched
//     instead would keep sl.dlss_g's swapchain hooks under DLSSG input or output, and on Vulkan ask
//     a non-NVIDIA GPU for NVIDIA-only extensions.
//   - KeepStreamlines (fail open; sl.common only): the parse / patch / dump threw -> Streamline's
//     own JSON, unpatched, and the interposer's file version stays in State - sl.common is the one
//     plugin nothing loads without.
// The patched text is committed (and handed to Streamline) only after the whole patch succeeded.
// The architecture restore (setArch) runs in the callers before this, so it runs on every path.
enum class PluginJsonOnFailure
{
    SkipPlugin,
    KeepStreamlines,
};

static bool PluginJsonPatchFailed(const char* plugin, const char* what, PluginJsonOnFailure onFailure)
{
    if (onFailure == PluginJsonOnFailure::KeepStreamlines)
    {
        LOG_ERROR("{} plugin JSON patch failed ({}); Streamline's own JSON is kept", plugin, what);
        return true;
    }

    LOG_ERROR("{} plugin JSON patch failed ({}); reporting the load as failed so Streamline skips {}", plugin, what,
              plugin);
    return false;
}

template <class Patch>
static bool PatchPluginJson(const char* plugin, bool loaded, const char** pluginJSON, std::string& store,
                            PluginJsonOnFailure onFailure, Patch&& patch)
{
    const bool haveJson = pluginJSON != nullptr && *pluginJSON != nullptr;

    if (!loaded)
    {
        if (SlInitDiagnostics())
            LOG_INFO("[SLINIT] {} slOnPluginLoad returned false (plugin JSON {}): Streamline skips the plugin", plugin,
                     haveJson ? "set" : "null");
        return false;
    }

    // No JSON is fail-closed for every plugin, sl.common included: Streamline reads *pluginJSON
    // after an onLoad that returned true, so handing the null back would only move the fault (and
    // the 0x18) from this wrapper into Streamline. Returning false sends it down its own path for a
    // plugin that did not load, with no exception. (sl.common always sets its JSON; this is the
    // guard, not an expected case.)
    if (!haveJson)
    {
        LOG_WARN("{} slOnPluginLoad returned true with no plugin JSON; reporting the load as failed so Streamline "
                 "skips {}",
                 plugin, plugin);
        return false;
    }

    try
    {
        auto configJson = nlohmann::json::parse(*pluginJSON);
        patch(configJson);
        auto patched = configJson.dump();

        store = std::move(patched);
        *pluginJSON = store.c_str();
    }
    catch (const std::exception& e)
    {
        return PluginJsonPatchFailed(plugin, e.what(), onFailure);
    }
    catch (...)
    {
        return PluginJsonPatchFailed(plugin, "non-standard exception", onFailure);
    }

    if (SlInitDiagnostics())
        LOG_INFO("[SLINIT] {} slOnPluginLoad returned true, plugin JSON patched", plugin);

    return true;
}

// D7, adapters: the list IdentifyGpu already holds (real vendor IDs; iGPU and software adapters
// included), once per slInit. Called only after the caller's getSystemCapsArch() - which asks
// IdentifyGpu for the primary GPU - so the cache is filled and this adds no DXGI call.
void StreamlineHooks::logCachedGpusForSlInit()
{
    if (!SlInitDiagnostics() || g_slInitGpusLogged.exchange(true))
        return;

    const auto gpus = IdentifyGpu::getAllGpus();
    LOG_INFO("[SLINIT] IdentifyGpu: {} adapter(s), primary first", gpus.size());

    for (size_t i = 0; i < gpus.size(); i++)
    {
        const auto& gpu = gpus[i];
        LOG_INFO("[SLINIT] adapter {}: '{}' vendor 0x{:X} device 0x{:X} rev 0x{:X} luid {:08X}:{:08X}{}{}", i,
                 gpu.name, static_cast<uint32_t>(gpu.vendorId), gpu.deviceId, gpu.revisionId,
                 static_cast<uint32_t>(gpu.luid.HighPart), static_cast<uint32_t>(gpu.luid.LowPart),
                 gpu.softwareAdapter ? ", software" : "", gpu.dlssCapable ? ", DLSS capable" : "");
    }
}

// D7, SystemCaps: what Streamline's adapter table says before the spoof, during the plugin's load and
// after the restore. The restore puts back the architecture only; vendor, driver 999 and HWS stay as
// spoofed for the rest of slInit, and this line is where that shows. Reads through the pointer
// hookSystemCaps has just re-read, so it touches only memory setArch touches anyway.
void StreamlineHooks::logSystemCapsForSlInit(const char* plugin, const char* stage, SystemCaps* altSystemCaps)
{
    if (!SlInitDiagnostics())
        return;

    if (altSystemCaps != nullptr || State::Instance().streamlineVersion.major > 1)
    {
        const SystemCaps* caps = altSystemCaps != nullptr ? altSystemCaps : systemCaps;
        if (caps == nullptr)
        {
            LOG_INFO("[SLINIT] {} SystemCaps {}: not available", plugin, stage);
            return;
        }

        std::string adapters;
        const uint32_t count = std::min<uint32_t>(caps->gpuCount, kMaxNumSupportedGPUs);
        for (uint32_t i = 0; i < count; i++)
        {
            const auto& adapter = caps->adapters[i];
            adapters += std::format("; #{} vendor 0x{:X} arch 0x{:X} impl 0x{:X} rev 0x{:X} device 0x{:X}", i,
                                    static_cast<uint32_t>(adapter.vendor), adapter.architecture,
                                    adapter.implementation, adapter.revision, adapter.deviceId);
        }

        LOG_INFO("[SLINIT] {} SystemCaps {}: {} GPU(s), driver {}.{}, HWS {}{}", plugin, stage, caps->gpuCount,
                 caps->driverVersionMajor, caps->driverVersionMinor, caps->hwsSupported, adapters);
    }
    else if (State::Instance().streamlineVersion.major == 1)
    {
        const SystemCapsSl15* caps = systemCapsSl15;
        if (caps == nullptr)
        {
            LOG_INFO("[SLINIT] {} SystemCaps (Streamline 1) {}: not available", plugin, stage);
            return;
        }

        std::string adapters;
        const uint32_t count = std::min<uint32_t>(caps->gpuCount, kMaxNumSupportedGPUs);
        for (uint32_t i = 0; i < count; i++)
            adapters += std::format("; #{} arch 0x{:X} impl 0x{:X} rev 0x{:X}", i, caps->architecture[i],
                                    caps->implementation[i], caps->revision[i]);

        LOG_INFO("[SLINIT] {} SystemCaps (Streamline 1) {}: {} GPU(s), driver {}.{}, HWS {}{}", plugin, stage,
                 caps->gpuCount, caps->driverVersionMajor, caps->driverVersionMinor, caps->hwSchedulingEnabled,
                 adapters);
    }
}

char* StreamlineHooks::trimStreamlineLog(const char* msg)
{
    char* result = (char*) malloc(strlen(msg) + 1);
    if (!result)
        return nullptr;

    strcpy(result, msg);

    size_t length = strlen(result);
    if (length > 0 && result[length - 1] == '\n')
    {
        result[length - 1] = '\0';
    }

    return result;
}

void StreamlineHooks::streamlineLogCallback(sl::LogType type, const char* msg)
{
    if (msg == nullptr)
        return;

    char* trimmed_msg = trimStreamlineLog(msg);
    if (trimmed_msg != nullptr)
    {
        switch (type)
        {
        case sl::LogType::eWarn:
            LOG_WARN("{}", trimmed_msg);
            break;
        case sl::LogType::eInfo:
            LOG_INFO("{}", trimmed_msg);
            break;
        case sl::LogType::eError:
            LOG_ERROR("{}", trimmed_msg);
            break;
        case sl::LogType::eCount:
            LOG_ERROR("{}", trimmed_msg);
            break;
        }

        free(trimmed_msg);
    }

    if (o_logCallback != nullptr)
        o_logCallback(type, msg);
}

sl::Result StreamlineHooks::hkslInit(const sl::Preferences& pref, uint64_t sdkVersion)
{
    LOG_FUNC();

    sl::Preferences localPref = pref;

    if (localPref.logMessageCallback != &streamlineLogCallback)
        o_logCallback = localPref.logMessageCallback;
    localPref.logLevel = sl::LogLevel::eCount;
    localPref.logMessageCallback = &streamlineLogCallback;

    // renderAPI is optional so need to be careful, should only matter for Vulkan
    renderApi = localPref.renderAPI;

    State::Instance().slFGInputs.reportEngineType(localPref.engine);

    // Treat engine type set in Streamline as ground truth
    if (localPref.engine == sl::EngineType::eUnreal)
        State::Instance().gameQuirks |= GameQuirk::ForceUnrealEngine;

    std::filesystem::path localSlPath(Config::Instance()->MainDllPath.value());
    localSlPath = localSlPath / L"streamline"; // Hardcoded streamline folder

    auto localSlPathStr = localSlPath.wstring();

    std::vector<const wchar_t*> storage;

    // Replace the SL files to allow for MFG
    if (State::Instance().activeFgInput == FGInput::NvngxFG && std::filesystem::exists(localSlPath / L"sl.common.dll"))
    {
        storage.assign(localPref.pathsToPlugins, localPref.pathsToPlugins + localPref.numPathsToPlugins);

        std::filesystem::path pluginsDir;

        // Find the first path that contains sl.common.dll
        // If storage is empty, look in the exe folder. pathsToPlugins is an optional field
        if (storage.empty())
        {
            std::filesystem::path exeFolder = Util::ExePath().parent_path();
            if (std::filesystem::exists(exeFolder / L"sl.common.dll"))
            {
                pluginsDir = exeFolder;
            }
        }
        else
        {
            for (const wchar_t* pathStr : storage)
            {
                if (!pathStr)
                    continue;

                std::filesystem::path p = pathStr;
                if (std::filesystem::exists(p / L"sl.common.dll"))
                {
                    pluginsDir = p;
                    break;
                }
            }
        }

        std::vector<std::string> missingDlls;
        bool hasNewerPlugin = false;

        // If we found the plugins folder, scan its contents
        if (!pluginsDir.empty() && std::filesystem::exists(pluginsDir))
        {
            for (const auto& entry : std::filesystem::directory_iterator(pluginsDir))
            {
                if (!entry.is_regular_file())
                    continue;

                std::wstring filename = entry.path().filename().wstring();

                std::wstring lowerName = filename;
                to_lower_in_place(lowerName);

                // Skip interposer
                if (lowerName == L"sl.interposer.dll")
                    continue;

                const bool isSlDll = lowerName.starts_with(L"sl.") && lowerName.ends_with(L".dll");
                const bool isNvLowLatency = lowerName == L"nvlowlatencyvk.dll";

                if (isSlDll || isNvLowLatency)
                {
                    std::filesystem::path localDllPath = localSlPath / filename;

                    // Check if localSlPath also has this DLL
                    if (!std::filesystem::exists(localDllPath))
                    {
                        missingDlls.push_back(entry.path().filename().string());
                    }
                    else
                    {
                        // Compare versions
                        version_t pluginVer, pluginProdVer;
                        version_t localVer, localProdVer;

                        bool gotPluginVer = Util::GetFileVersion(entry.path().wstring(), &pluginVer, &pluginProdVer);
                        bool gotLocalVer = Util::GetFileVersion(localDllPath.wstring(), &localVer, &localProdVer);

                        if (gotPluginVer && gotLocalVer)
                        {
                            if (localVer > pluginVer)
                            {
                                hasNewerPlugin = true;
                            }
                        }
                    }
                }
            }
        }

        // Insert local path only if a newer plugin was found
        if (hasNewerPlugin)
        {
            LOG_DEBUG("Making the game use local streamline files");

            storage.insert(storage.begin(), localSlPathStr.c_str());
            localPref.pathsToPlugins = storage.data();
            localPref.numPathsToPlugins = (uint32_t) storage.size();

            if (!missingDlls.empty())
            {
                std::string toastMsg = "You are missing the following dlls from the streamline folder:\n";
                for (const auto& missingDll : missingDlls)
                {
                    toastMsg += "- " + missingDll + "\n";
                }

                ImGui::InsertNotification({ ImGuiToastType::Warning, 20000, toastMsg.c_str() });
            }
        }
    }

    // Function scope: localPref points into it for the whole of the original slInit below. (It was
    // a block-local with an early return; one call site is what lets F3 keep the result.)
    std::vector<sl::Feature> localFeaturesToLoad;

    if (State::Instance().activeFgInput == FGInput::DLSSG || State::Instance().activeFgOutput == FGOutput::DLSSG)
    {
        localFeaturesToLoad.assign(pref.featuresToLoad, pref.featuresToLoad + pref.numFeaturesToLoad);
        std::erase(localFeaturesToLoad, sl::kFeatureDLSS_G);

        localPref.featuresToLoad = localFeaturesToLoad.data();
        localPref.numFeaturesToLoad = localFeaturesToLoad.size();
    }

    // bool hookSetTag =
    //     (State::Instance().activeFgInput == FGInput::NvngxFG || State::Instance().activeFgInput == FGInput::DLSSG);

    // if (hookSetTag)
    //     localPref->flags &= ~(sl::PreferenceFlags::eAllowOTA | sl::PreferenceFlags::eLoadDownloadedPlugins);

    // To prevent mixed up OTA situations
    // if (State::Instance().activeFgOutput == FGOutput::DLSSG)
    //{
    //    localPref.flags &= ~sl::PreferenceFlags::eAllowOTA;
    //    localPref.flags &= ~sl::PreferenceFlags::eLoadDownloadedPlugins;
    //}

    // D2: what the game asked for (memory reads only; the file listing waits until after slInit).
    LogSlInitPreferences(pref, localPref, sdkVersion);

    // F3: KEEP WHAT slInit RETURNED. A failed slInit makes the game switch DLSS off, so no upscaler
    // call ever reaches the neural pass; the log and the Neural tab now say why. The [SLINIT]
    // diagnostics are live only inside this call.
    g_slInitGpusLogged.store(false);
    const auto start = std::chrono::steady_clock::now();
    sl::Result result;
    {
        ScopedSlInitWindow window;
        result = o_slInit(localPref, sdkVersion);
    }
    const auto ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());

    RecordSlInitResult(static_cast<int32_t>(result), SlResultName(result), ms, false);

    if (result == sl::Result::eOk)
    {
        LOG_INFO("slInit returned 0 (eOk) in {} ms", ms);
    }
    else
    {
        LOG_ERROR("slInit returned {} (0x{:X}, {}) in {} ms{}", static_cast<int32_t>(result),
                  static_cast<uint32_t>(result), SlResultName(result), ms,
                  result == sl::Result::eErrorExceptionHandler
                      ? " - Streamline caught an exception inside slInit and wrote a minidump (and sl.log, when the "
                        "game sets a log folder) under %ProgramData%\\NVIDIA\\Streamline\\<game exe>\\"
                      : "");
    }

    // D2, second half: the plugin files and their versions, now that nothing we read can get
    // ahead of Streamline's own loads. Only after a FAILED slInit - the case a start-up error
    // report needs. Each file read toggles State's loader-skip map (DisableChecks / EnableChecks),
    // which has no lock and which the loader hooks read on any thread, so a successful start-up,
    // i.e. every launch of every Streamline game, does not open that window.
    if (result != sl::Result::eOk)
        LogStreamlinePluginFiles(localPref);

    return result;
}

// RAY RECONSTRUCTION UNDER SPOOFING. A title asks Streamline whether DLSS_RR is supported
// before it offers the option, and Streamline answers from the adapter - an AMD card, or the
// Pascal we spoof for Reflex - so the option never appears and the RR feature is never
// created, which leaves FSR Ray Regeneration with nothing to serve. burak113's branch
// answered "supported" for DLSS_RR in every game; this answers it only where the answer
// can be made true: a card Ray Regeneration is offered on (rrHardwareAllowed) and a denoiser.
static RrGate::GpuClass RrGpuClass(const GpuInformation& gpu)
{
    if (gpu.vendorId == VendorId::Invalid)
        return RrGate::GpuClass::Unknown;
    if (gpu.vendorId == VendorId::Nvidia)
        return RrGate::GpuClass::Nvidia;
    if (IdentifyGpu::isRdna4(gpu))
        return RrGate::GpuClass::Rdna4;
    if (IdentifyGpu::isRdna3(gpu))
        return RrGate::GpuClass::Rdna3;
    return RrGate::GpuClass::Other;
}

static RrGate::Decision RrDecisionFor(const GpuInformation& gpu)
{
    const auto& key = Config::Instance()->FfxDenoiserAllowPreRdna4;
    return RrGate::Decide(RrGpuClass(gpu), key.has_value() ? std::optional<bool>(*key) : std::nullopt);
}

RrGate::Decision StreamlineHooks::rrHardwareDecision()
{
    // (AMDNR 0.3.4, owner decision 2026-09-26 a) RDNA 4, and RDNA 3 / 3.5 (RX 7000 and the RDNA 3 APUs) by default again
    // as before 0.3.3.2 (0.3.3.2 offered it on RDNA 4 only). RDNA 2 and older, Intel: only with [FSR-RR]
    // FfxDenoiserAllowPreRdna4=true. A value in the ini always wins (false = RDNA 4 only). Where the driver refuses
    // the denoiser at Init, the fallback (FSR 3.1/4 with a toast and the session latch in NVNGX_DLSS_Dx12.cpp)
    // upscales the Ray Reconstruction image without a denoiser; the latch keeps later handles from retrying.
    return RrDecisionFor(IdentifyGpu::getPrimaryGpu());
}

bool StreamlineHooks::rrHardwareAllowed()
{
    const auto gpu = IdentifyGpu::getPrimaryGpu();
    const auto d = RrDecisionFor(gpu);

    // P2: inside DllMain the primary GPU is not known yet (IdentifyGpu gets no DXGI factory there). Say no for now
    // WITHOUT spending the one-time line below, so the first answer about a known card is the one the log keeps.
    if (d.why == RrGate::Why::GpuUnknown)
    {
        static std::atomic<bool> loggedUnknown { false };
        if (!loggedUnknown.exchange(true))
            LOG_DEBUG("FSR-RR: GPU not known yet; Ray Regeneration is decided once it is");
        return false;
    }

    static std::atomic<bool> logged { false };
    if (d.why != RrGate::Why::Nvidia && d.why != RrGate::Why::Rdna4 && !logged.exchange(true))
    {
        switch (d.why)
        {
        case RrGate::Why::Rdna3Default:
            LOG_INFO("FSR-RR: offered on {} (RDNA 3, offered by default); [FSR-RR] FfxDenoiserAllowPreRdna4=false "
                     "keeps the game's own denoiser",
                     gpu.name);
            break;
        case RrGate::Why::IniTrue:
            if (IdentifyGpu::isRdna3(gpu))
                LOG_INFO("FSR-RR: offered on {} (RDNA 3; [FSR-RR] FfxDenoiserAllowPreRdna4=true)", gpu.name);
            else
                LOG_WARN("FSR-RR: offered on {} (not RDNA 3 or 4) because [FSR-RR] FfxDenoiserAllowPreRdna4=true; if "
                         "the driver refuses it, the game's Ray Reconstruction image is upscaled without a denoiser",
                         gpu.name);
            break;
        case RrGate::Why::IniFalse:
            LOG_INFO("FSR-RR: not offered on {} because [FSR-RR] FfxDenoiserAllowPreRdna4=false (RDNA 4 only); the "
                     "game keeps its own denoiser",
                     gpu.name);
            break;
        default:
            LOG_INFO("FSR-RR: not offered on {} (device {:04X} rev {:02X}, not RDNA 3 or RDNA 4); the game keeps its "
                     "own denoiser. [FSR-RR] FfxDenoiserAllowPreRdna4=true tries it anyway",
                     gpu.name, gpu.deviceId, gpu.revisionId);
            break;
        }
    }

    return d.offered;
}

// A denoiser to serve with. The first version cached the ABI check, and in Cyberpunk sl.interposer was already in
// memory when OptiScaler initialised, so this ran 13 ms BEFORE the denoiser DLL was loaded, cached false, and every RR
// gate stayed shut for the session: no sl.dlss_d hook, no "supported" answer, the game's RR toggle greyed out. The
// question this has to answer at interposer-hook time is "will there be a denoiser", and the file on disk answers that
// before any module does. The ABI check still decides at feature creation, in the provider.
static bool RrDenoiserExpected()
{
    if (FfxApiProxy::IsDenoiserApiImplementedDx12())
        return true;
    static const bool fileThere = []
    {
        std::error_code ec;
        const auto dir = Util::DllPath().parent_path();
        return std::filesystem::exists(dir / L"OptiScaler" / L"amd_fidelityfx_denoiser_dx12.dll", ec) ||
               std::filesystem::exists(dir / L"amd_fidelityfx_denoiser_dx12.dll", ec);
    }();
    return fileThere;
}

bool StreamlineHooks::rrCanServe()
{
    // The card (rrHardwareAllowed; never cached, so a later answer can still say yes) and a denoiser. If the driver
    // refuses anyway, feature Init fails and NVNGX_DLSS_Dx12.cpp falls back to FFX (FSR 4 INT8 / 3.1; FSR 2.1.2 only
    // as the last resort), warns once and latches for the session; the RR image is then NOT denoised.
    if (!rrHardwareAllowed())
        return false;
    return RrDenoiserExpected();
}

// P2 (AMDNR 0.3.4): whether the Streamline RR detours (the two answers in hookInterposer, the sl.dlss_d hook) attach
// now. Where the GPU is not known yet (DllMain) they attach when a denoiser is expected and decide per call.
static bool RrAttachNow(const char* what)
{
    const auto gpuClass = RrGpuClass(IdentifyGpu::getPrimaryGpu());
    const bool canServe = gpuClass != RrGate::GpuClass::Unknown && StreamlineHooks::rrCanServe();
    const bool attach = RrGate::AttachAtHook(gpuClass, canServe, RrDenoiserExpected());
    if (attach && !canServe)
        LOG_INFO("FSR-RR: {} attached before the GPU is known; Ray Regeneration is decided on each call", what);
    return attach;
}

sl::Result StreamlineHooks::hkslIsFeatureSupported(sl::Feature feature, const sl::AdapterInfo& adapterInfo)
{
    // Gated on DLSSG input rather than on being attached: this hook now also attaches for
    // Ray Regeneration, and there it must not touch the game's own frame generation.
    if (feature == sl::kFeatureDLSS_G && State::Instance().activeFgInput == FGInput::DLSSG)
        return sl::Result::eOk;

    if (feature == sl::kFeatureDLSS_RR && rrCanServe())
    {
        LOG_INFO("slIsFeatureSupported(DLSS_RR): answering supported, FSR Ray Regeneration can serve it");
        return sl::Result::eOk;
    }

    return o_slIsFeatureSupported(feature, adapterInfo);
}

sl::Result StreamlineHooks::hkslIsFeatureLoaded(sl::Feature feature, bool& loaded)
{
    if (feature == sl::kFeatureDLSS_G)
    {
        loaded = true;
        return sl::Result::eOk;
    }

    return o_slIsFeatureLoaded(feature, loaded);
}

sl::Result StreamlineHooks::hkslGetFeatureRequirements(sl::Feature feature, sl::FeatureRequirements& requirements)
{
    if (feature == sl::kFeatureDLSS_G && State::Instance().activeFgInput == FGInput::DLSSG)
        return sl::Result::eOk;

    // The real plugin's requirements name an NVIDIA adapter; a title that checks them
    // hides the option. Answer as for DLSS_G when Ray Regeneration can take the feature.
    if (feature == sl::kFeatureDLSS_RR && rrCanServe())
        return sl::Result::eOk;

    return o_slGetFeatureRequirements(feature, requirements);
}

sl::Result StreamlineHooks::hkslGetFeatureVersion(sl::Feature feature, sl::FeatureVersion& version)
{
    if (feature == sl::kFeatureDLSS_G)
    {
        version.versionSL = { State::Instance().streamlineVersion.major, State::Instance().streamlineVersion.minor,
                              State::Instance().streamlineVersion.patch };
        version.versionNGX = { 4, 2, 0 };

        return sl::Result::eOk;
    }

    return o_slGetFeatureVersion(feature, version);
}

// The most generated frames per real frame a title is ever told about through the DLSS-G
// state, whatever the FG output behind it can do: 5, i.e. 6X. sl_dlss_g.h documents
// numFramesToGenerateMax up to "a 6x multiplier: 5", and 5 is what a title sees from us at
// the default. XeFG can be raised up to 10X (opt-in, [XeFG] MaxInterpolatedFrames up to 9) - that is
// picked in OptiScaler's menu, and a DLSS-G title's own count is not what drives XeFG anyway.
// So a title never sees a maximum above 6X, nor a presented count above 6.
static constexpr int DlssgReportedMaxGenerated = 5;

static sl::Result dummy_slDLSSGGetState(const sl::ViewportHandle& viewport, sl::DLSSGState& state,
                                        const sl::DLSSGOptions* options)
{
    state.numFramesActuallyPresented = 1; // TODO: can do better
    if(state.structVersion >= 2) {
        // REPORT WHAT THE OUTPUT BACKEND CAN ACTUALLY DO, not a hard-coded 1.
        //
        // This is the stub Streamline DLSSG that stands in when the real one is absent -
        // which is every AMD machine, so every AMD user reaches this line. A title asks
        // slDLSSGGetState how many frames it is allowed to request, and answering "1"
        // meant Cyberpunk never offered multi-frame generation at all: there is no
        // OptiScaler setting that can undo the game having already decided it cannot ask.
        // The report was "I tried everything, every option in OptiScaler", and it was
        // accurate - the decision had been made one layer below any of them.
        //
        // The honest answer is whatever the FG output really supports. XeFG reports its
        // own ceiling through xefg_swapchain_properties_t::maxSupportedInterpolations
        // (3 on the machines seen so far) and FSR-FG has its own; IFGFeature caches it.
        // Clamped to at least 1 so a null or not-yet-initialised backend degrades to
        // exactly the previous behaviour rather than to zero, and to at most 5 (6X, see
        // DlssgReportedMaxGenerated) so XeFG's opt-in ceiling above 6X (up to 10X) never reaches a title.
        int maxGenerated = 1;
        if (auto* fg = State::Instance().currentFG)
            maxGenerated = fg->GetMaxInterpolationCount();
        state.numFramesToGenerateMax = std::clamp(maxGenerated, 1, DlssgReportedMaxGenerated);
        state.bIsVsyncSupportAvailable = sl::Boolean::eTrue;
    }
    state.estimatedVRAMUsageInBytes = 300 * 1024 * 1024;

    return sl::Result::eOk;
}

static sl::Result dummy_slDLSSGSetOptions(const sl::ViewportHandle& viewport, const sl::DLSSGOptions& options)
{
    return sl::Result::eOk;
}

sl::Result StreamlineHooks::hkslGetFeatureFunction(sl::Feature feature, const char* functionName, void*& function)
{
    if (feature == sl::kFeatureDLSS_G)
    {
        if (strcmp(functionName, "slDLSSGSetOptions") == 0)
        {
            function = &dummy_slDLSSGSetOptions;

            return sl::Result::eOk;
        }

        if (strcmp(functionName, "slDLSSGGetState") == 0)
        {
            function = &dummy_slDLSSGGetState;

            return sl::Result::eOk;
        }
    }

    return o_slGetFeatureFunction(feature, functionName, function);
}

// P26 (AMDNR 0.3.4): the tag hooks read Streamline tags on D3D12 only; a D3D11 / Vulkan Streamline title calls them on
// every frame once they are attached (Skyrim: 2,104 ERROR lines). They pass the tags through untouched, which is not an
// error: one line per call site for occurrences 1-5, then every 1000th with the count (RR-24's rule in FSRDFeature_Dx12).
struct SlRepeatLogGate
{
    std::atomic<uint64_t> count { 0 };

    // The occurrence number when this one is logged, else 0.
    uint64_t Next()
    {
        const uint64_t n = count.fetch_add(1, std::memory_order_relaxed) + 1;
        return n <= 5 || n % 1000 == 0 ? n : 0;
    }
};

static void LogTagPassThrough(SlRepeatLogGate& gate, const char* hook, sl::RenderAPI api)
{
    if (const uint64_t n = gate.Next())
        LOG_INFO("{}: {} title, tags passed to Streamline untouched (OptiScaler reads Streamline tags on D3D12 only); "
                 "{}",
                 hook, api == sl::RenderAPI::eD3D11 ? "D3D11" : "Vulkan",
                 n == 5 ? std::string("occurrence 5; from here every 1000th is logged")
                        : std::format("occurrence {}", n));
}

sl::Result StreamlineHooks::hkslSetTag(const sl::ViewportHandle& viewport, const sl::ResourceTag* tags,
                                       uint32_t numTags, sl::CommandBuffer* cmdBuffer)
{
    if (renderApi == sl::RenderAPI::eD3D11 || renderApi == sl::RenderAPI::eVulkan)
    {
        static SlRepeatLogGate gate;
        LogTagPassThrough(gate, "hkslSetTag", renderApi);
        return o_slSetTag(viewport, tags, numTags, cmdBuffer);
    }

    if (renderApi == sl::RenderAPI::eCount)
        LOG_WARN("Incomplete Streamline hooks");

    if (tags == nullptr)
    {
        LOG_WARN("Game trying to remove a tag");
        return o_slSetTag(viewport, tags, numTags, cmdBuffer);
    }

    // This rule is about what OptiScaler reports as FG input, and it must not gate the
    // inventory probes in the loop below. The batch it matches is a DLSS call rather than an
    // FG frame, but it is still a tagging call, and it is often the only one that carries the
    // title's RR tags - emissive, hit distance, the title's own linear depth. Returning here
    // loses the discovery of inputs the RR path later looks for, which is a silent
    // degradation: the input is simply never bound, exactly as if the title never published
    // it. Probe every batch; suppress only the tagging.
    bool skipFgTagging = false;
    if (State::Instance().activeFgInput == FGInput::DLSSG &&
        State::Instance().gameQuirks[GameQuirk::IgnoreTagsWithoutHudlessForFG])
    {
        bool hasDepth = false;
        bool hasMVs = false;
        bool hasHudless = false;

        for (uint32_t i = 0; i < numTags; i++)
        {
            if (tags[i].resource == nullptr || tags[i].resource->native == nullptr)
                continue;

            if (tags[i].type == sl::kBufferTypeDepth)
                hasDepth = true;

            if (tags[i].type == sl::kBufferTypeMotionVectors)
                hasMVs = true;

            if (tags[i].type == sl::kBufferTypeHUDLessColor)
                hasHudless = true;
        }

        // Try to skip a DLSS call. The tags still reach Streamline untouched, so only the
        // bookkeeping below is skipped.
        if (hasDepth && hasMVs && !hasHudless)
        {
            LOG_DEBUG("Skipping the FG tagging of potential DLSS resources");
            skipFgTagging = true;
        }
    }

    for (uint32_t i = 0; i < numTags; i++)
    {
        if (renderApi == sl::RenderAPI::eD3D12)
        {
            probeSLResourceTag(
                tags[i], UINT32_MAX, static_cast<uint32_t>(viewport),
                RRTagSource::SetTag);
            probeRRResourceTag(
                tags[i], UINT32_MAX, static_cast<uint32_t>(viewport),
                RRTagSource::SetTag);
        }

        const auto typeEnum = (BufferType) tags[i].type;

        if (tags[i].resource == nullptr || tags[i].resource->native == nullptr)
        {
            LOG_TRACE("Resource of type: {} is null, continuing", magic_enum::enum_name(typeEnum));
            continue;
        }

        // A skipped batch carries no hudless colour by definition, so the state repair below -
        // which exists for that resource - has nothing to act on, and what remains is the FG
        // reporting this rule is about.
        if (skipFgTagging)
            continue;

        // Cyberpunk hudless state fix for RDNA 2
        if (State::Instance().gameQuirks & GameQuirk::CyberpunkHudlessState &&
            tags[i].resource->state ==
                (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) &&
            tags[i].type == sl::kBufferTypeHUDLessColor)
        {
            tags[i].resource->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            LOG_TRACE("Changing hudless resource state");
        }

        if (State::Instance().activeFgInput == FGInput::DLSSG &&
            (tags[i].type == sl::kBufferTypeHUDLessColor || tags[i].type == sl::kBufferTypeDepth ||
             tags[i].type == sl::kBufferTypeHiResDepth || tags[i].type == sl::kBufferTypeLinearDepth ||
             tags[i].type == sl::kBufferTypeMotionVectors || tags[i].type == sl::kBufferTypeUIColorAndAlpha ||
             tags[i].type == sl::kBufferTypeBidirectionalDistortionField))
        {
            State::Instance().slFGInputs.reportResource(tags[i], (ID3D12GraphicsCommandList*) cmdBuffer, 0);
        }
        else if (State::Instance().activeFgInput == FGInput::NvngxFG)
        {
            LOG_TRACE("Tagging resource of type: {}", magic_enum::enum_name(typeEnum));
        }
    }

    auto result = o_slSetTag(viewport, tags, numTags, cmdBuffer);
    return result;
}

sl::Result StreamlineHooks::hkslSetTagForFrame(const sl::FrameToken& frame, const sl::ViewportHandle& viewport,
                                               const sl::ResourceTag* resources, uint32_t numResources,
                                               sl::CommandBuffer* cmdBuffer)
{
    if (renderApi == sl::RenderAPI::eD3D11 || renderApi == sl::RenderAPI::eVulkan)
    {
        static SlRepeatLogGate gate;
        LogTagPassThrough(gate, "hkslSetTagForFrame", renderApi);
        return o_slSetTagForFrame(frame, viewport, resources, numResources, cmdBuffer);
    }

    if (renderApi == sl::RenderAPI::eCount)
        LOG_WARN("Incomplete Streamline hooks");

    if (resources == nullptr)
    {
        LOG_WARN("Game trying to remove a tag");
        return o_slSetTagForFrame(frame, viewport, resources, numResources, cmdBuffer);
    }

    LOG_DEBUG("frameIndex: {}", static_cast<uint32_t>(frame));

    // See hkslSetTag: the FG rule suppresses OptiScaler's FG bookkeeping, not the RR tag
    // inventory, which this batch may be the only carrier of.
    bool skipFgTagging = false;
    if (State::Instance().activeFgInput == FGInput::DLSSG &&
        State::Instance().gameQuirks[GameQuirk::IgnoreTagsWithoutHudlessForFG])
    {
        bool hasDepth = false;
        bool hasMVs = false;
        bool hasHudless = false;

        for (uint32_t i = 0; i < numResources; i++)
        {
            if (resources[i].resource == nullptr || resources[i].resource->native == nullptr)
                continue;

            if (resources[i].type == sl::kBufferTypeDepth)
                hasDepth = true;

            if (resources[i].type == sl::kBufferTypeMotionVectors)
                hasMVs = true;

            if (resources[i].type == sl::kBufferTypeHUDLessColor)
                hasHudless = true;
        }

        // Try to skip a DLSS call
        if (hasDepth && hasMVs && !hasHudless)
        {
            LOG_DEBUG("Skipping the FG tagging of potential DLSS resources");
            skipFgTagging = true;
        }
    }

    for (uint32_t i = 0; i < numResources; i++)
    {
        if (renderApi == sl::RenderAPI::eD3D12)
        {
            probeSLResourceTag(
                resources[i], static_cast<uint32_t>(frame),
                static_cast<uint32_t>(viewport), RRTagSource::SetTagForFrame);
            probeRRResourceTag(
                resources[i], static_cast<uint32_t>(frame),
                static_cast<uint32_t>(viewport), RRTagSource::SetTagForFrame);
        }

        const auto typeEnum = (BufferType) resources[i].type;

        if (resources[i].resource == nullptr || resources[i].resource->native == nullptr)
        {
            LOG_TRACE("Resource of type: {} is null, continuing", magic_enum::enum_name(typeEnum));
            continue;
        }

        if (skipFgTagging)
            continue;

        if (State::Instance().activeFgInput == FGInput::DLSSG &&
            (resources[i].type == sl::kBufferTypeHUDLessColor || resources[i].type == sl::kBufferTypeDepth ||
             resources[i].type == sl::kBufferTypeHiResDepth || resources[i].type == sl::kBufferTypeLinearDepth ||
             resources[i].type == sl::kBufferTypeMotionVectors || resources[i].type == sl::kBufferTypeUIColorAndAlpha ||
             resources[i].type == sl::kBufferTypeBidirectionalDistortionField))
        {
            State::Instance().slFGInputs.reportResource(resources[i], (ID3D12GraphicsCommandList*) cmdBuffer,
                                                        (uint32_t) frame);
        }
        else if (State::Instance().activeFgInput == FGInput::NvngxFG)
        {
            LOG_TRACE("Tagging resource of type: {}", magic_enum::enum_name(typeEnum));
        }
    }

    auto result = o_slSetTagForFrame(frame, viewport, resources, numResources, cmdBuffer);
    return result;
}

// FB-L11A / RR-23 (AMDNR 0.3.4): true on the first slEvaluateFeature call of each feature in the session.
static bool FirstSlEvaluateOfFeature(sl::Feature feature)
{
    static std::mutex seenMutex;
    static std::vector<uint32_t> seen;

    std::scoped_lock lock(seenMutex);
    const auto id = static_cast<uint32_t>(feature);
    if (std::find(seen.begin(), seen.end(), id) != seen.end())
        return false;

    seen.push_back(id);
    return true;
}

sl::Result StreamlineHooks::hkslEvaluateFeature(sl::Feature feature, const sl::FrameToken& frame,
                                                const sl::BaseStructure** inputs, uint32_t numInputs,
                                                sl::CommandBuffer* cmdBuffer)
{
    // This line ran at INFO on every frame (58% of an RE Requiem log). Now INFO once per feature, DEBUG after.
    if (FirstSlEvaluateOfFeature(feature))
        LOG_INFO("slEvaluateFeature: feature {} (first call; later calls are logged at debug level)", (int) feature);
    else
        LOG_DEBUG("slEvaluateFeature: feature {}", (int) feature);
    uint32_t activeViewport = UINT32_MAX;
    if (numInputs > 0 && inputs != nullptr)
    {
        for (uint32_t i = 0; i < numInputs; ++i)
        {
            if (inputs[i] != nullptr &&
                inputs[i]->structType == sl::ViewportHandle::s_structType)
            {
                activeViewport = static_cast<uint32_t>(
                    *reinterpret_cast<const sl::ViewportHandle*>(inputs[i]));
                break;
            }
        }
    }

    // B9: a pending release of the RR tag references runs only as an outermost evaluation returns.
    const bool outermostEvaluation = g_rrActiveEvaluationFrame == UINT32_MAX;
    const ScopedRRActiveEvaluationFrame activeEvaluationFrame(
        static_cast<uint32_t>(frame), activeViewport);
    LOG_DEBUG("frameIndex: {}", static_cast<uint32_t>(frame));

    // ResourceTag inputs to slEvaluateFeature are local to this evaluation and
    // must not replace tags submitted through slSetTag/slSetTagForFrame. Overlay
    // them while the intercepted feature runs, then restore the global entries.
    constexpr size_t rrSignalCount = static_cast<size_t>(RRTaggedSignal::Count);
    std::array<bool, rrSignalCount> localTagOverrides {};
    std::array<RRTaggedResourceDiagnostic, rrSignalCount> savedGlobalDiagnostics {};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, rrSignalCount> savedGlobalResources {};
    if (renderApi == sl::RenderAPI::eD3D12 && numInputs > 0 && inputs != nullptr)
    {
        std::scoped_lock lock(rrSignalTagMutex);
        for (uint32_t i = 0; i < numInputs; ++i)
        {
            if (inputs[i] == nullptr || inputs[i]->structType != sl::ResourceTag::s_structType)
                continue;

            const auto* tag = reinterpret_cast<const sl::ResourceTag*>(inputs[i]);
            RRTaggedSignal signal;
            if (!TryGetRRTaggedSignal(tag->type, signal))
                continue;

            const size_t signalIndex = static_cast<size_t>(signal);
            if (localTagOverrides[signalIndex])
                continue;

            localTagOverrides[signalIndex] = true;
            savedGlobalDiagnostics[signalIndex] = rrSignalTagDiagnostics.resources[signalIndex];
            savedGlobalResources[signalIndex] = rrTaggedD3D12Resources[signalIndex];
        }
    }

    if (numInputs > 0 && inputs != nullptr)
    {
        for (uint32_t i = 0; i < numInputs; i++)
        {
            if (inputs[i] == nullptr)
                continue;

            if (inputs[i]->structType == sl::ResourceTag::s_structType)
            {
                auto tag = (const sl::ResourceTag*) inputs[i];

                if (renderApi == sl::RenderAPI::eD3D12)
                {
                    probeSLResourceTag(
                        *tag, static_cast<uint32_t>(frame), activeViewport,
                        RRTagSource::EvaluateFeature);
                    probeRRResourceTag(
                        *tag, static_cast<uint32_t>(frame), activeViewport,
                        RRTagSource::EvaluateFeature);
                }

                if (State::Instance().activeFgInput == FGInput::DLSSG &&
                    (tag->type == sl::kBufferTypeHUDLessColor || tag->type == sl::kBufferTypeDepth ||
                     tag->type == sl::kBufferTypeHiResDepth || tag->type == sl::kBufferTypeLinearDepth ||
                     tag->type == sl::kBufferTypeMotionVectors || tag->type == sl::kBufferTypeUIColorAndAlpha ||
                     tag->type == sl::kBufferTypeBidirectionalDistortionField))
                {
                    State::Instance().slFGInputs.reportResource(*tag, (ID3D12GraphicsCommandList*) cmdBuffer,
                                                                (uint32_t) frame);
                }
            }
        }
    }

    auto result = o_slEvaluateFeature(feature, frame, inputs, numInputs, cmdBuffer);

    bool releasedRRTags = false;
    uint32_t releasedRRTagReferences = 0;
    if (renderApi == sl::RenderAPI::eD3D12)
    {
        std::scoped_lock lock(rrSignalTagMutex);
        bool resourcesChanged = false;
        for (size_t i = 0; i < rrSignalCount; ++i)
        {
            if (localTagOverrides[i])
            {
                rrSignalTagDiagnostics.resources[i] = std::move(savedGlobalDiagnostics[i]);
                rrTaggedD3D12Resources[i] = std::move(savedGlobalResources[i]);
                resourcesChanged = true;
            }

            const auto& diagnostic = rrSignalTagDiagnostics.resources[i];
            const bool belongsToEvaluation =
                diagnostic.viewport == activeViewport &&
                (diagnostic.frameIndex == UINT32_MAX ||
                 diagnostic.frameIndex == static_cast<uint32_t>(frame));
            if (belongsToEvaluation &&
                diagnostic.lifecycle == sl::ResourceLifecycle::eValidUntilEvaluate &&
                rrTaggedD3D12Resources[i])
            {
                // The declared lifetime ends as this evaluation returns. Keep
                // the last metadata for diagnostics, but release the retained
                // native resource so it cannot be rebound by a later frame.
                rrTaggedD3D12Resources[i].Reset();
                resourcesChanged = true;
            }
        }

        if (resourcesChanged)
            ++rrSignalTagDiagnostics.generation;

        // B9 (AMDNR 0.3.4): the last FSR_RR feature went (Ray Regeneration off, the handle released, or the
        // fallback's switch to FSR) and none was created since, not even inside this evaluation (a title's own
        // resize re-create). This frame's tags have no reader left, so the game textures they hold are released
        // once. A re-create never gets here with a count of zero, so it keeps the tags it was given.
        if (outermostEvaluation && g_rrTagReleasePending.load(std::memory_order_acquire) &&
            g_rrTagConsumers.load(std::memory_order_acquire) == 0)
        {
            g_rrTagReleasePending.store(false, std::memory_order_release);
            for (const auto& resource : rrTaggedD3D12Resources)
                releasedRRTagReferences += resource ? 1 : 0;
            const uint64_t generation = rrSignalTagDiagnostics.generation;
            rrSignalTagDiagnostics = {};
            rrSignalTagDiagnostics.generation = generation + 1;
            rrTaggedD3D12Resources = {};
            rrEvaluateTagLastLogged = {};
            releasedRRTags = true;
        }
    }

    if (releasedRRTags)
        LOG_INFO("FSR-RR: no Ray Regeneration feature is alive after slEvaluateFeature (frame {}); released the "
                 "Streamline RR tags ({} game texture references)",
                 static_cast<uint32_t>(frame), releasedRRTagReferences);

    return result;
}

sl::Result StreamlineHooks::hkslAllocateResources(sl::CommandBuffer* cmdBuffer, sl::Feature feature,
                                                  const sl::ViewportHandle& viewport)
{
    LOG_FUNC();
    auto result = o_slAllocateResources(cmdBuffer, feature, viewport);
    return result;
}

sl::Result StreamlineHooks::hkslGetNativeInterface(void* proxyInterface, void** baseInterface)
{
    LOG_FUNC();
    auto result = o_slGetNativeInterface(proxyInterface, baseInterface);
    return result;
}

sl::Result StreamlineHooks::hkslSetD3DDevice(void* d3dDevice)
{
    LOG_FUNC();
    auto result = o_slSetD3DDevice(d3dDevice);
    return result;
}

void StreamlineHooks::streamlineLogCallback_sl1(sl1::LogType type, const char* msg)
{
    if (msg == nullptr)
        return;

    char* trimmed_msg = trimStreamlineLog(msg);

    if (trimmed_msg != nullptr)
    {
        switch (type)
        {
        case sl1::LogType::eLogTypeWarn:
            LOG_WARN("{}", trimmed_msg);
            break;
        case sl1::LogType::eLogTypeInfo:
            LOG_INFO("{}", trimmed_msg);
            break;
        case sl1::LogType::eLogTypeError:
            LOG_ERROR("{}", trimmed_msg);
            break;
        case sl1::LogType::eLogTypeCount:
            LOG_ERROR("{}", trimmed_msg);
            break;
        }

        free(trimmed_msg);
    }

    if (o_logCallback_sl1)
        o_logCallback_sl1(type, msg);
}

bool StreamlineHooks::hkslInit_sl1(const sl1::Preferences& pref, int applicationId)
{
    LOG_FUNC();

    sl1::Preferences localPref = pref;

    if (localPref.logMessageCallback != &streamlineLogCallback_sl1)
        o_logCallback_sl1 = localPref.logMessageCallback;
    localPref.logLevel = sl1::LogLevel::eLogLevelCount;
    localPref.logMessageCallback = &streamlineLogCallback_sl1;

    // F3 for Streamline 1, whose slInit answers only true or false.
    g_slInitGpusLogged.store(false);
    const auto start = std::chrono::steady_clock::now();
    bool ok;
    {
        ScopedSlInitWindow window;
        ok = o_slInit_sl1(localPref, applicationId);
    }
    const auto ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());

    RecordSlInitResult(ok ? 0 : 1, ok ? "ok" : "failed", ms, true);

    if (ok)
        LOG_INFO("slInit (Streamline 1) returned true in {} ms", ms);
    else
        LOG_ERROR("slInit (Streamline 1) returned false in {} ms", ms);

    return ok;
}

bool StreamlineHooks::hkslSetTag_sl1(const sl1::Resource* resource, sl1::BufferType tag, uint32_t id,
                                     const sl1::Extent* extent)
{
    if (IsSL1AndFGActive())
        State::Instance().s_sl1FGInputs.setTag(resource, tag, id, extent);

    return o_slSetTag_sl1(resource, tag, id, extent);
}

bool StreamlineHooks::hkslSetConstants_sl1(const sl1::Constants& values, uint32_t frameIndex, uint32_t id)
{
    std::scoped_lock lock(setConstantsMutex);

    LOG_TRACE("SL1 slSetConstants frameIndex: {}, id: {}", frameIndex, id);
    g_slSetConstantsSl1Calls.fetch_add(1, std::memory_order_relaxed);

    if (IsSL1AndFGActive())
        State::Instance().s_sl1FGInputs.setConstants(values, frameIndex, id);

    return o_slSetConstants_interposer_sl1(values, frameIndex, id);
}

bool StreamlineHooks::hkslEvaluateFeature_sl1(sl1::CommandBuffer* cmdBuffer, sl1::Feature feature, uint32_t frameIndex,
                                              uint32_t id)
{
    LOG_TRACE("SL1 slEvaluateFeature feature: {}, frameIndex: {}, id: {}", magic_enum::enum_name(feature), frameIndex,
              id);

    if (IsSL1AndFGActive() && feature == sl1::Feature::eFeatureReflex)
    {
        const auto marker = (sl1::ReflexMarker) id;

        if (marker == sl1::ReflexMarker::eReflexMarkerRenderSubmitStart)
        {
            State::Instance().s_sl1FGInputs.evaluateState();
            State::Instance().s_sl1FGInputs.evaluateFeature(cmdBuffer, feature, frameIndex, id);
        }
        else if (marker == sl1::ReflexMarker::eReflexMarkerPresentStart)
        {
            State::Instance().s_sl1FGInputs.markPresent(frameIndex);
        }
    }

    return o_slEvaluateFeature_sl1(cmdBuffer, feature, frameIndex, id);
}

void StreamlineHooks::hookSystemCaps(sl::param::IParameters* params)
{
    // RE-READ ON EVERY CALL (0.3.3, F6). The pointer used to be fetched once and kept until
    // unhookCommon succeeded. Streamline frees plugins inside slInit, and a second slInit or a
    // plugin reload could leave the kept pointer aimed at memory that is gone - which setArch then
    // writes through, and an access violation inside slInit is error 0x18. Every plugin load hands
    // us the parameters, so the current pointer costs one lookup; within one slInit it is the same
    // pointer every time, so the normal case is unchanged. A lookup that finds nothing leaves no
    // pointer, and getSystemCapsArch / setArch then do what they did before the first lookup.
    if (State::Instance().streamlineVersion.major > 1)
    {
        SystemCaps* caps = nullptr;
        if (params != nullptr)
            sl::param::getPointerParam(params, sl::param::common::kSystemCaps, &caps);
        systemCaps = caps;
    }
    else if (State::Instance().streamlineVersion.major == 1)
    {
        // This should be Streamline 1.5 as previous versions don't even have slOnPluginLoad
        LOG_TRACE("Attempting to get system caps for Streamline v1, this could fail depending on the exact version");
        SystemCapsSl15* caps = nullptr;
        if (params != nullptr)
            sl::param::getPointerParam(params, sl::param::common::kSystemCaps, &caps);
        systemCapsSl15 = caps;
    }
}

uint32_t StreamlineHooks::getSystemCapsArch(SystemCaps* altSystemCaps)
{
    uint32_t highestArch = 0;

    auto primaryGpu = IdentifyGpu::getPrimaryGpu();
    if (!fakenvapi::isUsingAsMainNvapi() && primaryGpu.vendorId == VendorId::Nvidia)
    {
        if (State::Instance().streamlineVersion.major > 1)
        {
            auto caps = altSystemCaps != nullptr ? altSystemCaps : systemCaps;
            if (caps)
            {
                for (auto& adapter : caps->adapters)
                {
                    if (adapter.architecture > highestArch)
                        highestArch = adapter.architecture;
                }
            }
        }
        else if (State::Instance().streamlineVersion.major == 1)
        {
            if (systemCapsSl15)
            {
                for (uint32_t i = 0; i < systemCapsSl15->gpuCount; i++)
                {
                    if (systemCapsSl15->architecture[i] > highestArch)
                        highestArch = systemCapsSl15->architecture[i];
                }
            }
        }
    }

    // By default spoof Pascal, gets Reflex but not DLSSD
    // Could be problematic if not using fakenvapi but nvapi might not be initialized yet
    if (highestArch == 0)
        highestArch = NV_GPU_ARCHITECTURE_GP100;

    return highestArch;
}

void StreamlineHooks::setArch(uint32_t arch, SystemCaps* altSystemCaps)
{
    auto primaryGpu = IdentifyGpu::getPrimaryGpu();

    // altSystemCaps has to be sl2+
    if (State::Instance().streamlineVersion.major > 1 || altSystemCaps)
    {
        // Assumes that altCaps are always for SL2+
        auto caps = altSystemCaps != nullptr ? altSystemCaps : systemCaps;
        if (caps)
        {
            for (uint32_t i = 0; i < caps->gpuCount; i++)
            {
                caps->adapters[i].architecture = arch;
                caps->adapters[i].vendor = VendorId::Nvidia;
            }

            if (fakenvapi::isUsingAsMainNvapi() || primaryGpu.vendorId != VendorId::Nvidia)
                caps->driverVersionMajor = 999;

            caps->hwsSupported = true;
        }
    }
    else if (State::Instance().streamlineVersion.major == 1)
    {
        if (systemCapsSl15)
        {
            for (uint32_t i = 0; i < systemCapsSl15->gpuCount; i++)
                systemCapsSl15->architecture[i] = arch;

            if (fakenvapi::isUsingAsMainNvapi() || primaryGpu.vendorId != VendorId::Nvidia)
                systemCapsSl15->driverVersionMajor = 999;

            systemCapsSl15->hwSchedulingEnabled = true;
        }
    }
}

// Spoof arch based on feature and current arch
void StreamlineHooks::spoofArch(uint32_t currentArch, sl::Feature feature, SystemCaps* altSystemCaps)
{
    constexpr uint32_t maxArch = 0xFFFFFFFF;

    // Don't change arch for DLSS/DLSSD with turing and above
    if (feature == sl::kFeatureDLSS)
    {
        if (currentArch < NV_GPU_ARCHITECTURE_TU100)
            return setArch(maxArch, altSystemCaps);
    }

    // DLSSD was never spoofed, because nothing could run it on this hardware. FSR Ray
    // Regeneration can, on RDNA 4 with the 1.2 denoiser - and then sl.dlss_d has to see
    // an RTX-class adapter at load or it refuses and the title never creates the feature
    // Ray Regeneration is driven from. Same rule as DLSS: Turing or above.
    else if (feature == sl::kFeatureDLSS_RR)
    {
        if (rrCanServe() && currentArch < NV_GPU_ARCHITECTURE_TU100)
            return setArch(maxArch, altSystemCaps);
        return;
    }

    // Don't change arch for DLSSG with ada and above
    else if (feature == sl::kFeatureDLSS_G)
    {
        if (State::Instance().activeFgNvngx != FGNvngxReplacement::None)
        {
            if (!Nvngx_FG::isDx12Available() && !Nvngx_FG::isVulkanAvailable())
                return setArch(0);
        }

        if (currentArch < NV_GPU_ARCHITECTURE_AD100)
            return setArch(maxArch, altSystemCaps);
    }

    else if (feature == sl::kFeatureReflex || feature == sl::kFeaturePCL)
    {
        if (fakenvapi::isUsingAsMainNvapi())
            return setArch(maxArch, altSystemCaps);
    }
}

bool StreamlineHooks::hkdlss_slOnPluginLoad(sl::param::IParameters* params, const char* loaderJSON,
                                            const char** pluginJSON)
{
    LOG_FUNC();

    // TODO: do it better than "static" and hoping for the best
    static std::string config;

    // Read once, so the restore below always matches the spoof.
    const bool spoofing = Config::Instance()->StreamlineSpoofing.value_or_default();
    LogPluginLoadEntry("sl.dlss", spoofing);

    uint32_t currentArch = 0;
    if (spoofing)
    {
        hookSystemCaps(params);
        currentArch = getSystemCapsArch();
        logCachedGpusForSlInit();
        logSystemCapsForSlInit("sl.dlss", "before the spoof");
        spoofArch(currentArch, sl::kFeatureDLSS);
        logSystemCapsForSlInit("sl.dlss", "during the load");
    }

    auto result = o_dlss_slOnPluginLoad(params, loaderJSON, pluginJSON);

    if (spoofing)
    {
        setArch(currentArch);
        logSystemCapsForSlInit("sl.dlss", "after the restore");
    }

    return PatchPluginJson(
        "sl.dlss", result, pluginJSON, config, PluginJsonOnFailure::SkipPlugin,
        [](nlohmann::json& configJson)
        {
            auto primaryGpu = IdentifyGpu::getPrimaryGpu();
            if (primaryGpu.vendorId != VendorId::Nvidia || !primaryGpu.dlssCapable)
            {
                if (configJson.contains("/external/vk/instance/extensions"_json_pointer))
                    configJson["external"]["vk"]["instance"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/extensions"_json_pointer))
                    configJson["external"]["vk"]["device"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/1.2_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.2_features"].clear();

                if (configJson.contains("/external/vk/device/1.3_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.3_features"].clear();
            }

            PatchSL1PluginJson(configJson);
        });
}

sl::Result StreamlineHooks::hkslDLSSGetOptimalSettings(const sl::DLSSOptions& options,
                                                       sl::DLSSOptimalSettings& settings)
{
    static bool modesBroken = false;

    auto localOptions = options;

    if (localOptions.mode == sl::DLSSMode::eOff)
        modesBroken = true;

    if (modesBroken)
    {
        if (localOptions.mode == sl::DLSSMode::eMaxPerformance)
            localOptions.mode = sl::DLSSMode::eUltraPerformance;
        else if (localOptions.mode == sl::DLSSMode::eBalanced)
            localOptions.mode = sl::DLSSMode::eMaxPerformance;
        else if (localOptions.mode == sl::DLSSMode::eMaxQuality)
            localOptions.mode = sl::DLSSMode::eBalanced;
        else if (localOptions.mode == sl::DLSSMode::eUltraQuality)
            localOptions.mode = sl::DLSSMode::eMaxQuality;
        else if (localOptions.mode == sl::DLSSMode::eUltraPerformance)
            localOptions.mode = sl::DLSSMode::eDLAA;
    }

    return o_slDLSSGetOptimalSettings(localOptions, settings);
}

bool StreamlineHooks::hkdlssg_slOnPluginLoad(sl::param::IParameters* params, const char* loaderJSON,
                                             const char** pluginJSON)
{
    LOG_FUNC();

    // TODO: do it better than "static" and hoping for the best
    static std::string config;

    bool shouldSpoofArch =
        Config::Instance()->StreamlineSpoofing.value_or_default() &&
        (State::Instance().activeFgInput == FGInput::NvngxFG || State::Instance().activeFgInput == FGInput::DLSSG);

    LogPluginLoadEntry("sl.dlss_g", shouldSpoofArch);

    uint32_t currentArch = 0;
    if (shouldSpoofArch)
    {
        hookSystemCaps(params);
        currentArch = getSystemCapsArch();
        logCachedGpusForSlInit();
        logSystemCapsForSlInit("sl.dlss_g", "before the spoof");
        spoofArch(currentArch, sl::kFeatureDLSS_G);
        logSystemCapsForSlInit("sl.dlss_g", "during the load");
    }

    auto result = o_dlssg_slOnPluginLoad(params, loaderJSON, pluginJSON);

    if (shouldSpoofArch)
    {
        setArch(currentArch);
        logSystemCapsForSlInit("sl.dlss_g", "after the restore");
    }

    return PatchPluginJson(
        "sl.dlss_g", result, pluginJSON, config, PluginJsonOnFailure::SkipPlugin,
        [](nlohmann::json& configJson)
        {
            // Kill the DLSSG streamline swapchain hooks
            if (State::Instance().activeFgInput == FGInput::DLSSG ||
                State::Instance().activeFgOutput == FGOutput::DLSSG)
            {
                if (configJson.contains("/hooks"_json_pointer))
                    configJson["hooks"].clear();

                if (configJson.contains("/exclusive_hooks"_json_pointer))
                    configJson["exclusive_hooks"].clear();

                if (configJson.contains("/external/feature/tags"_json_pointer))
                    configJson["external"]["feature"]["tags"].clear(); // We handle the DLSSG resources

                if (configJson.contains("/external/vk/device/queues/compute/count"_json_pointer))
                    configJson["external"]["vk"]["device"]["queues"]["compute"]["count"] = 0;

                if (configJson.contains("/external/vk/device/queues/graphics/count"_json_pointer))
                    configJson["external"]["vk"]["device"]["queues"]["graphics"]["count"] = 0;

                if (configJson.contains("/external/vk/device/1.2_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.2_features"].clear();

                if (configJson.contains("/external/vk/device/1.3_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.3_features"].clear();
            }

            if (State::Instance().activeFgInput == FGInput::DLSSG ||
                State::Instance().activeFgInput == FGInput::NvngxFG)
            {
                if (configJson.contains("/vsync/supported"_json_pointer))
                    configJson["vsync"]["supported"] = true; // disable eVSyncOffRequired

                if (configJson.contains("/external/hws/required"_json_pointer))
                    configJson["external"]["hws"]["required"] = false; // disable eHardwareSchedulingRequired

                // if (configJson.contains("/external/vk/opticalflow/supported"_json_pointer))
                //     configJson["external"]["vk"]["opticalflow"]["supported"] = true;
            }

            auto primaryGpu = IdentifyGpu::getPrimaryGpu();
            if (primaryGpu.vendorId != VendorId::Nvidia || !primaryGpu.dlssCapable)
            {
                if (configJson.contains("/external/vk/instance/extensions"_json_pointer))
                    configJson["external"]["vk"]["instance"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/extensions"_json_pointer))
                    configJson["external"]["vk"]["device"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/1.2_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.2_features"].clear();

                if (configJson.contains("/external/vk/device/1.3_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.3_features"].clear();
            }

            PatchSL1PluginJson(configJson);
        });
}

const char* StreamlineHooks::hkdlssg_slGetPluginJSONConfig_sl1()
{
    static std::string patchedConfig;

    const char* originalConfig = o_dlssg_slGetPluginJSONConfig_sl1();

    if (originalConfig == nullptr)
        return originalConfig;

    try
    {
        auto configJson = nlohmann::json::parse(originalConfig);

        LOG_DEBUG("SL1 DLSSG JSON before patch: {}", configJson.dump());

        PatchSL1PluginJson(configJson);
        // RemoveSL1DLSSGHookEntriesRecursive(configJson);

        patchedConfig = configJson.dump();

        LOG_DEBUG("SL1 DLSSG JSON after patch: {}", patchedConfig);

        return patchedConfig.c_str();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to patch SL1 DLSSG JSON config: {}", e.what());
        return originalConfig;
    }
}

bool StreamlineHooks::hklocal_dlssg_slOnPluginLoad(sl::param::IParameters* params, const char* loaderJSON,
                                                   const char** pluginJSON)
{
    LOG_FUNC();

    // TODO: do it better than "static" and hoping for the best
    static std::string config;

    bool shouldSpoofArch = Config::Instance()->StreamlineSpoofing.value_or_default();
    LogPluginLoadEntry("sl.dlss_g (OptiScaler\\streamline)", shouldSpoofArch);

    uint32_t currentArch = 0;
    SystemCaps* localSystemCaps = nullptr;
    if (shouldSpoofArch)
    {
        if (params != nullptr)
            sl::param::getPointerParam(params, sl::param::common::kSystemCaps, &localSystemCaps);

        if (localSystemCaps)
        {
            currentArch = getSystemCapsArch(localSystemCaps);
            logCachedGpusForSlInit();
            logSystemCapsForSlInit("sl.dlss_g (OptiScaler\\streamline)", "before the spoof", localSystemCaps);
            spoofArch(currentArch, sl::kFeatureDLSS_G, localSystemCaps);
            logSystemCapsForSlInit("sl.dlss_g (OptiScaler\\streamline)", "during the load", localSystemCaps);
        }
    }

    auto result = o_local_dlssg_slOnPluginLoad(params, loaderJSON, pluginJSON);

    if (shouldSpoofArch && localSystemCaps)
    {
        setArch(currentArch, localSystemCaps);
        logSystemCapsForSlInit("sl.dlss_g (OptiScaler\\streamline)", "after the restore", localSystemCaps);
    }

    return PatchPluginJson(
        "sl.dlss_g (OptiScaler\\streamline)", result, pluginJSON, config, PluginJsonOnFailure::SkipPlugin,
        [](nlohmann::json& configJson)
        {
            if (configJson.contains("/external/hws/required"_json_pointer))
                configJson["external"]["hws"]["required"] = false; // disable eHardwareSchedulingRequired

            auto primaryGpu = IdentifyGpu::getPrimaryGpu();
            if (primaryGpu.vendorId != VendorId::Nvidia || !primaryGpu.dlssCapable)
            {
                if (configJson.contains("/external/vk/instance/extensions"_json_pointer))
                    configJson["external"]["vk"]["instance"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/extensions"_json_pointer))
                    configJson["external"]["vk"]["device"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/1.2_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.2_features"].clear();

                if (configJson.contains("/external/vk/device/1.3_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.3_features"].clear();
            }
        });
}

sl::Result StreamlineHooks::hkslSetConstants(const sl::Constants& values, const sl::FrameToken& frame,
                                             const sl::ViewportHandle& viewport)
{
    LOG_TRACE("called with frameIndex: {}, viewport: {}", (unsigned int) frame, (unsigned int) viewport);

    g_slSetConstantsCalls.fetch_add(1, std::memory_order_relaxed);
    g_slSetConstantsLastPresent.store(State::Instance().frameCount, std::memory_order_relaxed);

    {
        std::scoped_lock lock(setConstantsMutex);
        State::Instance().slLastConstants = values;
        State::Instance().slLastConstantsFrame = static_cast<uint32_t>(frame);
        State::Instance().slLastConstantsViewport = static_cast<uint32_t>(viewport);

        // SAT-P1: nothing is recorded unless the experiment is on.
        if (Config::Instance()->FfxDenoiserNgxDirectSLConstants.value_or_default())
        {
            SLConstantsRingEntry& entry = g_slConstantsRing[g_slConstantsRingNext];
            entry.constants = values;
            entry.frameIndex = static_cast<uint32_t>(frame);
            entry.viewport = static_cast<uint32_t>(viewport);
            entry.present = State::Instance().frameCount;
            entry.valid = true;
            g_slConstantsRingNext = (g_slConstantsRingNext + 1) % static_cast<uint32_t>(g_slConstantsRing.size());
        }

        // FG BOOKKEEPING ONLY UNDER THE FG INPUTS THAT OWN IT. Upstream attaches this hook
        // solely under DLSSG / NvngxFG input, so the body never needed a gate. The Ray
        // Regeneration port attaches it whenever the denoiser DLL is present - every install
        // of this build - because RR reads slLastConstants above. But setConstants() also
        // starts a new FG frame (Sl_Inputs_Dx12::CheckForFrame -> StartNewFrame), and in a
        // Streamline title driven by FSR-FG or upscaler input that was a SECOND frame counter
        // running beside the real input's: XeFG then saw "Depth or Velocity is not ready" on
        // every frame and the provider refused tags as "already tagged" (Spider-Man 2, FSR 3.1
        // FG -> XeFG, worked on 0.1.0 where this hook was not attached).
        const auto fgInput = State::Instance().activeFgInput;
        if (fgInput == FGInput::DLSSG || fgInput == FGInput::NvngxFG)
            State::Instance().slFGInputs.setConstants(values, (uint32_t) frame);
    }

    return o_slSetConstants(values, frame, viewport);
}

bool StreamlineHooks::hkcommon_slOnPluginLoad(sl::param::IParameters* params, const char* loaderJSON,
                                              const char** pluginJSON)
{
    LOG_FUNC();

    // TODO: do it better than "static" and hoping for the best
    static std::string config;

    LogPluginLoadEntry("sl.common", false);

    auto result = o_common_slOnPluginLoad(params, loaderJSON, pluginJSON);

    // Fail OPEN: sl.common is the plugin nothing else loads without, so a JSON this cannot read is
    // left to Streamline as it came, and the interposer's file version stays in State. A JSON that
    // was never set is the exception: PatchPluginJson reports that load as failed, as for any plugin.
    return PatchPluginJson(
        "sl.common", result, pluginJSON, config, PluginJsonOnFailure::KeepStreamlines,
        [](nlohmann::json& configJson)
        {
            // Grab a version of the potentially updated sl.common
            // Opti assumes that all plugins will have this version
            //
            // Into temporaries first: a JSON missing one of the three fields leaves the interposer's
            // file version in place, not a mix of both. Committed before PatchSL1PluginJson, which
            // reads the major version, as before.
            unsigned int versionMajor = 0, versionMinor = 0, versionBuild = 0;
            configJson.at("version").at("major").get_to(versionMajor);
            configJson.at("version").at("minor").get_to(versionMinor);
            configJson.at("version").at("build").get_to(versionBuild);

            auto& slVersion = State::Instance().streamlineVersion;
            slVersion.major = versionMajor;
            slVersion.minor = versionMinor;
            slVersion.patch = versionBuild;

            // Completely disables Streamline hooks
            // if (true)
            //    configJson["hooks"].clear();
            //    configJson["exclusive_hooks"].clear();
            //}

            PatchSL1PluginJson(configJson);
        });
}

sl::Result StreamlineHooks::hkslDLSSGSetOptions(const sl::ViewportHandle& viewport, const sl::DLSSGOptions& options)
{
    lastDlssgViewport = viewport;
    lastDlssgOptions = options;

    // Avoid reading past the game's struct's size
    sl::DLSSGOptions newOptions {};
    auto newStructVer = newOptions.structVersion;

    if (options.structVersion == 1)
        memcpy(&newOptions, &options, 104);
    else if (options.structVersion == 2 || options.structVersion == 3)
        memcpy(&newOptions, &options, 112);
    else if (options.structVersion == 4 || options.structVersion == 5)
        memcpy(&newOptions, &options, 120);
    else
        newOptions = options;

    newOptions.structVersion = newStructVer;

    auto& state = State::Instance();

    // Disable game's DLSSG when we are trying to create our own instance of DLSSG
    if (state.activeFgInput != FGInput::DLSSG && state.activeFgOutput == FGOutput::DLSSG)
    {
        newOptions.mode = sl::DLSSGMode::eOff;
        return o_slDLSSGSetOptions(viewport, newOptions);
    }

    // Make DLSSG auto always mean On
    if (newOptions.mode == sl::DLSSGMode::eAuto)
        newOptions.mode = sl::DLSSGMode::eOn;

    const auto dlssgPotentiallyActive = newOptions.mode == sl::DLSSGMode::eOn ||
                                        newOptions.mode == sl::DLSSGMode::eAuto ||
                                        newOptions.mode == sl::DLSSGMode::eDynamic;

    bool enableDynamicMode = Config::Instance()->FGDLSSGOverrideForceDMFG.value_or_default() &&
                             state.dlssgGameDMFGSupported && dlssgPotentiallyActive;

    if (enableDynamicMode)
    {
        newOptions.mode = sl::DLSSGMode::eDynamic;
    }

    if (newOptions.mode == sl::DLSSGMode::eDynamic && Config::Instance()->FGDLSSGFramerateTargetDMFG.has_value())
    {
        newOptions.dynamicTargetFrameRate = Config::Instance()->FGDLSSGFramerateTargetDMFG.value();
    }

    if (state.swapchainApi == API::Vulkan)
    {
        // Only matters for Vulkan, DX doesn't use this delay
        if (dlssgPotentiallyActive && !MenuOverlayBase::IsVisible())
            state.delayMenuRenderBy = 10;

        if (MenuOverlayBase::IsVisible())
        {
            newOptions.mode = sl::DLSSGMode::eOff;
            newOptions.flags |= sl::DLSSGFlags::eRetainResourcesWhenOff;
            ReflexHooks::setDlssgFrameCount(0);
        }
    }

    LOG_TRACE("DLSSG Modified Mode: {}", magic_enum::enum_name(newOptions.mode));

    if (dlssgPotentiallyActive && state.streamlineVersion >= feature_version { 2, 7, 1 })
    {
        // Populate dlssgMfgMax once
        if (!state.dlssgMfgMax.has_value())
        {
            sl::DLSSGState localState {};
            sl::DLSSGOptions localOptions {};
            if (o_slDLSSGGetState(viewport, localState, &localOptions) == sl::Result::eOk &&
                localState.numFramesToGenerateMax > 0 && localState.numFramesToGenerateMax < 6)
            {
                state.dlssgMfgMax = localState.numFramesToGenerateMax;
                LOG_TRACE("Saving original numFramesToGenerateMax: {}", state.dlssgMfgMax.value());

                if (Config::Instance()->FGDLSSGOverrideInterpolationCount.has_value() &&
                    Config::Instance()->FGDLSSGOverrideInterpolationCount.value() > state.dlssgMfgMax.value())
                {
                    Config::Instance()->FGDLSSGOverrideInterpolationCount.set_volatile_value(state.dlssgMfgMax.value());
                }
            }
        }

        // Won't take effect with Dynamic
        if (Config::Instance()->FGDLSSGOverrideInterpolationCount.has_value())
        {
            auto overrideCount = Config::Instance()->FGDLSSGOverrideInterpolationCount.value();
            if (overrideCount != 0)
                newOptions.numFramesToGenerate = overrideCount;
            else if (!enableDynamicMode)
                newOptions.mode = sl::DLSSGMode::eOff;
        }
    }

    state.dlssgLastSetMode = newOptions.mode;

    const auto result = o_slDLSSGSetOptions(viewport, newOptions);
    static thread_local int loggedMode = -1;
    static thread_local uint32_t loggedCount = ~0u;
    if (loggedMode != static_cast<int>(newOptions.mode) || loggedCount != newOptions.numFramesToGenerate ||
        result != sl::Result::eOk)
    {
        LOG_INFO("DLSSG options: requested={} forwarded={} generatedFrames={} result={}",
                 static_cast<int>(options.mode), static_cast<int>(newOptions.mode),
                 newOptions.numFramesToGenerate, static_cast<int>(result));
        loggedMode = static_cast<int>(newOptions.mode);
        loggedCount = newOptions.numFramesToGenerate;
    }
    return result;
}

sl::Result StreamlineHooks::hkslDLSSGGetState(const sl::ViewportHandle& viewport, sl::DLSSGState& state,
                                              const sl::DLSSGOptions* options)
{
    sl::Result result {};

    const auto originalStructVersion = state.structVersion;
    if (originalStructVersion < 4)
    {
        sl::DLSSGState newState {};

        // We might be feeding a newer struct to an older SL but that seems to work just fine for this Get function
        result = o_slDLSSGGetState(viewport, dynamic_cast<sl::DLSSGState&>(newState), options);
        if(result != sl::Result::eOk) return result;

        // Copy back data to game's struct
        memcpy(&state, &newState, 56); // struct ver 1 size
        state.structVersion = originalStructVersion;

        if (originalStructVersion >= 2)
        {
            state.numFramesToGenerateMax = newState.numFramesToGenerateMax;
            state.bReserved4 = newState.bReserved4;
            state.bIsVsyncSupportAvailable = newState.bIsVsyncSupportAvailable;
        }

        if (originalStructVersion >= 3)
        {
            state.inputsProcessingCompletionFence = newState.inputsProcessingCompletionFence;
            state.lastPresentInputsProcessingCompletionFenceValue =
                newState.lastPresentInputsProcessingCompletionFenceValue;
        }

        State::Instance().dlssgGameDMFGSupported = newState.bIsDynamicMFGSupported == sl::eTrue;
    }
    else
    {
        result = o_slDLSSGGetState(viewport, state, options);
        if(result != sl::Result::eOk) return result;
        State::Instance().dlssgGameDMFGSupported = state.bIsDynamicMFGSupported == sl::eTrue;
    }

    if (!State::Instance().dlssgGameDMFGSupported)
    {
        Config::Instance()->FGDLSSGOverrideForceDMFG.set_volatile_value(false);
    }

    auto& optiState = State::Instance();

    if (optiState.streamlineVersion >= feature_version { 2, 7, 1 })
    {
        if (!optiState.dlssgMfgMax.has_value())
        {
            sl::DLSSGState localState {};
            sl::DLSSGOptions localOptions {};
            if (o_slDLSSGGetState(viewport, localState, &localOptions) == sl::Result::eOk &&
                localState.numFramesToGenerateMax > 0 && localState.numFramesToGenerateMax < 6)
            {
                optiState.dlssgMfgMax = localState.numFramesToGenerateMax;
                LOG_TRACE("Saving original numFramesToGenerateMax: {}", optiState.dlssgMfgMax.value());

                if (Config::Instance()->FGDLSSGOverrideInterpolationCount.has_value() &&
                    Config::Instance()->FGDLSSGOverrideInterpolationCount.value() > optiState.dlssgMfgMax.value())
                {
                    Config::Instance()->FGDLSSGOverrideInterpolationCount.set_volatile_value(optiState.dlssgMfgMax.value());
                }
            }
        }
    }

    if (optiState.activeFgInput == FGInput::DLSSG)
    {
        auto fg = optiState.currentFG;

        if (fg != nullptr)
        {
            if (options != nullptr && options->flags & sl::DLSSGFlags::eRequestVRAMEstimate)
                state.estimatedVRAMUsageInBytes = static_cast<uint64_t>(256 * 1024) * 1024;

            if (fg->IsActive() && !fg->IsPaused())
            {
                // Held to the 6X a title is ever told about (DlssgReportedMaxGenerated): at
                // XeFG's opt-in 7X-10X the title would otherwise see more frames presented
                // than any maximum it was given.
                state.numFramesActuallyPresented =
                    (std::min)(fg->GetInterpolatedFrameCount() + 1, static_cast<UINT>(DlssgReportedMaxGenerated + 1));
            }
            else
            {
                state.numFramesActuallyPresented = 1;
            }
        }
        else
        {
            state.numFramesActuallyPresented = 1;
        }

        if(originalStructVersion >= 2) state.numFramesToGenerateMax = 1;

        LOG_DEBUG("Status: {}, numFramesActuallyPresented: {}", magic_enum::enum_name(state.status),
                  state.numFramesActuallyPresented);
    }

    return result;
}

bool StreamlineHooks::hkreflex_slOnPluginLoad(sl::param::IParameters* params, const char* loaderJSON,
                                              const char** pluginJSON)
{
    LOG_FUNC();

    // TODO: do it better than "static" and hoping for the best
    static std::string config;

    // Read once, so the restore below always matches the spoof.
    const bool spoofing = Config::Instance()->StreamlineSpoofing.value_or_default();
    LogPluginLoadEntry("sl.reflex", spoofing);

    uint32_t currentArch = 0;
    if (spoofing)
    {
        hookSystemCaps(params);
        currentArch = getSystemCapsArch();
        logCachedGpusForSlInit();
        logSystemCapsForSlInit("sl.reflex", "before the spoof");
        spoofArch(currentArch, sl::kFeatureReflex);
        logSystemCapsForSlInit("sl.reflex", "during the load");
    }

    auto result = o_reflex_slOnPluginLoad(params, loaderJSON, pluginJSON);

    if (spoofing)
    {
        setArch(currentArch);
        logSystemCapsForSlInit("sl.reflex", "after the restore");
    }

    return PatchPluginJson(
        "sl.reflex", result, pluginJSON, config, PluginJsonOnFailure::SkipPlugin,
        [](nlohmann::json& configJson)
        {
            auto primaryGpu = IdentifyGpu::getPrimaryGpu();
            if (primaryGpu.vendorId != VendorId::Nvidia || !primaryGpu.dlssCapable)
            {
                if (configJson.contains("/external/vk/instance/extensions"_json_pointer))
                    configJson["external"]["vk"]["instance"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/extensions"_json_pointer))
                    configJson["external"]["vk"]["device"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/1.2_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.2_features"].clear();

                if (configJson.contains("/external/vk/device/1.3_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.3_features"].clear();
            }

            PatchSL1PluginJson(configJson);
        });
}

sl::Result StreamlineHooks::hkslReflexSetOptions(const sl::ReflexOptions& options)
{
    reflexGamesLastMode = options.mode;

    sl::ReflexOptions newOptions = options;

    if (Config::Instance()->FN_ForceReflex == ForceReflex::ForceEnable)
        newOptions.mode = sl::ReflexMode::eLowLatencyWithBoost;

    // Will cause a pink screen when used with DLSSG
    // if (Config::Instance()->FN_ForceReflex == 1)
    //     newOptions.mode = sl::ReflexMode::eOff;

    return o_slReflexSetOptions(newOptions);
}

sl::Result StreamlineHooks::hkslReflexSleep(const sl::FrameToken& frame)
{
    // if (State::Instance().activeFgOutput == FGOutput::DLSSG && StreamlineProxy::IsD3D12Inited() &&
    //     Config::Instance()->FGDLSSGUseGamesReflexMarkers.value_or_default())
    //{
    //     return StreamlineProxy::ReflexSleep()(frame);
    // }

    return o_slReflexSleep(frame);
}

void* StreamlineHooks::hkdlss_slGetPluginFunction(const char* functionName)
{
    LOG_DEBUG("{}", functionName);

    if (strcmp(functionName, "slOnPluginLoad") == 0)
    {
        o_dlss_slOnPluginLoad = (PFN_slOnPluginLoad) o_dlss_slGetPluginFunction(functionName);
        return &hkdlss_slOnPluginLoad;
    }

    if (strcmp(functionName, "slDLSSGetOptimalSettings") == 0 &&
        State::Instance().gameQuirks & GameQuirk::PregmataFixDLSSModes)
    {
        o_slDLSSGetOptimalSettings = (decltype(&slDLSSGetOptimalSettings)) o_dlss_slGetPluginFunction(functionName);
        return &hkslDLSSGetOptimalSettings;
    }

    return o_dlss_slGetPluginFunction(functionName);
}

void* StreamlineHooks::hkdlssg_slGetPluginFunction(const char* functionName)
{
    // LOG_DEBUG("{}", functionName);

    if (strcmp(functionName, "slOnPluginLoad") == 0)
    {
        o_dlssg_slOnPluginLoad = (PFN_slOnPluginLoad) o_dlssg_slGetPluginFunction(functionName);
        return &hkdlssg_slOnPluginLoad;
    }

    if (strcmp(functionName, "slDLSSGSetOptions") == 0)
    {
        o_slDLSSGSetOptions = (decltype(&slDLSSGSetOptions)) o_dlssg_slGetPluginFunction(functionName);

        // Give steam overlay the original as it seems to be hooking it
        auto steamOverlay = KernelBaseProxy::GetModuleHandleA_()("gameoverlayrenderer64.dll");
        if (steamOverlay != nullptr)
        {
            if (HMODULE callerModule = Util::GetCallerModule(_ReturnAddress()); callerModule == steamOverlay)
                return o_slDLSSGSetOptions;
        }

        return &hkslDLSSGSetOptions;
    }

    if (strcmp(functionName, "slDLSSGGetState") == 0)
    {
        o_slDLSSGGetState = (decltype(&slDLSSGGetState)) o_dlssg_slGetPluginFunction(functionName);

        // Give steam overlay the original as it seems to be hooking it
        auto steamOverlay = KernelBaseProxy::GetModuleHandleA_()("gameoverlayrenderer64.dll");
        if (steamOverlay != nullptr)
        {
            if (HMODULE callerModule = Util::GetCallerModule(_ReturnAddress()); callerModule == steamOverlay)
                return o_slDLSSGGetState;
        }

        return &hkslDLSSGGetState;
    }

    if (strcmp(functionName, "slGetPluginJSONConfig") == 0 && IsSL1AndDLSSGActive())
    {
        o_dlssg_slGetPluginJSONConfig_sl1 =
            reinterpret_cast<PFN_slGetPluginJSONConfig_sl1>(o_dlssg_slGetPluginFunction(functionName));

        if (o_dlssg_slGetPluginJSONConfig_sl1 != nullptr)
        {
            LOG_WARN("Hooking SL1 DLSSG slGetPluginJSONConfig");
            return &hkdlssg_slGetPluginJSONConfig_sl1;
        }
    }

    // Ensure that we have those DLSSG calls
    if (!o_slDLSSGSetOptions)
        o_slDLSSGSetOptions = (decltype(&slDLSSGSetOptions)) o_dlssg_slGetPluginFunction("slDLSSGSetOptions");

    if (!o_slDLSSGGetState)
        o_slDLSSGGetState = (decltype(&slDLSSGGetState)) o_dlssg_slGetPluginFunction("slDLSSGGetState");

    return o_dlssg_slGetPluginFunction(functionName);
}

void* StreamlineHooks::hklocal_dlssg_slGetPluginFunction(const char* functionName)
{
    // LOG_DEBUG("{}", functionName);

    if (strcmp(functionName, "slOnPluginLoad") == 0 && State::Instance().activeFgNvngx != FGNvngxReplacement::None)
    {
        o_local_dlssg_slOnPluginLoad = (PFN_slOnPluginLoad) o_local_dlssg_slGetPluginFunction(functionName);
        return &hklocal_dlssg_slOnPluginLoad;
    }

    return o_local_dlssg_slGetPluginFunction(functionName);
}

bool StreamlineHooks::hkreflex_slSetConstants_sl1(const void* data, uint32_t frameIndex, uint32_t id)
{
    // Streamline v1's version of slReflexSetOptions + slPCLSetMarker
    static sl1::ReflexConstants constants {};
    constants = *(const sl1::ReflexConstants*) data;

    reflexGamesLastMode = (sl::ReflexMode) constants.mode;

    LOG_DEBUG("mode: {}, frameIndex: {}, id: {}", (uint32_t) constants.mode, frameIndex, id);

    if (Config::Instance()->FN_ForceReflex == ForceReflex::ForceEnable)
        constants.mode = sl1::ReflexMode::eReflexModeLowLatencyWithBoost;

    // Will cause a pink screen when used with DLSSG
    // else if (Config::Instance()->FN_ForceReflex == 1)
    //     constants.mode = sl1::ReflexMode::eReflexModeOff;

    return o_reflex_slSetConstants_sl1(&constants, frameIndex, id);
}

void* StreamlineHooks::hkreflex_slGetPluginFunction(const char* functionName)
{
    // LOG_DEBUG("{}", functionName);

    if (strcmp(functionName, "slSetConstants") == 0 && State::Instance().streamlineVersion.major == 1)
    {
        o_reflex_slSetConstants_sl1 = (PFN_slSetConstants_sl1) o_reflex_slGetPluginFunction(functionName);
        return &hkreflex_slSetConstants_sl1;
    }

    if (strcmp(functionName, "slOnPluginLoad") == 0)
    {
        o_reflex_slOnPluginLoad = (PFN_slOnPluginLoad) o_reflex_slGetPluginFunction(functionName);
        return &hkreflex_slOnPluginLoad;
    }

    if (strcmp(functionName, "slReflexSetOptions") == 0)
    {
        o_slReflexSetOptions = (decltype(&slReflexSetOptions)) o_reflex_slGetPluginFunction(functionName);
        return &hkslReflexSetOptions;
    }

    if (strcmp(functionName, "slReflexSleep") == 0)
    {
        o_slReflexSleep = (decltype(&slReflexSleep)) o_reflex_slGetPluginFunction(functionName);
        return &hkslReflexSleep;
    }

    // TODO: Hopefully a game doesn't call both, maybe separate
    if (strcmp(functionName, "slReflexSetMarker") == 0 &&
        (State::Instance().gameQuirks & GameQuirk::FixSlSimulationMarkers ||
         State::Instance().activeFgInput == FGInput::DLSSG))
    {
        o_slPCLSetMarker = (decltype(&slPCLSetMarker)) o_reflex_slGetPluginFunction(functionName);
        return &hkslPCLSetMarker;
    }

    return o_reflex_slGetPluginFunction(functionName);
}

sl::Result StreamlineHooks::hkslPCLSetMarker(sl::PCLMarker marker, const sl::FrameToken& frame)
{
    // if (State::Instance().activeFgOutput == FGOutput::DLSSG && StreamlineProxy::IsD3D12Inited() &&
    //     Config::Instance()->FGDLSSGUseGamesReflexMarkers.value_or_default())
    //{
    //     return StreamlineProxy::PCLSetMarker()(marker, frame);
    // }

    // HACK for broken games
    if (State::Instance().gameQuirks & GameQuirk::FixSlSimulationMarkers)
    {
        static uint64_t last_simulation_end_id = 0;
        if (marker == sl::PCLMarker::eSimulationEnd)
        {
            last_simulation_end_id = frame;
        }

        if (marker == sl::PCLMarker::eSimulationStart && last_simulation_end_id >= frame && o_slGetNewFrameToken)
        {
            const uint64_t correction_offset = last_simulation_end_id - frame + 1;
            uint32_t newFrameId = static_cast<uint32_t>(frame + correction_offset);

            sl::FrameToken* newFramePointer {};
            auto result = o_slGetNewFrameToken(newFramePointer, &newFrameId);

            LOG_WARN("Simulation start marker sent after end marker, offset: {}", correction_offset);

            result = o_slPCLSetMarker(marker, *newFramePointer);
            return result;
        }
    }

    if (State::Instance().activeFgInput == FGInput::DLSSG)
    {
        if (State::Instance().streamlineVersion.major == 1)
        {
            if (marker == sl::PCLMarker::eRenderSubmitStart)
            {
                State::Instance().s_sl1FGInputs.evaluateState();
            }
            else if (marker == sl::PCLMarker::ePresentStart)
            {
                State::Instance().s_sl1FGInputs.markPresent(frame);
            }
        }
        else
        {
            if (marker == sl::PCLMarker::eRenderSubmitStart)
            {
                State::Instance().slFGInputs.evaluateState();
            }
            else if (marker == sl::PCLMarker::ePresentStart)
            {
                State::Instance().slFGInputs.markPresent(frame);
            }
        }
    }

    return o_slPCLSetMarker(marker, frame);
}

bool StreamlineHooks::hkpcl_slOnPluginLoad(sl::param::IParameters* params, const char* loaderJSON,
                                           const char** pluginJSON)
{
    LOG_FUNC();

    // Read once, so the restore below always matches the spoof.
    const bool spoofing = Config::Instance()->StreamlineSpoofing.value_or_default();
    LogPluginLoadEntry("sl.pcl", spoofing);

    uint32_t currentArch = 0;
    if (spoofing)
    {
        hookSystemCaps(params);
        currentArch = getSystemCapsArch();
        logCachedGpusForSlInit();
        logSystemCapsForSlInit("sl.pcl", "before the spoof");
        spoofArch(currentArch, sl::kFeaturePCL);
        logSystemCapsForSlInit("sl.pcl", "during the load");
    }

    auto result = o_pcl_slOnPluginLoad(params, loaderJSON, pluginJSON);

    if (spoofing)
    {
        setArch(currentArch);
        logSystemCapsForSlInit("sl.pcl", "after the restore");
    }

    // PCL's JSON is not patched, so it is not read here either.
    if (SlInitDiagnostics())
        LOG_INFO("[SLINIT] sl.pcl slOnPluginLoad returned {}", result);

    return result;
}

void* StreamlineHooks::hkpcl_slGetPluginFunction(const char* functionName)
{
    // LOG_DEBUG("{}", functionName);

    if (strcmp(functionName, "slPCLSetMarker") == 0 &&
        (State::Instance().gameQuirks & GameQuirk::FixSlSimulationMarkers ||
         State::Instance().activeFgInput == FGInput::DLSSG))
    {
        o_slPCLSetMarker = (decltype(&slPCLSetMarker)) o_pcl_slGetPluginFunction(functionName);
        return &hkslPCLSetMarker;
    }

    if (strcmp(functionName, "slOnPluginLoad") == 0)
    {
        o_pcl_slOnPluginLoad = (PFN_slOnPluginLoad) o_pcl_slGetPluginFunction(functionName);
        return &hkpcl_slOnPluginLoad;
    }

    return o_pcl_slGetPluginFunction(functionName);
}

bool StreamlineHooks::hk_setVoid(void* self, const char* key, void** value)
{
    // LOG_DEBUG("{}", key);

    if (strcmp(key, sl::param::common::kSystemCaps) == 0)
    {
        LOG_TRACE("Attempting to change system caps for Streamline v1, this could fail depending on the exact version");

        // SystemCapsSl15 is not entirely correct for Streamline 1.3
        // But we here only use the beginning that matches + extra
        auto caps = (SystemCapsSl15*) value;

        if (caps)
        {
            caps->gpuCount = 1;
            caps->architecture[0] = UINT_MAX;
            caps->driverVersionMajor = 999;

            // HAGS
            *((char*) value + 56) = (char) 0x01;
        }
    }

    return o_setVoid(self, key, value);
}

void StreamlineHooks::hkcommon_slSetParameters_sl1(void* params)
{
    LOG_FUNC();

    if (o_setVoid == nullptr && params)
    {
        void** vtable = *(void***) params;

        // It's flipped, 0 -> set void*, 7 -> get void*
        o_setVoid = (PFN_setVoid) vtable[0];

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        if (o_setVoid != nullptr)
            DetourAttach(&(PVOID&) o_setVoid, hk_setVoid);

        auto detourResult = DetourTransactionCommit();
        if (detourResult != NO_ERROR)
        {
            LOG_ERROR("Failed to hook setVoid: {:X}", detourResult);
            o_setVoid = nullptr;
        }
    }

    o_common_slSetParameters_sl1(params);
}

void* StreamlineHooks::hkcommon_slGetPluginFunction(const char* functionName)
{
    // LOG_DEBUG("{}", functionName);

    if (strcmp(functionName, "slOnPluginLoad") == 0)
    {
        o_common_slOnPluginLoad = (PFN_slOnPluginLoad) o_common_slGetPluginFunction(functionName);
        return &hkcommon_slOnPluginLoad;
    }

    // Used around Streamline v1.3, as 1.5 doesn't seem to have it anymore
    if (strcmp(functionName, "slSetParameters") == 0)
    {
        o_common_slSetParameters_sl1 = (PFN_slSetParameters_sl1) o_common_slGetPluginFunction(functionName);
        return &hkcommon_slSetParameters_sl1;
    }

    return o_common_slGetPluginFunction(functionName);
}

void StreamlineHooks::updateForceReflex()
{
    // Not needed for Streamline v1 as slSetConstants is sent every frame
    if (o_slReflexSetOptions)
    {
        sl::ReflexOptions options;

        auto forceReflex = Config::Instance()->FN_ForceReflex.value_or_default();

        if (forceReflex == ForceReflex::ForceEnable)
            options.mode = sl::ReflexMode::eLowLatencyWithBoost;
        else if (forceReflex == ForceReflex::ForceDisable)
            options.mode = sl::ReflexMode::eOff;
        else if (forceReflex == ForceReflex::InGame)
            options.mode = reflexGamesLastMode;

        auto result = o_slReflexSetOptions(options);
        if (result != sl::Result::eOk)
        {
            LOG_WARN("Failed to update Reflex mode with error code: {} ({:X})", magic_enum::enum_name(result),
                     (UINT) result);
        }
    }
}

void StreamlineHooks::updateDlssgOptions()
{
    if (o_slDLSSGSetOptions)
    {
        LOG_FUNC();
        hkslDLSSGSetOptions(lastDlssgViewport, lastDlssgOptions);
    }
}

// SL INTERPOSER

void StreamlineHooks::unhookInterposer()
{
    LOG_FUNC();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (o_slSetTag)
        DetourDetach(&(PVOID&) o_slSetTag, hkslSetTag);

    if (o_slSetTagForFrame)
        DetourDetach(&(PVOID&) o_slSetTagForFrame, hkslSetTagForFrame);

    if (o_slSetConstants)
        DetourDetach(&(PVOID&) o_slSetConstants, hkslSetConstants);

    if (o_slEvaluateFeature)
        DetourDetach(&(PVOID&) o_slEvaluateFeature, hkslEvaluateFeature);

    if (o_slSetTagForFrame)
    {
        DetourDetach(&(PVOID&) o_slSetTagForFrame, hkslSetTagForFrame);
        o_slSetTagForFrame = nullptr;
    }

    if (o_slInit)
        DetourDetach(&(PVOID&) o_slInit, hkslInit);

    if (o_slInit_sl1)
        DetourDetach(&(PVOID&) o_slInit_sl1, hkslInit_sl1);

    if (o_slSetTag_sl1)
        DetourDetach(&(PVOID&) o_slSetTag_sl1, hkslSetTag_sl1);

    if (o_slSetConstants_interposer_sl1)
        DetourDetach(&(PVOID&) o_slSetConstants_interposer_sl1, hkslSetConstants_sl1);

    if (o_slEvaluateFeature_sl1)
        DetourDetach(&(PVOID&) o_slEvaluateFeature_sl1, hkslEvaluateFeature_sl1);

    // if (o_logCallback)
    //     DetourDetach(&(PVOID&) o_logCallback, streamlineLogCallback);
    // else if (o_logCallback_sl1)
    //     DetourDetach(&(PVOID&) o_logCallback_sl1, streamlineLogCallback);

    auto detourResult = DetourTransactionCommit();
    if (detourResult != NO_ERROR)
    {
        LOG_ERROR("DetourTransactionCommit error: {:X}", detourResult);
    }
    else
    {
        o_slInit = nullptr;
        o_slInit_sl1 = nullptr;
        o_slSetTag = nullptr;
        o_slSetTagForFrame = nullptr;
        o_slEvaluateFeature = nullptr;
        o_slSetConstants = nullptr;
        o_slSetTag_sl1 = nullptr;
        o_slSetConstants_interposer_sl1 = nullptr;
        o_slEvaluateFeature_sl1 = nullptr;
        o_logCallback = nullptr;
        o_logCallback_sl1 = nullptr;
    }
}

// Call it just after sl.interposer's load or if sl.interposer is already loaded
void StreamlineHooks::hookInterposer(HMODULE slInterposer)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;
    LOG_FUNC();

    if (!slInterposer)
    {
        LOG_WARN("Streamline module in NULL");
        return;
    }

    // Interposer needs this or it might end in an infinite loop calling itself
    static HMODULE last_slInterposer = nullptr;

    if (last_slInterposer == slInterposer)
        return;

    last_slInterposer = slInterposer;

    // Looks like when reading DLL version load methods are called
    // To prevent loops disabling checks for sl.interposer.dll
    auto owner = State::GetOwner();
    State::DisableChecks(owner, "sl.interposer");

    if (o_slSetTag || o_slInit || o_slInit_sl1 || o_slSetTag_sl1 || o_slSetConstants_interposer_sl1 ||
        o_slEvaluateFeature_sl1)
        unhookInterposer();

    {
        char dllPath[MAX_PATH];
        GetModuleFileNameA(slInterposer, dllPath, MAX_PATH);

        LOG_TRACE("slInterposer path: {}", dllPath);

        version_t sl_version;
        Util::GetFileVersion(string_to_wstring(dllPath), &sl_version);

        State::Instance().streamlineVersion.major = sl_version.major;
        State::Instance().streamlineVersion.minor = sl_version.minor;
        State::Instance().streamlineVersion.patch = sl_version.patch;

        LOG_INFO("Streamline version: {}.{}.{}", sl_version.major, sl_version.minor, sl_version.patch);

        if (sl_version.major >= 2)
        {
            o_slSetTag =
                reinterpret_cast<decltype(&slSetTag)>(KernelBaseProxy::GetProcAddress_()(slInterposer, "slSetTag"));
            o_slSetTagForFrame = reinterpret_cast<decltype(&slSetTagForFrame)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slSetTagForFrame"));
            o_slInit = reinterpret_cast<decltype(&slInit)>(KernelBaseProxy::GetProcAddress_()(slInterposer, "slInit"));
            o_slEvaluateFeature = reinterpret_cast<decltype(&slEvaluateFeature)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slEvaluateFeature"));
            o_slAllocateResources = reinterpret_cast<decltype(&slAllocateResources)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slAllocateResources"));
            o_slSetConstants = reinterpret_cast<decltype(&slSetConstants)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slSetConstants"));
            o_slGetNativeInterface = reinterpret_cast<decltype(&slGetNativeInterface)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slGetNativeInterface"));
            o_slSetD3DDevice = reinterpret_cast<decltype(&slSetD3DDevice)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slSetD3DDevice"));
            o_slGetNewFrameToken = reinterpret_cast<decltype(&slGetNewFrameToken)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slGetNewFrameToken")); // Not hooked

            // For making the game think DLSSG is loaded and supported
            // but making SL not actually load the plugin
            o_slIsFeatureSupported = reinterpret_cast<decltype(&slIsFeatureSupported)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slIsFeatureSupported"));
            o_slIsFeatureLoaded = reinterpret_cast<decltype(&slIsFeatureLoaded)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slIsFeatureLoaded"));
            o_slGetFeatureRequirements = reinterpret_cast<decltype(&slGetFeatureRequirements)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slGetFeatureRequirements"));
            o_slGetFeatureVersion = reinterpret_cast<decltype(&slGetFeatureVersion)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slGetFeatureVersion"));
            o_slGetFeatureFunction = reinterpret_cast<decltype(&slGetFeatureFunction)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slGetFeatureFunction"));

            if (o_slInit != nullptr)
            {
                LOG_TRACE("Hooking v2");
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());

                DetourAttach(&(PVOID&) o_slInit, hkslInit);

                // Ray Regeneration needs these tags, and frame generation needs them for its
                // own reasons - so they attach for either.
                //
                // What they must NOT do is attach for neither. The RR port widened this from
                // "only under FG" to "always", and that quietly put three new detours into
                // every game on earth to serve a feature that needs an RDNA 4 card, a
                // denoiser DLL most installs do not have, and a Ray Reconstruction title.
                // A tester's game then started crashing during Streamline init. That is not
                // proof this was the cause - it is a reason not to have taken the risk.
                //
                // With the denoiser absent this now attaches exactly what it attached before
                // the RR work, so an ordinary install is back to behaviour that was already
                // shipping and tested.
                const bool rrWantsTags = FfxApiProxy::IsDenoiserReady(false);
                const bool fgWantsTags = State::Instance().activeFgInput == FGInput::NvngxFG ||
                                         State::Instance().activeFgInput == FGInput::DLSSG;

                if (rrWantsTags || fgWantsTags)
                {
                    if (o_slSetTag != nullptr)
                        DetourAttach(&(PVOID&) o_slSetTag, hkslSetTag);

                    if (o_slSetTagForFrame != nullptr)
                        DetourAttach(&(PVOID&) o_slSetTagForFrame, hkslSetTagForFrame);

                    if (o_slSetConstants != nullptr)
                        DetourAttach(&(PVOID&) o_slSetConstants, hkslSetConstants);
                }

                if (o_slEvaluateFeature != nullptr)
                    DetourAttach(&(PVOID&) o_slEvaluateFeature, hkslEvaluateFeature);

                // These five detours exist to stand in for a DLSSG plugin that is not there,
                // and they attach ONLY under DLSSG input, exactly as upstream. The first RR
                // build attached all five whenever Ray Regeneration could serve - and their
                // bodies answer DLSS_G unconditionally, so in Forza (FGInput = NvngxFG) the
                // game's own DLSS frame generation was handed the stub option/state functions
                // and showed "off" no matter what the settings said. Ray Regeneration needs two
                // answers and nothing else, and it gets them below.
                const bool dlssgStubs = State::Instance().activeFgInput == FGInput::DLSSG;
                if (dlssgStubs)
                {
                    if (o_slIsFeatureSupported != nullptr)
                        DetourAttach(&(PVOID&) o_slIsFeatureSupported, hkslIsFeatureSupported);

                    if (o_slIsFeatureLoaded != nullptr)
                        DetourAttach(&(PVOID&) o_slIsFeatureLoaded, hkslIsFeatureLoaded);

                    if (o_slGetFeatureRequirements != nullptr)
                        DetourAttach(&(PVOID&) o_slGetFeatureRequirements, hkslGetFeatureRequirements);

                    if (o_slGetFeatureVersion != nullptr)
                        DetourAttach(&(PVOID&) o_slGetFeatureVersion, hkslGetFeatureVersion);

                    if (o_slGetFeatureFunction != nullptr)
                        DetourAttach(&(PVOID&) o_slGetFeatureFunction, hkslGetFeatureFunction);
                }
                else if (RrAttachNow("slIsFeatureSupported / slGetFeatureRequirements (DLSS_RR)"))
                {
                    // Support and requirements only. Their bodies special-case DLSS_G only
                    // under DLSSG input, so in any other mode they are transparent for
                    // everything but DLSS_RR.
                    //
                    // P2 (AMDNR 0.3.4): an interposer already in memory is hooked from DllMain, where
                    // the GPU is not known yet; 0.3.3.2 read that as "not RDNA 4" and never attached
                    // these for the session. Now they attach then too (a denoiser expected) and each
                    // call asks rrCanServe, so on a card RR does not serve they pass straight through.
                    if (o_slIsFeatureSupported != nullptr)
                        DetourAttach(&(PVOID&) o_slIsFeatureSupported, hkslIsFeatureSupported);

                    if (o_slGetFeatureRequirements != nullptr)
                        DetourAttach(&(PVOID&) o_slGetFeatureRequirements, hkslGetFeatureRequirements);
                }

                // if (o_slAllocateResources != nullptr)
                //     DetourAttach(&(PVOID&) o_slAllocateResources, hkslAllocateResources);

                // if (o_slGetNativeInterface != nullptr)
                //     DetourAttach(&(PVOID&) o_slGetNativeInterface, hkslGetNativeInterface);

                // if (o_slSetD3DDevice != nullptr)
                //     DetourAttach(&(PVOID&) o_slSetD3DDevice, hkslSetD3DDevice);

                auto detourResult = DetourTransactionCommit();
                if (detourResult != NO_ERROR)
                {
                    LOG_ERROR("Failed to hook sl.interposer v2: {:X}", detourResult);
                    o_slSetTag = nullptr;
                    o_slSetTagForFrame = nullptr;
                    o_slInit = nullptr;
                    o_slEvaluateFeature = nullptr;
                    o_slAllocateResources = nullptr;
                    o_slSetConstants = nullptr;
                    o_slGetNativeInterface = nullptr;
                    o_slSetD3DDevice = nullptr;
                    o_slIsFeatureSupported = nullptr;
                    o_slIsFeatureLoaded = nullptr;
                    o_slGetFeatureRequirements = nullptr;
                    o_slGetFeatureVersion = nullptr;
                    o_slGetFeatureFunction = nullptr;
                }
            }
        }
        else if (sl_version.major == 1)
        {
            o_slInit_sl1 =
                reinterpret_cast<decltype(&sl1::slInit)>(KernelBaseProxy::GetProcAddress_()(slInterposer, "slInit"));
            o_slSetTag_sl1 = reinterpret_cast<decltype(&sl1::slSetTag)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slSetTag"));
            o_slSetConstants_interposer_sl1 = reinterpret_cast<decltype(&sl1::slSetConstants)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slSetConstants"));
            o_slEvaluateFeature_sl1 = reinterpret_cast<decltype(&sl1::slEvaluateFeature)>(
                KernelBaseProxy::GetProcAddress_()(slInterposer, "slEvaluateFeature"));

            LOG_INFO("SL1 exports - slInit: {}, slSetTag: {}, slSetConstants: {}, slEvaluateFeature: {}",
                     o_slInit_sl1 != nullptr, o_slSetTag_sl1 != nullptr, o_slSetConstants_interposer_sl1 != nullptr,
                     o_slEvaluateFeature_sl1 != nullptr);

            if (o_slInit_sl1 || o_slSetTag_sl1 || o_slSetConstants_interposer_sl1 || o_slEvaluateFeature_sl1)
            {
                LOG_TRACE("Hooking v1");
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());

                if (o_slInit_sl1)
                    DetourAttach(&(PVOID&) o_slInit_sl1, hkslInit_sl1);

                if (IsSL1AndFGActive())
                {
                    if (o_slSetTag_sl1)
                        DetourAttach(&(PVOID&) o_slSetTag_sl1, hkslSetTag_sl1);

                    if (o_slSetConstants_interposer_sl1)
                        DetourAttach(&(PVOID&) o_slSetConstants_interposer_sl1, hkslSetConstants_sl1);

                    if (o_slEvaluateFeature_sl1)
                        DetourAttach(&(PVOID&) o_slEvaluateFeature_sl1, hkslEvaluateFeature_sl1);
                }

                auto detourResult = DetourTransactionCommit();
                if (detourResult != NO_ERROR)
                {
                    LOG_ERROR("Failed to hook sl.interposer v1: {:X}", detourResult);
                    o_slInit_sl1 = nullptr;
                    o_slSetTag_sl1 = nullptr;
                    o_slSetConstants_interposer_sl1 = nullptr;
                    o_slEvaluateFeature_sl1 = nullptr;
                }
            }
        }
    }

    State::EnableChecks(owner);
}

// SL DLSS

void StreamlineHooks::unhookDlss()
{
    LOG_FUNC();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (o_dlss_slGetPluginFunction)
        DetourDetach(&(PVOID&) o_dlss_slGetPluginFunction, hkdlss_slGetPluginFunction);

    auto detourResult = DetourTransactionCommit();
    if (detourResult != NO_ERROR)
    {
        LOG_ERROR("Failed to unhook DLSS: {:X}", detourResult);
    }
    else
    {
        o_dlss_slGetPluginFunction = nullptr;
    }
}

void StreamlineHooks::hookDlss(HMODULE slDlss)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;
    LOG_FUNC();

    if (!slDlss)
    {
        LOG_WARN("Dlss module in NULL");
        return;
    }

    if (o_dlss_slGetPluginFunction)
        unhookDlss();

    o_dlss_slGetPluginFunction =
        reinterpret_cast<PFN_slGetPluginFunction>(KernelBaseProxy::GetProcAddress_()(slDlss, "slGetPluginFunction"));

    if (o_dlss_slGetPluginFunction != nullptr)
    {
        LOG_TRACE("Hooking slGetPluginFunction in sl.dlss");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&) o_dlss_slGetPluginFunction, hkdlss_slGetPluginFunction);

        auto detourResult = DetourTransactionCommit();
        if (detourResult != NO_ERROR)
        {
            LOG_ERROR("Failed to hook DLSS: {:X}", detourResult);
            o_dlss_slGetPluginFunction = nullptr;
        }
    }
}

// SL DLSS_D (Ray Reconstruction)

void StreamlineHooks::unhookDlssd()
{
    LOG_FUNC();

    if (o_dlssd_slGetPluginFunction == nullptr)
        return;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&) o_dlssd_slGetPluginFunction, hkdlssd_slGetPluginFunction);

    auto detourResult = DetourTransactionCommit();
    if (detourResult != NO_ERROR)
        LOG_ERROR("Failed to unhook DLSS_D: {:X}", detourResult);
    else
        o_dlssd_slGetPluginFunction = nullptr;
}

void StreamlineHooks::hookDlssd(HMODULE slDlssd)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;

    // Only worth a detour where Ray Regeneration can take the feature; elsewhere the
    // plugin is left exactly as upstream leaves it. Where the GPU is not known yet it attaches when a denoiser is
    // expected (P2): the detour's arch spoof asks rrCanServe on every plugin call.
    if (!RrAttachNow("the sl.dlss_d hook"))
        return;

    LOG_FUNC();

    if (!slDlssd)
    {
        LOG_WARN("Dlss_d module in NULL");
        return;
    }

    if (o_dlssd_slGetPluginFunction)
        unhookDlssd();

    o_dlssd_slGetPluginFunction =
        reinterpret_cast<PFN_slGetPluginFunction>(KernelBaseProxy::GetProcAddress_()(slDlssd, "slGetPluginFunction"));

    if (o_dlssd_slGetPluginFunction != nullptr)
    {
        LOG_INFO("Hooking slGetPluginFunction in sl.dlss_d (Ray Reconstruction -> FSR Ray Regeneration)");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&) o_dlssd_slGetPluginFunction, hkdlssd_slGetPluginFunction);

        auto detourResult = DetourTransactionCommit();
        if (detourResult != NO_ERROR)
        {
            LOG_ERROR("Failed to hook DLSS_D: {:X}", detourResult);
            o_dlssd_slGetPluginFunction = nullptr;
        }
    }
}

bool StreamlineHooks::isDlssdHooked() { return o_dlssd_slGetPluginFunction != nullptr; }

void* StreamlineHooks::hkdlssd_slGetPluginFunction(const char* functionName)
{
    if (strcmp(functionName, "slOnPluginLoad") == 0)
    {
        o_dlssd_slOnPluginLoad = (PFN_slOnPluginLoad) o_dlssd_slGetPluginFunction(functionName);
        return &hkdlssd_slOnPluginLoad;
    }

    return o_dlssd_slGetPluginFunction(functionName);
}

// The same shape as hkdlss_slOnPluginLoad: spoof the architecture for the duration of the
// plugin's load so it accepts the adapter, restore it, and strip the Vulkan requirements
// the plugin JSON declares for hardware we do not have.
bool StreamlineHooks::hkdlssd_slOnPluginLoad(sl::param::IParameters* params, const char* loaderJSON,
                                             const char** pluginJSON)
{
    LOG_FUNC();

    static std::string config;

    // Read once, so the restore below always matches the spoof.
    const bool spoofing = Config::Instance()->StreamlineSpoofing.value_or_default();
    LogPluginLoadEntry("sl.dlss_d", spoofing);

    uint32_t currentArch = 0;
    if (spoofing)
    {
        hookSystemCaps(params);
        currentArch = getSystemCapsArch();
        logCachedGpusForSlInit();
        logSystemCapsForSlInit("sl.dlss_d", "before the spoof");
        spoofArch(currentArch, sl::kFeatureDLSS_RR);
        logSystemCapsForSlInit("sl.dlss_d", "during the load");
    }

    auto result = o_dlssd_slOnPluginLoad(params, loaderJSON, pluginJSON);

    if (spoofing)
    {
        setArch(currentArch);
        logSystemCapsForSlInit("sl.dlss_d", "after the restore");
    }

    LOG_INFO("sl.dlss_d slOnPluginLoad returned {} with architecture spoofed for Ray Regeneration", result);

    return PatchPluginJson(
        "sl.dlss_d", result, pluginJSON, config, PluginJsonOnFailure::SkipPlugin,
        [](nlohmann::json& configJson)
        {
            auto primaryGpu = IdentifyGpu::getPrimaryGpu();
            if (primaryGpu.vendorId != VendorId::Nvidia || !primaryGpu.dlssCapable)
            {
                if (configJson.contains("/external/vk/instance/extensions"_json_pointer))
                    configJson["external"]["vk"]["instance"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/extensions"_json_pointer))
                    configJson["external"]["vk"]["device"]["extensions"].clear();

                if (configJson.contains("/external/vk/device/1.2_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.2_features"].clear();

                if (configJson.contains("/external/vk/device/1.3_features"_json_pointer))
                    configJson["external"]["vk"]["device"]["1.3_features"].clear();
            }

            PatchSL1PluginJson(configJson);
        });
}

// SL DLSSG

void StreamlineHooks::unhookDlssg()
{
    LOG_FUNC();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (o_dlssg_slGetPluginFunction)
        DetourDetach(&(PVOID&) o_dlssg_slGetPluginFunction, hkdlssg_slGetPluginFunction);

    auto detourResult = DetourTransactionCommit();
    if (detourResult != NO_ERROR)
    {
        LOG_ERROR("Failed to unhook DLSSG: {:X}", detourResult);
        o_dlssg_slGetPluginFunction = nullptr;
    }
}

void StreamlineHooks::hookDlssg(HMODULE slDlssg)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;
    LOG_FUNC();

    if (!slDlssg)
    {
        LOG_WARN("Dlssg module in NULL");
        return;
    }

    if (o_dlssg_slGetPluginFunction)
        unhookDlssg();

    o_dlssg_slGetPluginFunction =
        reinterpret_cast<PFN_slGetPluginFunction>(KernelBaseProxy::GetProcAddress_()(slDlssg, "slGetPluginFunction"));

    if (o_dlssg_slGetPluginFunction != nullptr)
    {
        LOG_TRACE("Hooking slGetPluginFunction in sl.dlssg");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&) o_dlssg_slGetPluginFunction, hkdlssg_slGetPluginFunction);

        auto detourResult = DetourTransactionCommit();
        if (detourResult != NO_ERROR)
        {
            LOG_ERROR("Failed to hook DLSSG: {:X}", detourResult);
            o_dlssg_slGetPluginFunction = nullptr;
        }
    }
}

// Local SL DLSSG

void StreamlineHooks::unhookLocalDlssg()
{
    LOG_FUNC();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (o_local_dlssg_slGetPluginFunction)
    {
        DetourDetach(&(PVOID&) o_local_dlssg_slGetPluginFunction, hklocal_dlssg_slGetPluginFunction);
        o_local_dlssg_slGetPluginFunction = nullptr;
    }

    DetourTransactionCommit();
}

void StreamlineHooks::hookLocalDlssg(HMODULE slDlssg)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;
    LOG_FUNC();

    if (!slDlssg)
    {
        LOG_WARN("Dlssg module in NULL");
        return;
    }

    if (o_local_dlssg_slGetPluginFunction)
        unhookLocalDlssg();

    o_local_dlssg_slGetPluginFunction =
        reinterpret_cast<PFN_slGetPluginFunction>(KernelBaseProxy::GetProcAddress_()(slDlssg, "slGetPluginFunction"));

    if (o_local_dlssg_slGetPluginFunction != nullptr)
    {
        LOG_TRACE("Hooking slGetPluginFunction in local sl.dlssg");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&) o_local_dlssg_slGetPluginFunction, hklocal_dlssg_slGetPluginFunction);

        DetourTransactionCommit();
    }
}

// SL REFLEX

void StreamlineHooks::unhookReflex()
{
    LOG_FUNC();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (o_reflex_slGetPluginFunction)
        DetourDetach(&(PVOID&) o_reflex_slGetPluginFunction, hkreflex_slGetPluginFunction);

    auto detourResult = DetourTransactionCommit();
    if (detourResult != NO_ERROR)
    {
        LOG_ERROR("Failed to unhook Reflex: {:X}", detourResult);
    }
    else
    {
        o_reflex_slGetPluginFunction = nullptr;
    }
}

void StreamlineHooks::hookReflex(HMODULE slReflex)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;
    LOG_FUNC();

    if (!slReflex)
    {
        LOG_WARN("Reflex module in NULL");
        return;
    }

    if (o_reflex_slGetPluginFunction)
        unhookReflex();

    o_reflex_slGetPluginFunction =
        reinterpret_cast<PFN_slGetPluginFunction>(KernelBaseProxy::GetProcAddress_()(slReflex, "slGetPluginFunction"));

    if (o_reflex_slGetPluginFunction != nullptr)
    {
        LOG_TRACE("Hooking slGetPluginFunction in sl.reflex");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&) o_reflex_slGetPluginFunction, hkreflex_slGetPluginFunction);

        auto detourResult = DetourTransactionCommit();
        if (detourResult != NO_ERROR)
        {
            LOG_ERROR("Failed to hook Reflex: {:X}", detourResult);
            o_reflex_slGetPluginFunction = nullptr;
        }
    }
}

// SL PCL

void StreamlineHooks::unhookPcl()
{
    LOG_FUNC();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (o_pcl_slGetPluginFunction)
        DetourDetach(&(PVOID&) o_pcl_slGetPluginFunction, hkpcl_slGetPluginFunction);

    auto detourResult = DetourTransactionCommit();
    if (detourResult != NO_ERROR)
    {
        LOG_ERROR("Failed to unhook PCL: {:X}", detourResult);
    }
    else
    {
        o_pcl_slGetPluginFunction = nullptr;
    }
}

void StreamlineHooks::hookPcl(HMODULE slPcl)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;
    LOG_FUNC();

    if (!slPcl)
    {
        LOG_WARN("Pcl module in NULL");
        return;
    }

    if (o_pcl_slGetPluginFunction)
        unhookPcl();

    o_pcl_slGetPluginFunction =
        reinterpret_cast<PFN_slGetPluginFunction>(KernelBaseProxy::GetProcAddress_()(slPcl, "slGetPluginFunction"));

    if (o_pcl_slGetPluginFunction != nullptr)
    {
        LOG_TRACE("Hooking slGetPluginFunction in sl.pcl");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&) o_pcl_slGetPluginFunction, hkpcl_slGetPluginFunction);

        auto detourResult = DetourTransactionCommit();
        if (detourResult != NO_ERROR)
        {
            LOG_ERROR("Failed to hook PCL: {:X}", detourResult);
            o_pcl_slGetPluginFunction = nullptr;
        }
    }
}

// SL COMMON

void StreamlineHooks::unhookCommon()
{
    LOG_FUNC();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (o_common_slGetPluginFunction)
        DetourDetach(&(PVOID&) o_common_slGetPluginFunction, hkcommon_slGetPluginFunction);

    auto detourResult = DetourTransactionCommit();
    if (detourResult != NO_ERROR)
    {
        LOG_ERROR("Failed to unhook Common: {:X}", detourResult);
    }
    else
    {
        systemCaps = nullptr;
        systemCapsSl15 = nullptr;
        o_common_slGetPluginFunction = nullptr;
    }
}

void StreamlineHooks::hookCommon(HMODULE slCommon)
{
    if (State::Instance().externalFrameGeneration)
        return;

    if (DiagNoStreamlineHooks())
        return;
    LOG_FUNC();

    if (!slCommon)
    {
        LOG_WARN("Common module in NULL");
        return;
    }

    if (o_common_slGetPluginFunction)
        unhookCommon();

    o_common_slGetPluginFunction =
        reinterpret_cast<PFN_slGetPluginFunction>(KernelBaseProxy::GetProcAddress_()(slCommon, "slGetPluginFunction"));

    if (o_common_slGetPluginFunction != nullptr)
    {
        LOG_TRACE("Hooking slGetPluginFunction in sl.common");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&) o_common_slGetPluginFunction, hkcommon_slGetPluginFunction);

        auto detourResult = DetourTransactionCommit();
        if (detourResult != NO_ERROR)
        {
            LOG_ERROR("Failed to hook Common: {:X}", detourResult);
            o_common_slGetPluginFunction = nullptr;
        }
    }
}

bool StreamlineHooks::isInterposerHooked() { return o_slInit != nullptr || o_slInit_sl1 != nullptr; }

bool StreamlineHooks::isDlssHooked() { return o_dlss_slGetPluginFunction != nullptr; }

bool StreamlineHooks::isDlssgHooked() { return o_dlssg_slGetPluginFunction != nullptr; }

bool StreamlineHooks::isLocalDlssgHooked() { return o_local_dlssg_slGetPluginFunction != nullptr; }

bool StreamlineHooks::isCommonHooked() { return o_common_slGetPluginFunction != nullptr; }

bool StreamlineHooks::isSetConstantsHooked() { return o_slSetConstants != nullptr; }

bool StreamlineHooks::isPclHooked() { return o_pcl_slGetPluginFunction != nullptr; }

bool StreamlineHooks::isReflexHooked() { return o_reflex_slGetPluginFunction != nullptr; }
