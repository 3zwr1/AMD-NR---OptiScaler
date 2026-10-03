// Origin: FSR Ray Regeneration for OptiScaler by Zach Hembree (DarkHelmet), branch ffx-denoise-experimental
// (GPL-3.0), continued by burak113; ported to AMDNR from burak113's branch at 3da4808.
// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
#include "pch.h"
#include <Config.h>
#include <OptiTypes.h>
#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>
#include <unordered_set>
#include <vector>
#include <nvsdk_ngx_defs_dlssd.h>
#include <DirectXMath.h>
#include <d3d12sdklayers.h>
#include <cmath>
#include "NVNGX_Parameter.h"
#include <misc/SkipSpoof.h>
#include "hooks/Streamline_Hooks.h"
#include "hooks/RrHardwareGate.h" // RrGate::DefaultSharpnessWhenTitleSendsNone (0.3.4.1, Proton RR)
#include "resource_tracking/ResTrack_Dx12.h"
#include "FSRDFeature_Dx12.h"
#include "shaders/fsrd_preprocess/FSRDPreprocessor_Dx12.h"
#include "shaders/fsrd_preprocess/FSRDShaderUtils.h"
#include "shaders/fsrd_preprocess/FSRDRuntimeStatus.h"
#include "dlssnr/amd/SystemCompiler.h"
#include "MathUtils.h"

using namespace DirectX;
using namespace OptiMath;

using FSRDConvDesc = FSRDPreprocessor_Dx12::ConversionDesc;
using FSRDCompDesc = FSRDPreprocessor_Dx12::CompositionDesc;

// RR 1.2 uses the existing FFX effect-id field. These assertions prevent a
// descriptor-id packing change from silently routing RR calls to another module.
static_assert(FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER == 0x00050001u);
static_assert(FFX_API_DISPATCH_DESC_TYPE_DENOISER == 0x00050041u);
static_assert(FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION == 0x00050043u);
static_assert(FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE == 0x00050044u);
static_assert(FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR == 0x00050045u);
static_assert(FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE == 0x00050047u);
static_assert(FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR == 0x00050048u);
static_assert(FFX_API_DISPATCH_DESC_TYPE_DENOISER_SPECULAR_OCCLUSION == 0x00050049u);
static_assert(sizeof(ffxCreateContextDescDenoiser) == 40u);
static_assert(sizeof(ffxDispatchDescDenoiser) == 448u);
static_assert(sizeof(ffxDispatchDescDenoiserAmbientOcclusion) == 120u);
static_assert(sizeof(ffxDispatchDescDenoiserDirectDiffuse) == 120u);
static_assert(sizeof(ffxDispatchDescDenoiserDirectSpecular) == 120u);
static_assert(sizeof(ffxDispatchDescDenoiserIndirectDiffuse) == 120u);
static_assert(sizeof(ffxDispatchDescDenoiserIndirectSpecular) == 120u);
static_assert(sizeof(ffxDispatchDescDenoiserSpecularOcclusion) == 120u);

static bool UseIndirectSignal(const CustomOptional<int>& setting)
{
    return std::clamp(setting.value_or_default(), 0, 1) == 1;
}

static ffxStructType_t GetDiffuseSignalDescType(
    const Config& cfg, ffxStructType_t automaticSignalType)
{
    if (!cfg.FfxDenoiserDiffuseSignalType.has_value())
        return automaticSignalType;

    return UseIndirectSignal(cfg.FfxDenoiserDiffuseSignalType)
        ? FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE
        : FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE;
}

static ffxStructType_t GetSpecularSignalDescType(
    const Config& cfg, ffxStructType_t automaticSignalType)
{
    if (!cfg.FfxDenoiserSpecularSignalType.has_value())
        return automaticSignalType;

    return UseIndirectSignal(cfg.FfxDenoiserSpecularSignalType)
        ? FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR
        : FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR;
}

static uint32_t GetSignalFlag(ffxStructType_t descriptorType)
{
    switch (descriptorType)
    {
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION:
        return FFX_DENOISER_SIGNAL_AMBIENT_OCCLUSION;
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE:
        return FFX_DENOISER_SIGNAL_DIRECT_DIFFUSE;
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR:
        return FFX_DENOISER_SIGNAL_DIRECT_SPECULAR;
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE:
        return FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE;
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR:
        return FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR;
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_SPECULAR_OCCLUSION:
        return FFX_DENOISER_SIGNAL_SPECULAR_OCCLUSION;
    default:
        return FFX_DENOISER_SIGNAL_NONE;
    }
}

static const char* GetSignalTypeName(ffxStructType_t descriptorType)
{
    switch (descriptorType)
    {
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION:
        return "AmbientOcclusion";
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE:
        return "DirectDiffuse";
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR:
        return "DirectSpecular";
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE:
        return "IndirectDiffuse";
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR:
        return "IndirectSpecular";
    case FFX_API_DISPATCH_DESC_TYPE_DENOISER_SPECULAR_OCCLUSION:
        return "SpecularOcclusion";
    default:
        return "Unknown";
    }
}

class DenoiserOutputStateGuard
{
  public:
    DenoiserOutputStateGuard(
        const std::unique_ptr<FSRDPreprocessor_Dx12>& preprocessor,
        ID3D12GraphicsCommandList* commandList) :
        _preprocessor(&preprocessor), _commandList(commandList)
    {
    }

    ~DenoiserOutputStateGuard()
    {
        // Context recreation may replace the preprocessor during an evaluation
        // (for example when Auto resolves a signal type). Follow the owning
        // unique_ptr instead of retaining a raw pointer to the destroyed object.
        if (_preprocessor && *_preprocessor)
            (*_preprocessor)->TransitionDenoiserOutputsToRead(_commandList);
    }

    DenoiserOutputStateGuard(const DenoiserOutputStateGuard&) = delete;
    DenoiserOutputStateGuard& operator=(const DenoiserOutputStateGuard&) = delete;

  private:
    const std::unique_ptr<FSRDPreprocessor_Dx12>* _preprocessor;
    ID3D12GraphicsCommandList* _commandList;
};

// Hands back any title-owned resource the conversion borrowed for the frame. The title
// finds its resource in the state it declared, whatever path this evaluation took.
class TitleInputStateGuard
{
  public:
    TitleInputStateGuard(
        const std::unique_ptr<FSRDPreprocessor_Dx12>& preprocessor,
        ID3D12GraphicsCommandList* commandList) :
        _preprocessor(&preprocessor), _commandList(commandList)
    {
    }

    ~TitleInputStateGuard()
    {
        // Whether anything owes a transition back is the preprocessor's own
        // per-frame record, not a bool snapshotted before this frame's binding
        // decided it - the first frame would otherwise never restore.
        if (_preprocessor && *_preprocessor)
            (*_preprocessor)->RestoreTitleInputStates(_commandList);
    }

    TitleInputStateGuard(const TitleInputStateGuard&) = delete;
    TitleInputStateGuard& operator=(const TitleInputStateGuard&) = delete;

  private:
    const std::unique_ptr<FSRDPreprocessor_Dx12>* _preprocessor;
    ID3D12GraphicsCommandList* _commandList;
};

class EvaluationFrameGuard
{
  public:
    explicit EvaluationFrameGuard(long& frameCount) : _frameCount(frameCount) {}
    ~EvaluationFrameGuard() { ++_frameCount; }

    EvaluationFrameGuard(const EvaluationFrameGuard&) = delete;
    EvaluationFrameGuard& operator=(const EvaluationFrameGuard&) = delete;

  private:
    long& _frameCount;
};

/**
 * @brief Retrieves a column-major NGX matrix into the interop layer's row-major storage,
 * while retaining column-vector multiplication semantics.
 */
static bool TryGetNGXColumnVectorMatrix(const NVSDK_NGX_Parameter& ngxParams, const char* key,
                                        DirectX::XMMATRIX& outValue, int8_t* outKeyState = nullptr)
{
    float* pMat = nullptr;

    const bool present = ngxParams.Get(key, (void**) &pMat) == NVSDK_NGX_Result_Success;
    // SAT-P0-2 (AMDNR 0.3.4): what the key held - absent (the title never set it), present but null (the stock
    // Unreal DLSS plugin: the key exists and holds nullptr, because NVIDIA's Ray Reconstruction treats the
    // matrices as optional), or set.
    if (outKeyState != nullptr)
        *outKeyState = !present ? FSRDFeatureDx12::kMatrixKeyAbsent
                                : (pMat == nullptr ? FSRDFeatureDx12::kMatrixKeyNull : FSRDFeatureDx12::kMatrixKeySet);

    if (present && pMat != nullptr)
    {
        XMMATRIX packedMatrix = {};
        memcpy_s(&packedMatrix, sizeof(packedMatrix), pMat, sizeof(float) * 16);
        outValue = XMMatrixTranspose(packedMatrix);
        return true;
    }

    return false;
}

static const char* MatrixKeyStateName(int8_t state)
{
    switch (state)
    {
    case FSRDFeatureDx12::kMatrixKeyAbsent:
        return "absent";
    case FSRDFeatureDx12::kMatrixKeyNull:
        return "null";
    case FSRDFeatureDx12::kMatrixKeySet:
        return "set";
    default:
        return "not-read";
    }
}

template <typename T>
static bool TryGetLoggedResource(const NVSDK_NGX_Parameter& ngxParams, const char* key, T*& outValue)
{
    const bool success = TryGetNGXVoidPointer(ngxParams, key, outValue);

    if (success)
        LOG_DEBUG("{} exists..", key);
    else
        LOG_ERROR("{} is missing!!", key);

    return success;
}

static XMUINT2 GetSubrectBase(const NVSDK_NGX_Parameter& ngxParams,
                              const char* xKey, const char* yKey)
{
    unsigned int x = 0;
    unsigned int y = 0;
    ngxParams.Get(xKey, &x);
    ngxParams.Get(yKey, &y);
    return { x, y };
}

static bool ValidateSourceExtent(const char* name, ID3D12Resource* resource,
                                 const XMUINT2& base, uint32_t width, uint32_t height)
{
    if (!resource)
        return false;

    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    const bool valid = desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        desc.SampleDesc.Count == 1 && desc.DepthOrArraySize == 1 &&
        uint64_t(base.x) + uint64_t(width) <= desc.Width &&
        uint64_t(base.y) + uint64_t(height) <= desc.Height;
    if (!valid)
    {
        LOG_ERROR(
            "[RR_INPUT] {} does not cover requested Texture2D subrect: resource={}x{}, base=({}, {}), extent={}x{}, dimension={}, arrays={}, samples={}",
            name, desc.Width, desc.Height, base.x, base.y, width, height,
            static_cast<uint32_t>(desc.Dimension), desc.DepthOrArraySize,
            desc.SampleDesc.Count);
    }

    return valid;
}

enum class DepthResourceKind : uint8_t
{
    Unknown,       // Neither format nor flags say anything conclusive.
    HardwareDepth, // Depth-stencil capable, so it cannot hold a linear distance.
    GameWritten,   // Render-target or UAV capable: the title produced these contents.
};

/**
 * @brief Classifies the depth resource from its D3D12 description.
 *
 * A depth-stencil format or ALLOW_DEPTH_STENCIL is conclusive - no engine writes
 * linearized view-space distance into a depth-stencil attachment. Conversely a
 * render-target or UAV capable resource was produced by the title's own shaders,
 * which makes a linear declaration credible. Everything else, typically a plain
 * copy destination or a bare typeless resource, is genuinely undecidable here.
 */
static DepthResourceKind ClassifyDepthResource(ID3D12Resource* depth)
{
    if (!depth)
        return DepthResourceKind::Unknown;

    const D3D12_RESOURCE_DESC desc = depth->GetDesc();

    if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
        return DepthResourceKind::HardwareDepth;

    switch (desc.Format)
    {
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        return DepthResourceKind::HardwareDepth;

    default:
        break;
    }

    if ((desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                       D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)) != 0)
    {
        return DepthResourceKind::GameWritten;
    }

    return DepthResourceKind::Unknown;
}

static const char* DepthResourceKindName(DepthResourceKind kind)
{
    switch (kind)
    {
    case DepthResourceKind::HardwareDepth:
        return "HardwareDepth";
    case DepthResourceKind::GameWritten:
        return "GameWritten";
    default:
        return "Unknown";
    }
}

// SAT-P0-1 (AMDNR 0.3.4): "FORMAT WxH flags=0x.." of an RR input, or "absent", for the one-shot snapshot.
static std::string DescribeRRTexture(ID3D12Resource* resource)
{
    if (!resource)
        return "absent";

    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    const auto formatName = magic_enum::enum_name(desc.Format);
    return std::format("{} {}x{} flags={:#x}", formatName.empty() ? "UNKNOWN" : formatName, desc.Width, desc.Height,
                       static_cast<uint32_t>(desc.Flags));
}

static bool SupportsPackedRoughness(ID3D12Resource* normals)
{
    if (!normals)
        return false;

    switch (normals->GetDesc().Format)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        return true;
    default:
        return false;
    }
}

static void SetColumn(const XMVECTOR& vec, int col, XMMATRIX& mat)
{
    mat.r[0].m128_f32[col] = vec.m128_f32[0];
    mat.r[1].m128_f32[col] = vec.m128_f32[1];
    mat.r[2].m128_f32[col] = vec.m128_f32[2];
    mat.r[3].m128_f32[col] = vec.m128_f32[3];
}

static XMFLOAT3 GetFloat3Column(const XMMATRIX& mat, int col)
{
    return { mat.r[0].m128_f32[col], mat.r[1].m128_f32[col], mat.r[2].m128_f32[col] };
}

// A published matrix is only trustworthy if every element is finite. Titles that do
// not have a value to report sometimes publish a sentinel fill instead of omitting the
// parameter - an all-FLT_MAX matrix is one observed case - and such a matrix passes a
// presence check while making every derived quantity NaN.
static bool MatrixIsFinite(const XMMATRIX& matrix)
{
    for (int row = 0; row < 4; row++)
    {
        for (int column = 0; column < 4; column++)
        {
            if (!std::isfinite(matrix.r[row].m128_f32[column]))
                return false;
        }
    }

    return true;
}

/**
 * @brief Converts the interop layer's column-vector matrix into RR 1.2's canonical
 * row-major, row-vector layout.
 */
static FfxApiMatrix4x4 GetRRMatrix(const XMMATRIX& columnVectorMatrix)
{
    static_assert(sizeof(FfxApiMatrix4x4) == sizeof(XMFLOAT4X4));
    FfxApiMatrix4x4 result = {};
    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&result), XMMatrixTranspose(columnVectorMatrix));
    return result;
}

/**
 * @brief Uploads a column-vector matrix for HLSL's default column-major cbuffer
 * storage and mul(Matrix, Vector) usage.
 */
static void StoreHlslColumnVectorMatrix(XMFLOAT4X4& destination, const XMMATRIX& columnVectorMatrix)
{
    XMStoreFloat4x4(&destination, XMMatrixTranspose(columnVectorMatrix));
}

/**
 * @brief Creates an unjittered perspective projection in the interop layer's
 * column-vector convention. A zero far distance is treated as an infinite far plane.
 */
static XMMATRIX CreateColumnVectorPerspectiveProjection(float verticalFov, float aspectRatio,
                                                        float nearPlane, float farPlane,
                                                        bool isRightHanded, bool isDepthInverted)
{
    XMMATRIX rowVectorProjection = {};

    if (farPlane == 0.0f)
    {
        const float yScale = 1.0f / std::tan(verticalFov * 0.5f);
        const float xScale = yScale / aspectRatio;
        const float W = isRightHanded ? -1.0f : 1.0f;
        const float A = isDepthInverted ? 0.0f : W;
        const float B = isDepthInverted ? nearPlane : -nearPlane;

        rowVectorProjection = XMMatrixSet(
            xScale, 0.0f,   0.0f, 0.0f,
            0.0f,   yScale, 0.0f, 0.0f,
            0.0f,   0.0f,   A,    W,
            0.0f,   0.0f,   B,    0.0f);
    }
    else
    {
        // Swapping the physical near/far arguments produces a reversed-Z projection.
        const float matrixNear = isDepthInverted ? farPlane : nearPlane;
        const float matrixFar = isDepthInverted ? nearPlane : farPlane;

        rowVectorProjection = isRightHanded
            ? XMMatrixPerspectiveFovRH(verticalFov, aspectRatio, matrixNear, matrixFar)
            : XMMatrixPerspectiveFovLH(verticalFov, aspectRatio, matrixNear, matrixFar);
    }

    return XMMatrixTranspose(rowVectorProjection);
}

static ID3D12Resource* GetD3D12ResFromFFX(const FfxApiResource& resource)
{
    return static_cast<ID3D12Resource*>(resource.resource);
}

struct RequiredRRResource
{
    const char* name;
    FfxApiResource resource;
    DXGI_FORMAT format;
};

using RequiredRRResources = std::vector<RequiredRRResource>;

// albedoFormat: what the converter stores the albedos in (RR-05, AMDNR 0.3.4: R16G16B16A16_FLOAT only with
// [FSR-RR] FfxDenoiserAlbedoFp16=true; R8G8B8A8_UNORM as in 0.3.3.2 otherwise).
static RequiredRRResources GetRequiredRRResources(
    const ffxDispatchDescDenoiser& dispatchDesc,
    const ffxDispatchDescDenoiserDirectDiffuse& directDiffuse,
    const ffxDispatchDescDenoiserIndirectSpecular& indirectSpecular,
    const ffxDispatchDescDenoiserAmbientOcclusion* ambientOcclusion,
    DXGI_FORMAT albedoFormat = DXGI_FORMAT_R8G8B8A8_UNORM)
{
    RequiredRRResources resources {
        { "LinearDepth", dispatchDesc.linearDepth, DXGI_FORMAT_R32_FLOAT },
        { "MotionVectors", dispatchDesc.motionVectors, DXGI_FORMAT_R16G16B16A16_FLOAT },
        { "Normals", dispatchDesc.normals, DXGI_FORMAT_R10G10B10A2_UNORM },
        { "SpecularAlbedo", dispatchDesc.specularAlbedo, albedoFormat },
        { "DiffuseAlbedo", dispatchDesc.diffuseAlbedo, albedoFormat },
        { "DiffuseSignal.Input", directDiffuse.signal.input, DXGI_FORMAT_R16G16B16A16_FLOAT },
        { "DiffuseSignal.Output", directDiffuse.signal.output, DXGI_FORMAT_R16G16B16A16_FLOAT },
        { "SpecularSignal.Input", indirectSpecular.signal.input, DXGI_FORMAT_R16G16B16A16_FLOAT },
        { "SpecularSignal.Output", indirectSpecular.signal.output, DXGI_FORMAT_R16G16B16A16_FLOAT },
    };

    if (ambientOcclusion)
    {
        resources.push_back(
            { "AmbientOcclusion.Input", ambientOcclusion->signal.input, DXGI_FORMAT_R8_UNORM });
        resources.push_back(
            { "AmbientOcclusion.Output", ambientOcclusion->signal.output, DXGI_FORMAT_R8_UNORM });
    }

    return resources;
}

static bool ValidateRequiredRRResources(const ffxDispatchDescDenoiser& dispatchDesc,
                                        const ffxDispatchDescDenoiserDirectDiffuse& directDiffuse,
                                        const ffxDispatchDescDenoiserIndirectSpecular& indirectSpecular,
                                        const ffxDispatchDescDenoiserAmbientOcclusion* ambientOcclusion,
                                        DXGI_FORMAT albedoFormat)
{
    const RequiredRRResources requirements =
        GetRequiredRRResources(dispatchDesc, directDiffuse, indirectSpecular, ambientOcclusion, albedoFormat);

    bool valid = true;

    for (const auto& requirement : requirements)
    {
        ID3D12Resource* resource = GetD3D12ResFromFFX(requirement.resource);

        if (!resource)
        {
            LOG_ERROR("Required RR 1.2 resource {} is null", requirement.name);
            valid = false;
            continue;
        }

        const D3D12_RESOURCE_DESC desc = resource->GetDesc();

        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
            desc.SampleDesc.Count != 1 || desc.DepthOrArraySize != 1 ||
            desc.Width < dispatchDesc.renderSize.width ||
            desc.Height < dispatchDesc.renderSize.height)
        {
            LOG_ERROR("Required RR 1.2 resource {} has unsupported layout or insufficient coverage: dimension={}, arraySize={}, samples={}, size={}x{}; required coverage={}x{}",
                      requirement.name, magic_enum::enum_name(desc.Dimension),
                      desc.DepthOrArraySize, desc.SampleDesc.Count, desc.Width, desc.Height,
                      dispatchDesc.renderSize.width, dispatchDesc.renderSize.height);
            valid = false;
        }

        const DXGI_FORMAT viewFormat = FSRD::GetViewFormat(desc.Format);

        if (viewFormat != requirement.format)
        {
            LOG_ERROR("Required RR 1.2 resource {} has incompatible format {}; expected {}",
                      requirement.name, magic_enum::enum_name(viewFormat),
                      magic_enum::enum_name(requirement.format));
            valid = false;
        }
    }

    return valid;
}

static void LogRRDispatchSnapshot(const ffxDispatchDescDenoiser& dispatchDesc,
                                  const ffxDispatchDescDenoiserDirectDiffuse& directDiffuse,
                                  const ffxDispatchDescDenoiserIndirectSpecular& indirectSpecular,
                                  const ffxDispatchDescDenoiserAmbientOcclusion* ambientOcclusion)
{
    ID3D12GraphicsCommandList* commandList =
        static_cast<ID3D12GraphicsCommandList*>(dispatchDesc.commandList);
    const D3D12_COMMAND_LIST_TYPE commandListType =
        commandList ? commandList->GetType() : D3D12_COMMAND_LIST_TYPE(-1);

    LOG_INFO(
        "[RR_DIAG] dispatch snapshot: frame={}, reset={}, render={}x{}, commandList={:X}, commandListType={}, "
        "depthBounds=[{:.6f}, {:.6f}], mvScale=[{:.6f}, {:.6f}, {:.6f}], jitterPixels=[{:.6f}, {:.6f}], "
        "cameraDelta=[{:.6f}, {:.6f}, {:.6f}], flags={:#x}",
        dispatchDesc.frameIndex,
        !!(dispatchDesc.flags & FFX_DENOISER_DISPATCH_RESET),
        dispatchDesc.renderSize.width, dispatchDesc.renderSize.height,
        reinterpret_cast<uintptr_t>(dispatchDesc.commandList),
        magic_enum::enum_name(commandListType),
        dispatchDesc.linearDepthBounds.min, dispatchDesc.linearDepthBounds.max,
        dispatchDesc.motionVectorScale.x, dispatchDesc.motionVectorScale.y, dispatchDesc.motionVectorScale.z,
        dispatchDesc.jitterOffsets.x, dispatchDesc.jitterOffsets.y,
        dispatchDesc.cameraPositionDelta.x, dispatchDesc.cameraPositionDelta.y,
        dispatchDesc.cameraPositionDelta.z, dispatchDesc.flags);

    std::string chain = std::format("head={:#x}", dispatchDesc.header.type);
    for (const ffxDispatchDescHeader* signal = dispatchDesc.header.pNext;
         signal != nullptr; signal = signal->pNext)
    {
        chain += std::format(" -> {}={:#x}", GetSignalTypeName(signal->type), signal->type);
    }
    LOG_INFO("[RR_DIAG] chain: {} -> tail=0x0", chain);

    const RequiredRRResources requirements =
        GetRequiredRRResources(dispatchDesc, directDiffuse, indirectSpecular, ambientOcclusion);

    for (const RequiredRRResource& requirement : requirements)
    {
        ID3D12Resource* resource = GetD3D12ResFromFFX(requirement.resource);
        if (!resource)
        {
            LOG_ERROR("[RR_DIAG] resource {}: null", requirement.name);
            continue;
        }

        const D3D12_RESOURCE_DESC desc = resource->GetDesc();
        LOG_INFO(
            "[RR_DIAG] resource {}: ptr={:X}, size={}x{}, format={}, dimension={}, mips={}, samples={}, "
            "resourceFlags={:#x}, declaredFfxState={:#x}",
            requirement.name, reinterpret_cast<uintptr_t>(resource),
            desc.Width, desc.Height, magic_enum::enum_name(FSRD::GetViewFormat(desc.Format)),
            magic_enum::enum_name(desc.Dimension), desc.MipLevels, desc.SampleDesc.Count,
            static_cast<uint32_t>(desc.Flags), requirement.resource.state);
    }
}

static bool IsDiffuseHitDistanceFormat(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R16_FLOAT || format == DXGI_FORMAT_R32_FLOAT;
}

// A responsivity hint is a per-pixel scalar. Titles have been seen to publish it under
// their own NGX key, so the format is accepted broadly and only its coverage is required.
static bool IsResponsivityMaskFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return true;
    default:
        return false;
    }
}

static bool IsDiffuseRayDirectionHitDistanceFormat(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
           format == DXGI_FORMAT_R32G32B32A32_FLOAT;
}

static bool CoversSourceExtent(ID3D12Resource* resource, const XMUINT2& base,
                               uint32_t width, uint32_t height)
{
    if (!resource)
        return false;

    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    return desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        desc.SampleDesc.Count == 1 &&
        uint64_t(base.x) + uint64_t(width) <= desc.Width &&
        uint64_t(base.y) + uint64_t(height) <= desc.Height;
}

using SourceFormatValidator = bool (*)(DXGI_FORMAT);

static bool ValidateReprojectionGuideSource(
    const char* name, ID3D12Resource* resource, const XMUINT2& base,
    uint32_t width, uint32_t height, SourceFormatValidator validateFormat,
    const char* expectedFormat)
{
    if (!resource)
        return false;

    const DXGI_FORMAT viewFormat = FSRD::GetViewFormat(resource->GetDesc().Format);
    if (!validateFormat(viewFormat))
    {
        LOG_ERROR("[RR_INPUT] {} has incompatible format {}; expected {}",
                  name, magic_enum::enum_name(viewFormat), expectedFormat);
        return false;
    }

    return ValidateSourceExtent(name, resource, base, width, height);
}

static bool IsEmissiveProbeCompatible(ID3D12Resource* resource)
{
    if (!resource)
        return false;

    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
        desc.Width == 0 || desc.Height == 0 ||
        desc.SampleDesc.Count != 1 || desc.DepthOrArraySize != 1)
    {
        return false;
    }

    const DXGI_FORMAT viewFormat = FSRD::GetViewFormat(desc.Format);
    switch (viewFormat)
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
}

static void LogRREmissiveProbe(ID3D12Resource* resource,
                               bool compatible,
                               bool fromStreamline,
                               uint32_t renderWidth,
                               uint32_t renderHeight)
{
    if (!resource)
    {
        LOG_INFO("[RR_DIAG] emissive probe: absent (checked NGX {} and Streamline Emissive tag)",
                 NVSDK_NGX_Parameter_GBuffer_Emissive);
        return;
    }

    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    const DXGI_FORMAT viewFormat = FSRD::GetViewFormat(desc.Format);
    LOG_INFO(
        "[RR_DIAG] emissive probe: present, source={}, ptr={:X}, size={}x{}, format={}, dimension={}, mips={}, samples={}, "
        "resourceFlags={:#x}, renderSize={}x{}, exactRenderSize={}, previewCompatible={}",
        fromStreamline ? "Streamline.Emissive" : NVSDK_NGX_Parameter_GBuffer_Emissive,
        reinterpret_cast<uintptr_t>(resource),
        desc.Width, desc.Height, magic_enum::enum_name(viewFormat),
        magic_enum::enum_name(desc.Dimension), desc.MipLevels, desc.SampleDesc.Count,
        static_cast<uint32_t>(desc.Flags), renderWidth, renderHeight,
        desc.Width == renderWidth && desc.Height == renderHeight,
        compatible);
}

static void LogRRDiffuseHitDistanceProbe(const char* parameterName,
                                         ID3D12Resource* resource,
                                         uint32_t subrectBaseX,
                                         uint32_t subrectBaseY,
                                         uint32_t renderWidth,
                                         uint32_t renderHeight,
                                         bool combinedDirectionAndDistance)
{
    if (!resource)
    {
        LOG_INFO("[RR_DIAG] NGX probe {}: absent", parameterName);
        return;
    }

    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    const DXGI_FORMAT viewFormat = FSRD::GetViewFormat(desc.Format);
    const bool compatibleFormat = combinedDirectionAndDistance
        ? IsDiffuseRayDirectionHitDistanceFormat(viewFormat)
        : IsDiffuseHitDistanceFormat(viewFormat);
    const uint64_t requiredWidth = static_cast<uint64_t>(subrectBaseX) + renderWidth;
    const uint64_t requiredHeight = static_cast<uint64_t>(subrectBaseY) + renderHeight;
    const bool coversRenderArea =
        desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        desc.Width >= requiredWidth && desc.Height >= requiredHeight;

    LOG_INFO(
        "[RR_DIAG] NGX probe {}: present, ptr={:X}, size={}x{}, format={}, dimension={}, mips={}, samples={}, "
        "resourceFlags={:#x}, subrectBase=[{}, {}], expected={}, compatibleFormat={}, coversRenderArea={}",
        parameterName, reinterpret_cast<uintptr_t>(resource),
        desc.Width, desc.Height, magic_enum::enum_name(viewFormat),
        magic_enum::enum_name(desc.Dimension), desc.MipLevels, desc.SampleDesc.Count,
        static_cast<uint32_t>(desc.Flags), subrectBaseX, subrectBaseY,
        combinedDirectionAndDistance ? "RGBA16_FLOAT/RGBA32_FLOAT (distance in A)"
                                     : "R16_FLOAT/R32_FLOAT",
        compatibleFormat, coversRenderArea);
}

static void LogRRGBufferIdentityProbe(const char* parameterName,
                                      ID3D12Resource* resource,
                                      uint32_t renderWidth,
                                      uint32_t renderHeight)
{
    if (!resource)
    {
        LOG_INFO("[RR_DIAG] NGX identity probe {}: absent", parameterName);
        return;
    }

    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    const DXGI_FORMAT viewFormat = FSRD::GetViewFormat(desc.Format);
    const bool exactRenderSize =
        desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        desc.Width == renderWidth && desc.Height == renderHeight;

    LOG_INFO(
        "[RR_DIAG] NGX identity probe {}: present, ptr={:X}, size={}x{}, format={}, dimension={}, mips={}, samples={}, "
        "resourceFlags={:#x}, exactRenderSize={}; diagnostic-only, value encoding and temporal stability are unverified",
        parameterName, reinterpret_cast<uintptr_t>(resource),
        desc.Width, desc.Height, magic_enum::enum_name(viewFormat),
        magic_enum::enum_name(desc.Dimension), desc.MipLevels, desc.SampleDesc.Count,
        static_cast<uint32_t>(desc.Flags), exactRenderSize);
}

// FSR-RR path-traced profile ([FSR-RR] FfxDenoiserPathTracedProfile). The fork's defaults split
// the colour into a spatial floor that bypasses RR through the skip signal and blend the raw
// colour back in after RR - tuned on a hybrid raster+RT title. AMD's sample contract for a fully
// path-traced frame sends all lit radiance into RR and re-injects nothing raw. The profile is a
// set of custom defaults: a key that already has a value of its own (ini, menu, another quirk)
// keeps it. The temporal keys take AMD's documented defaults (hard-coded below, not the
// _denoiserAmdDefaults queried at runtime), except disocclusion, held at the top of AMD's sample
// range (0.05; AMD's own default is 0.01) because the depth delta is still camera-only for moving
// objects. Pixels the game's bias mask flags still take the floor/skip path
// (FfxDenoiserBiasMaskStrength is outside the profile). Outside these keys, the profile also keeps
// the title's NGX sharpness off FSR SR after RR (EvaluateInternal, after PrepareUpscalerInput).
// Every key below is read per frame (conversion, configure, composition), none only at init.
// FfxDenoiserProfileTextureRoute (not one of them) hands textured albedo back to the fork's
// floor and raw blend while the profile is on; at 0 the profile routes as in 0.3.3.
//
// Config is process-wide, so the record of which keys the profile wrote is too. Switching the
// profile off hands exactly those back to auto, unless something has set them since.
//
// RR-04 (AMDNR 0.3.4): the profile is two halves. The routing half (indices 0-3: floor isolation, raw blend,
// correlation bias, handover) follows FfxDenoiserPathTracedProfile. The temporal half (indices 4-9: the six
// configure values) follows it only while [FSR-RR] FfxDenoiserPathTracedTemporal is true, the default, which
// writes the same ten values as 0.3.3.2. With it false the six keys keep the values they have without the
// profile (the fork default, a quirk's value or the player's own), so the temporal half can be A/B'd alone.
//
// RR-25 (AMDNR 0.3.4): retired from the menu. Every RE Requiem report found it worse than the normal route (tester 1:
// etched textures; tester 2: lamps and light far too bright, hair gone; a Proton player on 0.3.3.2 with Texture route
// 1.00: "way way worse" at a lamp). The texture route only hands textured albedo back; lamps, glass, plain walls, fog,
// skin and hair keep the whole profile, and CorrelationBias 0 / FloorHandover 0 apply on every pixel. Its one gain
// (faces) is covered by skin smoothing. Nothing here changed: the keys, their defaults and the ten values still work
// from the ini for testing; LogRRProfile warns once when it is on. A real "all light through RR" profile needs
// demodulation by local regression (RR-18), not more routing.
static bool s_pathTracedProfileApplied = false;
static bool s_pathTracedTemporalApplied = false;
static uint32_t s_pathTracedProfileOwnedKeys = 0;

// Same order as the indices in SyncPathTracedProfile and LogRRProfile.
static constexpr const char* kPathTracedProfileKeyNames[] = {
    "FloorIsolation", "FloorRawBlend",  "CorrelationBias", "FloorHandover", "StabilityBias",
    "CrossBlNormStr", "GaussKernRelax", "RadianceClip",    "MaxRadiance",   "DisocThreshold",
};
static constexpr size_t kPathTracedProfileKeyCount = std::size(kPathTracedProfileKeyNames);

template <typename T>
static void SyncPathTracedDefault(bool apply, CustomOptional<T>& key, T value, uint32_t index)
{
    const uint32_t bit = 1u << index;

    if (apply)
    {
        if (key.has_value())
            return;

        key.set_volatile_value(value);
        s_pathTracedProfileOwnedKeys |= bit;
        return;
    }

    if ((s_pathTracedProfileOwnedKeys & bit) == 0)
        return;

    s_pathTracedProfileOwnedKeys &= ~bit;

    // A menu edit since the profile wrote the key made it explicit; that value stays.
    if (!key.value_for_config_ignore_default().has_value() && key.has_value() && key.value() == value)
        key = std::optional<T>();
}

// Applies or releases the profile's defaults when the flag changed; a no-op otherwise.
// Returns whether the profile is in effect.
// RR-04: each half is synced on its own flag. With FfxDenoiserPathTracedTemporal=true, temporalWanted equals
// wanted and the two applied flags move together, so this makes the same ten calls, in the same order, as 0.3.3.2.
static bool SyncPathTracedProfile(Config& cfg)
{
    const bool wanted = cfg.FfxDenoiserPathTracedProfile.value_or_default();
    const bool temporalWanted = wanted && cfg.FfxDenoiserPathTracedTemporal.value_or_default();
    if (wanted == s_pathTracedProfileApplied && temporalWanted == s_pathTracedTemporalApplied)
        return wanted;

    if (wanted != s_pathTracedProfileApplied)
    {
        SyncPathTracedDefault(wanted, cfg.FfxDenoiserFloorIsolation, 0.0f, 0);
        SyncPathTracedDefault(wanted, cfg.FfxDenoiserFloorRawBlend, 0.0f, 1);
        SyncPathTracedDefault(wanted, cfg.FfxDenoiserCorrelationBias, 0.0f, 2);
        SyncPathTracedDefault(wanted, cfg.FfxDenoiserFloorHandover, 0, 3);
    }

    if (temporalWanted != s_pathTracedTemporalApplied)
    {
        SyncPathTracedDefault(temporalWanted, cfg.FfxDenoiserStabilityBias, 1.0f, 4);
        SyncPathTracedDefault(temporalWanted, cfg.FfxDenoiserCrossBlNormStr, 1.0f, 5);
        SyncPathTracedDefault(temporalWanted, cfg.FfxDenoiserGaussKernRelax, 0.0f, 6);
        SyncPathTracedDefault(temporalWanted, cfg.FfxDenoiserRadianceClip, 50.0f, 7);
        SyncPathTracedDefault(temporalWanted, cfg.FfxDenoiserMaxRadiance, 65504.0f, 8);
        SyncPathTracedDefault(temporalWanted, cfg.FfxDenoiserDisocThreshold, 0.05f, 9);
    }

    s_pathTracedProfileApplied = wanted;
    s_pathTracedTemporalApplied = temporalWanted;
    return wanted;
}

// True when the profile did not write the key, or something made it explicit since.
template <typename T>
static bool KeepsOwnValue(CustomOptional<T>& key, uint32_t index)
{
    return (s_pathTracedProfileOwnedKeys & (1u << index)) == 0 ||
           key.value_for_config_ignore_default().has_value();
}

// RR-12 (AMDNR 0.3.4): where a temporal configure value comes from, for the log. AMD: [FSR-RR]
// FfxDenoiserUseAmdDefaults pushes the queried values instead; own: the ini or the menu; profile: the
// path-traced profile wrote it; quirk: a title default set by a quirk; fork: the key's default.
template <typename T>
static const char* TemporalValueSource(Config& cfg, CustomOptional<T>& key, uint32_t index)
{
    if (cfg.FfxDenoiserUseAmdDefaults.value_or_default())
        return "AMD";
    if (key.value_for_config_ignore_default().has_value())
        return "own";
    if ((s_pathTracedProfileOwnedKeys & (1u << index)) != 0)
        return "profile";
    return key.has_value() ? "quirk" : "fork";
}

// The route the conversion applies: the key while the profile is on, 0 otherwise.
static float ProfileTextureRoute(Config& cfg)
{
    return cfg.FfxDenoiserPathTracedProfile.value_or_default()
               ? std::clamp(cfg.FfxDenoiserProfileTextureRoute.value_or_default(), 0.0f, 1.0f)
               : 0.0f;
}

// What [RR_POST] and [RR_PROFILE] say after the bias mask strength (RR-08, AMDNR 0.3.4).
static const char* DescribeBiasMask(int biasMask)
{
    return biasMask > 0 ? "mask published" : (biasMask == 0 ? "no mask published: inert" : "not looked at yet");
}

