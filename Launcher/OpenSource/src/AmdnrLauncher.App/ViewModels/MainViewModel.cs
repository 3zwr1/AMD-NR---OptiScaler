// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Net.Http;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Diagnostics;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.ViewModels;

/// <summary>Everything StartAsync reaches outside the process for, so the order of its steps
/// can be tested without a network, a download or a real game folder. <paramref name="Scanners"/>
/// null means the real three — Steam, Epic, the Xbox app — which a test never wants.</summary>
internal sealed record StartupEnvironment(
    Func<string, CancellationToken, Task<string>> FetchManifest,
    Func<LauncherInfo, CancellationToken, Task<string?>> StageLauncherUpdate,
    IAssetStore Assets,
    string ManifestCachePath,
    GpuInfo? Gpu = null,
    string? ArtCacheRoot = null,
    IArtFetcher? ArtFetcher = null,
    IReadOnlyList<IGameScanner>? Scanners = null);

public sealed class MainViewModel : INotifyPropertyChanged
{
    /// <summary>What an install falls back to if the saved choice is somehow missing. The
    /// chooser runs before anything can be installed, so this is a backstop, not a default
    /// anyone sees.</summary>
    private static readonly InstallChoice DefaultChoice = new("amdnr", IncludeRuntime: true);

    // Not a bare HttpClient: the host the forwarder lives on drops requests with no User-Agent.
    private readonly HttpClient _http = LauncherHttp.Create(Services.SelfUpdater.CurrentVersion);
    private readonly StartupEnvironment _environment;
    private readonly GpuInfo _gpu;
    private readonly List<string> _startNotes = [];
    private LauncherService? _service;
    private InstallChoice? _downloadAfterScan;
    private BuildChooserViewModel? _chooser;
    private string? _steamRoot;
    private Artwork? _artwork;
    private string _statusMessage = "Starting…";
    private double _progress;
    private bool _busy;
    private GameRowViewModel? _selected;

    public ObservableCollection<GameRowViewModel> Games { get; } = [];

    public event PropertyChangedEventHandler? PropertyChanged;

    public MainViewModel() : this(Settings.Load()) { }

    public MainViewModel(Settings settings) : this(settings, null) { }

    internal MainViewModel(Settings settings, StartupEnvironment? environment)
    {
        Settings = settings;
        _environment = environment ?? new StartupEnvironment(
            FetchManifest: (url, ct) => _http.GetStringAsync(url, ct),
            StageLauncherUpdate: (launcher, ct) => new Services.SelfUpdater(_http)
                .DownloadIfNewerAsync(launcher, Services.SelfUpdater.CurrentVersion, ct),
            Assets: new AssetStore(settings.AssetRoot, new ResumableDownloader(_http)),
            ManifestCachePath: DefaultManifestCachePath,
            ArtFetcher: new HttpArtFetcher(_http));

        // Once, here: the registry read is cheap and never throws, and everything that follows
        // — the runtime a build gets, the chooser's note, the report — must agree on one answer.
        _gpu = _environment.Gpu ?? GpuProbe.Detect();

        // INSTALL puts in the build chosen in Settings; REPAIR / UPDATE puts back the one the
        // record names. That is the difference between the two buttons, and it is what lets a
        // user who changed builds keep their installed games as they are.
        InstallCommand     = new RelayCommand(_ => InstallAsync(keepInstalledBuild: false),
                                              _ => Selected?.IsSupported == true && !Busy);
        RepairCommand      = new RelayCommand(_ => InstallAsync(keepInstalledBuild: true),
                                              _ => Selected?.IsInstalled == true && !Busy);
        // Not on a row Ricochet guards: moving the mod to another name there could make a DLL
        // that was not loading start loading. UNINSTALL stays, to take it back out.
        TryNextProxyCommand= new RelayCommand(_ => TryNextProxyAsync(),
                                              _ => Selected is { IsInstalled: true, AntiCheatBlock: null } && !Busy);
        // The stage's picker: the same move as TRY NEXT PROXY, to a name the user chose, so the
        // same guard. On a game with nothing installed it only sets the name INSTALL will use.
        ChooseProxyCommand = new RelayCommand(name => ChooseProxyAsync(name as string),
                                              _ => Selected is { IsResolved: true, AntiCheatBlock: null } && !Busy);
        UninstallCommand   = new RelayCommand(_ => UninstallAsync(),   _ => Selected?.IsInstalled == true && !Busy);
        CollectLogsCommand = new RelayCommand(_ => CollectLogsAsync(), _ => Selected?.IsInstalled == true && !Busy);
        RescanCommand      = new RelayCommand(_ => RescanAsync(),      _ => !Busy);
        ResetIniCommand = new RelayCommand(_ => ResetIniAsync(),
            _ => Selected?.IsInstalled == true && !Busy);
        SettingsCommand = new RelayCommand(_ => ChangeBuildAsync(), _ => !Busy && Chooser is null);
    }

    public Settings Settings { get; }

    /// <summary>The graphics card found at startup, and the generation the runtime is chosen for.</summary>
    public GpuInfo Gpu => _gpu;

    /// <summary>Shown in the caption: a tester must never mistake a run against their branch's
    /// manifest for what users are getting.</summary>
    public bool IsTestManifest => Settings.IsTestManifest;

