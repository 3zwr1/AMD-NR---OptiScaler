// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
// moved from dlssnr/DlssNr_Menu.cpp
#include "pch.h"
#include "NeuralUi.h"

#include <menu/menu_common.h> // MenuCommon::ThemeColor: the mock's palette through the HDR tone map

#include <imgui/imgui_internal.h>

#include <cmath>
#include <cstdio>

namespace DlssNr::NeuralUi
{
namespace
{
// Every text a runtime or the ini can influence is drawn with TextUnformatted or "%s" (R8: a '%' in a status
// once crashed the menu, M:1176-1188 at 0.3.3.2).
void TooltipBody(const char* body, std::initializer_list<const char*> footnotes, const char* ini)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
    if (body != nullptr && *body)
        ImGui::TextUnformatted(body);
    bool first = true;
    for (const char* note : footnotes)
    {
        if (note == nullptr || !*note)
            continue;
        if (first)
            ImGui::Spacing();
        first = false;
        ImGui::TextUnformatted(note);
    }
    if (ini != nullptr && *ini)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("ini: %s", ini);
    }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// The runtime the hidden rows belong to: the other one.
const char* OtherRuntimeName()
{
    const auto& menu = RuntimeCaps::Menu();
    for (const auto& r : RuntimeCaps::All())
        if (r.id != menu.id)
            return r.name;
    return menu.name;
}

// (0.3.4) Whether the last item's help opens now. The item is hovered (after ImGui's short tooltip delay, also while
// greyed) and not being dragged, or it has keyboard/gamepad focus. For a framed widget with its label after it (a
// slider, a combo: ItemAdd gets the frame as its nav rect) the mouse must be over the LABEL part, so a tooltip never
// covers the bar while the player aims at it.
bool HelpHovered()
{
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled))
        return false;
    if (ImGui::IsItemActive())
        return false;
    const ImGuiContext& g = *GImGui;
    const ImGuiLastItemData& it = g.LastItemData;
    if (!g.NavHighlightItemUnderNav && it.NavRect.Max.x < it.Rect.Max.x - 1.0f && g.IO.MousePos.x < it.NavRect.Max.x)
        return false;
    return true;
}

ImVec4 Transparent() { return ImVec4(0.0f, 0.0f, 0.0f, 0.0f); }

// (0.3.4 MENU match1) A dim text line where the mock's flow puts it. The mock's plain dim lines (a div of text, about
// 14 px high) sit right under a 24 px row and 14 px under each other; ImGui puts every line 6 px under the item
// above, which is 3 px lower after a frame row (18 + 6 against the mock's centred 24) and 6 px lower after a text
// line (14 + 6 against 14). The line above is a text line when it was lower than a frame.
void DimLineFlow()
{
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window == nullptr || window->SkipItems)
        return;
    const float prevLine = window->DC.PrevLineSize.y;
    if (prevLine <= 0.0f)
        return;
    const bool afterText = prevLine < ImGui::GetFrameHeight() - 0.5f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImFloor(Px(afterText ? 6.0f : 3.0f)));
}

// ---- FillSlider: ImGui's slider with its own drawing hidden, then the mock's drawn over the same item -------------
// ImGui::SliderScalar keeps every behaviour (drag, Ctrl+Click typing, nav, MarkItemEdited, the item's ID and rects),
// so a caller's IsItemActive / IsItemDeactivatedAfterEdit read the slider itself; nothing here adds an item.
struct SliderLook
{
    bool skip = true;
    ImGuiWindow* window = nullptr;
    ImVec2 pos {};
    float width = 0.0f;
    ImGuiID id = 0;
    bool hid = false;
};

SliderLook BeginSliderLook(const char* label)
{
    SliderLook l;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window == nullptr || window->SkipItems)
        return l;
    l.skip = false;
    l.window = window;
    l.pos = window->DC.CursorPos;
    l.width = ImGui::CalcItemWidth(); // honours a SetNextItemWidth without consuming it
    l.id = window->GetID(label);
    // While Ctrl+Click typing runs, ImGui draws its input box and the label: leave those visible.
    l.hid = !ImGui::TempInputIsActive(l.id);
    if (l.hid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Transparent());
        ImGui::PushStyleColor(ImGuiCol_FrameBg, Transparent());
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Transparent());
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Transparent());
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, Transparent());
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Transparent());
    }
    // No grab: the whole track maps to the range (an int keeps one cell per value).
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 0.0f);
    return l;
}

