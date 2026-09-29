// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#pragma once
#include "upscalers/IFeature.h"

#include "misc/Quirks.h"
#include "misc/SkipSpoof.h"
#include "framegen/IFGFeature_Dx12.h"
#include <inputs/FG/Streamline_Inputs_Dx12.h>
#include <inputs/FG/Streamline_Inputs_Sl1_Dx12.h>

#include <set>
#include <deque>
#include <mutex>
#include <atomic>
#include <sl_dlss_g.h>
#include <vulkan/vulkan.h>
#include <ankerl/unordered_dense.h>

enum class FGPreset : uint32_t
{
    NoFG,
    OptiFG,
    Nukems,
};

enum class FrameTimeSource : uint32_t
{
    Input,
    Opti,
    Zero,
};

enum class FGInput : uint32_t
{
    NoFG,
    Upscaler, // OptiFG
    DLSSG,    // technically Streamline inputs
    NvngxFG,
    FSRFG,
    FSRFG30,
    XeFG,

    ForceXeLL, // Do not expose this option
};

enum class FGOutput : uint32_t
{
    NoFG,
    FSRFG,
    DLSSG,
    XeFG,
};

enum class FGNvngxReplacement : uint32_t
{
    None,
    Nukems,
    Arturs,
    FFX,
    Combo,
};

enum class WorkingMode : uint32_t
{
    Dxgi,
    D3d12,
    Other,
};

enum class PostCode : uint32_t
{
    SlPluginsAlreadyInMemory,
    TryingFsr4Fp8OnUnsupported,
    REFrameworkMissing, // AMDNR 0.3.3.2: re9 / re9demo / pragmata without REFramework (misc/REFrameworkCheck.h)
    _
};

enum class GameEngineType : uint32_t
{
    Unity,
    Unreal,
    Katana,
    Other,
};

enum class SwapchainInteropApi : uint32_t
{
    None,
    Dx11wDx12,
};

typedef struct CapturedHudlessInfo
{
    UINT64 usageCount = 1;
    UINT captureInfo = 0;
    bool enabled = true;
} captured_hudless_info;

class State
{
  public:
    static State& Instance()
    {
        static State instance;
        return instance;
    }

    std::string gameExe;
    std::string gameName;
    std::string gameVersion;
    GameEngineType gameEngine = GameEngineType::Other;

    bool nvngxDx11Inited = false;
    bool nvngxDx12Inited = false;
    bool nvngxVkInited = false;

    flag_set<GameQuirk> gameQuirks;
    bool isOptiPatcherSucceed = false;

    // Reseting on creation of new feature
    std::optional<bool> autoExposure;

    // FG
    uint64_t fgLastFrame = 0;

    // Nvngx FG, uses streamline swapchain
    bool nukemsFgFileAvailable = false;
    bool artursFgFileAvailable = false;
    bool dlssgDebugView = false;
    bool dlssgInterpolatedOnly = false;
    uint64_t dlssgLastFrame = 0;
    uint32_t delayMenuRenderBy = 0;

    // FSR Common
    float lastFsrCameraNear = 0.0f;
    float lastFsrCameraFar = 0.0f;

    // Frame Generation
    FGInput activeFgInput = FGInput::NoFG;
    bool externalFrameGeneration = false; // startup-only: do not switch hook ownership live
    FGOutput activeFgOutput = FGOutput::NoFG;
    std::string fgNote; // why the FG output differs from the ini (native XeSS FG present), shown in the Frame Gen tab
    // This should be set to a non-None value only if all other requirements are met and nvngx can be used
    FGNvngxReplacement activeFgNvngx = FGNvngxReplacement::None;

    // Streamline FG inputs
    Sl_Inputs_Dx12 slFGInputs = {};
    // The last sl::Constants the title published, kept whole. Ray Regeneration needs
    // the view and projection matrices, and the FG input path above only keeps the
    // fields frame generation happens to use.
    // What the loaded Ray Regeneration provider says it can do. Filled once from
    // ffxQuery and kept, so the menu can list real providers and debug modes by name
    // instead of hard-coding a table that goes stale with the next SDK.
    std::vector<const char*> ffxDenoiserVersionNames {};
    std::vector<uint64_t> ffxDenoiserVersionIds {};
    std::vector<uint64_t> ffxDenoiserDebugModes;
    std::unordered_map<uint64_t, const char*> ffxDenoiserDebugModeNames;
    sl::Constants slLastConstants = {};
    uint32_t slLastConstantsFrame = UINT32_MAX;
    uint32_t slLastConstantsViewport = UINT32_MAX;
    Sl1_Inputs_Dx12 s_sl1FGInputs {};

