// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using AmdnrLauncher.App.ViewModels;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>A cover for every row: what the rail and the stage bind to as the pictures arrive.
/// Every folder here is a temporary one; the fetcher is a stub that writes a hand-built header,
/// and nothing reaches the network.</summary>
public sealed class GameRowArtTests : IDisposable
{
    private readonly string _root = Path.Combine(Path.GetTempPath(), "amdnr-art-" + Guid.NewGuid().ToString("N"));

    public GameRowArtTests() => Directory.CreateDirectory(_root);

    public void Dispose()
    {
        try { Directory.Delete(_root, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    private string Cache => Path.Combine(_root, "art");

    private static GameEntry Entry(GameCandidate candidate, string? exeDirectory, string? exeName) => new(
        candidate,
        ExeDirectory: exeDirectory,
        ExeName: exeName,
        Status: GameStatus.NotInstalled,
        Record: null,
        Report: null,
        Gate: null,
        RecommendedProxy: "dxgi.dll",
        ProxyReason: "first free name",
        OccupiedProxies: []);

    /// <summary>Only the bytes a header reader looks at: SOI, one SOF0 frame of the given size, EOI.</summary>
    private static byte[] Jpeg(int width, int height) =>
    [
        0xFF, 0xD8,
        0xFF, 0xC0, 0x00, 0x11, 0x08, (byte)(height >> 8), (byte)height, (byte)(width >> 8), (byte)width,
        0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01,
        0xFF, 0xD9,
    ];

    private static readonly byte[] Png150 =
    [
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
        0, 0, 0, 13, (byte)'I', (byte)'H', (byte)'D', (byte)'R', 0, 0, 0, 150, 0, 0, 0, 150, 8, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, (byte)'I', (byte)'E', (byte)'N', (byte)'D', 0, 0, 0, 0,
    ];

    /// <summary>The dotnet muxer this test runs under, or notepad: real exes with real icons.</summary>
    internal static string? AnExeWithAnIcon()
    {
        string?[] candidates =
        [
            Path.GetFullPath(Path.Combine(RuntimeEnvironment.GetRuntimeDirectory(), "..", "..", "..", "dotnet.exe")),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "dotnet", "dotnet.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "notepad.exe"),
        ];
        return candidates.FirstOrDefault(File.Exists);
    }

    private sealed class StubFetcher : IArtFetcher
    {
        public List<string> Asked { get; } = [];
        public Dictionary<string, byte[]> Pictures { get; } = new(StringComparer.OrdinalIgnoreCase);

        public Task<ArtFetch> FetchAsync(Uri url, string destinationPath, CancellationToken ct)
        {
            Asked.Add(url.AbsoluteUri);
            if (!Pictures.TryGetValue(Path.GetFileName(url.LocalPath), out var bytes)) return Task.FromResult(ArtFetch.Missing);

            Directory.CreateDirectory(Path.GetDirectoryName(destinationPath)!);
            File.WriteAllBytes(destinationPath, bytes);
            return Task.FromResult(ArtFetch.Saved);
        }
    }

    [Fact]
    public async Task A_row_with_nowhere_to_look_shows_the_plate_and_the_initial_alone()
    {
        var row = new GameRowViewModel(Entry(new GameCandidate("Stray", GameStore.Steam, @"D:\Games\Stray"), null, null));
        await row.ArtLoaded;

        Assert.Equal("S", row.Initial);
        Assert.Null(row.PortraitPath);
        Assert.Null(row.SquareTilePath);
        Assert.False(row.HasTile);
        Assert.Null(row.HeroPath);
        Assert.Null(row.LogoPath);
        Assert.False(row.HasLogo);
        Assert.Null(row.HeroBlurPath);
        Assert.Null(row.BackdropToBlurPath);
        Assert.Null(row.StageBackdropPath);
    }

    [Fact]
    public async Task A_Steam_row_shows_the_exe_icon_letterboxed_until_the_cover_lands_then_the_cover()
    {
        if (AnExeWithAnIcon() is not { } withIcon) return;

        var game = Path.Combine(_root, "game");
        Directory.CreateDirectory(game);
        File.Copy(withIcon, Path.Combine(game, "Game.exe"));

        var fetcher = new StubFetcher();
        fetcher.Pictures["library_600x900.jpg"] = Jpeg(600, 900);
        fetcher.Pictures["library_hero.jpg"] = Jpeg(1920, 620);
        var artwork = new Artwork(Cache, steamRoot: null, fetcher);
        var candidate = new GameCandidate("Game", GameStore.Steam, game, "2124490");

        var changed = new List<string?>();
        var row = new GameRowViewModel(Entry(candidate, game, "Game.exe"), artwork);
        row.PropertyChanged += (_, e) => changed.Add(e.PropertyName);

        // The scan never waits for any of this: the row is usable at once.
        Assert.Equal("Game", row.Name);

        await row.ArtLoaded;

        Assert.Equal(artwork.CdnCachePath("2124490", "library_600x900.jpg"), row.PortraitPath);
        Assert.Null(row.SquareTilePath);
        Assert.True(row.HasTile);
        Assert.Equal(artwork.CdnCachePath("2124490", "library_hero.jpg"), row.HeroPath);
        Assert.Null(row.LogoPath);
        Assert.False(row.HasLogo);

        // No pre-blurred hero from Steam: the window blurs the hero itself; the stage has a hero
        // and needs no icon behind it.
        Assert.Null(row.HeroBlurPath);
        Assert.Equal(row.HeroPath, row.BackdropToBlurPath);
        Assert.Null(row.StageBackdropPath);

        Assert.Contains(nameof(GameRowViewModel.PortraitPath), changed);
        Assert.Contains(nameof(GameRowViewModel.HeroPath), changed);
        Assert.Equal(3, fetcher.Asked.Count);

        // The icon was the tile before the cover came — it is in the art folder now, extracted
        // while the CDN had not answered — and the CDN is not asked again for this game.
        Assert.Single(Directory.EnumerateFiles(Path.Combine(Cache, "exe")));
        var again = new GameRowViewModel(Entry(candidate, game, "Game.exe"), artwork);
        await again.ArtLoaded;
        Assert.Equal(row.PortraitPath, again.PortraitPath);
        Assert.Equal(3, fetcher.Asked.Count);
    }

    [Fact]
    public async Task The_CDN_is_not_asked_for_a_game_whose_Steam_cache_already_has_its_pictures()
    {
        // The owner's library: hundreds of games Steam has shown, each with its pictures in
        // Steam's own cache. The CDN is for what that cache did not yield — never a second copy
        // of every cover into the user's profile, three requests a game, on a first start.
        var steam = Path.Combine(_root, "Steam");
        var perApp = Path.Combine(steam, "appcache", "librarycache", "1332010");
        Directory.CreateDirectory(perApp);
        var tile = Path.Combine(perApp, "library_600x900.jpg");
        var hero = Path.Combine(perApp, "library_hero.jpg");
        var logo = Path.Combine(perApp, "logo.png");
        File.WriteAllBytes(tile, Jpeg(600, 900));
        File.WriteAllBytes(hero, Jpeg(1920, 620));
        File.WriteAllBytes(logo, Png150);
        var fetcher = new StubFetcher();
        fetcher.Pictures["library_600x900.jpg"] = Jpeg(600, 900);
        fetcher.Pictures["library_hero.jpg"] = Jpeg(1920, 620);
        fetcher.Pictures["logo.png"] = Png150;
        var artwork = new Artwork(Cache, steam, fetcher);

        var row = new GameRowViewModel(Entry(new GameCandidate("Stray", GameStore.Steam, @"D:\Games\Stray", "1332010"), null, null), artwork);
        await row.ArtLoaded;

        Assert.Equal(tile, row.PortraitPath);
        Assert.Equal(hero, row.HeroPath);
        Assert.Equal(logo, row.LogoPath);
        Assert.Empty(fetcher.Asked);
        Assert.False(Directory.Exists(Path.Combine(Cache, "steam")));
    }

    [Fact]
    public async Task An_Xbox_row_letterboxes_its_square_logo_and_puts_it_blurred_behind_the_stage_when_there_is_no_splash()
    {
        var content = Path.Combine(_root, "Forza Horizon 6", "Content");
        Directory.CreateDirectory(content);
        File.WriteAllBytes(Path.Combine(content, "Logo.png"), Png150);
        File.WriteAllBytes(Path.Combine(content, "SplashScreen.png"), Png150);
        File.WriteAllText(Path.Combine(content, "MicrosoftGame.Config"), """
            <Game><ShellVisuals DefaultDisplayName="Forza Horizon 6" Square150x150Logo="Logo.png" SplashScreenImage="SplashScreen.png" /></Game>
            """, new UTF8Encoding(true));
        var fetcher = new StubFetcher();
        var artwork = new Artwork(Cache, steamRoot: null, fetcher);
        var candidate = new GameCandidate("Forza Horizon 6", GameStore.Xbox, content, "Microsoft.ForteBaseGame", "forzahorizon6.exe");

        var row = new GameRowViewModel(Entry(candidate, content, "forzahorizon6.exe"), artwork);
        await row.ArtLoaded;

        Assert.Equal(Path.Combine(content, "Logo.png"), row.SquareTilePath);
        Assert.Null(row.PortraitPath);
        Assert.True(row.HasTile);
        Assert.Equal(Path.Combine(content, "SplashScreen.png"), row.HeroPath);
        Assert.Null(row.StageBackdropPath);                       // the splash is the stage
        Assert.Equal(row.HeroPath, row.BackdropToBlurPath);       // and, blurred, the window
        Assert.Empty(fetcher.Asked);                              // an Xbox game has no appid to ask about

        // Without a splash, the square logo — blurred and enlarged — stands behind the stage and the window.
        File.Delete(Path.Combine(content, "SplashScreen.png"));
        var bare = new GameRowViewModel(Entry(candidate, content, "forzahorizon6.exe"), artwork);
        await bare.ArtLoaded;
        Assert.Null(bare.HeroPath);
        Assert.Equal(bare.SquareTilePath, bare.StageBackdropPath);
        Assert.Equal(bare.SquareTilePath, bare.BackdropToBlurPath);
    }

    [Fact]
    public async Task Steams_own_pre_blurred_hero_is_used_as_it_is_and_nothing_is_blurred_twice()
    {
        var steam = Path.Combine(_root, "Steam");
        var perApp = Path.Combine(steam, "appcache", "librarycache", "2124490");
        Directory.CreateDirectory(perApp);
        var hero = Path.Combine(perApp, "library_hero.jpg");
        var blur = Path.Combine(perApp, "library_hero_blur.jpg");
        var logo = Path.Combine(perApp, "logo.png");
        File.WriteAllBytes(hero, Jpeg(1920, 620));
        File.WriteAllBytes(blur, Jpeg(192, 62));
        File.WriteAllBytes(logo, Png150);
        var artwork = new Artwork(Cache, steam);

        var row = new GameRowViewModel(Entry(new GameCandidate("SILENT HILL 2", GameStore.Steam, @"D:\Games\SH2", "2124490"), null, null), artwork);
        await row.ArtLoaded;

        Assert.Equal(hero, row.HeroPath);
        Assert.Equal(blur, row.HeroBlurPath);
        Assert.Equal(logo, row.LogoPath);
        Assert.True(row.HasLogo);
        Assert.Null(row.BackdropToBlurPath);
        Assert.Null(row.StageBackdropPath);
    }

    [Fact]
    public void The_icon_PNG_the_launcher_writes_is_one_WPF_decodes_with_its_alpha()
    {
        if (AnExeWithAnIcon() is not { } withIcon) return;

        var png = Path.Combine(_root, "icon.png");
        Assert.True(ExeIcon.ExtractPng(withIcon, png));

        var image = new BitmapImage();
        image.BeginInit();
        image.CacheOption = BitmapCacheOption.OnLoad;
        image.UriSource = new Uri(png, UriKind.Absolute);
        image.EndInit();
        image.Freeze();

        Assert.Equal(256, image.PixelWidth);
        Assert.Equal(256, image.PixelHeight);
        Assert.True(image.Format == PixelFormats.Bgra32 || image.Format == PixelFormats.Pbgra32,
            $"the icon decoded without an alpha channel: {image.Format}");
    }
}
