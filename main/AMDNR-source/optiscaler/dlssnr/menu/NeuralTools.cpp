// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
// moved from dlssnr/DlssNr_Menu.cpp
#include "pch.h"
#include "NeuralUi.h"

#include <dlssnr/amd/AmdBridge.h>
#include <dlssnr/gi/GiSeam.h> // AMDNR Screen GI (preview): stats, status line, Reset GI
#include <menu/menu_common.h> // MenuCommon::RenderSaveReportRow (Diagnostics' last row)
#include <shaders/fsrd_preprocess/FSRDRuntimeStatus.h>
#include <Config.h>
#include <State.h>

#include <imgui/imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

// The Neural tab's tools row (plan section 4.F, item T9): [Diagnostics] [Runtime options] [Experimental], closed by
// default. Diagnostics holds the overlays (Network output, Debug view, RR debug view), the edit shaper A/B (W3-D) and
// the readouts this file owns (NR cost, model-frame ghost, self-tuning error, edit accumulation, the GPU line).
// Runtime options holds the rows that tune how a runtime is fed and queued, gated by RuntimeCaps. Experimental is the
// inherited screen-space GI group ([AmdRtgi] keys; "RTGI" in code only, owner decision 6 of 2026-09-26 took the word
// off the labels). The appearance filter, the row's fourth sub-tab until 0.3.3.2, is drawn by NeuralLook.cpp (Image
// look).
//
// (0.3.4 menu rework) The owner-approved mock (the owner-approved menu mock, its tb() row and
// the three drawers; MOCK-SPEC.md 3.D): the row is compact and left-aligned (ToolButtons), 10 mock px under the row
// before it, the open tool red; the open drawer's rows are one indent (22 mock px) in; the mock's widgets
// (FillSlider / Checkbox / Combo / BeginCombo / Button of NeuralUi.h), 2-decimal numbers unless a row says otherwise.
// Drawer order as the mock, rows it does not show last: Diagnostics = intro line, Network output, Debug view, RR debug
// view, Edit shaper (A/B), readouts, GPU line, hidden count; Runtime options = Encoding, Every-frame NR, NR slots,
// Highlight proxy (+ Proxy knee), hidden count; Experimental = the screen-space GI checkbox, its rows only while it is
// ticked. (0.3.4 MENU SSGI) Experimental starts with AMDNR Screen GI (preview, [AmdGi], DrawAmdnrGi), then the
// inherited effect; the two are mutually exclusive. (0.3.4 MENU oldgi) The inherited effect is retired from the menu:
// only its checkbox, tagged "retired", and only while the ini has it on (DrawExperimental).
namespace DlssNr::NeuralUi
{
namespace
{
using RuntimeCaps::Cap;
using RuntimeCaps::Support;

bool MenuIsLmxxf()
{
    return RuntimeCaps::Menu().id == AmdBridge::NeuralRuntime::Lmxxf;
}

// Shown only while the game really uses Ray Reconstruction: FSR Ray Regeneration stamps every successful denoiser
// dispatch (State::fsrRrLastDispatchMs); 3 s of silence hides the row (the rule of the Ray Regeneration section).
bool RrActive()
{
    const ULONGLONG rrLast = State::Instance().fsrRrLastDispatchMs;
    return rrLast != 0 && GetTickCount64() - rrLast < 3000;
}

// Readouts (plan F1): dim text, each drawn only when it applies. 0.3.3.2 drew them in the tab under the Live
// section (DrawReadouts); they are for reading a problem, not for playing, so they sit in Diagnostics now.
void DrawDiagnosticsReadouts(const Ctx& ctx)
{
    Config* config = ctx.config;
    const auto st = DlssNr::AmdBridge::Stats();

    // (0.3.4 W3-D, P7.10) NR cost: Stats::nrGpuMs, -1 = not measured (not yet, NR idle, the menu was closed, or
    // OptiScaler's Vulkan / D3D11 bridge on danielblnc): no line then. Each runtime measures its own way (the hover).
    if (RuntimeCaps::Get(Cap::NrCostReadout) == Support::Yes && st.nrGpuMs >= 0.f)
    {
        char line[96];
        std::snprintf(line, sizeof line, "NR cost: %.1f ms of GPU time per model frame",
                      static_cast<double>(st.nrGpuMs));
        StatusLine(line, MenuIsLmxxf()
                             ? "lmxxf: the network's own GPU time per run (the runtime's hip_ms), the mean of the last"
                               " stats window. Frames filled between model frames (Model interleave) cost less."
                             : "danielblnc: the game queue's time inside NR on a model frame, the mean of the last"
                               " 60, measured while the Neural tab is open. Frames filled between model frames"
                               " (Model interleave) cost less.");
    }

    // Model-frame ghost: what the brain measures, for reading a ghost without a capture. A hint, not a verdict
    // (P3, 0.3.3.2): the meter compares the model's answer with the pass's previous output, so a tone change on a
    // smooth moving surface counts too, and its 8% mark is not calibrated. danielblnc only (Cap::GhostReadout;
    // lmxxf has no meter, so the line is absent there, not counted). Stats::modelHistoryInUse is the value the
    // runtime was really given, written on model frames only: before the first model frame of this backend
    // (Stats::modelFrameSeen, LF-C) the line waits instead of reporting "history off".
    if (st.interleaving && RuntimeCaps::Get(Cap::GhostReadout) == Support::Yes)
    {
        if (!st.modelFrameSeen)
            ImGui::TextDisabled("%s", "Model-frame ghost: waiting for the first model frame");
        else if (!st.modelHistoryInUse)
            ImGui::TextDisabled("%s", "Model-frame ghost: n/a (Network history off: the model has no history to ghost from)");
        else if (st.modelGhostSamples > 0)
        {
            // R5: the page has at most one orange line (the attention slot). A high reading is drawn in the normal
            // text colour among the dim readouts, with its advice on the next line.
            const bool high = st.modelGhostFraction > 0.08f;
            char line[160];
            std::snprintf(line, sizeof line, "Model-frame ghost: %.1f%% of %u moving px", 100.0 * st.modelGhostFraction,
                          st.modelGhostSamples);
            if (high)
                ImGui::TextUnformatted(line);
            else
                ImGui::TextDisabled("%s", line);
            HelpForLastItem("Of the moving pixels whose old content differs from the new frame, the share where the"
                            " model's answer looks like the old picture. A hint, not a verdict: a tone change on a"
                            " smooth moving surface counts too, and the 8% mark is not calibrated.");
            if (high)
                ImGui::TextDisabled("%s", "Possible model-frame ghost: compare with Network history off"
                                          " (Quality > More quality options).");
        }
    }

    if (st.interleaving && st.learnedValid && config->AmdInterleavePreset.value_or_default() == 8)
        ImGui::TextDisabled("Self-tuning error: reprojected %.4f | carried edit %.4f | curve %.4f", st.learnedError[0],
                            st.learnedError[1], st.learnedError[2]);

    // Edit accumulation: how much of the picture wears the tone curve because the carried correction was not valid
    // there, how much of that the raw test alone refused (the jitter-sign check: standing still on texture it should
    // be 0-3%), how much of the model's local detail the danielblnc fit keeps, and model calls the runtime declined.
    if (st.interleaving && st.accumulating)
    {
        char kept[32];
        if (st.carryDetailKept >= -0.5f)
            std::snprintf(kept, sizeof kept, "%.0f%%", 100.0 * st.carryDetailKept);
        else
            std::snprintf(kept, sizeof kept, "n/a");
        char line[200];
        std::snprintf(line, sizeof line,
                      "Edit accumulation: tone curve %.1f%% (raw test %.1f%%) | model detail kept %s | refused %llu",
                      100.0 * st.carryTone, 100.0 * st.carryRawRefused, kept, st.refusedCalls);
        ImGui::TextDisabled("%s", line);
    }
}

// RR debug view ([FSR-RR] FfxDenoiserDebugMode): the views that answer the face-grain questions, looked up by name
// in the table the feature publishes (FSRD::RuntimeStatus::DebugViews), so the values stay the feature's own.
// Applies live; Off returns to the picture. The feature keeps RR "running" (and so this row on screen) while a
// view that bypasses the denoiser is selected. Plan F1: item labels in plain words; the feature's own view name
// is in each item's hover, for reading logs and tester reports.
void DrawRrDebugViewRow(const Ctx& ctx)
{
    Config* config = ctx.config;

    struct RRView
    {
        const char* name;  // the feature's name for the view (kDebugModes)
        const char* label; // what the combo shows
        const char* tip;   // the item's hover
    };
    static constexpr RRView kViews[] = {
        { "None", "Off (the picture)", "The picture as it is played." },
        { "DenoiserFraction", "Through RR or around it",
          "DenoiserFraction: red = the pixel goes through Ray Regeneration, blue = it goes around it."
          " RR pauses while shown." },
        { "ProfileTextureRoute", "Path-traced profile's texture route",
          "ProfileTextureRoute: red = textured surfaces the path-traced profile sends back to the normal route"
          " (Texture route slider). RR pauses while shown." },
        { "InBiasMask", "What the bias mask sends around RR",
          "InBiasMask: the pixels the game's bias mask flags (Bias mask strength). RR pauses while shown." },
        { "SkipSignal", "Light added back without RR", "SkipSignal: the part composition adds back around RR." },
        { "DenoiserOutput", "RR's own output", "DenoiserOutput: Ray Regeneration's answer alone." },
        { "Correlation", "Raw colour blended back after RR", "Correlation: the raw colour blended back after RR." },
        { "SkipFloor", "Spatial floor around RR",
          "SkipFloor: the spatial floor kept out of RR. RR pauses while shown." },
        { "SkipUnmapped", "Light the albedo could not carry",
          "SkipUnmapped: light the albedo could not carry, kept out of RR. RR pauses while shown." },
        { "InDiffAlbedo", "Albedo RR divides by",
          "InDiffAlbedo: the diffuse albedo RR divides by. RR pauses while shown." },
    };

    // The feature's constant table, published once by pointer. State's name map belongs to the render thread,
    // which fills it while it creates a denoiser context, so the overlay thread does not read it. No table yet:
    // no row (0.3.3.2 drew a lone "(?)" beside the previous row then).
    const FSRD::RuntimeStatus::DebugViewTable* const modeTable =
        FSRD::RuntimeStatus::DebugViews.load(std::memory_order_acquire);
    if (modeTable == nullptr || modeTable->Count == 0)
        return;

    const auto valueOf = [modeTable](const char* name, uint64_t& value)
    {
        for (size_t i = 0; i < modeTable->Count; i++)
        {
            const FSRD::RuntimeStatus::DebugView& mode = modeTable->Views[i];
            if (mode.Name != nullptr && std::string_view(mode.Name) == name)
            {
                value = mode.Value;
                return true;
            }
        }
        return false;
    };

    const uint64_t currentMode = config->FfxDenoiserDebugMode.value_or_default();
    const char* preview = nullptr;
    for (const auto& view : kViews)
    {
        uint64_t value = 0;
        if (valueOf(view.name, value) && value == currentMode)
        {
            preview = view.label;
            break;
        }
    }

    char otherLabel[48];
    if (preview == nullptr)
    {
        std::snprintf(otherLabel, sizeof(otherLabel), "Other (0x%llx)", static_cast<unsigned long long>(currentMode));
        preview = otherLabel;
    }

    if (BeginCombo("RR debug view", preview))
    {
        for (const auto& view : kViews)
        {
            uint64_t value = 0;
            if (!valueOf(view.name, value))
                continue;

            const bool selected = value == currentMode;
            if (ImGui::Selectable(view.label, selected))
                config->FfxDenoiserDebugMode = value;
            HelpForLastItem(view.tip);
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    Help("Replaces the picture with one of Ray Regeneration's internal images, to see where grain comes from."
         " Applies live; Off brings the picture back. Views that pause RR say so in their hover: the first"
         " second after Off is noisier.",
         {}, "[FSR-RR] FfxDenoiserDebugMode");
}

// (0.3.4 W3-D) "Edit shaper (A/B, not saved)": the danielblnc edit shaper's modes for an A/B in game, one combo over
// [DlssNr] AmdEditShaper (Off) and AmdEditShaperLimit (Literal / F1 / F2), with its two switches under it while a mode
// is on (AmdEditShaperScope, AmdEditShaperCarryCap). In memory only: every write is set_volatile_value, and SaveIni
// never writes these four keys (Config.cpp), so Save Settings leaves the ini as it was and the next start reads the
// ini again. The bridge reads them per frame (BuildSettings), so a pick applies at once, and its change watcher logs
// each new value (AmdBridge.cpp), so every shot of an A/B has its state in amd_bridge.log. lmxxf never reads them
// (Cap::EditShaperAB No): the row is hidden and counted there. Returns whether it was hidden.
bool DrawEditShaperAB(const Ctx& ctx)
{
    if (Hidden(Cap::EditShaperAB))
        return true;
    Config* config = ctx.config;

    const bool shaperOn = config->AmdEditShaper.value_or_default();
    const int storedLimit = config->AmdEditShaperLimit.value_or_default();
    // The bridge's reading: a limit outside 0..2 is literal (0).
    const int limit = storedLimit >= 0 && storedLimit <= 2 ? storedLimit : 0;
    int mode = shaperOn ? 1 + limit : 0;
    // When a picked mode cannot act (AmdPreSr.cpp shape mode): at exactly 100% NR without Dynamic NR resolution (no
    // scaled pass), or under Network output. A tag says so; the combo stays live.
    const bool at100 = std::fabs(config->AmdNrScale.value_or_default() - 1.0f) < 0.001f &&
                       !config->AmdDynamicRes.value_or_default();
    const bool netOut = config->AmdNetworkOutput.value_or_default();
    const char* idleTag = at100 ? "no effect at 100% NR" : netOut ? "off under Network output" : nullptr;
    {
        // R3: where the table does not have it (only with kHideUnsupported false) a mode other than Off stays undoable.
        const bool capYes = RuntimeCaps::Get(Cap::EditShaperAB) == Support::Yes;
        Gate gate(Cap::EditShaperAB, mode != 0 && idleTag != nullptr, idleTag,
                  /*keepClickable*/ capYes || mode != 0);
        const char* const items[] = { "Off", "Literal", "F1 no limit", "F2 ramped" };
        if (Combo("Edit shaper (A/B, not saved)", &mode, items, IM_ARRAYSIZE(items)))
        {
            config->AmdEditShaper.set_volatile_value(mode != 0);
            if (mode != 0)
                config->AmdEditShaperLimit.set_volatile_value(mode - 1);
        }
        Help("danielblnc's edit shaper away from 100% NR, for an A/B. Off: Residual strength and limit act on the"
             " whole result. Literal: the limit caps the model's edit. F1: no cap. F2: no cap at 100%, the full"
             " limit at 50%. Not saved: the next start reads the ini again.",
             {}, "[DlssNr] AmdEditShaper, AmdEditShaperLimit");
    }
    if (mode != 0)
    {
        ImGui::Indent();
        bool belowOnly = config->AmdEditShaperScope.value_or_default() == 1;
        if (Checkbox("Only below 100%", &belowOnly))
            config->AmdEditShaperScope.set_volatile_value(belowOnly ? 1 : 0);
        Help("On: the shaper acts only below 100% NR; above it the whole result is dialled. Off: both sides of 100%."
             " Not saved.",
             {}, "[DlssNr] AmdEditShaperScope");
        bool carryCap = config->AmdEditShaperCarryCap.value_or_default();
        if (Checkbox("Carry cap", &carryCap))
            config->AmdEditShaperCarryCap.set_volatile_value(carryCap);
        Help("Under Edit accumulation the carried edit is capped at the shaper's limit instead of 4x the pixel, so it"
             " cannot grow past what the shaper allowed. Not saved.",
             {}, "[DlssNr] AmdEditShaperCarryCap");
        ImGui::Unindent();
    }
    return false;
}

// The GPU line (MOCK-SPEC 3.D): "GPU: <name> (<target>) - runs <the runtimes it can run>", from the device table
// (AmdBridge::GpuSupportInfo: the NR adapter once the backend is built, the primary GPU before). The table's note (why
// a runtime cannot run, or runs experimental) opens on the line. Runtime text through "%s" (StatusLine).
void DrawGpuLine()
{
    const auto& g = DlssNr::AmdBridge::GpuSupportInfo();
    std::string line = "GPU: ";
    line += g.name.empty() ? std::string("unknown") : g.name;
    if (g.known)
    {
        if (!g.target.empty())
            line += " (" + g.target + ")";
        std::string runs;
        if (g.danielOk)
            runs = g.danielExperimental ? "danielblnc (experimental)" : "danielblnc";
        if (g.lmxxfOk)
        {
            if (!runs.empty())
                runs += ", ";
            runs += g.lmxxfExperimental ? "lmxxf (experimental)" : "lmxxf";
        }
        line += runs.empty() ? std::string(" - runs no neural runtime") : " - runs " + runs;
    }
    StatusLine(line.c_str(), g.note.empty() ? nullptr : g.note.c_str());
}

void DrawDiagnostics(const Ctx& ctx)
{
    Config* config = ctx.config;
    const bool lmxxf = MenuIsLmxxf();

    // The mock's intro line, word for word.
    ImGui::TextDisabled("%s", "Overlays and readouts - leave them off while playing");

    // Network output. Stuck-value rule (R3): where the menu's table greys the row (a runtime whose cell is not Yes; both
    // are Yes since P7.6, 0.3.4), a ticked box may still act there, so it stays clickable until it is unticked.
    {
        bool netOut = config->AmdNetworkOutput.value_or_default();
        Gate gate(Cap::NetworkOutput, false, nullptr, /*keepClickable*/ netOut);
        if (Checkbox("Network output (raw model answer)", &netOut))
            config->AmdNetworkOutput = netOut;
        Help("Shows the network's own answer with everything after it off (no limit, temporal pass, appearance"
             " filter, proxy or sharpening). For comparing; leave it off to play. With Model interleave on, the"
             " frames between are still filled.",
             { lmxxf ? "lmxxf: ticked, it also stops RenoDX composition." : nullptr }, "[DlssNr] AmdNetworkOutput");
    }

    // Debug view: the interleave path's overlays. Live when Model interleave is on, or where the runtime reads the
    // view without interleave (Cap::DebugViewWithoutInterleave: lmxxf reads it on every frame), or while a view is
    // selected (R3: it can always be set back to Off). Otherwise greyed "needs Model interleave".
    {
        const bool interleaveOn = config->AmdInterleave.value_or_default() > 1.f;
        int dbgv = std::clamp(config->AmdInterleaveDebug.value_or_default(), 0, 5);
        const bool readsWithout = RuntimeCaps::Get(Cap::DebugViewWithoutInterleave) == Support::Yes;
        Gate gate(!interleaveOn && !readsWithout, "needs Model interleave", /*keepClickable*/ dbgv != 0);
        // One numbering for the items and the help. Item 5 differs per runtime (TemporalStability.h: danielblnc's
        // 5 is the edit's size; LmxxfBackend.cpp: lmxxf's 5 is the host's direct edit).
        const char* const items[] = { "Off",
                                      "1 Trust",
                                      "2 Frame type",
                                      "3 Clamp activity",
                                      "4 Raw on skipped frames",
                                      lmxxf ? "5 Direct edit, no carry" : "5 Edit size x10" };
        if (Combo("Debug view", &dbgv, items, IM_ARRAYSIZE(items)))
            config->AmdInterleaveDebug = dbgv;
        Help("Overlays of the filled frames; leave Off to play. 1 white = history used, black = fell back to the"
             " new frame. 2 red = model frame, blue = filled. 3 red = a guard refused the history. 4 no fill: what"
             " still ghosts is the model's.",
             { lmxxf ? "lmxxf: 5 adds the fresh edit, skipping the carry." : "danielblnc: 5 = the edit's size, x10." },
             "[DlssNr] AmdInterleaveDebug");
    }

    if (RrActive())
        DrawRrDebugViewRow(ctx);

    const bool hideShaper = DrawEditShaperAB(ctx);

    // The readouts and the GPU line follow the rows at the same pitch (the mock's dim kid lines; no Spacing()).
    DrawDiagnosticsReadouts(ctx);
    DrawGpuLine();

    HiddenCount({ hideShaper ? "Edit shaper (A/B, not saved)" : nullptr },
                lmxxf ? "lmxxf never reads the edit shaper keys: it shapes its edit at every NR size already."
                      : nullptr);

    // (0.3.4 MENU match1, the approved mock / D-3) Save report and its result, last: the header row no longer has it
    // (its other place is Advanced > Logging; one state for both).
    MenuCommon::RenderSaveReportRow("neural");
}

void DrawRuntimeOptions(const Ctx& ctx)
{
    Config* config = ctx.config;
    const bool lmxxf = MenuIsLmxxf();

    // Every row asks the runtime table (the blanket "danielblnc runtime only" line of 0.3.3.2 is gone). A row the
    // active runtime cannot have is hidden and counted (D3, Hidden()); with RuntimeCaps::kHideUnsupported=false it
    // is drawn greyed with its tag instead (the Gate below).

    // Encoding. R3 (stuck value): where the menu's table greys the row (a runtime whose cell is not Yes; both are Yes
    // since P7.5, 0.3.4, lmxxf on its upscaler path), any value other than Auto stays undoable, so the combo stays
    // clickable until it is back on Auto.
    {
        int encoding = std::clamp(config->AmdEncoding.value_or_default(), 0, 3);
        Gate gate(Cap::Encoding, false, nullptr, /*keepClickable*/ encoding != 0);
        if (Combo("Encoding", &encoding, "Auto (existing)\0Linear\0sRGB\0Gamma 2.2\0"))
            config->AmdEncoding = encoding;
        Help("The colour space the model is shown. Auto matches the game; change it only if colours look wrong.",
             { lmxxf ? "lmxxf: sRGB and Gamma 2.2 also stop RenoDX composition." : nullptr },
             "[DlssNr] AmdEncoding");
    }

    // Each Gate ends right after its own row's Help, so its tag sits on that row (R2) and never on the indented
    // knee or the 1-slot note below it. (0.3.4 menu rework) The mock's order: Encoding, Every-frame NR, NR slots; the
    // Highlight proxy, which the mock does not show, moved last (MOCK-SPEC 3.D).
    const bool hideEveryFrame = Hidden(Cap::EveryFrame);
    if (!hideEveryFrame)
    {
        Gate gate(Cap::EveryFrame);
        bool everyFrame = config->AmdEveryFrame.value_or_default();
        if (Checkbox("Every-frame NR", &everyFrame))
            config->AmdEveryFrame = everyFrame;
        Help("On (default): every frame waits for the model. Off: a frame skips NR while the model is still busy;"
             " can be smoother, but ghosts more.",
             {}, "[DlssNr] AmdEveryFrame");
    }

    const bool hideSlots = Hidden(Cap::NrSlots);
    if (!hideSlots)
    {
        const int storedSlots = std::clamp(config->AmdSlots.value_or_default(), 1, 5);
        {
            Gate gate(Cap::NrSlots);
            // Staged: written on release (function statics, as in 0.3.3.2). R9: nothing is drawn between the
            // slider and its IsItem* reads; the Help and the Gate's tag follow them.
            static int advSlots = 3;
            static bool editAdvSlots = false;
            if (!editAdvSlots)
                advSlots = std::clamp(storedSlots, 2, 5);
            FillSlider("NR slots", &advSlots, 2, 5);
            editAdvSlots = ImGui::IsItemActive();
            if (ImGui::IsItemDeactivatedAfterEdit())
                config->AmdSlots = advSlots;
            Help("How many frames may be in flight at the NR stage (2-5). More = fewer frames without NR under load,"
                 " at some extra VRAM. 3 is a good default; no restart needed.",
                 {}, "[DlssNr] AmdSlots");
        }
        if (storedSlots < 2)
            ImGui::TextDisabled("%s", "The ini holds 1 slot (single-slot mode), which this slider cannot select.");
    }

    const bool hideProxy = Hidden(Cap::HighlightProxy);
    if (!hideProxy)
    {
        int proxy = std::clamp(config->AmdHighlightProxy.value_or_default(), 0, 1);
        {
            Gate gate(Cap::HighlightProxy);
            if (Combo("Highlight proxy (experimental)", &proxy, "Off\0Hybrid\0"))
                config->AmdHighlightProxy = proxy;
            Help("What the model is shown. Off: the frame as it is; very bright content can come back dimmer. Hybrid:"
                 " highlights above the knee are squeezed by a reversible curve before the model and restored after"
                 " it, so detail in lights and sky survives.",
                 {}, "[DlssNr] AmdHighlightProxy");
        }
        if (proxy == 1)
        {
            ImGui::Indent();
            {
                Gate gate(Cap::HighlightProxy);
                float knee = std::clamp(config->AmdHighlightProxyKnee.value_or_default(), 0.25f, 8.f);
                if (FillSlider("Proxy knee", &knee, 0.25f, 4.f, "%.2f"))
                    config->AmdHighlightProxyKnee = knee;
                Help("Where the squeeze starts, in linear units (1.0 = paper white). Below it nothing changes; lower"
                     " reaches more of the frame.",
                     {}, "[DlssNr] AmdHighlightProxyKnee");
            }
            ImGui::Unindent();
        }
    }

    // Graphics wait ([DlssNr] AmdGraphicsWaitExperimental) has no row any more (plan F2, T9): it does nothing in
    // this build (the compute wait runs whatever it says). The key is still read and saved as before.

    // In draw order; on lmxxf the mock's "3 danielblnc-only options hidden".
    HiddenCount({ hideEveryFrame ? "Every-frame NR" : nullptr, hideSlots ? "NR slots" : nullptr,
                  hideProxy ? "Highlight proxy (experimental)" : nullptr },
                lmxxf ? "lmxxf never waits for the model, runs one job at a time and has no highlight proxy." : nullptr);
}

// One RTGI slider row: writes live, its one line of help in the row's hover. (0.3.4 menu rework) A function, not the
// local lambda of 0.3.3.2, so menu_rows.py resolves each row's label (RtgiSlider -> FillSlider -> ImGui::SliderFloat).
template <typename Option>
void RtgiSlider(const char* label, Option& option, float lo, float hi, const char* tip, const char* ini,
                const char* format = "%.2f")
{
    float value = option.value_or_default();
    if (FillSlider(label, &value, lo, hi, format))
        option = value;
    HelpForLastItem(tip, {}, ini);
}

// (0.3.4 MENU SSGI) Set when ticking one screen-space GI unticked the other; the drawer then says why, until both
// are off. Two GI passes would light the picture twice (design 4.5).
bool g_giExclusiveNote = false;

// One [AmdGi] slider row: writes live, help on the label.
template <typename Option>
void GiSlider(const char* label, Option& option, float lo, float hi, const char* tip, const char* ini,
              const char* format = "%.2f")
{
    float value = option.value_or_default();
    if (FillSlider(label, &value, lo, hi, format))
        option = std::clamp(value, lo, hi);
    HelpForLastItem(tip, {}, ini);
}

// (0.3.4 MENU SSGI) AMDNR Screen GI, preview (dlssnr/gi, written by 3zwr1 from papers; design
// the AMDNR SSGI design note 6.2, requests GI-M1..M5).
// Its own "AmdGi" ID scope, above the inherited effect's checkbox. The rows stay live with NR off: nothing here reads
// the NR switch, and the hook runs before the NR gate (DlssNr_Dx12.cpp). No row for Placement (only "before NR"
// exists), Distance fade / View-model fade (no player units yet) or the hidden keys; Debug view is never saved.
void DrawAmdnrGi(const Ctx& ctx)
{
    Config* config = ctx.config;

    ImGui::PushID("AmdGi");
    bool enabled = config->AmdGiEnabled.value_or_default();
    if (Checkbox("AMDNR Screen-space GI", &enabled))
    {
        config->AmdGiEnabled = enabled;
        if (enabled && config->AmdRtgiEnabled.value_or_default())
        {
            config->AmdRtgiEnabled = false;
            g_giExclusiveNote = true;
        }
    }
    Help("AMDNR's own screen-space global illumination: ambient occlusion and one bounce of light from nearby"
         " surfaces, traced from the game's depth before NR, the upscaler and the UI. Works with NR on or off. Costs"
         " GPU time. Preview: expect rough edges.",
         {}, "[AmdGi] Enabled");
    DimTag("preview - by 3zwr1");

    if (enabled)
    {
        ImGui::Indent();
        const AmdnrGi::Stats stats = AmdnrGi::Seam::GetStats();

        // Unset = Low on APUs, High elsewhere (GiSeam.cpp's rule), so the combo shows what runs.
        int quality = config->AmdGiQuality.has_value()
                          ? config->AmdGiQuality.value()
                          : (DlssNr::AmdBridge::GpuSupportInfo().apu ? 0 : config->AmdGiQuality.value_or_default());
        quality = std::clamp(quality, 0, 4);
        if (Combo("GI quality", &quality, "Low\0Medium\0High\0Ultra\0Auto\0"))
            config->AmdGiQuality = quality;
        HelpForLastItem("How much GI traces per frame; higher costs more GPU time. Low and Medium skip surface"
                        " normals of the light sources. Auto is High for now.",
                        {}, "[AmdGi] Quality");
        if (stats.gpuMs >= 0.0f)
        {
            char ms[32];
            std::snprintf(ms, sizeof ms, "%.2f ms", static_cast<double>(stats.gpuMs));
            DimTag(ms);
        }

        GiSlider("Bounce light", config->AmdGiIntensity, 0.0f, 3.0f,
                 "Light that bounces off nearby surfaces, traced from the game's depth before the UI. 0 = off"
                 " (ambient occlusion only).",
                 "[AmdGi] Intensity");
        GiSlider("Ambient occlusion", config->AmdGiOcclusion, 0.0f, 2.0f,
                 "Darkening in creases and contact areas. The game may have its own; lower this if corners look too"
                 " dark.",
                 "[AmdGi] Occlusion");
        GiSlider("Radius", config->AmdGiRadius, 0.25f, 4.0f,
                 "How far on screen GI looks for occluders and light. Larger costs more. Changing it restarts GI's"
                 " history.",
                 "[AmdGi] Radius");

        // 0 = auto for both; the band between 0 and the first real value snaps to the nearer end.
        {
            float thickness = config->AmdGiThickness.value_or_default();
            if (FillSlider("Object thickness", &thickness, 0.0f, 1.0f, thickness <= 0.0f ? "auto" : "%.2f"))
                config->AmdGiThickness = thickness < 0.005f ? 0.0f : std::clamp(thickness, 0.01f, 1.0f);
            HelpForLastItem("How thick objects are assumed to be behind what the screen shows, as a share of their"
                            " distance. Lower = light passes behind thin objects (fewer halos), higher = fewer leaks."
                            " Auto picks it by quality.",
                            {}, "[AmdGi] Thickness");
        }
        {
            float fov = config->AmdGiFov.value_or_default();
            if (FillSlider("Camera FOV", &fov, 0.0f, 140.0f, fov <= 0.0f ? "auto" : "%.0f"))
                config->AmdGiFov = fov < 10.0f ? 0.0f : std::clamp(fov, 20.0f, 140.0f);
            HelpForLastItem("The game's field of view in degrees. Auto reads it from the game (FSR, Streamline or"
                            " DLSS-FG) when it can, else uses 70. Set it only when the tag says 'default'.",
                            {}, "[AmdGi] Fov");
            if (stats.ready && stats.frames > 0 && stats.fovYDegrees > 0.0f)
            {
                // GI_CAM_* (dlssnr/gi/GiShared.h): 0 default, 1 FSR, 2 Streamline, 3 DLSS-FG, 4 manual, 5 [FSR] ini.
                static const char* const kSources[] = { "default", "from FSR", "from Streamline", "from DLSS-FG",
                                                        "manual", "from [FSR] ini" };
                const int src = stats.cameraSource >= 0 && stats.cameraSource < IM_ARRAYSIZE(kSources)
                                    ? stats.cameraSource
                                    : 0;
                char tag[64];
                std::snprintf(tag, sizeof tag, "%.0f deg %s", static_cast<double>(stats.fovYDegrees), kSources[src]);
                DimTag(tag);
            }
        }

        if (TreeNode("More GI options"))
        {
            GiSlider("Bounce colour", config->AmdGiSaturation, 0.0f, 2.0f,
                     "How much of the surfaces' colour the bounced light keeps.", "[AmdGi] Saturation");
            GiSlider("Sky light", config->AmdGiSky, 0.0f, 2.0f,
                     "Extra light from the sky that is on screen, through the parts of the view that nothing blocks."
                     " The game's image already holds its own sky light, so 0 is usual.",
                     "[AmdGi] Sky");
            GiSlider("Multi-bounce", config->AmdGiFeedback, 0.0f, 0.8f,
                     "Feeds last frame's bounce back in for more than one bounce. Held below a safe gain at any Bounce"
                     " light.",
                     "[AmdGi] Feedback");
            int axis = std::clamp(config->AmdGiFovAxis.value_or_default(), 0, 1);
            if (Combo("FOV axis", &axis, "Vertical\0Horizontal\0"))
                config->AmdGiFovAxis = axis;
            HelpForLastItem("Whether a manual Camera FOV is vertical or horizontal.", {}, "[AmdGi] FovAxis");
            int encoding = std::clamp(config->AmdGiEncoding.value_or_default(), -1, 2) + 1;
            if (Combo("Colour encoding", &encoding, "Auto\0Linear\0sRGB\0Gamma 2.2\0"))
                config->AmdGiEncoding = encoding - 1;
            HelpForLastItem("How the game's colour is stored before the upscaler. Auto is right for almost every"
                            " game.",
                            {}, "[AmdGi] Encoding");
            ImGui::TreePop();
        }

        // GI-M5: views 0-7 and 10 (Thickness 8 and Translucency 9 are not drawn in the preview); the last item's
        // index is not its value. Never saved.
        static const int kViewValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10 };
        static const char* const kViewNames[] = { "Off",           "GI only",      "AO only", "Normals",
                                                  "History length", "Disocclusion", "Depth",   "Radiance",
                                                  "Lit protection" };
        const int view = config->AmdGiDebugView.value_or_default();
        int viewIndex = 0;
        for (int i = 0; i < IM_ARRAYSIZE(kViewValues); ++i)
            if (kViewValues[i] == view)
                viewIndex = i;
        if (Combo("Debug view", &viewIndex, kViewNames, IM_ARRAYSIZE(kViewNames)))
            config->AmdGiDebugView.set_volatile_value(kViewValues[viewIndex]);
        HelpForLastItem("Shows one of GI's buffers instead of the image. Lit protection is orange where ambient"
                        " occlusion is spared because the pixel is much brighter than its surroundings. Not saved.",
                        {}, "[AmdGi] DebugView");

        if (Button("Reset GI"))
            AmdnrGi::Seam::ResetHistory();
        HelpForLastItem("Restarts GI's history (for A/B shots). Settings are kept.");

        // Dim; orange only when GI refuses to run.
        const std::string status = AmdnrGi::Seam::StatusLine();
        if (!status.empty())
        {
            static const char* const kRefusals[] = { "needs an AMD GPU", "turned off after an error",
                                                     "not supported", "second device", "HDR flag" };
            bool refusal = false;
            for (const char* r : kRefusals)
                refusal = refusal || status.find(r) != std::string::npos;
            if (refusal)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.55f, 0.2f, 1.f));
                ImGui::TextWrapped("%s", status.c_str());
                ImGui::PopStyleColor();
            }
            else
            {
                StatusLine(status.c_str(), nullptr);
            }
        }
        ImGui::Unindent();
    }

