// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Install;

/// <summary>Capcom's RE Engine games — Resident Evil 2, 3, 4, 7, Village and Requiem, Devil May Cry 5,
/// Monster Hunter, Street Fighter 6, Dragon's Dogma 2 — close themselves 15 to 60 seconds after
/// launch when a mod is loaded, unless REFramework is installed: it neutralises that check. RE
/// Requiem did exactly that with AMDNR until REFramework's dinput8.dll went in beside re9.exe, and
/// a player's RE3 did it twice in one evening. The engine is recognised by the archive every one
/// of its games ships, re_chunk_000.pak, not by a list of exe names that ages.</summary>
public static class ReEngine
{
    public const string ReframeworkReleases = "https://github.com/praydog/REFramework-nightly/releases";

    /// <summary>The line the stage and the Doctor show when REFramework is missing.</summary>
    public const string MissingReframework =
        "This Capcom game (RE Engine) closes itself 15–60 seconds after launch when mods are loaded, " +
        "unless REFramework is installed. Install the latest REFramework nightly (one zip covers RE2, RE3, " +
        "RE4, RE7, Village and Requiem) and put its dinput8.dll next to the game's exe: " + ReframeworkReleases;

    /// <summary>REFramework's dinput8.dll carries no version resource, so it is known by size —
    /// 23 MB on 2026-09-26, where a real dinput8 proxy from anyone else is a few hundred KB —
    /// or by what it writes beside itself on the first run.</summary>
    private const long ReframeworkMinimumBytes = 4L * 1024 * 1024;

    public static bool IsReEngineGame(string exeDirectory)
        => Exists(Path.Combine(exeDirectory, "re_chunk_000.pak"));

    /// <summary>dinput8.dll beside the exe, and either REFramework's size or its own folder or
    /// logs (reframework\, re2_framework_log.txt, reframework_accessed_files.txt …) — a first-run
    /// copy has only the DLL, a used one has all of them.</summary>
    public static bool HasReframework(string exeDirectory)
    {
        var dinput8 = Path.Combine(exeDirectory, "dinput8.dll");
        if (!Exists(dinput8)) return false;

        try
        {
            if (new FileInfo(dinput8).Length >= ReframeworkMinimumBytes) return true;
            if (Directory.Exists(Path.Combine(exeDirectory, "reframework"))) return true;
            return Directory.EnumerateFiles(exeDirectory, "*framework*", SearchOption.TopDirectoryOnly)
                .Any(f => Path.GetFileName(f).Contains("framework_log", StringComparison.OrdinalIgnoreCase)
                          || Path.GetFileName(f).StartsWith("reframework", StringComparison.OrdinalIgnoreCase));
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    /// <summary>The warning for this folder, or null when it is not an RE Engine game or
    /// REFramework is already there.</summary>
    public static string? MissingReframeworkWarning(string exeDirectory)
        => IsReEngineGame(exeDirectory) && !HasReframework(exeDirectory) ? MissingReframework : null;

    private static bool Exists(string path)
    {
        try { return File.Exists(path); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { return false; }
    }
}
