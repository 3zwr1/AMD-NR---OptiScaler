// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
// moved from dlssnr/DlssNr_Menu.cpp
#pragma once
// Shared helpers and state of the Neural tab's section files (plan items T2/T3,
// the menu-cleanup design note section 9).
//
// Menu thread only; nothing here is called from a render thread. (0.3.4 menu rework: unfrozen for the owner-approved
// look; the signatures below that existed at W2-M1 are unchanged, only bodies changed and helpers were added.)
//
// The files (0.3.4 W2-M1 split of dlssnr/DlssNr_Menu.cpp; "M:" = its line at 0.3.3.2, c2f392d):
//   dlssnr/DlssNr_Menu.cpp       DlssNr::RenderMenu (the dispatcher, draw order) and RenderNvidiaPath (M:2440-3524)
//   dlssnr/menu/NeuralTop.cpp    DrawTop, DrawMissingRuntimeCard, DrawAttention, DrawPlacement,
//                                DrawPresetAndStyle, DrawLive (M:248-836) and the NR style tables (M:32-117);
//                                (G2) DrawPlacement and DrawLive draw nothing since W2-M2a (plan A4/A5)
//   dlssnr/menu/NeuralPerfQuality.cpp  DrawPerformance, DrawQuality (M:889-1514)
//   dlssnr/menu/NeuralLook.cpp   DrawRayRegeneration, DrawImageLook, DrawAppearanceFilter (M:1516-2163, 2367-2431)
//   dlssnr/menu/NeuralTools.cpp  DrawReadouts, DrawRrDebugView, DrawTools (M:838-888, 1623-1716, 2166-2364);
//                                (G2) DrawReadouts draws nothing since W2-M3b (the readouts are in Diagnostics)
//   dlssnr/menu/NeuralUi.*       this: the shared helpers and state
//
// ==== THE MENU LOOK (0.3.4, the owner-approved mock) ============================================================
// Spec: the owner-approved menu mock (every row, colour and size), rendered 1:1 in
// APPROVED_MOCK.png; analyses SPEC-style.md / SPEC-content.md next to it. The main window pushes the mock's theme
// (colours, paddings, a 14 px font for the mock's 12.5 px text, 24 px rows; menu/menu_common.cpp MainMenuTheme) for
// every tab, and draws the header rows, the text-only tab bar and the one-line footer itself. A section file only
// calls the helpers below.
// Sizes are "mock px" and follow Menu Scale (Px()); colours come from MenuCommon::ThemeColor (HDR tone-mapped).
//
//   Px(mockPx)            one mock pixel at the current font size (Menu Scale and a [Menu] FontSize override;
//                         MenuCommon::MockPx).
//   ControlWidth()        190 mock px, the slider width: the tab pushes it as the item width of every tab.
//   ComboWidth()          174 mock px, the combo width (the mock's combos are 16 px narrower than its sliders).
//   FillSlider(label, &v, min, max, fmt = "%.2f" / "%d", flags)
//                         the mock's slider: a 14 px track filled from the left in proportion to the value, the
//                         value right-aligned inside it, no grab, the label after it. One ImGui item (drag,
//                         Ctrl+Click typing, keyboard/gamepad tweak, IsItemActive / IsItemDeactivatedAfterEdit all as
//                         ImGui::SliderFloat / SliderInt), so R9 holds: read IsItem* right after it. Floats default to
//                         2 decimals ("%.3f" only where the mock shows 3: Disocclusion threshold; "%.0f" for whole
//                         numbers such as Radius 16). Replaces ImGui::SliderFloat / ImGui::SliderInt.
//   Checkbox(label, &b)   12 px square: outline when off, solid red when on, no tick; 24 px row. Replaces
//                         ImGui::Checkbox.
//   Button(label, size = {}, selected = false)
//                         18 px, grey with a 1 px red-brown border; selected = filled red (Preset, open tool).
//                         Replaces ImGui::Button and ImGui::SmallButton (the mock has one button height).
//   Combo(label, &i, "A\0B\0") / Combo(label, &i, items, n) / BeginCombo(label, preview [, flags]) + ImGui::EndCombo()
//                         one flat box with a small "v", ComboWidth() wide unless SetNextItemWidth came first.
//   TreeNode(label [, flags]) + ImGui::TreePop()
//                         the mock's fold row: ">" / "v", label 15 px in, children 22 px in (IndentSpacing).
//   SectionHeader(label)  red title with a 1 px rule under it across the width (Performance, Quality, ...).
//                         NrSection(label) is the same (kept for its callers).
//   DimTag(text)          the short grey word 16 px after a row's label ("running", "1.00x cost", "off").
//   LabelWithHelp(label, body [, footnotes, ini])
//                         a plain label on the same line as the controls before it ("Preset" after the three
//                         buttons) whose hover shows the help.
//   Help(body [, footnotes, ini]) / HelpForLastItem(...) / HelpMarker(tip)
//                         no "(?)" any more: the help opens when the LABEL of the last item is hovered (for a
//                         slider or combo only its label part, so a tooltip never covers the bar while aiming), or
//                         when it has keyboard/gamepad focus; also while the row is greyed. Call it right after
//                         the widget it explains (it draws nothing, so it may also follow a slider's IsItem* reads).
//   ToolButtons(id, labels, active)
//                         the compact left-aligned tools row (Diagnostics / Runtime options / Experimental); the
//                         open one red. Segmented(..., fillWidth = false) is the same row.
//   Gate / HiddenCount / StatusLine / AttentionSlot
//                         as before; a greyed row is at 45 % with its tag greyed too; the hidden-count and status
//                         lines carry their hover on the text itself (no "(?)").
// Rows the mock does not show in its default view go into its trees/drawers ("More quality options", "Model
// strength", "Exposure and highlights", "Appearance filter", Diagnostics / Runtime options / Experimental).
// The NVIDIA path (RenderNvidiaPath) keeps 0.3.3.2's helpers through NeuralUi::Legacy (below); only the theme's
// colours reach it.
#include <dlssnr/amd/RuntimeCaps.h>