    // OptiFG
    bool fgPresentIsCalled = false;
    bool fgOnlyGenerated = false;
    bool fgHudlessCompare = false;
    bool fgChanged = false;
    bool scChanged = false;
    bool skipHeapCapture = false;

    bool fgCaptureResources = false;
    size_t fgCapturedResourceCount = 0;
    bool fgResetCapturedResources = false;
    bool fgOnlyUseCapturedResources = false;

    bool fsrfgFramePaceTuningChanged = false;
    bool fsrfgInputActive = false;

    ankerl::unordered_dense::map<void*, CapturedHudlessInfo> capturedHudlesses;
    bool clearCapturedHudlesses = false;

    // NVNGX init parameters
    uint64_t NVNGX_ApplicationId = 1337;
    std::wstring NVNGX_ApplicationDataPath;
    std::string NVNGX_ProjectId;
    NVSDK_NGX_Version NVNGX_Version {};
    const NVSDK_NGX_FeatureCommonInfo* NVNGX_FeatureInfo = nullptr;
    std::vector<std::wstring> NVNGX_FeatureInfo_Paths;
    NVSDK_NGX_LoggingInfo NVNGX_Logger { nullptr, NVSDK_NGX_LOGGING_LEVEL_OFF, false };
    NVSDK_NGX_EngineType NVNGX_Engine = NVSDK_NGX_ENGINE_TYPE_CUSTOM;
    std::string NVNGX_EngineVersion;
    std::optional<std::wstring> NVNGX_DLSS_Path;
    std::optional<std::wstring> NVNGX_DLSSD_Path;
    std::optional<std::wstring> NVNGX_DLSSG_Path;

    // optis dlls
    HMODULE optiSlInterposer = nullptr;
    HMODULE optiSlCommon = nullptr;
    HMODULE optiSlDLSSG = nullptr;
    HMODULE optiSlReflex = nullptr;
    HMODULE optiSlPCL = nullptr;
    HMODULE optiDLSSG = nullptr;

    // NGX OTA
    std::string NGX_OTA_Dlss;
    std::string NGX_OTA_Dlssd;

    feature_version streamlineVersion = { 0, 0, 0 };

    // WHAT THE GAME'S slInit RETURNED (0.3.3, NBA 2K27 "slInit error 0x18" report). hkslInit and
    // hkslInit_sl1 store it with the call's duration so the log and the Neural tab can say that the
    // game's Streamline failed to start, instead of leaving an idle pass unexplained.
    //   slInitResult: the sl::Result value (Streamline 2+), or 0 / 1 for Streamline 1's bool
    //                 (slInitIsSl1 set, 1 = failed); -1 = never seen - hkslInit was not installed
    //                 ([FrameGen] External=true, a failed detour) or the game has not called slInit
    //                 yet. The menu shows "unknown" for -1.
    //   slInitResultName: a string literal ("eOk", "eErrorExceptionHandler", ...), never freed;
    //                 nullptr until the first result. Written before slInitResult.
    std::atomic<int32_t> slInitResult { -1 };
    std::atomic<uint32_t> slInitMs { 0 };
    std::atomic<bool> slInitIsSl1 { false };
    std::atomic<const char*> slInitResultName { nullptr };
    // True only while the game's slInit runs under hkslInit / hkslInit_sl1. The [SLINIT]
    // diagnostics (plugin DLL loads, plugin onLoad results, SystemCaps before and after the
    // spoof, NGX feature queries) log at INFO only inside this window, so the shipped LogLevel=2
    // captures them and nothing is logged outside slInit.
    std::atomic<bool> slInitInProgress { false };

    // Has value when Opti was able to hook sl and the game set DLSSG options
    std::optional<int> dlssgMfgMax = std::nullopt;

    API api = API::NotSelected;
    API swapchainApi = API::NotSelected;
    SwapchainInteropApi swapchainInteropApi = SwapchainInteropApi::None;

