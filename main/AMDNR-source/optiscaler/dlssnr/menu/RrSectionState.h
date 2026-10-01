// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// AMDNR 0.3.4.2 (P2): what the Neural tab's Ray Regeneration section says about itself, as a pure function
// (tests\034\rr_section_state_test.cpp; no ImGui, no Config, no State, no Windows.h).
//
// Until 0.3.4.1 the section hid itself whenever FSR Ray Regeneration had not dispatched in the last 3 seconds, and
// hid its controls whenever it was not dispatching right now. An RX 7800 XT player on 0.3.4.1 therefore had nothing
// to find: his session never created a Ray Reconstruction feature, so the whole section was absent and he went
// looking for the RR sliders among the neural ones. The section now always draws its header and one dim status line,
// and the controls stay on screen, disabled, while RR is not denoising.
//
// The four answers a player needs are different answers to "it looks noisy" (RN1 of the RR noise analysis):
//   Running          a dispatch inside the 3 s window: RR is denoising this frame's ray tracing.
//   Fallback         FSR Ray Regeneration gave the Ray Reconstruction handle up and said why (State::rrFallbackReason,
//                    the 0.3.4 RR-20 lines): the game still asks for RR, FSR upscales without a denoiser.
//   NeverDispatched  no dispatch at all this session: the title has not turned Ray Reconstruction on. The in-game
//                    steps follow, plus what the card gate (hooks\RrHardwareGate.h) has to add on RDNA 3 or when the
//                    ini refuses the card - without that note an RX 7000 or RX 6000 player would be told to switch
//                    something on in the game that AMDNR is not going to be offered for.
//   Idle             it ran, then went quiet for more than 3 s (the player left the RR scene, or turned RR off).
// The status strings the shipped RR guide quotes are here (kRunningLine, kFallbackPrefix, kFallbackFsrRuns) and the
// test pins them. kFallbackFsrRuns is the one 0.3.4.1 sentence this change had to rewrite: it promised the settings
// would show again, and they no longer hide, so it now points at the disabled controls under it.
#include "hooks/RrHardwareGate.h"

#include <string>
#include <string_view>

