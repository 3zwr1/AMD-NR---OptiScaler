// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class EpicScannerTests
{
    private static string WriteItem(TempDir dir, string file, string displayName, string installLocation)
    {
        Directory.CreateDirectory(installLocation);
        return dir.Write($@"Manifests\{file}", $$"""
        {
          "FormatVersion": 0,
          "DisplayName": "{{displayName}}",
          "InstallLocation": "{{installLocation.Replace("\\", "\\\\")}}",
          "LaunchExecutable": "Game\\Binaries\\Win64\\Game.exe"
        }
        """);
    }

    [Fact]
    public void Reads_display_name_and_install_location()
    {
        using var dir = new TempDir();
        var install = Path.Combine(dir.Path, "Games", "Alan Wake 2");
        WriteItem(dir, "a.item", "Alan Wake 2", install);

        var game = new EpicScanner(Path.Combine(dir.Path, "Manifests")).Scan().Single();

        Assert.Equal("Alan Wake 2", game.Name);
        Assert.Equal(install, game.InstallRoot);
        Assert.Equal(GameStore.Epic, game.Store);
    }

    [Fact]
    public void Skips_entries_whose_install_location_is_gone()
    {
        using var dir = new TempDir();
        dir.Write(@"Manifests\gone.item", """
        { "DisplayName": "Gone", "InstallLocation": "Z:\\nope" }
        """);

        Assert.Empty(new EpicScanner(Path.Combine(dir.Path, "Manifests")).Scan());
    }

    [Fact]
    public void A_malformed_item_does_not_abort_the_scan()
    {
        using var dir = new TempDir();
        var install = Path.Combine(dir.Path, "Games", "Good");
        WriteItem(dir, "good.item", "Good", install);
        dir.Write(@"Manifests\bad.item", "{ not json");

        Assert.Equal("Good", new EpicScanner(Path.Combine(dir.Path, "Manifests")).Scan().Single().Name);
    }

    [Fact]
    public void Returns_nothing_when_the_manifests_directory_is_absent()
    {
        Assert.Empty(new EpicScanner(@"Z:\no\such\manifests").Scan());
    }

    [Fact]
    public void An_item_that_cannot_be_read_does_not_abort_the_scan()
    {
        // The malformed-JSON test exercises the parse branch; this one makes a file
        // genuinely unreadable so the I/O guard is actually covered.
        using var dir = new TempDir();
        WriteItem(dir, "good.item", "Good", Path.Combine(dir.Path, "Games", "Good"));
        var lockedItem = WriteItem(dir, "locked.item", "Locked", Path.Combine(dir.Path, "Games", "Locked"));

        using var hold = new FileStream(lockedItem, FileMode.Open, FileAccess.Read, FileShare.None);

        var games = new EpicScanner(Path.Combine(dir.Path, "Manifests")).Scan().ToList();

        Assert.Equal("Good", Assert.Single(games).Name);
    }
}