#include <Config.h>

#include <imgui/imgui.h>

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace DlssNr::NeuralUi
{
// ---- T3: what every section is handed, and the state the sections share -----------------------------------

struct Ctx
{
    Config* config = nullptr;
    float menuResScale = 1.0f;
    bool enabled = false;   // "Enable Neural Rendering" as drawn this frame (DrawTop's result)
    bool lmxxfMenu = false; // the runtime the controls talk to is lmxxf (MenuRuntimeIsLmxxf, set where M:572 set it)
};

// Function statics of 0.3.3.2's RenderMenu that more than one section reads (or will read after the W2 moves),
// kept in one object with static storage (NeuralUi.cpp): the same values and the same lifetime as the statics.
struct NeuralState
{
    // NR resolution slider (M:890-891): the value while dragged; the cost and VRAM lines read it too
    float scale = 100.f;
    bool editingScale = false;
    // Tools row (M:2172): the open sub-tab, -1 = none. (G2) The row starts closed since W2-M3b: DrawTools sets -1
    // on its first call (the 0 here is 0.3.3.2's start value, kept because this header is frozen).
    int nrSub = 0;
    // Retry lmxxf on a Vulkan title (M:412)
    bool vkRetried = false, vkRetryNow = false, vkRetryFailed = false;
    // Live rates (M:703-705)
    unsigned long long liveLastTick = 0, liveLastRec = 0, liveLastModel = 0, liveLastSkip = 0;
    float nrFps = 0.f, modelFps = 0.f;
    unsigned long long skipRate = 0;
    // "Idle" after 5 s without a recorded frame (M:787)
    double idleSince = -1.0;
    // The backend status line's half-second debounce (M:814-815)
    std::string statusPending, statusShown;
    double statusPendingSince = 0.0, statusShownAt = 0.0;
};
NeuralState& Shared();

// ---- 0.3.4 menu look: the widgets of the owner-approved mock (see the block at the top) -------------------------

float Px(float mockPx);
float ControlWidth();
float ComboWidth();
bool FillSlider(const char* label, float* v, float vMin, float vMax, const char* format = "%.2f",
                ImGuiSliderFlags flags = 0);
bool FillSlider(const char* label, int* v, int vMin, int vMax, const char* format = "%d", ImGuiSliderFlags flags = 0);
bool Checkbox(const char* label, bool* v);
bool Button(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f), bool selected = false);
bool Combo(const char* label, int* current, const char* itemsSeparatedByZeros, int popupMaxHeightInItems = -1);
bool Combo(const char* label, int* current, const char* const items[], int itemsCount, int popupMaxHeightInItems = -1);
bool BeginCombo(const char* label, const char* previewValue, ImGuiComboFlags flags = 0);
bool TreeNode(const char* label, ImGuiTreeNodeFlags flags = 0);
void SectionHeader(const char* label);
void DimTag(const char* text);
void LabelWithHelp(const char* label, const char* body, std::initializer_list<const char*> footnotes = {},
                   const char* ini = nullptr);