// Where the value sits on the track, 0..1. Ints fill to the end of their cell (1 of 1..3 = one third, as the mock).
float FillRatio(ImGuiDataType type, const void* v, const void* vMin, const void* vMax, ImGuiSliderFlags flags)
{
    if (type == ImGuiDataType_S32)
    {
        const double a = *static_cast<const int*>(vMin), b = *static_cast<const int*>(vMax);
        const double x = *static_cast<const int*>(v);
        if (b <= a)
            return 1.0f;
        return static_cast<float>(ImClamp((x - a + 1.0) / (b - a + 1.0), 0.0, 1.0));
    }
    const double a = *static_cast<const float*>(vMin), b = *static_cast<const float*>(vMax);
    const double x = *static_cast<const float*>(v);
    if (!(b > a) || !std::isfinite(x))
        return 0.0f;
    if ((flags & ImGuiSliderFlags_Logarithmic) && a > 0.0)
        return static_cast<float>(ImClamp(std::log(ImMax(x, a) / a) / std::log(b / a), 0.0, 1.0));
    return static_cast<float>(ImClamp((x - a) / (b - a), 0.0, 1.0));
}

void EndSliderLook(const SliderLook& l, const char* label, ImGuiDataType type, const void* v, const void* vMin,
                   const void* vMax, const char* format, ImGuiSliderFlags flags)
{
    // (0.3.4 G3) A skipped window (BeginSliderLook returned before its pushes) pops nothing: one PopStyleVar there
    // took the theme's own style var off the stack.
    if (l.skip)
        return;
    ImGui::PopStyleVar();
    if (l.hid)
        ImGui::PopStyleColor(6);
    if (!l.hid)
        return;
    ImGuiContext& g = *GImGui;
    if (ImGui::TempInputIsActive(l.id))
        return; // Ctrl+Click began this frame: ImGui draws the input box from the next frame on
    if (g.LastItemData.ID != l.id || !(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible))
        return; // clipped
    const ImGuiStyle& style = g.Style;
    ImDrawList* dl = l.window->DrawList;
    const float s = MenuCommon::MockPx();
    const ImRect frame(l.pos, l.pos + ImVec2(l.width, g.FontSize + style.FramePadding.y * 2.0f));
    const bool active = g.ActiveId == l.id;
    const bool hovered = active || g.HoveredId == l.id;

    // The track: 14 mock px, centred in the 18 px frame.
    const float trackH = ImFloor(Px(14.0f));
    const float cy = ImFloor(frame.GetCenter().y);
    const ImRect track(frame.Min.x, cy - ImFloor(trackH * 0.5f), frame.Max.x, cy - ImFloor(trackH * 0.5f) + trackH);
    const float rounding = style.FrameRounding;
    dl->AddRectFilled(track.Min, track.Max,
                      ImGui::GetColorU32(active ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered
                                                                                  : ImGuiCol_FrameBg),
                      rounding);

    // The fill, from the left, in proportion to the value.
    const float t = FillRatio(type, v, vMin, vMax, flags);
    const float fx = track.Min.x + ImFloor(t * track.GetWidth() + 0.5f);
    if (fx > track.Min.x + 0.5f)
    {
        const ImDrawFlags corners =
            fx >= track.Max.x - rounding ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft;
        dl->AddRectFilled(track.Min, ImVec2(fx, track.Max.y),
                          MenuCommon::ThemeColorU32(hovered ? MenuColor::FillActive : MenuColor::Fill), rounding,
                          corners);
    }

    // The value, right-aligned inside the track, a little smaller than the labels (the mock's 11 px to its 12.5 px:
    // 12 px at the 14 px menu font: the mock's 6 px per digit).
    char buf[64];
    const char* end = buf + ImGui::DataTypeFormatString(buf, IM_ARRAYSIZE(buf), type, v,
                                                        format != nullptr ? format : ImGui::DataTypeGetInfo(type)->PrintFmt);
    const float vs = ImFloor(g.FontSize * (11.0f / 12.5f) + 0.5f);
    const ImVec2 ts = g.Font->CalcTextSizeA(vs, FLT_MAX, 0.0f, buf, end);
    const ImVec2 tp(ImMax(track.Min.x + 2.0f * s, track.Max.x - 6.0f * s - ts.x), ImFloor(cy - ts.y * 0.5f));
    const ImVec4 clip(track.Min.x, track.Min.y, track.Max.x, track.Max.y);
    dl->AddText(g.Font, vs, tp, MenuCommon::ThemeColorU32(MenuColor::Value), buf, end, 0.0f, &clip);

    // The label, where ImGui puts it.
    const char* labelEnd = ImGui::FindRenderedTextEnd(label);
    if (labelEnd != label)
        ImGui::RenderText(ImVec2(frame.Max.x + style.ItemInnerSpacing.x, frame.Min.y + style.FramePadding.y), label,
                          labelEnd, false);
}

