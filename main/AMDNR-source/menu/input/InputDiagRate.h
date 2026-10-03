// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// AMDNR 0.3.4.2: rate limits of the menu input diagnostics (OptiInput, INFO at the default LogLevel 2), so the next
// Assetto Corsa report says which input source reaches the menu and how late AC's messages are, without flooding the
// log. Header-only and free of Windows and ImGui (tests\034\input_diag_rate_test.cpp). Times are GetTickCount64 ms.

#include <cstdint>

namespace MenuInput
{
// The per-second line while the menu is visible: one line per periodMs for the first burstLines lines of the session,
// then one per tailPeriodMs. Start() at every menu open: the first line of an open comes one period after it.
struct DiagLineLimiter
{
    uint32_t burstLines = 120;
    uint64_t periodMs = 1000;
    uint64_t tailPeriodMs = 30000;

    uint32_t emitted = 0; // lines this session (not reset by Start)
    bool started = false;
    uint64_t windowStartMs = 0;

    void Start(uint64_t nowMs)
    {
        started = true;
        windowStartMs = nowMs;
    }

    bool Due(uint64_t nowMs)
    {
        if (!started)
        {
            Start(nowMs);
            return false;
        }
        const uint64_t period = emitted < burstLines ? periodMs : tailPeriodMs;
        if (nowMs >= windowStartMs && nowMs - windowStartMs < period)
            return false;
        windowStartMs = nowMs;
        ++emitted;
        return true;
    }
};

// A check that runs at most once per interval, by time rather than by frame count (the "menu is visible but no input"
// WARN: every 600 frames was 60-150 s at AC's 4-10 fps).
struct EveryMs
{
    uint64_t lastMs = 0;
    bool any = false;

    bool Due(uint64_t nowMs, uint64_t intervalMs)
    {
        if (any && nowMs >= lastMs && nowMs - lastMs < intervalMs)
            return false;
        any = true;
        lastMs = nowMs;
        return true;
    }
};

// A per-session line budget (menu-key edges, menu-open snapshots): Take() is true for the first `cap` calls;
// CapReached() is true exactly once, on the first call past the cap (for a "no more lines" note).
struct LineBudget
{
    uint32_t cap = 40;
    uint32_t used = 0;
    bool noted = false;

    bool Take()
    {
        if (used >= cap)
            return false;
        ++used;
        return true;
    }
    bool CapReached()
    {
        if (used < cap || noted)
            return false;
        noted = true;
        return true;
    }
};
} // namespace MenuInput
