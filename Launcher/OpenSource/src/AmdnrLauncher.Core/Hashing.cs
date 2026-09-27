// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Security.Cryptography;

namespace AmdnrLauncher.Core;

public static class Hashing
{
    public static string Sha256OfFile(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        return Convert.ToHexString(SHA256.HashData(stream));
    }

    /// <summary>Case-insensitive comparison; manifest hashes may be written in either case.</summary>
    public static bool Matches(string hash, string expected)
        => string.Equals(hash, expected, StringComparison.OrdinalIgnoreCase);

    /// <summary>Null when the file cannot be read. Anything hashing a file inside someone's
    /// game folder has to survive a locked or permission-denied file — the game may be
    /// running — rather than throwing at the caller.</summary>
    public static string? TrySha256OfFile(string path)
    {
        try { return Sha256OfFile(path); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { return null; }
    }
}