    const bool anyGi = config->AmdGiEnabled.value_or_default() || config->AmdRtgiEnabled.value_or_default();
    const bool bothGi = config->AmdGiEnabled.value_or_default() && config->AmdRtgiEnabled.value_or_default();
    if (!anyGi)
        g_giExclusiveNote = false;
    if (g_giExclusiveNote || bothGi)
        StatusLine(bothGi ? "Only one screen-space GI at a time: untick one." : "Only one screen-space GI at a time.",
                   "Two screen-space GI passes would light the picture twice, so ticking one unticks the other.");
    ImGui::PopID();
}

void DrawExperimental(const Ctx& ctx)
{
    Config* config = ctx.config;

    DrawAmdnrGi(ctx);

    // (0.3.4 MENU oldgi, owner decision of 2026-09-26: "remove the old screen GI from the menu, but don't delete its
    // code") The inherited screen-space GI is retired from the menu, the RR path-traced profile's pattern (29b300f):
    // AMDNR Screen-space GI (preview, DrawAmdnrGi above) replaces it. With [AmdRtgi] Enabled off (the default) none
    // of it is drawn; while a player's ini has it on, only its checkbox is drawn, with a dim "retired" tag and a
    // hover that says how to turn it off, so a saved true can still be unticked. Its other rows and the Reset button
    // stay in the code behind kInheritedGiInMenu (false). The effect, its keys, the ini behaviour, the "RTGI" ID
    // scope and its credit (help below, NOTICE) are unchanged: RtgiNative still ships and runs from the ini.
    constexpr bool kInheritedGiInMenu = false;

    ImGui::PushID("RTGI");
    const bool supported = RuntimeCaps::Get(Cap::Rtgi) == Support::Yes;
    bool enabled = config->AmdRtgiEnabled.value_or_default();
    if (kInheritedGiInMenu || enabled)
    {
        // R3 (stuck value): a ticked box stays clickable where the menu's table greys the effect, because a runtime
        // may still run a saved Enabled=true; it can always be unticked. (Both runtimes have it since P7.7, 0.3.4: the
        // same AmdPreSr::RtgiNative pass, whose .cso shaders both backends load from the experimental_lighting folder
        // beside OptiScaler.dll, AmdBridge Directory().)
        // (0.3.4, owner decision 6 of 2026-09-26) Labelled "Screen-space GI (experimental)", without the inherited
        // "RTGI" (the approved mock's "Screen-space GI (RTGI, experimental)"); the help names where the shaders come
        // from, the OptiScaler-AMD-PreSR lineage, and that they are not AMDNR's own, on both runtimes (the folder is
        // needed on both). Keys, writes and the "RTGI" ID scope unchanged.
        Gate gate(Cap::Rtgi, false, nullptr, /*keepClickable*/ enabled);
        if (Checkbox("Screen-space GI (experimental)", &enabled))
        {
            config->AmdRtgiEnabled = enabled;
            // (0.3.4 MENU SSGI, design 4.5) One screen-space GI at a time: this unticks AMDNR Screen GI.
            if (enabled && config->AmdGiEnabled.value_or_default())
            {
                config->AmdGiEnabled = false;
                g_giExclusiveNote = true;
            }
        }
        if constexpr (kInheritedGiInMenu)
        {
            Help("Adds bounced light and ambient occlusion, traced in screen space from the game's depth. Costs GPU"
                 " time. Set Camera FOV and Depth range to match the game.",
                 { "Uses the experimental_lighting shaders from the OptiScaler-AMD-PreSR lineage (not AMDNR's own):"
                   " the experimental_lighting folder must be beside OptiScaler.dll." },
                 "[AmdRtgi] Enabled");
        }
        else
        {
            Help("Retired: replaced by AMDNR Screen-space GI (preview), the first row of this drawer. It is on from"
                 " your OptiScaler.ini and still works from there ([AmdRtgi] keys). To turn it off, untick it, then"
                 " press Save Settings (or set [AmdRtgi] Enabled=false in the ini).",
                 { "Uses the experimental_lighting shaders from the OptiScaler-AMD-PreSR lineage (not AMDNR's own):"
                   " the experimental_lighting folder must be beside OptiScaler.dll." },
                 "[AmdRtgi] Enabled");
            DimTag("retired");
        }
    }

    // (0.3.4 MENU oldgi) Retired: the rows below are drawn only while kInheritedGiInMenu is true (it is false), so
    // they stay in the code, unchanged, and are set from the ini ([AmdRtgi] keys).
    // (0.3.4 menu rework, MOCK-SPEC 3.D) The mock's Experimental drawer is the one checkbox: the group's 15 rows and
    // its Reset button are drawn only while it is ticked, one indent in (the mock's kid rows). Nothing is lost: until
    // 0.3.3.2 they were greyed while it was off, so they could not be set then either. Plan F3: greyed as one where
    // the runtime does not have the effect; the Reset button stays clickable there (R3), since it also unticks the
    // effect. Every row writes live, as in 0.3.3.2; one line of help each, in the row's own hover.
    if (kInheritedGiInMenu && enabled)
    {
        ImGui::Indent();
        ImGui::BeginDisabled(!supported);
        // Two decimals (the mock's numbers); Fade range keeps 3 (its range starts at 0.001), Camera FOV and Depth
        // range none. A format sets only the text and the drag's rounding step; the stored values are unchanged.
        int quality = config->AmdRtgiQuality.value_or_default();
        if (Combo("Quality", &quality, "Very low\0Low\0Medium\0High\0Ultra\0"))
            config->AmdRtgiQuality = uint32_t(quality);
        HelpForLastItem("Trace quality: higher is cleaner and costs more GPU time.", {}, "[AmdRtgi] Quality");
        int denoiser = config->AmdRtgiDenoiser.value_or_default();
        if (Combo("Denoiser", &denoiser, "Low\0Medium\0High\0"))
            config->AmdRtgiDenoiser = uint32_t(denoiser);
        HelpForLastItem("How strongly the traced light is denoised.", {}, "[AmdRtgi] Denoiser");
        RtgiSlider("Effect mix", config->AmdRtgiMix, 0, 1, "How much of the effect reaches the picture; 0 = none.",
                   "[AmdRtgi] Mix");
        RtgiSlider("Contact shading", config->AmdRtgiContact, 0, 2, "Extra shading where objects meet; 0 = off.",
                   "[AmdRtgi] Contact");
        RtgiSlider("Bounce saturation", config->AmdRtgiSaturation, 0, 2,
                   "Colour strength of the bounced light; 1 = as traced.", "[AmdRtgi] Saturation");
        RtgiSlider("Sample radius", config->AmdRtgiRadius, .25f, 3, "How far each pixel looks for light to gather.",
                   "[AmdRtgi] Radius");
        RtgiSlider("Bounce lighting", config->AmdRtgiLighting, 0, 10, "Brightness of the bounced light.",
                   "[AmdRtgi] Lighting");
        RtgiSlider("Ambient occlusion", config->AmdRtgiOcclusion, 0, 10,
                   "Strength of the darkening in corners and creases.", "[AmdRtgi] Occlusion");
        RtgiSlider("Ambient level", config->AmdRtgiAmbient, .25f, 1, "Level of the ambient light; 1 = the default.",
                   "[AmdRtgi] Ambient");
        RtgiSlider("Object thickness", config->AmdRtgiThickness, 0, 1,
                   "How thick surfaces are taken to be behind what the depth shows.", "[AmdRtgi] Thickness");
        RtgiSlider("Smoothness", config->AmdRtgiSmoothness, 0, 1, "How smooth the effect's result is.",
                   "[AmdRtgi] Smoothness");
        RtgiSlider("Fade range", config->AmdRtgiFade, .001f, 1, "How softly the effect fades out.", "[AmdRtgi] Fade",
                   "%.3f");
        RtgiSlider("Camera FOV", config->AmdRtgiFov, 20, 140, "The game's field of view in degrees; match it.",
                   "[AmdRtgi] Fov", "%.0f");
        RtgiSlider("Depth range", config->AmdRtgiFarPlane, 10, 10000, "The game's far plane distance; match it.",
                   "[AmdRtgi] FarPlane", "%.0f");
        int inspect = config->AmdRtgiInspect.value_or_default();
        if (Combo("Inspect", &inspect, "Final image\0Lighting\0"))
            config->AmdRtgiInspect = uint32_t(inspect);
        HelpForLastItem("Final image, or the traced lighting alone.", {}, "[AmdRtgi] Inspect");
        ImGui::EndDisabled();
        // (owner decision 6) "Reset RTGI" until the label change; the same writes.
        if (Button("Reset screen-space GI"))
        {
            config->AmdRtgiEnabled = false;
            config->AmdRtgiQuality = 2u;
            config->AmdRtgiDenoiser = 1u;
            config->AmdRtgiInspect = 0u;
            config->AmdRtgiContact = 0.0f;
            config->AmdRtgiSaturation = 1.0f;
            config->AmdRtgiRadius = 1.0f;
            config->AmdRtgiMix = 1.0f;
            config->AmdRtgiLighting = 5.0f;
            config->AmdRtgiOcclusion = 1.0f;
            config->AmdRtgiAmbient = 1.0f;
            config->AmdRtgiThickness = .1f;
            config->AmdRtgiSmoothness = .5f;
            config->AmdRtgiFade = .3f;
            config->AmdRtgiFov = 60.0f;
            config->AmdRtgiFarPlane = 600.0f;
        }
        HelpForLastItem("Every screen-space GI setting back to its default, and the effect off.");
        ImGui::Unindent();
    }
    ImGui::PopID();
}
} // namespace

