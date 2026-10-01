// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
// moved from dlssnr/DlssNr_Menu.cpp
#include "pch.h"
#include "NeuralUi.h"

#include <dlssnr/amd/AmdBridge.h>
#include <dlssnr/amd/InterleavePacing.h>
#include <dlssnr/amd/RuntimeCaps.h>
#include <dlssnr/lmxxf/LmxxfBackend.h>    // Lmxxf::FullNetworkRefused, SmallTierPolicy, TierCapInUse
#include <dlssnr/lmxxf/LmxxfTierPolicy.h> // Lmxxf::LmxxfNetworkTier, PlanLmxxfSize (pure)
#include <Config.h>

#include <imgui/imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

// The Neural tab's Performance and Quality sections (the menu-cleanup design note, sections
// 4.B and 4.C). One control per idea for every runtime: a row a runtime cannot use yet is greyed with a short tag from
// the runtime table (RuntimeCaps.h), a row it can never have is hidden and counted, and every runtime difference sits
// in a one-line tooltip footnote. The ini keys, their defaults and what each control writes are those of 0.3.3.2,
// except the two merged controls (Interleave preset, Network history), which write the active runtime's key, and
// lmxxf's Classic carry, stored as kLmxxfClassicCarryValue (D2).
//
// (0.3.4 menu rework) The layout is the owner-approved mock's (the owner-approved menu mock,
// APPROVED_MOCK.png; NeuralUi.h has the widget map). Default view, in this order:
//   Performance: NR resolution (%) "100%" + cost tag "1.00x cost", Neural passes "1", Full network (all 71 blocks),
//                Dynamic NR resolution (Target FPS as its kid while on), Model interleave (Interleave preset and the
//                pacing readout as its kids while on).
//   Quality:     Residual strength 1.00, Residual limit 0.25, Temporal stability 0.60, Sharpening (CAS) 0.00, then the
//                closed "More quality options" tree ("default" / "custom" after it, owner decision 5): Network
//                history, Output smoothing, Stability mode (+ Gate threshold), Residual temporal, Residual edge fade,
//                Still-surface steadiness (moved here from the main list, SPEC-content X4), and the hidden-count line.
// Sliders show 2 decimals except NR resolution (whole percent), the ints and Gate threshold (3, as 0.3.3.2). Kid rows
// keep the full control width (MOCK-SPEC decision K).
namespace DlssNr::NeuralUi
{
namespace
{
using RuntimeCaps::Cap;
using RuntimeCaps::Support;

// D2 (plan 10.2): lmxxf's "Classic carry" is written as 1, which danielblnc's list reads as Standard. 0.3.3.2 wrote 6,
// which danielblnc runs as Guided fill v2. lmxxf runs every stored value below 9 as Classic carry, so an old 6 still
// reads Classic carry there. Revert: 6.
constexpr int kLmxxfClassicCarryValue = 1;
// [DlssNr] AmdInterleavePreset 10: Edit accumulation, the default and the first item on both runtimes.
constexpr int kEditAccumulation = 10;

const ImVec4 kCostAbove(1.f, 0.55f, 0.2f, 1.f); // above the cost at 100%
const ImVec4 kCostBelow(0.4f, 0.9f, 0.5f, 1.f); // below it

bool IsLmxxf(const RuntimeCaps::RuntimeInfo& rt)
{
    return rt.id == AmdBridge::NeuralRuntime::Lmxxf;
}

// The cost tag after NR resolution: the mock's tag place (16 px after the label), dim at the cost of 100%, else
// orange above / green below it (plan B1).
void CostTag(const char* text, const ImVec4* colour)
{
    if (colour == nullptr)
    {
        DimTag(text);
        return;
    }
    ImGui::SameLine(0.0f, std::floor(Px(16.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, *colour);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

// (0.3.4, owner decision 5) The fold row's dim tag: "custom" when a row drawn inside the tree for the active runtime
// (greyed rows included, hidden ones not) holds a value other than its Config.h default, else "default". "Differs"
// is value_for_config(): the value Save Settings would write (none of these keys is ever set volatile). Read only.
template <typename T>
bool IsCustom(CustomOptional<T>& key)
{
    return key.value_for_config().has_value();
}

// "2nd", "3rd", "4th", ... for the Model interleave item of a stored value above 2.
const char* OrdinalSuffix(int n)
{
    if (n % 100 >= 11 && n % 100 <= 13)
        return "th";
    return n % 10 == 1 ? "st" : n % 10 == 2 ? "nd" : n % 10 == 3 ? "rd" : "th";
}

// ---- lmxxf's size tiers (plan N5; runtime work B11 and HX 1/2/6) --------------------------------------------------
// lmxxf runs its network at size tiers (LmxxfTierPolicy.h): 720 (up to 1280x720), 900 (up to 1600x900), else 1080, and
// on handhelds (or [DlssNr] AmdLmxxfTierCap=360 / 576) also 360 and 576. A tier costs the same whatever part of its box
// the picture fills, so the cost follows the tier, not the exact NR resolution.

// A tier's box area (the cost ratio's unit); 0 for an unknown tier.
double TierArea(int tier)
{
    for (const auto& box : Lmxxf::kLmxxfTierBoxes)
        if (static_cast<int>(box.h) == tier)
            return double(box.w) * double(box.h);
    return 0.0;
}

// The frame lmxxf's working size came from. The menu sees only the working size (AmdBridge::Stats), so it tries the
// upscaler's render size, then its output size (NR on the Ray Regeneration output), and keeps the one the backend's
// own sizing rule (PlanLmxxfSize with the cap, the small tiers and the tier snap as they apply) turns into exactly this
// working size at this NR resolution. False when neither does (a size set by another path, or the backend not rebuilt
// yet after a change): then only estimates are left.
// (0.3.4, lmxxf Fast mode) While the running session applies [DlssNr] AmdLmxxfFastMode (Lmxxf::FastModeFromTier() > 0)
// the backend plans the frame once more, one tier lower (LmxxfTierPolicy.h PlanLmxxfFastSize, from the tier the default
// size lands in); the same step here, so the frame is still recovered under Fast.
bool FrameOfWorkingSize(unsigned workW, unsigned workH, float nrScale, unsigned& frameW, unsigned& frameH)
{
    IFeature* feature = State::Instance().currentFeature;
    if (feature == nullptr || workW == 0 || workH == 0)
        return false;
    const int cap = Lmxxf::TierCapInUse();
    const bool smallTiers = Lmxxf::SmallTierPolicy();
    const bool snap = AmdBridge::LmxxfTierSnapOn();
    const bool fast = Lmxxf::FastModeFromTier() > 0;
    const unsigned sizes[2][2] = { { feature->RenderWidth(), feature->RenderHeight() },
                                   { feature->DisplayWidth(), feature->DisplayHeight() } };
    for (const auto& size : sizes)
    {
        if (size[0] == 0 || size[1] == 0)
            continue;
        Lmxxf::TierPlan plan = Lmxxf::PlanLmxxfSize(size[0], size[1], nrScale, cap, smallTiers, snap);
        if (fast)
        {
            Lmxxf::TierPlan fp;
            const unsigned defaultTier = static_cast<unsigned>(Lmxxf::LmxxfNetworkTier(plan.w, plan.h, smallTiers));
            if (Lmxxf::PlanLmxxfFastSize(size[0], size[1], nrScale, defaultTier, smallTiers, snap, fp))
                plan = fp;
        }
        if (plan.w == workW && plan.h == workH)
        {
            frameW = size[0];
            frameH = size[1];
            return true;
        }
    }
    return false;
}

struct LmxxfTiers
{
    int tier = 0;    // the tier the running network uses; 0 = unknown
    int tier100 = 0; // the tier the same frame runs at 100% NR resolution; 0 = unknown (the tag then is the estimate)
};

// The running tier and the tier at 100%, for a working size made at NR resolution `nrScale`.
LmxxfTiers ReadLmxxfTiers(unsigned workW, unsigned workH, float nrScale)
{
    LmxxfTiers t;
    if (workW == 0 || workH == 0 || !(nrScale > 0.f))
        return t;
    const bool smallTiers = Lmxxf::SmallTierPolicy();
    t.tier = Lmxxf::LmxxfNetworkTier(workW, workH, smallTiers);
    // Exact: the frame is known, so 100% is the backend's own rule applied to it (HX request 2). (0.3.4, lmxxf Fast
    // mode) Without the Fast step: the cost is against 100% NR resolution, so Fast's saving shows as a lower cost.
    unsigned frameW = 0, frameH = 0;
    if (FrameOfWorkingSize(workW, workH, nrScale, frameW, frameH))
    {
        t.tier100 = static_cast<int>(Lmxxf::PlanLmxxfSize(frameW, frameH, 1.f, Lmxxf::TierCapInUse(), smallTiers,
                                                          AmdBridge::LmxxfTierSnapOn())
                                         .to);
        return t;
    }
    // At 100% the default sizing's tier is the 100% tier; under Fast mode the backend names it (FastModeFromTier).
    const int fastFrom = Lmxxf::FastModeFromTier();
    if (std::fabs(nrScale - 1.f) < 0.001f)
    {
        t.tier100 = fastFrom > 0 ? fastFrom : t.tier;
        return t;
    }
    // Estimated: the frame is the working size over NR resolution, fitted into the 1920x1080 ceiling (LmxxfBackend
    // ModelSize). Only while the size was not moved: the tier snap (on by default on RDNA 3: AmdBridge::LmxxfTierSnapOn,
    // runtime work HB 1), a cap or the small tiers move it to a tier box and it no longer gives the frame back. Above
    // 100% on the ceiling the frame can be anything from there up, so it is known only when that lower bound already
    // gives the 1080 tier. (0.3.4) Fast mode moves the size too.
    if (AmdBridge::LmxxfTierSnapOn() || Lmxxf::TierCapInUse() != 0 || smallTiers || fastFrom != 0)
        return t;
    const double inW = workW / double(nrScale), inH = workH / double(nrScale);
    const double fit = (std::min)({ 1.0, 1920.0 / inW, 1080.0 / inH });
    const int t100 = Lmxxf::LmxxfNetworkTier(unsigned(std::lround(inW * fit)), unsigned(std::lround(inH * fit)), false);
    const bool onCeiling = nrScale > 1.f && (workW >= 1920 || workH >= 1080);
    t.tier100 = (!onCeiling || t100 == 1080) ? t100 : 0;
    return t;
}

// The Interleave preset item a stored [DlssNr] AmdInterleavePreset shows as, in the active runtime's list (plan B5a).
// danielblnc keeps 0.3.3.2's display table: 9 and 11 run as Edit accumulation (AmdBridge), 7 (Edit carry, removed)
// as Self-tuning, 3 and 4 (Held frame, Residual temporal) as Guided fill. lmxxf: 9 and up is Edit accumulation,
// anything else is its Classic carry.
int ShownPresetIndex(const RuntimeCaps::RuntimeInfo& rt, int stored)
{
    const auto fills = rt.fills;
    if (fills.empty())
        return 0;
    if (IsLmxxf(rt))
    {
        for (size_t i = 0; i < fills.size(); ++i)
            if ((stored >= 9) == (fills[i].value == kEditAccumulation))
                return static_cast<int>(i);
        return 0;
    }
    const int shown = stored >= 9                                  ? kEditAccumulation
                    : (stored == 8 || stored == 7)                 ? 8
                    : stored == 6                                  ? 6
                    : (stored == 5 || stored == 3 || stored == 4)  ? 5
                                                                   : std::clamp(stored, 1, 2);
    for (size_t i = 0; i < fills.size(); ++i)
        if (fills[i].value == shown)
            return static_cast<int>(i);
    return 0;
}

// Residual strength's footnote (plan N7: the ini-only [DlssNr] AmdEditShaper becomes a footnote condition). With the
// shaper off (default) danielblnc's resolve applies strength to the whole NR result at every NR resolution.
const char* ResidualNote(const RuntimeCaps::RuntimeInfo& rt, const Config* config)
{
    if (IsLmxxf(rt))
        return rt.noteResidual;
    return config->AmdEditShaper.value_or_default()
               ? "danielblnc: at 100% NR resolution it blends the whole result; away from 100% it acts on the"
                 " model's edit."
               : "danielblnc: it blends the whole NR result (Look, stability and sharpening included).";
}

// The interleave pacer's live measurement (G1 request; plan F1 lists it with the readouts, D9 "readout always").
// Drawn under Interleave preset while Model interleave is on, on both runtimes: since FB-L5 lmxxf feeds the pacer too,
// but paces only with an explicit [DlssNr] AmdInterleavePacing 0..1 (D9: auto is off there). Dim text, no control:
// the pacing strength is ini-only, as in 0.3.3.2. (0.3.4) " - " between the parts, as the status line (SPEC-content 5).
void DrawPacingReadout(const Config* config, bool lmxxf)
{
    const auto pace = Pacing::Read();
    const float key = config->AmdInterleavePacing.value_or_default();
    const bool measured = pace.modelMs > 0.0 && pace.fillMs > 0.0;
    // Why it does not pace, when it does not: lmxxf at auto (-1), or 0 on either runtime.
    const char* off = (lmxxf && key < 0.f) ? "off with lmxxf" : key == 0.f ? "off" : nullptr;
    char line[192];
    if (measured && pace.active)
        std::snprintf(line, sizeof line,
                      "Interleave pacing: model %.1f ms - fill %.1f ms - paced to %.1f ms (%.0f fps), %.0f%% even",
                      pace.modelMs, pace.fillMs, pace.targetMs, pace.targetMs > 0.0 ? 1000.0 / pace.targetMs : 0.0,
                      pace.evenness * 100.0);
    else if (measured)
        std::snprintf(line, sizeof line, "Interleave pacing: %s - model %.1f ms - fill %.1f ms (%.0f fps average)",
                      off != nullptr ? off : "not pacing", pace.modelMs, pace.fillMs, 2000.0 / (pace.modelMs + pace.fillMs));
    else
        std::snprintf(line, sizeof line, "Interleave pacing: %s", off != nullptr ? off : "measuring");
    ImGui::TextDisabled("%s", line);
    Help("Evens out the frame time when model frames and filled frames cost different amounts (the engine's"
         " judder). Automatic: nothing under a 15% split, fully even from 30% up.",
         { lmxxf ? "lmxxf: automatic (-1) does not pace on lmxxf; 0..1 pads the filled frames toward the model"
                   " frame's cost."
                 : nullptr },
         "[DlssNr] AmdInterleavePacing (-1 automatic, 0 off, 0..1 fixed)");
}
} // namespace

void DrawPerformance(const Ctx& ctx)
{
    Config* config = ctx.config;
    const bool lmxxfMenu = ctx.lmxxfMenu;
    const RuntimeCaps::RuntimeInfo& rt = RuntimeCaps::Menu();

    NrSection("Performance");

    // danielblnc on a Vulkan title runs 1 Neural pass whatever the slider says (AmdPreSr.cpp Record,
    // Settings::vulkanBridge): each extra pass would wait for the Vulkan bridge's submission, which comes only after
    // the game submits. The cost below counts that one pass.
    const bool danielVkOnePass = RuntimeCaps::Get(Cap::AllPassesOnVulkan) != Support::Yes &&
                                 State::Instance().api == API::Vulkan && AmdBridge::HasFiles();

    // ---- B1 NR resolution (%), with its cost on the same row -------------------------------------------------------
    // The staged value (scale / editingScale) lives in NeuralState (NeuralUi.h), with the same lifetime as the
    // function statics it replaced; the cost and VRAM lines below read it.
    NeuralState& ns = Shared();
    float& scale = ns.scale;
    bool& editingScale = ns.editingScale;
    if (!editingScale)
        scale = config->AmdNrScale.value_or_default() * 100.f;
    // Snapped to 5% steps on every change (drag or text entry), so the commit below and the cost read the snapped
    // value: each NEW NR working size can keep VRAM until the game restarts, so tuning has to land on a few reusable
    // sizes. A value set elsewhere (the ini, an NR style, Dynamic NR) is shown as it is until the slider is moved.
    if (FillSlider("NR resolution (%)", &scale, 25, 150, "%.0f%%"))
        scale = std::round(scale / 5.f) * 5.f;
    // IsItemActive / IsItemDeactivatedAfterEdit read the LAST item drawn. A note was once drawn between the slider
    // and these two lines; the commit never fired and the slider snapped back to the saved value every frame
    // ("whenever I raise or lower it, it goes back to 98"). Nothing goes between them (plan R9).
    editingScale = ImGui::IsItemActive();
    // Commit once after dragging or text entry, not one model rebuild per mouse move.
    if (ImGui::IsItemDeactivatedAfterEdit())
        config->AmdNrScale = scale / 100.f;
    // runtime work HB 8: on a handheld lmxxf runs at the 360 tier (the cap's auto); only once the running session has
    // the small tiers (Lmxxf::SmallTierPolicy), before that the APU runs the 720 tier.
    const bool handheld360 = IsLmxxf(rt) && AmdBridge::GpuSupportInfo().lmxxfExperimental &&
                             Lmxxf::SmallTierPolicy() && Lmxxf::TierCapInUse() == 360;
    Help("The main quality/speed lever: the size the model works at. Below 100% only its correction is lifted to"
         " full size; above 100% the cost grows with the square (150% = 2.25x).",
         { rt.noteNrResolution,
           handheld360 ? "lmxxf on handhelds: runs at the 360p network size. [DlssNr] AmdLmxxfTierCap=576 gives 576p"
                         " (1024x576): sharper, about 1.7x the cost."
                       : nullptr },
         "[DlssNr] AmdModelScale");
    // What the slider COSTS, where the hand is and live while dragging, passes included: a tester once set 125%
    // and three passes and read "2.25x" for a 6.75x bill. Two controls that multiply are priced together.
    // (0.3.4) Always the mock's form "N.NNx cost" / "N.NNx cost (N passes)"; lmxxf's tier goes into the hover.
    {
        const int passes = danielVkOnePass ? 1 : std::clamp(int(config->DlssNrPasses.value_or_default()), 1, 3);
        const float s = scale / 100.f;
        // lmxxf (plan N5): the network runs at a size tier, so the tier the working size landed on is the honest
        // cost. Read only while it prices THIS slider: NR on and frames arriving (the backend keeps its last working
        // size while NR is off, and is never released in-session), not while dragging (the new tier is not known yet)
        // and not under Dynamic NR (the working size is then the current step, the slider its ceiling). Otherwise the
        // size-squared estimate, as on danielblnc.
        LmxxfTiers tiers;
        if (IsLmxxf(rt) && ctx.enabled && !editingScale && !config->AmdDynamicRes.value_or_default() &&
            ns.nrFps >= 0.5f && AmdBridge::ActiveRuntime() == rt.id)
        {
            const auto work = AmdBridge::Stats();
            tiers = ReadLmxxfTiers(work.width, work.height, config->AmdNrScale.value_or_default());
        }
        // Priced by the tiers when both are known (85% of a 1920x1080 frame on the 1080 tier = 1.00x, 80% on the 900
        // tier = 0.69x); else the size squared, times passes.
        const bool byTier = tiers.tier != 0 && tiers.tier100 != 0;
        const float f = byTier ? float(TierArea(tiers.tier) / TierArea(tiers.tier100)) * float(passes)
                               : s * s * float(passes);
        char cost[64];
        if (passes > 1)
            std::snprintf(cost, sizeof cost, "%.2fx cost (%d passes)", f, passes);
        else
            std::snprintf(cost, sizeof cost, "%.2fx cost", f);
        // Orange above the cost at 100%, green below, dim at it (plan B1); passes always count. With the running tier
        // known but not the tier at 100% only passes colour the tag, so it is never coloured wrongly.
        const bool tierUnpriced = tiers.tier != 0 && !byTier;
        const bool above = tierUnpriced ? passes > 1 : f > 1.02f;
        const bool below = !tierUnpriced && f < 0.98f;
        CostTag(cost, above ? &kCostAbove : below ? &kCostBelow : nullptr);
        if (IsLmxxf(rt))
        {
            const int cap = Lmxxf::TierCapInUse();
            // (0.3.4, lmxxf Fast mode) said while the running session feeds the network one tier lower
            const char* fastNote = Lmxxf::FastModeFromTier() > 0 ? " (Fast mode: one tier lower)" : "";
            char now[192] = {};
            if (tiers.tier != 0 && cap != 0)
                std::snprintf(now, sizeof now, "Runs the %d tier now (cap %d%s: [DlssNr] AmdLmxxfTierCap)%s.",
                              tiers.tier, cap,
                              config->AmdLmxxfTierCap.value_or_default() == 0 ? ", auto on this handheld" : "",
                              fastNote);
            else if (tiers.tier != 0)
                std::snprintf(now, sizeof now, "Runs the %d tier now%s.", tiers.tier, fastNote);
            // (0.3.4.2, H5) RX 9000 (RDNA 4, gfx120x: GpuSupport::target) runs 0.3.3.2's sizes unless the ini says
            // [DlssNr] AmdLmxxfTierSnap=true (LmxxfTierSnapDefault is on for gfx11 only): the hover names the opt-in
            // while the snap is off and no tier cap is set. Text only: no key is added, read differently or saved.
            // Numbers: lmxxf's probe on an RX 9070 XT, network only, outside a game (1080 tier 14.08 ms, 900 tier
            // 9.96 ms); the moves are SnapToNetworkTier's (LmxxfBackend.cpp): a size nearer the smaller tier drops to
            // it, one filling 90% or more of its own tier grows to fill it.
            const bool rdna4 = AmdBridge::GpuSupportInfo().target.rfind("gfx12", 0) == 0;
            const char* snapHint =
                rdna4 && cap == 0 && !AmdBridge::LmxxfTierSnapOn()
                    ? "RX 9000: the ini key [DlssNr] AmdLmxxfTierSnap=true (off by default here) moves the NR size to a"
                      " network size at every render resolution, not only at 2K Quality. Some sizes drop a tier (2K"
                      " Quality 1707x960 -> the 900 size: network 14.08 -> 9.96 ms, about -4.1 ms measured outside a"
                      " game, a slightly softer image); others grow inside their tier (80% of 1080p -> 1600x900)."
                    : nullptr;
            HelpForLastItem("Cost against 100% NR resolution, times Neural passes. lmxxf runs its network at a size tier"
                            " and costs what the tier costs. While dragging, with NR off or under Dynamic NR (the"
                            " slider is then its ceiling) this is the size-squared estimate.",
                            { Lmxxf::SmallTierPolicy()
                                  ? "Tiers: 360 and 576 on handhelds, 720, 900 or 1080 (up to 640x360, 1024x576,"
                                    " 1280x720, 1600x900, else 1920x1080)."
                                  : "Tiers: 720, 900 or 1080 (up to 1280x720, 1600x900, else 1920x1080).",
                              cap != 0 ? nullptr
                              : AmdBridge::LmxxfTierSnapOn()
                                  ? "From a 1920x1080 input, 85% and 80% run the 900 tier, 70% the 720 tier (the size"
                                    " snaps to the nearer tier: [DlssNr] AmdLmxxfTierSnap)."
                                  : "From a 1920x1080 input, 85% still runs the 1080 tier, 80% the 900 tier.",
                              snapHint, now });
        }
        else
        {
            HelpForLastItem("Cost against 100% NR resolution: the size squared, times Neural passes. Under Dynamic NR"
                            " it prices the slider, its ceiling.");
        }
    }
    // What a NEW NR resolution keeps (leak audits R3/R4), said while the slider is in the hand. danielblnc's closed
    // runtime keeps the buffers of every working size above about 1 MP until the game restarts (per pass; a size
    // already used is free); lmxxf makes its network buffers once per network size and reuses them, so only a small
    // rest stays (RuntimeCaps vramPerNewSize reads whether this runtime DLL reuses them).
    if (editingScale)
        ImGui::TextDisabled("   a new size keeps VRAM until restart (%s)", rt.vramPerNewSize());
    // A runtime that is slow at full size on a large input (danielblnc: about 45 ms per frame at 2560x1440 on an
    // RX 9070 XT). Stats' working size is the input at 100%. Dim, not orange: the page has one orange line (R5).
    if (rt.slowAbovePixels != 0 && AmdBridge::ActiveRuntime() == rt.id &&
        std::fabs(config->AmdNrScale.value_or_default() - 1.0f) < 0.001f)
    {
        const auto work = AmdBridge::Stats();
        if (static_cast<unsigned long long>(work.width) * work.height >= rt.slowAbovePixels)
            ImGui::TextDisabled("   slow at this size with %s: try 70-85%%", rt.name);
    }

    // ---- B2 Neural passes: staged like NR resolution; the Vulkan tag is drawn after the IsItem* reads ------------
    {
        static int advPasses = 1;
        static bool editAdvPasses = false;
        if (!editAdvPasses)
            advPasses = int(config->DlssNrPasses.value_or_default());
        // Stays live on a danielblnc Vulkan game: the value is kept for the player's other games (plan B2).
        Gate gate(danielVkOnePass, "1 pass in Vulkan games here", /*keepClickable*/ true);
        FillSlider("Neural passes", &advPasses, 1, 3);
        editAdvPasses = ImGui::IsItemActive();
        if (ImGui::IsItemDeactivatedAfterEdit())
            config->DlssNrPasses = uint32_t(advPasses);
        Help("The model runs 1-3 times on its own answer. 1 is normal; 2 and 3 cost about 2x and 3x the model"
             " time, with smaller gains each time.",
             { lmxxfMenu ? "lmxxf: extra passes run inside the same job, so the frame still never waits."
                         : "danielblnc: 1 pass in Vulkan games, whatever this says." },
             "[DlssNr] Passes");
    }

    // ---- B3 Full network: greyed where the runtime cannot choose its blocks (plan B3: shown, never hidden) --------
    // Applies live (the host drops the job in flight and the runtime rebuilds its network at the next model frame),
    // so it is saved with Save Settings, not at once like the runtime choice.
    {
        const bool available = RuntimeCaps::Get(Cap::FullNetwork) == Support::Yes;
        bool fullNet = config->LmxxfFullNetwork.value_or_default();
        // An LmxxfNrRuntime.dll older than the flag refused it (Lmxxf::FullNetworkRefused, a latch for the process):
        // the default network runs whatever the box says. 0.3.3.2 drew an orange paragraph; now a dim tag (R5).
        const bool refused = available && fullNet && Lmxxf::FullNetworkRefused();
        const char* tag = !available ? (rt.id == AmdBridge::NeuralRuntime::Daniel ? "not in danielblnc: closed runtime"
                                                                                  : RuntimeCaps::Tag(Cap::FullNetwork))
                                     : "refused: runtime DLL too old";
        Gate gate(!available || refused, tag, /*keepClickable*/ available);
        if (Checkbox("Full network (all 71 blocks)", &fullNet))
            config->LmxxfFullNetwork = fullNet;
        Help("Runs all 71 network blocks instead of skipping three: slightly more faithful, about 0.5 ms slower at"
             " 1080p. Changing it rebuilds the network (about a second's hitch) and restarts its history.",
             { refused ? "This lmxxf runtime is older than the option and refuses it; the default network runs."
                         " Use the runtime shipped with this build."
                       : nullptr },
             "[DlssNr] LmxxfFullNetwork");
    }

    // ---- (0.3.4, danielblnc support) Fast mode: danielblnc's quality mode, drawn only for a build that has one ----------------
    // [DlssNr] AmdDanielFastMode: unset = the runtime's own mode (the host writes nothing, as 0.3.3.2; Fast unless its
    // dlssnr_on_amd.ini says otherwise), true = Fast, false = Reference. The box shows what runs: the key once set
    // here, else the runtime's own mode (read when it loaded; before that its default, Fast), tagged "runtime's own".
    // Live from the next frame; no restart, no history restart. Not drawn for a danielblnc build without the mode (the
    // public runtime the release ships), so the default view stays the approved mock's; on lmxxf hidden and counted,
    // only while the danielblnc files in the folder have the mode (AmdBridge::DanielFastModeSupported).
    const bool fastModeBuild = AmdBridge::DanielFastModeSupported() == 1;
    const bool fastModeHidden = Hidden(Cap::DanielFastMode);
    if (fastModeBuild && !fastModeHidden)
    {
        const bool keySet = config->AmdDanielFastMode.has_value();
        bool fast = keySet ? *config->AmdDanielFastMode : AmdBridge::DanielFastModeRuntimeOwn() != 0;
        if (Checkbox("Fast mode", &fast))
            config->AmdDanielFastMode = fast;
        Help("Faster network at a small quality cost (danielblnc runtime option). Takes effect on the next frame, no"
             " restart. Unticked runs Reference quality.",
             { "Until changed here the runtime keeps its own setting: Fast, unless its dlssnr_on_amd.ini says"
               " Quality=reference.",
               "The gain is largest on RX 9000; on other cards less of the network gets faster." },
             "[DlssNr] AmdDanielFastMode (auto = the runtime's own)");
        if (!keySet)
            DimTag("runtime's own");
    }

    // ---- (0.3.4, lmxxf Fast mode) Fast mode on lmxxf: the same place and label as danielblnc's row above ------------
    // [DlssNr] AmdLmxxfFastMode (default off, saved like LmxxfFullNetwork: false writes "auto"): the host feeds the
    // network one size tier lower than the default sizing (LmxxfTierPolicy.h PlanLmxxfFastSize: 1080 -> 900, 900 -> 720;
    // 720 -> 576, 576 -> 360 only with the small tiers). Live, read once per Record; a toggle is a new fed size, handled
    // as an NR resolution change. Only one of the two rows is ever drawn (Cap::DanielFastMode / Cap::LmxxfFastMode
    // follow the menu runtime). Lmxxf::FastModeFromTier(): 0 off (or no frame yet), -1 on but already the smallest tier
    // here (a dim tag; the box stays clickable, it is a saved preference), else the tier the default sizing feeds.
    // Distinct ID, so menu_rows reports no new duplicate against danielblnc's "Fast mode".
    if (RuntimeCaps::Get(Cap::LmxxfFastMode) == Support::Yes)
    {
        bool lmxxfFast = config->AmdLmxxfFastMode.value_or_default();
        if (Checkbox("Fast mode##lmxxf", &lmxxfFast))
            config->AmdLmxxfFastMode = lmxxfFast;
        Help("Faster network at a small quality cost: about 29% less network time at 1080p (RX 9070 XT). Off by"
             " default.",
             { "lmxxf runs its network one size tier lower (1080p -> 900p, 900p -> 720p). The picture stays at full"
               " size; fine detail gets a little softer, less with RenoDX composition.",
               "Takes effect on the next frame. Like a new NR resolution, the switch rebuilds the network at the new"
               " size (a short hitch) and restarts its history." },
             "[DlssNr] AmdLmxxfFastMode");
        if (lmxxfFast && Lmxxf::FastModeFromTier() == -1)
            DimTag("already the smallest network size here");
    }

    // ---- B4 Dynamic NR resolution, B4a Target FPS (its kid, drawn only while it is on) ----------------------------
    bool dynRes = config->AmdDynamicRes.value_or_default();
    if (Checkbox("Dynamic NR resolution", &dynRes))
        config->AmdDynamicRes = dynRes;
    Help("Lowers and raises NR resolution in steps (100, 85, 70, 58, 50%) to hold the target frame rate; NR"
         " resolution above is the ceiling. Each step is a new NR size, which can keep some VRAM until the game"
         " restarts.",
         {}, "[DlssNr] AmdDynamicRes");
    {
        static int dynFps = 60;
        static bool editingDynFps = false;
        if (dynRes)
        {
            if (!editingDynFps)
                dynFps = config->AmdDynamicTargetFps.value_or_default();
            ImGui::Indent();
            FillSlider("Target FPS", &dynFps, 30, 240);
            editingDynFps = ImGui::IsItemActive();
            if (ImGui::IsItemDeactivatedAfterEdit())
                config->AmdDynamicTargetFps = dynFps;
            Help("The frame rate Dynamic NR tries to hold. Separate from the FPS limit in the Frame Gen tab.", {},
                 "[DlssNr] AmdDynamicTargetFps");
            ImGui::Unindent();
        }
        else
        {
            editingDynFps = false;
        }
    }

    // ---- B5 Model interleave: writes 0 or 2 ------------------------------------------------------------------------
    // A stored value above 2 (a hand-set 3 or 4 in the ini, the Handheld preset's 4 (owner decision 1, saved), or the
    // handheld default: AmdBridge sets 4, never saved, on an lmxxf APU while [DlssNr] AmdInterleave is unset) is its
    // own third item, "On (every Nth frame)", selected, and stays until Off or On (every 2nd frame) is picked, which
    // writes 0 or 2 as before (runtime work HB 9, option a). Its hover says which: value_for_config() holds the value
    // Save Settings writes (the ini's or the Handheld preset's), and is empty for the bridge's volatile default.
    {
        const float stored = config->AmdInterleave.value_or_default();
        const int every = stored > 2.f ? static_cast<int>(std::lround(stored)) : 0;
        const bool other = every > 2;
        const bool apuHandheld = AmdBridge::GpuSupportInfo().lmxxfExperimental;
        char nth[48] = {};
        char otherNote[224] = {};
        if (other)
        {
            std::snprintf(nth, sizeof nth, "On (every %d%s frame)", every, OrdinalSuffix(every));
            if (config->AmdInterleave.value_for_config().has_value())
                std::snprintf(otherNote, sizeof otherNote,
                              "Every %d%s frame is your saved value (%s). Picking Off or On (every 2nd frame)"
                              " replaces it.",
                              every, OrdinalSuffix(every),
                              apuHandheld ? "the Handheld preset or the ini" : "set in the ini");
            else
                std::snprintf(otherNote, sizeof otherNote,
                              "Every %d%s frame is this handheld's default while [DlssNr] AmdInterleave is unset (not"
                              " saved). Picking Off or On (every 2nd frame) saves your own.",
                              every, OrdinalSuffix(every));
        }
        const char* items[3] = { "Off", "On (every 2nd frame)", nth };
        int ilIdx = other ? 2 : stored > 1.f ? 1 : 0;
        if (Combo("Model interleave", &ilIdx, items, other ? 3 : 2) && ilIdx != 2)
            config->AmdInterleave = ilIdx == 1 ? 2.f : 0.f;
        // runtime work r1 request 2: on an lmxxf APU the bridge keeps an explicit Off as 1 (AmdBridge
        // KeepApuInterleaveOff), so a player who reads the ini knows why it says 1.
        const bool apuOffAsOne = IsLmxxf(rt) && apuHandheld;
        Help("Runs the model every second frame for a large frame-rate gain; the frames between show its last"
             " answer, filled as the Interleave preset says. Can ghost in fast motion: test per game.",
             { other ? otherNote : nullptr,
               apuOffAsOne ? "On this handheld Off is saved as [DlssNr] AmdInterleave=1 (also off)." : nullptr },
             "[DlssNr] AmdInterleave");
    }
    const bool ilOn = config->AmdInterleave.value_or_default() > 1.f;

    // ---- B5a Interleave preset: Model interleave's kid, drawn only while it is on (SPEC-content X3) --------------
    // One combo for every runtime, items from the runtime table. Replaces danielblnc's "Interleave preset" and lmxxf's
    // "Interleave fill" of 0.3.3.2; the same key [DlssNr] AmdInterleavePreset (default 10). A stored value is rewritten
    // only when the player picks an item. It does nothing while interleave is off, so it is not drawn then (0.3.4: it
    // was greyed with "needs Model interleave"); the pacing readout stays under it.
    if (ilOn)
    {
        ImGui::Indent();
        {
            const auto fills = rt.fills;
            const int shownIdx = ShownPresetIndex(rt, config->AmdInterleavePreset.value_or_default());
            // (0.3.4 final menu check) The closed combo shows the item without its " (default)": in the mock's 174 px
            // combo "Edit accumulation (default)" was clipped to "Edit accumulation (de", right under the Preset row
            // once the Handheld preset turns Model interleave on. The open list still names the default.
            std::string preview = fills.empty() ? std::string() : std::string(fills[shownIdx].label);
            if (constexpr std::string_view kDefaultSuffix = " (default)"; preview.ends_with(kDefaultSuffix))
                preview.resize(preview.size() - kDefaultSuffix.size());
            if (BeginCombo("Interleave preset", preview.c_str()))
            {
                for (size_t i = 0; i < fills.size(); ++i)
                {
                    const bool selected = static_cast<int>(i) == shownIdx;
                    if (ImGui::Selectable(fills[i].label, selected))
                    {
                        int value = fills[i].value;
                        if (IsLmxxf(rt) && value != kEditAccumulation)
                            value = kLmxxfClassicCarryValue; // D2
                        config->AmdInterleavePreset = value;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                    HelpForLastItem(fills[i].help);
                }
                ImGui::EndCombo();
            }
            Help("What the frames between two model runs show. Hover an item for what it does.", {},
                 "[DlssNr] AmdInterleavePreset");
        }
        DrawPacingReadout(config, IsLmxxf(rt));
        ImGui::Unindent();
    }
    // Removed from the page (plan section G): the locked-off Adaptive interleave controls (AmdBridge forces the
    // feature off; the ini keys are read and ignored as in 0.3.3.2) and the "Interleave pacing: automatic" line,
    // which was not a setting. "Fresh model history" and "Sharp fill" stay ini-only, as in 0.3.3.2.
    // (0.3.4, lmxxf Fast mode) Neither Fast mode row is counted as hidden: on lmxxf that place holds lmxxf's own "Fast
    // mode" (a "1 danielblnc-only option hidden" line would name the row the player is looking at; the count was taken
    // danielblnc's row here), and on danielblnc it holds danielblnc's own row, or nothing for the public runtime, whose
    // default view stays the mock's.
}

void DrawQuality(const Ctx& ctx)
{
    Config* config = ctx.config;
    const bool lmxxfMenu = ctx.lmxxfMenu;
    const RuntimeCaps::RuntimeInfo& rt = RuntimeCaps::Menu();
    const bool ilOn = config->AmdInterleave.value_or_default() > 1.f;
    const bool at100 = std::fabs(config->AmdNrScale.value_or_default() - 1.0f) < 0.001f;
    const bool dynRes = config->AmdDynamicRes.value_or_default();

    NrSection("Quality");

    // ---- C1 Residual strength (its own tooltip; the README calls it the control that changes the picture most) ----
    {
        float ri = config->AmdResidualIntensity.value_or_default();
        if (FillSlider("Residual strength", &ri, 0.f, 2.f))
            config->AmdResidualIntensity = std::clamp(ri, 0.f, 2.f);
        Help("How much of the model's change lands on the frame. 1 = as the model made it; above 1 amplifies it;"
             " 0 = the game's own image.",
             { ResidualNote(rt, config) }, "[DlssNr] AmdResidualIntensity");
    }

    // ---- C2 Residual limit: greyed where it cannot act ----------------------------------------------------------
    // danielblnc at exactly 100% NR resolution resolves with no limit (AmdPreSr.cpp, rLimit = 0 when not scaled). What
    // is left is the temporal pass's residual cap (AmdPreSr.cpp stabResidualCap, TemporalStability.h residualCap),
    // which bounds a carried edit by it only in the standalone Residual temporal pass (interleave off) and in two
    // interleave presets: Self-tuning (8; a stored 7 runs as 8) and a hand-set Residual temporal preset (4). Edit
    // accumulation (10; 9 and 11 run as 10) gets a fixed 4x sanity bound instead; Standard, Extra temporal, Guided
    // fill (run as Held frame, 3) and Guided fill v2 never read it. Dynamic NR steps below 100%.
    {
        const int preset = config->AmdInterleavePreset.value_or_default();
        const bool capsCarriedEdit = ilOn ? (preset == 4 || preset == 7 || preset == 8)
                                          : config->AmdResidualTemporal.value_or_default();
        const bool limitIdle =
            RuntimeCaps::Get(Cap::ResidualLimitAt100) != Support::Yes && at100 && !dynRes && !capsCarriedEdit;
        Gate gate(limitIdle, "only below/above 100%");
        float rl = config->AmdResidualLimit.value_or_default();
        if (FillSlider("Residual limit", &rl, 0.f, 2.f))
            config->AmdResidualLimit = std::clamp(rl, 0.f, 2.f);
        Help("A ceiling on how far one pixel may move, as a share of its brightness. Blotchy patches: lower it (a"
             " higher value lets more through). 0 = no limit.",
             { lmxxfMenu ? "lmxxf: acts on the model's edit at every NR resolution; 0 still caps at 1 with 2 or 3"
                           " passes."
                         : "danielblnc: at exactly 100% it only caps Residual temporal or the Self-tuning interleave"
                           " preset." },
             "[DlssNr] AmdResidualLimit");
    }

    // ---- C3 Temporal stability: applies live every frame (a post pass, no model rebuild) ------------------------
    float stability = config->AmdTemporalStability.value_or_default();
    if (FillSlider("Temporal stability", &stability, 0.f, 1.f))
        config->AmdTemporalStability = std::clamp(stability, 0.f, 1.f);
    Help("Holds the result steady between frames: higher = less shimmer, slightly more lag in motion. 0 = off.",
         { rt.noteTemporalStability }, "[DlssNr] AmdTemporalStability");

    // ---- C4 Sharpening (CAS) --------------------------------------------------------------------------------------
    float sharpness = config->AmdSharpness.value_or_default();
    if (FillSlider("Sharpening (CAS)", &sharpness, 0.f, 1.f))
        config->AmdSharpness = std::clamp(sharpness, 0.f, 1.f);
    Help("Contrast-adaptive sharpening after the neural pass; restores crispness the denoise softens. 0 = off."
         " Keep it light (0.3-0.6): an upscaler after it may sharpen again.",
         {}, "[DlssNr] AmdSharpness");

    // ---- C5 More quality options (folded, closed by default: the mock's default view ends at Sharpening) ----------
    // (0.3.4, owner decision 5) The tree's tag, over the rows it draws for this runtime: Network history's active key,
    // Stability mode unless hidden (Gate threshold is drawn only in mode 1, which is already custom), Residual temporal
    // only where it is a switch (lmxxf draws it ticked and locked).
    const bool stabilityHidden = Hidden(Cap::StabilityMode);
    const bool residualTemporalAlways = RuntimeCaps::Get(Cap::ResidualTemporalSwitch) == Support::Always;
    const bool moreCustom = IsCustom(config->*(rt.historyKey)) || IsCustom(config->AmdLmxxfOutputSmooth) ||
                            (!stabilityHidden && IsCustom(config->AmdStabilityMode)) ||
                            (!residualTemporalAlways && IsCustom(config->AmdResidualTemporal)) ||
                            IsCustom(config->AmdResidualFade) || IsCustom(config->AmdStabilityStaticRelax);
    const bool open = TreeNode("More quality options");
    HelpForLastItem("Network history, Output smoothing, Stability mode, Residual temporal, Residual edge fade and"
                    " Still-surface steadiness.");
    DimTag(moreCustom ? "custom" : "default");
    if (!open)
        return;

    // C5a Network history: one checkbox, the active runtime's key (RuntimeInfo::historyKey): [DlssNr] AmdLmxxfHistory
    // (default true) under lmxxf, AmdInterleaveModelHistory (default false) under danielblnc. Switching runtime shows
    // the other key's value; both keys and defaults are 0.3.3.2's. Both runtimes feed their network history with or
    // without Model interleave since port 7.2 (0.3.4; the table cell is Yes on both). The needsInterleave grey is kept
    // for a runtime whose cell is not Yes; a ticked box stays clickable there (R3), so it can be undone.
    {
        CustomOptional<bool>& historyKey = config->*(rt.historyKey);
        bool history = historyKey.value_or_default();
        const bool needsInterleave =
            RuntimeCaps::Get(Cap::NetworkHistoryWithoutInterleave) != Support::Yes && !ilOn;
        Gate gate(needsInterleave, "needs Model interleave here", /*keepClickable*/ history);
        if (Checkbox("Network history", &history))
            historyKey = history;
        // (0.3.4, P11) danielblnc's closed runtime builds its history when the key flips on: a one-off freeze of about
        // 4 s mid-game (Forza, SH2; reachable with interleave off since P7.2). The host has no lever on it; the hover
        // says so. lmxxf feeds its history per frame (no rebuild), so no line there.
        Help("Feeds the network its own previous answer, so it builds up over frames: stronger denoise, steadier"
             " detail; can ghost in fast motion. Each runtime keeps its own setting.",
             { IsLmxxf(rt) ? nullptr : "danielblnc: switching it on mid-game costs a one-off pause of a few seconds." },
             IsLmxxf(rt) ? "[DlssNr] AmdLmxxfHistory" : "[DlssNr] AmdInterleaveModelHistory");
    }

    // C5b Output smoothing: upstream's output-side smoothing inside the lmxxf runtime; it needs Network history there.
    {
        Gate gate(Cap::OutputSmoothing, lmxxfMenu && !config->AmdLmxxfHistory.value_or_default(),
                  "needs Network history");
        float os = config->AmdLmxxfOutputSmooth.value_or_default();
        if (FillSlider("Output smoothing", &os, 0.f, 1.f))
            config->AmdLmxxfOutputSmooth = std::clamp(os, 0.f, 1.f);
        Help("Smooths the network's own answer over time: small changes against its previous answer are blended"
             " away, large ones pass through. 0 = off; above 0.8 adds visible lag.",
             {}, "[DlssNr] AmdLmxxfOutputSmooth");
    }

    // C5c Stability mode (+ Gate threshold, its kid while Difference-gated): acts on a reprojected picture, which the
    // lmxxf carry never uses; hidden and counted there (D3). The Gates only grey it if RuntimeCaps::kHideUnsupported
    // is reverted to false. (stabilityHidden: above the tree.)
    if (!stabilityHidden)
    {
        int stabMode = std::clamp(config->AmdStabilityMode.value_or_default(), 0, 1);
        {
            Gate gate(Cap::StabilityMode);
            if (Combo("Stability mode", &stabMode, "Variance clamp\0Difference-gated (cleaner)\0"))
                config->AmdStabilityMode = stabMode;
            Help("How Temporal stability decides what to keep. Variance clamp (default) holds strongly; Difference-gated"
                 " keeps history only where it already matches, so it shimmers less on flat areas and cannot ghost.",
                 {}, "[DlssNr] AmdStabilityMode");
        }
        if (stabMode == 1)
        {
            ImGui::Indent();
            {
                Gate gate(Cap::StabilityMode);
                float threshold = config->AmdStabilityThreshold.value_or_default();
                if (FillSlider("Gate threshold", &threshold, 0.02f, 0.3f, "%.3f"))
                    config->AmdStabilityThreshold = std::clamp(threshold, 0.005f, 0.5f);
                Help("How different a pixel may be from its history and still be smoothed: lower touches only the"
                     " faintest shimmer, higher smooths more but can trail. About 0.05-0.12 works well.",
                     {}, "[DlssNr] AmdStabilityThreshold");
            }
            ImGui::Unindent();
        }
    }

    // C5d Residual temporal: danielblnc's standalone switch (Model interleave's presets do the same for the frames it
    // fills, so it is greyed while interleave is on); lmxxf's carry IS residual temporal: drawn ticked and locked.
    {
        const bool always = residualTemporalAlways;
        bool residualTemporal = always || config->AmdResidualTemporal.value_or_default();
        Gate gate(Cap::ResidualTemporalSwitch, ilOn, "handled by Interleave preset");
        if (Checkbox("Residual temporal", &residualTemporal) && !always)
            config->AmdResidualTemporal = residualTemporal;
        Help("Keeps the model's edit, not the picture, steady between frames, so detail does not smear and nothing"
             " can ghost. How much it holds is Temporal stability.",
             {}, "[DlssNr] AmdResidualTemporal");
    }

    // C5e Residual edge fade: acts only where the edit is lifted, i.e. where the network is fed another size than the
    // frame's. danielblnc: away from exactly 100% (Dynamic NR steps). lmxxf (port 7.8, LmxxfBackend `editLifted`: the
    // fed size differs from the frame's): also at 100% when the render is above the network's 1920x1080 ceiling or the
    // size was moved to a tier box (the tier snap as it applies, AmdBridge::LmxxfTierSnapOn: on by default on RDNA 3,
    // runtime work HB 2; a [DlssNr] AmdLmxxfTierCap cap; the small tiers). When the frame is known (FrameOfWorkingSize)
    // that is exactly "working size != frame"; else it greys at 100% only while the running lmxxf backend's working
    // size is below the ceiling and nothing moves it; unknown = live.
    // R3: while the runtime table says Planned for the active runtime, a saved non-zero value stays clickable, so it
    // can always be set back to 0.
    {
        bool fadeApplies = !at100 || dynRes;
        if (!fadeApplies && IsLmxxf(rt))
        {
            const bool running = ctx.enabled && AmdBridge::ActiveRuntime() == rt.id;
            const auto work = running ? AmdBridge::Stats() : decltype(AmdBridge::Stats()) {};
            unsigned frameW = 0, frameH = 0;
            if (FrameOfWorkingSize(work.width, work.height, config->AmdNrScale.value_or_default(), frameW, frameH))
                fadeApplies = work.width != frameW || work.height != frameH;
            else
                fadeApplies = AmdBridge::LmxxfTierSnapOn() || Lmxxf::TierCapInUse() != 0 || Lmxxf::SmallTierPolicy() ||
                              Lmxxf::FastModeFromTier() > 0 || work.width == 0 || work.height == 0 || work.width >= 1920 || work.height >= 1080;
        }
        float rf = config->AmdResidualFade.value_or_default();
        const bool keepUndoable = RuntimeCaps::Get(Cap::ResidualEdgeFade) == Support::Planned && rf > 0.f;
        Gate gate(Cap::ResidualEdgeFade, !fadeApplies, "only below/above 100%", keepUndoable);
        if (FillSlider("Residual edge fade", &rf, 0.f, 0.25f))
            config->AmdResidualFade = std::clamp(rf, 0.f, 0.25f);
        Help("Fades the lifted edit near the screen border, where resampling can ring. Acts only where the edit is"
             " lifted: away from 100% NR resolution.",
             { IsLmxxf(rt) ? "lmxxf: also at 100% above a 1920x1080 input (the network's ceiling) or when the size snaps"
                             " to a tier; not under Network output."
                           : nullptr },
             "[DlssNr] AmdResidualFade");
    }

    // C5f Still-surface steadiness ([DlssNr] AmdStabilityStaticRelax, 0 = off; plan N1). (0.3.4) The last row of this
    // tree (SPEC-content X4): the mock's Quality list ends at Sharpening. Both runtimes: the danielblnc every-frame clamp
    // (and its Residual temporal edit clamp), the lmxxf carried-edit clamp. Committed when the drag ends, as Target FPS
    // (each change is one log line). Greyed where it does nothing, with the reason: Temporal stability 0, danielblnc
    // under Model interleave, lmxxf under Edit accumulation (stored 9..11 run as 10). Not a style field; the presets
    // leave it alone.
    {
        static float staticRelax = 0.f;
        static bool editingStaticRelax = false;
        if (!editingStaticRelax)
            staticRelax = std::clamp(config->AmdStabilityStaticRelax.value_or_default(), 0.f, 1.f);
        const char* idle = !(stability > 0.f)  ? "needs Temporal stability"
                         : (!lmxxfMenu && ilOn)  ? "not with Model interleave"
                         : (lmxxfMenu && ilOn && config->AmdInterleavePreset.value_or_default() >= 9)
                                                 ? "not with Edit accumulation"
                                                 : nullptr;
        Gate gate(Cap::StillSurfaceSteadiness, idle != nullptr, idle);
        FillSlider("Still-surface steadiness", &staticRelax, 0.f, 1.f, "%.2f");
        editingStaticRelax = ImGui::IsItemActive();
        if (ImGui::IsItemDeactivatedAfterEdit())
            config->AmdStabilityStaticRelax = std::clamp(staticRelax, 0.f, 1.f);
        Help("Steadies shadows and flat areas that pulse while nothing moves; moving objects are unchanged. 0 = off."
             " Higher values can make lighting that changes on still walls lag slightly.",
             { lmxxfMenu ? "lmxxf: acts on the carried edit; not under Edit accumulation."
                         : "danielblnc: acts without Model interleave only." },
             "[DlssNr] AmdStabilityStaticRelax");
    }

    HiddenCount({ stabilityHidden ? "Stability mode" : nullptr },
                "Stability mode: lmxxf's counterpart is Output smoothing.");
    ImGui::TreePop();
}
} // namespace DlssNr::NeuralUi
