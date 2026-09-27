// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using AmdnrLauncher.Core;

namespace AmdnrLauncher.App.Tests;

public sealed class SettingsTests : IDisposable
{
    // Never the real %LOCALAPPDATA%\AMDNR: a test that saved there would decide which build the
    // owner's own launcher installs next.
    private readonly string _root = Path.Combine(
        Path.GetTempPath(), "amdnr-settings-" + Guid.NewGuid().ToString("N"));

    private string SettingsPath => Path.Combine(_root, "AMDNR", "settings.json");

    public void Dispose()
    {
        try { Directory.Delete(_root, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    [Fact]
    public void Settings_round_trip_the_choice()
    {
        var fresh = Settings.Load(SettingsPath);
        Assert.Null(fresh.SourceId);
        Assert.Null(fresh.IncludeRuntime);
        Assert.Null(fresh.Choice);

        fresh.Choice = new InstallChoice("theautomatic", IncludeRuntime: false);
        Assert.True(fresh.Save());

        var reloaded = Settings.Load(SettingsPath);
        Assert.Equal("theautomatic", reloaded.SourceId);
        Assert.False(reloaded.IncludeRuntime);
        Assert.Null(reloaded.RuntimeId);
        Assert.Equal(new InstallChoice("theautomatic", false), reloaded.Choice);

        // The runtime the user picked travels with the choice; no pick reads back as no pick.
        reloaded.Choice = new InstallChoice("amdnr", IncludeRuntime: true, RuntimeId: "runtime-033");
        Assert.True(reloaded.Save());
        var withPick = Settings.Load(SettingsPath);
        Assert.Equal("runtime-033", withPick.RuntimeId);
        Assert.Equal(new InstallChoice("amdnr", true, "runtime-033"), withPick.Choice);

        // A settings file written before the pick existed is a choice that follows the manifest.
        File.WriteAllText(SettingsPath, """{ "SourceId": "amdnr", "IncludeRuntime": true }""");
        Assert.Equal(new InstallChoice("amdnr", true), Settings.Load(SettingsPath).Choice);
    }

    [Fact]
    public void The_command_line_manifest_override_is_never_saved()
    {
        const string testUrl = "https://raw.githubusercontent.com/3zwr1/AMD-NR---OptiScaler/launcher-0.4.0/Launcher/manifest.json";

        var settings = Settings.Load(SettingsPath);
        Assert.Equal(Settings.DefaultManifestUrl, settings.ManifestUrl);
        Assert.False(settings.IsTestManifest);

        settings.ApplyCommandLine(["--manifest", testUrl]);
        Assert.Equal(testUrl, settings.ManifestUrl);
        Assert.True(settings.IsTestManifest);

        // Saving for another reason — the chooser does, on every first run — must not carry
        // the tester's address into the next ordinary start.
        settings.Choice = new InstallChoice("amdnr", IncludeRuntime: true);
        Assert.True(settings.Save());
        Assert.DoesNotContain("launcher-0.4.0", File.ReadAllText(SettingsPath));

        var reloaded = Settings.Load(SettingsPath);
        Assert.Equal(Settings.DefaultManifestUrl, reloaded.ManifestUrl);
        Assert.False(reloaded.IsTestManifest);
    }

    [Fact]
    public void A_manifest_url_saved_by_an_earlier_launcher_is_ignored()
    {
        // Up to 0.3 the address was user-editable and persisted, and its default pointed at a
        // placeholder host. Honouring it would leave every upgraded user on a dead address.
        Directory.CreateDirectory(Path.GetDirectoryName(SettingsPath)!);
        File.WriteAllText(SettingsPath,
            """{ "ManifestUrl": "https://manifest.invalid/amdnr/manifest.json", "AssetRoot": "D:\\Assets" }""");

        var settings = Settings.Load(SettingsPath);

        Assert.Equal(Settings.DefaultManifestUrl, settings.ManifestUrl);
        Assert.False(settings.IsTestManifest);
        Assert.Equal(@"D:\Assets", settings.AssetRoot);
    }

    [Fact]
    public void The_compiled_in_manifest_is_the_one_on_main()
    {
        Assert.Equal(
            "https://raw.githubusercontent.com/3zwr1/AMD-NR---OptiScaler/main/Launcher/manifest.json",
            Settings.DefaultManifestUrl);
    }
}
