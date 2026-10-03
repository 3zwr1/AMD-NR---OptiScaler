// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// AMDNR 0.3.4.2 (Assetto Corsa at 3-10 fps): the menu's mouse buttons and nav keys reached ImGui only as a level
// sampled once per frame (OptiInput::FeedImGui), so a click shorter than one frame was lost. FeedImGui still sends the
// level, and also replays an edge pair the level cannot show: a press and its release inside one frame (down, up) or
// a release and a new press inside one frame (up, down). ImGui's trickle queue spreads the pair over two frames, so it
// is exactly one click. Only fresh presses are replayed: AC hands its messages over up to about 2 s late, and a late
// copy of a click the poll already delivered must not click a second time.
// The poll also reads GetAsyncKeyState's "pressed since the last call" bit: a tap that went down and up between two
// polls, which neither the level nor a late message would deliver in time.
// Header-only and free of Windows and ImGui (tests\034\edge_replay_test.cpp).

#include "KeyPressLatch.h" // TickBefore

#include <cstdint>

namespace MenuInput
{
enum class ReplayKind : uint8_t
{
    None,    // the level alone
    Click,   // down then up: a press and release inside one frame
    Reclick, // up then down: a release and a new press inside one frame
};

// One mouse button or key, as FeedImGui sees it this frame.
struct EdgeSample
{
    bool down = false;              // level now
    bool pressed = false;           // a down edge since the last frame
    bool released = false;          // an up edge since the last frame
    bool pressRepeat = false;       // that down edge is a key autorepeat
    bool pressByPoll = false;       // that down edge came from the poll (level or pressed-since-last-call bit)
    uint32_t pressCount = 0;        // down edges so far (never reset): one decision per edge
    uint32_t pressTimeMs = 0;       // event time of that down edge (message time; poll: tick of the poll)
    bool pollDownBeforeValid = false;
    uint32_t pollDownBeforeMs = 0;  // last poll that saw it down, before this frame's poll
};

struct ReplayDecision
{
    ReplayKind kind = ReplayKind::None;
    bool stale = false;  // an edge pair was there but its press was not fresh (dropped, counted in the diagnostics)
    uint32_t ageMs = 0;  // age of that press when judged
};

// Oldest press that is still replayed: max(500 ms, 3 frames).
inline uint32_t ReplayMaxAgeMs(float frameMs)
{
    const float three = frameMs > 0.0f ? frameMs * 3.0f : 0.0f;
    return three > 500.0f ? (three > 60000.0f ? 60000u : static_cast<uint32_t>(three)) : 500u;
}

// A press is fresh when it is at most maxAgeMs old and happened after the last poll that saw the key or button down
// before this frame: a late copy of a press the poll already saw carries an older event time than that poll.
inline bool PressIsFresh(uint32_t pressTimeMs, bool pollDownBeforeValid, uint32_t pollDownBeforeMs, uint32_t nowMs,
                         uint32_t maxAgeMs, uint32_t* ageMs = nullptr)
{
    const uint32_t age = TickBefore(nowMs, pressTimeMs) ? 0u : nowMs - pressTimeMs;
    if (ageMs != nullptr)
        *ageMs = age;
    if (age > maxAgeMs)
        return false;
    if (pollDownBeforeValid && !TickBefore(pollDownBeforeMs, pressTimeMs))
        return false;
    return true;
}

// `lastFedDown`: the level ImGui was given last frame. `lastSeenPressCount`: the press count judged last time (an edge
// is judged once even if FeedImGui runs twice before the flags are cleared).
inline ReplayDecision DecideReplay(const EdgeSample& s, bool lastFedDown, uint32_t lastSeenPressCount, uint32_t nowMs,
                                   uint32_t maxAgeMs)
{
    ReplayDecision d;
    if (!s.pressed || s.pressRepeat || s.pressCount == lastSeenPressCount)
        return d;

    ReplayKind kind = ReplayKind::None;
    if (!s.down && !lastFedDown)
        kind = ReplayKind::Click;
    // Not for a press by the poll: that is the level coming back after an up some source reported while the button
    // was held, not a second press.
    else if (s.down && s.released && lastFedDown && !s.pressByPoll)
        kind = ReplayKind::Reclick;
    else
        return d;

    if (!PressIsFresh(s.pressTimeMs, s.pollDownBeforeValid, s.pollDownBeforeMs, nowMs, maxAgeMs, &d.ageMs))
    {
        d.stale = true;
        return d;
    }
    d.kind = kind;
    return d;
}

// GetAsyncKeyState's bit 0 ("pressed since the last call") is believed only when the previous frame polled too (after
// focus returns it can be minutes old) and, for the menu's own keys and buttons, only when that poll already had the
// menu open (a tap made in the game must not click the menu that has just opened). The menu key itself only needs
// the previous poll. Other callers can take the bit first; that only means fewer detections, never false ones.
struct TapBitGate
{
    bool polledLastFrame = false;
    bool lastPollMenuVisible = false;

    // At the start of a poll: `menuSet` = the menu's nav keys and mouse buttons (else the menu key).
    bool Primed(bool menuSet, bool menuVisibleNow) const
    {
        return polledLastFrame && (!menuSet || (menuVisibleNow && lastPollMenuVisible));
    }
    void AfterPoll(bool menuVisibleNow)
    {
        polledLastFrame = true;
        lastPollMenuVisible = menuVisibleNow;
    }
    void NoPoll() { polledLastFrame = false; }
};

// A tap the poll missed: the bit is set, the key is up now and was up at the last poll, and no source reported a press
// of it since the previous poll of this key (pressCount now against pressCountAtLastPoll, the count that previous poll
// ended with). "Since the previous poll", not "since EndFrame": with the poll outside RenderMenu (OverlayMenu=false
// with frame generation) a whole click can be pumped and replayed between two polls, and EndFrame clears the Pressed
// flag before the next poll reads the bit; that press was delivered and must not become a second click.
inline bool IsMissedTap(bool primed, bool bitSet, bool downNow, bool polledDownBefore, bool levelDown,
                        uint32_t pressCount, uint32_t pressCountAtLastPoll)
{
    return primed && bitSet && !downNow && !polledDownBefore && !levelDown && pressCount == pressCountAtLastPoll;
}
} // namespace MenuInput
