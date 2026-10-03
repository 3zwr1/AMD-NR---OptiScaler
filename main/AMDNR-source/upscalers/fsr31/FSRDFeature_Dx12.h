// Origin: FSR Ray Regeneration for OptiScaler by Zach Hembree (DarkHelmet), branch ffx-denoise-experimental
// (GPL-3.0), continued by burak113; ported to AMDNR from burak113's branch at 3da4808.
// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FSR31Feature_Dx12.h"
#include "hooks/Streamline_Hooks.h"
#include "shaders/fsrd_preprocess/FSRDPreprocessor_Dx12.h"
#include <array>
#include <DirectXMath.h>

// SAT-P0-4 (AMDNR 0.3.4): the one-shot depth / normals content probe (defined in FSRDFeature_Dx12.cpp).
struct RRContentProbe;

/**
 * @brief Unfied denoiser-upscaler utilising AMD FSR Ray Regeneration and Super Resolution with
 * DLSS-RR inputs. Extends FSR 3.1+ upscaler implementation.
 */
class FSRDFeatureDx12 : public FSR31FeatureDx12
{
  public:
    using FSRDConvDesc = FSRDPreprocessor_Dx12::ConversionDesc;

    FSRDFeatureDx12(uint32_t InHandleId, NVSDK_NGX_Parameter* InParameters);

    ~FSRDFeatureDx12();

    feature_version Version() override { return FSR31FeatureDx12::Version(); }

    Upscaler GetUpscalerType() const override { return Upscaler::FSR_RR; }

    // AMDNR 0.3.4.1: the sharpening after Ray Regeneration when the title sends no NGX sharpness (RE Requiem sends
    // 0: it is tuned for DLSS-RR, which does not sharpen, and RR + FSR SR looked soft with nothing after it). FSR SR's
    // own sharpening runs at this value, or OptiScaler's RCAS when [Sharpness] RcasEnabled is on (never both). A title
    // that sends > 0 keeps its value; [Sharpness] OverrideSharpness=true with Sharpness=0 turns it off.
    static constexpr float kRrDefaultSharpness = 0.25f;
    // (AMDNR 0.3.4.2, RN3) The same number is the default of [Sharpness] RrDefaultSharpness (Config.h), which
    // DefaultSharpnessWhenTitleSendsNone reads: the value can be A/B'd from the ini without a build, and a test
    // asserts the constant and the key's default stay equal.
    // kRrDefaultSharpness, or 0 under [FSR-RR] FfxDenoiserPathTracedProfile (the profile does not sharpen RR) and
    // (0.3.4.1, Proton RR) on Wine/Proton: RrGate::DefaultSharpnessWhenTitleSendsNone in hooks/RrHardwareGate.h.
    float DefaultSharpnessWhenTitleSendsNone() const override;

    bool EvaluateInternal(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters) override;

    // Consecutive frames on which the title's Ray Reconstruction inputs could not be
    // gathered. The frame is still upscaled from the raw colour so the picture never
    // freezes; past a threshold the feature asks for FSR instead. See EvaluateInternal.
    uint32_t _missingInputFrames = 0;
    bool _requestedFsrFallback = false;
    std::string _denoiserBlocker; // the last reason the denoiser inputs were refused, for the fallback line and the menu

    // SAT-P0-2 (AMDNR 0.3.4): what an NGX camera matrix key (WorldToView / ViewToClip) held on a frame.
    static constexpr int8_t kMatrixKeyNotRead = -1; // ResolveCameraMatrices did not run on this frame
    static constexpr int8_t kMatrixKeyAbsent = 0;   // the title never set the key
    static constexpr int8_t kMatrixKeyNull = 1;     // the key exists and holds nullptr (the stock Unreal plugin)
    static constexpr int8_t kMatrixKeySet = 2;

    // Submits the deferred denoiser dispatch list (DeferredDispatch mode) to
    // the title's direct queue. Called from the present path, after every
    // title submission of the frame.

  private:

    struct DenoiserConfiguration
    {
        static constexpr uint32_t kScalarCount = FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD;
        static constexpr uint32_t kKeyCount = FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS;