    /// <summary>The build chooser while it is open, over the art; null otherwise.</summary>
    public BuildChooserViewModel? Chooser
    {
        get => _chooser;
        private set { _chooser = value; Raise(); SettingsCommand.RaiseCanExecuteChanged(); }
    }

    public RelayCommand InstallCommand { get; }
    public RelayCommand RepairCommand { get; }
    public RelayCommand TryNextProxyCommand { get; }
    public RelayCommand ChooseProxyCommand { get; }
    public RelayCommand UninstallCommand { get; }
    public RelayCommand CollectLogsCommand { get; }
    public RelayCommand RescanCommand { get; }
    public RelayCommand ResetIniCommand { get; }
    public RelayCommand SettingsCommand { get; }

    public bool NeedsElevation =>
        Selected?.Entry.Gate?.Findings.Any(f => f.Code == GateCode.NotWritable) == true;

    public GameRowViewModel? Selected
    {
        get => _selected;
        set { _selected = value; Raise(); SelectionChanged(); }
    }

    public string StatusMessage { get => _statusMessage; private set { _statusMessage = value; Raise(); } }
    public double Progress      { get => _progress;      private set { _progress = value;      Raise(); } }
    /// <summary>The setter is internal so a test can hold the launcher busy without running a
    /// real install into a real game folder.</summary>
    public bool Busy
    {
        get => _busy;
        internal set
        {
            _busy = value;
            Raise();
            RaiseCommands();
            Raise(nameof(CanApplyUpdate));
            Raise(nameof(CanRestartElevated));
            Raise(nameof(UpdateBanner));
        }
    }

    private string? _pendingUpdatePath;
    public string? PendingUpdatePath
    {
        get => _pendingUpdatePath;
        private set { _pendingUpdatePath = value; Raise(); Raise(nameof(CanApplyUpdate)); }
    }

    /// <summary>The manifest's launcher block behind PendingUpdatePath: its hash is checked again
    /// right before the swap, and its URL is where the user is sent when the swap cannot run.</summary>
    public LauncherInfo? PendingLauncher { get; private set; }

    /// <summary>False while anything else is running. Applying ends in Application.Shutdown,
    /// and the install, uninstall and proxy switch all await file work in a game folder: a
    /// process that exits part-way through a copy leaves our proxy placed, the game's own
    /// files in AMDNR_backup, and no install record for uninstall to restore them from.</summary>
    public bool CanApplyUpdate => PendingUpdatePath is not null && !Busy;

    /// <summary>False while anything else is running, for the same reason as CanApplyUpdate:
    /// restarting ends in Application.Shutdown. The button follows the selected game, so it
    /// can be on screen for an unwritable game while an install copies into another.</summary>
    public bool CanRestartElevated => !Busy;

    /// <summary>The button goes grey while the launcher is busy; this says why, so a greyed
    /// button does not read as a broken one.</summary>
    public string UpdateBanner => Busy
        ? "A new version of the launcher has been downloaded. It can be installed once the " +
          "current task finishes."
        : "A new version of the launcher has been downloaded.";

    /// <summary>The launcher block goes first: the banner appears when the path does, and a
    /// click on it needs both.</summary>
    internal void OfferUpdate(string stagedExe, LauncherInfo launcher)
    {
        PendingLauncher = launcher;
        PendingUpdatePath = stagedExe;
    }

    /// <summary>Returns what to tell the user, or null when there is nothing to say.</summary>
    public string? ApplyUpdate(Func<string, LauncherInfo, string?> swapAndRestart)
    {
        if (PendingUpdatePath is not { } path || PendingLauncher is not { } update) return null;

        // The button is bound to CanApplyUpdate, so this is the second line of defence, not
        // the first: nothing but a stale click queued behind the one that started an install
        // should ever get here.
        if (Busy)
        {
            return "The launcher is still busy with an install, an uninstall or a scan. Wait " +
                   "for it to finish, then click RESTART TO UPDATE again.";
        }

        return swapAndRestart(path, update);
    }

    /// <summary>Manifest, launcher update, the user's choice of build, the scan, then the
    /// chosen build's packages. The scan is local and takes seconds; the downloads can take
    /// the better part of a gigabyte, so they run last, under a library that is already
    /// listed — downloaded first, they left the window saying nothing had been found for as
    /// long as they took. Busy the whole way: INSTALL waits for the files it needs.</summary>
    public async Task StartAsync()
    {
        Busy = true;
        try
        {
            var service = await PrepareAsync();

            await ScanIntoGamesAsync();

            if (_downloadAfterScan is { } choice)
            {
                _downloadAfterScan = null;
                await DownloadAsync(service, choice);
            }

            StatusMessage = _startNotes.Count > 0
                ? $"{string.Join(" ", _startNotes)} {Games.Count} games found."
                : $"{Games.Count} games found.";
        }
        // Every way fetching or reading the manifest can fail lands here: a status message, not
        // a crash. UriFormatException, NotSupportedException and InvalidOperationException
        // cover an address HttpClient cannot use — a --manifest typo is exactly that —
        // TaskCanceledException a timeout, JsonException and FormatException a bad document.
        catch (Exception e) when (e is HttpRequestException
                                    or TaskCanceledException
                                    or UriFormatException
                                    or NotSupportedException
                                    or InvalidOperationException
                                    or JsonException
                                    or FormatException
                                    or IOException
                                    or UnauthorizedAccessException
                                    or UnsupportedManifestException)
        {
            StatusMessage = $"Could not load the update manifest: {e.Message}";
        }
        finally
        {
            Busy = false;
            Progress = 0;   // a failed run must not leave the bar frozen part-way
        }
    }

