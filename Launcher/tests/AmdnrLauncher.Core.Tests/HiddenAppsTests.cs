// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class HiddenAppsTests
{
    [Theory]
    [InlineData("228980")]
    [InlineData("1007")]
    [InlineData("250820")]
    public void Steam_tools_are_hidden_without_any_manifest_help(string appId)
    {
        var tool = new GameCandidate("Tool", GameStore.Steam, @"C:\x", appId);

        Assert.True(HiddenApps.IsHidden(tool, null));
    }

    [Fact]
    public void A_manifest_listed_appid_is_hidden_and_an_unlisted_one_is_not()
    {
        var listed = new GameCandidate("Bench", GameStore.Steam, @"C:\x", "4242");
        var game = new GameCandidate("Game", GameStore.Steam, @"C:\y", "2206210");

        Assert.True(HiddenApps.IsHidden(listed, ["4242"]));
        Assert.False(HiddenApps.IsHidden(game, ["4242"]));
    }

    [Fact]
    public void A_game_without_a_Steam_appid_is_never_hidden()
    {
        // An Epic catalog id or a hand-added folder can share digits with a Steam appid by
        // accident; the lists only speak about Steam.
        var epic = new GameCandidate("Epic", GameStore.Epic, @"C:\x", "228980");
        var manual = new GameCandidate("Manual", GameStore.Manual, @"C:\y");

        Assert.False(HiddenApps.IsHidden(epic, ["228980"]));
        Assert.False(HiddenApps.IsHidden(manual, ["228980"]));
    }
}
