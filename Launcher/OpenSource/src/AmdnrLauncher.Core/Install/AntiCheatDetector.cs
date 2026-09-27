// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Install;

public static class AntiCheatDetector
{
    private static readonly (string Product, string[] Markers)[] Known =
    [
        ("EasyAntiCheat",      ["easyanticheat", "eac_launcher"]),
        ("BattlEye",           ["battleye", "beservice", "belauncher"]),
        ("Vanguard",           ["vanguard", "vgk.sys", "vgc.exe"]),
        (Ricochet,             ["randgrid.sys", "ricochet"]),
        ("EA Javelin",         ["eaanticheat"]),
        ("nProtect GameGuard", ["gameguard"]),
        ("XIGNCODE",           ["xigncode"]),
        ("EQU8",               ["equ8"]),
        ("mhyprot",            ["mhyprot"]),
        ("PunkBuster",         ["pnkbstr"]),
    ];

    /// <summary>Call of Duty's kernel anti-cheat. Randgrid.sys is its driver, and it sits beside
    /// the game's exe in the Xbox app's Call of Duty folders.</summary>
    public const string Ricochet = "Ricochet";

    /// <summary>Whether no consent is enough. Ricochet bans for injected DLLs and the games it
    /// guards have no offline mode where a mod would be safe, so a Yes at the prompt could only
    /// ever mean a lost account.</summary>
    public static bool BlocksInstall(string product)
        => string.Equals(product, Ricochet, StringComparison.Ordinal);

    /// <summary>How far below the install root the directory sweep goes. Spec §7 step 1 says
    /// the target *tree*, and the runtime usually sits at the root — &lt;root&gt;\EasyAntiCheat\
    /// while the exe is away in &lt;root&gt;\Binaries\Win64\ — so scanning the exe directory
    /// alone misses the commonest layout entirely. It stays bounded because this runs for every
    /// candidate on every scan, and an install root is one bad scanner result away from being a
    /// whole Steam library.</summary>
    private const int RootRecursionDepth = 2;

    private static readonly EnumerationOptions Bounded = new()
    {
        IgnoreInaccessible = true,
        RecurseSubdirectories = true,
        MaxRecursionDepth = RootRecursionDepth,
    };

    private static readonly EnumerationOptions Shallow = new() { IgnoreInaccessible = true };

    /// <summary>The kernel drivers of the anti-cheats that block outright, which are also looked
    /// for in the folders above the exe. A block has to hold whichever folder the user picks:
    /// "Add game folder…" on Call of Duty's Content\sp23 makes that folder the exe directory and
    /// the install root at once, while Randgrid.sys sits one level up, in Content. Exact file
    /// names only, and only these: a parent such as steamapps\common lists every other game,
    /// and a sibling called "…Vanguard" is not this game's anti-cheat.</summary>
    private static readonly string[] BlockingDrivers = ["randgrid.sys"];

    /// <summary>How many folders above the exe directory are checked for a blocking driver. The
    /// campaign exe is one below Content; four covers a pick a couple of folders deeper than
    /// that, and reaches no further than the library a game sits in.</summary>
    private const int AncestorDepth = 4;

    /// <summary>The anti-cheat products protecting this game, if any. This is the one gate
    /// between a one-click installer and a banned account, so it looks at the exe directory's
    /// own contents and at the tree around the install root — not only where the exe happens
    /// to live.</summary>
    public static IReadOnlyList<string> Detect(string exeDirectory, string? installRoot = null)
    {
        var names = new List<string>();

        // Loose binaries next to the exe: EasyAntiCheat_EOS.dll, vgk.sys.
        names.AddRange(Names(exeDirectory, Shallow));

        // Directories around the root, where the runtime itself normally lives.
        if (installRoot is not null &&
            !string.Equals(
                Path.TrimEndingDirectorySeparator(installRoot),
                Path.TrimEndingDirectorySeparator(exeDirectory),
                StringComparison.OrdinalIgnoreCase))
        {
            names.AddRange(Names(installRoot, Shallow));
        }

        if (installRoot is not null) names.AddRange(Directories(installRoot));

        // Folders above the exe, for the drivers no consent can wave through.
        names.AddRange(BlockingDriversAbove(exeDirectory));

        return Known
            .Where(k => k.Markers.Any(m => names.Any(n => n.Contains(m, StringComparison.Ordinal))))
            .Select(k => k.Product)
            .ToList();
    }

    /// <summary>Lowercased entry names, or nothing at all. Detect runs on the scan path, which
    /// reaches the UI through a command with no catch — IgnoreInaccessible skips a subdirectory
    /// we cannot read, but the enumeration still throws when the directory handed in is itself
    /// unreadable, and UnauthorizedAccessException is not an IOException.</summary>
    private static List<string> Names(string directory, EnumerationOptions options)
    {
        if (!Directory.Exists(directory)) return [];
        try
        {
            return Directory.EnumerateFileSystemEntries(directory, "*", options)
                .Select(p => Path.GetFileName(p).ToLowerInvariant())
                .ToList();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }

    /// <summary>File.Exists answers false rather than throwing for a path it cannot read, so an
    /// unreadable parent is simply not a hit.</summary>
    private static List<string> BlockingDriversAbove(string exeDirectory)
    {
        var found = new List<string>();
        var directory = Path.GetDirectoryName(Path.TrimEndingDirectorySeparator(exeDirectory));

        for (var level = 0; level < AncestorDepth && !string.IsNullOrEmpty(directory); level++)
        {
            foreach (var driver in BlockingDrivers)
            {
                if (File.Exists(Path.Combine(directory, driver))) found.Add(driver);
            }

            directory = Path.GetDirectoryName(directory);
        }

        return found;
    }

    private static List<string> Directories(string root)
    {
        if (!Directory.Exists(root)) return [];
        try
        {
            return Directory.EnumerateDirectories(root, "*", Bounded)
                .Select(p => Path.GetFileName(p).ToLowerInvariant())
                .ToList();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }
}
