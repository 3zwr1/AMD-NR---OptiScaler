// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.ComponentModel;
using System.Runtime.CompilerServices;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.ViewModels;

/// <summary>One build the user can pick, as the chooser shows it.</summary>
public sealed class BuildOptionViewModel(SourceInfo source)
{
    public SourceInfo Source { get; } = source;
    public string Name => Source.Name;
    public string Byline => Source.Author.Length > 0 ? $"by {Source.Author}" : "";
    public string Summary => Source.Summary;

    /// <summary>Null unless it is a web address. It comes from the manifest and is opened with
    /// the shell, and the shell will run a program as readily as it opens a page.</summary>
    public Uri? Homepage => Uri.TryCreate(Source.Homepage, UriKind.Absolute, out var uri)
                            && (uri.Scheme == Uri.UriSchemeHttps || uri.Scheme == Uri.UriSchemeHttp)
        ? uri
        : null;

    public string HomepageText => Homepage is { } uri ? uri.Host + uri.AbsolutePath.TrimEnd('/') : "";
}

/// <summary>One answer to the DLSSNR AMD FILES question: a runtime the selected build lists for
/// this card, or No. Immutable, like the stage's DLL names: the chooser rebuilds the list when
/// the pick or the build changes, and the radios read IsSelected one-way.</summary>
public sealed class RuntimeOptionViewModel(string? id, string label, bool isSelected, bool isRecommended)
{
    /// <summary>The runtime package, or null for No.</summary>
    public string? Id { get; } = id;

    public string Label { get; } = label;
    public bool IsSelected { get; } = isSelected;

    /// <summary>The first runtime the manifest lists for the card: what the launcher would
    /// install unasked, and what a user who picks it keeps following.</summary>
    public bool IsRecommended { get; } = isRecommended;
}

/// <summary>Which build to install, and which of the DLSSNR AMD files to fetch with it — one
/// of the runtimes the build lists for this card, or none. The same panel serves the first run
/// and Settings; only the first run cannot be dismissed, because until it is answered there is
/// nothing to download.</summary>
public sealed class BuildChooserViewModel : INotifyPropertyChanged
{
    private readonly TaskCompletionSource<InstallChoice?> _answer =
        new(TaskCreationOptions.RunContinuationsAsynchronously);

    private readonly Manifest? _manifest;
    private readonly GpuInfo _gpu;
    private BuildOptionViewModel? _selected;
    private bool _includeRuntime;
    private string? _pickedRuntimeId;
    private IReadOnlyList<RuntimeOptionViewModel>? _runtimeOptions;

    public event PropertyChangedEventHandler? PropertyChanged;

    /// <summary>The manifest is null when Settings is opened after the manifest failed to
    /// load: ABOUT is still worth showing, and the builds say why they are missing. The GPU is
    /// the one the runtime will be chosen for, so the page can say which card that is.</summary>
    public BuildChooserViewModel(Manifest? manifest, InstallChoice? current, bool isFirstRun, GpuInfo gpu)
    {
        IsFirstRun = isFirstRun;
        _manifest = manifest;
        _gpu = gpu;
        Options = manifest?.OfferedSources.Select(s => new BuildOptionViewModel(s)).ToList() ?? [];

        // AMDNR first for anyone who has not chosen: it is the build this launcher is for, and
        // the one that runs on both RX 7000 and RX 9000.
        _selected = Options.FirstOrDefault(o => current is not null && Is(o, current.SourceId))
                    ?? Options.FirstOrDefault(o => Is(o, "amdnr"))
                    ?? Options.FirstOrDefault();

        // Yes unless they already said no: without the runtime, Neural Rendering does not run
        // at all on RX 7000 or on TheAutomatic's build. A runtime they picked before is picked
        // again, when the build still lists it for this card.
        _includeRuntime = current?.IncludeRuntime ?? true;
        _pickedRuntimeId = current?.RuntimeId;

        PickRuntimeCommand = new RelayCommand(id => { PickRuntime(id as string); return Task.CompletedTask; });
        ContinueCommand = new RelayCommand(_ => { Continue(); return Task.CompletedTask; },
                                           _ => _selected is not null);
        CancelCommand = new RelayCommand(_ => { Cancel(); return Task.CompletedTask; },
                                         _ => !IsFirstRun);
    }

    public bool IsFirstRun { get; }

    /// <summary>Settings, as opposed to the first run: the only time there is something to
    /// cancel, and the only time a change has installed games it does not touch.</summary>
    public bool IsChange => !IsFirstRun;

    public string Heading => IsFirstRun ? "BEFORE YOU START" : "SETTINGS";

    /// <summary>The card the probe found and the generation the runtime is chosen for, said
    /// before the question it bears on. Without an AMD card there is no generation to name;
    /// and when the registry could not be read at all, that is what is said — not that no card
    /// was found, which the launcher does not know.</summary>
    public string DetectedGpu => _gpu.IsUnread
        ? "Detected: the graphics card could not be read"
        : _gpu.IsAmd
            ? $"Detected: {_gpu.Name} ({_gpu.GenerationLabel})"
            : "Detected: no AMD graphics card";

