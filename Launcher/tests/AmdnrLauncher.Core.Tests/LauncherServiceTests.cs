// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Diagnostics;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;
using AmdnrLauncher.Core.State;

namespace AmdnrLauncher.Core.Tests;

public class LauncherServiceTests : IDisposable
{
    private readonly TempDir _dir = new();
    private readonly string _originalRecordRoot = InstallRecordStore.RootDirectory;
    private readonly string _originalDbPath = InstalledGamesDb.FilePath;

    public LauncherServiceTests()
    {
        InstallRecordStore.RootDirectory = Path.Combine(_dir.Path, "records");
        InstalledGamesDb.FilePath = Path.Combine(_dir.Path, "games.json");
    }

    public void Dispose()
    {
        InstallRecordStore.RootDirectory = _originalRecordRoot;
        InstalledGamesDb.FilePath = _originalDbPath;
        _dir.Dispose();
    }

    private sealed class StubScanner(params GameCandidate[] games) : IGameScanner
    {
        public IEnumerable<GameCandidate> Scan() => games;
    }

    private static readonly InstallChoice Amdnr = new("amdnr", IncludeRuntime: true);

    /// <summary>This PC, as the display class key describes it.</summary>
    private static readonly GpuInfo Rdna4 =
        new("AMD Radeon RX 9070 XT", "rdna4", "32.0.31041.3013", @"PCI\VEN_1002&DEV_7550&REV_C0");

    private static readonly GpuInfo Rdna3 = new("AMD Radeon RX 7900 XTX", "rdna3", "32.0.21001.1", null);

    private static string[] Ids(IReadOnlyList<PackageInfo> packages) => [.. packages.Select(p => p.Id)];

    // Reuses the same fake asset layout as InstallerTests.
    private LauncherService MakeService(params GameCandidate[] games)
        => MakeServiceHiding(null, games);

    private LauncherService MakeServiceHiding(
        IReadOnlyList<string>? hiddenSteamAppIds, params GameCandidate[] games)
    {
        var store = new StubAssetStore(Path.Combine(_dir.Path, "assets"));
        var manifest = new Manifest(
            1,
            new LauncherInfo("1.0.0", "https://h/l.exe", "AA"),
            [store.Amdnr, store.TheAutomatic, store.Runtime, store.Forwarder],
            [new CompatibilityRule("0.2.1", ["0.3.1"])],
            PayloadNames.ProxyNames,
            [],
            Sources: StubAssetStore.Sources,
            HiddenSteamAppIds: hiddenSteamAppIds);

        return new LauncherService(manifest, store, new Installer(store, new CopyFilePlacer()),
            [new StubScanner(games)], Rdna4);
    }