// The values the profile governs, and where the profile flag came from. Bias-mask routing is
// outside the profile and only reported. Returns the texture route it logged.
// RR-08 (AMDNR 0.3.4): the line also names the source of each half (routing: floor/raw-blend/correlation/
// handover/route; temporal: the six configure values), whether the title's bias mask exists (`biasMask`: -1 not
// looked at yet, 0 not published, 1 published), and the feature's signal choice (`signals`).
static float LogRRProfile(Config& cfg, const char* reason, const std::string& signals, int biasMask)
{
    auto& profileFlag = cfg.FfxDenoiserPathTracedProfile;
    const bool pathTraced = profileFlag.value_or_default();

    // No quirk sets the flag since 0.3.3.1, so it is either the default or the player's (ini or menu).
    const char* source = profileFlag.has_value() ? "ini" : "default";

    // Profile keys that kept a value of their own instead of the profile's. The temporal keys (4-9) count only
    // while the temporal half is applied (RR-04): with it off they are not the profile's to keep.
    const size_t profileKeyCount = s_pathTracedTemporalApplied ? kPathTracedProfileKeyCount : 4;
    std::string ownValues = "-";
    if (pathTraced)
    {
        const bool keeps[kPathTracedProfileKeyCount] = {
            KeepsOwnValue(cfg.FfxDenoiserFloorIsolation, 0),
            KeepsOwnValue(cfg.FfxDenoiserFloorRawBlend, 1),
            KeepsOwnValue(cfg.FfxDenoiserCorrelationBias, 2),
            KeepsOwnValue(cfg.FfxDenoiserFloorHandover, 3),
            KeepsOwnValue(cfg.FfxDenoiserStabilityBias, 4),
            KeepsOwnValue(cfg.FfxDenoiserCrossBlNormStr, 5),
            KeepsOwnValue(cfg.FfxDenoiserGaussKernRelax, 6),
            KeepsOwnValue(cfg.FfxDenoiserRadianceClip, 7),
            KeepsOwnValue(cfg.FfxDenoiserMaxRadiance, 8),
            KeepsOwnValue(cfg.FfxDenoiserDisocThreshold, 9),
        };

        ownValues.clear();
        for (size_t i = 0; i < profileKeyCount; ++i)
        {
            if (!keeps[i])
                continue;
            if (!ownValues.empty())
                ownValues += ',';
            ownValues += kPathTracedProfileKeyNames[i];
        }
        if (ownValues.empty())
            ownValues = "none";
    }

    const float textureRoute = ProfileTextureRoute(cfg);

    // Where each half comes from. The configure pass pushes AMD's queried values instead of the six temporal
    // keys while [FSR-RR] FfxDenoiserUseAmdDefaults is on; ownValues lists the keys that kept a value of their
    // own under the profile.
    const bool amdDefaults = cfg.FfxDenoiserUseAmdDefaults.value_or_default();
    const char* routingSource = pathTraced ? "profile" : "fork";
    const char* temporalSource = amdDefaults                   ? "AMD queried defaults"
                                 : s_pathTracedTemporalApplied ? "profile"
                                 : pathTraced ? "not the profile's (FfxDenoiserPathTracedTemporal=false)"
                                              : "fork";

    LOG_INFO("[RR_PROFILE] exe={} pathTraced={} source={} floorIsolation={:.2f} floorRawBlend={:.2f} "
             "correlationBias={:.2f} handover={} textureRoute={:.2f} biasMask={:.2f} ({}) disoc={:.3f} stab={:.2f} "
             "normal={:.2f} gauss={:.2f} clipK={:.1f} maxRad={:.1f} amdDefaults={} ownValues={} routingHalf={} "
             "temporalHalf={} {} ({}){}",
             State::Instance().gameExe, pathTraced ? "on" : "off", source,
             cfg.FfxDenoiserFloorIsolation.value_or_default(),
             cfg.FfxDenoiserFloorRawBlend.value_or_default(),
             cfg.FfxDenoiserCorrelationBias.value_or_default(),
             cfg.FfxDenoiserFloorHandover.value_or_default(), textureRoute,
             cfg.FfxDenoiserBiasMaskStrength.value_or_default(), DescribeBiasMask(biasMask),
             cfg.FfxDenoiserDisocThreshold.value_or_default(),
             cfg.FfxDenoiserStabilityBias.value_or_default(),
             cfg.FfxDenoiserCrossBlNormStr.value_or_default(),
             cfg.FfxDenoiserGaussKernRelax.value_or_default(),
             cfg.FfxDenoiserRadianceClip.value_or_default(),
             cfg.FfxDenoiserMaxRadiance.value_or_default(), amdDefaults ? "on" : "off", ownValues, routingSource,
             temporalSource, signals, reason,
             pathTraced ? " - profile retired (not recommended), on from [FSR-RR] FfxDenoiserPathTracedProfile" : "");

    // RR-25 (AMDNR 0.3.4): the first time the profile is seen on in this process, say once why it is retired and how
    // to turn it off (a 0.3.3.2 ini saved with the box ticked keeps it on). The text does not depend on how the menu
    // draws the row (the 0.3.4 menu rework's M1 shows it only while it is on): the checkbox is there whenever it is on.
    static bool warnedRetired = false;
    if (pathTraced && !warnedRetired)
    {
        warnedRetired = true;
        LOG_WARN("[RR_PROFILE] The path-traced profile is on ([FSR-RR] FfxDenoiserPathTracedProfile=true). It is "
                 "retired and not recommended: every RE Requiem player report found it worse than the normal route "
                 "(etched textures, lamps and emissive light too bright, thinner hair), also with Texture route 1. "
                 "Untick 'Path-traced profile' in Neural > Ray Regeneration and press Save Settings, or delete the "
                 "line from OptiScaler.ini, to get the normal route back; the key still works for testing.");
    }

    return textureRoute;
}

// "present WxH FORMAT" for a texture published under an NGX key, "absent" otherwise. The
// pointer is queried rather than cast: the NGX table folds every pointer kind into one slot.
static std::string DescribeNGXResource(const NVSDK_NGX_Parameter& inParams, const char* key)
{
    ID3D12Resource* resource = nullptr;
    if (!TryGetNGXVoidPointer(inParams, key, resource))
        return "absent";

    Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
    if (FAILED(resource->QueryInterface(IID_PPV_ARGS(d3d12Resource.GetAddressOf()))) || !d3d12Resource)
        return "present (not a D3D12 resource)";

    const D3D12_RESOURCE_DESC desc = d3d12Resource->GetDesc();
    const auto formatName = magic_enum::enum_name(desc.Format);
    return std::format("present {}x{} {}", desc.Width, desc.Height,
                       formatName.empty() ? "UNKNOWN" : formatName);
}

static void LogRRD3D12Messages(ID3D12InfoQueue* infoQueue, uint64_t firstMessage,
                               std::string_view scope)
{
    if (!infoQueue)
        return;

    const uint64_t messageCount = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    firstMessage = std::min(firstMessage, messageCount);

    constexpr uint64_t kMaxLoggedMessages = 32;
    uint64_t loggedMessages = 0;
    uint64_t matchingMessages = 0;

    for (uint64_t messageIndex = firstMessage; messageIndex < messageCount; ++messageIndex)
    {
        SIZE_T messageSize = 0;
        if (FAILED(infoQueue->GetMessage(messageIndex, nullptr, &messageSize)) || messageSize == 0)
            continue;

        std::vector<uint8_t> storage(messageSize);
        D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
        if (FAILED(infoQueue->GetMessage(messageIndex, message, &messageSize)))
            continue;

        if (message->Severity > D3D12_MESSAGE_SEVERITY_WARNING)
            continue;

        ++matchingMessages;
        if (loggedMessages >= kMaxLoggedMessages)
            continue;

        ++loggedMessages;
        LOG_WARN("[RR_DIAG][D3D12][{}] severity={}, category={}, id={} ({}) - {}",
                 scope, magic_enum::enum_name(message->Severity),
                 magic_enum::enum_name(message->Category), static_cast<uint32_t>(message->ID),
                 magic_enum::enum_name(message->ID), message->pDescription);
    }

    if (matchingMessages == 0)
    {
        LOG_INFO("[RR_DIAG][D3D12][{}] no corruption/error/warning messages were captured", scope);
    }
    else if (matchingMessages > loggedMessages)
    {
        LOG_WARN("[RR_DIAG][D3D12][{}] {} additional messages were omitted",
                 scope, matchingMessages - loggedMessages);
    }
}

struct ViewPlanes
{
    float nearPlane;
    float farPlane;
    bool isInfinite;
    bool isRightHanded;
};

static ViewPlanes GetViewPlanes(const DirectX::XMMATRIX& projection, bool isInverted)
{
    ViewPlanes planes = {};

    // Internal projection convention: row-major storage with column vectors.
    // clip.z = A * view.z + B; clip.w = W * view.z.
    const float A = projection.r[2].m128_f32[2];
    const float B = projection.r[2].m128_f32[3];
    const float W = projection.r[3].m128_f32[2];

    // A is the projection's depth term and W its perspective divide. On an infinite
    // far plane A is mathematically zero, but it survives matrix inversion as float
    // noise - values around 1e-6 are routine - so an absolute epsilon misclassifies
    // the projection as finite and the far plane below is then computed by dividing
    // by that noise. Cyberpunk lands on A = -1.19e-6 and produces a fabricated
    // 16777 unit far plane that way. Scale the test to W so it measures a ratio.
    const float infiniteCheckVal = isInverted ? A : (A - W);
    const float infiniteCheckScale = std::max(std::abs(W), 1e-12f);
    planes.isInfinite = std::abs(infiniteCheckVal) < 1e-4f * infiniteCheckScale;
    // W is +1 for LH projections and -1 for RH projections, independent of Z direction.
    planes.isRightHanded = W < 0.0f;

    // An infinite projection has no true far plane, but this value is consumed as a
    // log-normalisation denominator, as a depth clamp, and as RR's linearDepthBounds.
    // FLT_MAX degrades all three - log(3.4e38) is 88.7, which flattens every
    // realistic depth to the bottom tenth of the range - so report a large finite
    // horizon instead. FP16 max keeps it representable everywhere downstream.
    constexpr float kInfiniteHorizon = 65504.0f;

    if (isInverted)
    {
        // Inverted: Near is at D=1, Far is at D=0
        // 1 = A/W + B/(n*W) -> n = B / (W - A)
        planes.nearPlane = std::abs(B / (W - A));

        // 0 = A/W + B/(f*W) -> f = -B / A
        planes.farPlane = planes.isInfinite ? kInfiniteHorizon : std::abs(-B / A);
    }
    else
    {
        // Standard: Near is at D=0, Far is at D=1
        // 0 = A/W + B/(n*W) -> n = -B / A
        planes.nearPlane = std::abs(-B / A);

        // 1 = A/W + B/(f*W) -> f = B / (W - A)
        planes.farPlane = planes.isInfinite
            ? kInfiniteHorizon
            : std::abs(B / (W - A));
    }

    // A finite branch that survived the ratio test can still be wildly off if the
    // projection terms are noisy, so keep the result inside a range that cannot
    // break the consumers above.
    if (!std::isfinite(planes.nearPlane) || planes.nearPlane <= 0.0f)
    {
        // A degenerate or noisy projection divides by a term that is effectively
        // zero above. The near plane is consumed directly as RR's
        // linearDepthBounds.min and as the conversion shaders' depth clamp, so a
        // NaN here spreads into every reconstructed view-space position. Substitute
        // a usable default; the caller logs the resulting planes when they change.
        constexpr float kDefaultNearPlane = 0.01f;
        planes.nearPlane = kDefaultNearPlane;
        planes.farPlane = kInfiniteHorizon;
    }
    else if (!std::isfinite(planes.farPlane))
    {
        // std::clamp propagates NaN rather than replacing it, so filter it first.
        planes.farPlane = kInfiniteHorizon;
    }
    else
    {
        planes.farPlane = std::clamp(
            planes.farPlane, planes.nearPlane * 2.0f, kInfiniteHorizon);
    }

    return planes;
}

using FSRDConvFlags = FSRDPreprocessor_Dx12::ConvFlags;
using FSRDCompFlags = FSRDPreprocessor_Dx12::CompFlags;

enum class DebugModes : uint64_t
{
    None = 0,
    DenoiserBypass = 1,
    UpscalerBypass = 2,
    RawColor = 3,
    DlssBias = 4,
    DlssColorBeforeParticles = 5,
    DlssColorBeforeTransparency = 6,
    DlssTransparencyLayer = 7,
    FfxDebug = 8,
    AmbientOcclusionInput = 9,
    AmbientOcclusionOutput = 10,

    ConversionDebug = FSRDConvFlags::Debug,
    ConversionDebugMask = FSRDConvFlags::DebugModeMask,

    OutRadiance = FSRDConvFlags::DebugOutRadiance,

    InSpecHitDist = FSRDConvFlags::DebugInSpecHitDist,
    InMotion = FSRDConvFlags::DebugInMotion,
    InNormals = FSRDConvFlags::DebugInNormals,
    InRoughness = FSRDConvFlags::DebugInRoughness,
    InDiffAlbedo = FSRDConvFlags::DebugInDiffAlbedo,
    InSpecAlbedo = FSRDConvFlags::DebugInSpecAlbedo,

    OutSignalSplit = FSRDConvFlags::DebugOutSignalSplit,
    OutLinearDepth = FSRDConvFlags::DebugOutLinearDepth,
    OutMotion = FSRDConvFlags::DebugOutMotion,
    OutNormals = FSRDConvFlags::DebugOutNormals,
    OutSpecAlbedo = FSRDConvFlags::DebugOutSpecAlbedo,
    OutDiffAlbedo = FSRDConvFlags::DebugOutDiffAlbedo,

    OutDepthDelta = FSRDConvFlags::DebugOutDepthDelta,
    NormDepth = FSRDConvFlags::DebugNormDepth,
    AlbedoError = FSRDConvFlags::DebugAlbedoError,

    FloorColor = FSRDConvFlags::DebugFloorColor,
    RawIndirectSpecular = FSRDConvFlags::DebugRawIndirectSpecular,
    EffectiveRoughness = FSRDConvFlags::DebugEffectiveRoughness,
    RawRoughness = FSRDConvFlags::DebugRawRoughness,
    EmissiveMask = FSRDConvFlags::DebugEmissiveMask,
    AppliedRoughnessFloor = FSRDConvFlags::DebugAppliedRoughnessFloor,
    ResourceInspector = FSRDConvFlags::DebugResourceInspector,
    MaterialType = FSRDConvFlags::DebugMaterialType,
    InEmissive = FSRDConvFlags::DebugInEmissive,
    RRMaterialType = FSRDConvFlags::DebugRRMaterialType,
    AlbedoStructure = FSRDConvFlags::DebugAlbedoStructure,
    FloorHandoverOutput = FSRDConvFlags::DebugFloorHandover,

    SpecularSplit = FSRDConvFlags::DebugSpecularSplit,
    InTitleLinearDepth = FSRDConvFlags::DebugInTitleLinearDepth,
    TitleLinearDepthDiff = FSRDConvFlags::DebugTitleLinearDepthDiff,
    InResponsivityMask = FSRDConvFlags::DebugInResponsivityMask,

    InBiasMask = FSRDConvFlags::DebugInBiasMask,
    FloorStructure = FSRDConvFlags::DebugFloorStructure,
    SkipUnmapped = FSRDConvFlags::DebugSkipUnmapped,
    SkipFloor = FSRDConvFlags::DebugSkipFloor,
    SkipRawInject = FSRDConvFlags::DebugSkipRawInject,
    ProfileRoute = FSRDConvFlags::DebugProfileRoute,
    DemodGain = FSRDConvFlags::DebugDemodGain,
    HitDistGate = FSRDConvFlags::DebugHitDistGate,
    DenoiserFraction = FSRDConvFlags::DebugDenoiserFraction,

    CompositionDebugOffset = 16u,
    CompositionDebug = (uint64_t) FSRDCompFlags::Debug << CompositionDebugOffset,
    CompositionDebugMask = (uint64_t)FSRDCompFlags::DebugModeMask,

    Correlation = (uint64_t)FSRDCompFlags::DebugCorrelation << CompositionDebugOffset,
    SkipSignal = (uint64_t) FSRDCompFlags::DebugSkipSignal << CompositionDebugOffset,
    DenoiserOutput = (uint64_t) FSRDCompFlags::DebugDenoiserOutput << CompositionDebugOffset,
    DirectSpecular = (uint64_t) FSRDCompFlags::DebugDirectSpecular << CompositionDebugOffset,
    IndirectSpecular = (uint64_t) FSRDCompFlags::DebugIndirectSpecular << CompositionDebugOffset,
    DirectDiffuse = (uint64_t) FSRDCompFlags::DebugDirectDiffuse << CompositionDebugOffset,
    IndirectDiffuse = (uint64_t) FSRDCompFlags::DebugIndirectDiffuse << CompositionDebugOffset,
    HandoverRRBand = (uint64_t) FSRDCompFlags::DebugHandoverRRBand << CompositionDebugOffset,
    HandoverDetailBand = (uint64_t) FSRDCompFlags::DebugHandoverDetailBand << CompositionDebugOffset,
    HandoverBandMix = (uint64_t) FSRDCompFlags::DebugHandoverBandMix << CompositionDebugOffset,
    HandoverAnchor = (uint64_t) FSRDCompFlags::DebugHandoverAnchor << CompositionDebugOffset,
    HandoverWeight = (uint64_t) FSRDCompFlags::DebugHandoverWeight << CompositionDebugOffset,
};

static FSRDConvFlags GetConvDebugFlags(DebugModes mode) 
{ 
    uint32_t flags = uint32_t(mode);
    flags &= uint32_t(DebugModes::ConversionDebugMask);
    return FSRDConvFlags(flags);
}

static FSRDCompFlags GetCompDebugFlags(DebugModes mode) 
{ 
    uint64_t flags = uint64_t(mode);
    flags >>= uint64_t(DebugModes::CompositionDebugOffset);
    flags &= uint64_t(DebugModes::CompositionDebugMask);
    return FSRDCompFlags(flags);
}

using ModeNamePair = std::pair<const char*, uint64_t>;
constexpr auto kDebugModes = std::to_array<ModeNamePair>(
{
    { "None", (uint64_t) DebugModes::None },
    { "DebugOverview", (uint64_t) DebugModes::FfxDebug },

    { "DenoiserBypass", (uint64_t) DebugModes::DenoiserBypass },
    { "UpscalerBypass", (uint64_t) DebugModes::UpscalerBypass },
    { "DenoiserOutput", (uint64_t) DebugModes::DenoiserOutput },
    { "SkipSignal", (uint64_t) DebugModes::SkipSignal },

    { "RawColor", (uint64_t) DebugModes::RawColor },
    { "DlssBias", (uint64_t) DebugModes::DlssBias },
    { "DlssColorBeforeParticles", (uint64_t) DebugModes::DlssColorBeforeParticles },
    { "DlssColorBeforeTransparency", (uint64_t) DebugModes::DlssColorBeforeTransparency },
    { "DlssTransparencyLayer", (uint64_t) DebugModes::DlssTransparencyLayer },
    { "AmbientOcclusionInput", (uint64_t) DebugModes::AmbientOcclusionInput },
    { "AmbientOcclusionOutput", (uint64_t) DebugModes::AmbientOcclusionOutput },

    { "InputMotionVectors", (uint64_t) DebugModes::InMotion },
    { "InNormals", (uint64_t) DebugModes::InNormals },
    { "InputRoughness", (uint64_t) DebugModes::InRoughness },
    { "RawRoughness", (uint64_t) DebugModes::RawRoughness },
    { "EmissiveMask", (uint64_t) DebugModes::EmissiveMask },
    { "AppliedRoughnessFloor", (uint64_t) DebugModes::AppliedRoughnessFloor },
    { "ZeroRoughnessMaterialType", (uint64_t) DebugModes::MaterialType },
    { "ResourceInspector", (uint64_t) DebugModes::ResourceInspector },
    { "SpecularHitDistance", (uint64_t) DebugModes::InSpecHitDist },
    { "InDiffAlbedo", (uint64_t) DebugModes::InDiffAlbedo },
    { "InSpecAlbedo", (uint64_t) DebugModes::InSpecAlbedo },
    { "InputEmissive", (uint64_t) DebugModes::InEmissive },
    { "RRMaterialType", (uint64_t) DebugModes::RRMaterialType },
    { "AlbedoStructureAvailability", (uint64_t) DebugModes::AlbedoStructure },
    { "FloorHandoverOutput", (uint64_t) DebugModes::FloorHandoverOutput },

    { "SpecularSplit", (uint64_t) DebugModes::SpecularSplit },
    { "InTitleLinearDepth", (uint64_t) DebugModes::InTitleLinearDepth },
    { "TitleLinearDepthDiff", (uint64_t) DebugModes::TitleLinearDepthDiff },
    { "InResponsivityMask", (uint64_t) DebugModes::InResponsivityMask },

    { "InBiasMask", (uint64_t) DebugModes::InBiasMask },
    { "FloorStructure", (uint64_t) DebugModes::FloorStructure },
    { "SkipUnmapped", (uint64_t) DebugModes::SkipUnmapped },
    { "SkipFloor", (uint64_t) DebugModes::SkipFloor },
    { "SkipRawInject", (uint64_t) DebugModes::SkipRawInject },
    { "ProfileTextureRoute", (uint64_t) DebugModes::ProfileRoute },
    { "DemodGain", (uint64_t) DebugModes::DemodGain },
    { "HitDistGate", (uint64_t) DebugModes::HitDistGate },
    { "DenoiserFraction", (uint64_t) DebugModes::DenoiserFraction },

    { "OutRadiance", (uint64_t) DebugModes::OutRadiance },
    { "OutSignalSplit", (uint64_t) DebugModes::OutSignalSplit },
    { "OutLinearDepth", (uint64_t) DebugModes::OutLinearDepth },
    { "RRMotionVectors", (uint64_t) DebugModes::OutMotion },
    { "OutNormals", (uint64_t) DebugModes::OutNormals },
    { "OutSpecAlbedo", (uint64_t) DebugModes::OutSpecAlbedo },
    { "OutDiffAlbedo", (uint64_t) DebugModes::OutDiffAlbedo },
    { "OutDepthDelta", (uint64_t) DebugModes::OutDepthDelta },
    { "NormDepth", (uint64_t) DebugModes::NormDepth },

    { "AlbedoError", (uint64_t) DebugModes::AlbedoError },
    { "Correlation", (uint64_t) DebugModes::Correlation },

    { "HandoverRRLowBand", (uint64_t) DebugModes::HandoverRRBand },
    { "HandoverDetailHighBand", (uint64_t) DebugModes::HandoverDetailBand },
    { "HandoverBandMix", (uint64_t) DebugModes::HandoverBandMix },
    { "HandoverRRAnchor", (uint64_t) DebugModes::HandoverAnchor },
    { "HandoverEffectiveWeight", (uint64_t) DebugModes::HandoverWeight },

    { "FloorColor", (uint64_t) DebugModes::FloorColor },
    
    { "RawSpecularSignal", (uint64_t) DebugModes::RawIndirectSpecular },
    { "DenoisedDirectSpecSignal", (uint64_t) DebugModes::DirectSpecular },
    { "DenoisedIndirectSpecSignal", (uint64_t) DebugModes::IndirectSpecular },
    { "EffectiveRoughness", (uint64_t) DebugModes::EffectiveRoughness },
    { "DenoisedDirectDiffuseSignal", (uint64_t) DebugModes::DirectDiffuse },
    { "DenoisedIndirectDiffuseSignal", (uint64_t) DebugModes::IndirectDiffuse },
});

// The same table for the overlay (FSRD::RuntimeStatus::DebugViews). It is built at compile time and
// never written, so the overlay thread can read it while the render thread creates a context.
static constexpr auto kDebugViews = []
{
    std::array<FSRD::RuntimeStatus::DebugView, kDebugModes.size()> views {};
    for (size_t i = 0; i < kDebugModes.size(); i++)
        views[i] = { kDebugModes[i].first, kDebugModes[i].second };
    return views;
}();
static constexpr FSRD::RuntimeStatus::DebugViewTable kDebugViewTable { kDebugViews.data(), kDebugViews.size() };

// RR-22 (AMDNR 0.3.4, plan decision D29). A Ray Regeneration fallback latches Ray Regeneration off for the session
// (FSRD::RuntimeStatus::RayRegenOffForSession, read by NVNGX_DLSS_Dx12.cpp) only on the stock Unreal DLSS plugin's
// signature: an Unreal title evaluating RR through NGX directly (not inside slEvaluateFeature) whose WorldToView and
// ViewToClip keys were present but null, with no camera from any source, on every frame of the fallback's window. No later frame or re-create can change that, so a re-create then
// goes straight to FSR instead of 30 more frames without a denoiser. Never with [FSR-RR]
// FfxDenoiserNgxDirectSLConstants=true (Phase 1 may find a camera), and never for any other missing input: the
// fallback fires on any of them, title and loading screens included, and a real RR title recovers today after a
// re-create. false = 0.3.3.2 (every Ray Reconstruction create retries Ray Regeneration).
constexpr bool kRrFallbackSessionLatch = true;

// RR-19 (AMDNR 0.3.4). The handle whose Ray Regeneration fallback wrote State::rrFallbackReason, or UINT32_MAX.
static std::atomic<uint32_t> s_rrFallbackReasonHandle { UINT32_MAX };

// A Ray Regeneration fallback marks its handle (State::rrFallbackToSr, which puts the neural pass in the pre-SR
// placement: NVNGX_DLSS_Dx12.cpp) and writes the reason the menu shows. Both used to stay for the session, so the
// menu kept "Ray Regeneration is off" after a re-pick that worked. They are cleared here, and only here: when an
// FSR_RR feature is created on the handle (a re-pick, or a Reset re-create of an RR handle) or a denoiser dispatch
// succeeds on it. The FFX feature the fallback itself asks for is not an FSR_RR feature, never comes here, and keeps
// the mark. The reason also goes with any successful dispatch: Ray Regeneration then runs in this title.
static void ClearRRFallback(uint32_t handleId, bool denoiserDispatched)
{
    auto& state = State::Instance();

    bool clearedMark = false;
    if (const auto it = state.rrFallbackToSr.find(handleId); it != state.rrFallbackToSr.end() && it->second)
    {
        it->second = false;
        clearedMark = true;
    }

    bool clearedReason = false;
    if (!state.rrFallbackReason.empty() &&
        (denoiserDispatched || s_rrFallbackReasonHandle.load(std::memory_order_relaxed) == handleId))
    {
        // clear(), not an assignment: the overlay may be reading the text, and clear() keeps the buffer.
        state.rrFallbackReason.clear();
        s_rrFallbackReasonHandle.store(UINT32_MAX, std::memory_order_relaxed);
        clearedReason = true;
    }

    if (clearedMark || clearedReason)
        LOG_INFO("FSR-RR: handle {} serves Ray Regeneration again ({}): fallback mark {}, menu reason {}", handleId,
                 denoiserDispatched ? "a denoiser dispatch succeeded" : "FSR_RR feature created",
                 clearedMark ? "cleared" : "was not set", clearedReason ? "cleared" : "kept");
}

// B9 (AMDNR 0.3.4). The Streamline hooks keep a reference to each RR-tagged game texture they saw (up to one per
// RRTaggedSignal). Every FSR_RR feature registers with them while it lives (StreamlineHooks::rrTagConsumerAdded /
// rrTagConsumerRemoved). When the last one goes (Ray Regeneration switched off, the handle released, or the
// fallback's switch to FSR), the hooks drop those references at the end of the next outermost slEvaluateFeature,
// and only if no FSR_RR feature is alive by then. Not here: a title's own re-create (NVSDK_NGX_D3D12_ReleaseFeature
// then CreateFeature inside slEvaluateFeature, on every resize or quality change) destroys this feature after the
// game tagged the frame's inputs and before the new feature reads them, and the delayed destroy of a menu re-pick
// runs on another thread at any point of a frame.

// RR-24 (AMDNR 0.3.4). An RR error that repeats on every frame (a title that never publishes an input, a configure
// value the runtime keeps refusing) used to write one line per frame. Each call site keeps its own count and logs
// occurrences 1-5, then every 1000th, with the count in the line.
struct RRRepeatedErrorGate
{
    std::atomic<uint64_t> count { 0 };

    // The occurrence number when this one is logged, else 0.
    uint64_t Next()
    {
        const uint64_t n = count.fetch_add(1, std::memory_order_relaxed) + 1;
        return n <= 5 || n % 1000 == 0 ? n : 0;
    }
};

static std::string RRRepeatNote(uint64_t n)
{
    return n == 5 ? "occurrence 5; from here every 1000th is logged" : std::format("occurrence {}", n);
}

// SAT-P0 (AMDNR 0.3.4, Phase 0 of the UE5 Ray Regeneration work): the diagnostic lines below are written once per
// NGX handle. A menu re-pick or a Reset re-create builds a new feature on the same handle, and must not repeat
// them; each caller also keeps an instance flag, so the lock is taken once per line and feature, not per frame.
enum class RRHandleLine : uint32_t
{
    CameraKeys,      // [RR_CAM] matrix keys (SAT-P0-2)
    InputSnapshot,   // [RR_INPUT] snapshot resources / declared (SAT-P0-1)
    StreamlineFirst, // [RR_CAM] streamline, first evaluate (SAT-P0-3)
    StreamlineLater, // [RR_CAM] streamline, at the fallback or after 300 evaluates (SAT-P0-3)
    ContentProbe,    // [RR_INPUT] depth / normals probe (SAT-P0-4)
    FallbackDepth,   // FSR-RR: fallback depth flags (SAT-P0-7)
    SLJitterMatch,   // [RR_CAM] source=SL-ngx-direct, first match (SAT-P1)
};

static bool FirstRRLineForHandle(uint32_t handleId, RRHandleLine line)
{
    static std::mutex mutex;
    static std::unordered_set<uint64_t> written;
    std::scoped_lock lock(mutex);
    return written.insert((uint64_t(handleId) << 8) | uint64_t(line)).second;
}

static const char* NGXEngineName(NVSDK_NGX_EngineType engine)
{
    switch (engine)
    {
    case NVSDK_NGX_ENGINE_TYPE_CUSTOM:
        return "custom";
    case NVSDK_NGX_ENGINE_TYPE_UNREAL:
        return "Unreal";
    case NVSDK_NGX_ENGINE_TYPE_UNITY:
        return "Unity";
    case NVSDK_NGX_ENGINE_TYPE_OMNIVERSE:
        return "Omniverse";
    default:
        return "unknown";
    }
}

// SAT-P0-4 (AMDNR 0.3.4): a one-shot look at the title's depth and normals, taken only on a frame whose camera could
// not be resolved (the conversion never runs there, so nothing else ever reads them). It tells linear-centimetre,
// standard-Z and reversed-Z depth apart, and signed from UNORM-encoded normals - what a synthesized camera would need.
//
// A small compute pass reads a 64x64 grid over the render extent through SRVs, in the state the title hands its RR
// inputs in - the same assumption the conversion makes on every RR frame, so no barrier ever touches a title
// resource - into a buffer of our own, which is copied to a readback buffer. Map synchronises with nothing, and the
// title's queue is not ours to fence, so, as in the conversion's input probe, the GPU writes a token after the data
// and the CPU reads a capture only after it saw that token on two polls in a row. A ring of kSlots captures on
// consecutive blocked frames: whichever lands first is logged; nothing ever waits. The shader is compiled on first
// use with the system d3dcompiler; a title whose camera resolves never creates any of this.
struct RRContentProbe
{
    static constexpr UINT kGrid = 64;
    static constexpr UINT kSamples = kGrid * kGrid;
    static constexpr UINT kSlots = 3;
    static constexpr UINT64 kDataBytes = UINT64(kSamples) * 2u * sizeof(float) * 4u; // depth block, normals block
    static constexpr UINT kMaxPolls = 600; // evaluates to keep looking for a landed capture before giving up
    static constexpr UINT kMaxUnreadable = 10; // camera-blocked frames without depth/normals before giving up

