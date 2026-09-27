// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Scanning;


namespace AmdnrLauncher.Core.Tests;

/// <summary>The launchers that keep their games in the registry — Ubisoft Connect, GOG Galaxy,
/// Rockstar, the EA app, Battle.net, Amazon Games — read through a registry made here, never
/// the machine's: what each one writes, what is a game and what is the launcher itself, and a
/// folder that is not there any more.</summary>
public sealed class RegistryScannersTests : IDisposable
{
    private readonly TempDir _dir = new();
    private readonly FakeRegistry _registry = new();

    public void Dispose() => _dir.Dispose();

    private string Folder(string name)
    {
        var path = Path.Combine(_dir.Path, name);
        Directory.CreateDirectory(path);
        return path;
    }

    private const string Uninstall32 = @"SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall";
    private const string Uninstall64 = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall";

    [Fact]
    public void Ubisoft_games_are_read_from_Installs_and_named_from_Add_or_remove_programs()
    {
        var farCry = Folder("Far Cry 6");
        var unnamed = Folder("Assassins Creed Mirage");

        // Ubisoft writes forward slashes and a trailing slash.
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Ubisoft\Launcher\Installs\5266", "InstallDir", farCry.Replace('\\', '/') + "/");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Uplay Install 5266", "DisplayName", "Far Cry 6");
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Ubisoft\Launcher\Installs\7777", "InstallDir", unnamed);
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Ubisoft\Launcher\Installs\9999", "InstallDir", @"D:\Gone\Uninstalled by hand");

        var found = new UbisoftScanner(_registry).Scan().ToList();

