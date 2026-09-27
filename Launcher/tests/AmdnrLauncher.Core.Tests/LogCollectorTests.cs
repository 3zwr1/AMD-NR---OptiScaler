// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO.Compression;
using AmdnrLauncher.Core.Diagnostics;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class LogCollectorTests : IDisposable
{
    private readonly TempDir _dir = new();
    private readonly string _game;

    public LogCollectorTests()
    {
        _game = Path.Combine(_dir.Path, "game");
        Directory.CreateDirectory(_game);
    }

    public void Dispose() => _dir.Dispose();

    private InstallRecord Record() => new(
        _game, "SILENT HILL 2", "dxgi.dll", "amdnr", "0.2.1", "0.3.1", "0.2.1",
        [], DateTimeOffset.UnixEpoch, false, []);

    private static DoctorReport Report(params DoctorFinding[] findings)
        => new(findings, "OptiScaler v10.0.0-dev (unknown) (20260922_035320) loaded", false);

    private static readonly GpuInfo Gpu = new("Radeon RX 7900 XTX", "rdna3", "25.9.1", null);

    [Fact]
    public void Summary_names_the_GPU_its_generation_and_driver()
    {
        // The first question in every support thread. The generation is the manifest's own
        // word for it, because that word is what chose the runtime this game got.
        var gpu = new GpuInfo("AMD Radeon RX 9070 XT", "rdna4", "32.0.31041.3013", @"PCI\VEN_1002&DEV_7550&REV_C0");

        var summary = LogCollector.CreateSummary(Record(), Report(), gpu);

        Assert.Contains("GPU:      AMD Radeon RX 9070 XT (rdna4) driver 32.0.31041.3013", summary);
        Assert.DoesNotContain("not queried", summary);

        // Nothing readable is said as such, not left as a blank that reads like a lost line.
        Assert.Contains("GPU:      Unknown (none) driver unknown",
            LogCollector.CreateSummary(Record(), Report(), GpuInfo.Unknown));
    }

    [Fact]
    public void Finds_logs_beside_the_proxy()
    {
        File.WriteAllText(Path.Combine(_game, "OptiScaler.log"), "x");
        File.WriteAllText(Path.Combine(_game, "amd_presr.log"), "x");

        var logs = LogCollector.FindLogs(_game);

        Assert.Equal(2, logs.Count);
    }

    [Fact]
    public void Finds_logs_redirected_into_a_storage_folder()
    {
        var storage = Path.Combine(_game, "_storage_");
        Directory.CreateDirectory(storage);
        File.WriteAllText(Path.Combine(storage, "dlssnr_on_amd.log"), "x");

        Assert.Single(LogCollector.FindLogs(_game), p => p.Contains("_storage_"));
    }

    [Fact]
    public void Summary_starts_with_the_build_stamp()
    {
        var summary = LogCollector.CreateSummary(Record(), Report(), Gpu);

        // Deliberately NOT trimmed first: a blank line ahead of the stamp would be a
        // regression, and TrimStart would hide it.
        Assert.StartsWith("OptiScaler v10.0.0-dev (unknown) (20260922_035320) loaded",
            summary.Split('\n')[0].Trim());
    }

    [Fact]
    public void Summary_names_the_game_proxy_versions_and_gpu()
    {
        var summary = LogCollector.CreateSummary(Record(), Report(), Gpu);

        Assert.Contains("SILENT HILL 2", summary);
        Assert.Contains("dxgi.dll", summary);
        Assert.Contains("0.2.1", summary);
        Assert.Contains("0.3.1", summary);
        Assert.Contains("Radeon RX 7900 XTX", summary);
    }

    [Fact]
    public void Summary_names_the_build_and_says_when_the_runtime_was_not_installed()
    {
        // A support thread cannot tell "installed without the DLSSNR AMD files" from "the
        // summary lost a line" when the runtime line is simply blank.
        var record = Record() with { Source = "theautomatic", AmdnrVersion = "1.8.6-0.3.1", RuntimeVersion = null };

        var summary = LogCollector.CreateSummary(record, Report(), Gpu);

        Assert.Contains("theautomatic 1.8.6-0.3.1", summary);
        Assert.Contains("Runtime:  not installed", summary);
    }

    [Fact]
    public void Summary_lists_doctor_findings()
    {
        var summary = LogCollector.CreateSummary(
            Record(),
            Report(new DoctorFinding("weights.missing", DoctorSeverity.Error, "weights gone", "Repair")),
            Gpu);

        Assert.Contains("weights.missing", summary);
        Assert.Contains("weights gone", summary);
    }

    [Fact]
    public void Summary_says_so_when_the_build_stamp_is_unavailable()
    {
        var summary = LogCollector.CreateSummary(Record(), new DoctorReport([], null, false), Gpu);

        Assert.Contains("no build stamp", summary, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void Report_zip_contains_the_logs_and_the_summary()
    {
        File.WriteAllText(Path.Combine(_game, "OptiScaler.log"), "log body");
        var zipPath = Path.Combine(_dir.Path, "report.zip");

        LogCollector.CreateReportZip(Record(), Report(), Gpu, zipPath);

        using var zip = ZipFile.OpenRead(zipPath);
        Assert.Contains(zip.Entries, e => e.Name == "OptiScaler.log");
        Assert.Contains(zip.Entries, e => e.Name == "summary.txt");
    }

    [Fact]
    public void Report_zip_is_still_produced_when_no_logs_exist()
    {
        var zipPath = Path.Combine(_dir.Path, "report.zip");

        LogCollector.CreateReportZip(Record(), Report(), Gpu, zipPath);

        using var zip = ZipFile.OpenRead(zipPath);
        Assert.Single(zip.Entries, e => e.Name == "summary.txt");
    }

    [Fact]
    public void A_log_present_in_both_locations_produces_two_distinct_zip_entries()
    {
        // A store build can leave one copy beside the proxy and one under _storage_.
        // Two zip entries with the same name are rejected or silently mangled on extraction.
        File.WriteAllText(Path.Combine(_game, "OptiScaler.log"), "beside the proxy");
        var storage = Path.Combine(_game, "_storage_");
        Directory.CreateDirectory(storage);
        File.WriteAllText(Path.Combine(storage, "OptiScaler.log"), "redirected");
        var zipPath = Path.Combine(_dir.Path, "report.zip");

        LogCollector.CreateReportZip(Record(), Report(), Gpu, zipPath);

        using var zip = ZipFile.OpenRead(zipPath);
        Assert.Contains(zip.Entries, e => e.FullName == "OptiScaler.log");
        Assert.Contains(zip.Entries, e => e.FullName == "_storage_/OptiScaler.log");
    }

    [Fact]
    public void A_zip_that_cannot_be_written_returns_null_instead_of_throwing()
    {
        // Report generation runs when the user is already in trouble. It must not add a
        // crash of its own — the caller still has the summary, which is what gets pasted.
        var zipPath = Path.Combine(_dir.Path, "locked.zip");
        File.WriteAllText(zipPath, "existing");
        using var hold = new FileStream(zipPath, FileMode.Open, FileAccess.Read, FileShare.None);

        Assert.Null(LogCollector.CreateReportZip(Record(), Report(), Gpu, zipPath));
    }
}