        // Ordered by FfxApiConfigureDenoiserKey
        union
        {
            struct
            {
                float m_CrossBilateralNormalStrength;
                float m_StabilityBias;
                float m_MaxRadiance;
                float m_RadianceClipStdK;
                float m_GaussianKernelRelaxation;
                float m_DisocclusionThreshold;
            };

            float ScalarValues[kScalarCount];
        };

        FfxApiFloatBounds m_DebugViewLinearDepthBounds;

        static FfxApiConfigureDenoiserKey GetIndexKey(int index)
        {
            index = std::clamp(index + 1, 1, static_cast<int>(kKeyCount));
            return static_cast<FfxApiConfigureDenoiserKey>(index);
        }

        void* GetData(FfxApiConfigureDenoiserKey key)
        {
            if (key == FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS)
                return &m_DebugViewLinearDepthBounds;

            const int index = static_cast<int>(key) - 1;
            return index >= 0 && index < static_cast<int>(kScalarCount)
                ? &ScalarValues[index]
                : nullptr;
        }
    };

    ffxContext _pDenoiserCtx;
    ffxCreateContextDescDenoiser _denoiserCtxDesc;
    // Version parsed from the selected RR provider name. Kept separate from
    // FSR31Feature::_version so the SR upscaler version that Version() reports -
    // and every Version()-gated SR behaviour relies on - is never overwritten by
    // the denoiser provider version on context (re)creation.
    feature_version _denoiserVersion {};
    DenoiserConfiguration _denoiserSettings;
    // AMD's queried baseline, captured before the per-frame configure pass starts
    // overwriting _denoiserSettings with the INI values. Retained so the A/B switch
    // can restore it without recreating the context.
    DenoiserConfiguration _denoiserAmdDefaults {};
    ffxStructType_t _diffuseSignalDescType = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE;
    ffxStructType_t _specularSignalDescType = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR;
    // Single-signal denoising (DenoiseDiffuse/DenoiseSpecular ini keys): disabled
    // signals are neither declared at context creation nor dispatched.
    bool _denoiseDiffuse = true;
    bool _denoiseSpecular = true;
    // An unset INI value means Auto. Start safely in direct mode, then resolve
    // exactly once from the first frame's validated hit-distance resources.
    ffxStructType_t _autoSpecularSignalDescType = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR;
    bool _autoSpecularSignalResolved = false;
    // Same contract for diffuse: unset means Auto, resolved once from the first
    // validated frame that either has a diffuse ray length or does not.
    ffxStructType_t _autoDiffuseSignalDescType = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE;
    bool _autoDiffuseSignalResolved = false;
    bool _ambientOcclusionEnabled = false;
    bool _specularOcclusionEnabled = false;
    Microsoft::WRL::ComPtr<ID3D12Resource> _ambientOcclusionNoisy;
    Microsoft::WRL::ComPtr<ID3D12Resource> _ambientOcclusionDenoised;
    D3D12_RESOURCE_STATES _ambientOcclusionNoisyState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES _ambientOcclusionDenoisedState = D3D12_RESOURCE_STATE_COMMON;

    enum class RoughnessSource : uint8_t
    {
        Unknown,
        Separate,
        Packed
    };

    // Depth-type interpretation. NGX reports it at creation, but a title that
    // publishes nothing leaves it at Linear, and reading a hardware depth buffer as
    // linear distance collapses every scene depth into [near, 1]. Keep the reported
    // value and its presence separate from the effective one so the user can
    // override it and so the log can say which of the two is in play.
    // Latched across instances. NGX publishes the depth type only on a creation
    // carrying DLSSD create params, so a recreation can arrive without it; the type
    // itself does not change mid-session, and losing it drops the decision back onto
    // an inference that has to guess.
    // Static on purpose, not by accident: the depth type is a property of the title, not of one
    // feature instance. NGX publishes it only on a creation that carries DLSSD create params, so
    // a recreation - a resolution or preset change, or a backend switch - can arrive without it,
    // and re-deriving per instance would lose a declaration the title already made. See the
    // reuse path in the constructor for what that costs when it is lost.
    static bool s_ngxDepthTypeSeen;
    static bool s_ngxReportedHWDepth;

