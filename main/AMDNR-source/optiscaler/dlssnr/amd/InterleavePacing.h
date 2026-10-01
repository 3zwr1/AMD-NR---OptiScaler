// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Interleave pacing.
//
// THE PROBLEM THIS EXISTS FOR, stated precisely, because the obvious reading of it
// is wrong and leads to the wrong fix.
//
// With model interleave at cadence 2 the frames alternate expensive/cheap: the model
// runs on one and the other is filled by reprojection. Measured in Forza that is
// roughly 33 ms and 13 ms. The first instinct is that uneven intervals are themselves
// the judder - they are not. A game that simulates 33 ms of motion and shows it 33 ms
// later is correct; uneven intervals with matching content read as smooth.
//
// What actually breaks is the game's own delta time. Engines step the simulation with
// the PREVIOUS frame's duration, so an oscillating frame time feeds an oscillating dt
// into a frame that will be displayed for the other duration: the 33 ms camera step is
// shown 13 ms later and the 13 ms step is shown 33 ms later. Every frame's motion is
// wrong by the difference, alternating in sign. That is textbook frame-time-oscillation
// judder, and it is why the friend's report was "the image is quite smooth" and "it
// feels really choppy" at the same time. Both halves of that sentence are true, and
// neither is about image quality - which is why nothing in TemporalStability.h could
// ever have fixed it.
//
// WHAT CAN AND CANNOT BE DONE ABOUT IT. A 33 ms frame cannot be made shorter here. The
// only lever is making the cheap frame longer, so the trade is real and continuous:
//
//   target = average work  ->  no pacing possible; the work already takes that long
//   target = model  work   ->  perfectly even, and the frame rate is exactly what you
//                              get with interleave switched off. The entire gain, gone.
//
// So this is a dial, not a switch, and it is off by default. `strength` picks a point
// between those two ends. The useful part is that the win is not geometric: stabilising
// the interval stabilises the engine's dt estimate, which breaks the oscillation the
// judder comes from - and that loop settles well before the fully-even end. Partial
// pacing buys most of the smoothness for a fraction of the frame rate. The menu shows
// the measured numbers live so the trade is visible instead of guessed at.
//
// THREADING. Note() runs on the render thread inside Record; Pace() runs on the present
// thread. Nothing here locks: the present path must never be able to block behind the
// recording path, and a pacer that stalls is worse than no pacer. Everything crossing
// the two is an atomic, and the pacer's own state is touched by Pace() alone.
#include <atomic>
#include <cstdint>
#include <algorithm>
#include <misc/FrameLimit.h>

