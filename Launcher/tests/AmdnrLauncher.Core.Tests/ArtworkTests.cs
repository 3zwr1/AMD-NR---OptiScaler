// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Net;
using System.Text;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

/// <summary>A cover for every game: Steam's cache by name, then by size; the CDN; the Xbox
/// app's own pictures; the exe's icon. Every folder here is a temporary one and nothing here
/// reaches the network — the fetcher is a stub that writes hand-built pictures.</summary>
public class ArtworkTests
{
    /// <summary>Forza Horizon 6's config as the Xbox app wrote it on this PC: the attributes it
    /// uses, in its casing, naming files beside the config.</summary>
    private const string ForzaVisuals = """
        <?xml version="1.0" encoding="utf-8"?>
        <Game configVersion="1">
          <Identity Name="Microsoft.ForteBaseGame" Publisher="CN=Microsoft Corporation" Version="3.440.853.0" />
          <ExecutableList>
            <Executable Name="forzahorizon6.exe" Id="Forzahorizon6" TargetDeviceFamily="PC" />
          </ExecutableList>
          <ShellVisuals DefaultDisplayName="Forza Horizon 6" PublisherDisplayName="Microsoft Studios"
                        StoreLogo="Storelogo.png" Square150x150Logo="Logo.png" Square44x44Logo="SmallLogo.png"
                        SplashScreenImage="SplashScreen.png" />
        </Game>
        """;

    private static string XboxContent(TempDir dir, string config, params string[] pictures)
    {
        var content = Path.Combine(dir.Path, "Forza Horizon 6", "Content");
        Directory.CreateDirectory(content);
        File.WriteAllText(Path.Combine(content, "MicrosoftGame.Config"), config, new UTF8Encoding(true));
        foreach (var picture in pictures)
        {
            var path = Path.Combine(content, picture);
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllBytes(path, Pictures.Png(150, 150, 2));
        }

        return content;
    }

    [Fact]
    public void Xbox_visuals_give_a_square_tile_and_the_splash_screen_as_hero()
    {
        using var dir = new TempDir();
        var content = XboxContent(dir, ForzaVisuals, "Storelogo.png", "Logo.png", "SmallLogo.png", "SplashScreen.png");

        var art = XboxArtwork.Read(content);

        Assert.Equal(Path.Combine(content, "Logo.png"), art.Tile);
        Assert.True(art.TileIsSquare);
        Assert.Equal(Path.Combine(content, "SplashScreen.png"), art.Hero);
        Assert.Null(art.Logo);
        Assert.Null(art.Blur);
    }

    [Fact]
    public void The_largest_square_logo_the_config_names_is_the_tile_and_the_store_logo_the_last_resort()
    {
        using var dir = new TempDir();
        var content = XboxContent(dir, """
            <Game><ShellVisuals Square480x480Logo="Images\Big.png" Square150x150Logo="Logo.png" StoreLogo="Store.png" /></Game>
            """, @"Images\Big.png", "Logo.png", "Store.png");
        Assert.Equal(Path.Combine(content, "Images", "Big.png"), XboxArtwork.Read(content).Tile);

        using var storeOnly = new TempDir();
        var store = XboxContent(storeOnly, """
            <Game><ShellVisuals Square150x150Logo="Missing.png" StoreLogo="Store.png" /></Game>
            """, "Store.png");
        var art = XboxArtwork.Read(store);
        Assert.Equal(Path.Combine(store, "Store.png"), art.Tile);
        Assert.True(art.TileIsSquare);
        Assert.Null(art.Hero);
    }

    [Fact]
    public void A_visual_that_names_a_file_outside_the_game_folder_is_not_shown()
    {
        // The config sits in a folder any local user can write to, and the launcher — often
        // elevated — would otherwise probe, and WPF would load, whatever file it names.
        using var dir = new TempDir();
        using var elsewhere = new TempDir();
        var outside = elsewhere.Write("outside.png", Pictures.Png(150, 150));
        var content = XboxContent(dir, $"""
            <Game><ShellVisuals Square150x150Logo="{outside.Replace("\\", "\\\\")}" SplashScreenImage="..\..\..\{Path.GetFileName(elsewhere.Path)}\outside.png" /></Game>
            """);

        var art = XboxArtwork.Read(content);

        Assert.Null(art.Tile);
        Assert.Null(art.Hero);
        Assert.Equal(GameArt.None, art);
    }