    bool _isHWDepth = false;
    bool _ngxReportedHWDepth = false;
    bool _hasNGXDepthType = false;
    int _appliedHardwareDepth = -1;
    RoughnessSource _roughnessSource = RoughnessSource::Unknown;

    FSRDConvDesc _convDesc;
    // Diagnostic-only DLSS-RR probes. These are not bound to the converter or RR dispatch yet.
    ID3D12Resource* _diffuseHitDistanceProbe = nullptr;
    ID3D12Resource* _diffuseRayDirectionHitDistanceProbe = nullptr;
    ID3D12Resource* _emissiveProbe = nullptr;
    ID3D12Resource* _materialIdProbe = nullptr;
    ID3D12Resource* _shadingModelIdProbe = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> _emissiveTaggedResource;
    // Streamline tags only guarantee the native pointer for the declared
    // lifecycle. Retain whichever optional reprojection sources win selection
    // until this instance has finished submitting the frame.
    Microsoft::WRL::ComPtr<ID3D12Resource> _specularHitDistanceTaggedResource;
    // Retained per frame: the tag path hands back a resource whose lifetime the caller
    // must hold for as long as the command list that reads it.
    Microsoft::WRL::ComPtr<ID3D12Resource> _titleLinearDepthTaggedResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> _responsivityMaskTaggedResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> _specularRayDirectionHitDistanceTaggedResource;
    // Instance-local suppression state for the responsivity binding log. Function-static
    // state races when multiple feature instances evaluate concurrently and lets one instance
    // suppress another instance's first diagnostic.
    ID3D12Resource* _loggedResponsivityMask = nullptr;
    uint64_t _loggedResponsivityWidth = 0;
    uint32_t _loggedResponsivityHeight = 0;
    DXGI_FORMAT _loggedResponsivityViewFormat = DXGI_FORMAT_UNKNOWN;
    float _loggedResponsivityThreshold = -1.0f;
    bool _loggedResponsivityInvert = false;
    std::array<uint64_t, static_cast<size_t>(RRTaggedSignal::Count)>
        _lastConsumedSLTagUpdates {};
    std::array<uint32_t, static_cast<size_t>(RRTaggedSignal::Count)>
        _lastConsumedSLTagFrames {};
    bool _emissiveProbeCompatible = false;
    bool _emissiveProbeFromStreamline = false;
    uint32_t _diffuseHitDistanceBaseX = 0;
    uint32_t _diffuseHitDistanceBaseY = 0;
    uint32_t _diffuseRayDirectionHitDistanceBaseX = 0;
    uint32_t _diffuseRayDirectionHitDistanceBaseY = 0;
    DirectX::XMFLOAT3 _lastCamPos {}; // Last successfully dispatched world-space camera position
    DirectX::XMFLOAT2 _previousDenoiserJitter {};
    float _appliedRoughnessFloor = -1.0f;
    int _appliedNormalsInViewSpace = -1;
    // Path-traced profile state this instance last ran with (-1 before InitFSR3). A change
    // drops this instance's history once; the Config keys themselves are synced process-wide.
    int _appliedPathTracedProfile = -1;
    // RR-04 (AMDNR 0.3.4): the profile's temporal half this instance last ran with (-1 before InitFSR3). It
    // follows the profile unless [FSR-RR] FfxDenoiserPathTracedTemporal=false; a switch of the half alone is
    // logged ([RR_PROFILE]) and, like a temporal slider, keeps history.
    int _appliedPathTracedTemporal = -1;
    // Texture route last written to [RR_PROFILE], and a live change waiting to hold still
    // (a slider drag logs once, after the value stops changing).
    float _loggedTextureRoute = -1.0f;
    float _pendingTextureRoute = -1.0f;
    uint32_t _pendingTextureRouteFrames = 0;
    // [RR_GUIDES] is logged once on the first evaluate (RR may never dispatch) and once on
    // the first successful RR dispatch.
    bool _loggedRRGuidesEvaluate = false;
    bool _loggedRRGuidesDispatch = false;
    // [RR_POST]: what happens to RR's output after it (FSR SR sharpening, RCAS, masks), once.
    bool _loggedRRPost = false;
    // RR-14 (AMDNR 0.3.4): [RR_POST] is logged again when what it reports changes (FSR SR sharpening, RCAS, skin
    // smoothing, the SSS guide, the bias mask or its strength) and the change held for kRRPostSettleFrames RR
    // frames. It used to be logged only on the first RR frame, which in RE Requiem is the title screen.
    struct RRPostState
    {
        bool rcasSwitch = false;       // [Sharpness] RCAS on (it replaces FSR SR sharpening)
        bool profileSuppressed = false; // the path-traced profile kept the title's sharpness off FSR SR
        bool fsrSharpening = false;
        float fsrSharpness = 0.0f;
        bool overrideSharpness = false;
        bool rrDefaultSharpness = false; // AMDNR 0.3.4.1: kRrDefaultSharpness in use (the title sends none)
        bool rcas = false;
        float rcasSharpness = 0.0f;
        bool motionSharpening = false;
        bool skinSmoothed = false;
        bool skinShowMask = false;
        float skinStrength = 0.0f;
        int skinRadius = 0;
        int skinClassifier = 0;
        int sssGuide = -1;
        float biasStrength = 0.0f;
        int biasMask = -1;

