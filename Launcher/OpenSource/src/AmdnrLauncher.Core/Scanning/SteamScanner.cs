// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using Microsoft.Win32;

namespace AmdnrLauncher.Core.Scanning;

public sealed class SteamScanner(string steamRoot) : IGameScanner
{
    /// <summary>Null when Steam's location cannot be determined. The registry read is guarded
    /// because it is the first thing the launcher does on startup: a policy-locked hive or a
    /// key marked for deletion would otherwise take the app down before it drew a window,
    /// when "no Steam games found" is a perfectly usable outcome.</summary>
    public static string? FindSteamRoot()
    {
        if (!OperatingSystem.IsWindows()) return null;
        try
        {
            using var key = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Valve\Steam");
            return key?.GetValue("SteamPath") as string;
        }
        catch (Exception e) when (e is System.Security.SecurityException
                                    or IOException
                                    or UnauthorizedAccessException)
        {
            return null;
        }
    }

    public IEnumerable<GameCandidate> Scan()
    {
        foreach (var library in LibraryFolders())
        {
            var steamapps = Path.Combine(library, "steamapps");

            // Materialised inside the guard on purpose: EnumerateFiles is lazy, so an
            // access error on this library would otherwise surface mid-iteration and take
            // down the scan of every other library with it.
            List<string> manifests;
            try
            {
                manifests = Directory.Exists(steamapps)
                    ? Directory.EnumerateFiles(steamapps, "appmanifest_*.acf").ToList()
                    : [];
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException)
            {
                continue;
            }

            foreach (var acf in manifests)
            {
                GameCandidate? candidate = null;
                try
                {
                    var state = VdfReader.Parse(File.ReadAllText(acf))["AppState"];
                    var name = state?["name"]?.Value;
                    var installDir = state?["installdir"]?.Value;
                    if (name is null || installDir is null) continue;
                    if (!IsOneFolderName(installDir)) continue;
                    if (IsNotFullyInstalled(state?["StateFlags"]?.Value)) continue;

                    var root = Path.Combine(steamapps, "common", installDir);
                    if (Directory.Exists(root))
                    {
                        // Absent in a hand-edited manifest, and only used to look up cached
                        // artwork, so it is read the same forgiving way as the rest.
                        candidate = new GameCandidate(
                            name, GameStore.Steam, root, state?["appid"]?.Value);
                    }
                }
                // UnauthorizedAccessException does NOT derive from IOException — it is a
                // sibling type — and a permission-denied manifest is the exact "unreadable
                // file" case this guard exists for. It has to be named explicitly.
                catch (Exception e) when (e is IOException
                                            or UnauthorizedAccessException
                                            or FormatException
                                            or IndexOutOfRangeException)
                {
                    // One unreadable manifest must not take down the scan.
                }

                if (candidate is not null) yield return candidate;
            }
        }
    }

    /// <summary>Steam writes installdir as one folder name under steamapps\common, never
    /// anything else. A rooted value, one with a separator or a drive colon, or one Windows
    /// trims to nothing — ".", "..", any run of dots and spaces — names a folder outside the
    /// library or the library itself, and only a hand-edited .acf produces one. A library can
    /// sit anywhere a user can write, so listing it would let that file aim an install, possibly
    /// elevated, at C:\Windows\System32.</summary>
    private static bool IsOneFolderName(string installDir)
        => installDir.TrimEnd('.', ' ').Length > 0
           && !Path.IsPathRooted(installDir)
           && installDir.IndexOfAny([Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar, ':']) < 0;

    /// <summary>Steam sets bit 4 (FullyInstalled) in StateFlags only once a game is installed,
    /// and keeps it through a pending update (6). A game still downloading has its install
    /// folder created, often empty, long before that: listing it would offer an install into a
    /// folder with no game in it. Only a plain number Steam wrote can say so. A missing or
    /// unreadable value — a hand-edited or unusual manifest — keeps the game listed, because
    /// hiding somebody's game on a guess is worse than showing one that is not ready.</summary>
    private static bool IsNotFullyInstalled(string? stateFlags)
    {
        const uint fullyInstalled = 4;

        return uint.TryParse(stateFlags, System.Globalization.NumberStyles.None,
                   System.Globalization.CultureInfo.InvariantCulture, out var flags)
               && (flags & fullyInstalled) == 0;
    }

    private IEnumerable<string> LibraryFolders()
    {
        var vdf = Path.Combine(steamRoot, "steamapps", "libraryfolders.vdf");
        if (!File.Exists(vdf)) yield break;

        VdfNode root;
        try { root = VdfReader.Parse(File.ReadAllText(vdf)); }
        catch (Exception e) when (e is IOException
                                    or UnauthorizedAccessException
                                    or IndexOutOfRangeException) { yield break; }

        var folders = root["libraryfolders"];
        if (folders is null) yield break;

        foreach (var entry in folders.Children.Values)
        {
            var path = entry["path"]?.Value;
            if (!string.IsNullOrWhiteSpace(path)) yield return path;
        }
    }
}
