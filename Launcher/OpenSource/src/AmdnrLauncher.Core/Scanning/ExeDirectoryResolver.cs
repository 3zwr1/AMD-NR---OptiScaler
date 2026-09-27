// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Scanning;

public sealed record ExeResolution(string? ExeDirectory, string? ExeName, IReadOnlyList<string> Ambiguous);

/// <summary>Finds the folder the game's real exe lives in, which is where a proxy dll has to go.
/// A store only reports the install root, and the root is often wrong: Unreal games put a small
/// stub there (SILENT HILL 2's 329 KB SHProto.exe) that merely starts the real exe two levels
/// down, and a proxy beside the stub is never loaded.</summary>
public static class ExeDirectoryResolver
{
    /// <summary>Searched in order; the first sub-path holding a candidate wins.</summary>
    private static readonly string[] SubPaths =
    [
        Path.Combine("Binaries", "Win64"),
        "Win64",
        "bin",
        "game",
    ];

    /// <summary>WinGDK is the Game Pass build of the same project, and Unreal writes it to its
    /// own platform folder, never beside the Win64 one.</summary>
    private static readonly string[] UnrealBinaryFolders =
    [
        Path.Combine("Binaries", "Win64"),
        Path.Combine("Binaries", "WinGDK"),
    ];

    private static readonly string[] ShippingSuffixes = ["-Win64-Shipping.exe", "-WinGDK-Shipping.exe"];

    /// <summary>Unreal's own folder: crash reporter, CEF helper, sometimes a shipping-built
    /// engine tool. Whatever is in it, it is never the game.</summary>
    private const string EngineFolder = "Engine";

    /// <summary>Two root exes within this factor of each other are a real choice (a DX11 and a
    /// DX12 build can differ several times over); beyond it, the small one is a helper. The
    /// real roots measured: Spider-Man's game is 85x its largest helper.</summary>
    private const long DecisiveSizeRatio = 10;

    private const int PickListLimit = 50;

    /// <summary>Compared against the name lower-cased with every non-alphanumeric removed, so a
    /// fragment has to be written that way too — and then "VC_redist.x64" and "crs-handler"
    /// match without a spelling per vendor.</summary>
    private static readonly string[] ExcludedFragments =
    [
        "crashreport", "crashhandler", "crashpad", "crshandler", "crsvideo", "crashlogs",
        "vcredist", "dxsetup", "oalinst", "directx",
        "unins", "setup", "installer",
        "webhelper", "epicwebhelper", "launcher", "gamelaunchhelper", "bootstrapper",
        // Modding tools that live beside the game: RAGE Plugin Hook sat at 12 MB next to a
        // 47 MB GTA5.exe, close enough to leave the owner's own GTA V folder unresolved.
        "pluginhook", "scripthook",
    ];

    private static string Normalise(string name)
        => new(name.Where(char.IsLetterOrDigit).Select(char.ToLowerInvariant).ToArray());

    private static bool IsCandidate(string path)
    {
        var name = Normalise(Path.GetFileNameWithoutExtension(path));
        return !ExcludedFragments.Any(f => name.Contains(f, StringComparison.Ordinal));
    }

    /// <summary>IgnoreInaccessible skips a folder the user cannot read instead of throwing.
    /// Game directories routinely contain one.</summary>
    private static EnumerationOptions Options(bool recurse) => new()
    {
        IgnoreInaccessible = true,
        RecurseSubdirectories = recurse,
    };