    static constexpr const char* kShader = R"(
Texture2D<float> g_depth : register(t0);
Texture2D<float4> g_normals : register(t1);
RWStructuredBuffer<float4> g_out : register(u0);
cbuffer ProbeConstants : register(b0)
{
    uint2 g_depthBase;
    uint2 g_normalsBase;
    uint2 g_extent;
    uint g_grid;
    uint g_pad;
};
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_grid || id.y >= g_grid)
        return;
    const uint2 p = min(uint2((float2(id.xy) + 0.5f) * float2(g_extent) / float(g_grid)), g_extent - 1u);
    const uint index = id.y * g_grid + id.x;
    g_out[index] = float4(g_depth.Load(int3(g_depthBase + p, 0)), 0.0f, 0.0f, 0.0f);
    g_out[g_grid * g_grid + index] = g_normals.Load(int3(g_normalsBase + p, 0));
}
)";

    struct Slot
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> output;   // default heap, UAV, written by the pass
        Microsoft::WRL::ComPtr<ID3D12Resource> readback; // kDataBytes of data, then the 8-byte token
        uint64_t token = 0;                              // the value the readback waits for; 0 = not recorded
        bool tokenSeen = false;                          // seen once; read on the next poll
        DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
        DXGI_FORMAT normalsFormat = DXGI_FORMAT_UNKNOWN;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t frame = 0;
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    Microsoft::WRL::ComPtr<ID3D12Resource> tokenUpload; // kSlots tokens, persistently mapped
    uint64_t* tokenUploadPtr = nullptr;
    UINT descriptorSize = 0;
    std::array<Slot, kSlots> slots {};
    UINT recorded = 0;
    UINT polls = 0;
    UINT unreadableFrames = 0; // camera-blocked frames with no depth, no normals or no render size, before a record
    bool failed = false; // creation failed, an input cannot be read, or nothing landed in kMaxPolls
    bool done = false;   // logged (or given up)

    ~RRContentProbe()
    {
        if (tokenUpload && tokenUploadPtr)
            tokenUpload->Unmap(0, nullptr);
    }

    bool Create(ID3D12Device* device)
    {
        ScopedSkipHeapCapture skipHeapCapture {};

        Microsoft::WRL::ComPtr<ID3DBlob> code;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        const HRESULT compiled = DlssNr::SysCompiler::Compile(
            kShader, strlen(kShader), "RRContentProbe", nullptr, nullptr, "main", "cs_5_0",
            D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors);
        if (FAILED(compiled) || !code)
        {
            LOG_ERROR("[RR_INPUT] content probe: shader compile failed ({:#x}) {}", static_cast<uint32_t>(compiled),
                      errors ? std::string(static_cast<const char*>(errors->GetBufferPointer()),
                                           errors->GetBufferSize())
                             : std::string());
            return false;
        }

        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 2;
        srvRange.BaseShaderRegister = 0;
        srvRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER params[3] = {};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[0].DescriptorTable.NumDescriptorRanges = 1;
        params[0].DescriptorTable.pDescriptorRanges = &srvRange;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        params[1].Descriptor.ShaderRegister = 0;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[2].Constants.ShaderRegister = 0;
        params[2].Constants.Num32BitValues = 8;
        params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
        rootDesc.NumParameters = 3;
        rootDesc.pParameters = params;

        Microsoft::WRL::ComPtr<ID3DBlob> rootBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> rootError;
        if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &rootError)) ||
            FAILED(device->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                               IID_PPV_ARGS(&rootSignature))))
        {
            LOG_ERROR("[RR_INPUT] content probe: root signature creation failed");
            return false;
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.CS = { code->GetBufferPointer(), code->GetBufferSize() };
        if (FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipeline))))
        {
            LOG_ERROR("[RR_INPUT] content probe: pipeline creation failed");
            return false;
        }

        // Two SRVs per slot, so a capture still in flight never has its descriptors rewritten.
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 2 * kSlots;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap))))
        {
            LOG_ERROR("[RR_INPUT] content probe: descriptor heap creation failed");
            return false;
        }
        descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        const auto bufferDesc = [](UINT64 bytes, D3D12_RESOURCE_FLAGS flags)
        {
            D3D12_RESOURCE_DESC desc = {};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = bytes;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.SampleDesc = { 1, 0 };
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = flags;
            return desc;
        };

        const D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };
        const D3D12_HEAP_PROPERTIES readbackHeap = { D3D12_HEAP_TYPE_READBACK };
        const D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
        const D3D12_RESOURCE_DESC outputDesc = bufferDesc(kDataBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        const D3D12_RESOURCE_DESC readbackDesc = bufferDesc(kDataBytes + sizeof(uint64_t), D3D12_RESOURCE_FLAG_NONE);
        const D3D12_RESOURCE_DESC tokenDesc = bufferDesc(sizeof(uint64_t) * kSlots, D3D12_RESOURCE_FLAG_NONE);

        // Committed resources start zeroed, so a slot's token reads 0 until its capture lands.
        for (auto& slot : slots)
        {
            if (FAILED(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &outputDesc,
                                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                       IID_PPV_ARGS(&slot.output))) ||
                FAILED(device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
                                                       D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                       IID_PPV_ARGS(&slot.readback))))
            {
                LOG_ERROR("[RR_INPUT] content probe: buffer creation failed");
                return false;
            }
        }

        if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &tokenDesc,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                   IID_PPV_ARGS(&tokenUpload))) ||
            FAILED(tokenUpload->Map(0, nullptr, reinterpret_cast<void**>(&tokenUploadPtr))) || !tokenUploadPtr)
        {
            tokenUploadPtr = nullptr;
            LOG_ERROR("[RR_INPUT] content probe: token buffer creation failed");
            return false;
        }

        return true;
    }

    // The SRV format of a title texture, or UNKNOWN when this pass cannot read it.
    static DXGI_FORMAT ReadableFormat(ID3D12Device* device, const D3D12_RESOURCE_DESC& desc)
    {
        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.SampleDesc.Count != 1 ||
            (desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0)
            return DXGI_FORMAT_UNKNOWN;

        // A fully typed depth format admits no SRV at all (only its TYPELESS family does).
        switch (desc.Format)
        {
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_UNKNOWN;
        default:
            break;
        }

        D3D12_FEATURE_DATA_FORMAT_SUPPORT support = { FSRD::GetViewFormat(desc.Format) };
        if (IsIntegerFormat(support.Format) ||
            FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support))) ||
            (support.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) == 0)
            return DXGI_FORMAT_UNKNOWN;

        return support.Format;
    }

    // The shader declares float textures; a UINT/SINT view behind them reads undefined values, so such a title
    // texture (packed normals, say) is skipped rather than logged as if it were valid.
    static bool IsIntegerFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
            return true;
        default:
            return false;
        }
    }

    // Records one capture into the next slot. False when the inputs cannot be read (the probe then stops).
    bool Record(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12Resource* depth,
                XMUINT2 depthBase, ID3D12Resource* normals, XMUINT2 normalsBase, uint32_t width, uint32_t height,
                uint32_t frame)
    {
        if (recorded >= kSlots || width == 0 || height == 0)
            return true;

        const D3D12_RESOURCE_DESC depthDesc = depth->GetDesc();
        const D3D12_RESOURCE_DESC normalsDesc = normals->GetDesc();
        const DXGI_FORMAT depthView = ReadableFormat(device, depthDesc);
        const DXGI_FORMAT normalsView = ReadableFormat(device, normalsDesc);
        const bool fits = uint64_t(depthBase.x) + width <= depthDesc.Width &&
                          uint64_t(depthBase.y) + height <= depthDesc.Height &&
                          uint64_t(normalsBase.x) + width <= normalsDesc.Width &&
                          uint64_t(normalsBase.y) + height <= normalsDesc.Height;
        if (depthView == DXGI_FORMAT_UNKNOWN || normalsView == DXGI_FORMAT_UNKNOWN || !fits)
        {
            const auto depthName = magic_enum::enum_name(depthDesc.Format);
            const auto normalsName = magic_enum::enum_name(normalsDesc.Format);
            LOG_WARN("[RR_INPUT] content probe skipped: depth {} or normals {} cannot be read here (integer format, "
                     "shader load support, dimension, samples) or does not cover {}x{} at its base",
                     depthName.empty() ? "UNKNOWN" : depthName, normalsName.empty() ? "UNKNOWN" : normalsName, width,
                     height);
            return false;
        }

        ScopedSkipHeapCapture skipHeapCapture {};
        Slot& slot = slots[recorded];

        D3D12_CPU_DESCRIPTOR_HANDLE cpu = heap->GetCPUDescriptorHandleForHeapStart();
        cpu.ptr += SIZE_T(2 * recorded) * descriptorSize;
        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
        srv.Format = depthView;
        device->CreateShaderResourceView(depth, &srv, cpu);
        cpu.ptr += descriptorSize;
        srv.Format = normalsView;
        device->CreateShaderResourceView(normals, &srv, cpu);

        D3D12_GPU_DESCRIPTOR_HANDLE gpu = heap->GetGPUDescriptorHandleForHeapStart();
        gpu.ptr += UINT64(2 * recorded) * descriptorSize;

        const uint32_t constants[8] = { depthBase.x, depthBase.y, normalsBase.x, normalsBase.y, width, height,
                                        kGrid, 0u };
        // Each output buffer is used once: created in COMMON, written here, copied, never touched again.
        D3D12_RESOURCE_BARRIER toUav = {};
        toUav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toUav.Transition.pResource = slot.output.Get();
        toUav.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        toUav.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        toUav.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toUav);

        ID3D12DescriptorHeap* heaps[] = { heap.Get() };
        commandList->SetDescriptorHeaps(1, heaps);
        commandList->SetComputeRootSignature(rootSignature.Get());
        commandList->SetPipelineState(pipeline.Get());
        commandList->SetComputeRootDescriptorTable(0, gpu);
        commandList->SetComputeRootUnorderedAccessView(1, slot.output->GetGPUVirtualAddress());
        commandList->SetComputeRoot32BitConstants(2, 8, constants, 0);
        commandList->Dispatch(kGrid / 8, kGrid / 8, 1);

        D3D12_RESOURCE_BARRIER toCopy = {};
        toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toCopy.Transition.pResource = slot.output.Get();
        toCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toCopy);
        commandList->CopyBufferRegion(slot.readback.Get(), 0, slot.output.Get(), 0, kDataBytes);

        // The completion token, after the data (see the struct comment).
        slot.token = 0x5A5A000000000000ull | (uint64_t(frame) << 8) | uint64_t(recorded + 1);
        tokenUploadPtr[recorded] = slot.token;
        commandList->CopyBufferRegion(slot.readback.Get(), kDataBytes, tokenUpload.Get(),
                                      sizeof(uint64_t) * recorded, sizeof(uint64_t));

        slot.depthFormat = depthDesc.Format;
        slot.normalsFormat = normalsDesc.Format;
        slot.width = width;
        slot.height = height;
        slot.frame = frame;
        ++recorded;
        return true;
    }

    // The first recorded slot whose token has landed on two polls in a row, or -1.
    int Poll()
    {
        for (UINT i = 0; i < recorded; ++i)
        {
            Slot& slot = slots[i];
            // Map hands back the start of the buffer whatever the range; the range only says what is read.
            void* mapped = nullptr;
            const D3D12_RANGE range = { SIZE_T(kDataBytes), SIZE_T(kDataBytes + sizeof(uint64_t)) };
            if (FAILED(slot.readback->Map(0, &range, &mapped)) || mapped == nullptr)
                continue;
            uint64_t value = 0;
            memcpy(&value, static_cast<const uint8_t*>(mapped) + kDataBytes, sizeof(value));
            const D3D12_RANGE nothingWritten = { 0, 0 };
            slot.readback->Unmap(0, &nothingWritten);

            if (value != slot.token)
                continue;
            if (!slot.tokenSeen)
            {
                slot.tokenSeen = true;
                continue;
            }
            return static_cast<int>(i);
        }
        return -1;
    }

    // Logs the capture in slot i (its token has landed).
    void Log(uint32_t handleId, UINT i)
    {
        const Slot& slot = slots[i];
        void* mapped = nullptr;
        const D3D12_RANGE range = { 0, SIZE_T(kDataBytes) };
        if (FAILED(slot.readback->Map(0, &range, &mapped)) || mapped == nullptr)
        {
            LOG_WARN("[RR_INPUT] content probe: the readback could not be mapped");
            return;
        }
        const float* data = static_cast<const float*>(mapped);

        std::vector<float> depth;
        depth.reserve(kSamples);
        uint32_t nonFinite = 0;
        uint32_t zeros = 0;
        uint32_t ones = 0;
        for (UINT s = 0; s < kSamples; ++s)
        {
            const float d = data[s * 4];
            if (!std::isfinite(d))
            {
                ++nonFinite;
                continue;
            }
            zeros += d == 0.0f ? 1u : 0u;
            ones += d == 1.0f ? 1u : 0u;
            depth.push_back(d);
        }

        double lengthSum = 0.0;
        double unormLengthSum = 0.0;
        double alphaSum = 0.0;
        uint32_t unit = 0;
        uint32_t unormUnit = 0;
        uint32_t normalsCounted = 0;
        float minComponent = std::numeric_limits<float>::max();
        float maxComponent = std::numeric_limits<float>::lowest();
        for (UINT s = 0; s < kSamples; ++s)
        {
            const float* n = data + (kSamples + s) * 4;
            if (!std::isfinite(n[0]) || !std::isfinite(n[1]) || !std::isfinite(n[2]) || !std::isfinite(n[3]))
                continue;
            ++normalsCounted;
            const float length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
            const float ux = n[0] * 2.0f - 1.0f;
            const float uy = n[1] * 2.0f - 1.0f;
            const float uz = n[2] * 2.0f - 1.0f;
            const float unormLength = std::sqrt(ux * ux + uy * uy + uz * uz);
            lengthSum += length;
            unormLengthSum += unormLength;
            alphaSum += n[3];
            unit += std::abs(length - 1.0f) < 0.05f ? 1u : 0u;
            unormUnit += std::abs(unormLength - 1.0f) < 0.05f ? 1u : 0u;
            minComponent = std::min({ minComponent, n[0], n[1], n[2] });
            maxComponent = std::max({ maxComponent, n[0], n[1], n[2] });
        }

        const D3D12_RANGE nothingWritten = { 0, 0 };
        slot.readback->Unmap(0, &nothingWritten);

        const auto percentile = [&depth](double p) -> float
        {
            if (depth.empty())
                return std::numeric_limits<float>::quiet_NaN();
            const size_t k = std::min(depth.size() - 1, static_cast<size_t>(p * double(depth.size() - 1) + 0.5));
            std::nth_element(depth.begin(), depth.begin() + k, depth.end());
            return depth[k];
        };
        const float minDepth = depth.empty() ? std::numeric_limits<float>::quiet_NaN()
                                             : *std::min_element(depth.begin(), depth.end());
        const float maxDepth = depth.empty() ? std::numeric_limits<float>::quiet_NaN()
                                             : *std::max_element(depth.begin(), depth.end());
        const float p50 = percentile(0.50);
        const float p99 = percentile(0.99);
        const double rcpSamples = 1.0 / double(kSamples);
        const double rcpNormals = normalsCounted > 0 ? 1.0 / double(normalsCounted) : 0.0;
        const auto depthName = magic_enum::enum_name(slot.depthFormat);
        const auto normalsName = magic_enum::enum_name(slot.normalsFormat);

        LOG_INFO("[RR_INPUT] handle={} depth probe ({}x{} grid over {}x{}, frame {}): format={}, min={:.6g}, "
                 "p50={:.6g}, p99={:.6g}, max={:.6g}, zeroFraction={:.4f}, oneFraction={:.4f}, nonFinite={}",
                 handleId, kGrid, kGrid, slot.width, slot.height, slot.frame,
                 depthName.empty() ? "UNKNOWN" : depthName, minDepth, p50, p99, maxDepth, zeros * rcpSamples,
                 ones * rcpSamples, nonFinite);
        LOG_INFO("[RR_INPUT] handle={} normals probe: format={}, as stored: meanLength={:.4f}, unitFraction={:.4f}, "
                 "minComponent={:.4f}, maxComponent={:.4f}, alphaMean={:.4f}; read as UNORM (2x-1): meanLength={:.4f}, "
                 "unitFraction={:.4f}; samples={}",
                 handleId, normalsName.empty() ? "UNKNOWN" : normalsName, lengthSum * rcpNormals, unit * rcpNormals,
                 normalsCounted ? minComponent : 0.0f, normalsCounted ? maxComponent : 0.0f, alphaSum * rcpNormals,
                 unormLengthSum * rcpNormals, unormUnit * rcpNormals, normalsCounted);
    }
};

FSRDFeatureDx12::FSRDFeatureDx12(uint32_t InHandleId, NVSDK_NGX_Parameter* InParameters) :
    FSR31FeatureDx12(InHandleId, InParameters),
    IFeature(InHandleId, SetParameters(InParameters)),  
    _pDenoiserCtx(nullptr), 
    _denoiserCtxDesc({}),
    _denoiserSettings({}), 
    _convDesc({})
{
    StreamlineHooks::rrTagConsumerAdded();

    _moduleLoaded = FfxApiProxy::IsDenoiserApiImplementedDx12();

    if (FfxApiProxy::IsDenoiserApiImplementedDx12())
        LOG_INFO("amd_fidelityfx_denoiser_dx12.dll methods loaded!");
    else if (FfxApiProxy::IsDenoiserReady())
    {
        const feature_version version = FfxApiProxy::VersionDx12_RR();
        LOG_ERROR("amd_fidelityfx_denoiser_dx12.dll {}.{}.{} loaded, but this dispatch backend is not implemented",
                  version.major, version.minor, version.patch);
    }
    else
        LOG_ERROR("can't load amd_fidelityfx_denoiser_dx12.dll methods!");
}

FSRDFeatureDx12::~FSRDFeatureDx12()
{
    // B9: atomics only (no lock, no log), so it is safe at shutdown as well; the release itself is deferred.
    StreamlineHooks::rrTagConsumerRemoved();

    if (State::Instance().isShuttingDown)
        return;

    DestroyDenoiserContext();
}

