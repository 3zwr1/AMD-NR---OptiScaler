// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Scanning;

/// <summary>
/// Finds the artwork Steam has already downloaded for a game, so the launcher can show a
/// library that looks like the user's library instead of a list of names.
///
/// Nothing here is fetched: these are files the Steam client caches on disk. If a game has
/// no cached art the caller gets null and falls back to its own placeholder — which is the
/// common case for a game the user has never opened in the library view, and for games
/// whose art newer Steam clients store under content-hash filenames this code cannot
/// identify. A missing picture is not a failure.
/// </summary>
public static class SteamArtwork
{
    /// <summary>Tall box art, for a list row.</summary>
    public static string? Portrait(string? steamRoot, string? appId)
        => Find(steamRoot, appId, "library_600x900.jpg", "header.jpg");

    /// <summary>The wide banner, for the detail panel's header.</summary>
    public static string? Hero(string? steamRoot, string? appId)
        => Find(steamRoot, appId, "library_hero.jpg", "header.jpg");

    /// <summary>The game's transparent wordmark, drawn over the hero.</summary>
    public static string? Logo(string? steamRoot, string? appId)
        => Find(steamRoot, appId, "logo.png");

    /// <summary>The blurred hero, for filling the window behind everything. Steam ships this
    /// already blurred at about 2 KB, so the launcher never runs a blur over a full-window
    /// bitmap on the UI thread — it just draws a small image large.</summary>
    public static string? HeroBlur(string? steamRoot, string? appId)
        => Find(steamRoot, appId, "library_hero_blur.jpg");

    /// <summary>The first of <paramref name="names"/> cached for the appid, in either layout, for
    /// <see cref="Artwork"/> to rank against the other places a picture can come from.</summary>
    internal static string? Find(string? steamRoot, string? appId, params string[] names)
    {
        if (string.IsNullOrEmpty(steamRoot) || string.IsNullOrEmpty(appId)) return null;

        // A Steam appid is a number, and nothing else may be joined onto the cache path: another
        // store's id comes from a config file anyone can write, and a rooted one would replace
        // the cache path outright.
        if (!appId.All(char.IsAsciiDigit)) return null;

        var cache = Path.Combine(steamRoot, "appcache", "librarycache");

        foreach (var name in names)
        {
            // Two layouts, because Steam changed it: current clients use a directory per
            // app, older ones prefixed the filename with the appid in one flat folder.
            // Both still exist on machines that have been upgraded rather than reinstalled.
            var candidates = new[]
            {
                Path.Combine(cache, appId, name),
                Path.Combine(cache, $"{appId}_{name}"),
            };

            foreach (var path in candidates)
            {
                // Exists() is the whole check on purpose. Validating that the file decodes
                // would mean loading every image during a scan, and a corrupt one is
                // already handled where it is drawn.
                try
                {
                    if (File.Exists(path)) return path;
                }
                catch (Exception e) when (e is IOException or UnauthorizedAccessException)
                {
                    // An unreadable art cache is not a reason to lose the game.
                }
            }
        }

        return null;
    }
}
