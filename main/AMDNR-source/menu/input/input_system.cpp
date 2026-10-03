// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
#include "pch.h"
#include "input_system_internal.h"

#include <include/imgui/imgui.h>
#include <include/imgui/imgui_internal.h> // AMDNR 0.3.4.2 diagnostics: GImGui->HoveredWindow, WithinFrameScope

#include <cstdio>
#include <string>
#include <vector>

namespace OptiInput
{

InputState _state;

GetAsyncKeyState_t o_GetAsyncKeyState = ::GetAsyncKeyState;
GetKeyState_t o_GetKeyState = ::GetKeyState;
GetKeyboardState_t o_GetKeyboardState = ::GetKeyboardState;
GetCursorPos_t o_GetCursorPos = ::GetCursorPos;
SetCursorPos_t o_SetCursorPos = ::SetCursorPos;
GetPhysicalCursorPos_t o_GetPhysicalCursorPos = nullptr;
SetPhysicalCursorPos_t o_SetPhysicalCursorPos = nullptr;
GetMessagePos_t o_GetMessagePos = ::GetMessagePos;
GetMouseMovePointsEx_t o_GetMouseMovePointsEx = ::GetMouseMovePointsEx;
ClipCursor_t o_ClipCursor = ::ClipCursor;
SendInput_t o_SendInput = ::SendInput;
MouseEvent_t o_mouse_event = ::mouse_event;
PostMessageA_t o_PostMessageA = ::PostMessageA;
PostMessageW_t o_PostMessageW = ::PostMessageW;
SendMessageA_t o_SendMessageA = ::SendMessageA;
SendMessageW_t o_SendMessageW = ::SendMessageW;
CreateFileA_t o_CreateFileA = ::CreateFileA;
CreateFileW_t o_CreateFileW = ::CreateFileW;
ReadFile_t o_ReadFile = ::ReadFile;
DeviceIoControl_t o_DeviceIoControl = ::DeviceIoControl;
CloseHandle_t o_CloseHandle = ::CloseHandle;
PeekMessageA_t o_PeekMessageA = ::PeekMessageA;
PeekMessageW_t o_PeekMessageW = ::PeekMessageW;
GetMessageA_t o_GetMessageA = ::GetMessageA;
GetMessageW_t o_GetMessageW = ::GetMessageW;
GetRawInputData_t o_GetRawInputData = ::GetRawInputData;
GetRawInputBuffer_t o_GetRawInputBuffer = ::GetRawInputBuffer;
RegisterRawInputDevices_t o_RegisterRawInputDevices = ::RegisterRawInputDevices;
GetClipCursor_t o_GetClipCursor = ::GetClipCursor;
SetWindowsHookExA_t o_SetWindowsHookExA = ::SetWindowsHookExA;
SetWindowsHookExW_t o_SetWindowsHookExW = ::SetWindowsHookExW;
UnhookWindowsHookEx_t o_UnhookWindowsHookEx = ::UnhookWindowsHookEx;
GameInputCreate_t o_GameInputCreate = nullptr;
XInputGetState_t o_XInputGetState = nullptr;
XInputGetState_t o_XInputGetStateEx = nullptr;
XInputGetKeystroke_t o_XInputGetKeystroke = nullptr;
XInputSetState_t o_XInputSetState = nullptr;
DirectInput8Create_t o_DirectInput8Create = nullptr;
DirectInputCreateA_t o_DirectInputCreateA = nullptr;
DirectInputCreateW_t o_DirectInputCreateW = nullptr;
DirectInputCreateEx_t o_DirectInputCreateEx = nullptr;
DirectInputCreateDevice_t o_DirectInputCreateDeviceA = nullptr;
DirectInputCreateDevice_t o_DirectInputCreateDeviceW = nullptr;
DirectInputGetDeviceState_t o_DirectInputDeviceGetDeviceState = nullptr;
DirectInputGetDeviceData_t o_DirectInputDeviceGetDeviceData = nullptr;
DirectInputDeviceRelease_t o_DirectInputDeviceRelease = nullptr;

thread_local int bypassHookDepth = 0;

bool DiagInputDisabled()
{
    static const bool disabled = []
    {
        const bool value = Config::Instance()->DiagNoInputHooks.value_or_default();

        if (value)
        {
            LOG_INFO("[Diag] DiagNoInputHooks=true: OptiInput is not started (no WndProc subclass, no Win32/HID/raw/"
                     "SetWindowsHookEx/cursor/XInput/DirectInput/GameInput hooks); the OptiScaler menu gets no input");
        }

        return value;
    }();

    return disabled;
}

bool DiagSkipHookGroup(uint32_t group)
{
    static const uint32_t skipMask = []
    {
        const std::string raw = Config::Instance()->DiagInputHooksSkip.value_or_default();

        if (raw.empty())
            return 0u;

        struct GroupName
        {
            const char* Name;
            uint32_t Bits;
        };

        namespace G = DiagHookGroup;
        constexpr uint32_t minimalBits = G::KeyState | G::MsgPos | G::Clip | G::SendPost | G::Hid | G::Raw |
                                         G::WinHook | G::Cursor | G::XInput | G::DInput | G::GameInput;

        const GroupName names[] = {
            { "message", G::Message },     { "keystate", G::KeyState }, { "msgpos", G::MsgPos },
            { "clip", G::Clip },           { "sendpost", G::SendPost }, { "hid", G::Hid },
            { "raw", G::Raw },             { "winhook", G::WinHook },   { "cursor", G::Cursor },
            { "xinput", G::XInput },       { "dinput", G::DInput },     { "gameinput", G::GameInput },
            { "subclass", G::Subclass },   { "minimal", minimalBits },
            // AMDNR 0.3.4.2: not hook groups; the two menu input changes of 0.3.4.2 (off = 0.3.4.1 behaviour).
            { "presslatch", G::MenuPressLatch }, { "clickreplay", G::MenuClickReplay },
        };

        uint32_t mask = 0;
        std::string unknown;
        std::string token;

        auto flush = [&]
        {
            if (token.empty())
                return;

            bool found = false;
            for (const auto& entry : names)
            {
                if (token == entry.Name)
                {
                    mask |= entry.Bits;
                    found = true;
                    break;
                }
            }

            if (!found)
                unknown += (unknown.empty() ? "" : " ") + token;

            token.clear();
        };

        for (const char ch : raw)
        {
            if (ch == ',' || ch == ';' || ch == '|' || ch == ' ' || ch == '\t')
                flush();
            else
                token.push_back(ch);
        }

        flush();

        std::string skipped;
        for (const auto& entry : names)
        {
            if (entry.Bits != minimalBits && (mask & entry.Bits) != 0)
                skipped += (skipped.empty() ? "" : " ") + std::string(entry.Name);
        }

        LOG_INFO("[Diag] DiagInputHooksSkip={}: OptiInput leaves these hook groups uninstalled: {}{}{}", raw,
                 skipped.empty() ? "(none)" : skipped, unknown.empty() ? "" : " | unknown names ignored: ", unknown);

        return mask;
    }();

    return (skipMask & group) != 0;
}

bool MenuPressLatchEnabled() { return !DiagSkipHookGroup(DiagHookGroup::MenuPressLatch); }

bool MenuClickReplayEnabled() { return !DiagSkipHookGroup(DiagHookGroup::MenuClickReplay); }

const char* InputEventSourceName(InputEventSource source)
{
    switch (source)
    {
    case InputEventSource::Poll:
        return "poll";
    case InputEventSource::PollTap:
        return "pollTap";
    case InputEventSource::Queue:
        return "queue";
    case InputEventSource::WndProc:
        return "wndproc";
    case InputEventSource::Raw:
        return "raw";
    case InputEventSource::None:
    default:
        return "none";
    }
}

bool ShouldApplyBlockingPolicyLocked() { return bypassHookDepth == 0 && _state.MenuVisible; }

bool ShouldBlockKeyboardInputLocked() { return ShouldApplyBlockingPolicyLocked() && _state.BlockKeyboard; }

bool ShouldBlockMouseInputLocked() { return ShouldApplyBlockingPolicyLocked() && _state.BlockMouse; }

bool ShouldBlockCursorInputLocked() { return ShouldApplyBlockingPolicyLocked() && _state.BlockCursor; }

namespace
{
const char* YesNo(bool value) { return value ? "yes" : "no"; }

const char* AcquisitionModeName(InputAcquisitionMode mode)
{
    switch (mode)
    {
    case InputAcquisitionMode::WindowMessages:
        return "window";

    case InputAcquisitionMode::RawInput:
        return "raw";

    case InputAcquisitionMode::PolledAbsolute:
        return "polled-absolute";

    case InputAcquisitionMode::ExternalRawVirtualMouse:
        return "external-raw-virtual-mouse";

    case InputAcquisitionMode::None:
    default:
        return "none";
    }
}

InputAcquisitionMode ResolveInputAcquisitionModeLocked()
{
    if (_state.ExternalVirtualMouseAuthoritative || _state.ExternalVirtualMouseUsedThisFrame ||
        _state.ExternalVirtualMouseRelativeUsedThisFrame)
    {
        return InputAcquisitionMode::ExternalRawVirtualMouse;
    }

    if (_state.ReceivedRawInputThisFrame)
        return InputAcquisitionMode::RawInput;

    if (_state.ReceivedWindowMessageThisFrame || _state.ReceivedQueueMessageThisFrame)
        return InputAcquisitionMode::WindowMessages;

    if (_state.PolledInputUsedThisFrame || _state.PolledInputActive)
        return InputAcquisitionMode::PolledAbsolute;

    return InputAcquisitionMode::None;
}

void RefreshInputAcquisitionModeLocked() { _state.AcquisitionMode = ResolveInputAcquisitionModeLocked(); }

void LogHwndIdentityLocked(const char* label, HWND hwnd)
{
    if (hwnd == nullptr)
    {
        LOG_DEBUG("{}: hwnd:null", label);
        return;
    }

    DWORD processId = 0;
    const DWORD threadId = GetWindowThreadProcessId(hwnd, &processId);
    const HWND rootHwnd = IsWindow(hwnd) ? GetAncestor(hwnd, GA_ROOT) : nullptr;

    LOG_DEBUG("{}: hwnd:{} isWindow:{} root:{} pid:{} tid:{} currentPid:{}", label, static_cast<void*>(hwnd),
              IsWindow(hwnd) ? 1 : 0, static_cast<void*>(rootHwnd), processId, threadId, _state.CurrentProcessId);
}
} // namespace

void LogInputHealthSnapshotLocked(const char* origin)
{
    static std::uint64_t frameIndex = 0;
    static HWND lastTargetHwnd = nullptr;
    static HWND lastInputHwnd = nullptr;
    static HWND lastForegroundHwnd = nullptr;
    static bool lastFocused = false;
    static bool lastMenuVisible = false;
    static bool lastWndProcSubclassed = false;
    static bool lastExternalTargetProcess = false;
    static bool lastHasExplicitInputHwnd = false;
    static bool lastPolledInputActive = false;
    static bool lastExternalVirtualMouseActive = false;
    static bool lastExternalVirtualMouseAuthoritative = false;
    static bool lastExternalLowLevelMouseHookInstalled = false;
    static bool lastExternalRawInputSinkRegistered = false;
    static InputAcquisitionMode lastAcquisitionMode = InputAcquisitionMode::None;
    static DWORD lastTargetProcessId = 0;
    static DWORD lastInputProcessId = 0;
    static std::uint64_t lastNoInputWarnFrame = 0;
    static std::uint64_t lastNoInputHwndWarnFrame = 0;
    static std::uint64_t lastNoSubclassWarnFrame = 0;

    // P27 (0.3.4): GetTickCount64() of the last input from each channel that feeds the menu (0 = none yet).
    static std::uint64_t lastWindowInputMs = 0;
    static std::uint64_t lastQueueInputMs = 0;
    static std::uint64_t lastRawInputMs = 0;
    static std::uint64_t lastPolledInputMs = 0;
    static std::uint64_t lastAnyInputMs = 0;
    static std::uint64_t lastPolledInputFrameCount = 0;
    static std::uint64_t noInputWarnedStretchMs = ~0ull;
    static constexpr std::uint64_t kInputHealthWarnFrames = 600;
    static constexpr std::uint64_t kInputQuietMs = 10000;

    frameIndex++;

    // P27 (0.3.4): the two input-health warnings below used to count only window/queue/raw messages on the one
    // frame they sampled, so they fired on healthy sessions: frames without a key press or mouse move, menus fed by
    // polling, and titles where another program's WndProc took the subclass over (most likely the Steam overlay in
    // Hogwarts Legacy) while the queue detours and polling still fed the menu. Every channel now counts; the
    // menu-visible line is a WARN only after kInputQuietMs with no input from any of them, once per such quiet
    // stretch, and the subclass line is always a debug line (its loss has its own one-time WARN); the repeats (every
    // kInputHealthWarnFrames frames, as before) are debug lines. Polling is read from its cumulative counter
    // as well: with OverlayMenu=false BeginFrame runs twice per frame, and the second poll clears
    // PolledInputUsedThisFrame for a mouse move or button edge the first poll saw.
    const std::uint64_t nowMs = GetTickCount64();
    static const std::uint64_t firstSnapshotMs = nowMs;

    const bool polledThisFrame =
        _state.PolledInputUsedThisFrame || _state.PolledInputFrameCount != lastPolledInputFrameCount;
    lastPolledInputFrameCount = _state.PolledInputFrameCount;

    if (_state.ReceivedWindowMessageThisFrame)
        lastWindowInputMs = nowMs;
    if (_state.ReceivedQueueMessageThisFrame)
        lastQueueInputMs = nowMs;
    if (_state.ReceivedRawInputThisFrame)
        lastRawInputMs = nowMs;
    if (polledThisFrame)
        lastPolledInputMs = nowMs;

    const bool anyInputThisFrame = _state.ReceivedAnyInputThisFrame || polledThisFrame;
    if (anyInputThisFrame)
        lastAnyInputMs = nowMs;

    const std::uint64_t quietMs = nowMs - (lastAnyInputMs != 0 ? lastAnyInputMs : firstSnapshotMs);
    const bool menuInputFlowing = quietMs < kInputQuietMs;
    const auto recentInput = [nowMs](std::uint64_t lastMs) { return lastMs != 0 && nowMs - lastMs < kInputQuietMs; };

    // A quiet stretch is named by the time of its last input (0 = none since start); one WARN per stretch.
    const auto warnOncePerQuietStretch = [menuInputFlowing](std::uint64_t& warnedStretchMs)
    {
        if (menuInputFlowing || warnedStretchMs == lastAnyInputMs)
            return false;

        warnedStretchMs = lastAnyInputMs;
        return true;
    };

    HWND foregroundHwnd = GetForegroundWindow();
    DWORD foregroundProcessId = 0;
    DWORD foregroundThreadId = 0;

    if (foregroundHwnd != nullptr)
        foregroundThreadId = GetWindowThreadProcessId(foregroundHwnd, &foregroundProcessId);

    RefreshInputAcquisitionModeLocked();

    const bool stateChanged =
        lastTargetHwnd != _state.TargetHwnd || lastInputHwnd != _state.InputHwnd ||
        lastForegroundHwnd != foregroundHwnd || lastFocused != _state.Focused ||
        lastMenuVisible != _state.MenuVisible || lastWndProcSubclassed != _state.WndProcSubclassed ||
        lastExternalTargetProcess != _state.ExternalTargetProcess ||
        lastHasExplicitInputHwnd != _state.HasExplicitInputHwnd || lastPolledInputActive != _state.PolledInputActive ||
        lastExternalVirtualMouseActive != _state.ExternalVirtualMouseActive ||
        lastExternalVirtualMouseAuthoritative != _state.ExternalVirtualMouseAuthoritative ||
        lastExternalLowLevelMouseHookInstalled != _state.ExternalLowLevelMouseHookInstalled ||
        lastExternalRawInputSinkRegistered != _state.ExternalRawInputSinkRegistered ||
        lastAcquisitionMode != _state.AcquisitionMode || lastTargetProcessId != _state.TargetProcessId ||
        lastInputProcessId != _state.InputProcessId;

#if OPTIINPUT_VERBOSE_LOGGING
    const bool shouldLogHealth =
        stateChanged || (frameIndex % 120) == 0 || (_state.MenuVisible && (frameIndex % 30) == 0);
#else
    const bool shouldLogHealth = stateChanged || (frameIndex % 600) == 0;
#endif

    if (shouldLogHealth)
    {
#if OPTIINPUT_VERBOSE_LOGGING
        LOG_DEBUG(
            "{} health frame:{} mode:{} target:{} targetPid:{} input:{} inputPid:{} explicitInput:{} externalTarget:{} "
            "foreground:{} foregroundPid:{} foregroundTid:{} focused:{} menu:{} subclassed:{} hooks:{} recvWnd:{} "
            "recvQueue:{} recvRaw:{} recvAny:{} polledActive:{} polledUsed:{} polledMouse:{} polledKeyboard:{} "
            "externalVirtualMouse:{} externalAuthoritative:{} externalGetCursor:{} externalVirtualUsed:{} "
            "externalRelative:{} recenter:{} llMouseHook:{} rawSink:{} rawSinkPump:{} pendingDelta=({}, {}) "
            "virtualMouse=({}, {}) mouse=({}, {}) wheel:{} rawMouseReg:{} rawKeyboardReg:{} "
            "rawMouseNoLegacy:{} rawMouseSink:{} rawMouseCapture:{} rawKeyboardNoLegacy:{} rawKeyboardSink:{} "
            "trackedHooks:{} "
            "wndBlock:{} wndPass:{} queueBlock:{} queuePass:{} "
            "rawKeySan:{} rawKeyPass:{} rawMouseSan:{} rawMousePart:{} rawMousePass:{} "
            "asyncKeyBlock:{} keyStateBlock:{} keyboardStateFilter:{} cursorGetBlock:{} cursorPhysGetBlock:{} "
            "msgPosBlock:{} cursorSetBlock:{} cursorPhysSetBlock:{} clipBlock:{} getClipBlock:{} "
            "sendInputMouseBlock:{} sendInputKeyboardBlock:{} mouseEventBlock:{} postMouseBlock:{} postMousePass:{} "
            "sendMouseBlock:{} sendMousePass:{} hookKeyBlock:{} hookKeyPass:{} hookMouseBlock:{} hookMousePass:{} "
            "mouseMovePtsBlock:{} hidMouse:{} hidKeyboard:{} hidGamepad:{} hidOther:{} hidHandles:{} "
            "hidCreate:{} hidReadBlock:{} hidReadPass:{} hidIoctlBlock:{} hidIoctlPass:{} "
            "xinput:{} xGetState:{} xGetStateEx:{} xKeystroke:{} xSetState:{} xGetStateBlock:{} xKeyBlock:{} "
            "dinput:{} dinputLegacy:{} di8:{} diCreateA:{} diCreateW:{} diCreateEx:{} diDevA:{} diDevW:{} "
            "diState:{} diData:{} diKeyboard:{} diMouse:{} diOther:{} diStateBlock:{} diDataBlock:{}",
            origin != nullptr ? origin : "?", frameIndex, AcquisitionModeName(_state.AcquisitionMode),
            static_cast<void*>(_state.TargetHwnd), _state.TargetProcessId, static_cast<void*>(_state.InputHwnd),
            _state.InputProcessId, YesNo(_state.HasExplicitInputHwnd), YesNo(_state.ExternalTargetProcess),
            static_cast<void*>(foregroundHwnd), foregroundProcessId, foregroundThreadId, YesNo(_state.Focused),
            YesNo(_state.MenuVisible), YesNo(_state.WndProcSubclassed), YesNo(_state.HooksInstalled),
            YesNo(_state.ReceivedWindowMessageThisFrame), YesNo(_state.ReceivedQueueMessageThisFrame),
            YesNo(_state.ReceivedRawInputThisFrame), YesNo(_state.ReceivedAnyInputThisFrame),
            YesNo(_state.PolledInputActive), YesNo(_state.PolledInputUsedThisFrame),
            YesNo(_state.PolledMouseUsedThisFrame), YesNo(_state.PolledKeyboardUsedThisFrame),
            YesNo(_state.ExternalVirtualMouseActive), YesNo(_state.ExternalVirtualMouseAuthoritative),
            YesNo(_state.ExternalGetCursorPosVirtualizedThisFrame), YesNo(_state.ExternalVirtualMouseUsedThisFrame),
            YesNo(_state.ExternalVirtualMouseRelativeUsedThisFrame), YesNo(_state.ExternalCursorRecenteringDetected),
            YesNo(_state.ExternalLowLevelMouseHookInstalled), YesNo(_state.ExternalRawInputSinkRegistered),
            YesNo(_state.ExternalRawInputSinkPumpUsedThisFrame), _state.ExternalPendingMouseDeltaX,
            _state.ExternalPendingMouseDeltaY, _state.ExternalVirtualMouseClient.x, _state.ExternalVirtualMouseClient.y,
            _state.MouseClientPos.x, _state.MouseClientPos.y, _state.MouseWheel, YesNo(_state.RawMouseRegistered),
            YesNo(_state.RawKeyboardRegistered), YesNo(_state.RawMouseNoLegacy), YesNo(_state.RawMouseInputSink),
            YesNo(_state.RawMouseCaptureMouse), YesNo(_state.RawKeyboardNoLegacy), YesNo(_state.RawKeyboardInputSink),
            CountTrackedWindowsHooksLocked(), _state.WindowMessageBlockedCount, _state.WindowMessagePassedCount,
            _state.QueueMessageBlockedCount, _state.QueueMessagePassedCount, _state.RawKeyboardSanitizedCount,
            _state.RawKeyboardPassedCount, _state.RawMouseSanitizedCount, _state.RawMousePartialPassedCount,
            _state.RawMousePassedCount, _state.GetAsyncKeyStateBlockedCount, _state.GetKeyStateBlockedCount,
            _state.GetKeyboardStateFilteredCount, _state.GetCursorPosBlockedCount,
            _state.GetPhysicalCursorPosBlockedCount, _state.GetMessagePosBlockedCount, _state.SetCursorPosBlockedCount,
            _state.SetPhysicalCursorPosBlockedCount, _state.ClipCursorBlockedCount,
            _state.GetClipCursorVirtualizedCount, _state.SendInputMouseBlockedCount,
            _state.SendInputKeyboardBlockedCount, _state.MouseEventBlockedCount, _state.PostMouseMessageBlockedCount,
            _state.PostMouseMessagePassedCount, _state.SendMouseMessageBlockedCount, _state.SendMouseMessagePassedCount,
            _state.WindowsHookKeyboardBlockedCount, _state.WindowsHookKeyboardPassedCount,
            _state.WindowsHookMouseBlockedCount, _state.WindowsHookMousePassedCount, _state.MouseMovePointsBlockedCount,
            YesNo(_state.HidMouseHandleSeen), YesNo(_state.HidKeyboardHandleSeen), YesNo(_state.HidGamepadHandleSeen),
            YesNo(_state.HidOtherHandleSeen), _state.HidTrackedHandleCount, _state.HidCreateFileCallCount,
            _state.HidReadFileBlockedCount, _state.HidReadFilePassedCount, _state.HidDeviceIoControlBlockedCount,
            _state.HidDeviceIoControlPassedCount, YesNo(_state.XInputModuleLoaded),
            YesNo(_state.XInputGetStateHookInstalled), YesNo(_state.XInputGetStateExHookInstalled),
            YesNo(_state.XInputGetKeystrokeHookInstalled), YesNo(_state.XInputSetStateHookInstalled),
            _state.XInputGetStateBlockedCount, _state.XInputGetKeystrokeBlockedCount,
            YesNo(_state.DirectInputModuleLoaded), YesNo(_state.DirectInputLegacyModuleLoaded),
            YesNo(_state.DirectInput8CreateHookInstalled), YesNo(_state.DirectInputCreateAHookInstalled),
            YesNo(_state.DirectInputCreateWHookInstalled), YesNo(_state.DirectInputCreateExHookInstalled),
            YesNo(_state.DirectInputCreateDeviceAHookInstalled), YesNo(_state.DirectInputCreateDeviceWHookInstalled),
            YesNo(_state.DirectInputGetDeviceStateHookInstalled), YesNo(_state.DirectInputGetDeviceDataHookInstalled),
            YesNo(_state.DirectInputKeyboardDeviceSeen), YesNo(_state.DirectInputMouseDeviceSeen),
            YesNo(_state.DirectInputOtherDeviceSeen), _state.DirectInputGetDeviceStateBlockedCount,
            _state.DirectInputGetDeviceDataBlockedCount);
#else
        LOG_DEBUG("{} health frame:{} mode:{} target:{} input:{} focused:{} menu:{} subclassed:{} hooks:{} "
                  "recvWnd:{} recvQueue:{} recvRaw:{} polled:{} rawMouse:{} rawKeyboard:{} trackedHooks:{} "
                  "xinput:{} dinput:{} hidMouse:{} hidKeyboard:{} hidGamepad:{}",
                  origin != nullptr ? origin : "?", frameIndex, AcquisitionModeName(_state.AcquisitionMode),
                  static_cast<void*>(_state.TargetHwnd), static_cast<void*>(_state.InputHwnd), YesNo(_state.Focused),
                  YesNo(_state.MenuVisible), YesNo(_state.WndProcSubclassed), YesNo(_state.HooksInstalled),
                  YesNo(_state.ReceivedWindowMessageThisFrame), YesNo(_state.ReceivedQueueMessageThisFrame),
                  YesNo(_state.ReceivedRawInputThisFrame), YesNo(_state.PolledInputActive),
                  YesNo(_state.RawMouseRegistered), YesNo(_state.RawKeyboardRegistered),
                  CountTrackedWindowsHooksLocked(), YesNo(_state.XInputModuleLoaded),
                  YesNo(_state.DirectInputModuleLoaded), YesNo(_state.HidMouseHandleSeen),
                  YesNo(_state.HidKeyboardHandleSeen), YesNo(_state.HidGamepadHandleSeen));
#endif
    }

    if (_state.ExternalTargetProcess && _state.InputHwnd == nullptr && frameIndex - lastNoInputHwndWarnFrame >= 600)
    {
        lastNoInputHwndWarnFrame = frameIndex;
        LOG_WARN("external target process is active but InputHwnd is null; using polled/raw-sink/virtual mouse "
                 "fallback for menu input. "
                 "A local InputHwnd can improve menu input, but robust game input blocking still requires an "
                 "in-process input module.");
    }

    // P27 (0.3.4): the subclass loss itself is logged once, at WARN, where it happens (ValidateWindowSubclassLocked:
    // "subclass lost ..."; InstallWindowSubclass / SetInputWindow: "failed to subclass ..."), so this repeat is always
    // a debug line naming the channels that feed the menu. It is not a WARN even after a quiet stretch: with the menu
    // closed a quiet stretch is a player who does not touch keyboard or mouse (game start, loading, gamepad, alt-tab),
    // and with the menu open the 'menu is visible but no ...' line below reports it (with subclassed:no).
    // Re-installing the subclass over another program's WndProc is not done here (it can make a recursive WndProc
    // chain, see ValidateWindowSubclassLocked).
    if (_state.InputHwnd != nullptr && _state.UseWndProcSubclass && !_state.WndProcSubclassed &&
        frameIndex - lastNoSubclassWarnFrame >= kInputHealthWarnFrames)
    {
        lastNoSubclassWarnFrame = frameIndex;

        LOG_DEBUG("InputHwnd is set but WndProc is not subclassed; input in the last {} ms: window:{} queue:{} "
                  "raw:{} polled:{} (none for {} ms): input:{} inputPid:{} externalTarget:{}",
                  kInputQuietMs, YesNo(recentInput(lastWindowInputMs)), YesNo(recentInput(lastQueueInputMs)),
                  YesNo(recentInput(lastRawInputMs)), YesNo(recentInput(lastPolledInputMs)), quietMs,
                  static_cast<void*>(_state.InputHwnd), _state.InputProcessId, YesNo(_state.ExternalTargetProcess));
    }

    // P27 (0.3.4): polling counts as input too, and a quiet frame (no key press or mouse move) is not a fault: WARN
    // once per kInputQuietMs stretch without input on any channel, else a debug line.
    // AMDNR 0.3.4.2: checked every kInputQuietMs by time, not every kInputHealthWarnFrames frames (at Assetto Corsa's
    // 4-10 fps, 600 frames were 60-150 s).
    if (_state.MenuVisible && _state.InputHwnd != nullptr && !anyInputThisFrame &&
        _state.NoInputWarnCheck.Due(nowMs, kInputQuietMs))
    {
        lastNoInputWarnFrame = frameIndex;

        if (warnOncePerQuietStretch(noInputWarnedStretchMs))
        {
            LOG_WARN("menu is visible but no window/queue/raw/polled input was received for {} ms. input:{} "
                     "foreground:{} focused:{} subclassed:{}. Check that InputHwnd is the real overlay/menu HWND and "
                     "that its message pump runs.",
                     quietMs, static_cast<void*>(_state.InputHwnd), static_cast<void*>(foregroundHwnd),
                     YesNo(_state.Focused), YesNo(_state.WndProcSubclassed));
        }
        else
        {
            LOG_DEBUG("menu is visible and this frame had no input; input in the last {} ms: window:{} queue:{} "
                      "raw:{} polled:{} (none for {} ms). input:{} foreground:{} focused:{} subclassed:{}",
                      kInputQuietMs, YesNo(recentInput(lastWindowInputMs)), YesNo(recentInput(lastQueueInputMs)),
                      YesNo(recentInput(lastRawInputMs)), YesNo(recentInput(lastPolledInputMs)), quietMs,
                      static_cast<void*>(_state.InputHwnd), static_cast<void*>(foregroundHwnd), YesNo(_state.Focused),
                      YesNo(_state.WndProcSubclassed));
        }
    }

    lastTargetHwnd = _state.TargetHwnd;
    lastInputHwnd = _state.InputHwnd;
    lastForegroundHwnd = foregroundHwnd;
    lastFocused = _state.Focused;
    lastExternalRawInputSinkRegistered = _state.ExternalRawInputSinkRegistered;
    lastMenuVisible = _state.MenuVisible;
    lastWndProcSubclassed = _state.WndProcSubclassed;
    lastExternalTargetProcess = _state.ExternalTargetProcess;
    lastHasExplicitInputHwnd = _state.HasExplicitInputHwnd;
    lastPolledInputActive = _state.PolledInputActive;
    lastExternalVirtualMouseActive = _state.ExternalVirtualMouseActive;
    lastExternalVirtualMouseAuthoritative = _state.ExternalVirtualMouseAuthoritative;
    lastExternalLowLevelMouseHookInstalled = _state.ExternalLowLevelMouseHookInstalled;
    lastTargetProcessId = _state.TargetProcessId;
    lastInputProcessId = _state.InputProcessId;
}

namespace
{
bool ShouldPollVirtualKey(int vk)
{
    if (vk < 0 || vk >= 256)
        return false;

    switch (vk)
    {
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_MBUTTON:
    case VK_XBUTTON1:
    case VK_XBUTTON2:
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
        return false;

    default:
        return true;
    }
}

// AMDNR 0.3.4.2: the menu's nav keys (EdgeReplay.h) and the runtime chooser keys, whose "pressed since the last call"
// bit the poll believes while the menu is open.
bool IsMenuNavTapKey(int vk)
{
    switch (vk)
    {
    case VK_TAB:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_SPACE:
    case VK_RETURN:
    case VK_ESCAPE:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
        return true;
    default:
        return vk >= '0' && vk <= '9';
    }
}

// What the poll saw of one key or button (AMDNR 0.3.4.2): its physical reading, the last poll that saw it down, and
// that value as it was before this frame's first poll (the freshness line of EdgeReplay.h).
void RecordPollReadingLocked(ButtonState& button, bool down, DWORD time, bool firstPollThisFrame)
{
    if (firstPollThisFrame)
    {
        button.PollDownBeforeValid = button.LastPollDownValid;
        button.PollDownBeforeMs = button.LastPollDownMs;
    }

    button.PolledDown = down;

    if (down)
    {
        button.LastPollDownValid = true;
        button.LastPollDownMs = time;
    }
}

void PollVirtualKeyLocked(int vk, DWORD time, bool firstPollThisFrame, bool menuKeyTapPrimed, bool navTapPrimed)
{
    if (!ShouldPollVirtualKey(vk))
        return;

    const SHORT keyState = RealGetAsyncKeyStateSafe(vk);
    const bool down = (keyState & 0x8000) != 0;

    ButtonState& key = _state.Keys[vk];
    const bool polledDownBefore = key.PolledDown;
    RecordPollReadingLocked(key, down, time, firstPollThisFrame);

    // AMDNR 0.3.4.2: a tap that went down and up between two polls (GetAsyncKeyState bit 0), for the menu key and,
    // while the menu is open, its nav keys: one press and release now, and late messages of that tap are stale. A
    // press any source delivered since this key's previous poll is not a tap (PressCountAtLastPoll).
    const bool tapWatched = (menuKeyTapPrimed && vk == _state.MenuKeyVk) || (navTapPrimed && IsMenuNavTapKey(vk));

    if (down)
    {
        if (vk == _state.HeldKeyVk)
        {
            // The held key reads down again: the hold waits for its release, not for a late message pair.
            _state.HeldKeyAwaitingLatePair = false;
            _state.HeldKeyLastDownMs = time;
        }

        SetKeyDown(vk, time, ShouldBlockKeyboardInputLocked(), InputEventSource::Poll, false);
    }
    else if (tapWatched && MenuInput::IsMissedTap(true, (keyState & 1) != 0, down, polledDownBefore, key.Down,
                                                  key.PressCount, key.PressCountAtLastPoll))
    {
        key.LastPollDownValid = true;
        key.LastPollDownMs = time;
        SetKeyDown(vk, time, ShouldBlockKeyboardInputLocked(), InputEventSource::PollTap, false);
        SetKeyUpStateOnly(vk, time, InputEventSource::PollTap);
    }
    else if (key.Down)
    {
        SetKeyUpStateOnly(vk, time, InputEventSource::Poll);
    }

    key.PressCountAtLastPoll = key.PressCount;
}

bool PollMouseButtonLocked(int vk, int button, DWORD time, bool firstPollThisFrame, bool tapPrimed)
{
    if (button < 0 || button >= static_cast<int>(_state.MouseButtons.size()))
        return false;

    ButtonState& mouseButton = _state.MouseButtons[button];
    const bool wasDown = mouseButton.Down;
    const SHORT keyState = RealGetAsyncKeyStateSafe(vk);
    const bool down = (keyState & 0x8000) != 0;
    const bool polledDownBefore = mouseButton.PolledDown;
    RecordPollReadingLocked(mouseButton, down, time, firstPollThisFrame);

    bool used = false;

    if (down)
    {
        SetMouseDown(button, time, ShouldBlockMouseInputLocked(), InputEventSource::Poll);
        used = !wasDown;
    }
    // AMDNR 0.3.4.2: a click that went down and up between two polls while the menu is open (GetAsyncKeyState bit 0):
    // one click now (FeedImGui replays it), and the late messages of that click are older than this poll. A click any
    // source delivered since this button's previous poll is not a tap (PressCountAtLastPoll): with OverlayMenu=false
    // a whole click can be pumped and replayed between two polls, and must not click a second time.
    else if (MenuInput::IsMissedTap(tapPrimed, (keyState & 1) != 0, down, polledDownBefore, wasDown,
                                    mouseButton.PressCount, mouseButton.PressCountAtLastPoll))
    {
        mouseButton.LastPollDownValid = true;
        mouseButton.LastPollDownMs = time;
        SetMouseDown(button, time, ShouldBlockMouseInputLocked(), InputEventSource::PollTap);
        SetMouseUpStateOnly(button, time);
        used = true;
    }
    else if (wasDown)
    {
        SetMouseUpStateOnly(button, time);
        used = true;
    }

    mouseButton.PressCountAtLastPoll = mouseButton.PressCount;
    return used;
}

HWND GetPolledCoordinateHwndLocked()
{
    if (_state.InputHwnd != nullptr && IsWindow(_state.InputHwnd))
        return _state.InputHwnd;

    if (_state.TargetHwnd != nullptr && IsWindow(_state.TargetHwnd))
        return _state.TargetHwnd;

    return nullptr;
}

bool ShouldUseExternalVirtualMouseLocked()
{
    return _state.ExternalTargetProcess && _state.InputHwnd == nullptr && _state.TargetHwnd != nullptr &&
           IsWindow(_state.TargetHwnd);
}

bool IsExternalVirtualMouseAuthoritativeLocked()
{
    return ShouldUseExternalVirtualMouseLocked() && _state.ExternalVirtualMouseActive &&
           _state.ExternalVirtualMouseInitialized;
}

void ClampPointToClientLocked(HWND hwnd, POINT& point)
{
    RECT clientRect {};

    if (hwnd == nullptr || !GetClientRect(hwnd, &clientRect))
        return;

    const LONG minX = clientRect.left;
    const LONG minY = clientRect.top;
    const LONG maxX = clientRect.right > clientRect.left ? clientRect.right - 1 : clientRect.left;
    const LONG maxY = clientRect.bottom > clientRect.top ? clientRect.bottom - 1 : clientRect.top;

    if (point.x < minX)
        point.x = minX;
    else if (point.x > maxX)
        point.x = maxX;

    if (point.y < minY)
        point.y = minY;
    else if (point.y > maxY)
        point.y = maxY;
}

POINT GetClientCenterLocked(HWND hwnd)
{
    RECT clientRect {};

    if (hwnd == nullptr || !GetClientRect(hwnd, &clientRect))
        return {};

    POINT center {};
    center.x = (clientRect.left + clientRect.right) / 2;
    center.y = (clientRect.top + clientRect.bottom) / 2;
    return center;
}

bool UpdateExternalVirtualMouseScreenPosLocked(HWND coordinateHwnd)
{
    if (coordinateHwnd == nullptr || !_state.ExternalVirtualMouseInitialized)
        return false;

    POINT screenPos = _state.ExternalVirtualMouseClient;
    if (!ClientToScreen(coordinateHwnd, &screenPos))
        return false;

    _state.MouseScreenPos = screenPos;
    return true;
}

void InitializeExternalVirtualMouseLocked(HWND coordinateHwnd, const POINT& fallbackClientPos)
{
    if (_state.ExternalVirtualMouseInitialized)
        return;

    POINT initialPos = fallbackClientPos;

    if (initialPos.x == 0 && initialPos.y == 0)
        initialPos = GetClientCenterLocked(coordinateHwnd);

    ClampPointToClientLocked(coordinateHwnd, initialPos);

    _state.ExternalVirtualMouseClient = initialPos;
    _state.ExternalVirtualMouseInitialized = true;
    UpdateExternalVirtualMouseScreenPosLocked(coordinateHwnd);

    LOG_INFO("external virtual mouse initialized hwnd:{} pos=({}, {}) screen=({}, {})",
             static_cast<void*>(coordinateHwnd), _state.ExternalVirtualMouseClient.x,
             _state.ExternalVirtualMouseClient.y, _state.MouseScreenPos.x, _state.MouseScreenPos.y);
}

bool ShouldForceImGuiMouseDrawCursorLocked()
{
    return _state.MenuVisible && _state.Focused && _state.ExternalTargetProcess && _state.InputHwnd == nullptr &&
           _state.ExternalVirtualMouseInitialized &&
           (_state.ExternalVirtualMouseActive || _state.ExternalVirtualMouseAuthoritative ||
            _state.ExternalVirtualMouseUsedThisFrame || _state.ExternalVirtualMouseRelativeUsedThisFrame);
}

bool ShouldUseExternalCursorPolicyLocked()
{
    return _state.ExternalTargetProcess && _state.InputHwnd == nullptr && _state.TargetHwnd != nullptr &&
           IsWindow(_state.TargetHwnd);
}

void RestoreImGuiMouseDrawCursorLocked(ImGuiIO& io)
{
    if (_state.ImGuiMouseDrawCursorForced)
    {
        io.MouseDrawCursor = _state.SavedImGuiMouseDrawCursor;
        _state.ImGuiMouseDrawCursorForced = false;
        _state.SavedImGuiMouseDrawCursor = false;
    }

    if (_state.ImGuiNoMouseCursorChangeForced)
    {
        if (_state.SavedImGuiNoMouseCursorChange)
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        else
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;

        _state.ImGuiNoMouseCursorChangeForced = false;
        _state.SavedImGuiNoMouseCursorChange = false;
    }
}

void UpdateImGuiMouseDrawCursorLocked(ImGuiIO& io)
{
    const bool externalCursorPolicy = ShouldUseExternalCursorPolicyLocked();
    const bool shouldDrawSoftwareCursor = ShouldForceImGuiMouseDrawCursorLocked();

    if (externalCursorPolicy)
    {
        if (!_state.ImGuiMouseDrawCursorForced)
        {
            _state.SavedImGuiMouseDrawCursor = io.MouseDrawCursor;
            _state.ImGuiMouseDrawCursorForced = true;
        }

        if (!_state.ImGuiNoMouseCursorChangeForced)
        {
            _state.SavedImGuiNoMouseCursorChange = (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != 0;
            _state.ImGuiNoMouseCursorChangeForced = true;
        }

        // Portal RTX / Remix owns a foreign HWND and continuously recenters or
        // hides the real cursor. Do not let ImGui_ImplWin32 switch the OS cursor
        // back to an arrow in this process. We draw an ImGui software cursor only
        // while the OptiScaler menu owns the virtual cursor. When the menu is
        // closed, force the software cursor off every frame instead of restoring
        // a possibly-stale true value.
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.MouseDrawCursor = shouldDrawSoftwareCursor;

        if (!shouldDrawSoftwareCursor)
            ::SetCursor(nullptr);

        return;
    }

    RestoreImGuiMouseDrawCursorLocked(io);
}

bool ApplyExternalVirtualMouseLocked(HWND coordinateHwnd, const POINT& absoluteClientPos)
{
    _state.ExternalVirtualMouseUsedThisFrame = false;
    _state.ExternalVirtualMouseRelativeUsedThisFrame = false;

    if (!_state.MenuVisible)
    {
        _state.ExternalPendingMouseDeltaX = 0;
        _state.ExternalPendingMouseDeltaY = 0;
        _state.ExternalVirtualMouseActive = false;
        _state.ExternalVirtualMouseAuthoritative = false;
        return false;
    }

    if (!ShouldUseExternalVirtualMouseLocked() || coordinateHwnd == nullptr)
    {
        _state.ExternalVirtualMouseActive = false;
        _state.ExternalVirtualMouseAuthoritative = false;
        return false;
    }

    const LONG deltaX = _state.ExternalPendingMouseDeltaX;
    const LONG deltaY = _state.ExternalPendingMouseDeltaY;
    _state.ExternalPendingMouseDeltaX = 0;
    _state.ExternalPendingMouseDeltaY = 0;

    if (deltaX != 0 || deltaY != 0)
    {
        InitializeExternalVirtualMouseLocked(coordinateHwnd, absoluteClientPos);

        _state.ExternalVirtualMouseClient.x += deltaX;
        _state.ExternalVirtualMouseClient.y += deltaY;
        ClampPointToClientLocked(coordinateHwnd, _state.ExternalVirtualMouseClient);

        _state.ExternalVirtualMouseActive = true;
        _state.ExternalVirtualMouseUsedThisFrame = true;
        _state.ExternalVirtualMouseRelativeUsedThisFrame = true;
        _state.ExternalVirtualMouseFrameCount++;

        OPTIINPUT_LOG_VERBOSE("external virtual mouse delta=({}, {}) pos=({}, {})", deltaX, deltaY,
                              _state.ExternalVirtualMouseClient.x, _state.ExternalVirtualMouseClient.y);
    }

    if (!_state.ExternalVirtualMouseActive || !_state.ExternalVirtualMouseInitialized)
    {
        _state.ExternalVirtualMouseAuthoritative = false;
        return false;
    }

    _state.ExternalVirtualMouseAuthoritative = true;
    _state.ExternalVirtualMouseAuthoritativeFrameCount++;
    _state.ExternalVirtualMouseAbsoluteSuppressedCount++;
    _state.MouseClientPos = _state.ExternalVirtualMouseClient;

    // Keep the virtual cursor coherent in both coordinate spaces. This matters
    // because legacy/manual menu code may still call GetCursorPos and then
    // ScreenToClient. When the menu is open, hkGetCursorPos returns
    // MouseScreenPos, so MouseScreenPos must represent the virtual cursor, not
    // the real hardware cursor that Portal RTX keeps snapping back to center.
    UpdateExternalVirtualMouseScreenPosLocked(coordinateHwnd);
    return true;
}
} // namespace

void PollInputFallbackLocked()
{
    _state.PolledInputActive = false;
    _state.PolledInputUsedThisFrame = false;
    _state.PolledMouseUsedThisFrame = false;
    _state.PolledKeyboardUsedThisFrame = false;
    _state.ExternalVirtualMouseUsedThisFrame = false;
    _state.ExternalVirtualMouseRelativeUsedThisFrame = false;
    _state.ExternalVirtualMouseAuthoritative = false;

    if (!_state.Initialized || !_state.Focused)
    {
        // AMDNR 0.3.4.2: after a frame without a poll the "pressed since the last call" bits can be stale.
        _state.TapGate.NoPoll();
        RefreshInputAcquisitionModeLocked();
        return;
    }

    // Some games never dispatch normal Win32 messages to the render HWND, or they
    // process input via DirectInput / engine polling before our WndProc/message hooks
    // can see anything. Polling here is an input acquisition fallback for Opti's UI;
    // it does not by itself block the game from reading input.
    _state.PolledInputActive = true;

    const DWORD time = GetTickCount();

    POINT screenPos {};
    const bool haveCursor = RealGetCursorPosSafe(&screenPos) != FALSE;
    const HWND coordinateHwnd = GetPolledCoordinateHwndLocked();

    if (haveCursor && coordinateHwnd != nullptr)
    {
        POINT clientPos = screenPos;
        if (ScreenToClient(coordinateHwnd, &clientPos))
        {
            _state.MouseScreenPos = screenPos;

            const bool usedVirtualMouse = ApplyExternalVirtualMouseLocked(coordinateHwnd, clientPos);
            if (!usedVirtualMouse)
            {
                if (clientPos.x != _state.MouseClientPos.x || clientPos.y != _state.MouseClientPos.y)
                    _state.PolledMouseUsedThisFrame = true;

                _state.MouseClientPos = clientPos;
            }
            else if (_state.ExternalVirtualMouseUsedThisFrame)
            {
                _state.PolledMouseUsedThisFrame = true;
            }
        }
    }

    // AMDNR 0.3.4.2: "pressed since the last call" bits believed this poll (EdgeReplay.h, TapBitGate), and whether
    // this is the frame's first poll (BeginFrame can run twice per frame with OverlayMenu=false).
    const bool firstPollThisFrame = !_state.PollRanThisFrame;
    const bool menuKeyTapPrimed = _state.MenuKeyTapWatch && _state.TapGate.Primed(false, _state.MenuVisible);
    const bool navTapPrimed = _state.MenuVisible && MenuClickReplayEnabled() &&
                              _state.TapGate.Primed(true, _state.MenuVisible);

    _state.PolledMouseUsedThisFrame |= PollMouseButtonLocked(VK_LBUTTON, 0, time, firstPollThisFrame, navTapPrimed);
    _state.PolledMouseUsedThisFrame |= PollMouseButtonLocked(VK_RBUTTON, 1, time, firstPollThisFrame, navTapPrimed);
    _state.PolledMouseUsedThisFrame |= PollMouseButtonLocked(VK_MBUTTON, 2, time, firstPollThisFrame, navTapPrimed);
    _state.PolledMouseUsedThisFrame |= PollMouseButtonLocked(VK_XBUTTON1, 3, time, firstPollThisFrame, navTapPrimed);
    _state.PolledMouseUsedThisFrame |= PollMouseButtonLocked(VK_XBUTTON2, 4, time, firstPollThisFrame, navTapPrimed);

    if (_state.PolledMouseUsedThisFrame)
        _state.PolledMouseFrameCount++;

    for (int vk = 0; vk < 256; ++vk)
    {
        const bool wasDown = _state.Keys[vk].Down;
        PollVirtualKeyLocked(vk, time, firstPollThisFrame, menuKeyTapPrimed, navTapPrimed);

        if (_state.Keys[vk].Down != wasDown || _state.Keys[vk].Pressed || _state.Keys[vk].Released)
            _state.PolledKeyboardUsedThisFrame = true;
    }

    if (_state.PolledKeyboardUsedThisFrame)
        _state.PolledKeyboardFrameCount++;

    if (_state.PolledMouseUsedThisFrame || _state.PolledKeyboardUsedThisFrame)
    {
        _state.PolledInputUsedThisFrame = true;
        _state.PolledInputFrameCount++;
    }

    _state.TapGate.AfterPoll(_state.MenuVisible);
    _state.PollRanThisFrame = true;

    RefreshInputAcquisitionModeLocked();
}

void ApplyMenuVisibilityChangeLocked(bool visible)
{
    const bool wasMenuVisible = _state.MenuVisible;

    _state.MenuVisible = visible;
    _state.BlockMouse = visible;
    _state.BlockKeyboard = visible;
    _state.BlockCursor = visible;

    if (wasMenuVisible != visible)
    {
        LOG_INFO("menu visibility changed {} -> {} blockMouse:{} blockKeyboard:{} blockCursor:{} input:{} target:{}",
                 wasMenuVisible ? 1 : 0, visible ? 1 : 0, _state.BlockMouse ? 1 : 0, _state.BlockKeyboard ? 1 : 0,
                 _state.BlockCursor ? 1 : 0, static_cast<void*>(_state.InputHwnd),
                 static_cast<void*>(_state.TargetHwnd));
    }

    if (!visible && _state.ImGuiMouseDrawCursorForced && ImGui::GetCurrentContext() != nullptr)
        UpdateImGuiMouseDrawCursorLocked(ImGui::GetIO());

    if (!wasMenuVisible && visible)
    {
        // AMDNR 0.3.4.2: the menu is open again, so it blocks the key itself.
        EndKeyHoldLocked("the menu opened");

        POINT blockedCursorPos {};
        if (o_GetCursorPos != nullptr && o_GetCursorPos(&blockedCursorPos))
            _state.BlockedCursorScreenPos = blockedCursorPos;
        else
            _state.BlockedCursorScreenPos = _state.MouseScreenPos;

        _state.HasBlockedCursorScreenPos = true;

        LOG_DEBUG("captured blocked cursor position screen:({}, {})", _state.BlockedCursorScreenPos.x,
                  _state.BlockedCursorScreenPos.y);

        BeginCursorClipBlockLocked();
    }
    else if (wasMenuVisible && !visible)
    {
        EndCursorClipBlockLocked();

        _state.HasBlockedCursorScreenPos = false;
        _state.BlockedCursorScreenPos = {};

        // Drop synthetic down/up suppression that only existed while the
        // overlay owned input. New raw handles will rebuild sanitize decisions.
        ResetButtonBlockedStateLocked();
        ResetRawInputBlockStateLocked();
        ResetRawInputSanitizeCacheLocked();
    }
}

bool Initialize(const InitializeOptions& options)
{
    if (DiagInputDisabled())
        return false;

    std::unique_lock lock(_state.Mutex);

    if (_state.CurrentProcessId == 0)
        _state.CurrentProcessId = GetCurrentProcessId();

    LOG_INFO("Initialize requested target:{} input:{} isUwp:{} useSubclass:{} currentPid:{}",
             static_cast<void*>(options.TargetHwnd), static_cast<void*>(options.InputHwnd), options.IsUwp ? 1 : 0,
             options.UseWndProcSubclass ? 1 : 0, _state.CurrentProcessId);
    LogHwndIdentityLocked("Initialize target", options.TargetHwnd);
    LogHwndIdentityLocked("Initialize input", options.InputHwnd);

    if (_state.Initialized)
    {
        if (options.TargetHwnd != nullptr &&
            (options.TargetHwnd != _state.TargetHwnd || options.IsUwp != _state.IsUwp ||
             options.UseWndProcSubclass != _state.UseWndProcSubclass))
        {
            SetTargetWindow(options.TargetHwnd, options.IsUwp, options.UseWndProcSubclass);
        }

        if (options.InputHwnd != nullptr && options.InputHwnd != _state.InputHwnd)
            SetInputWindow(options.InputHwnd, options.UseWndProcSubclass, true);

        LOG_DEBUG(
            "Initialize re-entry state target:{} targetPid:{} input:{} inputPid:{} externalTarget:{} subclassed:{}",
            static_cast<void*>(_state.TargetHwnd), _state.TargetProcessId, static_cast<void*>(_state.InputHwnd),
            _state.InputProcessId, _state.ExternalTargetProcess ? 1 : 0, _state.WndProcSubclassed ? 1 : 0);

        return true;
    }

    _state.Initialized = true;
    _state.IsUwp = options.IsUwp;
    _state.UseWndProcSubclass = options.UseWndProcSubclass;
    _state.CurrentProcessId = GetCurrentProcessId();

    if (options.TargetHwnd != nullptr)
        SetTargetWindow(options.TargetHwnd, options.IsUwp, options.UseWndProcSubclass);

    if (options.InputHwnd != nullptr)
        SetInputWindow(options.InputHwnd, options.UseWndProcSubclass, true);

    const bool hooksInstalled = InstallHooks();
    LOG_INFO("Initialize completed hooksInstalled:{} target:{} targetPid:{} input:{} inputPid:{} externalTarget:{} "
             "subclassed:{}",
             hooksInstalled ? 1 : 0, static_cast<void*>(_state.TargetHwnd), _state.TargetProcessId,
             static_cast<void*>(_state.InputHwnd), _state.InputProcessId, _state.ExternalTargetProcess ? 1 : 0,
             _state.WndProcSubclassed ? 1 : 0);

    if (hooksInstalled)
    {
        UpdateGameInputIntegrationLocked();
        UpdateXInputIntegrationLocked();
        UpdateDirectInputIntegrationLocked();
    }

    return hooksInstalled;
}

bool Initialize(HWND targetHwnd, bool isUwp)
{
    InitializeOptions options {};
    options.TargetHwnd = targetHwnd;
    options.IsUwp = isUwp;
    options.UseWndProcSubclass = !DiagSkipHookGroup(DiagHookGroup::Subclass);

    return Initialize(options);
}

bool Initialize(HWND targetHwnd, HWND inputHwnd, bool isUwp)
{
    InitializeOptions options {};
    options.TargetHwnd = targetHwnd;
    options.InputHwnd = inputHwnd;
    options.IsUwp = isUwp;
    options.UseWndProcSubclass = !DiagSkipHookGroup(DiagHookGroup::Subclass);

    return Initialize(options);
}

void ResetStateAfterShutdown()
{
    if ((_state.ImGuiMouseDrawCursorForced || _state.ImGuiNoMouseCursorChangeForced) &&
        ImGui::GetCurrentContext() != nullptr)
    {
        RestoreImGuiMouseDrawCursorLocked(ImGui::GetIO());
    }

    _state.TargetHwnd = nullptr;
    _state.TargetRootHwnd = nullptr;
    _state.InputHwnd = nullptr;
    _state.InputRootHwnd = nullptr;
    _state.TargetProcessId = 0;
    _state.TargetThreadId = 0;
    _state.InputProcessId = 0;
    _state.InputThreadId = 0;
    _state.CurrentProcessId = 0;

    _state.IsUwp = false;
    _state.UseWndProcSubclass = true;
    _state.WndProcSubclassed = false;
    _state.ExternalTargetProcess = false;
    _state.HasExplicitInputHwnd = false;
    _state.PolledInputActive = false;
    _state.PolledInputUsedThisFrame = false;
    _state.PolledMouseUsedThisFrame = false;
    _state.PolledKeyboardUsedThisFrame = false;
    _state.AcquisitionMode = InputAcquisitionMode::None;
    _state.ExternalVirtualMouseActive = false;
    _state.ExternalVirtualMouseUsedThisFrame = false;
    _state.ExternalVirtualMouseRelativeUsedThisFrame = false;
    _state.ExternalVirtualMouseAuthoritative = false;
    _state.ExternalGetCursorPosVirtualizedThisFrame = false;
    _state.ExternalCursorRecenteringDetected = false;
    _state.ExternalVirtualMouseInitialized = false;
    _state.ExternalLowLevelMouseHookInstalled = false;
    _state.ExternalRawInputSinkRegistered = false;
    _state.ExternalRawInputSinkPumpUsedThisFrame = false;

    _state.OriginalWndProc = nullptr;

    _state.Initialized = false;
    _state.HooksInstalled = false;
    _state.Focused = false;

    _state.MenuVisible = false;
    _state.BlockMouse = false;
    _state.BlockKeyboard = false;
    _state.BlockCursor = false;

    _state.RawMouseTargetHwnd = nullptr;
    _state.RawKeyboardTargetHwnd = nullptr;

    _state.RawMouseFlags = 0;
    _state.RawKeyboardFlags = 0;

    _state.RawMouseRegistered = false;
    _state.RawKeyboardRegistered = false;

    _state.RawMouseNoLegacy = false;
    _state.RawKeyboardNoLegacy = false;

    _state.RawMouseInputSink = false;
    _state.RawKeyboardInputSink = false;
    _state.RawMouseCaptureMouse = false;

    _state.Keys = {};
    _state.MouseButtons = {};

    _state.MouseClientPos = {};
    _state.MouseScreenPos = {};
    _state.LastMouseClientPos = {};
    _state.ExternalVirtualMouseClient = {};
    _state.ExternalLastMouseHookScreen = {};
    _state.ExternalLastMouseHookScreenValid = false;
    _state.ExternalPendingMouseDeltaX = 0;
    _state.ExternalPendingMouseDeltaY = 0;
    _state.ExternalLowLevelMouseHook = nullptr;
    _state.ExternalRawInputSinkHwnd = nullptr;
    _state.ExternalRawInputSinkThreadId = 0;

    ResetRawInputBlockStateLocked();
    ResetRawInputSanitizeCacheLocked();

    _state.WindowsHookSlots = {};

    _state.WindowMessageBlockedCount = 0;
    _state.WindowMessagePassedCount = 0;
    _state.QueueMessageBlockedCount = 0;
    _state.QueueMessagePassedCount = 0;

    _state.GetAsyncKeyStateBlockedCount = 0;
    _state.GetKeyStateBlockedCount = 0;
    _state.GetKeyboardStateFilteredCount = 0;

    _state.GetCursorPosBlockedCount = 0;
    _state.GetPhysicalCursorPosBlockedCount = 0;
    _state.GetMessagePosBlockedCount = 0;
    _state.SetCursorPosBlockedCount = 0;
    _state.SetPhysicalCursorPosBlockedCount = 0;
    _state.ClipCursorBlockedCount = 0;
    _state.GetClipCursorVirtualizedCount = 0;
    _state.SendInputMouseBlockedCount = 0;
    _state.SendInputKeyboardBlockedCount = 0;
    _state.MouseEventBlockedCount = 0;
    _state.PostMouseMessageBlockedCount = 0;
    _state.PostMouseMessagePassedCount = 0;
    _state.SendMouseMessageBlockedCount = 0;
    _state.SendMouseMessagePassedCount = 0;

    _state.RawKeyboardSanitizedCount = 0;
    _state.RawKeyboardPassedCount = 0;
    _state.RawMouseSanitizedCount = 0;
    _state.RawMousePartialPassedCount = 0;
    _state.RawMousePassedCount = 0;

    _state.WindowsHookKeyboardBlockedCount = 0;
    _state.WindowsHookKeyboardPassedCount = 0;
    _state.WindowsHookMouseBlockedCount = 0;
    _state.WindowsHookMousePassedCount = 0;

    _state.PolledInputFrameCount = 0;
    _state.PolledMouseFrameCount = 0;
    _state.PolledKeyboardFrameCount = 0;
    _state.ImGuiMouseDrawCursorForced = false;
    _state.SavedImGuiMouseDrawCursor = false;
    _state.ImGuiNoMouseCursorChangeForced = false;
    _state.SavedImGuiNoMouseCursorChange = false;

    _state.ExternalVirtualMouseFrameCount = 0;
    _state.ExternalVirtualMouseAuthoritativeFrameCount = 0;
    _state.ExternalGetCursorPosVirtualizedCount = 0;
    _state.ExternalVirtualMouseAbsoluteSuppressedCount = 0;
    _state.ExternalMouseDeltaEventCount = 0;
    _state.ExternalCursorRecenteringEventCount = 0;
    _state.ExternalRawInputSinkMessageCount = 0;
    _state.ExternalRawInputSinkPumpFrameCount = 0;

    _state.GameInputModule = nullptr;
    _state.WindowsGamingInputModule = nullptr;
    _state.XInputModule = nullptr;
    _state.DirectInputModule = nullptr;
    _state.DirectInputLegacyModule = nullptr;
    _state.GameInputModuleLoaded = false;
    _state.GameInputCreateExportFound = false;
    _state.GameInputCreateHookInstalled = false;
    _state.GameInputCreateHookAttempted = false;
    _state.GameInputInterfaceSeen = false;
    _state.WindowsGamingInputModuleLoaded = false;
    _state.GameInputLastCreateResult = S_OK;
    _state.GameInputCreateCallCount = 0;
    _state.GameInputCreateSucceededCount = 0;
    _state.GameInputCreateFailedCount = 0;

    _state.XInputModuleLoaded = false;
    _state.XInputGetStateHookInstalled = false;
    _state.XInputGetStateExHookInstalled = false;
    _state.XInputGetKeystrokeHookInstalled = false;
    _state.XInputSetStateHookInstalled = false;
    _state.XInputGetStateCallCount = 0;
    _state.XInputGetStateBlockedCount = 0;
    _state.XInputGetStatePassedCount = 0;
    _state.XInputGetKeystrokeCallCount = 0;
    _state.XInputGetKeystrokeBlockedCount = 0;
    _state.XInputGetKeystrokePassedCount = 0;
    _state.XInputSetStateCallCount = 0;
    _state.XInputSetStateBlockedCount = 0;
    _state.XInputSetStatePassedCount = 0;

    _state.DirectInputModuleLoaded = false;
    _state.DirectInputLegacyModuleLoaded = false;
    _state.DirectInput8CreateHookInstalled = false;
    _state.DirectInputCreateAHookInstalled = false;
    _state.DirectInputCreateWHookInstalled = false;
    _state.DirectInputCreateExHookInstalled = false;
    _state.DirectInputCreateDeviceAHookInstalled = false;
    _state.DirectInputCreateDeviceWHookInstalled = false;
    _state.DirectInputGetDeviceStateHookInstalled = false;
    _state.DirectInputGetDeviceDataHookInstalled = false;
    _state.DirectInputDeviceReleaseHookInstalled = false;
    _state.DirectInputKeyboardDeviceSeen = false;
    _state.DirectInputMouseDeviceSeen = false;
    _state.DirectInputOtherDeviceSeen = false;
    _state.DirectInputDeviceSlots = {};
    _state.HidHandleSlots = {};
    _state.HidMouseHandleSeen = false;
    _state.HidKeyboardHandleSeen = false;
    _state.HidGamepadHandleSeen = false;
    _state.HidOtherHandleSeen = false;
    _state.HidCreateFileCallCount = 0;
    _state.HidTrackedHandleCount = 0;
    _state.HidReadFileCallCount = 0;
    _state.HidReadFileBlockedCount = 0;
    _state.HidReadFilePassedCount = 0;
    _state.HidDeviceIoControlCallCount = 0;
    _state.HidDeviceIoControlBlockedCount = 0;
    _state.HidDeviceIoControlPassedCount = 0;
    _state.MouseMovePointsBlockedCount = 0;
    _state.DirectInputCreateCallCount = 0;
    _state.DirectInputCreateSucceededCount = 0;
    _state.DirectInputCreateFailedCount = 0;
    _state.DirectInputCreateDeviceCallCount = 0;
    _state.DirectInputCreateDeviceSucceededCount = 0;
    _state.DirectInputCreateDeviceFailedCount = 0;
    _state.DirectInputTrackedDeviceCount = 0;
    _state.DirectInputGetDeviceStateCallCount = 0;
    _state.DirectInputGetDeviceStateBlockedCount = 0;
    _state.DirectInputGetDeviceStatePassedCount = 0;
    _state.DirectInputGetDeviceDataCallCount = 0;
    _state.DirectInputGetDeviceDataBlockedCount = 0;
    _state.DirectInputGetDeviceDataPassedCount = 0;

    _state.SavedClipRect = {};
    _state.HasSavedClipRect = false;
    _state.SavedClipWasActive = false;

    _state.DeferredClipRect = {};
    _state.HasDeferredClipRect = false;
    _state.DeferredClipIsNull = false;

    _state.CursorClipReleasedForMenu = false;

    _state.MouseWheel = 0.0f;
    _state.TextInput.clear();

    // AMDNR 0.3.4.2
    _state.MenuKeyVk = 0;
    _state.MenuKeyTapWatch = false;
    _state.MenuKeyEdgeHead = 0;
    _state.MenuKeyEdgeCount = 0;
    _state.MenuKeyEdgesDropped = 0;
    _state.PollRanThisFrame = false;
    _state.TapGate = {};
    _state.HeldKeyVk = 0;
    _state.HeldKeyLastDownMs = 0;
    _state.FedKeyDown = {};
    _state.ReplayedKeyCount = {};
    _state.FedMouseDown = {};
    _state.ReplayedMouseCount = {};
    _state.LastFedMousePos = { LONG_MIN, LONG_MIN };
    _state.DiagWindow = {};
    _state.DiagOpen = {};
    _state.DiagMenuVisible = false;
}

void Shutdown()
{
    std::unique_lock lock(_state.Mutex);

    RemoveWindowSubclass();
    ReleaseTrackedWindowsHooksLocked();
    RemoveExternalRawInputSinkLocked();
    RemoveExternalMouseHookLocked();
    RemoveDirectInputHooksLocked();
    RemoveXInputHooksLocked();
    RemoveGameInputHooksLocked();
    RemoveHooks();

    ResetStateAfterShutdown();
}

static void BeginFrameLocked(HWND targetHwnd, HWND inputHwnd, bool hasInputHwnd, bool isUwp)
{
    // Expected frame order:
    //   BeginFrame() validates game/input HWNDs, subclass/focus.
    //   Hooked WndProc/message/raw APIs accumulate state during message processing.
    //   FeedImGui() publishes the accumulated state.
    //   EndFrame() applies the menu block policy for the next frame and clears transients.

    if (targetHwnd != nullptr && (targetHwnd != _state.TargetHwnd || isUwp != _state.IsUwp))
        SetTargetWindow(targetHwnd, isUwp, _state.UseWndProcSubclass);

    if (hasInputHwnd && inputHwnd != _state.InputHwnd)
        SetInputWindow(inputHwnd, _state.UseWndProcSubclass, true);

    ValidateTargetWindowLocked();
    ValidateInputWindowLocked();
    ValidateWindowSubclassLocked();

    // Optional input APIs may be loaded after OptiInput initialization.
    UpdateGameInputIntegrationLocked();
    UpdateXInputIntegrationLocked();
    UpdateDirectInputIntegrationLocked();

    UpdateFocusState(_state.TargetHwnd);
    EnsureExternalRawInputSinkLocked();
    UpdateExternalMouseHookLocked();
    PumpExternalRawInputSinkLocked();
    PollInputFallbackLocked();
}

void BeginFrame(HWND targetHwnd, bool isUwp)
{
    if (DiagInputDisabled())
        return;

    std::unique_lock lock(_state.Mutex);

    if (!_state.Initialized)
        Initialize(targetHwnd, isUwp);

    BeginFrameLocked(targetHwnd, nullptr, false, isUwp);
}

void BeginFrame(HWND targetHwnd, HWND inputHwnd, bool isUwp)
{
    if (DiagInputDisabled())
        return;

    std::unique_lock lock(_state.Mutex);

    if (!_state.Initialized)
        Initialize(targetHwnd, inputHwnd, isUwp);

    BeginFrameLocked(targetHwnd, inputHwnd, inputHwnd != nullptr, isUwp);
}

void FeedImGui(bool menuVisible)
{
    std::unique_lock lock(_state.Mutex);

    ImGuiIO& io = ImGui::GetIO();

    // When not visible skip feeding input to ImGui and clear the event queue
    if (!menuVisible)
    {
        // This is for virtual mouse mode
        UpdateImGuiMouseDrawCursorLocked(io);

        io.ClearEventsQueue();
        io.ClearInputKeys();
        io.ClearInputMouse();

        // AMDNR 0.3.4.2: ImGui holds nothing down now; edges made while the menu was hidden are never replayed.
        _state.FedKeyDown = {};
        _state.FedMouseDown = {};
        for (int vk = 0; vk < 256; ++vk)
            _state.ReplayedKeyCount[vk] = _state.Keys[vk].PressCount;
        for (std::size_t b = 0; b < _state.MouseButtons.size(); ++b)
            _state.ReplayedMouseCount[b] = _state.MouseButtons[b].PressCount;

        return;
    }

    io.AddFocusEvent(_state.Focused);

    RefreshInputAcquisitionModeLocked();

    OPTIINPUT_LOG_VERBOSE("FeedImGui focused:{} mode:{} input:{} target:{} mouse=({}, {}) wheel:{} textChars:{} "
                          "keyCtrl:{} keyShift:{} keyAlt:{}",
                          _state.Focused ? 1 : 0, AcquisitionModeName(_state.AcquisitionMode),
                          static_cast<void*>(_state.InputHwnd), static_cast<void*>(_state.TargetHwnd),
                          _state.MouseClientPos.x, _state.MouseClientPos.y, _state.MouseWheel,
                          static_cast<unsigned>(_state.TextInput.size()), _state.Keys[VK_CONTROL].Down ? 1 : 0,
                          _state.Keys[VK_SHIFT].Down ? 1 : 0, _state.Keys[VK_MENU].Down ? 1 : 0);

    if (!_state.Focused)
    {
        UpdateImGuiMouseDrawCursorLocked(io);
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);

        io.KeyCtrl = false;
        io.KeyShift = false;
        io.KeyAlt = false;
        io.KeySuper = false;

        // AMDNR 0.3.4.2: ImGui clears its keys and buttons when the app loses focus.
        _state.FedKeyDown = {};
        _state.FedMouseDown = {};

        return;
    }