int ToolButtons(const char* id, std::span<const char* const> labels, int active);
// Whether the last item's help should open now (the rule Help() uses; menu_common.cpp's ShowHelpMarker uses it too).
bool LastItemHelpHovered();
// DeferredSlider's Reset: drawn on the slider's line only while `show`; the slider stays the last item, so a Help()
// after DeferredSlider still explains the slider.
bool ResetAfterSlider(const char* label, bool show);

// ---- 0.3.3.2 helpers, moved from dlssnr/DlssNr_Menu.cpp (bodies restyled in 0.3.4) ------------------------

// The help of the row before it. (0.3.4) No "(?)" any more: the tooltip opens on the last item's label (as Help()).
// AllowWhenDisabled, as MenuCommon::ShowTooltip does.
void HelpMarker(const char* tip);

// A slider that only writes its value when the handle is released.
//
// Some controls -- intensity, the structure and tone strengths -- are read by the model once, when
// the feature is built, so changing one rebuilds the whole feature. Writing on every pixel of a drag
// meant a rebuild per frame, felt as the picture hitching while you scrub. The slider still tracks
// live under the cursor; only the commit that triggers the rebuild waits for release. Cheap controls
// that are just shader constants (detail, colour, paper white) do not use this -- they can afford to
// apply live.
// (0.3.4) Used by NeuralLook.cpp (the NVIDIA path in DlssNr_Menu.cpp keeps Legacy::DeferredSlider). Drawn with
// FillSlider; its Reset button shows only while the stored value differs from `def` (inheritReset: while a value is
// stored), the only time it does anything.
template <typename Option>
bool DeferredSlider(const char* label, Option* opt, float mn, float mx,
                    float def, const char* fmt = "%.2f", bool inheritReset = false)
{
    static std::unordered_map<ImGuiID, float> pending;
    const ImGuiID id = ImGui::GetID(label);

    auto it = pending.find(id);
    float value = it != pending.end() ? it->second : (opt->has_value() ? opt->value() : def);
    bool changed = false;

    if (FillSlider(label, &value, mn, mx, fmt))
        pending[id] = value;

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        auto committed = pending.find(id);

        if (committed != pending.end())
        {
            *opt = std::clamp(committed->second, mn, mx);
            pending.erase(committed);
            changed = true;
        }
    }

    const bool resettable = opt->has_value() && (inheritReset || opt->value() != def);
    if (ResetAfterSlider(label, resettable))
    {
        if (inheritReset)
            *opt = std::optional<float> {};
        else
            *opt = def;
        pending.erase(id);
        changed = true;
    }

    return changed;
}

// Stage costly neural parameter edits in ImGui state. Keep rendering
// with the committed parameters until release/text-edit completion.
// (0.3.4) A local generic lambda of RenderMenu until 0.3.3.2 (M:683-693); the same body as a function template, drawn
// with FillSlider (2 decimals).
template <typename Option>
void neuralSlider(const char* label, Option& option, float lo, float hi)
{
    auto storage=ImGui::GetStateStorage();
    const ImGuiID id=ImGui::GetID(label);
    const ImGuiID activeId=id ^ 0x6e72534cu;
    float value=storage->GetBool(activeId,false)?storage->GetFloat(id):option.value_or_default();
    FillSlider(label,&value,lo,hi);
    const bool active=ImGui::IsItemActive();
    const bool commit=ImGui::IsItemDeactivatedAfterEdit();
    storage->SetFloat(id,value);storage->SetBool(activeId,active);
    if(commit)option=value;
}

// A section heading. (0.3.4) The mock's: the same as SectionHeader(label).
void NrSection(const char* label);