        bool operator==(const RRPostState&) const = default;
    };
    static constexpr uint32_t kRRPostSettleFrames = 30;
    RRPostState _loggedRRPostState {};
    RRPostState _pendingRRPostState {};
    uint32_t _pendingRRPostFrames = 0;
    // RR-09 (AMDNR 0.3.4): the specular hit-distance selection last written as [RR_INPUT]. It used to be a
    // per-frame line; now it is logged on this instance's first evaluate and when a new selection has held for
    // kSpecularHitSettleFrames frames (a title that publishes it on some frames only cannot flood the log). The
    // render extent is printed but is not part of the comparison, so dynamic resolution does not re-log it.
    struct SpecularHitSelection
    {
        const char* source = nullptr; // nullptr: nothing selected
        uint32_t baseX = 0;
        uint32_t baseY = 0;
        uint64_t width = 0;
        uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        bool combinedAlpha = false;

        bool operator==(const SpecularHitSelection&) const = default;
    };
    static constexpr uint32_t kSpecularHitSettleFrames = 30;
    SpecularHitSelection _loggedSpecularHit {};
    SpecularHitSelection _pendingSpecularHit {};
    uint32_t _pendingSpecularHitFrames = 0;
    bool _hasLoggedSpecularHit = false;
    // RR-08 (AMDNR 0.3.4): the title's DLSS bias mask on this instance's last frame whose inputs were read
    // (-1 not looked at yet, 0 not published, 1 published), for [RR_PROFILE] and [RR_POST].
    int _biasMaskState = -1;
    // RR-08: a context's [RR_PROFILE] line waits for its first evaluate (after the inputs were read), so it can
    // say whether the title publishes a bias mask. The reason to log it with, or null.
    const char* _pendingRRProfileReason = nullptr;
    // RR-12 (AMDNR 0.3.4): the live RR settings last logged as "[RR_CFG] settings changed", and a change waiting
    // to hold still for kRRSettingsSettleFrames frames (a slider drag logs once, after it stops). The first
    // evaluate records them without a line ([RR_DIAG] configure baseline and [RR_PROFILE] already have them).
    struct RRLiveSettings
    {
        std::array<float, 6> temporal {}; // requested disoc, stab, normal, gauss, clipK, maxRad
        uint64_t debugMode = 0;
        bool amdDefaults = false;

        bool operator==(const RRLiveSettings&) const = default;
    };
    static constexpr uint32_t kRRSettingsSettleFrames = 30;
    RRLiveSettings _loggedRRSettings {};
    RRLiveSettings _pendingRRSettings {};
    uint32_t _pendingRRSettingsFrames = 0;
    bool _hasLoggedRRSettings = false;