bool FSRDFeatureDx12::AcquireSLTaggedResource(
    const RRD3D12SignalTagSnapshot& snapshot, RRTaggedSignal signal,
    const char* sourceName, TagStatePolicy statePolicy,
    Microsoft::WRL::ComPtr<ID3D12Resource>& resource,
    RRTaggedResourceDiagnostic& diagnostic)
{
    resource.Reset();
    diagnostic = {};

    const size_t index = static_cast<size_t>(signal);
    if (index >= snapshot.resources.size())
        return false;

    const auto& entry = snapshot.resources[index];
    diagnostic = entry.diagnostic;
    if (!diagnostic.observed || !diagnostic.present)
        return false;

    if (diagnostic.lifecycle == sl::ResourceLifecycle::eOnlyValidNow &&
        diagnostic.source != RRTagSource::EvaluateFeature)
    {
        LOG_DEBUG(
            "[RR_INPUT] {} uses Streamline eOnlyValidNow outside the active EvaluateFeature call; refusing a cached binding",
            sourceName);
        return false;
    }

    const uint32_t activeFrame = snapshot.activeEvaluationFrame;
    const uint32_t activeViewport = snapshot.activeEvaluationViewport;
    if (activeFrame == UINT32_MAX || activeViewport == UINT32_MAX)
    {
        LOG_DEBUG(
            "[RR_INPUT] {} has no active Streamline frame/viewport; refusing an uncorrelated tag",
            sourceName);
        return false;
    }

    if (diagnostic.viewport != activeViewport)
    {
        LOG_DEBUG(
            "[RR_INPUT] {} belongs to Streamline viewport {}, active viewport is {}; rejecting cross-viewport tag",
            sourceName, diagnostic.viewport, activeViewport);
        return false;
    }

    const bool isLegacyTag = diagnostic.frameIndex == UINT32_MAX;

    if (!isLegacyTag)
    {
        if (diagnostic.frameIndex != activeFrame)
        {
            LOG_DEBUG(
                "[RR_INPUT] {} belongs to Streamline frame {}, active frame is {}; rejecting stale tag",
                sourceName, diagnostic.frameIndex, activeFrame);
            return false;
        }
    }
    else
    {
        // Legacy slSetTag has no frame token. Allow a newly submitted update and
        // repeated acquisition during this same evaluation, but never reuse it in
        // a later frame after its Present/Evaluate lifetime may have expired.
        const uint64_t lastUpdate = _lastConsumedSLTagUpdates[index];
        const uint32_t lastFrame = _lastConsumedSLTagFrames[index];
        if (diagnostic.updateCount < lastUpdate ||
            (diagnostic.updateCount == lastUpdate && lastFrame != activeFrame))
        {
            LOG_DEBUG(
                "[RR_INPUT] {} legacy tag update {} was already consumed in frame {}; rejecting stale reuse in frame {}",
                sourceName, diagnostic.updateCount, lastFrame, activeFrame);
            return false;
        }
    }

    if (!entry.resource || entry.resource.Get() != diagnostic.resourceAddress)
    {
        LOG_WARN(
            "[RR_INPUT] {} atomic tag snapshot has no matching retained D3D12 resource (generation={})",
            sourceName, snapshot.generation);
        return false;
    }

    if (diagnostic.state == UINT32_MAX)
    {
        LOG_WARN("[RR_INPUT] {} has no declared D3D12 resource state", sourceName);
        return false;
    }

    const D3D12_RESOURCE_STATES declaredState =
        static_cast<D3D12_RESOURCE_STATES>(diagnostic.state);
    const bool declaredShaderReadable =
        (declaredState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0;
    const bool declaredStateAcceptable =
        statePolicy == TagStatePolicy::AnyDeclaredState || declaredShaderReadable ||
        (statePolicy == TagStatePolicy::AllowCommonTransition && diagnostic.state == 0u);
    if (!declaredStateAcceptable)
    {
        LOG_WARN(
            "[RR_INPUT] {} is not declared NON_PIXEL_SHADER_RESOURCE (state={:#x}); refusing an untracked external-state transition",
            sourceName, diagnostic.state);
        return false;
    }

    const D3D12_RESOURCE_DESC desc = entry.resource->GetDesc();
    if (desc.Width != diagnostic.nativeWidth ||
        desc.Height != diagnostic.nativeHeight ||
        desc.Format != diagnostic.format ||
        desc.Dimension != diagnostic.dimension ||
        desc.DepthOrArraySize != diagnostic.arraySize ||
        desc.MipLevels != diagnostic.mipLevels ||
        desc.SampleDesc.Count != diagnostic.sampleCount)
    {
        LOG_WARN(
            "[RR_INPUT] {} resource description no longer matches its atomic tag metadata",
            sourceName);
        return false;
    }

    // Record legacy-tag consumption only after the tag has cleared every check.
    // Recording it earlier marks a tag consumed that we then reject, and the
    // staleness rule above would refuse that same updateCount on every later frame
    // unless the game happens to re-submit the tag.
    if (isLegacyTag)
    {
        _lastConsumedSLTagUpdates[index] = diagnostic.updateCount;
        _lastConsumedSLTagFrames[index] = activeFrame;
    }

    resource = entry.resource;
    return true;
}

bool FSRDFeatureDx12::AcquireTaggedAmbientOcclusionResources(bool logFailure)
{
    _ambientOcclusionNoisy.Reset();
    _ambientOcclusionDenoised.Reset();

    const RRD3D12SignalTagSnapshot snapshot =
        StreamlineHooks::getRRD3D12SignalTagSnapshot();
    RRTaggedResourceDiagnostic noisy {};
    RRTaggedResourceDiagnostic denoised {};
    Microsoft::WRL::ComPtr<ID3D12Resource> noisyResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> denoisedResource;
    const bool acquiredNoisy = AcquireSLTaggedResource(
        snapshot, RRTaggedSignal::AmbientOcclusionNoisy,
        "Streamline.AmbientOcclusionNoisy", TagStatePolicy::RequireShaderRead,
        noisyResource, noisy);
    const bool acquiredDenoised = AcquireSLTaggedResource(
        snapshot, RRTaggedSignal::AmbientOcclusionDenoised,
        "Streamline.AmbientOcclusionDenoised", TagStatePolicy::AnyDeclaredState,
        denoisedResource, denoised);

    const auto validMetadata = [this](const RRTaggedResourceDiagnostic& resource) {
        return resource.observed && resource.present &&
            resource.dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
            resource.format == DXGI_FORMAT_R8_UNORM &&
            resource.nativeWidth >= RenderWidth() && resource.nativeHeight >= RenderHeight() &&
            resource.effectiveWidth == RenderWidth() && resource.effectiveHeight == RenderHeight() &&
            resource.extentLeft == 0 && resource.extentTop == 0 &&
            resource.mipLevels == 1 && resource.arraySize == 1 && resource.sampleCount == 1 &&
            resource.state != UINT32_MAX;
    };

    if (!acquiredNoisy || !acquiredDenoised ||
        !validMetadata(noisy) || !validMetadata(denoised))
    {
        if (logFailure)
        {
            LOG_WARN(
                "[RR_AO] tagged AO requested but the noisy/denoised pair is not dispatch-safe. "
                "noisyPresent={}, denoisedPresent={}, noisy={}x{} {}, denoised={}x{} {}, noisyState={:#x}. "
                "Required: full-resolution R8_UNORM Texture2D resources and a shader-readable noisy input.",
                noisy.present, denoised.present,
                noisy.effectiveWidth, noisy.effectiveHeight, magic_enum::enum_name(noisy.format),
                denoised.effectiveWidth, denoised.effectiveHeight, magic_enum::enum_name(denoised.format),
                noisy.state);
        }
        return false;
    }

    _ambientOcclusionNoisy = std::move(noisyResource);
    _ambientOcclusionDenoised = std::move(denoisedResource);

    _ambientOcclusionNoisyState = static_cast<D3D12_RESOURCE_STATES>(noisy.state);
    _ambientOcclusionDenoisedState = static_cast<D3D12_RESOURCE_STATES>(denoised.state);
    return true;
}

bool FSRDFeatureDx12::PublishAmbientOcclusionOutput(ID3D12GraphicsCommandList* commandList)
{
    if (!_ambientOcclusionEnabled)
        return true;

    if (!_ambientOcclusionDenoised)
    {
        LOG_ERROR("[RR_AO] tagged AO output disappeared before publication");
        return false;
    }

    return FSRDConvShader->CopyAmbientOcclusionOutput(
        commandList, _ambientOcclusionDenoised.Get(), _ambientOcclusionDenoisedState,
        RenderWidth(), RenderHeight());
}

bool FSRDFeatureDx12::s_ngxDepthTypeSeen = false;
bool FSRDFeatureDx12::s_ngxReportedHWDepth = false;

bool FSRDFeatureDx12::InitFSR3(const NVSDK_NGX_Parameter* InParameters)
{
    LOG_FUNC();

    // Init upscaler first - borrow some init boilerplate and some cfg
    if (FSR31FeatureDx12::InitFSR3(InParameters))
    {
        SetInit(false);

        LOG_DEBUG("FSR Ray Regeneration Initializing");
        _name = "FSR-RR";

        if (int value; InParameters->Get(NVSDK_NGX_Parameter_Use_HW_Depth, &value) == NVSDK_NGX_Result_Success)
        {
            _hasNGXDepthType = true;
            _ngxReportedHWDepth = value == NVSDK_NGX_DLSS_Depth_Type_HW;
            s_ngxDepthTypeSeen = true;
            s_ngxReportedHWDepth = _ngxReportedHWDepth;
        }
        else if (s_ngxDepthTypeSeen)
        {
            // The depth type is a property of the title, not of one feature instance.
            // NGX only publishes it on a creation carrying DLSSD create params, so a
            // later recreation - a resolution or preset change, or a backend switch -
            // can arrive without it. Re-deriving per instance therefore loses a
            // declaration the title already made, and the resource inference below
            // then has to guess for the rest of the session.
            _hasNGXDepthType = true;
            _ngxReportedHWDepth = s_ngxReportedHWDepth;
            LOG_INFO("[RR_INPUT] {} absent on this creation; reusing the {} type this "
                     "title declared earlier",
                     NVSDK_NGX_Parameter_Use_HW_Depth,
                     s_ngxReportedHWDepth ? "hardware" : "linear");
        }
        else
        {
            // NVSDK_NGX_DLSS_Depth_Type_Linear is zero, so a title that never fills
            // the field reads as linear. Most DLSS-RR titles actually supply a
            // hardware depth buffer, and interpreting one as a view-space distance
            // clamps the entire scene into [near, 1]: RR then sees a metre-deep
            // world, disocclusion and the depth delta stop working, and the linear
            // depth debug view goes black. Warn loudly and name the override.
            _hasNGXDepthType = false;
            _ngxReportedHWDepth = false;
            LOG_WARN(
                "[RR_INPUT] title did not publish {}; the depth type will be inferred from the "
                "depth resource, falling back to LINEAR if its format is ambiguous. If the "
                "linear-depth debug view is black or geometry-dependent behaviour looks wrong, "
                "set [FSR-RR] HardwareDepth=true in OptiScaler.ini or use Depth Input in the menu.",
                NVSDK_NGX_Parameter_Use_HW_Depth);
        }
        _isHWDepth = _ngxReportedHWDepth;

        if (int value; InParameters->Get(NVSDK_NGX_Parameter_DLSS_Roughness_Mode, &value) == NVSDK_NGX_Result_Success)
        {
            if (value == NVSDK_NGX_DLSS_Roughness_Mode_Packed)
                _roughnessSource = RoughnessSource::Packed;
            else if (value == NVSDK_NGX_DLSS_Roughness_Mode_Unpacked)
                _roughnessSource = RoughnessSource::Separate;
            else
                LOG_WARN("Unknown DLSSD roughness mode {}; deferring source selection", value);
        }

        const char* roughnessSource = _roughnessSource == RoughnessSource::Packed
            ? "packed"
            : (_roughnessSource == RoughnessSource::Separate ? "separate" : "undetermined");
        LOG_INFO("DLSSD Flags HWDepth: {} (NGX reported: {}) - RoughnessSource: {}", _isHWDepth,
                 _hasNGXDepthType ? (_ngxReportedHWDepth ? "hardware" : "linear") : "absent",
                 roughnessSource);

        _autoSpecularSignalDescType = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR;
        _autoSpecularSignalResolved = false;
        _autoDiffuseSignalDescType = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE;
        _autoDiffuseSignalResolved = false;

        // Before the context's configure baseline and the first conversion read the keys the
        // path-traced profile covers.
        _appliedPathTracedProfile = SyncPathTracedProfile(*Config::Instance()) ? 1 : 0;
        _appliedPathTracedTemporal = s_pathTracedTemporalApplied ? 1 : 0;

        if (!CreateDenoiserContext())
            return false;

        LOG_INFO("FSR Ray Regeneration Initialized");

        // (AMDNR 0.3.4, RR report from a Proton player: "I don't have to have NR enabled for this to take effect
        // right?") Once per process: Ray Regeneration does not depend on Neural Rendering. Both NR runtimes load
        // AMD's HIP runtime (amdhip64_7.dll, part of the Windows AMD driver), which Wine/Proton does not provide
        // (amd_bridge.log: "HIP not available ... Win32 error 126"). (0.3.4.2, H3) The Wine/Proton line no longer
        // says RR "works here": 0.3.4.1's docs list FSR Ray Regeneration as a known issue under Wine/Proton (pink /
        // magenta patches in some games); FSR upscaling is what works there. Still once per process (the latch).
        static bool loggedRrWithoutNr = false;
        if (!loggedRrWithoutNr)
        {
            loggedRrWithoutNr = true;
            LOG_INFO("[RR_DIAG] FSR Ray Regeneration does not need Neural Rendering: it denoises the game's ray "
                     "tracing before NR runs and works the same with NR off.");
            if (State::Instance().isRunningOnLinux)
                LOG_INFO("[RR_DIAG] Running under Wine/Proton: FSR upscaling works here; FSR Ray Regeneration is a "
                         "known issue here (pink / magenta patches in some games: use plain FSR, FSR 4 on RX 9000, "
                         "and send a Save report). Neural Rendering cannot run here: both NR runtimes need AMD's "
                         "HIP runtime (amdhip64_7.dll from the Windows AMD driver), which Wine/Proton does not "
                         "provide.");
        }

        // RR-19: an FSR_RR feature now serves this handle (see ClearRRFallback).
        ClearRRFallback(Handle()->Id, false);

        SetInit(true);
        return true;
    }
 
    return false;
}

bool FSRDFeatureDx12::CreateDenoiserContext() 
{
    ScopedSkipSpoofingGlobal skipSpoofingGlobal {};
    auto& state = State::Instance();
    const auto& cfg = *Config::Instance();

    if (!QueryDenoiserVersions())
        return false;

    InvalidateDenoiserHistory();

    const int requestedDenoiserIndex = cfg.FfxDenoiserIndex.value_or_default();
    const size_t denoiserIndex = std::clamp<size_t>(
        requestedDenoiserIndex < 0 ? 0u : static_cast<size_t>(requestedDenoiserIndex),
        0u, state.ffxDenoiserVersionIds.size() - 1u);

    if (denoiserIndex != static_cast<size_t>(std::max(requestedDenoiserIndex, 0)))
    {
        LOG_WARN("Configured RR provider index {} is unavailable; using provider index {}",
                 requestedDenoiserIndex, denoiserIndex);
    }

    const char* providerName = state.ffxDenoiserVersionNames[denoiserIndex]
        ? state.ffxDenoiserVersionNames[denoiserIndex]
        : "<unnamed>";

    // Parse the denoiser provider version into this instance's own field; the
    // SR upscaler version reported by Version() must stay untouched.
    _denoiserVersion.parse_version(providerName);

    _diffuseSignalDescType = GetDiffuseSignalDescType(
        cfg, _autoDiffuseSignalDescType);
    _specularSignalDescType = GetSpecularSignalDescType(
        cfg, _autoSpecularSignalDescType);
    // Single-signal mode: a disabled signal is neither dispatched nor declared at
    // context creation. Both disabled is a configuration error - fall back to both.
    _denoiseDiffuse = cfg.FfxDenoiserDenoiseDiffuse.value_or_default();
    _denoiseSpecular = cfg.FfxDenoiserDenoiseSpecular.value_or_default();
    if (!_denoiseDiffuse && !_denoiseSpecular)
    {
        LOG_WARN("FSR-RR DenoiseDiffuse and DenoiseSpecular are both false; "
                 "denoising both signals instead");
        _denoiseDiffuse = true;
        _denoiseSpecular = true;
    }
    _ambientOcclusionEnabled =
        cfg.FfxDenoiserTaggedAmbientOcclusion.value_or_default() &&
        AcquireTaggedAmbientOcclusionResources(true);
    // RR 1.2 supports specular occlusion, and the preprocessor reserves an R8 output for it,
    // but Streamline exposes no semantic SO tag. Keep it source-gated until a real input exists.
    _specularOcclusionEnabled = false;

    uint32_t selectedSignalFlags = 0;
    if (_denoiseDiffuse)
        selectedSignalFlags |= GetSignalFlag(_diffuseSignalDescType);
    if (_denoiseSpecular)
        selectedSignalFlags |= GetSignalFlag(_specularSignalDescType);
    if (selectedSignalFlags == 0)
        selectedSignalFlags = GetSignalFlag(_diffuseSignalDescType) | GetSignalFlag(_specularSignalDescType);
    if (_ambientOcclusionEnabled)
        selectedSignalFlags |= FFX_DENOISER_SIGNAL_AMBIENT_OCCLUSION;

    ffxOverrideVersion vidOverride = 
    {
        .header = { .type = FFX_API_DESC_TYPE_OVERRIDE_VERSION },
        .versionId = state.ffxDenoiserVersionIds[denoiserIndex]
    };
    // Create context
    // Backend desc
    ffxCreateBackendDX12Desc backendDesc = 
    { 
        .header = 
        { 
            .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12,
            .pNext = &vidOverride.header // Chain override into backend desc
        },
        .device = Device
    };    
    // Chain: ContextDesc -> BackendDesc -> OverrideVersion.
    // DLSS-RR exposes generic diffuse/specular lighting, while RR 1.2 requires
    // direct/indirect classification. The selected approximation is configurable.
    // Preserve the allocation ceiling when this context is recreated for a
    // signal-policy change while DRS is currently below its creation size.
    const uint32_t maxRenderWidth = std::max(
        _denoiserCtxDesc.maxRenderSize.width, RenderWidth());
    const uint32_t maxRenderHeight = std::max(
        _denoiserCtxDesc.maxRenderSize.height, RenderHeight());
    _denoiserCtxDesc = 
    {
        .header = 
        { 
            .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER,
            // Chain backend desc into context desc
            .pNext = &backendDesc.header
        },
        .version = FFX_DENOISER_VERSION,
        .maxRenderSize = { maxRenderWidth, maxRenderHeight },
        .signalFlags = selectedSignalFlags,
        .checkerboardSignalFlags = FFX_DENOISER_SIGNAL_NONE,
        .flags = 0
    };

    if (cfg.FfxDenoiserInternalDebugViews.value_or_default())
    {
        _denoiserCtxDesc.flags |= FFX_DENOISER_ENABLE_DEBUGGING;
        LOG_INFO("[RR_DIAG] AMD internal debug views enabled for this denoiser context");
    }

#ifdef _DEBUG
    LOG_INFO("Debug views and validation enabled for denoiser!");
    _denoiserCtxDesc.flags |= FFX_DENOISER_ENABLE_DEBUGGING | FFX_DENOISER_ENABLE_VALIDATION;
#endif

    LOG_INFO(
        "[RR_DIAG] creating context: providerIndex={}, providerName='{}' ({}.{}.{}), providerId={:#x}, "
        "api={}.{}.{}, maxRenderSize={}x{}, signalFlags={:#x}, checkerboardFlags={:#x}, createFlags={:#x}",
        denoiserIndex, providerName, _denoiserVersion.major, _denoiserVersion.minor,
        _denoiserVersion.patch, state.ffxDenoiserVersionIds[denoiserIndex],
        FFX_DENOISER_VERSION_MAJOR, FFX_DENOISER_VERSION_MINOR, FFX_DENOISER_VERSION_PATCH,
        _denoiserCtxDesc.maxRenderSize.width, _denoiserCtxDesc.maxRenderSize.height,
        _denoiserCtxDesc.signalFlags, _denoiserCtxDesc.checkerboardSignalFlags,
        _denoiserCtxDesc.flags);
    LOG_INFO("[RR_DIAG] signal classification: diffuse={}, specular={}, ambientOcclusion={}, "
             "specularOcclusion={} (no semantic source)",
             GetSignalTypeName(_diffuseSignalDescType), GetSignalTypeName(_specularSignalDescType),
             _ambientOcclusionEnabled, _specularOcclusionEnabled);
    spdlog::info(L"" __FUNCTIONW__ L" [RR_DIAG] denoiser module: {}",
                 FfxApiProxy::Dx12Module_Denoiser_Path());

    // Create the denoiser context
    {   
        ScopedSkipHeapCapture skipHeapCapture {};
        auto ret = FfxApiProxy::D3D12_CreateContext(&_pDenoiserCtx, &_denoiserCtxDesc.header, NULL);

        if (ret != FFX_API_RETURN_OK)
        {
            LOG_ERROR("_denoiserCtx error: {0}", FfxApiProxy::ReturnCodeToString(ret));
            return false;
        }

        LOG_INFO("[RR_DIAG] context creation succeeded: context={:X}",
                 reinterpret_cast<uintptr_t>(_pDenoiserCtx));

        // RR-26 (AMDNR 0.3.4): the proxy's version query passes no D3D12 device (and runs before the game's device
        // exists), lists no provider and logs "RR denoiser did not report a version ... unverified" (every RE Requiem
        // log, Windows and Proton; with the gate's proxy patch it is an INFO "version not listed yet"). This query had
        // the device and named the provider, so say once what it named. A provider name without a version (parse ->
        // 0.x) is not called a mismatch: it is used as 1.2 exactly as the proxy already does.
        static bool versionConfirmLogged = false;
        if (!versionConfirmLogged)
        {
            versionConfirmLogged = true;
            if (_denoiserVersion.major == FFX_DENOISER_VERSION_MAJOR &&
                _denoiserVersion.minor == FFX_DENOISER_VERSION_MINOR)
                LOG_INFO("[RR_DIAG] FSR Ray Regeneration version confirmed with the game's D3D12 device: '{}', the "
                         "version this build implements ({}.{}.{}). An earlier version line from the query without "
                         "the device ('did not report a version' / 'version not listed yet') does not apply.",
                         providerName, FFX_DENOISER_VERSION_MAJOR, FFX_DENOISER_VERSION_MINOR,
                         FFX_DENOISER_VERSION_PATCH);
            else if (_denoiserVersion.major == 0)
                LOG_INFO("[RR_DIAG] FSR Ray Regeneration provider '{}' names no version, also with the game's D3D12 "
                         "device; its context was created and it is used as {}.{}.{}.",
                         providerName, FFX_DENOISER_VERSION_MAJOR, FFX_DENOISER_VERSION_MINOR,
                         FFX_DENOISER_VERSION_PATCH);
            else
                LOG_WARN("[RR_DIAG] FSR Ray Regeneration provider '{}' ({}.{}.{}) is not the version this build "
                         "implements ({}.{}.{}); its context was created, but if Ray Regeneration misbehaves, "
                         "check the amd_fidelityfx_denoiser_dx12.dll named by the '[RR_DIAG] denoiser module' line "
                         "first.",
                         providerName, _denoiserVersion.major, _denoiserVersion.minor, _denoiserVersion.patch,
                         FFX_DENOISER_VERSION_MAJOR, FFX_DENOISER_VERSION_MINOR, FFX_DENOISER_VERSION_PATCH);
        }
    }

    // Query default settings
    if (!SetDefaultConfiguration())
    {
        LOG_ERROR("Failed to query the RR 1.2 default configuration");
        DestroyDenoiserContext();
        return false;
    }

    // The queried values are AMD's tuned baseline, but the per-frame configure pass
    // overwrites every one of them with the INI values. Keep a copy so the A/B switch
    // can push AMD's numbers back without a context recreation, and record both so a
    // temporal artefact can be attributed to - or cleared of - an over-aggressive
    // override without guessing what RR would have used on its own.
    _denoiserAmdDefaults = _denoiserSettings;

    LOG_INFO("[RR_DIAG] configure baseline (AMD default -> fork default), active source={}: "
             "disocclusionThreshold={:.4f} -> {:.4f}, crossBilateralNormalStrength={:.4f} -> {:.4f}, "
             "stabilityBias={:.4f} -> {:.4f}, maxRadiance={:.1f} -> {:.1f}, "
             "radianceClipStdK={:.4f} -> {:.4f}, gaussianKernelRelaxation={:.4f} -> {:.4f}",
             cfg.FfxDenoiserUseAmdDefaults.value_or_default() ? "AMD" : "fork",
             _denoiserSettings.m_DisocclusionThreshold, cfg.FfxDenoiserDisocThreshold.value_or_default(),
             _denoiserSettings.m_CrossBilateralNormalStrength, cfg.FfxDenoiserCrossBlNormStr.value_or_default(),
             _denoiserSettings.m_StabilityBias, cfg.FfxDenoiserStabilityBias.value_or_default(),
             _denoiserSettings.m_MaxRadiance, cfg.FfxDenoiserMaxRadiance.value_or_default(),
             _denoiserSettings.m_RadianceClipStdK, cfg.FfxDenoiserRadianceClip.value_or_default(),
             _denoiserSettings.m_GaussianKernelRelaxation, cfg.FfxDenoiserGaussKernRelax.value_or_default());
    // RR-08: the [RR_PROFILE] line waits for this context's first evaluate, after the inputs were read, so it can
    // say whether the title publishes a bias mask. The route is recorded now, so the route-change check starts here.
    _loggedTextureRoute = ProfileTextureRoute(*Config::Instance());
    _pendingRRProfileReason = "context created";

    // Create DLSS-RR to FSR-RR input converter
    auto newConverter =
        std::make_unique<FSRDPreprocessor_Dx12>("FSRD Converter", Device);

    if (!newConverter->IsInit())
    {
        LOG_ERROR("Failed to initialize the FSR-RR input converter");
        DestroyDenoiserContext();
        return false;
    }

    // RR-05 (AMDNR 0.3.4): FP16 albedo storage, an experiment for the etched look on low albedo (RE Requiem),
    // read when the converter is created (so a change applies at the next feature or context creation). Off by
    // default: R8G8B8A8_UNORM targets and 255-level rounding, as in 0.3.3.2. Whether RR 1.2 accepts FP16 albedo is
    // the owner test; a refusal shows as dispatch errors in the log.
    if (cfg.FfxDenoiserAlbedoFp16.value_or_default())
    {
        newConverter->SetAlbedoStorageFp16(true);
        LOG_INFO("FSR-RR: albedo stored as R16G16B16A16_FLOAT ([FSR-RR] FfxDenoiserAlbedoFp16=true, experimental)");
    }

    if (!newConverter->SetMaxRenderSize(
            _denoiserCtxDesc.maxRenderSize.width,
            _denoiserCtxDesc.maxRenderSize.height))
    {
        LOG_ERROR("Failed to allocate the FSR-RR input converter");
        DestroyDenoiserContext();
        return false;
    }

    FSRDConvShader = std::move(newConverter);

    // The converter is brand new, so nothing in flight can reference its resources.
    _preprocessorHasRecordedWork = false;
    _logNextDenoiserDispatch = true;
    _lastDispatchRequestedReset = false;

    return true;
}

bool FSRDFeatureDx12::QueryDenoiserVersions() 
{
    ScopedSkipSpoofingGlobal skipSpoofingGlobal {};
    auto& state = State::Instance();

    // Get version count
    uint64_t versionCount = 0;
    ffxQueryDescGetVersions queryVersionsDesc = 
    { 
        .header = { .type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS },
        .createDescType = FFX_API_EFFECT_ID_DENOISER,
        .device = Device,
        .outputCount = &versionCount
    };
    const ffxReturnCode_t countResult = FfxApiProxy::D3D12_Query(nullptr, &queryVersionsDesc.header);

    if (countResult != FFX_API_RETURN_OK)
    {
        LOG_ERROR("Failed to query RR provider count: {}",
                  FfxApiProxy::ReturnCodeToString(countResult));
        return false;
    }

    state.ffxDenoiserVersionIds.resize(versionCount);
    state.ffxDenoiserVersionNames.resize(versionCount);

    // The debug views come from a constant table, so they are the same for every context: State's
    // copies are filled once and then left alone rather than cleared and rebuilt on every context
    // creation. The overlay reads the immutable table published below, never State's map.
    if (state.ffxDenoiserDebugModeNames.empty())
    {
        state.ffxDenoiserDebugModes.clear();

        for (const auto& mode : kDebugModes)
        {
            state.ffxDenoiserDebugModes.push_back(mode.second);
            state.ffxDenoiserDebugModeNames.emplace(mode.second, mode.first);
        }
    }

    FSRD::RuntimeStatus::DebugViews.store(&kDebugViewTable, std::memory_order_release);

    if (versionCount == 0)
    {
        LOG_ERROR("No FSR-RR denoisers were found.");
        return false;
    }
    else
        LOG_DEBUG("Found {} versions of FSR-RR", versionCount);

    LOG_DEBUG("Initialising FSR denoiser context");

    // Get version IDs
    queryVersionsDesc.versionIds = state.ffxDenoiserVersionIds.data();
    queryVersionsDesc.versionNames = state.ffxDenoiserVersionNames.data();
    const ffxReturnCode_t versionsResult = FfxApiProxy::D3D12_Query(nullptr, &queryVersionsDesc.header);
    if (versionsResult != FFX_API_RETURN_OK)
    {
        LOG_ERROR("Failed to query RR providers: {}",
                  FfxApiProxy::ReturnCodeToString(versionsResult));
        return false;
    }

    for (size_t i = 0; i < state.ffxDenoiserVersionIds.size(); ++i)
    {
        LOG_INFO("[RR_DIAG] provider[{}]: name='{}', id={:#x}", i,
                 state.ffxDenoiserVersionNames[i] ? state.ffxDenoiserVersionNames[i] : "<unnamed>",
                 state.ffxDenoiserVersionIds[i]);
    }

    return true;
}

void FSRDFeatureDx12::FlushPendingRRProfile(const char* why)
{
    if (_pendingRRProfileReason == nullptr)
        return;

    // The bias mask is as last seen on this instance (usually -1, not looked at yet).
    const std::string reason = std::format("{}; written {}", _pendingRRProfileReason, why);
    _loggedTextureRoute = LogRRProfile(*Config::Instance(), reason.c_str(), DescribeRRSignals(), _biasMaskState);
    _pendingRRProfileReason = nullptr;
}

void FSRDFeatureDx12::DestroyDenoiserContext() 
{
    // RR-08: a context that never reached an evaluate reading its inputs (destroyed first, or its creation failed
    // after the context itself was made) still gets the [RR_PROFILE] line 0.3.3.2 wrote at creation.
    FlushPendingRRProfile("at the context's destruction");

    if (_pDenoiserCtx != nullptr)
    {
        const uintptr_t contextAddress = reinterpret_cast<uintptr_t>(_pDenoiserCtx);
        const ffxReturnCode_t result = FfxApiProxy::D3D12_DestroyContext(&_pDenoiserCtx, nullptr);

        if (result == FFX_API_RETURN_OK)
        {
            LOG_INFO("[RR_DIAG] context destruction succeeded: context={:X}", contextAddress);
        }
        else
        {
            LOG_ERROR("[RR_DIAG] context destruction failed: context={:X}, result={}",
                      contextAddress, FfxApiProxy::ReturnCodeToString(result));
        }
    }

    _pDenoiserCtx = nullptr;
    _ambientOcclusionEnabled = false;
    _specularOcclusionEnabled = false;
    _ambientOcclusionNoisy.Reset();
    _ambientOcclusionDenoised.Reset();
    InvalidateDenoiserHistory();
}

bool FSRDFeatureDx12::UpdateSize()
{
    const uint32_t renderWidth = RenderWidth();
    const uint32_t renderHeight = RenderHeight();
    const uint32_t maxWidth = _denoiserCtxDesc.maxRenderSize.width;
    const uint32_t maxHeight = _denoiserCtxDesc.maxRenderSize.height;

    // maxRenderSize is an allocation ceiling; each dispatch supplies the current
    // logical size. Never release context resources while recording a frame because
    // earlier submitted command lists may still reference them.
    if (renderWidth > maxWidth || renderHeight > maxHeight)
    {
        LOG_WARN(
            "[RR_DIAG] the game renders {}x{}, above the {}x{} Ray Reconstruction was created with (often a 1 px "
            "rounding difference); re-creating FSR Ray Regeneration at the new size, one frame skipped",
            renderWidth, renderHeight, maxWidth, maxHeight);
        InvalidateDenoiserHistory(HistoryResetReason::RenderSize);
        State::Instance().changeBackend[Handle()->Id] = true;
        // 0.3.4.1: tells TryEvaluateOptiFeature this failed evaluate is the planned re-create, not an error.
        FSRD::RuntimeStatus::ResizeRecreateHandle.store(Handle()->Id + 1, std::memory_order_relaxed);
        return false;
    }

    if (_lastDenoiserRenderWidth != 0 &&
        (_lastDenoiserRenderWidth != renderWidth ||
         _lastDenoiserRenderHeight != renderHeight))
    {
        LOG_INFO(
            "[RR_DIAG] logical render size changed within the allocation ceiling: {}x{} -> {}x{}; resetting temporal history without recreating resources",
            _lastDenoiserRenderWidth, _lastDenoiserRenderHeight,
            renderWidth, renderHeight);
        InvalidateDenoiserHistory(HistoryResetReason::RenderSize);
    }

    _lastDenoiserRenderWidth = renderWidth;
    _lastDenoiserRenderHeight = renderHeight;

    // The history window's render-size range: a spread here is dynamic resolution at work.
    if (_historyWindowMinWidth == 0 || renderWidth * renderHeight < _historyWindowMinWidth * _historyWindowMinHeight)
    {
        _historyWindowMinWidth = renderWidth;
        _historyWindowMinHeight = renderHeight;
    }
    if (renderWidth * renderHeight > _historyWindowMaxWidth * _historyWindowMaxHeight)
    {
        _historyWindowMaxWidth = renderWidth;
        _historyWindowMaxHeight = renderHeight;
    }

    return true;
}

bool FSRDFeatureDx12::EvaluateInternal(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters) 
{
    LOG_FUNC();

    if (!IsInited())
        return false;

    // The application submitted a new evaluation even when a later validation,
    // composition, or upscaler step fails. Keep RR frame indices unique on every
    // exit path; in particular, never reuse an index after RR already advanced
    // its internal temporal history.
    EvaluationFrameGuard evaluationFrameGuard(_frameCount);

    auto& state = State::Instance();
    auto& cfg = *Config::Instance();
    const auto& inParams = *InParameters;

    // The previous frame's reset reasons go into the history window before this frame adds its own.
    AccountHistoryResets();

    // Refresh the current render subrect before deciding whether RR must resize.
    // PrepareUpscalerInput queries it too, but that runs after this decision, so the
    // resize would otherwise be made against the previous frame's extent.
    unsigned int currentRenderWidth = RenderWidth();
    unsigned int currentRenderHeight = RenderHeight();
    GetRenderResolution(InParameters, &currentRenderWidth, &currentRenderHeight);
    if (currentRenderWidth == 0 || currentRenderHeight == 0)
    {
        LOG_ERROR("[RR_INPUT] current render resolution is zero");
        FlushPendingRRProfile("early: the first evaluate stopped at a zero render size");
        InvalidateDenoiserHistory(HistoryResetReason::InputNotReady);
        return false;
    }

    if (!UpdateSize())
    {
        FlushPendingRRProfile("early: the first evaluate stopped at the RR size update");
        return false;
    }

    StreamlineHooks::probeRRNGXPointerParameters(inParams);

    // SAT-P0-4: log the content probe once the GPU has written a capture (a map of 8 bytes; never waits).
    PollContentProbe();

    if (!_loggedRRGuidesEvaluate)
    {
        _loggedRRGuidesEvaluate = true;
        LogRRGuides(inParams, "first-evaluate");
    }

    // A later toggle of the path-traced profile reaches this frame's conversion, configure and
    // composition. History built under the other routing is dropped once per instance.
    const int pathTracedProfile = SyncPathTracedProfile(cfg) ? 1 : 0;
    const int pathTracedTemporal = s_pathTracedTemporalApplied ? 1 : 0;
    if (_appliedPathTracedProfile != pathTracedProfile)
    {
        if (_appliedPathTracedProfile >= 0)
        {
            _loggedTextureRoute = LogRRProfile(cfg, "changed; resetting denoiser history", DescribeRRSignals(),
                                               _biasMaskState);
            _pendingRRProfileReason = nullptr;
            InvalidateDenoiserHistory(HistoryResetReason::ProfileToggle);
        }
        _appliedPathTracedProfile = pathTracedProfile;
        _appliedPathTracedTemporal = pathTracedTemporal; // the line above already describes both halves
    }

    // RR-04: the temporal half switched alone (FfxDenoiserPathTracedTemporal while the profile is on). Only the
    // six configure values move, as with a temporal slider, so history is kept.
    if (_appliedPathTracedTemporal != pathTracedTemporal)
    {
        if (_appliedPathTracedTemporal >= 0)
        {
            _loggedTextureRoute = LogRRProfile(cfg, "temporal half switched", DescribeRRSignals(), _biasMaskState);
            _pendingRRProfileReason = nullptr;
        }
        _appliedPathTracedTemporal = pathTracedTemporal;
    }

    // A live change of the texture route (menu slider) is logged once it has held for 60 frames.
    const float textureRoute =
        pathTracedProfile == 1
            ? std::clamp(cfg.FfxDenoiserProfileTextureRoute.value_or_default(), 0.0f, 1.0f)
            : 0.0f;
    if (textureRoute != _pendingTextureRoute)
    {
        _pendingTextureRoute = textureRoute;
        _pendingTextureRouteFrames = 0;
    }
    else if (textureRoute != _loggedTextureRoute && ++_pendingTextureRouteFrames >= 60)
    {
        _loggedTextureRoute = LogRRProfile(cfg, "texture route changed", DescribeRRSignals(), _biasMaskState);
        _pendingRRProfileReason = nullptr;
    }

    // RR-12: a live change of the temporal values, the AMD-defaults switch or the debug view, logged once settled.
    TrackRRLiveSettings();

    // Conversion leaves the RR signal outputs in UAV state. Always close that
    // state lifetime, including bypass and error paths where composition is skipped.
    DenoiserOutputStateGuard denoiserOutputStateGuard(FSRDConvShader, InCommandList);
    TitleInputStateGuard titleInputStateGuard(FSRDConvShader, InCommandList);

    const auto dbgMode = static_cast<DebugModes>(cfg.FfxDenoiserDebugMode.value_or_default());
    const bool isDebugVis = (uint32_t)dbgMode & (uint32_t) DebugModes::ConversionDebug;
    const bool isDebugComp = ((uint64_t)dbgMode & (uint64_t)DebugModes::CompositionDebug);
    const bool isFfxDebug = dbgMode == DebugModes::FfxDebug;
    const bool isAmbientOcclusionDebug =
        dbgMode == DebugModes::AmbientOcclusionInput ||
        dbgMode == DebugModes::AmbientOcclusionOutput;
    const bool hasAnyDebug = (dbgMode != DebugModes::None);
    _convDesc.FrameIndex = static_cast<uint64_t>(_frameCount);

    // Denoise is bypassed if we are debugging something OTHER than the final outputs
    const bool isDenoiseBypassed = !isFfxDebug && !isDebugComp &&
        hasAnyDebug && !isAmbientOcclusionDebug &&
        dbgMode != DebugModes::DenoiserOutput && dbgMode != DebugModes::UpscalerBypass;

    // Upscale is bypassed if we are in a debug mode that isn't the DenoiserBypass (final raw)
    const bool isUpscaleBypassed = hasAnyDebug && dbgMode != DebugModes::DenoiserBypass;

    _isInReset = false;

    if (uint32_t value = 0; inParams.Get(NVSDK_NGX_Parameter_Reset, &value) == NVSDK_NGX_Result_Success)
        _isInReset = value > 0;

    // The title's own reset drops RR history through the dispatch's RESET flag rather than
    // through InvalidateDenoiserHistory, so it is counted here.
    if (_isInReset)
        _historyResetReasonsThisFrame |= 1u << static_cast<uint32_t>(HistoryResetReason::NgxReset);

    // The conversion reads the title's color, depth and motion vectors in its very first
    // dispatch, so those inputs have to be acquired and taken out of their declared starting
    // states before the denoiser chain begins - acquiring them inside the upscaler branch
    // below would leave the floor and packing passes reading render-target and unordered-access
    // resources as if they were shader resources.
    ffxDispatchDescUpscale upscalerDesc = {};
    if (!PrepareUpscalerInput(InCommandList, inParams, upscalerDesc))
    {
        FlushPendingRRProfile("early: the first evaluate stopped at the upscaler inputs");
        InvalidateDenoiserHistory(HistoryResetReason::InputNotReady);
        return false;
    }

    // Under the path-traced profile the title's own NGX sharpness does not sharpen RR's output in
    // FSR SR. Sharpening is a high-pass gain, so it lifts the low-level grain RR leaves on skin.
    // [Sharpness] OverrideSharpness=true still applies [Sharpness] Sharpness, and RCAS stays the
    // player's own switch (for FSR_RR it only runs when RcasEnabled is set). FSR 4's debug view
    // needs its forced 0.01, so it is left alone.
    // (AMDNR 0.3.4.1) Without the profile, a title that sends no sharpness gets kRrDefaultSharpness
    // here (GetSharpness via DefaultSharpnessWhenTitleSendsNone); under the profile that default is 0,
    // so this block only ever sees the title's own value. (0.3.4.1, Proton RR) On Wine/Proton it is 0 too.
    const float titleSharpness = upscalerDesc.enableSharpening ? upscalerDesc.sharpness : 0.0f;
    bool profileSuppressedSharpness = false;
    if (pathTracedProfile != 0 && upscalerDesc.enableSharpening && !cfg.OverrideSharpness.value_or_default() &&
        !(Version() >= feature_version { 4, 0, 2 } && cfg.FsrDebugView.value_or_default() &&
          cfg.Fsr4EnableDebugView.value_or_default()))
    {
        static bool loggedProfileSharpness = false;
        if (!loggedProfileSharpness)
        {
            loggedProfileSharpness = true;
            LOG_INFO("[RR_PROFILE] title sharpness {:.2f} not applied to FSR SR after RR (path-traced profile; "
                     "[Sharpness] OverrideSharpness=true applies [Sharpness] Sharpness instead)",
                     upscalerDesc.sharpness);
        }

        upscalerDesc.enableSharpening = false;
        upscalerDesc.sharpness = 0.0f;
        profileSuppressedSharpness = true;
    }

    // Optional, configurable resource barriers. The window spans the whole chain and closes on
    // every exit path, including the debug bypass that never reaches the upscaler dispatch, so
    // the title finds each resource in the state it declared.
    FSR31FeatureDx12::ScopedConfigurableBarriers scopedBarriers(*this, InCommandList);

    // Denoiser start
    ffxDispatchDescDenoiserAmbientOcclusion ambientOcclusion = {};
    ffxDispatchDescDenoiserDirectDiffuse directDiffuse = {};
    ffxDispatchDescDenoiserIndirectSpecular indirectSpecular = {};
    ffxDispatchDescDenoiser denoiserDesc = {};
    bool isDenoiserReady = false;
    // The skin smoothing pass's finished colour, when it ran this frame; FSR SR reads it instead of
    // the composition output.
    ID3D12Resource* skinSmoothedColor = nullptr;

    // Pull configuration and input buffers for DLSS-RR from the param table, convert and 
    // repack input buffers into intermediate FSR-RR input buffers, and configure descriptors.
    //
    // A FRAME WHOSE RR INPUTS CANNOT BE GATHERED IS STILL UPSCALED. This used to return
    // false, which meant no output was written at all, and the title kept presenting
    // whatever the output texture last held - a frozen picture under a stack of "Upscaler
    // failed to run" toasts. That was Silent Hill 2, which creates a plain Super Sampling
    // feature and publishes no normals, albedo or camera matrices, so every frame failed
    // here. A missing denoiser input is a reason to skip the denoise, not the upscale: the
    // raw colour is already prepared in `upscalerDesc`, and the guards above close every
    // resource state on this path.
    //
    // And a title that never supplies them is asked to switch to FSR after a bounded run
    // of failures, with the reason logged once. Not on the first, because Streamline
    // constants and tags can arrive a few frames after the first evaluate in a real RR
    // title, and a one-frame gap is not evidence of anything.
    // SAT-P0-2: this frame's camera key state; a frame that stops before ResolveCameraMatrices reads "not read".
    _viewMatrixKeyState = kMatrixKeyNotRead;
    _projMatrixKeyState = kMatrixKeyNotRead;
    _cameraNullKeysThisFrame = false;
    _cameraBlockedThisFrame = false;
    const bool denoiserInputsReady = PrepareDenoiserInput(InCommandList, *InParameters, denoiserDesc,
                                                          ambientOcclusion, directDiffuse, indirectSpecular);

    // SAT-P0-3 (AMDNR 0.3.4): the Streamline camera state on this handle's first evaluate.
    if (!_loggedStreamlineFirst)
    {
        _loggedStreamlineFirst = true;
        if (FirstRRLineForHandle(Handle()->Id, RRHandleLine::StreamlineFirst))
            LogRRStreamlineState(inParams, "first evaluate");
    }

    // RR-08: the context's [RR_PROFILE] line, now that this frame's inputs were read (bias mask known).
    if (_pendingRRProfileReason != nullptr)
    {
        _loggedTextureRoute = LogRRProfile(cfg, _pendingRRProfileReason, DescribeRRSignals(), _biasMaskState);
        _pendingRRProfileReason = nullptr;
    }
    if (!denoiserInputsReady)
    {
        InvalidateDenoiserHistory(HistoryResetReason::InputNotReady);

        // RR-22: how many frames of this run had the Unreal plugin's signature (reset by any frame without it).
        _nullCameraKeyFrames = _cameraNullKeysThisFrame ? _nullCameraKeyFrames + 1 : 0;

        constexpr uint32_t kMissingInputFramesBeforeFallback = 30;
        if (++_missingInputFrames >= kMissingInputFramesBeforeFallback && !_requestedFsrFallback)
        {
            _requestedFsrFallback = true;
            // SAT-P0-5 (AMDNR 0.3.4): the title did create a Ray Reconstruction feature (Satisfactory, UE5); what it
            // does not pass is something FSR Ray Regeneration needs and NVIDIA's RR may not.
            LOG_ERROR("FSR-RR: the title's Ray Reconstruction inputs could not be gathered on {} consecutive "
                      "frames (see the [RR_INPUT] and [RR_CAM] lines above for which). The title created a Ray "
                      "Reconstruction feature but does not pass everything FSR Ray Regeneration needs; switching "
                      "this handle to FSR.",
                      _missingInputFrames);
            state.newBackend = Upscaler::FFX;
            state.changeBackend[Handle()->Id] = true;
            state.rrFallbackToSr[Handle()->Id] = true;
            state.rrFallbackReason = _denoiserBlocker.empty()
                                         ? std::string("its Ray Reconstruction inputs could not be gathered "
                                                       "(the [RR_INPUT] lines in OptiScaler.log say which)")
                                         : _denoiserBlocker;
            s_rrFallbackReasonHandle.store(Handle()->Id, std::memory_order_relaxed);
            LOG_ERROR("FSR-RR: reason: {}", state.rrFallbackReason);
            LOG_INFO("FSR-RR: handle {} is a Super Resolution handle from here on for the neural pass too (pre-SR "
                     "placement, as on any SR title)", Handle()->Id);

            // SAT-P0-7 (AMDNR 0.3.4): the FSR feature the fallback asks for is built from this handle's NGX table,
            // so it inherits the Ray Reconstruction feature's DepthInverted (IFeature::SetInitParameters: the user's
            // [InitFlags] DepthInverted, else the DLSSD create flags). Satisfactory's DLSSD says false while its SR
            // handle says true. Log only: the fix waits for the depth probe (SAT-P0-4) and a player log.
            if (FirstRRLineForHandle(Handle()->Id, RRHandleLine::FallbackDepth))
            {
                ID3D12Resource* rrDepth = nullptr; // this frame's, from the table (not a pointer an earlier frame left)
                TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_Depth, rrDepth);
                LOG_INFO("FSR-RR: the fallback FSR on handle {} inherits DepthInverted={} (source: {}); the Ray "
                         "Reconstruction depth is {} kind={}, declared {} (DLSS.Use.HW.Depth)",
                         Handle()->Id, DepthInverted() ? 1 : 0,
                         cfg.DepthInverted.has_value() ? "user [InitFlags] DepthInverted" : "the DLSSD create flags",
                         DescribeRRTexture(rrDepth), DepthResourceKindName(ClassifyDepthResource(rrDepth)),
                         _hasNGXDepthType ? (_ngxReportedHWDepth ? "hw" : "linear") : "absent");
            }

            // RR-22 (D29): the session latch, only on the Unreal plugin's signature (see kRrFallbackSessionLatch).
            if (kRrFallbackSessionLatch)
            {
                const bool unrealTitle = state.NVNGX_Engine == NVSDK_NGX_ENGINE_TYPE_UNREAL ||
                                         state.gameEngine == GameEngineType::Unreal ||
                                         (state.gameQuirks & GameQuirk::ForceUnrealEngine);
                const bool everyFrameNullKeys = _nullCameraKeyFrames == _missingInputFrames;
                const bool ngxDirectSL = cfg.FfxDenoiserNgxDirectSLConstants.value_or_default();
                if (everyFrameNullKeys && unrealTitle && !ngxDirectSL)
                {
                    FSRD::RuntimeStatus::RayRegenOffForSession.store(true, std::memory_order_relaxed);
                    LOG_WARN("FSR-RR: Ray Regeneration stays off for this session (RR-22): on all {} frames the game's "
                             "camera matrix keys were present but null with no Streamline camera (the stock Unreal "
                             "DLSS plugin), which no later frame or re-create changes. Later Ray Reconstruction "
                             "features and menu re-picks get FSR at once.",
                             _missingInputFrames);
                }
                else
                {
                    LOG_INFO("FSR-RR: no session latch (RR-22): {}; a later Ray Reconstruction feature retries Ray "
                             "Regeneration",
                             ngxDirectSL ? std::string("[FSR-RR] FfxDenoiserNgxDirectSLConstants=true")
                             : !everyFrameNullKeys
                                 ? std::format("the camera matrix keys were present but null on {} of the {} frames",
                                               _nullCameraKeyFrames, _missingInputFrames)
                                 : std::string("not an Unreal title"));
                }
            }
        }
    }
    else
    {
        _missingInputFrames = 0;
        _nullCameraKeyFrames = 0;
    }

    // SAT-P0-3: the second and last [RR_CAM] streamline line of this handle - at the fallback (Streamline constants
    // may start a few frames after the first evaluate), or after 300 evaluates on a handle that keeps RR.
    if (!_loggedStreamlineLater && (_requestedFsrFallback || _frameCount >= 300))
    {
        _loggedStreamlineLater = true;
        if (FirstRRLineForHandle(Handle()->Id, RRHandleLine::StreamlineLater))
            LogRRStreamlineState(inParams, _requestedFsrFallback ? "at the fallback" : "after 300 evaluates");
    }

    // Dispatch denoiser
    if (!isDenoiseBypassed && denoiserInputsReady)
    {
        ffxDispatchDescDenoiserDebugView dispatchDebugView = {};

        if (isFfxDebug)
        {
            if (!(_denoiserCtxDesc.flags & FFX_DENOISER_ENABLE_DEBUGGING))
            {
                LOG_ERROR("RR debug view requested, but this denoiser context was not created with debugging enabled");
                InvalidateDenoiserHistory(HistoryResetReason::DebugView);
                return false;
            }

            ID3D12Resource* debugOutput = FSRDConvShader->PrepareDebugViewOutput(
                InCommandList, TargetWidth(), TargetHeight());
            if (!debugOutput)
            {
                InvalidateDenoiserHistory(HistoryResetReason::DebugView);
                return false;
            }

            const D3D12_RESOURCE_DESC debugOutputDesc = debugOutput->GetDesc();
            const bool isCompatibleDebugOutput =
                debugOutputDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
                debugOutputDesc.Width == TargetWidth() &&
                debugOutputDesc.Height == TargetHeight() &&
                debugOutputDesc.DepthOrArraySize == 1 &&
                debugOutputDesc.MipLevels == 1 &&
                debugOutputDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT &&
                debugOutputDesc.SampleDesc.Count == 1 &&
                (debugOutputDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
            if (!isCompatibleDebugOutput)
            {
                LOG_ERROR(
                    "[RR_DIAG] dedicated AMD debug-view target is incompatible: size={}x{}, "
                    "format={}, dimension={}, arraySize={}, mips={}, samples={}, flags={:#x}",
                    debugOutputDesc.Width, debugOutputDesc.Height,
                    static_cast<uint32_t>(debugOutputDesc.Format),
                    static_cast<uint32_t>(debugOutputDesc.Dimension),
                    debugOutputDesc.DepthOrArraySize, debugOutputDesc.MipLevels,
                    debugOutputDesc.SampleDesc.Count, static_cast<uint32_t>(debugOutputDesc.Flags));
                InvalidateDenoiserHistory(HistoryResetReason::DebugView);
                return false;
            }

            const int debugViewport =
                std::clamp(cfg.FfxDenoiserDebugViewport.value_or_default(), -1,
                           FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS - 1);
            dispatchDebugView = 
            { 
                .header = { .type = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DEBUG_VIEW },
                .output = ffxApiGetResourceDX12(debugOutput, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS),
                .outputSize = { static_cast<uint32_t>(debugOutputDesc.Width), debugOutputDesc.Height },
                .mode = static_cast<uint32_t>(debugViewport < 0
                    ? FFX_API_DENOISER_DEBUG_VIEW_MODE_OVERVIEW
                    : FFX_API_DENOISER_DEBUG_VIEW_MODE_FULLSCREEN_VIEWPORT),
                .viewportIndex = static_cast<uint32_t>(std::max(debugViewport, 0))
            };

            // Debug view is optional and must be the tail of the typed-signal chain.
            ffxDispatchDescHeader* chainTail = denoiserDesc.header.pNext;
            while (chainTail && chainTail->pNext)
                chainTail = chainTail->pNext;

            if (!chainTail)
            {
                LOG_ERROR("RR 1.2 debug view could not find the typed-signal chain tail");
                InvalidateDenoiserHistory(HistoryResetReason::DebugView);
                return false;
            }

            chainTail->pNext = &dispatchDebugView.header;
        }

        isDenoiserReady = DispatchDenoiser(InCommandList, denoiserDesc);

        if (isFfxDebug)
            FSRDConvShader->TransitionDebugViewOutputToRead(InCommandList);

        if (!isDenoiserReady)
        {
            InvalidateDenoiserHistory();
            return false;
        }

        if (!_loggedRRGuidesDispatch)
        {
            _loggedRRGuidesDispatch = true;
            LogRRGuides(inParams, "first-dispatch");
        }
        // The Neural tab shows its Ray Regeneration controls only while this is recent (the game
        // has Ray Reconstruction on and FSR Ray Regeneration is really denoising).
        State::Instance().fsrRrLastDispatchMs = GetTickCount64();

        if (!PublishAmbientOcclusionOutput(InCommandList))
        {
            InvalidateDenoiserHistory();
            return false;
        }

        CommitDenoiserHistory();

        // Compose denoised signals
        uint32_t compositionFlags = (uint32_t)GetCompDebugFlags(dbgMode);
        if (_diffuseSignalDescType == FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE)
            compositionFlags |= (uint32_t)FSRDCompFlags::DiffuseSignalIndirect;
        if (_specularSignalDescType == FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR)
            compositionFlags |= (uint32_t) FSRDCompFlags::SpecularSignalIndirect;
        // A signal left out of the denoiser chain has no output this frame, and the
        // buffer it would have been written to is one of the floor passes' ping-pong
        // targets. Flag it so composition reads the raw signal rather than the floor
        // image that buffer still holds.
        if (!_denoiseDiffuse)
            compositionFlags |= (uint32_t)FSRDCompFlags::DiffuseSignalDisabled;
        if (!_denoiseSpecular)
            compositionFlags |= (uint32_t)FSRDCompFlags::SpecularSignalDisabled;

        FSRDCompDesc compDesc =
        { 
            .DstTexSize = _convDesc.RenderSize,
            .CorrelationBias = std::clamp(cfg.FfxDenoiserCorrelationBias.value_or_default(), 0.0f, 1.0f),
            .Flags = compositionFlags,
            .FloorHandoverAnchorClamp =
                std::clamp(cfg.FfxDenoiserFloorHandoverAnchorClamp.value_or_default(), 0.0f, 4.0f),
            .FloorHandoverCorrelationMix =
                std::clamp(cfg.FfxDenoiserFloorHandoverCorrelationMix.value_or_default(), 0.0f, 1.0f)
        };

        const XMUINT2 rawColorBase = GetSubrectBase(
            inParams, NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X,
            NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y);
        compDesc.SourceBase = {
            rawColorBase.x, rawColorBase.y,
            0, 0
        };

        if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Color, compDesc.InRawColor) ||
            !ValidateSourceExtent("CompositionColor", compDesc.InRawColor, rawColorBase,
                                  RenderWidth(), RenderHeight()))
        {
            InvalidateDenoiserHistory(HistoryResetReason::InputNotReady);
            return false;
        }

        // ColorBeforeParticles is a whole scene guide, not a premultiplied overlay.
        // It is deliberately absent from composition; the title's Color input already
        // contains the scene contribution that reaches the final frame.

        if (!isFfxDebug)
        {
            if (!FSRDConvShader->DispatchComposition(InCommandList, compDesc))
                return false;

            // Skin smoothing (AMDNR, experimental) and the SSS-guide range, on the finished picture
            // only: a composition debug view is shown as composed.
            if (!isDebugComp)
                skinSmoothedColor = ApplySkinSmoothing(InCommandList, inParams, compDesc.InRawColor, rawColorBase);
        }

        isDenoiserReady = true;
    }
    else
    {
        // A skipped RR frame breaks temporal continuity. The next real dispatch
        // must reset instead of reusing history across the gap.
        InvalidateDenoiserHistory(isDenoiseBypassed ? HistoryResetReason::DebugView
                                                    : HistoryResetReason::InputNotReady);

        // A debug view that bypasses the denoiser is still Ray Regeneration at work in a Ray
        // Reconstruction title, and the overlay's Ray Regeneration block - where the view is chosen
        // and switched back off - is shown only while this stamp is recent.
        if (isDenoiseBypassed && denoiserInputsReady)
            State::Instance().fsrRrLastDispatchMs = GetTickCount64();
    }

    // What happens to RR's output after it, now that this frame's choices are all made: on the first RR frame,
    // and again (RR-14) when what it reports changed and held still for kRRPostSettleFrames RR frames.
    if (isDenoiserReady)
    {
        const RRPostState postState =
            CaptureRRPostState(upscalerDesc, profileSuppressedSharpness, skinSmoothedColor != nullptr);

        if (!_loggedRRPost)
        {
            _loggedRRPost = true;
            _loggedRRPostState = postState;
            _pendingRRPostState = postState;
            // (AMDNR 0.3.4.1, Proton RR) Once per feature, and only while the default is in use: under Wine/Proton
            // a title that sends no sharpness gets none after RR (DefaultSharpnessWhenTitleSendsNone is 0 there).
            // Without the override, the value Evaluate settled on this frame (_actualSharpness, kept before any RCAS
            // hand-over zeroes _sharpness) is then the title's own, so "not > 0" = the title sends none.
            if (State::Instance().isRunningOnLinux && !cfg.OverrideSharpness.value_or_default() &&
                !(_actualSharpness.value_or(_sharpness) > 0.0f))
                LOG_INFO("[RR_POST] Wine/Proton: no default RR sharpening (set Image > Sharpness > Override to "
                         "sharpen)");
            LogRRPost(inParams, upscalerDesc, profileSuppressedSharpness, titleSharpness,
                      skinSmoothedColor != nullptr, "first RR frame");
        }
        else if (!(postState == _pendingRRPostState))
        {
            _pendingRRPostState = postState;
            _pendingRRPostFrames = 0;
        }
        else if (!(postState == _loggedRRPostState) && ++_pendingRRPostFrames >= kRRPostSettleFrames)
        {
            _loggedRRPostState = postState;
            LogRRPost(inParams, upscalerDesc, profileSuppressedSharpness, titleSharpness,
                      skinSmoothedColor != nullptr, "changed");
        }
    }

    // Upscaler start. Stays true on the debug/bypass paths where no upscale is requested.
    bool isUpscalerReady = true;

    if (!isUpscaleBypassed)
    {
        // Override upscaler config. The composition output is left in
        // NON_PIXEL | PIXEL shader-resource state, so declare both: the default
        // argument is COMPUTE_READ alone, which would leave the resource in a
        // narrower state than the preprocessor expects on the next frame. The skin
        // smoothing output, when that pass ran, is left in the same state.
        if (isDenoiserReady)
            upscalerDesc.color = ffxApiGetResourceDX12(
                skinSmoothedColor != nullptr ? skinSmoothedColor : FSRDConvShader->GetCompositionOutput(),
                FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        isUpscalerReady = DispatchUpscaler(InCommandList, upscalerDesc);

        // Post-processing (RCAS/output scaling/overlay) is run by IFeature_Dx12::Evaluate.
    }
    else // Debug visualization
    {
        ID3D12Resource* srcTex = nullptr;
        XMUINT2 debugSourceBase {};
        XMFLOAT2 debugSourceLogicalSize {
            static_cast<float>(RenderWidth()), static_cast<float>(RenderHeight())
        };

        if (isFfxDebug)
        {
            srcTex = FSRDConvShader->GetDebugViewOutput();
            debugSourceLogicalSize = {
                static_cast<float>(TargetWidth()), static_cast<float>(TargetHeight())
            };
        }
        else if (dbgMode == DebugModes::DlssColorBeforeParticles)
        {
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles, srcTex);
            debugSourceBase = GetSubrectBase(
                inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles_Subrect_Base_X,
                NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles_Subrect_Base_Y);
        }
        else if (dbgMode == DebugModes::DlssColorBeforeTransparency)
        {
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency, srcTex);
            debugSourceBase = GetSubrectBase(
                inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency_Subrect_Base_X,
                NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency_Subrect_Base_Y);
        }
        else if (dbgMode == DebugModes::DlssTransparencyLayer)
        {
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSS_TransparencyLayer, srcTex);
            debugSourceBase = GetSubrectBase(
                inParams, NVSDK_NGX_Parameter_DLSS_TransparencyLayer_Subrect_Base_X,
                NVSDK_NGX_Parameter_DLSS_TransparencyLayer_Subrect_Base_Y);
        }
        else if (dbgMode == DebugModes::DlssBias)
        {
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, srcTex);
            debugSourceBase = GetSubrectBase(
                inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_X,
                NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_Y);
        }
        else if (dbgMode == DebugModes::RawColor)
        {
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_Color, srcTex);
            debugSourceBase = GetSubrectBase(
                inParams, NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X,
                NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y);
        }
        else if (dbgMode == DebugModes::AmbientOcclusionInput)
            srcTex = _ambientOcclusionEnabled ? _ambientOcclusionNoisy.Get() : nullptr;
        else if (dbgMode == DebugModes::AmbientOcclusionOutput)
            srcTex = _ambientOcclusionEnabled ? FSRDConvShader->GetAmbientOcclusionOutput() : nullptr;
        else if (isDebugVis)
            srcTex = GetD3D12ResFromFFX(indirectSpecular.signal.input);
        else if (skinSmoothedColor != nullptr) // UpscalerBypass: what FSR SR would have read
            srcTex = skinSmoothedColor;
        else
            srcTex = FSRDConvShader->GetCompositionOutput();

        ID3D12Resource* dstTex;

        if (!srcTex || !TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Output, dstTex))
            return true;

        FSRDConvShader->Blit(
            InCommandList, srcTex, dstTex, {}, debugSourceLogicalSize,
            { static_cast<float>(debugSourceBase.x),
              static_cast<float>(debugSourceBase.y) });
    }

    // A failed upscale dispatch leaves the frame half finished. Report it to the caller
    // instead of masking it behind the denoiser result, which may well be true.
    if (!isUpscalerReady)
        return false;

    // A DENOISE SKIPPED FOR MISSING INPUTS IS NOT A FAILED FRAME. The branch above wrote a
    // complete, upscaled picture from the raw colour, and the title is told so. This used
    // to return false there, which the NGX layer turned into NVSDK_NGX_Result_Fail - and
    // Satisfactory (UE5, RX 9060 XT) died on the very first one: its DLSS plugin publishes
    // no camera matrices ("WorldToViewMatrix" / "ViewToClipMatrix" absent from the
    // parameter table) and the Streamline constants fallback had nothing either, so
    // ResolveCameraMatrices said "Denoiser not ready", the frame was upscaled anyway, the
    // evaluate reported failure, and the log ended one line later. The 30-frame switch to
    // plain FSR above still runs, and the [RR_INPUT] lines still say why.
    return isDenoiserReady || isDenoiseBypassed || !denoiserInputsReady;
}