    auto AddKey = [&](ImGuiKey key, int vk)
    {
        if (vk >= 0 && vk < 256)
            io.AddKeyEvent(key, _state.Keys[vk].Down);
    };

    // AMDNR 0.3.4.2 (Assetto Corsa at 3-10 fps): besides the level, a fresh edge pair inside one frame is replayed
    // (menu/input/EdgeReplay.h): a click shorter than a frame, or a release and a new press. ImGui's trickle queue
    // turns the pair into exactly one click over two frames.
    const bool replayEdges = MenuClickReplayEnabled();
    const DWORD replayNow = GetTickCount();
    const std::uint32_t replayMaxAgeMs = MenuInput::ReplayMaxAgeMs(io.DeltaTime * 1000.0f);

    auto SampleOf = [](const ButtonState& button)
    {
        MenuInput::EdgeSample sample {};
        sample.down = button.Down;
        sample.pressed = button.Pressed;
        sample.released = button.Released;
        sample.pressRepeat = button.PressRepeat;
        sample.pressByPoll =
            button.PressSource == InputEventSource::Poll || button.PressSource == InputEventSource::PollTap;
        sample.pressCount = button.PressCount;
        sample.pressTimeMs = button.PressTimeMs;
        sample.pollDownBeforeValid = button.PollDownBeforeValid;
        sample.pollDownBeforeMs = button.PollDownBeforeMs;
        return sample;
    };