// ---- Combo: ImGui's combo; its arrow square (Button == FrameBg in the theme) gets the mock's small "v" -----------
struct ComboLook
{
    bool draw = false;
    ImDrawList* dl = nullptr;
    ImRect bb {};
    ImGuiID id = 0;
    bool wasOpen = false;
};

ComboLook BeginComboLook(const char* label, ImGuiComboFlags flags)
{
    ComboLook c;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window == nullptr || window->SkipItems)
        return c;
    if ((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview | ImGuiComboFlags_WidthFitPreview)) != 0)
        return c;
    if (!(g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth))
        ImGui::SetNextItemWidth(ComboWidth());
    const float w = ImGui::CalcItemWidth();
    c.bb = ImRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, ImGui::GetFrameHeight()));
    c.draw = ImGui::IsRectVisible(c.bb.Min, c.bb.Max);
    c.dl = window->DrawList;
    c.id = window->GetID(label);
    c.wasOpen = ImGui::IsPopupOpen(ImHashStr("##ComboPopup", 0, c.id), ImGuiPopupFlags_None);
    return c;
}

void EndComboLook(const ComboLook& c, bool openNow)
{
    if (!c.draw)
        return;
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float arrow = ImGui::GetFrameHeight();
    if (c.bb.GetWidth() <= arrow)
        return;
    const bool hovered = g.HoveredId == c.id;
    const ImVec2 a(c.bb.Max.x - arrow, c.bb.Min.y);
    // (0.3.4 MENU match1) The square is opaque: on a greyed row (BeginDisabled, style alpha 45 %) a see-through
    // square let ImGui's triangle show behind the "v". Its colour is what ImGui's own square looks like there: the
    // button colour at the style alpha over the window background.
    ImVec4 cover = ImLerp(style.Colors[ImGuiCol_WindowBg],
                          style.Colors[(openNow || c.wasOpen || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button],
                          style.Alpha);
    cover.w = 1.0f;
    c.dl->AddRectFilled(a, c.bb.Max, ImGui::ColorConvertFloat4ToU32(cover), style.FrameRounding,
                        ImDrawFlags_RoundCornersRight);
    const ImVec2 vs = ImGui::CalcTextSize("v");
    c.dl->AddText(ImVec2(c.bb.Max.x - style.FramePadding.x - vs.x, c.bb.Min.y + style.FramePadding.y),
                  ImGui::GetColorU32(ImGuiCol_Text), "v");
}
} // namespace

// ---- T3: shared state --------------------------------------------------------------------------------------

NeuralState& Shared()
{
    // A function-local static, as the statics it replaces were: made on the first menu frame, never at DLL load.
    static NeuralState state;
    return state;
}

// ---- 0.3.4 menu look ---------------------------------------------------------------------------------------------

float Px(float mockPx) { return mockPx * MenuCommon::MockPx(); }

float ControlWidth() { return ImFloor(Px(190.0f)); }

float ComboWidth() { return ImMax(ImGui::CalcItemWidth() - ImFloor(Px(16.0f)), Px(40.0f)); }

bool FillSlider(const char* label, float* v, float vMin, float vMax, const char* format, ImGuiSliderFlags flags)
{
    const SliderLook look = BeginSliderLook(label);
    const bool changed = ImGui::SliderFloat(label, v, vMin, vMax, format, flags);
    EndSliderLook(look, label, ImGuiDataType_Float, v, &vMin, &vMax, format, flags);
    return changed;
}

bool FillSlider(const char* label, int* v, int vMin, int vMax, const char* format, ImGuiSliderFlags flags)
{
    const SliderLook look = BeginSliderLook(label);
    const bool changed = ImGui::SliderInt(label, v, vMin, vMax, format, flags);
    EndSliderLook(look, label, ImGuiDataType_S32, v, &vMin, &vMax, format, flags);
    return changed;
}