    // Framerate
    bool reflexLimitsFps = false;
    bool reflexShowWarning = false;
    bool rtssReflexInjection = false;
    bool fakenvapiReloadLowLatency = false;
    UINT64 reflexFrameId = 0;
    UINT64 frameCount = 0;
    bool vkAntiLagSupported = false;

    // for realtime changes
    ankerl::unordered_dense::map<unsigned int, bool> changeBackend;
    // Ray Reconstruction handles whose FSR-RR gave up (the title never published RR's inputs) and
    // were switched to plain FSR. The neural pass treats such a handle as Super Resolution: pre-SR
    // placement, no ApplyAfterRR gate - there is no RR left to stay behind (Satisfactory report).
    ankerl::unordered_dense::map<unsigned int, bool> rrFallbackToSr;
    std::string rrFallbackReason; // why (the Neural tab's Live section), set with the entry above
    // GetTickCount64() of the last successful FSR Ray Regeneration dispatch (0 = never): the Neural
    // tab shows its Ray Regeneration controls only while the game is really using Ray Reconstruction.
    volatile ULONGLONG fsrRrLastDispatchMs = 0;
    Upscaler newBackend = Upscaler::Reset;

    // XeSS debug stuff
    bool xessDebug = false;
    int xessDebugFrames = 5;
    float lastMipBias = 100.0f;
    float lastMipBiasMax = -100.0f;

    bool WAR_xefgRequestFGToggle = false;

    bool dlssgGameDMFGSupported = false;
    sl::DLSSGMode dlssgLastSetMode = sl::DLSSGMode::eOff;
    int dlssgDetectedInterpolationCount = 0;

    // DLSS
    bool dlssPresetsOverriddenExternally = false;
    bool dlssPresetsOverridenByOpti = false;
    uint32_t dlssRenderPresetExternal = 0;
    uint32_t dlssRenderPresetDLAA = 0;
    uint32_t dlssRenderPresetUltraQuality = 0;
    uint32_t dlssRenderPresetQuality = 0;
    uint32_t dlssRenderPresetBalanced = 0;
    uint32_t dlssRenderPresetPerformance = 0;
    uint32_t dlssRenderPresetUltraPerformance = 0;

    // DLSSD
    bool dlssdPresetsOverriddenExternally = false;
    bool dlssdPresetsOverridenByOpti = false;
    uint32_t dlssdRenderPresetExternal = 0;
    uint32_t dlssdRenderPresetDLAA = 0;
    uint32_t dlssdRenderPresetUltraQuality = 0;
    uint32_t dlssdRenderPresetQuality = 0;
    uint32_t dlssdRenderPresetBalanced = 0;
    uint32_t dlssdRenderPresetPerformance = 0;
    uint32_t dlssdRenderPresetUltraPerformance = 0;

    // Spoofing
    // For DXVK, it calls DXGI which cause softlock
    bool skipDxgiLoadChecks = false;
    bool skipParentWrapping = false;

    // quirks
    std::vector<std::string> detectedQuirks {};

    // FFX
    std::vector<const char*> ffxUpscalerVersionNames {};
    std::vector<uint64_t> ffxUpscalerVersionIds {};
    std::vector<const char*> ffxFGVersionNames {};
    std::vector<uint64_t> ffxFGVersionIds {};
    std::optional<uint32_t> currentFsr4Preset {};

    // Linux checks
    bool isRunningOnLinux = false;

    // Other checks
    WorkingMode workingMode = WorkingMode::Other;

    // Vulkan stuff
    bool vulkanCreatingSC = false;
    bool creatingD3DDevice = false;
    bool vulkanSkipHooks = false;
    VkInstance VulkanInstance = nullptr;

    // Framegraph
    std::deque<double> upscaleTimes;
    std::deque<double> frameTimes;
    std::vector<DetailedGpuTime> detailedGpuTimes;
    double lastFGFrameTime = 0.0;
    double presentFrameTime = 0.0;
    std::mutex frameTimeMutex;

    // Opti checking if everything is setup correctly on game launch
    // Takes effect up to the first time Opti can show anything on the screen
    // Beyond that use notifications
    bool postDone = false;
    flag_set<PostCode> postCodes;

