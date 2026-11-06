// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#include <pch.h>
#include <Config.h>
#include <Util.h>
#include <dlssnr/amd/AmdBridge.h>
#include <proxies/FfxApi_Proxy.h>
#include "FFXFeature_Dx12.h"
#include "MathUtils.h"

using namespace OptiMath;

NVSDK_NGX_Parameter* FFXFeatureDx12::SetParameters(NVSDK_NGX_Parameter* InParameters)
{
    InParameters->Set("OptiScaler.SupportsUpscaleSize", true);
    return InParameters;
}

FFXFeatureDx12::FFXFeatureDx12(unsigned int InHandleId, NVSDK_NGX_Parameter* InParameters)
    : FFXFeature(InHandleId, InParameters), IFeature_Dx12(InHandleId, InParameters),
      IFeature(InHandleId, SetParameters(InParameters))
{
    FfxApiProxy::InitFfxDx12();

    _moduleLoaded = FfxApiProxy::IsSRReady();

    if (_moduleLoaded)
        LOG_INFO("amd_fidelityfx_dx12.dll methods loaded!");
    else
        LOG_ERROR("can't load amd_fidelityfx_dx12.dll methods!");
}

bool FFXFeatureDx12::InitInternal(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters)
{
    LOG_DEBUG("FFXFeatureDx12::Init");

    if (IsInited())
        return true;

    return InitFFX(InParameters);
}

bool CreateBufferResourceWithSize(ID3D12Device* device, ID3D12Resource* source, D3D12_RESOURCE_STATES state,
                                  ID3D12Resource** target, UINT width, UINT height)
{
    if (device == nullptr || source == nullptr)
        return false;

    auto inDesc = source->GetDesc();

    if (*target != nullptr)
    {
        auto bufDesc = (*target)->GetDesc();

        if (bufDesc.Width != width || bufDesc.Height != height || bufDesc.Format != inDesc.Format ||
            bufDesc.Flags != inDesc.Flags)
        {
            (*target)->Release();
            (*target) = nullptr;
        }
        else
        {
            return true;
        }
    }

    D3D12_HEAP_PROPERTIES heapProperties;
    D3D12_HEAP_FLAGS heapFlags;
    HRESULT hr = source->GetHeapProperties(&heapProperties, &heapFlags);

    if (hr != S_OK)
    {
        LOG_ERROR("GetHeapProperties result: {:X}", (UINT64) hr);
        return false;
    }

    inDesc.Width = width;
    inDesc.Height = height;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &inDesc, state, nullptr,
                                         IID_PPV_ARGS(target));

    if (hr != S_OK)
    {
        LOG_ERROR("CreateCommittedResource result: {:X}", (UINT64) hr);
        return false;
    }

    LOG_DEBUG("Created new one: {}x{}", inDesc.Width, inDesc.Height);

    return true;
}

namespace
{
// Typed depth formats exist only with ALLOW_DEPTH_STENCIL; an ordinary texture takes their typeless form
DXGI_FORMAT PlainTextureFormat(DXGI_FORMAT InFormat)
{
    switch (InFormat)
    {
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_R32G8X24_TYPELESS;
    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_R16_TYPELESS;
    default:
        return InFormat;
    }
}

// Keeps *InTexture when it already has InDesc's size, format, flags, mips and array size, otherwise replaces it.
// Upstream OptiScaler hands the old one to DeferredRelease (copies recorded for earlier frames may still read it on
// the GPU); this tree has no DeferredRelease yet, so it is released here like smallerColor in
// CreateBufferResourceWithSize.
bool EnsureTexture(ID3D12Device* InDevice, ID3D12Resource** InTexture, const D3D12_RESOURCE_DESC& InDesc,
                   bool& OutCreated)
{
    OutCreated = false;

    if (*InTexture != nullptr)
    {
        auto current = (*InTexture)->GetDesc();

        if (current.Width == InDesc.Width && current.Height == InDesc.Height && current.Format == InDesc.Format &&
            current.Flags == InDesc.Flags && current.MipLevels == InDesc.MipLevels &&
            current.DepthOrArraySize == InDesc.DepthOrArraySize)
        {
            return true;
        }

        (*InTexture)->Release();
        *InTexture = nullptr;
    }

    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    const auto hr = InDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &InDesc,
                                                      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr,
                                                      IID_PPV_ARGS(InTexture));

    if (hr != S_OK || *InTexture == nullptr)
    {
        LOG_WARN("Sub-rectangle copy texture {}x{} format {} could not be created: {:X}", InDesc.Width, InDesc.Height,
                 (UINT) InDesc.Format, (UINT) hr);
        *InTexture = nullptr;
        return false;
    }

    OutCreated = true;
    return true;
}
} // namespace

