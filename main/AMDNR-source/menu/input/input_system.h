// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>

namespace OptiInput
{
enum class InputAcquisitionMode : std::uint32_t
{
    None = 0,
    WindowMessages = 1,
    RawInput = 2,
    PolledAbsolute = 3,
    ExternalRawVirtualMouse = 4,
};

struct InitializeOptions
{
    // Target/game window. Can be foreign-process in external overlay mode.
    HWND TargetHwnd = nullptr;

    // Local input window. Null means TargetHwnd is used when it belongs to this process.
    HWND InputHwnd = nullptr;

    bool IsUwp = false;
    bool UseWndProcSubclass = true;
};

struct DebugState
{
    HWND TargetHwnd = nullptr;
    HWND TargetRootHwnd = nullptr;
    HWND InputHwnd = nullptr;
    HWND InputRootHwnd = nullptr;
    HWND RawMouseTargetHwnd = nullptr;
    HWND RawKeyboardTargetHwnd = nullptr;

    DWORD TargetProcessId = 0;
    DWORD TargetThreadId = 0;
    DWORD InputProcessId = 0;
    DWORD InputThreadId = 0;
    DWORD CurrentProcessId = 0;
    DWORD RawMouseFlags = 0;
    DWORD RawKeyboardFlags = 0;

    bool Initialized = false;
    bool HooksInstalled = false;
    bool Focused = false;

    bool MenuVisible = false;
    bool BlockMouse = false;
    bool BlockKeyboard = false;
    bool BlockCursor = false;

    bool IsUwp = false;
    bool UseWndProcSubclass = true;
    bool WndProcSubclassed = false;
    bool ExternalTargetProcess = false;
    bool HasExplicitInputHwnd = false;

    bool PolledInputActive = false;
    bool PolledInputUsedThisFrame = false;
    bool PolledMouseUsedThisFrame = false;
    bool PolledKeyboardUsedThisFrame = false;

    InputAcquisitionMode AcquisitionMode = InputAcquisitionMode::None;

    bool ExternalVirtualMouseActive = false;
    bool ExternalVirtualMouseUsedThisFrame = false;
    bool ExternalVirtualMouseRelativeUsedThisFrame = false;
    bool ExternalVirtualMouseAuthoritative = false;
    bool ExternalGetCursorPosVirtualizedThisFrame = false;
    bool ExternalCursorRecenteringDetected = false;
    bool ExternalLowLevelMouseHookInstalled = false;
    bool ExternalRawInputSinkRegistered = false;
    bool ExternalRawInputSinkPumpUsedThisFrame = false;

    bool ReceivedWindowMessageThisFrame = false;
    bool ReceivedQueueMessageThisFrame = false;
    bool ReceivedRawInputThisFrame = false;
    bool ReceivedAnyInputThisFrame = false;

    bool RawMouseRegistered = false;
    bool RawKeyboardRegistered = false;
    bool RawMouseNoLegacy = false;
    bool RawKeyboardNoLegacy = false;
    bool RawMouseInputSink = false;
    bool RawKeyboardInputSink = false;
    bool RawMouseCaptureMouse = false;

    std::uint32_t WindowsHookTrackedCount = 0;

    bool GameInputModuleLoaded = false;
    bool GameInputCreateExportFound = false;
    bool GameInputCreateHookInstalled = false;
    bool GameInputCreateHookAttempted = false;
    bool GameInputInterfaceSeen = false;
    bool WindowsGamingInputModuleLoaded = false;

    bool XInputModuleLoaded = false;
    bool XInputGetStateHookInstalled = false;
    bool XInputGetStateExHookInstalled = false;
    bool XInputGetKeystrokeHookInstalled = false;
    bool XInputSetStateHookInstalled = false;

    bool DirectInputModuleLoaded = false;
    bool DirectInputLegacyModuleLoaded = false;
    bool DirectInput8CreateHookInstalled = false;
    bool DirectInputCreateAHookInstalled = false;
    bool DirectInputCreateWHookInstalled = false;
    bool DirectInputCreateExHookInstalled = false;
    bool DirectInputCreateDeviceAHookInstalled = false;
    bool DirectInputCreateDeviceWHookInstalled = false;
    bool DirectInputGetDeviceStateHookInstalled = false;
    bool DirectInputGetDeviceDataHookInstalled = false;
    bool DirectInputDeviceReleaseHookInstalled = false;
    bool DirectInputKeyboardDeviceSeen = false;
    bool DirectInputMouseDeviceSeen = false;
    bool DirectInputOtherDeviceSeen = false;

    HRESULT GameInputLastCreateResult = S_OK;