bool FSRDFeatureDx12::PrepareDenoiserInput(ID3D12GraphicsCommandList* InCommandList, const NVSDK_NGX_Parameter& inParams,
    ffxDispatchDescDenoiser& dispatchDesc, ffxDispatchDescDenoiserAmbientOcclusion& ambientOcclusion,
    ffxDispatchDescDenoiserDirectDiffuse& directDiffuse,
    ffxDispatchDescDenoiserIndirectSpecular& indirectSpecular)
{
    const auto& cfg = *Config::Instance(); 

    if (_ambientOcclusionEnabled && !AcquireTaggedAmbientOcclusionResources(true))
    {
        static RRRepeatedErrorGate aoPairGate;
        if (const uint64_t n = aoPairGate.Next())
            LOG_ERROR("[RR_AO] context requires AO, but a valid tagged pair was unavailable for this frame ({})",
                      RRRepeatNote(n));
        return false;
    }

    // Gather DLSS-RR input buffers for conversion and repacking for FSR-RR
    if (!PrepareDenoiseConvInput(inParams))
    {
        // SAT-P0-4 (AMDNR 0.3.4): the depth / normals content probe, only on a frame whose camera is missing.
        if (_cameraBlockedThisFrame)
            RecordContentProbe(InCommandList, inParams);
        return false;
    }

    if (!ConvertDenoiserBuffers(InCommandList))
        return false;

    // Camera matrix - translation and rotation, from viewMatrix^-1
    const XMFLOAT3 camPos = GetFloat3Column(_invViewMatrix, 3);
    const bool resetHistory = _isInReset || !_hasDenoiserHistory;
    const XMFLOAT3 camDelta = resetHistory
        ? XMFLOAT3 {}
        : XMFLOAT3 { _lastCamPos.x - camPos.x, _lastCamPos.y - camPos.y, _lastCamPos.z - camPos.z };

    // Pack dispatch configuration
    dispatchDesc = 
    {
        .commandList = InCommandList,
        .motionVectorScale = { 1.0f, 1.0f, 1.0f },
        // Camera movement since last frame (PreviousPosition - CurrentPosition)
        .cameraPositionDelta = { camDelta.x, camDelta.y, camDelta.z },
        .view = GetRRMatrix(_viewMatrix),
        .projection = GetRRMatrix(_projMatrix),
        .linearDepthBounds = { _convDesc.NearPlane, _convDesc.FarPlane },
        .renderSize = { RenderWidth(), RenderHeight() }, 
        .frameIndex = (uint32_t)_frameCount,
        .flags = FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO
    };

    // Populate resources and link signal header
    FSRDConvShader->GetSignals(dispatchDesc, directDiffuse, indirectSpecular);
    directDiffuse.header.type = _diffuseSignalDescType;
    indirectSpecular.header.type = _specularSignalDescType;

    const ffxDispatchDescDenoiserAmbientOcclusion* activeAmbientOcclusion = nullptr;
    if (_ambientOcclusionEnabled)
    {
        // Acquisition only guarantees NON_PIXEL_SHADER_RESOURCE. Declaring the wider
        // PIXEL_COMPUTE_READ when the title tagged a narrower state makes the FFX
        // backend record a transition whose "before" state the resource is not in.
        // Report the state that was actually validated.
        const uint32_t ambientOcclusionInputState =
            (_ambientOcclusionNoisyState & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) != 0
                ? FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ
                : FFX_API_RESOURCE_STATE_COMPUTE_READ;

        ambientOcclusion =
        {
            .header = { .type = FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION },
            .signal =
            {
                .input = ffxApiGetResourceDX12(
                    _ambientOcclusionNoisy.Get(), ambientOcclusionInputState),
                .output = ffxApiGetResourceDX12(
                    FSRDConvShader->GetAmbientOcclusionOutput(),
                    FFX_API_RESOURCE_STATE_UNORDERED_ACCESS),
                .checkerboardOrigin = 0
            }
        };
        activeAmbientOcclusion = &ambientOcclusion;
    }

    // RR 1.2 descriptors are ordered by their ABI type, matching AMD's sample chain.
    // Keeping this generic prevents future AO/SO additions from hard-coding pairwise links.
    // Single-signal mode (DenoiseDiffuse/DenoiseSpecular): disabled signals are not
    // linked into the chain at all, so the denoiser only sees the enabled ones.
    std::array<ffxDispatchDescHeader*, 3> signals { nullptr, nullptr, nullptr };
    size_t signalCount = 0;
    if (_denoiseDiffuse)
        signals[signalCount++] = &directDiffuse.header;
    if (_denoiseSpecular)
        signals[signalCount++] = &indirectSpecular.header;
    if (activeAmbientOcclusion)
        signals[signalCount++] = &ambientOcclusion.header;

    std::sort(signals.begin(), signals.begin() + signalCount,
              [](const ffxDispatchDescHeader* left, const ffxDispatchDescHeader* right) {
                  return left->type < right->type;
              });

    dispatchDesc.header.pNext = signalCount > 0 ? signals[0] : nullptr;
    for (size_t i = 0; i < signalCount; ++i)
        signals[i]->pNext = i + 1 < signalCount ? signals[i + 1] : nullptr;

    if (!ValidateRequiredRRResources(
            dispatchDesc, directDiffuse, indirectSpecular, activeAmbientOcclusion,
            FSRDConvShader ? FSRDConvShader->GetAlbedoFormat() : DXGI_FORMAT_R8G8B8A8_UNORM))
        return false;
    
    if (resetHistory)
        dispatchDesc.flags |= FFX_DENOISER_DISPATCH_RESET;

    // Conversion has already normalized source motion into unjittered UV space.
    // RR therefore consumes XY with a unit scale; Z remains signed-linear depth.
    dispatchDesc.motionVectorScale = { 1.0f, 1.0f, 1.0f };
    dispatchDesc.jitterOffsets.x = _convDesc.JitterOffsets.x;
    dispatchDesc.jitterOffsets.y = _convDesc.JitterOffsets.y;

    LOG_DEBUG("Jitter pixels [{:.6f}, {:.6f}]", dispatchDesc.jitterOffsets.x, dispatchDesc.jitterOffsets.y);

    return true;
}

// Picks the specular ray length RR will read, from the title's candidates.
//
// Selection is deferred until every source origin and extent is known, so an
// invalid high-priority candidate can never reach an SRV binding.
void FSRDFeatureDx12::ResolveSpecularHitDistance(
    const NVSDK_NGX_Parameter& inParams, const RRD3D12SignalTagSnapshot& rrTagSnapshot,
    uint32_t renderWidth, uint32_t renderHeight, uint32_t motionWidth, uint32_t motionHeight,
    ID3D12Resource* ngxSpecularHitDistance,
    ID3D12Resource* ngxSpecularRayDirectionHitDistance,
    const XMUINT2& ngxSpecularHitDistanceBase,
    const XMUINT2& ngxSpecularRayDirectionHitDistanceBase)
{
    struct SpecularHitDistanceSelection
    {
        ID3D12Resource* Resource = nullptr;
        XMUINT2 Base {};
        bool CombinedAlpha = false;
        const char* SourceName = nullptr;
    } specularHitDistanceSelection;

    auto tryNGXSpecularHitDistance = [&](const char* sourceName,
                                         ID3D12Resource* resource,
                                         const XMUINT2& base,
                                         bool combinedAlpha) -> bool {
        if (!resource)
            return false;

        const SourceFormatValidator formatValidator = combinedAlpha
            ? IsDiffuseRayDirectionHitDistanceFormat
            : IsDiffuseHitDistanceFormat;
        const char* expectedFormat = combinedAlpha
            ? "RGBA16F or RGBA32F (hit distance in alpha)"
            : "R16F or R32F";
        if (!ValidateReprojectionGuideSource(
                sourceName, resource, base, renderWidth, renderHeight,
                formatValidator, expectedFormat))
        {
            return false;
        }

        specularHitDistanceSelection = {
            resource, base, combinedAlpha, sourceName
        };
        return true;
    };

    auto trySLSpecularHitDistance = [&](RRTaggedSignal signal,
                                        const char* sourceName,
                                        bool combinedAlpha) -> bool {
        Microsoft::WRL::ComPtr<ID3D12Resource> taggedResource;
        RRTaggedResourceDiagnostic diagnostic {};
        if (!AcquireSLTaggedResource(
                rrTagSnapshot, signal, sourceName,
                TagStatePolicy::RequireShaderRead,
                taggedResource, diagnostic))
            return false;

        const XMUINT2 base = diagnostic.usesExtent
            ? XMUINT2 { diagnostic.extentLeft, diagnostic.extentTop }
            : XMUINT2 {};
        if (diagnostic.effectiveWidth != renderWidth ||
            diagnostic.effectiveHeight != renderHeight)
        {
            static RRRepeatedErrorGate specularTagExtentGate;
            if (const uint64_t n = specularTagExtentGate.Next())
                LOG_ERROR("[RR_INPUT] {} tag extent {}x{} does not exactly match the one-to-one render extent {}x{} "
                          "({})",
                          sourceName, diagnostic.effectiveWidth, diagnostic.effectiveHeight, renderWidth,
                          renderHeight, RRRepeatNote(n));
            return false;
        }

        const SourceFormatValidator formatValidator = combinedAlpha
            ? IsDiffuseRayDirectionHitDistanceFormat
            : IsDiffuseHitDistanceFormat;
        const char* expectedFormat = combinedAlpha
            ? "RGBA16F or RGBA32F (hit distance in alpha)"
            : "R16F or R32F";
        if (!ValidateReprojectionGuideSource(
                sourceName, taggedResource.Get(), base,
                diagnostic.effectiveWidth, diagnostic.effectiveHeight,
                formatValidator, expectedFormat))
        {
            return false;
        }

        specularHitDistanceSelection = {
            taggedResource.Get(), base, combinedAlpha, sourceName
        };
        if (combinedAlpha)
            _specularRayDirectionHitDistanceTaggedResource =
                std::move(taggedResource);
        else
            _specularHitDistanceTaggedResource = std::move(taggedResource);
        return true;
    };

    if (!tryNGXSpecularHitDistance(
            NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance,
            ngxSpecularHitDistance, ngxSpecularHitDistanceBase, false) &&
        !tryNGXSpecularHitDistance(
            NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance,
            ngxSpecularRayDirectionHitDistance,
            ngxSpecularRayDirectionHitDistanceBase, true) &&
        !trySLSpecularHitDistance(
            RRTaggedSignal::SpecularHitDistance,
            "Streamline.SpecularHitDistance", false))
    {
        trySLSpecularHitDistance(
            RRTaggedSignal::SpecularRayDirectionHitDistance,
            "Streamline.SpecularRayDirectionHitDistance", true);
    }

    if (specularHitDistanceSelection.Resource)
    {
        _convDesc.SpecularHitDistanceBase = specularHitDistanceSelection.Base;
        _convDesc.SpecularHitDistanceFromCombinedAlpha =
            specularHitDistanceSelection.CombinedAlpha;
        if (specularHitDistanceSelection.CombinedAlpha)
        {
            _convDesc.Resources.InSpecularRayDirectionHitDistance =
                specularHitDistanceSelection.Resource;
        }
        else
        {
            _convDesc.Resources.InSpecHitDist =
                specularHitDistanceSelection.Resource;
            // The legacy signal splitter consumes scalar hit distance directly.
            _convDesc.InputBase2.x = specularHitDistanceSelection.Base.x;
            _convDesc.InputBase2.y = specularHitDistanceSelection.Base.y;
        }

    }

    // RR-09: one line on the first evaluate and on a settled change of the selection (was a per-frame debug line).
    SpecularHitSelection selection {};
    if (specularHitDistanceSelection.Resource)
    {
        const D3D12_RESOURCE_DESC desc = specularHitDistanceSelection.Resource->GetDesc();
        selection.source = specularHitDistanceSelection.SourceName;
        selection.baseX = specularHitDistanceSelection.Base.x;
        selection.baseY = specularHitDistanceSelection.Base.y;
        selection.width = desc.Width;
        selection.height = desc.Height;
        selection.format = desc.Format;
        selection.combinedAlpha = specularHitDistanceSelection.CombinedAlpha;
    }

    bool logSelection = false;
    if (!_hasLoggedSpecularHit)
    {
        logSelection = true;
        _pendingSpecularHit = selection;
    }
    else if (!(selection == _pendingSpecularHit))
    {
        _pendingSpecularHit = selection;
        _pendingSpecularHitFrames = 0;
    }
    else if (!(selection == _loggedSpecularHit) && ++_pendingSpecularHitFrames >= kSpecularHitSettleFrames)
    {
        logSelection = true;
    }

    if (logSelection)
    {
        _hasLoggedSpecularHit = true;
        _loggedSpecularHit = selection;

        if (selection.source)
        {
            const auto formatName = magic_enum::enum_name(selection.format);
            LOG_INFO("[RR_INPUT] selected {} for specular hit distance: resource {}x{} {}, base=({}, {}), "
                     "render extent {}x{}, combinedAlpha={}",
                     selection.source, selection.width, selection.height,
                     formatName.empty() ? "UNKNOWN" : formatName, selection.baseX, selection.baseY, renderWidth,
                     renderHeight, selection.combinedAlpha);
        }
        else
        {
            LOG_INFO("[RR_INPUT] no specular hit distance selected (none published, or none passed the checks "
                     "above); render extent {}x{}",
                     renderWidth, renderHeight);
        }
    }
}

// Binds the resources a title may publish beyond the required set.
//
// Each is validated before use: presence says nothing about usability, and a
// sentinel-filled resource passes every presence check there is.
void FSRDFeatureDx12::AcquireOptionalInputs(const NVSDK_NGX_Parameter& inParams,
    const RRD3D12SignalTagSnapshot& rrTagSnapshot, uint32_t renderWidth,
    uint32_t renderHeight)
{
    const auto& cfg = *Config::Instance();

    // Optional title-published inputs. Each is validated before use: presence says nothing
    // about usability, and a sentinel-filled resource passes every presence check there is.
    _convDesc.Resources.InTitleLinearDepth = nullptr;
    _convDesc.Resources.InResponsivityMask = nullptr;
    _convDesc.TitleLinearDepthBase = {};
    _convDesc.TitleLinearDepthState = 0;
    _titleLinearDepthTaggedResource.Reset();
    _responsivityMaskTaggedResource.Reset();

    // The title's own linear depth. Opt-in: every consumer of view-space position switches
    // to it at once, so it is not a change to make by default before it has been compared
    // against the derived field.
    if (cfg.FfxDenoiserUseTitleLinearDepth.value_or_default())
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> taggedLinearDepth;
        RRTaggedResourceDiagnostic linearDepthDiagnostic {};

        // Titles that tag a resource without committing to a state declare COMMON, which
        // carries no shader-readable bit but is legal to transition from - the conversion
        // records the barrier out of the declared state and hands the resource back. The
        // transition allowance is a state policy on the acquisition itself, so this tag
        // clears the same frame/viewport/lifetime checks as every other signal: a legacy
        // tag whose Present/Evaluate lifetime may have expired is refused here like
        // anywhere else, no matter that the snapshot still holds the object alive.
        ID3D12Resource* linearDepthCandidate = nullptr;
        if (AcquireSLTaggedResource(rrTagSnapshot, RRTaggedSignal::LinearDepth,
                                    "Streamline.LinearDepth",
                                    TagStatePolicy::AllowCommonTransition,
                                    taggedLinearDepth, linearDepthDiagnostic))
        {
            linearDepthCandidate = taggedLinearDepth.Get();
        }

        if (linearDepthCandidate != nullptr)
        {
            const XMUINT2 base = linearDepthDiagnostic.usesExtent
                ? XMUINT2 { linearDepthDiagnostic.extentLeft, linearDepthDiagnostic.extentTop }
                : XMUINT2 {};
            const D3D12_RESOURCE_DESC linearDepthDesc = linearDepthCandidate->GetDesc();
            const DXGI_FORMAT linearDepthViewFormat = FSRD::GetViewFormat(linearDepthDesc.Format);

            if ((linearDepthViewFormat == DXGI_FORMAT_R32_FLOAT ||
                 linearDepthViewFormat == DXGI_FORMAT_R16_FLOAT) &&
                ValidateSourceExtent("TitleLinearDepth", linearDepthCandidate, base,
                                     renderWidth, renderHeight))
            {
                _titleLinearDepthTaggedResource = std::move(taggedLinearDepth);
                _convDesc.Resources.InTitleLinearDepth = _titleLinearDepthTaggedResource.Get();
                _convDesc.TitleLinearDepthBase = base;
                // The conversion records the barrier out of this state and back, so the state
                // has to travel with the resource rather than be assumed.
                _convDesc.TitleLinearDepthDeclaredState = linearDepthDiagnostic.state;
                _convDesc.TitleLinearDepthState = FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ;

                static bool loggedTitleLinearDepth = false;
                if (!loggedTitleLinearDepth)
                {
                    loggedTitleLinearDepth = true;
                    LOG_INFO("[RR_INPUT] title linear depth bound: {}x{}, base=({}, {}), {} "
                             "(declared state {:#x}); view-space positions and the denoiser's "
                             "depth input will use it",
                             linearDepthDesc.Width, linearDepthDesc.Height, base.x, base.y,
                             magic_enum::enum_name(linearDepthViewFormat),
                             linearDepthDiagnostic.state);
                }
            }
            else
            {
                static bool loggedTitleLinearDepthRejection = false;
                if (!loggedTitleLinearDepthRejection)
                {
                    loggedTitleLinearDepthRejection = true;
                    LOG_WARN("[RR_INPUT] title linear depth present but unusable: {}x{} "
                             "(base ({}, {})), required {}x{}+({}, {})",
                             linearDepthDesc.Width, linearDepthDesc.Height, base.x, base.y,
                             renderWidth, renderHeight, base.x, base.y);
                }
            }
        }
        else
        {
            // A silent failure here is indistinguishable from the option never having been
            // enabled, which is exactly how a missing input is misread as a working one.
            static bool loggedTitleLinearDepthUnavailable = false;
            if (!loggedTitleLinearDepthUnavailable)
            {
                loggedTitleLinearDepthUnavailable = true;
                const size_t index = static_cast<size_t>(RRTaggedSignal::LinearDepth);
                if (index < rrTagSnapshot.resources.size())
                {
                    const RRTaggedResourceDiagnostic& diagnostic =
                        rrTagSnapshot.resources[index].diagnostic;
                    LOG_WARN("[RR_INPUT] title linear depth is enabled but its tag could not be "
                             "acquired: observed={}, present={}, lifecycle={}, tagFrame={}, "
                             "tagViewport={}, activeFrame={}, activeViewport={}, "
                             "effectiveExtent={}x{}, declaredState={:#x}",
                             diagnostic.observed, diagnostic.present,
                             magic_enum::enum_name(diagnostic.lifecycle),
                             diagnostic.frameIndex, diagnostic.viewport,
                             rrTagSnapshot.activeEvaluationFrame,
                             rrTagSnapshot.activeEvaluationViewport,
                             diagnostic.effectiveWidth, diagnostic.effectiveHeight,
                             diagnostic.state);
                }
                else
                {
                    LOG_WARN("[RR_INPUT] title linear depth is enabled but the tag snapshot has no "
                             "slot for it ({} slots)", rrTagSnapshot.resources.size());
                }
            }
        }
    }

    // The title's responsivity hint. It is published under the title's own NGX key rather
    // than an NVSDK constant, and its polarity is the title's own convention, so only the
    // read is wired here and which side means "unstable" stays a setting.
    if (cfg.FfxDenoiserResponsivityThreshold.value_or_default() > 0.0f)
    {
        ID3D12Resource* responsivityMask = nullptr;
        TryGetNGXVoidPointer(inParams, "DLSSD.ResponsivityMask", responsivityMask);

        if (responsivityMask != nullptr &&
            ValidateReprojectionGuideSource(
                "ResponsivityMask", responsivityMask, { 0u, 0u }, renderWidth, renderHeight,
                IsResponsivityMaskFormat,
                "R8_UNORM, R16_FLOAT, R32_FLOAT, RGBA8_UNORM or RGBA16_FLOAT"))
        {
            const D3D12_RESOURCE_DESC responsivityDesc = responsivityMask->GetDesc();
            _convDesc.Resources.InResponsivityMask = responsivityMask;

            // This runs on every Evaluate, so the log is only re-emitted when the bound
            // resource or the polarity/threshold configuration actually changes; otherwise
            // a long session accumulates an identical line per frame. The cache belongs to
            // this feature instance so independent instances cannot race or suppress one
            // another's diagnostics.
            const DXGI_FORMAT responsivityViewFormat = FSRD::GetViewFormat(responsivityDesc.Format);
            const float responsivityThreshold = cfg.FfxDenoiserResponsivityThreshold.value_or_default();
            const bool responsivityInvert = cfg.FfxDenoiserResponsivityInvert.value_or_default();

            if (responsivityMask != _loggedResponsivityMask ||
                responsivityDesc.Width != _loggedResponsivityWidth ||
                responsivityDesc.Height != _loggedResponsivityHeight ||
                responsivityViewFormat != _loggedResponsivityViewFormat ||
                responsivityThreshold != _loggedResponsivityThreshold ||
                responsivityInvert != _loggedResponsivityInvert)
            {
                _loggedResponsivityMask = responsivityMask;
                _loggedResponsivityWidth = responsivityDesc.Width;
                _loggedResponsivityHeight = responsivityDesc.Height;
                _loggedResponsivityViewFormat = responsivityViewFormat;
                _loggedResponsivityThreshold = responsivityThreshold;
                _loggedResponsivityInvert = responsivityInvert;

                LOG_INFO("[RR_INPUT] responsivity hint bound: {}x{}, {}, threshold {:.4f} "
                         "({} counts as unstable)",
                         responsivityDesc.Width, responsivityDesc.Height,
                         magic_enum::enum_name(responsivityViewFormat),
                         responsivityThreshold,
                         responsivityInvert ? "above" : "below");
            }
        }
    }

    // Treat disappearance as a binding change, so the same resource is reported when it is
    // later published again instead of being suppressed by the last successful frame.
    if (_convDesc.Resources.InResponsivityMask == nullptr)
        _loggedResponsivityMask = nullptr;

}

// Binds the diffuse ray length, which RR's non-PSR handling of reflected geometry
// reads.
//
// Without it the diffuse signal declares every pixel a ray miss at infinity - the
// strongest possible claim, and almost always false when a title supplies one.
void FSRDFeatureDx12::ResolveDiffuseHitDistance(const NVSDK_NGX_Parameter& inParams,
    uint32_t renderWidth, uint32_t renderHeight)
{
    const auto& cfg = *Config::Instance();

    // Diagnostic-only probes for the two DLSS-RR diffuse hit-distance representations.
    // Do not bind or consume them until a title is confirmed to provide a compatible resource.
    _diffuseHitDistanceProbe = nullptr;
    _diffuseRayDirectionHitDistanceProbe = nullptr;
    _diffuseHitDistanceBaseX = 0;
    _diffuseHitDistanceBaseY = 0;
    _diffuseRayDirectionHitDistanceBaseX = 0;
    _diffuseRayDirectionHitDistanceBaseY = 0;
    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance,
                         _diffuseHitDistanceProbe);
    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance,
                         _diffuseRayDirectionHitDistanceProbe);
    inParams.Get(NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance_Subrect_Base_X,
                 &_diffuseHitDistanceBaseX);
    inParams.Get(NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance_Subrect_Base_Y,
                 &_diffuseHitDistanceBaseY);
    inParams.Get(NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance_Subrect_Base_X,
                 &_diffuseRayDirectionHitDistanceBaseX);
    inParams.Get(NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance_Subrect_Base_Y,
                 &_diffuseRayDirectionHitDistanceBaseY);

    // Bind the diffuse ray length instead of only probing it. RR's non-PSR handling
    // of reflected geometry is driven by hit distance, and the diffuse signal has
    // been declaring every pixel a ray miss at infinity - the strongest possible
    // claim, and almost always false when the title actually supplies a length.
    _convDesc.Resources.InDiffuseHitDistance = nullptr;
    _convDesc.DiffuseHitDistanceBase = {};
    _convDesc.DiffuseHitDistanceMode = 0;

    if (cfg.FfxDenoiserDiffuseHitDistance.value_or_default())
    {
        const XMUINT2 scalarBase {
            _diffuseHitDistanceBaseX, _diffuseHitDistanceBaseY
        };
        const XMUINT2 combinedBase {
            _diffuseRayDirectionHitDistanceBaseX, _diffuseRayDirectionHitDistanceBaseY
        };

        if (_diffuseHitDistanceProbe &&
            ValidateReprojectionGuideSource(
                NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance, _diffuseHitDistanceProbe,
                scalarBase, renderWidth, renderHeight, IsDiffuseHitDistanceFormat,
                "R16F or R32F"))
        {
            _convDesc.Resources.InDiffuseHitDistance = _diffuseHitDistanceProbe;
            _convDesc.DiffuseHitDistanceBase = scalarBase;
            _convDesc.DiffuseHitDistanceMode = 1;
        }
        else if (_diffuseRayDirectionHitDistanceProbe &&
                 ValidateReprojectionGuideSource(
                     NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance,
                     _diffuseRayDirectionHitDistanceProbe, combinedBase,
                     renderWidth, renderHeight, IsDiffuseRayDirectionHitDistanceFormat,
                     "RGBA16F or RGBA32F (hit distance in alpha)"))
        {
            _convDesc.Resources.InDiffuseHitDistance =
                _diffuseRayDirectionHitDistanceProbe;
            _convDesc.DiffuseHitDistanceBase = combinedBase;
            _convDesc.DiffuseHitDistanceMode = 2;
        }
    }

    // InputBase4.zw was reserved; it now carries the diffuse hit-distance origin.
    _convDesc.InputBase4.z = _convDesc.DiffuseHitDistanceBase.x;
    _convDesc.InputBase4.w = _convDesc.DiffuseHitDistanceBase.y;

}

// Resolves the view and projection the denoiser reprojects with.
//
// A published matrix is only usable if it inverts to a real camera position. A
// partially filled or sentinel-filled one passes a presence check and poisons
// every reconstructed position with NaN, which reaches the denoiser as
// cameraPositionDelta and reads there as a camera that never moved.
bool FSRDFeatureDx12::ResolveCameraMatrices(const NVSDK_NGX_Parameter& inParams,
    const sl::Constants& slData, bool hasCurrentSLConstants)
{
    bool isReady = true;

    // Get DLSSD matrices and derive related values
    // World to view/camera space (V)
    _viewMatrix = {};
    _viewFromStreamline = false;

    // A matrix is only usable if it inverts to a real camera position, whichever
    // source produced it. A partially filled or all-zero one passes a presence
    // check, and the NaN it produces then reaches the denoiser as
    // cameraPositionDelta - a value it uses for its own reprojection. The motion
    // vectors mask the same NaN behind the shader's isfinite guard, where it reads
    // as a zero depth delta, so "the camera never moved" and "the camera position
    // was NaN" are indistinguishable from the outside.
    const auto viewMatrixIsUsable = [](const XMMATRIX& view, const XMMATRIX& invView)
    {
        if (!MatrixIsFinite(view) || !MatrixIsFinite(invView))
            return false;

        const float determinant = XMVectorGetX(XMMatrixDeterminant(view));
        if (!std::isfinite(determinant) || determinant == 0.0f)
            return false;

        const XMFLOAT3 cameraPosition = GetFloat3Column(invView, 3);
        return std::isfinite(cameraPosition.x) && std::isfinite(cameraPosition.y) &&
               std::isfinite(cameraPosition.z);
    };

    // Builds the view matrix from the Streamline camera basis. Used both when NGX
    // publishes no matrix at all and when the published one cannot be inverted.
    // The basis is title data too: a degenerate or half-initialised one inverts to
    // the same poison a sentinel matrix would, so the result clears the same bar
    // as a published matrix before this fallback may claim success.
    const auto buildViewFromStreamline = [&]() -> bool
    {
        if (!StreamlineHooks::isSetConstantsHooked() || !hasCurrentSLConstants)
            return false;

        SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraRight), 0, _invViewMatrix);
        SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraUp), 1, _invViewMatrix);
        SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraFwd), 2, _invViewMatrix);
        SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraPos), 3, _invViewMatrix);
        _invViewMatrix.r[3].m128_f32[3] = 1.0f;

        _viewMatrix = XMMatrixInverse(nullptr, _invViewMatrix);

        if (!viewMatrixIsUsable(_viewMatrix, _invViewMatrix))
        {
            static bool loggedDegenerateSLView = false;
            if (!loggedDegenerateSLView)
            {
                loggedDegenerateSLView = true;
                LOG_ERROR(
                    "[RR_INPUT] the Streamline camera basis is degenerate; its view "
                    "matrix cannot be inverted, so camera position and reprojection "
                    "would read NaN. Rejecting the frame instead");
            }
            _viewMatrix = {};
            _invViewMatrix = {};
            return false;
        }

        _viewFromStreamline = true;
        return true;
    };

    XMMATRIX publishedViewMatrix = {};
    const bool hasPublishedViewMatrix =
        TryGetNGXColumnVectorMatrix(inParams, NVSDK_NGX_Parameter_DLSS_WORLD_TO_VIEW_MATRIX, publishedViewMatrix,
                                    &_viewMatrixKeyState);

    bool viewMatrixResolved = false;
    if (hasPublishedViewMatrix)
    {
        _viewMatrix = publishedViewMatrix;
        _invViewMatrix = XMMatrixInverse(nullptr, _viewMatrix);
        viewMatrixResolved = viewMatrixIsUsable(_viewMatrix, _invViewMatrix);

        if (!viewMatrixResolved)
        {
            static bool loggedDegenerateViewMatrix = false;
            if (!loggedDegenerateViewMatrix)
            {
                loggedDegenerateViewMatrix = true;
                const XMFLOAT3 cameraPosition = GetFloat3Column(_invViewMatrix, 3);
                LOG_ERROR(
                    "[RR_INPUT] the title's {} matrix cannot be inverted; camera position "
                    "reads ({}, {}, {}) and determinant is {}. Every consumer of the camera "
                    "position would receive NaN. Falling back to the Streamline camera "
                    "constants. Raw published matrix (row-major): "
                    "[{:.6f}, {:.6f}, {:.6f}, {:.6f}], [{:.6f}, {:.6f}, {:.6f}, {:.6f}], "
                    "[{:.6f}, {:.6f}, {:.6f}, {:.6f}], [{:.6f}, {:.6f}, {:.6f}, {:.6f}]",
                    NVSDK_NGX_Parameter_DLSS_WORLD_TO_VIEW_MATRIX,
                    cameraPosition.x, cameraPosition.y, cameraPosition.z,
                    XMVectorGetX(XMMatrixDeterminant(_viewMatrix)),
                    _viewMatrix.r[0].m128_f32[0], _viewMatrix.r[0].m128_f32[1],
                    _viewMatrix.r[0].m128_f32[2], _viewMatrix.r[0].m128_f32[3],
                    _viewMatrix.r[1].m128_f32[0], _viewMatrix.r[1].m128_f32[1],
                    _viewMatrix.r[1].m128_f32[2], _viewMatrix.r[1].m128_f32[3],
                    _viewMatrix.r[2].m128_f32[0], _viewMatrix.r[2].m128_f32[1],
                    _viewMatrix.r[2].m128_f32[2], _viewMatrix.r[2].m128_f32[3],
                    _viewMatrix.r[3].m128_f32[0], _viewMatrix.r[3].m128_f32[1],
                    _viewMatrix.r[3].m128_f32[2], _viewMatrix.r[3].m128_f32[3]);
            }
        }
    }

    bool viewMissing = false;
    if (!viewMatrixResolved && !buildViewFromStreamline())
    {
        static RRRepeatedErrorGate viewMissingGate;
        if (const uint64_t n = viewMissingGate.Next())
            LOG_ERROR("View matrix missing! Denoiser not ready. ({})", RRRepeatNote(n));
        viewMissing = true; // the blocker text is chosen below, once the projection key is known too (SAT-P0-5)
        _viewMatrix = {};
        _invViewMatrix = {};
        isReady = false;
    }

    if (_isInReset || !_hasDenoiserHistory)
        _prevViewMatrix = _viewMatrix;

    // Perspective projection matrix (P)
    _projMatrix = {};
    _projectionFromStreamline = false;

    // The projection is consumed through its inverse by the conversion shaders,
    // so it is only usable if that inverse exists and is finite, whichever source
    // produced it. A sentinel fill - one title publishes an all-FLT_MAX
    // world-to-view matrix - passes a presence check and then poisons every
    // reconstructed position with NaN.
    const auto projectionIsUsable = [&](const XMMATRIX& projection)
    {
        if (!MatrixIsFinite(projection))
            return false;

        const float determinant = XMVectorGetX(XMMatrixDeterminant(projection));
        if (!std::isfinite(determinant) || determinant == 0.0f)
            return false;

        return MatrixIsFinite(XMMatrixInverse(nullptr, projection));
    };

    // Reconstructs an unjittered projection from the Streamline scalar camera data,
    // which is what both the missing and the unusable published matrix fall back to.
    // The scalars are title data too: a zero aspect or a NaN FOV passes every
    // sentinel check and bakes straight into the matrix, so the rebuilt projection
    // must clear the same bar as a published one before this fallback may claim
    // success.
    const auto buildProjectionFromStreamline = [&]() -> bool
    {
        if (!StreamlineHooks::isSetConstantsHooked() || !hasCurrentSLConstants)
            return false;

        if (slData.cameraFOV == sl::INVALID_FLOAT ||
            slData.cameraNear == sl::INVALID_FLOAT ||
            slData.cameraFar == sl::INVALID_FLOAT ||
            slData.cameraAspectRatio == sl::INVALID_FLOAT ||
            slData.cameraNear == slData.cameraFar)
        {
            static RRRepeatedErrorGate slProjectionGate;
            if (const uint64_t n = slProjectionGate.Next())
                LOG_ERROR("Streamline projection data is incomplete! Denoiser not ready. ({})", RRRepeatNote(n));
            return false;
        }

        // These measurements are supposed to be in radians, but some titles supply degrees.
        // Valid FOV in radians never exceeds PI. Realistic FOV in degrees is basically never in the single
        // digits.
        const float fov = (slData.cameraFOV < 4.0f) ? slData.cameraFOV : GetRadiansFromDeg(slData.cameraFOV);
        const float nearPlane = slData.cameraNear;
        const float farPlane = slData.cameraFar;
        _isRightHanded = slData.cameraViewToClip[2].w < 0.0f;

        _projMatrix = CreateColumnVectorPerspectiveProjection(
            fov, slData.cameraAspectRatio, nearPlane, farPlane,
            _isRightHanded, DepthInverted());

        if (!projectionIsUsable(_projMatrix))
        {
            static bool loggedDegenerateSLProjection = false;
            if (!loggedDegenerateSLProjection)
            {
                loggedDegenerateSLProjection = true;
                LOG_ERROR(
                    "[RR_INPUT] the Streamline projection scalars do not build an "
                    "invertible matrix (fov={:.4f}, aspect={:.4f}, near={:.4f}, "
                    "far={:.4f}); camera position and reprojection would read NaN. "
                    "Rejecting the frame instead",
                    fov, slData.cameraAspectRatio, nearPlane, farPlane);
            }
            _projMatrix = {};
            return false;
        }

        _projectionFromStreamline = true;
        return true;
    };

    XMMATRIX publishedProjMatrix = {};
    const bool hasPublishedProjMatrix =
        TryGetNGXColumnVectorMatrix(inParams, NVSDK_NGX_Parameter_DLSS_VIEW_TO_CLIP_MATRIX, publishedProjMatrix,
                                    &_projMatrixKeyState);

    bool projMatrixResolved = false;
    if (hasPublishedProjMatrix)
    {
        _projMatrix = publishedProjMatrix;
        projMatrixResolved = projectionIsUsable(_projMatrix);

        if (!projMatrixResolved)
        {
            static bool loggedDegenerateProjMatrix = false;
            if (!loggedDegenerateProjMatrix)
            {
                loggedDegenerateProjMatrix = true;
                LOG_ERROR(
                    "[RR_INPUT] the title's {} matrix is not invertible, so no view-space "
                    "position can be reconstructed from it. Falling back to the Streamline "
                    "camera scalars. Raw published matrix (row-major): "
                    "[{:.6f}, {:.6f}, {:.6f}, {:.6f}], [{:.6f}, {:.6f}, {:.6f}, {:.6f}], "
                    "[{:.6f}, {:.6f}, {:.6f}, {:.6f}], [{:.6f}, {:.6f}, {:.6f}, {:.6f}]",
                    NVSDK_NGX_Parameter_DLSS_VIEW_TO_CLIP_MATRIX,
                    _projMatrix.r[0].m128_f32[0], _projMatrix.r[0].m128_f32[1],
                    _projMatrix.r[0].m128_f32[2], _projMatrix.r[0].m128_f32[3],
                    _projMatrix.r[1].m128_f32[0], _projMatrix.r[1].m128_f32[1],
                    _projMatrix.r[1].m128_f32[2], _projMatrix.r[1].m128_f32[3],
                    _projMatrix.r[2].m128_f32[0], _projMatrix.r[2].m128_f32[1],
                    _projMatrix.r[2].m128_f32[2], _projMatrix.r[2].m128_f32[3],
                    _projMatrix.r[3].m128_f32[0], _projMatrix.r[3].m128_f32[1],
                    _projMatrix.r[3].m128_f32[2], _projMatrix.r[3].m128_f32[3]);
            }
        }
    }

    if (!projMatrixResolved && !buildProjectionFromStreamline())
    {
        static RRRepeatedErrorGate projectionMissingGate;
        if (const uint64_t n = projectionMissingGate.Next())
            LOG_ERROR("Projection matrix missing! Denoiser not ready. ({})", RRRepeatNote(n));
        if (!viewMissing && _denoiserBlocker.empty())
            _denoiserBlocker =
                "the game publishes no usable projection matrix (ViewToClip) and no Streamline camera constants. "
                "NVIDIA's Ray Reconstruction can run without it; FSR Ray Regeneration needs it for its reprojection. "
                "Turn Ray Reconstruction off in the game for now, and restore any engine denoiser settings you "
                "changed for it";
        _projMatrix = {};
        isReady = false;
    }

    // SAT-P0-5 (AMDNR 0.3.4): honest wording. The menu shows "Ray Regeneration is off in this title: <this>" and
    // the fallback logs it. The old text called this "a game-side integration gap, not a setting"; but the stock
    // Unreal DLSS plugin passes the matrices as null on purpose (NVIDIA's Ray Reconstruction treats them as
    // optional), so the gap is on the FSR Ray Regeneration side. Literals only: this runs on every failing frame.
    if (viewMissing)
    {
        if (_viewMatrixKeyState == kMatrixKeyNull && _projMatrixKeyState == kMatrixKeyNull)
            _denoiserBlocker =
                "the game's DLSS plugin passes empty camera matrices (WorldToView and ViewToClip are null). NVIDIA's "
                "Ray Reconstruction treats them as optional; FSR Ray Regeneration needs them for its reprojection. "
                "Turn Ray Reconstruction off in the game for now, and restore any engine denoiser settings you "
                "changed for it";
        else if (_viewMatrixKeyState == kMatrixKeySet)
            _denoiserBlocker =
                "the game's WorldToView matrix does not invert to a camera position, and no Streamline camera "
                "constants replace it. FSR Ray Regeneration needs a camera for its reprojection. Turn Ray "
                "Reconstruction off in the game for now";
        else
            _denoiserBlocker =
                "the game publishes no camera matrices (WorldToView / ViewToClip) and no Streamline camera constants. "
                "NVIDIA's Ray Reconstruction can run without them; FSR Ray Regeneration needs them for its "
                "reprojection. Turn Ray Reconstruction off in the game for now, and restore any engine denoiser "
                "settings you changed for it";
    }

    // SAT-P0-2 / RR-22: the stock Unreal plugin's signature on this frame - no camera from any source while both
    // keys are present but null.
    _cameraNullKeysThisFrame =
        !isReady && _viewMatrixKeyState == kMatrixKeyNull && _projMatrixKeyState == kMatrixKeyNull;

    return isReady;
}

