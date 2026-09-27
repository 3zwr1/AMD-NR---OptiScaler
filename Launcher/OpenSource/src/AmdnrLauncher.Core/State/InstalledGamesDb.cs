// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text.Json;

namespace AmdnrLauncher.Core.State;

/// <summary>Remembers folders the user added by hand, so a rescan does not lose them.</summary>
public static class InstalledGamesDb
{
    public static string FilePath { get; set; } = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDNR", "games.json");

    /// <summary>Runs at the top of every Scan(). This file lives in %LOCALAPPDATA%, which on a
    /// real machine is OneDrive-synced and AV-swept, so it can be momentarily locked or
    /// permission-denied — and UnauthorizedAccessException is not an IOException, so both have
    /// to be named. Losing the manual list for one scan is survivable; taking the launcher down
    /// with it is not.</summary>
    public static IReadOnlyList<string> Load()
    {
        if (!File.Exists(FilePath)) return [];
        try
        {
            return JsonSerializer.Deserialize<List<string>>(File.ReadAllText(FilePath)) ?? [];
        }
        catch (Exception e) when (e is JsonException or IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }

    /// <summary>Best effort, and deliberately so: the callers are click handlers with no catch
    /// above them. A folder that could not be persisted is still returned to the caller and
    /// still shows in the list for this session — it simply will not survive a restart.</summary>
    private static void Save(IEnumerable<string> paths)
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(paths.ToList()));
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    public static void AddManual(string exeDirectory)
    {
        var all = Load().ToList();
        if (!all.Contains(exeDirectory, StringComparer.OrdinalIgnoreCase))
        {
            all.Add(exeDirectory);
            Save(all);
        }
    }

    public static void RemoveManual(string exeDirectory)
        => Save(Load().Where(p => !string.Equals(p, exeDirectory, StringComparison.OrdinalIgnoreCase)));
}