    private static List<string> CandidatesIn(string directory, bool recurse = false, int limit = int.MaxValue)
    {
        if (!Directory.Exists(directory)) return [];
        try
        {
            return Directory.EnumerateFiles(directory, "*.exe", Options(recurse))
                .Where(IsCandidate)
                .Take(limit)
                .ToList();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }

    /// <summary>First-level folders that may hold an Unreal project. The project folder is not
    /// always named after the exe (Stray's is "Hk_project"), so every one is looked in.</summary>
    private static List<string> ProjectFolders(string installRoot)
    {
        try
        {
            return Directory.EnumerateDirectories(installRoot, "*", Options(recurse: false))
                .Where(d => !string.Equals(Path.GetFileName(d), EngineFolder, StringComparison.OrdinalIgnoreCase))
                .ToList();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }

    private static string? ShippingPrefix(string path)
    {
        var name = Path.GetFileName(path);
        foreach (var suffix in ShippingSuffixes)
        {
            if (name.EndsWith(suffix, StringComparison.OrdinalIgnoreCase)) return name[..^suffix.Length];
        }
        return null;
    }

    private static ExeResolution At(string exePath)
        => new(Path.GetDirectoryName(exePath), Path.GetFileName(exePath), []);

    private static ExeResolution? ResolveUnreal(string installRoot, List<string> atRoot)
    {
        var shipping = ProjectFolders(installRoot)
            .Prepend(installRoot)
            .SelectMany(project => UnrealBinaryFolders.Select(b => Path.Combine(project, b)))
            .SelectMany(folder => CandidatesIn(folder))
            .Where(p => ShippingPrefix(p) is not null)
            .ToList();
        if (shipping.Count == 0) return null;

        // The root stub is named after the project, so when a game bundles a second shipping exe
        // (a benchmark, a bonus title) the stub says which one is the game. Size decides only
        // when nothing at the root speaks for either.
        var rootNames = atRoot.Select(Path.GetFileNameWithoutExtension).ToList();
        var named = shipping
            .Where(p => rootNames.Contains(ShippingPrefix(p), StringComparer.OrdinalIgnoreCase))
            .ToList();

        return At((named.Count > 0 ? named : shipping).OrderByDescending(FileFacts.TryLength).First());
    }

    /// <summary>A file we cannot measure reads as 0 bytes, and it may be the real game mid-update:
    /// then neither size is known well enough to call it, so the user is asked instead.</summary>
    private static string? DecisivelyLargest(List<string> candidates)
    {
        var bySize = candidates
            .Select(p => (Path: p, Length: FileFacts.TryLength(p)))
            .OrderByDescending(c => c.Length)
            .ToList();
        var (largest, second) = (bySize[0], bySize[1]);

        return second.Length > 0 && largest.Length >= DecisiveSizeRatio * second.Length
            ? largest.Path
            : null;
    }

    private static ExeResolution Pick(string directory, List<string> candidates)
    {
        // The length read sits outside the guarded enumeration above, so a file that vanished
        // between the two — a game mid-update, an AV scanner quarantining a shipping exe —
        // would throw here and take the whole scan down. A file we cannot measure simply
        // sorts last; it is still a candidate, just not the one we would pick blind.
        var chosen = candidates
            .OrderByDescending(FileFacts.TryLength)
            .First();
        return new ExeResolution(directory, Path.GetFileName(chosen), []);
    }

    /// <summary>The exe a store names, when it is usable: inside the install root, present, and
    /// not a launcher, crash handler or installer by the same rules the folder walk uses. Call
    /// of Duty's Xbox config names bootstrapper.exe, which only starts the real game — null
    /// then, and the caller falls back to <see cref="Resolve"/>.</summary>
    public static ExeResolution? FromHint(string installRoot, string? hint)
    {
        if (string.IsNullOrWhiteSpace(hint) || Path.IsPathRooted(hint)) return null;

        string root, path;
        try
        {
            root = Path.TrimEndingDirectorySeparator(Path.GetFullPath(installRoot));
            path = Path.GetFullPath(Path.Combine(root, hint));
        }
        catch (Exception e) when (e is ArgumentException or NotSupportedException or PathTooLongException)
        {
            return null;
        }

        // The hint comes from a file in a folder anyone can write to, so it is held to the
        // folder it describes.
        if (!path.StartsWith(root + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)) return null;

        return path.EndsWith(".exe", StringComparison.OrdinalIgnoreCase) && File.Exists(path) && IsCandidate(path)
            ? At(path)
            : null;
    }

    public static ExeResolution Resolve(string installRoot)
    {
        if (!Directory.Exists(installRoot)) return new ExeResolution(null, null, []);

        var atRoot = CandidatesIn(installRoot);

        // Unreal first: its stub sits at the root and would otherwise win as the only root exe.
        if (ResolveUnreal(installRoot, atRoot) is { } unreal) return unreal;

        if (atRoot.Count == 1) return Pick(installRoot, atRoot);
        if (atRoot.Count > 1 && DecisivelyLargest(atRoot) is { } largest)
            return new ExeResolution(installRoot, Path.GetFileName(largest), []);

        foreach (var sub in SubPaths)
        {
            var directory = Path.Combine(installRoot, sub);
            var candidates = CandidatesIn(directory);
            if (candidates.Count > 0) return Pick(directory, candidates);
        }

        // Nothing conclusive: hand every candidate back for the user to choose. Engine is left
        // out of the walk, not filtered after it: it is never the answer, and in an Unreal game
        // it is the biggest tree to crawl.
        var all = atRoot.Take(PickListLimit).ToList();
        foreach (var folder in ProjectFolders(installRoot))
        {
            if (all.Count >= PickListLimit) break;
            all.AddRange(CandidatesIn(folder, recurse: true, limit: PickListLimit - all.Count));
        }

        return new ExeResolution(null, null, all);
    }
}