    // SAT-P0-2 (AMDNR 0.3.4): the camera matrix keys on this frame (kMatrixKey*), reset before every
    // PrepareDenoiserInput, and whether this frame's camera stayed unresolved with both keys present but null
    // (the stock Unreal DLSS plugin's signature). [RR_CAM] matrix keys is written once per handle.
    int8_t _viewMatrixKeyState = kMatrixKeyNotRead;
    int8_t _projMatrixKeyState = kMatrixKeyNotRead;
    bool _cameraNullKeysThisFrame = false;
    bool _loggedCameraKeys = false;
    // SAT-P0-1 (AMDNR 0.3.4): the [RR_INPUT] snapshot of every input and declaration, once per handle.
    bool _loggedInputSnapshot = false;
    // SAT-P0-3 (AMDNR 0.3.4): [RR_CAM] streamline, on the first evaluate and once more (at the fallback or after
    // 300 evaluates), per handle.
    bool _loggedStreamlineFirst = false;
    bool _loggedStreamlineLater = false;
    // SAT-P1 (AMDNR 0.3.4): the first Streamline-constants jitter match of an NGX-direct evaluate, once per handle.
    bool _loggedSLJitterMatch = false;
    // SAT-P0-4 (AMDNR 0.3.4): this frame's camera could not be resolved (reset before every PrepareDenoiserInput),
    // and the content probe that runs only then: created on the first such frame, once per handle.
    bool _cameraBlockedThisFrame = false;
    std::unique_ptr<RRContentProbe> _contentProbe;
    // RR-22 (AMDNR 0.3.4): how many of the current run of missing-input frames (_missingInputFrames) had that
    // signature. The fallback latches Ray Regeneration off for the session only when every frame of the run had it.
    uint32_t _nullCameraKeyFrames = 0;

    // Why RR history was dropped, counted per frame and reported every kHistoryWindowFrames at
    // LOG_INFO ([RR_DIAG] history window). A frame counts once per reason however many checks fired.
    enum class HistoryResetReason : uint8_t
    {
        NgxReset,      // the title set NVSDK_NGX_Parameter_Reset
        RenderSize,    // the logical render size changed (dynamic resolution) or outgrew the ceiling
        InputNotReady, // the frame's RR (or SR) inputs could not be gathered
        ProfileToggle, // the path-traced profile was switched
        DebugView,     // a debug view bypassed the denoiser
        Setting,       // a setting that changes what history means (depth, normals, roughness floor, signals)
        Other,         // context (re)creation, a failed dispatch, AO publication
        Count
    };
    static constexpr uint32_t kHistoryWindowFrames = 600;
    uint32_t _historyResetReasonsThisFrame = 0;
    std::array<uint32_t, static_cast<size_t>(HistoryResetReason::Count)> _historyResetFrames {};
    uint32_t _historyWindowFrames = 0;
    uint32_t _historyWindowDispatches = 0;
    uint32_t _historyWindowResetDispatches = 0;
    uint32_t _historyWindowMinWidth = 0;
    uint32_t _historyWindowMinHeight = 0;
    uint32_t _historyWindowMaxWidth = 0;
    uint32_t _historyWindowMaxHeight = 0;

    // Skin smoothing and the SSS-guide range (AMDNR). See ApplySkinSmoothing.
    uint32_t _sssGuideFrames = 0;     // composed RR frames the pass ran on with a usable guide
    uint32_t _sssStatsNextFrame = 30; // guide frame at which the next range capture is recorded
    uint32_t _sssStatsCaptures = 0;   // captures this instance recorded
    bool _sssMaskWasOn = false;             // the mask view, last frame (its turning on asks for a capture)
    bool _sssStatsRequested = false;        // a capture the mask view asked for, not recorded yet
    bool _sssStatsPendingRequested = false; // the capture waiting to be read is one the mask view asked for
    // The skin smoothing state last logged as [RR_SKIN]: off, asked for with no usable guide, asked for
    // but the pass could not run, or running as Smooth | ShowMask << 1 | Radius << 2 (Radius >= 1, so
    // a running state is >= 4 and never meets the first three). It starts off, so the default logs nothing.
    static constexpr int kSkinStateOff = 0;
    static constexpr int kSkinStateNoGuide = 1;
    static constexpr int kSkinStateFailed = 2;
    int _loggedSkinState = kSkinStateOff;
    const void* _sssGuideChecked = nullptr; // the published guide AcquireSssGuide last judged