// Resolves the automatic diffuse and specular signal classification.
//
// Runs last, and only for a frame that cleared every validation before it: the
// lock is permanent, so a frame about to be rejected must not set it. The first
// Evaluate often arrives before the title has tagged its optional guides, and
// locking there would pin the classification for the life of the context.
bool FSRDFeatureDx12::ResolveSignalTypes(bool isReady, bool hasCurrentSLConstants)
{
    const auto& cfg = *Config::Instance();

    // AMD RR 1.2 requires a valid ray length in alpha for every active indirect-
    // specular pixel. The INI's unset value is Auto: resolve it once from a
    // semantically named, format/extent-validated guide, then keep the context
    // classification stable even if an optional tag temporarily disappears.
    //
    // This runs last, and only for a frame that cleared every validation above. The
    // lock is permanent, so a frame that is about to be rejected must not set it -
    // the first Evaluate frequently arrives before the title has tagged its optional
    // guides, and locking there would pin the classification to Direct for good.
    if (isReady && !cfg.FfxDenoiserSpecularSignalType.has_value() &&
        !_autoSpecularSignalResolved)
    {
        // A guide is present when the specular step bound one - the same question, asked of
        // the frame's result rather than of the step's local selection.
        const bool hasSpecularHitDistanceGuide =
            _convDesc.Resources.InSpecHitDist != nullptr ||
            _convDesc.Resources.InSpecularRayDirectionHitDistance != nullptr;

        const ffxStructType_t resolvedType = hasSpecularHitDistanceGuide
            ? FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR
            : FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR;

        LOG_INFO(
            "[RR_INPUT] automatic specular classification resolved to {} on the first validated frame ({})",
            GetSignalTypeName(resolvedType),
            hasSpecularHitDistanceGuide
                ? "validated hit-distance guide present"
                : "no validated hit-distance guide; keeping every specular pixel active");

        if (_specularSignalDescType == resolvedType)
        {
            _autoSpecularSignalDescType = resolvedType;
            _autoSpecularSignalResolved = true;
        }
        else if (_preprocessorHasRecordedWork)
        {
            // Recreating in place here would destroy converter textures that an
            // already-submitted command list can still reference. UpdateSize refuses
            // exactly this for exactly this reason; take the same escape hatch and let
            // the rebuilt instance resolve on its own first frame, where nothing has
            // been recorded yet. The resolution is deliberately not latched, so the
            // fresh instance repeats it rather than inheriting a stale decision.
            LOG_INFO(
                "[RR_INPUT] automatic specular classification needs {} -> {}; requesting a feature rebuild because this instance has already recorded GPU work",
                GetSignalTypeName(_specularSignalDescType),
                GetSignalTypeName(resolvedType));
            InvalidateDenoiserHistory(HistoryResetReason::Setting);
            State::Instance().changeBackend[Handle()->Id] = true;
            return false;
        }
        else
        {
            LOG_INFO(
                "[RR_INPUT] recreating RR context for automatic specular classification: {} -> {}",
                GetSignalTypeName(_specularSignalDescType),
                GetSignalTypeName(resolvedType));
            _autoSpecularSignalDescType = resolvedType;
            _autoSpecularSignalResolved = true;
            DestroyDenoiserContext();
            if (!CreateDenoiserContext())
            {
                LOG_ERROR(
                    "[RR_INPUT] failed to recreate RR context for automatic specular classification; requesting a feature rebuild");
                State::Instance().changeBackend[Handle()->Id] = true;
                return false;
            }
        }
    }

    // Same contract for the diffuse signal, resolved from the guide that signal
    // actually consumes: indirect diffuse reads a ray length from its alpha, direct
    // diffuse leaves that channel undefined. A title that supplies no diffuse ray
    // length therefore has nothing for the indirect path to work from, and every
    // pixel is better served staying active on the direct one.
    //
    // Deliberately a separate block rather than folded into the specular one above:
    // that block can return early to request a rebuild, and the two classifications
    // must not become order-dependent on each other.
    if (isReady && !cfg.FfxDenoiserDiffuseSignalType.has_value() &&
        !_autoDiffuseSignalResolved)
    {
        const bool hasDiffuseRayLength = _convDesc.Resources.InDiffuseHitDistance != nullptr &&
            _convDesc.DiffuseHitDistanceMode != 0u;

        const ffxStructType_t resolvedType = hasDiffuseRayLength
            ? FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE
            : FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE;

        LOG_INFO(
            "[RR_INPUT] automatic diffuse classification resolved to {} on the first validated frame ({})",
            GetSignalTypeName(resolvedType),
            hasDiffuseRayLength
                ? "validated diffuse ray length present"
                : "no diffuse ray length; keeping every diffuse pixel active");

        if (_diffuseSignalDescType == resolvedType)
        {
            _autoDiffuseSignalDescType = resolvedType;
            _autoDiffuseSignalResolved = true;
        }
        else if (_preprocessorHasRecordedWork)
        {
            LOG_INFO(
                "[RR_INPUT] automatic diffuse classification needs {} -> {}; requesting a feature rebuild because this instance has already recorded GPU work",
                GetSignalTypeName(_diffuseSignalDescType), GetSignalTypeName(resolvedType));
            InvalidateDenoiserHistory(HistoryResetReason::Setting);
            State::Instance().changeBackend[Handle()->Id] = true;
            return false;
        }
        else
        {
            LOG_INFO("[RR_INPUT] recreating RR context for automatic diffuse classification: {} -> {}",
                     GetSignalTypeName(_diffuseSignalDescType), GetSignalTypeName(resolvedType));
            _autoDiffuseSignalDescType = resolvedType;
            _autoDiffuseSignalResolved = true;
            DestroyDenoiserContext();
            if (!CreateDenoiserContext())
            {
                LOG_ERROR(
                    "[RR_INPUT] failed to recreate RR context for automatic diffuse classification; requesting a feature rebuild");
                State::Instance().changeBackend[Handle()->Id] = true;
                return false;
            }
        }
    }

    return isReady;
}

bool FSRDFeatureDx12::PrepareDenoiseConvInput(const NVSDK_NGX_Parameter& inParams)
{
    const auto& cfg = *Config::Instance();
    const SLConstantsSnapshot slConstantsSnapshot =
        StreamlineHooks::getSLConstantsSnapshot();
    const auto& slData = slConstantsSnapshot.constants;

    // Gather DLSS-RR input buffers for conversion and repacking for FSR-RR
    bool isReady = true;
    _convDesc.Resources = {};
    _convDesc.SpecularHitDistanceBase = {};
    _convDesc.SpecularHitDistanceFromCombinedAlpha = false;
    // The flag word is rebuilt from zero every frame, ahead of every producer
    // (ConvertDenoiserBuffers and ApplyDepthInterpretation, which only OR bits
    // in). Without this, a flag earned by an earlier frame's resources survives
    // that resource going away: a stale HasSpecHitDistance pins the scalar path
    // after the title moves to combined alpha, a stale TitleLinearDepth keeps
    // the shader reading a texture that is no longer bound, and a linear ->
    // hardware depth flip carries IsDepthLinear forward.
    _convDesc.Flags = 0;
    _specularHitDistanceTaggedResource.Reset();
    _specularRayDirectionHitDistanceTaggedResource.Reset();
    const RRD3D12SignalTagSnapshot rrTagSnapshot =
        StreamlineHooks::getRRD3D12SignalTagSnapshot();
    const bool hasCurrentSLConstants =
        rrTagSnapshot.activeEvaluationFrame != UINT32_MAX &&
        rrTagSnapshot.activeEvaluationViewport != UINT32_MAX &&
        slConstantsSnapshot.frameIndex == rrTagSnapshot.activeEvaluationFrame &&
        slConstantsSnapshot.viewport == rrTagSnapshot.activeEvaluationViewport;
    RRTaggedResourceDiagnostic emissiveTagDiagnostic {};

    // Standard TSR buffers
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Color, _convDesc.Resources.InColor))
        isReady = false;
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_MotionVectors, _convDesc.Resources.InMotionVectors))
        isReady = false;
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Depth, _convDesc.Resources.InDepth))
        isReady = false;

    // DLSSD-specific buffers
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_GBuffer_Normals, _convDesc.Resources.InNormals))
        isReady = false;

    // Roughness mode is instance state. When creation metadata is absent, lock it
    // once from the first usable frame; never flip modes because one resource is
    // temporarily missing, as that changes shader bindings and invalidates history.
    const bool hasSeparateRoughness = TryGetNGXVoidPointer(
        inParams, NVSDK_NGX_Parameter_GBuffer_Roughness,
        _convDesc.Resources.InRoughness);
    if (_roughnessSource == RoughnessSource::Unknown)
    {
        if (hasSeparateRoughness)
        {
            _roughnessSource = RoughnessSource::Separate;
            LOG_INFO("DLSSD roughness metadata absent; locked this instance to the separate resource");
        }
        else if (SupportsPackedRoughness(_convDesc.Resources.InNormals))
        {
            _roughnessSource = RoughnessSource::Packed;
            LOG_INFO("DLSSD roughness metadata absent; locked this instance to normals alpha");
        }
        else
        {
            static RRRepeatedErrorGate roughnessAbsentGate;
            if (const uint64_t n = roughnessAbsentGate.Next())
                LOG_ERROR("DLSSD roughness metadata and separate texture are absent, and normals format has no alpha "
                          "channel ({})",
                          RRRepeatNote(n));
            isReady = false;
        }
    }
    else if (_roughnessSource == RoughnessSource::Separate && !hasSeparateRoughness)
    {
        static RRRepeatedErrorGate roughnessMissingGate;
        if (const uint64_t n = roughnessMissingGate.Next())
            LOG_ERROR("DLSSD instance requires separate roughness, but the resource is missing this frame ({})",
                      RRRepeatNote(n));
        InvalidateDenoiserHistory(HistoryResetReason::InputNotReady);
        isReady = false;
    }

    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_DiffuseAlbedo, _convDesc.Resources.InDiffAlbedo))
        isReady = false;

    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_SpecularAlbedo, _convDesc.Resources.InSpecAlbedo))
        isReady = false;

    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, _convDesc.Resources.InBiasMask);
    // RR-10: for the overlay (RE Requiem publishes no bias mask, so its strength slider acts on nothing there).
    _biasMaskState = _convDesc.Resources.InBiasMask ? 1 : 0;
    FSRD::RuntimeStatus::BiasMaskPresent.store(_biasMaskState, std::memory_order_relaxed);

    // Optional diagnostic-only emissive input. Do not use it for RR signal
    // separation until its contents, exposure and composition semantics are verified.
    _emissiveProbe = nullptr;
    _emissiveTaggedResource.Reset();
    _emissiveProbeCompatible = false;
    _emissiveProbeFromStreamline = false;
    _convDesc.Resources.InEmissive = nullptr;
    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_GBuffer_Emissive, _emissiveProbe);
    if (!_emissiveProbe)
    {
        _emissiveProbeFromStreamline = AcquireSLTaggedResource(
            rrTagSnapshot, RRTaggedSignal::Emissive, "Streamline.Emissive",
            TagStatePolicy::RequireShaderRead,
            _emissiveTaggedResource, emissiveTagDiagnostic);
        if (_emissiveProbeFromStreamline)
            _emissiveProbe = _emissiveTaggedResource.Get();
    }
    _emissiveProbeCompatible = IsEmissiveProbeCompatible(_emissiveProbe);
    if (_emissiveProbeCompatible)
        _convDesc.Resources.InEmissive = _emissiveProbe;

    // Optional legacy NGX GBuffer identifiers. NRD can use a material identifier
    // to reject history across material boundaries, but DLSS-RR does not require
    // either resource. Probe only; do not consume an unverified title-specific
    // encoding as temporal-history metadata.
    _materialIdProbe = nullptr;
    _shadingModelIdProbe = nullptr;
    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_GBuffer_MaterialId,
                         _materialIdProbe);
    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_GBuffer_ShadingModelId,
                         _shadingModelIdProbe);

    // Optional specular hit-distance inputs. Selection is deferred until all source
    // origins and extents are known, so an invalid high-priority candidate can never
    // reach an SRV binding.
    ID3D12Resource* ngxSpecularHitDistance = nullptr;
    ID3D12Resource* ngxSpecularRayDirectionHitDistance = nullptr;
    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance,
                         ngxSpecularHitDistance);
    TryGetNGXVoidPointer(inParams,
                         NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance,
                         ngxSpecularRayDirectionHitDistance);

    const XMUINT2 colorBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y);
    const XMUINT2 depthBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y);
    const XMUINT2 declaredMotionBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X,
        NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y);
    XMUINT2 motionBase = declaredMotionBase;
    const XMUINT2 normalBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_Normals_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSS_Input_Normals_Subrect_Base_Y);
    const XMUINT2 roughnessBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_Roughness_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSS_Input_Roughness_Subrect_Base_Y);
    const XMUINT2 diffuseAlbedoBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_DiffuseAlbedo_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSS_Input_DiffuseAlbedo_Subrect_Base_Y);
    const XMUINT2 specularAlbedoBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_SpecularAlbedo_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSS_Input_SpecularAlbedo_Subrect_Base_Y);
    const XMUINT2 ngxSpecularHitDistanceBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance_Subrect_Base_Y);
    const XMUINT2 ngxSpecularRayDirectionHitDistanceBase = GetSubrectBase(
        inParams,
        NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance_Subrect_Base_X,
        NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance_Subrect_Base_Y);
    const XMUINT2 biasBase = GetSubrectBase(
        inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_X,
        NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_Y);

    XMUINT2 emissiveBase = colorBase;
    if (_emissiveProbeFromStreamline)
    {
        if (emissiveTagDiagnostic.present && emissiveTagDiagnostic.usesExtent)
            emissiveBase = {
                emissiveTagDiagnostic.extentLeft, emissiveTagDiagnostic.extentTop
            };
    }
    if (_convDesc.Resources.InEmissive &&
        !ValidateSourceExtent(
            "Emissive", _convDesc.Resources.InEmissive, emissiveBase,
            RenderWidth(), RenderHeight()))
    {
        _convDesc.Resources.InEmissive = nullptr;
        _emissiveProbeCompatible = false;
    }

    _convDesc.FloorSourceBase = { colorBase.x, colorBase.y, depthBase.x, depthBase.y };
    _convDesc.InputBase1 = { normalBase.x, normalBase.y, roughnessBase.x, roughnessBase.y };
    _convDesc.InputBase2 = { ngxSpecularHitDistanceBase.x, ngxSpecularHitDistanceBase.y,
                             diffuseAlbedoBase.x, diffuseAlbedoBase.y };
    _convDesc.InputBase3 = { specularAlbedoBase.x, specularAlbedoBase.y,
                             biasBase.x, biasBase.y };
    _convDesc.InputBase4 = { emissiveBase.x, emissiveBase.y, 0u, 0u };

    const uint32_t renderWidth = RenderWidth();
    const uint32_t renderHeight = RenderHeight();
    uint32_t motionWidth = LowResMV() ? RenderWidth() : DisplayWidth();
    uint32_t motionHeight = LowResMV() ? RenderHeight() : DisplayHeight();
    bool displayResolutionMotion = !LowResMV();

    // Streamline may replace a title's display-resolution MV with its own
    // camera-completed render-resolution texture before forwarding the NGX call.
    // Feature-creation metadata still describes the original resource, so the
    // actual D3D12 extent must win when the two contracts no longer fit.
    if (_convDesc.Resources.InMotionVectors)
    {
        const D3D12_RESOURCE_DESC motionDesc =
            _convDesc.Resources.InMotionVectors->GetDesc();
        const bool declaredExtentFits =
            uint64_t(motionBase.x) + motionWidth <= motionDesc.Width &&
            uint64_t(motionBase.y) + motionHeight <= motionDesc.Height;
        const bool zeroBasedLogicalExtentFits =
            uint64_t(motionWidth) <= motionDesc.Width &&
            uint64_t(motionHeight) <= motionDesc.Height;
        const bool zeroBasedRenderExtentFits =
            uint64_t(RenderWidth()) <= motionDesc.Width &&
            uint64_t(RenderHeight()) <= motionDesc.Height;
        if (!declaredExtentFits && zeroBasedLogicalExtentFits)
        {
            LOG_WARN(
                "[RR_INPUT] NGX motion resource is {}x{} and cannot contain the declared "
                "subrect {}x{}+({},{}); retaining its logical resolution with a "
                "zero-based origin",
                motionDesc.Width, motionDesc.Height, motionWidth, motionHeight,
                motionBase.x, motionBase.y);
            motionBase = { 0u, 0u };
        }
        else if (!declaredExtentFits && displayResolutionMotion &&
                 zeroBasedRenderExtentFits)
        {
            LOG_WARN(
                "[RR_INPUT] NGX motion resource is {}x{} and cannot contain the declared "
                "display-resolution subrect {}x{}+({},{}); treating it as Streamline's "
                "zero-based render-resolution camera-completed motion",
                motionDesc.Width, motionDesc.Height, motionWidth, motionHeight,
                motionBase.x, motionBase.y);
            motionWidth = RenderWidth();
            motionHeight = RenderHeight();
            motionBase = { 0u, 0u };
            displayResolutionMotion = false;
        }
    }

    _convDesc.InputBase0 = { colorBase.x, colorBase.y, motionBase.x, motionBase.y };
    if (motionWidth == 0 || motionHeight == 0)
    {
        static RRRepeatedErrorGate motionExtentGate;
        if (const uint64_t n = motionExtentGate.Next())
            LOG_ERROR("[RR_INPUT] motion-vector extent is zero ({})", RRRepeatNote(n));
        isReady = false;
    }
    else
    {
        float motionScaleX = 1.0f;
        float motionScaleY = 1.0f;
        const bool validMotionScaleX =
            inParams.Get(NVSDK_NGX_Parameter_MV_Scale_X, &motionScaleX) == NVSDK_NGX_Result_Success &&
            std::isfinite(motionScaleX) && motionScaleX != 0.0f;
        const bool validMotionScaleY =
            inParams.Get(NVSDK_NGX_Parameter_MV_Scale_Y, &motionScaleY) == NVSDK_NGX_Result_Success &&
            std::isfinite(motionScaleY) && motionScaleY != 0.0f;
        if (!validMotionScaleX)
            motionScaleX = 1.0f;
        if (!validMotionScaleY)
            motionScaleY = 1.0f;
        if (!validMotionScaleX || !validMotionScaleY)
        {
            // NGX helper APIs normalize a missing, non-finite, or zero component
            // to one. Preserve a valid negative component because it can encode
            // the title's axis convention.
            LOG_WARN(
                "[RR_INPUT] MV scale component missing/invalid/zero (x={}, y={}); "
                "using NGX's unit fallback per component",
                validMotionScaleX, validMotionScaleY);
        }

        _convDesc.MotionInputSize = {
            static_cast<float>(motionWidth), static_cast<float>(motionHeight),
            1.0f / static_cast<float>(motionWidth), 1.0f / static_cast<float>(motionHeight)
        };
        _convDesc.MotionTransform = {
            // NGX MV scale converts the stored value to render-pixel motion.
            // The texture extent only controls where that value is fetched; UV
            // normalization always uses the render extent, including high-res MV.
            motionScaleX / static_cast<float>(RenderWidth()),
            motionScaleY / static_cast<float>(RenderHeight()), 0.0f, 0.0f
        };
    }

    ResolveSpecularHitDistance(
        inParams, rrTagSnapshot, renderWidth, renderHeight, motionWidth, motionHeight,
        ngxSpecularHitDistance, ngxSpecularRayDirectionHitDistance,
        ngxSpecularHitDistanceBase, ngxSpecularRayDirectionHitDistanceBase);

    AcquireOptionalInputs(inParams, rrTagSnapshot, renderWidth, renderHeight);

    float jitterX = 0.0f;
    float jitterY = 0.0f;
    inParams.Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &jitterX);
    inParams.Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &jitterY);
    if (!std::isfinite(jitterX))
        jitterX = 0.0f;
    if (!std::isfinite(jitterY))
        jitterY = 0.0f;

    const XMFLOAT2 currentJitter { jitterX, jitterY };
    const XMFLOAT2 previousJitter = (_isInReset || !_hasDenoiserHistory)
        ? currentJitter
        : _previousDenoiserJitter;
    _convDesc.JitterOffsets = {
        currentJitter.x, currentJitter.y, previousJitter.x, previousJitter.y
    };
    _convDesc.MotionHistoryValid = _hasDenoiserHistory && !_isInReset;
    _convDesc.DisplayResolutionMotion = displayResolutionMotion;
    _convDesc.MotionVectorsJittered = JitteredMV();

    // SAT-P0-1 (AMDNR 0.3.4): one snapshot of every RR input and of what the title declared, once per handle and
    // before the camera check. A title whose camera is missing never reaches the conversion, so the depth format,
    // kind and flags ApplyDepthInterpretation prints were never logged for it (Satisfactory).
    if (!_loggedInputSnapshot)
    {
        _loggedInputSnapshot = true;
        if (FirstRRLineForHandle(Handle()->Id, RRHandleLine::InputSnapshot))
        {
            ID3D12Resource* output = nullptr;
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_Output, output);
            float mvScaleX = 1.0f;
            float mvScaleY = 1.0f;
            const bool hasMvScaleX = inParams.Get(NVSDK_NGX_Parameter_MV_Scale_X, &mvScaleX) == NVSDK_NGX_Result_Success;
            const bool hasMvScaleY = inParams.Get(NVSDK_NGX_Parameter_MV_Scale_Y, &mvScaleY) == NVSDK_NGX_Result_Success;
            const auto& state = State::Instance();

            LOG_INFO("[RR_INPUT] handle={} snapshot resources: color={}; depth={} kind={}; normals={}; roughness={}; "
                     "diffuseAlbedo={}; specularAlbedo={}; motion={}; output={}",
                     Handle()->Id, DescribeRRTexture(_convDesc.Resources.InColor),
                     DescribeRRTexture(_convDesc.Resources.InDepth),
                     DepthResourceKindName(ClassifyDepthResource(_convDesc.Resources.InDepth)),
                     DescribeRRTexture(_convDesc.Resources.InNormals),
                     _roughnessSource == RoughnessSource::Separate ? DescribeRRTexture(_convDesc.Resources.InRoughness)
                     : _roughnessSource == RoughnessSource::Packed ? std::string("normals alpha")
                                                                   : std::string("undetermined"),
                     DescribeRRTexture(_convDesc.Resources.InDiffAlbedo),
                     DescribeRRTexture(_convDesc.Resources.InSpecAlbedo),
                     DescribeRRTexture(_convDesc.Resources.InMotionVectors), DescribeRRTexture(output));
            LOG_INFO("[RR_INPUT] handle={} snapshot declared: depthType={} (DLSS.Use.HW.Depth at create), "
                     "depthInvertedFlag={}, roughnessMode={}, lowResMV={}, jitteredMV={}, autoExposure={}, "
                     "mvScale=({}{}, {}{}), renderSize={}x{}, displaySize={}x{}, colorSubrectBase=({}, {}), "
                     "depthSubrectBase=({}, {}), jitter=({:.4f}, {:.4f}), engine={} {}",
                     Handle()->Id, _hasNGXDepthType ? (_ngxReportedHWDepth ? "hw" : "linear") : "absent",
                     DepthInverted() ? 1 : 0,
                     _roughnessSource == RoughnessSource::Separate ? "separate"
                     : _roughnessSource == RoughnessSource::Packed ? "packed"
                                                                   : "undetermined",
                     LowResMV() ? 1 : 0, JitteredMV() ? 1 : 0, AutoExposure() ? 1 : 0, mvScaleX,
                     hasMvScaleX ? "" : " absent", mvScaleY, hasMvScaleY ? "" : " absent", renderWidth, renderHeight,
                     DisplayWidth(), DisplayHeight(), colorBase.x, colorBase.y, depthBase.x, depthBase.y,
                     currentJitter.x, currentJitter.y, NGXEngineName(state.NVNGX_Engine),
                     state.NVNGX_EngineVersion.empty() ? std::string("n/a") : state.NVNGX_EngineVersion);
        }
    }

    isReady &= ValidateSourceExtent("Color", _convDesc.Resources.InColor,
                                    colorBase, renderWidth, renderHeight);
    isReady &= ValidateSourceExtent("Depth", _convDesc.Resources.InDepth,
                                    depthBase, renderWidth, renderHeight);
    isReady &= ValidateSourceExtent("MotionVectors", _convDesc.Resources.InMotionVectors,
                                    motionBase, motionWidth, motionHeight);
    isReady &= ValidateSourceExtent("Normals", _convDesc.Resources.InNormals,
                                    normalBase, renderWidth, renderHeight);
    if (_roughnessSource == RoughnessSource::Separate)
        isReady &= ValidateSourceExtent("Roughness", _convDesc.Resources.InRoughness,
                                        roughnessBase, renderWidth, renderHeight);
    isReady &= ValidateSourceExtent("DiffuseAlbedo", _convDesc.Resources.InDiffAlbedo,
                                    diffuseAlbedoBase, renderWidth, renderHeight);
    isReady &= ValidateSourceExtent("SpecularAlbedo", _convDesc.Resources.InSpecAlbedo,
                                    specularAlbedoBase, renderWidth, renderHeight);
    if (_convDesc.Resources.InBiasMask)
        isReady &= ValidateSourceExtent("BiasCurrentColor", _convDesc.Resources.InBiasMask,
                                        biasBase, renderWidth, renderHeight);

    ResolveDiffuseHitDistance(inParams, renderWidth, renderHeight);

    // SAT-P1 (AMDNR 0.3.4, [FSR-RR] FfxDenoiserNgxDirectSLConstants, default off): an NGX-direct evaluate (the
    // stock Unreal plugin) never has "current" Streamline constants, because only slEvaluateFeature marks a frame.
    // If the title still sends slSetConstants (a DLSS-G spoof may make UE's Streamline plugin do so), the entry
    // whose jitter equals this frame's NGX jitter, at most two presents old, belongs to this frame; it then feeds
    // the Streamline camera fallback, which only runs where the published matrices are missing or unusable.
    // Inert when the key is false or nothing matches.
    const sl::Constants* cameraSlData = &slData;
    bool cameraSlCurrent = hasCurrentSLConstants;
    SLConstantsJitterMatch slMatch {};
    if (!hasCurrentSLConstants && rrTagSnapshot.activeEvaluationFrame == UINT32_MAX &&
        cfg.FfxDenoiserNgxDirectSLConstants.value_or_default() &&
        StreamlineHooks::findSLConstantsForJitter(currentJitter.x, currentJitter.y, 2, slMatch))
    {
        cameraSlData = &slMatch.constants;
        cameraSlCurrent = true;
        if (!_loggedSLJitterMatch)
        {
            _loggedSLJitterMatch = true;
            if (FirstRRLineForHandle(Handle()->Id, RRHandleLine::SLJitterMatch))
                LOG_INFO("[RR_CAM] handle={} source=SL-ngx-direct: Streamline constants of frame {} (viewport {}) "
                         "match this NGX evaluate's jitter ({:.4f}, {:.4f}): ageInPresents={}, jitterDelta={:.6f}, "
                         "sign={} - they stand in for missing camera matrices ([FSR-RR] "
                         "FfxDenoiserNgxDirectSLConstants=true)",
                         Handle()->Id, slMatch.frameIndex, slMatch.viewport, currentJitter.x, currentJitter.y,
                         slMatch.agePresents, slMatch.jitterDelta, slMatch.signFlipped ? "flipped" : "same");
        }
    }

    const bool cameraReady = ResolveCameraMatrices(inParams, *cameraSlData, cameraSlCurrent);
    isReady &= cameraReady;
    _cameraBlockedThisFrame = !cameraReady; // SAT-P0-4
    // RR-22: the stock Unreal plugin evaluates RR through NGX directly. A title that drives RR inside
    // slEvaluateFeature can still deliver Streamline camera constants on a later frame, so its frames never count.
    if (rrTagSnapshot.activeEvaluationFrame != UINT32_MAX)
        _cameraNullKeysThisFrame = false;

    // SAT-P0-2 (AMDNR 0.3.4): what the camera matrix keys held, once per handle. The stock Unreal DLSS plugin
    // leaves both present but null; "absent" is a title that never set them.
    if (!_loggedCameraKeys)
    {
        _loggedCameraKeys = true;
        if (FirstRRLineForHandle(Handle()->Id, RRHandleLine::CameraKeys))
        {
            const auto& state = State::Instance();
            LOG_INFO("[RR_CAM] handle={} evaluate={} engine={} engineVersion={} matrix keys: WorldToViewMatrix={}, "
                     "ViewToClipMatrix={}; camera={} (view from Streamline={}, projection from Streamline={})",
                     Handle()->Id,
                     rrTagSnapshot.activeEvaluationFrame == UINT32_MAX ? "NGX-direct" : "in slEvaluateFeature",
                     NGXEngineName(state.NVNGX_Engine),
                     state.NVNGX_EngineVersion.empty() ? std::string("n/a") : state.NVNGX_EngineVersion,
                     MatrixKeyStateName(_viewMatrixKeyState), MatrixKeyStateName(_projMatrixKeyState),
                     cameraReady ? "ready" : "missing", _viewFromStreamline ? 1 : 0, _projectionFromStreamline ? 1 : 0);
        }
    }

    if (!ResolveSignalTypes(isReady, hasCurrentSLConstants))
        return false;

    return isReady;
}

// Decides whether the title's depth is hardware or already linear, and applies it.
//
// The precedence is user override, then conclusive evidence from the resource itself, then the
// title's NGX declaration, then a base-rate assumption - and the declaration deliberately does
// not outrank a depth-stencil resource. NGX publishes DLSS.Use.HW.Depth from
// NVSDK_NGX_DLSSD_Create_Params::depthType, where Linear is zero, so every title that
// zero-initializes that struct "declares" linear without meaning to. A declaration is therefore
// only trustworthy when the resource does not contradict it, and nothing writes a linearised
// view-space distance into a depth-stencil attachment.
//
// Applied per frame rather than latched at init so it can be toggled while watching the depth
// debug view. Switching changes the units of the internal previous-depth texture, so it resets
// temporal history.
void FSRDFeatureDx12::ApplyDepthInterpretation()
{
    const auto& cfg = *Config::Instance();

    // Resolve the effective depth interpretation. A user override wins over whatever
    // NGX reported, and it is applied per frame rather than latched at init so it can
    // be toggled while watching the depth debug view. Switching changes the units of
    // the internal previous-depth texture, so it has to reset temporal history.
    //
    // Precedence: user override, then conclusive evidence from the resource itself,
    // then the NGX declaration, then a base-rate assumption.
    //
    // The declaration deliberately does NOT outrank a depth-stencil resource. NGX
    // publishes DLSS.Use.HW.Depth from NVSDK_NGX_DLSSD_Create_Params::depthType, and
    // Linear is zero - so every title that zero-initializes that struct "declares"
    // linear without meaning to. A declaration is therefore only trustworthy when the
    // resource does not contradict it, and nothing writes a linearized view-space
    // distance into a depth-stencil attachment.
    const DepthResourceKind depthKind = ClassifyDepthResource(_convDesc.Resources.InDepth);

    bool hardwareDepth;
    const char* depthSource;

    if (cfg.FfxDenoiserHardwareDepth.has_value())
    {
        hardwareDepth = cfg.FfxDenoiserHardwareDepth.value();
        depthSource = "user override";
    }
    else if (depthKind == DepthResourceKind::HardwareDepth)
    {
        hardwareDepth = true;
        depthSource = (_hasNGXDepthType && !_ngxReportedHWDepth)
            ? "depth-stencil resource, overriding the title's linear declaration"
            : "depth-stencil resource";
    }
    else if (_hasNGXDepthType)
    {
        hardwareDepth = _ngxReportedHWDepth;
        depthSource = "NGX declaration";
    }
    else if (depthKind == DepthResourceKind::GameWritten)
    {
        // Render-target or UAV capable and not depth-stencil: the title's own shaders
        // filled this, which is what a pre-linearized depth target looks like.
        hardwareDepth = false;
        depthSource = "undeclared; game-written target, assuming linear";
    }
    else
    {
        // Undeclared and unclassifiable. Hardware depth is the overwhelmingly common
        // case in DLSS-RR titles, and guessing linear here is the failure that
        // collapses the whole scene into [near, 1].
        hardwareDepth = true;
        depthSource = "undeclared and unclassifiable, assuming hardware";
    }

    const int appliedHardwareDepth = hardwareDepth ? 1 : 0;

    if (_appliedHardwareDepth != appliedHardwareDepth)
    {
        if (_appliedHardwareDepth >= 0)
        {
            LOG_INFO("[RR_INPUT] depth interpretation changed: {} -> {}; resetting denoiser history",
                     _appliedHardwareDepth != 0 ? "hardware" : "linear",
                     hardwareDepth ? "hardware" : "linear");
            InvalidateDenoiserHistory(HistoryResetReason::Setting);
        }
        else
        {
            // Report the evidence, not just the verdict: when the resource is
            // unclassifiable this is the only way to tell why auto chose what it did.
            DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
            uint32_t depthFlags = 0;
            if (_convDesc.Resources.InDepth)
            {
                const D3D12_RESOURCE_DESC depthDesc = _convDesc.Resources.InDepth->GetDesc();
                depthFormat = depthDesc.Format;
                depthFlags = static_cast<uint32_t>(depthDesc.Flags);
            }

            LOG_INFO(
                "[RR_INPUT] depth interpretation: {} (source: {}); resource format={}, "
                "resourceFlags={:#x}, ngxDeclared={}",
                hardwareDepth ? "hardware" : "linear", depthSource,
                magic_enum::enum_name(depthFormat), depthFlags,
                _hasNGXDepthType ? (_ngxReportedHWDepth ? "hardware" : "linear") : "absent");
        }

        _appliedHardwareDepth = appliedHardwareDepth;
    }

    _isHWDepth = hardwareDepth;

    if (!_isHWDepth)
        _convDesc.Flags |= (uint32_t) FSRDConvFlags::IsDepthLinear;

}

// SAT-P0-4: records one content-probe capture on a frame whose RR camera is missing (see RRContentProbe). The
// "reading" line is written only once a capture is recorded, and a probe that never gets a readable frame says so
// once and stops, so the log never promises a result that cannot come.
void FSRDFeatureDx12::RecordContentProbe(ID3D12GraphicsCommandList* commandList, const NVSDK_NGX_Parameter& inParams)
{
    if (!_contentProbe)
    {
        _contentProbe = std::make_unique<RRContentProbe>();

        // Once per handle: a re-pick or a Reset re-create on the same handle does not probe again.
        if (!FirstRRLineForHandle(Handle()->Id, RRHandleLine::ContentProbe))
        {
            _contentProbe->done = true;
            return;
        }

        if (Device == nullptr || !_contentProbe->Create(Device))
        {
            _contentProbe->failed = true;
            _contentProbe->done = true;
            return;
        }
    }

    if (_contentProbe->done || commandList == nullptr)
        return;

    // This frame's inputs and origins, as PrepareDenoiseConvInput just read them.
    ID3D12Resource* depth = _convDesc.Resources.InDepth;
    ID3D12Resource* normals = _convDesc.Resources.InNormals;
    const uint32_t width = RenderWidth();
    const uint32_t height = RenderHeight();
    if (depth == nullptr || normals == nullptr || width == 0 || height == 0)
    {
        // A capture already recorded keeps polling; with none, give up after kMaxUnreadable such frames.
        if (_contentProbe->recorded == 0 && ++_contentProbe->unreadableFrames >= RRContentProbe::kMaxUnreadable)
        {
            LOG_WARN("[RR_INPUT] handle={} content probe: not run, {} on {} frames whose camera is missing",
                     Handle()->Id,
                     depth == nullptr && normals == nullptr ? "the title published neither depth nor normals"
                     : depth == nullptr                     ? "the title published no depth"
                     : normals == nullptr                   ? "the title published no normals"
                                                            : "the render size was 0",
                     _contentProbe->unreadableFrames);
            _contentProbe->failed = true;
            _contentProbe->done = true;
        }
        return;
    }

    const UINT recordedBefore = _contentProbe->recorded;
    const XMUINT2 depthBase { _convDesc.FloorSourceBase.z, _convDesc.FloorSourceBase.w };
    const XMUINT2 normalsBase { _convDesc.InputBase1.x, _convDesc.InputBase1.y };
    if (!_contentProbe->Record(Device, commandList, depth, depthBase, normals, normalsBase, width, height,
                               static_cast<uint32_t>(_frameCount)))
    {
        _contentProbe->failed = true;
        _contentProbe->done = true;
        return;
    }

    if (recordedBefore == 0 && _contentProbe->recorded == 1)
        LOG_INFO("[RR_INPUT] handle={} content probe: the camera is missing, reading the title's depth and normals "
                 "on a {}x{} grid (logged once the GPU has written it)",
                 Handle()->Id, RRContentProbe::kGrid, RRContentProbe::kGrid);
}

void FSRDFeatureDx12::PollContentProbe()
{
    if (!_contentProbe || _contentProbe->done || _contentProbe->recorded == 0)
        return;

    if (const int slot = _contentProbe->Poll(); slot >= 0)
    {
        _contentProbe->Log(Handle()->Id, static_cast<UINT>(slot));
        _contentProbe->done = true;
        return;
    }

    if (++_contentProbe->polls >= RRContentProbe::kMaxPolls)
    {
        LOG_WARN("[RR_INPUT] handle={} content probe: no capture landed in {} evaluates; not logged",
                 Handle()->Id, RRContentProbe::kMaxPolls);
        _contentProbe->failed = true;
        _contentProbe->done = true;
    }
}