    /// <summary>Amber, and only ever a note. The probe is best effort and the manifest decides
    /// what is installed, so a card the table does not know, or takes for an older generation,
    /// is told what will happen and left to continue. Empty for a generation Neural Rendering
    /// runs on.</summary>
    public string GpuNote => _gpu.IsUnread
        ? "The graphics card could not be read, so the launcher cannot say whether Neural Rendering " +
          "will run here. AMDNR is for AMD GPUs."
        : _gpu.Generation switch
        {
            GpuInfo.Rdna2 => "RX 6000 is not supported yet — Neural Rendering will not run. AMDNR is working on it.",
            GpuInfo.Rdna1 => "RX 5000 is not supported — Neural Rendering will not run on it.",
            GpuInfo.UnknownAmd => "Could not tell which generation this AMD card is; installing anyway.",
            GpuInfo.None => "No AMD graphics card was found. AMDNR is for AMD GPUs; Neural Rendering will not run here.",
            _ => "",
        };

    public bool HasGpuNote => GpuNote.Length > 0;

    public string LauncherVersion => Services.SelfUpdater.CurrentVersion;

    public IReadOnlyList<BuildOptionViewModel> Options { get; }

    public bool HasOptions => Options.Count > 0;

    public string NoOptionsNote =>
        "The list of builds is not available until the update manifest loads. Check the " +
        "connection and restart the launcher.";

    public BuildOptionViewModel? SelectedOption
    {
        get => _selected;
        set
        {
            // A ListBox pushes null back when its items are re-templated; the choice itself
            // must not go with it.
            if (value is null || ReferenceEquals(value, _selected)) return;
            _selected = value;
            _runtimeOptions = null;
            Raise();
            Raise(nameof(RuntimeNote));
            Raise(nameof(RuntimeSize));
            Raise(nameof(HasRuntimeSize));
            Raise(nameof(OffersRuntime));
            Raise(nameof(RuntimeOptions));
            Raise(nameof(ShowRuntimeWarning));
            ContinueCommand.RaiseCanExecuteChanged();
        }
    }

    /// <summary>Yes or No to the files. Yes is the card's first-listed runtime unless one was
    /// picked; the picker's radios drive this through <see cref="PickRuntime"/>.</summary>
    public bool IncludeRuntime
    {
        get => _includeRuntime;
        set
        {
            if (value == _includeRuntime) return;
            _includeRuntime = value;
            RuntimeChanged();
        }
    }

    /// <summary>The runtimes the selected build lists for this card, in the manifest's order with
    /// the first marked as the one the launcher would install unasked, then No. What the
    /// DLSSNR AMD FILES radios show.</summary>
    public IReadOnlyList<RuntimeOptionViewModel> RuntimeOptions => _runtimeOptions ??= BuildRuntimeOptions();

    public RelayCommand PickRuntimeCommand { get; }

    /// <summary>A click on one of the radios: a runtime package, or null for No. Picking the
    /// card's first-listed runtime is stored as no pick at all — that user follows the manifest,
    /// and moves when the owner moves the card to a newer runtime; any other runtime is kept by
    /// id and stays until the user changes it.</summary>
    public void PickRuntime(string? id)
    {
        if (id is null)
        {
            _includeRuntime = false;
        }
        else
        {
            _includeRuntime = true;
            _pickedRuntimeId = Same(id, ListedRuntimes.FirstOrDefault()) ? null : id;
        }

        RuntimeChanged();
    }

    private IReadOnlyList<RuntimeOptionViewModel> BuildRuntimeOptions()
    {
        var listed = ListedRuntimes;
        var chosen = OfferedRuntimeId;
        var options = new List<RuntimeOptionViewModel>(listed.Count + 1);

        for (var i = 0; i < listed.Count; i++)
        {
            var id = listed[i];
            var version = _manifest?.TryPackage(id)?.Version ?? id;
            options.Add(new RuntimeOptionViewModel(id,
                i == 0 ? $"{version} · RECOMMENDED" : version,
                isSelected: _includeRuntime && Same(id, chosen),
                isRecommended: i == 0));
        }

        options.Add(new RuntimeOptionViewModel(null, "NO", isSelected: !_includeRuntime, isRecommended: false));
        return options;
    }

    /// <summary>The runtime packages the selected build lists for this card, in the manifest's
    /// order. Without a build to read a list from, the one package every older manifest had.</summary>
    private IReadOnlyList<string> ListedRuntimes => _selected is null
        ? [SourceInfo.LegacyRuntimePackage]
        : _selected.Source.RuntimesFor(_gpu.Generation);

    private void RuntimeChanged()
    {
        _runtimeOptions = null;
        Raise(nameof(IncludeRuntime));
        Raise(nameof(DeclineRuntime));
        Raise(nameof(RuntimeOptions));
        Raise(nameof(RuntimeSize));
        Raise(nameof(HasRuntimeSize));
        Raise(nameof(ShowRuntimeWarning));
    }