    // The definition of the depth field every view-space position in the chain is built from,
    // as one comparable token: which source produced it, and the convention that reads it.
    // A history accumulated under one definition cannot be reprojected onto another, so a
    // change here has to drop it in the same frame rather than let the two meet.
    //
    // The title's resource pointer is deliberately not part of the token. DLSS titles publish
    // a ring of per-frame buffers, so the pointer moves without the field moving, and a token
    // that followed it would reset every frame; a format or subrect change on the same source
    // is a precision or placement change, not a different field.
    struct DepthDefinition
    {
        bool titleLinearDepth = false; // the title's own linearisation, not the derived field
        bool rightHanded = false;      // the sign convention applied to depth

        bool operator==(const DepthDefinition&) const = default;
    };

    // False until a frame has resolved one, so the first definition is not reported - and
    // acted on - as a change from the zero-initialised default.
    bool _hasAppliedDepthDefinition = false;
    DepthDefinition _appliedDepthDefinition {};

    bool _hasDenoiserHistory = false;
    // True once this instance has recorded preprocessor work into a command list.
    // Releasing the converter's textures after that point can free resources an
    // already-submitted command list still references, so the automatic
    // signal-classification path must request a feature rebuild instead.
    bool _preprocessorHasRecordedWork = false;
    bool _viewFromStreamline = false;
    bool _projectionFromStreamline = false;
    bool _logNextDenoiserDispatch = true;
    bool _lastDispatchRequestedReset = false;
    // RR-28 (AMDNR 0.3.4): history-reset transitions dispatched by this instance; the dispatch snapshot is logged for
    // the first 3 and every 20th (RE Requiem resets on every camera cut: ~6,000 snapshot lines in 40 minutes).
    uint32_t _resetTransitionCount = 0;
    uint32_t _lastDenoiserRenderWidth = 0;
    uint32_t _lastDenoiserRenderHeight = 0;
    uint64_t _denoiserDispatchAttempts = 0;
    uint64_t _denoiserDispatchSuccesses = 0;
    uint64_t _denoiserDispatchFailures = 0;


    // One-shot GPU probe of the RR diffuse path: copies the denoiser's diffuse
    // OUTPUT and its (demodulated) INPUT signal into readback buffers on the
    // deferred list, and logs luma statistics once the fence retires them.
    // Distinguishes "output is zero", "output is a passthrough of the input"
    // and "output is actually smoothed" without trusting any visual reading.
    Microsoft::WRL::ComPtr<ID3D12Resource> _probeReadback[6];
    DXGI_FORMAT _probeFormats[6] = {};

    // Matrices
    // Row-major storage with column-vector multiplication semantics.
    DirectX::XMMATRIX _invViewMatrix;   // Camera rotation and translation
    DirectX::XMMATRIX _viewMatrix;      // World to camera space
    DirectX::XMMATRIX _prevViewMatrix;  // Last world to camera space
    DirectX::XMMATRIX _projMatrix;      // Unjittered perspective projection
    bool _isRightHanded;                // True if the camera matrix is right handed

    std::unique_ptr<FSRDPreprocessor_Dx12> FSRDConvShader;

    bool InitFSR3(const NVSDK_NGX_Parameter* InParameters) override;

    bool CreateDenoiserContext();

    bool QueryDenoiserVersions();

    void DestroyDenoiserContext();

    // RR-08 (AMDNR 0.3.4): writes a context's waiting [RR_PROFILE] line now, noting why it could not wait for an
    // evaluate that read the inputs (`why`). Nothing when no line is waiting.
    void FlushPendingRRProfile(const char* why);

    bool UpdateSize();