bool Checkbox(const char* label, bool* v)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window == nullptr || window->SkipItems)
        return ImGui::Checkbox(label, v);
    const ImGuiStyle& style = g.Style;
    const float frameH = ImGui::GetFrameHeight();
    const float box = ImFloor(Px(12.0f));       // 12 px square
    const float inset = (frameH - box) * 0.5f;  // centred in the 18 px row
    const ImVec2 pos = window->DC.CursorPos;
    const ImGuiID id = window->GetID(label);
    const bool on = *v;

    // ImGui's checkbox keeps the behaviour and the 24 px row; its own square and tick are hidden, and the label moves
    // in so it sits 8 px after the 12 px box (ImGui's square is a frame height wide).
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Transparent());
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Transparent());
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Transparent());
    ImGui::PushStyleColor(ImGuiCol_CheckMark, Transparent());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing,
                        ImVec2(ImMax(style.ItemInnerSpacing.x - (frameH - box), 0.0f), style.ItemInnerSpacing.y));
    const bool pressed = ImGui::Checkbox(label, v);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    if (g.LastItemData.ID == id && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible))
    {
        const bool hovered = g.HoveredId == id;
        const bool ticked = pressed ? *v : on;
        const ImVec2 bMin(pos.x, ImFloor(pos.y + inset));
        const ImVec2 bMax(bMin.x + box, bMin.y + box);
        const float r = ImMax(1.0f, Px(2.0f));
        if (ticked)
        {
            window->DrawList->AddRectFilled(bMin, bMax, MenuCommon::ThemeColorU32(MenuColor::CheckOn), r);
        }
        else
        {
            if (hovered)
                window->DrawList->AddRectFilled(bMin, bMax, MenuCommon::ThemeColorU32(MenuColor::FrameHovered), r);
            window->DrawList->AddRect(bMin, bMax,
                                      MenuCommon::ThemeColorU32(hovered ? MenuColor::Text : MenuColor::CheckOutline),
                                      r, 0, 1.0f);
        }
    }
    return pressed;
}

bool Button(const char* label, const ImVec2& size, bool selected)
{
    int colors = 1;
    ImGui::PushStyleColor(ImGuiCol_Border,
                          MenuCommon::ThemeColor(selected ? MenuColor::Selected : MenuColor::ButtonBorder));
    if (selected)
    {
        const ImVec4 red = MenuCommon::ThemeColor(MenuColor::Selected);
        ImGui::PushStyleColor(ImGuiCol_Button, red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, red);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, MenuCommon::ThemeColor(MenuColor::CheckOn));
        colors += 3;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImFloor(Px(10.0f)), ImGui::GetStyle().FramePadding.y));
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(colors);
    return pressed;
}

bool Combo(const char* label, int* current, const char* itemsSeparatedByZeros, int popupMaxHeightInItems)
{
    const ComboLook look = BeginComboLook(label, ImGuiComboFlags_None);
    const bool changed = ImGui::Combo(label, current, itemsSeparatedByZeros, popupMaxHeightInItems);
    EndComboLook(look, false);
    return changed;
}

bool Combo(const char* label, int* current, const char* const items[], int itemsCount, int popupMaxHeightInItems)
{
    const ComboLook look = BeginComboLook(label, ImGuiComboFlags_None);
    const bool changed = ImGui::Combo(label, current, items, itemsCount, popupMaxHeightInItems);
    EndComboLook(look, false);
    return changed;
}

bool BeginCombo(const char* label, const char* previewValue, ImGuiComboFlags flags)
{
    const ComboLook look = BeginComboLook(label, flags);
    const bool open = ImGui::BeginCombo(label, previewValue, flags);
    EndComboLook(look, open);
    return open;
}

bool TreeNode(const char* label, ImGuiTreeNodeFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window == nullptr || window->SkipItems)
        return ImGui::TreeNodeEx(label, flags | ImGuiTreeNodeFlags_FramePadding);
    const ImGuiStyle& style = g.Style;
    const ImVec2 pos = window->DC.CursorPos;
    const float baseOffset = window->DC.CurrLineTextBaseOffset;
    const ImVec2 pad(ImFloor(Px(1.5f) + 0.5f), style.FramePadding.y);
    const ImGuiID id = window->GetID(label);

    // ImGui's tree node (open state, nav, TreePush) with its triangle and label hidden; the mock's ">" / "v" and the
    // label are drawn after it. The hover bar stays faint.
    ImGui::PushStyleColor(ImGuiCol_Text, Transparent());
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.03f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, pad);
    const bool open = ImGui::TreeNodeEx(label, flags | ImGuiTreeNodeFlags_FramePadding);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    if (g.LastItemData.ID == id && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible))
    {
        const float textY = pos.y + ImMax(pad.y, baseOffset);
        const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
        window->DrawList->AddText(ImVec2(pos.x + pad.x, textY), col, open ? "v" : ">");
        const char* labelEnd = ImGui::FindRenderedTextEnd(label);
        if (labelEnd != label)
            window->DrawList->AddText(ImVec2(pos.x + g.FontSize + pad.x * 2.0f, textY), col, label, labelEnd);
    }
    return open;
}

