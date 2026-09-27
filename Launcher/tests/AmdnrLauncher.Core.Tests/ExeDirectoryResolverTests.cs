// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class ExeDirectoryResolverTests
{
    private static void Exe(TempDir dir, string relativePath, int sizeBytes = 1024)
        => dir.Write(relativePath, new byte[sizeBytes]);

    [Fact]
    public void Single_exe_in_the_root_wins()
    {
        using var dir = new TempDir();
        Exe(dir, @"Game\Onimusha.exe");

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "Game"));

        Assert.Equal(Path.Combine(dir.Path, "Game"), r.ExeDirectory);
        Assert.Equal("Onimusha.exe", r.ExeName);
        Assert.Empty(r.Ambiguous);
    }

    [Fact]
    public void Crash_handlers_and_redists_do_not_count_as_candidates()
    {
        using var dir = new TempDir();
        Exe(dir, @"Game\Onimusha.exe");
        Exe(dir, @"Game\UnrealCrashReportClient.exe");
        Exe(dir, @"Game\vcredist_x64.exe");
        Exe(dir, @"Game\unins000.exe");

        Assert.Equal("Onimusha.exe", ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "Game")).ExeName);
    }

    [Fact]
    public void Falls_through_to_Binaries_Win64_under_the_project_folder_from_the_Steam_root()
    {
        using var dir = new TempDir();
        // Steam hands over "SILENT HILL 2", not the project folder: the root holds a 329 KB stub
        // that only starts the real payload two levels down.
        Exe(dir, @"SILENT HILL 2\SHProto.exe", 329);
        Exe(dir, @"SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe", 142_127);
        Exe(dir, @"SILENT HILL 2\SHProto\Binaries\Win64\EpicWebHelper.exe", 64);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "SILENT HILL 2"));

        Assert.Equal(Path.Combine(dir.Path, "SILENT HILL 2", "SHProto", "Binaries", "Win64"), r.ExeDirectory);
        Assert.Equal("SHProto-Win64-Shipping.exe", r.ExeName);
    }

    [Fact]
    public void A_non_Unreal_Binaries_Win64_still_takes_the_largest_exe()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\Binaries\Win64\Game.exe", 4096);
        Exe(dir, @"G\Binaries\Win64\Tool.exe", 64);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G", "Binaries", "Win64"), r.ExeDirectory);
        Assert.Equal("Game.exe", r.ExeName);
    }

    // Sizes below are the real layouts on the reference PC scaled 1/1000, so the ratios the
    // rules depend on are the ones a real scan meets.

    [Fact]
    public void SILENT_HILL_2_resolves_to_SHProto_Binaries_Win64_not_the_root_stub()
    {
        using var dir = new TempDir();
        Exe(dir, @"SILENT HILL 2\SHProto.exe", 329);
        Exe(dir, @"SILENT HILL 2\Engine\Binaries\Win64\CrashReportClient.exe", 22_913);
        Exe(dir, @"SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe", 142_127);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "SILENT HILL 2"));

        Assert.Equal(Path.Combine(dir.Path, "SILENT HILL 2", "SHProto", "Binaries", "Win64"), r.ExeDirectory);
        Assert.Equal("SHProto-Win64-Shipping.exe", r.ExeName);
        Assert.Empty(r.Ambiguous);
    }

    [Fact]
    public void Stray_resolves_to_Hk_project_even_though_the_project_folder_is_not_named_after_the_exe()
    {
        using var dir = new TempDir();
        Exe(dir, @"Stray\Stray.exe", 530);
        Exe(dir, @"Stray\Hk_project\Binaries\Win64\Stray-Win64-Shipping.exe", 85_250);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "Stray"));

        Assert.Equal(Path.Combine(dir.Path, "Stray", "Hk_project", "Binaries", "Win64"), r.ExeDirectory);
        Assert.Equal("Stray-Win64-Shipping.exe", r.ExeName);
    }

    [Fact]
    public void Spider_Man_resolves_to_its_root_despite_two_crash_reporters_and_a_leftover_setup()
    {
        using var dir = new TempDir();
        Exe(dir, @"SM\Spider-Man.exe", 121_901);
        Exe(dir, @"SM\crs-handler.exe", 1_247);
        Exe(dir, @"SM\crs-video.exe", 1_437);
        Exe(dir, @"SM\dlssnr_on_amd_setup_v0.3.1-re-engine-test.exe", 7_597);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "SM"));

        Assert.Equal(Path.Combine(dir.Path, "SM"), r.ExeDirectory);
        Assert.Equal("Spider-Man.exe", r.ExeName);
        Assert.Empty(r.Ambiguous);
    }

    [Fact]
    public void A_largest_exe_ten_times_the_next_wins_in_a_multi_exe_root()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\Game.exe", 10_000);
        Exe(dir, @"G\Companion.exe", 1_000);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G"), r.ExeDirectory);
        Assert.Equal("Game.exe", r.ExeName);
    }

    [Fact]
    public void A_modding_tool_beside_the_game_does_not_keep_the_game_exe_from_winning()
    {
        // The owner's GTA V folder, sizes as found (scaled): the game at 47 MB and RAGE Plugin
        // Hook at 12 MB — under the ten-times rule no decision, and the row read "(executable
        // folder not identified)". The hook is a modding tool, never the game, and is not a
        // candidate; BattlEye's wrapper, the launcher stubs and the uninstaller are small or excluded.
        using var dir = new TempDir();
        Exe(dir, @"G\GTA5.exe", 4_746_712);
        Exe(dir, @"G\RAGEPluginHook.exe", 1_222_489);
        Exe(dir, @"G\GTA5_BE.exe", 147_314);
        Exe(dir, @"G\PlayGTAV.exe", 57_253);
        Exe(dir, @"G\GTAVLauncher.exe", 57_253);
        Exe(dir, @"G\GTAVLanguageSelect.exe", 57_253);
        Exe(dir, @"G\uninstall.exe", 102_360);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G"), r.ExeDirectory);
        Assert.Equal("GTA5.exe", r.ExeName);
    }

    [Fact]
    public void Two_similar_sized_exes_stay_ambiguous()
    {
        using var dir = new TempDir();
        // Five times bigger is not decisive: a DX11 and a DX12 build can differ that much.
        Exe(dir, @"G\GameDX12.exe", 5_000);
        Exe(dir, @"G\GameDX11.exe", 1_000);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Null(r.ExeDirectory);
        Assert.Equal(2, r.Ambiguous.Count);
    }

    [Fact]
    public void Engine_Binaries_are_never_chosen()
    {
        using var dir = new TempDir();
        // Bigger than the game's own shipping exe, and matching the pattern: still engine tooling.
        Exe(dir, @"G\Engine\Binaries\Win64\UnrealTool-Win64-Shipping.exe", 50_000);
        Exe(dir, @"G\Engine\Binaries\Win64\UnrealCEFSubProcess.exe", 40_000);
        Exe(dir, @"G\Proj\Binaries\Win64\Proj-Win64-Shipping.exe", 10_000);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G", "Proj", "Binaries", "Win64"), r.ExeDirectory);
        Assert.Equal("Proj-Win64-Shipping.exe", r.ExeName);
    }

    [Fact]
    public void Engine_exes_are_not_offered_in_the_ambiguous_pick_list()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\GameDX11.exe");
        Exe(dir, @"G\GameDX12.exe");
        Exe(dir, @"G\Engine\Binaries\Win64\UnrealCEFSubProcess.exe");

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Null(r.ExeDirectory);
        Assert.Equal(["GameDX11.exe", "GameDX12.exe"], r.Ambiguous.Select(p => Path.GetFileName(p)).Order());
    }

    [Fact]
    public void Exclusions_match_after_removing_punctuation()
    {
        using var dir = new TempDir();
        // Same size on purpose, so only the exclusions, not the size rule, can decide this.
        Exe(dir, @"G\Game.exe");
        Exe(dir, @"G\VC_redist.x64.exe");
        Exe(dir, @"G\crs-handler.exe");
        Exe(dir, @"G\crs-video.exe");
        Exe(dir, @"G\gamelaunchhelper.exe");

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G"), r.ExeDirectory);
        Assert.Equal("Game.exe", r.ExeName);
    }

    [Fact]
    public void Several_shipping_exes_prefer_the_one_named_like_a_root_exe()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\Proj.exe", 300);
        Exe(dir, @"G\Proj\Binaries\Win64\Proj-Win64-Shipping.exe", 10_000);
        Exe(dir, @"G\Bonus\Binaries\Win64\Bonus-Win64-Shipping.exe", 90_000);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G", "Proj", "Binaries", "Win64"), r.ExeDirectory);
        Assert.Equal("Proj-Win64-Shipping.exe", r.ExeName);
    }

    [Fact]
    public void Several_shipping_exes_with_no_root_name_match_take_the_largest()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\Proj\Binaries\Win64\Proj-Win64-Shipping.exe", 10_000);
        Exe(dir, @"G\Bonus\Binaries\Win64\Bonus-Win64-Shipping.exe", 90_000);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G", "Bonus", "Binaries", "Win64"), r.ExeDirectory);
        Assert.Equal("Bonus-Win64-Shipping.exe", r.ExeName);
    }

    [Fact]
    public void A_WinGDK_shipping_exe_is_found_where_Unreal_puts_it()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\gamelaunchhelper.exe", 300);
        Exe(dir, @"G\Proj\Binaries\WinGDK\Proj-WinGDK-Shipping.exe", 10_000);

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Equal(Path.Combine(dir.Path, "G", "Proj", "Binaries", "WinGDK"), r.ExeDirectory);
        Assert.Equal("Proj-WinGDK-Shipping.exe", r.ExeName);
    }

    [Fact]
    public void Sub_path_order_is_honoured()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\Binaries\Win64\Right.exe", 100);
        Exe(dir, @"G\bin\Wrong.exe", 9999);

        Assert.Equal("Right.exe", ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G")).ExeName);
    }

    [Fact]
    public void Two_plausible_exes_in_the_root_are_reported_as_ambiguous()
    {
        using var dir = new TempDir();
        Exe(dir, @"G\GameDX11.exe");
        Exe(dir, @"G\GameDX12.exe");

        var r = ExeDirectoryResolver.Resolve(Path.Combine(dir.Path, "G"));

        Assert.Null(r.ExeDirectory);
        Assert.Equal(2, r.Ambiguous.Count);
        Assert.All(r.Ambiguous, p => Assert.True(File.Exists(p)));
    }

    [Fact]
    public void A_missing_install_root_resolves_to_nothing()
    {
        var r = ExeDirectoryResolver.Resolve(@"Z:\no\such\game");

        Assert.Null(r.ExeDirectory);
        Assert.Empty(r.Ambiguous);
    }
}