        Assert.Equal(2, found.Count);
        var named = found.Single(c => c.StoreId == "5266");
        Assert.Equal(("Far Cry 6", GameStore.Ubisoft, farCry), (named.Name, named.Store, named.InstallRoot));
        Assert.Equal("Assassins Creed Mirage", found.Single(c => c.StoreId == "7777").Name);
    }

    [Fact]
    public void GOG_games_carry_their_name_and_exe_and_DLCs_are_not_games()
    {
        var witcher = Folder("The Witcher 3");
        const string key = @"SOFTWARE\WOW6432Node\GOG.com\Games\1207664663";
        _registry.Add(RegistryRoot.LocalMachine, key, "gameName", "The Witcher 3: Wild Hunt");
        _registry.Add(RegistryRoot.LocalMachine, key, "path", witcher);
        _registry.Add(RegistryRoot.LocalMachine, key, "exe", Path.Combine(witcher, "bin", "x64", "witcher3.exe"));

        const string dlc = @"SOFTWARE\WOW6432Node\GOG.com\Games\1640424747";
        _registry.Add(RegistryRoot.LocalMachine, dlc, "gameName", "The Witcher 3: Blood and Wine");
        _registry.Add(RegistryRoot.LocalMachine, dlc, "path", witcher);
        _registry.Add(RegistryRoot.LocalMachine, dlc, "dependsOn", "1207664663");

        var game = Assert.Single(new GogScanner(_registry).Scan());

        Assert.Equal("The Witcher 3: Wild Hunt", game.Name);
        Assert.Equal(GameStore.GOG, game.Store);
        Assert.Equal(witcher, game.InstallRoot);
        Assert.Equal("1207664663", game.StoreId);
        Assert.Equal(@"bin\x64\witcher3.exe", game.ExeHint);
    }

    [Fact]
    public void Rockstar_games_come_from_the_launchers_keys_and_from_Add_or_remove_programs_but_not_the_launcher()
    {
        var enhanced = Folder("Grand Theft Auto V");
        var legacy = Folder("GTAV No Mods");
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Rockstar Games\Grand Theft Auto V", "InstallFolder", enhanced);
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Rockstar Games\Launcher", "InstallFolder", Folder("Launcher"));
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Rockstar Games\Rockstar Games Social Club", "Version", "2.1");

        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Grand Theft Auto V Legacy", "DisplayName", "Grand Theft Auto V Legacy");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Grand Theft Auto V Legacy", "Publisher", "Rockstar Games");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Grand Theft Auto V Legacy", "InstallLocation", legacy);
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Rockstar Games Launcher", "DisplayName", "Rockstar Games Launcher");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Rockstar Games Launcher", "Publisher", "Rockstar Games");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Rockstar Games Launcher", "InstallLocation", Folder("Launcher"));
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Rockstar Games Social Club", "DisplayName", "Rockstar Games SDK");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Rockstar Games Social Club", "Publisher", "Rockstar Games");

        var found = new RockstarScanner(_registry).Scan().ToList();

        Assert.Equal(
            [("Grand Theft Auto V", enhanced), ("Grand Theft Auto V Legacy", legacy)],
            found.Select(c => (c.Name, c.InstallRoot)));
        Assert.All(found, c => Assert.Equal(GameStore.Rockstar, c.Store));
    }

    [Fact]
    public void EA_games_come_from_the_games_own_keys_and_from_Add_or_remove_programs_but_not_the_app()
    {
        var bf = Folder("Battlefield 6");
        var fc = Folder("EA SPORTS FC 26");
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Electronic Arts\EA Desktop", "InstallLocation", Folder("EA Desktop"));
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\Electronic Arts\EADM", "Install Dir", Folder("Origin"));
        _registry.Add(RegistryRoot.LocalMachine, @"SOFTWARE\WOW6432Node\EA Games\Battlefield 6", "Install Dir", bf + @"\");

        _registry.Add(RegistryRoot.LocalMachine, Uninstall64 + @"\{FC26}", "DisplayName", "EA SPORTS FC 26");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall64 + @"\{FC26}", "Publisher", "Electronic Arts");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall64 + @"\{FC26}", "InstallLocation", fc);
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\{7c5cb540}", "DisplayName", "EA app");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\{7c5cb540}", "Publisher", "Electronic Arts");

        var found = new EaScanner(_registry).Scan().ToList();

        Assert.Equal([("Battlefield 6", bf), ("EA SPORTS FC 26", fc)], found.Select(c => (c.Name, c.InstallRoot)));
        Assert.All(found, c => Assert.Equal(GameStore.EA, c.Store));
    }

    [Fact]
    public void Battle_net_and_Amazon_games_come_from_Add_or_remove_programs()
    {
        var diablo = Folder("Diablo IV");
        var cod = Folder("Call of Duty");
        var lost = Folder("Lost Ark");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Diablo IV", "DisplayName", "Diablo IV");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Diablo IV", "Publisher", "Blizzard Entertainment");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Diablo IV", "InstallLocation", diablo);
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Call of Duty", "DisplayName", "Call of Duty");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Call of Duty", "Publisher", "Activision");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Call of Duty", "InstallLocation", cod);
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Battle.net", "DisplayName", "Battle.net");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Battle.net", "Publisher", "Blizzard Entertainment");
        _registry.Add(RegistryRoot.LocalMachine, Uninstall32 + @"\Battle.net", "InstallLocation", Folder("Battle.net"));
        _registry.Add(RegistryRoot.CurrentUser, Uninstall64 + @"\AmazonGames/Lost Ark", "DisplayName", "Lost Ark");
        _registry.Add(RegistryRoot.CurrentUser, Uninstall64 + @"\AmazonGames/Lost Ark", "InstallLocation", lost);

        var blizzard = new BattleNetScanner(_registry).Scan().ToList();
        Assert.Equal([("Diablo IV", diablo), ("Call of Duty", cod)], blizzard.Select(c => (c.Name, c.InstallRoot)));
        Assert.All(blizzard, c => Assert.Equal(GameStore.BattleNet, c.Store));

        var amazon = Assert.Single(new AmazonScanner(_registry).Scan());
        Assert.Equal(("Lost Ark", GameStore.Amazon, lost), (amazon.Name, amazon.Store, amazon.InstallRoot));
    }

    [Fact]
    public void A_machine_without_the_launcher_yields_nothing_and_the_real_registry_can_be_read()
    {
        Assert.Empty(new UbisoftScanner(_registry).Scan());
        Assert.Empty(new GogScanner(_registry).Scan());
        Assert.Empty(new RockstarScanner(_registry).Scan());
        Assert.Empty(new EaScanner(_registry).Scan());
        Assert.Empty(new BattleNetScanner(_registry).Scan());
        Assert.Empty(new AmazonScanner(_registry).Scan());

        // The machine's registry answers with lists and strings, never with an exception: this
        // reads a key every Windows has and one that no Windows has.
        var real = new WindowsRegistryReader();
        Assert.NotEmpty(real.SubKeys(RegistryRoot.LocalMachine, @"SOFTWARE\Microsoft\Windows\CurrentVersion"));
        Assert.Empty(real.SubKeys(RegistryRoot.LocalMachine, @"SOFTWARE\AMDNR-test-no-such-key"));
        Assert.Null(real.String(RegistryRoot.LocalMachine, @"SOFTWARE\AMDNR-test-no-such-key", "Anything"));
    }

    [Fact]
    public void Every_store_has_a_label_a_person_reads()
    {
        Assert.Equal("Ubisoft Connect", GameStoreLabels.Label(GameStore.Ubisoft));
        Assert.Equal("EA app", GameStoreLabels.Label(GameStore.EA));
        Assert.Equal("GOG", GameStoreLabels.Label(GameStore.GOG));
        Assert.Equal("Rockstar", GameStoreLabels.Label(GameStore.Rockstar));
        Assert.Equal("Battle.net", GameStoreLabels.Label(GameStore.BattleNet));
        Assert.Equal("Amazon Games", GameStoreLabels.Label(GameStore.Amazon));
        Assert.Equal("Steam", GameStoreLabels.Label(GameStore.Steam));
        Assert.Equal("Xbox", GameStoreLabels.Label(GameStore.Xbox));
    }

    /// <summary>A registry made of the values a test adds. Subkeys are whatever was added
    /// under a path, one level down.</summary>
    private sealed class FakeRegistry : IRegistryReader
    {
        private readonly Dictionary<(RegistryRoot, string), Dictionary<string, string>> _keys =
            new(new KeyComparer());

        public void Add(RegistryRoot hive, string path, string name, string value)
        {
            if (!_keys.TryGetValue((hive, path), out var values))
                _keys[(hive, path)] = values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            values[name] = value;
        }

        public IReadOnlyList<string> SubKeys(RegistryRoot hive, string path)
        {
            var prefix = path.TrimEnd('\\') + @"\";
            return _keys.Keys
                .Where(k => k.Item1 == hive && k.Item2.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
                .Select(k => k.Item2[prefix.Length..].Split('\\')[0])
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToList();
        }

        public string? String(RegistryRoot hive, string path, string name)
            => _keys.TryGetValue((hive, path), out var values) && values.TryGetValue(name, out var value) ? value : null;

        private sealed class KeyComparer : IEqualityComparer<(RegistryRoot, string)>
        {
            public bool Equals((RegistryRoot, string) x, (RegistryRoot, string) y)
                => x.Item1 == y.Item1 && string.Equals(x.Item2, y.Item2, StringComparison.OrdinalIgnoreCase);

            public int GetHashCode((RegistryRoot, string) obj)
                => HashCode.Combine(obj.Item1, obj.Item2.ToUpperInvariant());
        }
    }
}