    /// <summary>StartAsync up to the scan, in the order the steps must run: fetch, stage a
    /// launcher update, parse, cache, ask. The update goes before the parse because a manifest
    /// this launcher cannot read — a newer schema — is exactly the one whose launcher update
    /// is the way out. The downloads themselves are left to StartAsync, for after the scan;
    /// this only notes which build's packages it is to fetch. The service is kept here rather
    /// than by StartAsync, so a test can install through it without running the scan.</summary>
    internal async Task<LauncherService> PrepareAsync()
    {
        _startNotes.Clear();
        StatusMessage = "Checking for updates…";

        var (json, fetched) = await FetchManifestAsync();

        if (fetched && ManifestParser.TryReadLauncher(json) is { } launcher)
            await StageLauncherUpdateAsync(launcher);

        Manifest manifest;
        try
        {
            manifest = ManifestParser.Parse(json);

            // Only now. A document that does not parse, cached, would be the only thing every
            // offline start had to go on — and it can never be read.
            if (fetched) CacheManifest(json);
        }
        catch (Exception e) when (fetched && e is JsonException
                                                 or FormatException
                                                 or UnsupportedManifestException)
        {
            // A captive portal's login page, or a manifest for a newer launcher. The last good
            // one still describes packages that exist, so the launcher stays usable while any
            // update staged above waits for a restart.
            if (ReadCachedManifest() is not { } cached) throw;

            manifest = ManifestParser.Parse(cached);
            fetched = false;
            _startNotes.Add("The update manifest could not be read, so the last downloaded one is in use.");
        }

        // Read once, here: the rows look for each game's pictures after the scan, and looking
        // Steam up again per row would mean a registry read for every game in the library. The
        // one Artwork is shared by every row so the CDN is asked once per game, not once per row.
        _steamRoot = SteamScanner.FindSteamRoot();
        _artwork = new Artwork(_environment.ArtCacheRoot ?? Artwork.DefaultCacheRoot, _steamRoot, _environment.ArtFetcher);

        // Every launcher a PC is likely to have. Steam, Epic and the Xbox app keep their own
        // lists; the rest write their games into the registry, read through one reader.
        var registry = new WindowsRegistryReader();
        var service = new LauncherService(
            manifest, _environment.Assets, new Installer(_environment.Assets, new CopyFilePlacer()),
            _environment.Scanners ??
            [
                new SteamScanner(_steamRoot ?? ""),
                new EpicScanner(EpicScanner.DefaultManifestsDirectory),
                new XboxScanner(),
                new UbisoftScanner(registry),
                new EaScanner(registry),
                new GogScanner(registry),
                new RockstarScanner(registry),
                new BattleNetScanner(registry),
                new AmazonScanner(registry),
            ],
            _gpu);

        // A saved choice naming a build the manifest has since dropped is no choice at all.
        var choice = Settings.Choice is { } saved && manifest.TrySource(saved.SourceId) is not null
            ? saved
            : await AskForChoiceAsync(manifest);

        // Offline there is nothing to fetch with; INSTALL fetches what is missing once the
        // connection is back. Online, the fetch waits for the scan.
        _downloadAfterScan = fetched ? choice : null;

        // Before the scan, so every row's Doctor measures against the runtime the user picked.
        service.PreferredRuntimeId = choice.RuntimeId;

        _service = service;
        return service;
    }

    /// <summary>The service PrepareAsync built, for a test that has run StartAsync.</summary>
    internal LauncherService? Service => _service;

    /// <summary>The manifest's text, and whether it came from the network just now. Offline,
    /// the last one that parsed is used instead.</summary>
    private async Task<(string Json, bool Fetched)> FetchManifestAsync()
    {
        try
        {
            return (await _environment.FetchManifest(Settings.ManifestUrl, default), true);
        }
        catch (Exception e) when (e is HttpRequestException
                                    or TaskCanceledException
                                    or UriFormatException
                                    or NotSupportedException
                                    or InvalidOperationException)
        {
            // With no cache there is genuinely nothing to work with, so let it surface.
            if (ReadCachedManifest() is not { } cached) throw;

            _startNotes.Add("Offline — using the last downloaded manifest.");
            return (cached, false);
        }
    }

    /// <summary>A failed launcher download must not cost the user the launcher they have: it
    /// is noted, and tried again on the next start.</summary>
    private async Task StageLauncherUpdateAsync(LauncherInfo launcher)
    {
        try
        {
            if (await _environment.StageLauncherUpdate(launcher, default) is not { } staged) return;

            OfferUpdate(staged, launcher);
            _startNotes.Add($"Launcher {launcher.Version} is ready to install.");
        }
        // HashMismatchException derives from Exception directly, so nothing else here would
        // catch a truncated download.
        catch (Exception e) when (e is HttpRequestException
                                    or TaskCanceledException
                                    or UriFormatException
                                    or NotSupportedException
                                    or InvalidOperationException
                                    or HashMismatchException
                                    or IOException
                                    or UnauthorizedAccessException)
        {
            _startNotes.Add($"Launcher {launcher.Version} could not be downloaded ({e.Message}).");
        }
    }

