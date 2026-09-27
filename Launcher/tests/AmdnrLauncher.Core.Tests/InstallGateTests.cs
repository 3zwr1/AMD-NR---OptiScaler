// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class InstallGateTests
{
    [Fact]
    public void Rejects_a_WindowsApps_path()
    {
        var result = InstallGate.Evaluate(@"C:\Program Files\WindowsApps\SomePublisher.Game_1.0\Game");

        var finding = Assert.Single(result.Findings, f => f.Code == GateCode.WindowsAppsRejected);
        Assert.True(finding.Blocking);
        Assert.True(result.IsHardBlocked);
    }

    [Fact]
    public void Accepts_an_XboxGames_content_path()
    {
        // XboxGames\<game>\Content is writable and installs work there; only
        // WindowsApps is off limits.
        using var dir = new TempDir();
        var target = Path.Combine(dir.Path, "XboxGames", "Forza Horizon 5", "Content");
        Directory.CreateDirectory(target);

        var result = InstallGate.Evaluate(target);

        Assert.DoesNotContain(result.Findings, f => f.Code == GateCode.WindowsAppsRejected);
        Assert.False(result.IsHardBlocked);
    }

    [Fact]
    public void Flags_a_directory_that_cannot_be_written()
    {
        var result = InstallGate.Evaluate(@"Z:\definitely\not\here");

        Assert.Contains(result.Findings, f => f.Code == GateCode.NotWritable && f.Blocking);
    }

    [Fact]
    public void Anticheat_is_blocking_but_consentable()
    {
        using var dir = new TempDir();
        Directory.CreateDirectory(Path.Combine(dir.Path, "BattlEye"));

        var result = InstallGate.Evaluate(dir.Path);

        var finding = Assert.Single(result.Findings, f => f.Code == GateCode.AntiCheatDetected);
        Assert.True(finding.Blocking);
        Assert.Contains("ban", finding.Message, StringComparison.OrdinalIgnoreCase);
        Assert.False(result.CanProceedWithoutConsent);
        Assert.False(result.IsHardBlocked);          // the user may still confirm
    }

    [Fact]
    public void The_anti_cheat_finding_names_each_system_it_found()
    {
        // The launcher's warning names the game and the system in one sentence, and it cannot
        // take the names back out of a message worded for somebody else.
        using var dir = new TempDir();
        Directory.CreateDirectory(Path.Combine(dir.Path, "BattlEye"));
        Directory.CreateDirectory(Path.Combine(dir.Path, "EasyAntiCheat"));

        var result = InstallGate.Evaluate(dir.Path);

        var finding = Assert.Single(result.Findings, f => f.Code == GateCode.AntiCheatDetected);
        Assert.Equal(["EasyAntiCheat", "BattlEye"], finding.Products);
    }

    /// <summary>The words the user reads instead of a prompt, held here word for word.</summary>
    private const string RicochetWords =
        "Call of Duty's Ricochet anti-cheat bans accounts for injected DLLs, and these games have no " +
        "offline mode where that is safe. AMDNR will not install here.";

    [Fact]
    public void Ricochet_blocks_the_install()
    {
        // Call of Duty MW3 from the Xbox app: its kernel driver sits beside the game's exe.
        using var dir = new TempDir();
        dir.Write("cod23-cod.exe", "x");
        dir.Write("Randgrid.sys", "x");

        var result = InstallGate.Evaluate(dir.Path);

        var finding = Assert.Single(result.Findings, f => f.Code == GateCode.AntiCheatBlocked);
        Assert.True(finding.Blocking);
        Assert.Equal(RicochetWords, finding.Message);
        Assert.Equal(["Ricochet"], finding.Products);

        // Never a question the user can answer yes to.
        Assert.True(result.IsHardBlocked);
        Assert.DoesNotContain(result.Findings, f => f.Code == GateCode.AntiCheatDetected);
    }

    [Theory]
    [InlineData("EAAntiCheat", true, "EA Javelin")]
    [InlineData("GameGuard", true, "nProtect GameGuard")]
    [InlineData("XIGNCODE", true, "XIGNCODE")]
    [InlineData("equ8.dll", false, "EQU8")]
    [InlineData("mhyprot2.sys", false, "mhyprot")]
    [InlineData("pnkbstrA.exe", false, "PunkBuster")]
    public void EA_Javelin_and_the_other_new_systems_warn_like_EasyAntiCheat(string marker, bool isFolder, string product)
    {
        using var dir = new TempDir();
        if (isFolder) Directory.CreateDirectory(Path.Combine(dir.Path, marker));
        else dir.Write(marker, "x");

        var result = InstallGate.Evaluate(dir.Path);

        var finding = Assert.Single(result.Findings, f => f.Code == GateCode.AntiCheatDetected);
        Assert.Equal([product], finding.Products);
        Assert.False(result.IsHardBlocked);
    }

    [Fact]
    public void A_clean_writable_folder_produces_no_findings()
    {
        using var dir = new TempDir();

        var result = InstallGate.Evaluate(dir.Path);

        Assert.Empty(result.Findings);
        Assert.True(result.CanProceedWithoutConsent);
    }

    [Fact]
    public void The_writability_probe_leaves_nothing_behind()
    {
        using var dir = new TempDir();

        InstallGate.Evaluate(dir.Path);

        Assert.Empty(Directory.EnumerateFileSystemEntries(dir.Path));
    }

    [Fact]
    public void Flags_a_process_running_from_the_target_directory()
    {
        // The current test host is, by definition, a process running from its own
        // directory — use it as the stand-in for a running game.
        var hostDirectory = AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar);

        var result = InstallGate.Evaluate(hostDirectory);

        Assert.Contains(result.Findings, f => f.Code == GateCode.GameRunning && f.Blocking);
    }
}
