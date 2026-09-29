// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#include "pch.h"
#include "FeatureProvider_Dx12.h"

#include "Util.h"
#include "Config.h"

#include "NVNGX_Parameter.h"

#include "upscalers/dlss/DLSSFeature_Dx12.h"
#include "upscalers/dlssd/DLSSDFeature_Dx12.h"
#include "upscalers/fsr2/FSR2Feature_Dx12.h"
#include "upscalers/fsr2_212/FSR2Feature_Dx12_212.h"
#include "upscalers/ffx/FFXFeature_Dx12.h"
#include "upscalers/fsr31/FSRDFeature_Dx12.h"
#include "upscalers/xess/XeSSFeature_Dx12.h"
#include "FeatureProvider_Dx11.h"
#include <misc/IdentifyGpu.h>

bool FeatureProvider_Dx12::GetFeature(Upscaler upscaler, UINT handleId, NVSDK_NGX_Parameter* parameters,
                                      std::unique_ptr<IFeature_Dx12>* feature, bool persistChoice)
{
    State& state = State::Instance();
    Config& cfg = *Config::Instance();
    auto primaryGpu = IdentifyGpu::getPrimaryGpu();
    ScopedSkipHeapCapture skipHeapCapture {};

    switch (upscaler)
    {
    case Upscaler::XeSS:
        *feature = std::make_unique<XeSSFeatureDx12>(handleId, parameters);
        break;

    case Upscaler::FSR21:
        *feature = std::make_unique<FSR2FeatureDx12_212>(handleId, parameters);
        break;

    case Upscaler::FSR22:
        *feature = std::make_unique<FSR2FeatureDx12>(handleId, parameters);
        break;

    case Upscaler::FFX:
        *feature = std::make_unique<FFXFeatureDx12>(handleId, parameters);
        break;

    case Upscaler::DLSS:
        if (primaryGpu.dlssCapable && state.NVNGX_DLSS_Path.has_value())
        {
            *feature = std::make_unique<DLSSFeatureDx12>(handleId, parameters);
            break;
        }
        else
        {
            *feature = std::make_unique<FSR2FeatureDx12_212>(handleId, parameters);
            upscaler = Upscaler::FSR21;
            break;
        }

    case Upscaler::DLSSD:
        if (primaryGpu.dlssCapable && state.NVNGX_DLSSD_Path.has_value())
        {
            *feature = std::make_unique<DLSSDFeatureDx12>(handleId, parameters);
            break;
        }
        else
        {
            *feature = std::make_unique<FSR2FeatureDx12_212>(handleId, parameters);
            upscaler = Upscaler::FSR21;
            break;
        }

    // AMD FSR Ray Regeneration, driven from the title's DLSS Ray Reconstruction inputs.
    //
    // Three things have to be true before this can do anything, and each fails in a
    // different way if it is not checked here: the denoiser DLL has to be present AND
    // report the 1.2 ABI this build was written against (1.2 broke ABI with 1.1, so an
    // older provider would be handed descriptors it cannot read), the GPU has to be one
    // the denoiser runs on because Ray Regeneration has no fallback path at all (the
    // driver or the DLL refuses it at Init, see below), and the title has to
    // actually publish Ray Reconstruction inputs - which is checked inside the feature,
    // since only it can see the parameter table.
    //
    // Falling back to plain FSR rather than refusing is deliberate: a user who picked
    // FSR-RR in a game that turns out not to qualify gets a working upscaler and a log
    // line, not a black screen.
    case Upscaler::FSR_RR:
        if (!FfxApiProxy::IsDenoiserApiImplementedDx12())
        {
            LOG_WARN("FSR-RR selected, but amd_fidelityfx_denoiser_dx12.dll is missing or is "
                     "not the 1.2 provider this build implements. Falling back to FSR.");
            *feature = std::make_unique<FFXFeatureDx12>(handleId, parameters);
            upscaler = Upscaler::FFX;
            break;
        }
        // Not refused here: the card rule lives in StreamlineHooks::rrHardwareAllowed (RDNA 4, RDNA 3 / 3.5 by
        // default since AMDNR 0.3.4, others by the opt-in), and an explicit Dx12Upscaler=fsr_rr still tries; a
        // refusal falls back in NVNGX_DLSS_Dx12.cpp (FSR 3.1/4, FSR 2.1.2 as the last resort).
        if (IdentifyGpu::isRdna3(primaryGpu))
            LOG_INFO("FSR-RR on {} (RDNA 3): creating the denoiser; if it fails to initialise the upscaler falls "
                     "back to FSR 3.1/4 (or FSR 2.1.2) and the log says why.",
                     primaryGpu.name);
        else if (!IdentifyGpu::isRdna4(primaryGpu))
            LOG_WARN("FSR-RR on a card that is not RDNA 4 ({}): AMD documents Ray Regeneration for RDNA 4 only. "
                     "Trying the denoiser anyway; if it fails to initialise the upscaler falls back to FSR 3.1/4 "
                     "(or FSR 2.1.2).",
                     primaryGpu.name);
        *feature = std::make_unique<FSRDFeatureDx12>(handleId, parameters);
        break;

    default:
        *feature = std::make_unique<FSR2FeatureDx12_212>(handleId, parameters);
        upscaler = Upscaler::FSR21;
        break;
    }

    bool loaded = (*feature)->ModuleLoaded();

    if (!loaded)
    {
        // Fail after the constructor
        ImGui::InsertNotification({ ImGuiToastType::Warning, 10000, "Falling back to FSR 2.1.2" });
        *feature = std::make_unique<FSR2FeatureDx12_212>(handleId, parameters);
        upscaler = Upscaler::FSR21;
        loaded = true; // Assuming the fallback always loads successfully
    }

    // DLSSD is stored in the config as DLSS
    if (upscaler == Upscaler::DLSSD)
        upscaler = Upscaler::DLSS;

    // A Ray Reconstruction handle's backend (FSR-RR, or what it fell back to) is not the player's
    // Super Resolution choice; storing it here plus any SaveIni pinned fsr21 on RDNA 3 (0.3.3.2).
    if (persistChoice)
        cfg.Dx12Upscaler = upscaler;

    return loaded;
}

