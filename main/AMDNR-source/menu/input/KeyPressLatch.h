// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// AMDNR 0.3.4.2 (Assetto Corsa, second report): the menu key toggles exactly once per physical press, on the press
// edge (MenuCommon::UpdateManualInput). AC hands its key messages to OptiScaler up to about 2 s late: a late
// WM_KEYDOWN after the poll had seen the key up set it down again, the next poll made a second release edge, and the
// menu closed and reopened by itself (pairs 1.4-2.0 s apart, beyond the 1000 ms cap of MenuToggleDebounceMs). Here a
// release never toggles. A press fires only while the latch is armed (the key was seen up since the last fire), when
// it is not an autorepeat, and when its event time is not older than the frame that armed the latch; a late copy of
// an earlier press is older than that frame and is dropped whatever source delivers it.
// The same file carries the pure rules for that key after the menu closed: how long it stays hidden from the game
// (HoldStillActive) and which DirectInput offset masks it (DirectInputOffsetFromScanCodeEx).
// Header-only and free of Windows and ImGui so the unit tests (tests\034\key_press_latch_test.cpp) build it alone.

#include <cstdint>

namespace MenuInput
{
// Ticks are GetTickCount / GetMessageTime values: milliseconds, 32-bit, wrapping every 49.7 days. true when tick `a`
// is earlier than tick `b` (valid while the two are less than 24.8 days apart).
inline bool TickBefore(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b) < 0; }

enum class LatchVerdict : uint8_t
{
    Fire,     // toggle the menu
    Stale,    // the press is older than the frame that armed the latch (a late message of an earlier press)
    Disarmed, // the key has not been seen up since the last fire (held, or a spurious up while held)
    Repeat,   // an autorepeat key-down (WM_KEYDOWN lParam bit 30)
    Gated,    // shortcuts are off this frame (keybind capture, and 1 s after it): consumed, never fires
};

inline const char* LatchVerdictName(LatchVerdict v)
{
    switch (v)
    {
    case LatchVerdict::Fire:
        return "fire";
    case LatchVerdict::Stale:
        return "stale";
    case LatchVerdict::Disarmed:
        return "disarmed";
    case LatchVerdict::Repeat:
        return "repeat";
    case LatchVerdict::Gated:
        return "gated";
    }
    return "?";
}

// One latch per key (the menu key). Not thread-safe: used from the menu thread only.
struct KeyPressLatch
{
    // A press up to this much older than the arming frame still fires: GetTickCount steps in about 16 ms, so the
    // event time of a real new press and the tick of the frame that saw the key up can differ by one step.
    static constexpr uint32_t kSlackMs = 20;

    bool armed = false;     // false until the key has been seen up: at start, and after every fire
    uint32_t armedAtMs = 0; // tick of the first frame the key was up since the last fire

    // One press edge of the key: its event time and whether it is an autorepeat. `gated`: the caller's shortcuts are
    // off this frame; the press is consumed and the latch stays as it is.
    LatchVerdict OnPress(uint32_t pressTimeMs, bool repeat, bool gated)
    {
        if (repeat)
            return LatchVerdict::Repeat;
        if (!armed)
            return LatchVerdict::Disarmed;
        if (TickBefore(pressTimeMs + kSlackMs, armedAtMs))
            return LatchVerdict::Stale;
        if (gated)
            return LatchVerdict::Gated;
        armed = false;
        return LatchVerdict::Fire;
    }

    // Once per frame, after that frame's presses. `keyUp`: the key is up this frame, physically by the poll when the
    // poll ran, else by the tracked level. The latch arms at the first up frame; armedAtMs is not refreshed while the
    // key stays up, so a late message from before that frame stays stale for as long as it takes to arrive.
    void OnFrame(bool keyUp, uint32_t nowMs)
    {
        if (keyUp && !armed)
        {
            armed = true;
            armedAtMs = nowMs;
        }
    }

    // The menu key changed: wait for the new key to be seen up.
    void Reset()
    {
        armed = false;
        armedAtMs = 0;
    }
};

// AMDNR 0.3.4.2: the menu key that closed the menu is held back from the game until that press is over
// (OptiInput::HoldKeyUntilReleased), so the press the player used to close the menu is not also an action in the
// game. Two cases, two windows:
//  - the key is still down when the menu closes: the hold ends at its key-up (a queued message, a raw break, or the
//    game's own keyboard hook) and, when none of those ever arrives, kHoldReleaseTimeoutMs after the key was last
//    seen down;
//  - the key already reads up: the poll found the whole press between two frames (a tap), and the game's own
//    WM_KEYDOWN / WM_KEYUP of that same physical press can still arrive up to about 2 s later (Assetto Corsa at
//    3-10 fps) with the menu already closed. The hold then waits for that one late pair, at most
//    kHoldLatePairTimeoutMs; the late down switches it to the case above, so its key-up ends it.
// Dropping the hold in the second case would let that late pair reach the game, which is the leak the hold exists
// to stop.
constexpr uint32_t kHoldReleaseTimeoutMs = 3000;
constexpr uint32_t kHoldLatePairTimeoutMs = 2500;

// true while the hold is still in force. `sinceMs`: the tick the held key was last seen down, or, while the hold
// waits for the late pair, the tick the hold started. A tick from the future (an event of this frame stamped by its
// own source) never counts as expired.
inline bool HoldStillActive(bool awaitingLatePair, uint32_t nowMs, uint32_t sinceMs)
{
    if (TickBefore(nowMs, sinceMs))
        return true;

    return nowMs - sinceMs <= (awaitingLatePair ? kHoldLatePairTimeoutMs : kHoldReleaseTimeoutMs);
}

// The DirectInput keyboard offset (DIK_*) of a virtual key, from the scan code MapVirtualKeyW returns for
// MAPVK_VK_TO_VSC_EX: the low byte, with bit 7 set when the scan code is extended (an E0 / E1 prefix in the high
// byte), e.g. DIK_INSERT 0xD2 = E0 52. 0 when the layout has no scan code for the key.
// Windows swaps the Pause / NumLock pair here: VK_NUMLOCK reports an E0-prefixed 0x45, which would read as
// DIK_PAUSE (0xC5), and VK_PAUSE a bare 0x45, which would read as DIK_NUMLOCK (0x45). Those two are taken from the
// pair below instead, so [Menu] ShortcutKey=Pause or NumLock masks the key the player really pressed and not the
// other one.
inline int DirectInputOffsetFromScanCodeEx(int vk, uint32_t scanCodeEx)
{
    constexpr int kVkPause = 0x13;    // VK_PAUSE
    constexpr int kVkNumLock = 0x90;  // VK_NUMLOCK
    constexpr int kDikPause = 0xC5;   // DIK_PAUSE
    constexpr int kDikNumLock = 0x45; // DIK_NUMLOCK

    if (vk == kVkPause)
        return kDikPause;
    if (vk == kVkNumLock)
        return kDikNumLock;

    const uint32_t code = scanCodeEx & 0xFFu;
    if (code == 0)
        return 0;

    return static_cast<int>((scanCodeEx & 0xFF00u) != 0 ? (code | 0x80u) : code);
}
} // namespace MenuInput