    auto DecideReplayOf = [&](const ButtonState& button, bool lastFedDown, std::uint32_t& lastSeenPressCount)
    {
        MenuInput::ReplayDecision decision {};
        if (replayEdges)
            decision = MenuInput::DecideReplay(SampleOf(button), lastFedDown, lastSeenPressCount, replayNow,
                                               replayMaxAgeMs);
        lastSeenPressCount = button.PressCount;

        if (decision.kind != MenuInput::ReplayKind::None)
            _state.DiagWindow.Replayed++;
        else if (decision.stale)
            _state.DiagWindow.Stale++;

        return decision;
    };

    auto AddNavKey = [&](ImGuiKey key, int vk)
    {
        if (vk < 0 || vk >= 256)
            return;

        const ButtonState& state = _state.Keys[vk];
        const MenuInput::ReplayDecision decision =
            DecideReplayOf(state, _state.FedKeyDown[vk], _state.ReplayedKeyCount[vk]);

        if (decision.kind == MenuInput::ReplayKind::Click)
        {
            io.AddKeyEvent(key, true);
            io.AddKeyEvent(key, false);
            _state.FedKeyDown[vk] = false;
        }
        else if (decision.kind == MenuInput::ReplayKind::Reclick)
        {
            io.AddKeyEvent(key, false);
            io.AddKeyEvent(key, true);
            _state.FedKeyDown[vk] = true;
        }
        else
        {
            io.AddKeyEvent(key, state.Down);
            _state.FedKeyDown[vk] = state.Down;
        }
    };