void SectionHeader(const char* label)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window == nullptr || window->SkipItems)
        return;
    // (0.3.4 MENU match1) As the mock: 18 px from the previous frame's bottom to the title's cap tops (6 of them are
    // the previous row's spacing), 14 px from there to the rule, 8 px from the rule to the next frame; 40 px from
    // frame to frame in all (measured at the 14 px font, whose cap tops sit 2 px into the line).
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImFloor(Px(9.0f)));
    // ImGui's SeparatorText gives the row (an item without an ID, so the tools see the heading); its text and line
    // are hidden and the mock's are drawn: the red title, then a 1 px rule under it across the width.
    ImGui::PushStyleColor(ImGuiCol_Text, Transparent());
    ImGui::PushStyleColor(ImGuiCol_Separator, Transparent());
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 0.0f);
    ImGui::SeparatorText(label);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    const ImVec2 mn = ImGui::GetItemRectMin();
    const ImVec2 mx = ImGui::GetItemRectMax();
    const char* labelEnd = ImGui::FindRenderedTextEnd(label);
    window->DrawList->AddText(mn, MenuCommon::ThemeColorU32(MenuColor::Section), label, labelEnd);
    // AddLine adds its own half pixel: a whole y is one sharp pixel row (a second half pixel drew it 2 px and dim).
    const float y = ImFloor(mn.y + g.FontSize + Px(2.0f));
    window->DrawList->AddLine(ImVec2(mn.x, y), ImVec2(ImMax(mx.x, window->WorkRect.Max.x), y),
                              MenuCommon::ThemeColorU32(MenuColor::Rule), 1.0f);
    // The first control starts 8 px under the rule.
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImFloor(Px(4.0f)));
}