    /// <summary>The No radio button's side of IncludeRuntime. Two properties rather than an
    /// inverting converter, because a converter on a two-way radio binding has to know how to
    /// undo an uncheck, and that is where such bindings go wrong.</summary>
    public bool DeclineRuntime
    {
        get => !_includeRuntime;
        set => IncludeRuntime = !value;
    }

    /// <summary>The runtime package Yes means here, by the rule the install itself uses: the one
    /// the user picked when the selected build lists it for this card, otherwise the first the
    /// build lists — 0.4.1 for RX 9000, 0.3.3 for most. Without a build to read a list from, the
    /// one package every older manifest had. Null when the build lists runtimes and names none
    /// for this generation.</summary>
    private string? OfferedRuntimeId => _selected is null
        ? SourceInfo.LegacyRuntimePackage
        : _selected.Source.RuntimePackageFor(_gpu.Generation, _pickedRuntimeId);

    /// <summary>False when the selected build lists no runtime for this card. Yes would then
    /// download nothing, so the Yes/No question gives way to <see cref="NoRuntimeNote"/>. The
    /// preference itself is left as it was: nothing was offered, so nothing was declined, and a
    /// manifest that one day lists the card should get the default the user never changed.</summary>
    public bool OffersRuntime => OfferedRuntimeId is not null;

    public string NoRuntimeNote => "No DLSSNR AMD files are offered for this card with this build.";

    /// <summary>The question names the runtime's author in full and says on what terms his
    /// files ship, in the words he asked for (<see cref="Attribution"/>). The page draws it
    /// from the three pieces below so that the project's name can be a link to his repository;
    /// this is the whole line as read.</summary>
    public string RuntimeQuestion => Attribution.RuntimeQuestion;

    public string RuntimeQuestionLead => Attribution.RuntimeQuestionLead;
    public string RuntimeProject => Attribution.RuntimeProject;
    public Uri RuntimeProjectUrl => Attribution.RuntimeProjectUrl;
    public string RuntimeQuestionTail => Attribution.RuntimeQuestionTail;

    /// <summary>Sized from the runtime this card would get with the selected build, because
    /// that is the download Yes means here. Its own line under the question, so the question
    /// stays the attribution and nothing else. Empty when the manifest gives no size, or there
    /// is no manifest: no line, rather than a size made up.</summary>
    public string RuntimeSize =>
        OfferedRuntimeId is { } runtimeId && _manifest?.TryPackage(runtimeId) is { Size: > 0 } runtime
            ? $"About {Megabytes(runtime.Size)} MB."
            : "";

    public bool HasRuntimeSize => RuntimeSize.Length > 0;

    /// <summary>ABOUT's credit for the runtime: his linked name, then his copyright line.</summary>
    public string RuntimeAuthor => Attribution.RuntimeAuthor;

    public string RuntimeCopyright => Attribution.RuntimeCopyright;

    /// <summary>ABOUT's line for the launcher itself: the string the exe's Details tab carries.</summary>
    public string LauncherCopyright => Attribution.LauncherCopyright;

    public string RuntimeNote => _selected?.Source.RuntimeNote ?? "";

    /// <summary>Only when there was something to decline: a build that requires the runtime,
    /// a card it lists one for, and No.</summary>
    public bool ShowRuntimeWarning => OffersRuntime && _selected?.Source.RuntimeRequired == true && !_includeRuntime;

    public string RuntimeWarning => "Neural Rendering will not run without them.";

    public string ChangeNote =>
        "Changing the build affects future installs only. A game that is already installed keeps " +
        "its build until you press INSTALL on it again.";

    public RelayCommand ContinueCommand { get; }
    public RelayCommand CancelCommand { get; }

    /// <summary>The user's answer, or null when Settings was closed without one.</summary>
    public Task<InstallChoice?> Answer => _answer.Task;

    public void Continue()
    {
        if (_selected is null) return;

        // The pick is saved only when it means something: Yes, a runtime the build lists for
        // this card, and not the one the card would get anyway.
        var listed = ListedRuntimes;
        var pick = _includeRuntime && _pickedRuntimeId is { } id
                   && listed.Contains(id, StringComparer.OrdinalIgnoreCase)
                   && !Same(id, listed[0])
            ? _pickedRuntimeId
            : null;

        _answer.TrySetResult(new InstallChoice(_selected.Source.Id, _includeRuntime, pick));
    }

    public void Cancel()
    {
        if (!IsFirstRun) _answer.TrySetResult(null);
    }

    private static bool Is(BuildOptionViewModel option, string id)
        => string.Equals(option.Source.Id, id, StringComparison.OrdinalIgnoreCase);

    private static bool Same(string? a, string? b) => string.Equals(a, b, StringComparison.OrdinalIgnoreCase);

    /// <summary>Rounded to ten: "about 110 MB" is a size, "about 111 MB" is a false precision
    /// that goes stale with every rebuild of the runtime.</summary>
    private static long Megabytes(long bytes) => Math.Max(10, (long)Math.Round(bytes / 1e7) * 10);

    private void Raise([CallerMemberName] string? name = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