    const bool polledCtrlDown = (RealGetAsyncKeyStateSafe(VK_CONTROL) & 0x8000) != 0 ||
                                (RealGetAsyncKeyStateSafe(VK_LCONTROL) & 0x8000) != 0 ||
                                (RealGetAsyncKeyStateSafe(VK_RCONTROL) & 0x8000) != 0;

    const bool polledShiftDown = (RealGetAsyncKeyStateSafe(VK_SHIFT) & 0x8000) != 0 ||
                                 (RealGetAsyncKeyStateSafe(VK_LSHIFT) & 0x8000) != 0 ||
                                 (RealGetAsyncKeyStateSafe(VK_RSHIFT) & 0x8000) != 0;

    const bool polledAltDown = (RealGetAsyncKeyStateSafe(VK_MENU) & 0x8000) != 0 ||
                               (RealGetAsyncKeyStateSafe(VK_LMENU) & 0x8000) != 0 ||
                               (RealGetAsyncKeyStateSafe(VK_RMENU) & 0x8000) != 0;

    const bool ctrlDown = _state.Keys[VK_CONTROL].Down || _state.Keys[VK_LCONTROL].Down ||
                          _state.Keys[VK_RCONTROL].Down || polledCtrlDown;

    const bool shiftDown =
        _state.Keys[VK_SHIFT].Down || _state.Keys[VK_LSHIFT].Down || _state.Keys[VK_RSHIFT].Down || polledShiftDown;