void DimTag(const char* text)
{
    if (text == nullptr || !*text)
        return;
    ImGui::SameLine(0.0f, ImFloor(Px(16.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, MenuCommon::ThemeColor(MenuColor::Tag));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

void LabelWithHelp(const char* label, const char* body, std::initializer_list<const char*> footnotes, const char* ini)
{
    ImGui::SameLine();
    ImGui::TextUnformatted(label, ImGui::FindRenderedTextEnd(label));
    HelpForLastItem(body, footnotes, ini);
}

int ToolButtons(const char* id, std::span<const char* const> labels, int active)
{
    return Segmented(id, labels, active, false);
}

bool LastItemHelpHovered() { return HelpHovered(); }

bool ResetAfterSlider(const char* label, bool show)
{
    if (!show)
        return false;
    ImGuiContext& g = *GImGui;
    const ImGuiLastItemData slider = g.LastItemData;
    ImGui::SameLine();
    const std::string resetId = std::string("Reset##") + label;
    const bool pressed = Button(resetId.c_str());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        ImGui::SetTooltip("%s", "Back to the default");
    g.LastItemData = slider; // a Help() after DeferredSlider explains the slider, not this button
    return pressed;
}

// ---- 0.3.3.2 helpers, moved from dlssnr/DlssNr_Menu.cpp (bodies restyled in 0.3.4) ------------------------

void HelpMarker(const char* tip)
{
    if (tip != nullptr && *tip && HelpHovered())
        TooltipBody(tip, {}, nullptr);
}

void NrSection(const char* label) { SectionHeader(label); }

bool MenuRuntimeIsLmxxf()
{
    return RuntimeCaps::Menu().id == AmdBridge::NeuralRuntime::Lmxxf;
}

// ---- T2 helpers ---------------------------------------------------------------------------------------------

void Help(const char* body, std::initializer_list<const char*> footnotes, const char* ini)
{
    if (HelpHovered())
        TooltipBody(body, footnotes, ini);
}

void HelpForLastItem(const char* body, std::initializer_list<const char*> footnotes, const char* ini)
{
    if (HelpHovered())
        TooltipBody(body, footnotes, ini);
}

Gate::Gate(RuntimeCaps::Cap cap, bool off, const char* offTag, bool keepClickable)
{
    const RuntimeCaps::Support s = RuntimeCaps::Get(cap);
    const bool capGreys = s != RuntimeCaps::Support::Yes;
    greyed_ = capGreys || off;
    tag_ = capGreys ? RuntimeCaps::Tag(cap) : offTag;
    disabled_ = greyed_ && !keepClickable;
    if (disabled_)
        ImGui::BeginDisabled();
}

Gate::Gate(bool off, const char* offTag, bool keepClickable)
{
    greyed_ = off;
    tag_ = offTag;
    disabled_ = greyed_ && !keepClickable;
    if (disabled_)
        ImGui::BeginDisabled();
}

Gate::~Gate()
{
    // (0.3.4) The tag is drawn inside the disabled scope, so a greyed row's tag is greyed with it (the mock's 45 %).
    if (greyed_ && tag_ != nullptr && *tag_)
        DimTag(tag_);
    if (disabled_)
        ImGui::EndDisabled();
}

int Segmented(const char* id, std::span<const char* const> labels, int active, bool fillWidth)
{
    int clicked = -1;
    const int n = static_cast<int>(labels.size());
    if (n <= 0)
        return clicked;
    ImGui::PushID(id);
    const ImGuiStyle& st = ImGui::GetStyle();
    const float w = fillWidth ? (ImGui::GetContentRegionAvail().x - st.ItemSpacing.x * float(n - 1)) / float(n) : 0.0f;
    for (int i = 0; i < n; ++i)
    {
        if (i)
            ImGui::SameLine();
        ImGui::PushID(i);
        if (Button(labels[i], ImVec2(w, 0.0f), i == active))
            clicked = i;
        ImGui::PopID();
    }
    ImGui::PopID();
    return clicked;
}

bool Hidden(RuntimeCaps::Cap cap)
{
    return RuntimeCaps::kHideUnsupported && !RuntimeCaps::NeverHidden(cap) &&
           RuntimeCaps::Get(cap) == RuntimeCaps::Support::No;
}

void HiddenCount(std::initializer_list<const char*> hiddenLabels, const char* why)
{
    int count = 0;
    for (const char* label : hiddenLabels)
        if (label != nullptr && *label)
            ++count;
    if (count == 0)
        return;
    DimLineFlow();
    ImGui::TextDisabled("%d %s-only option%s hidden", count, OtherRuntimeName(), count == 1 ? "" : "s");
    if (HelpHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
        for (const char* label : hiddenLabels)
            if (label != nullptr && *label)
                ImGui::TextUnformatted(label);
        if (why != nullptr && *why)
        {
            ImGui::Spacing();
            ImGui::TextUnformatted(why);
        }
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void StatusLine(const char* text, const char* hover)
{
    DimLineFlow();
    ImGui::TextDisabled("%s", text != nullptr ? text : "");
    if (hover != nullptr && *hover)
        Help(hover);
}

void AttentionSlot::Offer(std::string text, std::string hover, const char* button)
{
    items_.push_back(Item { std::move(text), std::move(hover), button });
}

int AttentionSlot::Draw() const
{
    if (items_.empty())
        return -1;
    int clicked = -1;
    const Item& top = items_.front();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.55f, 0.2f, 1.f));
    ImGui::TextWrapped("%s", top.text.c_str());
    ImGui::PopStyleColor();
    if (!top.hover.empty())
        Help(top.hover.c_str());
    if (top.button != nullptr)
    {
        ImGui::SameLine();
        if (Button(top.button))
            clicked = 0;
    }
    if (items_.size() > 1)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(+%d)", static_cast<int>(items_.size() - 1));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
            for (size_t i = 1; i < items_.size(); ++i)
                ImGui::TextUnformatted(items_[i].text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    return clicked;
}

// ---- The NVIDIA path's helpers, as 0.3.3.2 drew them (RenderNvidiaPath) -----------------------------------------

namespace Legacy
{
void HelpMarker(const char* tip)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
        ImGui::TextUnformatted(tip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void NrSection(const char* label)
{
    // Tinted toward whatever accent the user has configured, rather than a colour of
    // its own: the whole menu shares one theme now, and a section that invents its own
    // accent is the thing that makes a settings page look assembled from parts.
    ImGui::Spacing();
    const ImVec4 accent = ImGui::GetStyle().Colors[ImGuiCol_CheckMark];
    ImGui::PushStyleColor(ImGuiCol_Text, accent);
    ImGui::SeparatorText(label);
    ImGui::PopStyleColor();
}
} // namespace Legacy
} // namespace DlssNr::NeuralUi