// Triple-screen and split-screen games render every view into one texture and give each view's place in it through
// the DLSS input sub-rectangle parameters (Assetto Corsa Competizione's triple screen: three 1280x720 views side by
// side in a 3840-wide target, one upscaler context per view). The FFX API has no offset for its inputs, so every view
// was upscaled from the first one and all three screens showed the left view. A view that does not start at (0, 0) is
// copied into a texture of its own here; like the padding workaround in EvaluateInternal, the source is expected in
// NON_PIXEL_SHADER_RESOURCE (the state the FFX dispatch reads it in) and is left there.
//
// The copy is always an ordinary texture, also for a depth-stencil source: into a depth-stencil scratch, RDNA 4 copied
// wrong texels for depth-only formats (D32, D16) while the same box copy into an ordinary texture was exact. A box on a
// depth-stencil copy source needs programmable sample positions; without them the source is first copied whole into
// InWhole, and a view whose caller passes no InWhole stays uncropped.
ID3D12Resource* FFXFeatureDx12::CropToSubrect(ID3D12GraphicsCommandList* InCommandList, ID3D12Resource* InSource,
                                              ID3D12Resource** InScratch, ID3D12Resource** InWhole, UINT InX,
                                              UINT InY, UINT InWidth, UINT InHeight, const char* InName)
{
    if (InSource == nullptr || (InX == 0 && InY == 0) || InWidth == 0 || InHeight == 0)
        return InSource;

    const auto desc = InSource->GetDesc();

    if ((UINT64) InX + InWidth > desc.Width || (UINT64) InY + InHeight > desc.Height)
    {
        LOG_WARN("{} sub-rectangle {}x{} at ({}, {}) is outside the {}x{} texture, using the texture as is", InName,
                 InWidth, InHeight, InX, InY, desc.Width, desc.Height);
        return InSource;
    }

    const bool depthStencil = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0;

    if (depthStencil && !depthStencilBoxCopy.has_value())
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS2 options {};
        depthStencilBoxCopy =
            Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options, sizeof(options)) == S_OK &&
            options.ProgrammableSamplePositionsTier != D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;
    }

    const bool copyWholeFirst = depthStencil && !depthStencilBoxCopy.value_or(false);

    if (copyWholeFirst && InWhole == nullptr)
        return InSource;

    auto plainDesc = desc;
    plainDesc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
    plainDesc.Format = PlainTextureFormat(desc.Format);
    plainDesc.Alignment = 0;
    plainDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    auto scratchDesc = plainDesc;
    scratchDesc.Width = InWidth;
    scratchDesc.Height = InHeight;
    scratchDesc.DepthOrArraySize = 1;
    scratchDesc.MipLevels = 1;

    bool created = false;

    if (!EnsureTexture(Device, InScratch, scratchDesc, created))
        return InSource;

    if (created)
    {
        LOG_INFO("{} is taken from a {}x{} sub-rectangle at ({}, {}) of its texture{}", InName, InWidth, InHeight, InX,
                 InY, copyWholeFirst ? ", copying the texture whole first" : "");
    }

    if (copyWholeFirst && !EnsureTexture(Device, InWhole, plainDesc, created))
        return InSource;

    ID3D12Resource* boxSource = InSource;

    ResourceBarrier(InCommandList, InSource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_COPY_SOURCE);

    if (copyWholeFirst)
    {
        ResourceBarrier(InCommandList, *InWhole, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                        D3D12_RESOURCE_STATE_COPY_DEST);
        InCommandList->CopyResource(*InWhole, InSource);
        ResourceBarrier(InCommandList, *InWhole, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
        boxSource = *InWhole;
    }

    ResourceBarrier(InCommandList, *InScratch, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_BOX srcBox {};
    srcBox.left = InX;
    srcBox.top = InY;
    srcBox.front = 0;
    srcBox.right = InX + InWidth;
    srcBox.bottom = InY + InHeight;
    srcBox.back = 1;

    D3D12_TEXTURE_COPY_LOCATION dstLocation {};
    dstLocation.pResource = *InScratch;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLocation {};
    srcLocation.pResource = boxSource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    InCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, &srcBox);

    ResourceBarrier(InCommandList, InSource, D3D12_RESOURCE_STATE_COPY_SOURCE,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    if (copyWholeFirst)
    {
        ResourceBarrier(InCommandList, *InWhole, D3D12_RESOURCE_STATE_COPY_SOURCE,
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    ResourceBarrier(InCommandList, *InScratch, D3D12_RESOURCE_STATE_COPY_DEST,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    return *InScratch;
}

bool FFXFeatureDx12::EvaluateInternal(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters)
{
    LOG_FUNC();

    auto& cfg = *Config::Instance();

    struct ffxDispatchDescUpscale params = { 0 };
    params.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;

    params.flags = 0;

    if (Config::Instance()->FsrDebugView.value_or_default())
    {
        params.flags |= FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW;
    }

    if (Config::Instance()->FsrNonLinearPQ.value_or_default())
        params.flags |= FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_PQ;
    else if (Config::Instance()->FsrNonLinearSRGB.value_or_default())
        params.flags |= FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB;

    InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &params.jitterOffset.x);
    InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &params.jitterOffset.y);

    params.enableSharpening = _sharpness > 0.0f;
    params.sharpness = _sharpness;

    // Force enable RCAS when in FSR4 debug view mode
    // it crashes when sharpening is disabled
    // Debug view expects RCAS output (now sure why)
    if (Version() >= feature_version { 4, 0, 2 } && Config::Instance()->FsrDebugView.value_or_default() &&
        !params.enableSharpening)
    {
        params.enableSharpening = true;
        params.sharpness = 0.01f;
    }

    LOG_DEBUG("Jitter Offset: {0}x{1}", params.jitterOffset.x, params.jitterOffset.y);

    unsigned int reset;
    InParameters->Get(NVSDK_NGX_Parameter_Reset, &reset);
    params.reset = (reset == 1);

    GetRenderResolution(InParameters, &params.renderSize.width, &params.renderSize.height);

    LOG_DEBUG("Input Resolution: {0}x{1}", params.renderSize.width, params.renderSize.height);

    params.commandList = InCommandList;

    // The AMD replacement arrives in COMPUTE_READ and must remain there.
    // Game-specific barriers describe the original texture, not this scratch UAV.
    const bool amdReplacement = DlssNr::AmdBridge::HasReplacement(InParameters);
    ID3D12Resource* paramColor = nullptr;
    if (InParameters->Get(NVSDK_NGX_Parameter_Color, &paramColor) != NVSDK_NGX_Result_Success)
        InParameters->Get(NVSDK_NGX_Parameter_Color, (void**) &paramColor);
    // The colour as the caller handed it (AMDNR C3-C, 0.3.3.2 rebuild). The padding workaround below retargets
    // paramColor at its scratch copy; the source's own state is what the copy and the restore act on.
    ID3D12Resource* const sourceColor = paramColor;

    if (paramColor)
    {
        LOG_DEBUG("Color exist..");

        if (!amdReplacement && Config::Instance()->ColorResourceBarrier.has_value())
        {
            ResourceBarrier(InCommandList, paramColor,
                            (D3D12_RESOURCE_STATES) Config::Instance()->ColorResourceBarrier.value(),
                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
        else if (!amdReplacement && (State::Instance().NVNGX_Engine == NVSDK_NGX_ENGINE_TYPE_UNREAL ||
                 State::Instance().gameEngine == GameEngineType::Unreal ||
                 State::Instance().gameQuirks & GameQuirk::ForceUnrealEngine))
        {
            Config::Instance()->ColorResourceBarrier.set_volatile_value(D3D12_RESOURCE_STATE_RENDER_TARGET);
            ResourceBarrier(InCommandList, paramColor, D3D12_RESOURCE_STATE_RENDER_TARGET,
                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }

        // This view's place in a shared texture (triple / split screen); the AMD neural replacement is already the
        // view's own picture
        UINT colorX = 0, colorY = 0;
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X, &colorX);
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y, &colorY);
        const bool colorCropped = !amdReplacement && (colorX != 0 || colorY != 0);

        if (colorCropped)
            paramColor = CropToSubrect(InCommandList, paramColor, &subrectColor, nullptr, colorX, colorY,
                                       params.renderSize.width, params.renderSize.height, "Color");

        // WAR for FSR 4's autoexposure shader reading entire underlying resource
        // instead of what's specified by renderSize or color's FfxApiResourceDescription.
        // Only linear has this issue (a cropped colour already has exactly the render size)
        if (!colorCropped && Version().major >= 4 && AutoExposure() &&
            !Config::Instance()->FsrNonLinearPQ.value_or_default() &&
            !Config::Instance()->FsrNonLinearSRGB.value_or_default() &&
            !Config::Instance()->FsrNonLinearColorSpace.value_or_default())
        {
            D3D12_RESOURCE_DESC desc = paramColor->GetDesc();

            const auto diffW = static_cast<int>(desc.Width) - static_cast<int>(params.renderSize.width);
            const auto diffH = static_cast<int>(desc.Height) - static_cast<int>(params.renderSize.height);

            // Seemingly it may only be happening when both axes are 1 pixel too big
            // But let's be conservative here and also trigger when just one axis is too big
            const bool validW = (diffW >= 0 && diffW <= 2);
            const bool validH = (diffH >= 0 && diffH <= 2);
            const bool isPadded = (diffW > 0 || diffH > 0);

            if (validW && validH && isPadded)
            {
                static size_t count = 0;
                const size_t index = count % 2;

                CreateBufferResourceWithSize(Device, paramColor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                             &smallerColor[index], params.renderSize.width, params.renderSize.height);

                if (smallerColor[index])
                {
                    ResourceBarrier(InCommandList, paramColor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                    D3D12_RESOURCE_STATE_COPY_SOURCE);

                    ResourceBarrier(InCommandList, smallerColor[index], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                    D3D12_RESOURCE_STATE_COPY_DEST);

                    D3D12_BOX srcBox {};
                    srcBox.left = 0;
                    srcBox.top = 0;
                    srcBox.right = params.renderSize.width;
                    srcBox.bottom = params.renderSize.height;
                    srcBox.front = 0;
                    srcBox.back = 1;

                    D3D12_TEXTURE_COPY_LOCATION dstLocation {};
                    dstLocation.pResource = smallerColor[index];
                    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    dstLocation.SubresourceIndex = 0;

                    D3D12_TEXTURE_COPY_LOCATION srcLocation {};
                    srcLocation.pResource = paramColor;
                    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    srcLocation.SubresourceIndex = 0;

                    InCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, &srcBox);

                    // The source goes back to where it came from (it used to stay in COPY_SOURCE, and the restore
                    // below then transitioned the scratch copy instead). The copy stays in NPSR, which its next use
                    // (NPSR -> COPY_DEST above) assumes.
                    ResourceBarrier(InCommandList, sourceColor, D3D12_RESOURCE_STATE_COPY_SOURCE,
                                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    ResourceBarrier(InCommandList, smallerColor[index], D3D12_RESOURCE_STATE_COPY_DEST,
                                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

                    paramColor = smallerColor[index];
                }

                count++;
            }
        }

        params.color = ffxApiGetResourceDX12(paramColor, FFX_API_RESOURCE_STATE_COMPUTE_READ);
    }
    else
    {
        LOG_ERROR("Color not exist!!");
        return false;
    }

    ID3D12Resource* paramVelocity;
    if (InParameters->Get(NVSDK_NGX_Parameter_MotionVectors, &paramVelocity) != NVSDK_NGX_Result_Success)
        InParameters->Get(NVSDK_NGX_Parameter_MotionVectors, (void**) &paramVelocity);

    if (paramVelocity)
    {
        LOG_DEBUG("MotionVectors exist..");

        if (Config::Instance()->MVResourceBarrier.has_value())
        {
            ResourceBarrier(InCommandList, paramVelocity,
                            (D3D12_RESOURCE_STATES) Config::Instance()->MVResourceBarrier.value(),
                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
        else if (State::Instance().NVNGX_Engine == NVSDK_NGX_ENGINE_TYPE_UNREAL ||
                 State::Instance().gameEngine == GameEngineType::Unreal ||
                 State::Instance().gameQuirks & GameQuirk::ForceUnrealEngine)
        {
            Config::Instance()->MVResourceBarrier.set_volatile_value(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            ResourceBarrier(InCommandList, paramVelocity, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }

        // This view's place in a shared texture (see CropToSubrect). paramVelocity stays the title's texture for the
        // state restore after the dispatch.
        UINT mvX = 0, mvY = 0;
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X, &mvX);
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y, &mvY);
        ID3D12Resource* mvInput = paramVelocity;

        if (mvX != 0 || mvY != 0)
            mvInput = CropToSubrect(InCommandList, paramVelocity, &subrectMv, nullptr, mvX, mvY,
                                    LowResMV() ? params.renderSize.width : TargetWidth(),
                                    LowResMV() ? params.renderSize.height : TargetHeight(), "MotionVectors");

        params.motionVectors = ffxApiGetResourceDX12(mvInput, FFX_API_RESOURCE_STATE_COMPUTE_READ);
    }
    else
    {
        LOG_ERROR("MotionVectors not exist!!");
        return false;
    }

    ID3D12Resource* paramOutput;
    if (InParameters->Get(NVSDK_NGX_Parameter_Output, &paramOutput) != NVSDK_NGX_Result_Success)
        InParameters->Get(NVSDK_NGX_Parameter_Output, (void**) &paramOutput);

    if (paramOutput)
    {
        LOG_DEBUG("Output exist..");

        if (Config::Instance()->OutputResourceBarrier.has_value())
            ResourceBarrier(InCommandList, paramOutput,
                            (D3D12_RESOURCE_STATES) Config::Instance()->OutputResourceBarrier.value(),
                            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        UINT outX = 0, outY = 0;
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_X, &outX);
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_Y, &outY);

        if ((outX != 0 || outY != 0) && !outputSubrectWarned)
        {
            LOG_WARN("Output sub-rectangle at ({}, {}) is not supported by FFX, writing at (0, 0)", outX, outY);
            outputSubrectWarned = true;
        }

        params.output = ffxApiGetResourceDX12(paramOutput, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    else
    {
        LOG_ERROR("Output not exist!!");
        return false;
    }

    ID3D12Resource* paramDepth;
    if (InParameters->Get(NVSDK_NGX_Parameter_Depth, &paramDepth) != NVSDK_NGX_Result_Success)
        InParameters->Get(NVSDK_NGX_Parameter_Depth, (void**) &paramDepth);

    if (paramDepth)
    {
        LOG_DEBUG("Depth exist..");

        if (Config::Instance()->DepthResourceBarrier.has_value())
            ResourceBarrier(InCommandList, paramDepth,
                            (D3D12_RESOURCE_STATES) Config::Instance()->DepthResourceBarrier.value(),
                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        // This view's place in a shared texture (see CropToSubrect); paramDepth stays the title's texture
        UINT depthX = 0, depthY = 0;
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X, &depthX);
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y, &depthY);
        ID3D12Resource* depthInput = paramDepth;

        if (depthX != 0 || depthY != 0)
            depthInput = CropToSubrect(InCommandList, paramDepth, &subrectDepth, &subrectDepthWhole, depthX, depthY,
                                       params.renderSize.width, params.renderSize.height, "Depth");

        params.depth = ffxApiGetResourceDX12(depthInput, FFX_API_RESOURCE_STATE_COMPUTE_READ);
    }
    else
    {
        LOG_ERROR("Depth not exist!!");

        if (LowResMV())
            return false;
    }

    ID3D12Resource* paramExp = nullptr;
    if (AutoExposure())
    {
        LOG_DEBUG("AutoExposure enabled!");
    }
    else
    {
        if (InParameters->Get(NVSDK_NGX_Parameter_ExposureTexture, &paramExp) != NVSDK_NGX_Result_Success)
            InParameters->Get(NVSDK_NGX_Parameter_ExposureTexture, (void**) &paramExp);

        if (paramExp)
        {
            LOG_DEBUG("ExposureTexture exist..");

            if (Config::Instance()->ExposureResourceBarrier.has_value())
                ResourceBarrier(InCommandList, paramExp,
                                (D3D12_RESOURCE_STATES) Config::Instance()->ExposureResourceBarrier.value(),
                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            params.exposure = ffxApiGetResourceDX12(paramExp, FFX_API_RESOURCE_STATE_COMPUTE_READ);
        }
        else
        {
            LOG_DEBUG("AutoExposure disabled but ExposureTexture is not exist, it may cause problems!!");
            State::Instance().autoExposure = true;
            State::Instance().changeBackend[Handle()->Id] = true;
            return true;
        }
    }

    ID3D12Resource* paramTransparency = nullptr;
    if (InParameters->Get("FSR.transparencyAndComposition", &paramTransparency) == NVSDK_NGX_Result_Success)
        InParameters->Get("FSR.transparencyAndComposition", (void**) &paramTransparency);

    ID3D12Resource* paramReactiveMask = nullptr;
    if (InParameters->Get("FSR.reactive", &paramReactiveMask) == NVSDK_NGX_Result_Success)
        InParameters->Get("FSR.reactive", (void**) &paramReactiveMask);

    ID3D12Resource* paramReactiveMask2 = nullptr;
    if (InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, &paramReactiveMask2) !=
        NVSDK_NGX_Result_Success)
        InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, (void**) &paramReactiveMask2);

    if (!Config::Instance()->DisableReactiveMask.value_or(paramReactiveMask == nullptr &&
                                                          paramReactiveMask2 == nullptr))
    {
        if (paramTransparency != nullptr)
        {
            LOG_DEBUG("Using FSR transparency mask..");
            params.transparencyAndComposition =
                ffxApiGetResourceDX12(paramTransparency, FFX_API_RESOURCE_STATE_COMPUTE_READ);
        }

        if (paramReactiveMask != nullptr)
        {
            LOG_DEBUG("Using FSR reactive mask..");
            params.reactive = ffxApiGetResourceDX12(paramReactiveMask, FFX_API_RESOURCE_STATE_COMPUTE_READ);
        }
        else
        {
            if (paramReactiveMask2 != nullptr)
            {
                LOG_DEBUG("Input Bias mask exist..");
                Config::Instance()->DisableReactiveMask.set_volatile_value(false);

                if (Config::Instance()->MaskResourceBarrier.has_value())
                    ResourceBarrier(InCommandList, paramReactiveMask2,
                                    (D3D12_RESOURCE_STATES) Config::Instance()->MaskResourceBarrier.value(),
                                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

                // This view's place in a shared texture (see CropToSubrect); no state restore uses this pointer
                UINT maskX = 0, maskY = 0;
                InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_X, &maskX);
                InParameters->Get(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_Y, &maskY);

                if (maskX != 0 || maskY != 0)
                    paramReactiveMask2 =
                        CropToSubrect(InCommandList, paramReactiveMask2, &subrectMask, nullptr, maskX, maskY,
                                      params.renderSize.width, params.renderSize.height, "Bias mask");

                if (paramTransparency == nullptr && Config::Instance()->FsrUseMaskForTransparency.value_or_default())
                    params.transparencyAndComposition =
                        ffxApiGetResourceDX12(paramReactiveMask2, FFX_API_RESOURCE_STATE_COMPUTE_READ);

                if (Config::Instance()->DlssReactiveMaskBias.value_or_default() > 0.0f && Bias->IsInit() &&
                    Bias->CreateBufferResource(Device, paramReactiveMask2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS) &&
                    Bias->CanRender())
                {
                    Bias->SetBufferState(InCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

                    if (Bias->Dispatch(InCommandList, paramReactiveMask2,
                                       Config::Instance()->DlssReactiveMaskBias.value_or_default(), Bias->Buffer()))
                    {
                        Bias->SetBufferState(InCommandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        params.reactive = ffxApiGetResourceDX12(Bias->Buffer(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
                    }
                }
                else
                {
                    LOG_DEBUG("Skipping reactive mask, Bias: {0}, Bias Init: {1}, Bias CanRender: {2}",
                              Config::Instance()->DlssReactiveMaskBias.value_or_default(), Bias->IsInit(),
                              Bias->CanRender());
                }
            }
        }
    }

    _hasColor = params.color.resource != nullptr;
    _hasDepth = params.depth.resource != nullptr;
    _hasMV = params.motionVectors.resource != nullptr;
    _hasExposure = params.exposure.resource != nullptr;
    _hasTM = params.transparencyAndComposition.resource != nullptr;
    _accessToReactiveMask = paramReactiveMask != nullptr;
    _hasOutput = params.output.resource != nullptr;

    // For FSR 4 as it seems to be missing some conversions from typeless
    // transparencyAndComposition and exposure might be unnecessary here
    if (Version().major >= 4)
    {
        ffxResolveTypelessFormat(params.color.description.format);
        ffxResolveTypelessFormat(params.depth.description.format);
        ffxResolveTypelessFormat(params.motionVectors.description.format);
        ffxResolveTypelessFormat(params.exposure.description.format);
        ffxResolveTypelessFormat(params.transparencyAndComposition.description.format);
        ffxResolveTypelessFormat(params.output.description.format);
    }

    params.motionVectorScale.x = 1.0f;
    params.motionVectorScale.y = 1.0f;

    if (InParameters->Get(NVSDK_NGX_Parameter_MV_Scale_X, &params.motionVectorScale.x) != NVSDK_NGX_Result_Success ||
        InParameters->Get(NVSDK_NGX_Parameter_MV_Scale_Y, &params.motionVectorScale.y) != NVSDK_NGX_Result_Success)
    {
        LOG_WARN("Can't get motion vector scales!");
    }

    LOG_DEBUG("Sharpness: {0}", params.sharpness);

    if (!Config::Instance()->FsrUseFsrInputValues.value_or_default() ||
        InParameters->Get("FSR.cameraNear", &params.cameraNear) != NVSDK_NGX_Result_Success)
    {
        if (DepthInverted())
            params.cameraFar = Config::Instance()->FsrCameraNear.value_or_default();
        else
            params.cameraNear = Config::Instance()->FsrCameraNear.value_or_default();
    }

    if (!Config::Instance()->FsrUseFsrInputValues.value_or_default() ||
        InParameters->Get("FSR.cameraFar", &params.cameraFar) != NVSDK_NGX_Result_Success)
    {
        if (DepthInverted())
            params.cameraNear = cfg.FsrCameraFar.value_or_default();
        else
            params.cameraFar = cfg.FsrCameraFar.value_or_default();
    }

    if (!cfg.FsrUseFsrInputValues.value_or_default() ||
        InParameters->Get(OptiKeys::FSR_CameraFovVertical, &params.cameraFovAngleVertical) != NVSDK_NGX_Result_Success)
    {
        if (cfg.FsrVerticalFov.has_value())
            params.cameraFovAngleVertical = GetRadiansFromDeg(cfg.FsrVerticalFov.value());
        else if (cfg.FsrHorizontalFov.value_or_default() > 0.0f)
        {
            const float hFovRad = GetRadiansFromDeg(cfg.FsrHorizontalFov.value());
            params.cameraFovAngleVertical =
                GetVerticalFovFromHorizontal(hFovRad, (float) TargetWidth(), (float) TargetHeight());
        }
        else
            params.cameraFovAngleVertical = GetRadiansFromDeg(60);
    }

    if (!Config::Instance()->FsrUseFsrInputValues.value_or_default() ||
        InParameters->Get("FSR.frameTimeDelta", &params.frameTimeDelta) != NVSDK_NGX_Result_Success)
    {
        if (InParameters->Get(NVSDK_NGX_Parameter_FrameTimeDeltaInMsec, &params.frameTimeDelta) !=
                NVSDK_NGX_Result_Success ||
            params.frameTimeDelta < 1.0f)
            params.frameTimeDelta = (float) GetDeltaTime();
    }

    LOG_DEBUG("FrameTimeDeltaInMsec: {0}", params.frameTimeDelta);

    if (!Config::Instance()->FsrUseFsrInputValues.value_or_default() ||
        InParameters->Get("FSR.viewSpaceToMetersFactor", &params.viewSpaceToMetersFactor) != NVSDK_NGX_Result_Success)
        params.viewSpaceToMetersFactor = 0.0f;

    if (InParameters->Get(NVSDK_NGX_Parameter_DLSS_Pre_Exposure, &params.preExposure) != NVSDK_NGX_Result_Success)
        params.preExposure = 1.0f;

    if (Version() >= feature_version { 3, 1, 1 } && _velocity != Config::Instance()->FsrVelocity.value_or_default())
    {
        _velocity = Config::Instance()->FsrVelocity.value_or_default();
        ffxConfigureDescUpscaleKeyValue m_upscalerKeyValueConfig {};
        m_upscalerKeyValueConfig.header.type = FFX_API_CONFIGURE_DESC_TYPE_UPSCALE_KEYVALUE;
        m_upscalerKeyValueConfig.key = FFX_API_CONFIGURE_UPSCALE_KEY_FVELOCITYFACTOR;
        m_upscalerKeyValueConfig.ptr = &_velocity;
        auto result = FfxApiProxy::D3D12_Configure(&_context, &m_upscalerKeyValueConfig.header);

        if (result != FFX_API_RETURN_OK)
            LOG_WARN("Velocity configure result: {}", (UINT) result);
    }

    if (Version() >= feature_version { 3, 1, 4 })
    {
        if (_reactiveScale != Config::Instance()->FsrReactiveScale.value_or_default())
        {
            _reactiveScale = Config::Instance()->FsrReactiveScale.value_or_default();
            ffxConfigureDescUpscaleKeyValue m_upscalerKeyValueConfig {};
            m_upscalerKeyValueConfig.header.type = FFX_API_CONFIGURE_DESC_TYPE_UPSCALE_KEYVALUE;
            m_upscalerKeyValueConfig.key = FFX_API_CONFIGURE_UPSCALE_KEY_FREACTIVENESSSCALE;
            m_upscalerKeyValueConfig.ptr = &_reactiveScale;
            auto result = FfxApiProxy::D3D12_Configure(&_context, &m_upscalerKeyValueConfig.header);

            if (result != FFX_API_RETURN_OK)
                LOG_WARN("Reactive Scale configure result: {}", (UINT) result);
        }

        if (_shadingScale != Config::Instance()->FsrShadingScale.value_or_default())
        {
            _shadingScale = Config::Instance()->FsrShadingScale.value_or_default();
            ffxConfigureDescUpscaleKeyValue m_upscalerKeyValueConfig {};
            m_upscalerKeyValueConfig.header.type = FFX_API_CONFIGURE_DESC_TYPE_UPSCALE_KEYVALUE;
            m_upscalerKeyValueConfig.key = FFX_API_CONFIGURE_UPSCALE_KEY_FSHADINGCHANGESCALE;
            m_upscalerKeyValueConfig.ptr = &_shadingScale;
            auto result = FfxApiProxy::D3D12_Configure(&_context, &m_upscalerKeyValueConfig.header);

            if (result != FFX_API_RETURN_OK)
                LOG_WARN("Shading Scale configure result: {}", (UINT) result);
        }

        if (_accAddPerFrame != Config::Instance()->FsrAccAddPerFrame.value_or_default())
        {
            _accAddPerFrame = Config::Instance()->FsrAccAddPerFrame.value_or_default();
            ffxConfigureDescUpscaleKeyValue m_upscalerKeyValueConfig {};
            m_upscalerKeyValueConfig.header.type = FFX_API_CONFIGURE_DESC_TYPE_UPSCALE_KEYVALUE;
            m_upscalerKeyValueConfig.key = FFX_API_CONFIGURE_UPSCALE_KEY_FACCUMULATIONADDEDPERFRAME;
            m_upscalerKeyValueConfig.ptr = &_accAddPerFrame;
            auto result = FfxApiProxy::D3D12_Configure(&_context, &m_upscalerKeyValueConfig.header);

            if (result != FFX_API_RETURN_OK)
                LOG_WARN("Acc. Add Per Frame configure result: {}", (UINT) result);
        }

        if (_minDisOccAcc != Config::Instance()->FsrMinDisOccAcc.value_or_default())
        {
            _minDisOccAcc = Config::Instance()->FsrMinDisOccAcc.value_or_default();
            ffxConfigureDescUpscaleKeyValue m_upscalerKeyValueConfig {};
            m_upscalerKeyValueConfig.header.type = FFX_API_CONFIGURE_DESC_TYPE_UPSCALE_KEYVALUE;
            m_upscalerKeyValueConfig.key = FFX_API_CONFIGURE_UPSCALE_KEY_FMINDISOCCLUSIONACCUMULATION;
            m_upscalerKeyValueConfig.ptr = &_minDisOccAcc;
            auto result = FfxApiProxy::D3D12_Configure(&_context, &m_upscalerKeyValueConfig.header);

            if (result != FFX_API_RETURN_OK)
                LOG_WARN("Minimum Disocclusion Acc. configure result: {}", (UINT) result);
        }
    }

    if (InParameters->Get("FSR.upscaleSize.width", &params.upscaleSize.width) == NVSDK_NGX_Result_Success &&
        Config::Instance()->OutputScalingEnabled.value_or_default())
    {
        auto originalWidth = static_cast<float>(params.upscaleSize.width);
        params.upscaleSize.width =
            static_cast<uint32_t>(originalWidth * Config::Instance()->OutputScalingMultiplier.value_or_default());
    }
    else if (params.upscaleSize.width == 0)
    {
        params.upscaleSize.width = TargetWidth();
    }

    if (InParameters->Get("FSR.upscaleSize.height", &params.upscaleSize.height) == NVSDK_NGX_Result_Success &&
        Config::Instance()->OutputScalingEnabled.value_or_default())
    {
        auto originalHeight = static_cast<float>(params.upscaleSize.height);
        params.upscaleSize.height =
            static_cast<uint32_t>(originalHeight * Config::Instance()->OutputScalingMultiplier.value_or_default());
    }
    else if (params.upscaleSize.height == 0)
    {
        params.upscaleSize.height = TargetHeight();
    }

    LOG_DEBUG("Dispatch!!");
    auto result = FfxApiProxy::D3D12_Dispatch(&_context, &params.header);

    if (result != FFX_API_RETURN_OK)
    {
        LOG_ERROR("_dispatch error: {0}", FfxApiProxy::ReturnCodeToString(result));

        if (result == FFX_API_RETURN_ERROR_RUNTIME_ERROR)
        {
            LOG_WARN("Trying to recover by recreating the feature");
            State::Instance().changeBackend[Handle()->Id] = true;
        }

        return false;
    }

    // restore resource states
    // The title's colour (sourceColor), not the padding workaround's scratch copy that paramColor may point at now.
    if (!amdReplacement && sourceColor && Config::Instance()->ColorResourceBarrier.has_value())
        ResourceBarrier(InCommandList, sourceColor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                        (D3D12_RESOURCE_STATES) Config::Instance()->ColorResourceBarrier.value());

    if (paramVelocity && Config::Instance()->MVResourceBarrier.has_value())
        ResourceBarrier(InCommandList, paramVelocity, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                        (D3D12_RESOURCE_STATES) Config::Instance()->MVResourceBarrier.value());

    if (paramOutput && Config::Instance()->OutputResourceBarrier.has_value())
        ResourceBarrier(InCommandList, paramOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                        (D3D12_RESOURCE_STATES) Config::Instance()->OutputResourceBarrier.value());

    if (paramDepth && Config::Instance()->DepthResourceBarrier.has_value())
        ResourceBarrier(InCommandList, paramDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                        (D3D12_RESOURCE_STATES) Config::Instance()->DepthResourceBarrier.value());

    if (paramExp && Config::Instance()->ExposureResourceBarrier.has_value())
        ResourceBarrier(InCommandList, paramExp, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                        (D3D12_RESOURCE_STATES) Config::Instance()->ExposureResourceBarrier.value());

    if (paramReactiveMask && Config::Instance()->MaskResourceBarrier.has_value())
        ResourceBarrier(InCommandList, paramReactiveMask, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                        (D3D12_RESOURCE_STATES) Config::Instance()->MaskResourceBarrier.value());

    _frameCount++;

    return true;
}

bool FFXFeatureDx12::InitFFX(const NVSDK_NGX_Parameter* InParameters)
{
    LOG_FUNC();

    if (!ModuleLoaded())
        return false;

    if (IsInited())
        return true;

    if (Device == nullptr)
    {
        LOG_ERROR("D3D12Device is null!");
        return false;
    }

    {
        ScopedSkipSpoofingGlobal skipSpoofingGlobal {};

        LOG_INFO("[CREATE] FFX QueryVersions, device {:X}", (size_t) Device);
        QueryVersionsDx12(Device);
        LOG_INFO("[CREATE] FFX QueryVersions done: {} versions", State::Instance().ffxUpscalerVersionIds.size());

        InitFlags();

        ffxCreateBackendDX12Desc backendDesc = { 0 };
        backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        backendDesc.device = Device;

        _contextDesc.header.pNext = &backendDesc.header;

        if (Config::Instance()->FfxUpscalerIndex.value_or_default() < 0 ||
            Config::Instance()->FfxUpscalerIndex.value_or_default() >= State::Instance().ffxUpscalerVersionIds.size())
            Config::Instance()->FfxUpscalerIndex.set_volatile_value(0);

        ffxOverrideVersion override = { 0 };
        override.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
        override.versionId =
            State::Instance().ffxUpscalerVersionIds[Config::Instance()->FfxUpscalerIndex.value_or_default()];
        backendDesc.header.pNext = &override.header;

        LOG_DEBUG("_createContext!");

        {
            ScopedSkipHeapCapture skipHeapCapture {};

            LOG_INFO("[CREATE] FFX CreateContext, version index {}",
                     Config::Instance()->FfxUpscalerIndex.value_or_default());
            auto ret = FfxApiProxy::D3D12_CreateContext(&_context, &_contextDesc.header, NULL);
            LOG_INFO("[CREATE] FFX CreateContext returned {}", FfxApiProxy::ReturnCodeToString(ret));

            if (ret != FFX_API_RETURN_OK)
            {
                LOG_ERROR("_createContext error: {0}", FfxApiProxy::ReturnCodeToString(ret));
                return false;
            }
        }

        auto version =
            State::Instance().ffxUpscalerVersionNames[Config::Instance()->FfxUpscalerIndex.value_or_default()];
        _name = "FSR";
        parse_version(version);
    }

    SetInit(true);

    return true;
}