    [Fact]
    public void A_folder_without_a_config_or_with_one_that_does_not_parse_has_no_Xbox_art_and_does_not_throw()
    {
        using var dir = new TempDir();
        Assert.Equal(GameArt.None, XboxArtwork.Read(dir.Path));
        Assert.Equal(GameArt.None, XboxArtwork.Read(Path.Combine(dir.Path, "no such folder")));

        var broken = XboxContent(dir, "<Game><ShellVisuals Square150x150Logo=\"Logo.png\"", "Logo.png");
        Assert.Equal(GameArt.None, XboxArtwork.Read(broken));

        using var dtd = new TempDir();
        var entity = XboxContent(dtd, """
            <?xml version="1.0"?>
            <!DOCTYPE Game [ <!ENTITY logo "Logo.png"> ]>
            <Game><ShellVisuals Square150x150Logo="&logo;" /></Game>
            """, "Logo.png");
        Assert.Equal(GameArt.None, XboxArtwork.Read(entity));

        using var nothingNamed = new TempDir();
        Assert.Equal(GameArt.None, XboxArtwork.Read(XboxContent(nothingNamed, "<Game><ShellVisuals DefaultDisplayName=\"x\" /></Game>")));
    }

    private static GameCandidate Steam(string? appId, string installRoot = @"D:\Games\Game")
        => new("Game", GameStore.Steam, installRoot, appId);

    private static string LibraryCache(TempDir steam, string appId)
        => Path.Combine(steam.Path, "appcache", "librarycache", appId);

    /// <summary>Answers each file name with what it was told to, remembers every address it was
    /// asked for, and by default knows nothing — a 404 for everything.</summary>
    private sealed class StubFetcher : IArtFetcher
    {
        public List<string> Asked { get; } = [];
        public Dictionary<string, Func<string, ArtFetch>> Answers { get; } = new(StringComparer.OrdinalIgnoreCase);

        public Task<ArtFetch> FetchAsync(Uri url, string destinationPath, CancellationToken ct)
        {
            Asked.Add(url.AbsoluteUri);
            var name = Path.GetFileName(url.LocalPath);
            return Task.FromResult(Answers.TryGetValue(name, out var answer) ? answer(destinationPath) : ArtFetch.Missing);
        }
    }

    private static Func<string, ArtFetch> Saves(byte[] bytes) => destination =>
    {
        Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
        File.WriteAllBytes(destination, bytes);
        return ArtFetch.Saved;
    };

    private static Func<string, ArtFetch> Fails => _ => ArtFetch.Failed;

    private const string Cdn = "https://cdn.cloudflare.steamstatic.com/steam/apps/";

    [Fact]
    public void CDN_addresses_and_cache_paths_are_built_from_the_appid()
    {
        Assert.Equal(Cdn + "2124490/library_600x900.jpg", Artwork.CdnUrl("2124490", "library_600x900.jpg").AbsoluteUri);
        Assert.Equal(Cdn + "2124490/library_hero.jpg", Artwork.CdnUrl("2124490", "library_hero.jpg").AbsoluteUri);
        Assert.Equal(Cdn + "2124490/logo.png", Artwork.CdnUrl("2124490", "logo.png").AbsoluteUri);
        Assert.Equal(Cdn + "2124490/header.jpg", Artwork.CdnUrl("2124490", "header.jpg").AbsoluteUri);
        Assert.Equal(["library_600x900.jpg", "library_hero.jpg", "logo.png", "header.jpg"], Artwork.CdnFiles);

        using var root = new TempDir();
        var artwork = new Artwork(root.Path, steamRoot: null);
        Assert.Equal(Path.Combine(root.Path, "steam", "2124490", "logo.png"), artwork.CdnCachePath("2124490", "logo.png"));
        Assert.Equal(Path.Combine(root.Path, "steam", "2124490", "logo.png.missing"), artwork.MissingMarkerPath("2124490", "logo.png"));

        Assert.Equal(
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDNR", "art"),
            Artwork.DefaultCacheRoot);
        Assert.Equal(TimeSpan.FromDays(7), Artwork.RememberMissingFor);
    }

    [Fact]
    public void Steams_named_files_are_used_first_in_either_layout()
    {
        using var steam = new TempDir();
        using var root = new TempDir();
        var perApp = LibraryCache(steam, "2124490");
        var tile = steam.Write(Path.Combine(perApp, "library_600x900.jpg"), "art");
        var hero = steam.Write(Path.Combine(perApp, "library_hero.jpg"), "art");
        var logo = steam.Write(Path.Combine(perApp, "logo.png"), "art");
        var blur = steam.Write(Path.Combine(perApp, "library_hero_blur.jpg"), "art");
        var flatHero = steam.Write(Path.Combine("appcache", "librarycache", "1030840_library_hero.jpg"), "art");

        var artwork = new Artwork(root.Path, steam.Path);

        Assert.Equal(new GameArt(tile, TileIsSquare: false, hero, logo, blur), artwork.Resolve(Steam("2124490"), null, null));
        Assert.Equal(new GameArt(null, false, flatHero, null, null), artwork.Resolve(Steam("1030840"), null, null));
        Assert.False(artwork.Lookup(Steam("1030840"), null, null).NeedsFetch);   // no fetcher: nothing is ever asked
    }