    /// <summary>The first-run chooser, awaited. The answer is saved before anything is
    /// downloaded for it, so a download interrupted part-way resumes with the same build.</summary>
    private async Task<InstallChoice> AskForChoiceAsync(Manifest manifest)
    {
        var chooser = new BuildChooserViewModel(manifest, Settings.Choice, isFirstRun: true, _gpu);
        Chooser = chooser;
        StatusMessage = "Choose a build to continue.";

        InstallChoice choice;
        try
        {
            // Only Settings can be dismissed without an answer, so the fallback never applies.
            choice = await chooser.Answer ?? DefaultChoice;
        }
        finally
        {
            Chooser = null;
        }

        Settings.Choice = choice;
        if (!Settings.Save())
            _startNotes.Add("Your choice could not be saved, so the launcher will ask again next time.");

        return choice;
    }

    /// <summary>Exactly what an install of this choice will ask for, and nothing else. A
    /// failure is noted rather than thrown: the library is still worth showing, and INSTALL
    /// fetches whatever is missing.</summary>
    private async Task DownloadAsync(LauncherService service, InstallChoice choice)
    {
        try
        {
            foreach (var package in service.PackagesFor(choice))
            {
                if (_environment.Assets.Has(package)) continue;

                StatusMessage = $"Downloading {Label(service.Manifest, package)} {package.Version}…";
                await _environment.Assets.EnsureAsync(package, new Progress<DownloadProgress>(p =>
                    Progress = p.TotalBytes > 0 ? 100.0 * p.BytesReceived / p.TotalBytes : 0), default);
            }
        }
        // InvalidDataException is what ZipFile.ExtractToDirectory throws for an archive it
        // cannot open; it derives from SystemException, not IOException. InvalidOperationException
        // is the service saying the manifest does not list a package the build names.
        catch (Exception e) when (e is HttpRequestException
                                    or TaskCanceledException
                                    or UriFormatException
                                    or NotSupportedException
                                    or InvalidOperationException
                                    or HashMismatchException
                                    or IOException
                                    or InvalidDataException
                                    or UnauthorizedAccessException)
        {
            _startNotes.Add($"A download did not finish ({e.Message}). INSTALL will try again.");
        }
        finally
        {
            Progress = 0;
        }
    }

    /// <summary>What the status line calls a package while it downloads. Never the raw id: a
    /// stranger's first run must not read "Downloading runtime-033 0.3.3…". A package is the
    /// runtime when any offered build lists it among its runtimes, or when it is the one
    /// runtime package every older manifest had; the forwarder when a build names it as its
    /// forwarder; otherwise it is a build, called by the build's name.</summary>
    private static string Label(Manifest manifest, PackageInfo package)
    {
        var sources = manifest.OfferedSources;
        bool Is(string? id) => string.Equals(id, package.Id, StringComparison.OrdinalIgnoreCase);

        if (Is(SourceInfo.LegacyRuntimePackage) || sources.Any(s => s.Runtimes?.Any(r => Is(r.Package)) == true))
            return "the DLSSNR AMD files";
        if (Is("forwarder") || sources.Any(s => Is(s.Forwarder)))
            return "the DLSSNR forwarder";
        return sources.FirstOrDefault(s => Is(s.Package))?.Name ?? package.Id;
    }

    /// <summary>SETTINGS: the same chooser, dismissible, with ABOUT beside it. A new choice is
    /// saved and its packages fetched now, so the next INSTALL does not stop to download.</summary>
    private async Task ChangeBuildAsync()
    {
        if (Chooser is not null) return;

        var chooser = new BuildChooserViewModel(_service?.Manifest, Settings.Choice, isFirstRun: false, _gpu);
        Chooser = chooser;

        InstallChoice? choice;
        try
        {
            choice = await chooser.Answer;
        }
        finally
        {
            Chooser = null;
        }

        if (choice is null || choice == Settings.Choice || _service is not { } service) return;

        Settings.Choice = choice;
        var saved = Settings.Save();
        var name = service.Manifest.TrySource(choice.SourceId)?.Name ?? choice.SourceId;
        service.PreferredRuntimeId = choice.RuntimeId;

        Busy = true;
        try
        {
            _startNotes.Clear();
            await DownloadAsync(service, choice);

            // The rows are re-read: a runtime pick changes which installed games the Doctor
            // now calls up to date, and the stage must not keep saying yesterday's answer.
            await ScanIntoGamesAsync();

            _startNotes.Insert(0, saved
                ? $"{name} will be used for the next install. Installed games keep their build."
                : $"{name} will be used for the next install, but the choice could not be saved, " +
                  "so the launcher will ask again next time.");
            StatusMessage = string.Join(" ", _startNotes);
        }
        finally
        {
            Busy = false;
        }
    }

