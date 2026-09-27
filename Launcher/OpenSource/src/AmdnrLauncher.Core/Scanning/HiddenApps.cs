// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Scanning;

/// <summary>Steam installs its own tooling through the same library manifests as games, so a
/// scan lists "Steamworks Common Redistributables" beside real games. None of these can take a
/// mod, and offering an install into one is the kind of row that makes the whole list look
/// untrustworthy.</summary>
public static class HiddenApps
{
    /// <summary>Built in rather than left to the manifest, so the list is right even offline or
    /// with an older cached manifest. The manifest can only add to it.</summary>
    public static IReadOnlyList<string> SteamTools { get; } =
    [
        "228980", // Steamworks Common Redistributables
        "1007",   // Steamworks SDK Redist
        "250820", // SteamVR
    ];

    /// <summary>Only a Steam appid is compared: the lists are Steam's numbering, and an id from
    /// another store that happens to share the digits says nothing about what the game is.</summary>
    public static bool IsHidden(GameCandidate candidate, IReadOnlyList<string>? extraSteamAppIds)
    {
        if (candidate.Store != GameStore.Steam || candidate.StoreId is not { } appId) return false;

        return SteamTools.Contains(appId, StringComparer.Ordinal)
            || (extraSteamAppIds?.Contains(appId, StringComparer.Ordinal) ?? false);
    }
}
