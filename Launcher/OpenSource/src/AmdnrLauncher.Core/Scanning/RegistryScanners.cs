// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Runtime.Versioning;
using System.Security;
using Microsoft.Win32;

namespace AmdnrLauncher.Core.Scanning;

/// <summary>The two roots the store scanners read under.</summary>
public enum RegistryRoot { LocalMachine, CurrentUser }

/// <summary>The registry as the store scanners read it: names of a key's subkeys, and one
/// string value. Both answer "nothing" for a key that is not there or cannot be opened — a
/// launcher that is not installed is the ordinary case, not an error — and a test hands in a
/// dictionary instead of the machine's registry.</summary>
public interface IRegistryReader
{
    IReadOnlyList<string> SubKeys(RegistryRoot root, string path);

    string? String(RegistryRoot root, string path, string name);
}

/// <summary>The machine's registry, through the default view of a 64-bit process: 32-bit
/// launchers' keys are read under SOFTWARE\WOW6432Node, written out in each scanner's paths
/// rather than left to the redirector, so a path reads the same in a test. The only place in
/// Core that touches Microsoft.Win32, and only on Windows.</summary>
public sealed class WindowsRegistryReader : IRegistryReader
{
    public IReadOnlyList<string> SubKeys(RegistryRoot root, string path)
    {
        if (!OperatingSystem.IsWindows()) return [];

        try
        {
            using var hive = RegistryKey.OpenBaseKey(HiveOf(root), RegistryView.Default);
            using var key = hive.OpenSubKey(path);
            return key?.GetSubKeyNames() ?? [];
        }
        catch (Exception e) when (e is SecurityException or IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }

    public string? String(RegistryRoot root, string path, string name)
    {
        if (!OperatingSystem.IsWindows()) return null;

        try
        {
            using var hive = RegistryKey.OpenBaseKey(HiveOf(root), RegistryView.Default);
            using var key = hive.OpenSubKey(path);
            return key?.GetValue(name) as string;
        }
        catch (Exception e) when (e is SecurityException or IOException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    [SupportedOSPlatform("windows")]
    private static RegistryHive HiveOf(RegistryRoot root)
        => root == RegistryRoot.CurrentUser ? RegistryHive.CurrentUser : RegistryHive.LocalMachine;
}

/// <summary>What the store scanners share: Windows' own list of installed programs, which
/// every launcher but Steam, Epic and the Xbox app writes its games into, and the tidying a
/// registry path needs before it is a folder.</summary>
public static class RegistryGames
{
    /// <summary>One entry of Add or remove programs.</summary>
    public sealed record UninstallEntry(string Key, string? DisplayName, string? Publisher, string? InstallLocation);

    private static readonly (RegistryRoot Root, string Path)[] UninstallLists =
    [
        (RegistryRoot.LocalMachine, @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"),
        (RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall"),
        (RegistryRoot.CurrentUser, @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"),
    ];

    /// <summary>Every entry of Add or remove programs, from the 64-bit and 32-bit lists of the
    /// machine and the current user's own. The same program can appear in more than one; the
    /// scan drops duplicates by folder afterwards.</summary>
    public static IEnumerable<UninstallEntry> UninstallEntries(IRegistryReader registry)
    {
        foreach (var (root, list) in UninstallLists)
        {
            foreach (var key in registry.SubKeys(root, list))
            {
                var path = list + @"\" + key;
                yield return new UninstallEntry(
                    key,
                    registry.String(root, path, "DisplayName"),
                    registry.String(root, path, "Publisher"),
                    registry.String(root, path, "InstallLocation"));
            }
        }
    }

    /// <summary>A folder as a launcher wrote it — forward slashes, a trailing slash, quotes —
    /// as a path, or null when it is empty or the folder is not there: a game uninstalled by
    /// hand leaves its key behind.</summary>
    public static string? ExistingFolder(string? written)
    {
        if (string.IsNullOrWhiteSpace(written)) return null;

        var path = written.Trim().Trim('"').Replace('/', '\\').TrimEnd('\\');
        if (path.Length == 0) return null;

        try
        {
            return Directory.Exists(path) ? Path.GetFullPath(path) : null;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException
                                    or NotSupportedException or SecurityException)
        {
            return null;
        }
    }

    /// <summary>The name a folder gives a game when the launcher wrote none.</summary>
    public static string NameFromFolder(string folder) => new DirectoryInfo(folder).Name;
}

/// <summary>Ubisoft Connect: one key per installed game under Ubisoft\Launcher\Installs, named
/// by Ubisoft's numeric id, with the folder as InstallDir. The name is not there; the
/// matching entry of Add or remove programs, "Uplay Install <id>", has it, and failing that
/// the folder's own name is the game's — Ubisoft names its folders after the game.</summary>
public sealed class UbisoftScanner(IRegistryReader registry) : IGameScanner
{
    private const string Installs = @"SOFTWARE\WOW6432Node\Ubisoft\Launcher\Installs";

    public IEnumerable<GameCandidate> Scan()
    {
        var names = RegistryGames.UninstallEntries(registry)
            .Where(e => e.Key.StartsWith("Uplay Install ", StringComparison.OrdinalIgnoreCase))
            .GroupBy(e => e.Key["Uplay Install ".Length..], StringComparer.OrdinalIgnoreCase)
            .ToDictionary(g => g.Key, g => g.First().DisplayName, StringComparer.OrdinalIgnoreCase);

        foreach (var id in registry.SubKeys(RegistryRoot.LocalMachine, Installs))
        {
            var folder = RegistryGames.ExistingFolder(
                registry.String(RegistryRoot.LocalMachine, Installs + @"\" + id, "InstallDir"));
            if (folder is null) continue;

            var name = names.TryGetValue(id, out var display) && !string.IsNullOrWhiteSpace(display)
                ? display.Trim()
                : RegistryGames.NameFromFolder(folder);

            yield return new GameCandidate(name, GameStore.Ubisoft, folder, StoreId: id);
        }
    }
}

/// <summary>GOG Galaxy: one key per installed product under GOG.com\Games, with the game's name,
/// its folder and the exe it starts — the tidiest list any launcher keeps. A product that
/// dependsOn another is a DLC or a language pack, installed into the game's own folder, and is
/// not a game of its own.</summary>
public sealed class GogScanner(IRegistryReader registry) : IGameScanner
{
    private const string Games = @"SOFTWARE\WOW6432Node\GOG.com\Games";

    public IEnumerable<GameCandidate> Scan()
    {
        foreach (var id in registry.SubKeys(RegistryRoot.LocalMachine, Games))
        {
            var key = Games + @"\" + id;
            if (!string.IsNullOrWhiteSpace(registry.String(RegistryRoot.LocalMachine, key, "dependsOn"))) continue;

            var folder = RegistryGames.ExistingFolder(registry.String(RegistryRoot.LocalMachine, key, "path"));
            if (folder is null) continue;

            var name = registry.String(RegistryRoot.LocalMachine, key, "gameName");
            if (string.IsNullOrWhiteSpace(name)) name = RegistryGames.NameFromFolder(folder);

            yield return new GameCandidate(name.Trim(), GameStore.GOG, folder, StoreId: id,
                ExeHint: ExeHint(registry.String(RegistryRoot.LocalMachine, key, "exe"), folder));
        }
    }

    /// <summary>GOG writes the exe as a full path; the hint is the part inside the game folder,
    /// or nothing when it points elsewhere.</summary>
    private static string? ExeHint(string? exe, string folder)
    {
        if (string.IsNullOrWhiteSpace(exe)) return null;

        var full = exe.Trim().Trim('"').Replace('/', '\\');
        var prefix = folder.TrimEnd('\\') + @"\";
        return full.StartsWith(prefix, StringComparison.OrdinalIgnoreCase) && full.Length > prefix.Length
            ? full[prefix.Length..]
            : null;
    }
}

/// <summary>The Rockstar Games Launcher: one key per game under Rockstar Games, with the folder
/// as InstallFolder; the launcher and the Social Club keep keys there too and are skipped. A
/// game the launcher took over from a disc or another store — GTA V Legacy is the common one —
/// is in Add or remove programs under Rockstar's name instead, and is read from there.</summary>
public sealed class RockstarScanner(IRegistryReader registry) : IGameScanner
{
    private const string Root = @"SOFTWARE\WOW6432Node\Rockstar Games";

    private static readonly string[] NotGames = ["Launcher", "Rockstar Games Social Club", "Rockstar Games Launcher"];

    public IEnumerable<GameCandidate> Scan()
    {
        foreach (var key in registry.SubKeys(RegistryRoot.LocalMachine, Root))
        {
            if (NotGames.Contains(key, StringComparer.OrdinalIgnoreCase)) continue;

            var folder = RegistryGames.ExistingFolder(registry.String(RegistryRoot.LocalMachine, Root + @"\" + key, "InstallFolder"));
            if (folder is not null) yield return new GameCandidate(key.Trim(), GameStore.Rockstar, folder);
        }

        foreach (var entry in RegistryGames.UninstallEntries(registry))
        {
            if (entry.Publisher?.Contains("Rockstar", StringComparison.OrdinalIgnoreCase) != true) continue;
            if (string.IsNullOrWhiteSpace(entry.DisplayName)) continue;
            if (entry.DisplayName.Contains("Launcher", StringComparison.OrdinalIgnoreCase)
                || entry.DisplayName.Contains("Social Club", StringComparison.OrdinalIgnoreCase)
                || entry.DisplayName.Contains("SDK", StringComparison.OrdinalIgnoreCase)) continue;

            var folder = RegistryGames.ExistingFolder(entry.InstallLocation);
            if (folder is not null) yield return new GameCandidate(entry.DisplayName.Trim(), GameStore.Rockstar, folder);
        }
    }
}

/// <summary>The EA app (and Origin before it): every game it installs goes into Add or remove
/// programs under Electronic Arts with its folder, and most also write Electronic Arts\&lt;Game&gt;
/// or EA Games\&lt;Game&gt; with an Install Dir. The app itself, its predecessors and its shared
/// parts sit in the same places without a game folder, and are skipped by name.</summary>
public sealed class EaScanner(IRegistryReader registry) : IGameScanner
{
    private static readonly string[] Roots = [@"SOFTWARE\WOW6432Node\Electronic Arts", @"SOFTWARE\WOW6432Node\EA Games"];

    private static readonly string[] NotGames = ["EA Core", "EA Desktop", "EADM", "Origin", "EA app", "EA Games", "EA Shared"];

    public IEnumerable<GameCandidate> Scan()
    {
        foreach (var root in Roots)
        {
            foreach (var key in registry.SubKeys(RegistryRoot.LocalMachine, root))
            {
                if (NotGames.Contains(key, StringComparer.OrdinalIgnoreCase)) continue;

                var folder = RegistryGames.ExistingFolder(registry.String(RegistryRoot.LocalMachine, root + @"\" + key, "Install Dir"));
                if (folder is not null) yield return new GameCandidate(key.Trim(), GameStore.EA, folder);
            }
        }

        foreach (var entry in RegistryGames.UninstallEntries(registry))
        {
            if (entry.Publisher?.Contains("Electronic Arts", StringComparison.OrdinalIgnoreCase) != true) continue;
            if (string.IsNullOrWhiteSpace(entry.DisplayName)) continue;
            if (NotGames.Contains(entry.DisplayName.Trim(), StringComparer.OrdinalIgnoreCase)) continue;

            var folder = RegistryGames.ExistingFolder(entry.InstallLocation);
            if (folder is not null) yield return new GameCandidate(entry.DisplayName.Trim(), GameStore.EA, folder);
        }
    }
}

/// <summary>Battle.net: Blizzard keeps its own list in a binary file, but every game it installs
/// is in Add or remove programs under Blizzard Entertainment with its folder — Activision's
/// titles that ship through it under Activision. The Battle.net app itself is skipped.</summary>
public sealed class BattleNetScanner(IRegistryReader registry) : IGameScanner
{
    public IEnumerable<GameCandidate> Scan()
    {
        foreach (var entry in RegistryGames.UninstallEntries(registry))
        {
            var publisher = entry.Publisher ?? "";
            if (!publisher.Contains("Blizzard", StringComparison.OrdinalIgnoreCase)
                && !publisher.Contains("Activision", StringComparison.OrdinalIgnoreCase)) continue;
            if (string.IsNullOrWhiteSpace(entry.DisplayName)) continue;
            if (entry.DisplayName.Contains("Battle.net", StringComparison.OrdinalIgnoreCase)) continue;

            var folder = RegistryGames.ExistingFolder(entry.InstallLocation);
            if (folder is not null) yield return new GameCandidate(entry.DisplayName.Trim(), GameStore.BattleNet, folder);
        }
    }
}

/// <summary>Amazon Games: each installed game is an entry of the current user's Add or remove
/// programs whose key is "AmazonGames/&lt;title&gt;", with the folder as InstallLocation.</summary>
public sealed class AmazonScanner(IRegistryReader registry) : IGameScanner
{
    public IEnumerable<GameCandidate> Scan()
    {
        foreach (var entry in RegistryGames.UninstallEntries(registry))
        {
            if (!entry.Key.StartsWith("AmazonGames/", StringComparison.OrdinalIgnoreCase)) continue;

            var folder = RegistryGames.ExistingFolder(entry.InstallLocation);
            if (folder is null) continue;

            var name = string.IsNullOrWhiteSpace(entry.DisplayName)
                ? entry.Key["AmazonGames/".Length..]
                : entry.DisplayName;
            yield return new GameCandidate(name.Trim(), GameStore.Amazon, folder);
        }
    }
}