    /// <summary>Rescan from the button. LauncherService.Describe runs InstallGate.Evaluate for
    /// every candidate, which enumerates every process on the machine and touches MainModule on
    /// each — a call that throws Win32Exception for most protected processes, so it costs
    /// hundreds of thrown exceptions per game — plus a write probe created and deleted in every
    /// game folder, an anti-cheat sweep, and a 26 MB proxy hash for the installed ones. On a
    /// 150-title Steam library, run on the UI thread, that is a "Not Responding" window. Spec
    /// §7: all steps run off the UI thread.</summary>
    public async Task RescanAsync()
    {
        if (_service is null) return;

        Busy = true;
        try
        {
            await ScanIntoGamesAsync();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            // Awaited from an async void command, so an escaping exception kills the process.
            StatusMessage = $"The scan could not finish: {e.Message}";
        }
        finally { Busy = false; }
    }

    /// <summary>Busy is the caller's to set: StartAsync has its own run in progress and must not
    /// have it cleared half-way through by a rescan nested inside it.</summary>
    private async Task ScanIntoGamesAsync()
    {
        if (_service is null) return;

        StatusMessage = "Scanning for games…";

        var service = _service;
        ReplaceGames(await Task.Run(() => service.Scan()));

        StatusMessage = $"{Games.Count} games found.";
    }

    /// <summary>Refills the list on the UI thread, which is the only thread an
    /// ObservableCollection bound to a view may be touched from, and puts the selection back on
    /// the row that replaced the one the user had. Without that the selected row leaves the
    /// collection, the list view pushes null back through the two-way binding, and the detail
    /// panel the user was reading goes blank on every rescan — including the one that runs
    /// straight after an install.</summary>
    internal void ReplaceGames(IReadOnlyList<GameEntry> entries)
    {
        var wasSelected = Selected?.Entry.Candidate.InstallRoot;

        // The DLL name picked for a game not installed yet lives on its row, so it would go
        // with the row — on every rescan, including the one straight after installing some
        // other game. Kept by install root like the selection; a row with a record ignores it.
        var picked = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var row in Games)
        {
            if (row.ChosenProxy is { } name) picked.TryAdd(row.Entry.Candidate.InstallRoot, name);
        }

        Games.Clear();
        foreach (var entry in entries)
        {
            var row = new GameRowViewModel(entry, _artwork);
            if (picked.TryGetValue(entry.Candidate.InstallRoot, out var name)) row.ChosenProxy = name;
            Games.Add(row);
        }

