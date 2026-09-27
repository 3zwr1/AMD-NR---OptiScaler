// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class ProxyPlannerTests
{
    private static Manifest MakeManifest(params ProxyOverride[] overrides) => new(
        SchemaVersion: 1,
        Launcher: new LauncherInfo("1.0.0", "https://h/l.exe", "AA"),
        Packages: [],
        Compatibility: [],
        ProxyDefaults: ["dxgi.dll", "d3d12.dll", "winmm.dll", "version.dll", "dbghelp.dll", "d3d11.dll"],
        ProxyOverrides: overrides);

    [Fact]
    public void Recommends_dxgi_for_an_ordinary_game()
    {
        using var dir = new TempDir();

        Assert.Equal("dxgi.dll", ProxyPlanner.Recommend("Game.exe", dir.Path, MakeManifest()));
    }

    [Fact]
    public void A_manifest_override_wins()
    {
        using var dir = new TempDir();
        var manifest = MakeManifest(new ProxyOverride("SomeGame.exe", "winmm.dll", "overlay owns dxgi"));

        Assert.Equal("winmm.dll", ProxyPlanner.Recommend("SomeGame.exe", dir.Path, manifest));
    }

    [Fact]
    public void Override_matching_is_case_insensitive()
    {
        using var dir = new TempDir();
        var manifest = MakeManifest(new ProxyOverride("somegame.exe", "dbghelp.dll", null));

        Assert.Equal("dbghelp.dll", ProxyPlanner.Recommend("SomeGame.EXE", dir.Path, manifest));
    }

    [Fact]
    public void An_occupied_name_is_demoted_but_still_offered()
    {
        using var dir = new TempDir();
        dir.Write("dxgi.dll", "another mod");

        var order = ProxyPlanner.CandidateOrder("Game.exe", dir.Path, MakeManifest());

        Assert.Equal("d3d12.dll", order[0]);
        Assert.Contains("dxgi.dll", order);
        Assert.Equal("dxgi.dll", order[^1]);
    }

    [Fact]
    public void An_override_stays_first_even_when_its_own_file_already_exists()
    {
        // The exact case a per-game override exists for: a previous attempt already left a
        // winmm.dll in the folder, and the manifest says this game needs winmm.
        using var dir = new TempDir();
        dir.Write("winmm.dll", "from a previous attempt");
        var manifest = MakeManifest(new ProxyOverride("Game.exe", "winmm.dll", null));

        Assert.Equal("winmm.dll", ProxyPlanner.CandidateOrder("Game.exe", dir.Path, manifest)[0]);
    }

    [Fact]
    public void An_existing_OptiScaler_proxy_ranks_first_even_though_its_slot_is_occupied()
    {
        // Someone installed OptiScaler by hand as winmm.dll. That name is known to load in this
        // game, and installing under any other would leave two OptiScalers side by side.
        using var dir = new TempDir();
        ProxySlotProbeTests.PlaceHandMadeProxy(dir.Path, "winmm.dll");
        dir.Write("dxgi.dll", "another mod");

        var order = ProxyPlanner.CandidateOrder("Game.exe", dir.Path, MakeManifest());

        Assert.Equal("winmm.dll", order[0]);
        Assert.Equal("dxgi.dll", order[^1]);
        Assert.Equal(order.Count, order.Distinct(StringComparer.OrdinalIgnoreCase).Count());
    }

    [Fact]
    public void The_existing_proxy_beats_a_manifest_override()
    {
        using var dir = new TempDir();
        ProxySlotProbeTests.PlaceHandMadeProxy(dir.Path, "version.dll");
        dir.Write("dbghelp.dll", "left by a previous attempt");
        var manifest = MakeManifest(new ProxyOverride("Game.exe", "dbghelp.dll", null));

        var order = ProxyPlanner.CandidateOrder("Game.exe", dir.Path, manifest);

        // Both are exempt from demotion: the override keeps second place though its slot is taken.
        Assert.Equal(["version.dll", "dbghelp.dll"], order.Take(2));
        Assert.Equal(order.Count, order.Distinct(StringComparer.OrdinalIgnoreCase).Count());
        Assert.DoesNotContain("dinput8.dll", order);
    }

    [Fact]
    public void An_override_naming_dinput8_is_ignored_rather_than_offered()
    {
        using var dir = new TempDir();
        var manifest = MakeManifest(new ProxyOverride("Game.exe", "dinput8.dll", null));

        var order = ProxyPlanner.CandidateOrder("Game.exe", dir.Path, manifest);

        Assert.DoesNotContain("dinput8.dll", order);
        Assert.Equal("dxgi.dll", order[0]);
    }

    [Fact]
    public void dinput8_never_appears()
    {
        using var dir = new TempDir();

        Assert.DoesNotContain("dinput8.dll", ProxyPlanner.CandidateOrder("Game.exe", dir.Path, MakeManifest()));
    }

    [Fact]
    public void Next_skips_names_already_tried()
    {
        using var dir = new TempDir();

        var next = ProxyPlanner.Next(
            currentProxy: "dxgi.dll",
            alreadyTried: ["dxgi.dll", "d3d12.dll"],
            "Game.exe", dir.Path, MakeManifest());

        Assert.Equal("winmm.dll", next);
    }

    [Fact]
    public void Next_returns_null_once_every_name_has_been_tried()
    {
        using var dir = new TempDir();

        var next = ProxyPlanner.Next(
            currentProxy: "d3d11.dll",
            alreadyTried: PayloadNames.ProxyNames.ToList(),
            "Game.exe", dir.Path, MakeManifest());

        Assert.Null(next);
    }

    [Fact]
    public void An_empty_proxyDefaults_falls_back_to_the_built_in_order()
    {
        using var dir = new TempDir();
        var manifest = MakeManifest() with { ProxyDefaults = [] };

        Assert.Equal("dxgi.dll", ProxyPlanner.Recommend("Game.exe", dir.Path, manifest));
    }

    /// <summary>A game exe that names the given DLLs in its import table, and nothing else.</summary>
    private static void WriteExe(TempDir dir, string name, params string[] imports)
        => dir.Write(name, PeImportsTests.Build(pe32Plus: true, imports));

    [Fact]
    public void A_name_the_exe_loads_itself_comes_before_the_other_defaults()
    {
        // Resident Evil Requiem: the owner tried every name in turn and ended on d3d11.dll,
        // while re9.exe asks Windows for dxgi.dll the moment it starts. Reading that out of the
        // exe puts the name that will load first, and the other names it loads right after.
        using var dir = new TempDir();
        WriteExe(dir, "Game.exe", "KERNEL32.dll", "version.dll", "d3d12.dll");

        var order = ProxyPlanner.CandidateOrder("Game.exe", dir.Path, MakeManifest());

        Assert.Equal(["d3d12.dll", "version.dll", "dxgi.dll", "winmm.dll", "dbghelp.dll", "d3d11.dll"], order);
        Assert.Equal(new ProxyChoice("d3d12.dll", "Game.exe loads d3d12.dll itself when it starts"),
            ProxyPlanner.Explain("Game.exe", dir.Path, MakeManifest()));
    }

    [Fact]
    public void An_imported_name_another_file_already_holds_is_still_demoted()
    {
        using var dir = new TempDir();
        WriteExe(dir, "Game.exe", "d3d12.dll");
        dir.Write("d3d12.dll", "ReShade, say");

        var order = ProxyPlanner.CandidateOrder("Game.exe", dir.Path, MakeManifest());

        Assert.Equal("dxgi.dll", order[0]);
        Assert.Equal("d3d12.dll", order[^1]);
        Assert.Equal(new ProxyChoice("dxgi.dll", "first free name"),
            ProxyPlanner.Explain("Game.exe", dir.Path, MakeManifest()));
    }

    [Fact]
    public void The_earlier_install_and_the_override_still_come_before_what_the_exe_loads()
    {
        using var dir = new TempDir();
        WriteExe(dir, "Game.exe", "dxgi.dll");
        var manifest = MakeManifest(new ProxyOverride("Game.exe", "dbghelp.dll", null));

        Assert.Equal(["dbghelp.dll", "dxgi.dll", "d3d12.dll"],
            ProxyPlanner.CandidateOrder("Game.exe", dir.Path, manifest).Take(3));
        Assert.Equal(new ProxyChoice("dbghelp.dll", "from the AMDNR list for Game.exe"),
            ProxyPlanner.Explain("Game.exe", dir.Path, manifest));

        ProxySlotProbeTests.PlaceHandMadeProxy(dir.Path, "winmm.dll");

        Assert.Equal(["winmm.dll", "dbghelp.dll", "dxgi.dll"],
            ProxyPlanner.CandidateOrder("Game.exe", dir.Path, manifest).Take(3));
        Assert.Equal(new ProxyChoice("winmm.dll", "your earlier install used winmm.dll") { FromEarlierInstall = true },
            ProxyPlanner.Explain("Game.exe", dir.Path, manifest));
    }

    [Fact]
    public void Explain_says_first_free_name_when_nothing_else_decides_and_says_so_when_none_is_free()
    {
        using var dir = new TempDir();
        dir.Write("Game.exe", "not a PE at all");

        Assert.Equal(new ProxyChoice("dxgi.dll", "first free name"),
            ProxyPlanner.Explain("Game.exe", dir.Path, MakeManifest()));

        dir.Write("dxgi.dll", "another mod");
        Assert.Equal(new ProxyChoice("d3d12.dll", "first free name"),
            ProxyPlanner.Explain("Game.exe", dir.Path, MakeManifest()));

        foreach (var name in PayloadNames.ProxyNames) dir.Write(name, "another mod");
        Assert.Equal(new ProxyChoice("dxgi.dll", "every name is already taken here; dxgi.dll is first on the list"),
            ProxyPlanner.Explain("Game.exe", dir.Path, MakeManifest()));
    }

    [Fact]
    public void An_exe_that_is_missing_or_unnamed_falls_back_to_the_defaults()
    {
        using var dir = new TempDir();

        Assert.Equal(PayloadNames.ProxyNames, ProxyPlanner.CandidateOrder("Missing.exe", dir.Path, MakeManifest()));
        Assert.Equal(PayloadNames.ProxyNames, ProxyPlanner.CandidateOrder("", dir.Path, MakeManifest()));
        Assert.Equal("first free name", ProxyPlanner.Explain("", dir.Path, MakeManifest()).Reason);
    }

    [Fact]
    public void Next_follows_the_order_the_imports_give()
    {
        using var dir = new TempDir();
        WriteExe(dir, "Game.exe", "d3d11.dll");

        Assert.Equal("d3d11.dll", ProxyPlanner.Next("dxgi.dll", [], "Game.exe", dir.Path, MakeManifest()));
        Assert.Equal("d3d12.dll", ProxyPlanner.Next("d3d11.dll", ["dxgi.dll"], "Game.exe", dir.Path, MakeManifest()));
    }

    [Fact]
    public void Next_never_offers_a_name_another_file_holds()
    {
        // CandidateOrder demotes an occupied name and keeps it, for the picker to grey out. TRY
        // NEXT PROXY cannot use it at all: the installer refuses to move over a file it did not
        // place, before saving anything, so offering the name once would be offering it on
        // every press after that. It is skipped, and null says the free names are used up.
        using var dir = new TempDir();
        File.WriteAllText(Path.Combine(dir.Path, "dxgi.dll"), "ReShade, say");

        Assert.Equal("winmm.dll", ProxyPlanner.Next(
            "d3d12.dll", ["version.dll", "dbghelp.dll", "d3d11.dll"], "Game.exe", dir.Path, MakeManifest()));
        Assert.Null(ProxyPlanner.Next(
            "d3d11.dll", ["d3d12.dll", "winmm.dll", "version.dll", "dbghelp.dll"], "Game.exe", dir.Path, MakeManifest()));
    }
}