    [Fact]
    public async Task The_CDN_is_asked_only_for_the_kinds_Steams_cache_did_not_yield()
    {
        using var steam = new TempDir();
        using var root = new TempDir();
        var fetcher = new StubFetcher();
        fetcher.Answers["library_600x900.jpg"] = Saves(Pictures.Jpeg(600, 900));
        fetcher.Answers["library_hero.jpg"] = Saves(Pictures.Jpeg(1920, 620));
        fetcher.Answers["logo.png"] = Saves(Pictures.Png(640, 360, 6));
        fetcher.Answers["header.jpg"] = Saves(Pictures.Jpeg(460, 215));
        var artwork = new Artwork(root.Path, steam.Path, fetcher);

        // Everything Steam's cache has, by name: nothing to ask for, and nothing asked — not a
        // second copy of the owner's whole library into the profile on a first start.
        var whole = LibraryCache(steam, "2124490");
        steam.Write(Path.Combine(whole, "library_600x900.jpg"), Pictures.Jpeg(600, 900));
        steam.Write(Path.Combine(whole, "library_hero.jpg"), Pictures.Jpeg(1920, 620));
        steam.Write(Path.Combine(whole, "logo.png"), Pictures.Png(640, 360, 6));
        var complete = artwork.Lookup(Steam("2124490"), null, null);
        Assert.False(complete.NeedsFetch);
        Assert.Empty(complete.WantedFromCdn);
        Assert.False(await artwork.FetchAsync(Steam("2124490"), complete, CancellationToken.None));
        Assert.Empty(fetcher.Asked);
        Assert.False(Directory.Exists(Path.Combine(root.Path, "steam")));

        // The capsule and the hero under hash names and no logo: the logo alone is asked for,
        // and what Steam had stays what is shown.
        var partial = LibraryCache(steam, "1332010");
        var capsule = steam.Write(Path.Combine(partial, "9c1bae81b2603c25ccf2a8f4944b3b58227005d7", "library_capsule.jpg"), Pictures.Jpeg(300, 450));
        steam.Write(Path.Combine(partial, "7ae44e45f5a81726a84c71a69cf8ab0fc5102c84.jpg"), Pictures.Jpeg(1920, 620));
        var lookup = artwork.Lookup(Steam("1332010"), null, null);
        Assert.Equal(["logo.png"], lookup.WantedFromCdn);
        Assert.True(await artwork.FetchAsync(Steam("1332010"), lookup, CancellationToken.None));
        Assert.Equal([Cdn + "1332010/logo.png"], fetcher.Asked);
        var after = artwork.Resolve(Steam("1332010"), null, null);
        Assert.Equal(capsule, after.Tile);
        Assert.Equal(artwork.CdnCachePath("1332010", "logo.png"), after.Logo);

        // A header alone stands in for the tile and the hero, and both are still wanted — the
        // cover and the key art are better than the capsule cropped. The header itself is not.
        var headerOnly = LibraryCache(steam, "1030840");
        var header = steam.Write(Path.Combine(headerOnly, "header.jpg"), Pictures.Jpeg(460, 215));
        var standing = artwork.Lookup(Steam("1030840"), null, null);
        Assert.Equal(new GameArt(header, TileIsSquare: false, header, null, null), standing.Art);
        Assert.Equal(["library_600x900.jpg", "library_hero.jpg", "logo.png"], standing.WantedFromCdn);

        // What the CDN sent counts as found on the next start, as Steam's own files do.
        fetcher.Asked.Clear();
        Assert.False(new Artwork(root.Path, steam.Path, fetcher).Lookup(Steam("1332010"), null, null).NeedsFetch);
        Assert.Empty(fetcher.Asked);
    }