    std::uint64_t GameInputCreateCallCount = 0;
    std::uint64_t GameInputCreateSucceededCount = 0;
    std::uint64_t GameInputCreateFailedCount = 0;

    std::uint64_t XInputGetStateCallCount = 0;
    std::uint64_t XInputGetStateBlockedCount = 0;
    std::uint64_t XInputGetStatePassedCount = 0;
    std::uint64_t XInputGetKeystrokeCallCount = 0;
    std::uint64_t XInputGetKeystrokeBlockedCount = 0;
    std::uint64_t XInputGetKeystrokePassedCount = 0;
    std::uint64_t XInputSetStateCallCount = 0;
    std::uint64_t XInputSetStateBlockedCount = 0;
    std::uint64_t XInputSetStatePassedCount = 0;

    std::uint64_t DirectInputCreateCallCount = 0;
    std::uint64_t DirectInputCreateSucceededCount = 0;
    std::uint64_t DirectInputCreateFailedCount = 0;
    std::uint64_t DirectInputCreateDeviceCallCount = 0;
    std::uint64_t DirectInputCreateDeviceSucceededCount = 0;
    std::uint64_t DirectInputCreateDeviceFailedCount = 0;
    std::uint64_t DirectInputTrackedDeviceCount = 0;
    std::uint64_t DirectInputGetDeviceStateCallCount = 0;
    std::uint64_t DirectInputGetDeviceStateBlockedCount = 0;
    std::uint64_t DirectInputGetDeviceStatePassedCount = 0;
    std::uint64_t DirectInputGetDeviceDataCallCount = 0;
    std::uint64_t DirectInputGetDeviceDataBlockedCount = 0;
    std::uint64_t DirectInputGetDeviceDataPassedCount = 0;

    bool HidMouseHandleSeen = false;
    bool HidKeyboardHandleSeen = false;
    bool HidGamepadHandleSeen = false;
    bool HidOtherHandleSeen = false;
    std::uint64_t HidCreateFileCallCount = 0;
    std::uint64_t HidTrackedHandleCount = 0;
    std::uint64_t HidReadFileCallCount = 0;
    std::uint64_t HidReadFileBlockedCount = 0;
    std::uint64_t HidReadFilePassedCount = 0;
    std::uint64_t HidDeviceIoControlCallCount = 0;
    std::uint64_t HidDeviceIoControlBlockedCount = 0;
    std::uint64_t HidDeviceIoControlPassedCount = 0;
    std::uint64_t MouseMovePointsBlockedCount = 0;

    std::uint64_t RawKeyboardSanitizedCount = 0;
    std::uint64_t RawKeyboardPassedCount = 0;
    std::uint64_t RawMouseSanitizedCount = 0;
    std::uint64_t RawMousePartialPassedCount = 0;
    std::uint64_t RawMousePassedCount = 0;

    std::uint64_t WindowsHookKeyboardBlockedCount = 0;
    std::uint64_t WindowsHookKeyboardPassedCount = 0;
    std::uint64_t WindowsHookMouseBlockedCount = 0;
    std::uint64_t WindowsHookMousePassedCount = 0;

    std::uint64_t PolledInputFrameCount = 0;
    std::uint64_t PolledMouseFrameCount = 0;
    std::uint64_t PolledKeyboardFrameCount = 0;

    std::uint64_t ExternalVirtualMouseFrameCount = 0;
    std::uint64_t ExternalVirtualMouseAuthoritativeFrameCount = 0;
    std::uint64_t ExternalGetCursorPosVirtualizedCount = 0;
    std::uint64_t ExternalVirtualMouseAbsoluteSuppressedCount = 0;
    std::uint64_t ExternalMouseDeltaEventCount = 0;
    std::uint64_t ExternalCursorRecenteringEventCount = 0;
    std::uint64_t ExternalRawInputSinkMessageCount = 0;
    std::uint64_t ExternalRawInputSinkPumpFrameCount = 0;