bool FeatureProvider_Dx12::ChangeFeature(Upscaler upscaler, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
                                         UINT handleId, NVSDK_NGX_Parameter* parameters,
                                         ContextData<IFeature_Dx12>* contextData)
{
    State& state = State::Instance();
    Config& cfg = *Config::Instance();

    if (!state.changeBackend[handleId])
        return false;

    const bool dlssOnNonCapable = !IdentifyGpu::getPrimaryGpu().dlssCapable && state.newBackend == Upscaler::DLSS;
    // A Ray Reconstruction handle rebuilds itself with Reset (FSR-RR recreation, output size) and keeps
    // its own backend: cfg.Dx12Upscaler is the Super Resolution choice and no longer tracks it (0.3.3.2).
    if (state.newBackend == Upscaler::Reset && contextData->rayReconstruction && contextData->feature != nullptr)
        state.newBackend = contextData->feature->GetUpscalerType();
    else if (state.newBackend == Upscaler::Reset || dlssOnNonCapable)
        state.newBackend = cfg.Dx12Upscaler.value_or_default();

    contextData->changeBackendCounter++;

    LOG_INFO("changeBackend is true, counter: {0}", contextData->changeBackendCounter);

    // first release everything
    if (contextData->changeBackendCounter == 1)
    {
        if (state.currentFG != nullptr && state.activeFgInput == FGInput::Upscaler)
        {
            state.fgChanged = true;
            state.clearCapturedHudlesses = true;
        }

        if (contextData->feature != nullptr)
        {
            LOG_INFO("changing backend to {}", UpscalerDisplayName(state.newBackend));

            auto* dc = contextData->feature.get();
            // Use given params if using DLSS passthrough
            const bool isPassthrough = state.newBackend == Upscaler::DLSSD || state.newBackend == Upscaler::DLSS;

            contextData->createParams = isPassthrough ? parameters : GetNGXParameters(API::DX12, false);
            contextData->createParams->Set(NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, dc->GetFeatureFlags());
            contextData->createParams->Set(NVSDK_NGX_Parameter_Width, dc->RenderWidth());
            contextData->createParams->Set(NVSDK_NGX_Parameter_Height, dc->RenderHeight());
            contextData->createParams->Set(NVSDK_NGX_Parameter_OutWidth, dc->DisplayWidth());
            contextData->createParams->Set(NVSDK_NGX_Parameter_OutHeight, dc->DisplayHeight());
            contextData->createParams->Set(NVSDK_NGX_Parameter_PerfQualityValue, dc->PerfQualityValue());

            dc = nullptr;

            State::Instance().currentFeature = nullptr;

            Util::DelayedDestroy(std::move(contextData->feature));

            // LOG_DEBUG("sleeping before reset of current feature for 1000ms");
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            // contextData->feature.reset();
            // contextData->feature = nullptr;
        }
        else // Clean up state if no feature is set
        {
            LOG_ERROR("can't find handle {0} in Dx12Contexts!", handleId);

            state.newBackend = Upscaler::Reset;
            state.changeBackend[handleId] = false;

            if (contextData->createParams != nullptr)
            {
                TryDestroyNGXParameters(contextData->createParams, NVNGXProxy::D3D12_DestroyParameters());
                contextData->createParams = nullptr;
            }

            contextData->changeBackendCounter = 0;

            return false;
        }

        return true;
    }

    // create new feature
    if (contextData->changeBackendCounter == 2)
    {
        LOG_INFO("Creating new {} upscaler", UpscalerDisplayName(state.newBackend));
        contextData->feature.reset();

        if (!GetFeature(state.newBackend, handleId, contextData->createParams, &contextData->feature,
                        !contextData->rayReconstruction))
        {
            LOG_ERROR("Upscaler can't created");
            return false;
        }

        return true;
    }

    // init feature
    if (contextData->changeBackendCounter == 3)
    {
        auto initResult = contextData->feature->Init(device, cmdList, contextData->createParams);

        contextData->changeBackendCounter = 0;

        if (!initResult)
        {
            LOG_ERROR("init failed with {0} feature", UpscalerDisplayName(state.newBackend));

            if (state.newBackend != Upscaler::DLSSD)
            {
                if (cfg.Dx12Upscaler == Upscaler::DLSS)
                {
                    state.newBackend = Upscaler::XeSS;
                    ImGui::InsertNotification({ ImGuiToastType::Warning, 10000, "Falling back to XeSS" });
                }
                else
                {
                    state.newBackend = Upscaler::FSR21;
                    ImGui::InsertNotification({ ImGuiToastType::Warning, 10000, "Falling back to FSR 2.1.2" });
                }
            }
            else
            {
                // Retry DLSSD
                state.newBackend = Upscaler::DLSSD;
            }

            state.changeBackend[handleId] = true;

            return false;
        }
        else
        {
            LOG_INFO("init successful for {0}, upscaler changed", UpscalerDisplayName(state.newBackend));

            state.newBackend = Upscaler::Reset;
            state.changeBackend[handleId] = false;
        }

        // If this is an OptiScaler fake NVNGX param table, delete it
        int optiParam = 0;

        if (contextData->createParams->Get("OptiScaler", &optiParam) == NVSDK_NGX_Result_Success && optiParam == 1)
        {
            TryDestroyNGXParameters(contextData->createParams, NVNGXProxy::D3D12_DestroyParameters());
            contextData->createParams = nullptr;
        }
    }

    // if initial feature can't be inited
    state.currentFeature = contextData->feature.get();

    if (state.currentFG != nullptr && state.activeFgInput == FGInput::Upscaler)
        state.currentFG->UpdateTarget();

    return true;
}