    /// <summary>The manifest as published for 0.3.3.1: runtime-033 for every generation but
    /// rdna1, runtime-040 for rdna4 — in that order, or the other way round when the owner has
    /// made 0.4.0 the RDNA 4 default by reordering it.</summary>
    private LauncherService MakeServiceWithRuntimes(GpuInfo gpu, bool zeroFourZeroFirst, params GameCandidate[] games)
    {
        var store = new StubAssetStore(Path.Combine(_dir.Path, "assets"));
        var runtime033 = store.RuntimeNamed("runtime-033", "0.3.3");
        var runtime040 = store.RuntimeNamed("runtime-040", "0.4.0");

        IReadOnlyList<RuntimeChoice> runtimes =
        [
            new("runtime-033", ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"]),
            new("runtime-040", ["rdna4"]),
        ];
        if (zeroFourZeroFirst) runtimes = runtimes.Reverse().ToList();

        var manifest = new Manifest(
            1,
            new LauncherInfo("1.0.0", "https://h/l.exe", "AA"),
            [store.Amdnr, store.TheAutomatic, runtime033, runtime040, store.Forwarder],
            [],
            PayloadNames.ProxyNames,
            [],
            Sources: StubAssetStore.Sources.Select(s => s with { Runtimes = runtimes }).ToList());

        return new LauncherService(manifest, store, new Installer(store, new CopyFilePlacer()),
            [new StubScanner(games)], gpu);
    }

    private string MakeGameFolder(string name)
    {
        var root = Path.Combine(_dir.Path, name);
        Directory.CreateDirectory(root);
        File.WriteAllText(Path.Combine(root, name + ".exe"), "game");
        return root;
    }

    [Fact]
    public void Scan_resolves_the_exe_directory_and_recommends_a_proxy()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));

        var entry = service.Scan().Single();

        Assert.Equal(root, entry.ExeDirectory);
        Assert.Equal("Onimusha.exe", entry.ExeName);
        Assert.Equal("dxgi.dll", entry.RecommendedProxy);
        Assert.Equal(GameStatus.NotInstalled, entry.Status);
    }

    [Fact]
    public void Scan_hides_Steam_tools_and_manifest_listed_appids()
    {
        var service = MakeServiceHiding(
            ["4242"],
            new GameCandidate("Steamworks Common Redistributables", GameStore.Steam,
                MakeGameFolder("Redist"), "228980"),
            new GameCandidate("SteamVR", GameStore.Steam, MakeGameFolder("SteamVR"), "250820"),
            new GameCandidate("Some Benchmark", GameStore.Steam, MakeGameFolder("Bench"), "4242"),
            new GameCandidate("Onimusha", GameStore.Steam, MakeGameFolder("Onimusha"), "2206210"));

        Assert.Equal(["Onimusha"], service.Scan().Select(e => e.Candidate.Name));
    }

    [Fact]
    public void A_WindowsApps_game_is_marked_Unsupported()
    {
        var service = MakeService(new GameCandidate(
            "StoreGame", GameStore.Steam, @"C:\Program Files\WindowsApps\Pub.Game_1.0"));

        Assert.Equal(GameStatus.Unsupported, service.Scan().Single().Status);
    }

    [Fact]
    public async Task Install_then_refresh_reports_Installed()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = service.Scan().Single();

        var installed = await service.InstallAsync(entry, "dxgi.dll", Amdnr, false, null, default);

        Assert.Equal(GameStatus.Installed, installed.Status);
        Assert.NotNull(installed.Record);
        Assert.True(File.Exists(Path.Combine(root, "dxgi.dll")));
    }

    [Fact]
    public async Task Install_refuses_a_hard_blocked_target()
    {
        var service = MakeService(new GameCandidate(
            "StoreGame", GameStore.Steam, @"C:\Program Files\WindowsApps\Pub.Game_1.0"));
        var entry = service.Scan().Single();

        await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.InstallAsync(entry, "dxgi.dll", Amdnr, false, null, default));
    }

    [Fact]
    public async Task Install_refuses_an_unacknowledged_anticheat_game()
    {
        var root = MakeGameFolder("Shooter");
        Directory.CreateDirectory(Path.Combine(root, "EasyAntiCheat"));
        var service = MakeService(new GameCandidate("Shooter", GameStore.Steam, root));
        var entry = service.Scan().Single();

        await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.InstallAsync(entry, "dxgi.dll", Amdnr, antiCheatAcknowledged: false, null, default));

        // The same install succeeds once the user has confirmed.
        var installed = await service.InstallAsync(entry, "dxgi.dll", Amdnr, true, null, default);
        Assert.Equal(GameStatus.Installed, installed.Status);
    }

    [Fact]
    public async Task A_Ricochet_game_is_not_supported_and_refuses_even_an_acknowledged_install()
    {
        var root = MakeGameFolder("CoD");
        File.WriteAllText(Path.Combine(root, "Randgrid.sys"), "driver");
        var service = MakeService(new GameCandidate("Call of Duty", GameStore.Steam, root));
        var entry = service.Scan().Single();

        Assert.Equal(GameStatus.Unsupported, entry.Status);

        var refused = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.InstallAsync(entry, "dxgi.dll", Amdnr, antiCheatAcknowledged: true, null, default));
        Assert.Contains("Ricochet", refused.Message);
        Assert.False(File.Exists(Path.Combine(root, "dxgi.dll")));
    }

    private string MakeContentFolder(string name, params (string File, int Size)[] files)
    {
        var content = Path.Combine(_dir.Path, "XboxGames", name, "Content");
        Directory.CreateDirectory(content);
        foreach (var (file, size) in files) File.WriteAllBytes(Path.Combine(content, file), new byte[size]);
        return content;
    }

    [Fact]
    public void The_config_exe_is_used_when_it_is_the_real_game()
    {
        // Two exes of similar size at the root are a question the resolver cannot answer by
        // itself. The Xbox config already did.
        var content = MakeContentFolder("Forza Horizon 6",
            ("forzahorizon6.exe", 1839), ("ForzaTelemetry.exe", 1500), ("gamelaunchhelper.exe", 100));
        var service = MakeService(
            new GameCandidate("Forza Horizon 6", GameStore.Xbox, content, "Microsoft.ForteBaseGame", "forzahorizon6.exe"));

        var entry = service.Scan().Single();

        Assert.Equal(content, entry.ExeDirectory);
        Assert.Equal("forzahorizon6.exe", entry.ExeName);
    }

    [Fact]
    public void A_bootstrapper_named_in_the_config_is_passed_over_for_the_real_exe()
    {
        // Call of Duty MW3 as the Xbox app installs it, sizes scaled 1/1000. Its config names
        // bootstrapper.exe, a launcher; the game is cod23-cod.exe. The Ricochet driver beside it
        // means the row is then not supported at all — but it has to be found to say so.
        var content = MakeContentFolder("Call of Duty- Modern Warfare 3",
            ("bootstrapper.exe", 241), ("bootstrapperCrashHandler.exe", 1297), ("codCrashHandler.exe", 1485),
            ("CrashLogsGenerator.exe", 398), ("gamelaunchhelper.exe", 101), ("cod23-cod.exe", 432085),
            ("Randgrid.sys", 10));
        var service = MakeService(
            new GameCandidate("Call of Duty Modern Warfare 3", GameStore.Xbox, content, "Activision.CoD", "bootstrapper.exe"));

        var entry = service.Scan().Single();

        Assert.Equal(content, entry.ExeDirectory);
        Assert.Equal("cod23-cod.exe", entry.ExeName);
        Assert.Equal(GameStatus.Unsupported, entry.Status);
    }

    [Fact]
    public async Task A_Ricochet_game_added_by_hand_through_its_campaign_subfolder_is_still_blocked()
    {
        // The row for Call of Duty says "Not supported", and the launcher's own advice for an
        // unresolved game is "Add game folder…". Picking Content\sp23, where the campaign exe
        // is, must not turn the hard block into an ordinary install: Randgrid.sys is one level up.
        var content = MakeContentFolder("Call of Duty- Modern Warfare 3", ("Randgrid.sys", 10));
        var sp23 = Path.Combine(content, "sp23");
        Directory.CreateDirectory(sp23);
        File.WriteAllBytes(Path.Combine(sp23, "sp23-cod.exe"), new byte[327]);
        var service = MakeService();

        var entry = service.AddManual(sp23);

        Assert.Equal(sp23, entry.ExeDirectory);
        Assert.Equal(GameStatus.Unsupported, entry.Status);
        Assert.Contains(entry.Gate!.Findings, f => f.Code == GateCode.AntiCheatBlocked);

        var refused = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.InstallAsync(entry, "dxgi.dll", Amdnr, antiCheatAcknowledged: true, null, default));
        Assert.Contains("Ricochet", refused.Message);
        Assert.Equal(["sp23-cod.exe"], Directory.EnumerateFileSystemEntries(sp23).Select(Path.GetFileName));
    }

    [Fact]
    public async Task A_Ricochet_game_whose_exe_cannot_be_found_is_still_not_supported()
    {
        // A campaign-only install: no cod23-cod.exe at the root, every root exe a launcher or a
        // crash tool, and the only game exe in sp23. Resolution gives up — and a row that then
        // reads "Not installed" sends the user to "Add game folder…", straight to sp23.
        var content = MakeContentFolder("Call of Duty- Modern Warfare 3",
            ("bootstrapper.exe", 241), ("bootstrapperCrashHandler.exe", 1297), ("codCrashHandler.exe", 1485),
            ("CrashLogsGenerator.exe", 398), ("gamelaunchhelper.exe", 101), ("Randgrid.sys", 10));
        Directory.CreateDirectory(Path.Combine(content, "sp23"));
        File.WriteAllBytes(Path.Combine(content, "sp23", "sp23-cod.exe"), new byte[327]);
        var service = MakeService(
            new GameCandidate("Call of Duty Modern Warfare 3", GameStore.Xbox, content, "Activision.CoD", "bootstrapper.exe"));

        var entry = service.Scan().Single();

        Assert.Null(entry.ExeDirectory);
        Assert.Equal(GameStatus.Unsupported, entry.Status);
        Assert.Contains(entry.Gate!.Findings, f => f.Code == GateCode.AntiCheatBlocked);

        var refused = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.InstallAsync(entry, "dxgi.dll", Amdnr, antiCheatAcknowledged: true, null, default));
        Assert.Contains("Ricochet", refused.Message);
        Assert.DoesNotContain("Add game folder", refused.Message);
    }

    [Fact]
    public void A_hint_that_leaves_the_install_root_is_ignored()
    {
        var content = MakeContentFolder("Hinted", ("game.exe", 10));
        File.WriteAllBytes(Path.Combine(_dir.Path, "XboxGames", "outside.exe"), new byte[10]);
        var service = MakeService(
            new GameCandidate("Hinted", GameStore.Xbox, content, null, @"..\..\outside.exe"));

        Assert.Equal("game.exe", service.Scan().Single().ExeName);
    }

    [Fact]
    public async Task TryNextProxy_advances_through_the_candidate_order()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        var next = await service.TryNextProxyAsync(entry, default);

        Assert.False(next.StartedOver);
        Assert.Equal("d3d12.dll", next.Entry.Record!.Proxy);
        Assert.False(File.Exists(Path.Combine(root, "dxgi.dll")));
    }

    [Fact]
    public async Task TryNextProxy_keeps_the_names_tried_in_the_record_so_a_restart_carries_on()
    {
        // Until now the list lived in the row alone. The documented workflow — press, launch the
        // game, look, come back — invites closing the launcher in between, and a restart then
        // offered the very name that had just failed.
        var root = MakeGameFolder("Onimusha");
        var candidate = new GameCandidate("Onimusha", GameStore.Steam, root);
        var service = MakeService(candidate);
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);
        Assert.Empty(entry.Record!.TriedProxies);

        var second = await service.TryNextProxyAsync(entry, default);
        Assert.Equal(["dxgi.dll"], second.Entry.Record!.TriedProxies);
        Assert.Equal(["dxgi.dll"], InstallRecordStore.Load(root)!.TriedProxies);

        // A new service over the same folder, as after a restart.
        var restarted = MakeService(candidate);
        var third = await restarted.TryNextProxyAsync(restarted.Scan().Single(), default);

        Assert.Equal("winmm.dll", third.Entry.Record!.Proxy);
        Assert.Equal(["dxgi.dll", "d3d12.dll"], third.Entry.Record.TriedProxies);
    }

    [Fact]
    public async Task TryNextProxy_starts_again_from_the_first_name_once_every_name_has_been_tried()
    {
        // Resident Evil Requiem on the owner's own machine: six presses, six names, then
        // "Every proxy name has already been tried for this game." — with the game left on
        // d3d11.dll and the name that works there, dxgi.dll, out of reach. A dead end is no answer.
        var root = MakeGameFolder("Requiem");
        var service = MakeService(new GameCandidate("Resident Evil Requiem", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        foreach (var expected in new[] { "d3d12.dll", "winmm.dll", "version.dll", "dbghelp.dll", "d3d11.dll" })
        {
            var step = await service.TryNextProxyAsync(entry, default);
            Assert.False(step.StartedOver);
            Assert.Equal(expected, step.Entry.Record!.Proxy);
            entry = step.Entry;
        }
        Assert.Equal(5, entry.Record!.TriedProxies.Count);

        var again = await service.TryNextProxyAsync(entry, default);

        Assert.True(again.StartedOver);
        Assert.Equal("dxgi.dll", again.Entry.Record!.Proxy);
        Assert.Empty(again.Entry.Record.TriedProxies);
        Assert.True(File.Exists(Path.Combine(root, "dxgi.dll")));
        Assert.False(File.Exists(Path.Combine(root, "d3d11.dll")));

        // And the round goes on from there.
        var onward = await service.TryNextProxyAsync(again.Entry, default);
        Assert.False(onward.StartedOver);
        Assert.Equal("d3d12.dll", onward.Entry.Record!.Proxy);
        Assert.Equal(["dxgi.dll"], onward.Entry.Record.TriedProxies);
    }

    [Fact]
    public async Task TryNextProxy_starts_again_past_a_name_another_file_holds_instead_of_refusing_it_forever()
    {
        // The same round with ReShade dropped in as dxgi.dll after the install. The planner
        // demotes an occupied name but never drops it, so once the free names were used up the
        // button handed the installer dxgi.dll; the installer refused to move over a file it did
        // not place — before saving anything — and every press after that repeated the same
        // refusal with the DLL stuck on d3d11.dll. The round has to skip that name and, when
        // only such names remain, start again the way it does when every name has been tried.
        var root = MakeGameFolder("Requiem");
        var service = MakeService(new GameCandidate("Resident Evil Requiem", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "d3d12.dll", Amdnr, false, null, default);
        var foreign = Path.Combine(root, "dxgi.dll");
        File.WriteAllText(foreign, "ReShade, say");

        foreach (var expected in new[] { "winmm.dll", "version.dll", "dbghelp.dll", "d3d11.dll" })
        {
            var step = await service.TryNextProxyAsync(entry, default);
            Assert.False(step.StartedOver);
            Assert.Equal(expected, step.Entry.Record!.Proxy);
            entry = step.Entry;
        }

        var again = await service.TryNextProxyAsync(entry, default);

        Assert.True(again.StartedOver);
        Assert.Equal("d3d12.dll", again.Entry.Record!.Proxy);
        Assert.Empty(again.Entry.Record.TriedProxies);
        Assert.True(File.Exists(Path.Combine(root, "d3d12.dll")));
        Assert.False(File.Exists(Path.Combine(root, "d3d11.dll")));
        Assert.Equal("ReShade, say", File.ReadAllText(foreign));

        // And the round goes on past the foreign name again.
        var onward = await service.TryNextProxyAsync(again.Entry, default);
        Assert.False(onward.StartedOver);
        Assert.Equal("winmm.dll", onward.Entry.Record!.Proxy);
    }

    [Fact]
    public async Task TryNextProxy_says_so_when_every_other_name_has_a_file_under_it()
    {
        var root = MakeGameFolder("Crowded");
        var service = MakeService(new GameCandidate("Crowded", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);
        foreach (var name in new[] { "d3d12.dll", "winmm.dll", "version.dll", "dbghelp.dll", "d3d11.dll" })
            File.WriteAllText(Path.Combine(root, name), "somebody else's");

        var refused = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.TryNextProxyAsync(entry, default));

        Assert.Contains("Every other proxy name", refused.Message);
        Assert.Contains("Move one aside", refused.Message);
        Assert.Equal("dxgi.dll", InstallRecordStore.Load(root)!.Proxy);
        Assert.True(File.Exists(Path.Combine(root, "dxgi.dll")));
    }

    [Fact]
    public async Task SwitchProxy_moves_the_DLL_to_the_chosen_name_and_counts_the_one_left_as_tried()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        var switched = await service.SwitchProxyAsync(entry, "winmm.dll", default);

        Assert.Equal("winmm.dll", switched.Record!.Proxy);
        Assert.Equal(["dxgi.dll"], switched.Record.TriedProxies);
        Assert.True(File.Exists(Path.Combine(root, "winmm.dll")));
        Assert.False(File.Exists(Path.Combine(root, "dxgi.dll")));
        Assert.Equal(GameStatus.Installed, switched.Status);

        // TRY NEXT PROXY carries on past both.
        var next = await service.TryNextProxyAsync(switched, default);
        Assert.Equal("d3d12.dll", next.Entry.Record!.Proxy);
        Assert.Equal(["dxgi.dll", "winmm.dll"], next.Entry.Record.TriedProxies);
    }

    [Fact]
    public async Task SwitchProxy_to_the_name_already_in_use_changes_nothing()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);
        var before = File.GetLastWriteTimeUtc(Path.Combine(root, "dxgi.dll"));

        var same = await service.SwitchProxyAsync(entry, "DXGI.dll", default);

        Assert.Equal("dxgi.dll", same.Record!.Proxy);
        Assert.Empty(same.Record.TriedProxies);
        Assert.Equal(before, File.GetLastWriteTimeUtc(Path.Combine(root, "dxgi.dll")));
    }

    [Fact]
    public async Task SwitchProxy_refuses_a_game_with_nothing_installed_and_one_Ricochet_arrived_in()
    {
        var root = MakeGameFolder("CoD");
        var service = MakeService(new GameCandidate("Call of Duty", GameStore.Steam, root));
        var bare = service.Scan().Single();

        var nothing = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.SwitchProxyAsync(bare, "winmm.dll", default));
        Assert.Equal("Nothing is installed for this game yet.", nothing.Message);

        var installed = await service.InstallAsync(bare, "dxgi.dll", Amdnr, false, null, default);
        File.WriteAllText(Path.Combine(root, "Randgrid.sys"), "driver");

        var refused = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.SwitchProxyAsync(installed, "winmm.dll", default));

        Assert.Contains("Ricochet", refused.Message);
        Assert.True(File.Exists(Path.Combine(root, "dxgi.dll")));
        Assert.False(File.Exists(Path.Combine(root, "winmm.dll")));
    }

    [Fact]
    public async Task Scan_says_why_the_name_was_picked_and_which_names_other_files_hold()
    {
        var root = MakeGameFolder("Onimusha");
        File.WriteAllText(Path.Combine(root, "winmm.dll"), "ReShade, say");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));

        var plain = service.Scan().Single();
        Assert.Equal("dxgi.dll", plain.RecommendedProxy);
        Assert.Equal("first free name", plain.ProxyReason);
        Assert.Equal(["winmm.dll"], plain.OccupiedProxies);

        // An exe that names d3d12.dll in its import table gets it, and the row says so.
        File.WriteAllBytes(Path.Combine(root, "Onimusha.exe"), PeImportsTests.Build(pe32Plus: true, ["d3d12.dll"]));
        var imports = service.Refresh(plain);
        Assert.Equal("d3d12.dll", imports.RecommendedProxy);
        Assert.Equal("Onimusha.exe loads d3d12.dll itself when it starts", imports.ProxyReason);

        // Installed, the name in use is the one the row shows, whatever the planner would pick
        // for an empty folder today. This fixture's stand-in DLL is no OptiScaler, so the reason
        // is the one for a record whose file the probe does not recognise.
        var installed = await service.InstallAsync(imports, "dxgi.dll", Amdnr, false, null, default);
        Assert.Equal("dxgi.dll", installed.RecommendedProxy);
        Assert.Equal("this install was placed as dxgi.dll", installed.ProxyReason);

        // The install moved the other injector into AMDNR_backup, so its name is free again.
        Assert.False(File.Exists(Path.Combine(root, "winmm.dll")));
        Assert.DoesNotContain("winmm.dll", installed.OccupiedProxies);
    }

    [Fact]
    public async Task An_installed_DLL_the_probe_recognises_reads_as_installed_here_not_as_an_earlier_install()
    {
        // The stand-in payload is no OptiScaler; the built fixture is. With it under the
        // record's name the planner sees an OptiScaler loading through that name and says
        // "your earlier install used dxgi.dll" — the words for a hand-made install found with no
        // record, which read oddly a moment after pressing INSTALL. With a record the row says
        // the one thing that is certainly true of it.
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var installed = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);
        File.Copy(ProxySlotProbeTests.FixtureOptiScalerDll, Path.Combine(root, "dxgi.dll"), overwrite: true);

        var shown = service.Refresh(installed);

        Assert.Equal("dxgi.dll", shown.RecommendedProxy);
        Assert.Equal("installed here as dxgi.dll", shown.ProxyReason);
    }

    [Fact]
    public async Task TryNextProxy_refuses_a_game_Ricochet_arrived_in_after_the_install()
    {
        // Installed before the block existed, or before a game update brought Randgrid.sys in.
        // Moving the mod to another proxy name there could make a DLL that was not loading
        // start loading, in a game where that bans the account — with no prompt at all.
        var root = MakeGameFolder("CoD");
        var service = MakeService(new GameCandidate("Call of Duty", GameStore.Steam, root));
        var installed = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        File.WriteAllText(Path.Combine(root, "Randgrid.sys"), "driver");
        Assert.Equal(GameStatus.Unsupported, service.Refresh(installed).Status);

        // The entry from before the driver arrived: Core looks again rather than trusting it.
        var refused = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.TryNextProxyAsync(installed, default));

        Assert.Contains("Ricochet", refused.Message);
        Assert.True(File.Exists(Path.Combine(root, "dxgi.dll")));
        Assert.False(File.Exists(Path.Combine(root, "d3d12.dll")));
    }

    [Fact]
    public async Task Uninstall_returns_the_entry_to_NotInstalled()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        var after = await service.UninstallAsync(entry, default);

        Assert.Equal(GameStatus.NotInstalled, after.Entry.Status);
        Assert.Null(after.Entry.Record);
        Assert.True(after.Result.IsComplete);
    }

    [Fact]
    public async Task Installing_without_a_resolved_exe_folder_names_the_way_out()
    {
        // Two plausible exes in the root: resolution gives up, the Install button stays
        // enabled, and the message it produced was a dead end.
        var root = Path.Combine(_dir.Path, "Ambiguous");
        Directory.CreateDirectory(root);
        File.WriteAllText(Path.Combine(root, "GameDX11.exe"), "a");
        File.WriteAllText(Path.Combine(root, "GameDX12.exe"), "b");

        var service = MakeService(new GameCandidate("Ambiguous", GameStore.Steam, root));
        var entry = service.Scan().Single();
        Assert.Null(entry.ExeDirectory);

        var problem = await Assert.ThrowsAsync<InvalidOperationException>(
            () => service.InstallAsync(entry, "dxgi.dll", Amdnr, false, null, default));

        Assert.Contains("Add game folder", problem.Message);
    }

    [Fact]
    public void An_anticheat_runtime_beside_the_exe_directory_reaches_the_gate()
    {
        // <root>\Binaries\Win64\Game.exe with <root>\EasyAntiCheat\ next to it. This is the
        // layout the warning exists for, and it is the one the old exe-directory-only scan
        // could not see.
        var root = Path.Combine(_dir.Path, "Fortress");
        var exeDirectory = Path.Combine(root, "Binaries", "Win64");
        Directory.CreateDirectory(exeDirectory);
        Directory.CreateDirectory(Path.Combine(root, "EasyAntiCheat"));
        File.WriteAllText(Path.Combine(exeDirectory, "Fortress-Win64-Shipping.exe"), "game");

        var service = MakeService(new GameCandidate("Fortress", GameStore.Steam, root));

        var entry = service.Scan().Single();

        Assert.Equal(exeDirectory, entry.ExeDirectory);
        Assert.Contains(entry.Gate!.Findings, f => f.Code == GateCode.AntiCheatDetected);
    }

    [Fact]
    public void A_manually_added_folder_survives_a_rescan()
    {
        var root = MakeGameFolder("ManualGame");
        var service = MakeService();

        service.AddManual(root);

        var entry = service.Scan().Single();
        Assert.Equal(GameStore.Manual, entry.Candidate.Store);
        Assert.Equal(root, entry.ExeDirectory);
    }

    [Fact]
    public void Refreshing_a_manual_entry_whose_folder_is_gone_does_not_throw()
    {
        // Deleted, renamed, or on a drive that is no longer plugged in. The facade absorbs
        // it so the UI does not need a try/catch around Refresh.
        var root = MakeGameFolder("ManualGame");
        var service = MakeService();
        var entry = service.AddManual(root);

        Directory.Delete(root, recursive: true);

        var after = service.Refresh(entry);

        Assert.Null(after.ExeDirectory);
    }

    [Fact]
    public async Task A_broken_install_is_reported_as_Problem()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        File.Delete(Path.Combine(root, PayloadNames.Weights));

        Assert.Equal(GameStatus.Problem, service.Refresh(entry).Status);
    }

    [Fact]
    public async Task A_source_without_a_forwarder_places_none_and_Doctor_skips_the_forwarder_check()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));

        var installed = await service.InstallAsync(service.Scan().Single(), "dxgi.dll",
            new InstallChoice("theautomatic", IncludeRuntime: true), false, null, default);

        Assert.False(File.Exists(Path.Combine(root, PayloadNames.Forwarder)));
        Assert.Equal("theautomatic", installed.Record!.Source);
        Assert.Null(installed.Record.ForwarderVersion);
        Assert.DoesNotContain(installed.Report!.Findings, f => f.Id.StartsWith("forwarder."));
        Assert.Equal(GameStatus.Installed, installed.Status);
    }

    [Fact]
    public async Task The_runtime_is_installed_only_when_the_choice_includes_it()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));

        var installed = await service.InstallAsync(service.Scan().Single(), "dxgi.dll",
            new InstallChoice("amdnr", IncludeRuntime: false), false, null, default);

        Assert.All(PayloadNames.PassDlls, p => Assert.False(File.Exists(Path.Combine(root, p)), p));
        Assert.Null(installed.Record!.RuntimeVersion);
        Assert.Null(installed.Record.RuntimeId);
        Assert.True(File.Exists(Path.Combine(root, PayloadNames.Forwarder)));
        Assert.Equal(GameStatus.Installed, installed.Status);
    }

    [Fact]
    public void PackagesFor_returns_mod_plus_runtime_only_when_chosen()
    {
        var service = MakeService();

        Assert.Equal(["amdnr", "runtime", "forwarder"], Ids(service.PackagesFor(new InstallChoice("amdnr", true))));
        Assert.Equal(["amdnr", "forwarder"], Ids(service.PackagesFor(new InstallChoice("amdnr", false))));
        Assert.Equal(["theautomatic", "runtime"], Ids(service.PackagesFor(new InstallChoice("theautomatic", true))));
        Assert.Equal(["theautomatic"], Ids(service.PackagesFor(new InstallChoice("TheAutomatic", false))));
    }

    [Fact]
    public void A_manifest_without_sources_still_installs_AMDNR_as_before()
    {
        var service = MakeService();
        var legacy = new LauncherService(service.Manifest with { Sources = null }, service.Assets,
            new Installer(service.Assets, new CopyFilePlacer()), [], Rdna4);

        Assert.Equal(["amdnr", "runtime", "forwarder"],
            legacy.PackagesFor(new InstallChoice("amdnr", true)).Select(p => p.Id));
    }

    [Fact]
    public void The_runtime_is_the_first_one_the_manifest_lists_for_the_GPU_generation()
    {
        // 0.3.3 first: it runs on RX 7000 and RX 9000 and is the default for every GPU.
        Assert.Equal(["amdnr", "runtime-033", "forwarder"],
            Ids(MakeServiceWithRuntimes(Rdna4, zeroFourZeroFirst: false).PackagesFor(Amdnr)));
        Assert.Equal(["amdnr", "runtime-033", "forwarder"],
            Ids(MakeServiceWithRuntimes(Rdna3, zeroFourZeroFirst: false).PackagesFor(Amdnr)));
        Assert.Equal(["theautomatic", "runtime-033"],
            Ids(MakeServiceWithRuntimes(Rdna3, zeroFourZeroFirst: false)
                .PackagesFor(new InstallChoice("theautomatic", IncludeRuntime: true))));

        // Reordering the manifest is how the owner makes 0.4.0 the RDNA 4 default without a
        // new launcher. RDNA 3 is untouched, because 0.4.0's list names only rdna4.
        Assert.Equal(["amdnr", "runtime-040", "forwarder"],
            Ids(MakeServiceWithRuntimes(Rdna4, zeroFourZeroFirst: true).PackagesFor(Amdnr)));
        Assert.Equal(["amdnr", "runtime-033", "forwarder"],
            Ids(MakeServiceWithRuntimes(Rdna3, zeroFourZeroFirst: true).PackagesFor(Amdnr)));
    }

    [Fact]
    public void A_runtime_the_user_picked_is_installed_when_the_build_lists_it_for_the_card()
    {
        // 0.4.0 first for rdna4 is the card's default; the user picked 0.3.3 in the chooser.
        var picked033 = new InstallChoice("amdnr", IncludeRuntime: true, RuntimeId: "runtime-033");
        Assert.Equal(["amdnr", "runtime-033", "forwarder"],
            Ids(MakeServiceWithRuntimes(Rdna4, zeroFourZeroFirst: true).PackagesFor(picked033)));

        // A pick the build does not list for this card is no pick — 0.4.0's kernels are RX 9000
        // only — and the card's own first runtime is installed instead.
        var picked040 = new InstallChoice("amdnr", IncludeRuntime: true, RuntimeId: "runtime-040");
        Assert.Equal(["amdnr", "runtime-033", "forwarder"],
            Ids(MakeServiceWithRuntimes(Rdna3, zeroFourZeroFirst: true).PackagesFor(picked040)));

        // No files is no files, whatever was picked at some other time.
        Assert.Equal(["amdnr", "forwarder"],
            Ids(MakeServiceWithRuntimes(Rdna4, zeroFourZeroFirst: true)
                .PackagesFor(new InstallChoice("amdnr", IncludeRuntime: false, RuntimeId: "runtime-033"))));
    }

    [Fact]
    public async Task The_Doctor_measures_an_install_against_the_runtime_the_user_prefers()
    {
        // 0.4.0 first for rdna4. One game installed on the card's default, one on a picked 0.3.3.
        var onDefault = MakeGameFolder("Onimusha");
        var onPick = MakeGameFolder("Stray");
        var service = MakeServiceWithRuntimes(Rdna4, zeroFourZeroFirst: true,
            new GameCandidate("Onimusha", GameStore.Steam, onDefault),
            new GameCandidate("Stray", GameStore.Steam, onPick));
        var games = service.Scan();
        var defaulted = await service.InstallAsync(games.Single(g => g.Candidate.Name == "Onimusha"), "dxgi.dll", Amdnr, false, null, default);
        var picked = await service.InstallAsync(games.Single(g => g.Candidate.Name == "Stray"), "dxgi.dll",
            new InstallChoice("amdnr", true, "runtime-033"), false, null, default);
        Assert.Equal("runtime-040", defaulted.Record!.RuntimeId);
        Assert.Equal("runtime-033", picked.Record!.RuntimeId);

        // Following the manifest (no pick): the game on 0.3.3 is offered the card's 0.4.0.
        service.PreferredRuntimeId = null;
        Assert.Null(service.Refresh(defaulted).Report!.AvailableRuntimeVersion);
        Assert.Equal("0.4.0", service.Refresh(picked).Report!.AvailableRuntimeVersion);

        // The user picked 0.3.3: the game on it is left alone, the one on 0.4.0 is offered the move.
        service.PreferredRuntimeId = "runtime-033";
        Assert.Null(service.Refresh(picked).Report!.AvailableRuntimeVersion);
        Assert.Equal("0.3.3", service.Refresh(defaulted).Report!.AvailableRuntimeVersion);
    }

    [Fact]
    public void A_generation_no_runtime_lists_is_offered_none_even_when_asked()
    {
        // Nothing in the manifest names rdna1. That GPU gets no runtime rather than one that
        // was not meant for it — and the install goes ahead without, as the note said it would.
        var rdna1 = new GpuInfo("AMD Radeon RX 5700 XT", "rdna1", "31.0.12027.9001", null);

        Assert.Equal(["amdnr", "forwarder"],
            Ids(MakeServiceWithRuntimes(rdna1, zeroFourZeroFirst: false).PackagesFor(Amdnr)));
    }

    [Fact]
    public async Task An_install_records_which_runtime_package_it_placed_and_Doctor_checks_against_it()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeServiceWithRuntimes(Rdna4, zeroFourZeroFirst: true,
            new GameCandidate("Onimusha", GameStore.Steam, root));

        var installed = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        Assert.Equal("runtime-040", installed.Record!.RuntimeId);
        Assert.Equal("0.4.0", installed.Record.RuntimeVersion);
        Assert.Equal(GameStatus.Installed, installed.Status);

        // Checked against runtime-040's accepted hashes, not against a package called
        // "runtime" that this manifest does not have.
        Assert.DoesNotContain(installed.Report!.Findings,
            f => f.Id is "manifest.incomplete" or "pass.unverifiable" or "pass.unknownBuild");
    }

    [Fact]
    public async Task A_build_without_a_runtime_list_records_the_runtime_package()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));

        var installed = await service.InstallAsync(service.Scan().Single(), "dxgi.dll", Amdnr, false, null, default);

        Assert.Equal("runtime", installed.Record!.RuntimeId);
    }

    [Fact]
    public async Task Repair_and_update_reinstall_the_build_the_record_names()
    {
        // Whatever the user would pick for a new game today, Repair and Update on an installed
        // row must put back what is there. Otherwise Update on a TheAutomatic row silently
        // switches the game to AMDNR, and Repair on a row installed without the runtime
        // downloads and places ~110 MB the user declined.
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var fresh = service.Scan().Single();

        Assert.Equal(Amdnr, LauncherService.ChoiceFor(fresh, Amdnr));

        // The runtime pick travels with the current choice: REPAIR / UPDATE on an installed game
        // moves it to the runtime the user wants today, not the one the record happens to name.
        var picked = new InstallChoice("amdnr", IncludeRuntime: true, RuntimeId: "runtime-033");
        Assert.Equal(picked, LauncherService.ChoiceFor(fresh, picked));

        var installed = await service.InstallAsync(fresh, "dxgi.dll",
            new InstallChoice("theautomatic", IncludeRuntime: false), false, null, default);

        Assert.Equal(new InstallChoice("theautomatic", IncludeRuntime: false),
            LauncherService.ChoiceFor(installed, Amdnr));
    }

    [Fact]
    public async Task Installing_an_unknown_source_says_so_plainly()
    {
        // A choice saved against an older manifest can name a build the current one dropped.
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var choice = new InstallChoice("withdrawn", IncludeRuntime: true);

        var problem = await Assert.ThrowsAsync<InvalidOperationException>(() => service.InstallAsync(
            service.Scan().Single(), "dxgi.dll", choice, false, null, default));

        Assert.Contains("withdrawn", problem.Message);
        Assert.Throws<InvalidOperationException>(() => service.PackagesFor(choice));
        Assert.False(File.Exists(Path.Combine(root, "dxgi.dll")));
    }

    [Fact]
    public async Task A_newer_build_of_the_installed_source_is_offered_as_an_update()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = await service.InstallAsync(service.Scan().Single(), "dxgi.dll",
            new InstallChoice("theautomatic", true), false, null, default);

        var store = (StubAssetStore)service.Assets;
        var bumped = service.Manifest with
        {
            Packages = [store.Amdnr with { Version = "9.9.9" }, store.TheAutomatic, store.Runtime, store.Forwarder],
        };
        var otherBuildNewer = new LauncherService(bumped, store, new Installer(store, new CopyFilePlacer()), [], Rdna4);
        Assert.Equal(GameStatus.Installed, otherBuildNewer.Refresh(entry).Status);

        var ownBuildNewer = new LauncherService(
            bumped with { Packages = [store.Amdnr, store.TheAutomatic with { Version = "1.8.7-0.3.1" }, store.Runtime, store.Forwarder] },
            store, new Installer(store, new CopyFilePlacer()), [], Rdna4);
        Assert.Equal(GameStatus.UpdateAvailable, ownBuildNewer.Refresh(entry).Status);
    }

    [Fact]
    public void A_game_with_a_hand_made_install_and_no_record_is_ExistingInstall()
    {
        var root = MakeGameFolder("Onimusha");
        ProxySlotProbeTests.PlaceHandMadeProxy(root, "winmm.dll");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));

        var entry = service.Scan().Single();

        Assert.Equal(GameStatus.ExistingInstall, entry.Status);
        Assert.Null(entry.Record);
        Assert.Equal("winmm.dll", entry.RecommendedProxy);
    }

    [Fact]
    public void A_game_whose_proxy_is_some_other_mod_is_still_NotInstalled()
    {
        var root = MakeGameFolder("Onimusha");
        File.WriteAllText(Path.Combine(root, "dxgi.dll"), "ReShade, say");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));

        Assert.Equal(GameStatus.NotInstalled, service.Scan().Single().Status);
    }

    [Fact]
    public async Task Installing_over_a_hand_made_install_keeps_its_proxy_name_and_backs_the_old_dll_up()
    {
        var root = MakeGameFolder("Onimusha");
        var handMade = ProxySlotProbeTests.PlaceHandMadeProxy(root, "winmm.dll");
        var oldBytes = File.ReadAllBytes(handMade);
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        var entry = service.Scan().Single();

        var installed = await service.InstallAsync(entry, entry.RecommendedProxy, Amdnr, false, null, default);

        Assert.Equal("winmm.dll", installed.Record!.Proxy);
        Assert.Equal("optiscaler-payload", File.ReadAllText(handMade));
        Assert.False(File.Exists(Path.Combine(root, "dxgi.dll")));
        Assert.Equal(GameStatus.Installed, installed.Status);

        var backup = Assert.Single(installed.Record.BackupDirectories);
        Assert.StartsWith(Path.Combine(root, "AMDNR_backup"), backup);
        Assert.Equal(oldBytes, File.ReadAllBytes(Path.Combine(backup, "winmm.dll")));
    }

    private static readonly InstallChoice TheAutomaticWithRuntime = new("theautomatic", IncludeRuntime: true);

    /// <summary>What TheAutomatic 1.9.1-alpha ships: NR off, and the lmxxf backend selected,
    /// whose model files are not in its release.</summary>
    private const string TheAutomaticIni =
        "; OptiScaler AMD pre-SR\r\n[DlssNr]\r\nEnabled=false\r\nNrBackend=lmxxf\r\n\r\n[Upscalers]\r\nDx12Upscaler=auto\r\n";

    /// <summary>The seed with the runtime: Neural Rendering switched on, on danielblnc's backend,
    /// which is what TheAutomatic's own Setup.ps1 writes. Everything else is left as shipped.</summary>
    private const string TheAutomaticIniWithDaniel =
        "; OptiScaler AMD pre-SR\r\n[DlssNr]\r\nEnabled=true\r\nNrBackend=daniel\r\n\r\n[Upscalers]\r\nDx12Upscaler=auto\r\n";

    private (LauncherService Service, string Root) MakeTheAutomaticGame()
    {
        var root = MakeGameFolder("Onimusha");
        var service = MakeService(new GameCandidate("Onimusha", GameStore.Steam, root));
        File.WriteAllText(((StubAssetStore)service.Assets).TheAutomaticPackagedIni, TheAutomaticIni);
        return (service, root);
    }

    [Fact]
    public async Task Seeded_ini_gets_NrBackend_daniel_when_the_source_says_so_and_the_runtime_is_installed()
    {
        var (service, root) = MakeTheAutomaticGame();

        var installed = await service.InstallAsync(
            service.Scan().Single(), "dxgi.dll", TheAutomaticWithRuntime, false, null, default);

        var ini = Path.Combine(root, PayloadNames.OptiScalerIni);
        Assert.Equal(TheAutomaticIniWithDaniel, File.ReadAllText(ini));
        Assert.Equal(Hashing.Sha256OfFile(ini),
            installed.Record!.Files.Single(f => f.RelativePath == PayloadNames.OptiScalerIni).Sha256);

        // The staged seed lives beside the package, never inside it, so nothing of it is placed.
        Assert.DoesNotContain(Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories),
            f => f.EndsWith("-runtime", StringComparison.OrdinalIgnoreCase));
        Assert.DoesNotContain(installed.Report!.Findings, f => f.Id == "ini.lmxxfWithoutModels");
    }

    [Fact]
    public async Task A_user_ini_is_never_edited_but_Doctor_warns_about_lmxxf_without_models()
    {
        var (service, root) = MakeTheAutomaticGame();
        var ini = Path.Combine(root, PayloadNames.OptiScalerIni);
        const string mine = "[DlssNr]\r\nEnabled=true\r\nNrBackend=lmxxf\r\n; tuned by me\r\n";
        File.WriteAllText(ini, mine);

        var installed = await service.InstallAsync(
            service.Scan().Single(), "dxgi.dll", TheAutomaticWithRuntime, false, null, default);

        Assert.Equal(mine, File.ReadAllText(ini));
        var warning = Assert.Single(installed.Report!.Findings, f => f.Id == "ini.lmxxfWithoutModels");
        Assert.Equal(DoctorSeverity.Warning, warning.Severity);
        Assert.Equal(
            "This OptiScaler.ini selects the lmxxf backend, but its model files are not installed, so " +
            "Neural Rendering will not run. Use RESET INI to switch to the runtime the launcher installed.",
            warning.Message);

        // With lmxxf's models beside the game, the choice is a working one and not ours to question.
        Directory.CreateDirectory(Path.Combine(root, "native-game-tiled-assets"));
        Assert.DoesNotContain(service.Refresh(installed).Report!.Findings, f => f.Id == "ini.lmxxfWithoutModels");
    }

    [Fact]
    public async Task Reset_ini_writes_the_staged_seed()
    {
        var (service, root) = MakeTheAutomaticGame();
        var installed = await service.InstallAsync(
            service.Scan().Single(), "dxgi.dll", TheAutomaticWithRuntime, false, null, default);
        var ini = Path.Combine(root, PayloadNames.OptiScalerIni);
        File.WriteAllText(ini, "[DlssNr]\r\nNrBackend=lmxxf\r\n; tuned by me\r\n");

        var backup = service.ResetIni(installed);

        Assert.Equal(TheAutomaticIniWithDaniel, File.ReadAllText(ini));
        Assert.Equal("[DlssNr]\r\nNrBackend=lmxxf\r\n; tuned by me\r\n", File.ReadAllText(backup!));
        Assert.DoesNotContain(service.Refresh(installed).Report!.Findings, f => f.Id == "ini.lmxxfWithoutModels");
    }

    [Fact]
    public async Task Reset_ini_without_the_runtime_writes_the_packaged_ini()
    {
        var (service, root) = MakeTheAutomaticGame();
        var installed = await service.InstallAsync(service.Scan().Single(), "dxgi.dll",
            TheAutomaticWithRuntime with { IncludeRuntime = false }, false, null, default);
        var ini = Path.Combine(root, PayloadNames.OptiScalerIni);
        File.WriteAllText(ini, "; tuned by me");

        service.ResetIni(installed);

        Assert.Equal(TheAutomaticIni, File.ReadAllText(ini));
    }
}