namespace DlssNr::Pacing
{

struct Snapshot
{
    bool active = false;   // pacing is engaged (enabled, and a real split was measured)
    double modelMs = 0;    // measured work time of a frame the model ran on
    double fillMs = 0;     // measured work time of a filled frame
    double targetMs = 0;   // the interval being paced to
    double evenness = 0;   // 0 = untouched sawtooth, 1 = fully even
};

namespace detail
{
// Set by the render thread, read by the present thread.
inline std::atomic<unsigned> g_noteSeq { 0 };     // bumped once per recorded frame
inline std::atomic<bool> g_noteModel { false };   // was the model run on that frame
inline std::atomic<float> g_strength { 0.f };     // 0 = off, 1 = fully even

// Published for the menu. Written by Pace(), read by the UI; plain atomics rather
// than a lock because a torn readout costs a wrong digit for one frame and a lock
// on this path costs a frame.
inline std::atomic<double> g_showModelMs { 0 };
inline std::atomic<double> g_showFillMs { 0 };
inline std::atomic<double> g_showTargetMs { 0 };
inline std::atomic<bool> g_showActive { false };

// Owned by Pace() only.
struct State
{
    uint64_t lastExitNs = 0;    // when the previous Pace() returned, i.e. the frame's start
    uint64_t lastNoteNs = 0;    // when a fresh note last arrived
    double deadlineNs = 0;      // where this frame's present was supposed to land
    double modelNs = 0;         // EMA of model-frame work
    double fillNs = 0;          // EMA of filled-frame work
    unsigned lastSeq = 0;       // the note this pacer has already consumed
    bool logged = false;        // one-shot "this actually engaged" line
};

// How long without a note before the deadline is abandoned. Time, not a count of
// presents: with frame generation there are two presents per rendered frame (more with
// multi-frame generation), so a count would call a perfectly healthy FG session starved
// and keep resetting the deadline it is supposed to be carrying.
constexpr uint64_t kStarveNs = 500'000'000; // 500 ms
inline State& state()
{
    static State s;
    return s;
}

// An EMA slow enough that one hitched frame does not move the target, and fast enough
// to follow a genuine load change within about a second at 60 fps.
constexpr double kEmaAlpha = 0.03;
inline void ema(double& acc, double sample)
{
    acc = acc == 0.0 ? sample : acc + kEmaAlpha * (sample - acc);
}
} // namespace detail

// Called once per recorded frame from the render thread, with whether the model ran.
inline void Note(bool modelRan)
{
    detail::g_noteModel.store(modelRan, std::memory_order_relaxed);
    detail::g_noteSeq.fetch_add(1, std::memory_order_release);
}

// 0 disables. Called from the frame setup that already reads the config.
inline void SetStrength(float strength)
{
    // -1 = automatic (see Pace), 0 = off, 0..1 = manual.
    detail::g_strength.store(std::clamp(strength, -1.f, 1.f), std::memory_order_relaxed);
}

// For the menu's live readout. Free to call every frame.
inline Snapshot Read()
{
    using namespace detail;
    Snapshot s;
    s.active = g_showActive.load(std::memory_order_relaxed);
    s.modelMs = g_showModelMs.load(std::memory_order_relaxed);
    s.fillMs = g_showFillMs.load(std::memory_order_relaxed);
    s.targetMs = g_showTargetMs.load(std::memory_order_relaxed);
    if (s.modelMs > s.fillMs && s.fillMs > 0.0)
    {
        const double avg = 0.5 * (s.modelMs + s.fillMs);
        s.evenness = std::clamp((s.targetMs - avg) / (s.modelMs - avg), 0.0, 1.0);
    }
    return s;
}

// Called from the present path, immediately after Present returns. Safe to call
// unconditionally: it does nothing unless interleave is running and the user asked
// for pacing, and it never sleeps past a deadline another limiter already met.
inline void Pace()
{
    using namespace detail;
    auto& s = state();

    const unsigned seq = g_noteSeq.load(std::memory_order_acquire);
    float strength = g_strength.load(std::memory_order_relaxed);

    // No fresh note means this present does not correspond to a new rendered frame. That
    // is the normal case for every generated frame under FG, and it is also what a paused
    // game or a loading screen looks like - so the two are told apart by how long it has
    // been, not by how many presents have gone by. Below the threshold we simply do
    // nothing and let the generated frame through untouched.
    if (seq == s.lastSeq)
    {
        const uint64_t idle = FrameLimit::now_ns();
        if (s.lastNoteNs != 0 && idle - s.lastNoteNs > kStarveNs)
        {
            s.deadlineNs = 0;
            s.lastExitNs = 0;
            s.lastNoteNs = 0;
            g_showActive.store(false, std::memory_order_relaxed);
        }
        return;
    }
    const bool wasModel = g_noteModel.load(std::memory_order_relaxed);
    s.lastSeq = seq;

    const uint64_t now = FrameLimit::now_ns();
    s.lastNoteNs = now;
    if (s.lastExitNs == 0)
    {
        s.lastExitNs = now;
        s.deadlineNs = 0;
        return;
    }

    // MEASURE WORK, NOT THE PACED INTERVAL. `now` is entry, before any sleep this call
    // makes, and lastExitNs is the previous call's exit - so this span is what the frame
    // actually cost, with our own pacing excluded. Measuring after the sleep would make
    // the pacer read back its own target and lock onto it, and the trade the user set
    // would quietly become "fully even" after a few seconds.
    const double workNs = double(now - s.lastExitNs);
    // A loading screen or an alt-tab is not a sample. 500 ms is far outside anything a
    // frame can legitimately cost and one of them would poison the EMA for a minute.
    if (workNs > 0.0 && workNs < 500'000'000.0)
        ema(wasModel ? s.modelNs : s.fillNs, workNs);

    g_showModelMs.store(s.modelNs / 1e6, std::memory_order_relaxed);
    g_showFillMs.store(s.fillNs / 1e6, std::memory_order_relaxed);

    // Engage only on a real split. Below 15% the two frame types cost the same, there is
    // no sawtooth to remove, and pacing would only be a frame limiter set to the average -
    // which costs frame rate and fixes nothing.
    const bool split = s.modelNs > 0.0 && s.fillNs > 0.0 && s.modelNs > s.fillNs * 1.15;
    if (strength < 0.f)
    {
        // AUTOMATIC, by the user's request: no slider. Engage in proportion to the split
        // the measurement shows. Below 15% the two frame types cost the same and nothing
        // is padded; from 30% up the filled frame is padded all the way to the model
        // frame's cost, because that is the only strength that removes the alternation
        // rather than shrinking it - and a 30% alternation is already visible judder.
        const double ratio = s.fillNs > 0.0 ? s.modelNs / s.fillNs : 1.0;
        strength = static_cast<float>(std::clamp((ratio - 1.15) / 0.15, 0.0, 1.0));
    }
    if (strength <= 0.f || !split)
    {
        s.deadlineNs = 0;
        s.lastExitNs = FrameLimit::now_ns();
        g_showTargetMs.store(0.0, std::memory_order_relaxed);
        g_showActive.store(false, std::memory_order_relaxed);
        return;
    }

    // The two ends of the trade described at the top of this file.
    const double avgNs = 0.5 * (s.modelNs + s.fillNs);
    const double targetNs = avgNs + double(strength) * (s.modelNs - avgNs);
    g_showTargetMs.store(targetNs / 1e6, std::memory_order_relaxed);
    g_showActive.store(true, std::memory_order_relaxed);

    if (s.deadlineNs == 0.0)
        s.deadlineNs = double(now);
    s.deadlineNs += targetNs;

    // Proof of life, once. "I could not feel it doing anything" is not distinguishable
    // from "it never ran" without this, and the first time that question came up the
    // answer was that it had never run - so the log line earns its place.
    if (!s.logged)
    {
        s.logged = true;
        LOG_INFO("Interleave pacing engaged: model {:.1f} ms, fill {:.1f} ms, target {:.1f} ms "
                 "({:.0f} fps) at strength {:.2f}",
                 s.modelNs / 1e6, s.fillNs / 1e6, targetNs / 1e6,
                 targetNs > 0.0 ? 1e9 / targetNs : 0.0, strength);
    }

    if (double(now) < s.deadlineNs)
    {
        FrameLimit::sleep_ns(int64_t(s.deadlineNs - double(now)));
    }
    else if (double(now) - s.deadlineNs > targetNs)
    {
        // More than a whole interval behind: a hitch, a load, or another limiter holding
        // the frame longer than we ever would. Catching that up would mean presenting a
        // burst of frames as fast as possible, which is a worse artefact than the one
        // this is here to fix. Start again from where we actually are.
        s.deadlineNs = double(now);
    }

    s.lastExitNs = FrameLimit::now_ns();
}

} // namespace DlssNr::Pacing
