// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.State;

namespace AmdnrLauncher.Core.Tests;

public class InstalledGamesDbTests : IDisposable
{
    private readonly TempDir _dir = new();
    private readonly string _originalPath = InstalledGamesDb.FilePath;

    public InstalledGamesDbTests()
        => InstalledGamesDb.FilePath = Path.Combine(_dir.Path, "games.json");

    public void Dispose()
    {
        InstalledGamesDb.FilePath = _originalPath;
        _dir.Dispose();
    }

    [Fact]
    public void Round_trips_a_manually_added_folder()
    {
        InstalledGamesDb.AddManual(@"D:\Games\Foo");

        Assert.Equal([@"D:\Games\Foo"], InstalledGamesDb.Load());
    }

    [Fact]
    public void A_games_file_that_cannot_be_read_yields_no_games()
    {
        // Load runs at the top of every Scan(). A locked or permission-denied games.json is
        // ordinary on a machine with OneDrive-synced %LOCALAPPDATA%, and losing the manual
        // list for one scan is survivable; taking the launcher down with it is not.
        InstalledGamesDb.AddManual(@"D:\Games\Foo");
        using var hold = new FileStream(
            InstalledGamesDb.FilePath, FileMode.Open, FileAccess.Read, FileShare.None);

        Assert.Empty(InstalledGamesDb.Load());
    }

    [Fact]
    public void Adding_a_folder_that_cannot_be_persisted_does_not_throw()
    {
        // Reached from the Add game folder… button, whose click handler has no catch.
        InstalledGamesDb.FilePath = Path.Combine(_dir.Path, "locked", "games.json");
        Directory.CreateDirectory(Path.GetDirectoryName(InstalledGamesDb.FilePath)!);
        using var hold = new FileStream(
            InstalledGamesDb.FilePath, FileMode.Create, FileAccess.Write, FileShare.None);

        InstalledGamesDb.AddManual(@"D:\Games\Foo");
    }
}
