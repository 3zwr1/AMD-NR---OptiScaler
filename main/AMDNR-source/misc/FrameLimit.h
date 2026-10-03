// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#pragma once
#include "SysUtils.h"

class FrameLimit
{
    static uint64_t get_timestamp();
    static int timer_sleep(int64_t hundred_ns);
    static int busywait_sleep(int64_t ns);
    static int combined_sleep(int64_t ns);

  public:
    // outputPerBase: frames shown per game frame while FG runs (interpolated + 1); 2 = the old assumption
    static void sleep(bool fgActive, uint32_t outputPerBase = 2);

    // The tuned wait and clock, exposed so a second pacer does not have to reimplement
    // them. `sleep_ns` is the high-resolution timer with a busy-wait tail and deviation
    // correction; a plain Sleep() here would overshoot by up to a scheduler quantum,
    // which on a 13 ms frame is the whole budget. `now_ns` is the same monotonic clock,
    // so deadlines computed against it line up with the limiter's own.
    static int sleep_ns(int64_t ns);
    static uint64_t now_ns();
};