// The runtime the tab's controls and notes talk to. Built already: that one. Not yet: the one the
// choice says, if its files are there. Not the "Neural runtime" combo, which after a change in this
// session, or while lmxxf_vk_launch.pending holds lmxxf back, names the next start's runtime.
// (0.3.4, plan T1) The rule lives in RuntimeCaps::Menu(); this forwards to it.
bool MenuRuntimeIsLmxxf();

// ---- T2 helpers (new in 0.3.4; not drawn by the 0.3.3.2 layout) -------------------------------------------

// The help of the last item (0.3.4: no "(?)"; the tooltip opens on the item's label, see the block at the top); it
// opens while the row is disabled too. Plan R4: `body` at most 4 lines of about 60 characters; `footnotes` one line
// per runtime ("lmxxf: ..."; nullptr entries are skipped), usually RuntimeInfo notes; `ini` the key as
// "[Section] Key", drawn last and dim as "ini: ...".
void Help(const char* body, std::initializer_list<const char*> footnotes = {}, const char* ini = nullptr);
// The same (kept for its callers: a combo item, a button, a tree header).
void HelpForLastItem(const char* body, std::initializer_list<const char*> footnotes = {}, const char* ini = nullptr);

// Greys one row and says why on the same line (plan R2/R3). Construct it right before the row's widget; it
// ends when the scope closes, so put the scope's end AFTER the widget's IsItemActive/IsItemDeactivatedAfterEdit
// reads (R9: nothing may be drawn between a staged slider and those reads; the tag is drawn by the destructor).
// A Help() inside the scope sits between the row and the tag.
class Gate
{
  public:
    // Greyed when the active runtime does not have `cap` (Planned / No) or always has it on (Always), or when `off`
    // (a condition on other settings). The tag is RuntimeCaps::Tag(cap) when the cap greys the row, else `offTag`.
    // keepClickable (R3, stuck value): the row stays enabled, e.g. while a stored non-default value still acts
    // and must stay undoable; the tag is still drawn.
    explicit Gate(RuntimeCaps::Cap cap, bool off = false, const char* offTag = nullptr, bool keepClickable = false);
    // A condition only, no cap ("needs Model interleave").
    Gate(bool off, const char* offTag, bool keepClickable = false);
    ~Gate();
    Gate(const Gate&) = delete;
    Gate& operator=(const Gate&) = delete;

    bool Greyed() const { return greyed_; }     // the row is greyed or tagged
    bool Disabled() const { return disabled_; } // BeginDisabled is open

  private:
    const char* tag_ = nullptr;
    bool greyed_ = false;
    bool disabled_ = false;
};

// A row of buttons, the active one drawn selected (red; the tools-row style). `id` scopes the buttons' IDs. fillWidth:
// the buttons share the available width equally; else each fits its label. Returns the index clicked this frame,
// or -1; clicking the active one returns its index too (the caller decides whether that closes it).
int Segmented(const char* id, std::span<const char* const> labels, int active, bool fillWidth = false);

// Plan D3: a row the active runtime cannot have (Support::No) is hidden while RuntimeCaps::kHideUnsupported.
// Never for the caps RuntimeCaps::NeverHidden() lists (Full network, Residual limit at 100%, Debug view without
// interleave, all passes on Vulkan): those rows stay on screen and grey (or tag) through a Gate instead.
bool Hidden(RuntimeCaps::Cap cap);
// The dim line under a group whose rows Hidden() skipped: "1 danielblnc-only option hidden"; its hover lists the
// rows (nullptr entries skipped) and `why`. Draws nothing when every entry is nullptr. (0.3.4 MENU match1) Placed as
// the mock's dim lines (see StatusLine).
void HiddenCount(std::initializer_list<const char*> hiddenLabels, const char* why = nullptr);

// Plan A4: one dim status line; `hover` (several lines allowed; '%' is safe in both) opens on the line itself.
// (0.3.4 MENU match1) Placed as the mock's plain dim lines: its cap tops 2 px under the row above, 14 px under a dim
// line above (ImGui's own flow puts them 3 and 6 px lower).
void StatusLine(const char* text, const char* hover);

