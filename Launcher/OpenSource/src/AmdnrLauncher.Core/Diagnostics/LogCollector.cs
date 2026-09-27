// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO.Compression;
using System.Text;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Diagnostics;

public static class LogCollector
{
    public static IReadOnlyList<string> FindLogs(string exeDirectory)
    {
        // Store builds redirect writes into _storage_, so treat it as a second home.
        var roots = new[] { exeDirectory, Path.Combine(exeDirectory, "_storage_") };

        return (from root in roots
                where Directory.Exists(root)
                from name in PayloadNames.LogFiles
                let path = Path.Combine(root, name)
                where File.Exists(path)
                select path).ToList();
    }

    /// <summary>A log found under `_storage_` keeps that prefix in the zip. Without it, a log
    /// present in both places would produce two entries with the same name, which extraction
    /// tools either reject or silently overwrite — and the prefix also tells the reader which
    /// location the file came from, which is itself diagnostic.</summary>
    private static string EntryNameFor(string logPath)
    {
        var name = Path.GetFileName(logPath);
        var parent = Path.GetFileName(Path.GetDirectoryName(logPath) ?? string.Empty);

        return string.Equals(parent, "_storage_", StringComparison.OrdinalIgnoreCase)
            ? "_storage_/" + name
            : name;
    }

    public static string CreateSummary(InstallRecord record, DoctorReport report, GpuInfo gpu)
    {
        var sb = new StringBuilder();

        // First line answers "which build are you running?" before it gets asked.
        sb.AppendLine(report.BuildStamp ?? "(no build stamp — OptiScaler.log not found or empty)");
        sb.AppendLine();
        sb.AppendLine($"Game:     {record.GameName}");
        sb.AppendLine($"Folder:   {record.ExeDirectory}");
        sb.AppendLine($"Proxy:    {record.Proxy}");
        sb.AppendLine($"Build:    {record.Source} {record.AmdnrVersion}");
        sb.AppendLine($"Runtime:  {record.RuntimeVersion ?? "not installed"}");
        // The generation in the manifest's own word for it — that word is what chose the
        // runtime this game got, so a reader can check the two against each other.
        sb.AppendLine($"GPU:      {gpu.Name} ({gpu.Generation}) driver {gpu.DriverVersion ?? "unknown"}");
        sb.AppendLine($"Installed:{record.InstalledAtUtc:u}");
        sb.AppendLine();

        if (report.Findings.Count == 0)
        {
            sb.AppendLine("Doctor: no findings.");
        }
        else
        {
            sb.AppendLine("Doctor findings:");
            foreach (var finding in report.Findings)
                sb.AppendLine($"  [{finding.Severity}] {finding.Id}: {finding.Message}");
        }

        var logs = FindLogs(record.ExeDirectory);
        sb.AppendLine();
        sb.AppendLine(logs.Count == 0
            ? "Logs: none found."
            : "Logs: " + string.Join(", ", logs.Select(EntryNameFor)));

        return sb.ToString();
    }

    /// <summary>Returns the zip path, or null when the zip could not be written. Report
    /// generation runs when the user is already in trouble, so it must not add a crash of its
    /// own — and the caller still has the summary, which is the part that actually gets
    /// pasted.</summary>
    public static string? CreateReportZip(
        InstallRecord record, DoctorReport report, GpuInfo gpu, string zipPath)
    {
        var summary = CreateSummary(record, report, gpu);

        try
        {
            var directory = Path.GetDirectoryName(zipPath);
            if (!string.IsNullOrEmpty(directory)) Directory.CreateDirectory(directory);
            if (File.Exists(zipPath)) File.Delete(zipPath);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException)
        {
            return null;
        }

        try
        {
            using var zip = ZipFile.Open(zipPath, ZipArchiveMode.Create);

            var summaryEntry = zip.CreateEntry("summary.txt");
            using (var writer = new StreamWriter(summaryEntry.Open()))
                writer.Write(summary);

            foreach (var log in FindLogs(record.ExeDirectory))
            {
                try
                {
                    // The game may still hold a log open; copy through a shared handle.
                    var entry = zip.CreateEntry(EntryNameFor(log));
                    using var source = new FileStream(log, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    using var target = entry.Open();
                    source.CopyTo(target);
                }
                // UnauthorizedAccessException is not an IOException — both have to be named, or a
                // permission-denied log takes down the report the user is trying to send us.
                catch (Exception e) when (e is IOException or UnauthorizedAccessException)
                {
                    /* skip a log we cannot read; the rest of the report is still worth having */
                }
            }

            return zipPath;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return null;
        }
    }
}
