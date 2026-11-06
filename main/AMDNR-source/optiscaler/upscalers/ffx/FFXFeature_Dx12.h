#pragma once
#include "FFXFeature.h"
#include <upscalers/IFeature_Dx12.h>

#include "dx12/ffx_api_dx12.h"
#include "proxies/FfxApi_Proxy.h"

class FFXFeatureDx12 : public FFXFeature, public IFeature_Dx12
{
  private:
    ID3D12Resource* smallerColor[2];

    // Inputs that start inside a bigger texture (the DLSS input sub-rectangles a triple-screen or split-screen game
    // passes for each view) are copied here first: the FFX API takes no offset for its inputs.
    ID3D12Resource* subrectColor = nullptr;
    ID3D12Resource* subrectDepth = nullptr;
    ID3D12Resource* subrectMv = nullptr;
    ID3D12Resource* subrectMask = nullptr;
    // Whole copy of a depth-stencil depth, for GPUs that do not allow a box on a depth-stencil copy source
    ID3D12Resource* subrectDepthWhole = nullptr;
    std::optional<bool> depthStencilBoxCopy;
    bool outputSubrectWarned = false;

    ID3D12Resource* CropToSubrect(ID3D12GraphicsCommandList* InCommandList, ID3D12Resource* InSource,
                                  ID3D12Resource** InScratch, ID3D12Resource** InWhole, UINT InX, UINT InY,
                                  UINT InWidth, UINT InHeight, const char* InName);

    NVSDK_NGX_Parameter* SetParameters(NVSDK_NGX_Parameter* InParameters);

  protected:
    bool InitFFX(const NVSDK_NGX_Parameter* InParameters);

  public:
    FFXFeatureDx12(unsigned int InHandleId, NVSDK_NGX_Parameter* InParameters);

    bool InitInternal(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters) override;
    bool EvaluateInternal(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters) override;
    bool QueryProviders(ID3D12Device* device);

    feature_version Version() override { return FFXFeature::Version(); }
    Upscaler GetUpscalerType() const final { return Upscaler::FFX; }
    API Api() const override { return IFeature_Dx12::Api(); }
    bool CallsUpscalerEndByItself() override { return IFeature_Dx12::CallsUpscalerEndByItself(); }

    bool IsWithDx12() final { return false; }

    ~FFXFeatureDx12()
    {
        if (State::Instance().isShuttingDown)
            return;

        if (_context != nullptr)
            FfxApiProxy::D3D12_DestroyContext(&_context, NULL);

        SAFE_RELEASE(smallerColor[0]);
        SAFE_RELEASE(smallerColor[1]);
        SAFE_RELEASE(subrectColor);
        SAFE_RELEASE(subrectDepth);
        SAFE_RELEASE(subrectMv);
        SAFE_RELEASE(subrectMask);
        SAFE_RELEASE(subrectDepthWhole);
    }
};
