// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Diagnostics;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class DoctorTests : IDisposable
{
    private readonly TempDir _dir = new();
    private readonly string _game;

    // Hash of the string "pass-bytes", used as the one accepted runtime build in these tests.
    private readonly string _passSha;
    private readonly string _forwarderSha;
    private readonly string _proxySha;

    public DoctorTests()
    {
        _game = Path.Combine(_dir.Path, "game");
        Directory.CreateDirectory(Path.Combine(_game, "OptiScaler"));

        foreach (var pass in PayloadNames.PassDlls)
            File.WriteAllText(Path.Combine(_game, pass), "pass-bytes");
        File.WriteAllText(Path.Combine(_game, PayloadNames.Weights), new string('w', 4096));
        File.WriteAllText(Path.Combine(_game, PayloadNames.Forwarder), "forwarder-bytes");
        File.WriteAllText(Path.Combine(_game, "dxgi.dll"), "optiscaler-payload");
        File.WriteAllText(Path.Combine(_game, "OptiScaler", "libxess.dll"), "xess");
        File.WriteAllText(Path.Combine(_game, PayloadNames.OptiScalerIni), "LogToFile=true\n");

        _passSha = Hashing.Sha256OfFile(Path.Combine(_game, PayloadNames.PassDlls[0]));
        _forwarderSha = Hashing.Sha256OfFile(Path.Combine(_game, PayloadNames.Forwarder));
        _proxySha = Hashing.Sha256OfFile(Path.Combine(_game, "dxgi.dll"));
    }

    public void Dispose() => _dir.Dispose();

    private Manifest Manifest(string amdnrVersion = "0.2.1", string theAutomaticVersion = "1.8.6-0.3.1") => new(
        SchemaVersion: 1,
        Launcher: new LauncherInfo("1.0.0", "https://h/l.exe", "AA"),
        Packages:
        [
            new PackageInfo("amdnr", amdnrVersion, "https://h/a.zip", "AA", 1, null),
            new PackageInfo("theautomatic", theAutomaticVersion, "https://h/t.zip", "DD", 1, null),
            new PackageInfo("runtime", "0.3.1", "https://h/r.zip", "BB", 1,
                ["0000000000000000000000000000000000000000000000000000000000000000", _passSha]),
            new PackageInfo("forwarder", "0.2.1", "https://h/f.dll", _forwarderSha, 1, null),
        ],
        Compatibility: [new CompatibilityRule("0.2.1", ["0.3.1", "0.3.0"])],
        ProxyDefaults: PayloadNames.ProxyNames,
        ProxyOverrides: [],
        Sources: StubAssetStore.Sources);

    private InstallRecord Record(string proxy = "dxgi.dll", string runtimeVersion = "0.3.1") => new(
        _game, "Test Game", proxy, "amdnr", "0.2.1", runtimeVersion, "0.2.1",
        [], DateTimeOffset.UtcNow, false,
        [new PlacedFile(proxy, _proxySha, PlacementMode.Copy)]);

    /// <summary>TheAutomatic's build as installed without the DLSSNR AMD files: no runtime, no
    /// forwarder, and none of their files in the folder.</summary>
    private InstallRecord TheAutomaticWithoutRuntime()
    {
        foreach (var pass in PayloadNames.PassDlls) File.Delete(Path.Combine(_game, pass));
        File.Delete(Path.Combine(_game, PayloadNames.Weights));
        File.Delete(Path.Combine(_game, PayloadNames.Forwarder));

        return Record() with
        {
            Source = "theautomatic",
            AmdnrVersion = "1.8.6-0.3.1",
            RuntimeVersion = null,
            ForwarderVersion = null,
        };
    }

    [Fact]
    public void A_healthy_install_reports_no_errors()
    {
        var report = Doctor.Run(Record(), Manifest());

        Assert.True(report.IsHealthy);
        Assert.False(report.UpdateAvailable);
        Assert.DoesNotContain(report.Findings, f => f.Severity == DoctorSeverity.Error);
    }

    [Fact]
    public void A_pass_dll_matching_any_accepted_build_passes()
    {
        // _passSha is the second entry in the accepted list, not the first.
        Assert.DoesNotContain(Doctor.Run(Record(), Manifest()).Findings, f => f.Id == "pass.unknownBuild");
    }

    [Fact]
    public void An_unknown_pass_build_is_an_error()
    {
        File.WriteAllText(Path.Combine(_game, PayloadNames.PassDlls[0]), "some other build");

        Assert.Contains(Doctor.Run(Record(), Manifest()).Findings,
            f => f.Id == "pass.unknownBuild" && f.Severity == DoctorSeverity.Error);
    }

    [Fact]
    public void Mixed_pass_versions_are_an_error()
    {
        File.WriteAllText(Path.Combine(_game, PayloadNames.PassDlls[2]), "a different accepted build");

        Assert.Contains(Doctor.Run(Record(), Manifest()).Findings, f => f.Id == "pass.mixedVersions");
    }

    [Fact]
    public void A_missing_weights_file_is_an_error()
    {
        File.Delete(Path.Combine(_game, PayloadNames.Weights));

        Assert.Contains(Doctor.Run(Record(), Manifest()).Findings,
            f => f.Id == "weights.missing" && f.Severity == DoctorSeverity.Error);
    }

    [Fact]
    public void A_stray_version_dll_beside_another_proxy_is_double_injection()
    {
        File.WriteAllText(Path.Combine(_game, "version.dll"), "author runtime");

        Assert.Contains(Doctor.Run(Record("dxgi.dll"), Manifest()).Findings, f => f.Id == "doubleInjection");
    }

    [Fact]
    public void A_newer_manifest_version_sets_UpdateAvailable()
    {
        var report = Doctor.Run(Record(), Manifest(amdnrVersion: "0.3.0"));

        Assert.True(report.UpdateAvailable);
        Assert.Contains(report.Findings, f => f.Id == "proxy.outdated");
    }

    [Fact]
    public void A_changed_prerelease_package_version_is_offered_as_an_update_and_an_equal_one_is_not()
    {
        // Package versions are the publisher's own strings: TheAutomatic's 1.9.1-alpha cannot be
        // parsed as a version at all, and read as "not newer" no TheAutomatic user would ever be
        // offered it. Whatever the manifest calls current is what an update installs.
        var record = TheAutomaticWithoutRuntime();

        var changed = Doctor.Run(record, Manifest(theAutomaticVersion: "1.9.1-alpha"));
        Assert.True(changed.UpdateAvailable);
        Assert.Contains(changed.Findings, f => f.Id == "proxy.outdated" && f.Message.Contains("1.9.1-alpha"));

        // Named, not ranked: the banner says which build it is, and nothing parses whether it is newer.
        Assert.Equal("1.9.1-alpha", changed.AvailableVersion);
        Assert.Null(Doctor.Run(record, Manifest(theAutomaticVersion: record.AmdnrVersion)).AvailableVersion);

        // A build the owner rolled back to differs too, and is offered and named the same way.
        var rolledBack = Doctor.Run(record with { AmdnrVersion = "1.9.1-alpha" }, Manifest());
        Assert.True(rolledBack.UpdateAvailable);
        Assert.Equal("1.8.6-0.3.1", rolledBack.AvailableVersion);

        Assert.False(Doctor.Run(record with { AmdnrVersion = "1.9.1-alpha" },
            Manifest(theAutomaticVersion: "1.9.1-ALPHA")).UpdateAvailable);
    }

    /// <summary>A manifest that lists the runtimes by card, as the shipping one does: 0.4.1
    /// first for RX 9000, 0.3.3 for every card after it. Both accept the same pass build here.</summary>
    private Manifest ManifestWithRuntimesByGeneration() => new(
        SchemaVersion: 1,
        Launcher: new LauncherInfo("1.0.0", "https://h/l.exe", "AA"),
        Packages:
        [
            new PackageInfo("amdnr", "0.2.1", "https://h/a.zip", "AA", 1, null),
            new PackageInfo("runtime-033", "0.3.3", "https://h/r33.zip", "BB", 1, [_passSha]),
            new PackageInfo("runtime-041", "0.4.1", "https://h/r41.zip", "CC", 1, [_passSha]),
            new PackageInfo("forwarder", "0.2.1", "https://h/f.dll", _forwarderSha, 1, null),
        ],
        Compatibility: [],
        ProxyDefaults: PayloadNames.ProxyNames,
        ProxyOverrides: [],
        Sources:
        [
            new SourceInfo("amdnr", "AMDNR", "3zwr1", "s", "https://h", "amdnr", "forwarder",
                RuntimeRequired: false, RuntimeNote: "n",
                Runtimes:
                [
                    new RuntimeChoice("runtime-041", ["rdna4"]),
                    new RuntimeChoice("runtime-033", ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"]),
                ]),
        ]);

    [Fact]
    public void A_changed_default_runtime_for_the_card_is_offered_as_an_update()
    {
        // The manifest's order for a card is the publisher's word on which runtime it gets. A
        // game installed when another was first is offered the move — the way Daniel's next
        // runtime reaches players without a new launcher — and the note names both versions.
        var record = Record() with { RuntimeId = "runtime-033", RuntimeVersion = "0.3.3" };

        var rdna4 = Doctor.Run(record, ManifestWithRuntimesByGeneration(), "rdna4");

        Assert.True(rdna4.UpdateAvailable);
        Assert.Equal("0.4.1", rdna4.AvailableRuntimeVersion);
        Assert.Null(rdna4.AvailableVersion);
        var finding = Assert.Single(rdna4.Findings, f => f.Id == "runtime.outdated");
        Assert.Equal(DoctorSeverity.Warning, finding.Severity);
        Assert.Equal("Installed DLSSNR AMD files 0.3.3; 0.4.1 is now the runtime for this card.", finding.Message);
        Assert.Equal("Update", finding.SuggestedAction);
        Assert.True(rdna4.IsHealthy);

        // On a card whose runtime is still 0.3.3, nothing changed.
        var rdna3 = Doctor.Run(record, ManifestWithRuntimesByGeneration(), "rdna3");
        Assert.False(rdna3.UpdateAvailable);
        Assert.Null(rdna3.AvailableRuntimeVersion);
        Assert.DoesNotContain(rdna3.Findings, f => f.Id == "runtime.outdated");
    }

    [Fact]
    public void A_runtime_the_card_already_has_or_never_had_is_not_an_update()
    {
        var manifest = ManifestWithRuntimesByGeneration();

        // Already on the card's runtime.
        var current = Record() with { RuntimeId = "runtime-041", RuntimeVersion = "0.4.1" };
        Assert.False(Doctor.Run(current, manifest, "rdna4").UpdateAvailable);

        // Installed without the files — No at the chooser, or the lmxxf path: nothing to move.
        var without = Record() with { RuntimeId = null, RuntimeVersion = null };
        Assert.False(Doctor.Run(without, manifest, "rdna4").UpdateAvailable);

        // A card the build names no runtime for keeps what it has.
        var old = Record() with { RuntimeId = "runtime-033", RuntimeVersion = "0.3.3" };
        Assert.False(Doctor.Run(old, manifest, "rdna1").UpdateAvailable);

        // Without knowing the card nothing is offered: a wrong runtime is worse than an old one.
        Assert.False(Doctor.Run(old, manifest).UpdateAvailable);

        // A build written before runtimes were listed by card names one runtime for everyone,
        // and a record from that time is on it.
        Assert.False(Doctor.Run(Record(), Manifest(), "rdna4").UpdateAvailable);
    }

    [Fact]
    public void The_runtime_the_user_picked_outranks_the_cards_first_listed_one()
    {
        // 0.4.1 first for rdna4; the user picked 0.3.3 in the chooser. The game on 0.3.3 stays,
        // the one on 0.4.1 is offered the move — a manual choice is a choice.
        var manifest = ManifestWithRuntimesByGeneration();
        var on033 = Record() with { RuntimeId = "runtime-033", RuntimeVersion = "0.3.3" };
        var on041 = Record() with { RuntimeId = "runtime-041", RuntimeVersion = "0.4.1" };

        Assert.False(Doctor.Run(on033, manifest, "rdna4", "runtime-033").UpdateAvailable);
        Assert.Equal("0.3.3", Doctor.Run(on041, manifest, "rdna4", "runtime-033").AvailableRuntimeVersion);

        // A pick the build does not list for this card is no pick: the card's first runtime decides.
        Assert.Equal("0.4.1", Doctor.Run(on033, manifest, "rdna4", "runtime-999").AvailableRuntimeVersion);
        Assert.False(Doctor.Run(on033, manifest, "rdna3", "runtime-041").UpdateAvailable);
    }

    [Fact]
    public void An_RE_Engine_game_without_REFramework_is_a_Doctor_warning_not_an_error()
    {
        File.WriteAllText(Path.Combine(_game, "re_chunk_000.pak"), "archive");

        var report = Doctor.Run(Record(), Manifest());

        var finding = Assert.Single(report.Findings, f => f.Id == "reframework.missing");
        Assert.Equal(DoctorSeverity.Warning, finding.Severity);
        Assert.Equal(ReEngine.MissingReframework, finding.Message);
        Assert.True(report.IsHealthy);

        // REFramework in place: nothing to say.
        File.WriteAllBytes(Path.Combine(_game, "dinput8.dll"), new byte[5 * 1024 * 1024]);
        Assert.DoesNotContain(Doctor.Run(Record(), Manifest()).Findings, f => f.Id == "reframework.missing");
    }

    [Fact]
    public void A_matching_version_does_not_set_UpdateAvailable()
    {
        // Guards the bug this check replaced: comparing the installed DLL against the
        // package's zip hash would flag every healthy install as outdated forever.
        Assert.False(Doctor.Run(Record(), Manifest()).UpdateAvailable);
    }

    [Fact]
    public void A_replaced_proxy_file_is_an_error()
    {
        File.WriteAllText(Path.Combine(_game, "dxgi.dll"), "someone else's dll");

        Assert.Contains(Doctor.Run(Record(), Manifest()).Findings,
            f => f.Id == "proxy.replaced" && f.Severity == DoctorSeverity.Error);
    }

    [Fact]
    public void An_incompatible_runtime_version_is_reported()
    {
        Assert.Contains(Doctor.Run(Record(runtimeVersion: "0.2.17"), Manifest()).Findings,
            f => f.Id == "compat.mismatch");
    }

    [Fact]
    public void A_stale_forwarder_is_reported()
    {
        File.WriteAllText(Path.Combine(_game, PayloadNames.Forwarder), "an older build");

        Assert.Contains(Doctor.Run(Record(), Manifest()).Findings, f => f.Id == "forwarder.stale");
    }

    [Fact]
    public void Ini_findings_are_included()
    {
        File.WriteAllText(Path.Combine(_game, PayloadNames.OptiScalerIni), "LogToFile=false\n");

        Assert.Contains(Doctor.Run(Record(), Manifest()).Findings, f => f.Id == "ini.LogToFile");
    }

    [Fact]
    public void Unknown_files_are_not_faults()
    {
        File.WriteAllText(Path.Combine(_game, "OptiScaler.ini.bak-bf16ee6b"), "x");
        File.WriteAllText(Path.Combine(_game, "dxgi.dll2"), "x");
        File.WriteAllText(Path.Combine(_game, "Win64.rar"), "x");
        File.WriteAllText(Path.Combine(_game, PayloadNames.AuthorIni), "[DlssNrOnAmd]");
        Directory.CreateDirectory(Path.Combine(_game, "amd-nr-capture-20260921-161200"));

        Assert.True(Doctor.Run(Record(), Manifest()).IsHealthy);
    }

    [Fact]
    public void Reads_the_build_stamp_from_the_first_line_of_the_log()
    {
        File.WriteAllText(Path.Combine(_game, "OptiScaler.log"),
            "[03:54:47.245093] [W] OptiScaler v10.0.0-dev (unknown) (20260922_035320) loaded\nmore\n");

        Assert.Contains("20260922_035320", Doctor.Run(Record(), Manifest()).BuildStamp);
    }

    [Fact]
    public void A_file_that_cannot_be_read_is_reported_without_crashing_the_report()
    {
        // Doctor is what the user opens when something is already wrong. A locked file —
        // the game still running is the ordinary cause — must not take the report down.
        var pass1 = Path.Combine(_game, PayloadNames.PassDlls[0]);
        using var hold = new FileStream(pass1, FileMode.Open, FileAccess.Read, FileShare.None);

        var report = Doctor.Run(Record(), Manifest());

        Assert.Contains(report.Findings, f => f.Id == "file.unreadable");
        // An unreadable pass DLL must not also manufacture a mixed-versions finding.
        Assert.DoesNotContain(report.Findings, f => f.Id == "pass.mixedVersions");
    }

    private Manifest ManifestWithPassHashes(IReadOnlyList<string>? hashes) => Manifest() with
    {
        Packages = Manifest().Packages
            .Select(p => string.Equals(p.Id, "runtime", StringComparison.OrdinalIgnoreCase)
                ? p with { PassSha256 = hashes }
                : p)
            .ToList(),
    };

    [Theory]
    [InlineData(true)]      // the field is present but empty
    [InlineData(false)]     // the field is absent, or misspelled and silently dropped
    public void A_manifest_with_no_accepted_runtime_builds_warns_instead_of_passing(bool empty)
    {
        // This is the check that exists to catch spec §2's "Private AMD runtime hash mismatch",
        // the most common real breakage. Passing every pass DLL unconditionally with nothing
        // said turns Doctor into something that reports fine when it cannot check anything.
        var report = Doctor.Run(Record(), ManifestWithPassHashes(empty ? [] : null));

        var finding = Assert.Single(report.Findings, f => f.Id == "pass.unverifiable");
        Assert.Equal(DoctorSeverity.Warning, finding.Severity);
        Assert.DoesNotContain(report.Findings, f => f.Id == "pass.unknownBuild");
    }

    [Fact]
    public void An_install_that_never_finished_is_an_error()
    {
        // The record is written before the first file lands, so one still marked unfinished
        // means the install died part-way — a power cut, a kill — with no chance to roll back.
        var report = Doctor.Run(Record() with { Completed = false }, Manifest());

        Assert.Contains(report.Findings,
            f => f.Id == "install.incomplete" && f.Severity == DoctorSeverity.Error);
        Assert.False(report.IsHealthy);
    }

    /// <summary>A stale or hand-edited cached manifest can be missing any package. Doctor runs
    /// for every installed game on every scan, off an async void command with no catch, so a
    /// KeyNotFoundException here kills the process rather than showing a finding.</summary>
    private Manifest ManifestWithout(string packageId) => Manifest() with
    {
        Packages = Manifest().Packages
            .Where(p => !string.Equals(p.Id, packageId, StringComparison.OrdinalIgnoreCase))
            .ToList(),
    };

    [Theory]
    [InlineData("amdnr")]
    [InlineData("runtime")]
    [InlineData("forwarder")]
    public void A_manifest_missing_a_package_is_reported_rather_than_thrown(string packageId)
    {
        var report = Doctor.Run(Record(), ManifestWithout(packageId));

        Assert.Contains(report.Findings,
            f => f.Id == "manifest.incomplete" && f.Severity == DoctorSeverity.Warning);
    }

    [Fact]
    public void The_runtime_is_checked_against_the_package_the_record_names()
    {
        // The 0.3.3.1 manifest lists runtime-033 and runtime-040 and no package called
        // "runtime". A Doctor that looked "runtime" up would find nothing, call every new
        // install unverifiable, and never again catch 'Private AMD runtime hash mismatch' —
        // the most common real breakage — for anyone.
        var manifest = Manifest() with
        {
            Packages = Manifest().Packages
                .Select(p => p.Id == "runtime" ? p with { Id = "runtime-040", Version = "0.4.0" } : p)
                .ToList(),
        };

        var report = Doctor.Run(Record() with { RuntimeId = "runtime-040" }, manifest);

        Assert.DoesNotContain(report.Findings,
            f => f.Id is "manifest.incomplete" or "pass.unverifiable" or "pass.unknownBuild");
    }

    [Fact]
    public void A_manifest_missing_the_runtime_the_record_names_says_which()
    {
        var report = Doctor.Run(Record() with { RuntimeId = "runtime-033" }, Manifest());

        Assert.Contains(report.Findings,
            f => f.Id == "manifest.incomplete" && f.Message.Contains("runtime-033"));
    }

    [Fact]
    public void A_record_that_names_no_runtime_package_is_checked_against_runtime()
    {
        // The id every record written before runtimes had names installed from. Also what a
        // hand-edited record that lost the member falls back to, rather than nothing.
        var report = Doctor.Run(Record() with { RuntimeId = null }, Manifest());

        Assert.DoesNotContain(report.Findings, f => f.Id is "manifest.incomplete" or "pass.unverifiable");
    }

    [Fact]
    public void Doctor_warns_when_a_runtime_required_source_was_installed_without_it()
    {
        // TheAutomatic's build ships no neural runtime of its own. Installed without the
        // DLSSNR AMD files it loads, but Neural Rendering never runs — and without this the
        // row would read as healthy while the one thing the user installed it for is off.
        var report = Doctor.Run(TheAutomaticWithoutRuntime(), Manifest());

        var finding = Assert.Single(report.Findings, f => f.Id == "runtime.notInstalled");
        Assert.Equal(DoctorSeverity.Warning, finding.Severity);
        Assert.Equal("The DLSSNR AMD files are not installed, so Neural Rendering cannot run with this build.",
            finding.Message);

        // Not installed is not missing: none of the checks for files that were never placed.
        Assert.DoesNotContain(report.Findings, f => f.Id.StartsWith("pass.") || f.Id.StartsWith("weights."));
        Assert.DoesNotContain(report.Findings, f => f.Id.StartsWith("forwarder."));
        Assert.DoesNotContain(report.Findings, f => f.Id == "compat.mismatch" || f.Id == "manifest.incomplete");
        Assert.True(report.IsHealthy);
    }

    [Fact]
    public void A_source_that_does_not_need_the_runtime_is_not_warned_about_it()
    {
        // AMDNR can run on its built-in lmxxf runtime on RX 9000.
        foreach (var pass in PayloadNames.PassDlls) File.Delete(Path.Combine(_game, pass));
        File.Delete(Path.Combine(_game, PayloadNames.Weights));

        var report = Doctor.Run(Record() with { RuntimeVersion = null }, Manifest());

        Assert.DoesNotContain(report.Findings, f => f.Id == "runtime.notInstalled");
        Assert.DoesNotContain(report.Findings, f => f.Id.StartsWith("pass.") || f.Id.StartsWith("weights."));
        Assert.True(report.IsHealthy);
    }

    [Fact]
    public void Update_detection_follows_the_records_own_source()
    {
        // A new AMDNR is not an update for someone running TheAutomatic's build, and the
        // other way round: offering it would install a build the user did not choose.
        var record = TheAutomaticWithoutRuntime();

        Assert.False(Doctor.Run(record, Manifest(amdnrVersion: "9.9.9")).UpdateAvailable);

        var newer = Doctor.Run(record, Manifest(theAutomaticVersion: "1.8.7-0.3.1"));
        Assert.True(newer.UpdateAvailable);
        Assert.Contains(newer.Findings, f => f.Id == "proxy.outdated" && f.Message.Contains("OptiScaler AMD pre-SR"));
    }

    [Fact]
    public void A_record_whose_source_the_manifest_no_longer_lists_is_reported_rather_than_thrown()
    {
        var report = Doctor.Run(Record() with { Source = "gone" }, Manifest());

        Assert.Contains(report.Findings,
            f => f.Id == "manifest.incomplete" && f.Severity == DoctorSeverity.Warning);
        Assert.False(report.UpdateAvailable);
    }

    [Fact]
    public void A_manifest_without_sources_still_checks_and_updates_an_AMDNR_install()
    {
        // An older cached manifest used offline lists the amdnr package but no builds. Reading
        // that as "amdnr is not listed" tells the user something false and loses the update.
        var report = Doctor.Run(Record(), Manifest(amdnrVersion: "9.9.9") with { Sources = null });

        Assert.DoesNotContain(report.Findings, f => f.Id == "manifest.incomplete");
        Assert.True(report.UpdateAvailable);
        Assert.Contains(report.Findings, f => f.Id == "proxy.outdated" && f.Message.Contains("AMDNR"));
    }

    [Fact]
    public void Findings_name_the_build_that_is_installed()
    {
        // Someone running TheAutomatic's build who is told the folder "holds only part of AMDNR"
        // goes looking for help with a build they do not have, from an author who did not ship it.
        var record = TheAutomaticWithoutRuntime() with { Completed = false };

        var report = Doctor.Run(record, Manifest(theAutomaticVersion: "1.8.7-0.3.1"));

        Assert.Contains(report.Findings,
            f => f.Id == "install.incomplete" && f.Message.Contains("OptiScaler AMD pre-SR"));
        Assert.All(report.Findings, f => Assert.DoesNotContain("AMDNR", f.Message));
    }

    [Fact]
    public void The_report_names_the_build_the_record_came_from()
    {
        // The stage's update banner says which build REPAIR / UPDATE will put in, and that is
        // the record's build, not the launcher's own.
        Assert.Equal("OptiScaler AMD pre-SR", Doctor.Run(TheAutomaticWithoutRuntime(), Manifest()).BuildName);
        Assert.Equal("AMDNR", Doctor.Run(Record(), Manifest()).BuildName);
    }
}
