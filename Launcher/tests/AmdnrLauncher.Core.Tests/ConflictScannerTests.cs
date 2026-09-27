// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class ConflictScannerTests
{
    private const string GoodForwarderSha = "F5935DD3089F8262000000000000000000000000000000000000000000000000";

    private static IReadOnlyList<Conflict> Scan(TempDir dir, string proxy = "dxgi.dll")
        => ConflictScanner.Scan(dir.Path, proxy, GoodForwarderSha);

    [Fact]
    public void A_foreign_injection_dll_under_another_proxy_name_is_a_conflict()
    {
        using var dir = new TempDir();
        dir.Write("winmm.dll", "some other mod");

        var conflict = Assert.Single(Scan(dir), c => c.Kind == ConflictKind.InjectionDll);
        Assert.EndsWith("winmm.dll", conflict.Path);
    }

    [Fact]
    public void The_chosen_proxy_name_is_not_reported_as_a_conflict()
    {
        using var dir = new TempDir();
        dir.Write("dxgi.dll", "previous AMDNR install");

        Assert.DoesNotContain(Scan(dir, "dxgi.dll"), c => c.Path.EndsWith("dxgi.dll"));
    }

    [Fact]
    public void A_leftover_version_dll_is_a_conflict_because_it_double_injects()
    {
        using var dir = new TempDir();
        dir.Write("version.dll", "author runtime from a manual install");

        Assert.Contains(Scan(dir), c => c.Path.EndsWith("version.dll") && c.RecommendMoveAside);
    }

    [Fact]
    public void XeFGUnlock_asi_is_stale_and_recommended_for_removal()
    {
        using var dir = new TempDir();
        dir.Write(@"OptiScaler\plugins\XeFGUnlock.asi", "x");

        var conflict = Assert.Single(Scan(dir), c => c.Path.EndsWith("XeFGUnlock.asi"));
        Assert.Equal(ConflictKind.StaleArtifact, conflict.Kind);
        Assert.True(conflict.RecommendMoveAside);
        Assert.Contains("built in", conflict.Reason, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void Old_dlss5_neural_files_are_stale()
    {
        using var dir = new TempDir();
        dir.Write("dlss5-neural.ini", "x");
        dir.Write("dlss5-neural.install.json", "{}");

        Assert.Equal(2, Scan(dir).Count(c => c.Kind == ConflictKind.StaleArtifact));
    }

    [Fact]
    public void A_forwarder_with_the_expected_hash_is_not_flagged()
    {
        using var dir = new TempDir();
        var path = dir.Write(PayloadNames.Forwarder, "x");

        Assert.Empty(ConflictScanner.Scan(dir.Path, "dxgi.dll", Hashing.Sha256OfFile(path)));
    }

    [Fact]
    public void A_forwarder_with_a_different_hash_is_stale()
    {
        using var dir = new TempDir();
        dir.Write(PayloadNames.Forwarder, "an older 1.25 MB build");

        var conflict = Assert.Single(Scan(dir), c => c.Path.EndsWith(PayloadNames.Forwarder));
        Assert.Equal(ConflictKind.StaleArtifact, conflict.Kind);
    }

    [Fact]
    public void Unrelated_debris_is_ignored()
    {
        using var dir = new TempDir();
        // Real folders are full of this; none of it is the launcher's business.
        dir.Write("OptiScaler.ini.bak-bf16ee6b", "x");
        dir.Write("dxgi.dll2", "x");
        dir.Write("Win64.rar", "x");
        dir.Write("dlssnr_on_amd.ini", "[DlssNrOnAmd]");
        Directory.CreateDirectory(Path.Combine(dir.Path, "amd-nr-capture-20260921-161200"));

        Assert.Empty(Scan(dir));
    }

    [Fact]
    public void A_forwarder_that_cannot_be_read_is_reported_rather_than_throwing()
    {
        using var dir = new TempDir();
        var path = dir.Write(PayloadNames.Forwarder, "x");
        var correctHash = Hashing.Sha256OfFile(path);

        using var hold = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.None);

        // Even given the CORRECT hash: a file we cannot read is one we cannot verify,
        // so it is reported rather than silently trusted — and the scan does not throw.
        var conflict = Assert.Single(ConflictScanner.Scan(dir.Path, "dxgi.dll", correctHash));
        Assert.Equal(ConflictKind.StaleArtifact, conflict.Kind);
    }

    [Fact]
    public void A_clean_folder_produces_no_conflicts()
    {
        using var dir = new TempDir();
        dir.Write("Game.exe", "x");

        Assert.Empty(Scan(dir));
    }
}