    /**
     * @brief Generates FFX denoiser configuration and input buffers from DLSS-RR inputs and NGX configurations.
     * Converts and repacks resources internally.
     */
    bool PrepareDenoiserInput(ID3D12GraphicsCommandList* InCommandList, const NVSDK_NGX_Parameter& ngxParams,
                              ffxDispatchDescDenoiser& dispatchDesc,
                              ffxDispatchDescDenoiserAmbientOcclusion& ambientOcclusion,
                              ffxDispatchDescDenoiserDirectDiffuse& directDiffuse,
                              ffxDispatchDescDenoiserIndirectSpecular& indirectSpecular);

    bool AcquireTaggedAmbientOcclusionResources(bool logFailure);
    bool PublishAmbientOcclusionOutput(ID3D12GraphicsCommandList* commandList);

    // What declared D3D12 resource states a Streamline tag may carry when acquired.
    // Every policy shares the same frame/viewport/lifetime validation; only the
    // state requirement differs.
    enum class TagStatePolicy : uint8_t
    {
        // The tag is read as-is, so the declaration must already carry a
        // shader-readable bit.
        RequireShaderRead,
        // Any declared state is accepted; the consumer owns how the resource is used.
        AnyDeclaredState,
        // Shader-readable states plus COMMON. COMMON carries no readable bit but is
        // legal to transition out of: the consumer records the declared-to-read
        // barrier and hands the resource back through that state.
        AllowCommonTransition,
    };

    bool AcquireSLTaggedResource(
        const RRD3D12SignalTagSnapshot& snapshot, RRTaggedSignal signal,
        const char* sourceName, TagStatePolicy statePolicy,
        Microsoft::WRL::ComPtr<ID3D12Resource>& resource,
        RRTaggedResourceDiagnostic& diagnostic);

    /**
     * @brief Retrieves DLSS-RR inputs to populate the inputs for the interop layer in order to generate
     FSR-RR compatible buffers.
     */
    bool PrepareDenoiseConvInput(const NVSDK_NGX_Parameter& inParams);

    void ResolveSpecularHitDistance(const NVSDK_NGX_Parameter& inParams,
                                    const RRD3D12SignalTagSnapshot& rrTagSnapshot,
                                    uint32_t renderWidth, uint32_t renderHeight,
                                    uint32_t motionWidth, uint32_t motionHeight,
                                    ID3D12Resource* ngxSpecularHitDistance,
                                    ID3D12Resource* ngxSpecularRayDirectionHitDistance,
                                    const DirectX::XMUINT2& ngxSpecularHitDistanceBase,
                                    const DirectX::XMUINT2& ngxSpecularRayDirectionHitDistanceBase);

    void AcquireOptionalInputs(const NVSDK_NGX_Parameter& inParams,
                              const RRD3D12SignalTagSnapshot& rrTagSnapshot,
                              uint32_t renderWidth, uint32_t renderHeight);

    void ResolveDiffuseHitDistance(const NVSDK_NGX_Parameter& inParams,
                                   uint32_t renderWidth, uint32_t renderHeight);

    bool ResolveCameraMatrices(const NVSDK_NGX_Parameter& inParams,
                               const sl::Constants& slData, bool hasCurrentSLConstants);

    bool ResolveSignalTypes(bool isReady, bool hasCurrentSLConstants);


    /**
     * @brief Converts previously retrieved DLSS-RR resources into FSR-RR inputs.
     */
    bool ConvertDenoiserBuffers(ID3D12GraphicsCommandList* InCommandList);

    // Decides whether the title's depth is hardware or already linear, and applies it.
    void ApplyDepthInterpretation();

    // Read-only [RR_GUIDES] line: which optional DLSS-RR guides the title publishes.
    void LogRRGuides(const NVSDK_NGX_Parameter& inParams, const char* stage) const;

    // SAT-P0-3 (AMDNR 0.3.4): one [RR_CAM] streamline line - whether the title feeds Streamline camera constants,
    // how old the last ones are and whether their jitter matches NGX's. `stage`: why it is written now.
    void LogRRStreamlineState(const NVSDK_NGX_Parameter& inParams, const char* stage);

