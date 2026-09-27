// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.App.ViewModels;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

public class MainViewModelTests
{
    private static GameEntry Entry(string name, string installRoot) => new(
        new GameCandidate(name, GameStore.Steam, installRoot),
        ExeDirectory: installRoot,
        ExeName: name + ".exe",
        Status: GameStatus.NotInstalled,
        Record: null,
        Report: null,
        Gate: null,
        RecommendedProxy: "dxgi.dll",
        ProxyReason: "first free name",
        OccupiedProxies: []);

    [Fact]
    public void A_rescan_keeps_the_game_the_user_had_selected()
    {
        // Refilling the list leaves Selected pointing at a row that is no longer in it, which
        // is how the detail panel empties on every rescan — the list view pushes null back
        // through the two-way binding the moment its selected item leaves the collection. The
        // selection has to be re-pointed at the row that replaced it.
        var viewModel = new MainViewModel();
        viewModel.ReplaceGames([Entry("Onimusha", @"D:\Games\Onimusha"), Entry("SH2", @"D:\Games\SH2")]);
        viewModel.Selected = viewModel.Games[1];

        viewModel.ReplaceGames([Entry("Onimusha", @"D:\Games\Onimusha"), Entry("SH2", @"D:\Games\SH2")]);

        Assert.Same(viewModel.Games[1], viewModel.Selected);
    }

    [Fact]
    public void A_selected_game_that_is_gone_after_a_rescan_leaves_nothing_selected()
    {
        var viewModel = new MainViewModel();
        viewModel.ReplaceGames([Entry("Onimusha", @"D:\Games\Onimusha")]);
        viewModel.Selected = viewModel.Games[0];

        viewModel.ReplaceGames([Entry("SH2", @"D:\Games\SH2")]);

        Assert.Null(viewModel.Selected);
        Assert.Single(viewModel.Games);
    }

    [Fact]
    public void A_rescan_keeps_the_DLL_name_the_user_picked_for_a_game_not_installed_yet()
    {
        // The pick lives on the row and a rescan builds new rows — including the rescan that
        // runs straight after installing some other game — so it went back to the recommendation
        // without a word. Carried across by install root, the way the selection is.
        var viewModel = new MainViewModel();
        viewModel.ReplaceGames([Entry("Onimusha", @"D:\Games\Onimusha"), Entry("SH2", @"D:\Games\SH2")]);
        viewModel.Games[1].ChosenProxy = "winmm.dll";

        viewModel.ReplaceGames([Entry("Onimusha", @"D:\Games\Onimusha"), Entry("SH2", @"D:\Games\SH2")]);

        Assert.Equal("dxgi.dll", viewModel.Games[0].Proxy);
        Assert.Equal("winmm.dll", viewModel.Games[1].Proxy);
        Assert.Equal(["winmm.dll"], viewModel.Games[1].ProxyOptions.Where(o => o.IsSelected).Select(o => o.Name));
    }

    private const string StagedUpdate = @"C:\Temp\AmdnrLauncher-0.4.1.exe";

    private static readonly LauncherInfo Update = new(
        "0.4.1",
        "https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/v0.4.1/AMDNR-Launcher.exe",
        "00");

    [Fact]
    public void The_update_is_not_applied_while_an_install_is_running()
    {
        // Applying ends in Application.Shutdown. Clicked while an install is copying into a
        // game folder, the process exits part-way through: our proxy placed, the game's own
        // files in AMDNR_backup, and no install record for uninstall to restore them from.
        var viewModel = new MainViewModel();
        viewModel.OfferUpdate(StagedUpdate, Update);
        viewModel.Busy = true;

        var applied = false;
        var message = viewModel.ApplyUpdate((_, _) => { applied = true; return null; });

        Assert.False(applied);
        Assert.False(viewModel.CanApplyUpdate);
        Assert.Contains("finish", message);
    }

    [Fact]
    public void The_update_is_applied_once_nothing_else_is_running()
    {
        var viewModel = new MainViewModel();
        viewModel.OfferUpdate(StagedUpdate, Update);

        Assert.True(viewModel.CanApplyUpdate);

        string? appliedPath = null;
        LauncherInfo? appliedUpdate = null;
        var message = viewModel.ApplyUpdate((path, update) =>
        {
            appliedPath = path;
            appliedUpdate = update;
            return "swap problem";
        });

        Assert.Equal(StagedUpdate, appliedPath);
        Assert.Same(Update, appliedUpdate);
        Assert.Equal("swap problem", message);
    }

    [Fact]
    public void Restart_as_administrator_is_greyed_while_anything_is_running()
    {
        // Restarting ends in Application.Shutdown, the same as the update: while an install
        // copies into one game, the button can be on screen for another that is not writable.
        var viewModel = new MainViewModel();
        Assert.True(viewModel.CanRestartElevated);

        var raised = new List<string?>();
        viewModel.PropertyChanged += (_, e) => raised.Add(e.PropertyName);

        viewModel.Busy = true;
        Assert.False(viewModel.CanRestartElevated);
        Assert.Contains(nameof(MainViewModel.CanRestartElevated), raised);

        viewModel.Busy = false;
        Assert.True(viewModel.CanRestartElevated);
    }

    [Fact]
    public void The_first_run_chooser_and_a_scan_do_not_hold_the_window_open()
    {
        // Both are Busy, and neither touches a game folder: stopping a scan or a resumable
        // download costs nothing, and the first-run chooser must never trap anyone.
        var viewModel = new MainViewModel();
        var asked = false;
        viewModel.ConfirmCloseMidChange = _ => { asked = true; return false; };

        viewModel.Busy = true;

        Assert.True(viewModel.MayClose());
        Assert.False(asked);
    }

    [Fact]
    public void The_update_button_comes_back_when_the_install_finishes()
    {
        // The banner's button is bound to CanApplyUpdate, which is computed from Busy: without
        // a change notification of its own it would stay greyed out after the install ends.
        var viewModel = new MainViewModel();
        viewModel.OfferUpdate(StagedUpdate, Update);
        viewModel.Busy = true;
        Assert.Contains("finish", viewModel.UpdateBanner);

        var raised = new List<string?>();
        viewModel.PropertyChanged += (_, e) => raised.Add(e.PropertyName);
        viewModel.Busy = false;

        Assert.True(viewModel.CanApplyUpdate);
        Assert.Contains(nameof(MainViewModel.CanApplyUpdate), raised);
        Assert.Contains(nameof(MainViewModel.UpdateBanner), raised);
        Assert.DoesNotContain("finish", viewModel.UpdateBanner);
    }
}