    const bool altDown =
        _state.Keys[VK_MENU].Down || _state.Keys[VK_LMENU].Down || _state.Keys[VK_RMENU].Down || polledAltDown;

    // In external/Remix virtual-mouse mode the real OS cursor may be hidden or
    // snapped to the game center. Ask ImGui to draw its own cursor only while
    // the menu actually owns the virtual mouse. Restore the previous setting
    // as soon as the menu closes, focus is lost, or external virtual mode ends.
    UpdateImGuiMouseDrawCursorLocked(io);

    // Legacy modifier fields for older ImGui versions.
    // Must be set before mouse button events.
    io.KeyCtrl = ctrlDown;
    io.KeyShift = shiftDown;
    io.KeyAlt = altDown;
    io.KeySuper = false;

    // Required for ImGui versions using the event input queue.
    // These must happen before mouse button events.
    io.AddKeyEvent(ImGuiMod_Ctrl, ctrlDown);
    io.AddKeyEvent(ImGuiMod_Shift, shiftDown);
    io.AddKeyEvent(ImGuiMod_Alt, altDown);
    io.AddKeyEvent(ImGuiMod_Super, false);

    // Also feed side-specific modifier keys before mouse buttons.
    AddKey(ImGuiKey_LeftCtrl, VK_LCONTROL);
    AddKey(ImGuiKey_RightCtrl, VK_RCONTROL);
    AddKey(ImGuiKey_LeftShift, VK_LSHIFT);
    AddKey(ImGuiKey_RightShift, VK_RSHIFT);
    AddKey(ImGuiKey_LeftAlt, VK_LMENU);
    AddKey(ImGuiKey_RightAlt, VK_RMENU);