    // SAT-P0-4 (AMDNR 0.3.4): records the depth / normals content probe on a camera-blocked frame (up to its ring
    // size, once per handle), and logs the first capture the GPU has written (polled, never waited for).
    void RecordContentProbe(ID3D12GraphicsCommandList* commandList, const NVSDK_NGX_Parameter& inParams);
    void PollContentProbe();

    // The title's SSS guide, validated for a read at the render extent; null when absent or unusable.
    ID3D12Resource* AcquireSssGuide(const NVSDK_NGX_Parameter& inParams, DirectX::XMUINT2& base);

    // With skin smoothing and its mask off: whether the title publishes the guide, for the overlay,
    // from its NGX slot alone - the resource itself is not touched.
    void NoteSssGuidePresence(const NVSDK_NGX_Parameter& inParams);

    // Skin smoothing (AMDNR, experimental) and the SSS-guide range capture, after composition.
    // rawColor / rawColorBase: the title's colour composition read (the robust classifier reads it).
    // Returns the finished colour FSR SR must read instead of the composition output, or null.
    ID3D12Resource* ApplySkinSmoothing(ID3D12GraphicsCommandList* InCommandList, const NVSDK_NGX_Parameter& inParams,
                                       ID3D12Resource* rawColor, DirectX::XMUINT2 rawColorBase);

    // One [RR_SKIN] line, with the frame, on a change to a state in which the pass does not run:
    // kSkinStateOff, kSkinStateNoGuide or kSkinStateFailed. The same state again logs nothing.
    void LogSkinNotRunning(int state);

    // Logs a landed SSS-guide range capture as [RR_GUIDES] stage=sss-range.
    void PollSssGuideStats();

    // RR-08: "specular=<type> (<auto|ini>) signalFlags=<flags>" for [RR_PROFILE].
    std::string DescribeRRSignals() const;

    // RR-12: logs a live change of the temporal values, the AMD-defaults switch or the debug view once it held
    // still (see _loggedRRSettings). Called once per evaluate.
    void TrackRRLiveSettings();

    // RR-12: the six configured temporal values with their source, for the periodic [RR_CFG] line.
    std::string DescribeRRTemporal() const;

    // One [RR_POST] line: FSR SR sharpening, RCAS, and the reactive / transparency sources. `reason`: why it is
    // logged (first RR frame, or a change, RR-14).
    void LogRRPost(const NVSDK_NGX_Parameter& inParams, const ffxDispatchDescUpscale& upscalerDesc,
                   bool profileSuppressedSharpness, float titleSharpness, bool skinSmoothed, const char* reason);

    // RR-14: the values [RR_POST] reports, as this frame set them (same rules as LogRRPost).
    RRPostState CaptureRRPostState(const ffxDispatchDescUpscale& upscalerDesc, bool profileSuppressedSharpness,
                                   bool skinSmoothed) const;

    // Folds the last frame's reset reasons into the window and logs the window when it is full.
    void AccountHistoryResets();

    /**
     * @brief Dispatches FSR-RR denoiser converted inputs. Runs before upscaler.
     */
    bool DispatchDenoiser(ID3D12GraphicsCommandList* InCommandList, const ffxDispatchDescDenoiser& dispatchDesc);

    void CommitDenoiserHistory() noexcept;

    // Re-derives the conversion inputs that PrepareDenoiseConvInput froze from the value
    // _hasDenoiserHistory had before a change check invalidated it. Every check that resets
    // history after that point has to call this, or the conversion pass reprojects against a
    // previous-frame depth produced under the setting that was just abandoned, on the very
    // frame the RR dispatch resets.
    void RefreshHistoryDerivedInputs() noexcept;

    void InvalidateDenoiserHistory(HistoryResetReason reason = HistoryResetReason::Other) noexcept
    {
        _hasDenoiserHistory = false;
        _lastDispatchRequestedReset = false;
        _historyResetReasonsThisFrame |= 1u << static_cast<uint32_t>(reason);
    }

    bool SetDefaultConfiguration();

    ffxReturnCode_t SetDefaultConfiguration(FfxApiConfigureDenoiserKey key);

    ffxReturnCode_t ApplyConfiguration(FfxApiConfigureDenoiserKey key);
};
