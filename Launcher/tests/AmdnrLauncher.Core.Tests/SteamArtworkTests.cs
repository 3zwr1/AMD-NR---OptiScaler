// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class SteamArtworkTests
{
    [Fact]
    public void Cached_art_is_found_in_either_layout()
    {
        using var steam = new TempDir();
        var perApp = steam.Write(Path.Combine("appcache", "librarycache", "2206210", "library_600x900.jpg"), "art");
        var flat = steam.Write(Path.Combine("appcache", "librarycache", "1030840_library_hero.jpg"), "art");

        Assert.Equal(perApp, SteamArtwork.Portrait(steam.Path, "2206210"));
        Assert.Equal(flat, SteamArtwork.Hero(steam.Path, "1030840"));
        Assert.Null(SteamArtwork.Logo(steam.Path, "2206210"));
    }

    [Fact]
    public void An_id_that_is_not_a_Steam_appid_finds_nothing()
    {
        // An Xbox config's Identity Name, from a folder any local user can write to, is joined
        // onto the art cache like an appid. A rooted one replaces the cache path altogether, and
        // the launcher — often elevated — then probes, and WPF loads, whatever it names.
        using var steam = new TempDir();
        using var elsewhere = new TempDir();
        elsewhere.Write("library_600x900.jpg", "not Steam's");
        steam.Write(Path.Combine("appcache", "librarycache", "Microsoft.ForteBaseGame", "library_600x900.jpg"), "art");

        Assert.Null(SteamArtwork.Portrait(steam.Path, elsewhere.Path));
        Assert.Null(SteamArtwork.Portrait(steam.Path, @"..\..\x"));
        Assert.Null(SteamArtwork.Portrait(steam.Path, "Microsoft.ForteBaseGame"));
    }
}
