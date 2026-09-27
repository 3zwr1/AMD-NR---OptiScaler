// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.ComponentModel;
using System.Runtime.CompilerServices;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Diagnostics;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.ViewModels;

/// <summary>One of the six DLL names on the stage's picker. Selected is the name in use, or the
/// one INSTALL will use; a name some other file already holds is not enabled, and its tooltip
/// says which file and what to do about it. Immutable: the row rebuilds the list whenever the
/// entry or the choice changes, which is what lets a refused click snap back.</summary>
public sealed class ProxyOptionViewModel(string name, bool isSelected, bool isEnabled, string? tooltip)
{
    public string Name { get; } = name;
    public bool IsSelected { get; } = isSelected;
    public bool IsEnabled { get; } = isEnabled;
    public string? Tooltip { get; } = tooltip;
}

public sealed class GameRowViewModel : INotifyPropertyChanged
{
    private readonly Artwork? _artwork;
    private GameEntry _entry;
    private GameArt _art = GameArt.None;
    private string? _chosenProxy;
    private IReadOnlyList<ProxyOptionViewModel>? _proxyOptions;

    /// <summary>A row with no <paramref name="artwork"/> has nowhere to look for a picture and
    /// shows the plate with the initial: the rows a test builds, and the rows built before the
    /// launcher has read where Steam is.</summary>
    public GameRowViewModel(GameEntry entry, Artwork? artwork = null)
    {
        _entry = entry;
        _artwork = artwork;
        ArtLoaded = artwork is null ? Task.CompletedTask : LoadArtAsync();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public GameEntry Entry
    {
        get => _entry;
        set
        {
            var hadFolder = _entry.ExeDirectory is not null;
            _entry = value;
            _proxyOptions = null;
            RaiseAll();

            // A folder that has just been found — a manual add resolved, a rescan that now knows
            // the exe — can hold an icon the first look had no path to.
            if (_artwork is not null && !hadFolder && value.ExeDirectory is not null && _art.Tile is null)
                ArtLoaded = LoadArtAsync();
        }
    }

    public string Name => _entry.Candidate.Name;
    public string Store => GameStoreLabels.Label(_entry.Candidate.Store);
    public string Folder => _entry.ExeDirectory ?? "(executable folder not identified)";

    /// <summary>Bound by the row template's status pill, which picks its colours from the
    /// status itself. The existing IsInstalled/HasProblem booleans cannot express
    /// "unsupported" and "not installed" as two different looks.</summary>
    public GameStatus Status => _entry.Status;

    public string StatusText => _entry.Status switch
    {
        GameStatus.NotInstalled    => "Not installed",
        GameStatus.Installed       => $"Installed {_entry.Record?.AmdnrVersion}",
        GameStatus.UpdateAvailable => "Update available",
        GameStatus.Problem         => "Problem",
        GameStatus.Unsupported     => "Not supported",
        GameStatus.ExistingInstall => "Earlier install found",
        _                          => "",
    };

    public bool IsInstalled => _entry.Record is not null;
    public bool HasProblem => _entry.Status == GameStatus.Problem;
    public bool UpdateAvailable => _entry.Status == GameStatus.UpdateAvailable;

    /// <summary>Names the build REPAIR / UPDATE reinstalls, which is the one the record names —
    /// not necessarily the one Settings would pick for a new install. Saying AMDNR here for a
    /// game running TheAutomatic's build would promise a switch the button does not make.
    ///
    /// Never "newer": an update is offered whenever the manifest's version differs from the
    /// installed one, and a build the owner rolled back to differs too. Both versions are named
    /// instead, so the user can see which way it goes.
    ///
    /// The runtime has its own sentence when the manifest now lists another one first for this
    /// card — the way Daniel's next runtime reaches players without a new launcher — and the
    /// sentence says why the files are offered: the card, not a new build. One press brings
    /// whatever is named, so the closing line counts what it is.</summary>
    public string? UpdateNote
    {
        get
        {
            if (!UpdateAvailable) return null;

            var build = _entry.Report?.BuildName is { Length: > 0 } name ? name + " " : "";
            var mod = _entry.Report?.AvailableVersion is { } available && _entry.Record?.AmdnrVersion is { } installed
                ? $"{build}{available} is available; this game has {installed}."
                : null;
            var runtime = _entry.Report?.AvailableRuntimeVersion is { } wanted
                ? $"DLSSNR AMD files {wanted} are now the runtime for this card; this game has {_entry.Record?.RuntimeVersion}."
                : null;

            if (mod is null && runtime is null)
                return $"Another {build}build is available. Use REPAIR / UPDATE to install it.";

            var what = runtime is null ? "it" : "them";
            return string.Join(" ", new[] { mod, runtime }.Where(s => s is not null)) +
                   $" Use REPAIR / UPDATE to install {what}.";
        }
    }

    public bool IsSupported => _entry.Status != GameStatus.Unsupported;

    /// <summary>The name in use when something is installed; otherwise the name the user picked
    /// on the stage, or failing that the planner's recommendation. INSTALL reads this.</summary>
    public string Proxy => _entry.Record?.Proxy ?? _chosenProxy ?? _entry.RecommendedProxy;

    /// <summary>The name the user picked for INSTALL on a game with nothing installed yet. Kept
    /// on the row, not the entry: a rescan re-reads the entry and the choice is the user's, not
    /// the folder's — MainViewModel.ReplaceGames hands it to the row that replaces this one.
    /// Once a record exists the record's name wins, whatever this says.</summary>
    internal string? ChosenProxy
    {
        get => _chosenProxy;
        set
        {
            _chosenProxy = value;
            _proxyOptions = null;
            RaiseAll();
        }
    }

    /// <summary>The proxy name as the rail and the stage show it. With no exe folder there is
    /// nowhere to put a DLL, and printing the planner's first default would read as a decision
    /// the launcher has not made. A game the launcher refuses outright gets none either, unless
    /// something is already in place from before the refusal: "dxgi.dll" beside a Ricochet game
    /// would read as the file the launcher is about to put there.</summary>
    public string? DllName => IsResolved && (IsSupported || IsInstalled) ? Proxy : null;

    /// <summary>The six names as the stage's picker shows them, in the launcher's own order —
    /// not the planner's, so a name stays where the eye left it. The same list until the entry
    /// or the choice changes, so the bindings are not rebuilt on every read.</summary>
    public IReadOnlyList<ProxyOptionViewModel> ProxyOptions => _proxyOptions ??= BuildProxyOptions();

    private IReadOnlyList<ProxyOptionViewModel> BuildProxyOptions()
    {
        var current = Proxy;
        return PayloadNames.ProxyNames.Select(name =>
        {
            var isCurrent = string.Equals(name, current, StringComparison.OrdinalIgnoreCase);

            // The name in use is always the name in use, whatever the probe makes of the file
            // under it — a build without the version resource reads as "some other DLL".
            var taken = !isCurrent && _entry.OccupiedProxies.Contains(name, StringComparer.OrdinalIgnoreCase);

            // Occupied covers a file the probe could not open as well as one it read as some
            // other DLL, and the first may well be an OptiScaler an antivirus is holding. What
            // is true of both — and what the installer itself says when refusing the move — is
            // that this launcher did not put it there.
            return new ProxyOptionViewModel(name, isCurrent, !taken,
                taken
                    ? $"{name} is already in this game folder and was not put there by this launcher. Move it aside yourself to use this name."
                    : null);
        }).ToList();
    }

    /// <summary>Why the marked name is the marked name, under the picker, in the planner's own
    /// words with the name in front: "dxgi.dll — re9.exe loads dxgi.dll itself when it starts."
    /// When the user has picked another name for INSTALL, both are said, so the recommendation
    /// is still there to go back to. Nothing without an exe folder: there is nothing to explain.</summary>
    public string? ProxyReason
    {
        get
        {
            if (DllName is null || _entry.ProxyReason.Length == 0) return null;

            var picked = $"{_entry.RecommendedProxy} — {_entry.ProxyReason}.";
            var overruled = _chosenProxy is not null && !IsInstalled
                            && !string.Equals(_chosenProxy, _entry.RecommendedProxy, StringComparison.OrdinalIgnoreCase);

            return overruled
                ? $"INSTALL will use {_chosenProxy}. The launcher would have picked {picked}"
                : picked;
        }
    }

    /// <summary>Shown under the folder path: on an Unreal game the folder alone does not say
    /// whether the launcher found the real game or the launcher stub at its root.</summary>
    public string? ExeName => _entry.ExeName;

    /// <summary>Kept on the stage for as long as the gate reports anti-cheat, not only in the
    /// prompt: the risk is in playing online afterwards, long after the prompt is gone. An
    /// anti-cheat that blocks the install outright says so in its own words instead: it is the
    /// only explanation a "Not supported" row gets.</summary>
    public string? AntiCheatWarning =>
        AntiCheatBlock
        ?? (_entry.Gate?.Findings.FirstOrDefault(f => f.Code == GateCode.AntiCheatDetected) is { } finding
            ? $"{Name} uses {(finding.Products.Count > 0 ? string.Join(" and ", finding.Products) : "anti-cheat")}. " +
              "Mods like AMDNR can get your account banned in online play. Only continue if you " +
              "play offline, and uninstall before playing online."
            : null);

    /// <summary>Why nothing can be installed here, when an anti-cheat forbids it outright.</summary>
    public string? AntiCheatBlock =>
        _entry.Gate?.Findings.FirstOrDefault(f => f.Code == GateCode.AntiCheatBlocked)?.Message;

    /// <summary>Capcom's RE Engine without REFramework: the game closes itself soon after launch
    /// with a mod loaded. Kept on the stage like the anti-cheat line — the fix is a file the user
    /// adds, and nothing the launcher installs makes the line go away until they do.</summary>
    public string? EngineWarning =>
        _entry.Gate?.Findings.FirstOrDefault(f => f.Code == GateCode.ReframeworkMissing)?.Message;

    /// <summary>Says what INSTALL will do to somebody's own install before they press it. The
    /// name in the brackets is the one the old build loads through — the planner's first name
    /// for an earlier install — and not the one INSTALL will use: with another name picked on
    /// the stage, the mod goes in under that name and the old DLL is moved into AMDNR_backup,
    /// where UNINSTALL leaves it.</summary>
    public string? EarlierInstallNote
    {
        get
        {
            if (_entry.Status != GameStatus.ExistingInstall) return null;

            var found = _entry.RecommendedProxy;
            var keeps = _chosenProxy is null || string.Equals(_chosenProxy, found, StringComparison.OrdinalIgnoreCase);

            return keeps
                ? $"Found an earlier AMDNR / OptiScaler install ({found}). INSTALL updates it and keeps that DLL name."
                : $"Found an earlier AMDNR / OptiScaler install ({found}). INSTALL moves it into AMDNR_backup and installs as {_chosenProxy}.";
        }
    }

    private bool _installedHere;

    /// <summary>Set once an install from this window has finished. Not derived from the record:
    /// a game installed on an earlier day needs no reminder of how to open the menu.</summary>
    internal bool InstalledHere
    {
        get => _installedHere;
        set { _installedHere = value; Raise(nameof(NextSteps)); }
    }

    /// <summary>Follows the proxy, so after TRY NEXT PROXY it names the DLL now in place. Not
    /// shown on a row Doctor calls a problem: starting the game is not the next step there.</summary>
    public string? NextSteps => _installedHere && IsInstalled && !HasProblem
        ? $"Installed as {Proxy}. Start the game, then press INSERT to open the AMDNR menu. " +
          "Nothing changes? Use TRY NEXT PROXY."
        : null;

    /// <summary>The findings themselves rather than pre-formatted strings, so the view can
    /// colour each one by severity. Flattening them to "[Error] …" here would push that
    /// decision into a string the UI would have to parse back out.</summary>
    public IReadOnlyList<DoctorFinding> Findings => _entry.Report?.Findings ?? [];

    public bool HasFindings => Findings.Count > 0;

    /// <summary>The build stamp Doctor read out of the installed DLL. The user named this
    /// their number-one support problem: without it, "which build are you on?" costs a
    /// round trip on every report.</summary>
    public string BuildStamp => _entry.Report?.BuildStamp ?? "unknown";

    /// <summary>Drawn on the coloured plate while there is no tile at all — the moment before
    /// the pictures are found, and for good when none is.</summary>
    public string Initial => Name.Length > 0 ? Name[..1].ToUpperInvariant() : "?";

    // ------------------------------------------------------------------ a cover for every game

    /// <summary>Completes when the pictures on this PC have been looked for and, for a Steam
    /// game with something missing, the CDN has been asked. For tests; the view binds and waits
    /// for nothing — the row is drawn at once and again as each picture lands.</summary>
    internal Task ArtLoaded { get; private set; }

    /// <summary>Off the UI thread, both halves. The look on disk reads headers, stats the CDN's
    /// markers and may extract an icon, which on a slow drive is real time for a library of a
    /// hundred games; the CDN is the network, asked only for what that look did not find. Each
    /// result comes back to the thread the row was built on — the dispatcher in the app — and the
    /// bindings are told which pictures changed.</summary>
    private async Task LoadArtAsync()
    {
        var artwork = _artwork!;
        var candidate = _entry.Candidate;
        var exeDirectory = _entry.ExeDirectory;
        var exeName = _entry.ExeName;

        var lookup = await Task.Run(() => artwork.Lookup(candidate, exeDirectory, exeName));
        Art = lookup.Art;

        if (lookup.NeedsFetch && await artwork.FetchAsync(candidate, lookup, CancellationToken.None))
            Art = await Task.Run(() => artwork.Resolve(candidate, exeDirectory, exeName));
    }

    private GameArt Art
    {
        get => _art;
        set
        {
            if (value == _art) return;

            _art = value;
            foreach (var name in ArtProperties) Raise(name);
        }
    }

    private static readonly string[] ArtProperties =
    [
        nameof(PortraitPath), nameof(SquareTilePath), nameof(HasTile), nameof(HeroPath), nameof(LogoPath),
        nameof(HasLogo), nameof(HeroBlurPath), nameof(BackdropToBlurPath), nameof(StageBackdropPath),
    ];

    /// <summary>Box art at 2:3, cropped to the spine in the rail. Null is the ordinary case
    /// until the pictures are found: the view falls back to the coloured tile and the initial.</summary>
    public string? PortraitPath => _art.TileIsSquare ? null : _art.Tile;

    /// <summary>A square picture standing in for box art — the Xbox app's logo, or the exe's
    /// own icon — letterboxed on the plate rather than cropped: a cropped icon is nothing.</summary>
    public string? SquareTilePath => _art.TileIsSquare ? _art.Tile : null;

    /// <summary>Hides the initial: a letterboxed square would otherwise show it through the
    /// plate on either side.</summary>
    public bool HasTile => _art.Tile is not null;

    public string? HeroPath => _art.Hero;
    public string? LogoPath => _art.Logo;

    /// <summary>Steam's pre-blurred hero, drawn large behind the window as it is.</summary>
    public string? HeroBlurPath => _art.Blur;

    /// <summary>What the window blurs itself when Steam has not: the hero, or failing that the
    /// square tile enlarged. Null while a pre-blurred hero is there, so nothing is blurred twice.</summary>
    public string? BackdropToBlurPath => _art.Blur is not null ? null : _art.Hero ?? SquareTilePath;

    /// <summary>Behind the stage, blurred and enlarged, when there is no hero to be the stage:
    /// the exe's icon or the Xbox logo, so the page is still that game's and not a dark panel.</summary>
    public string? StageBackdropPath => _art.Hero is null ? SquareTilePath : null;

    /// <summary>Drives the fallback title. The wordmark replaces the name where Steam has
    /// one, and the name is shown only where it does not — never both, which would read as
    /// the same words printed twice.</summary>
    public bool HasLogo => LogoPath is not null;

    /// <summary>False while the executable folder is unresolved, which is what makes the
    /// row's actions meaningless rather than merely unavailable.</summary>
    public bool IsResolved => _entry.ExeDirectory is not null;

    private void RaiseAll([CallerMemberName] string? _ = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(string.Empty));

    private void Raise(string name) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
