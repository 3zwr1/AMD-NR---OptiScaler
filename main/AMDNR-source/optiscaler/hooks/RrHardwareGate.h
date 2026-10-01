// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// AMDNR 0.3.4, Ray Regeneration rework. Which cards FSR Ray Regeneration is offered on (the answer every DLSS-RR gate uses:
// StreamlineHooks::rrHardwareAllowed / rrCanServe -> slIsFeatureSupported, slGetFeatureRequirements, the sl.dlss_d
// hook and arch spoof, NGX SuperSamplingDenoising.Available, the D3D12 feature requirements, the RR-to-FSR_RR pick and
// the menu's backend list), and whether the two Streamline RR detours attach when sl.interposer is hooked (P2). Pure
// functions without OptiScaler state, so tests\034 (suite rr_gate) checks them without the DLL.
//
// Owner decision 2026-09-26 ~18:30 (a): RDNA 4 always; RDNA 3 / 3.5 (gfx11 / gfx11.5: RX 7000 and the RDNA 3 APUs) by
// default again, as before 0.3.3.2; everything else (RDNA 2 and older, Intel, unknown AMD ids) only with the opt-in.
// [FSR-RR] FfxDenoiserAllowPreRdna4 given in the ini wins: true = any non-NVIDIA card, false = RDNA 4 only. NVIDIA never
// (its own DLSS-RR serves the title).
#include <optional>

namespace RrGate
{
enum class GpuClass
{
    Unknown, // the primary GPU is not known yet (DXGI refuses a factory inside DllMain)
    Nvidia,
    Rdna4,
    Rdna3, // gfx11 or gfx11.5 in the card table
    Other, // RDNA 2 and older, Intel, an AMD id the table does not list, a software adapter
};

enum class Why
{
    GpuUnknown,   // not offered yet; asked again on the next call (never cached)
    Nvidia,       // not offered
    Rdna4,        // offered
    Rdna3Default, // offered: the ini leaves FfxDenoiserAllowPreRdna4 unset (auto)
    IniTrue,      // offered: FfxDenoiserAllowPreRdna4=true
    IniFalse,     // not offered: FfxDenoiserAllowPreRdna4=false (RDNA 4 only)
    OptInOnly,    // not offered: neither RDNA 3 nor RDNA 4 and the ini leaves the key unset
};

struct Decision
{
    bool offered = false;
    Why why = Why::GpuUnknown;
};

// allowPreRdna4Ini = the ini's FfxDenoiserAllowPreRdna4 when it holds true / false, nullopt for auto (the shipped ini).
inline Decision Decide(GpuClass gpu, std::optional<bool> allowPreRdna4Ini)
{
    switch (gpu)
    {
    case GpuClass::Unknown:
        return { false, Why::GpuUnknown };
    case GpuClass::Nvidia:
        return { false, Why::Nvidia };
    case GpuClass::Rdna4:
        return { true, Why::Rdna4 };
    default:
        break;
    }

    if (allowPreRdna4Ini.has_value())
        return *allowPreRdna4Ini ? Decision { true, Why::IniTrue } : Decision { false, Why::IniFalse };

    return gpu == GpuClass::Rdna3 ? Decision { true, Why::Rdna3Default } : Decision { false, Why::OptInOnly };
}

// P2: sl.interposer already in memory when OptiScaler loads (Cyberpunk 2077 and other titles) is hooked from DllMain,
// where the GPU is not known yet, and nothing hooks it again later. Attach slIsFeatureSupported /
// slGetFeatureRequirements then as well, when a denoiser is expected (the same file check rrCanServe makes); both bodies
// re-ask rrCanServe on every call and pass everything else to Streamline's own functions, so on a card RR does not
// serve they are transparent. A known GPU keeps the 0.3.3.2 rule: attach only where RR can serve.
inline bool AttachAtHook(GpuClass gpu, bool canServe, bool denoiserExpected)
{
    return canServe || (gpu == GpuClass::Unknown && denoiserExpected);
}

// AMDNR 0.3.4.1 (Proton RR): the one FSR Ray Regeneration default AMDNR added after 0.3.3.2, the light sharpening
// after RR when the title sends none (FSRDFeatureDx12::kRrDefaultSharpness), stays off under Wine/Proton
// (State::isRunningOnLinux). A Linux player (Proton 11, vkd3d-proton, RADV, RX 9070 XT, RE Requiem) saw magenta
// patches on high-contrast edges with RR on the 0.3.4.1 hotfix build; his 0.3.3.2 RR session ran without it. Windows
// keeps it, unchanged. An explicit value ([Sharpness] OverrideSharpness + Sharpness, or the menu) still wins on every
// platform.

// The sharpness FSR SR uses after RR when the title sends none and [Sharpness] OverrideSharpness is off
// (FSRDFeatureDx12::DefaultSharpnessWhenTitleSendsNone): 0 under the path-traced profile and on Wine/Proton, else
// windowsDefault (FSRDFeatureDx12::kRrDefaultSharpness). The override is read before this (IFeature::GetSharpness).
inline float DefaultSharpnessWhenTitleSendsNone(bool pathTracedProfile, bool onWine, float windowsDefault)
{
    if (pathTracedProfile || onWine)
        return 0.0f;
    return windowsDefault;
}
} // namespace RrGate
