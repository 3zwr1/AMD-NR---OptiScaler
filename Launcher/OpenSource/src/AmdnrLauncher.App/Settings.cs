// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.App;

public sealed class Settings
{
    /// <summary>Compiled in, not a setting. Every launcher ever shipped reads this one address,
    /// so the file behind it is the release; a user-editable copy saved by an earlier launcher
    /// would pin that user to whatever it said, and theirs pointed at a placeholder host.</summary>
    public const string DefaultManifestUrl =
        "https://raw.githubusercontent.com/3zwr1/AMD-NR---OptiScaler/main/Launcher/manifest.json";

    public static string DefaultFilePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "AMDNR", "settings.json");

    private string _filePath = DefaultFilePath;

    /// <summary>The build the user picked in the chooser. Null until they have: that is what
    /// makes the next start show the chooser.</summary>
    public string? SourceId { get; set; }

    /// <summary>Whether they asked for the DLSSNR AMD files. Nullable for the same reason as
    /// SourceId — "not asked yet" is not the same answer as "no".</summary>
    public bool? IncludeRuntime { get; set; }

    /// <summary>The runtime package they picked in the chooser, or null to follow the one the
    /// manifest lists first for their card. Absent from files written before the picker existed,
    /// which reads as no pick — the default those users had.</summary>
    public string? RuntimeId { get; set; }

    public string AssetRoot { get; set; } = AssetStore.DefaultRoot;

    /// <summary>Ignored by the serializer both ways: never written, so a tester's address
    /// cannot outlive the run it was given for, and never read, so the address an earlier
    /// launcher saved is dropped on upgrade.</summary>
    [JsonIgnore]
    public string ManifestUrl { get; private set; } = DefaultManifestUrl;

    [JsonIgnore]
    public bool IsTestManifest { get; private set; }

    /// <summary>Both halves or nothing. Half a choice cannot be installed, and asking again is
    /// cheaper than guessing the other half.</summary>
    [JsonIgnore]
    public InstallChoice? Choice
    {
        get => SourceId is { Length: > 0 } id && IncludeRuntime is { } runtime
            ? new InstallChoice(id, runtime, RuntimeId)
            : null;
        set
        {
            SourceId = value?.SourceId;
            IncludeRuntime = value?.IncludeRuntime;
            RuntimeId = value?.RuntimeId;
        }
    }

    /// <summary><c>--manifest &lt;url&gt;</c> or <c>--manifest=&lt;url&gt;</c>: points this run,
    /// and only this run, at another manifest, which is how a release is tried before users see
    /// it. Not validated here — an address nothing can fetch fails on the fetch with a message,
    /// and since it is never saved the next start is back to normal.</summary>
    public void ApplyCommandLine(IEnumerable<string> args)
    {
        var list = args.ToList();
        for (var i = 0; i < list.Count; i++)
        {
            string? url = null;
            if (string.Equals(list[i], "--manifest", StringComparison.OrdinalIgnoreCase) && i + 1 < list.Count)
                url = list[i + 1];
            else if (list[i].StartsWith("--manifest=", StringComparison.OrdinalIgnoreCase))
                url = list[i]["--manifest=".Length..];

            if (string.IsNullOrWhiteSpace(url)) continue;

            ManifestUrl = url.Trim();
            IsTestManifest = true;
        }
    }

    /// <summary>Never throws. This runs before the window renders, so an unreadable settings
    /// file would stop the app from starting at all — defaults are a far better outcome than
    /// no launcher.</summary>
    public static Settings Load() => Load(DefaultFilePath);

    internal static Settings Load(string filePath)
    {
        Settings settings;
        try
        {
            settings = File.Exists(filePath)
                ? JsonSerializer.Deserialize<Settings>(File.ReadAllText(filePath)) ?? new Settings()
                : new Settings();
        }
        catch (Exception e) when (e is JsonException or IOException or UnauthorizedAccessException)
        {
            settings = new Settings();
        }

        settings._filePath = filePath;
        return settings;
    }

    /// <summary>Returns false when the settings could not be written, so the caller can say so
    /// rather than crashing a click handler.</summary>
    public bool Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_filePath)!);
            File.WriteAllText(
                _filePath, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }
}
