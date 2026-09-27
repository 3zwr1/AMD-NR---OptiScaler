// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class ProxySlotProbeTests
{
    /// <summary>Built by the AmdnrLauncher.Fixtures.OptiScaler project, whose AssemblyName gives
    /// it the version resource a real OptiScaler build carries: OriginalFilename=OptiScaler.dll.</summary>
    public static string FixtureOptiScalerDll => Path.Combine(AppContext.BaseDirectory, "OptiScaler.dll");

    /// <summary>What someone installing by hand leaves: OptiScaler.dll renamed to a proxy name.</summary>
    public static string PlaceHandMadeProxy(string gameDirectory, string proxy)
    {
        var path = Path.Combine(gameDirectory, proxy);
        File.Copy(FixtureOptiScalerDll, path, overwrite: true);
        return path;
    }

    [Fact]
    public void Classify_recognises_the_fixture_OptiScaler_dll_under_a_proxy_name()
    {
        using var dir = new TempDir();
        var proxy = PlaceHandMadeProxy(dir.Path, "winmm.dll");

        Assert.Equal(SlotContent.OptiScaler, ProxySlotProbe.Classify(proxy));
    }

    [Fact]
    public void Classify_reports_Other_for_a_non_PE_file_and_Empty_for_a_missing_one()
    {
        using var dir = new TempDir();
        var text = dir.Write("dxgi.dll", "not a PE file at all");

        Assert.Equal(SlotContent.Other, ProxySlotProbe.Classify(text));
        Assert.Equal(SlotContent.Empty, ProxySlotProbe.Classify(Path.Combine(dir.Path, "d3d12.dll")));
    }

    [Fact]
    public void Classify_reports_another_DLL_as_Other()
    {
        // A real PE whose version resource names something else — ReShade, a DXVK build — is
        // exactly what must not be mistaken for an earlier OptiScaler install.
        using var dir = new TempDir();
        var other = Path.Combine(dir.Path, "dxgi.dll");
        File.Copy(typeof(ProxySlotProbe).Assembly.Location, other);

        Assert.Equal(SlotContent.Other, ProxySlotProbe.Classify(other));
    }

    [Fact]
    public void Classify_reports_a_file_it_cannot_open_as_Unreadable()
    {
        using var dir = new TempDir();
        var proxy = PlaceHandMadeProxy(dir.Path, "dxgi.dll");

        using (new FileStream(proxy, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
            Assert.Equal(SlotContent.Unreadable, ProxySlotProbe.Classify(proxy));
    }

    [Fact]
    public void FindOptiScalerProxy_returns_the_proxy_name_that_holds_OptiScaler()
    {
        using var dir = new TempDir();
        dir.Write("dxgi.dll", "ReShade, say");
        PlaceHandMadeProxy(dir.Path, "version.dll");

        Assert.Equal("version.dll", ProxySlotProbe.FindOptiScalerProxy(dir.Path));
    }

    [Fact]
    public void FindOptiScalerProxy_returns_null_without_one()
    {
        using var dir = new TempDir();
        dir.Write("dxgi.dll", "ReShade, say");

        // dinput8 is never a proxy name this launcher uses, so it is not one it recognises either.
        PlaceHandMadeProxy(dir.Path, "dinput8.dll");

        Assert.Null(ProxySlotProbe.FindOptiScalerProxy(dir.Path));
    }
}
