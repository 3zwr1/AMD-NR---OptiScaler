// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

/// <summary>Capcom's RE Engine games close themselves after launch with a mod loaded unless
/// REFramework is installed; the launcher says so before INSTALL and in the Doctor, and never
/// downloads REFramework itself.</summary>
public sealed class ReEngineTests : IDisposable
{
    private readonly TempDir _dir = new();

    public void Dispose() => _dir.Dispose();

    private string ReEngineGame(string name)
    {
        var folder = Path.Combine(_dir.Path, name);
        Directory.CreateDirectory(folder);
        File.WriteAllText(Path.Combine(folder, "re9.exe"), "game");
        File.WriteAllText(Path.Combine(folder, "re_chunk_000.pak"), "archive");
        return folder;
    }

    [Fact]
    public void An_RE_Engine_game_without_REFramework_gets_the_warning_as_a_non_blocking_gate_finding()
    {
        var folder = ReEngineGame("RE Requiem");

        var gate = InstallGate.Evaluate(folder);

        var finding = Assert.Single(gate.Findings, f => f.Code == GateCode.ReframeworkMissing);
        Assert.False(finding.Blocking);
        Assert.Equal(ReEngine.MissingReframework, finding.Message);
        Assert.Contains("15–60 seconds", finding.Message);
        Assert.Contains("https://github.com/praydog/REFramework-nightly/releases", finding.Message);
        Assert.True(gate.CanProceedWithoutConsent);
        Assert.False(gate.IsHardBlocked);
    }

    [Fact]
    public void REFramework_is_recognised_by_its_size_or_by_what_it_writes_beside_itself()
    {
        // A copy never run: only the DLL, 23 MB where any other dinput8 proxy is a few hundred KB.
        var fresh = ReEngineGame("Fresh");
        File.WriteAllBytes(Path.Combine(fresh, "dinput8.dll"), new byte[5 * 1024 * 1024]);
        Assert.True(ReEngine.HasReframework(fresh));
        Assert.Null(ReEngine.MissingReframeworkWarning(fresh));
        Assert.DoesNotContain(InstallGate.Evaluate(fresh).Findings, f => f.Code == GateCode.ReframeworkMissing);

        // A copy that has run: its folder and logs, as in the owner's RE Requiem install.
        var used = ReEngineGame("Used");
        File.WriteAllText(Path.Combine(used, "dinput8.dll"), "small stand-in");
        Directory.CreateDirectory(Path.Combine(used, "reframework"));
        Assert.True(ReEngine.HasReframework(used));

        var logged = ReEngineGame("Logged");
        File.WriteAllText(Path.Combine(logged, "dinput8.dll"), "small stand-in");
        File.WriteAllText(Path.Combine(logged, "re2_framework_log.txt"), "log");
        Assert.True(ReEngine.HasReframework(logged));

        // Somebody else's small dinput8 proxy is not REFramework.
        var other = ReEngineGame("Other");
        File.WriteAllText(Path.Combine(other, "dinput8.dll"), "another mod's proxy");
        Assert.False(ReEngine.HasReframework(other));
        Assert.Equal(ReEngine.MissingReframework, ReEngine.MissingReframeworkWarning(other));
    }

    [Fact]
    public void A_game_on_another_engine_is_not_warned()
    {
        var folder = Path.Combine(_dir.Path, "Stray");
        Directory.CreateDirectory(folder);
        File.WriteAllText(Path.Combine(folder, "Stray.exe"), "game");

        Assert.False(ReEngine.IsReEngineGame(folder));
        Assert.Null(ReEngine.MissingReframeworkWarning(folder));
        Assert.DoesNotContain(InstallGate.Evaluate(folder).Findings, f => f.Code == GateCode.ReframeworkMissing);
    }

    [Fact]
    public void The_owners_RE_Requiem_install_has_REFramework_when_it_is_on_this_PC()
    {
        // Read-only: the folder the crash was first seen and fixed in. Skipped elsewhere.
        const string requiem = @"E:\SteamLibrary\steamapps\common\RESIDENT EVIL requiem BIOHAZARD requiem";
        if (!Directory.Exists(requiem)) return;

        Assert.True(ReEngine.IsReEngineGame(requiem));
        Assert.True(ReEngine.HasReframework(requiem));
        Assert.Null(ReEngine.MissingReframeworkWarning(requiem));
    }

    [Fact]
    public void The_launcher_introduces_itself_to_every_host()
    {
        using var client = LauncherHttp.Create("0.3.4");

        Assert.Equal("AMD-NR-Launcher/0.3.4 (+https://github.com/3zwr1/AMD-NR---OptiScaler)", LauncherHttp.UserAgent("0.3.4"));
        Assert.Equal(LauncherHttp.UserAgent("0.3.4"), client.DefaultRequestHeaders.UserAgent.ToString());
    }
}