    [Fact]
    public void Hash_named_files_are_told_apart_by_size_and_the_header_stands_in_for_what_is_missing()
    {
        // Stray's folder on this PC: a 32×32 icon under a hash name at the top, then named files
        // inside hash-named folders — the capsule at 300×450, the header at 460×215, the blur.
        using var steam = new TempDir();
        using var root = new TempDir();
        var perApp = LibraryCache(steam, "1332010");
        steam.Write(Path.Combine(perApp, "7794cc0b4e70bd3e4b9a0d07e2570ab8668c5f0b.jpg"), Pictures.Jpeg(32, 32));
        var capsule = steam.Write(Path.Combine(perApp, "9c1bae81b2603c25ccf2a8f4944b3b58227005d7", "library_capsule.jpg"), Pictures.Jpeg(300, 450));
        var header = steam.Write(Path.Combine(perApp, "c93bdad6d6245920edb85e0d3ff81bdfa237b993", "library_header.jpg"), Pictures.Jpeg(460, 215));
        var blur = steam.Write(Path.Combine(perApp, "7ae44e45f5a81726a84c71a69cf8ab0fc5102c84", "library_hero_blur.jpg"), Pictures.Jpeg(192, 62));
        var logo = steam.Write(Path.Combine(perApp, "logo.png"), Pictures.Png(640, 360, 6));
        steam.Write(Path.Combine(perApp, "notes.txt"), "not a picture");

        var artwork = new Artwork(root.Path, steam.Path);
        var art = artwork.Resolve(Steam("1332010"), null, null);

        Assert.Equal(capsule, art.Tile);
        Assert.False(art.TileIsSquare);
        Assert.Equal(header, art.Hero);        // no hero: the header stands in, as it always has
        Assert.Equal(logo, art.Logo);
        Assert.Equal(blur, art.Blur);

        // A hero under a hash name beats the header, and the larger of two tiles wins.
        var realHero = steam.Write(Path.Combine(perApp, "7ae44e45f5a81726a84c71a69cf8ab0fc5102c84", "library_hero.jpg"), Pictures.Jpeg(1920, 620));
        var bigger = steam.Write(Path.Combine(perApp, "0000000000000000000000000000000000000000.jpg"), Pictures.Jpeg(600, 900));
        var again = artwork.Resolve(Steam("1332010"), null, null);
        Assert.Equal(realHero, again.Hero);
        Assert.Equal(bigger, again.Tile);

        // The flat layout's hash-named files are read too.
        using var flat = new TempDir();
        var flatTile = flat.Write(Path.Combine("appcache", "librarycache", "300_abcdef0123456789abcdef0123456789abcdef01.jpg"), Pictures.Jpeg(600, 900));
        Assert.Equal(flatTile, new Artwork(root.Path, flat.Path).Resolve(Steam("300"), null, null).Tile);
    }

    [Fact]
    public async Task The_CDN_is_asked_once_for_what_the_cache_lacks_and_what_lands_outranks_the_icon()
    {
        if (ExeIconTests.AnExeWithAnIcon() is not { } withIcon) return;

        using var game = new TempDir();
        using var root = new TempDir();
        var exe = Path.Combine(game.Path, "Game.exe");
        File.Copy(withIcon, exe);

        var fetcher = new StubFetcher();
        fetcher.Answers["library_600x900.jpg"] = Saves(Pictures.Jpeg(600, 900));
        fetcher.Answers["library_hero.jpg"] = Saves(Pictures.Jpeg(1920, 620));
        var artwork = new Artwork(root.Path, steamRoot: null, fetcher);
        var candidate = Steam("2124490", game.Path);

        // Before the CDN has answered: the exe's icon, square, and nothing else — and an icon
        // standing in for the tile leaves the tile wanted, as everything else is.
        var before = artwork.Lookup(candidate, game.Path, "Game.exe");
        Assert.NotNull(before.Art.Tile);
        Assert.True(before.Art.TileIsSquare);
        Assert.StartsWith(Path.Combine(root.Path, "exe") + Path.DirectorySeparatorChar, before.Art.Tile);
        Assert.Null(before.Art.Hero);
        Assert.True(before.NeedsFetch);
        Assert.Equal(Artwork.CdnFiles, before.WantedFromCdn);

        Assert.True(await artwork.FetchAsync(candidate, before, CancellationToken.None));

        // Tile, hero and logo asked for; the header only stands in for a hero and was not needed.
        Assert.Equal(
            [Cdn + "2124490/library_600x900.jpg", Cdn + "2124490/library_hero.jpg", Cdn + "2124490/logo.png"],
            fetcher.Asked);

        var after = artwork.Resolve(candidate, game.Path, "Game.exe");
        Assert.Equal(artwork.CdnCachePath("2124490", "library_600x900.jpg"), after.Tile);
        Assert.False(after.TileIsSquare);
        Assert.Equal(artwork.CdnCachePath("2124490", "library_hero.jpg"), after.Hero);
        Assert.Null(after.Logo);
        Assert.Null(after.Blur);

        // The logo's 404 is remembered, and this run asks nothing more for this game — not even
        // from a look taken before the answer came.
        Assert.True(File.Exists(artwork.MissingMarkerPath("2124490", "logo.png")));
        Assert.False(artwork.Lookup(candidate, game.Path, "Game.exe").NeedsFetch);
        Assert.False(await artwork.FetchAsync(candidate, before, CancellationToken.None));
        Assert.Equal(3, fetcher.Asked.Count);

        // A fresh run with everything cached or remembered asks nothing either.
        var nextRun = new Artwork(root.Path, steamRoot: null, fetcher);
        Assert.False(nextRun.Lookup(candidate, game.Path, "Game.exe").NeedsFetch);
    }

