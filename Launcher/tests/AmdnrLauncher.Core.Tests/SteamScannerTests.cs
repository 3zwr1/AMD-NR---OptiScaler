// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class SteamScannerTests
{
    /// <summary>Builds a Steam root with two libraries, mirroring a real dual-drive setup.</summary>
    private static (string SteamRoot, string SecondLibrary) MakeSteam(TempDir dir)
    {
        var steamRoot = Path.Combine(dir.Path, "Steam");
        var second = Path.Combine(dir.Path, "D_SteamLibrary");

        dir.Write(@"Steam\steamapps\libraryfolders.vdf", $$"""
        "libraryfolders"
        {
            "0" { "path" "{{steamRoot.Replace("\\", "\\\\")}}" }
            "1" { "path" "{{second.Replace("\\", "\\\\")}}" }
        }
        """);

        dir.Write(@"Steam\steamapps\appmanifest_100.acf", """
        "AppState" { "appid" "100" "name" "Onimusha" "installdir" "Onimusha" }
        """);
        Directory.CreateDirectory(Path.Combine(steamRoot, "steamapps", "common", "Onimusha"));

        dir.Write(@"D_SteamLibrary\steamapps\appmanifest_200.acf", """
        "AppState" { "appid" "200" "name" "SILENT HILL 2" "installdir" "SILENT HILL 2" }
        """);
        Directory.CreateDirectory(Path.Combine(second, "steamapps", "common", "SILENT HILL 2"));

        return (steamRoot, second);
    }

    /// <summary>Adds a game to the first library with its install folder in place, as Steam
    /// creates it before the first byte arrives. StateFlags is written only when given.</summary>
    private static void AddGame(TempDir dir, string steamRoot, string appId, string name, string? stateFlags)
    {
        var flags = stateFlags is null ? "" : $"\"StateFlags\" \"{stateFlags}\"";
        dir.Write($@"Steam\steamapps\appmanifest_{appId}.acf", $$"""
        "AppState" { "appid" "{{appId}}" "name" "{{name}}" {{flags}} "installdir" "{{name}}" }
        """);
        Directory.CreateDirectory(Path.Combine(steamRoot, "steamapps", "common", name));
    }

    [Fact]
    public void A_game_Steam_is_still_downloading_is_not_listed()
    {
        // Resident Evil Requiem as a real library had it: 12 GB of 80 GB down, the install
        // folder already there and empty, and StateFlags 1042, which lacks bit 4 (FullyInstalled).
        // There is no game in that folder to install into yet.
        using var dir = new TempDir();
        var (steamRoot, _) = MakeSteam(dir);
        AddGame(dir, steamRoot, "3764200", "Resident Evil Requiem", "1042");

        var games = new SteamScanner(steamRoot).Scan().ToList();

        Assert.DoesNotContain(games, g => g.Name == "Resident Evil Requiem");
        Assert.Equal(2, games.Count);
    }

    [Theory]
    [InlineData("4")] // FullyInstalled
    [InlineData("6")] // FullyInstalled, with an update pending
    public void An_installed_game_is_listed_with_or_without_an_update_pending(string stateFlags)
    {
        using var dir = new TempDir();
        var (steamRoot, _) = MakeSteam(dir);
        AddGame(dir, steamRoot, "300", "Stray", stateFlags);

        Assert.Contains(new SteamScanner(steamRoot).Scan(), g => g.Name == "Stray");
    }

    [Theory]
    [InlineData(null)]
    [InlineData("")]
    [InlineData("installed")]
    [InlineData("10.42")]
    public void A_game_whose_StateFlags_is_missing_or_unreadable_is_still_listed(string? stateFlags)
    {
        // Only a number Steam wrote can say a game is not installed. A hand-edited or unusual
        // manifest keeps the game listed rather than hiding it on a guess.
        using var dir = new TempDir();
        var (steamRoot, _) = MakeSteam(dir);
        AddGame(dir, steamRoot, "300", "Stray", stateFlags);

        Assert.Contains(new SteamScanner(steamRoot).Scan(), g => g.Name == "Stray");
    }

    [Theory]
    [InlineData("{System32}")]           // rooted: the folder an elevated install must never reach
    [InlineData(@"..\..\..\Elsewhere")]  // a separator, climbing out of the library
    [InlineData("Onimusha/Binaries")]    // a separator, even one that stays inside it
    [InlineData(".")]                    // steamapps\common itself
    [InlineData("..")]                   // steamapps
    [InlineData("...")]                  // Windows trims it to steamapps\common too
    [InlineData("{Drive}")]              // "C:" — a colon, and that drive's current folder
    public void An_installdir_that_is_not_one_folder_name_is_not_listed(string installDir)
    {
        // Steam writes installdir as one folder name under steamapps\common. A library can sit
        // anywhere a user can write, so a hand-edited .acf is the only source of anything else,
        // and listed it would aim an install — elevated, when the launcher runs as admin — at
        // whatever folder it names.
        using var dir = new TempDir();
        var (steamRoot, _) = MakeSteam(dir);
        Directory.CreateDirectory(Path.Combine(dir.Path, "Elsewhere"));
        Directory.CreateDirectory(Path.Combine(steamRoot, "steamapps", "common", "Onimusha", "Binaries"));

        var value = installDir
            .Replace("{System32}", Environment.SystemDirectory)
            .Replace("{Drive}", Path.GetPathRoot(Environment.SystemDirectory)![..2]);

        // It would be listed on the strength of the folder being there.
        Assert.True(Directory.Exists(Path.Combine(steamRoot, "steamapps", "common", value)));

        dir.Write(@"Steam\steamapps\appmanifest_666.acf", $$"""
        "AppState" { "appid" "666" "name" "Hijack" "StateFlags" "4" "installdir" "{{value.Replace("\\", "\\\\")}}" }
        """);

        var games = new SteamScanner(steamRoot).Scan().ToList();

        Assert.DoesNotContain(games, g => g.Name == "Hijack");
        Assert.Equal(2, games.Count);
    }

    [Fact]
    public void Finds_games_in_every_library_folder()
    {
        using var dir = new TempDir();
        var (steamRoot, second) = MakeSteam(dir);

        var games = new SteamScanner(steamRoot).Scan().ToList();

        Assert.Equal(2, games.Count);
        Assert.Contains(games, g => g.Name == "SILENT HILL 2"
            && g.InstallRoot == Path.Combine(second, "steamapps", "common", "SILENT HILL 2"));
        Assert.All(games, g => Assert.Equal(GameStore.Steam, g.Store));
    }

    [Fact]
    public void Skips_manifests_whose_install_directory_is_gone()
    {
        using var dir = new TempDir();
        var (steamRoot, _) = MakeSteam(dir);
        dir.Write(@"Steam\steamapps\appmanifest_300.acf", """
        "AppState" { "appid" "300" "name" "Uninstalled Game" "installdir" "Ghost" }
        """);

        Assert.DoesNotContain(new SteamScanner(steamRoot).Scan(), g => g.Name == "Uninstalled Game");
    }

    [Fact]
    public void Returns_nothing_when_the_steam_root_does_not_exist()
    {
        Assert.Empty(new SteamScanner(@"Z:\no\such\steam").Scan());
    }

    [Fact]
    public void A_corrupt_manifest_does_not_abort_the_whole_scan()
    {
        using var dir = new TempDir();
        var (steamRoot, _) = MakeSteam(dir);
        dir.Write(@"Steam\steamapps\appmanifest_999.acf", "this is not vdf {{{");

        Assert.Equal(2, new SteamScanner(steamRoot).Scan().Count());
    }

    [Fact]
    public void A_manifest_that_cannot_be_read_does_not_abort_the_scan()
    {
        // The corrupt-manifest test above never reaches the I/O guard: the parser returns a
        // node with no AppState rather than throwing. This one makes the file genuinely
        // unreadable, which is the failure real libraries actually produce.
        using var dir = new TempDir();
        var (steamRoot, _) = MakeSteam(dir);

        var locked = Path.Combine(steamRoot, "steamapps", "appmanifest_777.acf");
        File.WriteAllText(locked, """ "AppState" { "name" "Locked" "installdir" "Locked" } """);
        Directory.CreateDirectory(Path.Combine(steamRoot, "steamapps", "common", "Locked"));

        using var hold = new FileStream(locked, FileMode.Open, FileAccess.Read, FileShare.None);

        var games = new SteamScanner(steamRoot).Scan().ToList();

        Assert.Equal(2, games.Count);
        Assert.DoesNotContain(games, g => g.Name == "Locked");
    }
}
