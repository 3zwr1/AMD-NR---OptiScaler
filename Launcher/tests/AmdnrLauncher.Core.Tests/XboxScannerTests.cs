// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

/// <summary>Xbox app libraries as this PC has them: a .GamingRoot file at the root of each
/// drive that hosts one, naming the library folder, and a game per folder whose Content holds
/// MicrosoftGame.config.</summary>
public class XboxScannerTests
{
    /// <summary>The 28 bytes of E:\.GamingRoot on this PC: "RGBX", version 1, then
    /// "XboxGames" as NUL-terminated UTF-16LE.</summary>
    private static byte[] GamingRoot(params string[] libraries)
    {
        var bytes = new List<byte>(Encoding.ASCII.GetBytes("RGBX"));
        bytes.AddRange(BitConverter.GetBytes(1));
        foreach (var library in libraries) bytes.AddRange(Encoding.Unicode.GetBytes(library + "\0"));
        return [.. bytes];
    }

    private const string ForzaConfig = """
        <?xml version="1.0" encoding="utf-8"?>
        <Game configVersion="1">
          <Identity Name="Microsoft.ForteBaseGame" Publisher="CN=Microsoft Corporation" Version="3.440.853.0" />
          <ExecutableList>
            <Executable Name="forzahorizon6.exe" Id="Forzahorizon6" OverrideDisplayName="Forza Horizon 6" TargetDeviceFamily="PC" IsDevOnly="false" />
          </ExecutableList>
          <ShellVisuals DefaultDisplayName="Forza Horizon 6" PublisherDisplayName="Microsoft Studios" />
        </Game>
        """;

    private static string AddGame(string library, string folder, string? config, string configName = "MicrosoftGame.config")
    {
        var content = Path.Combine(library, folder, "Content");
        Directory.CreateDirectory(content);
        if (config is not null) File.WriteAllText(Path.Combine(content, configName), config, new UTF8Encoding(true));
        return content;
    }

    [Fact]
    public void GamingRoot_file_is_parsed_into_the_library_folder_name()
    {
        var real = GamingRoot("XboxGames");
        Assert.Equal(28, real.Length);

        Assert.Equal(["XboxGames"], XboxScanner.ParseGamingRoot(real));
        Assert.Equal(["XboxGames", @"Games\Xbox"], XboxScanner.ParseGamingRoot(GamingRoot("XboxGames", @"Games\Xbox")));

        // Anything without the header is not a GamingRoot file at all.
        Assert.Empty(XboxScanner.ParseGamingRoot(Encoding.ASCII.GetBytes("not a gaming root")));
        Assert.Empty(XboxScanner.ParseGamingRoot([]));
    }

    [Fact]
    public void A_Forza_shaped_folder_becomes_an_Xbox_candidate_named_from_its_config_with_Content_as_its_root()
    {
        using var drive = new TempDir();
        File.WriteAllBytes(Path.Combine(drive.Path, ".GamingRoot"), GamingRoot("XboxGames"));
        var content = AddGame(Path.Combine(drive.Path, "XboxGames"), "Forza Horizon 6", ForzaConfig);

        var game = Assert.Single(new XboxScanner([drive.Path]).Scan());

        Assert.Equal("Forza Horizon 6", game.Name);
        Assert.Equal(GameStore.Xbox, game.Store);
        Assert.Equal(content, game.InstallRoot);
        Assert.Equal("Microsoft.ForteBaseGame", game.StoreId);
        Assert.Equal("forzahorizon6.exe", game.ExeHint);
    }

    [Fact]
    public void A_drive_without_GamingRoot_but_with_XboxGames_is_still_scanned()
    {
        using var drive = new TempDir();
        AddGame(Path.Combine(drive.Path, "XboxGames"), "Forza Horizon 6", ForzaConfig);

        Assert.Equal(["Forza Horizon 6"], new XboxScanner([drive.Path]).Scan().Select(g => g.Name));
    }

    [Fact]
    public void The_library_the_GamingRoot_names_is_the_one_scanned()
    {
        using var drive = new TempDir();
        File.WriteAllBytes(Path.Combine(drive.Path, ".GamingRoot"), GamingRoot("Games"));
        AddGame(Path.Combine(drive.Path, "Games"), "Forza Horizon 6", ForzaConfig);

        Assert.Single(new XboxScanner([drive.Path]).Scan());
    }

    [Fact]
    public void A_folder_without_a_config_is_skipped()
    {
        // C:\XboxGames\GameSave on this PC: a folder the Xbox app keeps, not a game.
        using var drive = new TempDir();
        var library = Path.Combine(drive.Path, "XboxGames");
        Directory.CreateDirectory(Path.Combine(library, "GameSave"));
        AddGame(library, "No Config", config: null);

        Assert.Empty(new XboxScanner([drive.Path]).Scan());
    }

    [Fact]
    public void A_malformed_config_skips_that_game_and_the_scan_continues()
    {
        using var drive = new TempDir();
        var library = Path.Combine(drive.Path, "XboxGames");
        AddGame(library, "Broken", "<Game><Identity Name=\"x\"");
        AddGame(library, "Uses a DTD", """
            <?xml version="1.0"?>
            <!DOCTYPE Game [ <!ENTITY name "Expanded"> ]>
            <Game><ShellVisuals DefaultDisplayName="&name;" /></Game>
            """);
        AddGame(library, "Forza Horizon 6", ForzaConfig);

        Assert.Equal(["Forza Horizon 6"], new XboxScanner([drive.Path, @"Z:\no\such\drive"]).Scan().Select(g => g.Name));
    }

    [Fact]
    public void The_name_falls_back_and_loses_its_trademark_signs()
    {
        // Call of Duty's config names it with a registered sign, and the folder name is what is
        // left when the config names nothing a person can read.
        using var drive = new TempDir();
        var library = Path.Combine(drive.Path, "XboxGames");
        AddGame(library, "Call of Duty- Modern Warfare 3", """
            <Game><ExecutableList><Executable Name="bootstrapper.exe" Id="codShip" /></ExecutableList>
            <ShellVisuals DefaultDisplayName="Call of Duty® Modern Warfare 3" /></Game>
            """, configName: "MicrosoftGame.Config");
        AddGame(library, "Localised", """
            <Game><ExecutableList><Executable Name="game.exe" OverrideDisplayName="Some Game™ " /></ExecutableList>
            <ShellVisuals DefaultDisplayName="ms-resource:ApplicationDisplayName" /></Game>
            """);
        AddGame(library, "Nameless Folder", "<Game />");

        var names = new XboxScanner([drive.Path]).Scan().Select(g => g.Name).Order().ToList();

        Assert.Equal(["Call of Duty Modern Warfare 3", "Nameless Folder", "Some Game"], names);
    }

    [Fact]
    public void A_folder_named_only_in_trademark_signs_is_still_listed_and_the_scan_does_not_throw()
    {
        // Nothing readable in the config, and a folder name that is empty once ® and ™ go.
        // The scanner never throws: the scan runs from a command with nothing to catch it.
        using var drive = new TempDir();
        var library = Path.Combine(drive.Path, "XboxGames");
        AddGame(library, "®™", "<Game><ShellVisuals DefaultDisplayName=\"ms-resource:Name\" /></Game>");
        AddGame(library, "Forza Horizon 6", ForzaConfig);

        var names = new XboxScanner([drive.Path]).Scan().Select(g => g.Name).Order(StringComparer.Ordinal).ToList();

        Assert.Equal(["Forza Horizon 6", "®™"], names);
    }
}
