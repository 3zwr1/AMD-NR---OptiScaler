// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class FilePlacerTests
{
    [Fact]
    public void Copies_content_and_reports_Copy_mode()
    {
        using var dir = new TempDir();
        var source = dir.Write("src.dll", "payload");
        var destination = Path.Combine(dir.Path, "game", "dxgi.dll");

        var mode = new CopyFilePlacer().Place(source, destination, default);

        Assert.Equal(PlacementMode.Copy, mode);
        Assert.Equal("payload", File.ReadAllText(destination));
    }

    [Fact]
    public void Overwrites_a_destination_the_caller_has_already_cleared_it_to_overwrite()
    {
        // A placer knows nothing about which files are ours, so it cannot be the thing that
        // decides whether a destination may be replaced — a Repair has to overwrite its own
        // files, and only the Installer knows which those are. It asks that question first and
        // moves anything else aside; by the time a destination reaches here it is ours to
        // write. This test fixes the placer's half of that contract, not the end-to-end
        // behaviour: Installer's own tests cover a file it did not place.
        using var dir = new TempDir();
        var source = dir.Write("src.dll", "new");
        var destination = dir.Write("dxgi.dll", "old");

        new CopyFilePlacer().Place(source, destination, default);

        Assert.Equal("new", File.ReadAllText(destination));
    }

    [Fact]
    public void A_cancelled_copy_leaves_no_file_and_no_temp_file_behind()
    {
        using var dir = new TempDir();
        var source = dir.Write("src.bin", new byte[4 * 1024 * 1024]);
        var destination = Path.Combine(dir.Path, "game", "big.bin");
        using var cts = new CancellationTokenSource();
        cts.Cancel();

        Assert.Throws<OperationCanceledException>(
            () => new CopyFilePlacer().Place(source, destination, cts.Token));

        Assert.False(File.Exists(destination));
        Assert.Empty(Directory.EnumerateFiles(Path.Combine(dir.Path, "game")));
    }

    [Fact]
    public void Creates_missing_destination_directories()
    {
        using var dir = new TempDir();
        var source = dir.Write("src.dll", "x");
        var destination = Path.Combine(dir.Path, "a", "b", "c", "libxess.dll");

        new CopyFilePlacer().Place(source, destination, default);

        Assert.True(File.Exists(destination));
    }
}
