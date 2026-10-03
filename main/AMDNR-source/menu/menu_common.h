// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#pragma once

#include "SysUtils.h"
#include <Config.h>

#include <imgui/imgui.h>

// AMDNR 0.3.4 menu look (dlssnr/menu/NeuralUi.h): the mock's fold row, for ScopedCollapsingHeader below.
namespace DlssNr::NeuralUi
{
bool TreeNode(const char* label, ImGuiTreeNodeFlags flags);
}

class ScopedIndent
{
  public:
    explicit ScopedIndent(float indent = 16.0f) : m_indent(indent) { ImGui::Indent(m_indent); }

    ~ScopedIndent() { ImGui::Unindent(m_indent); }

  private:
    float m_indent;
};

class ScopedCollapsingHeader
{
  public:
    explicit ScopedCollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0)
    {
        ImGui::PushID(label);

        // (AMDNR 0.3.4, the approved mock) The child is a window of its own, which starts at ImGui's default item
        // width (65 % of the window, about 470 px): the tab's control width (190 px at Menu Scale 1.0) is carried in, so
        // the sliders and combos inside a fold keep the one column of the rest of the tab.
        const float itemWidth = ImGui::CalcItemWidth();

        // No tinted box behind a section (AMDNR 0.3.4): the child is only a layout scope, so its background is
        // transparent. Begin reads ChildBg, so the colour is popped right after it (ImGui's own FrameStyle does the
        // same); children drawn inside keep the theme's ChildBg.
        ImVec4 childBg = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
        childBg.w = 0.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
        ImGui::BeginChild("##CollapsingHeaderChild", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleColor();
        ImGui::PushItemWidth(itemWidth);

        // (AMDNR 0.3.4, the approved mock) A flat fold row (">" / "v" and the label) instead of a filled header bar;
        // the same ID and open state as the CollapsingHeader it replaces (NoTreePushOnOpen, like a collapsing header),
        // and its contents indented 22 px as the mock's children.
        _headerOpen = DlssNr::NeuralUi::TreeNode(label, flags | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (_headerOpen)
            ImGui::Indent();
        _active = true;
    }

    bool IsHeaderOpen() const { return _headerOpen; }

    ~ScopedCollapsingHeader()
    {
        if (_active)
        {
            if (_headerOpen)
                ImGui::Unindent();
            ImGui::PopItemWidth();
            ImGui::EndChild();
            ImGui::PopID();
        }
    }

  private:
    bool _active = false;
    bool _headerOpen = false;
};

// AMDNR 0.3.4 menu look: the main window's palette, taken from the owner-approved mock
// (the owner-approved menu mock, APPROVED_MOCK.png). MenuCommon::ThemeColor() hands
// them out; the helpers that draw by hand (dlssnr/menu/NeuralUi.h) use it.
enum class MenuColor : int
{
    Text,         // #e6e4df labels
    Dim,          // #8d8a84 credits, status lines, inactive tabs, footer
    Tag,          // #7d7a74 the short word after a row ("running", "1.00x cost")
    Section,      // #e05a5a section titles
    Rule,         // #3a2226 the 1 px lines (sections, tab row, footer) and the window border
    Frame,        // #26262a slider track, combo box, button
    FrameHovered, // #2e2e33
    FrameActive,  // #35353b
    Fill,         // #7a1f2b slider fill
    FillActive,   // #8f1d2c slider fill while hovered or dragged
    Value,        // #dddddd the value inside a slider
    CheckOutline, // #777777 an unticked checkbox
    CheckOn,      // #c73a45 a ticked checkbox, the active tab's underline
    ButtonBorder, // #4a2a30
    Selected,     // #8f1d2c a selected button, the title bar
    White,        // #ffffff the active tab, the title text
    Count
};

template <typename T> struct MenuOption
{
    T value;
    std::string label;
    std::string tooltip;
    bool disabled = false;
    bool hidden = false;

    MenuOption& set_disabled(bool condition, const std::string& reason = "")
    {
        if (condition)
        {
            disabled = true;
            if (!reason.empty())
                tooltip = reason;
        }
        return *this;
    }

    MenuOption& set_hidden(bool condition)
    {
        if (condition)
        {
            hidden = true;
        }
        return *this;
    }
};

class MenuCommon
{
  private:
    // internal values
    inline static HWND _handle = nullptr;
    // inline static WNDPROC _oWndProc = nullptr;
    inline static bool _isVisible = false;
    inline static bool _isInited = false;
    inline static bool _isUWP = false;

    // mipmap calculations
    inline static bool _showMipmapCalcWindow = false;
    inline static bool _showHudlessWindow = false;
    inline static float _mipBias = 0.0f;
    inline static float _mipBiasCalculated = 0.0f;
    inline static uint32_t _mipmapUpscalerQuality = 0;
    inline static float _mipmapUpscalerRatio = 0;
    inline static uint32_t _displayWidth = 0;
    inline static uint32_t _renderWidth = 0;

    inline static UINT64 _frameCount = 0;

    // reflex
    inline static float _limitFps = std::numeric_limits<float>::infinity();

    // ffx
    inline static int _ffxUpscalerIndex = -1;
    inline static int _ffxFGIndex = -1;

    // output scaling
    inline static float _ssRatio = 0.0f;
    inline static bool _ssEnabled = false;
    inline static Scaler _ssDownsampler = Scaler::FSR1;

    // ui scale
    inline static int _selectedScale = 0;

    // overlay states
    inline static bool _dx11Ready = false;
    inline static bool _dx12Ready = false;
    inline static bool _vulkanReady = false;

    inline static void ShowTooltip(const char* tip);

    inline static void ShowHelpMarker(const char* tip);
    inline static void ShowResetButton(CustomOptional<bool, NoDefault>* initFlag, std::string buttonName);
    inline static void ReInitUpscaler();

    inline static void SeparatorWithHelpMarker(const char* label, const char* tip);

    static Upscaler GetBackendCode(const API api);
    static void GetCurrentBackendInfo(const API api, Upscaler& upscaler, std::string* name);
    static void RenderUpscalerCombo(const API api, Upscaler currentUpscaler, const std::vector<Upscaler>& options);
    static void AddDx11Backends(Upscaler upscaler);
    static void AddDx12Backends(Upscaler upscaler);
    static void AddVulkanBackends(Upscaler upscaler);
    template <HasDefaultValue B> static void AddResourceBarrier(std::string name, CustomOptional<int32_t, B>* value);
    template <HasDefaultValue B> static void AddDLSSRenderPreset(std::string name, CustomOptional<uint32_t, B>* value);
    template <HasDefaultValue B> static void AddDLSSDRenderPreset(std::string name, CustomOptional<uint32_t, B>* value);
    template <typename TStorage, typename T>
    static void PopulateCombo(const std::string& name, TStorage& currentValue,
                              const std::vector<MenuOption<T>>& options);

    struct RenderMenuContext;

    // RenderMenu orchestration helpers. These keep the public RenderMenu() flow short
    // while preserving the original ImGui layout and draw order.
    static void UpdateRenderTiming(RenderMenuContext& ctx);
    static void UpdateMenuInputMode(RenderMenuContext& ctx);
    static void HandleMenuShortcuts(RenderMenuContext& ctx);
    static void UpdateVersionAndStartupNotifications(RenderMenuContext& ctx);
    static void BeginMenuFrameIfNeeded(RenderMenuContext& ctx);
    static void RenderSplashWindow(RenderMenuContext& ctx);
    static void RenderNotifications(RenderMenuContext& ctx);
    static void UpdateFrameTimeAverages(RenderMenuContext& ctx);
    static void RenderPerformanceOverlay(RenderMenuContext& ctx);

    // Labels for the Neural Rendering comparison views. Drawn every frame, into the frame plane,
    // so a screenshot keeps them and the wipe reveals and hides them like the images.
    static void RenderNrCompareTags();
    static void RenderMainMenuWindow(RenderMenuContext& ctx);

    // RenderMainMenuWindow section helpers. These keep the main window flow readable
    // without changing the existing ImGui layout, labels, or setting side effects.
    static void RenderMainMenuHeaderMessages(RenderMenuContext& ctx);
    static void RenderMainMenuTable(RenderMenuContext& ctx);
    static void RenderActiveUpscalerSettings(RenderMenuContext& ctx);
    static void RenderFrameGenerationSelection(RenderMenuContext& ctx);
    static void RenderFrameGenerationRuntimeSettings(RenderMenuContext& ctx);
    // AMDNR 0.3.4, the approved other-tabs mock: Frame Gen > Compatibility (the external FG box), Upscaling > Render
    // resolution (Upscale Ratio Override and Output Scaling, from the Image tab), Image > Init Flags (split out so
    // Textures sits between Sharpness and it).
    static void RenderFrameGenerationCompatibility(RenderMenuContext& ctx);
    static void RenderRenderResolutionSettings(RenderMenuContext& ctx);
    static void RenderInitFlagsSettings(RenderMenuContext& ctx);
    static void RenderFsrCommonSettings(RenderMenuContext& ctx);
    static void RenderFramerateSettings(RenderMenuContext& ctx);
    static void RenderFakenvapiSettings(RenderMenuContext& ctx);
    static void RenderLowLatencySettings(RenderMenuContext& ctx);
    static void RenderActiveImageSettings(RenderMenuContext& ctx);
    static void RenderMagnifierSettings(RenderMenuContext& ctx);
    static void RenderQuirksSettings(RenderMenuContext& ctx);
    static void RenderDisplaySettings(RenderMenuContext& ctx); // Advanced > Display (V-Sync), AMDNR 0.3.4
    static void RenderAdvancedSettings(RenderMenuContext& ctx);
    static void RenderLoggingSettings(RenderMenuContext& ctx);
    static void RenderFpsOverlaySettings(RenderMenuContext& ctx);
    static void RenderUpscalerInputsSettings(RenderMenuContext& ctx);
    static void RenderApiAndTextureSettings(RenderMenuContext& ctx);
    static void RenderKeybindSettings(RenderMenuContext& ctx);
    static void RenderMainMenuGraphs(RenderMenuContext& ctx);
    static void RenderMainMenuBottomBar(RenderMenuContext& ctx);
    static void RenderMipmapBiasWindow(RenderMenuContext& ctx, ImGuiWindowFlags flags);
    static void RenderHudlessResourcesWindow(RenderMenuContext& ctx, ImGuiWindowFlags flags);

    static void UpdateManualInput(HWND targetHwnd);

  public:
    // The main window's tabs, in their order (AMDNR 0.3.4, T11g).
    enum class MenuTab
    {
        Neural,
        Upscaling,
        FrameGen,
        Image,
        Interface,
        Advanced,
    };
    // Selects a tab of the main window on the next frame, e.g. an "Open Upscaling" button on the Neural tab
    // (AMDNR 0.3.4, T11g). Menu thread only.
    static void RequestTab(MenuTab tab);

    // AMDNR 0.3.4 menu look: a palette colour through the menu's HDR tone map (MenuHdrCheck tone-maps the style
    // colours; a colour drawn by hand must get the same, or it glows in an HDR game). With [Menu] LightTheme=true the
    // light style's own colours stand in (the mock has no light variant). Menu thread only.
    static ImVec4 ThemeColor(MenuColor c);
    // The same with the current style alpha applied (a greyed row), ready for an ImDrawList call.
    static ImU32 ThemeColorU32(MenuColor c);
    // AMDNR 0.3.4 menu look: one mock px at the current font size (the menu font is 14 px at Menu Scale 1.0, which
    // draws the mock's 12.5 px CSS text; a [Menu] FontSize override and the Menu Scale scale it). Every hand-drawn
    // size of the main window is in mock px times this (DlssNr::NeuralUi::Px). Menu thread only.
    static float MockPx();
    // AMDNR 0.3.4 (the approved mock, D-3): the Save report button and its result block (Saving / Report saved with
    // Open folder / Hide, or the orange failure), shared state; drawn first in Advanced > Logging and last in the
    // Neural tab's Diagnostics drawer. idSuffix keeps the two places' IDs apart. Menu thread only.
    static void RenderSaveReportRow(const char* idSuffix);

    // The Neural Rendering toggle-key binder, for the Neural tab; and a key's display name.
    static void RenderDlssNrToggleKeybind();
    static std::string KeyName(int virtualKey);
    static void Dx11Inited() { _dx11Ready = true; }
    static void Dx12Inited() { _dx12Ready = true; }
    static void VulkanInited() { _vulkanReady = true; }
    static bool IsInited() { return _isInited; }
    static bool IsVisible() { return _isVisible; }
    static HWND Handle() { return _handle; }

    static bool RenderMenu();
    static void Init(HWND InHwnd, bool isUWP);
    static void Shutdown();
    static void HideMenu();
    static void Present();
};