    // Version check
    std::mutex versionCheckMutex;
    bool versionCheckInProgress = false;
    bool versionCheckCompleted = false;
    bool updateAvailable = false;
    std::string latestVersionTag;
    std::string latestVersionUrl;
    std::string versionCheckError;

    // Swapchain info
    float screenWidth = 800.0;
    float screenHeight = 450.0;
    bool realExclusiveFullscreen = false;
    bool SCExclusiveFullscreen = false;
    bool SCAllowTearing = false;
    UINT SCLastFlags = 0;

    // HDR
    std::vector<IUnknown*> scBuffers;
    bool isHdrActive = false;

    std::optional<ApiUpscalerInput> setInputApiName;
    ApiUpscalerInput currentInputApiName;

    bool isShuttingDown = false;
    std::set<PVOID> modulesToFree;

    // menu warnings
    bool fgSettingsChanged = false;
    bool nvngxIniDetected = false;

    bool nvngxExists = false;
    std::optional<std::wstring> nvngxReplacement = std::nullopt;
    bool libxessExists = false;
    bool fsrHooks = false;

    IFeature* currentFeature = nullptr;
    IFGFeature_Dx12* currentFG = nullptr;
    IDXGISwapChain* currentSwapchain = nullptr;
    IDXGISwapChain* currentWrappedSwapchain = nullptr;
    IDXGISwapChain* currentRealSwapchain = nullptr;
    IDXGISwapChain* currentFGSwapchain = nullptr;
    ID3D12Device* currentD3D12Device = nullptr;
    ID3D11Device* currentD3D11Device = nullptr;
    ID3D12CommandQueue* currentCommandQueue = nullptr;
    VkDevice currentVkDevice = nullptr;
    DXGI_SWAP_CHAIN_DESC currentSwapchainDesc {};

    std::vector<ID3D12Device*> d3d12Devices;
    std::vector<ID3D11Device*> d3d11Devices;

    static UINT GetOwner()
    {
        _lastOwner++;
        return _lastOwner;
    }

    // Moved checks here to prevent circular includes
    /// <summary>
    /// Enables skipping of LoadLibrary checks
    /// </summary>
    /// <param name="dllName">Lower case dll name without `.dll` at the end. Leave blank for skipping all dll's</param>
    static void DisableChecks(UINT owner, std::string dllName = "")
    {
        // if (_skipOwner == 0 || _skipOwner < owner)
        //{
        _skipOwner = owner;
        _skipChecks = true;
        _skipDllName[_skipOwner] = dllName;
        //}
    };

    static void EnableChecks(UINT owner)
    {
        _skipDllName.erase(_skipOwner);

        // if (_skipOwner == owner)
        //{
        if (_skipDllName.size() > 0)
        {
            // loop in reverse to get the last added owner
            _skipOwner = _skipDllName.rbegin()->first;
        }
        else
        {
            _skipOwner = 0;
        }

        _skipChecks = (_skipOwner != 0);
        //}
    };

    static void DisableServeOriginal(UINT owner)
    {
        if (_serveOwner == 0 || _serveOwner == owner)
        {
            _serveOriginal = false;
            _skipOwner = 0;
        }
    };

    static void EnableServeOriginal(UINT owner)
    {
        if (_serveOwner == 0 || _serveOwner == owner)
        {
            _serveOriginal = true;
            _skipOwner = owner;
        }
    };

    static bool SkipDllChecks() { return _skipChecks; }
    static std::string SkipDllName()
    {
        return _skipOwner == 0 ? "" : (_skipDllName.contains(_skipOwner) ? _skipDllName[_skipOwner] : "");
    }
    static bool ServeOriginal() { return _serveOriginal; }

  private:
    inline static bool _skipChecks = false;
    inline static std::map<UINT, std::string> _skipDllName;
    inline static UINT _skipOwner = 0;
    inline static UINT _lastOwner = 0;

    inline static bool _serveOriginal = false;
    inline static UINT _serveOwner = 0;

    State() = default;
};

class ScopedSkipSpoofingBase
{
  private:
    uint64_t entryId;

  public:
    explicit ScopedSkipSpoofingBase(SkipSpoofType type) { entryId = SkipSpoof::AddEntry(type); }

