// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using System.Runtime.ExceptionServices;
using System.Windows.Threading;
using AmdnrLauncher.App.ViewModels;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>What the stage tells the user about the selected game, and the install that runs
/// behind it. Every folder these touch is a temporary one: the game, the package it installs
/// from, and the records the install writes.</summary>
public sealed class StageTests : IDisposable
{
    private readonly string _root = Path.Combine(
        Path.GetTempPath(), "amdnr-stage-" + Guid.NewGuid().ToString("N"));

    private readonly string _previousRecordRoot = InstallRecordStore.RootDirectory;

    private string Game => Path.Combine(_root, "game");
    private string Packages => Path.Combine(_root, "packages");

    public StageTests()
    {
        Directory.CreateDirectory(Game);
        File.WriteAllText(Path.Combine(Game, "Game.exe"), "stand-in for a game");

        Directory.CreateDirectory(Path.Combine(Packages, "amdnr"));
        File.WriteAllText(Path.Combine(Packages, "amdnr", "OptiScaler.dll"), "stand-in for OptiScaler");
        File.WriteAllText(Path.Combine(Packages, "amdnr", "OptiScaler.ini"), "[DlssNr]\r\nEnabled=true\r\n");

        // Doctor calls an install without its support folder a problem, and a problem row
        // rightly shows no next steps.
        Directory.CreateDirectory(Path.Combine(Packages, "amdnr", "OptiScaler"));
        File.WriteAllText(Path.Combine(Packages, "amdnr", "OptiScaler", "support.txt"), "stand-in");

        // The install writes its record here, not into the user's own AMDNR folder.
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
            { "id": "amdnr", "name": "AMDNR", "author": "3zwr1", "summary": "s",
              "homepage": "https://github.com/3zwr1/AMD-NR---OptiScaler",
              "package": "amdnr", "forwarder": null, "runtimeRequired": false, "runtimeNote": "n" }
          ],
          "packages": [
            { "id": "amdnr", "version": "0.3.2", "url": "https://example.test/AMDNR-v0.3.2.zip", "sha256": "00", "size": 1 }
          ],
          "proxyDefaults": ["dxgi.dll", "d3d12.dll"],
          "proxyOverrides": []
        }
        """;

    private static readonly string AntiCheatWords =
        "Elden Ring uses EasyAntiCheat. Mods like AMDNR can get your account banned in online " +
        "play. Only continue if you play offline, and uninstall before playing online.";

    private static GameEntry Entry(
        string name, string? exeDirectory, GameStatus status = GameStatus.NotInstalled,
        GateResult? gate = null, string installRoot = @"D:\Games\Somewhere",
        IReadOnlyList<string>? occupied = null) => new(
        new GameCandidate(name, GameStore.Steam, installRoot),
        ExeDirectory: exeDirectory,
        ExeName: exeDirectory is null ? null : "Game-Win64-Shipping.exe",
        Status: status,
        Record: null,
        Report: null,
        Gate: gate,
        RecommendedProxy: "dxgi.dll",
        ProxyReason: "first free name",
        OccupiedProxies: occupied ?? []);

    private static GateResult AntiCheat(params string[] products) => new(
    [
        new GateFinding(GateCode.AntiCheatDetected, "worded for somebody else", Blocking: true, products),
    ]);

    [Fact]
    public void The_anti_cheat_line_names_the_game_and_the_system_and_says_to_play_offline()
    {
        var row = new GameRowViewModel(Entry("Elden Ring", @"D:\Games\ER", gate: AntiCheat("EasyAntiCheat")));

        Assert.Equal(AntiCheatWords, row.AntiCheatWarning);

        var two = new GameRowViewModel(Entry("Elden Ring", @"D:\Games\ER", gate: AntiCheat("EasyAntiCheat", "BattlEye")));
        Assert.StartsWith("Elden Ring uses EasyAntiCheat and BattlEye. ", two.AntiCheatWarning);
    }

    [Fact]
    public void An_RE_Engine_game_without_REFramework_says_so_on_the_stage()
    {
        // The gate's finding, word for word: the game would close itself after launch with the
        // mod loaded, and the line names the file that stops that and where it comes from.
        var gate = new GateResult([new GateFinding(GateCode.ReframeworkMissing, ReEngine.MissingReframework, Blocking: false)]);
        var row = new GameRowViewModel(Entry("Resident Evil Requiem", @"D:\Games\RE9", gate: gate));

        Assert.Equal(ReEngine.MissingReframework, row.EngineWarning);
        Assert.Null(row.AntiCheatWarning);
        Assert.True(row.IsSupported);

        Assert.Null(new GameRowViewModel(Entry("Stray", @"D:\Games\Stray")).EngineWarning);
    }

    [Fact]
    public void A_game_without_anti_cheat_has_no_warning_line()
    {
        var gate = new GateResult([new GateFinding(GateCode.GameRunning, "Close the game first.", Blocking: true)]);

        Assert.Null(new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", gate: gate)).AntiCheatWarning);
        Assert.Null(new GameRowViewModel(Entry("Stray", @"D:\Games\Stray")).AntiCheatWarning);
    }

    [Fact]
    public void The_update_banner_names_the_build_REPAIR_puts_back()
    {
        // REPAIR / UPDATE reinstalls the record's own build. A banner that always said AMDNR
        // would promise a switch of build the button does not make.
        var automatic = new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", GameStatus.UpdateAvailable) with
        {
            Report = new Core.Diagnostics.DoctorReport([], null, UpdateAvailable: true, BuildName: "OptiScaler AMD pre-SR"),
        });

        Assert.Equal("Another OptiScaler AMD pre-SR build is available. Use REPAIR / UPDATE to install it.",
            automatic.UpdateNote);

        // An update is offered whenever the manifest's version differs, which includes a build the
        // owner rolled back to: "newer" would then be wrong, so the banner names both versions.
        var rolledBack = new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", GameStatus.UpdateAvailable) with
        {
            Record = new InstallRecord(@"D:\Games\Stray", "Stray", "dxgi.dll", "theautomatic", "1.9.1-alpha",
                null, null, [], DateTimeOffset.UtcNow, false, []),
            Report = new Core.Diagnostics.DoctorReport([], null, UpdateAvailable: true,
                BuildName: "OptiScaler AMD pre-SR", AvailableVersion: "1.8.6-0.3.1"),
        });

        Assert.Equal("OptiScaler AMD pre-SR 1.8.6-0.3.1 is available; this game has 1.9.1-alpha. " +
                     "Use REPAIR / UPDATE to install it.", rolledBack.UpdateNote);

        // A genuinely newer build reads the same way: nothing here ranks two publishers' strings.
        var newer = new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", GameStatus.UpdateAvailable) with
        {
            Record = new InstallRecord(@"D:\Games\Stray", "Stray", "dxgi.dll", "amdnr", "0.3.1",
                null, null, [], DateTimeOffset.UtcNow, false, []),
            Report = new Core.Diagnostics.DoctorReport([], null, UpdateAvailable: true,
                BuildName: "AMDNR", AvailableVersion: "0.3.2"),
        });

        Assert.Equal("AMDNR 0.3.2 is available; this game has 0.3.1. Use REPAIR / UPDATE to install it.",
            newer.UpdateNote);

        // The card's runtime moved on while the build itself is current: the files are named, and
        // the sentence says why they are offered — the card, not a new build.
        var runtimeOnly = new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", GameStatus.UpdateAvailable) with
        {
            Record = new InstallRecord(@"D:\Games\Stray", "Stray", "dxgi.dll", "amdnr", "0.3.2", "0.3.3", null,
                [], DateTimeOffset.UtcNow, false, []),
            Report = new Core.Diagnostics.DoctorReport([], null, UpdateAvailable: true, BuildName: "AMDNR",
                AvailableRuntimeVersion: "0.4.1"),
        });
        Assert.Equal("DLSSNR AMD files 0.4.1 are now the runtime for this card; this game has 0.3.3. " +
                     "Use REPAIR / UPDATE to install them.", runtimeOnly.UpdateNote);

        // Both at once: one press brings both, and the note says so.
        var both = new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", GameStatus.UpdateAvailable) with
        {
            Record = new InstallRecord(@"D:\Games\Stray", "Stray", "dxgi.dll", "amdnr", "0.3.1", "0.3.3", null,
                [], DateTimeOffset.UtcNow, false, []),
            Report = new Core.Diagnostics.DoctorReport([], null, UpdateAvailable: true, BuildName: "AMDNR",
                AvailableVersion: "0.3.2", AvailableRuntimeVersion: "0.4.1"),
        });
        Assert.Equal("AMDNR 0.3.2 is available; this game has 0.3.1. DLSSNR AMD files 0.4.1 are now the runtime " +
                     "for this card; this game has 0.3.3. Use REPAIR / UPDATE to install them.", both.UpdateNote);

        Assert.Null(new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", GameStatus.Installed)).UpdateNote);
    }

    [Fact]
    public void A_row_with_no_resolved_exe_folder_shows_no_DLL_name_and_no_exe()
    {
        var unresolved = new GameRowViewModel(Entry("Mystery", exeDirectory: null));
        Assert.Null(unresolved.DllName);
        Assert.Null(unresolved.ExeName);

        var resolved = new GameRowViewModel(Entry("SILENT HILL 2", @"D:\Games\SH2\SHProto\Binaries\Win64"));
        Assert.Equal("dxgi.dll", resolved.DllName);
        Assert.Equal("Game-Win64-Shipping.exe", resolved.ExeName);
    }

    [Fact]
    public void An_earlier_install_reads_as_found_and_says_INSTALL_keeps_its_DLL_name()
    {
        var row = new GameRowViewModel(Entry("SILENT HILL 2", @"D:\Games\SH2", GameStatus.ExistingInstall));

        Assert.Equal("Earlier install found", row.StatusText);
        Assert.Equal(
            "Found an earlier AMDNR / OptiScaler install (dxgi.dll). INSTALL updates it and keeps that DLL name.",
            row.EarlierInstallNote);

        Assert.Null(new GameRowViewModel(Entry("Stray", @"D:\Games\Stray")).EarlierInstallNote);
    }

    [Fact]
    public void Picking_another_name_on_an_earlier_install_says_the_old_DLL_is_moved_aside_not_kept()
    {
        // The hand-made OptiScaler loads through dxgi.dll and the user picks winmm.dll: INSTALL
        // puts the mod in as winmm.dll and moves that dxgi.dll into AMDNR_backup, so the note
        // must not promise to keep a name nothing will be kept under.
        var row = new GameRowViewModel(
            Entry("SILENT HILL 2", @"D:\Games\SH2", GameStatus.ExistingInstall) with
            {
                ProxyReason = "your earlier install used dxgi.dll",
            });

        row.ChosenProxy = "winmm.dll";

        Assert.Equal(
            "Found an earlier AMDNR / OptiScaler install (dxgi.dll). INSTALL moves it into AMDNR_backup and installs as winmm.dll.",
            row.EarlierInstallNote);
        Assert.Equal(
            "INSTALL will use winmm.dll. The launcher would have picked dxgi.dll — your earlier install used dxgi.dll.",
            row.ProxyReason);

        // Back on the name the earlier install used, it is kept again.
        row.ChosenProxy = "dxgi.dll";
        Assert.Equal(
            "Found an earlier AMDNR / OptiScaler install (dxgi.dll). INSTALL updates it and keeps that DLL name.",
            row.EarlierInstallNote);
    }

    [Fact]
    public void The_uninstall_message_gives_the_full_folder_the_earlier_build_was_kept_in()
    {
        // On an Unreal game the exe folder is three levels under the game's root, which is
        // where the user looks first: a path relative to it sends them to the wrong place.
        var game = @"D:\Games\SH2\SHProto\Binaries\Win64";
        var kept = @"AMDNR_backup\20260924-101500\dxgi.dll";
        var folder = @"D:\Games\SH2\SHProto\Binaries\Win64\AMDNR_backup\20260924-101500";

        Assert.Equal("Uninstalled.", MainViewModel.UninstallMessage(UninstallResult.Nothing, game));

        Assert.Equal(
            $"Uninstalled. Your earlier OptiScaler build was kept, switched off, in {folder}.",
            MainViewModel.UninstallMessage(new UninstallResult([], [], [kept]), game));

        // Two names from the same backup are one place to look, not two.
        Assert.Equal(
            $"Uninstalled. Your earlier OptiScaler build was kept, switched off, in {folder}.",
            MainViewModel.UninstallMessage(
                new UninstallResult([], [], [kept, @"AMDNR_backup\20260924-101500\winmm.dll"]), game));

        // A partial uninstall must still read as one, and still say where the old build went.
        var both = MainViewModel.UninstallMessage(new UninstallResult(["OptiScaler.ini"], [], [kept]), game);
        Assert.Contains("could not be removed", both);
        Assert.Contains("OptiScaler.ini", both);
        Assert.Contains($"Your earlier OptiScaler build was kept, switched off, in {folder}.", both);
    }

    [Fact]
    public void The_uninstall_message_tells_changed_files_from_stuck_ones_and_says_where_originals_wait()
    {
        var game = @"D:\Games\SH2\SHProto\Binaries\Win64";

        // Kept on purpose: "close the game and try again" would send the user after nothing.
        var changed = MainViewModel.UninstallMessage(new UninstallResult([], [], [], ["OptiScaler.ini"]), game);
        Assert.DoesNotContain("close the game", changed);
        Assert.Contains("changed after the install: OptiScaler.ini.", changed);

        // Stuck: the record is kept, so pressing Uninstall again is the way on — and the user's
        // own file is named with the folder it waits in, not just its name.
        var stuck = MainViewModel.UninstallMessage(
            new UninstallResult(["dxgi.dll"], [@"AMDNR_backup\20260924-101500\dxgi.dll"], []), game);
        Assert.Contains("close the game and press UNINSTALL again: dxgi.dll.", stuck);
        Assert.Contains(
            @"Your own dxgi.dll could not be put back, and is still in D:\Games\SH2\SHProto\Binaries\Win64\AMDNR_backup\20260924-101500.",
            stuck);

        // Nothing of ours left, but a backup that could not go back yet: the record is kept for
        // another Uninstall, and the message has to say so rather than "Uninstalled." — the row
        // then offers Repair too, which would put the mod straight back in.
        var waiting = MainViewModel.UninstallMessage(
            new UninstallResult([], [@"AMDNR_backup\20260924-101500\dxgi.dll"], []) { CanFinishLater = true }, game);
        Assert.StartsWith("Not finished:", waiting);
        Assert.Contains("close the game and press UNINSTALL again", waiting);
        Assert.Contains(@"is still in D:\Games\SH2\SHProto\Binaries\Win64\AMDNR_backup\20260924-101500.", waiting);
    }

    [Fact]
    public async Task The_uninstall_message_says_a_backup_could_not_be_read_and_where_it_is()
    {
        // A backup Core could not even list is named by the folder itself, relative to the game
        // folder: AMDNR_backup\<stamp>. Nothing holds it that closing the game would free, and
        // "Your own 20260924-101500" is not a file anyone can look for. Taken from Core itself
        // first, so the form this reads is the one Core emits: a backup whose name answers but
        // which cannot be listed, the way an unreadable one fails.
        var backup = Path.Combine(Game, "AMDNR_backup", "20260924-101500");
        var notThere = Path.Combine(_root, "not-there");
        Directory.CreateDirectory(Path.GetDirectoryName(backup)!);
        Directory.CreateSymbolicLink(backup, notThere);

        var record = new InstallRecord(Game, "Test Game", "dxgi.dll", "amdnr", "0.3.2", null, null,
            [backup], DateTimeOffset.UtcNow, false, []);
        var emitted = await new Installer(new FolderAssetStore(Packages), new CopyFilePlacer())
            .UninstallAsync(record, default);
        Directory.CreateDirectory(notThere);

        var fromCore = MainViewModel.UninstallMessage(emitted, Game);
        Assert.StartsWith($"Not finished: the backup of your own files in {backup} could not be read", fromCore);
        Assert.DoesNotContain("close the game", fromCore);

        var game = @"D:\Games\SH2\SHProto\Binaries\Win64";
        var folder = @"D:\Games\SH2\SHProto\Binaries\Win64\AMDNR_backup\20260924-101500";

        var unread = MainViewModel.UninstallMessage(
            new UninstallResult([], [@"AMDNR_backup\20260924-101500"], []) { CanFinishLater = true }, game);

        Assert.StartsWith($"Not finished: the backup of your own files in {folder} could not be read", unread);
        Assert.DoesNotContain("close the game", unread);
        Assert.DoesNotContain("Your own 20260924-101500", unread);

        // Beside a file that waits in another backup, each is still told the way that fits it.
        var both = MainViewModel.UninstallMessage(
            new UninstallResult([],
                [@"AMDNR_backup\20260924-101500", @"AMDNR_backup\20260923-090000\dxgi.dll"], [])
            {
                CanFinishLater = true,
            }, game);

        Assert.StartsWith($"Not finished: the backup of your own files in {folder} could not be read", both);
        Assert.Contains(
            @"Your own dxgi.dll could not be put back, and is still in D:\Games\SH2\SHProto\Binaries\Win64\AMDNR_backup\20260923-090000.",
            both);
        Assert.DoesNotContain("20260924-101500 could not be put back", both);

        // With something of ours still held too, the held files lead and the backup follows.
        var held = MainViewModel.UninstallMessage(
            new UninstallResult(["dxgi.dll"], [@"AMDNR_backup\20260924-101500"], []) { CanFinishLater = true }, game);

        Assert.StartsWith("Uninstalled, but these are in use", held);
        Assert.Contains($"The backup of your own files in {folder} could not be read", held);
    }

    [Fact]
    public async Task Saying_No_at_the_anti_cheat_prompt_installs_nothing_and_it_was_asked_in_the_stage_words()
    {
        var store = new FolderAssetStore(Packages);
        var viewModel = await PreparedViewModel(store);
        viewModel.ReplaceGames([Entry("Elden Ring", Game, gate: AntiCheat("EasyAntiCheat"), installRoot: Game)]);
        viewModel.Selected = viewModel.Games[0];

        string? asked = null;
        viewModel.ConfirmAntiCheat = message => { asked = message; return false; };

        await viewModel.InstallAsync(keepInstalledBuild: false);

        Assert.Equal(AntiCheatWords, asked);
        Assert.Equal("Install cancelled.", viewModel.StatusMessage);
        Assert.Empty(store.EnsuredOnPoolThread);
        Assert.False(File.Exists(Path.Combine(Game, "dxgi.dll")));
    }

    [Fact]
    public async Task A_Ricochet_game_says_why_on_the_stage_and_is_never_asked_about()
    {
        const string words = "Call of Duty's Ricochet anti-cheat bans accounts for injected DLLs, and " +
                             "these games have no offline mode where that is safe. AMDNR will not install here.";
        var blocked = new GateResult(
            [new GateFinding(GateCode.AntiCheatBlocked, words, Blocking: true, ["Ricochet"])]);
        var entry = Entry("Call of Duty", Game, GameStatus.Unsupported, gate: blocked, installRoot: Game);

        var row = new GameRowViewModel(entry);
        Assert.Equal(words, row.AntiCheatWarning);
        Assert.False(row.IsSupported);

        // Nothing will ever be installed here, so no DLL name: "dxgi.dll" beside a blocked game
        // reads as the file the launcher is about to put in it.
        Assert.Null(row.DllName);

        // REPAIR stays reachable on a row installed before the block existed; a prompt there
        // would ask a question whose Yes can only fail.
        var store = new FolderAssetStore(Packages);
        var viewModel = await PreparedViewModel(store);
        viewModel.ReplaceGames([entry]);
        viewModel.Selected = viewModel.Games[0];

        var asked = false;
        viewModel.ConfirmAntiCheat = _ => { asked = true; return true; };

        await viewModel.InstallAsync(keepInstalledBuild: true);

        Assert.False(asked);
        Assert.Equal(words, viewModel.StatusMessage);
        Assert.Empty(store.EnsuredOnPoolThread);
    }

    [Fact]
    public async Task TRY_NEXT_PROXY_is_off_on_an_installed_Ricochet_row_and_says_why()
    {
        const string words = "Call of Duty's Ricochet anti-cheat bans accounts for injected DLLs, and " +
                             "these games have no offline mode where that is safe. AMDNR will not install here.";
        var blocked = new GateResult(
            [new GateFinding(GateCode.AntiCheatBlocked, words, Blocking: true, ["Ricochet"])]);
        var record = new InstallRecord(Game, "Call of Duty", "dxgi.dll", "amdnr", "0.3.2", null, null,
            [], DateTimeOffset.UtcNow, false, []);
        var entry = Entry("Call of Duty", Game, GameStatus.Unsupported, gate: blocked, installRoot: Game)
            with { Record = record };

        var viewModel = await PreparedViewModel(new FolderAssetStore(Packages));
        viewModel.ReplaceGames([entry]);
        viewModel.Selected = viewModel.Games[0];

        // Moving the mod to another name could start loading a DLL that was not loading.
        Assert.False(viewModel.TryNextProxyCommand.CanExecute(null));

        // UNINSTALL is how the user gets the mod back out, so it stays.
        Assert.True(viewModel.UninstallCommand.CanExecute(null));

        await viewModel.TryNextProxyAsync();
        Assert.Equal(words, viewModel.StatusMessage);

        // The picker is the same move under another name, and is off for the same reason.
        Assert.False(viewModel.ChooseProxyCommand.CanExecute("winmm.dll"));
        await viewModel.ChooseProxyAsync("winmm.dll");
        Assert.Equal(words, viewModel.StatusMessage);
        Assert.Equal("dxgi.dll", viewModel.Games[0].Proxy);
    }

    [Fact]
    public void The_picker_lists_the_six_names_with_the_one_in_use_marked_and_taken_names_greyed()
    {
        var row = new GameRowViewModel(Entry("Stray", @"D:\Games\Stray", occupied: ["winmm.dll"]));

        Assert.Equal(PayloadNames.ProxyNames, row.ProxyOptions.Select(o => o.Name));
        Assert.Equal(["dxgi.dll"], row.ProxyOptions.Where(o => o.IsSelected).Select(o => o.Name));

        var taken = Assert.Single(row.ProxyOptions, o => !o.IsEnabled);
        Assert.Equal("winmm.dll", taken.Name);
        Assert.Equal(
            "winmm.dll is already in this game folder and was not put there by this launcher. Move it aside yourself to use this name.",
            taken.Tooltip);
        Assert.All(row.ProxyOptions.Where(o => o.IsEnabled), o => Assert.Null(o.Tooltip));

        Assert.Equal("dxgi.dll — first free name.", row.ProxyReason);

        // The same list until something changes, so the bindings are not rebuilt on every read.
        Assert.Same(row.ProxyOptions, row.ProxyOptions);

        // No exe folder, nothing to pick from and nothing to explain.
        var unresolved = new GameRowViewModel(Entry("Mystery", exeDirectory: null));
        Assert.Null(unresolved.ProxyReason);
        Assert.Null(unresolved.DllName);
    }

    [Fact]
    public void RESET_INI_works_the_game_folder_off_the_UI_thread_and_says_where_the_old_ini_was_kept()
    {
        RunOnDispatcher(async () =>
        {
            var store = new FolderAssetStore(Packages);
            var viewModel = await PreparedViewModel(store);
            viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
            viewModel.Selected = viewModel.Games[0];
            await viewModel.InstallAsync(keepInstalledBuild: false);

            var ini = Path.Combine(Game, "OptiScaler.ini");
            File.WriteAllText(ini, "[DlssNr]\r\nEnabled=false\r\n; tuned by me\r\n");
            store.PathForOnPoolThread.Clear();
            var ui = Environment.CurrentManagedThreadId;

            await viewModel.ResetIniAsync();

            // The seed is read from the store on the pool — the rule for every look at a game
            // folder — and the continuation comes back to the window's thread.
            Assert.NotEmpty(store.PathForOnPoolThread);
            Assert.All(store.PathForOnPoolThread, onPool => Assert.True(onPool));
            Assert.Equal(ui, Environment.CurrentManagedThreadId);

            Assert.Equal("[DlssNr]\r\nEnabled=true\r\n", File.ReadAllText(ini));
            Assert.StartsWith("OptiScaler.ini reset. Your previous file was kept as ", viewModel.StatusMessage);
            Assert.False(viewModel.Busy);
            Assert.Null(viewModel.ChangingGame);
        });
    }

    [Fact]
    public void The_name_in_use_stays_pickable_even_when_the_probe_calls_its_file_taken()
    {
        // Installed with a DLL the probe cannot tell for an OptiScaler — a build without the
        // version resource — the name in use is still the name in use.
        var record = new InstallRecord(@"D:\Games\Stray", "Stray", "winmm.dll", "amdnr", "0.3.2", null, null,
            [], DateTimeOffset.UtcNow, false, []);
        var row = new GameRowViewModel(
            Entry("Stray", @"D:\Games\Stray", GameStatus.Installed, occupied: ["winmm.dll", "dxgi.dll"]) with
            {
                Record = record,
                RecommendedProxy = "winmm.dll",
                ProxyReason = "your earlier install used winmm.dll",
            });

        var current = Assert.Single(row.ProxyOptions, o => o.IsSelected);
        Assert.Equal("winmm.dll", current.Name);
        Assert.True(current.IsEnabled);
        Assert.False(row.ProxyOptions.Single(o => o.Name == "dxgi.dll").IsEnabled);
        Assert.Equal("winmm.dll — your earlier install used winmm.dll.", row.ProxyReason);
    }

    [Fact]
    public async Task Choosing_a_name_on_a_game_not_yet_installed_is_the_name_INSTALL_uses()
    {
        var store = new FolderAssetStore(Packages);
        var viewModel = await PreparedViewModel(store);
        viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
        var row = viewModel.Games[0];
        viewModel.Selected = row;
        Assert.True(viewModel.ChooseProxyCommand.CanExecute("d3d12.dll"));

        await viewModel.ChooseProxyAsync("d3d12.dll");

        Assert.Equal("d3d12.dll", row.Proxy);
        Assert.Equal("d3d12.dll", row.DllName);
        Assert.Equal(["d3d12.dll"], row.ProxyOptions.Where(o => o.IsSelected).Select(o => o.Name));
        Assert.Equal("INSTALL will use d3d12.dll. The launcher would have picked dxgi.dll — first free name.",
            row.ProxyReason);
        Assert.Equal("INSTALL will use d3d12.dll.", viewModel.StatusMessage);
        Assert.Empty(store.EnsuredOnPoolThread);

        await viewModel.InstallAsync(keepInstalledBuild: false);

        Assert.True(File.Exists(Path.Combine(Game, "d3d12.dll")));
        Assert.False(File.Exists(Path.Combine(Game, "dxgi.dll")));
        Assert.Equal("d3d12.dll", row.Entry.Record!.Proxy);
        Assert.Equal(["d3d12.dll"], row.ProxyOptions.Where(o => o.IsSelected).Select(o => o.Name));
    }

    [Fact]
    public async Task Choosing_a_name_on_an_installed_game_moves_the_DLL_there_and_a_refused_name_snaps_back()
    {
        var viewModel = await PreparedViewModel(new FolderAssetStore(Packages));
        viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
        var row = viewModel.Games[0];
        viewModel.Selected = row;
        await viewModel.InstallAsync(keepInstalledBuild: false);

        await viewModel.ChooseProxyAsync("winmm.dll");

        Assert.Equal("winmm.dll", row.Proxy);
        Assert.True(File.Exists(Path.Combine(Game, "winmm.dll")));
        Assert.False(File.Exists(Path.Combine(Game, "dxgi.dll")));
        Assert.Equal(["dxgi.dll"], row.Entry.Record!.TriedProxies);
        Assert.Equal("Switched to winmm.dll. Start the game, then press INSERT.", viewModel.StatusMessage);
        Assert.Equal(["winmm.dll"], row.ProxyOptions.Where(o => o.IsSelected).Select(o => o.Name));

        // Choosing the name in use is nothing to do.
        var options = row.ProxyOptions;
        await viewModel.ChooseProxyAsync("winmm.dll");
        Assert.Same(options, row.ProxyOptions);

        // A name another file holds: Core refuses, its words reach the status line, and the
        // picker shows the name still in place — greyed where it is taken — not the one clicked.
        File.WriteAllText(Path.Combine(Game, "version.dll"), "somebody else's");
        await viewModel.ChooseProxyAsync("version.dll");

        Assert.Contains("version.dll already exists", viewModel.StatusMessage);
        Assert.Equal("winmm.dll", row.Proxy);
        Assert.NotSame(options, row.ProxyOptions);
        Assert.Equal(["winmm.dll"], row.ProxyOptions.Where(o => o.IsSelected).Select(o => o.Name));
        Assert.False(row.ProxyOptions.Single(o => o.Name == "version.dll").IsEnabled);
        Assert.True(File.Exists(Path.Combine(Game, "winmm.dll")));
        Assert.Equal("somebody else's", File.ReadAllText(Path.Combine(Game, "version.dll")));
    }

    [Fact]
    public async Task TRY_NEXT_PROXY_says_so_when_it_starts_the_round_again()
    {
        // This manifest lists two names, so the round is short: dxgi, d3d12, and then — not a
        // dead end — dxgi again, said in so many words.
        var viewModel = await PreparedViewModel(new FolderAssetStore(Packages));
        viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
        var row = viewModel.Games[0];
        viewModel.Selected = row;
        await viewModel.InstallAsync(keepInstalledBuild: false);

        await viewModel.TryNextProxyAsync();
        Assert.Equal("d3d12.dll", row.Proxy);
        Assert.Equal("Switched to d3d12.dll. Start the game, then press INSERT.", viewModel.StatusMessage);

        await viewModel.TryNextProxyAsync();

        Assert.Equal("dxgi.dll", row.Proxy);
        Assert.Equal(
            "Every name has been tried once; starting again from dxgi.dll. Start the game, then press INSERT.",
            viewModel.StatusMessage);
        Assert.True(File.Exists(Path.Combine(Game, "dxgi.dll")));
        Assert.False(File.Exists(Path.Combine(Game, "d3d12.dll")));
        Assert.Empty(row.Entry.Record!.TriedProxies);
    }

    [Fact]
    public async Task A_game_that_gained_anti_cheat_since_the_scan_is_not_installed_without_asking()
    {
        var store = new FolderAssetStore(Packages);
        var viewModel = await PreparedViewModel(store);

        // Scanned clean, so the row has no warning and INSTALL shows no prompt...
        viewModel.ReplaceGames([Entry("Elden Ring", Game, installRoot: Game)]);
        viewModel.Selected = viewModel.Games[0];

        // ...and then a game update, with the launcher still open, brings anti-cheat in.
        Directory.CreateDirectory(Path.Combine(Game, "EasyAntiCheat"));

        var asked = false;
        viewModel.ConfirmAntiCheat = _ => { asked = true; return true; };

        await viewModel.InstallAsync(keepInstalledBuild: false);

        // Nobody was warned, so nobody consented: Core's own gate check has to refuse, rather
        // than install and record a consent the user never gave.
        Assert.False(asked);
        Assert.False(File.Exists(Path.Combine(Game, "dxgi.dll")));
        Assert.Null(InstallRecordStore.Load(Game));
        Assert.Contains("Anti-cheat", viewModel.StatusMessage);
    }

    [Fact]
    public async Task Closing_while_an_install_changes_the_game_folder_asks_first_and_No_keeps_the_window()
    {
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var store = new FolderAssetStore(Packages, hold: release.Task);
        var viewModel = await PreparedViewModel(store);
        viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
        viewModel.Selected = viewModel.Games[0];

        var asked = new List<string>();
        var answer = false;
        viewModel.ConfirmCloseMidChange = message => { asked.Add(message); return answer; };

        Assert.True(viewModel.MayClose());
        Assert.Empty(asked);

        // Held inside the Core call, which is the part a close would cut short.
        var install = viewModel.InstallAsync(keepInstalledBuild: false);
        await store.Entered.Task.WaitAsync(TimeSpan.FromSeconds(30));

        Assert.False(viewModel.MayClose());
        Assert.Contains("Test Game", Assert.Single(asked));

        // Their call to make once they have been told: a download that has stalled must not
        // leave Task Manager as the only way out.
        answer = true;
        Assert.True(viewModel.MayClose());

        release.SetResult();
        await install;

        asked.Clear();
        Assert.True(viewModel.MayClose());
        Assert.Empty(asked);
        Assert.True(File.Exists(Path.Combine(Game, "dxgi.dll")));
    }

    [Fact]
    public void Uninstall_and_TRY_NEXT_PROXY_keep_the_window_answering_and_say_what_they_are_doing()
    {
        RunOnDispatcher(async () =>
        {
            var uiThread = Environment.CurrentManagedThreadId;
            var store = new FolderAssetStore(Packages);
            var viewModel = await PreparedViewModel(store);
            viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
            viewModel.Selected = viewModel.Games[0];
            await viewModel.InstallAsync(keepInstalledBuild: false);

            var said = new List<string>();
            var raisedElsewhere = new List<string?>();
            viewModel.PropertyChanged += (_, e) =>
            {
                if (Environment.CurrentManagedThreadId != uiThread) raisedElsewhere.Add(e.PropertyName);
                else if (e.PropertyName == nameof(MainViewModel.StatusMessage)) said.Add(viewModel.StatusMessage);
            };

            // Queued before the command starts. It runs while the command is still busy only if
            // the Core call gave the UI thread back; run on the UI thread, the whole command
            // would finish first and this would find the launcher idle.
            var answeredMidSwitch = false;
            _ = Dispatcher.CurrentDispatcher.InvokeAsync(() => answeredMidSwitch = viewModel.Busy);
            await viewModel.TryNextProxyAsync();

            Assert.True(answeredMidSwitch);
            Assert.Equal(
                ["Switching to the next proxy DLL…", "Switched to d3d12.dll. Start the game, then press INSERT."],
                said);
            Assert.True(File.Exists(Path.Combine(Game, "d3d12.dll")));

            said.Clear();
            var answeredMidUninstall = false;
            _ = Dispatcher.CurrentDispatcher.InvokeAsync(() => answeredMidUninstall = viewModel.Busy);
            await viewModel.UninstallAsync();

            Assert.True(answeredMidUninstall);
            Assert.Equal(["Uninstalling…", "Uninstalled."], said);
            Assert.False(File.Exists(Path.Combine(Game, "d3d12.dll")));
            Assert.Empty(raisedElsewhere);
        });
    }

    [Fact]
    public void An_install_runs_off_the_UI_thread_reports_progress_on_it_and_then_shows_next_steps()
    {
        RunOnDispatcher(async () =>
        {
            var uiThread = Environment.CurrentManagedThreadId;
            var store = new FolderAssetStore(Packages);
            var viewModel = await PreparedViewModel(store);
            viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
            var row = viewModel.Games[0];
            viewModel.Selected = row;

            var progressOnUiThread = 0;
            var raisedElsewhere = new List<string?>();
            viewModel.PropertyChanged += (_, e) =>
            {
                if (Environment.CurrentManagedThreadId != uiThread) raisedElsewhere.Add(e.PropertyName);
                else if (e.PropertyName == nameof(MainViewModel.StatusMessage) &&
                         viewModel.StatusMessage.StartsWith("Installing files", StringComparison.Ordinal))
                    progressOnUiThread++;
            };
            row.PropertyChanged += (_, e) =>
            {
                if (Environment.CurrentManagedThreadId != uiThread) raisedElsewhere.Add("row." + e.PropertyName);
            };

            await viewModel.InstallAsync(keepInstalledBuild: false);

            // The Core call itself ran on the pool: the window keeps painting through a copy of
            // several hundred megabytes. Everything the bindings see still came back here.
            Assert.Equal([true], store.EnsuredOnPoolThread);
            Assert.Empty(raisedElsewhere);
            Assert.True(progressOnUiThread > 0);

            Assert.True(File.Exists(Path.Combine(Game, "dxgi.dll")));
            Assert.True(row.IsInstalled);
            Assert.Equal(
                "Installed as dxgi.dll. Start the game, then press INSERT to open the AMDNR menu. " +
                "Nothing changes? Use TRY NEXT PROXY.",
                row.NextSteps);
        });
    }

    [Fact]
    public async Task Uninstall_with_the_game_folder_unplugged_says_so_and_keeps_the_install()
    {
        // "Uninstalled." here was false twice over: nothing had been removed, and the record
        // Uninstall needs once the drive is back had been deleted.
        var viewModel = await PreparedViewModel(new FolderAssetStore(Packages));
        viewModel.ReplaceGames([Entry("Test Game", Game, installRoot: Game)]);
        viewModel.Selected = viewModel.Games[0];
        await viewModel.InstallAsync(keepInstalledBuild: false);
        Assert.NotNull(InstallRecordStore.Load(Game));

        Directory.Move(Game, Path.Combine(_root, "unplugged"));

        await viewModel.UninstallAsync();

        Assert.Equal("The game folder is not reachable (is its drive connected?). Nothing was changed.",
            viewModel.StatusMessage);
        Assert.NotNull(InstallRecordStore.Load(Game));
        Assert.True(viewModel.Games[0].IsInstalled);
    }

    [Fact]
    public void Next_steps_are_not_shown_for_a_game_nobody_installed_here()
    {
        Assert.Null(new GameRowViewModel(Entry("Stray", @"D:\Games\Stray")).NextSteps);
    }

    private async Task<MainViewModel> PreparedViewModel(IAssetStore store)
    {
        var settings = Settings.Load(Path.Combine(_root, "settings.json"));
        settings.Choice = new InstallChoice("amdnr", IncludeRuntime: false);

        var viewModel = new MainViewModel(settings, new StartupEnvironment(
            FetchManifest: (_, _) => Task.FromResult(Manifest),
            StageLauncherUpdate: (_, _) => Task.FromResult<string?>(null),
            Assets: store,
            ManifestCachePath: Path.Combine(_root, "manifest-cache.json"),
            // The rows look for each game's pictures once the launcher is prepared; whatever
            // they write — an exe's icon — goes here, never into the user's own AMDNR folder.
            ArtCacheRoot: Path.Combine(_root, "art")));

        await viewModel.PrepareAsync();
        return viewModel;
    }

    /// <summary>A WPF dispatcher on a thread of its own, which is what the window runs on: an
    /// await there comes back to that thread, and Progress&lt;T&gt; posts to it. A test thread
    /// from the pool has neither, so "on the UI thread" would mean nothing there.</summary>
    private static void RunOnDispatcher(Func<Task> test)
    {
        ExceptionDispatchInfo? failure = null;
        var thread = new Thread(() =>
        {
            var frame = new DispatcherFrame();
            Dispatcher.CurrentDispatcher.InvokeAsync(async () =>
            {
                try { await test(); }
                catch (Exception e) { failure = ExceptionDispatchInfo.Capture(e); }
                finally { frame.Continue = false; }
            });
            Dispatcher.PushFrame(frame);
            Dispatcher.CurrentDispatcher.InvokeShutdown();
        });
        thread.SetApartmentState(ApartmentState.STA);
        thread.Start();

        Assert.True(thread.Join(TimeSpan.FromMinutes(2)), "The install never finished on the dispatcher.");
        failure?.Throw();
    }

    /// <summary>Hands the installer a package already sitting in a folder, and notes which kind
    /// of thread asked for it: the installer asks first thing, so that is the Core call's thread.</summary>
    /// <remarks>With a hold, the install waits here until the test lets it go, so a test can
    /// look at the launcher while the Core call is part-way through.</remarks>
    private sealed class FolderAssetStore(string root, Task? hold = null) : IAssetStore
    {
        public List<bool> EnsuredOnPoolThread { get; } = [];

        /// <summary>Which kind of thread asked where a package is: the installer's ini seed
        /// reads that, so it is the Core call's thread for RESET INI.</summary>
        public List<bool> PathForOnPoolThread { get; } = [];

        public TaskCompletionSource Entered { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);

        public bool Has(PackageInfo package) => true;

        public string PathFor(PackageInfo package)
        {
            lock (PathForOnPoolThread) PathForOnPoolThread.Add(Thread.CurrentThread.IsThreadPoolThread);
            return Path.Combine(root, package.Id);
        }

        public async Task EnsureAsync(PackageInfo package, IProgress<DownloadProgress>? progress, CancellationToken ct)
        {
            lock (EnsuredOnPoolThread) EnsuredOnPoolThread.Add(Thread.CurrentThread.IsThreadPoolThread);
            Entered.TrySetResult();

            if (hold is not null) await hold;
        }
    }
}
