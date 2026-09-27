// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Diagnostics;

namespace AmdnrLauncher.Core.Install;

public enum GateCode { WindowsAppsRejected, NotWritable, GameRunning, AntiCheatDetected, AntiCheatBlocked, ReframeworkMissing }

/// <summary>Products names the anti-cheat systems behind an AntiCheatDetected finding, so the
/// launcher can word its own warning around the game's name; it is empty for every other code.</summary>
public sealed record GateFinding(GateCode Code, string Message, bool Blocking, IReadOnlyList<string>? Products = null)
{
    public IReadOnlyList<string> Products { get; init; } = Products ?? [];
}

public sealed record GateResult(IReadOnlyList<GateFinding> Findings)
{
    public bool CanProceedWithoutConsent => Findings.All(f => !f.Blocking);

    /// <summary>True when something no confirmation can fix is in the way. Anti-cheat the user
    /// may consent to is excluded; the one that blocks outright is not.</summary>
    public bool IsHardBlocked =>
        Findings.Any(f => f.Blocking && f.Code != GateCode.AntiCheatDetected);
}

public static class InstallGate
{
    /// <summary>installRoot is the store's own folder for the game, which is usually a level or
    /// three above the exe: the anti-cheat runtime lives up there, not beside the exe.</summary>
    public static GateResult Evaluate(string exeDirectory, string? installRoot = null)
    {
        var findings = new List<GateFinding>();

        if (IsUnderWindowsApps(exeDirectory))
        {
            findings.Add(new GateFinding(
                GateCode.WindowsAppsRejected,
                "This game is installed under WindowsApps, which Windows keeps read-only " +
                "(owned by TrustedInstaller). AMDNR cannot be installed there.",
                Blocking: true));

            // Nothing else is worth probing once the path is off limits.
            return new GateResult(findings);
        }

        if (!IsWritable(exeDirectory))
        {
            findings.Add(new GateFinding(
                GateCode.NotWritable,
                "This folder cannot be written to. Try restarting the launcher as administrator.",
                Blocking: true));
        }

        var running = RunningProcessesIn(exeDirectory);
        if (running.Count > 0)
        {
            findings.Add(new GateFinding(
                GateCode.GameRunning,
                $"Close the game first — still running: {string.Join(", ", running)}.",
                Blocking: true));
        }

        var detected = AntiCheatDetector.Detect(exeDirectory, installRoot);
        var blocked = detected.Where(AntiCheatDetector.BlocksInstall).ToList();
        var antiCheat = detected.Except(blocked).ToList();

        if (blocked.Count > 0)
        {
            findings.Add(new GateFinding(
                GateCode.AntiCheatBlocked,
                "Call of Duty's Ricochet anti-cheat bans accounts for injected DLLs, and these games " +
                "have no offline mode where that is safe. AMDNR will not install here.",
                Blocking: true,
                blocked));
        }

        if (antiCheat.Count > 0)
        {
            findings.Add(new GateFinding(
                GateCode.AntiCheatDetected,
                $"This game uses {string.Join(" and ", antiCheat)}. Injecting a proxy DLL into a " +
                "game protected by anti-cheat can get your account banned. Continue only if you " +
                "understand that risk.",
                Blocking: true,
                antiCheat));
        }

        // Neither a block nor a consent: the install itself works, the game then closes itself.
        // Said here, before INSTALL, where the anti-cheat line is said, and again by the Doctor
        // afterwards — the fix is a file the user adds, not one the launcher places.
        if (ReEngine.MissingReframeworkWarning(exeDirectory) is { } reframework)
            findings.Add(new GateFinding(GateCode.ReframeworkMissing, reframework, Blocking: false));

        return new GateResult(findings);
    }

    private static bool IsUnderWindowsApps(string path) =>
        path.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
            .Any(segment => string.Equals(segment, "WindowsApps", StringComparison.OrdinalIgnoreCase));

    private static bool IsWritable(string directory)
    {
        if (!Directory.Exists(directory)) return false;
        var probe = Path.Combine(directory, ".amdnr-write-probe-" + Guid.NewGuid().ToString("N"));
        try
        {
            File.WriteAllText(probe, "");
            return true;      // we could write; that is the whole question
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
        finally
        {
            // Cleanup belongs here, not in the try: if the write succeeded and only the
            // delete failed, the old shape returned "not writable" AND left the probe
            // behind. A stray file in someone's game folder is exactly the debris this
            // launcher exists to avoid creating.
            try { File.Delete(probe); }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
        }
    }

    private static List<string> RunningProcessesIn(string directory)
    {
        var normalized = Path.TrimEndingDirectorySeparator(Path.GetFullPath(directory));
        var hits = new List<string>();

        foreach (var process in Process.GetProcesses())
        {
            try
            {
                var image = process.MainModule?.FileName;
                if (image is null) continue;

                var imageDirectory = Path.TrimEndingDirectorySeparator(
                    Path.GetDirectoryName(Path.GetFullPath(image)) ?? "");

                if (string.Equals(imageDirectory, normalized, StringComparison.OrdinalIgnoreCase))
                    hits.Add(process.ProcessName + ".exe");
            }
            catch (Exception e) when (e is InvalidOperationException
                                        or System.ComponentModel.Win32Exception
                                        or NotSupportedException)
            {
                // Access denied or the process exited mid-enumeration; skip it.
            }
            finally
            {
                process.Dispose();
            }
        }

        return hits.Distinct(StringComparer.OrdinalIgnoreCase).ToList();
    }
}
