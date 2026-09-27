// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class AntiCheatDetectorTests
{
    [Fact]
    public void Detects_EasyAntiCheat_directory()
    {
        using var dir = new TempDir();
        Directory.CreateDirectory(Path.Combine(dir.Path, "EasyAntiCheat"));

        Assert.Equal(["EasyAntiCheat"], AntiCheatDetector.Detect(dir.Path));
    }

    [Fact]
    public void Detects_BattlEye_directory()
    {
        using var dir = new TempDir();
        Directory.CreateDirectory(Path.Combine(dir.Path, "BattlEye"));

        Assert.Equal(["BattlEye"], AntiCheatDetector.Detect(dir.Path));
    }

    [Fact]
    public void Detects_a_loose_anticheat_binary()
    {
        using var dir = new TempDir();
        dir.Write("EasyAntiCheat_EOS.dll", "x");

        Assert.Equal(["EasyAntiCheat"], AntiCheatDetector.Detect(dir.Path));
    }

    [Fact]
    public void Detects_Vanguard()
    {
        using var dir = new TempDir();
        dir.Write("vgk.sys", "x");

        Assert.Equal(["Vanguard"], AntiCheatDetector.Detect(dir.Path));
    }

    [Fact]
    public void Reports_nothing_for_a_clean_folder()
    {
        using var dir = new TempDir();
        dir.Write("Game.exe", "x");
        dir.Write("dxgi.dll", "x");

        Assert.Empty(AntiCheatDetector.Detect(dir.Path));
    }

    [Fact]
    public void Detects_a_runtime_that_sits_beside_the_exe_directory_not_in_it()
    {
        // The extremely common Unreal layout: the exe lives in <root>\Binaries\Win64\ and the
        // anti-cheat runtime in <root>\EasyAntiCheat\. Scanning the exe directory alone finds
        // nothing, and the one gate between a one-click installer and a banned account never
        // fires. Spec §7 step 1 says the target *tree*.
        using var dir = new TempDir();
        var exeDirectory = Path.Combine(dir.Path, "Binaries", "Win64");
        Directory.CreateDirectory(exeDirectory);
        Directory.CreateDirectory(Path.Combine(dir.Path, "EasyAntiCheat"));

        Assert.Equal(["EasyAntiCheat"], AntiCheatDetector.Detect(exeDirectory, dir.Path));
    }

    [Fact]
    public void Detects_a_runtime_a_couple_of_levels_under_the_install_root()
    {
        using var dir = new TempDir();
        Directory.CreateDirectory(Path.Combine(dir.Path, "Engine", "Binaries", "BattlEye"));

        Assert.Equal(["BattlEye"], AntiCheatDetector.Detect(dir.Path, dir.Path));
    }

    [Fact]
    public void Does_not_walk_the_whole_tree_below_the_install_root()
    {
        // The walk is bounded on purpose: this runs for every candidate on every scan, and an
        // install root is one bad scanner result away from being a whole Steam library.
        using var dir = new TempDir();
        Directory.CreateDirectory(Path.Combine(dir.Path, "a", "b", "c", "d", "EasyAntiCheat"));

        Assert.Empty(AntiCheatDetector.Detect(dir.Path, dir.Path));
    }

    [Fact]
    public void A_directory_it_cannot_enumerate_is_not_a_crash()
    {
        Assert.Empty(AntiCheatDetector.Detect(@"Z:\no\such\folder", @"Z:\no\such"));
    }

    [Fact]
    public void The_Ricochet_driver_one_folder_above_a_hand_picked_subfolder_is_found()
    {
        // Call of Duty MW3 from the Xbox app keeps its campaign exe in Content\sp23, and
        // Randgrid.sys one level up in Content. "Add game folder…" on sp23 makes sp23 both the
        // exe folder and the install root, and nothing named randgrid or ricochet is inside it.
        using var dir = new TempDir();
        var content = Path.Combine(dir.Path, "XboxGames", "Call of Duty- Modern Warfare 3", "Content");
        var sp23 = Path.Combine(content, "sp23");
        Directory.CreateDirectory(sp23);
        File.WriteAllText(Path.Combine(content, "Randgrid.sys"), "driver");
        File.WriteAllText(Path.Combine(sp23, "sp23-cod.exe"), "game");

        Assert.Equal(["Ricochet"], AntiCheatDetector.Detect(sp23, sp23));
        Assert.Equal(["Ricochet"], AntiCheatDetector.Detect(Path.Combine(sp23, "bin", "x64")));
    }

    [Fact]
    public void The_walk_above_the_exe_folder_is_bounded()
    {
        // It runs for every candidate on every scan, and a driver many folders up is not the
        // game's own.
        using var dir = new TempDir();
        dir.Write("Randgrid.sys", "driver");
        var deep = Path.Combine(dir.Path, "a", "b", "c", "d", "e", "f");
        Directory.CreateDirectory(deep);

        Assert.Empty(AntiCheatDetector.Detect(deep, deep));
    }

    [Fact]
    public void Does_not_report_the_same_product_twice()
    {
        using var dir = new TempDir();
        Directory.CreateDirectory(Path.Combine(dir.Path, "EasyAntiCheat"));
        dir.Write("EasyAntiCheat_EOS.dll", "x");

        Assert.Single(AntiCheatDetector.Detect(dir.Path));
    }
}
