// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using System.Text.Json;
using AmdnrLauncher.App.ViewModels;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>StartAsync's steps, in their order. The scan runs over scanners these tests hand
/// in — none, or one naming a folder made here — never over the real ones: a scan probes every
/// game folder it finds for write access, and a test must never touch a real game install.</summary>
public sealed class StartupTests : IDisposable
{
    private readonly string _root = Path.Combine(
        Path.GetTempPath(), "amdnr-startup-" + Guid.NewGuid().ToString("N"));

    private readonly string _previousRecordRoot = InstallRecordStore.RootDirectory;

    private string CachePath => Path.Combine(_root, "manifest-cache.json");
    private string SettingsPath => Path.Combine(_root, "settings.json");

    /// <summary>Where the rows would put an exe's icon if a test ever built rows after
    /// PrepareAsync: here, never in the user's own AMDNR folder.</summary>
    private string ArtRoot => Path.Combine(_root, "art");

    private readonly RecordingAssetStore _store = new();
    private readonly List<LauncherInfo> _updateChecks = [];

    public StartupTests()
    {
        Directory.CreateDirectory(_root);

        // The scan reads install records by game folder; here, never from the user's own AMDNR folder.
        InstallRecordStore.RootDirectory = Path.Combine(_root, "records");
    }

    public void Dispose()
    {
        InstallRecordStore.RootDirectory = _previousRecordRoot;

        try { Directory.Delete(_root, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    private const string Manifest = """
        {
          "schemaVersion": 1,
          "launcher": { "version": "0.4.0", "url": "https://example.test/AMDNR-Launcher.exe", "sha256": "00" },
          "sources": [
            { "id": "amdnr", "name": "AMDNR", "author": "3zwr1", "summary": "AMDNR summary",
              "homepage": "https://github.com/3zwr1/AMD-NR---OptiScaler",
              "package": "amdnr", "forwarder": "forwarder", "runtimeRequired": false,
              "runtimeNote": "Needed on RX 7000." },
            { "id": "theautomatic", "name": "OptiScaler AMD pre-SR", "author": "TheAutomatic",
              "summary": "pre-SR summary", "homepage": "https://github.com/TheAutomatic/dlss-5-amd-project",
              "package": "theautomatic", "forwarder": null, "runtimeRequired": true,
              "runtimeNote": "Required." }
          ],
          "packages": [
            { "id": "amdnr", "version": "0.3.2", "url": "https://example.test/AMDNR-v0.3.2.zip", "sha256": "00", "size": 1 },
            { "id": "theautomatic", "version": "1.8.6-0.3.1", "url": "https://example.test/ta.zip", "sha256": "00", "size": 1 },
            { "id": "runtime", "version": "0.3.1", "url": "https://example.test/v0.3.1-Runtime.zip", "sha256": "00", "size": 110992515 },
            { "id": "forwarder", "version": "0.2.1", "url": "https://example.test/nvngx.dll_dlssnr.dll", "sha256": "00", "size": 114688 }
          ],
          "proxyDefaults": ["dxgi.dll", "d3d12.dll"],
          "proxyOverrides": []
        }
        """;

    private MainViewModel ViewModel(Settings settings, Func<string, string> fetch, IReadOnlyList<IGameScanner>? scanners = null)
        => new(settings, new StartupEnvironment(
            FetchManifest: (url, _) => Task.FromResult(fetch(url)),
            StageLauncherUpdate: (launcher, _) =>
            {
                _updateChecks.Add(launcher);
                return Task.FromResult<string?>(
                    launcher.Version == "0.9.0" ? Path.Combine(_root, "AmdnrLauncher-0.9.0.exe") : null);
            },
            Assets: _store,
            ManifestCachePath: CachePath,
            ArtCacheRoot: ArtRoot,
            Scanners: scanners ?? []));

    /// <summary>A library of one game, in a folder made here: what a scan has to find before
    /// the downloads begin.</summary>
    private sealed class OneGame(GameCandidate game) : IGameScanner
    {
        public IEnumerable<GameCandidate> Scan() => [game];
    }

    private GameCandidate GameInTemp()
    {
        var folder = Path.Combine(_root, "game");
        Directory.CreateDirectory(folder);
        File.WriteAllText(Path.Combine(folder, "Game.exe"), "stand-in for a game");
        return new GameCandidate("Test Game", GameStore.Steam, folder);
    }

    private Settings SettingsWith(InstallChoice? choice)
    {
        var settings = Settings.Load(SettingsPath);
        settings.Choice = choice;
        return settings;
    }

    [Theory]
    [InlineData("theautomatic", false, new[] { "theautomatic" })]
    [InlineData("theautomatic", true, new[] { "theautomatic", "runtime" })]
    [InlineData("amdnr", false, new[] { "amdnr", "forwarder" })]
    [InlineData("amdnr", true, new[] { "amdnr", "runtime", "forwarder" })]
    public async Task PackagesFor_the_saved_choice_is_what_gets_downloaded(
        string source, bool runtime, string[] expected)
    {
        var choice = new InstallChoice(source, runtime);
        var viewModel = ViewModel(SettingsWith(choice), _ => Manifest);

        await viewModel.StartAsync();

        // Every package the old launcher fetched unconditionally is ~110 MB of runtime a user
        // who said No never asked for, or a forwarder a build without one never loads.
        Assert.Equal(expected, _store.Ensured);
        Assert.Equal(viewModel.Service!.PackagesFor(choice).Select(p => p.Id), _store.Ensured);
        Assert.Null(viewModel.Chooser);
        Assert.Equal("0 games found.", viewModel.StatusMessage);
    }

    [Fact]
    public async Task A_manifest_that_fails_to_parse_is_not_cached()
    {
        var viewModel = ViewModel(SettingsWith(new InstallChoice("amdnr", true)),
            _ => """{ "schemaVersion": 1, "launcher": { "version": "0.4.0" """);

        await Assert.ThrowsAnyAsync<JsonException>(viewModel.PrepareAsync);

        // Cached, it would be what every offline start reads from now on — and it can never
        // be read.
        Assert.False(File.Exists(CachePath));
        Assert.Empty(_store.Ensured);
    }

    [Fact]
    public async Task A_manifest_that_fails_to_parse_leaves_the_last_good_one_in_use()
    {
        File.WriteAllText(CachePath, Manifest);
        var viewModel = ViewModel(SettingsWith(new InstallChoice("amdnr", true)), _ => "<html>captive portal</html>");

        await viewModel.StartAsync();

        Assert.Equal(Manifest, File.ReadAllText(CachePath));
        Assert.Equal("0.3.2", viewModel.Service!.Manifest.Package("amdnr").Version);

        // What the cache names was downloaded when it was fetched; a manifest nobody can read
        // is no reason to start fetching again.
        Assert.Empty(_store.Ensured);
    }

    [Fact]
    public async Task A_manifest_that_parses_is_cached()
    {
        var viewModel = ViewModel(SettingsWith(new InstallChoice("amdnr", true)), _ => Manifest);

        await viewModel.PrepareAsync();

        Assert.Equal(Manifest, File.ReadAllText(CachePath));
    }

    [Fact]
    public async Task A_launcher_update_is_staged_even_when_the_rest_of_the_manifest_cannot_be_read()
    {
        // A manifest written for a newer launcher is exactly the one this launcher cannot parse,
        // and the update it names is the only way out of that.
        var future = Manifest
            .Replace("\"schemaVersion\": 1", "\"schemaVersion\": 2")
            .Replace("\"version\": \"0.4.0\"", "\"version\": \"0.9.0\"");

        var viewModel = ViewModel(SettingsWith(new InstallChoice("amdnr", true)), _ => future);

        await Assert.ThrowsAsync<UnsupportedManifestException>(viewModel.PrepareAsync);

        Assert.Equal("0.9.0", Assert.Single(_updateChecks).Version);
        Assert.Equal(Path.Combine(_root, "AmdnrLauncher-0.9.0.exe"), viewModel.PendingUpdatePath);
        Assert.Empty(_store.Ensured);
    }

    [Fact]
    public async Task The_launcher_update_is_checked_before_anything_is_downloaded()
    {
        var order = new List<string>();
        _store.OnEnsure = id => order.Add(id);

        var viewModel = new MainViewModel(SettingsWith(new InstallChoice("amdnr", false)), new StartupEnvironment(
            FetchManifest: (_, _) => Task.FromResult(Manifest),
            StageLauncherUpdate: (_, _) => { order.Add("launcher"); return Task.FromResult<string?>(null); },
            Assets: _store,
            ManifestCachePath: CachePath,
            ArtCacheRoot: ArtRoot,
            Scanners: []));

        await viewModel.StartAsync();

        Assert.Equal(["launcher", "amdnr", "forwarder"], order);
    }

    [Fact]
    public async Task The_games_are_listed_before_the_downloads_begin()
    {
        // A first run, or a start after the manifest moved to a new build, fetches the better
        // part of a gigabyte. Downloaded first, that left the library empty for as long as it
        // took, under a line saying nothing had been found — the one thing a new user reads.
        // The scan is local and takes seconds; it goes first, and the downloads run under it.
        MainViewModel? viewModel = null;
        var gamesWhenEachDownloadBegan = new List<int>();
        _store.OnEnsure = _ => gamesWhenEachDownloadBegan.Add(viewModel!.Games.Count);

        viewModel = ViewModel(SettingsWith(new InstallChoice("amdnr", false)), _ => Manifest, [new OneGame(GameInTemp())]);

        await viewModel.StartAsync();

        Assert.Equal(["amdnr", "forwarder"], _store.Ensured);
        Assert.Equal([1, 1], gamesWhenEachDownloadBegan);
        Assert.Equal("Test Game", Assert.Single(viewModel.Games).Name);
        Assert.Equal("1 games found.", viewModel.StatusMessage);
        Assert.False(viewModel.Busy);
    }

    [Fact]
    public async Task Without_a_saved_choice_the_chooser_decides_what_is_downloaded_and_is_saved()
    {
        var viewModel = ViewModel(SettingsWith(null), _ => Manifest);

        var preparing = viewModel.StartAsync();

        var chooser = viewModel.Chooser;
        Assert.NotNull(chooser);
        Assert.True(chooser.IsFirstRun);
        Assert.Empty(_store.Ensured);

        // AMDNR and Yes are what an unsure user should end up with.
        Assert.Equal("amdnr", chooser.SelectedOption?.Source.Id);
        Assert.True(chooser.IncludeRuntime);
        Assert.Equal("Needed on RX 7000.", chooser.RuntimeNote);
        Assert.False(chooser.ShowRuntimeWarning);

        chooser.SelectedOption = chooser.Options.Single(o => o.Source.Id == "theautomatic");
        chooser.IncludeRuntime = false;
        Assert.True(chooser.ShowRuntimeWarning);
        Assert.Equal("Neural Rendering will not run without them.", chooser.RuntimeWarning);

        chooser.Continue();
        await preparing;

        Assert.Null(viewModel.Chooser);
        Assert.Equal(["theautomatic"], _store.Ensured);
        Assert.Equal(new InstallChoice("theautomatic", false), Settings.Load(SettingsPath).Choice);
    }

    [Fact]
    public async Task A_saved_choice_the_manifest_no_longer_offers_asks_again()
    {
        var viewModel = ViewModel(SettingsWith(new InstallChoice("withdrawn", true)), _ => Manifest);

        var preparing = viewModel.PrepareAsync();

        Assert.NotNull(viewModel.Chooser);
        viewModel.Chooser.Continue();
        await preparing;

        Assert.Equal(new InstallChoice("amdnr", true), Settings.Load(SettingsPath).Choice);
    }

    [Fact]
    public async Task A_test_manifest_is_never_cached()
    {
        // The next ordinary start that finds itself offline would otherwise read the tester's
        // manifest, with no TEST MANIFEST label to say so.
        var settings = SettingsWith(new InstallChoice("amdnr", true));
        settings.ApplyCommandLine(["--manifest", "https://example.test/branch/manifest.json"]);
        string? fetched = null;
        var viewModel = ViewModel(settings, url => { fetched = url; return Manifest; });

        await viewModel.PrepareAsync();

        Assert.Equal("https://example.test/branch/manifest.json", fetched);
        Assert.False(File.Exists(CachePath));
        Assert.True(viewModel.IsTestManifest);
    }

    /// <summary>A manifest that names its runtimes per generation, as the shipping one does —
    /// runtime-033, runtime-040 — with 0.4.0 listed first for RDNA 4, as it will be once the
    /// owner makes it that card's default by reordering. The forwarder too is called by a name
    /// of the publisher's choosing, not the one the first manifests happened to use.</summary>
    private const string ManifestWithRuntimesByGeneration = """
        {
          "schemaVersion": 1,
          "launcher": { "version": "0.4.0", "url": "https://example.test/AMDNR-Launcher.exe", "sha256": "00" },
          "sources": [
            { "id": "amdnr", "name": "AMDNR", "author": "3zwr1", "summary": "AMDNR summary",
              "homepage": "https://github.com/3zwr1/AMD-NR---OptiScaler",
              "package": "amdnr", "forwarder": "nvngx-forwarder", "runtimeRequired": false,
              "runtimeNote": "Needed on RX 7000.",
              "runtimes": [
                { "package": "runtime-040", "gpus": ["rdna4"] },
                { "package": "runtime-033", "gpus": ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"] } ] }
          ],
          "packages": [
            { "id": "amdnr", "version": "0.3.3.1", "url": "https://example.test/AMDNR-v0.3.3.1.zip", "sha256": "00", "size": 1 },
            { "id": "runtime-033", "version": "0.3.3", "url": "https://example.test/v0.3.3-Runtime.zip", "sha256": "00", "size": 109730870 },
            { "id": "runtime-040", "version": "0.4.0", "url": "https://example.test/v0.4.0-Runtime.zip", "sha256": "00", "size": 113039884 },
            { "id": "nvngx-forwarder", "version": "0.2.1", "url": "https://example.test/nvngx.dll_dlssnr.dll", "sha256": "00", "size": 114688 }
          ],
          "proxyDefaults": ["dxgi.dll", "d3d12.dll"],
          "proxyOverrides": []
        }
        """;

    [Theory]
    [InlineData("rdna3", "runtime-033", "0.3.3")]
    [InlineData("rdna4", "runtime-040", "0.4.0")]
    public async Task The_status_line_calls_each_package_what_it_is_whatever_the_manifest_names_it(
        string generation, string runtimeId, string runtimeVersion)
    {
        // A stranger's first run. "Downloading runtime-033 0.3.3…" is a package id, not a
        // sentence; the line has to say what is being fetched in words, for every id the
        // publisher might give the runtime or the forwarder.
        var status = new Dictionary<string, string>();
        MainViewModel? viewModel = null;
        _store.OnEnsure = id => status[id] = viewModel!.StatusMessage;

        viewModel = new MainViewModel(SettingsWith(new InstallChoice("amdnr", true)), new StartupEnvironment(
            FetchManifest: (_, _) => Task.FromResult(ManifestWithRuntimesByGeneration),
            StageLauncherUpdate: (_, _) => Task.FromResult<string?>(null),
            Assets: _store,
            ManifestCachePath: CachePath,
            Gpu: new GpuInfo("AMD Radeon RX", generation, "32.0.1", null),
            ArtCacheRoot: ArtRoot,
            Scanners: []));

        await viewModel.StartAsync();

        Assert.Equal(["amdnr", runtimeId, "nvngx-forwarder"], _store.Ensured);
        Assert.Equal($"Downloading the DLSSNR AMD files {runtimeVersion}…", status[runtimeId]);
        Assert.Equal("Downloading AMDNR 0.3.3.1…", status["amdnr"]);
        Assert.Equal("Downloading the DLSSNR forwarder 0.2.1…", status["nvngx-forwarder"]);
    }

    [Fact]
    public async Task The_runtime_the_user_picked_is_what_downloads_and_what_the_Doctor_measures_against()
    {
        // 0.4.0 first for rdna4 in this manifest; the saved choice picks 0.3.3.
        var viewModel = new MainViewModel(SettingsWith(new InstallChoice("amdnr", true, "runtime-033")), new StartupEnvironment(
            FetchManifest: (_, _) => Task.FromResult(ManifestWithRuntimesByGeneration),
            StageLauncherUpdate: (_, _) => Task.FromResult<string?>(null),
            Assets: _store,
            ManifestCachePath: CachePath,
            Gpu: new GpuInfo("AMD Radeon RX", "rdna4", "32.0.1", null),
            ArtCacheRoot: ArtRoot,
            Scanners: []));

        await viewModel.StartAsync();

        Assert.Equal(["amdnr", "runtime-033", "nvngx-forwarder"], _store.Ensured);
        Assert.Equal("runtime-033", viewModel.Service!.PreferredRuntimeId);
    }

    private sealed class RecordingAssetStore : IAssetStore
    {
        public List<string> Ensured { get; } = [];
        public Action<string>? OnEnsure { get; set; }

        public bool Has(PackageInfo package) => false;

        public string PathFor(PackageInfo package) => Path.Combine(Path.GetTempPath(), "never", package.Id);

        public Task EnsureAsync(PackageInfo package, IProgress<DownloadProgress>? progress, CancellationToken ct)
        {
            Ensured.Add(package.Id);
            OnEnsure?.Invoke(package.Id);
            return Task.CompletedTask;
        }
    }
}