    [Fact]
    public async Task A_missing_marker_is_honoured_for_seven_days_and_then_forgotten()
    {
        using var root = new TempDir();
        var fetcher = new StubFetcher();
        var candidate = Steam("2124490");

        var fresh = new Artwork(root.Path, steamRoot: null, fetcher);
        foreach (var name in Artwork.CdnFiles)
        {
            var marker = fresh.MissingMarkerPath("2124490", name);
            Directory.CreateDirectory(Path.GetDirectoryName(marker)!);
            File.WriteAllText(marker, "404");
        }
        var remembered = fresh.Lookup(candidate, null, null);
        Assert.False(remembered.NeedsFetch);
        Assert.False(await fresh.FetchAsync(candidate, remembered, CancellationToken.None));
        Assert.Empty(fetcher.Asked);

        // Eight days on, the markers have expired: everything is asked again, and a 404 now is
        // remembered anew, so the marker's date moves.
        var stale = new Artwork(root.Path, steamRoot: null, fetcher);
        var eightDaysAgo = DateTime.UtcNow - TimeSpan.FromDays(8);
        foreach (var name in Artwork.CdnFiles) File.SetLastWriteTimeUtc(stale.MissingMarkerPath("2124490", name), eightDaysAgo);

        var expired = stale.Lookup(candidate, null, null);
        Assert.True(expired.NeedsFetch);
        Assert.False(await stale.FetchAsync(candidate, expired, CancellationToken.None));   // still 404 everywhere: nothing landed
        Assert.Equal(4, fetcher.Asked.Count);
        Assert.All(Artwork.CdnFiles, name =>
            Assert.True(DateTime.UtcNow - File.GetLastWriteTimeUtc(stale.MissingMarkerPath("2124490", name)) < TimeSpan.FromHours(1)));
    }

    [Fact]
    public async Task The_header_is_fetched_only_when_there_is_no_hero_and_then_stands_in_for_it()
    {
        using var root = new TempDir();
        var fetcher = new StubFetcher();
        fetcher.Answers["header.jpg"] = Saves(Pictures.Jpeg(460, 215));
        var artwork = new Artwork(root.Path, steamRoot: null, fetcher);
        var candidate = Steam("1030840");

        Assert.True(await artwork.FetchAsync(candidate, artwork.Lookup(candidate, null, null), CancellationToken.None));

        Assert.Equal(
            [Cdn + "1030840/library_600x900.jpg", Cdn + "1030840/library_hero.jpg", Cdn + "1030840/logo.png", Cdn + "1030840/header.jpg"],
            fetcher.Asked);
        var art = artwork.Resolve(candidate, null, null);
        Assert.Equal(artwork.CdnCachePath("1030840", "header.jpg"), art.Hero);
        Assert.Equal(artwork.CdnCachePath("1030840", "header.jpg"), art.Tile);   // a tile of sorts, as the local header has always been
        Assert.False(art.TileIsSquare);
    }

    [Fact]
    public async Task A_fetched_file_that_is_not_a_picture_is_thrown_away_and_a_failure_is_not_remembered_as_missing()
    {
        using var root = new TempDir();
        var fetcher = new StubFetcher();
        fetcher.Answers["library_600x900.jpg"] = Saves(Encoding.ASCII.GetBytes("<html>Access Denied</html>"));
        fetcher.Answers["library_hero.jpg"] = Fails;
        fetcher.Answers["logo.png"] = Fails;
        fetcher.Answers["header.jpg"] = Fails;
        var artwork = new Artwork(root.Path, steamRoot: null, fetcher);
        var candidate = Steam("2124490");

        Assert.False(await artwork.FetchAsync(candidate, artwork.Lookup(candidate, null, null), CancellationToken.None));

        Assert.False(File.Exists(artwork.CdnCachePath("2124490", "library_600x900.jpg")));
        Assert.Equal(GameArt.None, artwork.Resolve(candidate, null, null));
        Assert.All(Artwork.CdnFiles, name => Assert.False(File.Exists(artwork.MissingMarkerPath("2124490", name))));

        // Asked once in this run, whatever the answer was; the next start asks again.
        Assert.False(artwork.Lookup(candidate, null, null).NeedsFetch);
        Assert.True(new Artwork(root.Path, steamRoot: null, fetcher).Lookup(candidate, null, null).NeedsFetch);
    }

    [Fact]
    public void Xbox_games_get_their_own_visuals_and_never_ask_the_CDN()
    {
        using var dir = new TempDir();
        using var root = new TempDir();
        var content = XboxContent(dir, ForzaVisuals, "Storelogo.png", "Logo.png", "SmallLogo.png", "SplashScreen.png");
        var fetcher = new StubFetcher();
        var artwork = new Artwork(root.Path, steamRoot: null, fetcher);
        var candidate = new GameCandidate("Forza Horizon 6", GameStore.Xbox, content, "Microsoft.ForteBaseGame", "forzahorizon6.exe");

        var lookup = artwork.Lookup(candidate, content, "forzahorizon6.exe");

        Assert.Equal(new GameArt(Path.Combine(content, "Logo.png"), TileIsSquare: true, Path.Combine(content, "SplashScreen.png"), null, null), lookup.Art);
        Assert.False(lookup.NeedsFetch);
        Assert.Empty(lookup.WantedFromCdn);
    }

