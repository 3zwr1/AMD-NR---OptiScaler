// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Scanning;

public enum GameStore { Steam, Epic, Manual, Xbox, Ubisoft, EA, GOG, Rockstar, BattleNet, Amazon }

/// <summary>The store's name as a person reads it in the rail: "Battle.net" and "EA app", not
/// the enum's spelling.</summary>
public static class GameStoreLabels
{
    public static string Label(GameStore store) => store switch
    {
        GameStore.Steam => "Steam",
        GameStore.Epic => "Epic",
        GameStore.Manual => "Manual",
        GameStore.Xbox => "Xbox",
        GameStore.Ubisoft => "Ubisoft Connect",
        GameStore.EA => "EA app",
        GameStore.GOG => "GOG",
        GameStore.Rockstar => "Rockstar",
        GameStore.BattleNet => "Battle.net",
        GameStore.Amazon => "Amazon Games",
        _ => store.ToString(),
    };
}

/// <summary>A game as a store reports it. The folder that actually receives the
/// install is resolved separately by <see cref="ExeDirectoryResolver"/>.
///
/// <para><paramref name="StoreId"/> is the store's own identifier — Steam's appid — and is
/// null for a folder the user added by hand. It exists so the launcher can find the
/// artwork the store has already cached; nothing about installing depends on it, which is
/// why it is optional rather than part of a game's identity.</para>
///
/// <para><paramref name="ExeHint"/> is the game's exe as the store names it, relative to
/// <paramref name="InstallRoot"/>. The Xbox app's config says which exe it starts, which is
/// more than a folder of similar-sized exes can say for itself — but it can name a launcher
/// rather than the game, so it is a hint, checked before it is used.</para></summary>
public sealed record GameCandidate(
    string Name, GameStore Store, string InstallRoot, string? StoreId = null, string? ExeHint = null);

public interface IGameScanner
{
    IEnumerable<GameCandidate> Scan();
}