    const float mouseX = static_cast<float>(_state.MouseClientPos.x);
    const float mouseY = static_cast<float>(_state.MouseClientPos.y);
    io.AddMousePosEvent(mouseX, mouseY);

    if (_state.MouseClientPos.x != _state.LastFedMousePos.x || _state.MouseClientPos.y != _state.LastFedMousePos.y)
    {
        _state.LastFedMousePos = _state.MouseClientPos;
        _state.DiagWindow.Moves++;
    }

    if (_state.MouseWheel != 0.0f)
        io.AddMouseWheelEvent(0.0f, _state.MouseWheel);

    for (int b = 0; b < static_cast<int>(_state.MouseButtons.size()); ++b)
    {
        const ButtonState& mouseButton = _state.MouseButtons[b];
        const MenuInput::ReplayDecision decision =
            DecideReplayOf(mouseButton, _state.FedMouseDown[b], _state.ReplayedMouseCount[b]);

        if (decision.kind == MenuInput::ReplayKind::Click)
        {
            // Down and up where the button went down (a message's lParam), then back to the cursor.
            if (mouseButton.PressPosValid)
                io.AddMousePosEvent(static_cast<float>(mouseButton.PressPos.x),
                                    static_cast<float>(mouseButton.PressPos.y));
            io.AddMouseButtonEvent(b, true);
            io.AddMouseButtonEvent(b, false);
            if (mouseButton.PressPosValid)
                io.AddMousePosEvent(mouseX, mouseY);
            _state.FedMouseDown[b] = false;
        }
        else if (decision.kind == MenuInput::ReplayKind::Reclick)
        {
            io.AddMouseButtonEvent(b, false);
            io.AddMouseButtonEvent(b, true);
            _state.FedMouseDown[b] = true;
        }
        else
        {
            io.AddMouseButtonEvent(b, mouseButton.Down);
            _state.FedMouseDown[b] = mouseButton.Down;
        }
    }

    AddNavKey(ImGuiKey_Tab, VK_TAB);
    AddNavKey(ImGuiKey_LeftArrow, VK_LEFT);
    AddNavKey(ImGuiKey_RightArrow, VK_RIGHT);
    AddNavKey(ImGuiKey_UpArrow, VK_UP);
    AddNavKey(ImGuiKey_DownArrow, VK_DOWN);
    AddKey(ImGuiKey_PageUp, VK_PRIOR);
    AddKey(ImGuiKey_PageDown, VK_NEXT);
    AddKey(ImGuiKey_Home, VK_HOME);
    AddKey(ImGuiKey_End, VK_END);
    AddKey(ImGuiKey_Insert, VK_INSERT);
    AddKey(ImGuiKey_Delete, VK_DELETE);
    AddKey(ImGuiKey_Backspace, VK_BACK);
    AddNavKey(ImGuiKey_Space, VK_SPACE);
    AddNavKey(ImGuiKey_Enter, VK_RETURN);
    AddNavKey(ImGuiKey_Escape, VK_ESCAPE);

    for (int vk = 'A'; vk <= 'Z'; vk++)
    {
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_A + (vk - 'A')), _state.Keys[vk].Down);
    }

    for (int vk = '0'; vk <= '9'; vk++)
    {
        AddNavKey(static_cast<ImGuiKey>(ImGuiKey_0 + (vk - '0')), vk);
    }

    for (wchar_t ch : _state.TextInput)
        io.AddInputCharacterUTF16(ch);
}

namespace
{
std::string DiagSourceCounts(const std::array<std::uint32_t, InputEventSourceCount>& counts)
{
    return "wnd " + std::to_string(counts[static_cast<std::size_t>(InputEventSource::WndProc)]) + " queue " +
           std::to_string(counts[static_cast<std::size_t>(InputEventSource::Queue)]) + " raw " +
           std::to_string(counts[static_cast<std::size_t>(InputEventSource::Raw)]) + " poll " +
           std::to_string(counts[static_cast<std::size_t>(InputEventSource::Poll)]) + " tap " +
           std::to_string(counts[static_cast<std::size_t>(InputEventSource::PollTap)]);
}

// AMDNR 0.3.4.2 (D2 / D4): while the menu is visible, one INFO line per second (menu/input/InputDiagRate.h: 120 per
// session, then one per 30 s) and a summary when it closes: which source delivered the menu's clicks and keys, how late
// the game's messages were, and whether ImGui turned them into clicks.
void UpdateMenuInputDiagLocked(bool menuVisible)
{
    static MenuInput::LineBudget summaryBudget { 60 };
    const std::uint64_t now = GetTickCount64();
    const bool wasVisible = _state.DiagMenuVisible;
    _state.DiagMenuVisible = menuVisible;

    if (menuVisible && !wasVisible)
    {
        // Opened this frame: count from now on.
        _state.DiagWindow = {};
        _state.DiagOpen = {};
        _state.DiagOpenStartMs = now;
        _state.DiagLastEndFrameMs = now;
        _state.DiagLimiter.Start(now);
        return;
    }

    if (!menuVisible)
    {
        if (wasVisible)
        {
            MenuInputDiagCounters open = _state.DiagOpen;
            open.Add(_state.DiagWindow);

            if (open.Frames != 0 && summaryBudget.Take())
            {
                LOG_INFO("menu input, that open: {:.1f} s, {} frames (avg {} ms), focused {}, foreground elsewhere {} "
                         "(pid {}) | mouse downs {}, replayed {}, stale {}, max message age {} ms | ImGui clicks {}, "
                         "hovered {} | key downs {}",
                         static_cast<double>(now - _state.DiagOpenStartMs) / 1000.0, open.Frames,
                         open.FrameMsSum / open.Frames, open.FocusedFrames, open.ForegroundOtherFrames,
                         open.ForegroundOtherPid, DiagSourceCounts(open.MouseDowns), open.Replayed, open.Stale,
                         open.MaxMsgAgeMs, open.ImGuiClicks, open.HoveredFrames, DiagSourceCounts(open.KeyDowns));
            }
        }

        _state.DiagWindow = {};
        _state.DiagOpen = {};
        return;
    }

    MenuInputDiagCounters& window = _state.DiagWindow;
    window.Frames++;
    window.FrameMsSum += now - _state.DiagLastEndFrameMs;
    _state.DiagLastEndFrameMs = now;

    if (_state.Focused)
        window.FocusedFrames++;

    if (const HWND foreground = GetForegroundWindow(); foreground != nullptr)
    {
        const HWND foregroundRoot = GetAncestor(foreground, GA_ROOT);
        if (foregroundRoot != _state.TargetRootHwnd && foregroundRoot != _state.InputRootHwnd)
        {
            window.ForegroundOtherFrames++;
            DWORD foregroundPid = 0;
            GetWindowThreadProcessId(foreground, &foregroundPid);
            window.ForegroundOtherPid = foregroundPid;
        }
    }

    // RenderMenu calls EndFrame after ImGui::NewFrame, so this frame's clicks and hover are known.
    if (ImGuiContext* context = ImGui::GetCurrentContext(); context != nullptr && context->WithinFrameScope)
    {
        const ImGuiIO& io = context->IO;
        for (int b = 0; b < ImGuiMouseButton_COUNT; ++b)
        {
            if (io.MouseClicked[b])
                window.ImGuiClicks++;
        }

        if (context->HoveredWindow != nullptr)
            window.HoveredFrames++;
    }

    if (!_state.DiagLimiter.Due(now))
        return;

    LOG_INFO("menu input: {} frames (avg {} ms), focused {}, foreground elsewhere {} | mouse downs {}, replayed {}, "
             "stale {}, max message age {} ms | moves {}, ImGui clicks {}, hovered {}, pos ({}, {}) | key downs {}",
             window.Frames, window.Frames != 0 ? window.FrameMsSum / window.Frames : 0, window.FocusedFrames,
             window.ForegroundOtherFrames, DiagSourceCounts(window.MouseDowns), window.Replayed, window.Stale,
             window.MaxMsgAgeMs, window.Moves, window.ImGuiClicks, window.HoveredFrames, _state.MouseClientPos.x,
             _state.MouseClientPos.y, DiagSourceCounts(window.KeyDowns));

    _state.DiagOpen.Add(window);
    window = {};
}
} // namespace

