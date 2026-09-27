// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Install;

public enum ConflictKind { InjectionDll, StaleArtifact }

public sealed record Conflict(string Path, ConflictKind Kind, string Reason, bool RecommendMoveAside);

public static class ConflictScanner
{
    private static readonly string[] StaleFileNames =
    [
        "dlss5-neural.ini",
        "dlss5-neural.install.json",
    ];

    /// <param name="forwarderSha256">The forwarder about to be placed, or null when the build
    /// being installed has none. With none there is nothing for a forwarder already in the
    /// folder to be stale against, and it is not in the way of anything being placed.</param>
    public static IReadOnlyList<Conflict> Scan(
        string exeDirectory, string chosenProxy, string? forwarderSha256)
    {
        var conflicts = new List<Conflict>();
        if (!Directory.Exists(exeDirectory)) return conflicts;

        foreach (var proxy in PayloadNames.ProxyNames)
        {
            if (string.Equals(proxy, chosenProxy, StringComparison.OrdinalIgnoreCase)) continue;

            var path = Path.Combine(exeDirectory, proxy);
            if (!File.Exists(path)) continue;

            conflicts.Add(new Conflict(
                path,
                ConflictKind.InjectionDll,
                $"{proxy} already exists. Leaving it in place can load two mods at once.",
                RecommendMoveAside: true));
        }

        foreach (var name in StaleFileNames)
        {
            var path = Path.Combine(exeDirectory, name);
            if (File.Exists(path))
            {
                conflicts.Add(new Conflict(
                    path, ConflictKind.StaleArtifact,
                    $"{name} is left over from an earlier mod generation and is no longer used.",
                    RecommendMoveAside: true));
            }
        }

        // .asi plugins are not injection DLLs, so a proxy-name sweep would miss this one.
        var asi = Path.Combine(exeDirectory, "OptiScaler", "plugins", "XeFGUnlock.asi");
        if (File.Exists(asi))
        {
            conflicts.Add(new Conflict(
                asi, ConflictKind.StaleArtifact,
                "XeFGUnlock.asi is superseded — the unlock is built in now, and OptiScaler.ini " +
                "states the built-in setting replaces this plugin.",
                RecommendMoveAside: true));
        }

        var forwarder = Path.Combine(exeDirectory, PayloadNames.Forwarder);
        if (forwarderSha256 is not null && File.Exists(forwarder) && !ForwarderMatches(forwarder, forwarderSha256))
        {
            conflicts.Add(new Conflict(
                forwarder, ConflictKind.StaleArtifact,
                $"{PayloadNames.Forwarder} is an outdated build and will be replaced.",
                RecommendMoveAside: true));
        }

        return conflicts;
    }

    /// <summary>Fails closed. A forwarder we cannot read is one we cannot verify, so it is
    /// treated as not matching and gets replaced. Letting the exception escape would abort
    /// the entire pre-install scan over a single locked file.</summary>
    private static bool ForwarderMatches(string path, string expectedSha256)
    {
        try
        {
            return Hashing.Matches(Hashing.Sha256OfFile(path), expectedSha256);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }
}