namespace DlssNr::RrSection
{
enum class State
{
    Running,         // a denoiser dispatch inside the window
    Fallback,        // RR gave the handle up; the reason is the line
    NeverDispatched, // nothing dispatched in this session
    Idle,            // it ran, then went quiet
};

// The silence after which RR counts as not running (0.3.3.2's rule, unchanged: State::fsrRrLastDispatchMs stamps
// every successful dispatch).
inline constexpr unsigned long long kActiveWindowMs = 3000;

// The status lines. The first three are quoted by release\guides\RR-BEST-SETTINGS.md: kRunningLine and
// kFallbackPrefix are 0.3.4.1's bytes, kFallbackFsrRuns is the rewritten one (the guide's section 2 follows it).
inline constexpr const char* kRunningLine = "FSR Ray Regeneration is denoising this game's ray tracing";
inline constexpr const char* kFallbackPrefix = "Ray Regeneration is off in this title: ";
inline constexpr const char* kFallbackFsrRuns =
    "FSR upscaling runs in its place; the settings below act again once Ray Regeneration runs.";
inline constexpr const char* kNeverLine =
    "Not running: the game has not turned on DLSS Ray Reconstruction. In the game, pick DLSS as the upscaler, turn"
    " ray tracing on, then Ray Reconstruction on.";
inline constexpr const char* kIdlePrefix = "Not running now (last ran ";
inline constexpr const char* kIdleSuffix = " s ago)";

// The card gate's own word, drawn under kNeverLine only (once RR has run, the card plainly offered it).
inline constexpr const char* kGateRdna3Default =
    "RX 7000: offered by default; the driver may refuse it (then the game keeps its own denoiser)";
inline constexpr const char* kGateIniFalse = "off for this GPU by [FSR-RR] FfxDenoiserAllowPreRdna4=false";
inline constexpr const char* kGateOptInOnly = "not offered on this GPU: AMD ships Ray Regeneration for RDNA 4;"
                                              " [FSR-RR] FfxDenoiserAllowPreRdna4=true offers it anyway";
// (0.3.4.2) On an NVIDIA GPU with an NR runtime installed the section is drawn (Neural Rendering itself runs there),
// and the never-dispatched line's three in-game steps would otherwise read as a promise that FSR Ray Regeneration
// follows them. It never does on NVIDIA (RrGate::Why::Nvidia), so the note says what serves the game instead.
inline constexpr const char* kGateNvidia =
    "your GPU's own DLSS Ray Reconstruction serves this game; Ray Regeneration is AMDNR's denoiser for AMD cards";

// Decide() copies these literals into std::string (and formats the Idle line), so an open Neural tab pays one or two
// small heap allocations per frame - on the menu thread only, never on the render path, and the same cost the DimTag /
// StrFmt rows beside it already pay.
struct Status
{
    State state = State::NeverDispatched;
    std::string line;              // the one dim status line, ready for ImGui::TextWrapped("%s", ...)
    std::string extra;             // a second dim line, "" = none
    const char* gateNote = nullptr; // a third dim line from the card gate, nullptr = none
    bool controlsActive = false;   // the sliders and checkboxes act (else they are drawn disabled)
    int secondsAgo = 0;            // Idle: whole seconds since the last dispatch, else 0
};

// What the card gate adds to the never-dispatched line. Nothing for a card RR is simply offered on (RDNA 4, an ini
// that already opted in) and nothing while the GPU is not known yet: a line that cannot be acted on is worse than no
// line. On NVIDIA the note is not an instruction, it is the correction of the in-game steps above it.
inline const char* GateNote(RrGate::Decision gate)
{
    switch (gate.why)
    {
    case RrGate::Why::Rdna3Default:
        return kGateRdna3Default;
    case RrGate::Why::IniFalse:
        return kGateIniFalse;
    case RrGate::Why::OptInOnly:
        return kGateOptInOnly;
    case RrGate::Why::Nvidia:
        return kGateNvidia;
    default:
        return nullptr;
    }
}

// rrLastDispatchMs = State::fsrRrLastDispatchMs (0 = never), nowMs = GetTickCount64(), fallbackWhy =
// State::rrFallbackReason (read once per frame by the caller, another thread writes it), gate =
// StreamlineHooks::rrHardwareDecision().
inline Status Decide(unsigned long long rrLastDispatchMs, unsigned long long nowMs, std::string_view fallbackWhy,
                     RrGate::Decision gate)
{
    Status s;
    const bool dispatched = rrLastDispatchMs != 0;
    // A stamp newer than this clock read (the render thread stamped it between the two reads) is age 0, not a
    // wrapped-around age of 49 days.
    const unsigned long long since = nowMs > rrLastDispatchMs ? nowMs - rrLastDispatchMs : 0;

    if (dispatched && since < kActiveWindowMs)
    {
        // A reason the feature has not cleared yet (it clears it on its next recorded frame) is kept as the second
        // line, under the active one, as 0.3.4.1 drew it; "FSR upscaling runs in its place" is not, because it does
        // not.
        s.state = State::Running;
        s.line = kRunningLine;
        s.controlsActive = true;
        if (!fallbackWhy.empty())
            s.extra = std::string(kFallbackPrefix).append(fallbackWhy);
        return s;
    }

    if (!fallbackWhy.empty())
    {
        s.state = State::Fallback;
        s.line = std::string(kFallbackPrefix).append(fallbackWhy);
        s.extra = kFallbackFsrRuns;
        return s;
    }

    if (!dispatched)
    {
        s.state = State::NeverDispatched;
        s.line = kNeverLine;
        s.gateNote = GateNote(gate);
        return s;
    }

    s.state = State::Idle;
    s.secondsAgo = static_cast<int>(since / 1000ull);
    s.line = std::string(kIdlePrefix).append(std::to_string(s.secondsAgo)).append(kIdleSuffix);
    return s;
}
} // namespace DlssNr::RrSection
