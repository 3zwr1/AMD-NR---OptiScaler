// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using Xunit;

// Several test classes redirect the same process-wide statics — InstallRecordStore.RootDirectory
// and, later, InstalledGamesDb.FilePath — at their own temp directory. xUnit parallelises across
// test classes by default, so those redirects race and the suite fails intermittently.
// The whole suite runs in well under a second: determinism is worth more than parallelism here.
[assembly: CollectionBehavior(DisableTestParallelization = true)]