void FSRDFeatureDx12::LogRRStreamlineState(const NVSDK_NGX_Parameter& inParams, const char* stage)
{
    const SLSetConstantsDiagnostics sl = StreamlineHooks::getSetConstantsDiagnostics();
    const SLConstantsSnapshot snapshot = StreamlineHooks::getSLConstantsSnapshot();
    const RRD3D12SignalTagSnapshot tags = StreamlineHooks::getRRD3D12SignalTagSnapshot();
    const bool hasConstants = snapshot.frameIndex != UINT32_MAX;

    float ngxJitterX = 0.0f;
    float ngxJitterY = 0.0f;
    inParams.Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &ngxJitterX);
    inParams.Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &ngxJitterY);

    // Jitter changes every frame, so constants whose jitter equals NGX's belong to the same frame (Phase 1's rule).
    // A zero jitter on both sides matches trivially and proves nothing.
    const auto near1e3 = [](float a, float b) { return std::isfinite(a) && std::isfinite(b) && std::abs(a - b) < 1e-3f; };
    const float slJitterX = snapshot.constants.jitterOffset.x;
    const float slJitterY = snapshot.constants.jitterOffset.y;
    const char* jitterMatch = "n/a";
    if (hasConstants)
    {
        if (near1e3(slJitterX, 0.0f) && near1e3(slJitterY, 0.0f) && near1e3(ngxJitterX, 0.0f) &&
            near1e3(ngxJitterY, 0.0f))
            jitterMatch = "zero on both (not conclusive)";
        else if (near1e3(slJitterX, ngxJitterX) && near1e3(slJitterY, ngxJitterY))
            jitterMatch = "yes";
        else if (near1e3(slJitterX, -ngxJitterX) && near1e3(slJitterY, -ngxJitterY))
            jitterMatch = "yes (sign flipped)";
        else
            jitterMatch = "no";
    }

    const uint64_t presents = State::Instance().frameCount;
    const std::string age = sl.lastPresent == UINT64_MAX ? std::string("n/a")
                            : presents >= sl.lastPresent ? std::to_string(presents - sl.lastPresent)
                                                         : std::string("n/a");

    LOG_INFO("[RR_CAM] handle={} streamline ({}): evaluate={}, setConstantsHooked={} (sl1 {}), calls={} (sl1 {}), "
             "lastFrame={}, lastViewport={}, ageInPresents={}, fov={}, near={}, far={}, aspect={}, "
             "slJitter=({:.4f}, {:.4f}), ngxJitter=({:.4f}, {:.4f}), jitterMatch={}",
             Handle()->Id, stage, tags.activeEvaluationFrame == UINT32_MAX ? "NGX-direct" : "in slEvaluateFeature",
             sl.hooked ? 1 : 0, sl.sl1Hooked ? 1 : 0, sl.calls, sl.sl1Calls,
             hasConstants ? std::to_string(snapshot.frameIndex) : std::string("none"),
             snapshot.viewport != UINT32_MAX ? std::to_string(snapshot.viewport) : std::string("none"), age,
             snapshot.constants.cameraFOV, snapshot.constants.cameraNear, snapshot.constants.cameraFar,
             snapshot.constants.cameraAspectRatio, slJitterX, slJitterY, ngxJitterX, ngxJitterY, jitterMatch);
}

// Which optional DLSS-RR guides the title publishes, for deciding what a later step can use
// (the SSS guide above all). Read-only: nothing here is bound or changes any path.
void FSRDFeatureDx12::LogRRGuides(const NVSDK_NGX_Parameter& inParams, const char* stage) const
{
    int useHwDepth = 0;
    const bool hasUseHwDepth =
        inParams.Get(NVSDK_NGX_Parameter_Use_HW_Depth, &useHwDepth) == NVSDK_NGX_Result_Success;
    float preExposure = 0.0f;
    const bool hasPreExposure =
        inParams.Get(NVSDK_NGX_Parameter_DLSS_Pre_Exposure, &preExposure) == NVSDK_NGX_Result_Success;
    ID3D12Resource* exposureTexture = nullptr;
    const bool hasExposureTexture =
        TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_ExposureTexture, exposureTexture);

    // Streamline buffer type 58 is the same guide as a tag. The inventory keeps every type the
    // title has tagged this session, so no entry means never tagged (or no Streamline).
    // never-tagged can also mean the inventory was never populated at all: the SL tag probes
    // cache RRProbingWanted() (IsDenoiserReady(false)) on the first SetTag, and if that ran
    // before the denoiser DLL was loaded they stay off for the session. slObservedTypes=0
    // exposes that case, and the value gets an "(inventory empty)" suffix.
    const SLTagInventoryDiagnostics slInventory = StreamlineHooks::getSLTagInventoryDiagnostics();
    std::string slSssGuide = "never-tagged";
    for (const auto& entry : slInventory.resources)
    {
        if (entry.type != sl::kBufferTypeScreenSpaceSubsurfaceScatteringGuide)
            continue;

        if (!entry.resource.present)
        {
            slSssGuide = std::format("tagged-then-cleared (updates {})", entry.resource.updateCount);
        }
        else
        {
            const auto formatName = magic_enum::enum_name(entry.resource.format);
            slSssGuide = std::format("tagged {}x{} {} (updates {})", entry.resource.effectiveWidth,
                                     entry.resource.effectiveHeight,
                                     formatName.empty() ? "UNKNOWN" : formatName,
                                     entry.resource.updateCount);
        }
        break;
    }

    if (slInventory.resources.empty())
        slSssGuide += " (inventory empty)";

    LOG_INFO("[RR_GUIDES] stage={} exe={} sssGuide={} colorBeforeSSS={} colorAfterSSS={} "
             "gbufferSubsurface={} biasMask={} specHitDist={} specRayDirHitDist={} diffHitDist={} "
             "diffRayDirHitDist={} diffuseAlbedo={} specularAlbedo={} useHwDepth={} (creation={}, "
             "effective={}) preExposure={} exposureTex={} slTag58={} slObservedTypes={} disocclusionMask={} "
             "gbufferDisocclusionMask={}",
             stage, State::Instance().gameExe,
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceSubsurfaceScattering),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceSubsurfaceScattering),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_GBuffer_Subsurface),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DiffuseAlbedo),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_SpecularAlbedo),
             hasUseHwDepth ? (useHwDepth == NVSDK_NGX_DLSS_Depth_Type_HW ? "declared-hw" : "declared-linear")
                           : "absent",
             _hasNGXDepthType ? (_ngxReportedHWDepth ? "hardware" : "linear") : "absent",
             _appliedHardwareDepth < 0 ? "pending" : (_isHWDepth ? "hardware" : "linear"),
             hasPreExposure ? std::format("{:.3f}", preExposure) : std::string("absent"),
             hasExposureTexture ? "present" : "absent", slSssGuide, slInventory.resources.size(),
             // RR-16 (AMDNR 0.3.4): RE Requiem publishes DLSS.DisocclusionMask; nothing reads it yet (log only).
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_DLSS_DisocclusionMask),
             DescribeNGXResource(inParams, NVSDK_NGX_Parameter_GBuffer_DisocclusionMask));
}

// SKIN SMOOTHING AND THE SSS-GUIDE RANGE (AMDNR).
//
// The skin pass itself is FSRDSkinFilter.hlsl; this side decides whether it runs and on what.
// It runs only on a composed RR frame (no debug view), only when the title publishes
// DLSSD.ScreenSpaceSubsurfaceScatteringGuide in a usable form, and only when the player turned it
// (or its mask) on - so every other title, and this one by default, gets the composition output
// exactly as before, with no GPU work, no allocation and no read of the guide resource: while both
// switches are off only the guide's NGX slot is looked at, for the overlay. The guide range capture
// runs under the same switches. The guide is read like every other NGX input the conversion binds:
// as published, in the shader-readable state DLSS-RR requires of its inputs, from its subrect
// origin. The Streamline tag
// of the same guide (buffer type 58) is only inventoried by the hooks, not retained, so it is not
// bound here; the titles seen so far publish the NGX key as well (RE Requiem: same pointer).

// Range lines are logged process-wide: the first capture always, then up to this many captures
// that saw a non-zero guide (what the guide looks like on skin is the question the line answers).
// Captures are recorded only while the pass or its mask is on. Turning the mask view on also asks
// for one capture of the scene on screen, up to kSssRangeMaxLines lines in all - so a tester can
// log the skin classifiers where it matters.
static constexpr uint32_t kSssRangeNonZeroLines = 3;
static constexpr uint32_t kSssRangeMaxLines = 16;
static constexpr uint32_t kSssRangeInterval = 600; // composed guide frames between captures
static uint32_t s_sssRangeLines = 0;
static uint32_t s_sssRangeNonZeroLines = 0;

static bool IsSssGuideFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return true;
    default:
        return false;
    }
}

ID3D12Resource* FSRDFeatureDx12::AcquireSssGuide(const NVSDK_NGX_Parameter& inParams, XMUINT2& base)
{
    using namespace FSRD::RuntimeStatus;

    base = {};
    ID3D12Resource* published = nullptr;
    if (!TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide, published) ||
        published == nullptr)
    {
        SssGuide.store(0, std::memory_order_relaxed);
        SssGuideRejection.store(0, std::memory_order_relaxed);
        return nullptr;
    }

    const auto reject = [](int reason, const std::string& why) -> ID3D12Resource*
    {
        static bool logged[4] = {};
        if (reason >= 0 && reason < 4 && !logged[reason])
        {
            logged[reason] = true;
            LOG_WARN("[RR_SKIN] the title's {} is not usable, so skin smoothing and the guide range stay off: {}",
                     NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide, why);
        }

        SssGuide.store(0, std::memory_order_relaxed);
        SssGuideRejection.store(reason, std::memory_order_relaxed);
        return nullptr;
    };

    // The verdict below stands for this pointer while the pass is off (NoteSssGuidePresence).
    _sssGuideChecked = published;

    // The NGX table folds every pointer kind into one slot, so the pointer is queried rather than cast.
    Microsoft::WRL::ComPtr<ID3D12Resource> guide;
    if (FAILED(published->QueryInterface(IID_PPV_ARGS(guide.GetAddressOf()))) || !guide)
        return reject(1, "the pointer is not a D3D12 resource");

    base = GetSubrectBase(inParams, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide_Subrect_Base_X,
                          NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide_Subrect_Base_Y);

    const D3D12_RESOURCE_DESC desc = guide->GetDesc();
    const DXGI_FORMAT viewFormat = FSRD::GetViewFormat(desc.Format);
    if (!IsSssGuideFormat(viewFormat))
    {
        const auto formatName = magic_enum::enum_name(viewFormat);
        return reject(2, std::format("format {} is not a float or unorm colour format",
                                     formatName.empty() ? "UNKNOWN" : formatName));
    }

    if (!CoversSourceExtent(guide.Get(), base, RenderWidth(), RenderHeight()) || desc.DepthOrArraySize != 1)
    {
        return reject(3, std::format("{}x{} (arrays {}, samples {}) does not cover the render extent {}x{} from ({}, {})",
                                     desc.Width, desc.Height, desc.DepthOrArraySize, desc.SampleDesc.Count,
                                     RenderWidth(), RenderHeight(), base.x, base.y));
    }

    SssGuide.store(1, std::memory_order_relaxed);
    SssGuideRejection.store(0, std::memory_order_relaxed);
    SssGuideWidth.store(static_cast<uint32_t>(desc.Width), std::memory_order_relaxed);
    SssGuideHeight.store(desc.Height, std::memory_order_relaxed);

    // The title owns the resource for the whole evaluate; the query's reference is only the check.
    return guide.Get();
}

void FSRDFeatureDx12::NoteSssGuidePresence(const NVSDK_NGX_Parameter& inParams)
{
    using namespace FSRD::RuntimeStatus;

    ID3D12Resource* published = nullptr;
    if (!TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide, published) ||
        published == nullptr)
    {
        SssGuide.store(0, std::memory_order_relaxed);
        SssGuideRejection.store(0, std::memory_order_relaxed);
        _sssGuideChecked = nullptr;
        return;
    }

    // The last full check's verdict stands while the title publishes the same pointer; a pointer not
    // checked yet reads as published until the pass is turned on and AcquireSssGuide judges it.
    if (static_cast<const void*>(published) != _sssGuideChecked)
    {
        SssGuide.store(1, std::memory_order_relaxed);
        SssGuideRejection.store(0, std::memory_order_relaxed);
    }
}

// [FSR-RR] FfxDenoiserSkinSmoothingGuideLow / High, made safe for smoothstep: finite, 0..10, High above Low.
static void GetSkinGuideMapping(const Config& cfg, float& low, float& high)
{
    low = cfg.FfxDenoiserSkinSmoothingGuideLow.value_or_default();
    high = cfg.FfxDenoiserSkinSmoothingGuideHigh.value_or_default();
    low = std::isfinite(low) ? std::clamp(low, 0.0f, 10.0f) : 0.004f;
    high = std::isfinite(high) ? std::clamp(high, 0.0f, 10.0f) : 0.04f;
    if (!(high > low))
        high = low + 1e-4f;
}

// [FSR-RR] FfxDenoiserSkinSmoothingMovedLow / High, made safe for smoothstep: finite, 0..1, High above Low.
static void GetSkinMovedMapping(const Config& cfg, float& low, float& high)
{
    low = cfg.FfxDenoiserSkinSmoothingMovedLow.value_or_default();
    high = cfg.FfxDenoiserSkinSmoothingMovedHigh.value_or_default();
    low = std::isfinite(low) ? std::clamp(low, 0.0f, 1.0f) : 0.05f;
    high = std::isfinite(high) ? std::clamp(high, 0.0f, 1.0f) : 0.12f;
    if (!(high > low))
        high = low + 1e-4f;
}

void FSRDFeatureDx12::PollSssGuideStats()
{
    FSRDPreprocessor_Dx12::GuideStats stats;
    if (!FSRDConvShader || !FSRDConvShader->ReadGuideStats(stats))
        return;

    // A capture the mask view asked for is logged whatever it saw, and does not use up the periodic ones.
    const bool requested = _sssStatsPendingRequested;
    _sssStatsPendingRequested = false;

    const bool first = s_sssRangeLines == 0;
    if (!first && !requested && (stats.NonZero == 0 || s_sssRangeNonZeroLines >= kSssRangeNonZeroLines))
        return;

    ++s_sssRangeLines;
    if (stats.NonZero > 0 && !requested)
        ++s_sssRangeNonZeroLines;

    const double rcpPixels = stats.Pixels > 0 ? 100.0 / double(stats.Pixels) : 0.0;
    LOG_INFO("[RR_GUIDES] stage=sss-range sample={} exe={} guide over {}x{} ({} px): min={:.6g} max={:.6g} "
             "mean|g|={:.6g} nonZero={:.2f}% | over the non-zero pixels |g|/luminance(RR colour): mean={:.4g} "
             "max={:.4g} | own skin weight >= 0.5: {:.2f}% of the frame (skin weight = smoothstep({:.4g}, {:.4g}, "
             "|g|/luminance) - [FSR-RR] FfxDenoiserSkinSmoothingGuideLow / High - then a same-surface 5x5 max)",
             s_sssRangeLines, State::Instance().gameExe, stats.Width, stats.Height, stats.Pixels, stats.Min,
             stats.Max, stats.MeanAbs, double(stats.NonZero) * rcpPixels, stats.MeanRatio, stats.MaxRatio,
             double(stats.Weighted) * rcpPixels, stats.GuideLow, stats.GuideHigh);

    // What both classifiers make of the same frame - the robust one's flagged share is the number a
    // tester log checks: about the area of the faces and hands on screen, not most of the frame.
    const auto share = [](uint64_t part, uint64_t whole)
    { return whole > 0 ? 100.0 * double(part) / double(whole) : 0.0; };
    LOG_INFO("[RR_GUIDES] stage=skin-classifier sample={} trigger={} running={} | weight >= 0.5 (before the "
             "low-pass): robust={:.2f}% (above 0: {:.2f}%; without the hold: {:.2f}%; held from the last frame: "
             "{:.2f}%{}) guide-only(0.3.3)={:.2f}% of the frame | skin-tone "
             "albedo={:.2f}% | moved share |g|/max(L(colour)+|g|, L(RR colour)) on the skin-tone guide pixels "
             "({:.2f}% of the frame): mean={:.4g} >={:.3g}: {:.1f}% >={:.3g}: {:.1f}% coverage mean={:.3g}; on the "
             "other guide pixels ({:.2f}%): mean={:.4g} >={:.3g}: {:.1f}% >={:.3g}: {:.1f}% coverage mean={:.3g} | "
             "guide pixels with L(colour)-g below -2% of L(colour): {:.2f}%, L(colour)+g below it: {:.2f}% (the "
             "first near 0: the title's colour is the after-SSS one, in the guide's units) - [FSR-RR] "
             "FfxDenoiserSkinSmoothingClassifier / MovedLow / MovedHigh",
             s_sssRangeLines, requested ? "mask-on" : "periodic", stats.Robust ? "robust" : "guide-only(0.3.3)",
             share(stats.RobustFlagged, stats.Pixels), share(stats.RobustTouched, stats.Pixels),
             share(stats.RobustEntry, stats.Pixels), share(stats.Held, stats.Pixels),
             stats.HistoryValid ? "" : ", no previous frame",
             share(stats.LegacyFlagged, stats.Pixels), share(stats.SkinTone, stats.Pixels),
             share(stats.Skin.Pixels, stats.Pixels), stats.Skin.Mean, stats.MovedLow,
             share(stats.Skin.AboveLow, stats.Skin.Pixels), stats.MovedHigh,
             share(stats.Skin.AboveHigh, stats.Skin.Pixels), stats.Skin.MeanCoverage,
             share(stats.Rest.Pixels, stats.Pixels), stats.Rest.Mean, stats.MovedLow,
             share(stats.Rest.AboveLow, stats.Rest.Pixels), stats.MovedHigh,
             share(stats.Rest.AboveHigh, stats.Rest.Pixels), stats.Rest.MeanCoverage,
             share(stats.RawMinusGuideNegative, stats.NonZero), share(stats.RawPlusGuideNegative, stats.NonZero));
}

ID3D12Resource* FSRDFeatureDx12::ApplySkinSmoothing(ID3D12GraphicsCommandList* InCommandList,
                                                    const NVSDK_NGX_Parameter& inParams, ID3D12Resource* rawColor,
                                                    XMUINT2 rawColorBase)
{
    using namespace FSRD::RuntimeStatus;
    const auto& cfg = *Config::Instance();

    // A capture recorded while the pass was on is still read (and logged) after it is turned off;
    // with nothing pending this returns at once.
    PollSssGuideStats();

    const float strength = std::clamp(cfg.FfxDenoiserSkinSmoothingStrength.value_or_default(), 0.0f, 1.0f);
    // Strength 0 is off, not a pass that writes the picture back unchanged.
    const bool smooth = cfg.FfxDenoiserSkinSmoothing.value_or_default() && strength > 0.0f;
    const bool showMask = cfg.FfxDenoiserSkinSmoothingShowMask.value_or_default();
    // The robust classifier reads the title's colour; composition has just validated it.
    const bool robust = cfg.FfxDenoiserSkinSmoothingClassifier.value_or_default() != 0 && rawColor != nullptr;

    // Turning the mask view on asks for one guide capture of the scene on screen (see kSssRangeMaxLines).
    // Turning it off before the capture could be recorded (an earlier one still owed a read) drops the
    // request, so a later scene is never logged as the one the mask was turned on for.
    if (showMask && !_sssMaskWasOn && s_sssRangeLines < kSssRangeMaxLines)
        _sssStatsRequested = true;
    else if (!showMask)
        _sssStatsRequested = false;
    _sssMaskWasOn = showMask;

    // Whether this converter failed to create the pass, for the overlay's skin controls. A flag the
    // converter keeps on the CPU, so it is read with the pass off too: a new converter (the feature
    // recreated) clears it, and the overlay lets the player try again.
    SkinSmoothingUnavailable.store(FSRDConvShader && FSRDConvShader->IsSkinSmoothingUnavailable(),
                                   std::memory_order_relaxed);

    // Off (the default): no GPU work, no allocation, and the guide resource is not touched - only
    // its NGX slot, so the overlay can say whether the title publishes it.
    if (!smooth && !showMask)
    {
        SkinSmoothingActive.store(false, std::memory_order_relaxed);
        NoteSssGuidePresence(inParams);
        LogSkinNotRunning(kSkinStateOff);
        return nullptr;
    }

    XMUINT2 guideBase {};
    ID3D12Resource* guide = AcquireSssGuide(inParams, guideBase);
    if (guide == nullptr || !FSRDConvShader)
    {
        SkinSmoothingActive.store(false, std::memory_order_relaxed);
        LogSkinNotRunning(guide == nullptr ? kSkinStateNoGuide : kSkinStateFailed);
        return nullptr;
    }

    FSRDPreprocessor_Dx12::SkinSmoothingDesc desc =
    {
        .RenderSize = _convDesc.RenderSize,
        .InSssGuide = guide,
        .SssGuideBase = guideBase,
        .Strength = strength,
        .Radius = static_cast<uint32_t>(std::clamp(cfg.FfxDenoiserSkinSmoothingRadius.value_or_default(), 1, 16)),
        .Robust = robust,
        .InRawColor = rawColor,
        .RawColorBase = rawColorBase,
        .FrameIndex = static_cast<uint64_t>(_frameCount),
        .Smooth = smooth,
        .ShowMask = showMask,
        .ShowCues = showMask && robust && cfg.FfxDenoiserSkinSmoothingShowCues.value_or_default()
    };
    GetSkinGuideMapping(cfg, desc.GuideLow, desc.GuideHigh);
    GetSkinMovedMapping(cfg, desc.MovedLow, desc.MovedHigh);

    // The guide's range and what both classifiers make of it, for setting the thresholds from data:
    // read-only, a single reduction every kSssRangeInterval frames the pass runs on until the lines it
    // exists for have been logged, and one whenever the mask view is turned on.
    ++_sssGuideFrames;
    const bool periodicCapture =
        s_sssRangeNonZeroLines < kSssRangeNonZeroLines && _sssGuideFrames >= _sssStatsNextFrame;
    if ((periodicCapture || _sssStatsRequested) && FSRDConvShader->RecordGuideStats(InCommandList, desc))
    {
        ++_sssStatsCaptures;
        // A requested capture on a frame the periodic one was due is the requested one; the periodic
        // capture stays due and is taken once this one has been read.
        _sssStatsPendingRequested = _sssStatsRequested;
        if (periodicCapture && !_sssStatsRequested)
            _sssStatsNextFrame = _sssGuideFrames + kSssRangeInterval;
        _sssStatsRequested = false;
    }

    if (!FSRDConvShader->DispatchSkinSmoothing(InCommandList, desc))
    {
        SkinSmoothingActive.store(false, std::memory_order_relaxed);
        // The dispatch is where the pass is created, so a creation failure shows here first.
        SkinSmoothingUnavailable.store(FSRDConvShader->IsSkinSmoothingUnavailable(), std::memory_order_relaxed);
        LogSkinNotRunning(kSkinStateFailed);
        return nullptr;
    }

    SkinSmoothingActive.store(true, std::memory_order_relaxed);

    // Logged when the pass starts and when its shape changes; the strength slider and the thresholds
    // move freely. Every change to a state in which it does not run is logged by LogSkinNotRunning.
    const int state = (desc.Smooth ? 1 : 0) | (desc.ShowMask ? 2 : 0) | static_cast<int>(desc.Radius << 2) |
                      (desc.Robust ? 1 << 7 : 0) | (desc.ShowCues ? 1 << 8 : 0);
    if (state != _loggedSkinState)
    {
        _loggedSkinState = state;
        const auto steps = FSRDPreprocessor_Dx12::GetSkinSmoothingSteps(desc.Radius);
        const D3D12_RESOURCE_DESC guideDesc = guide->GetDesc();
        const auto formatName = magic_enum::enum_name(FSRD::GetViewFormat(guideDesc.Format));
        LOG_INFO("[RR_SKIN] skin smoothing (AMDNR, experimental; diffuse term only): {} strength={:.2f} radius={} "
                 "({} a-trous iteration(s), strides {}/{}/{}) classifier={} guideLow={:.4g} guideHigh={:.4g} "
                 "movedLow={:.4g} movedHigh={:.4g} guide={}x{} {} base=({}, {}) render={}x{} frame={}",
                 desc.ShowCues ? "mask view (robust classifier cues)" : desc.ShowMask ? "mask view" : "on",
                 desc.Strength, desc.Radius, steps.Iterations, steps.Step[0],
                 steps.Iterations > 1 ? steps.Step[1] : 0, steps.Iterations > 2 ? steps.Step[2] : 0,
                 desc.Robust ? "robust (guide x moved-share coverage x skin-tone albedo, held over frames)"
                             : "guide-only (0.3.3)",
                 desc.GuideLow, desc.GuideHigh, desc.MovedLow, desc.MovedHigh, guideDesc.Width, guideDesc.Height,
                 formatName.empty() ? "UNKNOWN" : formatName, guideBase.x, guideBase.y, RenderWidth(),
                 RenderHeight(), _frameCount);
    }

    return FSRDConvShader->GetSkinSmoothingOutput();
}

void FSRDFeatureDx12::LogSkinNotRunning(int state)
{
    if (state == _loggedSkinState)
        return;

    _loggedSkinState = state;

    switch (state)
    {
    case kSkinStateOff:
        LOG_INFO("[RR_SKIN] skin smoothing and its mask view off: FSR SR reads RR's composition as it is "
                 "frame={}", _frameCount);
        break;
    case kSkinStateNoGuide:
        LOG_INFO("[RR_SKIN] skin smoothing or its mask view is on but not running: the title publishes no "
                 "usable {} (a published guide that is not usable is explained once, above) frame={}",
                 NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide, _frameCount);
        break;
    default:
        LOG_WARN("[RR_SKIN] skin smoothing or its mask view is on but not running: the pass could not run "
                 "(the [RR_SKIN] error before this line has the cause{}) frame={}",
                 FSRDConvShader && FSRDConvShader->IsSkinSmoothingUnavailable()
                     ? "; it stays off until the feature is recreated"
                     : "",
                 _frameCount);
        break;
    }
}

// The six temporal keys in RRLiveSettings::temporal order, with their path-traced profile index.
static constexpr const char* kRRTemporalNames[6] = { "disoc", "stab", "normal", "gauss", "clipK", "maxRad" };
static constexpr uint32_t kRRTemporalProfileIndex[6] = { 9, 4, 5, 6, 7, 8 };

static std::array<CustomOptional<float>*, 6> RRTemporalKeys(Config& cfg)
{
    return { &cfg.FfxDenoiserDisocThreshold, &cfg.FfxDenoiserStabilityBias, &cfg.FfxDenoiserCrossBlNormStr,
             &cfg.FfxDenoiserGaussKernRelax,  &cfg.FfxDenoiserRadianceClip,  &cfg.FfxDenoiserMaxRadiance };
}

static std::string RRDebugModeName(uint64_t mode)
{
    for (const auto& entry : kDebugModes)
    {
        if (entry.second == mode)
            return entry.first;
    }
    return std::format("{:#x}", mode);
}

void FSRDFeatureDx12::TrackRRLiveSettings()
{
    auto& cfg = *Config::Instance();
    const auto keys = RRTemporalKeys(cfg);

    // What the configure pass asks RR for this frame (DispatchDenoiser's updateConfiguration rule).
    RRLiveSettings current {};
    current.amdDefaults = cfg.FfxDenoiserUseAmdDefaults.value_or_default();
    current.debugMode = cfg.FfxDenoiserDebugMode.value_or_default();
    const float amdValues[6] = {
        _denoiserAmdDefaults.m_DisocclusionThreshold,      _denoiserAmdDefaults.m_StabilityBias,
        _denoiserAmdDefaults.m_CrossBilateralNormalStrength, _denoiserAmdDefaults.m_GaussianKernelRelaxation,
        _denoiserAmdDefaults.m_RadianceClipStdK,           _denoiserAmdDefaults.m_MaxRadiance,
    };
    for (size_t i = 0; i < current.temporal.size(); ++i)
        current.temporal[i] = current.amdDefaults ? amdValues[i] : keys[i]->value_or_default();

    if (!_hasLoggedRRSettings)
    {
        _hasLoggedRRSettings = true;
        _loggedRRSettings = current;
        _pendingRRSettings = current;
        return;
    }

    if (!(current == _pendingRRSettings))
    {
        _pendingRRSettings = current;
        _pendingRRSettingsFrames = 0;
        return;
    }

    if (current == _loggedRRSettings || ++_pendingRRSettingsFrames < kRRSettingsSettleFrames)
        return;

    std::string changes;
    if (current.amdDefaults != _loggedRRSettings.amdDefaults)
        changes += std::format(" amdDefaults {} -> {}", _loggedRRSettings.amdDefaults ? "on" : "off",
                               current.amdDefaults ? "on" : "off");
    for (size_t i = 0; i < current.temporal.size(); ++i)
    {
        if (current.temporal[i] != _loggedRRSettings.temporal[i])
            changes += std::format(" {} {:.4g} -> {:.4g} ({})", kRRTemporalNames[i], _loggedRRSettings.temporal[i],
                                   current.temporal[i], TemporalValueSource(cfg, *keys[i], kRRTemporalProfileIndex[i]));
    }
    if (current.debugMode != _loggedRRSettings.debugMode)
        changes += std::format(" debugView {} -> {}", RRDebugModeName(_loggedRRSettings.debugMode),
                               RRDebugModeName(current.debugMode));

    LOG_INFO("[RR_CFG] settings changed:{} frame={}", changes, _frameCount);
    _loggedRRSettings = current;
}

std::string FSRDFeatureDx12::DescribeRRTemporal() const
{
    auto& cfg = *Config::Instance();
    const auto keys = RRTemporalKeys(cfg);
    const float configured[6] = {
        _denoiserSettings.m_DisocclusionThreshold,      _denoiserSettings.m_StabilityBias,
        _denoiserSettings.m_CrossBilateralNormalStrength, _denoiserSettings.m_GaussianKernelRelaxation,
        _denoiserSettings.m_RadianceClipStdK,           _denoiserSettings.m_MaxRadiance,
    };

    std::string text = "temporal=[";
    for (size_t i = 0; i < std::size(configured); ++i)
        text += std::format("{}{} {:.4g} ({})", i == 0 ? "" : ", ", kRRTemporalNames[i], configured[i],
                            TemporalValueSource(cfg, *keys[i], kRRTemporalProfileIndex[i]));
    text += ']';
    return text;
}

std::string FSRDFeatureDx12::DescribeRRSignals() const
{
    const auto& cfg = *Config::Instance();
    const char* specularSource = cfg.FfxDenoiserSpecularSignalType.has_value() ? "ini"
                                 : _autoSpecularSignalResolved              ? "auto"
                                                                            : "auto, not resolved yet";
    return std::format("specular={} ({}) signalFlags={:#x}", GetSignalTypeName(_specularSignalDescType),
                       specularSource, _denoiserCtxDesc.signalFlags);
}

float FSRDFeatureDx12::DefaultSharpnessWhenTitleSendsNone() const
{
    // The path-traced profile keeps its contract: nothing sharpens RR's output unless the player overrides it.
    // (AMDNR 0.3.4.1, Proton RR) Neither does Wine/Proton (hooks/RrHardwareGate.h); Windows keeps kRrDefaultSharpness.
    // (AMDNR 0.3.4.2, RN3) The Windows value is [Sharpness] RrDefaultSharpness, whose default is
    // kRrDefaultSharpness: an unset ini is the shipped number, byte for byte as 0.3.4.1. Clamped here as well as in
    // IFeature::GetSharpness, so an out-of-range ini value never reaches the [RR_POST] line or the menu's tag.
    const bool pathTracedProfile = Config::Instance()->FfxDenoiserPathTracedProfile.value_or_default();
    const float windowsDefault = std::clamp(Config::Instance()->RrDefaultSharpness.value_or_default(), 0.0f, 1.0f);
    return RrGate::DefaultSharpnessWhenTitleSendsNone(pathTracedProfile, State::Instance().isRunningOnLinux,
                                                      windowsDefault);
}

FSRDFeatureDx12::RRPostState FSRDFeatureDx12::CaptureRRPostState(const ffxDispatchDescUpscale& upscalerDesc,
                                                                 bool profileSuppressedSharpness,
                                                                 bool skinSmoothed) const
{
    const auto& cfg = *Config::Instance();

    RRPostState state {};
    state.rcasSwitch = cfg.RcasEnabled.value_or_default();
    state.profileSuppressed = profileSuppressedSharpness;
    state.rrDefaultSharpness = _sharpnessIsFeatureDefault;
    state.fsrSharpening = upscalerDesc.enableSharpening;
    state.fsrSharpness = upscalerDesc.enableSharpening ? upscalerDesc.sharpness : 0.0f;
    state.overrideSharpness = cfg.OverrideSharpness.value_or_default();

    // The RCAS rule of LogRRPost below.
    state.rcasSharpness = _actualSharpness.value_or(_sharpness);
    state.motionSharpening =
        cfg.MotionSharpnessEnabled.value_or_default() && cfg.MotionSharpness.value_or_default() > 0.0f;
    state.rcas = RCAS != nullptr && RCAS->CanRender() &&
                 ((state.rcasSwitch && state.rcasSharpness != 0.0f) || state.motionSharpening);
    if (!state.rcas)
        state.rcasSharpness = 0.0f;

    state.skinSmoothed = skinSmoothed;
    if (skinSmoothed)
    {
        state.skinShowMask = cfg.FfxDenoiserSkinSmoothingShowMask.value_or_default();
        state.skinStrength = cfg.FfxDenoiserSkinSmoothingStrength.value_or_default();
        state.skinRadius = cfg.FfxDenoiserSkinSmoothingRadius.value_or_default();
        state.skinClassifier = cfg.FfxDenoiserSkinSmoothingClassifier.value_or_default();
    }

    state.sssGuide = FSRD::RuntimeStatus::SssGuide.load(std::memory_order_relaxed);
    state.biasStrength = cfg.FfxDenoiserBiasMaskStrength.value_or_default();
    state.biasMask = _biasMaskState;
    return state;
}

void FSRDFeatureDx12::LogRRPost(const NVSDK_NGX_Parameter& inParams, const ffxDispatchDescUpscale& upscalerDesc,
                                bool profileSuppressedSharpness, float titleSharpness, bool skinSmoothed,
                                const char* reason)
{
    const auto& cfg = *Config::Instance();
    const bool overrideSharpness = cfg.OverrideSharpness.value_or_default();

    // FSR SR's own sharpening of RR's output, as this frame configured it.
    // (AMDNR 0.3.4.1) The source names AMDNR's RR default when the title sends none (GetSharpness);
    // RCAS also replaces it when motion adaptive sharpening alone made Evaluate hand the value to RCAS.
    const char* rrDefaultSource = "AMDNR's RR default (the game sends none)";
    std::string sharpening;
    if (cfg.RcasEnabled.value_or_default() || _sharpnessHandedToRcas)
        sharpening = "off (RCAS replaces it)";
    else if (profileSuppressedSharpness)
        sharpening = std::format("off (path-traced profile; the title asked {:.2f})", titleSharpness);
    else if (upscalerDesc.enableSharpening)
        sharpening = std::format("on {:.2f} (source: {})", upscalerDesc.sharpness,
                                 overrideSharpness            ? "[Sharpness] OverrideSharpness"
                                 : _sharpnessIsFeatureDefault ? rrDefaultSource
                                                              : "the title's NGX sharpness");
    else
        sharpening = std::format("off (source: {} is 0)",
                                 overrideSharpness ? "[Sharpness] Sharpness" : "the title's NGX sharpness");

    // (AMDNR 0.3.4.2, RN2) A [Sharpness] Sharpness the ini kept while OverrideSharpness is off is inert: the value
    // above is what sharpens, and ticking Override hands the stored number to FSR SR at once (one report carries
    // 1.00, four times the RR default). Naming it here answers "my sharpness slider does nothing" from the log
    // alone. The stored value is read the way SaveIni reads it, so an explicit 0 counts as stored.
    if (!overrideSharpness)
    {
        if (const auto stored = Config::Instance()->Sharpness.value_for_config_ignore_default(); stored.has_value())
            sharpening += std::format(" ([Sharpness] Sharpness {:.2f} is stored and inert: it waits for"
                                      " OverrideSharpness)",
                                      stored.value());
    }

    // RCAS after FSR SR, decided the way IFeature_Dx12::Evaluate decides it for FSR_RR: the player's
    // switch with a non-zero sharpness, or motion adaptive sharpening, which needs it - and only when
    // RCAS can render (initialised, and its target made by Evaluate's setup before this evaluate ran).
    // The strength is the one Evaluate settled on and kept in _actualSharpness for RCAS: when RCAS
    // runs, Evaluate has already set the NGX sharpness (and _sharpness) to 0 so the upscaler does not
    // sharpen twice, so reading either of those here would say 'off' while RCAS sharpens.
    const float rcasSharpness = _actualSharpness.value_or(_sharpness);
    const bool motionSharpening =
        cfg.MotionSharpnessEnabled.value_or_default() && cfg.MotionSharpness.value_or_default() > 0.0f;
    const bool rcas = RCAS != nullptr && RCAS->CanRender() &&
                      ((cfg.RcasEnabled.value_or_default() && rcasSharpness != 0.0f) || motionSharpening);
    const std::string rcasText =
        rcas ? std::format("on {:.2f}{}{}", rcasSharpness, motionSharpening ? " + motion adaptive" : "",
                           _sharpnessIsFeatureDefault && !overrideSharpness
                               ? std::format(" (source: {})", rrDefaultSource)
                               : std::string())
             : std::string("off");

    // Which mask FSR SR was handed, by the resource it was handed.
    ID3D12Resource* fsrReactive = nullptr;
    ID3D12Resource* fsrTransparency = nullptr;
    ID3D12Resource* dlssBias = nullptr;
    TryGetNGXVoidPointer(inParams, OptiKeys::FSR_Reactive, fsrReactive);
    TryGetNGXVoidPointer(inParams, OptiKeys::FSR_TransparencyAndComp, fsrTransparency);
    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, dlssBias);

    const void* reactive = upscalerDesc.reactive.resource;
    const void* transparency = upscalerDesc.transparencyAndComposition.resource;

    const std::string reactiveText =
        reactive == nullptr ? std::string("none")
        : reactive == fsrReactive ? std::string("the title's FSR reactive mask")
        : reactive == dlssBias ? std::string("the DLSS bias mask as published")
        : std::format("the DLSS bias mask through the Bias pass ([FSR] DlssReactiveMaskBias {:.2f})",
                      cfg.DlssReactiveMaskBias.value_or_default());
    const char* transparencyText =
        transparency == nullptr ? "none"
        : transparency == fsrTransparency ? "the title's FSR transparency mask"
        : transparency == dlssBias ? "the DLSS bias mask ([FSR] UseReactiveMaskForTransparency)"
        : "another resource";

    const int sssGuide = FSRD::RuntimeStatus::SssGuide.load(std::memory_order_relaxed);
    const std::string skinText =
        skinSmoothed ? (cfg.FfxDenoiserSkinSmoothingShowMask.value_or_default()
                            ? std::string("mask view")
                            : std::format("on (strength {:.2f}, radius {}, {} classifier)",
                                          cfg.FfxDenoiserSkinSmoothingStrength.value_or_default(),
                                          cfg.FfxDenoiserSkinSmoothingRadius.value_or_default(),
                                          cfg.FfxDenoiserSkinSmoothingClassifier.value_or_default() != 0
                                              ? "robust"
                                              : "guide-only"))
                     : std::string("off");

    LOG_INFO("[RR_POST] exe={} after Ray Regeneration: fsrSharpening={} rcas={} reactive={} transparency={} "
             "rrBiasMaskStrength={:.2f} ({}) skinSmoothing={} sssGuide={} ({}, frame={})",
             State::Instance().gameExe, sharpening, rcasText, reactiveText, transparencyText,
             cfg.FfxDenoiserBiasMaskStrength.value_or_default(), DescribeBiasMask(_biasMaskState), skinText,
             sssGuide > 0 ? "present" : (sssGuide == 0 ? "absent" : "not looked at (debug view)"), reason,
             _frameCount);
}