void EndFrame(bool menuVisible)
{
    if (DiagInputDisabled())
        return;

    std::unique_lock lock(_state.Mutex);

    UpdateMenuInputDiagLocked(menuVisible);
    ApplyMenuVisibilityChangeLocked(menuVisible);
    LogInputHealthSnapshotLocked("EndFrame");
    ClearTransientState();
}

void SetMenuVisible(bool visible)
{
    if (DiagInputDisabled())
        return;

    std::unique_lock lock(_state.Mutex);

    ApplyMenuVisibilityChangeLocked(visible);
}

bool IsFocused()
{
    std::unique_lock lock(_state.Mutex);
    return _state.Focused;
}

bool IsKeyDown(int vk)
{
    std::unique_lock lock(_state.Mutex);

    if (vk < 0 || vk >= 256)
        return false;

    return _state.Keys[vk].Down;
}

bool IsKeyPressed(int vk)
{
    std::unique_lock lock(_state.Mutex);

    if (vk < 0 || vk >= 256)
        return false;

    return _state.Keys[vk].Pressed;
}

bool IsKeyReleased(int vk)
{
    std::unique_lock lock(_state.Mutex);

    if (vk < 0 || vk >= 256)
        return false;

    return _state.Keys[vk].Released;
}

int GetLastPressedKey()
{
    std::unique_lock lock(_state.Mutex);
    return _state.LastPressedKey;
}

bool IsMouseDown(int button)
{
    std::unique_lock lock(_state.Mutex);

    if (button < 0 || button >= static_cast<int>(_state.MouseButtons.size()))
        return false;

    return _state.MouseButtons[button].Down;
}

bool IsMousePressed(int button)
{
    std::unique_lock lock(_state.Mutex);

    if (button < 0 || button >= static_cast<int>(_state.MouseButtons.size()))
        return false;

    return _state.MouseButtons[button].Pressed;
}

bool IsMouseReleased(int button)
{
    std::unique_lock lock(_state.Mutex);

    if (button < 0 || button >= static_cast<int>(_state.MouseButtons.size()))
        return false;

    return _state.MouseButtons[button].Released;
}

float GetMouseWheel()
{
    std::unique_lock lock(_state.Mutex);
    return _state.MouseWheel;
}

POINT GetMouseScreenPos()
{
    std::unique_lock lock(_state.Mutex);
    return _state.MouseScreenPos;
}

bool ShouldBlockMouse()
{
    std::unique_lock lock(_state.Mutex);
    return _state.BlockMouse;
}

bool ShouldBlockKeyboard()
{
    std::unique_lock lock(_state.Mutex);
    return _state.BlockKeyboard;
}

bool ShouldBlockCursor()
{
    std::unique_lock lock(_state.Mutex);
    return _state.BlockCursor;
}

bool ShouldBlockVirtualKey(int vk)
{
    std::unique_lock lock(_state.Mutex);

    // AMDNR 0.3.4.2: the menu key that closed the menu, until it is released (HoldKeyUntilReleased).
    if (bypassHookDepth == 0 && IsHeldKeyLocked(vk))
        return true;

    if (!ShouldApplyBlockingPolicyLocked())
        return false;

    if (IsMouseVirtualKey(vk))
        return _state.BlockMouse;

    return _state.BlockKeyboard;
}

DebugState GetDebugState()
{
    std::unique_lock lock(_state.Mutex);

    DebugState state {};

    state.TargetHwnd = _state.TargetHwnd;
    state.TargetRootHwnd = _state.TargetRootHwnd;
    state.InputHwnd = _state.InputHwnd;
    state.InputRootHwnd = _state.InputRootHwnd;
    state.RawMouseTargetHwnd = _state.RawMouseTargetHwnd;
    state.RawKeyboardTargetHwnd = _state.RawKeyboardTargetHwnd;

    state.TargetProcessId = _state.TargetProcessId;
    state.TargetThreadId = _state.TargetThreadId;
    state.InputProcessId = _state.InputProcessId;
    state.InputThreadId = _state.InputThreadId;
    state.CurrentProcessId = _state.CurrentProcessId;
    state.RawMouseFlags = _state.RawMouseFlags;
    state.RawKeyboardFlags = _state.RawKeyboardFlags;

    state.Initialized = _state.Initialized;
    state.HooksInstalled = _state.HooksInstalled;
    state.Focused = _state.Focused;

    state.MenuVisible = _state.MenuVisible;
    state.BlockMouse = _state.BlockMouse;
    state.BlockKeyboard = _state.BlockKeyboard;
    state.BlockCursor = _state.BlockCursor;

    state.IsUwp = _state.IsUwp;
    state.UseWndProcSubclass = _state.UseWndProcSubclass;
    state.WndProcSubclassed = _state.WndProcSubclassed;
    state.ExternalTargetProcess = _state.ExternalTargetProcess;
    state.HasExplicitInputHwnd = _state.HasExplicitInputHwnd;
    state.PolledInputActive = _state.PolledInputActive;
    state.PolledInputUsedThisFrame = _state.PolledInputUsedThisFrame;
    state.PolledMouseUsedThisFrame = _state.PolledMouseUsedThisFrame;
    state.PolledKeyboardUsedThisFrame = _state.PolledKeyboardUsedThisFrame;
    state.AcquisitionMode = _state.AcquisitionMode;
    state.ExternalVirtualMouseActive = _state.ExternalVirtualMouseActive;
    state.ExternalVirtualMouseUsedThisFrame = _state.ExternalVirtualMouseUsedThisFrame;
    state.ExternalVirtualMouseRelativeUsedThisFrame = _state.ExternalVirtualMouseRelativeUsedThisFrame;
    state.ExternalVirtualMouseAuthoritative = _state.ExternalVirtualMouseAuthoritative;
    state.ExternalGetCursorPosVirtualizedThisFrame = _state.ExternalGetCursorPosVirtualizedThisFrame;
    state.ExternalCursorRecenteringDetected = _state.ExternalCursorRecenteringDetected;
    state.ExternalLowLevelMouseHookInstalled = _state.ExternalLowLevelMouseHookInstalled;
    state.ExternalRawInputSinkRegistered = _state.ExternalRawInputSinkRegistered;
    state.ExternalRawInputSinkPumpUsedThisFrame = _state.ExternalRawInputSinkPumpUsedThisFrame;

    state.ReceivedWindowMessageThisFrame = _state.ReceivedWindowMessageThisFrame;
    state.ReceivedQueueMessageThisFrame = _state.ReceivedQueueMessageThisFrame;
    state.ReceivedRawInputThisFrame = _state.ReceivedRawInputThisFrame;
    state.ReceivedAnyInputThisFrame = _state.ReceivedAnyInputThisFrame;

    state.RawMouseRegistered = _state.RawMouseRegistered;
    state.RawKeyboardRegistered = _state.RawKeyboardRegistered;
    state.RawMouseNoLegacy = _state.RawMouseNoLegacy;
    state.RawKeyboardNoLegacy = _state.RawKeyboardNoLegacy;
    state.RawMouseInputSink = _state.RawMouseInputSink;
    state.RawKeyboardInputSink = _state.RawKeyboardInputSink;
    state.RawMouseCaptureMouse = _state.RawMouseCaptureMouse;

    state.WindowsHookTrackedCount = CountTrackedWindowsHooksLocked();

    state.GameInputModuleLoaded = _state.GameInputModuleLoaded;
    state.GameInputCreateExportFound = _state.GameInputCreateExportFound;
    state.GameInputCreateHookInstalled = _state.GameInputCreateHookInstalled;
    state.GameInputCreateHookAttempted = _state.GameInputCreateHookAttempted;
    state.GameInputInterfaceSeen = _state.GameInputInterfaceSeen;
    state.WindowsGamingInputModuleLoaded = _state.WindowsGamingInputModuleLoaded;
    state.XInputModuleLoaded = _state.XInputModuleLoaded;
    state.XInputGetStateHookInstalled = _state.XInputGetStateHookInstalled;
    state.XInputGetStateExHookInstalled = _state.XInputGetStateExHookInstalled;
    state.XInputGetKeystrokeHookInstalled = _state.XInputGetKeystrokeHookInstalled;
    state.XInputSetStateHookInstalled = _state.XInputSetStateHookInstalled;
    state.DirectInputModuleLoaded = _state.DirectInputModuleLoaded;
    state.DirectInputLegacyModuleLoaded = _state.DirectInputLegacyModuleLoaded;
    state.DirectInput8CreateHookInstalled = _state.DirectInput8CreateHookInstalled;
    state.DirectInputCreateAHookInstalled = _state.DirectInputCreateAHookInstalled;
    state.DirectInputCreateWHookInstalled = _state.DirectInputCreateWHookInstalled;
    state.DirectInputCreateExHookInstalled = _state.DirectInputCreateExHookInstalled;
    state.DirectInputCreateDeviceAHookInstalled = _state.DirectInputCreateDeviceAHookInstalled;
    state.DirectInputCreateDeviceWHookInstalled = _state.DirectInputCreateDeviceWHookInstalled;
    state.DirectInputGetDeviceStateHookInstalled = _state.DirectInputGetDeviceStateHookInstalled;
    state.DirectInputGetDeviceDataHookInstalled = _state.DirectInputGetDeviceDataHookInstalled;
    state.DirectInputDeviceReleaseHookInstalled = _state.DirectInputDeviceReleaseHookInstalled;
    state.DirectInputKeyboardDeviceSeen = _state.DirectInputKeyboardDeviceSeen;
    state.DirectInputMouseDeviceSeen = _state.DirectInputMouseDeviceSeen;
    state.DirectInputOtherDeviceSeen = _state.DirectInputOtherDeviceSeen;
    state.GameInputLastCreateResult = _state.GameInputLastCreateResult;
    state.GameInputCreateCallCount = _state.GameInputCreateCallCount;
    state.GameInputCreateSucceededCount = _state.GameInputCreateSucceededCount;
    state.GameInputCreateFailedCount = _state.GameInputCreateFailedCount;

    state.XInputGetStateCallCount = _state.XInputGetStateCallCount;
    state.XInputGetStateBlockedCount = _state.XInputGetStateBlockedCount;
    state.XInputGetStatePassedCount = _state.XInputGetStatePassedCount;
    state.XInputGetKeystrokeCallCount = _state.XInputGetKeystrokeCallCount;
    state.XInputGetKeystrokeBlockedCount = _state.XInputGetKeystrokeBlockedCount;
    state.XInputGetKeystrokePassedCount = _state.XInputGetKeystrokePassedCount;
    state.XInputSetStateCallCount = _state.XInputSetStateCallCount;
    state.XInputSetStateBlockedCount = _state.XInputSetStateBlockedCount;
    state.XInputSetStatePassedCount = _state.XInputSetStatePassedCount;

    state.DirectInputCreateCallCount = _state.DirectInputCreateCallCount;
    state.DirectInputCreateSucceededCount = _state.DirectInputCreateSucceededCount;
    state.DirectInputCreateFailedCount = _state.DirectInputCreateFailedCount;
    state.DirectInputCreateDeviceCallCount = _state.DirectInputCreateDeviceCallCount;
    state.DirectInputCreateDeviceSucceededCount = _state.DirectInputCreateDeviceSucceededCount;
    state.DirectInputCreateDeviceFailedCount = _state.DirectInputCreateDeviceFailedCount;
    state.DirectInputTrackedDeviceCount = _state.DirectInputTrackedDeviceCount;
    state.DirectInputGetDeviceStateCallCount = _state.DirectInputGetDeviceStateCallCount;
    state.DirectInputGetDeviceStateBlockedCount = _state.DirectInputGetDeviceStateBlockedCount;
    state.DirectInputGetDeviceStatePassedCount = _state.DirectInputGetDeviceStatePassedCount;
    state.DirectInputGetDeviceDataCallCount = _state.DirectInputGetDeviceDataCallCount;
    state.DirectInputGetDeviceDataBlockedCount = _state.DirectInputGetDeviceDataBlockedCount;
    state.DirectInputGetDeviceDataPassedCount = _state.DirectInputGetDeviceDataPassedCount;

    state.HidMouseHandleSeen = _state.HidMouseHandleSeen;
    state.HidKeyboardHandleSeen = _state.HidKeyboardHandleSeen;
    state.HidGamepadHandleSeen = _state.HidGamepadHandleSeen;
    state.HidOtherHandleSeen = _state.HidOtherHandleSeen;
    state.HidCreateFileCallCount = _state.HidCreateFileCallCount;
    state.HidTrackedHandleCount = _state.HidTrackedHandleCount;
    state.HidReadFileCallCount = _state.HidReadFileCallCount;
    state.HidReadFileBlockedCount = _state.HidReadFileBlockedCount;
    state.HidReadFilePassedCount = _state.HidReadFilePassedCount;
    state.HidDeviceIoControlCallCount = _state.HidDeviceIoControlCallCount;
    state.HidDeviceIoControlBlockedCount = _state.HidDeviceIoControlBlockedCount;
    state.HidDeviceIoControlPassedCount = _state.HidDeviceIoControlPassedCount;
    state.MouseMovePointsBlockedCount = _state.MouseMovePointsBlockedCount;

    state.RawKeyboardSanitizedCount = _state.RawKeyboardSanitizedCount;
    state.RawKeyboardPassedCount = _state.RawKeyboardPassedCount;
    state.RawMouseSanitizedCount = _state.RawMouseSanitizedCount;
    state.RawMousePartialPassedCount = _state.RawMousePartialPassedCount;
    state.RawMousePassedCount = _state.RawMousePassedCount;

    state.WindowsHookKeyboardBlockedCount = _state.WindowsHookKeyboardBlockedCount;
    state.WindowsHookKeyboardPassedCount = _state.WindowsHookKeyboardPassedCount;
    state.WindowsHookMouseBlockedCount = _state.WindowsHookMouseBlockedCount;
    state.WindowsHookMousePassedCount = _state.WindowsHookMousePassedCount;

    state.PolledInputFrameCount = _state.PolledInputFrameCount;
    state.PolledMouseFrameCount = _state.PolledMouseFrameCount;
    state.PolledKeyboardFrameCount = _state.PolledKeyboardFrameCount;
    state.ExternalVirtualMouseFrameCount = _state.ExternalVirtualMouseFrameCount;
    state.ExternalVirtualMouseAuthoritativeFrameCount = _state.ExternalVirtualMouseAuthoritativeFrameCount;
    state.ExternalGetCursorPosVirtualizedCount = _state.ExternalGetCursorPosVirtualizedCount;
    state.ExternalVirtualMouseAbsoluteSuppressedCount = _state.ExternalVirtualMouseAbsoluteSuppressedCount;
    state.ExternalMouseDeltaEventCount = _state.ExternalMouseDeltaEventCount;
    state.ExternalCursorRecenteringEventCount = _state.ExternalCursorRecenteringEventCount;
    state.ExternalRawInputSinkMessageCount = _state.ExternalRawInputSinkMessageCount;
    state.ExternalRawInputSinkPumpFrameCount = _state.ExternalRawInputSinkPumpFrameCount;
    state.ExternalVirtualMouseClient = _state.ExternalVirtualMouseClient;
    state.ExternalPendingMouseDeltaX = _state.ExternalPendingMouseDeltaX;
    state.ExternalPendingMouseDeltaY = _state.ExternalPendingMouseDeltaY;

    return state;
}