    ~ScopedSkipSpoofingBase() { SkipSpoof::RemoveEntry(entryId); }
};

class ScopedSkipSpoofingGlobal : public ScopedSkipSpoofingBase
{
  public:
    explicit ScopedSkipSpoofingGlobal() : ScopedSkipSpoofingBase(SkipSpoofType::Global) {}
};

class ScopedSkipSpoofingThread : public ScopedSkipSpoofingBase
{
  public:
    explicit ScopedSkipSpoofingThread() : ScopedSkipSpoofingBase(SkipSpoofType::Thread) {}
};

class ScopedSkipDxgiLoadChecks
{
  private:
    bool previousState;

  public:
    ScopedSkipDxgiLoadChecks()
    {
        previousState = State::Instance().skipDxgiLoadChecks;
        State::Instance().skipDxgiLoadChecks = true;
    }

    ~ScopedSkipDxgiLoadChecks() { State::Instance().skipDxgiLoadChecks = previousState; }
};

class ScopedSkipParentWrapping
{
  private:
    bool previousState;

  public:
    ScopedSkipParentWrapping()
    {
        previousState = State::Instance().skipParentWrapping;
        State::Instance().skipParentWrapping = true;
    }

    ~ScopedSkipParentWrapping() { State::Instance().skipParentWrapping = previousState; }
};

class ScopedSkipHeapCapture
{
  private:
    bool previousState;

  public:
    ScopedSkipHeapCapture()
    {
        previousState = State::Instance().skipHeapCapture;
        State::Instance().skipHeapCapture = true;
    }

    ~ScopedSkipHeapCapture() { State::Instance().skipHeapCapture = previousState; }
};

// (AMDNR 0.3.3.2) Vulkan objects OptiScaler creates for itself, on this thread. Under Proton or DXVK its own
// D3D12 / DXGI calls (the GPU probe, the Vulkan bridge's D3D12 device, Anti-Lag 2) create a VkInstance and a
// VkDevice through the same hooks the game's go through, and those took them for the game's: State::
// VulkanInstance pointed at an instance about to be destroyed, and the Anti-Lag 2 state at a dead device.
// Decided per create call on the calling thread - never per instance (DXVK 2.4+ shares one VkInstance) and
// never process-wide (vulkanSkipHooks is, so another thread's game device could be skipped). Read by the
// Vulkan hooks while [Hooks] SkipOwnVulkanObjects is on. ScopedSkipVulkanHooks counts too.
inline thread_local unsigned t_ownVulkanObjects = 0;
inline bool CreatingOwnVulkanObjects() { return t_ownVulkanObjects != 0; }
class ScopedOwnVulkanObjects
{
  public:
    ScopedOwnVulkanObjects() { ++t_ownVulkanObjects; }
    ~ScopedOwnVulkanObjects() { --t_ownVulkanObjects; }
    ScopedOwnVulkanObjects(const ScopedOwnVulkanObjects&) = delete;
    ScopedOwnVulkanObjects& operator=(const ScopedOwnVulkanObjects&) = delete;
};

class ScopedSkipVulkanHooks
{
  private:
    bool previousState;
    ScopedOwnVulkanObjects own; // (0.3.3.2) the per-thread mark, beside the process-wide flag

  public:
    ScopedSkipVulkanHooks()
    {
        previousState = State::Instance().vulkanSkipHooks;
        State::Instance().vulkanSkipHooks = true;
    }
    ~ScopedSkipVulkanHooks() { State::Instance().vulkanSkipHooks = previousState; }
};

class ScopedVulkanCreatingSC
{
  private:
    bool previousState;

  public:
    ScopedVulkanCreatingSC()
    {
        previousState = State::Instance().vulkanCreatingSC;
        State::Instance().vulkanCreatingSC = true;
    }
    ~ScopedVulkanCreatingSC() { State::Instance().vulkanCreatingSC = previousState; }
};

class ScopedCreatingD3DDevice
{
  private:
    bool previousState;

  public:
    ScopedCreatingD3DDevice()
    {
        previousState = State::Instance().creatingD3DDevice;
        State::Instance().creatingD3DDevice = true;
    }
    ~ScopedCreatingD3DDevice() { State::Instance().creatingD3DDevice = previousState; }
};
