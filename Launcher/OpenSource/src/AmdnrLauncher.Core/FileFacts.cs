// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core;

public static class FileFacts
{
    /// <summary>The file's length, or 0 when it cannot be read. Every caller here reads a
    /// length for a path something else enumerated moments earlier, and a real game folder
    /// changes under a scan — an update starting, an AV scanner quarantining a file. Then
    /// FileInfo.Length throws for a path that no longer resolves, on a code path with no
    /// catch above it. "No bytes" is the answer every caller already treats as unusable.</summary>
    public static long TryLength(string path)
    {
        try { return new FileInfo(path).Length; }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { return 0; }
    }
}
