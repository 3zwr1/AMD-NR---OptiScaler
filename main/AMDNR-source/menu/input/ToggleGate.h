// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Re-toggle guard for the menu and NR toggle keys ([Hotfix] MenuToggleDebounceMs, MenuCommon::HandleMenuShortcuts).
// One key press can arrive as two release edges 16-316 ms apart (Assetto Corsa), which toggled twice. Header-only
// and free of Windows and ImGui so the unit tests (tests\034) build it without the DLL.
//
// The signatures are fixed (W0-2; a change is a gate request). Accept() is W1-H's body, ToggleDebounceMsFromIni()
// is G0's.

#include <cstdint>

namespace MenuInput
{
// Upper bound of [Hotfix] MenuToggleDebounceMs. Final (not part of the stub): a hand-edited 60000 would allow one
// toggle per minute, and a negative value cast to uint32_t would never toggle again.
inline constexpr int kMaxToggleDebounceMs = 1000;

// The ini value as the debounce Accept() takes: clamped to 0..kMaxToggleDebounceMs before the cast. Final (not part
// of the stub); the caller (W1-H) passes Config's MenuToggleDebounceMs through it.
inline uint32_t ToggleDebounceMsFromIni(int iniValue)
{
    return static_cast<uint32_t>(iniValue < 0 ? 0 : (iniValue > kMaxToggleDebounceMs ? kMaxToggleDebounceMs : iniValue));
}

// One gate per toggle key (menu, NR). Not thread-safe: used from the menu thread only.
struct ToggleGate
{
    uint64_t lastAcceptedMs = 0; // time of the last accepted edge (GetTickCount64 units)
    bool anyAccepted = false;    // false until the first accepted edge

    // A toggle edge at `nowMs`: true = act on it. An edge less than `debounceMs` after the last accepted one is
    // rejected; `debounceMs` 0 accepts every edge.
    // A rejected edge does not move the window: it is measured from the last ACCEPTED edge, so a held or mashed key
    // still toggles again once `debounceMs` has passed. A clock that went backwards (nowMs before the last accepted
    // edge) accepts, so a bad time can never lock the key out.
    bool Accept(uint64_t nowMs, uint32_t debounceMs)
    {
        if (debounceMs != 0 && anyAccepted && nowMs >= lastAcceptedMs && nowMs - lastAcceptedMs < debounceMs)
            return false;
        lastAcceptedMs = nowMs;
        anyAccepted = true;
        return true;
    }
};
} // namespace MenuInput