    [Fact]
    public void The_exes_own_icon_is_the_tile_of_last_resort_and_is_extracted_once()
    {
        if (ExeIconTests.AnExeWithAnIcon() is not { } withIcon) return;

        using var game = new TempDir();
        using var root = new TempDir();
        var exeDirectory = Path.Combine(game.Path, "Binaries", "Win64");
        Directory.CreateDirectory(exeDirectory);
        File.Copy(withIcon, Path.Combine(exeDirectory, "Game-Win64-Shipping.exe"));
        var artwork = new Artwork(root.Path, steamRoot: null);
        var candidate = new GameCandidate("Game", GameStore.Manual, exeDirectory);

        var art = artwork.Resolve(candidate, exeDirectory, "Game-Win64-Shipping.exe");

        Assert.NotNull(art.Tile);
        Assert.True(art.TileIsSquare);
        Assert.Equal(Path.Combine(root.Path, "exe"), Path.GetDirectoryName(art.Tile));
        Assert.Equal(".png", Path.GetExtension(art.Tile));
        Assert.Equal(new ImageInfo(ImageFormat.Png, 256, 256, HasAlpha: true), ImageHeader.Read(art.Tile));
        Assert.Null(art.Hero);
        Assert.Null(art.Logo);

        var written = File.GetLastWriteTimeUtc(art.Tile);
        Assert.Equal(art, artwork.Resolve(candidate, exeDirectory, "Game-Win64-Shipping.exe"));
        Assert.Equal(written, File.GetLastWriteTimeUtc(art.Tile));   // cached, not extracted again
    }

    [Fact]
    public void When_the_game_exe_has_no_icon_the_launcher_stub_at_the_install_root_lends_its_own()
    {
        // An Unreal shipping exe without an icon under Binaries\Win64, and the stub at the root with one.
        if (ExeIconTests.AnExeWithAnIcon() is not { } withIcon) return;

        using var game = new TempDir();
        using var root = new TempDir();
        var exeDirectory = Path.Combine(game.Path, "Hk_project", "Binaries", "Win64");
        Directory.CreateDirectory(exeDirectory);
        File.WriteAllBytes(Path.Combine(exeDirectory, "Stray-Win64-Shipping.exe"), PeImportsTests.Build(pe32Plus: true, ["KERNEL32.dll"]));
        File.Copy(withIcon, Path.Combine(game.Path, "Stray.exe"));
        var artwork = new Artwork(root.Path, steamRoot: null);

        var art = artwork.Resolve(new GameCandidate("Stray", GameStore.Epic, game.Path), exeDirectory, "Stray-Win64-Shipping.exe");

        Assert.NotNull(art.Tile);
        Assert.True(art.TileIsSquare);

        // Neither exe with an icon anywhere: no tile at all, and nothing thrown.
        using var bare = new TempDir();
        File.WriteAllBytes(Path.Combine(bare.Path, "Bare.exe"), PeImportsTests.Build(pe32Plus: false, ["KERNEL32.dll"]));
        Assert.Equal(GameArt.None, artwork.Resolve(new GameCandidate("Bare", GameStore.Manual, bare.Path), bare.Path, "Bare.exe"));
    }

    /// <summary>Two real exes whose icons differ — the dotnet muxer and notepad — so a test can
    /// tell whose icon a tile came from. Null when this PC has not both.</summary>
    private static (string Game, string Tool)? TwoExesWithDifferentIcons()
    {
        var dotnet = ExeIconTests.AnExeWithAnIcon();
        var notepad = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "notepad.exe");
        if (dotnet is null || !File.Exists(notepad) || string.Equals(dotnet, notepad, StringComparison.OrdinalIgnoreCase)) return null;