// Plan A5: 0 or 1 orange line, an optional button, and a dim "(+N)" whose hover lists the other offers.
class AttentionSlot
{
  public:
    // Offers are ranked by call order: the first one is drawn. `text` is wrapped; `hover` opens on the text (empty =
    // none); `button` is a Button label with its own "##id" (nullptr = none).
    void Offer(std::string text, std::string hover = {}, const char* button = nullptr);
    bool Empty() const { return items_.empty(); }
    // Draws the winner. Returns the offer index (0 = the first offered) whose button was clicked, or -1.
    int Draw() const;

  private:
    struct Item
    {
        std::string text;
        std::string hover;
        const char* button = nullptr;
    };
    std::vector<Item> items_;
};

// ---- T3: the sections, in 0.3.3.2's draw order (DlssNr::RenderMenu calls them) ------------------------------

// NeuralTop.cpp
bool DrawTop(const Ctx& ctx);                // M:248-543; returns "Enable Neural Rendering" as drawn
void DrawMissingRuntimeCard(const Ctx& ctx); // M:545-557 (0.3.3.2: then the NVIDIA layout follows)
void DrawAttention(const Ctx& ctx);          // M:560-567, danielblnc's standalone found beside the game
void DrawPlacement(const Ctx& ctx);          // M:575-594, "AMD processing: ..."
void DrawPresetAndStyle(const Ctx& ctx);     // M:595-680, Preset, NR style, Custom style slots
void DrawLive(const Ctx& ctx);               // M:695-836, the Live section
// NeuralTools.cpp
void DrawReadouts(const Ctx& ctx);           // M:838-888, model-frame ghost / self-tuning / edit accumulation
// NeuralPerfQuality.cpp
void DrawPerformance(const Ctx& ctx);        // M:889-1439
void DrawQuality(const Ctx& ctx);            // M:1440-1514
// NeuralLook.cpp
void DrawRayRegeneration(const Ctx& ctx);    // M:1516-1883 (calls DrawRrDebugView where M:1623-1716 stood)
void DrawImageLook(const Ctx& ctx);          // M:1885-2163
void DrawAppearanceFilter(const Ctx& ctx);   // M:2367-2431; (G2) the rows of Image look's Appearance filter tree
                                             // (DrawImageLook calls it; 0.3.3.2's tools-row "Appearance" sub-tab)
// NeuralTools.cpp
void DrawRrDebugView(const Ctx& ctx);        // M:1623-1716
void DrawTools(const Ctx& ctx);              // M:2166-2364: Diagnostics / Runtime options / Experimental (G2: the
                                             // "Appearance" dispatch of M:2365-2432 is gone, see DrawAppearanceFilter)

// ---- The NVIDIA path's helpers, as 0.3.3.2 drew them ------------------------------------------------------------
// RenderNvidiaPath (dlssnr/DlssNr_Menu.cpp) is moved verbatim and stays untouched: it keeps the "(?)" markers, the
// SeparatorText headings and the stock slider with its Reset button through these copies of the 0.3.3.2 bodies
// (the using-declarations above it name them).
namespace Legacy
{
void HelpMarker(const char* tip);
void NrSection(const char* label);

template <typename Option>
bool DeferredSlider(const char* label, Option* opt, float mn, float mx,
                    float def, const char* fmt = "%.2f", bool inheritReset = false)
{
    static std::unordered_map<ImGuiID, float> pending;
    const ImGuiID id = ImGui::GetID(label);

    auto it = pending.find(id);
    float value = it != pending.end() ? it->second : (opt->has_value() ? opt->value() : def);
    bool changed = false;

    if (ImGui::SliderFloat(label, &value, mn, mx, fmt))
        pending[id] = value;

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        auto committed = pending.find(id);

        if (committed != pending.end())
        {
            *opt = std::clamp(committed->second, mn, mx);
            pending.erase(committed);
            changed = true;
        }
    }

    ImGui::SameLine();

    const std::string resetId = std::string("Reset##") + label;
    if (ImGui::SmallButton(resetId.c_str()))
    {
        if (inheritReset)
            *opt = std::optional<float> {};
        else
            *opt = def;
        pending.erase(id);
        changed = true;
    }

    return changed;
}
} // namespace Legacy
} // namespace DlssNr::NeuralUi
