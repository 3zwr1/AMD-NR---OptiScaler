// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Tests;

public class FileFactsTests
{
    [Fact]
    public void Reports_the_length_of_a_file_that_is_there()
    {
        using var dir = new TempDir();

        Assert.Equal(1024, FileFacts.TryLength(dir.Write("big.exe", new byte[1024])));
    }

    [Fact]
    public void Reports_zero_for_a_file_that_vanished()
    {
        // The scan picks the largest .exe in a folder, and Doctor checks the weights file is
        // not empty — both read a length for a path enumerated moments earlier. A game folder
        // mid-update, or an AV scanner quarantining a file, makes that path disappear in
        // between, and FileInfo.Length throws FileNotFoundException for it.
        using var dir = new TempDir();

        Assert.Equal(0, FileFacts.TryLength(Path.Combine(dir.Path, "gone.exe")));
    }
}