bool IsExternalVirtualMouseAuthoritative()
{
    std::unique_lock lock(_state.Mutex);
    return IsExternalVirtualMouseAuthoritativeLocked();
}

InputAcquisitionMode GetInputAcquisitionMode()
{
    std::unique_lock lock(_state.Mutex);
    RefreshInputAcquisitionModeLocked();
    return _state.AcquisitionMode;
}

void ResetMenuInputTransientState()
{
    std::unique_lock lock(_state.Mutex);

    _state.ExternalPendingMouseDeltaX = 0;
    _state.ExternalPendingMouseDeltaY = 0;
    _state.MouseWheel = 0.0f;
    _state.TextInput.clear();

    for (auto& key : _state.Keys)
    {
        key.Pressed = false;
        key.Released = false;
    }

    for (auto& button : _state.MouseButtons)
    {
        button.Pressed = false;
        button.Released = false;
    }

    // AMDNR 0.3.4.2: the menu opens with ImGui's input cleared; nothing from before is replayed into it.
    _state.FedKeyDown = {};
    _state.FedMouseDown = {};
    for (int vk = 0; vk < 256; ++vk)
        _state.ReplayedKeyCount[vk] = _state.Keys[vk].PressCount;
    for (std::size_t b = 0; b < _state.MouseButtons.size(); ++b)
        _state.ReplayedMouseCount[b] = _state.MouseButtons[b].PressCount;
}

KeyObservation GetKeyObservation(int vk)
{
    std::unique_lock lock(_state.Mutex);

    KeyObservation observation {};

    if (vk < 0 || vk >= 256)
        return observation;

    const ButtonState& key = _state.Keys[vk];
    observation.Down = key.Down;
    observation.Pressed = key.Pressed;
    observation.Released = key.Released;
    observation.PressCount = key.PressCount;
    observation.PressTimeMs = key.PressTimeMs;
    observation.PressSource = key.PressSource;
    observation.PressRepeat = key.PressRepeat;
    observation.PolledThisFrame = _state.PollRanThisFrame && ShouldPollVirtualKey(vk);
    observation.PolledDown = key.PolledDown;
    observation.PollDownBeforeValid = key.PollDownBeforeValid;
    observation.PollDownBeforeMs = key.PollDownBeforeMs;
    return observation;
}

void SetMenuKey(int vk, bool readTapBit)
{
    std::unique_lock lock(_state.Mutex);

    const int key = vk > 0 && vk < 256 ? vk : 0;
    if (key != _state.MenuKeyVk)
    {
        _state.MenuKeyVk = key;
        _state.MenuKeyEdgeHead = 0;
        _state.MenuKeyEdgeCount = 0;
        EndKeyHoldLocked("the menu key changed");
    }

    _state.MenuKeyTapWatch = key != 0 && readTapBit;
}

std::size_t DrainMenuKeyEdges(KeyEdgeRecord* out, std::size_t maxCount, std::size_t* dropped)
{
    std::unique_lock lock(_state.Mutex);

    std::size_t written = 0;
    std::size_t lost = _state.MenuKeyEdgesDropped;

    while (_state.MenuKeyEdgeCount != 0)
    {
        if (out != nullptr && written < maxCount)
            out[written++] = _state.MenuKeyEdges[_state.MenuKeyEdgeHead];
        else
            lost++;

        _state.MenuKeyEdgeHead = (_state.MenuKeyEdgeHead + 1) % _state.MenuKeyEdges.size();
        _state.MenuKeyEdgeCount--;
    }

    _state.MenuKeyEdgeHead = 0;
    _state.MenuKeyEdgesDropped = 0;

    if (dropped != nullptr)
        *dropped = lost;

    return written;
}

void HoldKeyUntilReleased(int vk)
{
    std::unique_lock lock(_state.Mutex);

    if (vk <= 0 || vk >= 256 || IsMouseVirtualKey(vk))
        return;

    // Already up: the poll found the whole press between two frames (a tap: SetKeyDown and SetKeyUpStateOnly in one
    // poll), so there is no release left to wait for -- but the game's own WM_KEYDOWN / WM_KEYUP of that same
    // physical press can still arrive up to about 2 s later (Assetto Corsa at 3-10 fps) with the menu already
    // closed, and nothing else would keep that stray press from the game. The hold then waits for exactly that late
    // pair (MenuInput::kHoldLatePairTimeoutMs, ended by the pair's key-up), instead of a release with the 3 s
    // window refreshed while the key is seen down.
    const ButtonState& key = _state.Keys[vk];
    const bool stillDown = key.Down || key.PolledDown;

    _state.HeldKeyVk = vk;
    _state.HeldKeyAwaitingLatePair = !stillDown;
    _state.HeldKeyLastDownMs = GetTickCount();

    if (!stillDown)
        LOG_DEBUG("menu key {:#x} closed the menu and already reads up: held back from the game until the late "
                  "messages of that press arrive, at most {} ms",
                  vk, MenuInput::kHoldLatePairTimeoutMs);
}

void LogMenuOpenDiagnostics(int backBufferWidth, int backBufferHeight, float imguiDisplayWidth,
                            float imguiDisplayHeight)
{
    if (DiagInputDisabled())
        return;

    std::unique_lock lock(_state.Mutex);

    // AMDNR 0.3.4.2 (D3): the first opens of a session only.
    static MenuInput::LineBudget budget { 5 };
    if (!budget.Take())
        return;

    const HWND target = _state.TargetHwnd;

    RECT client {};
    RECT window {};
    const bool haveClient = target != nullptr && GetClientRect(target, &client) != FALSE;
    const bool haveWindow = target != nullptr && GetWindowRect(target, &window) != FALSE;

    // DPI (user32 of Windows 10 1607 and later, resolved at run time): -1 = not available.
    // (DPI_AWARENESS_CONTEXT is a handle and DPI_AWARENESS an int enum; spelled so for any SDK target.)
    using GetDpiForWindow_t = UINT(WINAPI*)(HWND);
    using GetThreadDpiAwarenessContext_t = HANDLE(WINAPI*)();
    using GetWindowDpiAwarenessContext_t = HANDLE(WINAPI*)(HWND);
    using GetAwarenessFromDpiAwarenessContext_t = int(WINAPI*)(HANDLE);

    int dpi = -1;
    int threadAwareness = -1;
    int windowAwareness = -1;

    if (const HMODULE user32 = GetModuleHandleW(L"user32.dll"); user32 != nullptr)
    {
        const auto getDpiForWindow =
            reinterpret_cast<GetDpiForWindow_t>(GetProcAddress(user32, "GetDpiForWindow"));
        const auto getThreadContext =
            reinterpret_cast<GetThreadDpiAwarenessContext_t>(GetProcAddress(user32, "GetThreadDpiAwarenessContext"));
        const auto getWindowContext =
            reinterpret_cast<GetWindowDpiAwarenessContext_t>(GetProcAddress(user32, "GetWindowDpiAwarenessContext"));
        const auto getAwareness = reinterpret_cast<GetAwarenessFromDpiAwarenessContext_t>(
            GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext"));

        if (getDpiForWindow != nullptr && target != nullptr)
            dpi = static_cast<int>(getDpiForWindow(target));

        if (getAwareness != nullptr && getThreadContext != nullptr)
            threadAwareness = static_cast<int>(getAwareness(getThreadContext()));

        if (getAwareness != nullptr && getWindowContext != nullptr && target != nullptr)
            windowAwareness = static_cast<int>(getAwareness(getWindowContext(target)));
    }

    RECT clip {};
    BOOL haveClip = FALSE;
    {
        ScopedHookBypass bypass;
        haveClip = o_GetClipCursor != nullptr ? o_GetClipCursor(&clip) : ::GetClipCursor(&clip);
    }

    POINT cursor {};
    POINT cursorClient {};
    if (RealGetCursorPosSafe(&cursor))
    {
        cursorClient = cursor;
        if (target != nullptr)
            ScreenToClient(target, &cursorClient);
    }

    const HWND foreground = GetForegroundWindow();
    DWORD foregroundPid = 0;
    if (foreground != nullptr)
        GetWindowThreadProcessId(foreground, &foregroundPid);
    const HWND foregroundRoot = foreground != nullptr ? GetAncestor(foreground, GA_ROOT) : nullptr;
    const bool foregroundIsGame =
        foregroundRoot != nullptr && (foregroundRoot == _state.TargetRootHwnd || foregroundRoot == _state.InputRootHwnd);

    LOG_INFO("menu open #{}: client {}x{} window {}x{} back buffer {}x{} ImGui display {:.0f}x{:.0f} | dpi {} "
             "awareness thread {} window {} | cursor client ({}, {}) clip {} ({}, {}, {}, {}) | foreground {} pid {} "
             "focused {} subclassed {}",
             budget.used, haveClient ? client.right - client.left : -1, haveClient ? client.bottom - client.top : -1,
             haveWindow ? window.right - window.left : -1, haveWindow ? window.bottom - window.top : -1,
             backBufferWidth, backBufferHeight, imguiDisplayWidth, imguiDisplayHeight, dpi, threadAwareness,
             windowAwareness, cursorClient.x, cursorClient.y, haveClip ? "yes" : "no", clip.left, clip.top, clip.right,
             clip.bottom, foregroundIsGame ? "game" : "other", foregroundPid, _state.Focused ? 1 : 0,
             _state.WndProcSubclassed ? 1 : 0);

    // Raw input registrations of this process: usage page / usage, flags (0x30 NOLEGACY, 0x100 INPUTSINK, 0x200
    // CAPTUREMOUSE / NOHOTKEYS, 0x1000 EXINPUTSINK), and whether their target is the game window.
    std::string rawDevices;
    UINT deviceCount = 0;
    if (GetRegisteredRawInputDevices(nullptr, &deviceCount, sizeof(RAWINPUTDEVICE)) == 0 && deviceCount != 0)
    {
        std::vector<RAWINPUTDEVICE> devices(deviceCount);
        const UINT got = GetRegisteredRawInputDevices(devices.data(), &deviceCount, sizeof(RAWINPUTDEVICE));
        if (got != static_cast<UINT>(-1))
        {
            for (UINT i = 0; i < got && i < 16; ++i)
            {
                const RAWINPUTDEVICE& device = devices[i];
                const char* targetName = device.hwndTarget == nullptr ? "focus"
                                         : (device.hwndTarget == target || device.hwndTarget == _state.InputHwnd)
                                             ? "game"
                                             : "other";
                char item[96] {};
                std::snprintf(item, sizeof(item), "%s0x%x/0x%x flags 0x%lx target %s", rawDevices.empty() ? "" : ", ",
                              static_cast<unsigned>(device.usUsagePage), static_cast<unsigned>(device.usUsage),
                              static_cast<unsigned long>(device.dwFlags), targetName);
                rawDevices += item;
            }
        }
    }

    unsigned hooks[4] {}; // WH_KEYBOARD (2), WH_MOUSE (7), WH_KEYBOARD_LL (13), WH_MOUSE_LL (14)
    for (const WindowsHookSlot& slot : _state.WindowsHookSlots)
    {
        if (!slot.InUse || slot.Hook == nullptr)
            continue;
        switch (slot.HookType)
        {
        case WH_KEYBOARD:
            hooks[0]++;
            break;
        case WH_MOUSE:
            hooks[1]++;
            break;
        case WH_KEYBOARD_LL:
            hooks[2]++;
            break;
        case WH_MOUSE_LL:
            hooks[3]++;
            break;
        default:
            break;
        }
    }

    LOG_INFO("menu open #{}: raw input {} | tracked hooks WH_KEYBOARD {} WH_MOUSE {} WH_KEYBOARD_LL {} WH_MOUSE_LL {} | "
             "DirectInput devices {} (keyboard {} mouse {}), CreateDevice calls {} | menu key {:#x} on press {} | "
             "click replay {}",
             budget.used, rawDevices.empty() ? "none" : rawDevices, hooks[0], hooks[1], hooks[2], hooks[3],
             _state.DirectInputTrackedDeviceCount, _state.DirectInputKeyboardDeviceSeen ? 1 : 0,
             _state.DirectInputMouseDeviceSeen ? 1 : 0, _state.DirectInputCreateDeviceCallCount, _state.MenuKeyVk,
             _state.MenuKeyTapWatch ? 1 : 0, MenuClickReplayEnabled() ? 1 : 0);
}

} // namespace OptiInput
