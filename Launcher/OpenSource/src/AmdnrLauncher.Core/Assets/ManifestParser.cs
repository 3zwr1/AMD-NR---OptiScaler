// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text.Json;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Assets;

public static class ManifestParser
{
    public const int SupportedSchemaVersion = 1;

    private static readonly JsonSerializerOptions Options = new()
    {
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        AllowTrailingCommas = true,
    };

    public static Manifest Parse(string json)
    {
        // Deserializing into records ignores unknown members by default, which is
        // the forward-compatibility rule the spec asks for.
        var manifest = JsonSerializer.Deserialize<Manifest>(json, Options)
                       ?? throw new FormatException("Manifest JSON was empty.");

        if (manifest.SchemaVersion > SupportedSchemaVersion)
            throw new UnsupportedManifestException(manifest.SchemaVersion, SupportedSchemaVersion);

        // The parameterised-constructor path fills an absent member with null whatever the
        // declared type says. The release manifest carries no compatibility table, and Doctor
        // walks it for every installed game on every scan.
        return manifest with
        {
            Compatibility = manifest.Compatibility ?? [],
            Sources = manifest.Sources?.Where(IsOfferable).Select(Completed).ToList(),
            Packages = manifest.Packages?.Where(IsStorable).ToList() ?? [],

            // A proxy name becomes a path in the game folder, and the launcher often runs as
            // administrator: "..\dxgi.dll" or a rooted name would have it write outside the
            // folder, over a file nothing backs up. Only the names the launcher knows get through.
            ProxyDefaults = manifest.ProxyDefaults?.Where(PayloadNames.IsProxyName).ToList() ?? [],
            ProxyOverrides = manifest.ProxyOverrides?
                .Where(o => o is not null && !string.IsNullOrWhiteSpace(o.Exe) && PayloadNames.IsProxyName(o.Proxy))
                .ToList() ?? [],
        };
    }

    /// <summary>A package's id and version name its folder in the download cache, and a stale
    /// version folder there is deleted recursively before a fresh download. A rooted name, or one
    /// with a separator or "..", would point that delete anywhere on the disk. A package that is
    /// not a zip is also stored under the last segment of its URL, so that has to be one plain
    /// file name as well: "C:version.dll" would put the download beside the launcher.</summary>
    private static bool IsStorable(PackageInfo package)
        => package is not null && IsFolderName(package.Id) && IsFolderName(package.Version)
           && !string.IsNullOrWhiteSpace(package.Url)
           && (AssetStore.IsZip(package) || AssetStore.StoredFileName(package) is not null);

    /// <summary>One file or folder name, and nothing that could make a path out of it.</summary>
    internal static bool IsFolderName(string? name)
        => !string.IsNullOrWhiteSpace(name)
           && name.Trim() == name
           && name is not ("." or "..")
           && name.IndexOfAny(Path.GetInvalidFileNameChars()) < 0;

    /// <summary>The launcher block alone, from a manifest that may not parse as a whole — a
    /// newer schema, most likely, which is precisely the manifest whose launcher update is the
    /// way out. Null when the document is not JSON or the block is incomplete. Nothing here is
    /// trusted more than a full parse would be: the download it leads to is still checked
    /// against the block's own sha256.</summary>
    public static LauncherInfo? TryReadLauncher(string json)
    {
        try
        {
            using var document = JsonDocument.Parse(json, new JsonDocumentOptions
            {
                CommentHandling = JsonCommentHandling.Skip,
                AllowTrailingCommas = true,
            });

            if (document.RootElement.ValueKind != JsonValueKind.Object
                || Member(document.RootElement, "launcher") is not { ValueKind: JsonValueKind.Object } launcher)
            {
                return null;
            }

            return Text(launcher, "version") is { } version
                   && Text(launcher, "url") is { } url
                   && Text(launcher, "sha256") is { } sha256
                ? new LauncherInfo(version, url, sha256)
                : null;
        }
        catch (JsonException)
        {
            return null;
        }
    }

    /// <summary>Case-insensitive, to match what the full parse accepts.</summary>
    private static JsonElement? Member(JsonElement parent, string name)
        => parent.EnumerateObject()
            .Where(p => string.Equals(p.Name, name, StringComparison.OrdinalIgnoreCase))
            .Select(p => (JsonElement?)p.Value)
            .FirstOrDefault();

    private static string? Text(JsonElement parent, string name)
        => Member(parent, name) is { ValueKind: JsonValueKind.String } value
           && value.GetString() is { Length: > 0 } text
            ? text
            : null;

    /// <summary>A build without an id cannot be chosen or recorded, and one without a package
    /// has nothing to install. Offered anyway, either is a choice that fails only after the
    /// user has picked it and waited for the download.</summary>
    private static bool IsOfferable(SourceInfo source)
        => source is not null && !string.IsNullOrWhiteSpace(source.Id) && !string.IsNullOrWhiteSpace(source.Package);

    /// <summary>The descriptive members are only ever shown, so an absent one is shown as
    /// nothing rather than left as a null the UI would trip over.</summary>
    private static SourceInfo Completed(SourceInfo source) => source with
    {
        Name = string.IsNullOrWhiteSpace(source.Name) ? source.Id : source.Name,
        Author = source.Author ?? "",
        Summary = source.Summary ?? "",
        Homepage = source.Homepage ?? "",
        RuntimeNote = source.RuntimeNote ?? "",
        Forwarder = string.IsNullOrWhiteSpace(source.Forwarder) ? null : source.Forwarder,
        IniWhenRuntime = source.IniWhenRuntime?.Where(IsWritable).ToList(),

        // An entry without a package would win the pick for its generations and then name
        // nothing to download; one without a generation can never win. The constructor path
        // leaves an absent gpus member null whatever the declared type says, and a blank
        // generation string is one no GPU probe will ever report.
        Runtimes = source.Runtimes?
            .Where(r => r is not null && !string.IsNullOrWhiteSpace(r.Package))
            .Select(r => r with { Gpus = r.Gpus?.Where(g => !string.IsNullOrWhiteSpace(g)).ToList() ?? [] })
            .Where(r => r.Gpus.Count > 0)
            .ToList(),
    };

    /// <summary>Each setting becomes a line in someone's OptiScaler.ini. One without a section or
    /// key, or with a character that ends a line, a section name or a key, would write a line the
    /// game's ini reader takes for something else — and it is hashed as our seed, so it would
    /// stay that way through every update.</summary>
    private static bool IsWritable(IniSetting setting)
        => setting is not null
           && IsName(setting.Section, ']')
           && IsName(setting.Key, '=') && setting.Key[0] is not (';' or '#')
           && setting.Value is not null && setting.Value.IndexOfAny(['\r', '\n']) < 0;

    private static bool IsName(string? name, char ends)
        => !string.IsNullOrWhiteSpace(name)
           && name.Trim() == name
           && name.IndexOfAny(['\r', '\n', '[', ends]) < 0;
}
