// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Tests;

public class HashingTests
{
    [Fact]
    public void Sha256OfFile_returns_uppercase_hex_of_content()
    {
        using var dir = new TempDir();
        // SHA256("abc") is a published test vector.
        var file = dir.Write("a.bin", "abc");

        Assert.Equal(
            "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD",
            Hashing.Sha256OfFile(file));
    }

    [Fact]
    public void Sha256OfFile_handles_paths_containing_apostrophes_and_brackets()
    {
        using var dir = new TempDir();           // TempDir names contain ' and [ ]
        var file = dir.Write("Marvel's [x].bin", "abc");

        Assert.StartsWith("BA7816BF", Hashing.Sha256OfFile(file));
    }
}
