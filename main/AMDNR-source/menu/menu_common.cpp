// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#include "pch.h"
#include <dlssnr/amd/AmdBridge.h>
#include <dlssnr/amd/RuntimeCaps.h> // ToggleNoticeRuntime: the Neural tab's runtime rule
#include "menu_common.h"
#include <dlssnr/DlssNr_ExposureScan.h>

#include <algorithm>
#include <cfloat>

#include <dlssnr/DlssNr.h>

#include "input/input_system.h"
#include "input/ToggleGate.h"
#include "input/KeyPressLatch.h"   // AMDNR 0.3.4.2: the menu key toggles once per physical press, on the press
#include "input/EdgeReplay.h"      // AMDNR 0.3.4.2: MenuInput::PressIsFresh (runtime chooser keys)
#include "input/InputDiagRate.h"   // AMDNR 0.3.4.2: MenuInput::LineBudget (menu key log)
#include "RuntimeChooserSession.h" // AMDNR 0.3.4.2: the first-launch runtime chooser's rules

#include "font/Hack_Compressed.h"

#include <proxies/XeSS_Proxy.h>
#include <proxies/XeFG_Proxy.h>
#include <proxies/FfxApi_Proxy.h>
#include <proxies/Streamline_Proxy.h>

#include <framegen/nvngx/Nvngx_FG.h>

#include <nvapi/fakenvapi.h>
#include <hooks/Reflex_Hooks.h>

#include <version_check.h>

#include <upscaler_time/UpscalerTime_Vk.h>

#include <imgui/imgui_internal.h>
#include <imgui/ImGuiNotify.hpp>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_uwp.h>

#include <mutex>
#include <cstdarg>

#include <array>
#include <chrono>
#include <memory>
#include <type_traits>
#include <misc/IdentifyGpu.h>
#include <misc/REFrameworkCheck.h>
#include <misc/AmdnrReport.h> // Save report (AMDNR 0.3.4, RenderSaveReportRow)
#include <dlssnr/menu/NeuralUi.h> // AMDNR 0.3.4 menu look: the mock's widgets (Button, SectionHeader, ...)
#include <hooks/Xell_Hooks.h>
#include <low_latency/input/input_common.h>

#define MARK_ALL_BACKENDS_CHANGED()                                                                                    \
    for (auto& singleChangeBackend : State::Instance().changeBackend)                                                  \
        singleChangeBackend.second = true;

static float fontSize = 14.0f; // just changing this doesn't make other elements scale ideally
static ImVec2 overlaySize(0.0f, 0.0f);
static ImVec2 overlayPosition(-1000.0f, -1000.0f);
static bool _hdrTonemapApplied = false;
static ImVec4 SdrColors[ImGuiCol_COUNT];

static bool inputMenu = false;
static bool inputFG = false;
static bool inputFps = false;
static bool inputFpsCycle = false;
// Not a key debounce: UpdateManualInput refreshes lastInputTick only while a keybind is being captured, so this
// keeps every shortcut quiet for 1 s after a capture ends (the key just bound must not fire at once). The re-toggle
// guard of the menu and NR keys is menuToggleGate / nrToggleGate in HandleMenuShortcuts (AMDNR 0.3.4).
static uint64_t lastInputTick = 0;
constexpr uint64_t debounceThreshold = 1000;
// AMDNR 0.3.4 (Assetto Corsa): one press of the menu or NR key could arrive as two release edges 16-316 ms apart
// (a late WM_KEYDOWN re-arms the key after the poll saw it up), which toggled twice. HandleMenuShortcuts drops an
// edge closer than [Hotfix] MenuToggleDebounceMs (400; 0 = 0.3.3.2) to the last accepted one of the same key.
static MenuInput::ToggleGate menuToggleGate;
static MenuInput::ToggleGate nrToggleGate;
// AMDNR 0.3.4.2 (Assetto Corsa, second report): AC hands its key messages over up to about 2 s late, so one press still
// toggled the menu twice (pairs 1.4-2.0 s apart, beyond the gate's 1000 ms cap). The menu key now toggles once per
// physical press, on the press edge (menu/input/KeyPressLatch.h, UpdateMenuKeyLatch); a release never toggles. UWP and
// [Hotfix] DiagInputHooksSkip=presslatch keep the 0.3.4.1 release edge. The gate above stays as a backstop.
static MenuInput::KeyPressLatch menuKeyLatch;
static int menuKeyLatchVk = -1;        // the key the latch follows; the latch starts over when ShortcutKey changes
static bool menuToggleByLatch = false; // this frame's menu toggle came from the latch (a close holds the key back)
static MenuInput::LineBudget menuKeyEdgeLog { 40 }; // one INFO line per menu-key edge, 40 per session
static MenuInput::LineBudget menuGateLog { 10 };
// AMDNR 0.3.4.2: the first-launch runtime chooser (menu/RuntimeChooserSession.h). RenderNeuralRuntimeChooser draws
// it, UpdateManualInput feeds it keys, and closing the menu answers it "Decide later".
static MenuUi::RuntimeChooserSession chooserSession;
// AMDNR 0.3.4.2 (H2): the chooser no longer opens the menu by itself at boot; RaiseNeuralChoiceNotice posts one
// notification per process instead.
static MenuUi::RuntimeChooserBoot chooserBoot;
static std::array<uint32_t, std::size(MenuUi::kChooserKeyVks)> chooserKeySeen {}; // press counts already judged
static constexpr const char* kChooserTitle = "Choose the neural runtime";

static bool hasGamepad = false;
static bool ffxInitTried = false;
static bool xefgInitTried = false;
static std::string windowTitle;
static std::string selectedUpscalerName = "";
static Upscaler currentBackend = Upscaler::Reset;
static std::string currentBackendName = "";
static int refreshRate = 0;
static ImVec2 lastPosition(-1000.0f, -1000.0f);

// The AMD neural runtime stopped for this session: danielblnc refused or failed, lmxxf failed to load or
// create, or poisoned by a fatal error. The backend's own flag first; the status words as a fallback
// (danielblnc's Status never blocks; lmxxf's is short). "backend stopped" covers danielblnc's
// "AMD idle: backend stopped" and lmxxf's "lmxxf backend stopped[ at submission]: ...".
static bool AmdNeuralRuntimeStopped()
{
    if (DlssNr::AmdBridge::RuntimeStopped())
        return true;
    const std::string st = DlssNr::AmdBridge::Status();
    for (const char* key : { "AMD idle: stopped", "backend stopped", "AMD pre-SR stopped", "lmxxf poisoned",
                             "session is poisoned", "lmxxf stopped" })
    {
        if (st.find(key) != std::string::npos)
            return true;
    }
    return false;
}

// The runtime the NR toggle notice names (AMDNR 0.3.4), or nullptr when there is none to name: the one this process
// built its backend for; before one is built, what the bridge will build, by the Neural tab's own rule
// (RuntimeCaps::detail::MenuIsLmxxfNow, G1: one rule for both); nothing while a choice between two installed
// runtimes is still open, when neither will run, or on a GPU that is not AMD. Only when the key is pressed: the
// file checks are not for every frame.
static const char* ToggleNoticeRuntime()
{
    using DlssNr::AmdBridge::NeuralRuntime;
    switch (DlssNr::AmdBridge::ActiveRuntime())
    {
    case NeuralRuntime::Lmxxf:
        return "lmxxf";
    case NeuralRuntime::Daniel:
        return "danielblnc";
    default:
        break;
    }
    if (!DlssNr::AmdBridge::GpuSupportInfo().amd)
        return nullptr;
    const bool daniel = DlssNr::AmdBridge::HasFiles();
    if (DlssNr::AmdBridge::ChosenRuntime() == NeuralRuntime::Unchosen && daniel && DlssNr::AmdBridge::LmxxfReady())
        return nullptr;
    if (DlssNr::RuntimeCaps::detail::MenuIsLmxxfNow())
        return "lmxxf";
    return daniel ? "danielblnc" : nullptr;
}

static ImVec2 splashPosition(-1000.0f, -1000.0f);
static ImVec2 splashSize(0.0f, 0.0f);
static double splashStart = 0.0;
static double splashLimit = 0.0;
static std::vector<std::string> splashText = { "Cope smarter, not harder",
                                               "Coping is strong with this one...",
                                               "This is where the fun begins...",
                                               "Got any more of them scalers?...",
                                               "Fake pixels and even faker frames...",
                                               "Fake frames, get your fake frames...",
                                               "I'm here to kick pixels and chew frames...",
                                               "I find your lack of supersampling disturbing...",
                                               "Frame by frame, I scale-up!",
                                               "Resistance is futile. Your pixels will be upscaled.",
                                               "I've got 99 problems, but low-res ain't one.",
                                               "It's over, DLSS, I have the higher ground!",
                                               "This isn't the resolution you're looking for",
                                               "To infinity and beyond... with ray tracing off",
                                               "I have a bad feeling about this frame pacing",
                                               "It's Dangerous to Go Alone-Take This Upscaler",
                                               "Upscaled beyond recognition.",
                                               "Trust the process. Ignore the shimmer.",
                                               "Real fake frames. Certified.",
                                               "The illusion of performance",
                                               "This upscaler belongs in a museum!",
                                               "Because native rendering is overrated.",
                                               "The more you upscaler, the more you save",
                                               "It's never too late to buy a better GPU",
                                               "We don't need real pixels where we're going",
                                               "Did you know that Intel released XeFG for everyone?",
                                               "MFG totally works with Nukem's 100%% no scam",
                                               "Some of those pixels might even be real!",
                                               "Just don't look too closely at the image",
                                               "Even supports \"software\" XeSS!",
                                               "It's too blurry to go alone, take RCAS with you",
                                               "Thanks nitec, back to you nitec",
                                               "Tested and approved by By-U",
                                               "0.8 was an inside job",
                                               "FSR4 DP4a wenETA, AMD plz",
                                               "OptiCopers, assemble!",
                                               "The Way It's Meant To Be Upscaled",
                                               "Your game may not even crash today",
                                               "Expanded and Enhanced",
                                               "It's only my 5th crash today",
                                               "Latency with FG? But I have good internet",
                                               "Console peasants can't do that",
                                               "Hope you don't have a good eyesight",
                                               "Such an aggressive upscaling? A bold move",
                                               "I almost don't feel the input lag",
                                               "And that's how you get to 60 FPS",
                                               "Together We Upscale",
                                               "For upscalers, by upscalers",
                                               "Opti Sports, it's in the sampling",
                                               "Render in your world. Upscale in ours",
                                               "All your pixels are belong to us",
                                               "Upscaling for the masses, not the classes",
                                               "Generating discord since 2023",
                                               "Enabling DLSS since 2023",
                                               "[REDACTED] never looked better",
                                               "Free and always free",
                                               "Getting unshackled from green chains in progress...",
                                               "Who's Nukem anyway?",
                                               "Compiling shaders... ETA: 05h:49m",
                                               "Did you really just pay 70 EUR for this game?!",
                                               "Guess who forgot about a nullptr check again",
                                               "AI can't outslop this",
                                               "Guess we're pre-alpha build demos now",
                                               "New app on the block - TH",
                                               "One more stutter and I might lose it",
                                               "Mostly stable, unlike the driver",
                                               "Vul... what? ~AMD",
                                               "My 8 points are floating",
                                               "No floating here - I'm strictly between -128 and 127",
                                               "Fake it til you bake it",
                                               "Worst case just turn it off and on",
                                               "*On a generative damage control mode at geometry level*",
                                               "Deep Learning Slop Sampling 5",
                                               "2D AI filters, now powered by just 2x 5090s",
                                               "Neural Slop Sampling with DLSS5",
                                               "DLSS 5 - the way it's meant to be slopped",
                                               "Just when I think I'm out, they scale me back in",
                                               "Like going in the first gear on the highway",
                                               "Nitec's Bizarre Upscaling",
                                               "\"Framegen really attracts some strange clientelle\"",
                                               "How to remove those corny messages?!",
                                               "<Your funny text goes here>" };

static std::string updateNoticeTag;
static std::string updateNoticeUrl;
// T11d (AMDNR 0.3.4, menu plan 6.4): true = the tabs scroll inside a transparent child while the header and the footer
// stay pinned; false = the whole window scrolls past its height cap (0.3.3.2's behaviour). Off until a screenshot (or
// the desk preview host) shows the auto-resizing child does not make the window's size oscillate at Menu Scale 1.0 and
// 1.3; none has been taken for this build.
static constexpr bool kPinnedBody = false;
static float lastMenuScale = 0.0f;
// Frames left in which a window the player moved is pulled back inside the screen, after a Menu
// Scale change grew it (its height settles a frame after the width) or its size changed at all.
static int menuClampFrames = 0;
// The main window's height cap of this frame (RenderMainMenuWindow): the window is placed from it, not from its
// current height, so its top edge and the tab bar stay put when a tab of another height is opened (AMDNR 0.3.4).
static float mainMenuMaxHeight = 0.0f;
static ImVec2 lastMenuWinSize {};
static CustomOptional<uint32_t> comboPreset { 0 };
static int lastKey = 0;
static bool inputDlssNr = false;
static bool inputCapture = false;
static bool capturingKey = false;

template <typename T, size_t N> struct RingBuffer
{
    std::array<T, N> data {};
    size_t head { 0 };
    size_t count { N };
    double sum { 0.0 };

    RingBuffer() { data.fill(static_cast<T>(0)); }

    void Push(T v)
    {
        if (count == N)
        {
            sum -= data[head];
        }
        else
        {
            ++count;
        }
        data[head] = v;
        sum += v;
        head = (head + 1) % N;
    }

    size_t Size() const { return N; }

    T At(size_t i) const
    {
        size_t start = head;
        return data[(start + i) % N];
    }

    float Average() const { return static_cast<float>(sum / static_cast<double>(N)); }
};

const int plotWidth = 360;
static RingBuffer<float, plotWidth> gFrameTimes;
static RingBuffer<float, plotWidth> gUpscalerTimes;

struct FsExistsCache
{
    std::wstring lastPath;
    bool cached { false };
    std::chrono::steady_clock::time_point nextRefresh { std::chrono::steady_clock::time_point::min() };
    std::chrono::milliseconds interval { 2000 };

    bool Get(const std::filesystem::path& path)
    {
        auto now = std::chrono::steady_clock::now();
        if (path != lastPath || now >= nextRefresh)
        {
            lastPath = path;
            cached = std::filesystem::exists(path);
            nextRefresh = now + interval;
        }
        return cached;
    }
};

static FsExistsCache nukemsExists;
static FsExistsCache nukemsExistsShipped; // amdnr_dlssg_fsr3.dll, the name the zip ships it under
static FsExistsCache enablerExists;

// AMDNR does not ship DLSS Enabler, so every place that needs it says where it comes from.
static constexpr const char* kEnablerUrl = "https://www.nexusmods.com/site/mods/757";
static constexpr const char* kEnablerHowTo =
    "dlss-enabler-headless.dll is not included with AMDNR. To use this option:\n"
    "1. Download DLSS Enabler 4.9.0 or newer by Artur Graniszewski:\n"
    "   https://www.nexusmods.com/site/mods/757\n"
    "2. Rename its version.dll to dlss-enabler-headless.dll\n"
    "3. Put it in the OptiScaler folder (next to OptiScaler.ini)\n"
    "The menu finds it the next time it is opened; restart the game to use it.";

// AMDNR 0.3.4 menu look: every tab's section headings are the approved mock's (dlssnr/menu/NeuralUi.h).
using DlssNr::NeuralUi::SectionHeader;

struct FlagDefinition
{
    std::string name;
    uint32_t mask;
    std::string description;
};

inline std::string StrFmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = std::vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    std::string out(len, '\0');
    va_start(args, fmt);
    std::vsnprintf(out.data(), len + 1, fmt, args);
    va_end(args);
    return out;
}

// AMDNR 0.3.4.2: the menu key goes through the press latch, except on UWP (its KeyUp callback toggles on the release),
// with [Hotfix] DiagInputHooksSkip=presslatch, and for a key OptiInput does not track with edges of its own (the
// Shift / Ctrl / Alt aggregates, mouse buttons): those keep the 0.3.4.1 release edge.
static bool MenuKeyUsesLatch(int vk, bool isUwp)
{
    if (isUwp || vk <= 0 || vk >= 256 || !OptiInput::MenuPressLatchEnabled())
        return false;

    switch (vk)
    {
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_MBUTTON:
    case VK_XBUTTON1:
    case VK_XBUTTON2:
        return false;
    default:
        return true;
    }
}

// AMDNR 0.3.4.2: the menu key's edges since the last frame, each press judged by the latch; one INFO line per edge (40
// per session: source, repeat bit, event tick, age, the poll's reading, verdict), so the next report names the late
// source. `gated`: keybind capture, or the second after it. Returns whether the latch handles the menu key.
static bool UpdateMenuKeyLatch(int vk, bool isUwp, bool gated)
{
    const bool useLatch = MenuKeyUsesLatch(vk, isUwp);

    if (vk != menuKeyLatchVk)
    {
        menuKeyLatch.Reset();
        menuKeyLatchVk = vk;
    }

    OptiInput::SetMenuKey(vk > 0 && vk < 256 ? vk : 0, useLatch);

    std::array<OptiInput::KeyEdgeRecord, OptiInput::MaxMenuKeyEdges> edges {};
    std::size_t dropped = 0;
    const std::size_t count = OptiInput::DrainMenuKeyEdges(edges.data(), edges.size(), &dropped);
    const DWORD now = GetTickCount();

    for (std::size_t i = 0; i < count; ++i)
    {
        const OptiInput::KeyEdgeRecord& edge = edges[i];
        const char* verdict = "release, ignored";

        if (edge.Down && useLatch)
        {
            // `gated`: keybind capture (or the second after it). `inputMenu`: a toggle another path already asked
            // for this frame, which then takes the press rather than the menu toggling twice -- a guard only since
            // 0.3.4.2, because the runtime chooser no longer raises the menu and nothing else sets inputMenu before
            // the latch runs for a latch key.
            const MenuInput::LatchVerdict latch = menuKeyLatch.OnPress(edge.EventMs, edge.Repeat, gated || inputMenu);
            verdict = MenuInput::LatchVerdictName(latch);

            if (latch == MenuInput::LatchVerdict::Fire)
            {
                inputMenu = true;
                menuToggleByLatch = true;
                lastKey = vk;
            }
        }
        else if (edge.Down)
        {
            verdict = "press (release-edge mode)";
        }
        else if (!useLatch)
        {
            verdict = "release (release-edge mode)";
        }

        const DWORD age = MenuInput::TickBefore(now, edge.EventMs) ? 0 : now - edge.EventMs;
        if (menuKeyEdgeLog.Take())
        {
            LOG_INFO("menu key {:#x} {} src {} repeat {} event {} age {} ms polled {} visible {} -> {}", vk,
                     edge.Down ? "down" : "up", OptiInput::InputEventSourceName(edge.Source), edge.Repeat ? 1 : 0,
                     edge.EventMs, age, edge.PolledDown ? "down" : "up", edge.MenuVisible ? 1 : 0, verdict);
        }
        else if (menuKeyEdgeLog.CapReached())
        {
            LOG_INFO("menu key: {} edges logged, no more this session", menuKeyEdgeLog.cap);
        }
    }

    if (dropped != 0)
        LOG_DEBUG("menu key: {} older edges did not fit the ring", dropped);

    if (useLatch)
    {
        // Re-arm at the first frame the key is up: physically by this frame's poll when it ran, else the level.
        const OptiInput::KeyObservation key = OptiInput::GetKeyObservation(vk);
        menuKeyLatch.OnFrame(key.PolledThisFrame ? !key.PolledDown : !key.Down, now);
    }

    return useLatch;
}

// AMDNR 0.3.4.2 (C2): the runtime chooser's keys, read from OptiInput (not ImGui), so they work at any frame rate and
// even when ImGui is not fed keys: 1 / 2 (Numpad too) pick a row, Enter uses it, Esc decides later. Each press is
// judged once; an autorepeat, a late copy of a press (EdgeReplay.h's freshness), a press from before the chooser
// appeared and the menu key itself do nothing. `menuVisible`: the menu was on screen last frame (a hidden menu draws no
// chooser, so its keys belong to the game).
static void UpdateRuntimeChooserKeys(int menuKeyVk, bool menuVisible)
{
    const DWORD now = GetTickCount();
    if (!menuVisible)
        chooserSession.OnFrame(false, now);

    const float frameMs = ImGui::GetCurrentContext() != nullptr ? ImGui::GetIO().DeltaTime * 1000.0f : 0.0f;
    const uint32_t maxAgeMs = MenuInput::ReplayMaxAgeMs(frameMs);
    const auto runtimes = DlssNr::RuntimeCaps::All();

    for (std::size_t i = 0; i < std::size(MenuUi::kChooserKeyVks); ++i)
    {
        const int vk = MenuUi::kChooserKeyVks[i];
        const OptiInput::KeyObservation key = OptiInput::GetKeyObservation(vk);

        if (key.PressCount == chooserKeySeen[i])
            continue;

        chooserKeySeen[i] = key.PressCount;

        const MenuUi::ChooserKey chooserKey = MenuUi::ChooserKeyFromVk(vk, menuKeyVk);
        if (key.PressRepeat || chooserKey == MenuUi::ChooserKey::None || !chooserSession.visibleLastFrame)
            continue;

        uint32_t age = 0;
        if (!MenuInput::PressIsFresh(key.PressTimeMs, key.PollDownBeforeValid, key.PollDownBeforeMs, now, maxAgeMs,
                                     &age))
        {
            LOG_INFO("runtime chooser: key {:#x} ignored, a late copy of an earlier press ({} ms old)", vk, age);
            continue;
        }

        switch (chooserSession.OnKey(chooserKey, key.PressTimeMs))
        {
        case MenuUi::ChooserEvent::Stale:
            LOG_INFO("runtime chooser: key {:#x} ignored, pressed before the chooser appeared", vk);
            break;
        case MenuUi::ChooserEvent::Picked:
        {
            const int row = chooserSession.pick;
            LOG_INFO("runtime chooser: row {} ({}) picked by key {:#x}", row + 1,
                     row >= 0 && row < static_cast<int>(runtimes.size()) ? runtimes[row].name : "?", vk);
            break;
        }
        case MenuUi::ChooserEvent::UseRequested:
            LOG_INFO("runtime chooser: Enter pressed, using row {}", chooserSession.EffectivePick() + 1);
            break;
        case MenuUi::ChooserEvent::Later:
            LOG_INFO("runtime chooser: decided later ({}); not shown again this session",
                     MenuUi::LaterReasonName(MenuUi::LaterReason::Escape));
            break;
        default:
            break;
        }
    }
}

// AMDNR 0.3.4.2 (C1): the player closed the menu (menu key, Close button) while the runtime chooser was open: that is
// "Decide later" for this session (before, the modal stayed in ImGui's popup stack and came back on every open).
static void ChooserMenuClosed(const char* how)
{
    if (chooserSession.OnMenuHidden(DlssNr::AmdBridge::RuntimeChoiceNeeded()))
        LOG_INFO("runtime chooser: menu closed ({}), treated as Decide later for this session", how);
}

void MenuCommon::UpdateManualInput(HWND targetHwnd)
{
    OptiInput::BeginFrame(targetHwnd);

    const auto config = Config::Instance();

    auto CheckShortcut = [&](int vk, bool& inputFlag, const char* logMessage)
    {
        if (inputFlag)
            return;

        if (vk <= 0 || vk >= 256)
            return;

        if (OptiInput::IsKeyReleased(vk))
        {
            lastKey = vk;
            // receivingWmInputs = false;
            inputFlag = true;
            LOG_DEBUG("{}", logMessage);
        }
    };

    const auto currentTick = GetTickCount64();
    const bool canAcceptInputs = lastInputTick + debounceThreshold < currentTick;

    // AMDNR 0.3.4.2: the menu key through the press latch (gated like every shortcut); else the release edge below.
    const int menuKey = config->ShortcutKey.value_or_default();
    const bool menuKeyLatched = UpdateMenuKeyLatch(menuKey, _isUWP, capturingKey || !canAcceptInputs);

    if (!capturingKey && canAcceptInputs)
    {
        if (!menuKeyLatched)
            CheckShortcut(menuKey, inputMenu, "Menu key pressed, will be switching menu");
        CheckShortcut(config->FpsShortcutKey.value_or_default(), inputFps, "Menu key pressed, will be switching FPS");
        CheckShortcut(config->FGShortcutKey.value_or_default(), inputFG, "Menu key pressed, will be switching FG mode");
        CheckShortcut(config->FpsCycleShortcutKey.value_or_default(), inputFpsCycle,
                      "Menu key pressed, will be switching FPS mode");
        CheckShortcut(config->DlssNrToggleKey.value_or_default(), inputDlssNr,
                      "Neural Rendering key pressed, will be toggling the pass");
        CheckShortcut(config->DlssNrCaptureKey.value_or_default(), inputCapture,
                      "Capture key pressed, will request 8 frames");
    }
    else if (capturingKey)
    {
        lastInputTick = currentTick;
    }

    // AMDNR 0.3.4.2 (C2): keys of the runtime chooser while it is on screen.
    UpdateRuntimeChooserKeys(menuKey, _isVisible);

    lastKey = OptiInput::GetLastPressedKey();
}

void MenuCommon::ShowTooltip(const char* tip)
{
    // (AMDNR 0.3.4, the approved mock) The same hover rule as ShowHelpMarker: on the label of a combo or slider (not
    // over its box, where a tooltip would cover what the player aims at), on keyboard/gamepad focus, also while greyed.
    if (DlssNr::NeuralUi::LastItemHelpHovered())
    {
        ImGui::BeginTooltip();
        // Unformatted (AMDNR 0.3.4): the tip is text, not a format string (a '%' in it was a risk), and it ends at
        // an ImGui "##" ID suffix like a label does (the FG rectangle help showed a literal "##2").
        ImGui::TextUnformatted(tip, ImGui::FindRenderedTextEnd(tip));
        ImGui::EndTooltip();
    }
}

// (AMDNR 0.3.4, the approved mock) No "(?)" any more: the help opens on the label of the item before it (for a slider
// or combo on its label part), after ImGui's short tooltip delay, or on keyboard/gamepad focus; also while greyed.
void MenuCommon::ShowHelpMarker(const char* tip)
{
    if (tip == nullptr || !*tip || !DlssNr::NeuralUi::LastItemHelpHovered())
        return;
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(tip, ImGui::FindRenderedTextEnd(tip));
    ImGui::EndTooltip();
}

void MenuCommon::ShowResetButton(CustomOptional<bool, NoDefault>* initFlag, std::string buttonName)
{
    ImGui::SameLine();

    ImGui::BeginDisabled(!initFlag->has_value());

    if (DlssNr::NeuralUi::Button(buttonName.c_str()))
    {
        initFlag->reset();
        ReInitUpscaler();
    }

    ImGui::EndDisabled();
}

inline void MenuCommon::ReInitUpscaler()
{
    if (!State::Instance().currentFeature)
        return;

    if (State::Instance().currentFeature->GetUpscalerType() == Upscaler::DLSSD)
        State::Instance().newBackend = Upscaler::DLSSD;
    else
        State::Instance().newBackend = currentBackend;

    MARK_ALL_BACKENDS_CHANGED();
}

// (AMDNR 0.3.4) The mock's section heading; its help opens on the heading.
void MenuCommon::SeparatorWithHelpMarker(const char* label, const char* tip)
{
    DlssNr::NeuralUi::SectionHeader(label);
    ShowHelpMarker(tip);
}

class Keybind
{
    std::string name;
    int id;
    bool waitingForKey = false;

  public:
    Keybind(std::string name, int id) : name(name), id(id) {}

    static std::string KeyNameFromVirtualKeyCode(USHORT virtualKey)
    {
        if (virtualKey == (USHORT) UnboundKey)
            return "Unbound";

        UINT scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);

        // Keys like Home would display as Num 0 without this fix
        switch (virtualKey)
        {
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_NUMLOCK:
        case VK_DIVIDE:
        case VK_RCONTROL:
        case VK_RMENU:
            scanCode |= 0xE000;
            break;
        }

        LONG lParam = (scanCode & 0xFF) << 16;
        if (scanCode & 0xE000)
            lParam |= 1 << 24;

        wchar_t buf[64] = {};
        if (GetKeyNameTextW(lParam, buf, static_cast<int>(std::size(buf))) != 0)
            return wstring_to_string(buf);

        return "Unknown";
    }

    // The capture (0.3.3.2's, moved here unchanged so both layouts share it): mouse buttons are ignored, Escape
    // cancels, Backspace unbinds, any other key is the new binding.
    void Capture(CustomOptional<int>& configKey)
    {
        if (lastKey == 0 || lastKey == VK_LBUTTON || lastKey == VK_RBUTTON || lastKey == VK_MBUTTON)
            return;

        if (lastKey == VK_ESCAPE)
        {
            waitingForKey = false;
            capturingKey = false;
            return;
        }

        if (lastKey == VK_BACK)
            lastKey = UnboundKey;

        configKey = lastKey;
        waitingForKey = false;
        capturingKey = false;
    }

    void StartCapture()
    {
        waitingForKey = true;
        capturingKey = true;
        lastKey = 0;
    }

    // The key's button (AMDNR 0.3.4, menu plan 5.2 / 5.10): the key itself is the button, one width whatever it
    // shows, so it does not jump while it waits for a key; reset appears only while the key differs from its default.
    // Returns whether that reset button is to be drawn (the caller places it).
    bool KeyButton(CustomOptional<int>& configKey, bool sharedWidth = true)
    {
        CustomOptional<int> defaults = configKey;
        defaults.reset();
        const bool changed = configKey.has_value() && configKey.value() != defaults.value_or_default();

        static const char* const kWaiting = "Press a key...";
        const std::string keyName =
            waitingForKey ? std::string(kWaiting) : KeyNameFromVirtualKeyCode(configKey.value_or_default());
        // (AMDNR 0.3.4) sharedWidth = false: the key's own width (the approved mock's [Home] beside Enable Neural
        // Rendering); the button's padding is the mock button's 10 px.
        const float pad = std::floor(10.0f * MenuCommon::MockPx()) * 2.0f;
        const float width =
            sharedWidth ? std::max(ImGui::CalcTextSize(kWaiting).x, ImGui::CalcTextSize(keyName.c_str()).x) + pad : 0.0f;
        if (DlssNr::NeuralUi::Button((keyName + "###key").c_str(), ImVec2(width, 0.0f)))
            StartCapture();
        return changed && !waitingForKey;
    }

    // The Neural tab's binder beside the Enable checkbox (menu plan 5.2): ( Home ), and R once rebound.
    void Render(CustomOptional<int>& configKey)
    {
        ImGui::PushID(id);
        if (KeyButton(configKey, false))
        {
            ImGui::SameLine();
            if (DlssNr::NeuralUi::Button("R"))
                configKey.reset();
        }
        ImGui::PopID();

        if (waitingForKey)
            Capture(configKey);
    }

    // Interface > Keybinds (AMDNR 0.3.4, X-IF; menu plan 5.10): the key button left of the action's name (the buttons
    // share the "Press a key..." width, so the names line up); the approved other-tabs mock's tag after the name, if
    // any; Reset once rebound.
    void RenderRow(CustomOptional<int>& configKey, const char* tag = nullptr)
    {
        ImGui::PushID(id);
        const bool showReset = KeyButton(configKey);
        ImGui::SameLine();
        ImGui::TextUnformatted(name.c_str());
        DlssNr::NeuralUi::DimTag(tag);
        if (showReset)
        {
            ImGui::SameLine();
            if (DlssNr::NeuralUi::Button("Reset"))
                configKey.reset();
        }
        ImGui::PopID();

        if (waitingForKey)
            Capture(configKey);
    }
};

// The Neural tab shows the toggle binder beside the Enable checkbox (the Keybinds panel under
// Interface keeps its own copy of the same binding).
void MenuCommon::RenderDlssNrToggleKeybind()
{
    static auto nrToggle = Keybind("Toggle hotkey", 15);
    nrToggle.Render(Config::Instance()->DlssNrToggleKey);
}
std::string MenuCommon::KeyName(int virtualKey)
{
    return Keybind::KeyNameFromVirtualKeyCode(static_cast<USHORT>(virtualKey));
}

Upscaler MenuCommon::GetBackendCode(const API api)
{
    if (auto feature = State::Instance().currentFeature)
        return feature->GetUpscalerType();

    Upscaler upscaler;

    if (api == DX11)
        upscaler = Config::Instance()->Dx11Upscaler.value_or_default();
    else if (api == DX12)
        upscaler = Config::Instance()->Dx12Upscaler.value_or_default();
    else
        upscaler = Config::Instance()->VulkanUpscaler.value_or_default();

    return upscaler;
}

void MenuCommon::GetCurrentBackendInfo(const API api, Upscaler& upscaler, std::string* name)
{
    upscaler = GetBackendCode(api);
    *name = UpscalerDisplayName(upscaler, api);
}

// The backends a D3D11 or Vulkan game reaches the neural pass through: OptiScaler's D3D12 bridge, the "w/Dx12" ones.
// The native D3D11 and Vulkan backends run on the game's own device and never reach it (the Neural tab's hint).
static bool UpscalerReachesNeuralPass(Upscaler upscaler)
{
    switch (upscaler)
    {
    case Upscaler::XeSS_on12:
    case Upscaler::FSR21_on12:
    case Upscaler::FSR22_on12:
    case Upscaler::FFX_on12:
    case Upscaler::DLSS_on12:
        return true;
    default:
        return false;
    }
}

void MenuCommon::RenderUpscalerCombo(const API api, Upscaler currentUpscaler, const std::vector<Upscaler>& options)
{
    auto primaryGpu = IdentifyGpu::getPrimaryGpu();

    // AMDNR 0.3.4 (menu plan 5.9): in a D3D11 or Vulkan game with Neural Rendering on, the items that carry the neural
    // pass end in " - Neural", so the working pick is visible in the list itself. AMD cards only (and any non-NVIDIA
    // card); the NVIDIA DLSS-NR path is not touched.
    const bool tagNeural = (api == API::DX11 || api == API::Vulkan) && primaryGpu.vendorId != VendorId::Nvidia &&
                           Config::Instance()->DlssNrEnabled.value_or_default();
    auto displayName = [&](Upscaler upscaler)
    {
        std::string name = UpscalerDisplayName(upscaler, api);
        if (tagNeural && UpscalerReachesNeuralPass(upscaler))
            name += " - Neural";
        return name;
    };

    // Determine display name
    Upscaler targetBackend = State::Instance().newBackend;
    if (targetBackend == Upscaler::Reset)
        targetBackend = currentUpscaler;

    std::string selectedName = displayName(targetBackend);

    if (DlssNr::NeuralUi::BeginCombo("##UpscalerCombo", selectedName.c_str()))
    {
        for (auto opt : options)
        {
            // Check if GPU is capable of a given backend
            if (opt == Upscaler::DLSS && !primaryGpu.dlssCapable)
                continue;

            // DLSS over the D3D12 bridge needs NVIDIA's DLSS as well: not listed on other cards (AMDNR 0.3.4; the
            // check above covered plain DLSS only).
            if (opt == Upscaler::DLSS_on12 && primaryGpu.vendorId != VendorId::Nvidia)
                continue;

            // Not all Intel GPUs support native DX11 XeSS but don't think we have a good way to check exactly
            if (opt == Upscaler::XeSS && api == API::DX11 && primaryGpu.vendorId != VendorId::Intel)
                continue;

            bool isSelected = (currentUpscaler == opt);
            if (ImGui::Selectable(displayName(opt).c_str(), isSelected))
            {
                State::Instance().newBackend = opt;
            }
        }
        ImGui::EndCombo();
    }
    // (AMDNR 0.3.4, the approved other-tabs mock) The row's label, where ImGui puts a combo's label; drawn as text so
    // the combo keeps its "##UpscalerCombo" ID.
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::TextUnformatted("Upscaler");
}

void MenuCommon::AddDx11Backends(Upscaler upscaler)
{
    RenderUpscalerCombo(API::DX11, upscaler,
                        { Upscaler::XeSS, Upscaler::FSR22, Upscaler::FSR31, Upscaler::XeSS_on12, Upscaler::FSR21_on12,
                          Upscaler::FSR22_on12, Upscaler::FFX_on12, Upscaler::DLSS, Upscaler::DLSS_on12 });
}

void MenuCommon::AddDx12Backends(Upscaler upscaler)
{
    std::vector<Upscaler> backends { Upscaler::XeSS, Upscaler::FSR21, Upscaler::FSR22, Upscaler::FFX,
                                     Upscaler::DLSS };

    // FSR Ray Regeneration is offered only where it can actually run, rather than listed
    // and then refused. Two of its three gates are knowable here - the denoiser provider
    // has to be the 1.2 ABI this build implements, and the card has to be one Ray Regeneration
    // is offered on (RDNA 4; RDNA 3 / 3.5 by default since the 0.3.4 RR-7000 decision; other
    // cards with the opt-in: hooks/RrHardwareGate.h) - so a machine that fails either never
    // sees an entry it would only be disappointed by. The third gate is whether the title publishes Ray
    // Reconstruction inputs at all, and only the feature can answer that; picking it in a
    // game that does not falls back to FSR with the reason named in the log.
    const bool rrProvider = FfxApiProxy::IsDenoiserApiImplementedDx12();
    // RDNA 4, RDNA 3 / 3.5 while [FSR-RR] FfxDenoiserAllowPreRdna4 is unset, other cards with it true: the gate
    // every RR answer uses (RR-7000).
    const bool rrHardware = StreamlineHooks::rrHardwareAllowed();

    if (rrProvider && rrHardware)
        backends.push_back(Upscaler::FSR_RR);

    // SAY WHY IT IS NOT THERE, once.
    //
    // The first build that offered FSR-RR did not appear in three different games and
    // there was no way to tell which gate had refused, or whether the build even had the
    // feature - the entry was simply absent, which looks identical to a menu that was
    // never changed. A gate that hides an option has to be able to account for itself.
    //
    // The usual cause is the most boring one: OptiScaler loads its proxy DLLs from the
    // `OptiScaler` folder beside the game executable, so the denoiser has to be in EACH
    // game's folder, not just the one it was first tested in.
    static bool loggedRrGate = false;
    if (!loggedRrGate)
    {
        loggedRrGate = true;
        // Name RDNA 4 only for a gfx12 card (0.3.3.2); the other reasons from the 0.3.4 Ray Regeneration rework's decision (RR-7000:
        // RDNA 3 by default, the ini's true / false).
        const auto& rrGpu = IdentifyGpu::getPrimaryGpu();
        const RrGate::Why rrWhy = StreamlineHooks::rrHardwareDecision().why;
        if (rrProvider && rrHardware)
        {
            if (IdentifyGpu::isRdna4(rrGpu))
                LOG_INFO("FSR-RR offered: denoiser provider 1.2 loaded and GPU {} is RDNA 4.", rrGpu.name);
            else if (rrWhy == RrGate::Why::Rdna3Default)
                LOG_INFO("FSR-RR offered: denoiser provider 1.2 loaded; GPU {} is RDNA 3 (offered by default; "
                         "[FSR-RR] FfxDenoiserAllowPreRdna4=false turns it off); the driver decides at create.",
                         rrGpu.name);
            else
                LOG_INFO("FSR-RR offered: denoiser provider 1.2 loaded; GPU {} is not RDNA 4 - offered because "
                         "[FSR-RR] FfxDenoiserAllowPreRdna4=true; the driver decides at create.",
                         rrGpu.name);
        }
        else
        {
            const auto gen = FfxApiProxy::DenoiserApiGenerationDx12();
            const char* why = gen == FfxDenoiserApiGeneration::NotLoaded
                                  ? "amd_fidelityfx_denoiser_dx12.dll is not loaded - it must be in this "
                                    "game's OptiScaler folder, beside the other amd_fidelityfx_*.dll files"
                              : gen == FfxDenoiserApiGeneration::Unknown
                                  ? "the denoiser loaded but reported no parseable version"
                              : gen == FfxDenoiserApiGeneration::V1_1
                                  ? "the denoiser is Ray Regeneration 1.1; this build implements 1.2, which "
                                    "is a different ABI"
                              : gen == FfxDenoiserApiGeneration::Unsupported
                                  ? "the denoiser is a version this build does not implement"
                                                       : "provider ok";
            const char* hwWhy = rrGpu.vendorId == VendorId::Nvidia
                                    ? "the GPU is NVIDIA (FSR-RR is offered on non-NVIDIA cards only)"
                                : rrWhy == RrGate::Why::IniFalse
                                    ? "[FSR-RR] FfxDenoiserAllowPreRdna4=false (RDNA 4 only)"
                                : rrWhy == RrGate::Why::GpuUnknown
                                    ? "the GPU is not known yet"
                                    : "not RDNA 3 or 4; [FSR-RR] FfxDenoiserAllowPreRdna4=true offers it anyway";
            LOG_WARN("FSR-RR not offered. Provider: {}. GPU {}: {}. Reason: {}", rrProvider ? "ok" : "no",
                     rrGpu.name,
                     !rrHardware && rrGpu.vendorId == VendorId::Nvidia ? "NVIDIA"
                     : IdentifyGpu::isRdna4(rrGpu)                     ? "RDNA 4"
                     : IdentifyGpu::isRdna3(rrGpu)                     ? "RDNA 3"
                                                                       : "not RDNA 3 or 4",
                     rrProvider ? hwWhy : why);
        }
    }

    RenderUpscalerCombo(API::DX12, upscaler, backends);

    // SAY IT ON SCREEN, not only in the log.
    //
    // Hiding the entry was meant to spare people an option they would only be refused.
    // What it actually did was make the refusal undiagnosable: three testers in three games
    // reported "FSR-RR does not appear" and none of them could tell whether the build had
    // the feature, the DLL was missing, or their card was wrong - an absent menu entry looks
    // exactly like a menu that was never changed. The log line added for it only helps
    // someone who knows to go and read a log.
    //
    // Only shown where the card is offered Ray Regeneration (RDNA 4; RX 7000 / RDNA 3 by default since RR-7000;
    // other cards with the opt-in), because elsewhere it is not a missing file, it is a missing capability, and
    // there is nothing the reader could do about it.
    if (!rrProvider && rrHardware)
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "%s",
                           "FSR Ray Regeneration: needs amd_fidelityfx_denoiser_dx12.dll");
        ShowHelpMarker("AMD's Ray Regeneration denoiser is not loaded, so the backend is not offered.\n"
                   "\nPut amd_fidelityfx_denoiser_dx12.dll (FidelityFX SDK 2.3, Ray Regeneration"
                   "\n1.2) in this game's OptiScaler folder, beside the other amd_fidelityfx_*.dll"
                   "\nfiles, and restart the game.\n"
                   "\nIt only does anything in games that use DLSS Ray Reconstruction. Elsewhere"
                   "\nit falls back to FSR and says so in the log.");
    }
}

void MenuCommon::AddVulkanBackends(Upscaler upscaler)
{
    RenderUpscalerCombo(API::Vulkan, upscaler,
                        { Upscaler::XeSS, Upscaler::FSR21, Upscaler::FSR22, Upscaler::FFX, Upscaler::FSR21_on12,
                          Upscaler::FFX_on12, Upscaler::DLSS });
}

template <HasDefaultValue B> void MenuCommon::AddResourceBarrier(std::string name, CustomOptional<int32_t, B>* value)
{
    const char* states[] = { "AUTO",
                             "COMMON",
                             "VERTEX_AND_CONSTANT_BUFFER",
                             "INDEX_BUFFER",
                             "RENDER_TARGET",
                             "UNORDERED_ACCESS",
                             "DEPTH_WRITE",
                             "DEPTH_READ",
                             "NON_PIXEL_SHADER_RESOURCE",
                             "PIXEL_SHADER_RESOURCE",
                             "STREAM_OUT",
                             "INDIRECT_ARGUMENT",
                             "COPY_DEST",
                             "COPY_SOURCE",
                             "RESOLVE_DEST",
                             "RESOLVE_SOURCE",
                             "RAYTRACING_ACCELERATION_STRUCTURE",
                             "SHADING_RATE_SOURCE",
                             "GENERIC_READ",
                             "ALL_SHADER_RESOURCE",
                             "PRESENT",
                             "PREDICATION",
                             "VIDEO_DECODE_READ",
                             "VIDEO_DECODE_WRITE",
                             "VIDEO_PROCESS_READ",
                             "VIDEO_PROCESS_WRITE",
                             "VIDEO_ENCODE_READ",
                             "VIDEO_ENCODE_WRITE" };
    const int values[] = { -1,  0,   1,     2,      4,      8,      16,      32,       64,   128,
                           256, 512, 1024,  2048,   4096,   8192,   4194304, 16777216, 2755, 192,
                           0,   310, 65536, 131072, 262144, 524288, 2097152, 8388608 };

    int selected = value->value_or(-1);

    const char* selectedName = "";

    for (int n = 0; n < 28; n++)
    {
        if (values[n] == selected)
        {
            selectedName = states[n];
            break;
        }
    }

    if (DlssNr::NeuralUi::BeginCombo(name.c_str(), selectedName))
    {
        if (ImGui::Selectable(states[0], !value->has_value()))
            value->reset();

        for (int n = 1; n < 28; n++)
        {
            if (ImGui::Selectable(states[n], selected == values[n]))
                *value = values[n];
        }

        ImGui::EndCombo();
    }
}

static uint32_t GetPresetIndex(IFeature* feature, bool dlssd = false)
{
    auto ratio = (float) feature->TargetWidth() / (float) feature->RenderWidth();

    if (!dlssd)
    {
        if (State::Instance().dlssPresetsOverridenByOpti)
        {
            LOG_DEBUG("DLSS Presets overridden by Opti, using Opti preset indices with ratio: {}", ratio);

            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetUltraPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetBalanced.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetQuality.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetUltraQuality.value_or_default());
            }
            else
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetDLAA.value_or_default());
            }
        }
        else if (State::Instance().dlssPresetsOverriddenExternally)
        {
            LOG_DEBUG("DLSS Presets overridden externally, using external preset index: {}",
                      State::Instance().dlssRenderPresetExternal);

            return State::Instance().dlssRenderPresetExternal;
        }
        else
        {
            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetUltraPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetBalanced;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetQuality;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetUltraQuality;
            }
            else
            {
                return State::Instance().dlssRenderPresetDLAA;
            }
        }
    }
    else
    {
        if (State::Instance().dlssdPresetsOverridenByOpti)
        {
            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetUltraPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetBalanced.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetQuality.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetUltraQuality.value_or_default());
            }
            else
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetDLAA.value_or_default());
            }
        }
        else if (State::Instance().dlssdPresetsOverriddenExternally)
        {
            return State::Instance().dlssdRenderPresetExternal;
        }
        else
        {
            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetUltraPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetBalanced;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetQuality;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetUltraQuality;
            }
            else
            {
                return State::Instance().dlssdRenderPresetDLAA;
            }
        }
    }

    return 0;
}

// TODO: disable presets based on the detected DLSS version
template <HasDefaultValue B> void MenuCommon::AddDLSSRenderPreset(std::string name, CustomOptional<uint32_t, B>* value)
{
    // clang-format off
    static const std::vector<MenuOption<uint32_t>> presets = {
        { NVSDK_NGX_DLSS_Hint_Render_Preset_Default, "DEFAULT", 
            "Whatever the game uses" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_A, "PRESET A",
            "Intended for Performance/Balanced/Quality modes.\nAn older variant best suited to combat ghosting...\nRemoved on recent versions!" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_B, "PRESET B",
            "Intended for Ultra Performance mode.\nSimilar to Preset A...\nRemoved on recent versions!" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_C, "PRESET C",
            "Intended for Performance/Balanced/Quality modes.\nGenerally favors current frame information...\nRemoved on recent versions!" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_D, "PRESET D",
            "Default preset for Performance/Balanced/Quality modes;\ngenerally favors image stability.\nRemoved on recent versions!" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_E, "PRESET E",
            "DLSS 3.7+, a better D preset\nRemoved on recent versions!" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_F, "PRESET F",
            "Default preset for Ultra Performance and DLAA modes\nRemoved on recent versions!" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_G, "PRESET G",
            "Unused" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_H_Reserved, "PRESET H",
            "Unused" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_I_Reserved, "PRESET I",
            "Unused" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_J, "PRESET J",
            "Similar to preset K. Preset J might exhibit slightly\nless ghosting...\n1st Gen Transformer" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_K, "PRESET K",
            "Default preset for DLAA/Balanced/Quality modes...\n1st Gen Transformer" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_L, "PRESET L",
            "Default for Ultra Perf mode\n2nd Gen Transformers" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_M, "PRESET M",
            "Default for Perf mode\n2nd Gen Transformer" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_N, "PRESET N",
            "Unused" },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_O, "PRESET O",
            "Unused" },
        { NV_PRESET_LATEST, "Latest",
            "Latest supported by the dll" }
    };
    // clang-format on

    PopulateCombo(name, *value, presets);
}

template <HasDefaultValue B> void MenuCommon::AddDLSSDRenderPreset(std::string name, CustomOptional<uint32_t, B>* value)
{
    // We don't have DLSSD definitions so using raw values
    static const std::vector<MenuOption<uint32_t>> presets = {
        { 0, "DEFAULT", "Whatever the game uses" },
        { 1, "PRESET A", "Preset A\nRemoved on recent versions!" },
        { 2, "PRESET B", "Preset B\nRemoved on recent versions!" },
        { 3, "PRESET C", "Preset C\nRemoved on recent versions!" },
        { 4, "PRESET D", "Default model, Transformer" },
        { 5, "PRESET E", "Latest Transformer model\nMust use if DoF guide is needed" },
        { 6, "PRESET F", "Latest Transformer model\nMust use if DoF guide is needed" },
        { NV_PRESET_LATEST, "Latest", "Latest supported by the dll" }
    };

    PopulateCombo(name, *value, presets);
}

template <typename TStorage, typename T>
void MenuCommon::PopulateCombo(const std::string& name, TStorage& currentValue,
                               const std::vector<MenuOption<T>>& options)
{
    if (options.empty())
        return;

    // Assumes that different types mean that TStorage is std::optional
    T currentVal;
    if constexpr (std::is_same_v<TStorage, T>)
        currentVal = currentValue;
    else
        currentVal = currentValue.value_or(options[0].value);

    // Find the label for the currently selected item
    std::string preview = "Unknown";
    for (const auto& opt : options)
    {
        if (opt.value == currentVal)
        {
            preview = opt.label;
            break;
        }
    }

    if (DlssNr::NeuralUi::BeginCombo(name.c_str(), preview.c_str()))
    {
        for (const auto& opt : options)
        {
            if (opt.hidden)
                continue;

            if (opt.disabled)
                ImGui::BeginDisabled();

            bool isSelected = (currentVal == opt.value);
            if (ImGui::Selectable(opt.label.c_str(), isSelected))
                currentValue = opt.value;

            // Show tooltip for the individual item if it exists
            if (!opt.tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", opt.tooltip.c_str());

            if (opt.disabled)
                ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
}

static ImVec4 toneMapColor(const ImVec4& color)
{
    if (State::Instance().isHdrActive ||
        (!Config::Instance()->OverlayMenu.value_or_default() && State::Instance().currentFeature != nullptr &&
         State::Instance().currentFeature->IsHdr()))
    {
        // Controls how strongly HDR/UI colors are pushed into the tone mapper before compression.
        // Higher values make colors brighter before mapping; lower values make the result dimmer.
        constexpr float exposure = 1.0f;

        // Blends between original color and fully tone-mapped color.
        // 0.0 = no tone mapping, 1.0 = full Reinhard compression.
        constexpr float strength = 1.0f;

        float peak = std::max(color.x, std::max(color.y, color.z));

        if (peak <= 0.0f)
            return color;

        float exposedPeak = peak * exposure;
        float mappedPeak = exposedPeak / (1.0f + exposedPeak);

        float reinhardScale = mappedPeak / peak;
        float scale = 1.0f + (reinhardScale - 1.0f) * strength;

        return ImVec4(color.x * scale, color.y * scale, color.z * scale, color.w);
    }

    return color;
}

static void MenuHdrCheck(ImGuiIO io)
{
    // If game is using HDR, apply tone mapping to the ImGui style
    if (State::Instance().isHdrActive ||
        (!Config::Instance()->OverlayMenu.value_or_default() && State::Instance().currentFeature != nullptr &&
         State::Instance().currentFeature->IsHdr()))
    {
        if (!_hdrTonemapApplied)
        {
            ImGuiStyle& style = ImGui::GetStyle();

            CopyMemory(SdrColors, style.Colors, sizeof(style.Colors));

            // Apply tone mapping to the ImGui style
            for (int i = 0; i < ImGuiCol_COUNT; ++i)
            {
                ImVec4 color = style.Colors[i];
                style.Colors[i] = toneMapColor(color);
            }

            _hdrTonemapApplied = true;
        }
    }
    else
    {
        if (_hdrTonemapApplied)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            CopyMemory(style.Colors, SdrColors, sizeof(style.Colors));
            _hdrTonemapApplied = false;
        }
    }
}

static float MenuResolutionScale(ImGuiIO io)
{
    if (Config::Instance()->MenuScale.has_value())
        return Config::Instance()->MenuScale.value();

    // Calculate menu scale according to display resolution
    float y = State::Instance().screenHeight;

    if (io.DisplaySize.y != 0)
        y = (float) io.DisplaySize.y;

    // 1000p is minimum for 1.0 menu ratio
    float result = (float) ((int) (y / 108.0f)) / 10.0f;

    result = std::round(result * 10.0f) / 10.0f;

    if (result < 0.5f)
        result = 0.5f;

    if (result > 2.0f)
        result = 2.0f;

    return result;
}

// ---- AMDNR 0.3.4 menu look (the owner-approved mock) ----------------------------------------------------------
// The mock (the owner-approved menu mock, rendered 1:1 in APPROVED_MOCK.png) is drawn at
// a 12.5 px monospace font (14 px here, kMenuFontPx); every size below is in "mock px" and is multiplied by the main
// window's scale (Menu Scale, and a [Menu] FontSize override), so the window keeps its proportions at every scale.
// Only the main window (and the windows it owns) takes this look: the FPS overlay, the splash and the toasts keep
// ApplyThemeStyle's.
namespace
{
constexpr ImVec4 MockHex(unsigned rgb, float a = 1.0f)
{
    return ImVec4(float((rgb >> 16) & 0xFF) / 255.0f, float((rgb >> 8) & 0xFF) / 255.0f, float(rgb & 0xFF) / 255.0f, a);
}

// In MenuColor order (menu_common.h).
constexpr ImVec4 kMockPalette[] = {
    MockHex(0xe6e4df), // Text
    MockHex(0x8d8a84), // Dim
    MockHex(0x7d7a74), // Tag
    MockHex(0xe05a5a), // Section
    MockHex(0x3a2226), // Rule
    MockHex(0x26262a), // Frame
    MockHex(0x2e2e33), // FrameHovered
    MockHex(0x35353b), // FrameActive
    MockHex(0x7a1f2b), // Fill
    MockHex(0x8f1d2c), // FillActive
    MockHex(0xdddddd), // Value
    MockHex(0x777777), // CheckOutline
    MockHex(0xc73a45), // CheckOn
    MockHex(0x4a2a30), // ButtonBorder
    MockHex(0x8f1d2c), // Selected
    MockHex(0xffffff), // White
};
static_assert(std::size(kMockPalette) == static_cast<size_t>(MenuColor::Count), "one palette entry per MenuColor");

// (AMDNR 0.3.4 MENU match1) The menu font: 14 px at Menu Scale 1.0, the atlas font's own size. ImGui's font size is
// the font's ascent-descent height, not the CSS em: Hack at 12 drew 6 px per character, about 15 % smaller than the
// mock's 6.9 px (12.5 px CSS mono). FreeType hints Hack at 13 and at 14 to a 7 px advance (the mock's pitch); 14 also
// gives the mock's glyph height (12 px from cap top to descender, 10 at 13) and its ink (about 95 % of the mock's
// stroke weight on the header rows, 78 % at 13), and puts the text 4 px into an 18 px frame as the mock does.
// Every hand-drawn size is in mock px (MenuCommon::MockPx: the font size / kMenuFontPx).
constexpr float kMenuFontPx = 14.0f;
constexpr float kBitmapFontPx = 13.0f; // ImGui's built-in font ([Menu] UseHQFont=false), scaled by the Menu Scale
constexpr float kMenuWidthPx = 750.0f;     // the mock's window width, mock px
constexpr float kMenuMaxHeightPx = 960.0f; // the height cap: the mock with Ray Regeneration and a tools drawer open

// A theme size in whole pixels (AMDNR 0.3.4 MENU match1): a fractional WindowPadding (10 x 1.333 at Menu Scale 1.3)
// made the auto-fit height, which ImGui truncates, fall short of the content by less than a pixel, and ImGui then
// drew a full-height scrollbar. Rounded to the nearest pixel.
float WholePx(float v) { return ImFloor(v + 0.5f); }

// The main window's look, pushed around its Begin/End (and the utility windows it opens), popped after them. `s` is
// the size of one mock px. Colours only on the dark theme; the light theme keeps its colours and gets the geometry.
struct MainMenuTheme
{
    int colors = 0;
    int vars = 0;

    void Color(ImGuiCol idx, const ImVec4& c)
    {
        ImGui::PushStyleColor(idx, toneMapColor(c));
        ++colors;
    }
    void Var(ImGuiStyleVar idx, float v)
    {
        ImGui::PushStyleVar(idx, v);
        ++vars;
    }
    void Var(ImGuiStyleVar idx, const ImVec2& v)
    {
        ImGui::PushStyleVar(idx, v);
        ++vars;
    }

    // (AMDNR 0.3.4 MENU match1) Every size in whole pixels (WholePx), and the frames 18 mock px high whatever the
    // font: FramePadding.y = (18 px - the font size) / 2, 2 at Menu Scale 1.0 (14 px font), so a row stays 24 px.
    // fontPx is the text height inside the window (the bitmap font gets the Menu Scale only after Begin).
    void Push(float s, float fontPx)
    {
        const float frameH = WholePx(18.0f * s);
        // The mock's padding is 14 x 10 px inside its 1 px border; ImGui's WindowPadding counts from the window's
        // outer edge, so 15 x 11 (the left column and the footer's bottom margin at the mock's; measured, 0.3.4 MENU
        // match1).
        Var(ImGuiStyleVar_WindowPadding, ImVec2(WholePx(15.0f * s), WholePx(11.0f * s)));
        Var(ImGuiStyleVar_WindowRounding, WholePx(10.0f * s));
        Var(ImGuiStyleVar_WindowBorderSize, 1.0f);
        Var(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.0f, 0.5f));
        Var(ImGuiStyleVar_ChildRounding, 0.0f);
        Var(ImGuiStyleVar_ChildBorderSize, 0.0f);
        Var(ImGuiStyleVar_PopupRounding, WholePx(4.0f * s));
        Var(ImGuiStyleVar_PopupBorderSize, 1.0f);
        Var(ImGuiStyleVar_FramePadding, ImVec2(WholePx(8.0f * s), ImMax((frameH - fontPx) * 0.5f, 0.0f)));
        Var(ImGuiStyleVar_FrameRounding, WholePx(3.0f * s));
        Var(ImGuiStyleVar_FrameBorderSize, 0.0f);
        Var(ImGuiStyleVar_ItemSpacing, ImVec2(WholePx(8.0f * s), WholePx(24.0f * s) - frameH)); // 18 + 6 = a 24 px row
        Var(ImGuiStyleVar_ItemInnerSpacing, ImVec2(WholePx(8.0f * s), WholePx(4.0f * s)));
        Var(ImGuiStyleVar_IndentSpacing, WholePx(22.0f * s));
        Var(ImGuiStyleVar_CellPadding, ImVec2(WholePx(6.0f * s), WholePx(3.0f * s)));
        Var(ImGuiStyleVar_ScrollbarSize, WholePx(10.0f * s));
        Var(ImGuiStyleVar_ScrollbarRounding, WholePx(3.0f * s));
        Var(ImGuiStyleVar_GrabMinSize, WholePx(10.0f * s));
        Var(ImGuiStyleVar_GrabRounding, WholePx(3.0f * s));
        Var(ImGuiStyleVar_TabRounding, 0.0f);
        Var(ImGuiStyleVar_TabBorderSize, 0.0f);
        Var(ImGuiStyleVar_TabBarBorderSize, 0.0f); // the tab row's rule is drawn by hand (RenderMainMenuTable)
        Var(ImGuiStyleVar_TabBarOverlineSize, 0.0f);
        Var(ImGuiStyleVar_DisabledAlpha, 0.45f); // a greyed row in the mock is at 45 %
        Var(ImGuiStyleVar_SeparatorTextBorderSize, 1.0f);

        if (Config::Instance()->LightTheme.value_or_default())
            return;

        const auto P = [](MenuColor c) { return kMockPalette[static_cast<int>(c)]; };
        const float bgAlpha = Config::Instance()->MenuBGColorA.value_or_default();
        const ImVec4 none(0.0f, 0.0f, 0.0f, 0.0f);
        Color(ImGuiCol_Text, P(MenuColor::Text));
        Color(ImGuiCol_TextDisabled, P(MenuColor::Dim));
        Color(ImGuiCol_WindowBg, MockHex(0x131315, bgAlpha));
        Color(ImGuiCol_ChildBg, none);
        Color(ImGuiCol_PopupBg, MockHex(0x1b1b1f, 0.98f));
        Color(ImGuiCol_Border, P(MenuColor::Rule));
        Color(ImGuiCol_BorderShadow, none);
        Color(ImGuiCol_FrameBg, P(MenuColor::Frame));
        Color(ImGuiCol_FrameBgHovered, P(MenuColor::FrameHovered));
        Color(ImGuiCol_FrameBgActive, P(MenuColor::FrameActive));
        Color(ImGuiCol_TitleBg, P(MenuColor::Selected));
        Color(ImGuiCol_TitleBgActive, P(MenuColor::Selected));
        Color(ImGuiCol_TitleBgCollapsed, P(MenuColor::Selected));
        Color(ImGuiCol_ScrollbarBg, none);
        Color(ImGuiCol_ScrollbarGrab, P(MenuColor::FrameHovered));
        Color(ImGuiCol_ScrollbarGrabHovered, MockHex(0x3a3a40));
        Color(ImGuiCol_ScrollbarGrabActive, P(MenuColor::Selected));
        Color(ImGuiCol_CheckMark, P(MenuColor::CheckOn));
        Color(ImGuiCol_SliderGrab, P(MenuColor::Fill));
        Color(ImGuiCol_SliderGrabActive, P(MenuColor::FillActive));
        // Button == FrameBg, so a combo's arrow square is part of its box, as in the mock.
        Color(ImGuiCol_Button, P(MenuColor::Frame));
        Color(ImGuiCol_ButtonHovered, P(MenuColor::FrameHovered));
        Color(ImGuiCol_ButtonActive, P(MenuColor::Selected));
        Color(ImGuiCol_Header, P(MenuColor::Rule));
        Color(ImGuiCol_HeaderHovered, MockHex(0x2a2a2f));
        Color(ImGuiCol_HeaderActive, P(MenuColor::Rule));
        Color(ImGuiCol_Separator, P(MenuColor::Rule));
        Color(ImGuiCol_SeparatorHovered, P(MenuColor::Rule));
        Color(ImGuiCol_SeparatorActive, P(MenuColor::Rule));
        // Text-only tabs: no tab fill at all; the active one gets a white label and an underline (RenderMainMenuTable).
        Color(ImGuiCol_Tab, none);
        Color(ImGuiCol_TabHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.04f));
        Color(ImGuiCol_TabSelected, none);
        Color(ImGuiCol_TabSelectedOverline, none);
        Color(ImGuiCol_TabDimmed, none);
        Color(ImGuiCol_TabDimmedSelected, none);
        Color(ImGuiCol_TabDimmedSelectedOverline, none);
        Color(ImGuiCol_TextLink, P(MenuColor::Section));
        Color(ImGuiCol_TextSelectedBg, MockHex(0x7a1f2b, 0.5f));
        Color(ImGuiCol_NavCursor, P(MenuColor::CheckOn));
        Color(ImGuiCol_TableBorderStrong, P(MenuColor::Rule));
        Color(ImGuiCol_TableBorderLight, P(MenuColor::Rule));
        Color(ImGuiCol_TableHeaderBg, P(MenuColor::Frame));
        Color(ImGuiCol_TreeLines, P(MenuColor::Rule));
    }

    void Pop()
    {
        ImGui::PopStyleColor(colors);
        ImGui::PopStyleVar(vars);
        colors = vars = 0;
    }
};
} // namespace

ImVec4 MenuCommon::ThemeColor(MenuColor c)
{
    const int i = std::clamp(static_cast<int>(c), 0, static_cast<int>(MenuColor::Count) - 1);
    if (Config::Instance()->LightTheme.value_or_default())
    {
        // The light style's own colours (already tone-mapped by MenuHdrCheck when HDR is on).
        switch (c)
        {
        case MenuColor::Dim:
        case MenuColor::Tag:
        case MenuColor::CheckOutline:
            return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        case MenuColor::Section:
        case MenuColor::CheckOn:
            return ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        case MenuColor::Rule:
        case MenuColor::ButtonBorder:
            return ImGui::GetStyleColorVec4(ImGuiCol_Separator);
        case MenuColor::Frame:
            return ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
        case MenuColor::FrameHovered:
            return ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered);
        case MenuColor::FrameActive:
            return ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive);
        case MenuColor::Fill:
            return ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
        case MenuColor::FillActive:
        case MenuColor::Selected:
            return ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive);
        default:
            return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        }
    }
    return toneMapColor(kMockPalette[i]);
}

ImU32 MenuCommon::ThemeColorU32(MenuColor c) { return ImGui::GetColorU32(ThemeColor(c)); }

float MenuCommon::MockPx()
{
    // The built-in bitmap font is 13 px x the Menu Scale inside the window (its font scale), so one mock px is the
    // Menu Scale there, as RenderMainMenuWindow's theme has it.
    return ImGui::GetFontSize() / (Config::Instance()->UseHQFont.value_or_default() ? kMenuFontPx : kBitmapFontPx);
}

inline static std::string GetSourceString(UINT source)
{
    switch (source)
    {
    case 1:
        return "RTV";
    case 2:
        return "SRV";
    case 4:
        return "UAV";
    case 8:
        return "OM";
    case 16:
        return "Ups";
    case 32:
        return "SCR";
    case 64:
        return "SGR";
    default:
        return std::format("{}", source);
    }
}

inline static std::string GetDispatchString(UINT source)
{
    switch (source)
    {
    case 512:
        return "DI";
    case 1024:
        return "DII";
    case 256:
        return "Disp";
    default:
        return std::format("{}", source);
    }
}

// A compact status badge: a dot for the state and the name beside it, on a rounded
// plate. Replaces rows of "name: Exists / Doesn't Exist" text.
//
// The text form spent a whole line on each item and made the reader parse a sentence to
// learn one bit. A row of badges says the same thing in a glance and in a quarter of the
// vertical space - which is the difference between a header you skim and a header you
// scroll past to reach the settings.
static void StatusChip(const char* label, bool ok, bool sameLine = true)
{
    const ImVec4 good = ImVec4(0.30f, 0.78f, 0.42f, 1.0f);
    const ImVec4 bad = ImVec4(0.55f, 0.55f, 0.58f, 1.0f);   // absent, not broken
    const ImVec2 pad = ImVec2(8.0f, 3.0f);
    const float dot = ImGui::GetFontSize() * 0.32f;

    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 size(textSize.x + pad.x * 2.0f + dot * 2.0f + 6.0f, textSize.y + pad.y * 2.0f);

    // Why these came out as a column instead of a row. The test used to be "is the
    // cursor past the left margin? then SameLine" - but the badge is emitted with
    // ImGui::Dummy, and Dummy leaves the cursor at the START of the next line. So the
    // answer was "at the left margin" every single time, SameLine never ran, and every
    // badge landed under the one before it. The flag the caller passes already says
    // what was wanted; use it, and wrap only when the badge genuinely will not fit.
    if (sameLine)
    {
        const float spacing = 6.0f;
        const float prevEnd = ImGui::GetItemRectMax().x;
        // The cursor sits at the left margin here (Dummy put it there), so the available
        // region measured from it reaches exactly the right-hand content edge.
        const float lineEnd = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        if (prevEnd + spacing + size.x <= lineEnd)
            ImGui::SameLine(0.0f, spacing);
    }

    const ImVec2 p0 = ImGui::GetCursorScreenPos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, ImVec2(p0.x + size.x, p0.y + size.y),
                      ImGui::GetColorU32(ImGuiCol_FrameBg), size.y * 0.5f);
    dl->AddCircleFilled(ImVec2(p0.x + pad.x + dot, p0.y + size.y * 0.5f), dot,
                        ImGui::GetColorU32(ok ? good : bad));
    dl->AddText(ImVec2(p0.x + pad.x + dot * 2.0f + 6.0f, p0.y + pad.y),
                ImGui::GetColorU32(ok ? ImGuiCol_Text : ImGuiCol_TextDisabled), label);

    ImGui::Dummy(size);
}

// ---- Component status (AMDNR 0.3.4, S13; menu plan section 13, the owner's pick C + A) -------------------------
// 0.3.3.2 drew seven chips on the header row of every tab. 0.3.4 draws one summary line, "Components: 4 of 7 active",
// with the one absence that is expected on AMD beside it; clicking it opens the chips grouped by purpose (Upscalers,
// Frame gen, Hooks, NVIDIA). (Owner decision 4, 2026-09-26) The line is in the header again, right under the
// "Neural runtimes:" credits line, on every tab (it was the first row of the Advanced tab). Each chip is a rounded
// pill with a dot (green = loaded or found, grey = not present) and a hover that says what it is, where it was loaded
// from, or why grey can be normal. The owner's style also has an orange dot for "present but failed"; no component
// reports such a state the menu can read, so none is drawn. The same seven checks as 0.3.3.2's chips.
namespace
{
struct ComponentChip
{
    const char* label;      // the chip's name (0.3.3.2's)
    bool active;            // loaded or found
    const char* what;       // hover: what it is
    const char* absentNote; // hover while grey
    std::string (*where)(); // hover while green: where it was loaded from (called only while hovered), or nullptr
};

struct ComponentGroup
{
    const char* label;
    int first;
    int count;
};

std::string ComponentModuleFile(HMODULE module)
{
    if (module == nullptr)
        return {};
    wchar_t buf[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(module, buf, MAX_PATH);
    return n == 0 ? std::string() : wstring_to_string(std::wstring(buf, n));
}

// (AMDNR 0.3.4, the owner's pick A: amdnr_status_chips_styles.html) A pill: #1f1f23 with a 1 px #33333a border, fully
// rounded, 9 px of padding, a 7 px dot (green #3fbf6f = active, grey #6b6b70 = not present) 6 px before the name. A
// grey pill is at 60 %, as the mock's nvngx.dll pill. The pill is one row high (18 px at Menu Scale 1.0). Colours
// through the menu's HDR tone map; the light theme uses its own frame and separator colours.
struct PillMetrics
{
    float padX, dot, gap, height;
};

PillMetrics ComponentPillMetrics()
{
    const float s = MenuCommon::MockPx();
    return { ImFloor(9.0f * s), ImFloor(7.0f * s), ImFloor(6.0f * s), ImGui::GetFrameHeight() };
}

ImVec2 ComponentPillSize(const char* label)
{
    const PillMetrics m = ComponentPillMetrics();
    return ImVec2(m.padX * 2.0f + m.dot + m.gap + ImGui::CalcTextSize(label).x, m.height);
}

ImU32 ComponentDotColor(bool active)
{
    return ImGui::GetColorU32(toneMapColor(active ? MockHex(0x3fbf6f) : MockHex(0x6b6b70)));
}

void DrawComponentPill(const ComponentChip& chip)
{
    const PillMetrics m = ComponentPillMetrics();
    const ImVec2 size = ComponentPillSize(chip.label);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const bool light = Config::Instance()->LightTheme.value_or_default();
    const float alpha = chip.active ? 1.0f : 0.6f;
    auto withAlpha = [alpha](ImVec4 c)
    {
        c.w *= alpha;
        return ImGui::GetColorU32(c);
    };
    const ImVec4 fill = light ? ImGui::GetStyleColorVec4(ImGuiCol_FrameBg) : toneMapColor(MockHex(0x1f1f23));
    const ImVec4 border = light ? ImGui::GetStyleColorVec4(ImGuiCol_Separator) : toneMapColor(MockHex(0x33333a));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    dl->AddRectFilled(p0, p1, withAlpha(fill), size.y * 0.5f);
    dl->AddRect(p0, p1, withAlpha(border), size.y * 0.5f, 0, 1.0f);
    const float cy = ImFloor(p0.y + size.y * 0.5f) + 0.5f;
    ImVec4 dot = ImGui::ColorConvertU32ToFloat4(ComponentDotColor(chip.active));
    dl->AddCircleFilled(ImVec2(p0.x + m.padX + m.dot * 0.5f, cy), m.dot * 0.5f, withAlpha(dot));
    const float textY = p0.y + (size.y - ImGui::GetFontSize()) * 0.5f;
    dl->AddText(ImVec2(p0.x + m.padX + m.dot + m.gap, textY), withAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Text)),
                chip.label);
    ImGui::Dummy(size);

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(chip.what);
        if (!chip.active)
        {
            ImGui::TextDisabled("%s", chip.absentNote);
        }
        else if (chip.where != nullptr)
        {
            const std::string where = chip.where();
            if (!where.empty())
                ImGui::TextDisabled("Loaded from %s", where.c_str());
        }
        ImGui::EndTooltip();
    }
}

void RenderComponentStatus(const State& state, bool nvidiaGpu)
{
    const bool libxess = state.libxessExists || XeSSProxy::Module() != nullptr;
    const ComponentChip chips[] = {
        // Upscalers
        { "FSR 3.1", FfxApiProxy::Dx12Module() != nullptr,
          "AMD's FidelityFX API (amd_fidelityfx_loader_dx12.dll or amd_fidelityfx_dx12.dll):\n"
          "the FSR 3.X/4 backends and FSR frame generation go through it.",
          "Not loaded in this process.", []() { return wstring_to_string(FfxApiProxy::Dx12Module_Path()); } },
        { "FSR 3.1 SR", FfxApiProxy::Dx12Module_SR() != nullptr,
          "AMD's FSR upscaler (amd_fidelityfx_upscaler_dx12.dll): the FSR 3.X/4 backends.",
          "Not loaded in this process.", []() { return wstring_to_string(FfxApiProxy::Dx12Module_SR_Path()); } },
        { "libxess", libxess, "Intel XeSS (libxess.dll), beside the game or loaded by it: the XeSS backends.",
          "Not beside the game and not loaded.", []() { return ComponentModuleFile(XeSSProxy::Module()); } },
        // Frame gen
        { "FSR 3.1 FG", FfxApiProxy::Dx12Module_FG() != nullptr,
          "AMD's frame generation (amd_fidelityfx_framegeneration_dx12.dll): FSR frame generation.",
          "Not loaded in this process.", []() { return wstring_to_string(FfxApiProxy::Dx12Module_FG_Path()); } },
        // Hooks
        { "FSR hooks", state.fsrHooks,
          "OptiScaler hooked the game's own FSR 2 / FSR 3 calls, so an FSR game's\nupscaler inputs reach it.",
          "The game made no FSR 2 / FSR 3 call OptiScaler hooked\n(normal in DLSS and XeSS games).", nullptr },
        { "nvngx replacement", state.nvngxReplacement.has_value(),
          "nvngx_dlss.dll beside OptiScaler: with spoofing on, it is handed to a game\nthat looks for NVIDIA's "
          "nvngx.dll when the game folder has none.",
          "No nvngx_dlss.dll beside OptiScaler (normal on AMD).",
          []()
          {
              const auto& path = State::Instance().nvngxReplacement;
              return path.has_value() ? wstring_to_string(path.value()) : std::string();
          } },
        // NVIDIA
        { "nvngx.dll", state.nvngxExists,
          "NVIDIA's NGX loader, beside the game or loaded by it; DLSS games look for it.",
          nvidiaGpu ? "Not beside the game and not loaded."
                    : "Not present: normal on AMD, OptiScaler answers the game's DLSS calls itself.",
          nullptr },
    };
    const ComponentGroup groups[] = {
        { "Upscalers", 0, 3 },
        { "Frame gen", 3, 1 },
        { "Hooks", 4, 2 },
        { "NVIDIA", 6, 1 },
    };

    int active = 0;
    for (const ComponentChip& chip : chips)
        active += chip.active ? 1 : 0;

    // (AMDNR 0.3.4, the owner's pick C) One summary row that opens: the fold arrow, a dot (green while anything is
    // active), "Components: 4 of 7 active", and the one absence that is expected on AMD as a dim tag. A button, not a
    // tree node, so the dot can sit between the arrow and the text; keyboard and gamepad open it the same way.
    // (Owner decision 4) In the header the row is one text line high, as the credits line above it, so the header
    // grows by one dim-line step (the tab labels keep their gap under it) and the dim tag's baseline is the text's.
    static bool open = false;
    const std::string summary = StrFmt("Components: %d of %d active", active, static_cast<int>(std::size(chips)));
    {
        const float s = MenuCommon::MockPx();
        const float pad = ImFloor(1.5f * s + 0.5f); // the fold arrow's inset, as DlssNr::NeuralUi::TreeNode
        const float dot = ImFloor(7.0f * s);
        const float gap = ImFloor(6.0f * s);
        const float arrowW = ImGui::GetFontSize() + pad * 2.0f;
        const ImVec2 textSize = ImGui::CalcTextSize(summary.c_str());
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const float h = ImGui::GetTextLineHeight();
        if (ImGui::InvisibleButton("##amdnr_components", ImVec2(arrowW + dot + gap + textSize.x, h)))
            open = !open;
        const bool hovered = ImGui::IsItemHovered();
        if (hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (hovered)
            dl->AddRectFilled(p0, ImVec2(p0.x + arrowW + dot + gap + textSize.x, p0.y + h),
                              ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.03f)));
        const float textY = p0.y + (h - ImGui::GetFontSize()) * 0.5f;
        const ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
        dl->AddText(ImVec2(p0.x + pad, textY), text, open ? "v" : ">");
        dl->AddCircleFilled(ImVec2(p0.x + arrowW + dot * 0.5f, ImFloor(p0.y + h * 0.5f) + 0.5f), dot * 0.5f,
                            ComponentDotColor(active > 0));
        dl->AddText(ImVec2(p0.x + arrowW + dot + gap, textY), text, summary.c_str());
    }
    if (!state.nvngxExists && !nvidiaGpu)
        DlssNr::NeuralUi::DimTag("nvngx.dll not present (normal on AMD)");
    if (!open)
        return;

    // (AMDNR 0.3.4, the owner's pick A) The chips grouped by purpose on one row, each group a dim name and its pills
    // (6 px apart, 12 px between groups); a group moves to the next line only when all of it does not fit. Indented
    // like a fold's children. A dim legend under them.
    const float s = MenuCommon::MockPx();
    const float gap = ImFloor(6.0f * s);
    const float groupGap = ImFloor(12.0f * s);
    ImGui::Indent();
    for (size_t g = 0; g < std::size(groups); ++g)
    {
        const ComponentGroup& group = groups[g];
        float width = ImGui::CalcTextSize(group.label).x;
        for (int i = group.first; i < group.first + group.count; ++i)
            width += gap + ComponentPillSize(chips[i].label).x;
        if (g > 0)
        {
            // Dummy left the cursor at the next line's start, so the available width measured from it reaches the
            // content's right edge (as in StatusChip).
            const float prevEnd = ImGui::GetItemRectMax().x;
            const float lineEnd = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
            if (prevEnd + groupGap + width <= lineEnd)
                ImGui::SameLine(0.0f, groupGap);
        }
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", group.label);
        for (int i = group.first; i < group.first + group.count; ++i)
        {
            ImGui::SameLine(0.0f, gap);
            DrawComponentPill(chips[i]);
        }
    }
    ImGui::TextDisabled("%s", "green active - grey not present (often normal on AMD) - hover a pill for details");
    ImGui::Unindent();
}
} // namespace

// The DLSS Enabler frame generation sections' note while dlss-enabler-headless.dll is missing: what it
// is, where it comes from, and a link. AMDNR does not ship it.
static void RenderEnablerMissing()
{
    ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "dlss-enabler-headless.dll is missing");
    ImGui::TextWrapped("%s", kEnablerHowTo);
    ImGui::TextLinkOpenURL("DLSS Enabler on Nexus Mods", kEnablerUrl);
}

static void ApplyThemeStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    auto conf = Config::Instance();
    bool lightTheme = conf->LightTheme.value_or_default();

    // Geometry. This is the part that decides whether the menu reads as a different
    // program at a glance - far more than the palette does, because the eye registers
    // shape and spacing before it registers colour. The old values (2px rounding, 1px
    // borders everywhere, tight padding) are the stock ImGui look that every tool built
    // on it shares.
    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 7.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 7.0f;
    style.TabRounding = 7.0f;

    // Borderless. Depth comes from the background steps below instead of from outlines,
    // which is what stops a dense settings page looking like a spreadsheet. The light
    // theme keeps thin borders, where low contrast would otherwise lose the edges.
    style.WindowBorderSize = lightTheme ? 1.0f : 0.0f;
    style.PopupBorderSize = lightTheme ? 1.0f : 0.0f;
    style.FrameBorderSize = lightTheme ? 1.0f : 0.0f;
    style.TabBorderSize = 0.0f;
    style.SeparatorTextBorderSize = 2.0f;

    // Room to breathe. A settings menu is read, not just clicked, and line spacing is
    // most of what makes a long one readable.
    style.WindowPadding = ImVec2(14.0f, 12.0f);
    style.FramePadding = ImVec2(9.0f, 5.0f);
    style.ItemSpacing = ImVec2(9.0f, 7.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 5.0f);
    style.CellPadding = ImVec2(7.0f, 4.0f);
    style.IndentSpacing = 20.0f;
    style.SeparatorTextPadding = ImVec2(18.0f, 4.0f);
    style.SeparatorTextAlign = ImVec2(0.0f, 0.5f);

    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 14.0f;

    // Controls sit slightly left of centre in their row rather than hard left, which
    // keeps label and value from colliding on narrow windows.
    style.WindowTitleAlign = ImVec2(0.02f, 0.5f);

    auto Clamp01 = [](float v) { return std::max(0.0f, std::min(v, 1.0f)); };

    auto Mix = [](const ImVec4& a, const ImVec4& b, float t, float alpha = 1.0f)
    { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, alpha); };

    auto Luminance = [](const ImVec4& c) { return c.x * 0.2126f + c.y * 0.7152f + c.z * 0.0722f; };

    auto Saturate = [&](const ImVec4& color, float amount)
    {
        float lum = Luminance(color);

        return ImVec4(Clamp01(lum + (color.x - lum) * amount), Clamp01(lum + (color.y - lum) * amount),
                      Clamp01(lum + (color.z - lum) * amount), color.w);
    };

    // The palette is FIXED. It used to be read from MenuAccentColor / MenuBGColor, which
    // meant the look was whatever a user had left in the ini - usually stock ImGui blue,
    // which is precisely the look this is meant not to have. One identity, applied the
    // same way every time, is what makes a tool recognisable; a theme that is anything
    // the user last clicked is not a theme.
    ImVec4 accent = ImVec4(0.86f, 0.14f, 0.18f, 1.0f);
    ImVec4 bgAccent = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    float luminance = Luminance(accent);

    // Three background steps with a slight cool cast rather than three neutral greys.
    // Without borders these steps are what separates a panel from the window behind it,
    // so they are spaced a little wider apart than before, and the faint blue keeps the
    // dark theme from reading as washed-out black on an OLED.
    // Near-black with a faint warm cast, so red reads as the accent rather than as an
    // alarm colour sitting on cold grey. Three steps still, spaced to carry the depth
    // that the removed borders used to.
    const ImVec4 bgDark = lightTheme ? ImVec4(0.88f, 0.86f, 0.86f, 1.00f) : ImVec4(0.043f, 0.037f, 0.039f, 1.00f);
    const ImVec4 bgMid = lightTheme ? ImVec4(0.94f, 0.92f, 0.92f, 1.00f) : ImVec4(0.082f, 0.070f, 0.073f, 1.00f);
    const ImVec4 bgLight = lightTheme ? ImVec4(0.99f, 0.97f, 0.97f, 1.00f) : ImVec4(0.130f, 0.110f, 0.114f, 1.00f);

    const ImVec4 textPrimary = lightTheme ? ImVec4(0.05f, 0.06f, 0.08f, 1.00f) : ImVec4(0.90f, 0.93f, 0.95f, 1.00f);
    const ImVec4 textDim = lightTheme ? ImVec4(0.22f, 0.25f, 0.31f, 1.00f) : ImVec4(0.54f, 0.58f, 0.62f, 1.00f);

    const ImVec4 borderCol = lightTheme ? ImVec4(0.35f, 0.40f, 0.50f, 1.00f) : ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    const ImVec4 dimBg = lightTheme ? ImVec4(0.30f, 0.33f, 0.38f, 0.20f) : ImVec4(0.09f, 0.10f, 0.13f, 0.20f);
    const ImVec4 modalDimBg = lightTheme ? ImVec4(0.22f, 0.24f, 0.28f, 0.55f) : ImVec4(0.04f, 0.04f, 0.07f, 0.55f);

    // MenuBGColor: only background/surface tint.
    auto BgTint = [&](const ImVec4& base, float strength = 1.0f, float alpha = 1.0f)
    {
        float t = lightTheme ? (0.180f * strength) : (0.120f * strength);
        return Mix(base, bgAccent, t, alpha);
    };

    // MenuAccentColor: all visible interactive accent colors.
    auto AccentSoft = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.14f, alpha) : Mix(bgDark, accent, 0.32f, alpha); };

    auto AccentMed = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.42f, alpha) : Mix(bgDark, accent, 0.55f, alpha); };

    auto AccentStrong = [&](float alpha = 1.0f) { return ImVec4(accent.x, accent.y, accent.z, alpha); };

    const ImVec4 bgTitle = AccentSoft();

    auto SurfaceHover = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.12f, alpha) : Mix(bgLight, accent, 0.18f, alpha); };

    auto SurfaceActive = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.20f, alpha) : Mix(bgLight, accent, 0.28f, alpha); };

    auto TitleActive = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgTitle, accent, 0.18f, alpha) : Mix(bgTitle, accent, 0.16f, alpha); };

    auto PlotAccent = [&](float alpha = 1.0f)
    {
        if (lightTheme)
        {
            // Darken slightly for contrast on light bg — no channel floors
            return Mix(accent, ImVec4(0.00f, 0.00f, 0.00f, 1.00f), 0.20f, alpha);
        }

        // Brighten slightly for visibility on dark bg — no channel floors
        return Mix(accent, ImVec4(1.00f, 1.00f, 1.00f, 1.00f), 0.35f, alpha);
    };

    auto PlotAccentHovered = [&](float alpha = 1.0f)
    {
        if (lightTheme)
        {
            return Mix(PlotAccent(alpha), ImVec4(0.00f, 0.00f, 0.00f, 1.00f), 0.15f, alpha);
        }

        return Mix(PlotAccent(alpha), ImVec4(1.00f, 1.00f, 1.00f, 1.00f), 0.25f, alpha);
    };

    auto AccentReadable = [&](float alpha = 1.0f)
    {
        // Apply saturation boost and luminance correction only here,
        // so AccentStrong / AccentMed / AccentSoft stay true to the user's pick.
        ImVec4 a = Saturate(accent, lightTheme ? 1.35f : 1.25f);
        float lum = Luminance(a);

        if (lightTheme && lum > 0.72f)
            a = Mix(a, ImVec4(0.0f, 0.0f, 0.0f, 1.0f), 0.35f, 1.0f);

        if (!lightTheme && lum < 0.25f)
            a = Mix(a, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 0.30f, 1.0f);

        return ImVec4(a.x, a.y, a.z, alpha);
    };

    ImVec4* c = ImGui::GetStyle().Colors;

    float minAlpha = Config::Instance()->MenuBGColorA.value_or_default() >= 0.5f
                         ? Config::Instance()->MenuBGColorA.value_or_default()
                         : 0.5f;

    c[ImGuiCol_Text] = textPrimary;
    c[ImGuiCol_TextDisabled] = textDim;
    c[ImGuiCol_TextLink] = AccentReadable();

    // MenuBGColor only.
    c[ImGuiCol_WindowBg] = BgTint(bgDark, 1.00f, Config::Instance()->MenuBGColorA.value_or_default());
    c[ImGuiCol_ChildBg] = BgTint(bgMid, 1.10f, minAlpha + 0.1f);
    c[ImGuiCol_PopupBg] =
        lightTheme ? BgTint(bgLight, 0.90f) : BgTint(ImVec4(0.09f, 0.10f, 0.13f, 0.97f), 0.90f, 0.97f);
    c[ImGuiCol_MenuBarBg] = BgTint(bgDark, 0.85f);
    c[ImGuiCol_DockingEmptyBg] = BgTint(bgDark, 0.75f);

    c[ImGuiCol_Border] = borderCol;
    c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Neutral background, not MenuBGColor.
    c[ImGuiCol_FrameBg] = BgTint(bgLight, 0.50f, minAlpha + 0.15f);
    c[ImGuiCol_FrameBgHovered] = SurfaceHover();
    c[ImGuiCol_FrameBgActive] = SurfaceActive();

    c[ImGuiCol_TitleBg] = BgTint(bgTitle, 0.40f);
    c[ImGuiCol_TitleBgActive] = TitleActive();
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(bgTitle.x, bgTitle.y, bgTitle.z, 0.75f);

    c[ImGuiCol_ScrollbarBg] = BgTint(bgDark, 0.60f, minAlpha + 0.2f);
    c[ImGuiCol_ScrollbarGrab] = AccentSoft();
    c[ImGuiCol_ScrollbarGrabHovered] = AccentMed();
    c[ImGuiCol_ScrollbarGrabActive] = AccentStrong();

    c[ImGuiCol_CheckMark] = AccentReadable();
    c[ImGuiCol_SliderGrab] = AccentMed();
    c[ImGuiCol_SliderGrabActive] = AccentReadable();
    c[ImGuiCol_InputTextCursor] = AccentReadable();

    c[ImGuiCol_Button] = AccentSoft();
    c[ImGuiCol_ButtonHovered] = AccentMed();
    c[ImGuiCol_ButtonActive] = AccentStrong();

    // A collapsing header is a container, not a control. Filling it with accent - which
    // is what AccentSoft did - turned every section into a solid red bar and left the
    // page looking like a warning list. It sits one step above the panel it opens on.
    c[ImGuiCol_Header] = BgTint(bgLight, 1.00f, 0.90f);
    c[ImGuiCol_HeaderHovered] = AccentMed(0.95f);
    c[ImGuiCol_HeaderActive] = AccentStrong();

    c[ImGuiCol_Separator] = borderCol;
    c[ImGuiCol_SeparatorHovered] = AccentMed(0.85f);
    c[ImGuiCol_SeparatorActive] = AccentStrong();

    c[ImGuiCol_ResizeGrip] = AccentSoft(0.30f);
    c[ImGuiCol_ResizeGripHovered] = AccentStrong(0.70f);
    c[ImGuiCol_ResizeGripActive] = AccentStrong(0.95f);

    // Tabs: an unselected tab is background, a selected one is solid accent with white
    // text. Stock ImGui tints every tab with the accent and marks the active one with a
    // hairline, which reads as "these are all slightly on" rather than "this one is".
    c[ImGuiCol_Tab] = BgTint(bgMid, 1.00f, 0.0f);
    c[ImGuiCol_TabHovered] = Mix(bgMid, accent, 0.35f, 1.0f);
    c[ImGuiCol_TabSelected] = accent;
    c[ImGuiCol_TabSelectedOverline] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TabDimmed] = BgTint(bgMid, 1.00f, 0.0f);
    c[ImGuiCol_TabDimmedSelected] = Mix(bgMid, accent, 0.55f, 1.0f);
    c[ImGuiCol_TabDimmed] = BgTint(bgDark, 0.60f);
    c[ImGuiCol_TabDimmedSelected] = AccentSoft(0.75f);
    c[ImGuiCol_TabDimmedSelectedOverline] = borderCol;

    c[ImGuiCol_DockingPreview] = AccentStrong(0.70f);

    c[ImGuiCol_PlotLines] = PlotAccent();
    c[ImGuiCol_PlotLinesHovered] = PlotAccentHovered();
    c[ImGuiCol_PlotHistogram] = PlotAccent(0.85f);
    c[ImGuiCol_PlotHistogramHovered] = PlotAccentHovered();

    c[ImGuiCol_TableHeaderBg] = BgTint(bgMid, 0.80f, minAlpha + 0.25f);
    c[ImGuiCol_TableBorderStrong] = borderCol;
    c[ImGuiCol_TableBorderLight] = lightTheme ? ImVec4(0.68f, 0.72f, 0.80f, 1.00f) : AccentSoft();
    c[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
    c[ImGuiCol_TableRowBgAlt] = lightTheme ? ImVec4(0.00f, 0.00f, 0.00f, 0.045f) : ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

    c[ImGuiCol_TreeLines] = borderCol;
    c[ImGuiCol_TextSelectedBg] = AccentMed(0.38f);
    c[ImGuiCol_DragDropTarget] = AccentStrong(0.90f);
    c[ImGuiCol_NavCursor] = AccentReadable();
    c[ImGuiCol_NavWindowingHighlight] = AccentStrong(0.70f);
    c[ImGuiCol_NavWindowingDimBg] = dimBg;
    c[ImGuiCol_ModalWindowDimBg] = modalDimBg;

    _hdrTonemapApplied = false;
    MenuHdrCheck(ImGui::GetIO());
}

static double lastTime = 0.0;
static double lastFrameTime = 0.0;
static UINT64 uwpTargetFrame = 0;

void MenuCommon::Present()
{
    _frameCount++;

    auto now = Util::MillisecondsNow();

    if (lastTime > 0.0)
        lastFrameTime = now - lastTime;

    lastTime = now;

    if (_handle != nullptr)
        UpdateManualInput(_handle);
}

struct VersionCheckStatus
{
    bool completed = false;
    bool updateAvailable = false;
    std::string latestTag;
    std::string latestUrl;
    std::string error;
};

struct MenuCommon::RenderMenuContext
{
    State& state;
    decltype(Config::Instance()) config;
    ImGuiIO& io;
    IFeature* currentFeature = nullptr;

    double now = 0.0;
    double frameTime = 0.0;
    double frameRate = 0.0;
    float menuResScale = 1.0f;
    float fpsScale = 1.0f;
    float averageFrameTime = 0.0f;
    float averageUpscalerFT = 0.0f;

    bool frameTimesCalculated = false;
    bool newFrame = false;

    VersionCheckStatus versionStatus;
    std::string currentVersionText;

    // Cached when the menu is visible and shared by RenderMainMenuWindow section helpers.
    std::unique_ptr<std::decay_t<decltype(IdentifyGpu::getPrimaryGpu())>> primaryGpu;
};

static std::string splashMessage;

void MenuCommon::UpdateRenderTiming(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& now = ctx.now;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;

    if (config->OverlayMenu.value_or_default())
    {
        _frameCount++;

        // FPS & frame time calculation
        if (lastTime > 0.0)
        {
            frameTime = now - lastTime;
            frameRate = 1000.0 / frameTime;
        }

        lastTime = now;

        if (_handle != nullptr)
            UpdateManualInput(_handle);
    }
    else
    {
        if (state.activeFgInput == FGInput::NoFG || state.activeFgOutput == FGOutput::NoFG)
            MenuCommon::Present();

        frameTime = lastFrameTime;
        frameRate = 1000.0 / frameTime;
    }

    state.frameTimes.pop_front();
    state.frameTimes.push_back(frameTime);
}

void MenuCommon::UpdateMenuInputMode(RenderMenuContext& ctx)
{
    auto& io = ctx.io;

    // Moved here to prevent gamepad key replay
    if (_isVisible)
    {
        if (hasGamepad)
            io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

        io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    }
    else
    {
        capturingKey = false;
        hasGamepad = (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        io.ConfigFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoKeyboard;
    }
}

void MenuCommon::HandleMenuShortcuts(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;

    // Handle Inputs
    {
        // Re-toggle guard (AMDNR 0.3.4) for the menu and NR keys, here where every source of the two flags (the
        // poll and messages in UpdateManualInput, UWP KeyUp) is consumed once per RenderMenu. Keybind capture and
        // the other shortcuts are untouched.
        const bool menuToggledByLatch = menuToggleByLatch; // AMDNR 0.3.4.2
        menuToggleByLatch = false;
        if (inputMenu || inputDlssNr)
        {
            const uint32_t debounceMs =
                MenuInput::ToggleDebounceMsFromIni(config->MenuToggleDebounceMs.value_or_default());
            const uint64_t nowMs = GetTickCount64();
            if (inputMenu && !menuToggleGate.Accept(nowMs, debounceMs))
            {
                inputMenu = false;
                if (menuGateLog.Take())
                    LOG_INFO("Menu key edge ignored: {} ms after the last toggle (MenuToggleDebounceMs {})",
                             nowMs - menuToggleGate.lastAcceptedMs, debounceMs);
                else
                    LOG_DEBUG("Menu key edge ignored: {} ms after the last toggle (MenuToggleDebounceMs {})",
                              nowMs - menuToggleGate.lastAcceptedMs, debounceMs);
            }
            if (inputDlssNr && !nrToggleGate.Accept(nowMs, debounceMs))
            {
                inputDlssNr = false;
                LOG_DEBUG("Neural Rendering key edge ignored: {} ms after the last toggle (MenuToggleDebounceMs {})",
                          nowMs - nrToggleGate.lastAcceptedMs, debounceMs);
            }
        }

        if (inputFG)
        {
            inputFG = false;

            if (state.activeFgInput != FGInput::NoFG && state.activeFgOutput != FGOutput::NoFG &&
                (state.currentFGSwapchain != nullptr || state.activeFgInput == FGInput::NvngxFG))
            {
                config->FGEnabled = !config->FGEnabled.value_or_default();
                LOG_DEBUG("FG toggle key pressed, setting FGEnabled to {}", config->FGEnabled.value_or_default());

                if (config->FGEnabled.value_or_default())
                    state.fgChanged = true;
            }
        }

        if (inputFps)
        {
            inputFps = false;
            config->ShowFps = !config->ShowFps.value_or_default();
        }

        if (inputCapture)
        {
            inputCapture = false;
            AmdNrRequestCapture(0);
            ImGuiToast toast { ImGuiToastType::Info, 1500 };
            toast.setTitle("DLSS Neural Rendering");
            toast.setContent("Capturing 8 frames");
            ImGui::InsertNotification(toast);
        }

        if (inputDlssNr)
        {
            inputDlssNr = false;
            config->DlssNrEnabled = !config->DlssNrEnabled.value_or_default();
            // INFO, not DEBUG: every in-game NR on/off belongs in a report, and the default LogLevel 2
            // dropped this line (the TLOU logs could not say when NR had been toggled)
            LOG_INFO("Neural Rendering toggle key pressed, setting DlssNrEnabled to {}",
                     config->DlssNrEnabled.value_or_default());
            // Both AMD runtimes: the carried edit and the runtime history are from before the
            // pause; start clean when the pass comes back.
            DlssNr::AmdBridge::InvalidateHistory();

            // "On" must not hide a runtime that stopped for this session (refused, failed, poisoned).
            // Read before the toggle note below, so nothing written for the note can hide the stop.
            const bool nrOn = config->DlssNrEnabled.value_or_default();
            const bool runtimeStopped = nrOn && AmdNeuralRuntimeStopped();

            // Same line in amd_presr.log / lmxxf_backend.log, next to the Record lines (0.3.3.2)
            DlssNr::AmdBridge::NoteToggle(nrOn, "key");
            // The notice names the runtime that carries the pass (AMDNR 0.3.4): "Neural Rendering: On (lmxxf)".
            const char* runtime = nrOn ? ToggleNoticeRuntime() : nullptr;
            ImGuiToast toast { runtimeStopped ? ImGuiToastType::Warning : ImGuiToastType::Info,
                               runtimeStopped ? 5000 : 2000 };
            if (!nrOn)
                toast.setTitle("%s", "Neural Rendering: Off");
            else if (runtime != nullptr)
                toast.setTitle("Neural Rendering: On (%s)", runtime);
            else
                toast.setTitle("%s", "Neural Rendering: On");
            if (runtimeStopped)
                toast.setContent("%s", "but the neural runtime is stopped (see the Neural tab)");
            ImGui::InsertNotification(toast);
        }

        if (inputFpsCycle && config->ShowFps.value_or_default())
            config->FpsOverlayType = (FpsOverlay) ((config->FpsOverlayType.value_or_default() + 1) % FpsOverlay_COUNT);

        if (inputMenu)
        {
            inputMenu = false;
            _isVisible = !_isVisible;

            LOG_DEBUG("Menu key pressed, {0}", _isVisible ? "opening ImGui" : "closing ImGui");

            if (_isVisible)
            {
                io.ClearEventsQueue();
                io.ClearInputKeys();
                io.ClearInputMouse();

                OptiInput::ResetMenuInputTransientState();

                ApplyThemeStyle();
                // ApplyThemeStyle writes the 1.0 geometry (padding, spacing, rounding); only the
                // rescale in RenderMainMenuWindow multiplies it by the Menu Scale, so let it run again.
                lastMenuScale = 0.0f;

                refreshRate = Util::GetActiveRefreshRate(_handle);

                auto optiPath = std::filesystem::path(Config::Instance()->MainDllPath.value());
                state.artursFgFileAvailable = enablerExists.Get(optiPath / L"dlss-enabler-headless.dll");
                state.nukemsFgFileAvailable = nukemsExistsShipped.Get(optiPath / L"amdnr_dlssg_fsr3.dll") ||
                                              nukemsExists.Get(optiPath / L"dlssg_to_fsr3_amd_is_better.dll");

                if (State::Instance().currentFeature != nullptr)
                {
                    if (State::Instance().currentFeature->GetUpscalerType() == Upscaler::DLSSD)
                        comboPreset = config->DLSSDRenderPresetForAll.value_or_default();
                    else if (State::Instance().currentFeature->GetUpscalerType() == Upscaler::DLSS)
                        comboPreset = config->RenderPresetForAll.value_or_default();
                }

                // AMDNR 0.3.4.2 (D3): what can keep input from the menu, on the first opens of a session.
                OptiInput::LogMenuOpenDiagnostics(static_cast<int>(state.screenWidth),
                                                  static_cast<int>(state.screenHeight), io.DisplaySize.x,
                                                  io.DisplaySize.y);
            }
            else
            {
                // AMDNR 0.3.4.2: CloseCurrentPopup did nothing here (no popup is being drawn between two frames), so
                // the runtime chooser's modal stayed in ImGui's popup stack and came back on every open. This trims
                // the stack (between frames that is all it does; it also closes an open combo, as meant before), and
                // the chooser takes it as "Decide later" for this session.
                ImGui::ClosePopupsOverWindow(nullptr, false);
                ChooserMenuClosed("menu key");

                // Closed on the press: keep that key from the game until it is released (a close on the release
                // never let the game see it either).
                if (menuToggledByLatch)
                    OptiInput::HoldKeyUntilReleased(config->ShortcutKey.value_or_default());

                _showMipmapCalcWindow = false;
                _showHudlessWindow = false;
            }

            io.MouseDrawCursor = _isVisible;
            io.WantCaptureKeyboard = _isVisible;
            io.WantCaptureMouse = _isVisible;
        }

        inputFpsCycle = false;
    }
}

void MenuCommon::UpdateVersionAndStartupNotifications(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& now = ctx.now;
    auto& versionStatus = ctx.versionStatus;

    constexpr double splashTime = 7000.0;
    constexpr int updateNoticeTime = 10000;

    // Version check state is copied while locked, then consumed by the UI render pass.
    {
        std::scoped_lock lock(state.versionCheckMutex);
        versionStatus.completed = state.versionCheckCompleted;
        versionStatus.updateAvailable = state.updateAvailable;
        versionStatus.latestTag = state.latestVersionTag;
        versionStatus.latestUrl = state.latestVersionUrl;
        versionStatus.error = state.versionCheckError;
    }

    ctx.currentVersionText = VersionCheck::CurrentVersionString();

    if (versionStatus.completed && versionStatus.updateAvailable && !versionStatus.latestTag.empty())
    {
        if (updateNoticeTag != versionStatus.latestTag)
        {
            updateNoticeTag = versionStatus.latestTag;
            updateNoticeUrl = versionStatus.latestUrl;
            const auto notice = [&]()
            {
                ImGuiToast updateNotification { ImGuiToastType::Error, updateNoticeTime };
                updateNotification.setTitle("AMDNR update available");
                updateNotification.setContent(
                    "Press %s for more info",
                    Keybind::KeyNameFromVirtualKeyCode(config->ShortcutKey.value_or_default()).c_str());
                ImGui::InsertNotification(updateNotification);
                return true;
            };
            static auto res = notice();
        }
    }

    // One-shot startup warning notifications.
    if (!state.postDone)
    {
        if (state.postCodes & PostCode::SlPluginsAlreadyInMemory)
        {
            auto filename = Util::DllPath().filename().string();
            to_lower_in_place(filename);

            ImGuiToast notification { ImGuiToastType::Warning, 10000 };
            notification.setTitle("Late Streamline hook detected");
            notification.setContent(
                "Consider renaming OptiScaler from %s to other supported name.\nYou may experience issues otherwise.",
                filename.c_str());
            ImGui::InsertNotification(notification);
        }

        if (state.postCodes & PostCode::TryingFsr4Fp8OnUnsupported)
        {
            ImGuiToast notification { ImGuiToastType::Warning, 10000 };
            notification.setTitle("Silly goose detected");
            notification.setContent("FSR 4 FP8 only works on AMD");
            ImGui::InsertNotification(notification);
        }

        // REFramework (AMDNR 0.3.3.2, misc/REFrameworkCheck.h): the dinput8.dll actually loaded decides. The startup
        // check only looked at the files, so it is corrected here once, in either direction, with one log line.
        if (state.gameQuirks & GameQuirk::NeedsREFramework)
        {
            bool loaded = false;
            const std::wstring proxy = REFrameworkCheck::LoadedProxy(loaded);
            const bool warned = static_cast<bool>(state.postCodes & PostCode::REFrameworkMissing);
            if (loaded && !proxy.empty() && warned)
            {
                state.postCodes.reset(PostCode::REFrameworkMissing);
                LOG_INFO("REFramework found after all: dinput8.dll loaded from {}", wstring_to_string(proxy));
            }
            else if (loaded && proxy.empty() && !warned)
            {
                if (config->WarnMissingREFramework.value_or_default())
                {
                    state.postCodes |= PostCode::REFrameworkMissing;
                    LOG_WARN("REFramework NOT loaded (this process uses Windows' own dinput8.dll). {}",
                             REFrameworkCheck::Advice(state.gameExe, false));
                }
                else
                {
                    // WarnMissingREFramework=false: no post code, as at startup (dllmain.cpp, OI-12B). One INFO only
                    // when the startup check had found REFramework's files, the new fact here; when it found none it
                    // already logged "REFramework not found (warning silenced ...)" (G1).
                    bool folderOnly = false;
                    if (!REFrameworkCheck::FilesFound(Util::ExePath().parent_path(), folderOnly).empty())
                        LOG_INFO("REFramework NOT loaded (this process uses Windows' own dinput8.dll); no warning: "
                                 "[Hotfix] WarnMissingREFramework=false");
                }
            }

            // The notice honours [Hotfix] WarnMissingREFramework (AMDNR 0.3.4); the state above stays as it is.
            if ((state.postCodes & PostCode::REFrameworkMissing) && config->WarnMissingREFramework.value_or_default())
            {
                const std::string title = REFrameworkCheck::GameTitle(state.gameExe);
                const bool confirmed = REFrameworkCheck::Confirmed(state.gameExe);
                ImGuiToast notification { ImGuiToastType::Warning, 20000 };
                notification.setTitle("REFramework not found");
                notification.setContent(
                    "%s needs REFramework with OptiScaler%s,\nor it crashes 15-60 s after launch.\n"
                    "Put dinput8.dll from the REFramework nightly in the game folder.\nThe menu (%s) has the link.",
                    title.c_str(), confirmed ? "" : " (probably)",
                    Keybind::KeyNameFromVirtualKeyCode(config->ShortcutKey.value_or_default()).c_str());
                ImGui::InsertNotification(notification);
            }
        }

        state.postDone = true;
    }

    // Initialize splash timing and select the splash text once per process.
    if (splashLimit < 1.0f)
    {
        splashStart = now + 100.0;
        splashLimit = splashStart + splashTime;

        std::srand(static_cast<unsigned>(std::time(nullptr)));
        splashMessage = splashText[std::rand() % splashText.size()];
    }
}

void MenuCommon::BeginMenuFrameIfNeeded(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& now = ctx.now;
    auto& newFrame = ctx.newFrame;

    // New frame check
    // The lamp is drawn while the menu is closed, which is the whole point of it. Tied to its own
    // setting and nothing else: an overlay that appears because a scan is running, rather than
    // because someone asked for it, is an overlay nobody asked for.
    const bool scanIndicator = config->DlssNrScanMeter.value_or_default() &&
                               DlssNr::ExposureScan::Where() != DlssNr::ExposureScan::Verdict::Off;

    if ((!config->DisableSplash.value_or_default() && now > splashStart && now < splashLimit) ||
        config->ShowFps.value_or_default() || _isVisible || ImGui::notifications.size() > 0 || scanIndicator ||
        (config->DlssNrCompare.value_or_default() != 0 && config->DlssNrCompareTags.value_or_default()))
    {
        if (!_isUWP)
        {
            ImGui_ImplWin32_NewFrame();
        }
        else
        {
            ImVec2 displaySize { state.screenWidth, state.screenHeight };
            ImGui_ImplUwp_NewFrame(displaySize);
        }

        OptiInput::FeedImGui(_isVisible);

        MenuHdrCheck(io);
        ImGui::NewFrame();

        newFrame = true;
    }
}

void MenuCommon::RenderSplashWindow(RenderMenuContext& ctx)
{
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& now = ctx.now;

    constexpr double fadeTime = 1000.0;

    // Splash screen
    if (!config->DisableSplash.value_or_default())
    {
        if (now > splashStart && now < splashLimit)
        {

            ImGui::SetNextWindowSize({ 0.0f, 0.0f });
            ImGui::SetNextWindowBgAlpha(config->FpsOverlayAlpha.value_or_default());
            ImGui::SetNextWindowPos(splashPosition, ImGuiCond_Always);

            float windowAlpha = 1.0f;
            if (auto diff = now - splashStart; diff < fadeTime)
                windowAlpha = static_cast<float>(diff / fadeTime);
            else if (auto diff = splashLimit - now; diff < fadeTime)
                windowAlpha = static_cast<float>(diff / fadeTime);

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, windowAlpha);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));

            if (!config->OverlaysUseTheme.value_or_default())
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, toneMapColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
            }

            if (ImGui::Begin("Splash", nullptr,
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav))
            {
                float splashScale = 1.0f;
                float baseScaleHeight = 720.0f;

                if (io.DisplaySize.y > baseScaleHeight)
                    splashScale = io.DisplaySize.y / baseScaleHeight;

                if (config->UseHQFont.value_or_default())
                    ImGui::PushFontSize(std::round(splashScale * fontSize));
                else
                    ImGui::SetWindowFontScale(splashScale);

                ImGui::Text("AMDNR - %s for menu",
                            Keybind::KeyNameFromVirtualKeyCode(config->ShortcutKey.value_or_default()).c_str());
                ImGui::TextColored(toneMapColor(ImVec4(1.0f, 1.0f, 1.0f, 0.7f)), "%s", splashMessage.c_str());

                splashSize = ImGui::GetWindowSize();

                if (config->UseHQFont.value_or_default())
                    ImGui::PopFontSize();

                ImGui::End();

                splashPosition.x = 0.0f; // io.DisplaySize.x - splashWinSize.x;
                splashPosition.y = io.DisplaySize.y - splashSize.y;
            }

            if (!config->OverlaysUseTheme.value_or_default())
                ImGui::PopStyleColor(4);
            else
                ImGui::PopStyleColor(2);

            ImGui::PopStyleVar(2);
        }
    }
}

void MenuCommon::RenderNotifications(RenderMenuContext& ctx)
{
    auto config = ctx.config;
    auto& io = ctx.io;

    // Notifications
    bool tonemapRequired = State::Instance().isHdrActive ||
                           (!Config::Instance()->OverlayMenu.value_or_default() &&
                            State::Instance().currentFeature != nullptr && State::Instance().currentFeature->IsHdr());

    float screenHeight = State::Instance().screenHeight;
    if (io.DisplaySize.y != 0)
        screenHeight = io.DisplaySize.y;

    // Map resolution height to scale, 0.5 for 480p, 2.0 for 1440p
    constexpr float slope = (2.0f - 0.5f) / (1440.f - 480.f);
    float notificationScale = 0.5f + slope * (screenHeight - 480.f);
    notificationScale = std::clamp(notificationScale, 0.5f, 2.0f);

    if (config->UseHQFont.value_or_default())
        ImGui::PushFontSize(std::round(notificationScale * fontSize));

    // No fallback font, SetWindowFontScale needs to be called after Begin()

    ImGui::RenderNotifications(ImGuiToastPos::TopCenter, notificationScale, tonemapRequired);

    if (config->UseHQFont.value_or_default())
        ImGui::PopFontSize();
}

void MenuCommon::UpdateFrameTimeAverages(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;
    auto& frameTimesCalculated = ctx.frameTimesCalculated;
    auto& menuResScale = ctx.menuResScale;
    auto& fpsScale = ctx.fpsScale;
    auto& averageFrameTime = ctx.averageFrameTime;
    auto& averageUpscalerFT = ctx.averageUpscalerFT;

    // FPS Overlay font
    fpsScale = config->FpsScale.value_or(menuResScale);

    // Update frame time & upscaler time averages
    averageFrameTime = 0.0f;
    averageUpscalerFT = 0.0f;

    if (config->ShowFps.value_or_default() || _isVisible)
    {
        float frameCnt = 0;
        frameTime = 0;
        for (size_t i = 299; i > 199; i--)
        {
            if (state.frameTimes[i] > 0.0)
            {
                frameTime += state.frameTimes[i];
                frameCnt++;
            }
        }

        frameTime /= frameCnt;
        frameRate = 1000.0 / frameTime;
        frameTimesCalculated = true;

        float lastFT = static_cast<float>(state.frameTimes.empty() ? 0.0f : state.frameTimes.back());
        float lastUT = static_cast<float>(state.upscaleTimes.empty() ? 0.0f : state.upscaleTimes.back());
        gFrameTimes.Push(lastFT);
        gUpscalerTimes.Push(lastUT);

        averageFrameTime = gFrameTimes.Average();
        averageUpscalerFT = gUpscalerTimes.Average();
    }
}

// Labels for the comparison views.
//
// Drawn straight onto the foreground draw list, not as ImGui windows -- the last attempt made them
// draggable windows and the clamping fought the split. Here each label is clipped to its own side of
// the comparison, so in the wipe the moving split reveals and hides it exactly as it does the
// pictures, and there is nothing to drag. Both wipe labels sit in the same top-left corner, each
// clipped to its side, so whichever picture currently owns that corner is the one whose label shows.
void MenuCommon::RenderNrCompareTags()
{
    auto* config = Config::Instance();

    const uint32_t mode = config->DlssNrCompare.value_or_default();

    if (mode == 0 || !config->DlssNrCompareTags.value_or_default())
        return;

    const ImVec2 screen = ImGui::GetIO().DisplaySize;

    if (screen.x < 1.0f || screen.y < 1.0f)
        return;

    const bool swap = config->DlssNrCompareSwap.value_or_default();
    const float split = mode == 1 ? 0.5f
                                  : std::clamp(config->DlssNrCompareSplit.value_or_default(), 0.0f, 1.0f);
    const float splitX = split * screen.x;

    const float scale = std::clamp(config->DlssNrTagScale.value_or_default(), 0.5f, 5.0f);

    // The left side is the untouched frame unless swapped -- matching the shader's
    // showOriginal = (uv.x < split) != swap.
    const char* leftText = swap ? "DLSS NR : ON" : "DLSS NR : OFF";
    const char* rightText = swap ? "DLSS NR : OFF" : "DLSS NR : ON";

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize() * scale;
    const float margin = 10.0f * scale;

    // Both labels flank the divider along the top: the left picture's label is right-aligned just
    // left of the split, the right picture's is left-aligned just right of it. Each is clipped to its
    // own side, so in the wipe the split reveals and hides them along with the images.
    auto drawTag = [&](const char* text, float x, ImVec2 clipMin, ImVec2 clipMax)
    {
        const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);

        // Never let a label run off the visible frame as it grows.
        x = std::min(std::max(x, 0.0f), screen.x - size.x);
        float y = std::min(margin, screen.y - size.y - margin);
        y = std::max(y, 0.0f);

        dl->PushClipRect(clipMin, clipMax, true);
        dl->AddText(font, fontSize, ImVec2(x + 2.0f, y + 2.0f), IM_COL32(0, 0, 0, 210), text);
        dl->AddText(font, fontSize, ImVec2(x, y), IM_COL32(255, 255, 255, 255), text);
        dl->PopClipRect();
    };

    const ImVec2 leftSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, leftText);

    // Left picture's label: right edge a margin in from the split. Right picture's: left edge a margin
    // out from the split.
    drawTag(leftText, splitX - margin - leftSize.x, ImVec2(0.0f, 0.0f), ImVec2(splitX, screen.y));
    drawTag(rightText, splitX + margin, ImVec2(splitX, 0.0f), ImVec2(screen.x, screen.y));
}

void MenuCommon::RenderPerformanceOverlay(RenderMenuContext& ctx)
{
    RenderNrCompareTags();


    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& currentFeature = ctx.currentFeature;
    auto& now = ctx.now;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;
    auto& menuResScale = ctx.menuResScale;
    auto& fpsScale = ctx.fpsScale;
    auto& averageFrameTime = ctx.averageFrameTime;
    auto& averageUpscalerFT = ctx.averageUpscalerFT;

    // If Fps overlay is visible
    if (config->ShowFps.value_or_default())
    {
        bool stylePushed = false;

        const static auto defaultStyle = ImGuiStyle();

        // Rescale the fps overlay every frame because it shares style with the main menu
        if (config->FpsScale.has_value() && config->FpsScale.value() != menuResScale)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultStyle.WindowPadding * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, defaultStyle.FramePadding * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, defaultStyle.CellPadding * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, defaultStyle.SeparatorTextPadding * fpsScale);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, defaultStyle.ItemSpacing * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, defaultStyle.ItemInnerSpacing * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, defaultStyle.IndentSpacing * fpsScale);

            stylePushed = true;
        }

        // Set overlay position
        ImGui::SetNextWindowPos(overlayPosition, ImGuiCond_Always);

        // Set overlay window properties
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));  // Transparent border
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0)); // Transparent frame background

        if (!config->OverlaysUseTheme.value_or_default())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, toneMapColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }

        ImGui::SetNextWindowBgAlpha(config->FpsOverlayAlpha.value_or_default()); // Transparent background

        if (!config->OverlaysUseTheme.value_or_default())
        {
            ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, toneMapColor(green));
        }

        if (ImGui::Begin("Performance Overlay", nullptr,
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav))
        {
            std::string api;
            if (IdentifyGpu::getPrimaryGpu().usesDxvk && state.api == DX11)
            {
                if (state.swapchainInteropApi == SwapchainInteropApi::None)
                    api = "DXVK";
                else
                    api = "DXVK w/Dx12";
            }
            else if (IdentifyGpu::getPrimaryGpu().usesVkd3dProton && state.api == DX12)
            {
                api = "VKD3D";
            }
            else
            {
                switch (state.swapchainApi)
                {
                case Vulkan:
                    api = "VLK";
                    break;

                case DX11:
                    api = "D3D11";
                    break;

                case DX12:
                    if (state.swapchainInteropApi == SwapchainInteropApi::Dx11wDx12)
                        api = "D3D11 w/DX12";
                    else
                        api = "D3D12";

                    break;

                default:
                    switch (state.api)
                    {
                    case Vulkan:
                        api = "VLK";
                        break;

                    case DX11:
                        api = "D3D11";
                        break;

                    case DX12:
                        api = "D3D12";
                        break;

                    default:
                        api = "???";
                        break;
                    }

                    break;
                }
            }

            if (config->UseHQFont.value_or_default())
                ImGui::PushFontSize(std::round(fpsScale * fontSize));
            else
                ImGui::SetWindowFontScale(fpsScale);

            std::string firstLine = "";
            std::string secondLine = "";
            std::string thirdLine = "";

            auto fg = state.currentFG;
            auto fgText = (fg != nullptr && fg->IsActive() && !fg->IsPaused()) ? (" (" + std::string(fg->Name()) + ")")
                                                                               : std::string();

            const int fakeFramesCount = state.dlssgDetectedInterpolationCount;
            auto formatFg = [&](std::string_view name, int maxFakeFrames)
            {
                if (fakeFramesCount > maxFakeFrames)
                    return std::format(" ({} Doesn't support more than {}x)", name, maxFakeFrames);

                else if (fakeFramesCount == 0)
                    return std::format(" ({} off)", name);

                return std::format(" ({} x{})", name, fakeFramesCount + 1);
            };

            const FGNvngxReplacement activeNvngxFg = state.activeFgNvngx;
            if (activeNvngxFg == FGNvngxReplacement::Arturs)
            {
                fgText = formatFg("Enabler", Nvngx_FG::getMaxFakeFramesCount());
            }
            else if (activeNvngxFg == FGNvngxReplacement::Nukems)
            {
                fgText = formatFg("Nukems", Nvngx_FG::getMaxFakeFramesCount());
            }
            else if (activeNvngxFg == FGNvngxReplacement::FFX)
            {
                fgText = formatFg("FFX", Nvngx_FG::getMaxFakeFramesCount());
            }
            else if (activeNvngxFg == FGNvngxReplacement::Combo)
            {
                fgText = formatFg("Combo", Nvngx_FG::getMaxFakeFramesCount());
            }
            else if (state.activeFgOutput == FGOutput::DLSSG && fg)
            {
                fgText = formatFg("DLSSG", fg->GetMaxInterpolationCount());
            }

            const auto overlayType = config->FpsOverlayType.value_or_default();
            const bool hasFeature = currentFeature && !currentFeature->IsFrozen();

            // Prepare Line 1
            std::string featurePart;
            std::string fpsPart;

            if (hasFeature)
            {
                const bool usesDx12CompatLayer = currentFeature->IsWithDx12();

                featurePart = StrFmt(" | %s -> %s %u.%u.%u%s", ApiUpscalerInputName(state.currentInputApiName).c_str(),
                                     currentFeature->ShortName().c_str(), currentFeature->Version().major,
                                     currentFeature->Version().minor, currentFeature->Version().patch,
                                     usesDx12CompatLayer ? " w/Dx12" : "");
            }

            if (fg != nullptr && fg->IsActive() && !fg->IsPaused())
            {
                const double baseFps = frameRate / (double) (fg->GetInterpolatedFrameCount() + 1);

                switch (overlayType)
                {
                case FpsOverlay_JustFPS:
                    fpsPart = StrFmt("%6.1f/%5.1f ", frameRate, baseFps);
                    break;

                case FpsOverlay_Simple:
                    fpsPart = StrFmt("FPS: %6.1f/%5.1f, %7.2f ms", frameRate, baseFps, frameTime);
                    break;

                default:
                    fpsPart = StrFmt("FPS: %6.1f/%5.1f, Avg: %6.1f", frameRate, baseFps, 1000.0f / averageFrameTime);
                    break;
                }
            }
            else
            {
                switch (overlayType)
                {
                case FpsOverlay_JustFPS:
                    fpsPart = StrFmt("%6.1f ", frameRate);
                    break;

                case FpsOverlay_Simple:
                    fpsPart = StrFmt("FPS: %6.1f, %7.2f ms", frameRate, frameTime);
                    break;

                default:
                    fpsPart = StrFmt("FPS: %6.1f, Avg: %6.1f", frameRate, 1000.0f / averageFrameTime);
                    break;
                }
            }

            if (overlayType == FpsOverlay_JustFPS)
                firstLine = StrFmt("%s", fpsPart.c_str());
            else
                firstLine = StrFmt("%s | %s%s%s", api.c_str(), fpsPart.c_str(), fgText.c_str(), featurePart.c_str());

            // Prepare Line 2
            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Detailed)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                {
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text(" | ");
                    ImGui::SameLine(0.0f, 0.0f);
                }
                else
                {
                    ImGui::Spacing();
                }

                secondLine = StrFmt("Frame Time: %7.2f ms, Avg: %7.2f ms", state.frameTimes.back(), averageFrameTime);
            }

            // Prepare Line 3
            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Full)
            {
                thirdLine =
                    StrFmt("Upscaler Time: %7.2f ms, Avg: %7.2f ms", state.upscaleTimes.back(), averageUpscalerFT);
            }

            ImVec2 plotSize;
            if (config->FpsOverlayHorizontal.value_or_default())
            {
                plotSize = { fpsScale * 150, fpsScale * 16 };
            }
            else
            {
                // Find the widest text width
                auto firstSize = ImGui::CalcTextSize(firstLine.c_str());
                auto secondSize = ImGui::CalcTextSize(secondLine.c_str());
                auto thirdSize = ImGui::CalcTextSize(thirdLine.c_str());
                auto textWidth = 0.0f;

                if (firstSize.x > secondSize.x)
                    textWidth = firstSize.x > thirdSize.x ? firstSize.x : thirdSize.x;
                else
                    textWidth = secondSize.x > thirdSize.x ? secondSize.x : thirdSize.x;

                auto minWidth = fpsScale * 300.0f;
                auto plotWidth = textWidth < minWidth ? minWidth : textWidth;

                plotSize = { plotWidth, fpsScale * 30 };
            }

            // Draw the overlay
            ImGui::TextUnformatted(firstLine.c_str()); // runtime text, not a format (R8)

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Detailed)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                {
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text(" | ");
                    ImGui::SameLine(0.0f, 0.0f);
                }
                else
                {
                    ImGui::Spacing();
                }

                ImGui::TextUnformatted(secondLine.c_str());
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_DetailedGraph)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                    ImGui::SameLine(0.0f, 0.0f);

                // Graph of frame times
                ImGui::PlotLines(
                    "##FrameTimeGraph",
                    [](void* rb, int idx) -> float { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); },
                    &gFrameTimes, plotWidth, 0, nullptr, 0.0f, 66.6f, plotSize);
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Full)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                {
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text(" | ");
                    ImGui::SameLine(0.0f, 0.0f);
                }
                else
                {
                    ImGui::Spacing();
                }

                ImGui::TextUnformatted(thirdLine.c_str());
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_FullGraph)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                    ImGui::SameLine(0.0f, 0.0f);

                // Graph of upscaler times
                ImGui::PlotLines(
                    "##UpscalerFrameTimeGraph",
                    [](void* rb, int idx) -> float { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); },
                    &gUpscalerTimes, plotWidth, 0, nullptr, 0.0f, 20.0f, plotSize);
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_ReflexTimings)
            {
                constexpr auto delayBetweenPollsMs = 500;
                static auto previousPoll = 0.0;
                static bool gotData = false;

#ifdef LOW_LATENCY_INPUTS
                static TimingData timingData {};

                if (previousPoll <= 0.001 || previousPoll + delayBetweenPollsMs < now)
                {
                    gotData = InputCommon::get_timing_data(timingData);
                    previousPoll = now;
                }

                if (gotData && timingData.timeRange.has_value())
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    constexpr float offsetForText = 155;

                    const auto& rangeInNs = timingData.timeRange.value().length;

                    UINT64 localFrameCount = 0;

                    if (fg != nullptr)
                        localFrameCount = fg->FrameCount();

                    ImGui::Text("FGId: %llu, RfxId: %llu", localFrameCount, state.reflexFrameId);
                    ImGui::Text("Low latency timings, whole frame: %.1fms", rangeInNs / 1000.0);

                    const auto maxWidth =
                        config->FpsOverlayHorizontal.value_or_default() ? ImGui::GetWindowWidth() : plotSize.x;

                    const auto drawTiming = [&](const auto& timingOpt, const char* desc, ImVec4 color)
                    {
                        if (!timingOpt.has_value())
                            return;

                        auto toneMappedColor = State::Instance().isHdrActive ? toneMapColor(color) : color;

                        const auto& timing = timingOpt.value();
                        float duration = static_cast<float>(timing.length * rangeInNs / 1000.0);

                        ImGui::TextColored(toneMappedColor, "%-12s %4.1fms", desc, duration);

                        auto leftLimit = ImGui::GetItemRectMin().x + offsetForText * fpsScale;

                        auto start = static_cast<float>(leftLimit + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                        timing.position);

                        auto end = static_cast<float>(start + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                  timing.length);

                        auto pos = ImVec2(start, ImGui::GetItemRectMin().y);
                        auto size = ImVec2(end, ImGui::GetItemRectMax().y);

                        drawList->AddRectFilled(pos, size, ImGui::ColorConvertFloat4ToU32(toneMappedColor));
                    };

                    drawTiming(timingData.simulation, "Simulation", ImVec4(0.768f, 0.169f, 0.169f, 1.0f));
                    drawTiming(timingData.renderSubmit, "RenderSubmit", ImVec4(0.235f, 0.705f, 0.294f, 1.0f));
                    drawTiming(timingData.present, "Present", ImVec4(1.0f, 0.88f, 0.098f, 1.0f));
                    drawTiming(timingData.driver, "Driver", ImVec4(0.263f, 0.388f, 0.847f, 1.0f));
                    drawTiming(timingData.osRenderQueue, "RenderQueue", ImVec4(0.76f, 0.51f, 0.188f, 1.0f));
                    drawTiming(timingData.gpuRender, "GpuRender", ImVec4(0.569f, 0.117f, 0.705f, 1.0f));
                }
#else
                if (previousPoll <= 0.001 || previousPoll + delayBetweenPollsMs < now)
                {
                    gotData = ReflexHooks::updateTimingData();
                    previousPoll = now;
                }

                auto& timingData = ReflexHooks::timingData;

                if (gotData && timingData[TimingType::TimeRange].has_value())
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    constexpr float offsetForText = 155;

                    const auto& rangeInNs = timingData[TimingType::TimeRange].value().length;

                    UINT64 localFrameCount = 0;

                    if (fg != nullptr)
                        localFrameCount = fg->FrameCount();

                    ImGui::Text("FGId: %llu, RfxId: %llu", localFrameCount, state.reflexFrameId);
                    ImGui::Text("Reflex timings, whole frame: %.1fms", rangeInNs / 1000.0);

                    const auto maxWidth =
                        config->FpsOverlayHorizontal.value_or_default() ? ImGui::GetWindowWidth() : plotSize.x;

                    const auto drawTiming = [&](TimingType type, const char* desc, ImVec4 color)
                    {
                        if (!timingData[type].has_value())
                            return;

                        auto toneMappedColor = toneMapColor(color);

                        auto& timing = timingData[type].value();
                        float duration = static_cast<float>(timing.length * rangeInNs / 1000.0);
                        ImGui::TextColored(toneMappedColor, "%-12s %4.1fms", desc, duration);
                        auto leftLimit = ImGui::GetItemRectMin().x + offsetForText * fpsScale;
                        auto start = static_cast<float>(leftLimit + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                        timing.position);
                        auto end = static_cast<float>(start + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                  timing.length);
                        auto pos = ImVec2(start, ImGui::GetItemRectMin().y);
                        auto size = ImVec2(end, ImGui::GetItemRectMax().y);
                        drawList->AddRectFilled(pos, size, ImGui::ColorConvertFloat4ToU32(toneMappedColor));
                    };

                    drawTiming(TimingType::Simulation, "Simulation", ImVec4(0.768f, 0.169f, 0.169f, 1.0f));
                    drawTiming(TimingType::RenderSubmit, "RenderSubmit", ImVec4(0.235f, 0.705f, 0.294f, 1.0f));
                    drawTiming(TimingType::Present, "Present", ImVec4(1.0f, 0.88f, 0.098f, 1.0f));
                    drawTiming(TimingType::Driver, "Driver", ImVec4(0.263f, 0.388f, 0.847f, 1.0f));
                    drawTiming(TimingType::OsRenderQueue, "RenderQueue", ImVec4(0.76f, 0.51f, 0.188f, 1.0f));
                    drawTiming(TimingType::GpuRender, "GpuRender", ImVec4(0.569f, 0.117f, 0.705f, 1.0f));
                }
#endif
            }
        }

        // Restore the style
        if (!config->OverlaysUseTheme.value_or_default())
            ImGui::PopStyleColor(5);
        else
            ImGui::PopStyleColor(2);

        // Get size for postioning
        overlaySize = ImGui::GetWindowSize();

        if (config->UseHQFont.value_or_default())
            ImGui::PopFontSize();

        ImGui::End();

        if (stylePushed)
            ImGui::PopStyleVar(7);

        // Left / Right
        if (config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_TopLeft ||
            config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_BottomLeft)
        {
            overlayPosition.x = 0;
        }
        else
        {
            overlayPosition.x = io.DisplaySize.x - overlaySize.x;
        }

        // Top / Bottom
        if (config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_TopLeft ||
            config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_TopRight)
        {
            overlayPosition.y = 0;
        }
        else
        {
            // Prevent overlapping with splash message
            if (!config->DisableSplash.value_or_default() && now > splashStart && now < splashLimit)
                overlayPosition.y = io.DisplaySize.y - overlaySize.y - splashSize.y;
            else
                overlayPosition.y = io.DisplaySize.y - overlaySize.y;
        }
    }
}

// Save report (AMDNR 0.3.4, CRASH-BTN): one zip for a bug report, written on a worker thread (misc/AmdnrReport);
// greyed while a save runs. (0.3.4 MENU match1, the approved mock / D-3) Out of the header row: the first row of
// Advanced > Logging and the last of the Neural tab's Diagnostics drawer, one state for both (one tab is drawn at a
// time, so the result shows under the button the player is looking at).
void MenuCommon::RenderSaveReportRow(const char* idSuffix)
{
    const char* suffix = (idSuffix != nullptr && *idSuffix) ? idSuffix : "report";
    ImGui::PushID(suffix);
    static bool reportLineHidden = false;
    const AmdnrReport::Result report = AmdnrReport::LastResult();
    const std::string buttonLabel = std::string("Save report##") + suffix;
    ImGui::BeginDisabled(report.running);
    if (DlssNr::NeuralUi::Button(buttonLabel.c_str()) && AmdnrReport::SaveAsync())
        reportLineHidden = false;
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s", "One zip for a bug report: the logs, OptiScaler.ini and a system summary,\n"
                                "user names masked. Saved in the game folder, else on the Desktop,\n"
                                "else in %TEMP%; nothing is uploaded. Send it on the Discord with\n"
                                "what happened.");

    // The Save report result: dim while it runs and once saved (the zip's path, wrapped, with Open folder), in
    // orange when it failed; Hide removes it until the next save. The path is shown with the user folder name
    // masked (a zip on the Desktop or in %TEMP% sits under C:\Users\<name>, which reads C:\Users\<user>, and
    // players post menu screenshots); Open folder still gets the real path. The failure text is masked the same way
    // (a filesystem exception's text can name a path).
    if (report.running || (report.done && !reportLineHidden))
    {
        ImGui::PushTextWrapPos(0.0f);
        if (report.running)
            ImGui::TextDisabled("%s", "Saving the report...");
        else if (report.ok)
            ImGui::TextDisabled("Report saved: %s", AmdnrReport::MaskUserFolders(report.text).c_str());
        else
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.55f, 0.2f, 1.f)), "%s",
                               AmdnrReport::MaskUserFolders(report.text).c_str());
        ImGui::PopTextWrapPos();
        if (!report.running)
        {
            if (report.ok)
            {
                const size_t slash = report.text.find_last_of("\\/");
                if (slash != std::string::npos)
                {
                    if (DlssNr::NeuralUi::Button("Open folder##report"))
                    {
                        auto pIO = &ImGui::GetPlatformIO();
                        if (pIO->Platform_OpenInShellFn)
                            pIO->Platform_OpenInShellFn(ImGui::GetCurrentContext(),
                                                        report.text.substr(0, slash).c_str());
                    }
                    ImGui::SameLine();
                }
            }
            if (DlssNr::NeuralUi::Button("Hide##report"))
                reportLineHidden = true;
        }
    }
    ImGui::PopID();
}

void MenuCommon::RenderMainMenuHeaderMessages(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;
    auto& versionStatus = ctx.versionStatus;
    auto& currentVersionText = ctx.currentVersionText;
    auto& primaryGpu = *ctx.primaryGpu;

    if (!_showMipmapCalcWindow && !_showHudlessWindow && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        ImGui::SetWindowFocus();

    // The seven component chips that stood here on every tab are one summary line under the credits line (AMDNR
    // 0.3.4, S13 and owner decision 4: it opens them, grouped; RenderComponentStatus).
    // AMDNR's own two doors, where a player looks first: the Discord (support, reports, test
    // builds) and the GitHub releases, with the copyright line on the same row. The addresses
    // are in the buttons' hovers (the dimmed address line repeated them). Row 2 names the two
    // neural runtimes' authors on every tab, Daniel's name linked (the condition of his
    // permission to ship his runtime: prominent attribution); its hover holds the full credits.
    // One line each, no box, opened in the default browser like the Guide button below.
    {
        static const char* const kDiscordUrl = "https://discord.gg/AMDNR";
        static const char* const kGitHubUrl = "https://github.com/3zwr1/AMD-NR---OptiScaler";
        static const char* const kDanielUrl = "https://github.com/danielblnc/DLSS-NR-on-AMD";
        auto openUrl = [](const char* url)
        {
            auto pIO = &ImGui::GetPlatformIO();
            auto ctx = ImGui::GetCurrentContext();
            if (pIO->Platform_OpenInShellFn)
                pIO->Platform_OpenInShellFn(ctx, url);
        };
        // (AMDNR 0.3.4, the approved mock) Row 1: "AMDNR [Discord] [GitHub] (c) 2026 3zwr1 - see ...", the buttons in
        // the mock's style (18 px, grey, red-brown border), 13 px under the title bar as in the mock. The "AMDNR"
        // label's hover holds the version and the build stamp that the title bar showed until 0.3.4. Save report is
        // not here (the mock, D-3): it is the first row of Advanced > Logging and the last of Neural > Diagnostics
        // (RenderSaveReportRow).
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImFloor(2.0f * MockPx()));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("AMDNR");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("AMDNR %s\nbuild %s (%s)", AMDNR_VERSION_STR, VER_BUILD_DATE, VER_BUILD_COMMIT);
        ImGui::SameLine();
        if (DlssNr::NeuralUi::Button("Discord"))
            openUrl(kDiscordUrl);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Support, bug reports and test builds\n%s", kDiscordUrl);
        ImGui::SameLine();
        // The GitHub page also opens the guide (its README), which the footer's Guide button opened until 0.3.4.
        if (DlssNr::NeuralUi::Button("GitHub"))
            openUrl(kGitHubUrl);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Releases, source and the guide (the README: installation, the settings\n"
                              "worth knowing and what to do if something goes wrong)\n%s",
                              kGitHubUrl);
        ImGui::SameLine();
        // AMDNR's copyright notice (Licenses\AMDNR_NOTICE.txt); OptiScaler's own notices are unchanged.
        ImGui::TextDisabled("%s", "(c) 2026 3zwr1 - see Licenses\\AMDNR_NOTICE.txt");

        // Row 2: the runtime credits, one plain dim line as in the mock, on every tab (the condition of Daniel's
        // permission to ship his runtime: prominent attribution). Each author's part opens that author's page (the
        // hand cursor shows it); the hover holds every entry of the README's Credits section.
        // (0.3.4 MENU match1) 7 px between the buttons' frames and this line's text, as the mock.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImFloor(1.0f * MockPx()));
        static const char* const kLmxxfUrl = "https://github.com/lmxxf/dlss5-on-amd-9070xt-porting";
        auto creditPart = [&openUrl](const char* text, const char* url)
        {
            ImGui::TextDisabled("%s", text);
            if (url != nullptr && ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if (ImGui::IsItemClicked())
                    openUrl(url);
            }
        };
        ImGui::BeginGroup();
        creditPart("Neural runtimes: ", nullptr);
        ImGui::SameLine(0.0f, 0.0f);
        creditPart("DLSS-NR on AMD by Daniel Blanco (danielblnc)", kDanielUrl);
        ImGui::SameLine(0.0f, 0.0f);
        creditPart(" - ", nullptr);
        ImGui::SameLine(0.0f, 0.0f);
        creditPart("lmxxf by Kien (MIT)", kLmxxfUrl);
        ImGui::EndGroup();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        {
            static const char* const kCredits[] = {
                "DLSS-NR on AMD by Daniel Blanco (danielblnc), his runtime unmodified:",
                "    github.com/danielblnc/DLSS-NR-on-AMD (click the name to open it)",
                "lmxxf runtime by Kien (MIT): github.com/lmxxf/dlss5-on-amd-9070xt-porting",
                "lmxxf on RDNA 3: AMDNR's RDNA 3 backend by 3zwr1",
                "TheAutomatic: DLSS 5 AMD project, which the lmxxf integration builds on",
                "Matheus: dlss-5-amd project",
                "Dagherbou: OptiScaler_DLSSNR    wilsjo2: OptiScaler-DLSSNR-PreSR-Multipass",
                "RenoDX by clshortfuse (MIT): the colour composition maths",
                "Nukem9: dlssg-to-fsr3 (GPLv3, unmodified)",
                "Coldwood1026: XeFGUnlock (GPL-3.0), the base of the XeFG multi-frame unlock",
                "FSR Ray Regeneration by Zach Hembree (DarkHelmet), continued by burak113 (GPL-3.0)",
                "OptiScaler by Overclockers (GPL-3.0): the base of AMDNR",
                "AMDNR (c) 2026 3zwr1: Licenses\\AMDNR_NOTICE.txt; every licence is in the Licenses folder",
            };
            ImGui::BeginTooltip();
            ImGui::TextDisabled("Credits");
            for (const char* line : kCredits)
                ImGui::TextUnformatted(line);
            ImGui::EndTooltip();
        }
    }

    // Row 3 (owner decision 4, 2026-09-26): the component summary, "Components: N of 7 active" with the expected
    // nvngx.dll absence as a dim tag (the owner's pick C + A), on every tab right under the credits line; a click
    // opens the grouped pills under it. It was the first row of the Advanced tab.
    RenderComponentStatus(state, ctx.primaryGpu != nullptr && ctx.primaryGpu->vendorId == VendorId::Nvidia);

    // REFramework missing (0.3.3.2): shown until hidden for this session. No MessageBox. [Hotfix]
    // WarnMissingREFramework=false (AMDNR 0.3.4) silences it for a player who runs the game without REFramework.
    static bool refHidden = false;
    if (!refHidden && (state.postCodes & PostCode::REFrameworkMissing) &&
        config->WarnMissingREFramework.value_or_default())
    {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.55f, 0.2f, 1.f)), "%s",
                           ("REFramework not found: " + REFrameworkCheck::Advice(state.gameExe, false)).c_str());
        ImGui::PopTextWrapPos();
        if (DlssNr::NeuralUi::Button("REFramework nightly"))
        {
            auto pIO = &ImGui::GetPlatformIO();
            if (pIO->Platform_OpenInShellFn)
                pIO->Platform_OpenInShellFn(ImGui::GetCurrentContext(), REFrameworkCheck::kNightlyUrl);
        }
        ImGui::SameLine();
        if (DlssNr::NeuralUi::Button("Hide##ref"))
            refHidden = true;
    }

    if (config->MenuScale.has_value())
    {
        _selectedScale = ((int) (menuResScale * 10.0f)) - 4;
    }
    else
    {
        _selectedScale = 0;
    }

    if (versionStatus.completed)
    {
        if (versionStatus.updateAvailable && !versionStatus.latestTag.empty())
        {
            ImGui::Spacing();
            // AMDNR's own releases (version_check.cpp, 0.3.3.2); the current version is AMDNR's.
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), "AMDNR update available: %s (current %s)",
                               versionStatus.latestTag.c_str(), currentVersionText.c_str());

            if (!versionStatus.latestUrl.empty())
            {
                ImGui::SameLine();
                ImGui::TextLinkOpenURL("Open release page", versionStatus.latestUrl.c_str());
            }

            ImGui::Spacing();
        }
        else if (!versionStatus.error.empty())
        {
            LOG_ERROR("Version check failed: {0}", versionStatus.error);
            versionStatus.error.clear();
        }
        // Disabled error message
        // else if (!versionStatus.error.empty())
        //{
        //    ImGui::Spacing();
        //    ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.4f, 0.f, 1.f)), "%s", versionStatus.error.c_str());
        //    ImGui::Spacing();
        //}
    }

    // No active upscaler message. (AMDNR 0.3.4, the approved mock) One wrapped line at the menu's own size: at 2.5x and
    // 3x it took about 110 px above the tabs on every tab (critique item 2).
    if (currentFeature == nullptr || !currentFeature->IsInited())
    {
        if (state.nvngxExists || state.nvngxReplacement.has_value() ||
            (state.libxessExists || XeSSProxy::Module() != nullptr))
        {
            std::vector<std::string> upscalers;

            if (state.fsrHooks)
                upscalers.push_back("FSR");

            if (state.nvngxExists || state.nvngxReplacement.has_value() || primaryGpu.dlssCapable)
                upscalers.push_back("DLSS");

            if (state.libxessExists || XeSSProxy::Module() != nullptr)
                upscalers.push_back("XeSS");

            auto joined = upscalers | std::views::join_with(std::string { " or " });

            std::string joinedUpscalers(joined.begin(), joined.end());

            ImGui::PushTextWrapPos(0.0f);
            ImGui::Text("Please select %s as upscaler from game options and load a save game to enable Opti "
                        "settings. Upscalers don't always work in menus.",
                        joinedUpscalers.c_str());
            ImGui::PopTextWrapPos();

            if (primaryGpu.dlssCapable)
            {
                StatusChip("nvngx_dlss", state.NVNGX_DLSS_Path.has_value(), false);
                StatusChip("nvngx_dlssd", state.NVNGX_DLSSD_Path.has_value());
            }
        }
        else
        {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted("Can't find nvngx.dll and libxess.dll and FSR inputs. Upscaling support will NOT "
                                   "work.");
            ImGui::PopTextWrapPos();
        }
    }
    else if (currentFeature->IsFrozen())
    {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::Text("%s is active, but not currently used by the game. Please enter the game.",
                    currentFeature->Name().c_str());
        ImGui::PopTextWrapPos();
    }
}

void MenuCommon::RenderActiveUpscalerSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;
    auto& primaryGpu = *ctx.primaryGpu;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // UPSCALERS -----------------------------
        SectionHeader("Upscaler");

        GetCurrentBackendInfo(state.api, currentBackend, &currentBackendName);

        std::string spoofingText;

        const bool usesDlssd = currentFeature->GetUpscalerType() == Upscaler::DLSSD;
        const bool usesDx12CompatLayer = currentFeature->IsWithDx12();

        // (AMDNR 0.3.4, the approved other-tabs mock) The upscaler combo first ("Upscaler", Change Upscaler beside it),
        // then one dim status line: GPU - API - upscaler and version - input - spoofing. 0.3.3.2 drew the GPU name and
        // an "API | upscaler and version | Input | Spoof" line above an unlabelled combo; the same facts.
        const char* apiName = "Vulkan";
        switch (state.api)
        {
        case DX11:
            apiName = "D3D11";
            spoofingText = config->DxgiSpoofing.value_or_default() ? "on" : "off";
            break;

        case DX12:
            apiName = "D3D12";
            spoofingText = config->DxgiSpoofing.value_or_default() ? "on" : "off";
            break;

        default:
        {
            auto vlkSpoof = config->VulkanSpoofing.value_or_default();
            auto vlkExtSpoof = config->VulkanExtensionSpoofing.value_or_default();

            if (vlkSpoof && vlkExtSpoof)
                spoofingText = "on + ext";
            else if (vlkSpoof)
                spoofingText = "on";
            else if (vlkExtSpoof)
                spoofingText = "ext only";
            else
                spoofingText = "off";
        }
        }

        if (!usesDlssd)
        {
            switch (state.api)
            {
            case DX11:
                AddDx11Backends(currentBackend);
                break;
            case DX12:
                AddDx12Backends(currentBackend);
                break;
            default:
                AddVulkanBackends(currentBackend);
            }

            ImGui::SameLine();

            if (DlssNr::NeuralUi::Button("Change Upscaler##2") && state.newBackend != Upscaler::Reset &&
                state.newBackend != currentBackend)
            {
                if (state.newBackend == Upscaler::XeSS)
                {
                    // Reseting them for xess
                    config->DisableReactiveMask.reset();
                    config->DlssReactiveMaskBias.reset();
                }

                MARK_ALL_BACKENDS_CHANGED();
            }
        }

        {
            const std::string status = StrFmt(
                "%s - %s%s - %s %d.%d.%d%s - input %s - spoofing %s", primaryGpu.name.c_str(), apiName,
                primaryGpu.usesDxvk ? " (DXVK)" : "", currentFeature->ShortName().c_str(),
                currentFeature->Version().major, currentFeature->Version().minor, currentFeature->Version().patch,
                (state.api != DX12 && usesDx12CompatLayer) ? " w/Dx12" : "",
                ApiUpscalerInputName(state.currentInputApiName).c_str(), spoofingText.c_str());
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextDisabled("%s", status.c_str()); // runtime text through "%s" (R8)
            // The render and output sizes and the upscaler's time (menu decision D-4: 0.3.3.2's footer line; the
            // footer's FrameTime hover has them too, with the graph and the per-shader times).
            std::string sizes = StrFmt("render %ux%u -> %ux%u (%.2fx), display %ux%u", currentFeature->RenderWidth(),
                                       currentFeature->RenderHeight(), currentFeature->TargetWidth(),
                                       currentFeature->TargetHeight(),
                                       currentFeature->RenderWidth() > 0
                                           ? (float) currentFeature->TargetWidth() / (float) currentFeature->RenderWidth()
                                           : 0.0f,
                                       currentFeature->DisplayWidth(), currentFeature->DisplayHeight());
            if (!state.upscaleTimes.empty())
                sizes += StrFmt(" - upscaler %.2f ms", state.upscaleTimes.back());
            ImGui::TextDisabled("%s", sizes.c_str());
            ImGui::PopTextWrapPos();
        }

        if (!usesDlssd)
        {
            // The combo shows the pending pick; nothing changes until Change Upscaler (RE Requiem report, 0.3.3.2).
            if (state.newBackend != Upscaler::Reset && state.newBackend != currentBackend)
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                                   "Not applied yet - press Change Upscaler (running: %s)", currentBackendName.c_str());
            else if (state.api == DX12 && currentBackend == Upscaler::FSR21 &&
                     primaryGpu.fsr4Support != FSR4Support::None)
                ImGui::TextDisabled("FSR 2.1.2 is running on a card with FSR 4 (OptiScaler.ini Dx12Upscaler=fsr21?).\n"
                                    "Pick FSR 3.X/4, press Change Upscaler, then Save Settings.");
        }

        // The [FSR-RR] FfxDenoiserAllowPreRdna4 opt-in as a checkbox (AMDNR 0.3.4), shown where it means something:
        // D3D12, the upscaler combo shown, an AMD card that is not RDNA 4. Drawn after the Change Upscaler row so that
        // button stays beside the combo. The game asks whether Ray Reconstruction is supported while it starts, so the
        // choice is written to the ini at once (like the Neural runtime combo) and applies after a restart. (G3, the
        // the 0.3.4 Ray Regeneration rework's request 1 for RR-7000) The box shows what applies (StreamlineHooks::rrHardwareDecision): ticked on
        // RX 7000 / RDNA 3 while the key is unset (offered by default), unticked on older cards; writing is unchanged
        // (unticking on RX 7000 writes false = RDNA 4 only, ticking on an older card writes true).
        if (state.api == DX12 && !usesDlssd && primaryGpu.vendorId == VendorId::AMD &&
            !IdentifyGpu::isRdna4(primaryGpu))
        {
            bool allowPreRdna4 = StreamlineHooks::rrHardwareDecision().offered;
            if (DlssNr::NeuralUi::Checkbox("Offer FSR Ray Regeneration on this GPU (restart)", &allowPreRdna4))
            {
                config->FfxDenoiserAllowPreRdna4 = allowPreRdna4;
                config->SaveIni();
                LOG_INFO("Menu: FfxDenoiserAllowPreRdna4 set to {} (saved; applies after a game restart)",
                         allowPreRdna4);
            }
            if (IdentifyGpu::isRdna3(primaryGpu))
                ShowHelpMarker("On RDNA 3 (RX 7000, the RDNA 3 APUs) FSR Ray Regeneration is offered by default.\n"
                               "Untick to keep the game's own denoiser.\n"
                               "If the driver refuses it, the upscaler falls back to FSR and the Ray\n"
                               "Reconstruction image is NOT denoised. It also needs\n"
                               "amd_fidelityfx_denoiser_dx12.dll in this game's OptiScaler folder.\n\n"
                               "Saved at once; restart the game to apply.\n"
                               "[FSR-RR] FfxDenoiserAllowPreRdna4");
            else
                ShowHelpMarker("AMD ships Ray Regeneration for RDNA 4 only. Ticked, FSR Ray Regeneration is\n"
                               "offered on this card anyway and the driver decides when it is created.\n"
                               "If the driver refuses it, the upscaler falls back to FSR and the Ray\n"
                               "Reconstruction image is NOT denoised. It also needs\n"
                               "amd_fidelityfx_denoiser_dx12.dll in this game's OptiScaler folder.\n\n"
                               "Saved at once; restart the game to apply.\n"
                               "[FSR-RR] FfxDenoiserAllowPreRdna4");
        }

        // RR-20 (AMDNR 0.3.4): a Ray Reconstruction title that FSR Ray Regeneration gave up on (Satisfactory: the UE5
        // plugin publishes no camera matrices) says so here as well as on the Neural tab, so a player without a neural
        // runtime also sees why the denoiser is not running. State::rrFallbackReason is read as the Neural tab reads
        // it. In this section's warning colour, dim while the "Not applied yet" line above is showing.
        if (state.api == DX12 && !state.rrFallbackReason.empty())
        {
            const std::string rrWhy = state.rrFallbackReason;
            const bool pendingPick = !usesDlssd && state.newBackend != Upscaler::Reset &&
                                     state.newBackend != currentBackend;
            const char* rrLine = "Ray Regeneration is off in this title: FSR runs in its place";
            if (pendingPick)
                ImGui::TextDisabled("%s", rrLine);
            else
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), "%s", rrLine);
            const std::string rrTip = "FSR Ray Regeneration stopped in this title: " + rrWhy +
                                      "\n\nThe game still asks for Ray Reconstruction; FSR upscaling runs in its"
                                      "\nplace, without the denoiser. OptiScaler.log has the FSR-RR lines."
                                      "\n\nRay Regeneration's own options (Skin smoothing, the skin mask's SSS"
                                      "\nshare, ...) are in Neural > Ray Regeneration and show only while"
                                      "\nRay Regeneration runs.";
            ShowHelpMarker(rrTip.c_str());
            // P6 (Hogwarts Legacy: "no SSS option"): where Ray Regeneration's options live, in one dim line under the
            // warning (the section is hidden while RR is off, RR-20), so a player looking here finds them.
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextDisabled("%s", "Its options (Skin smoothing, SSS) are in Neural > Ray Regeneration and show "
                                      "only while it runs.");
            ImGui::PopTextWrapPos();
        }

        if (currentFeature->AccessToReactiveMask())
        {
            ImGui::BeginDisabled(config->DisableReactiveMask.value_or(false));

            auto useAsTransparency = config->FsrUseMaskForTransparency.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Use Reactive Mask as Transparency Mask", &useAsTransparency))
                config->FsrUseMaskForTransparency = useAsTransparency;

            ImGui::EndDisabled();
        }

        if (primaryGpu.dlssCapable && !state.NVNGX_DLSS_Path.has_value())
        {
            ImGui::Spacing();
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), "nvngx_dlss.dll not found, DLSS disabled!");
        }
    }

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        const bool usesDlssd = currentFeature->GetUpscalerType() == Upscaler::DLSSD;

        // Dx11 with Dx12
        if (state.api == DX11 && currentFeature->IsWithDx12())
        {
            if (auto ch = ScopedCollapsingHeader("Dx11 with Dx12 Settings"); ch.IsHeaderOpen())
            {
                if (bool dontUseNTShared = config->DontUseNTShared.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Don't Use NTShared", &dontUseNTShared))
                    config->DontUseNTShared = dontUseNTShared;
            }
        }

        if (state.api == Vulkan && currentFeature->IsWithDx12())
        {
            if (auto ch = ScopedCollapsingHeader("Vulkan with Dx12 Settings"); ch.IsHeaderOpen())
            {
                if (bool inputsUseCopy = config->VulkanUseCopyForInputs.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Use CopyResource for Inputs", &inputsUseCopy))
                    config->VulkanUseCopyForInputs = inputsUseCopy;

                if (bool outputUseCopy = config->VulkanUseCopyForOutput.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Use CopyResource for Output", &outputUseCopy))
                    config->VulkanUseCopyForOutput = outputUseCopy;
            }
        }

        // UPSCALER SPECIFIC -----------------------------

        // XeSS -----------------------------
        if (currentBackend == Upscaler::XeSS && !usesDlssd)
        {
            if (auto ch = ScopedCollapsingHeader("XeSS Settings"); ch.IsHeaderOpen())
            {
                const char* models[] = { "KPSS", "SPLAT", "MODEL_3", "MODEL_4", "MODEL_5", "MODEL_6" };
                auto configModes = config->NetworkModel.value_or_default();

                if (configModes < 0 || configModes > 5)
                    configModes = 0;

                const char* selectedModel = models[configModes];

                if (DlssNr::NeuralUi::BeginCombo("Network Models", selectedModel))
                {
                    for (int n = 0; n < 6; n++)
                    {
                        if (ImGui::Selectable(models[n], (config->NetworkModel.value_or_default() == n)))
                        {
                            config->NetworkModel = n;
                            state.newBackend = currentBackend;
                            MARK_ALL_BACKENDS_CHANGED();
                        }
                    }

                    ImGui::EndCombo();
                }
                ShowHelpMarker("Likely doesn't do much");

                if (bool dbg = state.xessDebug; DlssNr::NeuralUi::Checkbox("Dump (Shift+Del)", &dbg))
                    state.xessDebug = dbg;

                ImGui::SameLine(0.0f, 6.0f);
                int dbgCount = state.xessDebugFrames;

                ImGui::PushItemWidth(95.0f * menuResScale);
                if (ImGui::InputInt("frames", &dbgCount))
                {
                    if (dbgCount < 4)
                        dbgCount = 4;
                    else if (dbgCount > 999)
                        dbgCount = 999;

                    state.xessDebugFrames = dbgCount;
                }

                ImGui::PopItemWidth();
            }
        }

        // FFX -----------------
        if (!usesDlssd && (currentBackend == Upscaler::FFX || currentBackend == Upscaler::FFX_on12))
        {
            SectionHeader("FFX Settings");

            if (_ffxUpscalerIndex < 0)
                _ffxUpscalerIndex = config->FfxUpscalerIndex.value_or_default();

            if (currentBackend == Upscaler::FFX ||
                currentBackend == Upscaler::FFX_on12 && state.ffxUpscalerVersionNames.size() > 0)
            {
                auto currentName = StrFmt("FSR %s", state.ffxUpscalerVersionNames[_ffxUpscalerIndex]);
                if (DlssNr::NeuralUi::BeginCombo("FFX Upscaler", currentName.c_str()))
                {
                    for (int n = 0; n < state.ffxUpscalerVersionIds.size(); n++)
                    {
                        auto name = StrFmt("FSR %s##%d", state.ffxUpscalerVersionNames[n], n);
                        if (ImGui::Selectable(name.c_str(), config->FfxUpscalerIndex.value_or_default() == n))
                            _ffxUpscalerIndex = n;
                    }

                    ImGui::EndCombo();
                }

                ShowHelpMarker("List of upscalers reported by FFX SDK");

                ImGui::SameLine(0.0f, 6.0f);

                if (DlssNr::NeuralUi::Button("Change Upscaler") &&
                    _ffxUpscalerIndex != config->FfxUpscalerIndex.value_or_default())
                {
                    config->FfxUpscalerIndex = _ffxUpscalerIndex;
                    state.newBackend = currentBackend;
                    MARK_ALL_BACKENDS_CHANGED();
                }

                auto majorFsrVersion = currentFeature->Version().major;

                if (majorFsrVersion >= 4)
                {
                    ImGui::Spacing();

                    // Colorspaces
                    const char* colorSpaces[] = { "Linear (Default)", "Non-Linear", "Non-Linear sRGB",
                                                  "Non-Linear PQ" };
                    int currentColorSpace = 0;
                    if (config->FsrNonLinearPQ.value_or_default())
                        currentColorSpace = 3;
                    else if (config->FsrNonLinearSRGB.value_or_default())
                        currentColorSpace = 2;
                    else if (config->FsrNonLinearColorSpace.value_or_default())
                        currentColorSpace = 1;

                    if (DlssNr::NeuralUi::Combo("Input Color Space", &currentColorSpace, colorSpaces, IM_ARRAYSIZE(colorSpaces)))
                    {
                        bool isSrgb = (currentColorSpace == 2);
                        bool isPq = (currentColorSpace == 3);

                        config->FsrNonLinearSRGB = isSrgb;
                        config->FsrNonLinearPQ = isPq;

                        if (isSrgb || isPq)
                        {
                            config->FsrNonLinearColorSpace.set_volatile_value(true);
                        }
                        else if (currentColorSpace == 1) // Just non-Linear
                        {
                            config->FsrNonLinearColorSpace = true;
                        }
                        else // Linear
                        {
                            config->FsrNonLinearColorSpace = false;
                        }

                        state.newBackend = currentBackend;
                        MARK_ALL_BACKENDS_CHANGED();
                    }
                    ShowHelpMarker("Select the input color space that the game uses.\n"
                                   "Non-Linear / sRGB: Might improve FSR4 upscaling quality, might increase ghosting.\n"
                                   "PQ: Rarest, might increase ghosting and break lights.");

                    // FSR 4 Presets
                    const char* presets[] = { "Default",  "Preset 0", "Preset 1", "Preset 2",
                                              "Preset 3", "Preset 4", "Preset 5" };
                    int currentPresetIdx = config->Fsr4Preset.has_value() ? config->Fsr4Preset.value() + 1 : 0;

                    if (currentPresetIdx < 0 || currentPresetIdx >= IM_ARRAYSIZE(presets))
                        currentPresetIdx = 0;

                    if (DlssNr::NeuralUi::Combo("FSR4 Preset", &currentPresetIdx, presets, IM_ARRAYSIZE(presets)))
                    {
                        if (currentPresetIdx == 0)
                            config->Fsr4Preset.reset();
                        else
                            config->Fsr4Preset = currentPresetIdx - 1;

                        state.newBackend = currentBackend;
                        MARK_ALL_BACKENDS_CHANGED();
                    }
                    ShowHelpMarker("Each internal FSR4 preset is tuned for a specific resolution.\n"
                                   "Selecting an FSR4 preset won't change the in-game\nupscaler preset!!!\n\n"
                                   "Preset 0 is meant for FSR Native AA\n"
                                   "Preset 1 is meant for Quality/Ultra Quality\n"
                                   "Preset 2 is meant for Balanced\n"
                                   "Preset 3 is meant for Performance\n"
                                   "Preset 4 is meant for DRS\n"
                                   "Preset 5 is meant for Ultra Performance");

                    // Display the active preset right next to the combo box instead of using a table
                    ImGui::SameLine();
                    if (state.currentFsr4Preset.has_value())
                        ImGui::TextDisabled("(Active: %d)", state.currentFsr4Preset.value());
                    else if (FSR4ModelSelection::IsInt8FsrHooked())
                        ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), "(Potential FSR3 fallback)");
                    else
                        ImGui::TextDisabled("(Failed to hook)");
                }

                if (majorFsrVersion >= 3)
                {
                    ImGui::Spacing();

                    bool debugView = config->FsrDebugView.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("Upscaler Debug View", &debugView))
                    {
                        config->FsrDebugView = debugView;

                        // FSR 4's debug view requires backend reinit
                        if (majorFsrVersion > 3)
                        {
                            state.newBackend = currentBackend;
                            MARK_ALL_BACKENDS_CHANGED();
                        }
                    }

                    if (majorFsrVersion > 3)
                    {
                        ShowHelpMarker("Top left: Dilated Motion Vectors\n"
                                       "Top right: Predicted Blend Factor");
                    }
                    else
                    {
                        ShowHelpMarker("Top left: Dilated Motion Vectors\n"
                                       "Top middle: Protected Areas\n"
                                       "Top right: Dilated Depth\n"
                                       "Middle: Upscaled frame\n"
                                       "Bottom left: Disocclusion mask\n"
                                       "Bottom middle: Reactiveness\n"
                                       "Bottom right: Detail Protection Takedown");
                    }

                    if (majorFsrVersion > 3)
                    {
                        ImGui::SameLine(0.0f, 20.0f * menuResScale);
                        bool fsr4wm = config->Fsr4EnableWatermark.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Watermark", &fsr4wm))
                        {
                            LOG_DEBUG("FSR4 Watermark set to {}", fsr4wm);
                            config->Fsr4EnableWatermark = fsr4wm;
                        }

                        ShowHelpMarker("After changing this option, please Save Settings.\n"
                                       "It will be applied on next launch.");
                    }
                }

                if (currentFeature->Version() >= feature_version { 3, 1, 1 } &&
                    currentFeature->Version() < feature_version { 4, 0, 0 })
                {
                    ImGui::Spacing();

                    if (currentFeature != nullptr)
                    {
                        ImGui::Text("FSR 3.1 Presets:");

                        ImGui::SameLine(0.0f, 6.0f);

                        // This will be applied by default
                        if (DlssNr::NeuralUi::Button("Stability"))
                        {
                            auto const scaleRatioX =
                                (float) currentFeature->TargetWidth() / (float) currentFeature->RenderWidth();
                            auto const scaleRatioY =
                                (float) currentFeature->TargetHeight() / (float) currentFeature->RenderHeight();
                            auto const scaleRatio = std::max(scaleRatioX, scaleRatioY);

                            config->FsrVelocity = 0.5f;
                            config->FsrReactiveScale = 0.25f;

                            config->FsrShadingScale.reset();
                            config->FsrAccAddPerFrame.reset();
                            config->FsrMinDisOccAcc.reset();
                            config->FsrShadingScale.set_volatile_value(0.5f / scaleRatio);
                            config->FsrAccAddPerFrame.set_volatile_value(scaleRatio / 10.0f);
                            config->FsrMinDisOccAcc.set_volatile_value(scaleRatio / 20.0f);
                        }

                        ImGui::SameLine(0.0f, 6.0f);

                        if (DlssNr::NeuralUi::Button("Motion"))
                        {
                            auto const scaleRatioX =
                                (float) currentFeature->TargetWidth() / (float) currentFeature->RenderWidth();
                            auto const scaleRatioY =
                                (float) currentFeature->TargetHeight() / (float) currentFeature->RenderHeight();
                            auto const scaleRatio = std::max(scaleRatioX, scaleRatioY);

                            config->FsrVelocity = 1.0f;
                            config->FsrReactiveScale = 0.5f;

                            config->FsrShadingScale.reset();
                            config->FsrAccAddPerFrame.reset();
                            config->FsrMinDisOccAcc.reset();
                            config->FsrShadingScale.set_volatile_value(1.0f / scaleRatio);
                            config->FsrAccAddPerFrame.set_volatile_value(scaleRatio / 10.0f);
                            config->FsrMinDisOccAcc.set_volatile_value(scaleRatio / 20.0f);
                        }

                        ImGui::SameLine(0.0f, 6.0f);

                        if (DlssNr::NeuralUi::Button("Default"))
                        {
                            config->FsrVelocity = 1.0f;
                            config->FsrReactiveScale = 1.0f;
                            config->FsrShadingScale = 1.0f;
                            config->FsrAccAddPerFrame = 0.333f;
                            config->FsrMinDisOccAcc = -0.333f;
                        }
                    }

                    if (auto ch = ScopedCollapsingHeader("FSR 3 Upscaler Manual Tuning"); ch.IsHeaderOpen())
                    {

                        float velocity = config->FsrVelocity.value_or_default();
                        if (DlssNr::NeuralUi::FillSlider("Velocity Factor", &velocity, 0.00f, 1.0f, "%.2f"))
                            config->FsrVelocity = velocity;

                        ShowHelpMarker("Value of 0.0f can improve temporal stability of bright pixels\n"
                                       "Lower values are more stable with ghosting\n"
                                       "Higher values are more pixelly, but less ghosting");

                        if (currentFeature->Version() >= feature_version { 3, 1, 4 })
                        {
                            // Reactive Scale
                            float reactiveScale = config->FsrReactiveScale.value_or_default();
                            if (DlssNr::NeuralUi::FillSlider("Reactive Scale", &reactiveScale, 0.0f, 1.0f, "%.3f"))
                                config->FsrReactiveScale = reactiveScale;

                            ShowHelpMarker("Meant for development purpose to test if\n"
                                           "writing a larger value to reactive mask, reduces ghosting.");

                            // Shading Scale
                            float shadingScale = config->FsrShadingScale.value_or_default();
                            if (DlssNr::NeuralUi::FillSlider("Shading Scale", &shadingScale, 0.0f, 1.0f, "%.3f"))
                                config->FsrShadingScale = shadingScale;

                            ShowHelpMarker("Increasing this scales FSR3.1 computed shading\n"
                                           "change value at read to have higher reactiveness.");

                            // Accumulation Added Per Frame
                            float accAddPerFrame = config->FsrAccAddPerFrame.value_or_default();
                            if (DlssNr::NeuralUi::FillSlider("Acc. Added Per Frame", &accAddPerFrame, 0.0f, 1.0f, "%.3f"))
                                config->FsrAccAddPerFrame = accAddPerFrame;

                            ShowHelpMarker("Corresponds to amount of accumulation added per frame\n"
                                           "at pixel coordinate where disocclusion occured or when\n"
                                           "reactive mask value is > 0.0f. Decreasing this and \n"
                                           "drawing the ghosting object (IE no mv) to reactive mask \n"
                                           "with value close to 1.0f can decrease temporal ghosting.\n"
                                           "Decreasing this could result in more thin feature pixels flickering.");

                            // Min Disocclusion Accumulation
                            float minDisOccAcc = config->FsrMinDisOccAcc.value_or_default();
                            if (DlssNr::NeuralUi::FillSlider("Min. Disocclusion Acc.", &minDisOccAcc, -1.0f, 1.0f, "%.3f"))
                                config->FsrMinDisOccAcc = minDisOccAcc;

                            ShowHelpMarker("Increasing this value may reduce white pixel temporal\n"
                                           "flickering around swaying thin objects that are disoccluding \n"
                                           "one another often. Too high value may increase ghosting.");
                        }

                    }
                }
            }
        }

        // DLSS -----------------
        if ((config->DLSSEnabled.value_or_default() && currentBackend == Upscaler::DLSS &&
             currentFeature->Version().major > 2) ||
            usesDlssd)
        {

            if (usesDlssd)
                SectionHeader("DLSSD Settings");
            else
                SectionHeader("DLSS Settings");

            auto overridden =
                usesDlssd ? state.dlssdPresetsOverriddenExternally : state.dlssPresetsOverriddenExternally;

            if (overridden)
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), "Presets are overridden externally");
                ShowHelpMarker("This usually happens due to using tools\n"
                               "such as Nvidia App or Nvidia Inspector");
                // ImGui::Text("Selecting setting below will disable that external override\n"
                //             "but you need to Save Settings and restart the game");

                ImGui::Spacing();
            }

            if (usesDlssd)
            {
                if (bool pOverride = config->DLSSDRenderPresetOverride.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Render Presets Override", &pOverride))
                    config->DLSSDRenderPresetOverride = pOverride;

                ShowHelpMarker("Each render preset has it strengths and weaknesses\n"
                               "Override to potentially improve image quality\n"
                               "Press apply after enable/disable");

                /*
                auto currentPresetIndex = GetPresetIndex(currentFeature, true);

                if (currentPresetIndex == 0)
                    ImGui::Text("Current Preset: Default");
                else
                    ImGui::Text("Current Preset: %c", 64 + currentPresetIndex);
                */

                ImGui::BeginDisabled(!config->DLSSDRenderPresetOverride.value_or_default() /*|| overridden*/);

                AddDLSSDRenderPreset("Override Preset", &comboPreset);

                ImGui::EndDisabled();
            }
            else
            {
                if (bool pOverride = config->RenderPresetOverride.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Render Presets Override", &pOverride))
                    config->RenderPresetOverride = pOverride;

                ShowHelpMarker("Each render preset has it strengths and weaknesses\n"
                               "Override to potentially improve image quality\n"
                               "Press Apply after enable/disable");

                /*
                auto currentPresetIndex = GetPresetIndex(currentFeature, false);

                if (currentPresetIndex == 0)
                    ImGui::Text("Current Preset: Default");
                else
                    ImGui::Text("Current Preset: %c", 64 + currentPresetIndex);
                */

                ImGui::BeginDisabled(!config->RenderPresetOverride.value_or_default() /*|| overridden*/);

                AddDLSSRenderPreset("Override Preset", &comboPreset);

                ImGui::EndDisabled();
            }

            ImGui::SameLine(0.0f, 6.0f);

            if (DlssNr::NeuralUi::Button("Apply Changes"))
            {
                LOG_DEBUG("Applying DLSS/DLSSD preset override changes, preset index: {}",
                          comboPreset.value_or_default());

                if (usesDlssd)
                {
                    config->DLSSDRenderPresetForAll = comboPreset.value_or_default();
                    state.newBackend = Upscaler::DLSSD;
                }
                else
                {
                    config->RenderPresetForAll = comboPreset.value_or_default();
                    state.newBackend = currentBackend;
                }

                MARK_ALL_BACKENDS_CHANGED();
            }

            if (auto ch = ScopedCollapsingHeader(usesDlssd ? "Advanced DLSSD Settings" : "Advanced DLSS Settings");
                ch.IsHeaderOpen())
            {
                bool appIdOverride = config->UseGenericAppIdWithDlss.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Use Generic App Id with DLSS", &appIdOverride))
                    config->UseGenericAppIdWithDlss = appIdOverride;

                ShowHelpMarker("Use generic appid with NGX\n"
                               "Fixes OptiScaler preset override not working with certain games\n"
                               "Requires a game restart");

                ImGui::BeginDisabled(!config->RenderPresetOverride.value_or_default() || overridden);
                ImGui::Spacing();

                if (usesDlssd)
                {
                    AddDLSSDRenderPreset("DLAA Preset", &config->DLSSDRenderPresetDLAA);
                    AddDLSSDRenderPreset("UltraQ Preset", &config->DLSSDRenderPresetUltraQuality);
                    AddDLSSDRenderPreset("Quality Preset", &config->DLSSDRenderPresetQuality);
                    AddDLSSDRenderPreset("Balanced Preset", &config->DLSSDRenderPresetBalanced);
                    AddDLSSDRenderPreset("Perf Preset", &config->DLSSDRenderPresetPerformance);
                    AddDLSSDRenderPreset("UltraP Preset", &config->DLSSDRenderPresetUltraPerformance);
                }
                else
                {
                    AddDLSSRenderPreset("DLAA Preset", &config->RenderPresetDLAA);
                    AddDLSSRenderPreset("UltraQ Preset", &config->RenderPresetUltraQuality);
                    AddDLSSRenderPreset("Quality Preset", &config->RenderPresetQuality);
                    AddDLSSRenderPreset("Balanced Preset", &config->RenderPresetBalanced);
                    AddDLSSRenderPreset("Perf Preset", &config->RenderPresetPerformance);
                    AddDLSSRenderPreset("UltraP Preset", &config->RenderPresetUltraPerformance);
                }
                ImGui::EndDisabled();
            }
        }
    }
}

void MenuCommon::RenderFrameGenerationSelection(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    // (AMDNR 0.3.4, the approved other-tabs mock) "External frame generation / MFG unlocker" moved into the Compatibility
    // tree at the end of this tab (RenderFrameGenerationCompatibility); while it is in force this tab says so here.
    if (state.externalFrameGeneration)
    {
        ImGui::TextWrapped("External FG is active. Set the multiplier in the game or unlocker, not OptiScaler.");
        ImGui::TextDisabled("%s", "Compatibility, at the end of this tab, turns it off.");
        return;
    }
    auto& menuResScale = ctx.menuResScale;
    auto& primaryGpu = *ctx.primaryGpu;

    /// FG INPUTS

    static std::vector<MenuOption<FGInput>> inputOptions;
    inputOptions.clear();

    // clang-format off

    inputOptions = {
        { FGInput::NoFG, "None" },
        { FGInput::Upscaler, "OptiFG (Upscaler)",
            "Upscaler must be enabled\n\nCan be used with any FG Output, but might be imperfect with some\nTo prevent UI glitching, HUDfix required" },
        { FGInput::DLSSG, "DLSSG via Streamline",
            "Can be used with any FG Output\n\nRequires enabling DLSS-FG in game settings\nSupports HUDless out of the box\n\nLimited to games that use Streamline" },
        { FGInput::NvngxFG, "DLSSG via Nvngx",
            "Limited to variants of FSR FG\n\nRequires enabling DLSS-FG in game settings\nSupports HUDless out of the box\nUses Streamline swapchain for pacing" },
        { FGInput::FSRFG, "FSR 3.1 FG",
            "Can be used with any FG Output\n\nRequires enabling FSR-FG in game settings\nSupports HUDless out of the box" },
        { FGInput::FSRFG30, "FSR 3.0 FG",
            "Can be used with any FG Output\n\nRequires enabling FSR-FG in game settings\nSupports HUDless out of the box" },
        { FGInput::XeFG, "XeFG" }
    };

    // clang-format on

    auto constexpr nvngxInputIndex = (uint32_t) FGInput::NvngxFG;

    // XeFG input requirements
    auto constexpr xefgInputIndex = (uint32_t) FGInput::XeFG;
    inputOptions[xefgInputIndex].set_disabled(true, "Support not implemented, they meant FG Output");

    // OptiFG requirements
    auto constexpr optiFgIndex = (uint32_t) FGInput::Upscaler;
    inputOptions[optiFgIndex].set_disabled(state.swapchainApi == API::Vulkan, "Unsupported API");

    if (!inputOptions[optiFgIndex].disabled && state.activeFgOutput == FGOutput::FSRFG && !FfxApiProxy::IsFGReady() &&
        !ffxInitTried)
    {
        ffxInitTried = true;
        FfxApiProxy::InitFfxDx12();
        inputOptions[optiFgIndex].set_disabled(!FfxApiProxy::IsFGReady(), "amd_fidelityfx_dx12.dll is missing");
    }
    else if (!inputOptions[optiFgIndex].disabled && state.activeFgOutput == FGOutput::XeFG && !xefgInitTried &&
             XeFGProxy::Module() == nullptr)
    {
        xefgInitTried = true;
        XeFGProxy::InitXeFG();
        inputOptions[optiFgIndex].set_disabled(XeFGProxy::Module() == nullptr, "libxess_fg.dll is missing");
    }

    // DLSSG inputs requirements
    auto constexpr dlssgInputIndex = (uint32_t) FGInput::DLSSG;
    // inputOptions[dlssgInputIndex].set_disabled(state.streamlineVersion.major == 0, "Game doesn't use streamline");
    inputOptions[dlssgInputIndex].set_disabled(state.swapchainApi == API::DX11, "Unsupported API");

    // FSRFG inputs requirements
    auto constexpr fsrfgInputIndex = (uint32_t) FGInput::FSRFG;
    inputOptions[fsrfgInputIndex].set_disabled(state.swapchainApi != API::DX12, "Unsupported API");

    // FSRFG30 inputs requirements
    auto constexpr fsrfg30InputIndex = (uint32_t) FGInput::FSRFG30;
    inputOptions[fsrfg30InputIndex].set_disabled(state.swapchainApi != API::DX12, "Unsupported API");

    if (!config->FGInput.has_value())
        config->FGInput = config->FGInput.value_or_default(); // need to have a value before combo

    /// FG OUTPUTS

    static std::vector<MenuOption<FGOutput>> outputOptions;
    outputOptions.clear();

    // clang-format off

    outputOptions = {
        { FGOutput::NoFG, "None" },
        { FGOutput::FSRFG, "FSR FG", "FSR3/4-FG, RDNA4 autoupgrades to FSR4-FG\n\nFSR4-FG sometimes better/worse than XeFG" },
        { FGOutput::DLSSG, "DLSSG", "DLSSG output\ncan be used in conjuction with Nukem's for example" },
        { FGOutput::XeFG, "XeFG", "XeFG - heaviest, but best universal FG\n\nXeFG 3 overall deals best with HUD\n\nEnable UI Composition if HUD ghosting" },
    };

    // clang-format on

    // DLSSG output requirements
    auto constexpr dlssgOutputIndex = (uint32_t) FGOutput::DLSSG;
    const bool supportsDlssg = primaryGpu.nvidiaArchInfo.architecture_id >= NV_GPU_ARCHITECTURE_AD100;
    const bool hasDlssgReplacement =
        state.nukemsFgFileAvailable || state.artursFgFileAvailable || FfxApiProxy::IsFGReady(false);

    if (!supportsDlssg && hasDlssgReplacement)
    {
        outputOptions[dlssgOutputIndex].tooltip =
            "No real DLSSG, unsupported hardware\nOnly Nvngx FG replacements available";
    }

    outputOptions[dlssgOutputIndex].set_disabled(state.swapchainApi == API::Vulkan, "Unsupported API");
    outputOptions[dlssgOutputIndex].set_disabled(!supportsDlssg && !hasDlssgReplacement,
                                                 "Unsupported hardware and no replacements");

    // For that one case of DX11 DLSSG
    const auto streamlineVersion = state.streamlineVersion;
    const bool nukemsUnsupportedApi =
        state.swapchainApi == API::DX11 &&
        (streamlineVersion == feature_version { 0, 0, 0 } || streamlineVersion > feature_version { 2, 0, 1 });
    inputOptions[nvngxInputIndex].set_disabled(nukemsUnsupportedApi, "Unsupported API");

    // FSR FG output requirements
    auto constexpr fsrfgOutputIndex = (uint32_t) FGOutput::FSRFG;
    outputOptions[fsrfgOutputIndex].set_disabled(state.swapchainApi == API::Vulkan, "Unsupported API");

    // XeFG output requirements
    auto constexpr xefgOutputIndex = (uint32_t) FGOutput::XeFG;
    outputOptions[xefgOutputIndex].set_disabled(state.swapchainApi == API::Vulkan, "Unsupported API");
    // Unsupported FG input selected
    const auto currentInputIndex = (uint32_t) state.activeFgInput;
    if (config->FGInput != FGInput::NoFG && inputOptions.size() > currentInputIndex &&
        inputOptions[currentInputIndex].disabled && state.activeFgInput == config->FGInput)
    {
        LOG_WARN("Resetting FGInput to NoFG: {}", inputOptions[currentInputIndex].label);
        config->FGInput = FGInput::NoFG;

        // Changing active can be dangerous but we are talking about an unsupported mode
        // which shouldn't even actually have taken affect
        state.activeFgInput = FGInput::NoFG;
    }

    // Unsupported FG output selected
    const auto currentOutputIndex = (uint32_t) state.activeFgOutput;
    if (config->FGOutput != FGOutput::NoFG && outputOptions.size() > currentOutputIndex &&
        outputOptions[currentOutputIndex].disabled && state.activeFgOutput == config->FGOutput)
    {
        LOG_WARN("Resetting FGOutput to NoFG: {}", outputOptions[currentOutputIndex].label);
        config->FGOutput = FGOutput::NoFG;
        state.activeFgOutput = FGOutput::NoFG;
    }

    if (!config->FGOutput.has_value())
        config->FGOutput = config->FGOutput.value_or_default(); // need to have a value before combo

    /// FG NVNGX REPLACEMENT

    static std::vector<MenuOption<FGNvngxReplacement>> nvngxOptions;
    nvngxOptions.clear();

    // clang-format off

    nvngxOptions = {
        { FGNvngxReplacement::None, "None (Real DLSSG)", "Real DLSSG, For RTX 40xx and above"},
        { FGNvngxReplacement::Nukems, "Nukem's", "FSR 3 FG" },
        { FGNvngxReplacement::Arturs, "Enabler", "FSR 3 MFG" },
        { FGNvngxReplacement::FFX, "FSR 3/4 FG", "FSR 3/4 FG using the FFX" },
        { FGNvngxReplacement::Combo, "FFX + Enabler", "FFX for the middle fake frame, Enabler for the rest\n\n"
                                                      "2x - FFX\n3x - Enabler\n4x - FFX + Enabler\n5x - Enabler\n6x - FFX + Enabler" },
    };

    // clang-format on

    bool replaceFgOutputWithNvngx = false;
    bool showNvngxFgDowndown = false;

    if (config->FGInput == FGInput::NvngxFG)
    {
        config->FGOutput = FGOutput::NoFG;
        replaceFgOutputWithNvngx = true;
    }
    else if (config->FGOutput == FGOutput::DLSSG)
    {
        showNvngxFgDowndown = true;
    }

    auto constexpr fgNvngxNoneIndex = (uint32_t) FGNvngxReplacement::None;
    nvngxOptions[fgNvngxNoneIndex].set_disabled(!supportsDlssg, "Unsupported hardware");

    if (replaceFgOutputWithNvngx)
    {
        nvngxOptions[fgNvngxNoneIndex].label = "None";
        nvngxOptions[fgNvngxNoneIndex].set_hidden(true);
    }

    auto constexpr fgNvngxNukemsIndex = (uint32_t) FGNvngxReplacement::Nukems;
    nvngxOptions[fgNvngxNukemsIndex].set_disabled(!state.nukemsFgFileAvailable,
                                                  "Missing amdnr_dlssg_fsr3.dll (Nukem's dlssg_to_fsr3) in the OptiScaler folder");

    auto constexpr fgNvngxArtursIndex = (uint32_t) FGNvngxReplacement::Arturs;
    nvngxOptions[fgNvngxArtursIndex].set_disabled(!state.artursFgFileAvailable,
                                                  std::string("Missing dlss-enabler-headless.dll\n\n") + kEnablerHowTo);

    auto constexpr fgNvngxFfxIndex = (uint32_t) FGNvngxReplacement::FFX;
    nvngxOptions[fgNvngxFfxIndex].set_disabled(!FfxApiProxy::IsFGReady(false),
                                               "Missing amd_fidelityfx_framegeneration_dx12.dll");

    auto constexpr fgNvngxComboIndex = (uint32_t) FGNvngxReplacement::Combo;
    nvngxOptions[fgNvngxComboIndex].set_disabled(
        !FfxApiProxy::IsFGReady(false) || !state.artursFgFileAvailable,
        !state.artursFgFileAvailable
            ? std::string(FfxApiProxy::IsFGReady(false) ? "" : "Missing amd_fidelityfx_framegeneration_dx12.dll\n") +
                  "Missing dlss-enabler-headless.dll\n\n" + kEnablerHowTo
            : std::string("Missing amd_fidelityfx_framegeneration_dx12.dll"));

    // TODO: Automatically switch to any other option

    if (!config->FGNvngxReplacement.has_value())
        config->FGNvngxReplacement = config->FGNvngxReplacement.value_or_default(); // need to have a value before combo

    if (state.activeFgInput != FGInput::ForceXeLL)
    {
        // (AMDNR 0.3.4, the approved other-tabs mock) The tab opens with FG Input and FG Output, one per row and no
        // heading (0.3.3.2: a "Frame Generation" heading and a two-column table). Their help on the label.
        PopulateCombo("FG Input", config->FGInput, inputOptions);
        ShowTooltip("The data source to be used for FG\n"
                    "The native FG which the game supports");

        if (replaceFgOutputWithNvngx)
        {
            // Disable None?
            PopulateCombo("FG Nvngx", config->FGNvngxReplacement, nvngxOptions);
            ShowTooltip("What backend to use instead of the real DLSSG");
        }
        else
        {
            PopulateCombo("FG Output", config->FGOutput, outputOptions);
            ShowTooltip("The FG that you will actually be using");
            // Wrapped: the note is a few lines long. Through the menu's HDR tone map like its other orange lines.
            if (!State::Instance().fgNote.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, toneMapColor(ImVec4(1.f, 0.8f, 0.3f, 1.f)));
                ImGui::TextWrapped("%s", State::Instance().fgNote.c_str());
                ImGui::PopStyleColor();
            }
        }

        // Should be on a new line
        if (showNvngxFgDowndown)
        {
            PopulateCombo("FG Nvngx Replacement", config->FGNvngxReplacement, nvngxOptions);
            ShowTooltip("What backend to use instead of the real DLSSG");
        }

        // XeFG ceiling ([XeFG] MaxInterpolatedFrames). The unlock reads it once, when the
        // provider loads, so it sits here with the output choice and not in the XeFG block of
        // the runtime settings: that block only exists while an XeFG swapchain does, and after
        // a failed init there is none.
        if (config->FGOutput.value_or_default() == FGOutput::XeFG || state.activeFgOutput == FGOutput::XeFG)
        {
            // In interpolated frames: 3 = 4X, 5 = 6X (the default), 7 = 8X, 9 = 10X (the opt-in
            // ceiling, Config::XeFGMaxInterpolations; entries above it are skipped below).
            const char* ceilingModes[] = { "4X", "6X (default)", "8X", "10X (opt-in)" };
            const int ceilingValues[] = { 3, 5, 7, 9 };
            static_assert(IM_ARRAYSIZE(ceilingModes) == IM_ARRAYSIZE(ceilingValues));

            const int ceiling = config->FGXeFGMaxInterpolatedFrames.value_or_default();

            // An ini value that is not in the list keeps its own label.
            std::string ceilingLabel = std::to_string(ceiling + 1) + "X";
            for (int i = 0; i < IM_ARRAYSIZE(ceilingValues); i++)
            {
                if (ceilingValues[i] == ceiling)
                    ceilingLabel = ceilingModes[i];
            }

            if (DlssNr::NeuralUi::BeginCombo("XeFG ceiling (restart)", ceilingLabel.c_str()))
            {
                for (int i = 0; i < IM_ARRAYSIZE(ceilingValues); i++)
                {
                    if (ceilingValues[i] > Config::XeFGMaxInterpolations)
                        continue;

                    if (ImGui::Selectable(ceilingModes[i], ceilingValues[i] == ceiling))
                    {
                        config->FGXeFGMaxInterpolatedFrames = ceilingValues[i];
                        config->SaveXeFG();
                    }
                }

                ImGui::EndCombo();
            }

            ShowHelpMarker("The highest multiplier XeFG's MFG combo offers ([XeFG] MaxInterpolatedFrames).\n"
                           "Saved at once, applied when the game restarts. 6X is the default.\n"
                           "10X: opt-in, needs a 360 Hz+ display and a frame cap; +128 MiB at 4K.\n"
                           "Not yet confirmed in a game. Cap the frame rate at refresh / 10 (48 fps\n"
                           "at 480 Hz); latency is high. The provider reserves that VRAM (8X: +64 MiB) over 6X\n"
                           "whatever multiplier you then pick. Above 6X needs Extra pacing; a game's own\n"
                           "XeSS 3 menu stays at 6X. D3D12 games only.");

            const int running = XeFGUnlock::OursCeiling();

            // Wrapped: the menu's width follows the Menu Scale, and the longer notes are wider than it.
            ImGui::PushStyleColor(ImGuiCol_Text, toneMapColor(ImVec4(1.f, 0.8f, 0.3f, 1.f)));

            // A provider the game loaded first was unlocked as the game's own copy (6X at most),
            // even if OptiScaler's pacing then went onto it: that note comes first, and the
            // pacing note only shows while the pacing engine really is missing.
            if (XeFGUnlock::OursCappedAsGameCopy() && ceiling > XeFGUnlock::UnpacedMaxInterpolations)
                ImGui::TextWrapped("This session runs up to 6X: OptiScaler's XeFG provider is the game's own copy, "
                                   "loaded and unlocked for the game first ('XeFG unlock' in the log)");
            else if (XeFGUnlock::OursCappedWithoutPacing() && ceiling > XeFGUnlock::UnpacedMaxInterpolations)
                ImGui::TextWrapped("This session runs up to 6X: above 6X needs XeFG pacing on OptiScaler's "
                                   "1.3.1.78 provider ('XeFG unlock' in the log)");
            else if (running > 0 && running != ceiling)
                ImGui::TextWrapped("This session runs up to %dX; restart the game for %dX", running + 1, ceiling + 1);

            if (XeFGProxy::InitFailedAt() > XeFGUnlock::UnpacedMaxInterpolations + 1)
                ImGui::TextWrapped("XeFG could not start at %dX (error %d) and is off on that swapchain; "
                                   "6X is the way back",
                                   XeFGProxy::InitFailedAt(), XeFGProxy::InitFailedCode());

            ImGui::PopStyleColor();
        }

        // Try to avoid having None selected when the gpu doesn't support DLSSG + some fallbacks
        if (!supportsDlssg && (replaceFgOutputWithNvngx || showNvngxFgDowndown) &&
            config->FGNvngxReplacement.value_or_default() == FGNvngxReplacement::None)
        {
            if (state.nukemsFgFileAvailable)
                config->FGNvngxReplacement.set_volatile_value(FGNvngxReplacement::Nukems);

            else if (state.artursFgFileAvailable)
                config->FGNvngxReplacement.set_volatile_value(FGNvngxReplacement::Arturs);

            else if (FfxApiProxy::IsFGReady(false))
                config->FGNvngxReplacement.set_volatile_value(FGNvngxReplacement::FFX);
        }

        const bool nvngxFgChanged = (replaceFgOutputWithNvngx || showNvngxFgDowndown) &&
                                    state.activeFgNvngx != config->FGNvngxReplacement.value_or_default();
        state.fgSettingsChanged = state.activeFgOutput != config->FGOutput.value_or_default() ||
                                  state.activeFgInput != config->FGInput.value_or_default() || nvngxFgChanged;

        if (state.fgSettingsChanged)
        {
            ImGui::Spacing();
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.0f, 1.f)),
                               "Save Settings and restart to apply the changes");
            ImGui::Spacing();
        }

        const bool dlssgInputOrOutput =
            state.activeFgOutput == FGOutput::DLSSG || state.activeFgInput == FGInput::DLSSG;

        ImGui::BeginDisabled(state.dlssgGameDMFGSupported && config->FGDLSSGOverrideForceDMFG.value_or_default());
        if (state.dlssgMfgMax.has_value() && state.dlssgMfgMax.value() >= 1 && !dlssgInputOrOutput)
        {
            auto maxInterpolationCount = state.dlssgMfgMax.value();

            if (maxInterpolationCount >= 1)
            {
                const char* intModes[] = { "Default", "Off", "2X", "3X", "4X", "5X", "6X", "7X", "8X" };

                // Map config value to UI index
                int currentSet = 0;
                if (config->FGDLSSGOverrideInterpolationCount.has_value())
                {
                    currentSet = config->FGDLSSGOverrideInterpolationCount.value() + 1;
                }

                const char* currentIntCount = intModes[currentSet];

                if (DlssNr::NeuralUi::BeginCombo("Override DLSSG Ratio", currentIntCount))
                {
                    for (int i = 0; i <= maxInterpolationCount + 1; i++)
                    {
                        if (ImGui::Selectable(intModes[i], (currentSet == i)))
                        {
                            if (i == 0)
                            {
                                // Default, no override
                                config->FGDLSSGOverrideInterpolationCount.reset();
                            }
                            else
                            {
                                // UI index, store value
                                int framesToGenerate = i - 1;

                                LOG_DEBUG("DLSSG Interpolation Count set to: {}", framesToGenerate);
                                config->FGDLSSGOverrideInterpolationCount = framesToGenerate;
                            }

                            StreamlineHooks::updateDlssgOptions();
                        }
                    }

                    ImGui::EndCombo();
                }
            }
        }

        ImGui::EndDisabled();

        if (state.dlssgGameDMFGSupported && !dlssgInputOrOutput)
        {
            ImGui::SameLine(0.0f, 16.0f);

            if (bool dynamicMFG = config->FGDLSSGOverrideForceDMFG.value_or_default();
                DlssNr::NeuralUi::Checkbox("Force Dynamic MFG", &dynamicMFG))
            {
                config->FGDLSSGOverrideForceDMFG = dynamicMFG;
                StreamlineHooks::updateDlssgOptions();
            }

            ImGui::BeginDisabled(state.dlssgLastSetMode != sl::DLSSGMode::eDynamic);
            static float fpsTarget = config->FGDLSSGFramerateTargetDMFG.value_or_default();
            DlssNr::NeuralUi::FillSlider("DMFG FPS Target", &fpsTarget, 0, 200, "%.0f");

            ShowHelpMarker("An active limit of 0 means auto-detect the display refresh rate");

            if (DlssNr::NeuralUi::Button("Apply Target"))
            {
                config->FGDLSSGFramerateTargetDMFG = fpsTarget;
                StreamlineHooks::updateDlssgOptions();
            }

            ImGui::SameLine(0.0f, 16.0f);

            if (DlssNr::NeuralUi::Button("Reset Target"))
            {
                fpsTarget = 0.0f;
                config->FGDLSSGFramerateTargetDMFG.reset();
            }

            ImGui::EndDisabled();
        }

        auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);
        if (((state.activeFgOutput == FGOutput::FSRFG || state.activeFgOutput == FGOutput::XeFG ||
              state.activeFgOutput == FGOutput::DLSSG) &&
             state.activeFgInput != FGInput::NoFG && state.activeFgInput != FGInput::NvngxFG) &&
            fgOutput)
        {
            DlssNr::NeuralUi::Checkbox("Show Detected UI", &state.fgHudlessCompare);
            ShowHelpMarker("Needs HUDless texture to compare with final image.\n"
                           "UI elements and ONLY UI elements should have a pink tint!");

            const auto isUsingUIAny = fgOutput->IsUsingUIAny();

            ImGui::BeginDisabled(!isUsingUIAny);

            if (bool drawUIOverFG = config->FGDrawUIOverFG.value_or_default();
                DlssNr::NeuralUi::Checkbox("Draw UI over", &drawUIOverFG))
            {
                config->FGDrawUIOverFG = drawUIOverFG;
            }
            ShowHelpMarker("Draws UI resource over the final image\n"
                           "If no UI visible, enable this!");

            ImGui::EndDisabled();

            ImGui::BeginDisabled(!isUsingUIAny || !config->FGDrawUIOverFG.value_or_default());

            if (bool uiPremultipliedAlpha = config->FGUIPremultipliedAlpha.value_or_default();
                DlssNr::NeuralUi::Checkbox("UI Premult. alpha", &uiPremultipliedAlpha))
            {
                config->FGUIPremultipliedAlpha = uiPremultipliedAlpha;
            }
            ShowHelpMarker("If UI is too faint, disable this option");

            ImGui::EndDisabled();
        }

        const bool showOutputSpecificFGSettings = state.activeFgInput == FGInput::DLSSG ||
                                                  state.activeFgInput == FGInput::FSRFG ||
                                                  state.activeFgInput == FGInput::FSRFG30;

        const bool showHudCutoff = state.activeFgInput == FGInput::NvngxFG || state.activeFgOutput == FGOutput::FSRFG;

        if (showOutputSpecificFGSettings || showHudCutoff)
        {
            if (auto ch = ScopedCollapsingHeader("Advanced FG Settings"); ch.IsHeaderOpen())
            {
                if (showOutputSpecificFGSettings)
                {
                    auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);
                    if (fgOutput)
                    {
                        ImGui::BeginDisabled(!fgOutput->IsActive());

                        const auto isUsingUIAny = fgOutput->IsUsingUIAny();
                        const auto isUsingHudlessAny = fgOutput->IsUsingHudlessAny();

                        bool disableUI = config->FGDisableUI.value_or_default();
                        ImGui::BeginDisabled(!isUsingUIAny && !disableUI);

                        if (DlssNr::NeuralUi::Checkbox("Disable UI texture", &disableUI))
                        {
                            config->FGDisableUI = disableUI;
                            fgOutput->UpdateTarget();
                        }

                        ShowHelpMarker("For when the game sends a UI texture, but you want to disable it");

                        ImGui::EndDisabled();

                        bool disableHudless = config->FGDisableHudless.value_or_default();
                        ImGui::BeginDisabled(!isUsingHudlessAny && !disableHudless);

                        if (DlssNr::NeuralUi::Checkbox("Disable HUDless", &disableHudless))
                        {
                            config->FGDisableHudless = disableHudless;
                        }

                        ShowHelpMarker("For when the game sends HUDless, but you want to disable it");

                        ImGui::EndDisabled();

                        bool depthValidNow = config->FGDepthValidNow.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Depth as ValidNow", &depthValidNow))
                            config->FGDepthValidNow = depthValidNow;

                        ShowHelpMarker("Will use more VRAM, but Uniscaler needs this\n"
                                       "Maybe some other games might need too");

                        bool velocityValidNow = config->FGVelocityValidNow.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Velocity as ValidNow", &velocityValidNow))
                            config->FGVelocityValidNow = velocityValidNow;

                        ShowHelpMarker("Will use more VRAM, but Uniscaler needs this\n"
                                       "Maybe some other games might need too");

                        bool hudlessValidNow = config->FGHudlessValidNow.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("HUDless as ValidNow", &hudlessValidNow))
                            config->FGHudlessValidNow = hudlessValidNow;

                        ShowHelpMarker("Will use more VRAM, but some games might need this");

                        bool firstHudless = config->FGOnlyAcceptFirstHudless.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Accept First HUDless", &firstHudless))
                            config->FGOnlyAcceptFirstHudless = firstHudless;

                        ShowHelpMarker("If source tags more than one HUDless, only use the first one");

                        if (bool skipReset = config->FGSkipReset.value_or_default();
                            DlssNr::NeuralUi::Checkbox("Skip Reset", &skipReset))
                        {
                            config->FGSkipReset = skipReset;
                        }

                        ShowHelpMarker("Don't use reset signals from FG Inputs");

                        ImGui::EndDisabled();

                        ImGui::PushItemWidth(80.0f * menuResScale);

                        auto frameAhead = config->FGAllowedFrameAhead.value_or_default();
                        if (ImGui::InputInt("Frame Ahead", &frameAhead, 1, 1) && frameAhead > 0 && frameAhead < 4)
                        {
                            config->FGAllowedFrameAhead = frameAhead;
                        }

                        ShowHelpMarker("Number of frames the FG is allowed to be ahead of the game\n"
                                       "Might prevent FG on/off switching, but also might cause issues");

                        ImGui::PopItemWidth();

                        ImGui::SameLine(0.0f, 16.0f);

                        const char* ftSources[] = { "Input", "Opti", "Zero" };
                        const char* ftSourceInfos[] = { "Uses frametimes provided by\nDLSSG or FSR-FG ",
                                                        "Uses frametimes calculated by Opti",
                                                        "Let XeFG to handle frametimes" };

                        auto currentSet = (int) config->FTInput.value_or_default();
                        auto currentSourceCount = state.activeFgOutput == FGOutput::XeFG ? 3 : 2;

                        if (DlssNr::NeuralUi::BeginCombo("FT Input", ftSources[currentSet]))
                        {
                            for (size_t i = 0; i < currentSourceCount; i++)
                            {

                                if (ImGui::Selectable(ftSources[i], currentSet == i))
                                {
                                    LOG_DEBUG("FTInput has changed {} -> {}", ftSources[currentSet], ftSources[i]);
                                    config->FTInput = (FrameTimeSource) i;
                                }

                                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                                    ImGui::SetTooltip("%s", ftSourceInfos[i]);
                            }

                            ImGui::EndCombo();
                        }

                        ShowHelpMarker("Select source for frametime\n"
                                       "Might help frame pacing and stutter issues");
                    }
                }

                if (showHudCutoff)
                {
                    float fgHudCutoff = config->FGHudCutoff.value_or_default();
                    if (DlssNr::NeuralUi::FillSlider("Hud Cutoff", &fgHudCutoff, 0.00f, 1.0f, "%.2f"))
                        config->FGHudCutoff = fgHudCutoff;

                    ShowHelpMarker("Cutoffs transparency from UI to help with interpolation\n"
                                   "You can use Show Detected UI to see the difference\n0.0 is auto");
                }
            }
        }
    }
}

// Frame Gen > Compatibility (AMDNR 0.3.4, the approved other-tabs mock): the "External frame generation / MFG
// unlocker" box, which opened the tab until 0.3.4, at its end in a closed tree. The tree opens by itself while
// external FG is in force or a change waits for the restart, so the way back is in sight. Same key, same notes.
void MenuCommon::RenderFrameGenerationCompatibility(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    bool external = config->ExternalFrameGeneration.value_or_default();
    const bool pending = external != state.externalFrameGeneration;
    if (state.externalFrameGeneration || pending)
        ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
    if (!DlssNr::NeuralUi::TreeNode("Compatibility##fg_compat"))
        return;
    if (DlssNr::NeuralUi::Checkbox("External frame generation / MFG unlocker", &external))
        config->ExternalFrameGeneration = external;
    ShowHelpMarker("Leaves Streamline, Reflex and FG control to the game/external mod."
                   "\nNR and NGX upscaling remain available. Save Settings and restart."
                   "\nDoes not install an unlocker or enable FG in unsupported games.");
    DlssNr::NeuralUi::DimTag("next game start");
    if (external != state.externalFrameGeneration)
        ImGui::TextWrapped("Save Settings and restart to change frame-generation ownership.");
    ImGui::TreePop();
}

void MenuCommon::RenderFrameGenerationRuntimeSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;
    auto& primaryGpu = *ctx.primaryGpu;
    auto fgOutput = state.currentFG;

    // FSR FG controls
    if (state.activeFgOutput == FGOutput::FSRFG && state.activeFgInput != FGInput::NoFG &&
        state.currentFGSwapchain != nullptr)
    {
        if (state.activeFgInput != FGInput::Upscaler ||
            (currentFeature != nullptr && !currentFeature->IsFrozen()) && FfxApiProxy::IsFGReady())
        {
            SectionHeader("Frame Generation (FSR FG)");

            if (_ffxFGIndex < 0)
                _ffxFGIndex = config->FfxFGIndex.value_or_default();

            if (state.ffxFGVersionNames.size() > 0)
            {
                auto currentName = StrFmt("FSR %s", state.ffxFGVersionNames[_ffxFGIndex]);
                if (DlssNr::NeuralUi::BeginCombo("FFX FG", currentName.c_str()))
                {
                    for (int n = 0; n < state.ffxFGVersionIds.size(); n++)
                    {
                        auto name = StrFmt("FSR %s", state.ffxFGVersionNames[n]);
                        if (ImGui::Selectable(name.c_str(), config->FfxFGIndex.value_or_default() == n))
                            _ffxFGIndex = n;
                    }

                    ImGui::EndCombo();
                }

                ShowHelpMarker("List of FGs reported by FFX SDK");

                ImGui::SameLine(0.0f, 6.0f);

                if (DlssNr::NeuralUi::Button("Change FG") && _ffxFGIndex != config->FfxFGIndex.value_or_default())
                {
                    config->FfxFGIndex = _ffxFGIndex;
                    state.fgChanged = true;
                    state.scChanged = true;
                }
            }

            bool fgActive = config->FGEnabled.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Active##2", &fgActive))
            {
                config->FGEnabled = fgActive;
                LOG_DEBUG("FGEnabled set FGEnabled: {}", fgActive);

                if (config->FGEnabled.value_or_default())
                    state.fgChanged = true;
            }
            ShowHelpMarker("Enable Frame Generation");

            bool fgAsync = config->FGAsync.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Allow Async", &fgAsync))
            {
                config->FGAsync = fgAsync;

                if (config->FGEnabled.value_or_default())
                {
                    state.fgChanged = true;
                    state.scChanged = true;
                    LOG_DEBUG("Async set FGChanged");
                }
            }
            ShowHelpMarker("Enable Async for better FG performance\nMight cause crashes, especially with HUD Fix!");

            ImGui::SameLine(0.0f, 16.0f);

            bool fgDV = config->FGDebugView.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Debug View##2", &fgDV))
            {
                config->FGDebugView = fgDV;

                if (config->FGEnabled.value_or_default())
                {
                    state.fgChanged = true;
                    LOG_DEBUG("DebugView set FGChanged");
                }
            }
            ShowHelpMarker("Enable FSR3.1-FG Debug view\n\n"
                           "Top left: Game Motion Vectors\n"
                           "Top middle: GMV Depth\n"
                           "Top right: Optical Flow MV\n"
                           "Middle: Interpolated frame only\n"
                           "Bottom left: Disocclusion mask\n"
                           "Bottom middle: Interpolation source (w/o UI)\n"
                           "Bottom right: HUDless resource");

            ImGui::SameLine(0.0f, 16.0f);

            if (state.currentFG && state.currentFG->Version().major > 3)
            {
                if (bool fgwm = config->FSRFGEnableWatermark.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Enable Watermark", &fgwm))
                {
                    LOG_DEBUG("FSRFGEnableWatermark set FGWatermark: {}", fgwm);
                    config->FSRFGEnableWatermark = fgwm;
                }

                ShowHelpMarker("After changing this option, please Save Settings\n"
                               "It will be applied on next launch.");
            }

            if (auto ch = ScopedCollapsingHeader("Extended FSR FG Settings"); ch.IsHeaderOpen())
            {
                DlssNr::NeuralUi::Checkbox("FG Only Generated", &state.fgOnlyGenerated);
                ShowHelpMarker("Display only FSR 3.1 Generated frames");

                auto debugResetLines = config->FGDebugResetLines.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Debug Reset Lines", &debugResetLines))
                {
                    config->FGDebugResetLines = debugResetLines;
                    LOG_DEBUG("Enabled set FGDebugLines: {}", debugResetLines);
                }
                ShowHelpMarker("Enables drawing of Interpolation skip lines");

                auto debugTearLines = config->FGDebugTearLines.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Debug Tear Lines", &debugTearLines))
                {
                    config->FGDebugTearLines = debugTearLines;
                    LOG_DEBUG("Enabled set FGDebugLines: {}", debugTearLines);
                }
                ShowHelpMarker("Enables drawing of Tear and Interpolation skip lines");

                auto debugPacingLines = config->FGDebugPacingLines.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Debug Pacing Lines", &debugPacingLines))
                {
                    config->FGDebugPacingLines = debugPacingLines;
                    LOG_DEBUG("Enabled set FGDebugLines: {}", debugPacingLines);
                }
                ShowHelpMarker("Enables drawing of Pacing lines");

                ImGui::Spacing();
                if (DlssNr::NeuralUi::TreeNode("FG Rectangle Settings"))
                {
                    ImGui::PushItemWidth(95.0f * menuResScale);
                    int rectLeft = config->FGRectLeft.value_or(0);
                    if (ImGui::InputInt("Rect Left", &rectLeft))
                        config->FGRectLeft = rectLeft;

                    ImGui::SameLine(0.0f, 16.0f);
                    int rectTop = config->FGRectTop.value_or(0);
                    if (ImGui::InputInt("Rect Top", &rectTop))
                        config->FGRectTop = rectTop;

                    int rectWidth = config->FGRectWidth.value_or(0);
                    if (ImGui::InputInt("Rect Width", &rectWidth))
                        config->FGRectWidth = rectWidth;

                    ImGui::SameLine(0.0f, 16.0f);
                    int rectHeight = config->FGRectHeight.value_or(0);
                    if (ImGui::InputInt("Rect Height", &rectHeight))
                        config->FGRectHeight = rectHeight;

                    ImGui::PopItemWidth();
                    ShowHelpMarker("Frame generation rectangle, adjust for letterboxed content");

                    ImGui::BeginDisabled(!config->FGRectLeft.has_value() && !config->FGRectTop.has_value() &&
                                         !config->FGRectWidth.has_value() && !config->FGRectHeight.has_value());

                    if (DlssNr::NeuralUi::Button("Reset FG Rect"))
                    {
                        config->FGRectLeft.reset();
                        config->FGRectTop.reset();
                        config->FGRectWidth.reset();
                        config->FGRectHeight.reset();
                    }

                    ShowHelpMarker("Resets Frame generation rectangle");

                    ImGui::EndDisabled();
                    ImGui::TreePop();
                }

                auto fg = state.currentFG;
                if (fg != nullptr && strcmp(fg->Name(), "FSR-FG") == 0 &&
                    FfxApiProxy::VersionDx12_FG() >= feature_version { 3, 1, 3 })
                {
                    ImGui::Spacing();

                    if (DlssNr::NeuralUi::TreeNode("Frame Pacing Tuning"))
                    {
                        auto fptEnabled = config->FGFramePacingTuning.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Enable Tuning", &fptEnabled))
                        {
                            config->FGFramePacingTuning = fptEnabled;
                            state.fsrfgFramePaceTuningChanged = true;
                        }

                        ImGui::BeginDisabled(!config->FGFramePacingTuning.value_or_default());

                        ImGui::PushItemWidth(115.0f * menuResScale);
                        auto fptSafetyMargin = config->FGFPTSafetyMarginInMs.value_or_default();
                        if (ImGui::InputFloat("Safety Margins in ms", &fptSafetyMargin, 0.01f, 0.1f, "%.2f"))
                            config->FGFPTSafetyMarginInMs = fptSafetyMargin;
                        ShowHelpMarker("Safety margins in millisecons\n"
                                       "FSR default value: 0.1ms\n"
                                       "Opti default value: 0.01ms");

                        auto fptVarianceFactor = config->FGFPTVarianceFactor.value_or_default();
                        if (DlssNr::NeuralUi::FillSlider("Variance Factor", &fptVarianceFactor, 0.0f, 1.0f, "%.2f"))
                            config->FGFPTVarianceFactor = fptVarianceFactor;
                        ShowHelpMarker("Variance factor\n"
                                       "FSR default value: 0.1\n"
                                       "Opti default value: 0.3");
                        ImGui::PopItemWidth();

                        auto fpHybridSpin = config->FGFPTAllowHybridSpin.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Enable Hybrid Spin", &fpHybridSpin))
                            config->FGFPTAllowHybridSpin = fpHybridSpin;
                        ShowHelpMarker("Allows pacing spinlock to sleep, should reduce CPU usage\n"
                                       "Might cause slow ramp up of FPS");

                        auto fptHybridSpinTime = config->FGFPTHybridSpinTime.value_or_default();
                        if (DlssNr::NeuralUi::FillSlider("Hybrid Spin Time", &fptHybridSpinTime, 0, 100))
                            config->FGFPTHybridSpinTime = fptHybridSpinTime;
                        ShowHelpMarker("How long to spin if FPTHybridSpin is true. Measured in timer "
                                       "resolution units.\n"
                                       "Not recommended to go below 2. Will result in frequent overshoots");

                        auto fpWaitForSingleObjectOnFence =
                            config->FGFPTAllowWaitForSingleObjectOnFence.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Enable WaitForSingleObjectOnFence", &fpWaitForSingleObjectOnFence))
                        {
                            config->FGFPTAllowWaitForSingleObjectOnFence = fpWaitForSingleObjectOnFence;
                        }
                        ShowHelpMarker("Allows WaitForSingleObject instead of spinning for fence value");

                        if (DlssNr::NeuralUi::Button("Apply Timing Changes"))
                            state.fsrfgFramePaceTuningChanged = true;

                        ImGui::EndDisabled();
                        ImGui::TreePop();
                    }
                }
            }
        }
    }

    // XeFG controls
    if (state.activeFgOutput == FGOutput::XeFG && state.activeFgInput != FGInput::NoFG &&
        state.activeFgInput != FGInput::ForceXeLL && state.currentFGSwapchain != nullptr && XeFGProxy::InitXeFG() &&
        fgOutput)
    {
        SectionHeader("Frame Generation (XeFG)");

        if (XeFGUnlock::Applied())
            // What the provider reports, i.e. the ceiling actually written into OptiScaler's
            // copy (6X when the ini asked for more and XeFG pacing is not installed).
            ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "MFG unlock: active (up to %dX, %d provider cop%s)",
                               fgOutput->GetMaxInterpolationCount() + 1,
                               XeFGUnlock::UnlockedModules(), XeFGUnlock::UnlockedModules() == 1 ? "y" : "ies");
        else if (XeFGUnlock::Attempted())
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f),
                               "MFG unlock: not applied (see OptiScaler.log, 'XeFG unlock')");
        ShowHelpMarker("Multi-frame generation (3X up to 10X) on non-Intel cards is built into this build:\n"
                       "every XeSS-FG provider in the process is patched in memory when it loads - the copy\n"
                       "OptiScaler uses and a game's own copy (its native XeSS 3 menu, up to 6X there).\n"
                       "The ceiling is 'XeFG ceiling (restart)' under FG Output: 6X by default.\n"
                       "10X: opt-in, needs a 360 Hz+ display and a frame cap; +128 MiB at 4K.\n"
                       "XeFGUnlock.asi is not needed and is skipped by the plugin loader.\n"
                       "XeFG\\UnlockMFG=false turns this off.");

        bool extraPacing = config->FGXeFGExtraPacing.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Extra pacing (3X and above)", &extraPacing))
            config->FGXeFGExtraPacing = extraPacing;
        ShowHelpMarker("Spaces the generated frames of a burst (3X up to 10X) evenly through the provider's\n"
                       "own scheduler and stops the frame-time feedback loop that made high multipliers\n"
                       "laggy and made the frame rate drop periodically. Installed when the provider\n"
                       "loads, so a change takes effect on the next launch. A ceiling above 6X needs it:\n"
                       "without it the ceiling stays at 6X. XeFG\\ExtraPacing in the ini.");

        // AMDNR 0.3.3 (MFG stutter, experimental, off by default).
        bool paceOnSwapchain = config->FGXeFGPaceOnSwapchain.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Pace on swapchain (experimental)", &paceOnSwapchain))
            config->FGXeFGPaceOnSwapchain = paceOnSwapchain;
        ShowHelpMarker("For testing stutter at 4X and above: before each paced generated frame, wait (never\n"
                       "past that frame's deadline) until XeFG's swapchain has room, so a blocking present\n"
                       "is taken before the frame is due instead of bunching the frames after it.\n"
                       "Needs Extra pacing. Turning it on takes effect on the next launch; turning it off\n"
                       "applies at once. Off by default. XeFG\\PaceOnSwapchain in the ini.");

        bool ignoreChecks = config->FGXeFGIgnoreInitChecks.value_or_default();

        bool nativeAA = false;
        if (state.activeFgInput == FGInput::Upscaler && currentFeature != nullptr)
            nativeAA = currentFeature->RenderWidth() == currentFeature->DisplayWidth();

        // AMDNR: XeFG\AllowDilatedMV passes the motion-vector check alone (the same test as
        // XeFG_Dx12::Activate); the fullscreen and HDR checks below still apply. Off by default.
        bool allowDilatedMV = config->FGXeFGAllowDilatedMV.value_or_default();
        const bool mvsAccepted = fgOutput->IsLowResMV() || nativeAA ||
                                 (State::Instance().gameQuirks & GameQuirk::ForceFGRenderSizeMVs);
        const bool correctMVs = mvsAccepted || ignoreChecks || allowDilatedMV;

        if (!correctMVs || state.realExclusiveFullscreen)
        {
            config->FGEnabled.reset();
            config->FGXeFGDebugView.reset();
        }

        const bool restartNeeded = config->FGXeFGDepthInverted.value_or_default() != fgOutput->IsInvertedDepth() ||
                                   config->FGXeFGJitteredMV.value_or_default() != fgOutput->IsJitteredMVs() ||
                                   config->FGXeFGHighResMV.value_or_default() == fgOutput->IsLowResMV();

        bool cantActivate = false;
        if (restartNeeded)
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                               "Restart the game to apply correct XeFG settings!");
        }
        else
        {
            if (!correctMVs)
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)),
                                   "Requires disabling dilated motion vectors (or 'Allow dilated motion vectors' below)");

            // Shown while the game sends dilated MVs, and while it is on so it can be turned off.
            if (!mvsAccepted || allowDilatedMV)
            {
                if (DlssNr::NeuralUi::Checkbox("Allow dilated motion vectors", &allowDilatedMV))
                    config->FGXeFGAllowDilatedMV = allowDilatedMV;

                ShowHelpMarker("Lets XeFG run with the game's dilated / display-resolution motion vectors\n"
                               "(Intel's high-res MV mode). Only the motion-vector check is skipped;\n"
                               "fullscreen and HDR checks still apply.\n"
                               "Experimental: generated frames can show artefacts in some games.");
            }

            if (!ignoreChecks && state.realExclusiveFullscreen)
            {
                cantActivate = true;
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "Borderless display mode required!");
            }

            if (!ignoreChecks && state.isHdrActive)
            {
                if (state.currentSwapchainDesc.BufferDesc.Format >= DXGI_FORMAT_R32G32B32A32_TYPELESS &&
                    state.currentSwapchainDesc.BufferDesc.Format <= DXGI_FORMAT_R16G16B16A16_SINT)
                {
                    cantActivate = true;
                    ImGui::TextColored(toneMapColor(ImVec4(1.0f, 0.0f, 0.0f, 1.f)), "XeFG only supports HDR10");
                }
            }
        }

        if (!correctMVs || cantActivate || ignoreChecks)
        {
            if (DlssNr::NeuralUi::Checkbox("Ignore Init Checks", &ignoreChecks))
                config->FGXeFGIgnoreInitChecks = ignoreChecks;

            ShowHelpMarker("Ignores all prechecks for XeFG\n"
                           "Don't use this option to skip MV size warning for UE games!\n"
                           "It might cause crashes and bad IQ!");
        }

        ImGui::BeginDisabled(!correctMVs || cantActivate);

        bool fgActive = config->FGEnabled.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Active##3", &fgActive))
        {
            config->FGEnabled = fgActive;
            LOG_DEBUG("Enabled set FGEnabled: {}", fgActive);

            if (config->FGEnabled.value_or_default())
                state.fgChanged = true;
        }

        ShowHelpMarker("Enable Frame Generation");

        auto maxInterpolationCount = fgOutput->GetMaxInterpolationCount();

        if (maxInterpolationCount > 1)
        {
            ImGui::SameLine(0.0f, 16.0f);

            const char* intModes[] = { "2X", "3X", "4X", "5X", "6X", "7X", "8X", "9X", "10X" };
            static_assert(IM_ARRAYSIZE(intModes) == Config::XeFGMaxInterpolations,
                          "one MFG entry per interpolation count up to the XeFG ceiling");
            auto currentSet =
                std::clamp((int) fgOutput->GetInterpolatedFrameCount() - 1, 0, (int) IM_ARRAYSIZE(intModes) - 1);
            auto currentIntCount = intModes[currentSet];

            ImGui::PushItemWidth(95.0f * menuResScale);

            if (DlssNr::NeuralUi::BeginCombo("MFG", currentIntCount))
            {
                for (int i = 0; i < maxInterpolationCount && i < IM_ARRAYSIZE(intModes); i++)
                {
                    if (ImGui::Selectable(intModes[i], (currentSet == i)))
                    {
                        LOG_DEBUG("XeFG Interpolation Count set to: {}", i + 1);
                        state.fgChanged = true;
                        config->FGXeFGInterpolationCount = i + 1;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();

            ShowHelpMarker("Set XeFG interpolation count");
        }

        ImGui::SameLine(0.0f, 16.0f);
        ImGui::BeginDisabled(!fgOutput->IsUsingHudlessAny() || XeFGProxy::SetUiCompositionState() == nullptr);
        bool fgCompositeUI = config->FGXeFGUIComposition.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("UI Composition", &fgCompositeUI))
            config->FGXeFGUIComposition = fgCompositeUI;

        ShowHelpMarker("Disable HUD/UI interpolation\n"
                       "Reverts back to previous XeFG 2 behaviour\n\n"
                       "Fixes artifacting transparent HUD/UI");
        ImGui::EndDisabled();

        bool fgDV = config->FGXeFGDebugView.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Debug View##2", &fgDV))
        {
            config->FGXeFGDebugView = fgDV;

            if (config->FGXeFGDebugView.value_or_default())
            {
                state.fgChanged = true;
                LOG_DEBUG("DebugView set FGChanged");
            }
        }
        ShowHelpMarker("Enable XeFG Debug view");

        ImGui::EndDisabled();

        ImGui::SameLine(0.0f, 16.0f);
        bool fgBorderless = config->FGXeFGForceBorderless.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Force Borderless", &fgBorderless))
            config->FGXeFGForceBorderless = fgBorderless;

        ShowHelpMarker("Forces Borderless display mode\n\n"
                       "For best results, set fullscreen \n"
                       "resolution to your display resolution\n"
                       "Might cause some instability issues.\n\n"
                       "NEEDS GAME RESTART TO BE ACTIVE!");

        // Disable this for now
        // ImGui::SameLine(0.0f, 16.0f);
        // DlssNr::NeuralUi::Checkbox("Only Generated##2", &state.fgOnlyGenerated);
        // ShowHelpMarker("Display only XeFG generated frames");

        if (auto ch = ScopedCollapsingHeader("Extended XeFG Settings"); ch.IsHeaderOpen())
        {
            ImGui::Spacing();
            if (DlssNr::NeuralUi::TreeNode("Rectangle Settings"))
            {
                ImGui::PushItemWidth(95.0f * menuResScale);
                int rectLeft = config->FGRectLeft.value_or(0);
                if (ImGui::InputInt("Rect Left##2", &rectLeft))
                    config->FGRectLeft = rectLeft;

                ImGui::SameLine(0.0f, 16.0f);
                int rectTop = config->FGRectTop.value_or(0);
                if (ImGui::InputInt("Rect Top##2", &rectTop))
                    config->FGRectTop = rectTop;

                int rectWidth = config->FGRectWidth.value_or(0);
                if (ImGui::InputInt("Rect Width##2", &rectWidth))
                    config->FGRectWidth = rectWidth;

                ImGui::SameLine(0.0f, 16.0f);
                int rectHeight = config->FGRectHeight.value_or(0);
                if (ImGui::InputInt("Rect Height##2", &rectHeight))
                    config->FGRectHeight = rectHeight;

                ImGui::PopItemWidth();
                ShowHelpMarker("Frame generation rectangle, adjust for letterboxed content##2");

                ImGui::BeginDisabled(!config->FGRectLeft.has_value() && !config->FGRectTop.has_value() &&
                                     !config->FGRectWidth.has_value() && !config->FGRectHeight.has_value());

                if (DlssNr::NeuralUi::Button("Reset FG Rect##2"))
                {
                    config->FGRectLeft.reset();
                    config->FGRectTop.reset();
                    config->FGRectWidth.reset();
                    config->FGRectHeight.reset();
                }

                ShowHelpMarker("Resets Frame generation rectangle##2");

                ImGui::EndDisabled();
                ImGui::TreePop();
            }
        }
    }

    // DLSSG controls
    if (state.activeFgOutput == FGOutput::DLSSG && state.activeFgInput != FGInput::NoFG &&
        state.currentFGSwapchain != nullptr && StreamlineProxy::LoadStreamline() && fgOutput)
    {
        SectionHeader("Frame Generation (DLSSG)");

        if (state.activeFgNvngx == FGNvngxReplacement::None && state.isHdrActive)
        {
            if (state.currentSwapchainDesc.BufferDesc.Format >= DXGI_FORMAT_R32G32B32A32_TYPELESS &&
                state.currentSwapchainDesc.BufferDesc.Format <= DXGI_FORMAT_R16G16B16A16_SINT)
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.0f, 0.0f, 0.0f, 1.f)), "DLSSG only supports HDR10");
            }
        }

        ImGui::Text("Current DLSSG state:");
        ImGui::SameLine();
        if (auto count = state.dlssgDetectedInterpolationCount; count > 0)
        {
            ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)), std::format("ON {}x", count + 1).c_str());
        }
        else
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "OFF");
        }

        bool fgActive = config->FGEnabled.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Active##4", &fgActive))
        {
            config->FGEnabled = fgActive;
            LOG_DEBUG("Enabled set FGEnabled: {}", fgActive);

            if (config->FGEnabled.value_or_default())
                state.fgChanged = true;
        }

        ShowHelpMarker("Enable Frame Generation");

        auto maxInterpolationCount = fgOutput->GetMaxInterpolationCount();

        if (maxInterpolationCount > 1)
        {
            ImGui::SameLine(0.0f, 16.0f);

            ImGui::BeginDisabled(config->FGDLSSGForceDMFG.value_or_default());

            const char* intModes[] = { "2X", "3X", "4X", "5X", "6X", "7X", "8X" };
            auto currentSet = std::clamp((int) fgOutput->GetInterpolatedFrameCount() - 1, 0, 6);
            auto currentIntCount = intModes[currentSet];

            ImGui::PushItemWidth(95.0f * menuResScale);

            if (DlssNr::NeuralUi::BeginCombo("MFG", currentIntCount))
            {
                for (int i = 0; i < maxInterpolationCount && i < 7; i++)
                {
                    if (ImGui::Selectable(intModes[i], (currentSet == i)))
                    {
                        LOG_DEBUG("DLSSG Interpolation Count set to: {}", i + 1);
                        config->FGDLSSGInterpolationCount = i + 1;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();

            ShowHelpMarker("Set DLSSG interpolation count");

            ImGui::EndDisabled();

            if (fgOutput->GetDMFGSupport())
            {
                ImGui::SameLine(0.0f, 16.0f);

                if (bool dynamicMFG = config->FGDLSSGForceDMFG.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Force Dynamic MFG", &dynamicMFG))
                {
                    config->FGDLSSGForceDMFG = dynamicMFG;
                }

                ImGui::BeginDisabled(!config->FGDLSSGForceDMFG.value_or_default());
                static float fpsTarget = config->FGDLSSGFramerateTargetDMFG.value_or_default();
                DlssNr::NeuralUi::FillSlider("DMFG FPS Target", &fpsTarget, 0, 200, "%.0f");

                ShowHelpMarker("An active limit of 0 means auto-detect the display refresh rate");

                if (DlssNr::NeuralUi::Button("Apply Target"))
                {
                    config->FGDLSSGFramerateTargetDMFG = fpsTarget;
                }

                ImGui::SameLine(0.0f, 16.0f);

                if (DlssNr::NeuralUi::Button("Reset Target"))
                {
                    fpsTarget = 0.0f;
                    config->FGDLSSGFramerateTargetDMFG.reset();
                }

                ImGui::EndDisabled();
            }
        }

        bool useGamesMarkers = config->FGDLSSGUseGamesReflexMarkers.value_or_default();
        ImGui::BeginDisabled(!ReflexHooks::gameIsSendingMarkers());
        if (DlssNr::NeuralUi::Checkbox("Use Game's Reflex Markers", &useGamesMarkers))
        {
            config->FGDLSSGUseGamesReflexMarkers = useGamesMarkers;
            LOG_DEBUG("Changed set FGDLSSGUseGamesReflexMarkers: {}", useGamesMarkers);
        }
        ImGui::EndDisabled();
    }

    // OptiFG
    if (state.api != API::Vulkan && state.currentFGSwapchain != nullptr && state.activeFgInput == FGInput::Upscaler)
    {
        SeparatorWithHelpMarker("Frame Generation (OptiFG)", "Using upscaler data for FG");

        if (currentFeature != nullptr && !currentFeature->IsFrozen() &&
            ((state.activeFgOutput == FGOutput::FSRFG && FfxApiProxy::IsFGReady()) ||
             (state.activeFgOutput == FGOutput::XeFG && XeFGProxy::Module() != nullptr) ||
             (state.activeFgOutput == FGOutput::DLSSG && StreamlineProxy::Module() != nullptr)))
        {
            if (!Config::Instance()->FGDisableHUDFix.value_or_default() &&
                state.swapchainInteropApi == SwapchainInteropApi::None)
            {
                bool fgHudfix = config->FGHUDFix.value_or_default();

                if (DlssNr::NeuralUi::Checkbox("HUDFix", &fgHudfix))
                {
                    config->FGHUDFix = fgHudfix;
                    LOG_DEBUG("Enabled set FGHUDFix: {}", fgHudfix);
                    state.clearCapturedHudlesses = true;
                    state.fgChanged = true;
                }

                ShowHelpMarker("Enable HUD stability fix, might cause crashes!");

                ImGui::BeginDisabled(!config->FGHUDFix.value_or_default());

                ImGui::SameLine(0.0f, 16.0f);
                ImGui::PushItemWidth(95.0f * menuResScale);
                int hudFixLimit = config->FGHUDLimit.value_or_default();
                if (ImGui::InputInt("Limit", &hudFixLimit))
                {
                    if (hudFixLimit < 1)
                        hudFixLimit = 1;
                    else if (hudFixLimit > 999)
                        hudFixLimit = 999;

                    config->FGHUDLimit = hudFixLimit;
                    LOG_DEBUG("Enabled set FGHUDLimit: {}", hudFixLimit);
                }
                ShowHelpMarker("Delay HUDless capture, high values might cause crash!");

                ImGui::SameLine(0.0f, 16.0f);
                if (DlssNr::NeuralUi::Button("Res##2"))
                    _showHudlessWindow = !_showHudlessWindow;

                ImGui::EndDisabled();

                auto hudExtended = config->FGHUDFixExtended.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Extended", &hudExtended))
                {
                    LOG_DEBUG("Enabled set FGHUDFixExtended: {}", hudExtended);
                    config->FGHUDFixExtended = hudExtended;
                }
                ShowHelpMarker("Extended format checks for possible HUDless\nMight cause crashes and slowdowns!");

                ImGui::BeginDisabled(!config->FGHUDFix.value_or_default());

                auto immediate = config->FGImmediateCapture.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Immediate Capture", &immediate))
                {
                    LOG_DEBUG("Enabled set FGImmediateCapture: {}", immediate);
                    config->FGImmediateCapture = immediate;
                }
                ShowHelpMarker("Enables capturing of resources before shader execution.\nIncrease HUDless "
                               "capture chances, but might cause capturing of unnecessary resources.");

                ImGui::PopItemWidth();

                ImGui::EndDisabled();
            }

            bool depthScale = config->FGEnableDepthScale.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Scale Depth to fix DLSS RR", &depthScale))
                config->FGEnableDepthScale = depthScale;
            ShowHelpMarker("Fix for DLSS-D wrong depth inputs");

            bool resourceFlip = config->FGResourceFlip.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Flip (Unity)", &resourceFlip))
                config->FGResourceFlip = resourceFlip;
            ShowHelpMarker("Flip Velocity & Depth resources of Unity games");

            bool resourceFlipOffset = config->FGResourceFlipOffset.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Flip Use Offset", &resourceFlipOffset))
                config->FGResourceFlipOffset = resourceFlipOffset;
            ShowHelpMarker("Use height difference as offset");

            if (auto ch = ScopedCollapsingHeader("Advanced OptiFG Settings"); ch.IsHeaderOpen())
            {
                if (!Config::Instance()->FGDisableHUDFix.value_or_default() &&
                    state.swapchainInteropApi == SwapchainInteropApi::None)
                {
                    ImGui::Spacing();

                    auto rb = config->FGResourceBlocking.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("Resource Blocking", &rb))
                    {
                        config->FGResourceBlocking = rb;
                        LOG_DEBUG("Enabled set FGResourceBlocking: {}", rb);
                    }
                    ShowHelpMarker("Block rarely used resources from using as HUDless \n"
                                   "to prevent flickers and other issues\n\n"
                                   "HUDfix enable/disable will reset the block list!");

                    auto rrc = config->FGRelaxedResolutionCheck.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("Relaxed Resource Check", &rrc))
                    {
                        config->FGRelaxedResolutionCheck = rrc;
                        LOG_DEBUG("Enabled set FGRelaxedResolutionCheck: {}", rrc);
                    }
                    ShowHelpMarker("Relax resolution checks for HUDless by 32 pixels \n"
                                   "Helps games which use black borders for some \n"
                                   "resolutions and screen ratios (e.g. Witcher 3)");

                    ImGui::BeginDisabled(state.fgResetCapturedResources);
                    ImGui::PushItemWidth(95.0f * menuResScale);
                    if (DlssNr::NeuralUi::Checkbox("FG Create List", &state.fgCaptureResources))
                    {
                        if (!state.fgCaptureResources)
                            config->FGHUDLimit = 1;
                        else
                            state.fgOnlyUseCapturedResources = false;
                    }

                    if (DlssNr::NeuralUi::Checkbox("FG Use List", &state.fgOnlyUseCapturedResources))
                    {
                        if (state.fgCaptureResources)
                        {
                            state.fgCaptureResources = false;
                            config->FGHUDLimit = 1;
                        }
                    }

                    ImGui::SameLine(0.0f, 8.0f);
                    ImGui::Text("(%d)", state.fgCapturedResourceCount);

                    ImGui::PopItemWidth();

                    ImGui::SameLine(0.0f, 16.0f);

                    if (DlssNr::NeuralUi::Button("Reset List"))
                    {
                        LOG_DEBUG("Resetting captured resource list");

                        state.fgResetCapturedResources = true;
                        state.fgOnlyUseCapturedResources = false;
                    }

                    ImGui::EndDisabled();

                    ImGui::Spacing();
                    ImGui::Spacing();
                    if (DlssNr::NeuralUi::TreeNode("Tracking Settings"))
                    {
                        auto ath = config->FGAlwaysTrackHeaps.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Always Track Heaps", &ath))
                        {
                            config->FGAlwaysTrackHeaps = ath;
                            LOG_DEBUG("Enabled set FGAlwaysTrackHeaps: {}", ath);
                        }
                        ShowHelpMarker("Always track resources, might cause performance issues\n, but also might "
                                       "fix HUDFix related crashes!");

                        auto disableRTV = config->FGHudfixDisableRTV.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable RTV Tracking", &disableRTV))
                            config->FGHudfixDisableRTV = disableRTV;
                        ShowHelpMarker("Disable tracking of CreateRenderTargetView\n"
                                       "This might help filtering of wrong HUDless resources");

                        auto disableSRV = config->FGHudfixDisableSRV.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable SRV Tracking", &disableSRV))
                            config->FGHudfixDisableSRV = disableSRV;
                        ShowHelpMarker("Disable tracking of CreateShaderResourceView\n"
                                       "This might help filtering of wrong HUDless resources");

                        auto disableUAV = config->FGHudfixDisableUAV.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable UAV Tracking", &disableUAV))
                            config->FGHudfixDisableUAV = disableUAV;
                        ShowHelpMarker("Disable tracking of CreateUnorderedAccessView\n"
                                       "This might help filtering of wrong HUDless resources");

                        auto disableOM = config->FGHudfixDisableOM.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable OM Tracking", &disableOM))
                            config->FGHudfixDisableOM = disableOM;
                        ShowHelpMarker("Disable tracking of OMSetRenderTargets\n"
                                       "This might help filtering of wrong HUDless resources");

                        auto disableSCR = config->FGHudfixDisableSCR.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable SCR Tracking", &disableSCR))
                            config->FGHudfixDisableSCR = disableSCR;
                        ShowHelpMarker("Disable tracking of SetComputeRootDescriptorTable\n"
                                       "This might help filtering of wrong HUDless resources");

                        auto disableSGR = config->FGHudfixDisableSGR.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable SGR Tracking", &disableSGR))
                            config->FGHudfixDisableSGR = disableSGR;
                        ShowHelpMarker("Disable tracking of SetGraphicsRootDescriptorTable\n"
                                       "This might help filtering of wrong HUDless resources");

                        ImGui::Spacing();

                        auto disableDI = config->FGHudfixDisableDI.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable DI Tracking", &disableDI))
                            config->FGHudfixDisableDI = disableDI;
                        ShowHelpMarker("Disable tracking of DrawInstanced\n"
                                       "This might help filtering of wrong HUDless resources");

                        auto disableDII = config->FGHudfixDisableDII.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable DII Tracking", &disableDII))
                            config->FGHudfixDisableDII = disableDII;
                        ShowHelpMarker("Disable tracking of DrawIndexedInstanced\n"
                                       "This might help filtering of wrong HUDless resources");

                        auto disableDispatch = config->FGHudfixDisableDispatch.value_or_default();
                        if (DlssNr::NeuralUi::Checkbox("Disable Dispatch Tracking", &disableDispatch))
                            config->FGHudfixDisableDispatch = disableDispatch;
                        ShowHelpMarker("Disable tracking of Dispatch\n"
                                       "This might help filtering of wrong HUDless resources");

                        ImGui::TreePop();
                    }
                }

                ImGui::Spacing();
                if (DlssNr::NeuralUi::TreeNode("Resource Settings"))
                {
                    bool makeMVCopies = config->FGMakeMVCopy.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("FG Make MV Copies", &makeMVCopies))
                        config->FGMakeMVCopy = makeMVCopies;
                    ShowHelpMarker("Make a copy of motion vectors to use with OptiFG\n"
                                   "For preventing corruptions that might happen");

                    bool makeDepthCopies = config->FGMakeDepthCopy.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("FG Make Depth Copies", &makeDepthCopies))
                        config->FGMakeDepthCopy = makeDepthCopies;
                    ShowHelpMarker("Make a copy of depth to use with OptiFG\n"
                                   "For preventing corruptions that might happen");

                    ImGui::PushItemWidth(115.0f * menuResScale);
                    float depthScaleMax = config->FGDepthScaleMax.value_or_default();
                    if (ImGui::InputFloat("FG Scale Depth Max", &depthScaleMax, 10.0f, 100.0f, "%.1f"))
                        config->FGDepthScaleMax = depthScaleMax;
                    ShowHelpMarker("Depth values will be divided to this value");
                    ImGui::PopItemWidth();

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                if (DlssNr::NeuralUi::TreeNode("Syncing Settings"))
                {
                    bool useMutexForPresent = config->FGUseMutexForSwapchain.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("FG Use Mutex for Present", &useMutexForPresent))
                        config->FGUseMutexForSwapchain = useMutexForPresent;
                    ShowHelpMarker("Use mutex to prevent desync of FG and crashes\n"
                                   "Disabling might improve the perf but decrease stability");

                    ImGui::TreePop();
                }
            }
        }
        else if (currentFeature == nullptr || currentFeature->IsFrozen())
        {
            ImGui::Text("Upscaler is not active"); // Probably never will be visible
        }
        else if (state.activeFgOutput == FGOutput::FSRFG && !FfxApiProxy::IsFGReady())
        {
            ImGui::TextColored(toneMapColor({ 1.0f, 0.0f, 0.0f, 1.0f }),
                               "amd_fidelityfx_dx12.dll is missing!"); // Probably never will be visible
        }
        else if (state.activeFgOutput == FGOutput::XeFG && XeFGProxy::Module() == nullptr)
        {
            ImGui::TextColored(toneMapColor({ 1.0f, 0.0f, 0.0f, 1.0f }),
                               "libxess_fg.dll is missing!"); // Probably never will be visible
        }
    }

    const FGNvngxReplacement activeNvngxFg = state.activeFgNvngx;
    if (activeNvngxFg != FGNvngxReplacement::None)
    {
        if (activeNvngxFg == FGNvngxReplacement::Nukems)
        {
            SeparatorWithHelpMarker("Frame Generation (FSR3-FG via Nukem's DLSSG)",
                                    "Requires Nukem's dlssg_to_fsr3 dll");

            if (!state.nukemsFgFileAvailable)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)));
                ImGui::TextWrapped("Please put amdnr_dlssg_fsr3.dll (from the zip; Nukem's "
                                   "dlssg_to_fsr3_amd_is_better.dll works too) into the OptiScaler folder");
                ImGui::PopStyleColor();
            }
        }
        else if (activeNvngxFg == FGNvngxReplacement::Arturs)
        {
            SeparatorWithHelpMarker("Frame Generation (FSR3-MFG via DLSS Enabler)",
                                    "DLSS Enabler 4.9.0 or newer by Artur Graniszewski, its version.dll\n"
                                    "renamed to dlss-enabler-headless.dll (not included with AMDNR)");

            if (!state.artursFgFileAvailable)
                RenderEnablerMissing();

            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                               "Using a subset of features from DLSS Enabler");
        }
        else if (activeNvngxFg == FGNvngxReplacement::FFX)
        {
            SeparatorWithHelpMarker("Frame Generation (FSRFG via FFX)", "FFX using the DLSSG swapchain");
        }
        else if (activeNvngxFg == FGNvngxReplacement::Combo)
        {
            SeparatorWithHelpMarker("Frame Generation (Enabler + FFX)",
                                    "FFX for middle fake frames, and Enabler for the rest\n\n2x - FFX\n"
                                    "3x - Enabler\n4x - FFX + Enabler\n5x - Enabler\n6x - FFX + Enabler");

            if (!state.artursFgFileAvailable)
                RenderEnablerMissing();
        }

        if (state.activeFgInput == FGInput::NvngxFG)
        {

            bool dmfgActive = state.dlssgGameDMFGSupported && config->FGDLSSGOverrideForceDMFG.value_or_default();

            if (!ReflexHooks::isReflexHooked())
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "Reflex not hooked");
                ImGui::Text("If you are using an AMD/Intel GPU, then make sure you have Fakenvapi");
            }
            else if (ReflexHooks::dlssgFrameCountToGenerate() == 0 && !dmfgActive)
            {
                ImGui::Text("Please select DLSS Frame Generation in the game options\n"
                            "You might need to select DLSS first");
            }

            if (state.swapchainApi == DX12)
            {
                ImGui::Text("Current DLSSG state:");
                ImGui::SameLine();
                if (auto count = state.dlssgDetectedInterpolationCount; count > 0)
                {
                    ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)),
                                       std::format("ON {}x", count + 1).c_str());
                }
                else
                {
                    ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "OFF");
                }

                // Issue mostly shows up on AMD on Windows on pre-RDNA3 in some non-UE games
                // Hide to reduce confusion, config is still read
                const bool isUnrealEngine = State::Instance().NVNGX_Engine == NVSDK_NGX_ENGINE_TYPE_UNREAL ||
                                            State::Instance().gameQuirks & GameQuirk::ForceUnrealEngine;
                const bool isDllProxyNvngxType =
                    activeNvngxFg == FGNvngxReplacement::Nukems || activeNvngxFg == FGNvngxReplacement::Arturs;
                if (isDllProxyNvngxType && !primaryGpu.dlssCapable && primaryGpu.fsr4Support == FSR4Support::None &&
                    !primaryGpu.usesVkd3dProton && !isUnrealEngine)
                {
                    if (bool makeDepthCopy = config->NvngxFGMakeDepthCopy.value_or_default();
                        DlssNr::NeuralUi::Checkbox("Fix broken visuals", &makeDepthCopy))
                    {
                        config->NvngxFGMakeDepthCopy = makeDepthCopy;
                    }
                    ShowHelpMarker("Makes a copy of the depth buffer\nCan fix broken visuals in some games on AMD "
                                   "GPUs under Windows\nCan cause stutters, so best to use only when necessary");
                }
            }
            else if (state.swapchainApi == Vulkan)
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                                   "DLSSG is purposefully disabled when this menu is visible");
                ImGui::Spacing();
            }
        }

        bool isLoaded = false;
        if (state.swapchainApi == Vulkan)
            isLoaded = Nvngx_FG::isVulkanAvailable();
        if (state.swapchainApi == DX12)
            isLoaded = Nvngx_FG::isDx12Available();

        if (isLoaded)
        {
            if (activeNvngxFg == FGNvngxReplacement::Arturs || activeNvngxFg == FGNvngxReplacement::Combo)
            {
                auto featureVer = Nvngx_FG::version();
                auto antighostingVer = Nvngx_FG::extraVersion();
                ImGui::Text("DE Ver: %d.%d.%d.%d   GB Ver: %d.%d", featureVer.major, featureVer.minor, featureVer.patch,
                            featureVer.reserved, antighostingVer.major, antighostingVer.minor);

                static std::vector<FlagDefinition> common_flags = {
                    { "Antighosting (GB)", 0x00100000, "Enable anti-ghosting correction" },
                    { "Temporal HUD pin", 0x04000000, "Enable temporal HUD pinning (present-backbuffer stability)" }
                };

                static std::vector<FlagDefinition> uncommon_flags = {
                    //{ "Hudless UI mask", 0x02000000, "Use HUD-less as UI mask (DL2 inverted semantics)" },
                    { "HUD interpolation", 0x08000000, "HUD OF interpolation (0=legacy pin-present, 1=OF warp)" },
                    { "Ignore UI texture", 0x10000000, "Ignore dedicated DLSSG.UI texture (force legacy HUD path)" },
                    //{ "Dp4a active", 0x20000000, "OF pipeline using dp4a-accelerated SSD (SM 6.4+)" },
                    { "Pin backbuffer", 0x40000000, "Pin DLSSG.Backbuffer to subframe-1 snapshot across MFG frame" }
                };

                static std::vector<FlagDefinition> debug_flags = {
                    { "Antighosting red tint", 0x00200000, "Debug: red tint on corrected pixels" },
                    { "Antighosting split screen", 0x00400000, "Debug: split screen comparison" },
                    { "Frame index line", 0x00010000, "" },
                    { "HUD detection", 0x00020000, "" },
                    { "Disocclusion tint", 0x00040000, "" },
                    { "Artifacts detection", 0x00080000, "" },
                    { "Camera MV debug", 0x00800000, "Debug: blue tint where camera MV fallback is used" },
                    { "Generic visualization", 0x01000000, "Debug: trapezoid zone visualization" }
                };

                uint32_t temp_flags = config->NvngxFGDispatchFlags.value_or_default();
                bool changed = false;

                ImGui::Text("Raw DispatchFlags:");
                changed |= ImGui::InputScalar("##RawFlags", ImGuiDataType_U32, &temp_flags, NULL, NULL, "%08X",
                                              ImGuiInputTextFlags_CharsHexadecimal);

                ImGui::SameLine(0.0f, 20.0f * menuResScale);
                if (bool showDebug = config->NvngxFGShowDebug.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Show Debug", &showDebug))
                {
                    config->NvngxFGShowDebug = showDebug;
                }
                ShowHelpMarker("Required for Debug flags to work correctly");

                if (auto ch = ScopedCollapsingHeader("Active DispatchFlags"); ch.IsHeaderOpen())
                {
                    auto render_flags = [&](const std::vector<FlagDefinition>& flags)
                    {
                        for (const auto& flag : flags)
                        {
                            changed |= ImGui::CheckboxFlags(flag.name.c_str(), &temp_flags, flag.mask);

                            if (ImGui::IsItemHovered() && !flag.description.empty())
                            {
                                ImGui::SetTooltip("%s", flag.description.c_str());
                            }
                        }
                    };

                    ImGui::TextDisabled("Common");
                    render_flags(common_flags);

                    ImGui::Spacing();
                    ImGui::TextDisabled("Uncommon");
                    render_flags(uncommon_flags);

                    if (config->NvngxFGShowDebug.value_or_default())
                    {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Debug");
                        render_flags(debug_flags);
                    }
                }

                if (changed)
                {
                    config->NvngxFGDispatchFlags = temp_flags;
                }
            }

            if (activeNvngxFg == FGNvngxReplacement::Nukems)
            {
                if (DlssNr::NeuralUi::Checkbox("Enable Debug View", &state.dlssgDebugView))
                {
                    Nvngx_FG::setDebugView(state.dlssgDebugView);
                }
                if (DlssNr::NeuralUi::Checkbox("Interpolated frames only", &state.dlssgInterpolatedOnly))
                {
                    Nvngx_FG::setInterpolatedOnly(state.dlssgInterpolatedOnly);
                }
            }

            if (activeNvngxFg == FGNvngxReplacement::FFX || activeNvngxFg == FGNvngxReplacement::Combo)
            {
                if (_ffxFGIndex < 0)
                    _ffxFGIndex = config->FfxFGIndex.value_or_default();

                if (state.ffxFGVersionNames.size() > 0)
                {
                    auto currentName = StrFmt("FSR %s", state.ffxFGVersionNames[_ffxFGIndex]);
                    if (DlssNr::NeuralUi::BeginCombo("FFX FG", currentName.c_str()))
                    {
                        for (int n = 0; n < state.ffxFGVersionIds.size(); n++)
                        {
                            auto name = StrFmt("FSR %s", state.ffxFGVersionNames[n]);
                            if (ImGui::Selectable(name.c_str(), config->FfxFGIndex.value_or_default() == n))
                                _ffxFGIndex = n;
                        }

                        ImGui::EndCombo();
                    }

                    ShowHelpMarker("List of FGs reported by FFX SDK");

                    ImGui::SameLine(0.0f, 6.0f);

                    if (DlssNr::NeuralUi::Button("Change FG") && _ffxFGIndex != config->FfxFGIndex.value_or_default())
                    {
                        config->FfxFGIndex = _ffxFGIndex;
                        state.fgChanged = true;
                    }
                }

                bool fgAsync = config->FGAsync.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Allow Async##2", &fgAsync))
                {
                    config->FGAsync = fgAsync;

                    if (config->FGEnabled.value_or_default())
                    {
                        state.fgChanged = true;
                        LOG_DEBUG("Async set FGChanged");
                    }
                }
                ShowHelpMarker("Enable Async for better FG performance\nMight cause crashes, especially with HUD Fix!");

                bool fgDV = config->FGDebugView.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("Debug View##3", &fgDV))
                {
                    config->FGDebugView = fgDV;

                    if (config->FGEnabled.value_or_default())
                    {
                        state.fgChanged = true;
                        LOG_DEBUG("DebugView set FGChanged");
                    }
                }
                ShowHelpMarker("Enable FSR3.1-FG Debug view\n\n"
                               "Top left: Game Motion Vectors\n"
                               "Top middle: GMV Depth\n"
                               "Top right: Optical Flow MV\n"
                               "Middle: Interpolated frame only\n"
                               "Bottom left: Disocclusion mask\n"
                               "Bottom middle: Interpolation source (w/o UI)\n"
                               "Bottom right: HUDless resource");

                if (Nvngx_FG::version().major > 3)
                {
                    ImGui::SameLine(0.0f, 20.0f * menuResScale);
                    if (bool fgwm = config->FSRFGEnableWatermark.value_or_default();
                        DlssNr::NeuralUi::Checkbox("Enable Watermark", &fgwm))
                    {
                        LOG_DEBUG("FSRFGEnableWatermark set FGWatermark: {}", fgwm);
                        config->FSRFGEnableWatermark = fgwm;
                    }

                    ShowHelpMarker("After changing this option, please Save Settings\n"
                                   "It will be applied on next launch.");
                }
            }

            if (bool disableHudless = config->NvngxFGDisableHudless.value_or_default();
                DlssNr::NeuralUi::Checkbox("Disable HUDless", &disableHudless))
            {
                config->NvngxFGDisableHudless = disableHudless;
            }
            ShowHelpMarker("Might be required for some sets of DispatchFlags");
        }
    }

    // FSR-FG Inputs
    if (state.currentFGSwapchain != nullptr &&
        (state.activeFgInput == FGInput::FSRFG || state.activeFgInput == FGInput::FSRFG30))
    {
        SeparatorWithHelpMarker("Frame Generation (FSR-FG Inputs)", "Select FSR-FG in-game");

        auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);
        if (fgOutput != nullptr)
        {
            ImGui::Text("Current FSR-FG state:");
            ImGui::SameLine();
            if (state.fsrfgInputActive)
            {
                if (fgOutput->IsActive())
                    ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)), "ON");
                else
                    ImGui::TextColored(toneMapColor(ImVec4(1.0f, 0.647f, 0.0f, 1.f)), "ACTIVATE FG");
            }
            else
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "OFF");
                ImGui::Text("Please select FSR Frame Generation in the game options\n"
                            "You might need to select FSR first");
            }
        }

        bool skipConfig = config->FSRFGSkipConfigForHudless.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Skip Config for HUDless", &skipConfig))
            config->FSRFGSkipConfigForHudless = skipConfig;

        ShowHelpMarker("Do not use HUDless set at ffxConfig");

        bool skipDispatch = config->FSRFGSkipDispatchForHudless.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Skip Dispatch for HUDless", &skipDispatch))
            config->FSRFGSkipDispatchForHudless = skipDispatch;

        ShowHelpMarker("Do not use HUDless set at ffxDispatch");
    }

    // Streamline FG Inputs
    if (state.currentFGSwapchain != nullptr && state.activeFgInput == FGInput::DLSSG)
    {
        SeparatorWithHelpMarker("Frame Generation (Streamline FG Inputs)", "Select DLSS-FG in-game");

        auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);

        if (!ReflexHooks::isReflexHooked())
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "Reflex not hooked");
            ImGui::Text("If you are using an AMD/Intel GPU, then make sure you have fakenvapi");
        }
        else if (fgOutput != nullptr)
        {
            ImGui::Text("Current Streamline FG state:");
            ImGui::SameLine();
            if ((state.fgLastFrame - state.dlssgLastFrame) < 3)
            {
                if (fgOutput->IsActive())
                    ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)), "ON");
                else
                    ImGui::TextColored(toneMapColor(ImVec4(1.0f, 0.647f, 0.0f, 1.f)), "ACTIVATE FG");
            }
            else
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), "OFF");
                ImGui::Text("Please select DLSS Frame Generation in the game options\n"
                            "You might need to select DLSS first");
            }
        }
    }
}

void MenuCommon::RenderFsrCommonSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // FSR Common -----------------
        if (currentFeature != nullptr && !currentFeature->IsFrozen() &&
            (state.activeFgOutput == FGOutput::FSRFG || IsFsr(currentBackend)))
        {
            SeparatorWithHelpMarker("FSR Common Settings", "Affects both FSR-FG & Upscalers");

            bool useFsrVales = config->FsrUseFsrInputValues.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Use FSR Input Values", &useFsrVales))
                config->FsrUseFsrInputValues = useFsrVales;

            if (auto ch = ScopedCollapsingHeader("FoV & Camera Values"); ch.IsHeaderOpen())
            {
                bool useVFov = config->FsrVerticalFov.has_value() || !config->FsrHorizontalFov.has_value();

                float vfov = config->FsrVerticalFov.value_or_default();
                float hfov = config->FsrHorizontalFov.value_or(90.0f);

                if (useVFov && !config->FsrVerticalFov.has_value())
                    config->FsrVerticalFov = vfov;
                else if (!useVFov && !config->FsrHorizontalFov.has_value())
                    config->FsrHorizontalFov = hfov;

                if (ImGui::RadioButton("Use Vert. Fov", useVFov))
                {
                    config->FsrHorizontalFov.reset();
                    config->FsrVerticalFov = vfov;
                    useVFov = true;
                }

                ImGui::SameLine(0.0f, 6.0f);

                if (ImGui::RadioButton("Use Horz. Fov", !useVFov))
                {
                    config->FsrVerticalFov.reset();
                    config->FsrHorizontalFov = hfov;
                    useVFov = false;
                }

                if (useVFov)
                {
                    if (DlssNr::NeuralUi::FillSlider("Vert. FOV", &vfov, 0.0f, 180.0f, "%.1f"))
                        config->FsrVerticalFov = vfov;

                    ShowHelpMarker("Might help achieve better image quality");
                }
                else
                {
                    if (DlssNr::NeuralUi::FillSlider("Horz. FOV", &hfov, 0.0f, 180.0f, "%.1f"))
                        config->FsrHorizontalFov = hfov;

                    ShowHelpMarker("Might help achieve better image quality");
                }

                float cameraNear;
                float cameraFar;

                cameraNear = config->FsrCameraNear.value_or_default();
                cameraFar = config->FsrCameraFar.value_or_default();

                if (DlssNr::NeuralUi::FillSlider("Camera Near", &cameraNear, 0.1f, 500000.0f, "%.1f"))
                    config->FsrCameraNear = cameraNear;
                ShowHelpMarker("Might help achieve better image quality\n"
                               "And potentially less ghosting");

                if (DlssNr::NeuralUi::FillSlider("Camera Far", &cameraFar, 0.1f, 500000.0f, "%.1f"))
                    config->FsrCameraFar = cameraFar;
                ShowHelpMarker("Might help achieve better image quality\n"
                               "And potentially less ghosting");

                if (DlssNr::NeuralUi::Button("Reset Camera Values"))
                {
                    config->FsrVerticalFov.reset();
                    config->FsrHorizontalFov.reset();
                    config->FsrCameraNear.reset();
                    config->FsrCameraFar.reset();
                }

                ImGui::SameLine(0.0f, 6.0f);
                ImGui::Text("Near: %.1f Far: %.1f",
                            state.lastFsrCameraNear < 500000.0f ? state.lastFsrCameraNear : 500000.0f,
                            state.lastFsrCameraFar < 500000.0f ? state.lastFsrCameraFar : 500000.0f);
            }
        }
    }
}

void MenuCommon::RenderFramerateSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& menuResScale = ctx.menuResScale;

    // Framerate ---------------------
    if (state.reflexLimitsFps || config->OverlayMenu.value_or_default())
    {
        SeparatorWithHelpMarker(
            "Framerate", "Uses Reflex when possible\nOn AMD/Intel cards, you can use Fakenvapi to substitute Reflex");

        static std::string currentMethod {};
        LowLatencyMode fakenvapiMode = {};
        if (state.reflexLimitsFps)
        {
            fakenvapiMode = fakenvapi::getCurrentMode();

            if (fakenvapiMode == LowLatencyMode::AntiLag2)
                currentMethod = "FSR Anti-Lag 2.0";
            else if (fakenvapiMode == LowLatencyMode::LatencyFlex)
                currentMethod = "LatencyFlex";
            else if (fakenvapiMode == LowLatencyMode::XeLL)
                currentMethod = "XeLL";
            else if (fakenvapiMode == LowLatencyMode::AntiLagVk)
                currentMethod = "Vulkan AntiLag";
            else if (fakenvapiMode == LowLatencyMode::None)
            {
                if (fakenvapi::isUsingAsMainNvapi())
                    currentMethod = "None";
                else
                    currentMethod = "Reflex";
            }

            if (state.rtssReflexInjection && fakenvapiMode == LowLatencyMode::AntiLag2 &&
                config->FGOutput.value_or_default() == FGOutput::FSRFG)
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                                   "Using RTSS Reflex injection with FSR Anti-Lag 2.0 and FSR FG "
                                   "might cause issues");
        }
        else
        {
            if (XellHooks::canLimit())
                currentMethod = "Game's XeLL";
            else
                currentMethod = "Fallback";
        }

        if (state.rtssReflexInjection)
            currentMethod.append(" (RTSS)");

        const bool fakenvapiInactive = (fakenvapi::isUsingAsMainNvapi() || fakenvapiMode == LowLatencyMode::XeLL) &&
                                       !fakenvapi::isLowLatencyActive() && state.reflexLimitsFps;

        if (fakenvapiInactive)
            currentMethod.append(" (inactive)");

        if (state.reflexShowWarning)
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)),
                               "Using Reflex's limit with FSR FG has performance overhead");

            ImGui::Spacing();
        }

        // set initial value
        if (std::isinf(_limitFps))
            _limitFps = config->FramerateLimit.value_or_default();

        DlssNr::NeuralUi::FillSlider("FPS Limit", &_limitFps, 0, 200, "%.0f");
        // (AMDNR 0.3.4, the approved other-tabs mock) The applied limit and the method as the row's tag (0.3.3.2: a
        // "Current method: ..." line above the slider).
        {
            const float applied = config->FramerateLimit.value_or_default();
            const std::string limitTag =
                (applied > 0.0f ? StrFmt("%.0f fps", applied) : std::string("off")) + " - limiter: " + currentMethod;
            DlssNr::NeuralUi::DimTag(limitTag.c_str());
            if (fakenvapiMode == LowLatencyMode::AntiLag2)
                ShowHelpMarker("FSR Anti-Lag 2.0 is the new name for AntiLag 2\nDon't ask me why");
        }

        if (DlssNr::NeuralUi::Button("Apply Limit"))
        {
            config->FramerateLimit = _limitFps;
        }

        ImGui::SameLine(0.0f, 16.0f);

        if (DlssNr::NeuralUi::Button("Reset Limit"))
        {
            _limitFps = 0.0f;
            config->FramerateLimit = _limitFps;
        }

        if (auto ch = ScopedCollapsingHeader("VRR Frame Cap Calculator"); ch.IsHeaderOpen())
        {
            ImGui::PushItemWidth(105.0f * menuResScale);
            ImGui::InputInt("Refresh Rate", &refreshRate, 1, 1, ImGuiInputTextFlags_None);
            ImGui::PopItemWidth();

            float refreshRateF = static_cast<float>(refreshRate);
            // it's fine to use with real reflex, we only care about antilag
            auto fpsLimitTech = fakenvapi::getCurrentMode();
            constexpr float margin = 0.3f; // in ms
            float frameCap = std::round(10000.f / (1000.f / refreshRateF + margin)) / 10.f;

            if (fpsLimitTech == LowLatencyMode::AntiLag2 || fpsLimitTech == LowLatencyMode::AntiLagVk)
                frameCap = std::round(frameCap);

            ImGui::Text("Calculated Cap: %.1f", frameCap);

            ImGui::SameLine(0.0f, 16.0f);

            if (DlssNr::NeuralUi::Button("Set as FPS Limit"))
            {
                _limitFps = frameCap;
                config->FramerateLimit = _limitFps;
            }
        }
    }
}

void MenuCommon::RenderFakenvapiSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;

    // FAKENVAPI ---------------------------
    // (AMDNR 0.3.4, the approved other-tabs mock) "Low latency" (0.3.3.2: "fakenvapi", the library's name), one control
    // per row, each greyed box with its reason as the row's tag.
    SectionHeader("Low latency");

    // Using state.reflexLimitsFps as a detection for Reflex being used on Nvidia
    bool showLatencyFlex =
        fakenvapi::isUsingAsMainNvapi() || (state.activeFgOutput == FGOutput::XeFG && state.reflexLimitsFps);

    if (showLatencyFlex)
    {
        const bool lfxBlocked = state.activeFgOutput == FGOutput::XeFG || state.activeFgInput == FGInput::ForceXeLL;
        ImGui::BeginDisabled(lfxBlocked);
        if (bool forceLFX = config->FN_ForceLatencyFlex.value_or_default();
            DlssNr::NeuralUi::Checkbox("Force LatencyFlex", &forceLFX))
        {
            config->FN_ForceLatencyFlex = forceLFX;
        }
        ShowHelpMarker("By default, FSR Anti-Lag 2.0/XeLL is used when available.\n"
                       "This setting lets you force LatencyFlex instead");
        if (lfxBlocked)
            DlssNr::NeuralUi::DimTag(state.activeFgOutput == FGOutput::XeFG ? "not with XeFG" : "not with Force XeLL");
        ImGui::EndDisabled();
    }

    // Force XeLL is always visible
    bool forceXell = config->ForceXeLL.value_or_default();
    static bool activeForceXeLL = forceXell;

    if (DlssNr::NeuralUi::Checkbox("Force XeLL", &forceXell))
    {
        config->ForceXeLL = forceXell;
    }
    ShowHelpMarker("Allows XeLL to work without FG on non-Intel cards.\n\nDisables FG "
                   "options\n\nRequires a restart");
    DlssNr::NeuralUi::DimTag("turns frame gen off");

    if (activeForceXeLL != forceXell)
    {
        ImGui::Spacing();
        ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.0f, 1.f)), "Save INI and restart to apply the changes");
        ImGui::Spacing();
    }

    if (showLatencyFlex)
    {
        // clang-format off
        static const std::vector<MenuOption<LFXMode>> lfx_modes = {
            { LFXMode::Conservative, "Conservative",
                "The safest, but might not reduce latency well" },
            { LFXMode::Aggressive, "Aggressive",
                "Improves latency, but in some cases will lower FPS more than expected" },
            { LFXMode::ReflexIDs, "Reflex ID",
                "Best when can be used, some games are not compatible (e.g. Cyberpunk)\n"
                "and will fallback to Aggressive" }
        };

        bool usingLFX = fakenvapi::getCurrentMode() == LowLatencyMode::LatencyFlex;

        ImGui::BeginDisabled(!usingLFX);
        PopulateCombo("LatencyFlex mode", config->FN_LatencyFlexMode, lfx_modes);
        ImGui::EndDisabled();

        static std::vector<MenuOption<ForceReflex>> reflex_modes = { { ForceReflex::InGame, "Follow in-game" },
                                                                { ForceReflex::ForceDisable, "Force Disable" },
                                                                { ForceReflex::ForceEnable, "Force Enable" } };

        PopulateCombo("Force Reflex", config->FN_ForceReflex, reflex_modes);
        // clang-format on
    }
}

template <typename T> std::string GetMenuOptionLabel(const std::vector<MenuOption<T>>& options, T targetValue)
{
    auto it = std::find_if(options.begin(), options.end(),
                           [targetValue](const MenuOption<T>& option) { return option.value == targetValue; });

    if (it != options.end())
    {
        return it->label;
    }

    return "Unknown";
}

void MenuCommon::RenderLowLatencySettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;

    // Low Latency ---------------------------
    SectionHeader("Low Latency");

    static std::vector<MenuOption<LowLatencyInput>> lowLatencyInput = {
        { LowLatencyInput::None, "None (Off)" },    { LowLatencyInput::Auto, "Auto" },
        { LowLatencyInput::AntiLag2, "AntiLag 2" }, { LowLatencyInput::Reflex, "Reflex" },
        { LowLatencyInput::XeLL, "XeLL" },          { LowLatencyInput::UeLowLatency, "UeLowLatency" },
    };

    static std::vector<MenuOption<LowLatencyMode>> lowLatencyOutput = {
        { LowLatencyMode::None, "None (Off)" },
        { LowLatencyMode::Auto, "Auto" },
        { LowLatencyMode::LatencyFlex, "LatencyFlex" },
        { LowLatencyMode::AntiLag2, "AntiLag 2" },
        { LowLatencyMode::XeLL, "XeLL" },
        { LowLatencyMode::AntiLagVk, "AntiLag Vk" },
        { LowLatencyMode::Reflex, "Reflex" },
    };

    LowLatencyInput activeInput {};
    LowLatencyMode activeOutput {};

    if (ImGui::BeginTable("lowLatencyActive", 2, ImGuiTableFlags_SizingStretchSame))
    {
        InputCommon::get_currently_active(activeInput, activeOutput);

        ImGui::TableNextColumn();

        ImGui::Text("Active input: %s", GetMenuOptionLabel(lowLatencyInput, activeInput).c_str());

        ImGui::TableNextColumn();

        ImGui::Text("Active output: %s", GetMenuOptionLabel(lowLatencyOutput, activeOutput).c_str());

        ImGui::EndTable();
    }

    if (ImGui::BeginTable("lowLatencySelection", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextColumn();

        auto avalibleInputs = InputCommon::get_avaliable_inputs();

        lowLatencyInput[(uint32_t) LowLatencyInput::AntiLag2].set_disabled(!avalibleInputs[LowLatencyInput::AntiLag2]);
        lowLatencyInput[(uint32_t) LowLatencyInput::Reflex].set_disabled(!avalibleInputs[LowLatencyInput::Reflex]);
        lowLatencyInput[(uint32_t) LowLatencyInput::XeLL].set_disabled(!avalibleInputs[LowLatencyInput::XeLL]);
        lowLatencyInput[(uint32_t) LowLatencyInput::UeLowLatency].set_disabled(
            !avalibleInputs[LowLatencyInput::UeLowLatency]);

        // need to have a value before combo
        if (!config->LowLatencyInput.has_value())
            config->LowLatencyInput = config->LowLatencyInput.value_or_default();

        PopulateCombo("Input", config->LowLatencyInput, lowLatencyInput);

        ImGui::TableNextColumn();

        lowLatencyOutput[(uint32_t) LowLatencyMode::AntiLagVk].set_disabled(true, "No support");
        lowLatencyOutput[(uint32_t) LowLatencyMode::Reflex].set_disabled(true, "No support");

        // need to have a value before combo
        if (!config->LowLatencyOutput.has_value())
            config->LowLatencyOutput = config->LowLatencyOutput.value_or_default();

        PopulateCombo("Output", config->LowLatencyOutput, lowLatencyOutput);

        ImGui::EndTable();
    }

    if (activeOutput == LowLatencyMode::LatencyFlex)
    {
        static const std::vector<MenuOption<LFXMode>> lfx_modes = {
            { LFXMode::Conservative, "Conservative", "The safest, but might not reduce latency well" },
            { LFXMode::Aggressive, "Aggressive",
              "Improves latency, but in some cases will lower FPS more than expected" },
            { LFXMode::ReflexIDs, "Reflex ID",
              "Best when can be used, some games are not compatible (e.g. Cyberpunk)\n"
              "and will fallback to Aggressive" }
        };

        PopulateCombo("LatencyFlex mode", config->FN_LatencyFlexMode, lfx_modes);
    }

    static std::vector<MenuOption<ForceReflex>> lowlatency_states = { { ForceReflex::InGame, "Follow in-game" },
                                                                      { ForceReflex::ForceDisable, "Force Disable" },
                                                                      { ForceReflex::ForceEnable, "Force Enable" } };

    PopulateCombo("Force State", config->FN_ForceReflex, lowlatency_states);
}

void MenuCommon::RenderActiveImageSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;

    bool rcasEnabled = false;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // SHARPNESS -----------------------------
        SectionHeader("Sharpness");

        if (bool overrideSharpness = config->OverrideSharpness.value_or_default();
            DlssNr::NeuralUi::Checkbox("Override", &overrideSharpness))
        {
            config->OverrideSharpness = overrideSharpness;

            if (currentBackend == Upscaler::DLSS && currentFeature->Version().major < 3)
            {
                state.newBackend = currentBackend;
                MARK_ALL_BACKENDS_CHANGED();
            }
        }
        // (AMDNR 0.3.4.1) With FSR Ray Regeneration a game that sends no sharpness gets AMDNR's RR default
        // (IFeature::DefaultSharpnessWhenTitleSendsNone; 0 under the path-traced profile): the help says so.
        const float rrDefaultSharpness = currentFeature->GetUpscalerType() == Upscaler::FSR_RR
                                             ? currentFeature->DefaultSharpnessWhenTitleSendsNone()
                                             : 0.0f;
        // (AMDNR 0.3.4.1, Proton RR) Under Wine/Proton that default is 0 (hooks/RrHardwareGate.h): the help and the
        // tag below say so instead of naming the Windows value.
        const bool rrDefaultOffOnProton =
            currentFeature->GetUpscalerType() == Upscaler::FSR_RR && State::Instance().isRunningOnLinux;
        if (rrDefaultSharpness > 0.0f)
            ShowHelpMarker(StrFmt("Ignores the value sent by the game\n"
                                  "and uses the value set below\n\n"
                                  "With Ray Regeneration, AMDNR sharpens by %.2f when the game\n"
                                  "sends no sharpness; tick Override and set 0 to turn it off",
                                  rrDefaultSharpness)
                               .c_str());
        else if (rrDefaultOffOnProton)
            ShowHelpMarker("Ignores the value sent by the game\n"
                           "and uses the value set below\n\n"
                           "With Ray Regeneration on Wine/Proton, AMDNR does not sharpen\n"
                           "when the game sends no sharpness (default 0, off on Proton);\n"
                           "tick Override and set a value to sharpen");
        else
            ShowHelpMarker("Ignores the value sent by the game\n"
                           "and uses the value set below");

        // (AMDNR 0.3.4, the approved other-tabs mock) The sharpness in use as the row's tag.
        const float featuresCurrentSharpness = currentFeature->Sharpness();
        if (featuresCurrentSharpness > 0.0f)
            DlssNr::NeuralUi::DimTag(StrFmt("current %.3f", featuresCurrentSharpness).c_str());
        else
            DlssNr::NeuralUi::DimTag("current: off");
        // (AMDNR 0.3.4.1) The value in use is AMDNR's RR default, not the game's.
        if (!config->OverrideSharpness.value_or_default() && currentFeature->SharpnessIsFeatureDefault())
            DlssNr::NeuralUi::DimTag("RR default: the game sends none");
        // (AMDNR 0.3.4.1, Proton RR) On Wine/Proton the game's none stands: the real default, 0.
        else if (!config->OverrideSharpness.value_or_default() && rrDefaultOffOnProton &&
                 !(featuresCurrentSharpness > 0.0f))
            DlssNr::NeuralUi::DimTag("RR default 0 (off on Proton)");
        // (AMDNR 0.3.4.2, RN2) A [Sharpness] Sharpness left in the ini while Override is off does nothing at all, and
        // ticking Override applies it at once - one Ray Regeneration report carries 1.00, four times the RR default.
        // Name the waiting number, so a player chasing grain or softness sees what Override would do before ticking
        // it. Only when ticking Override would really change the picture, i.e. when the stored number differs from the
        // sharpness in use: that is the title's own value when it sends one, and AMDNR's RR default only when it sends
        // none (comparing with the default alone hid the tag in the case that matters most - a title sending 0.60 with
        // 0.25 waiting in the ini). Compared with a tolerance, so a stored 0.2500001 is not named as waiting.
        if (currentFeature->GetUpscalerType() == Upscaler::FSR_RR && !config->OverrideSharpness.value_or_default())
        {
            if (const auto storedSharpness = config->Sharpness.value_for_config_ignore_default();
                storedSharpness.has_value() &&
                (storedSharpness.value() > featuresCurrentSharpness + 0.0005f ||
                 storedSharpness.value() < featuresCurrentSharpness - 0.0005f))
                DlssNr::NeuralUi::DimTag(
                    StrFmt("ini Sharpness %.2f waits for Override", storedSharpness.value()).c_str());
        }

        ImGui::BeginDisabled(!config->OverrideSharpness.value_or_default());

        float sharpness = config->Sharpness.value_or_default();

        if (DlssNr::NeuralUi::FillSlider("Sharpness", &sharpness, 0.0f, 1.0f, "%.3f"))
            config->Sharpness = sharpness;
        if (!config->OverrideSharpness.value_or_default())
            DlssNr::NeuralUi::DimTag("needs Override");

        ImGui::EndDisabled();

        // RCAS
        // if (state.api == DX12 || state.api == DX11)
        {
            // xess or dlss version >= 2.5.1
            constexpr feature_version requiredDlssVersion = { 2, 5, 1 };
            rcasEnabled = (currentBackend == Upscaler::XeSS ||
                           (currentBackend == Upscaler::DLSS && currentFeature->Version() >= requiredDlssVersion));

            if (bool rcas = config->RcasEnabled.value_or(rcasEnabled); DlssNr::NeuralUi::Checkbox("Enable RCAS/DA", &rcas))
                config->RcasEnabled = rcas;

            ShowHelpMarker("Enable OptiScaler's sharpening filter\n"
                           "By default uses a sharpening value provided by the game\n"
                           "Select 'Override' under 'Sharpness' and adjust the slider\n"
                           "to change it\n\n"
                           "Some upscalers have their own sharpness filter, so this\n"
                           "option is not always needed");

            ImGui::BeginDisabled(!config->RcasEnabled.value_or(rcasEnabled));

            // (AMDNR 0.3.4, the approved other-tabs mock) One "Sharpener" combo instead of three radio buttons; the
            // same three choices write the same [Sharpness] SharpnessShader values, each keeps its help in its item.
            static const SharpenShader kShaders[] = { SharpenShader::RCAS, SharpenShader::DepthAware,
                                                      SharpenShader::LocalContrastDepthAware };
            static const char* const kShaderNames[] = { "RCAS", "Depth Aware (RCAS)", "Depth Aware (DAS)" };
            static const char* const kShaderHelp[] = {
                "Use AMD's RCAS\n"
                "Modified to add Contrast parameter\n"
                "and MAS support",
                "Use Depth Aware Sharpening (RCAS)\n"
                "Smarter sharpening with less artifacts,\n"
                "but also heavier\n\n"
                "The farther away is the object, the more\n"
                "sharpening is applied",
                "Use Depth Aware Sharpening (DAS)\n"
                "Depth-aware directional adaptive luma sharpener\n"
                "Smarter sharpening with less artifacts,\n"
                "but also heavier\n\n"
                "The farther away is the object, the more\n"
                "sharpening is applied",
            };
            const SharpenShader currentShader = Config::Instance()->SharpnessShader.value_or_default();
            int shaderIndex = -1;
            for (int i = 0; i < IM_ARRAYSIZE(kShaders); ++i)
            {
                if (kShaders[i] == currentShader)
                    shaderIndex = i;
            }
            const std::string shaderPreview =
                shaderIndex >= 0 ? kShaderNames[shaderIndex] : StrFmt("%d", static_cast<int>(currentShader));

            if (DlssNr::NeuralUi::BeginCombo("Sharpener", shaderPreview.c_str()))
            {
                for (int i = 0; i < IM_ARRAYSIZE(kShaders); ++i)
                {
                    if (ImGui::Selectable(kShaderNames[i], shaderIndex == i))
                        Config::Instance()->SharpnessShader = kShaders[i];
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                        ImGui::SetTooltip("%s", kShaderHelp[i]);
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker("OptiScaler's sharpening filter:\n"
                           "RCAS: AMD's RCAS, modified to add a Contrast parameter and MAS support.\n"
                           "Depth Aware (RCAS) / (DAS): smarter sharpening with less artifacts, but\n"
                           "heavier; the farther away the object, the more sharpening is applied.\n"
                           "Hover an item in the list for more.");
            if (!config->RcasEnabled.value_or(rcasEnabled))
                DlssNr::NeuralUi::DimTag("needs Enable RCAS/DA");

            if (bool overrideMotionSharpness = config->MotionSharpnessEnabled.value_or_default();
                DlssNr::NeuralUi::Checkbox("Enable Motion Adaptive Sharpness", &overrideMotionSharpness))
                config->MotionSharpnessEnabled = overrideMotionSharpness;
            ShowHelpMarker("Enables sharpness adjustments according to the motion");

            if (Config::Instance()->SharpnessShader.value_or_default() != SharpenShader::RCAS)
            {
                if (bool overrideMSDebug = config->MotionSharpnessDebug.value_or_default();
                    DlssNr::NeuralUi::Checkbox("DA + MAS Debug", &overrideMSDebug))
                    config->MotionSharpnessDebug = overrideMSDebug;

                ShowHelpMarker("Enable DA + MAS debug views\n"
                               "Blue tint for DA detected edges\n\n"
                               "More red areas will have more sharpness applied\n"
                               "Green areas will get reduced sharpness");

                if (auto ch = ScopedCollapsingHeader("Advanced DA Parameters"); ch.IsHeaderOpen())
                {
                    if (bool clamp = config->DAClampOutput.value_or(false); DlssNr::NeuralUi::Checkbox("Clamp Output", &clamp))
                    {
                        if (clamp)
                            config->DAClampOutput = true;
                        else
                            config->DAClampOutput.reset();
                    }

                    ShowHelpMarker("Clamps the final image to the [0, 1] range.\n\n"
                                   "Prevents overshoot artifacts such as bright halos or negative colors.\n"
                                   "Recommended for LDR pipelines; optional for HDR depending on tone-mapping.\n\n"
                                   "When not set OptiScaler controls it via upscalers HDR flag");

                    if (currentFeature->DepthLinear())
                    {
                        float depthBias = config->DADepthBias.value_or(0.0015f);
                        if (DlssNr::NeuralUi::FillSlider("Depth Bias", &depthBias, 0.005f, 0.03f, "%.4f"))
                            config->DADepthBias = depthBias;

                        ShowHelpMarker("Ignores small depth differences before edge detection.\n\n"
                                       "Higher values reduce flickering and noise from minor depth changes, but may "
                                       "soften real geometry edges.\n"
                                       "Lower values preserve fine detail but can cause unstable or noisy edge "
                                       "detection.");

                        float depthScale = config->DADepthScale.value_or(250.0f);
                        if (DlssNr::NeuralUi::FillSlider("Depth Scale", &depthScale, 100.0f, 600.0f, "%.1f"))
                            config->DADepthScale = depthScale;

                        ShowHelpMarker("Controls how strongly sharpening is reduced across depth edges.\n\n"
                                       "Higher values more aggressively prevent sharpening across object boundaries "
                                       "(reduces halos).\n"
                                       "Lower values allow more sharpening to pass across edges (sharper but "
                                       "riskier).");
                    }
                    else
                    {
                        float depthBias = config->DADepthBias.value_or(0.001f);
                        if (DlssNr::NeuralUi::FillSlider("Depth Bias", &depthBias, 0.0001f, 0.003f, "%.4f"))
                            config->DADepthBias = depthBias;

                        ShowHelpMarker("Ignores small depth differences before edge detection.\n\n"
                                       "Higher values reduce flickering and noise from minor depth changes, but may "
                                       "soften real geometry edges.\n"
                                       "Lower values preserve fine detail but can cause unstable or noisy edge "
                                       "detection.");

                        float depthScale = config->DADepthScale.value_or(35.0f);
                        if (DlssNr::NeuralUi::FillSlider("Depth Scale", &depthScale, 25.0f, 400.0f, "%.1f"))
                            config->DADepthScale = depthScale;

                        ShowHelpMarker("Controls how strongly sharpening is reduced across depth edges.\n\n"
                                       "Higher values more aggressively prevent sharpening across object boundaries "
                                       "(reduces halos).\n"
                                       "Lower values allow more sharpening to pass across edges (sharper but "
                                       "riskier).");
                    }

                    if (DlssNr::NeuralUi::Button("Reset Depth Values"))
                    {
                        config->DADepthBias.reset();
                        config->DADepthScale.reset();
                    }
                }
            }
            else
            {
                if (bool contrastEnabled = config->ContrastEnabled.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Contrast Enabled", &contrastEnabled))
                    config->ContrastEnabled = contrastEnabled;

                ShowHelpMarker("Controls sharpness at high contrast areas.");

                ImGui::BeginDisabled(!config->ContrastEnabled.value_or_default());

                float contrast = config->Contrast.value_or_default();
                if (DlssNr::NeuralUi::FillSlider("Contrast", &contrast, -2.0f, 2.0f, "%.2f"))
                    config->Contrast = contrast;

                ShowHelpMarker("Positive values decrease sharpness at high contrast areas.\n"
                               "Negative values increase sharpness at high contrast areas.");

                ImGui::EndDisabled();
            }

            if (auto ch = ScopedCollapsingHeader("Motion Adaptive Sharpness##2"); ch.IsHeaderOpen())
            {
                ImGui::BeginDisabled(!config->MotionSharpnessEnabled.value_or_default());

                if (Config::Instance()->SharpnessShader.value_or_default() == SharpenShader::RCAS)
                {
                    if (bool overrideMSDebug = config->MotionSharpnessDebug.value_or_default();
                        DlssNr::NeuralUi::Checkbox("MAS Debug", &overrideMSDebug))
                        config->MotionSharpnessDebug = overrideMSDebug;
                    ShowHelpMarker("Areas that are more red will have more sharpness applied\n"
                                   "Green areas will get reduced sharpness");
                }

                float motionSharpness = config->MotionSharpness.value_or_default();
                DlssNr::NeuralUi::FillSlider("MotionSharpness", &motionSharpness, -1.0f, 1.0f, "%.3f");
                config->MotionSharpness = motionSharpness;

                ShowHelpMarker("Maximum amount of sharpness that motion can add or remove.\n\n"
                               "Negative values reduce sharpening in motion (recommended).\n"
                               "Positive values increase sharpening in motion.\n\n"
                               "The final adjustment scales with motion and is capped at this value.");

                float motionThreshod = config->MotionThreshold.value_or_default();
                // The label's typo is fixed (AMDNR 0.3.4); the ImGui ID stays "MotionThreshod".
                DlssNr::NeuralUi::FillSlider("Motion Threshold###MotionThreshod", &motionThreshod, 0.0f, 100.0f, "%.2f");
                config->MotionThreshold = motionThreshod;

                ShowHelpMarker("Minimum motion required before motion-based sharpening adjustment begins.\n\n"
                               "Higher values ignore small movements (more stable).\n"
                               "Lower values react to subtle motion (more sensitive).");

                float motionScale = config->MotionScaleLimit.value_or_default();
                DlssNr::NeuralUi::FillSlider("MotionRange", &motionScale, 0.01f, 100.0f, "%.2f");
                config->MotionScaleLimit = motionScale;

                ShowHelpMarker("Defines the motion range over which the effect ramps from zero to full strength.\n\n"
                               "Values above the threshold are mapped into this range.\n"
                               "Larger values make the response smoother and more gradual.\n"
                               "Smaller values make the effect react more quickly and aggressively.");

                ImGui::EndDisabled();
            }

            ImGui::EndDisabled();
        }
    }
}

// Render resolution (AMDNR 0.3.4, the approved other-tabs mock): 0.3.3.2's "Upscale Ratio Override" and "Output
// Scaling" sections of the Image tab, now one section of the Upscaling tab. The same controls, keys and Apply rule.
void MenuCommon::RenderRenderResolutionSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // UPSCALE RATIO OVERRIDE -----------------

        auto minSliderLimit = config->ExtendedLimits.value_or_default() ? 0.1f : 1.0f;
        auto maxSliderLimit = config->ExtendedLimits.value_or_default() ? 6.0f : 3.0f;

        SectionHeader("Render resolution");

        // Labels name the setting now that the section is "Render resolution" (0.3.3.2: "Override all", "Override per
        // quality preset" under an "Upscale Ratio Override" heading); the same keys.
        if (bool upOverride = config->UpscaleRatioOverrideEnabled.value_or_default();
            DlssNr::NeuralUi::Checkbox("Upscale Ratio Override (all presets)", &upOverride))
        {
            config->UpscaleRatioOverrideEnabled = upOverride;

            if (upOverride)
                config->QualityRatioOverrideEnabled = false;
        }
        ShowHelpMarker("Overrides every upscaler preset with the set value\n\n"
                       "1.5x on a 1080p screen means an internal res of 720p\n"
                       "1080 / 1.5 = 720");

        if (bool qOverride = config->QualityRatioOverrideEnabled.value_or_default();
            DlssNr::NeuralUi::Checkbox("Upscale Ratio Override (per preset)", &qOverride))
        {
            config->QualityRatioOverrideEnabled = qOverride;

            if (qOverride)
                config->UpscaleRatioOverrideEnabled = false;
        }

        ShowHelpMarker("Lets you override each preset's ratio individually\n"
                       "Note that not every game supports every quality preset\n\n"
                       "1.5x on a 1080p screen means internal resolution of 720p\n"
                       "1080 / 1.5 = 720");

        // The ratios are the checked override's children (22 px in, as the mock's option rows).
        if (config->UpscaleRatioOverrideEnabled.value_or_default())
        {
            ImGui::Indent();
            float urOverride = config->UpscaleRatioOverrideValue.value_or_default();
            DlssNr::NeuralUi::FillSlider("All Ratios", &urOverride, minSliderLimit, maxSliderLimit, "%.3f");
            config->UpscaleRatioOverrideValue = urOverride;
            ImGui::Unindent();
        }

        if (config->QualityRatioOverrideEnabled.value_or_default())
        {
            ImGui::Indent();
            float qDlaa = config->QualityRatio_DLAA.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("DLAA", &qDlaa, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_DLAA = qDlaa;

            float qUq = config->QualityRatio_UltraQuality.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("Ultra Quality", &qUq, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_UltraQuality = qUq;

            float qQ = config->QualityRatio_Quality.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("Quality", &qQ, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_Quality = qQ;

            float qB = config->QualityRatio_Balanced.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("Balanced", &qB, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_Balanced = qB;

            float qP = config->QualityRatio_Performance.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("Performance", &qP, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_Performance = qP;

            float qUp = config->QualityRatio_UltraPerformance.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("Ultra Performance", &qUp, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_UltraPerformance = qUp;
            ImGui::Unindent();
        }

        if (currentFeature != nullptr && !currentFeature->IsFrozen())
        {
            // OUTPUT SCALING -----------------------------
            // if (state.api == DX12 || state.api == DX11)
            {
                // if motion vectors are not display size
                ImGui::BeginDisabled(!currentFeature->LowResMV() &&
                                     currentFeature->RenderWidth() != currentFeature->DisplayWidth());

                // (AMDNR 0.3.4, the approved other-tabs mock) One "Output Scaling" row (0.3.3.2: an "Output Scaling"
                // heading and an "Enable" box); Downscaler, Ratio, Apply Change and the result line are its children.
                float defaultRatio = 1.5f;

                if (_ssRatio == 0.0f)
                {
                    _ssRatio = config->OutputScalingMultiplier.value_or(defaultRatio);
                    _ssEnabled = config->OutputScalingEnabled.value_or_default();
                    _ssDownsampler = config->OutputScalingDownscaler.value_or_default();
                }

                ImGui::BeginDisabled((currentBackend == Upscaler::XeSS || currentBackend == Upscaler::DLSS) &&
                                     currentFeature->RenderWidth() > currentFeature->DisplayWidth());
                DlssNr::NeuralUi::Checkbox("Output Scaling##os_enable", &_ssEnabled);
                ImGui::EndDisabled();

                ShowHelpMarker("Upscales the image internally to a higher output resolution\n"
                               "then downscales it back to your display resolution\n\n"
                               "Values <1.0 make the upscaler cheaper\n"
                               "Values >1.0 make image sharper at the cost of performance\n\n"
                               "If greyed out, please check Git Wiki - Unreal Engine tweaks\n\n"
                               "Target res and total ratio at the bottom (max. total 3.0!)");

                ImGui::Indent();
                ImGui::BeginDisabled(!_ssEnabled);
                {
                    // clang-format off
                    std::vector<MenuOption<Scaler>> ds_options = {
                        { Scaler::FSR1, "FSR1",
                            "Default option.\nGood enough image quality and very fast." },
                        { Scaler::Bicubic, "Bicubic",
                            "Fastest traditional option.\nProduces a very soft/blurry image, but might be okay for downscaling." },
                        { Scaler::CatmullRom, "Catmull-Rom",
                            "Designed primarily for downscaling.\nRetains good contrast with minimal artefacts, but softer than Lanczos." },
                        { Scaler::Lanczos2, "Lanczos2",
                            "Lighter and faster than Lanczos3.\nLess prone to ringing artefacts, but slightly blurrier." },
                        { Scaler::Lanczos3, "Lanczos3",
                            "Heavier version of Lanczos2.\nOffers the sharpest image, but is the most prone to ringing.\nConsidered the best along with Kaiser3." },
                        { Scaler::Kaiser2, "Kaiser2",
                            "Similar to Lanczos2.\nSmoother and less prone to artefacts than Lanczos, but slightly blurrier." },
                        { Scaler::Kaiser3, "Kaiser3",
                            "Similar to Lanczos3.\nFar less prone to artefacting than Lanczos3, but much heavier on the GPU.\nConsidered the best along with Lanczos3." },
                        { Scaler::Magic, "MAGIC",
                            "Specialised to prevent artifacts.\nEliminates harsh halos for a natural look, but can appear slightly soft." }
                    };
                    // clang-format on

                    const bool isUpsampleRatio = _ssRatio < 1.0f;
                    const std::string disabledReason = "Only FSR1 and Bicubic are supported when Ratio is below 1.0.";

                    for (auto& opt : ds_options)
                    {
                        if (isUpsampleRatio && opt.value > Scaler::Bicubic)
                            opt.set_disabled(true, opt.tooltip + "\n\n" + disabledReason);
                    }

                    if (isUpsampleRatio && _ssDownsampler > Scaler::Bicubic)
                        _ssDownsampler = Scaler::FSR1;

                    PopulateCombo("Downscaler", _ssDownsampler, ds_options);
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(!_ssEnabled || currentFeature->RenderWidth() > currentFeature->DisplayWidth());
                DlssNr::NeuralUi::FillSlider("Ratio", &_ssRatio, 0.5f, 3.0f, "%.2f");
                ImGui::EndDisabled();

                bool applyEnabled = _ssEnabled != config->OutputScalingEnabled.value_or_default() ||
                                    _ssRatio != config->OutputScalingMultiplier.value_or(defaultRatio) ||
                                    _ssDownsampler != config->OutputScalingDownscaler.value_or_default();

                ImGui::BeginDisabled(!applyEnabled);
                if (DlssNr::NeuralUi::Button("Apply Change"))
                {
                    config->OutputScalingEnabled = _ssEnabled;
                    config->OutputScalingMultiplier = _ssRatio;

                    if (_ssRatio < 1.0f && _ssDownsampler > Scaler::Bicubic)
                        _ssDownsampler = Scaler::FSR1;

                    config->OutputScalingDownscaler = _ssDownsampler;

                    const bool usesDlssd = currentFeature->GetUpscalerType() == Upscaler::DLSSD;
                    if (usesDlssd)
                        state.newBackend = Upscaler::DLSSD;
                    else
                        state.newBackend = currentBackend;

                    MARK_ALL_BACKENDS_CHANGED();
                }
                ImGui::EndDisabled();
                if (applyEnabled)
                    DlssNr::NeuralUi::DimTag("not applied yet");

                if (currentFeature != nullptr && !currentFeature->IsFrozen())
                {
                    ImGui::TextDisabled("Output Scaling is %s, Target Res: %dx%d (%.2f)\nJitter Count: %d",
                                        config->OutputScalingEnabled.value_or_default() ? "ENABLED" : "DISABLED",
                                        (uint32_t) (currentFeature->DisplayWidth() * _ssRatio),
                                        (uint32_t) (currentFeature->DisplayHeight() * _ssRatio),
                                        ((float) currentFeature->DisplayWidth() * _ssRatio) /
                                            (float) currentFeature->RenderWidth(),
                                        currentFeature->JitterCount());
                }
                ImGui::Unindent();

                ImGui::EndDisabled();
            }
        }
    }
}

// Init Flags (AMDNR 0.3.4, the approved other-tabs mock): its own function, so the Image tab draws Textures between
// Sharpness and it; one flag per row (0.3.3.2 drew two columns), each flag's help on its own label (it was on the
// "R" button before). The same controls, keys and resets.
void MenuCommon::RenderInitFlagsSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // INIT -----------------------------
        SectionHeader("Init Flags");
        {
            // AutoExposure is always enabled for XeSS with native Dx11
            bool autoExposureDisabled = state.api == API::DX11 && currentBackend == Upscaler::XeSS;
            ImGui::BeginDisabled(autoExposureDisabled);

            // "Upscaler auto exposure" (AMDNR 0.3.4), so it is not read as the Neural tab's Auto-exposure; ID unchanged.
            if (bool autoExposure = currentFeature->AutoExposure();
                DlssNr::NeuralUi::Checkbox("Upscaler auto exposure###Auto Exposure", &autoExposure))
            {
                config->AutoExposure = autoExposure;
                ReInitUpscaler();
            }
            ShowHelpMarker("Some Unreal Engine games need this\n\n"
                           "Try using if colours flickering or\n"
                           "objects have ghosting trails");
            ShowResetButton(&config->AutoExposure, "R");
            // (AMDNR 0.3.4) Unset in the ini = the game's own init flag (the mock's tag).
            if (!config->AutoExposure.has_value())
                DlssNr::NeuralUi::DimTag("from the game");

            ImGui::EndDisabled();

            auto accessToReactiveMask = currentFeature->AccessToReactiveMask();
            ImGui::BeginDisabled(!accessToReactiveMask);

            bool canUseReactiveMask =
                accessToReactiveMask && currentBackend != Upscaler::DLSS &&
                (currentBackend != Upscaler::XeSS || currentFeature->Version() >= feature_version { 2, 0, 1 });

            bool disableReactiveMask = config->DisableReactiveMask.value_or(!canUseReactiveMask);

            if (DlssNr::NeuralUi::Checkbox("Disable Reactive Mask", &disableReactiveMask))
            {
                config->DisableReactiveMask = disableReactiveMask;

                if (currentBackend == Upscaler::XeSS)
                {
                    state.newBackend = currentBackend;
                    MARK_ALL_BACKENDS_CHANGED();
                }
            }

            ImGui::EndDisabled();

            if (accessToReactiveMask)
                ShowHelpMarker("Allows the use of a Reactive mask\n"
                               "Keep in mind that a Reactive mask sent to DLSS\n"
                               "will not produce a good image in combination with FSR/XeSS");
            else
                ShowHelpMarker("Option disabled because the game doesn't provide a Reactive mask");

            if (auto ch = ScopedCollapsingHeader("Advanced Init Flags"); ch.IsHeaderOpen())
            {
                {
                    if (bool depth = currentFeature->DepthInverted(); DlssNr::NeuralUi::Checkbox("Depth Inverted", &depth))
                    {
                        config->DepthInverted = depth;
                        ReInitUpscaler();
                    }
                    ShowHelpMarker("You shouldn't need to change it");
                    ShowResetButton(&config->DepthInverted, "R##2");
                    // (AMDNR 0.3.4) Unset in the ini = the game's own init flag (the mock's tag).
                    if (!config->DepthInverted.has_value())
                        DlssNr::NeuralUi::DimTag("from the game");

                    if (bool hdr = currentFeature->IsHdr(); DlssNr::NeuralUi::Checkbox("HDR", &hdr))
                    {
                        config->HDR = hdr;
                        ReInitUpscaler();
                    }
                    ShowHelpMarker("Might help with purple hue in some games");
                    ShowResetButton(&config->HDR, "R##1");
                    // (AMDNR 0.3.4) Unset in the ini = the game's own init flag (the mock's tag).
                    if (!config->HDR.has_value())
                        DlssNr::NeuralUi::DimTag("from the game");

                    if (bool mv = !currentFeature->LowResMV(); DlssNr::NeuralUi::Checkbox("Display Res. MV", &mv))
                    {
                        config->DisplayResolution = mv;

                        // Disable output scaling when
                        // Display res MV is active
                        if (mv)
                        {
                            config->OutputScalingEnabled = false;
                            _ssEnabled = false;
                        }

                        ReInitUpscaler();
                    }
                    ShowHelpMarker("Mostly a fix for Unreal Engine games\n"
                                   "Top left part of the screen will be blurry");
                    ShowResetButton(&config->DisplayResolution, "R##4");
                    // (AMDNR 0.3.4) Unset in the ini = the game's own init flag (the mock's tag).
                    if (!config->DisplayResolution.has_value())
                        DlssNr::NeuralUi::DimTag("from the game");

                    if (bool jitter = currentFeature->JitteredMV(); DlssNr::NeuralUi::Checkbox("Jitter Cancellation", &jitter))
                    {
                        config->JitterCancellation = jitter;
                        ReInitUpscaler();
                    }
                    ShowHelpMarker("Fix for games that send motion data with preapplied jitter");
                    ShowResetButton(&config->JitterCancellation, "R##3");
                    // (AMDNR 0.3.4) Unset in the ini = the game's own init flag (the mock's tag).
                    if (!config->JitterCancellation.has_value())
                        DlssNr::NeuralUi::DimTag("from the game");
                }

                if (currentFeature->AccessToReactiveMask() && currentBackend != Upscaler::DLSS)
                {
                    ImGui::BeginDisabled(config->DisableReactiveMask.value_or(currentBackend == Upscaler::XeSS));

                    bool binaryMask = state.api == Vulkan || currentBackend == Upscaler::XeSS;
                    auto defaultBias = binaryMask ? 0.0f : 0.45f;
                    auto maskBias = config->DlssReactiveMaskBias.value_or(defaultBias);

                    if (!binaryMask)
                    {
                        if (DlssNr::NeuralUi::FillSlider("React. Mask Bias", &maskBias, 0.0f, 0.9f, "%.2f"))
                            config->DlssReactiveMaskBias = maskBias;

                        ShowHelpMarker("Values above 0 activate usage of Reactive mask");
                    }
                    else
                    {
                        bool useRM = maskBias > 0.0f;
                        if (DlssNr::NeuralUi::Checkbox("Use Binary Reactive Mask", &useRM))
                        {
                            if (useRM)
                                config->DlssReactiveMaskBias = 0.45f;
                            else
                                config->DlssReactiveMaskBias.reset();
                        }
                    }

                    ImGui::EndDisabled();
                }
            }
        }
    }
}

void MenuCommon::RenderMagnifierSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;

    // Magnifier -----------------------------
    if (auto ch = ScopedCollapsingHeader("Magnifier"); ch.IsHeaderOpen())
    {
        bool magnifierEnabled = config->MagnifierEnabled.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Enable Magnifier", &magnifierEnabled))
            config->MagnifierEnabled = magnifierEnabled;

        ImGui::BeginDisabled(!magnifierEnabled);

        float magnifierSize = config->MagnifierSize.value_or_default();
        if (DlssNr::NeuralUi::FillSlider("Size", &magnifierSize, 5.0f, 50.0f, "%.1f%% of screen"))
            config->MagnifierSize = magnifierSize;

        int zoomFactor = config->MagnifierZoomFactor.value_or_default();
        if (DlssNr::NeuralUi::FillSlider("Zoom Factor", &zoomFactor, 2, 20, "%dx"))
            config->MagnifierZoomFactor = zoomFactor;

        float borderSize = config->MagnifierBorderSize.value_or_default();
        if (DlssNr::NeuralUi::FillSlider("Border Size", &borderSize, 0.0f, 2.0f, "%.2f%% of screen"))
            config->MagnifierBorderSize = borderSize;

        ImGui::TextDisabled("%s", "Positioning");

        bool staticMode = config->MagnifierStaticPosX.has_value() && config->MagnifierStaticPosY.has_value();
        if (staticMode)
        {
            float staticX = config->MagnifierStaticPosX.value();
            if (DlssNr::NeuralUi::FillSlider("Static Pos X", &staticX, 0.0f, 100.0f, "%.1f%%"))
                config->MagnifierStaticPosX = staticX;

            float staticY = config->MagnifierStaticPosY.value();
            if (DlssNr::NeuralUi::FillSlider("Static Pos Y", &staticY, 0.0f, 100.0f, "%.1f%%"))
                config->MagnifierStaticPosY = staticY;

            if (DlssNr::NeuralUi::Button("Reset Static Position (Follow Cursor)"))
            {
                config->MagnifierStaticPosX.reset();
                config->MagnifierStaticPosY.reset();
            }
        }
        else
        {
            // Button to initialize static position mode
            if (DlssNr::NeuralUi::Button("Set Static Position"))
            {
                config->MagnifierStaticPosX = 50.0f;
                config->MagnifierStaticPosY = 50.0f;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(Currently following cursor)");

            float offsetX = config->MagnifierCursorOffsetX.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("Cursor Offset X", &offsetX, -300.0f, 300.0f, "%.0f px"))
                config->MagnifierCursorOffsetX = offsetX;

            float offsetY = config->MagnifierCursorOffsetY.value_or_default();
            if (DlssNr::NeuralUi::FillSlider("Cursor Offset Y", &offsetY, -300.0f, 300.0f, "%.0f px"))
                config->MagnifierCursorOffsetY = offsetY;
        }

        ImGui::EndDisabled();
        ImGui::Spacing();
    }
}

// A section of the Advanced and Interface tabs (AMDNR 0.3.4, X-ADV / X-IF; menu plan 5.10): the Neural tab's section
// heading (the approved mock: a red title, a 1 px rule under it), no box and no header bar to open. Rarely wanted
// groups inside a section are closed trees. (0.3.4 menu rework) Its callers call DlssNr::NeuralUi::SectionHeader directly.

void MenuCommon::RenderQuirksSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;

    // QUIRKS -----------------------------
    // Also what the window title's "(OP)" said before AMDNR 0.3.4: OptiPatcher patched this game.
    // AMDNR 0.3.4 (X-ADV): a closed tree in the tab's top block, under the intro line, with the count.
    if (state.detectedQuirks.size() > 0 || state.isOptiPatcherSucceed)
    {
        const int count = static_cast<int>(state.detectedQuirks.size()) + (state.isOptiPatcherSucceed ? 1 : 0);
        const std::string quirksLabel = StrFmt("Active Quirks (%d)###amdnr_active_quirks", count);
        if (DlssNr::NeuralUi::TreeNode(quirksLabel.c_str()))
        {
            for (const auto& quirk : state.detectedQuirks)
            {
                ImGui::TextWrapped("%s", quirk.c_str());
            }

            if (state.isOptiPatcherSucceed)
                ImGui::TextWrapped("%s", "OptiPatcher patched this game (spoofing is off unless the ini sets it)");

            ImGui::TreePop();
        }
    }
    else
    {
        // (AMDNR 0.3.4, the approved other-tabs mock) Said, not hidden, when there are none.
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", "Active Quirks: none");
    }
}

// Display (AMDNR 0.3.4, X-ADV): 0.3.3.2's "V-Sync Settings" header. The two checkboxes that could not both be ticked
// and the Reset button are one choice now, Game / On / Off, writing the same [V-Sync] ForceVsync (unset / true /
// false); the sync interval is unchanged. D3D11 and D3D12 swapchains only, as before.
void MenuCommon::RenderDisplaySettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& menuResScale = ctx.menuResScale;

    if (state.swapchainApi == Vulkan)
        return;

    SectionHeader("Display");

    const int vsyncMode = !config->ForceVsync.has_value() ? 0 : (config->ForceVsync.value() ? 1 : 2);
    bool vsyncChanged = false;

    // (AMDNR 0.3.4, the approved other-tabs mock) V-Sync is one combo, Game / On / Off (0.3.3.2's two boxes, then three
    // radio buttons in W2): the same [V-Sync] ForceVsync writes (unset / true / false).
    static const char* const kVsyncModes[] = { "Game", "On", "Off" };
    if (DlssNr::NeuralUi::BeginCombo("V-Sync", kVsyncModes[vsyncMode]))
    {
        for (int mode = 0; mode < static_cast<int>(std::size(kVsyncModes)); ++mode)
        {
            if (ImGui::Selectable(kVsyncModes[mode], vsyncMode == mode) && mode != vsyncMode)
            {
                if (mode == 0)
                    config->ForceVsync.reset();
                else
                    config->ForceVsync = mode == 1;
                vsyncChanged = true;
            }
        }
        ImGui::EndCombo();
    }
    ShowHelpMarker("Game: the game's own V-Sync setting.\n"
                   "On / Off: OptiScaler forces V-Sync on or off.");

    ImGui::BeginDisabled(vsyncMode != 1);

    // The interval with what it means (the mock's "1 - every refresh"); the same [V-Sync] SyncInterval values 0-3. An
    // ini value outside them shows as its number.
    static const char* const kIntervals[] = { "0 - no wait", "1 - every refresh", "2 - every 2nd refresh",
                                              "3 - every 3rd refresh" };
    const int interval = config->VsyncInterval.value_or_default();
    const std::string intervalPreview = (interval >= 0 && interval < static_cast<int>(std::size(kIntervals)))
                                            ? std::string(kIntervals[interval])
                                            : StrFmt("%d", interval);
    if (DlssNr::NeuralUi::BeginCombo("Sync interval", intervalPreview.c_str()))
    {
        for (int i = 0; i < static_cast<int>(std::size(kIntervals)); ++i)
        {
            if (ImGui::Selectable(kIntervals[i], interval == i))
            {
                config->VsyncInterval = i;
                vsyncChanged = true;
            }
        }

        ImGui::EndCombo();
    }

    ShowHelpMarker("Controls the DXGI Present sync interval, which determines how\n"
                   "the swap chain waits for vertical refresh (with V-Sync On).\n\n"
                   "0  = Present immediately, no VSync wait.\n"
                   "1  = Sync to every refresh, normal VSync.\n"
                   "2+ = Present every N refreshes, reducing effective frame rate.\n\n"
                   "Higher values can reduce tearing but may increase latency and cap FPS.\n"
                   "For most games, use 0 for lowest latency or 1 for normal VSync.");
    if (vsyncMode != 1)
        DlssNr::NeuralUi::DimTag("needs V-Sync On");

    ImGui::EndDisabled();

    if (vsyncChanged && state.activeFgOutput == FGOutput::XeFG && state.currentFG != nullptr)
    {
        // To prevent XeLL issues
        LOG_DEBUG("V-Sync change detected, forcing XeFG reset");
        state.WAR_xefgRequestFGToggle = true;
    }
}

// Compatibility (AMDNR 0.3.4, X-ADV): 0.3.3.2's "Advanced Settings" header, the same controls; Resource Barriers and
// Root Signatures are closed trees inside it.
void MenuCommon::RenderAdvancedSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    SectionHeader("Compatibility");

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        bool extendedLimits = config->ExtendedLimits.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Enable Extended Limits", &extendedLimits))
            config->ExtendedLimits = extendedLimits;

        ShowHelpMarker("Extended sliders limit for quality presets\n\n"
                       "Using this option changes resolution detection logic\n"
                       "and might cause issues and crashes!");
    }

    bool pcShaders = config->UsePrecompiledShaders.value_or_default();
    if (DlssNr::NeuralUi::Checkbox("Use Precompiled Shaders", &pcShaders))
    {
        config->UsePrecompiledShaders = pcShaders;
        state.newBackend = currentBackend;
        MARK_ALL_BACKENDS_CHANGED();
    }

    // DRS (AMDNR 0.3.4, the approved other-tabs mock: a closed fold with its state as the tag, like Resource Barriers;
    // 0.3.3.2 drew a dim line and a two-column table). The same two boxes and keys.
    {
        const bool drsMinOn = config->DrsMinOverrideEnabled.value_or_default();
        const bool drsMaxOn = config->DrsMaxOverrideEnabled.value_or_default();
        const bool drsOpen = DlssNr::NeuralUi::TreeNode("Dynamic Resolution Scaling (DRS)");
        DlssNr::NeuralUi::DimTag(drsMinOn && drsMaxOn ? "min and max overridden"
                                 : drsMinOn           ? "min overridden"
                                 : drsMaxOn           ? "max overridden"
                                                      : "off");
        if (drsOpen)
        {
            if (bool drsMin = drsMinOn; DlssNr::NeuralUi::Checkbox("Override Minimum", &drsMin))
                config->DrsMinOverrideEnabled = drsMin;
            ShowHelpMarker("Fix for games ignoring official DRS limits");

            if (bool drsMax = drsMaxOn; DlssNr::NeuralUi::Checkbox("Override Maximum", &drsMax))
                config->DrsMaxOverrideEnabled = drsMax;
            ShowHelpMarker("Fix for games ignoring official DRS limits");

            ImGui::TreePop();
        }
    }

    // Non-DLSS hotfixes -----------------------------
    if (currentFeature != nullptr && !currentFeature->IsFrozen() && currentBackend != Upscaler::DLSS)
    {
        // BARRIERS -----------------------------
        // (AMDNR 0.3.4, the approved other-tabs mock) The fold's tag says whether any barrier is set ("all AUTO").
        const int barriersSet =
            (config->ColorResourceBarrier.has_value() ? 1 : 0) + (config->DepthResourceBarrier.has_value() ? 1 : 0) +
            (config->MVResourceBarrier.has_value() ? 1 : 0) + (config->ExposureResourceBarrier.has_value() ? 1 : 0) +
            (config->MaskResourceBarrier.has_value() ? 1 : 0) + (config->OutputResourceBarrier.has_value() ? 1 : 0);
        const bool barriersOpen = DlssNr::NeuralUi::TreeNode("Resource Barriers");
        DlssNr::NeuralUi::DimTag(barriersSet == 0 ? "all AUTO" : StrFmt("%d of 6 set", barriersSet).c_str());
        if (barriersOpen)
        {
            AddResourceBarrier("Color", &config->ColorResourceBarrier);
            AddResourceBarrier("Depth", &config->DepthResourceBarrier);
            AddResourceBarrier("Motion", &config->MVResourceBarrier);
            AddResourceBarrier("Exposure", &config->ExposureResourceBarrier);
            AddResourceBarrier("Mask", &config->MaskResourceBarrier);
            AddResourceBarrier("Output", &config->OutputResourceBarrier);
            ImGui::TreePop();
        }

        // HOTFIXES -----------------------------
        if (state.api == DX12)
        {
            const bool rootSigOpen = DlssNr::NeuralUi::TreeNode("Root Signatures");
            DlssNr::NeuralUi::DimTag("can pause Neural Rendering on some frames");
            if (rootSigOpen)
            {
                if (bool crs = config->RestoreComputeSignature.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Restore Compute Root Signature", &crs))
                    config->RestoreComputeSignature = crs;

                if (bool grs = config->RestoreGraphicSignature.value_or_default();
                    DlssNr::NeuralUi::Checkbox("Restore Graphic Root Signature", &grs))
                    config->RestoreGraphicSignature = grs;

                // (AMDNR 0.3.4, T11h) Checked in inputs/NVNGX_DLSS_Dx12.cpp: with either restore on, a frame whose
                // root signature cannot be restored skips the upscaler, and the AMD neural pass after it is skipped
                // with it (NR history restarts, AmdBridge::UpscalerSkipped).
                ImGui::TextDisabled("Can pause Neural Rendering on some frames");
                ShowHelpMarker("With either restore on, a frame whose root signature cannot be restored\n"
                               "skips the upscaler, and Neural Rendering skips that frame with it\n"
                               "(its history restarts). Leave both off unless a game needs them.");
                ImGui::TreePop();
            }
        }
    }
}

void MenuCommon::RenderLoggingSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    // LOGGING -----------------------------
    // AMDNR 0.3.4 (X-ADV): a section of the Advanced tab, always shown. 0.3.3.2 re-applied the log level on every frame
    // its closed-by-default header was open; the same rule (the ini level while a log goes to a file, the console or
    // NGX, else off) is now applied when To File, To Console or Log Level changes, the only moments it can differ.
    SectionHeader("Logging");

    // (AMDNR 0.3.4, the approved mock / D-3) Save report, out of the header row: the section's first row.
    RenderSaveReportRow("advanced");

    auto applyLogLevel = [config]()
    {
        if (config->LogToConsole.value_or_default() || config->LogToFile.value_or_default() ||
            config->LogToNGX.value_or_default())
            spdlog::default_logger()->set_level((spdlog::level::level_enum) config->LogLevel.value_or_default());
        else
            spdlog::default_logger()->set_level(spdlog::level::off);
    };

    if (bool toFile = config->LogToFile.value_or_default(); DlssNr::NeuralUi::Checkbox("To File", &toFile))
    {
        config->LogToFile = toFile;
        PrepareLogger();
        applyLogLevel();
    }

    if (bool toConsole = config->LogToConsole.value_or_default(); DlssNr::NeuralUi::Checkbox("To Console", &toConsole))
    {
        config->LogToConsole = toConsole;
        PrepareLogger();
        applyLogLevel();
    }

    // The ini's LogLevel is read without a clamp (Config.cpp), and this section is drawn on every frame the Advanced
    // tab is open (X-ADV): a hand-set 5 or 6 (spdlog critical/off) or -1 must not index past the list.
    const char* logLevels[] = { "Trace", "Debug", "Information", "Warning", "Error" };
    const int currentLevel = config->LogLevel.value_or_default();
    const char* selectedLevel =
        (currentLevel >= 0 && currentLevel < IM_ARRAYSIZE(logLevels)) ? logLevels[currentLevel] : "Custom";

    if (DlssNr::NeuralUi::BeginCombo("Log Level", selectedLevel))
    {
        for (int n = 0; n < 5; n++)
        {
            if (ImGui::Selectable(logLevels[n], (config->LogLevel.value_or_default() == n)))
            {
                config->LogLevel = n;
                applyLogLevel();
            }
        }

        ImGui::EndCombo();
    }

    // The build stamp (AMDNR 0.3.4, menu decision D-2): it left the title bar for the header's "AMDNR" hover, and it is
    // here too, where a tester's screenshot of the Advanced tab shows it.
    ImGui::TextDisabled("AMDNR %s, build %s (%s)", AMDNR_VERSION_STR, VER_BUILD_DATE, VER_BUILD_COMMIT);
}

// RenderThemeSettings is gone, and with it the whole "Menu Theme and Color" panel:
// roughly 450 lines of accent pickers, light/dark toggle, custom background colour
// and alpha. The theme is fixed - red on black, decided once in ApplyThemeStyle -
// so every control in there wrote a setting that the next call to ApplyThemeStyle
// overwrote. A panel of controls that cannot change anything is worse than no panel:
// it costs a section of the menu and teaches the reader that settings here do not work.

void MenuCommon::RenderFpsOverlaySettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    // FPS OVERLAY -----------------------------
    // AMDNR 0.3.4 (X-IF, menu plan 5.10): a section of the Interface tab (no header bar to open): Enabled, Overlay Type
    // and Overlay Position, the rest under a closed "More overlay options" tree. Same keys and values as 0.3.3.2.
    SectionHeader("FPS Overlay");

    // (AMDNR 0.3.4, the approved other-tabs mock) "FPS Overlay Enabled" with its in-game key as the tag.
    bool fpsEnabled = config->ShowFps.value_or_default();
    if (DlssNr::NeuralUi::Checkbox("FPS Overlay Enabled##fps_overlay", &fpsEnabled))
        config->ShowFps = fpsEnabled;
    if (const int fpsKey = config->FpsShortcutKey.value_or_default(); fpsKey != UnboundKey)
        DlssNr::NeuralUi::DimTag((KeyName(fpsKey) + " toggles it in game").c_str());

    const char* fpsType[] = { "Just FPS", "Simple",       "Detailed",      "Detailed + Graph",
                              "Full",     "Full + Graph", "Reflex timings" };
    const char* selectedType = fpsType[config->FpsOverlayType.value_or_default()];

    if (DlssNr::NeuralUi::BeginCombo("Overlay Type", selectedType))
    {
        for (int n = 0; n < std::size(fpsType); n++)
        {
            if (ImGui::Selectable(fpsType[n], (config->FpsOverlayType.value_or_default() == n)))
                config->FpsOverlayType = (FpsOverlay) n;
        }

        ImGui::EndCombo();
    }

    const char* fpsPosition[] = { "Top Left", "Top Right", "Bottom Left", "Bottom Right" };
    const char* selectedPosition = fpsPosition[config->FpsOverlayPosition.value_or_default()];

    if (DlssNr::NeuralUi::BeginCombo("Overlay Position", selectedPosition))
    {
        for (int n = 0; n < std::size(fpsPosition); n++)
        {
            if (ImGui::Selectable(fpsPosition[n], (config->FpsOverlayPosition.value_or_default() == n)))
                config->FpsOverlayPosition = (FpsOverlayPos) n;
        }

        ImGui::EndCombo();
    }

    if (DlssNr::NeuralUi::TreeNode("More overlay options"))
    {
        bool fpsHorizontal = config->FpsOverlayHorizontal.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Horizontal", &fpsHorizontal))
            config->FpsOverlayHorizontal = fpsHorizontal;

        float fpsAlpha = config->FpsOverlayAlpha.value_or_default();
        if (DlssNr::NeuralUi::FillSlider("Background Alpha", &fpsAlpha, 0.0f, 1.0f, "%.2f"))
            config->FpsOverlayAlpha = fpsAlpha;

        const char* options[] = { "Same as menu", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1", "1.2",
                                  "1.3",          "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0" };
        int currentIndex = std::max(((int) (config->FpsScale.value_or(0.0f) * 10.0f)) - 4, 0);
        float values[] = { 0.0f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f,
                           1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f };

        if (DlssNr::NeuralUi::FillSlider("Scale", &currentIndex, 0, IM_ARRAYSIZE(options) - 1, options[currentIndex],
                             ImGuiSliderFlags_ClampOnInput))
        {
            if (currentIndex == 0)
                config->FpsScale.reset();
            else
                config->FpsScale = values[currentIndex];
        }

        bool useTheme = config->OverlaysUseTheme.value_or_default();
        if (DlssNr::NeuralUi::Checkbox("Use Theme Colors", &useTheme))
            config->OverlaysUseTheme = useTheme;

        ImGui::TreePop();
    }
}

void MenuCommon::RenderUpscalerInputsSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    // UPSCALER INPUTS -----------------------------
    auto uiStateOpen = currentFeature == nullptr || currentFeature->IsFrozen();
    if (auto ch = ScopedCollapsingHeader("Upscaler Inputs", uiStateOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        ch.IsHeaderOpen())
    {
        if (config->EnableFsr2Inputs.value_or_default())
        {
            bool fsr2Inputs = config->UseFsr2Inputs.value_or_default();
            bool fsr2Pattern = config->Fsr2Pattern.value_or_default();

            if (DlssNr::NeuralUi::Checkbox("Use Fsr2 Inputs", &fsr2Inputs))
                config->UseFsr2Inputs = fsr2Inputs;

            if (DlssNr::NeuralUi::Checkbox("Use Fsr2 Pattern Matching", &fsr2Pattern))
                config->Fsr2Pattern = fsr2Pattern;
            ShowTooltip("This setting will become active on next boot!");
            DlssNr::NeuralUi::DimTag("next game start");
        }

        if (config->EnableFsr3Inputs.value_or_default())
        {
            bool fsr3Inputs = config->UseFsr3Inputs.value_or_default();
            bool fsr3Pattern = config->Fsr3Pattern.value_or_default();

            if (DlssNr::NeuralUi::Checkbox("Use Fsr3 Inputs", &fsr3Inputs))
                config->UseFsr3Inputs = fsr3Inputs;

            if (DlssNr::NeuralUi::Checkbox("Use Fsr3 Pattern Matching", &fsr3Pattern))
                config->Fsr3Pattern = fsr3Pattern;
            ShowTooltip("This setting will become active on next boot!");
            DlssNr::NeuralUi::DimTag("next game start");
        }

        if (config->EnableFfxInputs.value_or_default())
        {
            bool ffxInputs = config->UseFfxInputs.value_or_default();

            if (DlssNr::NeuralUi::Checkbox("Use Ffx Inputs", &ffxInputs))
                config->UseFfxInputs = ffxInputs;
        }
    }
}

void MenuCommon::RenderApiAndTextureSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    // DX11 & DX12 -----------------------------
    if (state.swapchainApi != Vulkan)
    {
        // V-Sync moved to the Display section (RenderDisplaySettings, AMDNR 0.3.4 X-ADV). (AMDNR 0.3.4, the approved
        // other-tabs mock) The Textures section is on the Image tab now: Anisotropic Filtering, then Mipmap Bias, closed
        // folds (open by default while no upscaler runs, like 0.3.3.2's headers) whose tag is the value in force.
        SectionHeader("Textures");

        // MIPMAP BIAS & Anisotropy -----------------------------
        const ImGuiTreeNodeFlags texturesOpen =
            (currentFeature == nullptr || currentFeature->IsFrozen()) ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        auto selectedAF =
            config->AnisotropyOverride.has_value() ? std::to_string(config->AnisotropyOverride.value()) : "Auto";
        const bool afOpen = DlssNr::NeuralUi::TreeNode("Anisotropic Filtering", texturesOpen);
        DlssNr::NeuralUi::DimTag(config->AnisotropyOverride.has_value() ? (selectedAF + "x").c_str() : "Auto");
        if (afOpen)
        {
            if (DlssNr::NeuralUi::BeginCombo("Force Anisotropic Filtering", selectedAF.c_str()))
            {
                if (ImGui::Selectable("Auto", !config->AnisotropyOverride.has_value()))
                    config->AnisotropyOverride.reset();

                if (ImGui::Selectable("1", config->AnisotropyOverride.value_or(0) == 1))
                    config->AnisotropyOverride = 1;

                if (ImGui::Selectable("2", config->AnisotropyOverride.value_or(0) == 2))
                    config->AnisotropyOverride = 2;

                if (ImGui::Selectable("4", config->AnisotropyOverride.value_or(0) == 4))
                    config->AnisotropyOverride = 4;

                if (ImGui::Selectable("8", config->AnisotropyOverride.value_or(0) == 8))
                    config->AnisotropyOverride = 8;

                if (ImGui::Selectable("16", config->AnisotropyOverride.value_or(0) == 16))
                    config->AnisotropyOverride = 16;

                ImGui::EndCombo();
            }

            bool afComp = config->AnisotropyModifyComp.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Modify Compare", &afComp))
                config->AnisotropyModifyComp = afComp;

            ShowHelpMarker("Update comparison filters");

            bool afMinMax = config->AnisotropyModifyMinMax.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Modify Min/Max", &afMinMax))
                config->AnisotropyModifyMinMax = afMinMax;

            ShowHelpMarker("Update min/max filters");

            bool afSkipPoint = config->AnisotropySkipPointFilter.value_or_default();
            if (DlssNr::NeuralUi::Checkbox("Skip Point Filters", &afSkipPoint))
                config->AnisotropySkipPointFilter = afSkipPoint;

            ShowHelpMarker("Skip updating of point filters");

            ImGui::TextDisabled("%s", "May apply only after a resolution or preset change.");
            ImGui::TreePop();
        }

        const bool mipOpen = DlssNr::NeuralUi::TreeNode("Mipmap Bias", texturesOpen);
        DlssNr::NeuralUi::DimTag(config->MipmapBiasOverride.has_value()
                                     ? StrFmt("%.3f", config->MipmapBiasOverride.value()).c_str()
                                     : "off");
        if (mipOpen)
        {
            if (config->MipmapBiasOverride.has_value() && _mipBias == 0.0f)
                _mipBias = config->MipmapBiasOverride.value();

            DlssNr::NeuralUi::FillSlider("Mipmap Bias##2", &_mipBias, -15.0f, 15.0f, "%.3f");
            ShowHelpMarker("Can help with blurry textures in broken games\n"
                           "Negative values will make textures sharper\n"
                           "Positive values will make textures more blurry\n\n"
                           "Has a small performance impact");

            ImGui::BeginDisabled(!config->MipmapBiasOverride.has_value());
            {
                ImGui::BeginDisabled(config->MipmapBiasScaleOverride.has_value() &&
                                     config->MipmapBiasScaleOverride.value());
                {
                    bool mbFixed = config->MipmapBiasFixedOverride.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("MB Fixed Override", &mbFixed))
                    {
                        config->MipmapBiasScaleOverride.reset();
                        config->MipmapBiasFixedOverride = mbFixed;
                    }

                    ShowHelpMarker("Apply same override value to all textures");
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(config->MipmapBiasFixedOverride.has_value() &&
                                     config->MipmapBiasFixedOverride.value());
                {
                    bool mbScale = config->MipmapBiasScaleOverride.value_or_default();
                    if (DlssNr::NeuralUi::Checkbox("MB Scale Override", &mbScale))
                    {
                        config->MipmapBiasFixedOverride.reset();
                        config->MipmapBiasScaleOverride = mbScale;
                    }

                    ShowHelpMarker("Apply override value as scale multiplier\n"
                                   "When using scale mode, please use positive\n"
                                   "override values to increase sharpness!");
                }
                ImGui::EndDisabled();

                bool mbAll = config->MipmapBiasOverrideAll.value_or_default();
                if (DlssNr::NeuralUi::Checkbox("MB Override All Textures", &mbAll))
                    config->MipmapBiasOverrideAll = mbAll;

                ShowHelpMarker("Override all textures mipmap values\n"
                               "Normally OptiScaler only overrides\n"
                               "below zero mipmap values!");
            }
            ImGui::EndDisabled();

            ImGui::BeginDisabled(config->MipmapBiasOverride.has_value() &&
                                 config->MipmapBiasOverride.value() == _mipBias);
            {
                if (DlssNr::NeuralUi::Button("Set"))
                {
                    config->MipmapBiasOverride = _mipBias;
                    state.lastMipBias = 100.0f;
                    state.lastMipBiasMax = -100.0f;
                }
            }
            ImGui::EndDisabled();

            ImGui::SameLine(0.0f, 6.0f);

            ImGui::BeginDisabled(!config->MipmapBiasOverride.has_value());
            {
                if (DlssNr::NeuralUi::Button("Reset"))
                {
                    config->MipmapBiasOverride.reset();
                    _mipBias = 0.0f;
                    state.lastMipBias = 100.0f;
                    state.lastMipBiasMax = -100.0f;
                }
            }
            ImGui::EndDisabled();

            if (currentFeature != nullptr && !currentFeature->IsFrozen())
            {
                ImGui::SameLine(0.0f, 6.0f);

                if (DlssNr::NeuralUi::Button("Calculate Mipmap Bias"))
                    _showMipmapCalcWindow = true;
            }

            if (config->MipmapBiasOverride.has_value())
            {
                if (config->MipmapBiasFixedOverride.value_or_default())
                {
                    ImGui::TextDisabled("Current : %.3f / %.3f, Target: %.3f", state.lastMipBias,
                                        state.lastMipBiasMax, config->MipmapBiasOverride.value());
                }
                else if (config->MipmapBiasScaleOverride.value_or_default())
                {
                    ImGui::TextDisabled("Current : %.3f / %.3f, Target: Base * %.3f", state.lastMipBias,
                                        state.lastMipBiasMax, config->MipmapBiasOverride.value());
                }
                else
                {
                    ImGui::TextDisabled("Current : %.3f / %.3f, Target: Base + %.3f", state.lastMipBias,
                                        state.lastMipBiasMax, config->MipmapBiasOverride.value());
                }
            }
            else
            {
                ImGui::TextDisabled("Current : %.3f / %.3f", state.lastMipBias, state.lastMipBiasMax);
            }

            ImGui::TextDisabled("%s", "Applies after a resolution or preset change.");
            ImGui::TreePop();
        }
    }
}

void MenuCommon::RenderKeybindSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    // AMDNR 0.3.4 (X-IF, menu plan 5.10): a section of the Interface tab; one dim line of instructions, then one row per
    // action, the key button left of its name (Menu, Neural Rendering, Frame Generation, FPS Overlay, FPS Overlay
    // Cycle). Same keys, same capture.
    SectionHeader("Keybinds");
    ImGui::TextDisabled("Click a key, then press the new one: Escape cancels, Backspace unbinds. One key per action.");

    static auto menu = Keybind("Menu", 10);
    static auto fpsOverlay = Keybind("FPS Overlay", 11);
    static auto fpsOverlayCycle = Keybind("FPS Overlay Cycle", 12);
    static auto fgEnable = Keybind("Frame Generation", 13);
    static auto dlssNrToggle = Keybind("Neural Rendering", 14);

    menu.RenderRow(config->ShortcutKey);
    dlssNrToggle.RenderRow(config->DlssNrToggleKey, "also beside Enable on the Neural tab");
    fgEnable.RenderRow(config->FGShortcutKey, "needs FG Input and FG Output");
    fpsOverlay.RenderRow(config->FpsShortcutKey);
    fpsOverlayCycle.RenderRow(config->FpsCycleShortcutKey);
}

// THE NEURAL RUNTIME CHOOSER. Two runtimes can carry DLSS Neural Rendering on AMD: danielblnc's
// closed runtime and lmxxf's open-source HIP runtime. The first launch that finds either
// one installed asks which, once, and records the answer as [DlssNr] NrBackend; the Neural tab
// changes it later. A modal, because the answer changes which files the pass loads.
// AMDNR 0.3.4: one radio row per runtime (its credit, installed or not, runs on this GPU or not), the file
// names in the row's hover, no runtime versions. The rows come from the runtime table (RuntimeCaps::All(): name,
// credit, ini value, files check, GPU check); a future runtime is one more table row.
// AMDNR 0.3.4.2 (Assetto Corsa: the chooser opened by itself during loading and could not be answered at 3-10 fps):
// the rules live in menu/RuntimeChooserSession.h. The row of the runtime running now is preselected until the player
// picks, so "Use this runtime" works at once; keys 1 / 2, Enter and Esc work at any frame rate (UpdateManualInput);
// the title bar X, Esc and closing the menu all mean "Decide later".
static void ApplyChooserUse(Config* config, int row, const char* how)
{
    const auto runtimes = DlssNr::RuntimeCaps::All();
    if (row < 0 || row >= static_cast<int>(runtimes.size()))
        return;

    config->DlssNrBackend = std::string(runtimes[row].iniValue); // "daniel" / "lmxxf", as before
    config->SaveIni();
    LOG_INFO("runtime chooser: {} chosen ({}); [DlssNr] NrBackend={} saved", runtimes[row].name, how,
             runtimes[row].iniValue);
}

static void RenderNeuralRuntimeChooser(Config* config)
{
    const DWORD nowTick = GetTickCount();
    const bool needed = DlssNr::AmdBridge::RuntimeChoiceNeeded();

    if (!chooserSession.NeedsRender(needed))
    {
        // Answered, or no longer needed: ImGui must not keep the modal open unseen, as an open modal blocks every
        // click on the menu behind it.
        if (chooserSession.opened && ImGui::IsPopupOpen(kChooserTitle))
        {
            if (ImGui::BeginPopupModal(kChooserTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        }
        chooserSession.OnFrame(false, nowTick);
        return;
    }

    if (chooserSession.OpenOnce())
    {
        LOG_INFO("runtime chooser: shown (files for both AMD neural runtimes present, [DlssNr] NrBackend not "
                 "set); keys: 1 / 2 pick, Enter use, Esc or the menu key decide later");
    }

    if (!ImGui::IsPopupOpen(kChooserTitle))
    {
        if (!chooserSession.Wanted(needed))
        {
            // An Esc answer for a popup that is already gone: nothing to close.
            chooserSession.TakeClose();
            chooserSession.usePending = false;
            chooserSession.OnFrame(false, nowTick);
            return;
        }
        ImGui::OpenPopup(kChooserTitle);
    }

    bool keepOpen = true; // the title bar X
    if (!ImGui::BeginPopupModal(kChooserTitle, &keepOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!keepOpen && chooserSession.DecideLater())
        {
            LOG_INFO("runtime chooser: decided later ({}); not shown again this session",
                     MenuUi::LaterReasonName(MenuUi::LaterReason::TitleBarX));
        }
        chooserSession.OnFrame(false, nowTick);
        return;
    }
    const auto& g = DlssNr::AmdBridge::GpuSupportInfo();
    const bool gpuKnown = g.amd && g.known;
    const bool lmxxfRdna3 = g.lmxxfOk && g.target.rfind("gfx11", 0) == 0;

    struct ChooserRow
    {
        std::string label;  // radio label (unique in this popup)
        std::string credit; // who made it
        const char* files;  // hover: what it loads and from where
        bool installed;     // what "Use this runtime" requires
        std::string state;  // installed / not installed / partly installed
        bool gpuOk;         // runs on this GPU (only said when the GPU is known)
        bool experimental = false; // lmxxf on a handheld APU (GpuSupport::lmxxfExperimental): runs, slow, untested
    };
    const auto runtimes = DlssNr::RuntimeCaps::All();
    std::vector<ChooserRow> rows;
    rows.reserve(runtimes.size());
    for (const auto& r : runtimes)
    {
        const bool lmxxf = r.id == DlssNr::AmdBridge::NeuralRuntime::Lmxxf;
        const bool complete = r.installed();
        ChooserRow row;
        row.label = std::string(r.name) + " runtime";
        row.credit = r.credit;
        // lmxxf on RDNA 3 runs AMDNR's own backend: its credit stays on the row (0.3.3.2's chooser had it).
        if (lmxxf && lmxxfRdna3)
            row.credit += " - RDNA 3 backend by 3zwr1 (AMDNR)";
        row.files = lmxxf ? "lmxxf's open-source HIP runtime: LmxxfNrRuntime.dll beside OptiScaler.dll and\n"
                            "LmxxfNrRuntime.pak beside it (or DLSS5-AMD\\native-game-tiled-assets next to the game).\n"
                            "Its edit is applied one frame late, carried by the motion vectors,\n"
                            "so the frame never waits for the network."
                          : "danielblnc's closed runtime: dlssnr_amd_pass1..3.dll and dlssnr_on_amd_weights.bin\n"
                            "beside OptiScaler.dll.";
        // What can be picked is unchanged from 0.3.3.2: danielblnc's pass DLL, or lmxxf's assets (the chooser only
        // opens when both are there); a missing LmxxfNrRuntime.dll is named in the row.
        row.installed = lmxxf ? DlssNr::AmdBridge::LmxxfAssetsPresent() : complete;
        row.state = complete ? std::string("installed")
                    : row.installed ? r.missing()
                                    : "not installed (" + r.missing() + ")";
        row.gpuOk = r.gpuOk(g);
        row.experimental = lmxxf && g.lmxxfExperimental; // runtime work HB request 7 (C8)
        rows.push_back(std::move(row));
    }

    bool installed[MenuUi::RuntimeChooserSession::kMaxRows] {};
    for (int i = 0; i < static_cast<int>(std::size(rows)) && i < MenuUi::RuntimeChooserSession::kMaxRows; ++i)
        installed[i] = rows[i].installed;
    chooserSession.SetRows(static_cast<int>(std::size(rows)), installed);

    // The runtime this process runs now (read only; danielblnc in the Assetto Corsa logs) is preselected.
    const auto active = DlssNr::AmdBridge::ActiveRuntime();
    int activeRow = -1;
    for (int i = 0; i < static_cast<int>(runtimes.size()); ++i)
    {
        if (active != DlssNr::AmdBridge::NeuralRuntime::Unchosen && runtimes[i].id == active)
            activeRow = i;
    }
    chooserSession.SetPreselect(activeRow);

    // Answers given by key (UpdateManualInput) are applied before the rows are drawn.
    if (const int row = chooserSession.TakeUse(false); row >= 0 || chooserSession.TakeClose())
    {
        if (row >= 0)
            ApplyChooserUse(config, row, "Enter key");
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        chooserSession.OnFrame(false, nowTick);
        return;
    }
    chooserSession.OnFrame(true, nowTick);

    ImGui::TextUnformatted("Two AMD neural runtimes can carry DLSS Neural Rendering. Pick one.");
    ImGui::TextDisabled("You can change this later under Neural > Neural runtime.");
    ImGui::Spacing();
    const int effectivePick = chooserSession.EffectivePick();
    for (int i = 0; i < static_cast<int>(std::size(rows)); ++i)
    {
        const ChooserRow& row = rows[i];
        // A fixed ID (###): the "(running now)" tag can appear while the row is being clicked.
        const std::string radioLabel =
            row.label + (i == activeRow ? " (running now)" : "") + "###chooser_row_" + std::to_string(i);
        ImGui::BeginGroup();
        if (ImGui::RadioButton(radioLabel.c_str(), effectivePick == i) && chooserSession.PickRow(i))
            LOG_INFO("runtime chooser: row {} ({}) picked by click", i + 1, runtimes[i].name);
        ImGui::SameLine();
        ImGui::TextDisabled("%s - %s%s%s", row.credit.c_str(), row.state.c_str(),
                            !gpuKnown ? "" : (row.gpuOk ? ", runs on this GPU" : ", does not run on this GPU"),
                            gpuKnown && row.gpuOk && row.experimental ? " (experimental)" : "");
        ImGui::EndGroup();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", row.files);
    }
    if (gpuKnown)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("This GPU: %s (%s)", g.name.c_str(), g.target.c_str());
        if (!g.danielOk && !g.lmxxfOk)
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
            ImGui::TextColored(ImVec4(1.f, 0.55f, 0.2f, 1.f), "%s", g.note.c_str());
            ImGui::PopTextWrapPos();
        }
    }
    ImGui::Spacing();
    // AMDNR 0.3.4.2: the keys work at any frame rate; clicks can be missed at a few frames per second.
    ImGui::TextDisabled("Keys: 1 / 2 pick, Enter use, Esc or %s decide later",
                        MenuCommon::KeyName(config->ShortcutKey.value_or_default()).c_str());
    const float menuFps = ImGui::GetIO().Framerate;
    if (menuFps > 0.0f && menuFps < 20.0f)
        ImGui::TextDisabled("%s", "Low frame rate: use the keys if clicks are missed");
    ImGui::Spacing();
    const bool can = chooserSession.CanUse();
    ImGui::BeginDisabled(!can);
    if (DlssNr::NeuralUi::Button("Use this runtime", ImVec2(ImGui::GetFontSize() * 13.0f, 0.0f)) && can)
    {
        if (const int row = chooserSession.TakeUse(true); row >= 0)
        {
            ApplyChooserUse(config, row, "button");
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (DlssNr::NeuralUi::Button("Decide later", ImVec2(ImGui::GetFontSize() * 10.0f, 0.0f)))
    {
        if (chooserSession.DecideLater())
        {
            LOG_INFO("runtime chooser: decided later ({}); not shown again this session",
                     MenuUi::LaterReasonName(MenuUi::LaterReason::Button));
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// T11g (AMDNR 0.3.4, menu plan 6.7): a tab another tab asked to open (RequestTab), applied to the tab bar below with
// ImGuiTabItemFlags_SetSelected and cleared once the bar has been drawn. -1 = none.
static int pendingMenuTab = -1;

void MenuCommon::RequestTab(MenuTab tab) { pendingMenuTab = static_cast<int>(tab); }

void MenuCommon::RenderMainMenuTable(RenderMenuContext& ctx)
{
    RenderNeuralRuntimeChooser(ctx.config);
    // Tabs rather than one long scroll of collapsing headers.
    //
    // The old layout put every section into a two-column table, so the window was a
    // single page you scrolled through hunting for a heading, and something you touch
    // once a year (log levels, resource barriers) sat at the same level as something
    // you touch every session. Grouping by what you came to do, and pushing the rest
    // behind an Advanced tab, is most of what makes a settings page feel designed
    // rather than accumulated - more than any amount of restyling does.
    //
    // Nothing is removed; everything that was reachable before is still reachable, just
    // sorted by how often it is actually wanted.

    // Controls stop stretching to the full window width, which is what made the old
    // full-width sliders awkward to set precisely on a wide screen. (AMDNR 0.3.4, the approved mock) One control width
    // on every tab (R7): the mock's 190 px at Menu Scale 1.0.
    const float itemWidth = DlssNr::NeuralUi::ControlWidth();

    // (AMDNR 0.3.4, the approved mock) Text-only tabs: no tab fill; the open tab's label white with a 2 px red
    // underline 12 px under the labels' cap tops (under the text line, above the tab's bottom padding), the others dim;
    // a 1 px rule under the row across the width, 5 px under the underline, the tab's body 12 px under the rule; the
    // tab labels 24 px under the credits line. ImGui's tab bar still does the rest (selection, RequestTab,
    // keyboard/gamepad). The bar reads FramePadding here only.
    const float px = MenuCommon::MockPx();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImFloor(2.0f * px));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, ImFloor(2.0f * px)));
    const bool tabBarOpen = ImGui::BeginTabBar("opti_main", ImGuiTabBarFlags_None);
    ImGui::PopStyleVar();
    if (!tabBarOpen)
        return;
    ImGuiTabBar* tabBar = ImGui::GetCurrentTabBar();
    // ImDrawList::AddLine adds its own half pixel, so a whole y draws one sharp pixel row (0.3.4 MENU match1: a
    // second half pixel here drew it 2 px wide and dim).
    const float ruleY = ImFloor(tabBar->BarRect.Max.y + 5.0f * px);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(tabBar->BarRect.Min.x, ruleY), ImVec2(tabBar->BarRect.Max.x, ruleY),
                                        ThemeColorU32(MenuColor::Rule), 1.0f);

    // The tab a RequestTab() asked for before this frame's tab bar, selected by its BeginTabItem below (a request made
    // while a tab's contents are drawn is applied on the next frame).
    const int requestedTab = pendingMenuTab;
    auto tabFlags = [requestedTab](MenuTab tab)
    { return requestedTab == static_cast<int>(tab) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None; };
    // One tab: its label white while it is the open one (the bar's choice of the last frame), else dim; when open, the
    // underline under its label and the gap down to the body.
    auto beginTab = [&](const char* label, MenuTab tab)
    {
        const bool wasSelected = tabBar->SelectedTabId == ImGui::GetID(label);
        ImGui::PushStyleColor(ImGuiCol_Text, ThemeColor(wasSelected ? MenuColor::White : MenuColor::Dim));
        // A tab's width is its label (+1 px) and the 8 px between tabs, as the mock's 8-9 px gaps.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, ImFloor(2.0f * px)));
        const bool open = ImGui::BeginTabItem(label, nullptr, tabFlags(tab));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        if (open)
        {
            const ImVec2 mn = ImGui::GetItemRectMin();
            const ImVec2 mx = ImGui::GetItemRectMax();
            const float ulY = mx.y - ImFloor(2.0f * px); // the tab's bottom padding (its FramePadding.y) is below it
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(mn.x, ulY),
                                                      ImVec2(mx.x, ulY + ImMax(1.0f, ImFloor(2.0f * px))),
                                                      ThemeColorU32(MenuColor::CheckOn));
            ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, ruleY + ImFloor(12.0f * px)));
        }
        return open;
    };

    // --- Neural: this fork's headline feature, so it opens first ------------------
    if (beginTab("Neural", MenuTab::Neural))
    {
        // One item width on every tab (AMDNR 0.3.4, menu plan R7): the Neural tab now pushes the same width as the
        // tabs below (0.3.3.2 left it at ImGui's default, and its rows set fixed 230/260 px widths instead). Not on an
        // NVIDIA GPU without AMD runtime files, where the tab draws the NVIDIA DLSS-NR layout, which keeps its 0.3.3.2
        // widths. (G2) With runtime files present (the AMDNR package unzipped as is) an NVIDIA GPU gets the AMD
        // layout too (DlssNr::RenderMenu), whose rows no longer set fixed widths, so it gets the push as well.
        const bool neuralItemWidth = ctx.primaryGpu == nullptr || ctx.primaryGpu->vendorId != VendorId::Nvidia ||
                                     DlssNr::AmdBridge::AnyRuntimePresent();
        if (neuralItemWidth)
            ImGui::PushItemWidth(itemWidth);
        DlssNr::RenderMenu(ctx.config, ctx.menuResScale);
        if (neuralItemWidth)
            ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // --- Upscaling ---------------------------------------------------------------
    // (AMDNR 0.3.4, the approved other-tabs mock) The upscaler and its options, then Render resolution (Upscale Ratio
    // Override and Output Scaling, on the Image tab until 0.3.4), then the Upscaler Inputs fold.
    if (beginTab("Upscaling", MenuTab::Upscaling))
    {
        ImGui::PushItemWidth(itemWidth);
        RenderActiveUpscalerSettings(ctx);
        RenderFsrCommonSettings(ctx);
        RenderRenderResolutionSettings(ctx);
        RenderUpscalerInputsSettings(ctx);
        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // --- Frame generation and everything that decides frame pacing ----------------
    if (beginTab("Frame Gen", MenuTab::FrameGen))
    {
        ImGui::PushItemWidth(itemWidth);
        RenderFrameGenerationSelection(ctx);
        RenderFrameGenerationRuntimeSettings(ctx);
        RenderFramerateSettings(ctx);
#ifdef LOW_LATENCY_INPUTS
        RenderLowLatencySettings(ctx);
#else
        RenderFakenvapiSettings(ctx);
#endif
        RenderFrameGenerationCompatibility(ctx);
        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // --- Image: what the picture looks like once everything else has run ----------
    // (AMDNR 0.3.4, the approved other-tabs mock) Sharpness, Textures (Anisotropic Filtering and Mipmap Bias, on the
    // Advanced tab until 0.3.4), Init Flags, then the Magnifier fold.
    if (beginTab("Image", MenuTab::Image))
    {
        ImGui::PushItemWidth(itemWidth);
        RenderActiveImageSettings(ctx);
        RenderApiAndTextureSettings(ctx);
        RenderInitFlagsSettings(ctx);
        RenderMagnifierSettings(ctx);
        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // --- Interface: the menu's own appearance, the overlay, the keys --------------
    if (beginTab("Interface", MenuTab::Interface))
    {
        ImGui::PushItemWidth(itemWidth);
        RenderFpsOverlaySettings(ctx);
        RenderKeybindSettings(ctx);
        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // --- Advanced: correct to expose, rarely correct to touch ---------------------
    if (beginTab("Advanced", MenuTab::Advanced))
    {
        // AMDNR 0.3.4 (X-ADV, menu plan 5.10; the approved other-tabs mock): one intro line; the top block holds the
        // active quirks, then the sections Display, Compatibility and Logging (Textures moved to the Image tab, as the
        // mock has it). The component summary (S13, the owner's pick C + A) is in the header on every tab since owner
        // decision 4 (RenderMainMenuHeaderMessages).
        ImGui::TextDisabled("%s", "Compatibility fixes and diagnostics. The defaults suit almost every game.");
        ImGui::PushItemWidth(itemWidth);
        RenderQuirksSettings(ctx);
        RenderDisplaySettings(ctx);
        RenderAdvancedSettings(ctx);
        RenderLoggingSettings(ctx);
        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // The request has been handed to its tab (a SetSelected flag acts on the bar's next frame).
    if (requestedTab >= 0 && pendingMenuTab == requestedTab)
        pendingMenuTab = -1;

    ImGui::EndTabBar();
}

void MenuCommon::RenderMainMenuGraphs(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto& currentFeature = ctx.currentFeature;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;

    // (AMDNR 0.3.4, the approved mock) The footer is one line: a 1 px rule, then "FrameTime ~~~ 15.3 ms" on the left
    // and Menu Scale / Save Settings / Close on the right (RenderMainMenuBottomBar). What the two graphs, the
    // resolution line and the frame count showed until 0.3.4 is in the FrameTime hover.
    const float px = MenuCommon::MockPx();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImFloor(6.0f * px));
    {
        const float y = ImFloor(ImGui::GetCursorScreenPos().y); // AddLine adds the half pixel (one sharp row)
        const float x0 = ImGui::GetCursorScreenPos().x;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(x0, y), ImVec2(x0 + ImGui::GetContentRegionAvail().x, y),
                                            ThemeColorU32(MenuColor::Rule), 1.0f);
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImFloor(7.0f * px));

    ImGui::BeginGroup();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", "FrameTime");
    ImGui::SameLine();
    // The sparkline: text-high, no frame, in the dim colour.
    const float lineY = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(lineY + ImGui::GetStyle().FramePadding.y);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ThemeColor(MenuColor::Dim));
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ThemeColor(MenuColor::Text));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PlotLines(
        "##frametime", [](void* rb, int idx) -> float { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); },
        &gFrameTimes, plotWidth, 0, nullptr, 0.0f, std::max(2.0f * gFrameTimes.Average(), 1.0f),
        ImVec2(ImFloor(41.0f * px), ImGui::GetFontSize()));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetCursorPosY(lineY);
    ImGui::TextDisabled("%.1f ms", frameTime);
    ImGui::EndGroup();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
        ImGui::BeginTooltip();
        ImGui::Text("Frame time %.2f ms / %.1f fps", frameTime, frameRate);

        if (currentFeature != nullptr && !currentFeature->IsFrozen())
        {
            if (!state.upscaleTimes.empty())
                ImGui::Text("Upscaler %.2f ms", state.upscaleTimes.back());
            ImGui::PlotLines(
                "##upscaler", [](void* rb, int idx) -> float
                { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); }, &gUpscalerTimes, plotWidth, 0,
                nullptr, 0.0f, 20.0f, ImVec2(ImGui::GetFontSize() * 20.0f, ImGui::GetFontSize() * 2.5f));
            ImGui::Text("%dx%d -> %dx%d (%.1f) [%dx%d (%.1f)]  frame %d", currentFeature->RenderWidth(),
                        currentFeature->RenderHeight(), currentFeature->TargetWidth(), currentFeature->TargetHeight(),
                        (float) currentFeature->TargetWidth() / (float) currentFeature->RenderWidth(),
                        currentFeature->DisplayWidth(), currentFeature->DisplayHeight(),
                        (float) currentFeature->DisplayWidth() / (float) currentFeature->RenderWidth(),
                        (int) currentFeature->FrameCount());

            if (!state.detailedGpuTimes.empty())
            {
                ImGui::Spacing();
                ImGui::TextDisabled("Per shader breakdown:");
                if (ImGui::BeginTable("ShaderTimes", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    bool hasExtra = false;

                    for (auto& [name, time, includedInUpscalerTime] : state.detailedGpuTimes)
                    {
                        if (!includedInUpscalerTime)
                        {
                            hasExtra = true;
                            continue;
                        }

                        auto formattedTime = StrFmt("%7.2f ms", time);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(name.c_str());

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(formattedTime.c_str());
                    }

                    // The NVIDIA path's Neural Rendering time (DlssNr::LastGpuTime), never set on an AMD card, where
                    // the AMD runtimes run instead (AMDNR 0.3.4: the row is not offered there at all).
                    std::optional<double> nrTime {};
                    if (!DlssNr::AmdBridge::GpuSupportInfo().amd && !DlssNr::AmdBridge::BackendActive())
                        nrTime = DlssNr::LastGpuTime();
                    if (hasExtra || nrTime.has_value())
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("Extra shaders:");
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("");
                        for (auto& [name, time, includedInUpscalerTime] : state.detailedGpuTimes)
                        {
                            if (includedInUpscalerTime)
                                continue;

                            auto formattedTime = StrFmt("%7.2f ms", time);

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(name.c_str());

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(formattedTime.c_str());
                        }

                        if (nrTime.has_value())
                        {
                            ImGui::TableNextColumn();
                            ImGui::Text("Neural Rendering");
                            ImGui::TableNextColumn();
                            ImGui::Text("%.2f ms", nrTime.value());
                        }
                    }

                    ImGui::EndTable();
                }
            }
        }
        ImGui::EndTooltip();
    }
}

void MenuCommon::RenderMainMenuBottomBar(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& menuResScale = ctx.menuResScale;

    // BOTTOM LINE --------------- (AMDNR 0.3.4, the approved mock) the right half of the footer's one line, flush with
    // the content's right edge: "Menu Scale [1.0] [Save Settings] [Close]". The Guide button that stood here is the
    // header's GitHub button (the same page); the resolution line and the frame count are in the FrameTime hover.
    const float px = MenuCommon::MockPx();
    const ImGuiStyle& style = ImGui::GetStyle();

    auto autoText = config->MenuScale.has_value() ? "Auto" : StrFmt("Auto (%3.1f)", menuResScale);
    // clang-format off
    const char* uiScales[] = { autoText.c_str(), "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1",
                               "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0" };
    // clang-format on
    // The scale in use, in brackets, as the mock shows it; the list still offers Auto and 0.5 .. 2.0.
    const std::string scalePreview = StrFmt("[%.1f]", menuResScale);
    const float buttonPad = ImFloor(10.0f * px) * 2.0f;
    const float scalePad = ImFloor(1.0f * px);
    const float scaleW = ImGui::CalcTextSize(scalePreview.c_str()).x + scalePad * 2.0f;
    const float groupW = ImGui::CalcTextSize("Menu Scale").x + style.ItemSpacing.x + scaleW + style.ItemSpacing.x +
                         ImGui::CalcTextSize("Save Settings").x + buttonPad + style.ItemSpacing.x +
                         ImGui::CalcTextSize("Close").x + buttonPad;
    ImGui::SameLine(0.0f, 0.0f);
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > groupW + style.ItemSpacing.x)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - groupW);
    else
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.ItemSpacing.x);

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", "Menu Scale");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(scaleW);
    ImGui::PushStyleColor(ImGuiCol_Text, ThemeColor(MenuColor::Dim));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(scalePad, style.FramePadding.y));
    const bool scaleOpen = ImGui::BeginCombo("##menuScale", scalePreview.c_str(), ImGuiComboFlags_NoArrowButton);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    if (scaleOpen)
    {
        for (int n = 0; n < std::size(uiScales); n++)
        {
            if (ImGui::Selectable(uiScales[n], (_selectedScale == n)))
            {
                _selectedScale = n;

                if (n == 0)
                    config->MenuScale.reset();
                else
                    config->MenuScale = 0.4f + (float) n / 10.0f;
            }
        }

        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        ImGui::SetTooltip("Menu Scale: %s", uiScales[std::clamp(_selectedScale, 0, (int) std::size(uiScales) - 1)]);

    ImGui::SameLine();

    if (DlssNr::NeuralUi::Button("Save Settings"))
        config->SaveIni();

    ImGui::SameLine();

    if (DlssNr::NeuralUi::Button("Close"))
    {
        _isVisible = false;
        ChooserMenuClosed("Close button"); // AMDNR 0.3.4.2 (C1)
        hasGamepad = (io.BackendFlags | ImGuiBackendFlags_HasGamepad) > 0;
        io.BackendFlags &= 30;
        io.ConfigFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoKeyboard;

        _showMipmapCalcWindow = false;
        _showHudlessWindow = false;
        io.MouseDrawCursor = false;
        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = false;
    }

    auto winSize = ImGui::GetWindowSize();
    auto winPos = ImGui::GetWindowPos();

    if (state.nvngxIniDetected)
    {
        ImGui::Spacing();
        ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)),
                           "nvngx.ini detected, please move over to using OptiScaler.ini and delete the old config");
        ImGui::Spacing();
    }

    // The height follows the contents (a taller tab, a section opened), so a moved window is checked
    // against the screen's edges on every size change, not only after a Menu Scale change.
    if (winSize.x != lastMenuWinSize.x || winSize.y != lastMenuWinSize.y)
    {
        lastMenuWinSize = winSize;
        menuClampFrames = std::max(menuClampFrames, 1);
    }

    if (lastPosition.x < -900.0f || (lastPosition.x >= winPos.x - 1.0f && lastPosition.y >= winPos.y - 1.0f &&
                                     lastPosition.x <= winPos.x + 1.0f && lastPosition.y <= winPos.y + 1.0f))
    {
        float posX;
        float posY;

        posX = ((float) io.DisplaySize.x - winSize.x) / 2.0f;
        // (AMDNR 0.3.4) Centred on the height cap rather than on the current height, so the top edge and the tab bar
        // stay put when a tab of another height is opened (critique item 1); a taller tab grows downward.
        posY = ((float) io.DisplaySize.y - std::max(winSize.y, mainMenuMaxHeight)) / 2.0f;

        // don't position menu outside of screen
        if (posX < 0.0 || posY < 0.0)
        {
            posX = 50;
            posY = 50;
        }

        ImGui::SetWindowPos(ImVec2 { posX, posY });
        lastPosition.x = posX;
        lastPosition.y = posY;
    }
    else if (menuClampFrames > 0)
    {
        // A window the player moved stays where it was put, but a larger Menu Scale or taller contents
        // must not grow it past the screen's edge. lastPosition is left alone: the next frame would otherwise read the
        // clamped position as never moved and centre the window.
        const ImVec2 maxPos(std::max((float) io.DisplaySize.x - winSize.x, 0.0f),
                            std::max((float) io.DisplaySize.y - winSize.y, 0.0f));
        const ImVec2 clamped(std::clamp(winPos.x, 0.0f, maxPos.x), std::clamp(winPos.y, 0.0f, maxPos.y));
        if (clamped.x != winPos.x || clamped.y != winPos.y)
            ImGui::SetWindowPos(clamped);
    }

    if (menuClampFrames > 0)
        menuClampFrames--;
}

void MenuCommon::RenderMipmapBiasWindow(RenderMenuContext& ctx, ImGuiWindowFlags flags)
{
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& currentFeature = ctx.currentFeature;

    // Metrics window (for debug)
    // ImGui::ShowMetricsWindow();

    // Mipmap calculation window
    if (_showMipmapCalcWindow && currentFeature != nullptr && !currentFeature->IsFrozen() && currentFeature->IsInited())
    {
        auto posX = (io.DisplaySize.x - 450.0f) / 2.0f;
        auto posY = (io.DisplaySize.y - 200.0f) / 2.0f;

        ImGui::SetNextWindowPos(ImVec2 { posX, posY }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2 { 450.0f, 200.0f }, ImGuiCond_FirstUseEver);

        if (_displayWidth == 0)
        {
            if (config->OutputScalingEnabled.value_or_default())
            {
                _displayWidth = static_cast<uint32_t>(currentFeature->DisplayWidth() *
                                                      config->OutputScalingMultiplier.value_or_default());
            }
            else
            {
                _displayWidth = currentFeature->DisplayWidth();
            }

            _renderWidth = static_cast<uint32_t>(_displayWidth / 3.0f);
            _mipmapUpscalerQuality = 0;
            _mipmapUpscalerRatio = 3.0f;
            _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
        }

        if (ImGui::Begin("Mipmap Bias", nullptr, flags))
        {
            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
                ImGui::SetWindowFocus();

            if (ImGui::InputScalar("Display Width", ImGuiDataType_U32, &_displayWidth, NULL, NULL, "%u"))
            {
                if (_displayWidth <= 0)
                {
                    if (config->OutputScalingEnabled.value_or_default())
                    {
                        _displayWidth = static_cast<uint32_t>(currentFeature->DisplayWidth() *
                                                              config->OutputScalingMultiplier.value_or_default());
                    }
                    else
                    {
                        _displayWidth = currentFeature->DisplayWidth();
                    }
                }

                _renderWidth = static_cast<uint32_t>(_displayWidth / _mipmapUpscalerRatio);
                _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
            }

            const char* q[] = { "Ultra Performance", "Performance", "Balanced", "Quality", "Ultra Quality", "DLAA" };
            float fr[] = { 3.0f, 2.0f, 1.7f, 1.5f, 1.3f, 1.0f };
            auto configQ = _mipmapUpscalerQuality;

            const char* selectedQ = q[configQ];

            ImGui::BeginDisabled(config->UpscaleRatioOverrideEnabled.value_or_default());

            if (DlssNr::NeuralUi::BeginCombo("Upscaler Quality", selectedQ))
            {
                for (int n = 0; n < 6; n++)
                {
                    if (ImGui::Selectable(q[n], (_mipmapUpscalerQuality == n)))
                    {
                        _mipmapUpscalerQuality = n;

                        float ov = -1.0f;

                        if (config->QualityRatioOverrideEnabled.value_or_default())
                        {
                            switch (n)
                            {
                            case 0:
                                ov = config->QualityRatio_UltraPerformance.value_or(-1.0f);
                                break;

                            case 1:
                                ov = config->QualityRatio_Performance.value_or(-1.0f);
                                break;

                            case 2:
                                ov = config->QualityRatio_Balanced.value_or(-1.0f);
                                break;

                            case 3:
                                ov = config->QualityRatio_Quality.value_or(-1.0f);
                                break;

                            case 4:
                                ov = config->QualityRatio_UltraQuality.value_or(-1.0f);
                                break;
                            }
                        }

                        if (ov > 0.0f)
                            _mipmapUpscalerRatio = ov;
                        else
                            _mipmapUpscalerRatio = fr[n];

                        _renderWidth = static_cast<uint32_t>(_displayWidth / _mipmapUpscalerRatio);
                        _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::EndDisabled();

            auto minLimit = config->ExtendedLimits.value_or_default() ? 0.1f : 1.0f;
            auto maxLimit = config->ExtendedLimits.value_or_default() ? 6.0f : 3.0f;
            if (DlssNr::NeuralUi::FillSlider("Upscaler Ratio", &_mipmapUpscalerRatio, minLimit, maxLimit, "%.2f"))
            {
                _renderWidth = static_cast<uint32_t>(_displayWidth / _mipmapUpscalerRatio);
                _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
            }

            if (ImGui::InputScalar("Render Width", ImGuiDataType_U32, &_renderWidth, NULL, NULL, "%u"))
                _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);

            DlssNr::NeuralUi::FillSlider("Mipmap Bias", &_mipBiasCalculated, -15.0f, 0.0f, "%.3f");

            // BOTTOM LINE
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SameLine();
            ImGui::Spacing();

            constexpr float spacing = 6.0f;
            auto textSize = ImGui::CalcTextSize("Use Value");
            textSize += ImGui::CalcTextSize("Close");
            textSize.x += ImGui::GetStyle().FramePadding.x * 5.0f + spacing; // 2 sides * 2 buttons + 1

            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - textSize.x);

            if (DlssNr::NeuralUi::Button("Use Value"))
            {
                _mipBias = _mipBiasCalculated;
                _showMipmapCalcWindow = false;
            }

            ImGui::SameLine(0.0f, spacing);

            if (DlssNr::NeuralUi::Button("Close"))
                _showMipmapCalcWindow = false;

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::End();
        }
    }
}

void MenuCommon::RenderHudlessResourcesWindow(RenderMenuContext& ctx, ImGuiWindowFlags flags)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;

    auto fg = state.currentFG;
    if (_showHudlessWindow && config->FGHUDFix.value_or_default() && fg != nullptr && fg->IsActive())
    {
        // Its size is set every frame, so it follows the Menu Scale here.
        const ImVec2 hudlessSize { 400.0f * ctx.menuResScale, 300.0f * ctx.menuResScale };
        auto posX = (io.DisplaySize.x - hudlessSize.x) / 2.0f;
        auto posY = (io.DisplaySize.y - hudlessSize.y) / 2.0f;

        ImGui::SetNextWindowPos(ImVec2 { posX, posY }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(hudlessSize);

        if (ImGui::Begin("HUDless Resources", nullptr, flags))
        {
            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
                ImGui::SetWindowFocus();

            int btnCount = 100;

            if (ImGui::BeginTable("HUDlessTable", 2, ImGuiTableFlags_SizingFixedFit))
            {
                ImGui::TableSetupColumn("##1", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##2", ImGuiTableColumnFlags_WidthFixed);

                ankerl::unordered_dense::map<void*, CapturedHudlessInfo>::iterator it;

                for (it = state.capturedHudlesses.begin(); it != state.capturedHudlesses.end(); it++)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    ImGui::Text("%08x, %s->%s, Count: %llu, %s", (size_t) it->first,
                                GetSourceString(it->second.captureInfo & 0xFF).c_str(),
                                GetDispatchString(it->second.captureInfo & 0xFF00).c_str(), it->second.usageCount,
                                it->second.enabled ? "Active" : "Passive");

                    ImGui::TableSetColumnIndex(1);

                    btnCount++;
                    std::string text;

                    if (it->second.enabled)
                        text = StrFmt("Disable##%d", btnCount);
                    else
                        text = StrFmt("Enable##%d", btnCount);

                    if (DlssNr::NeuralUi::Button(text.c_str()))
                    {
                        LOG_DEBUG("HUDless {:X}: {}", (size_t) it->first,
                                  it->second.enabled ? "Disabling" : "Enabling");
                        it->second.enabled = !it->second.enabled;
                    }
                }

                ImGui::EndTable();
            }

            if (DlssNr::NeuralUi::Button("Clear##4"))
            {
                LOG_DEBUG("Clearing captured HUDless resources");
                state.clearCapturedHudlesses = true;
            }

            ImGui::SameLine(0.0f, 8.0f);

            if (DlssNr::NeuralUi::Button("Close##4"))
                _showHudlessWindow = false;

            ImGui::End();
        }
    }
}

// T11d: opens the scrolling tab body (kPinnedBody), a child with no border and no background whose height follows its
// contents up to maxHeight; the caller closes it with ImGui::EndChild() after the tab bar. Its return value is not
// needed: a clipped child may still be submitted to.
static void BeginPinnedMenuBody(float maxHeight)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, maxHeight));
    ImVec4 bodyBg = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
    bodyBg.w = 0.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bodyBg);
    ImGui::BeginChild("##menu_body", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY);
    ImGui::PopStyleColor();
}

void MenuCommon::RenderMainMenuWindow(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;
    auto& frameTimesCalculated = ctx.frameTimesCalculated;
    auto& menuResScale = ctx.menuResScale;

    if (!_isVisible)
        return;

    // Check for GPU support once and reuse the result in all menu sections.
    // DXVK might call Vulkan device creation, which would destroy our objects.
    // (AMDNR 0.3.3.2) Scoped: it puts back what was there instead of clearing a skip another scope still
    // holds, and marks this thread's creations as OptiScaler's own (State.h).
    {
        ScopedSkipVulkanHooks skipVulkanHooks {};
        ctx.primaryGpu =
            std::make_unique<std::decay_t<decltype(IdentifyGpu::getPrimaryGpu())>>(IdentifyGpu::getPrimaryGpu());
    }

    // Menu font (AMDNR 0.3.4): 14 px at Menu Scale 1.0 (kMenuFontPx: the approved mock's 12.5 px CSS text), a [Menu]
    // FontSize override scaling it (fontSize is the atlas font's size, 14 unless overridden). The FPS overlay, splash
    // and toasts keep fontSize.
    if (config->UseHQFont.value_or_default())
        ImGui::PushFontSize(std::round(menuResScale * fontSize * (kMenuFontPx / 14.0f)));

    // If overlay is not visible frame needs to be inited
    if (!frameTimesCalculated)
    {
        float frameCnt = 0;
        frameTime = 0;
        for (size_t i = 299; i > 199; i--)
        {
            if (state.frameTimes[i] > 0.0)
            {
                frameTime += state.frameTimes[i];
                frameCnt++;
            }
        }

        frameTime /= frameCnt;
        frameRate = 1000.0 / frameTime;
    }

    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoSavedSettings;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_AlwaysAutoResize;

    if (lastMenuScale != menuResScale)
    {
        lastMenuScale = menuResScale;

        // if UI scale is changed rescale the style
        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiStyle styleold = style; // Backup colors
        style = ImGuiStyle();        // IMPORTANT: ScaleAllSizes will change the original size,
                                     // so we should reset all style config

        ApplyThemeStyle();

        style.ScaleAllSizes(menuResScale);
        style.MouseCursorScale = 1.0f;
        CopyMemory(style.Colors, styleold.Colors, sizeof(style.Colors)); // Restore colors

        menuClampFrames = 3;
    }

    // The window's size follows the Menu Scale. AlwaysAutoResize alone measured it from its contents,
    // but most of the contents size themselves from the window - every section is a full-width child
    // (ScopedCollapsingHeader), the Neural tab's section buttons split the width, wrapped text wraps
    // at the edge and the Guide button is pushed to it - so the width it measured was always the current
    // one: the window could grow but never shrink, and a smaller scale shrank only the text. A
    // one-frame SetNextWindowSize({ 1, 1 }) did not break that loop: a child left no room auto-fits
    // to its previous frame's contents and hands the old width straight back. So the width is a
    // number of font heights, and the height auto-fits up to a cap and scrolls beyond it; both stay
    // inside the screen. Constraints apply after the auto-fit, so AlwaysAutoResize stays.
    // (AMDNR 0.3.4 MENU match1) One mock px (MockPx): the HQ font is 14 px x Menu Scale (x a FontSize override); the
    // bitmap font follows the Menu Scale (through the window's font scale, set after Begin). menuFontPx is the text
    // height inside the window.
    const bool hqFont = config->UseHQFont.value_or_default();
    const float mockPx = hqFont ? ImGui::GetFontSize() / kMenuFontPx : std::max(menuResScale, 0.5f);
    const float menuFontPx = ImGui::GetFontSize() * (hqFont ? 1.0f : menuResScale);
    const ImVec2 menuWorkSize = ImGui::GetMainViewport()->WorkSize;
    const float menuMargin = 16.0f * menuResScale;
    // (AMDNR 0.3.4, the approved mock) The mock's 750 px at Menu Scale 1.0, whole pixels; the height cap is 960 px,
    // which the mock needs with Ray Regeneration and a tools drawer open.
    const float menuWidth =
        std::max(std::min(WholePx(kMenuWidthPx * mockPx), ImFloor(menuWorkSize.x - menuMargin * 2.0f)), 200.0f);
    const float menuMaxHeight =
        std::max(std::min(WholePx(kMenuMaxHeightPx * mockPx), ImFloor(menuWorkSize.y - menuMargin * 2.0f)), 200.0f);
    mainMenuMaxHeight = menuMaxHeight;
    ImGui::SetNextWindowSizeConstraints(ImVec2(menuWidth, 0.0f), ImVec2(menuWidth, menuMaxHeight));

    // Main menu window: "AMDNR v<ver> - <exe> - <game>" (AMDNR 0.3.4, the approved mock). The build stamp that stood
    // in the title (how a tester's screenshot is matched to a build, resource.h) is in the hover of the header's
    // "AMDNR" label, in Save report's zip and in the log header; the OptiScaler version is in the log header
    // (VER_PRODUCT_NAME, unchanged). The (Q) / (OP) markers moved into Advanced > Active Quirks.
    if (windowTitle.empty())
    {
        windowTitle = StrFmt("AMDNR v%s - %s%s", AMDNR_VERSION_STR, state.gameExe.c_str(),
                             state.gameName.empty() ? "" : StrFmt(" - %s", state.gameName.c_str()).c_str());
    }

    // The mock's look for the main window and the windows it opens (MainMenuTheme), popped at the end.
    MainMenuTheme theme;
    theme.Push(mockPx, menuFontPx);

    // The title bar: 25 px with the window's top border, so 24 px of red as the mock (the text and (25 - text) / 2
    // above and below), the title 12 px in, white on the red bar. Whole pixels, so the window's auto-fit height is
    // exact (a fraction here drew a scrollbar, WholePx). Begin reads these for the title bar only; they are popped
    // right after it (ImGui allows Push / Begin / Pop / End). The title is drawn before the bitmap font gets the Menu
    // Scale, so it is measured with the font as it is here.
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(WholePx(12.0f * mockPx),
                               ImMax((WholePx(25.0f * mockPx) - ImGui::GetFontSize()) * 0.5f, 0.0f)));
    ImGui::PushStyleColor(ImGuiCol_Text, ThemeColor(MenuColor::White));
    const bool mainOpen = ImGui::Begin(windowTitle.c_str(), NULL, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (mainOpen)
    {
        // Without the HQ font the text follows the scale through the window's font scale.
        if (!config->UseHQFont.value_or_default())
            ImGui::SetWindowFontScale(menuResScale);

        // Header/status messages shown above the two-column settings table.
        RenderMainMenuHeaderMessages(ctx);

        // T11d (kPinnedBody): the tabs scroll inside a borderless, transparent child capped at what the header and
        // the footer (measured on the previous frame) leave of the window's height cap, so both stay on screen.
        static float pinnedFooterHeight = 0.0f;
        if constexpr (kPinnedBody)
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            BeginPinnedMenuBody(std::max(menuMaxHeight - ImGui::GetCursorPosY() - pinnedFooterHeight -
                                             style.WindowPadding.y - style.ItemSpacing.y,
                                         WholePx(96.0f * mockPx)));
        }

        // Main two-column settings content.
        RenderMainMenuTable(ctx);

        if constexpr (kPinnedBody)
            ImGui::EndChild();
        const float footerTop = ImGui::GetCursorPosY();

        // Diagnostics and footer actions below the settings table.
        RenderMainMenuGraphs(ctx);
        RenderMainMenuBottomBar(ctx);

        if constexpr (kPinnedBody)
            pinnedFooterHeight = ImGui::GetCursorPosY() - footerTop;

        ImGui::End();
    }

    // Detached utility windows owned by the main menu.
    RenderMipmapBiasWindow(ctx, flags);
    RenderHudlessResourcesWindow(ctx, flags);

    theme.Pop();

    if (config->UseHQFont.value_or_default())
        ImGui::PopFontSize();
}

void KeyUp(UINT vKey)
{
    inputMenu = vKey == Config::Instance()->ShortcutKey.value_or_default();
    inputFps = vKey == Config::Instance()->FpsShortcutKey.value_or_default();
    inputFG = vKey == Config::Instance()->FGShortcutKey.value_or_default();
    inputFpsCycle = vKey == Config::Instance()->FpsCycleShortcutKey.value_or_default();
    // The neural keys were missing here: titles whose input reaches us through the window
    // procedure (not the raw-input poll) never saw the toggle fire.
    const int nrKey = Config::Instance()->DlssNrToggleKey.value_or_default();
    const int capKey = Config::Instance()->DlssNrCaptureKey.value_or_default();
    if (nrKey != UnboundKey && vKey == static_cast<UINT>(nrKey)) inputDlssNr = true;
    if (capKey != UnboundKey && vKey == static_cast<UINT>(capKey)) inputCapture = true;
}

// The lamp, and only the lamp.
//
// Red for dark, green for full light, with its reading beside it. No status sentence: the whole
// point of a light meter is that it is read at a glance while playing, and a paragraph in the corner
// of somebody's game is not that. Everything wordy lives in the menu, which is where someone has
// already decided to stop and read.
//
// Drawn only when its own setting is on. An overlay that appears because a scan happens to be
// running is an overlay nobody asked for.
void RenderExposureScanIndicator(float alpha)
{
    using DlssNr::ExposureScan::Verdict;

    if (!Config::Instance()->DlssNrScanMeter.value_or_default())
        return;

    if (DlssNr::ExposureScan::Where() == Verdict::Off)
        return;

    int which = 0;
    float low = 0.0f, high = 0.0f;
    const float now = DlssNr::ExposureScan::BestValue(&which, &low, &high);

    // Nothing found yet, or no range to place it in: a dim lamp, which says "watching, no reading"
    // without saying it in words.
    const bool reading = now > 0.0f && high > low;

    float lit = 0.0f;

    if (reading)
    {
        // An exposure falls as the scene brightens, so the value reads backwards unless the buffer
        // holds the reciprocal -- the same question the anchor asks, answered from the same setting,
        // because a lamp contradicting the picture would be worse than no lamp.
        lit = (high - now) / (high - low);

        if (Config::Instance()->DlssNrScanInverted.value_or_default())
            lit = 1.0f - lit;

        lit = lit < 0.0f ? 0.0f : (lit > 1.0f ? 1.0f : lit);
    }

    // Red to amber to green. A straight red-to-green fade passes through a muddy brown at the
    // midpoint, and the midpoint is where most of a session is spent.
    const ImVec4 dark(0.90f, 0.22f, 0.20f, 1.0f);
    const ImVec4 mid(0.95f, 0.75f, 0.20f, 1.0f);
    const ImVec4 bright(0.35f, 0.88f, 0.38f, 1.0f);
    const ImVec4 idle(0.45f, 0.45f, 0.45f, 1.0f);

    ImVec4 lamp = idle;

    if (reading)
    {
        const float t = lit < 0.5f ? lit * 2.0f : (lit - 0.5f) * 2.0f;
        const ImVec4& a = lit < 0.5f ? dark : mid;
        const ImVec4& b = lit < 0.5f ? mid : bright;
        lamp = ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1.0f);
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - 12.0f, vp->WorkPos.y + 12.0f),
                            ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(alpha);

    if (ImGui::Begin("DlssNrExposureScan", nullptr,
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove))
    {
        const float r = ImGui::GetFontSize() * 0.38f;
        const ImVec2 at = ImGui::GetCursorScreenPos();
        const ImVec2 centre(at.x + r, at.y + ImGui::GetTextLineHeight() * 0.5f);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddCircleFilled(centre, r, ImGui::GetColorU32(lamp), 20);
        draw->AddCircle(centre, r, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.6f)), 20, 1.5f);

        ImGui::Dummy(ImVec2(r * 2.0f + 6.0f, ImGui::GetTextLineHeight()));
        ImGui::SameLine();

        if (reading)
            ImGui::TextColored(lamp, "%3.0f%%  %.5f", lit * 100.0f, now);
        else
            ImGui::TextColored(idle, "--");
    }

    ImGui::End();
}

// AMDNR 0.3.4.2 (H2): danielblnc's runtime and lmxxf's assets both installed and [DlssNr] NrBackend not set
// (AmdBridge::RuntimeChoiceNeeded). Until 0.3.4.1 RenderMenu
// opened the menu here by itself so the chooser was the first thing on screen, which trapped players over a loading
// game (Assetto Corsa, F1 25). Now one notification per process says which runtime runs until the player picks and
// how to open the menu (MenuUi::RuntimeChooserBoot); the chooser (RenderNeuralRuntimeChooser) appears the first time
// the player opens the menu, and what runs meanwhile is the bridge's own rule, untouched. The runtime is named by the
// Neural tab's own rule (RuntimeCaps::Menu(): the backend built, else the one the bridge will build), the key by its
// configured name. The choice is polled only until the notice is posted, as the forced open was.
static void RaiseNeuralChoiceNotice(Config* config, bool menuVisible)
{
    if (chooserBoot.notified)
        return;
    if (chooserBoot.OnFrame(DlssNr::AmdBridge::RuntimeChoiceNeeded(), menuVisible) != MenuUi::BootAction::Notify)
        return;

    const char* runtime = DlssNr::RuntimeCaps::Menu().name;
    const std::string key = MenuCommon::KeyName(config->ShortcutKey.value_or_default());
    const std::string text = MenuUi::BootNoticeText(runtime, key.c_str());
    ImGuiToast toast { ImGuiToastType::Info, 15000 };
    toast.setTitle("%s", "DLSS Neural Rendering");
    toast.setContent("%s", text.c_str());
    ImGui::InsertNotification(toast);
    LOG_INFO("runtime chooser: a second AMD neural runtime's files are installed and [DlssNr] NrBackend is not set; "
             "{} runs until the player picks; the chooser opens with the menu ({}), no longer by itself",
             runtime, key);
}

bool MenuCommon::RenderMenu()
{
    if (!_isInited)
        return false;

    RenderMenuContext ctx { State::Instance(), Config::Instance(), ImGui::GetIO() };
    ctx.now = Util::MillisecondsNow();
    ctx.currentFeature = ctx.state.currentFeature;

    // AMDNR 0.3.4.2 (H2): the first launch with both AMD neural runtimes and no choice no longer opens the menu by
    // itself; one notification says what runs and how to open the menu.
    RaiseNeuralChoiceNotice(ctx.config, _isVisible);

    // 1) Collect timing and input state before any ImGui drawing.
    UpdateRenderTiming(ctx);
    UpdateMenuInputMode(ctx);
    HandleMenuShortcuts(ctx);

    // 2) Prepare one-shot notifications and start a new ImGui frame only when needed.
    UpdateVersionAndStartupNotifications(ctx);
    BeginMenuFrameIfNeeded(ctx);
    OptiInput::EndFrame(_isVisible);

    // 3) Draw lightweight overlay windows first, preserving the original order.
    ctx.menuResScale = MenuResolutionScale(ctx.io);
    RenderSplashWindow(ctx);
    RenderNotifications(ctx);
    UpdateFrameTimeAverages(ctx);
    RenderPerformanceOverlay(ctx);
    RenderExposureScanIndicator(ctx.config->FpsOverlayAlpha.value_or_default());

    // 4) Draw the full settings menu last so popups and child windows keep their existing behavior.
    RenderMainMenuWindow(ctx);

    if (ctx.newFrame)
        ImGui::EndFrame();

    return ctx.newFrame;
}

void MenuCommon::Init(HWND InHwnd, bool isUWP)
{
    // Reset shutdown flag in case of re-init
    State::Instance().isShuttingDown = false;

    HWND oldHandle = nullptr;

    if (_handle != nullptr)
    {
        oldHandle = _handle;
        LOG_DEBUG("Old Handle: {:X}, ImGui Handle: {:X}", (size_t) oldHandle,
                  (size_t) ImGui::GetMainViewport()->PlatformHandleRaw);
    }

    _handle = InHwnd;
    _isVisible = false;
    _isUWP = isUWP;
    lastPosition = { -1000.0f, -1000.0f };

    LOG_DEBUG("Handle: {0:X}", (size_t) _handle);

    // In case d3d12 wasn't yet used up to this point, try to update GPU info late here
    IdentifyGpu::updateD3d12Capabilities();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    (void) io;

    hasGamepad = (io.BackendFlags | ImGuiBackendFlags_HasGamepad) > 0;
    io.BackendFlags &= 30;
    io.ConfigFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoKeyboard;

    io.MouseDrawCursor = _isVisible;
    io.WantCaptureKeyboard = _isVisible;
    io.WantCaptureMouse = _isVisible;
    io.WantSetMousePos = _isVisible;

    io.IniFilename = io.LogFilename = nullptr;

    bool initResult = false;

    if (io.BackendPlatformUserData == nullptr)
    {
        if (!isUWP)
        {
            initResult = ImGui_ImplWin32_Init(InHwnd);
            LOG_DEBUG("ImGui_ImplWin32_Init result: {0}", initResult);
        }
        else
        {
            initResult = ImGui_ImplUwp_Init(InHwnd);
            ImGui_BindUwpKeyUp(KeyUp);
            LOG_DEBUG("ImGui_ImplUwp_Init result: {0}", initResult);
        }
    }

    if (io.Fonts->Fonts.empty() && Config::Instance()->UseHQFont.value_or_default())
    {
        ImFontAtlas* atlas = io.Fonts;
        atlas->Clear();

        // This automatically becomes the next default font
        ImFontConfig fontConfig;

        if (Config::Instance()->FontSize.has_value())
            fontSize = Config::Instance()->FontSize.value();

        if (Config::Instance()->TTFFontPath.has_value())
        {
            io.FontDefault =
                atlas->AddFontFromFileTTF(wstring_to_string(Config::Instance()->TTFFontPath.value()).c_str(), fontSize,
                                          &fontConfig, io.Fonts->GetGlyphRangesDefault());
        }
        else
        {
            // (AMDNR 0.3.4 MENU match1) The approved mock's text is heavier than Hack's thin strokes (its labels
            // carry about 1.3x the ink at the same size and pitch): the built-in font's coverage is brightened 1.2x.
            // A [Menu] TTFFontPath font is drawn as it is.
            fontConfig.RasterizerMultiply = 1.2f;
            io.FontDefault = atlas->AddFontFromMemoryCompressedBase85TTF(hack_compressed_compressed_data_base85,
                                                                         fontSize, &fontConfig);
        }
    }

    if (!Config::Instance()->OverlayMenu.value_or_default())
    {
        _hdrTonemapApplied = false;
    }

    DWORD hwndPid = 0;
    DWORD hwndTid = GetWindowThreadProcessId(_handle, &hwndPid);

    LOG_DEBUG("HWND: {:X}, IsWindow: {}, HWND PID: {}, Current PID: {}, HWND TID: {}, Current TID: {}",
              (ULONG64) _handle, IsWindow(_handle), hwndPid, GetCurrentProcessId(), hwndTid, GetCurrentThreadId());

    OptiInput::Initialize(_handle, isUWP);

    ApplyThemeStyle();
    // A new context starts from a default style: the rescale has to run on it too.
    lastMenuScale = 0.0f;
    _isInited = true;
}

void MenuCommon::Shutdown()
{
    if (!MenuCommon::_isInited)
        return;

    // if (_oWndProc != nullptr)
    //{
    //     auto handle = (HWND) ImGui::GetMainViewport()->PlatformHandleRaw;
    //     SetLastError(0);
    //     auto restoreResult = SetWindowLongPtr(handle, GWLP_WNDPROC, (LONG_PTR) _oWndProc);
    //     auto error = GetLastError();

    //    if (restoreResult == 0 && error != 0)
    //    {
    //        LOG_ERROR("Failed to restore old WndProc. Error: {:X}", error);
    //    }

    //    _oWndProc = nullptr;
    //}

    if (!_isUWP)
        ImGui_ImplWin32_Shutdown();
    else
        ImGui_ImplUwp_Shutdown();

    ImGui::DestroyContext();

    _handle = nullptr;
    _isInited = false;
    _isVisible = false;
}

void MenuCommon::HideMenu()
{
    if (!_isVisible)
        return;

    _isVisible = false;

    ImGuiIO& io = ImGui::GetIO();
    (void) io;

    _showMipmapCalcWindow = false;
    _showHudlessWindow = false;

    io.MouseDrawCursor = _isVisible;
    io.WantCaptureKeyboard = _isVisible;
    io.WantCaptureMouse = _isVisible;
}
