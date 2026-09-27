// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Diagnostics;

public enum DoctorSeverity { Ok, Warning, Error }

public sealed record DoctorFinding(string Id, DoctorSeverity Severity, string Message, string? SuggestedAction);

public static class IniHealth
{
    /// <summary>Keys whose shipped default is safe, so a different value is always a
    /// deliberate edit. Warn; never silently revert.</summary>
    private static readonly (string Key, string BadValue, string Message)[] Flagged =
    [
        ("AmdGraphicsWaitExperimental", "true",
            "AmdGraphicsWaitExperimental=true enables an experimental wait path that is not wired " +
            "up in this build. Expect stalls or a black screen. The shipped default is 'auto'."),
        ("AmdInterleaveModelHistory", "true",
            "AmdInterleaveModelHistory=true is an experimental setting known to cause artefacts. " +
            "The shipped default is 'auto'."),
        ("LogToFile", "false",
            "LogToFile=false means no logs are written, so nobody can diagnose a problem you " +
            "report. The shipped default is 'true'."),
    ];

    public static IReadOnlyList<DoctorFinding> Check(string iniPath)
    {
        if (!File.Exists(iniPath)) return [];

        var findings = new List<DoctorFinding>();

        // ReadLines is lazy, so the open happens inside the loop: the guard has to wrap the
        // whole enumeration, not just the call. This runs on every scan of an installed game,
        // off a command with no catch above it — the game holding its own ini open, or an AV
        // scanner touching it, is ordinary, and it must produce a finding rather than a crash.
        try
        {
            foreach (var rawLine in File.ReadLines(iniPath))
            {
                var line = rawLine.Trim();
                if (line.Length == 0 || line[0] is ';' or '#') continue;

                var separator = line.IndexOf('=');
                if (separator < 0) continue;

                var key = line[..separator].Trim();
                var value = line[(separator + 1)..].Trim();

                foreach (var (flaggedKey, badValue, message) in Flagged)
                {
                    if (string.Equals(key, flaggedKey, StringComparison.OrdinalIgnoreCase) &&
                        string.Equals(value, badValue, StringComparison.OrdinalIgnoreCase))
                    {
                        findings.Add(new DoctorFinding(
                            $"ini.{flaggedKey}", DoctorSeverity.Warning, message, "Reset ini to defaults"));
                    }
                }
            }
        }
        // UnauthorizedAccessException is not an IOException; both have to be named.
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            // Partial findings would be worse than none: the reader stopped at an arbitrary
            // line, so "no problems below here" is a claim we cannot make.
            return
            [
                new DoctorFinding("ini.unreadable", DoctorSeverity.Warning,
                    $"{Path.GetFileName(iniPath)} could not be read, so its settings could not be " +
                    "checked. If the game is running, close it and check again.",
                    "Check again"),
            ];
        }

        return findings;
    }

    /// <summary>Where a build that selects lmxxf finds its models, relative to the game folder:
    /// TheAutomatic's layout, lmxxf's own release, and the pak AMDNR ships beside
    /// LmxxfNrRuntime.dll. Any one of them makes lmxxf a working choice.</summary>
    private static readonly string[] LmxxfModelFolders =
        ["native-game-tiled-assets", Path.Combine("DLSS5-AMD", "native-game-tiled-assets")];

    private const string LmxxfPak = "LmxxfNrRuntime.pak";

    /// <summary>An ini that selects lmxxf with none of its models beside the game. TheAutomatic's
    /// release ships exactly that — NrBackend=lmxxf, and a model folder it links to rather than
    /// includes — so a user who installed it by hand, or kept its ini, has a Neural Rendering
    /// that silently does nothing. Their ini is theirs and is never edited: this says what is
    /// wrong and what fixes it. Only asked when the launcher installed the runtime, which is
    /// what RESET INI then switches to.</summary>
    public static IReadOnlyList<DoctorFinding> CheckLmxxfModels(string iniPath)
    {
        string? backend = null;

        // Unreadable is Check's finding to report; saying it twice helps nobody.
        try
        {
            var inSection = false;
            foreach (var rawLine in File.ReadLines(iniPath))
            {
                var line = rawLine.Trim();
                if (line.StartsWith('['))
                {
                    var close = line.IndexOf(']');
                    inSection = close > 0 &&
                                string.Equals(line[1..close].Trim(), "DlssNr", StringComparison.OrdinalIgnoreCase);
                    continue;
                }

                if (!inSection || line.Length == 0 || line[0] is ';' or '#') continue;

                var separator = line.IndexOf('=');
                if (separator > 0 &&
                    string.Equals(line[..separator].Trim(), "NrBackend", StringComparison.OrdinalIgnoreCase))
                {
                    backend = line[(separator + 1)..].Trim();
                }
            }
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return [];
        }

        if (!string.Equals(backend, "lmxxf", StringComparison.OrdinalIgnoreCase)) return [];

        var game = Path.GetDirectoryName(iniPath) ?? "";
        if (LmxxfModelFolders.Any(f => Directory.Exists(Path.Combine(game, f))) ||
            File.Exists(Path.Combine(game, LmxxfPak)))
        {
            return [];
        }

        return
        [
            new DoctorFinding("ini.lmxxfWithoutModels", DoctorSeverity.Warning,
                "This OptiScaler.ini selects the lmxxf backend, but its model files are not installed, " +
                "so Neural Rendering will not run. Use RESET INI to switch to the runtime the launcher installed.",
                "Reset ini to defaults"),
        ];
    }

    /// <summary>Returns the path the previous ini was backed up to, or null when there was no
    /// ini to back up. It never returns a path that does not exist — a caller telling the user
    /// "your settings are saved at X" must not be pointing at nothing.</summary>
    public static string? ResetToDefaults(string iniPath, string packagedIniPath)
    {
        string? backup = null;

        if (File.Exists(iniPath))
        {
            // Second resolution alone would let two resets within one second silently clobber
            // the first backup — destroying the very thing this step exists to preserve.
            var stamped = Path.Combine(
                Path.GetDirectoryName(iniPath)!,
                $"OptiScaler.ini.bak-{DateTime.Now:yyyyMMdd-HHmmss}");

            backup = stamped;
            for (var n = 2; File.Exists(backup); n++) backup = $"{stamped}-{n}";

            File.Copy(iniPath, backup, overwrite: false);
        }

        File.Copy(packagedIniPath, iniPath, overwrite: true);
        return backup;
    }
}