void FSRDFeatureDx12::AccountHistoryResets()
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(HistoryResetReason::Count); i++)
    {
        if ((_historyResetReasonsThisFrame & (1u << i)) != 0)
            ++_historyResetFrames[i];
    }
    _historyResetReasonsThisFrame = 0;

    if (++_historyWindowFrames < kHistoryWindowFrames)
        return;

    const auto count = [this](HistoryResetReason reason) { return _historyResetFrames[static_cast<size_t>(reason)]; };
    LOG_INFO("[RR_DIAG] history window ({} frames): frames with an RR history reset by reason: ngxReset={} "
             "renderSize={} inputNotReady={} profileToggle={} debugView={} setting={} other={}; RR dispatches={} "
             "(reset flag on {}); render size {}x{} .. {}x{}",
             _historyWindowFrames, count(HistoryResetReason::NgxReset), count(HistoryResetReason::RenderSize),
             count(HistoryResetReason::InputNotReady), count(HistoryResetReason::ProfileToggle),
             count(HistoryResetReason::DebugView), count(HistoryResetReason::Setting), count(HistoryResetReason::Other),
             _historyWindowDispatches, _historyWindowResetDispatches, _historyWindowMinWidth,
             _historyWindowMinHeight, _historyWindowMaxWidth, _historyWindowMaxHeight);

    _historyResetFrames = {};
    _historyWindowFrames = 0;
    _historyWindowDispatches = 0;
    _historyWindowResetDispatches = 0;
    _historyWindowMinWidth = 0;
    _historyWindowMinHeight = 0;
    _historyWindowMaxWidth = 0;
    _historyWindowMaxHeight = 0;
}

bool FSRDFeatureDx12::ConvertDenoiserBuffers(ID3D12GraphicsCommandList* InCommandList)
{
    const auto& cfg = *Config::Instance(); 
    const auto dbgMode = static_cast<DebugModes>(cfg.FfxDenoiserDebugMode.value_or_default());
    // Prepare input converter
    _convDesc.RenderSize = 
    { 
        (float) RenderWidth(), (float) RenderHeight(), 
        1.0f / (float) RenderWidth(), 1.0f / (float) RenderHeight()
    };
    

    if (_convDesc.MotionVectorsJittered)
        _convDesc.Flags |= (uint32_t)FSRDConvFlags::MotionVectorsJittered;
    if (_convDesc.DisplayResolutionMotion)
        _convDesc.Flags |= (uint32_t)FSRDConvFlags::DisplayResolutionMotion;
    // The packing shader selects its debug view from the flag word's debug bits
    // (GetDebugMode) and writes it to the specular signal output, which is what
    // the debug blit shows. Nothing else carried the user's selection there, so
    // every conversion debug mode rendered the packed signal instead. Non-
    // conversion modes mask to zero here, leaving the pass untouched.
    _convDesc.Flags |= (uint32_t) GetConvDebugFlags(dbgMode);
    const bool normalsInViewSpace = cfg.FfxDenoiserNormalsInViewSpace.value_or_default();
    if (normalsInViewSpace)
        _convDesc.Flags |= (uint32_t)FSRDConvFlags::NormalsViewSpace;

    const int appliedNormalsInViewSpace = normalsInViewSpace ? 1 : 0;
    if (_appliedNormalsInViewSpace != appliedNormalsInViewSpace)
    {
        if (_appliedNormalsInViewSpace >= 0)
        {
            LOG_INFO(
                "[RR_INPUT] normal space changed: {}->{}; resetting denoiser history",
                _appliedNormalsInViewSpace != 0 ? "view" : "world",
                normalsInViewSpace ? "view" : "world");
            InvalidateDenoiserHistory(HistoryResetReason::Setting);
        }
        _appliedNormalsInViewSpace = appliedNormalsInViewSpace;
    }
    if (_specularSignalDescType == FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR)
        _convDesc.Flags |= (uint32_t)FSRDConvFlags::SpecularSignalIndirect;
    _convDesc.FloorIsolation = std::clamp(cfg.FfxDenoiserFloorIsolation.value_or_default(), 0.0f, 1.0f);
    _convDesc.RoughnessFloor = std::clamp(
        cfg.FfxDenoiserRoughnessFloor.value_or_default(), 0.0f, 1.0f);
    _convDesc.FloorHandoverMode = static_cast<uint32_t>(
        std::clamp(cfg.FfxDenoiserFloorHandover.value_or_default(), 0, 2));
    _convDesc.FloorHandoverStrength =
        std::clamp(cfg.FfxDenoiserFloorHandoverStrength.value_or_default(), 0.0f, 1.0f);
    _convDesc.FloorRawBlend =
        std::clamp(cfg.FfxDenoiserFloorRawBlend.value_or_default(), 0.0f, 1.0f);
    // Only while the path-traced profile is in effect: a FloorIsolation / FloorRawBlend set by
    // hand without the profile is not re-routed. 0 leaves the conversion unchanged.
    _convDesc.ProfileTextureRoute =
        _appliedPathTracedProfile == 1
            ? std::clamp(cfg.FfxDenoiserProfileTextureRoute.value_or_default(), 0.0f, 1.0f)
            : 0.0f;
    _convDesc.FloorStructureGate =
        std::clamp(cfg.FfxDenoiserFloorStructureGate.value_or_default(), 0.0f, 1.0f);
    _convDesc.DemodDivisorFloor =
        std::clamp(cfg.FfxDenoiserDemodDivisorFloor.value_or_default(), 1e-4f, 0.5f);
    _convDesc.FloorClampSmoothing =
        std::clamp(cfg.FfxDenoiserFloorClampSmoothing.value_or_default(), 0.0f, 1.0f);
    _convDesc.FloorHandoverDetail = std::clamp(
        cfg.FfxDenoiserFloorHandoverDetail.value_or_default(), 0.0f, 1.0f);

    _convDesc.Resources.InInspector = nullptr;
    _convDesc.InspectorChannel = ResTrack_Dx12::GetRRResourceChannel();
    _convDesc.InspectorScale = ResTrack_Dx12::GetRRResourceViewScale();
    // Same clamp as the RR debug-bounds configure key below and as the menu slider.
    // They previously disagreed - max(v, 1.0) here against clamp(v, 0.001, 1024)
    // there - so any value under 1.0 normalized OptiScaler's own NormDepth view
    // differently from AMD's, and the two could not be compared.
    _convDesc.DebugDepthMax =
        std::clamp(cfg.FfxDenoiserDebugDepthMax.value_or_default(), 0.001f, 1024.0f);

    Microsoft::WRL::ComPtr<ID3D12Resource> inspectorResource;
    if (ResTrack_Dx12::IsRRResourceInspectorEnabled())
    {
        ResTrack_Dx12::NotifyRRResourceInspectorFrame(_frameCount);
        ResTrack_Dx12::RefreshRRResourceCandidates(RenderWidth(), RenderHeight());
        if (dbgMode == DebugModes::ResourceInspector)
        {
            inspectorResource.Attach(ResTrack_Dx12::AcquireRRResourceCandidate());
            _convDesc.Resources.InInspector = inspectorResource.Get();
        }
    }

    if (_convDesc.RoughnessFloor != _appliedRoughnessFloor)
    {
        if (_appliedRoughnessFloor >= 0.0f)
        {
            LOG_INFO(
                "[RR_DIAG] RR roughness floor changed: floor {:.4f}->{:.4f}; "
                "resetting denoiser history",
                _appliedRoughnessFloor, _convDesc.RoughnessFloor);
        }
        else
        {
            LOG_INFO("[RR_DIAG] RR roughness floor initialized: floor={:.4f}",
                     _convDesc.RoughnessFloor);
        }

        _appliedRoughnessFloor = _convDesc.RoughnessFloor;
        InvalidateDenoiserHistory(HistoryResetReason::Setting);
    }

    if (_roughnessSource == RoughnessSource::Packed)
        _convDesc.Flags |= (uint32_t) FSRDConvFlags::IsRoughnessPacked;

    if (_convDesc.Resources.InSpecHitDist)
        _convDesc.Flags |= (uint32_t)FSRDConvFlags::HasSpecHitDistance;
    if (_convDesc.Resources.InEmissive)
        _convDesc.Flags |= (uint32_t) FSRDConvFlags::HasEmissiveInput;

    // Flags follow the resources that were actually validated this frame, so an unavailable
    // or rejected input leaves the shader on its derived path instead of reading a texture
    // that was never bound.
    if (_convDesc.Resources.InTitleLinearDepth != nullptr)
        _convDesc.Flags |= (uint32_t) FSRDConvFlags::TitleLinearDepth;
    if (_convDesc.Resources.InResponsivityMask != nullptr)
        _convDesc.Flags |= (uint32_t) FSRDConvFlags::HasResponsivityMask;

    _convDesc.ResponsivityTrustThreshold =
        std::max(cfg.FfxDenoiserResponsivityThreshold.value_or_default(), 0.0f);
    _convDesc.ResponsivityInvert = cfg.FfxDenoiserResponsivityInvert.value_or_default();
    _convDesc.DiagnosticsEnabled = cfg.FfxDenoiserDiagnostics.value_or_default();

    _convDesc.BiasMaskStrength = cfg.FfxDenoiserBiasMaskStrength.value_or_default();
    _convDesc.FloorDetailBoost = cfg.FfxDenoiserFloorDetailBoost.value_or_default();
    _convDesc.FloorNormalSharpness = cfg.FfxDenoiserFloorNormalSharpness.value_or_default();
    _convDesc.FloorAlbedoGuide = cfg.FfxDenoiserFloorAlbedoGuide.value_or_default();
    _convDesc.FloorLumSymmetry = cfg.FfxDenoiserFloorLumSymmetry.value_or_default();
    _convDesc.FloorGrazingSharpness = cfg.FfxDenoiserFloorGrazingSharpness.value_or_default();
    _convDesc.FloorEnvelopeBias = cfg.FfxDenoiserFloorEnvelopeBias.value_or_default();
    _convDesc.FloorSoftMin = cfg.FfxDenoiserFloorSoftMin.value_or_default();

    StoreHlslColumnVectorMatrix(_convDesc.InvViewMatrix, _invViewMatrix);

    // Inverse perspective projection
    const XMMATRIX invProjMatrix = XMMatrixInverse(nullptr, _projMatrix);
    StoreHlslColumnVectorMatrix(_convDesc.InvProjMatrix, invProjMatrix);

    // Previous world to view for linear depth delta
    StoreHlslColumnVectorMatrix(_convDesc.PrevViewMatrix, _prevViewMatrix);

    // Near and far planes
    const ViewPlanes planes = GetViewPlanes(_projMatrix, DepthInverted());
    _convDesc.NearPlane = planes.nearPlane;
    _convDesc.FarPlane = planes.farPlane;
    _isRightHanded = planes.isRightHanded;

    // Every geometric quantity downstream - linearised depth, world-position
    // reconstruction and the far-plane skip test - is derived from these two numbers
    // and the raw projection terms behind them. Report them directly rather than inferring
    // them from debug-view colours, which cannot distinguish a depth clamped to
    // near from one clamped to far. Logged only when the values change.
    {
        static float loggedNear = -1.0f;
        static float loggedFar = -1.0f;
        if (planes.nearPlane != loggedNear || planes.farPlane != loggedFar)
        {
            loggedNear = planes.nearPlane;
            loggedFar = planes.farPlane;
            LOG_INFO(
                "[RR_DIAG] view planes: near={}, far={}, infinite={}, rightHanded={}, "
                "depthInverted={}, hardwareDepth={}, projectionFromStreamline={}, "
                "clipA={}, clipB={}, clipW={}",
                planes.nearPlane, planes.farPlane, planes.isInfinite,
                planes.isRightHanded, DepthInverted(), _isHWDepth,
                _projectionFromStreamline,
                _projMatrix.r[2].m128_f32[2], _projMatrix.r[2].m128_f32[3],
                _projMatrix.r[3].m128_f32[2]);
        }
    }

    if (_isRightHanded)
        _convDesc.Flags |= (uint32_t)FSRDConvFlags::RightHanded;

    ApplyDepthInterpretation();

    // What this frame's view-space positions are actually built from. It has to be read here,
    // after ApplyDepthInterpretation and the right-handed flag, because both are part of the
    // definition - and it has to be the *effective* outcome of AcquireOptionalInputs rather
    // than the option that drives it. Enabling the option on a title that publishes no tagged
    // field leaves the definition untouched, and a title's tag becoming usable or unusable
    // changes it with no menu interaction at all, which is the half of this that a config
    // comparison cannot see.
    const DepthDefinition depthDefinition =
    {
        .titleLinearDepth = _convDesc.Resources.InTitleLinearDepth != nullptr,
        .rightHanded = _isRightHanded
    };

    if (_hasAppliedDepthDefinition && depthDefinition != _appliedDepthDefinition)
    {
        LOG_INFO("[RR_INPUT] depth definition changed: {} (right-handed {}) -> {} "
                 "(right-handed {}); resetting denoiser history",
                 _appliedDepthDefinition.titleLinearDepth ? "title linear depth" : "derived depth",
                 _appliedDepthDefinition.rightHanded,
                 depthDefinition.titleLinearDepth ? "title linear depth" : "derived depth",
                 depthDefinition.rightHanded);
        InvalidateDenoiserHistory(HistoryResetReason::Setting);
    }
    _appliedDepthDefinition = depthDefinition;
    _hasAppliedDepthDefinition = true;

    // Every check that resets history - the normal space and roughness floor above, the depth
    // interpretation just above this, and the depth definition in between - runs after
    // PrepareDenoiseConvInput already froze the reprojection inputs from the value
    // _hasDenoiserHistory had at the time. Re-derive them once here, after the last of those
    // producers and before the conversion dispatch that consumes them, or the conversion pass
    // reprojects against a previous-frame depth produced under the setting that was just
    // abandoned on the very frame the RR dispatch resets. The denoiser dispatch that follows
    // reads _hasDenoiserHistory for its own reset flag, so it sees the same decision.
    RefreshHistoryDerivedInputs();

    // Per-frame lines stay behind the switch: they say nothing a shipped build needs, and a
    // session of them is a large part of a gigabyte of log.
    if (_convDesc.DiagnosticsEnabled)
        LOG_DEBUG("Dispatching FSRD Input Converter");

    // Dispatch resource converter. Outputs are automatically transitioned for reading.
    if (!FSRDConvShader->DispatchConversion(InCommandList, _convDesc))
    {
        _convDesc.Resources.InInspector = nullptr;
        return false;
    }

    // From here on the converter's textures are referenced by a command list the
    // application will submit, so they must not be released out from under it.
    _preprocessorHasRecordedWork = true;

    _convDesc.Resources.InInspector = nullptr;
    return true;
}

static bool ValidateRRDispatchChain(ID3D12GraphicsCommandList* commandList,
                                    const ffxDispatchDescDenoiser& dispatchDesc,
                                    ffxStructType_t expectedDiffuseType,
                                    ffxStructType_t expectedSpecularType,
                                    bool expectDiffuseSignal,
                                    bool expectSpecularSignal,
                                    bool expectedAmbientOcclusion)
{
    if (!commandList || dispatchDesc.commandList != commandList)
    {
        LOG_ERROR("RR 1.2 dispatch has a null or mismatched D3D12 command list");
        return false;
    }

    if (dispatchDesc.header.type != FFX_API_DISPATCH_DESC_TYPE_DENOISER)
    {
        LOG_ERROR("RR 1.2 dispatch head has an invalid descriptor type: {0:X}", dispatchDesc.header.type);
        return false;
    }

    bool foundDiffuse = false;
    bool foundSpecular = false;
    bool foundAmbientOcclusion = false;
    bool foundDebugView = false;

    for (const ffxDispatchDescHeader* signal = dispatchDesc.header.pNext;
         signal != nullptr; signal = signal->pNext)
    {
        if (signal->type == FFX_API_DISPATCH_DESC_TYPE_DENOISER_DEBUG_VIEW)
        {
            if (foundDebugView || signal->pNext)
            {
                LOG_ERROR("RR 1.2 debug descriptor must appear once at the chain tail");
                return false;
            }

            foundDebugView = true;
            continue;
        }

        bool* found = nullptr;
        if (signal->type == expectedDiffuseType)
            found = &foundDiffuse;
        else if (signal->type == expectedSpecularType)
            found = &foundSpecular;
        else if (signal->type == FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION)
            found = &foundAmbientOcclusion;
        else
        {
            LOG_ERROR("RR 1.2 dispatch contains unexpected descriptor {} ({:#x})",
                      GetSignalTypeName(signal->type), signal->type);
            return false;
        }

        if (*found)
        {
            LOG_ERROR("RR 1.2 dispatch contains duplicate {} descriptor",
                      GetSignalTypeName(signal->type));
            return false;
        }
        *found = true;
    }

    if (foundDiffuse != expectDiffuseSignal || foundSpecular != expectSpecularSignal ||
        foundAmbientOcclusion != expectedAmbientOcclusion)
    {
        LOG_ERROR("RR 1.2 dispatch signal mismatch: diffuse={}, specular={}, AO={}; "
                  "expected diffuse={}, specular={}, AO={}",
                  foundDiffuse, foundSpecular, foundAmbientOcclusion,
                  expectDiffuseSignal, expectSpecularSignal, expectedAmbientOcclusion);
        return false;
    }

    return true;
}

bool FSRDFeatureDx12::DispatchDenoiser(ID3D12GraphicsCommandList* InCommandList,
                                       const ffxDispatchDescDenoiser& dispatchDesc)
{
    auto& state = State::Instance();
    const auto& cfg = *Config::Instance();

    if (!_pDenoiserCtx)
    {
        LOG_ERROR("RR 1.2 dispatch attempted without a denoiser context");
        return false;
    }


    if (!ValidateRRDispatchChain(InCommandList, dispatchDesc,
                                 _diffuseSignalDescType, _specularSignalDescType,
                                 _denoiseDiffuse, _denoiseSpecular,
                                 _ambientOcclusionEnabled))
        return false;

    const ffxDispatchDescHeader* diffuseHeader = nullptr;
    const ffxDispatchDescHeader* specularHeader = nullptr;
    const ffxDispatchDescHeader* ambientOcclusionHeader = nullptr;
    for (const ffxDispatchDescHeader* signal = dispatchDesc.header.pNext;
         signal != nullptr; signal = signal->pNext)
    {
        if (signal->type == _diffuseSignalDescType)
            diffuseHeader = signal;
        else if (signal->type == _specularSignalDescType)
            specularHeader = signal;
        else if (signal->type == FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION)
            ambientOcclusionHeader = signal;
    }

    // All four RR 1.2 diffuse/specular descriptor structures have the same
    // header + signal ABI. The headers stay nullable: a single-signal chain
    // legitimately omits one, so references are formed only where that
    // signal's presence is known.
    const auto* ambientOcclusion = ambientOcclusionHeader
        ? reinterpret_cast<const ffxDispatchDescDenoiserAmbientOcclusion*>(ambientOcclusionHeader)
        : nullptr;
    const bool resetRequested = !!(dispatchDesc.flags & FFX_DENOISER_DISPATCH_RESET);
    const bool resetTransition = resetRequested && !_lastDispatchRequestedReset;
    // RR-28 (AMDNR 0.3.4): the ~60-line snapshot (inventories, tag and probe diagnostics) was logged on every reset
    // transition; it is the same after the first few, so resets 1-3 and every 20th log it (the one-line "reset
    // dispatch succeeded" below still names every reset). The feature's first dispatch always logs it.
    if (resetTransition)
        ++_resetTransitionCount;
    const bool resetSnapshot =
        resetTransition && (_resetTransitionCount <= 3u || (_resetTransitionCount % 20u) == 0u);
    const bool logDispatchSnapshot = _logNextDenoiserDispatch || resetSnapshot;

    if (logDispatchSnapshot)
    {
        // Single-signal mode: LogRRDispatchSnapshot's resource inventory and the
        // per-signal probe battery both assume the two-signal chain; something in
        // there throws under the SL VEH when the diffuse descriptor is absent
        // (hundreds of swallowed dumps per session, dispatch never reached). The
        // chain shape is the information that matters, so log it directly here.
        if (_denoiseDiffuse && _denoiseSpecular)
        {
            // The validated chain carries both signals here, so the header
            // dereference below is sound.
            const auto& directDiffuse =
                *reinterpret_cast<const ffxDispatchDescDenoiserDirectDiffuse*>(diffuseHeader);
            const auto& indirectSpecular =
                *reinterpret_cast<const ffxDispatchDescDenoiserIndirectSpecular*>(specularHeader);
            LogRRDispatchSnapshot(dispatchDesc, directDiffuse, indirectSpecular, ambientOcclusion);
        }
        else
        {
            LOG_INFO("[RR_DIAG] single-signal dispatch: head={:#x} -> {} -> tail=0x0, frame={}, reset={}",
                     dispatchDesc.header.type, GetSignalTypeName(_denoiseDiffuse ? _diffuseSignalDescType : _specularSignalDescType),
                     dispatchDesc.frameIndex, !!(dispatchDesc.flags & FFX_DENOISER_DISPATCH_RESET));
        }
        if (_denoiseDiffuse && _denoiseSpecular)
        {
        LogRRDiffuseHitDistanceProbe(
            NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance,
            _diffuseHitDistanceProbe,
            _diffuseHitDistanceBaseX,
            _diffuseHitDistanceBaseY,
            dispatchDesc.renderSize.width,
            dispatchDesc.renderSize.height,
            false);
        LogRRDiffuseHitDistanceProbe(
            NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance,
            _diffuseRayDirectionHitDistanceProbe,
            _diffuseRayDirectionHitDistanceBaseX,
            _diffuseRayDirectionHitDistanceBaseY,
            dispatchDesc.renderSize.width,
            dispatchDesc.renderSize.height,
            true);
        LogRREmissiveProbe(
            _emissiveProbe,
            _emissiveProbeCompatible,
            _emissiveProbeFromStreamline,
            dispatchDesc.renderSize.width,
            dispatchDesc.renderSize.height);
        LogRRGBufferIdentityProbe(
            NVSDK_NGX_Parameter_GBuffer_MaterialId,
            _materialIdProbe,
            dispatchDesc.renderSize.width,
            dispatchDesc.renderSize.height);
        LogRRGBufferIdentityProbe(
            NVSDK_NGX_Parameter_GBuffer_ShadingModelId,
            _shadingModelIdProbe,
            dispatchDesc.renderSize.width,
            dispatchDesc.renderSize.height);
        StreamlineHooks::logRRSignalTagDiagnostics(
            dispatchDesc.renderSize.width, dispatchDesc.renderSize.height);
        StreamlineHooks::logSLTagInventoryDiagnostics(
            dispatchDesc.renderSize.width, dispatchDesc.renderSize.height);
        StreamlineHooks::logRRNGXPointerDiagnostics();
        ResTrack_Dx12::LogRRScalarResourceCandidates(
            dispatchDesc.renderSize.width, dispatchDesc.renderSize.height);
        LOG_INFO(
            "[RR_DIAG] conversion snapshot: viewSource={}, projectionSource={}, handedness={}, depthInput={}, "
            "depthDirection={}, motionResolution={}, roughness={}, roughnessFloor={:.4f}, "
            "zeroRoughnessMaterialType=unified-type-1, "
            "specularHitDistance={} (invalid={}), diffuseHitDistance={}, diffuseDirectionHitDistance={}, "
            "emissiveInput={} (previewCompatible={})",
            _viewFromStreamline ? "Streamline" : "NGX",
            _projectionFromStreamline ? "StreamlineReconstruction" : "NGX",
            _isRightHanded ? "RH" : "LH",
            _isHWDepth ? "hardware" : "linear",
            DepthInverted() ? "reversed-Z" : "standard-Z",
            LowResMV() ? "render" : "output",
            _roughnessSource == RoughnessSource::Packed ? "packed" : "separate",
            _convDesc.RoughnessFloor,
            (_convDesc.Resources.InSpecHitDist ||
             _convDesc.Resources.InSpecularRayDirectionHitDistance)
                ? "present"
                : "absent",
            _specularSignalDescType == FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR
                ? "negative"
                : "zero",
            _diffuseHitDistanceProbe ? "present" : "absent",
            _diffuseRayDirectionHitDistanceProbe ? "present" : "absent",
            _emissiveProbe ? "present" : "absent",
            _emissiveProbeCompatible);
        }
    }

    // A/B reference: push AMD's queried baseline instead of the fork's tuned values.
    // The INI/menu values are left untouched so toggling back restores them, and the
    // comparison below re-applies whichever source just became active.
    const bool useAmdDefaults = cfg.FfxDenoiserUseAmdDefaults.value_or_default();

    const auto updateConfiguration = [this, useAmdDefaults](
                                         const CustomOptional<float>& cfgValue, float amdDefault,
                                         float& currentValue, FfxApiConfigureDenoiserKey key)
    {
        const float requestedValue = useAmdDefaults ? amdDefault : cfgValue.value_or_default();
        if (requestedValue == currentValue)
            return true;

        const float previousValue = currentValue;
        currentValue = requestedValue;

        const ffxReturnCode_t result = ApplyConfiguration(key);
        if (result == FFX_API_RETURN_OK)
            return true;

        // Retry on the next frame instead of treating the failed value as applied.
        currentValue = previousValue;
        static RRRepeatedErrorGate configureGate;
        if (const uint64_t n = configureGate.Next())
            LOG_ERROR("[RR_DIAG] RR 1.2 configure key {} failed: {} ({})", static_cast<uint64_t>(key),
                      FfxApiProxy::ReturnCodeToString(result), RRRepeatNote(n));
        return false;
    };

    if (!updateConfiguration(cfg.FfxDenoiserDisocThreshold, _denoiserAmdDefaults.m_DisocclusionThreshold,
                             _denoiserSettings.m_DisocclusionThreshold,
                             FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD) ||
        !updateConfiguration(cfg.FfxDenoiserCrossBlNormStr, _denoiserAmdDefaults.m_CrossBilateralNormalStrength,
                             _denoiserSettings.m_CrossBilateralNormalStrength,
                             FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH) ||
        !updateConfiguration(cfg.FfxDenoiserStabilityBias, _denoiserAmdDefaults.m_StabilityBias,
                             _denoiserSettings.m_StabilityBias,
                             FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS) ||
        !updateConfiguration(cfg.FfxDenoiserMaxRadiance, _denoiserAmdDefaults.m_MaxRadiance,
                             _denoiserSettings.m_MaxRadiance,
                             FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE) ||
        !updateConfiguration(cfg.FfxDenoiserRadianceClip, _denoiserAmdDefaults.m_RadianceClipStdK,
                             _denoiserSettings.m_RadianceClipStdK,
                             FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K) ||
        !updateConfiguration(cfg.FfxDenoiserGaussKernRelax, _denoiserAmdDefaults.m_GaussianKernelRelaxation,
                             _denoiserSettings.m_GaussianKernelRelaxation,
                             FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION))
    {
        return false;
    }

    const float requestedDebugDepthMax =
        std::clamp(cfg.FfxDenoiserDebugDepthMax.value_or_default(), 0.001f, 1024.0f);
    if (requestedDebugDepthMax != _denoiserSettings.m_DebugViewLinearDepthBounds.max)
    {
        const FfxApiFloatBounds previousBounds = _denoiserSettings.m_DebugViewLinearDepthBounds;
        _denoiserSettings.m_DebugViewLinearDepthBounds.max = requestedDebugDepthMax;

        const ffxReturnCode_t result =
            ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS);
        if (result != FFX_API_RETURN_OK)
        {
            _denoiserSettings.m_DebugViewLinearDepthBounds = previousBounds;
            LOG_ERROR("[RR_DIAG] RR 1.2 debug linear-depth bounds configure failed: {}",
                      FfxApiProxy::ReturnCodeToString(result));
            return false;
        }
    }

    ID3D12InfoQueue* infoQueue = nullptr;
    uint64_t firstD3D12Message = 0;
    const bool hasScopedInfoQueue =
        logDispatchSnapshot && Device &&
        SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&infoQueue)));

    if (hasScopedInfoQueue)
    {
        firstD3D12Message = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    }
    else if (logDispatchSnapshot)
    {
        LOG_INFO(
            "[RR_DIAG][D3D12] InfoQueue unavailable. Dispatch return codes and device-removal reason are still "
            "captured; enable the D3D12 debug layer before device creation for validation messages.");
    }

    ++_denoiserDispatchAttempts;
    if (_convDesc.DiagnosticsEnabled)
    {
        LOG_DEBUG("Dispatching FSR-RR 1.2 frame {} with {} + {}",
                  dispatchDesc.frameIndex,
                  GetSignalTypeName(_diffuseSignalDescType),
                  GetSignalTypeName(_specularSignalDescType));
    }
    const ffxReturnCode_t result = FfxApiProxy::D3D12_Dispatch(&_pDenoiserCtx, &dispatchDesc.header);
    _lastDispatchRequestedReset = resetRequested;

    if (hasScopedInfoQueue)
    {
        const std::string scope = std::format("frame {}", dispatchDesc.frameIndex);
        LogRRD3D12Messages(infoQueue, firstD3D12Message, scope);
    }

    if (result != FFX_API_RETURN_OK)
    {
        ++_denoiserDispatchFailures;
        LOG_ERROR(
            "[RR_DIAG] dispatch failed: frame={}, result={}, attempts={}, successes={}, failures={}",
            dispatchDesc.frameIndex, FfxApiProxy::ReturnCodeToString(result),
            _denoiserDispatchAttempts, _denoiserDispatchSuccesses, _denoiserDispatchFailures);

        if (!hasScopedInfoQueue && Device &&
            SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            const uint64_t messageCount = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
            const uint64_t firstRecentMessage = messageCount > 32u ? messageCount - 32u : 0u;
            const std::string scope = std::format("recent messages after failed frame {}", dispatchDesc.frameIndex);
            LogRRD3D12Messages(infoQueue, firstRecentMessage, scope);
        }

        if (Device)
        {
            const HRESULT removedReason = Device->GetDeviceRemovedReason();
            if (removedReason == S_OK)
            {
                LOG_INFO("[RR_DIAG] D3D12 device remains operational after the failed RR dispatch");
            }
            else
            {
                LOG_ERROR("[RR_DIAG] D3D12 device is removed: HRESULT={:#x}",
                          static_cast<uint32_t>(removedReason));
                Util::GetDeviceRemovedReason(Device);
            }
        }

        if (result == FFX_API_RETURN_ERROR_RUNTIME_ERROR)
        {
            LOG_WARN("Trying to recover by recreating the feature");
            state.changeBackend[Handle()->Id] = true;
        }

        if (infoQueue)
            infoQueue->Release();

        return false;
    }

    ++_denoiserDispatchSuccesses;
    ++_historyWindowDispatches;
    if (resetRequested)
        ++_historyWindowResetDispatches;

    // RR-19: Ray Regeneration runs; a fallback mark or reason left from before goes (logs only when one did).
    ClearRRFallback(Handle()->Id, true);

    // Per-dispatch contract, sampled regularly enough to land next to the input
    // readback. It carries the two values the input probe cannot observe directly:
    // how far the camera actually moved, and the jitter pair the reprojection is
    // aligned against. Both are read here rather than inferred, because a reset or a
    // bypassed frame forces the depth delta in the motion vectors to zero and would
    // otherwise read as "the camera never moved".
    if ((_denoiserDispatchSuccesses % 60u) == 0u)
    {
        const XMFLOAT3 cameraPosition = GetFloat3Column(_invViewMatrix, 3);
        LOG_INFO(
            "[RR_CFG] frame={}, reset={}, render={}x{}, depthBounds=[{:.4f}, {:.4f}], "
            "jitter=[{:.6f}, {:.6f}] (prev [{:.6f}, {:.6f}]), cameraDelta=[{:.6f}, {:.6f}, {:.6f}], "
            "cameraPosition=[{:.6f}, {:.6f}, {:.6f}], "
            "depthInterpretation={}, resetFrame={}, historyValid={}, viewSource={}, "
            "signals={} + {} (diffuseEnabled={}, specularEnabled={}), {}",
            dispatchDesc.frameIndex, resetRequested,
            dispatchDesc.renderSize.width, dispatchDesc.renderSize.height,
            dispatchDesc.linearDepthBounds.min, dispatchDesc.linearDepthBounds.max,
            dispatchDesc.jitterOffsets.x, dispatchDesc.jitterOffsets.y,
            _convDesc.JitterOffsets.z, _convDesc.JitterOffsets.w,
            dispatchDesc.cameraPositionDelta.x, dispatchDesc.cameraPositionDelta.y,
            dispatchDesc.cameraPositionDelta.z,
            cameraPosition.x, cameraPosition.y, cameraPosition.z,
            _isHWDepth ? "hardware" : "linear", _isInReset, _hasDenoiserHistory,
            _viewFromStreamline ? "Streamline" : "NGX",
            GetSignalTypeName(_diffuseSignalDescType), GetSignalTypeName(_specularSignalDescType),
            _denoiseDiffuse, _denoiseSpecular, DescribeRRTemporal());
    }

    if (_logNextDenoiserDispatch)
    {
        LOG_INFO(
            "[RR_DIAG] first dispatch succeeded: frame={}, context={:X}, attempts={}, reset={}",
            dispatchDesc.frameIndex, reinterpret_cast<uintptr_t>(_pDenoiserCtx),
            _denoiserDispatchAttempts, resetRequested);
        _logNextDenoiserDispatch = false;
    }
    else if (resetTransition)
    {
        LOG_INFO("[RR_DIAG] reset dispatch succeeded: frame={}, reset {} of this feature{}", dispatchDesc.frameIndex,
                 _resetTransitionCount,
                 resetSnapshot ? "" : " (its input snapshot is logged for resets 1-3 and every 20th)");
    }
    else if (_denoiserDispatchSuccesses == 60u || (_denoiserDispatchSuccesses % 600u) == 0u)
    {
        LOG_INFO("[RR_DIAG] dispatch stability milestone: {} successful dispatches, {} failures",
                 _denoiserDispatchSuccesses, _denoiserDispatchFailures);
    }

    if (infoQueue)
        infoQueue->Release();

    return true;
}

void FSRDFeatureDx12::CommitDenoiserHistory() noexcept
{
    _lastCamPos = GetFloat3Column(_invViewMatrix, 3);
    _prevViewMatrix = _viewMatrix;
    _previousDenoiserJitter = {
        _convDesc.JitterOffsets.x,
        _convDesc.JitterOffsets.y
    };
    _hasDenoiserHistory = true;
}

void FSRDFeatureDx12::RefreshHistoryDerivedInputs() noexcept
{
    // PrepareDenoiseConvInput derives these from _hasDenoiserHistory while it runs, and it
    // runs before the change checks in ConvertDenoiserBuffers that can invalidate it. Without
    // this the conversion pass is told its reprojection history is valid on the frame that
    // just abandoned it, and reprojects against a previous-frame depth produced under the
    // previous setting - which is the one frame the reset exists to avoid.
    if (!_hasDenoiserHistory || _isInReset)
    {
        _convDesc.MotionHistoryValid = false;
        _convDesc.JitterOffsets.z = _convDesc.JitterOffsets.x;
        _convDesc.JitterOffsets.w = _convDesc.JitterOffsets.y;
    }
}

// IEEE 754 half -> float for the probe readback (no DirectXPackedVector here).
static float ProbeHalfToFloat(uint16_t h)
{
    const uint32_t sign = (h >> 15) & 1u;
    uint32_t exponent = (h >> 10) & 0x1Fu;
    uint32_t mantissa = h & 0x3FFu;

    uint32_t bits = 0;
    if (exponent == 0u)
    {
        if (mantissa == 0u)
        {
            bits = sign << 31;
        }
        else
        {
            exponent = 1u;
            while ((mantissa & 0x400u) == 0u)
            {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x3FFu;
            bits = (sign << 31) | ((exponent - 15u + 127u) << 23) | (mantissa << 13);
        }
    }
    else if (exponent == 0x1Fu)
    {
        bits = (sign << 31) | 0x7F800000u | (mantissa << 13);
    }
    else
    {
        bits = (sign << 31) | ((exponent - 15u + 127u) << 23) | (mantissa << 13);
    }

    float result = 0.0f;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

bool FSRDFeatureDx12::SetDefaultConfiguration()
{
    for (int i = 0; i < DenoiserConfiguration::kKeyCount; i++)
    {
        const FfxApiConfigureDenoiserKey key = DenoiserConfiguration::GetIndexKey(i);
        const ffxReturnCode_t result = SetDefaultConfiguration(key);
        if (result != FFX_API_RETURN_OK)
        {
            LOG_ERROR("RR 1.2 default query for key {} failed: {}", static_cast<uint64_t>(key),
                      FfxApiProxy::ReturnCodeToString(result));
            return false;
        }
    }

    return true;
}

ffxReturnCode_t FSRDFeatureDx12::SetDefaultConfiguration(FfxApiConfigureDenoiserKey key)
{
    void* data = _denoiserSettings.GetData(key);
    if (!data)
        return FFX_API_RETURN_ERROR_PARAMETER;

    ffxQueryDescDenoiserGetDefaultKeyValue queryDesc = 
    {
        .header = { .type = FFX_API_QUERY_DESC_TYPE_DENOISER_GET_DEFAULT_KEYVALUE }, 
        .key = (uint64_t)key, 
        .count = 1u,
        .data = data
    };

    const ffxReturnCode_t code = FfxApiProxy::D3D12_Query(&_pDenoiserCtx, &queryDesc.header);
    return code;
}

ffxReturnCode_t FSRDFeatureDx12::ApplyConfiguration(FfxApiConfigureDenoiserKey key)
{
    const void* data = _denoiserSettings.GetData(key);
    if (!data)
        return FFX_API_RETURN_ERROR_PARAMETER;

    ffxConfigureDescDenoiserKeyValue configureDesc =
    {
        .header = { .type = FFX_API_CONFIGURE_DESC_TYPE_DENOISER_KEYVALUE }, 
        .key = (uint64_t)key, 
        .count = 1u,
        .data = data
    };

    const ffxReturnCode_t code = FfxApiProxy::D3D12_Configure(&_pDenoiserCtx, &configureDesc.header);
    return code;
}