        // Matched on the install root rather than the name: it is what Scan() itself
        // de-duplicates on, so it is the one value that identifies a row across a rescan.
        Selected = wasSelected is null
            ? null
            : Games.FirstOrDefault(g => string.Equals(
                g.Entry.Candidate.InstallRoot, wasSelected, StringComparison.OrdinalIgnoreCase));
    }

    public void AddManual(string exeDirectory)
    {
        if (_service is null) return;

        Games.Add(new GameRowViewModel(_service.AddManual(exeDirectory), _artwork));
    }

    /// <summary>Set by the view: shows the anti-cheat warning and returns the user's answer.</summary>
    public Func<string, bool> ConfirmAntiCheat { get; set; } = _ => false;

    /// <summary>Set by the view: says what closing now would cut short and returns whether to
    /// close anyway.</summary>
    public Func<string, bool> ConfirmCloseMidChange { get; set; } = _ => false;

    /// <summary>The game whose folder an install, uninstall or proxy switch is changing right
    /// now, or null. Narrower than Busy on purpose: the first-run chooser, a scan and a
    /// download are Busy too, and all three are safe to stop. The setter is internal so a test
    /// can hold this state without running a real install into a real game folder.</summary>
    internal string? ChangingGame { get; set; }

    /// <summary>Asked by the window before it closes. Those three Core calls run on the pool,
    /// so the window answers its close button mid-copy, and Application.Shutdown then ends the
    /// copy wherever it has got to: our proxy placed, the game's own DLLs in AMDNR_backup, and
    /// a record listing none of the files uninstall would have to take out. Asked rather than
    /// refused, because a download that has stalled must not leave Task Manager as the only
    /// way out.</summary>
    public bool MayClose()
        => ChangingGame is not { } game
           || ConfirmCloseMidChange(
               $"{Brand.Name} is still changing {game}'s files. Closing now can stop it " +
               "part-way, with some AMDNR files in the game folder and the game's own DLLs " +
               "still in AMDNR_backup.");

    private static string DefaultManifestCachePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "AMDNR", "manifest-cache.json");

    /// <summary>Best effort. Failing to cache must not fail the run that just succeeded. A test
    /// manifest is never cached: the next ordinary start that found itself offline would read
    /// it, with no TEST MANIFEST label to say so.</summary>
    private void CacheManifest(string json)
    {
        if (Settings.IsTestManifest) return;

        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_environment.ManifestCachePath)!);
            File.WriteAllText(_environment.ManifestCachePath, json);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    /// <summary>Never for a test manifest: the cache holds the real one, and falling back to it
    /// would test the wrong thing without saying so.</summary>
    private string? ReadCachedManifest()
    {
        if (Settings.IsTestManifest) return null;

        try
        {
            return File.Exists(_environment.ManifestCachePath)
                ? File.ReadAllText(_environment.ManifestCachePath)
                : null;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { return null; }
    }

    internal async Task InstallAsync(bool keepInstalledBuild)
    {
        if (_service is not { } service || Selected is not { } row) return;

        Busy = true;
        try
        {
            var entry = row.Entry;

            // No prompt for this one: its Yes could only end in Core refusing, after the user had
            // been asked to accept a ban risk for nothing.
            if (row.AntiCheatBlock is { } block)
            {
                StatusMessage = block;
                return;
            }

            // The same words the stage shows, so the prompt and the line under it never
            // disagree about what the risk is.
            var warning = row.AntiCheatWarning;
            if (warning is not null && !ConfirmAntiCheat(warning))
            {
                StatusMessage = "Install cancelled.";
                return;
            }

            // Consent only when the prompt was actually shown and answered. The warning comes
            // from the last scan; a game update since then can have brought anti-cheat in, and
            // Core runs the gate again. Passing true for a prompt nobody saw would switch that
            // check off and record a consent the user never gave.
            var acknowledged = warning is not null;

            StatusMessage = "Installing…";

            // IProgress<T>.Report posts asynchronously, so a callback queued by the last file
            // can arrive AFTER the final status is set and overwrite it — leaving the user
            // looking at "Installing files (3/3)" on an install that has actually finished.
            // Both this flag and the callback run on the UI thread, so clearing it before
            // writing the final message is enough.
            var installing = true;

            var chosen = Settings.Choice ?? DefaultChoice;
            var choice = keepInstalledBuild ? LauncherService.ChoiceFor(entry, chosen) : chosen;

            // Built here, on the UI thread, so its reports are posted back to it from the pool.
            var proxy = row.Proxy;
            var progress = new Progress<InstallProgress>(p =>
            {
                if (!installing) return;
                Progress = p.FilesTotal > 0 ? 100.0 * p.FilesDone / p.FilesTotal : 0;
                StatusMessage = $"{p.Stage} ({p.FilesDone}/{p.FilesTotal})";
            });

            // The row is the one captured above, because the user can pick another game
            // while the copy runs.
            row.Entry = await ChangeGameFolderAsync(row, () =>
                service.InstallAsync(entry, proxy, choice, acknowledged, progress, default));

            installing = false;
            row.InstalledHere = true;
            SelectionChanged();

            StatusMessage = row.HasProblem
                ? "Installed, but Doctor found problems."
                : "Installed.";
        }
        // Offline with an uncached package: the install reaches a real download and fails on
        // the network. Nothing gates the Install button on cache state, so this is ordinary,
        // and a raw exception string would not tell the user what to do about it.
        catch (Exception e) when (e is HttpRequestException or TaskCanceledException)
        {
            StatusMessage = "This game's files have not been downloaded yet, and there is no " +
                            "connection to fetch them. Reconnect and try again.";
        }
        // InvalidDataException is what ZipFile.ExtractToDirectory throws for a package archive
        // it cannot open. It derives from SystemException, so IOException does not cover it.
        // UriFormatException is the downloader's HttpRequestMessage refusing a malformed package
        // URL from a stale cached manifest; AssetStore.PathFor says InvalidOperationException
        // for a URL it cannot store. Either would crash the Install button without a catch.
        catch (Exception e) when (e is InvalidOperationException
                                    or IOException
                                    or InvalidDataException
                                    or UriFormatException
                                    or UnauthorizedAccessException
                                    or HashMismatchException)
        {
            StatusMessage = e.Message;
        }
        finally
        {
            Busy = false;
            Progress = 0;
        }
    }

    internal async Task TryNextProxyAsync()
    {
        if (_service is not { } service || Selected is not { } row) return;

        // The button is off for this row; Core refuses it as well, whatever the row says.
        if (row.AntiCheatBlock is { } block)
        {
            StatusMessage = block;
            return;
        }

        Busy = true;
        try
        {
            // The window keeps answering while this runs, so without a word here the pulsing
            // dot and the greyed buttons would be the only sign that anything is happening.
            StatusMessage = "Switching to the next proxy DLL…";

            var entry = row.Entry;
            var outcome = await ChangeGameFolderAsync(row, () => service.TryNextProxyAsync(entry, default));
            row.Entry = outcome.Entry;
            SelectionChanged();

            // Once every name has been tried the round starts again rather than stopping, and
            // that has to be said: the DLL has just moved to a name the user saw fail before.
            StatusMessage = outcome.StartedOver
                ? $"Every name has been tried once; starting again from {row.Proxy}. Start the game, then press INSERT."
                : $"Switched to {row.Proxy}. Start the game, then press INSERT.";
        }
        // File.Move on the proxy DLL is what this does, and the documented workflow has the
        // user launching the game between attempts — a locked file here is routine, not exotic.
        catch (Exception e) when (e is InvalidOperationException
                                    or IOException
                                    or UnauthorizedAccessException)
        {
            StatusMessage = e.Message;
        }
        finally { Busy = false; }
    }

    /// <summary>The stage's picker. On an installed game the DLL moves to the chosen name under
    /// TRY NEXT PROXY's guards; on a game with nothing installed the choice is the name INSTALL
    /// will use. The name already in use is nothing to do, and a name the launcher does not
    /// install under is ignored: nothing on the picker offers one, but a command parameter is
    /// only a string.</summary>
    internal async Task ChooseProxyAsync(string? name)
    {
        if (_service is not { } service || Selected is not { } row || name is null) return;
        if (!PayloadNames.IsProxyName(name)) return;
        if (string.Equals(name, row.Proxy, StringComparison.OrdinalIgnoreCase)) return;

        // The picker is off for this row; Core refuses it as well, whatever the row says.
        if (row.AntiCheatBlock is { } block)
        {
            StatusMessage = block;
            return;
        }

        if (!row.IsInstalled)
        {
            row.ChosenProxy = name;
            SelectionChanged();
            StatusMessage = $"INSTALL will use {name}.";
            return;
        }

        Busy = true;
        try
        {
            StatusMessage = $"Switching to {name}…";

            var entry = row.Entry;
            row.Entry = await ChangeGameFolderAsync(row, () => service.SwitchProxyAsync(entry, name, default));
            StatusMessage = $"Switched to {row.Proxy}. Start the game, then press INSERT.";
        }
        catch (Exception e) when (e is InvalidOperationException
                                    or IOException
                                    or UnauthorizedAccessException)
        {
            StatusMessage = e.Message;

            // The radio the user clicked marked itself. Re-reading the row rebuilds the picker
            // around the name still in place, with the one just refused greyed where it is taken.
            await ReadBackAsync(service, row);
        }
        finally
        {
            SelectionChanged();
            Busy = false;
        }
    }

    /// <summary>Puts the row back in step with the game folder after a change was refused.
    /// Off the UI thread like every other look at a game folder; when even that fails, the
    /// row is re-raised as it is, which is enough to rebuild the picker.</summary>
    private static async Task ReadBackAsync(LauncherService service, GameRowViewModel row)
    {
        try
        {
            row.Entry = await Task.Run(() => service.Refresh(row.Entry));
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            row.Entry = row.Entry;
        }
    }

    internal async Task UninstallAsync()
    {
        if (_service is not { } service || Selected is not { } row) return;

        Busy = true;
        try
        {
            StatusMessage = "Uninstalling…";

            var entry = row.Entry;
            var outcome = await ChangeGameFolderAsync(row, () => service.UninstallAsync(entry, default));
            row.Entry = outcome.Entry;
            SelectionChanged();

            // The record's folder, because that is what the result's paths are relative to.
            StatusMessage = UninstallMessage(outcome.Result, entry.Record?.ExeDirectory ?? entry.ExeDirectory);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            StatusMessage = e.Message;
        }
        finally { Busy = false; }
    }

    /// <summary>Off the UI thread: each of these hashes or moves files in the game folder and
    /// then re-runs the install gate, which walks every process on the machine. Awaited on the
    /// UI thread, that is a window that stops painting until it is done — and because it is
    /// not, the window has to ask before it closes on one part-way through.</summary>
    private async Task<T> ChangeGameFolderAsync<T>(GameRowViewModel row, Func<Task<T>> change)
    {
        ChangingGame = row.Name;
        try
        {
            return await Task.Run(change);
        }
        finally
        {
            ChangingGame = null;
        }
    }

    /// <summary>A partial uninstall must not read as a clean one. The usual cause is the game
    /// being launched between running Doctor and pressing this button, and naming the files is
    /// the difference between "try again with the game closed" and confusion — but only for
    /// files something holds. One the user changed was kept on purpose, and sending them to
    /// close the game over it would be advice that can never work. Anything waiting in a
    /// backup is named with its full folder: on an Unreal game the backup sits under
    /// Binaries\Win64, not in the game's root where the user will look first.</summary>
    internal static string UninstallMessage(UninstallResult result, string? exeDirectory)
    {
        string Full(string path) => exeDirectory is null ? path : Path.Combine(exeDirectory, path);

        string Folders(IEnumerable<string> paths) => string.Join(", ", paths
            .Select(p => Path.GetDirectoryName(Full(p)) ?? p)
            .Distinct(StringComparer.OrdinalIgnoreCase));

        // Core names a backup it could not even read by the folder itself, AMDNR_backup\<stamp>,
        // and a file waiting in one it could read by its path inside it. Nothing the running
        // game holds keeps a backup unreadable, so "close the game" would be advice that cannot
        // work — and the stamp is not a file anybody can look for.
        static bool IsBackupFolder(string path) => string.Equals(
            Path.GetFileName(Path.GetDirectoryName(path)), "AMDNR_backup", StringComparison.OrdinalIgnoreCase);

        var unread = result.NotRestored.Where(IsBackupFolder).ToList();
        var waiting = result.NotRestored.Where(p => !IsBackupFolder(p)).ToList();

        var unreadNote = unread.Count == 0
            ? null
            : unread.Count == 1
                ? $"backup of your own files in {Full(unread[0])} could not be read, so they are still " +
                  "there. Once that folder can be opened, press UNINSTALL again."
                : $"backups of your own files in {string.Join(", ", unread.Select(Full))} could not be " +
                  "read, so they are still there. Once those folders can be opened, press UNINSTALL again.";

        // A kept record with nothing of ours left in use means a backup still waits to go back.
        // "Uninstalled." would hide that, and the row it leaves offers Repair beside Uninstall.
        List<string> parts =
        [
            result.LeftBehind.Count > 0
                ? "Uninstalled, but these are in use and could not be removed — close the game and " +
                  $"press UNINSTALL again: {string.Join(", ", result.LeftBehind)}."
                : unreadNote is not null
                    ? $"Not finished: the {unreadNote}"
                    : result.CanFinishLater
                        ? "Not finished: close the game and press UNINSTALL again to put your own files back."
                        : "Uninstalled.",
        ];

        if (result.LeftBehind.Count > 0 && unreadNote is not null)
            parts.Add($"The {unreadNote}");

        if (result.Changed.Count > 0)
            parts.Add($"Kept because they were changed after the install: {string.Join(", ", result.Changed)}.");

        if (waiting.Count > 0)
        {
            var names = waiting.Select(Path.GetFileName).ToList();
            parts.Add($"Your own {string.Join(", ", names)} could not be put back, and " +
                      $"{(names.Count == 1 ? "is" : "are")} still in {Folders(waiting)}.");
        }

        if (result.Superseded.Count > 0)
            parts.Add($"Your earlier OptiScaler build was kept, switched off, in {Folders(result.Superseded)}.");

        return string.Join(" ", parts);
    }

    private async Task CollectLogsAsync()
    {
        if (Selected?.Entry.Record is null || Selected.Entry.Report is null) return;

        var zipPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.Desktop),
            $"amdnr-report-{DateTime.Now:yyyyMMdd-HHmmss}.zip");

        var record = Selected.Entry.Record;
        var report = Selected.Entry.Report;
        var gpu = _gpu;

        Busy = true;
        try
        {
            StatusMessage = "Collecting logs…";

            // Zipping a game folder's logs is disk work, and the game may be holding them open
            // through a shared handle, so it can take a visible moment. The clipboard write
            // that follows is not: it needs the UI thread, and awaiting brings us back to it.
            var summary = await Task.Run(() => LogCollector.CreateSummary(record, report, gpu));
            var written = await Task.Run(() => LogCollector.CreateReportZip(record, report, gpu, zipPath));

            // The two artefacts fail independently. Neither may take the other down, and
            // neither may crash the app — RelayCommand.Execute is async void, so an escaping
            // exception kills the process rather than showing a message.
            var copied = TryCopyToClipboard(summary);

            StatusMessage = (written, copied) switch
            {
                (not null, true) => $"Report saved to {written}. Summary copied to the clipboard.",
                (not null, false) => $"Report saved to {written}. The clipboard was unavailable.",
                (null, true) => "Summary copied to the clipboard. The log archive could not be written.",
                _ => "Could not write the log archive or copy the summary.",
            };
        }
        finally { Busy = false; }
    }

    /// <summary>The Windows clipboard is a shared single-owner resource: any other process
    /// holding it makes SetText throw, which is common rather than rare.</summary>
    private static bool TryCopyToClipboard(string text)
    {
        try
        {
            System.Windows.Clipboard.SetText(text);
            return true;
        }
        catch (Exception e) when (e is ExternalException or InvalidOperationException)
        {
            return false;
        }
    }

    internal async Task ResetIniAsync()
    {
        if (_service is not { } service || Selected is not { } row || row.Entry.ExeDirectory is null) return;

        Busy = true;
        try
        {
            // The row is the one captured here: the user can pick another game while this runs.
            var entry = row.Entry;

            // The service resets to the ini an install of this row's own build would seed —
            // which for TheAutomatic's build with the runtime is not its packaged file.
            // Null means there was no ini to preserve — do not promise a backup that
            // does not exist. Off the UI thread like every other change to a game folder: the
            // seed is staged beside the package, the ini written, and the row then re-read from
            // a folder that, on a sleeping drive, takes its time to answer.
            var backup = await ChangeGameFolderAsync(row, () => Task.FromResult(service.ResetIni(entry)));

            row.Entry = await Task.Run(() => service.Refresh(entry));
            SelectionChanged();
            StatusMessage = backup is null
                ? "OptiScaler.ini written from the packaged defaults."
                : $"OptiScaler.ini reset. Your previous file was kept as {Path.GetFileName(backup)}.";
        }
        // InvalidOperationException is the service saying, in words meant for the user, that
        // the manifest no longer offers this row's build or its package.
        catch (Exception e) when (e is IOException
                                    or UnauthorizedAccessException
                                    or InvalidOperationException)
        {
            StatusMessage = e.Message;
        }
        finally { Busy = false; }
    }

    /// <summary>Everything computed from the selected entry has to be re-raised by hand.
    /// GameRowViewModel raises its own PropertyChanged, but that is a different object and
    /// cannot reach a binding on the window's DataContext.</summary>
    private void SelectionChanged()
    {
        Raise(nameof(NeedsElevation));
        RaiseCommands();
    }

    private void RaiseCommands()
    {
        InstallCommand.RaiseCanExecuteChanged();
        RepairCommand.RaiseCanExecuteChanged();
        TryNextProxyCommand.RaiseCanExecuteChanged();
        ChooseProxyCommand.RaiseCanExecuteChanged();
        UninstallCommand.RaiseCanExecuteChanged();
        CollectLogsCommand.RaiseCanExecuteChanged();
        RescanCommand.RaiseCanExecuteChanged();
        ResetIniCommand.RaiseCanExecuteChanged();
        SettingsCommand.RaiseCanExecuteChanged();
    }

    private void Raise([CallerMemberName] string? name = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
