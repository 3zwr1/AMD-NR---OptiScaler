// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using Xunit;

// ScheduleTheOneSwap latches a process-wide flag on purpose — there is only ever one update in
// flight — so the test that exercises it cannot share a process with a parallel run of itself.
[assembly: CollectionBehavior(DisableTestParallelization = true)]