// (0.3.4, plan F1) The readouts draw inside the Diagnostics drawer (DrawDiagnosticsReadouts). RenderMenu still calls
// this where 0.3.3.2 drew them, under the Live section; it draws nothing now.
void DrawReadouts(const Ctx&)
{
}

// (0.3.4, plan F1) The RR debug view is a Diagnostics row (DrawRrDebugViewRow, shown while RR runs). Diagnostics is
// in the tools row, which the dispatcher draws only while an NR runtime is installed; Ray Regeneration needs none
// (RR-20 draws its section under the missing-files card too). So the Ray Regeneration section's call draws the row
// only while no runtime is installed, and nothing otherwise: the row exists once per frame either way.
void DrawRrDebugView(const Ctx& ctx)
{
    if (!DlssNr::AmdBridge::AnyRuntimePresent() && RrActive())
        DrawRrDebugViewRow(ctx);
}

void DrawTools(const Ctx& ctx)
{
    // (0.3.4 menu rework) The mock's 10 px above the row (its margin-top), instead of 0.3.3.2's Spacing().
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::floor(Px(10.0f)));

    // These were stacked collapsibles once. Every time one was opened the panel's height changed and everything
    // under it jumped, and the one you wanted was usually below the fold. As a row they are fixed targets on one
    // line, at most one is open at a time, and nothing moves when you switch. Clicking the open one closes it.
    // (0.3.4) A function static until 0.3.3.2; shared state now (NeuralState, NeuralUi.h), same lifetime.
    int& nrSub = Shared().nrSub;
    // Plan F/T9: the row starts closed (0.3.3.2 opened Diagnostics). NeuralState::nrSub (NeuralUi.h, frozen after
    // W2-M1) still starts at 0.3.3.2's 0, so the first draw of the session closes it once.
    static bool startedClosed = false;
    if (!startedClosed)
    {
        nrSub = -1;
        startedClosed = true;
    }
    // "Advanced" is called "Runtime options" so it does not clash with the Advanced tab.
    static const char* const kNames[] = { "Diagnostics", "Runtime options", "Experimental" };
    if (nrSub < -1 || nrSub >= static_cast<int>(IM_ARRAYSIZE(kNames)))
        nrSub = -1;
    // The mock's tools row: compact and left-aligned, each button as wide as its label, the open one red (the same
    // "NeuralTools" ID scope and button IDs as the 0.3.3.2 full-width row).
    const int clicked = ToolButtons("NeuralTools", kNames, nrSub);
    if (clicked >= 0)
        nrSub = clicked == nrSub ? -1 : clicked;
    if (nrSub < 0)
        return;

    // The open drawer's rows one indent in (IndentSpacing = the mock's 22 px kid indent), right under the row.
    ImGui::Indent();
    if (nrSub == 0)
        DrawDiagnostics(ctx);
    else if (nrSub == 1)
        DrawRuntimeOptions(ctx);
    else if (nrSub == 2)
        DrawExperimental(ctx);
    ImGui::Unindent();
}
} // namespace DlssNr::NeuralUi