    POINT ExternalVirtualMouseClient = {};
    LONG ExternalPendingMouseDeltaX = 0;
    LONG ExternalPendingMouseDeltaY = 0;
};

bool Initialize(const InitializeOptions& options);
bool Initialize(HWND targetHwnd, bool isUwp = false);
bool Initialize(HWND targetHwnd, HWND inputHwnd, bool isUwp = false);

void Shutdown();

void BeginFrame(HWND targetHwnd, bool isUwp = false);
void BeginFrame(HWND targetHwnd, HWND inputHwnd, bool isUwp = false);
void FeedImGui(bool menuVisible);
void EndFrame(bool menuVisible);

void SetMenuVisible(bool visible);
void ResetMenuInputTransientState();

bool IsFocused();

DebugState GetDebugState();

bool IsKeyDown(int vk);
bool IsKeyPressed(int vk);
bool IsKeyReleased(int vk);
int GetLastPressedKey();

bool IsMouseDown(int button);
bool IsMousePressed(int button);
bool IsMouseReleased(int button);

float GetMouseWheel();
POINT GetMouseScreenPos();

bool ShouldBlockMouse();
bool ShouldBlockKeyboard();
bool ShouldBlockCursor();
bool ShouldBlockVirtualKey(int vk);

bool IsExternalVirtualMouseAuthoritative();
InputAcquisitionMode GetInputAcquisitionMode();

// AMDNR 0.3.4.2 (Assetto Corsa menu): where a key or button edge came from, its event time, and the menu key's own
// edge stream, so the menu toggles once per physical press (menu/input/KeyPressLatch.h), low-fps clicks are replayed
// (menu/input/EdgeReplay.h) and the log says which source reaches the menu and how late.
enum class InputEventSource : std::uint8_t
{
    None,
    Poll,    // GetAsyncKeyState level read once per frame (BeginFrame)
    PollTap, // GetAsyncKeyState "pressed since the last call" bit: a tap between two polls
    Queue,   // PeekMessage / GetMessage detour (message removed from the queue)
    WndProc, // the WndProc subclass
    Raw,     // WM_INPUT
};
constexpr std::size_t InputEventSourceCount = 6;
const char* InputEventSourceName(InputEventSource source);

// One key as the menu code sees it now. Press* describe the freshest press edge since the last EndFrame.
struct KeyObservation
{
    bool Down = false;
    bool Pressed = false;
    bool Released = false;
    std::uint32_t PressCount = 0; // press edges so far (never reset): consume each edge once
    DWORD PressTimeMs = 0;        // event time (message time; poll: tick of the poll)
    InputEventSource PressSource = InputEventSource::None;
    bool PressRepeat = false;     // WM_KEYDOWN autorepeat (lParam bit 30)
    bool PolledThisFrame = false; // the poll read this key since the last EndFrame
    bool PolledDown = false;      // the poll's last physical reading
    bool PollDownBeforeValid = false;
    DWORD PollDownBeforeMs = 0;   // last poll that saw it down, before this frame's poll
};
KeyObservation GetKeyObservation(int vk);

// One edge of the menu key (SetMenuKey), in arrival order.
struct KeyEdgeRecord
{
    bool Down = false;
    InputEventSource Source = InputEventSource::None;
    bool Repeat = false;
    DWORD EventMs = 0;         // event time
    DWORD SeenMs = 0;          // GetTickCount when OptiInput recorded it
    bool PolledDown = false;   // the poll's last physical reading of the key at that moment
    bool MenuVisible = false;
};
constexpr std::size_t MaxMenuKeyEdges = 32;
// The menu key whose edges are recorded (0 = none); `readTapBit`: the poll also reads its "pressed since the last
// call" bit (a tap between two polls becomes a press and a release), for the press latch only.
void SetMenuKey(int vk, bool readTapBit);
// Edges of the menu key since the last call, oldest first; returns how many were written (at most maxCount) and, in
// `dropped`, how many older ones did not fit.
std::size_t DrainMenuKeyEdges(KeyEdgeRecord* out, std::size_t maxCount, std::size_t* dropped);

// The menu key closed the menu on its press: keep that key from the game (messages, raw input, its keyboard hooks,
// DirectInput, the key state reads) until it is released: its key-up message, its raw key-up, its key-up at the
// game's keyboard hook, or 3 s after the poll last saw it down; a press that closes the menu on release never reached
// the game either. Nothing when the key is already up (a tap): a hold would keep its next press from the game.
void HoldKeyUntilReleased(int vk);

// [Hotfix] DiagInputHooksSkip tokens "presslatch" / "clickreplay": the two 0.3.4.2 menu input changes off (0.3.4.1
// behaviour: the menu key toggles on release; clicks and nav keys only as a per-frame level).
bool MenuPressLatchEnabled();
bool MenuClickReplayEnabled();

// One snapshot of what can keep input from the menu, on a menu open (first opens of a session only): window and
// back-buffer sizes, DPI, clip, foreground, registered raw input devices, tracked hooks, DirectInput devices.
void LogMenuOpenDiagnostics(int backBufferWidth, int backBufferHeight, float imguiDisplayWidth,
                            float imguiDisplayHeight);

} // namespace OptiInput