        using var dir = new TempDir();
        var a = Path.Combine(dir.Path, "a.png");
        var b = Path.Combine(dir.Path, "b.png");
        if (!ExeIcon.ExtractPng(dotnet, a) || !ExeIcon.ExtractPng(notepad, b)) return null;
        return File.ReadAllBytes(a).AsSpan().SequenceEqual(File.ReadAllBytes(b)) ? null : (dotnet, notepad);
    }

    private static byte[] IconOf(string exe)
    {
        using var dir = new TempDir();
        var png = Path.Combine(dir.Path, "icon.png");
        Assert.True(ExeIcon.ExtractPng(exe, png));
        return File.ReadAllBytes(png);
    }

    private static readonly byte[] IconlessExe = PeImportsTests.Build(pe32Plus: true, ["KERNEL32.dll"]);

    [Fact]
    public void At_the_root_the_exe_the_store_names_or_the_one_named_like_the_folder_lends_its_icon_before_any_other()
    {
        // Spider-Man's root on this PC holds crs-handler.exe and crs-video.exe beside the game;
        // alphabetically the crash handler comes first, and its icon is not the game's.
        if (TwoExesWithDifferentIcons() is not var (gameExe, toolExe)) return;

        using var library = new TempDir();
        using var root = new TempDir();
        var artwork = new Artwork(root.Path, steamRoot: null);

        // Named like the folder: Stray\Stray.exe, with the shipping exe under Binaries iconless.
        var stray = Path.Combine(library.Path, "Stray");
        var exeDirectory = Path.Combine(stray, "Hk_project", "Binaries", "Win64");
        Directory.CreateDirectory(exeDirectory);
        File.WriteAllBytes(Path.Combine(exeDirectory, "Stray-Win64-Shipping.exe"), IconlessExe);
        File.Copy(toolExe, Path.Combine(stray, "crs-handler.exe"));
        File.Copy(gameExe, Path.Combine(stray, "Stray.exe"));

        var art = artwork.Resolve(new GameCandidate("Stray", GameStore.Epic, stray), exeDirectory, "Stray-Win64-Shipping.exe");
        Assert.NotNull(art.Tile);
        Assert.Equal(IconOf(gameExe), File.ReadAllBytes(art.Tile));

        // Named by the store: the Xbox config's exe, whatever the folder is called.
        var named = Path.Combine(library.Path, "Forza Horizon 6", "Content");
        Directory.CreateDirectory(named);
        File.Copy(toolExe, Path.Combine(named, "crs-handler.exe"));
        File.Copy(gameExe, Path.Combine(named, "forzahorizon6.exe"));

        var hinted = artwork.Resolve(new GameCandidate("Forza Horizon 6", GameStore.Xbox, named, "Microsoft.ForteBaseGame", "forzahorizon6.exe"), null, null);
        Assert.NotNull(hinted.Tile);
        Assert.Equal(IconOf(gameExe), File.ReadAllBytes(hinted.Tile));
    }

    [Fact]
    public void A_root_crowded_with_exes_lends_an_icon_only_through_the_exe_named_for_the_game()
    {
        // A root with more than a handful of exes is a tools folder — redistributables, crash
        // handlers, a setup — and the first of them alphabetically is nobody's cover.
        if (ExeIconTests.AnExeWithAnIcon() is not { } withIcon) return;

        using var library = new TempDir();
        using var root = new TempDir();
        var game = Path.Combine(library.Path, "Game");
        Directory.CreateDirectory(game);
        for (var i = 1; i <= 5; i++) File.WriteAllBytes(Path.Combine(game, $"tool{i}.exe"), IconlessExe);
        File.Copy(withIcon, Path.Combine(game, "a-redist.exe"));   // first alphabetically, with an icon
        var artwork = new Artwork(root.Path, steamRoot: null);
        var candidate = new GameCandidate("Game", GameStore.Manual, game);

        Assert.Equal(GameArt.None, artwork.Resolve(candidate, null, null));
        Assert.False(Directory.Exists(Path.Combine(root.Path, "exe")));

        File.Copy(withIcon, Path.Combine(game, "Game.exe"));
        var art = artwork.Resolve(candidate, null, null);
        Assert.NotNull(art.Tile);
        Assert.True(art.TileIsSquare);
    }

    [Fact]
    public async Task Nothing_is_found_and_nothing_throws_for_a_game_with_no_folder_or_no_appid()
    {
        using var root = new TempDir();
        var fetcher = new StubFetcher();
        var artwork = new Artwork(root.Path, steamRoot: @"Z:\no\such\steam", fetcher);

        Assert.Equal(GameArt.None, artwork.Resolve(new GameCandidate("Epic Game", GameStore.Epic, @"Z:\no\such\game"), null, null));
        Assert.Equal(GameArt.None, artwork.Resolve(Steam(null), @"Z:\no\such\game", "Game.exe"));
        Assert.Equal(GameArt.None, artwork.Resolve(Steam("Microsoft.ForteBaseGame"), null, null));   // not an appid
        Assert.Equal(GameArt.None, artwork.Resolve(Steam(@"..\..\x"), null, null));

        Assert.False(artwork.Lookup(Steam(null), null, null).NeedsFetch);
        Assert.False(artwork.Lookup(Steam("Microsoft.ForteBaseGame"), null, null).NeedsFetch);
        Assert.False(artwork.Lookup(new GameCandidate("Epic Game", GameStore.Epic, @"Z:\no\such\game", "300"), null, null).NeedsFetch);
        Assert.False(await artwork.FetchAsync(Steam(@"..\..\x"), artwork.Lookup(Steam(@"..\..\x"), null, null), CancellationToken.None));

        // Nor is a look for one game good for another, or a name that is not the CDN's: only the
        // CDN's own file names are ever joined onto its address.
        Assert.False(await artwork.FetchAsync(Steam(@"..\..\x"), new ArtLookup(GameArt.None, ["library_hero.jpg"]), CancellationToken.None));
        Assert.False(await artwork.FetchAsync(Steam("300"), new ArtLookup(GameArt.None, ["..\\..\\evil.jpg"]), CancellationToken.None));
        Assert.Empty(fetcher.Asked);
        Assert.False(Directory.Exists(Path.Combine(root.Path, "steam")));
    }

    // ---------------------------------------------------------------- the real fetcher, offline

    private sealed class StubHandler(Func<HttpRequestMessage, HttpResponseMessage> answer) : HttpMessageHandler
    {
        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken ct)
            => Task.FromResult(answer(request));
    }

    private static HttpArtFetcher Fetcher(Func<HttpRequestMessage, HttpResponseMessage> answer)
        => new(new HttpClient(new StubHandler(answer)));

    [Fact]
    public async Task The_HTTP_fetcher_saves_a_200_reports_a_404_as_missing_and_anything_else_as_failed()
    {
        using var root = new TempDir();
        var destination = Path.Combine(root.Path, "steam", "2124490", "library_hero.jpg");
        var picture = Pictures.Jpeg(1920, 620);
        var url = Artwork.CdnUrl("2124490", "library_hero.jpg");

        var saved = Fetcher(_ => new HttpResponseMessage(HttpStatusCode.OK) { Content = new ByteArrayContent(picture) });
        Assert.Equal(ArtFetch.Saved, await saved.FetchAsync(url, destination, CancellationToken.None));
        Assert.Equal(picture, File.ReadAllBytes(destination));
        Assert.False(File.Exists(destination + ".part"));

        var missing = Fetcher(_ => new HttpResponseMessage(HttpStatusCode.NotFound));
        Assert.Equal(ArtFetch.Missing, await missing.FetchAsync(url, Path.Combine(root.Path, "m.jpg"), CancellationToken.None));

        var error = Fetcher(_ => new HttpResponseMessage(HttpStatusCode.InternalServerError));
        Assert.Equal(ArtFetch.Failed, await error.FetchAsync(url, Path.Combine(root.Path, "e.jpg"), CancellationToken.None));

        var offline = Fetcher(_ => throw new HttpRequestException("no network"));
        Assert.Equal(ArtFetch.Failed, await offline.FetchAsync(url, Path.Combine(root.Path, "o.jpg"), CancellationToken.None));

        // Bigger than any piece of Steam art: not downloaded into the user's profile.
        var huge = Fetcher(_ =>
        {
            var response = new HttpResponseMessage(HttpStatusCode.OK) { Content = new ByteArrayContent(new byte[16]) };
            response.Content.Headers.ContentLength = HttpArtFetcher.MaxBytes + 1;
            return response;
        });
        Assert.Equal(ArtFetch.Failed, await huge.FetchAsync(url, Path.Combine(root.Path, "h.jpg"), CancellationToken.None));

        Assert.Equal([destination], Directory.EnumerateFiles(root.Path, "*", SearchOption.AllDirectories));
    }

    /// <summary>A connection that never answers: the request waits for as long as it is let.</summary>
    private sealed class StallingHandler : HttpMessageHandler
    {
        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken ct)
        {
            await Task.Delay(Timeout.Infinite, ct);
            throw new InvalidOperationException("a stalled request does not answer");
        }
    }

    [Fact]
    public async Task A_CDN_connection_that_stalls_is_given_up_on_and_reported_as_failed()
    {
        // The launcher's HttpClient waits 100 s by default, and a fetch that waits that long holds
        // one of the four slots every other game's cover queues for.
        using var root = new TempDir();
        var fetcher = new HttpArtFetcher(new HttpClient(new StallingHandler()), perFileTimeout: TimeSpan.FromMilliseconds(200));
        var destination = Path.Combine(root.Path, "steam", "2124490", "library_hero.jpg");

        var watch = System.Diagnostics.Stopwatch.StartNew();
        var result = await fetcher.FetchAsync(Artwork.CdnUrl("2124490", "library_hero.jpg"), destination, CancellationToken.None);

        Assert.Equal(ArtFetch.Failed, result);
        Assert.True(watch.Elapsed < TimeSpan.FromSeconds(10), $"gave up only after {watch.Elapsed}");
        Assert.Empty(Directory.EnumerateFiles(root.Path, "*", SearchOption.AllDirectories));
        Assert.Equal(TimeSpan.FromSeconds(15), HttpArtFetcher.DefaultPerFileTimeout);
    }
}
