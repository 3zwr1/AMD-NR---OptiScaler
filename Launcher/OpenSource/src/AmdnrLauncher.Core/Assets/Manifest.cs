// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Assets;

public sealed record LauncherInfo(string Version, string Url, string Sha256);

public sealed record PackageInfo(
    string Id,
    string Version,
    string Url,
    string Sha256,
    long Size,
    IReadOnlyList<string>? PassSha256,
    IReadOnlyList<string>? Exclude = null);

public sealed record SourceInfo(
    string Id,
    string Name,
    string Author,
    string Summary,
    string Homepage,
    string Package,
    string? Forwarder,
    bool RuntimeRequired,
    string RuntimeNote,
    IReadOnlyList<IniSetting>? IniWhenRuntime = null,
    IReadOnlyList<RuntimeChoice>? Runtimes = null)
{
    /// <summary>The one runtime package id every manifest written before builds could list
    /// their runtimes carried, and what a source without the list still means.</summary>
    public const string LegacyRuntimePackage = "runtime";

    /// <summary>The runtime package to install on a GPU of this generation (rdna1, rdna2,
    /// rdna3, rdna4, unknown-amd or none): the first entry of <see cref="Runtimes"/> whose Gpus
    /// lists it. Order decides, not how specific an entry is, because reordering the manifest is
    /// how the publisher changes a generation's default without a new launcher. Null when the
    /// build lists runtimes and none of them names the generation: that GPU is offered no runtime
    /// rather than one that was not meant for it. A source written before there was a list
    /// falls back to <see cref="LegacyRuntimePackage"/>.</summary>
    public string? RuntimePackageFor(string generation)
        => Runtimes is null
            ? LegacyRuntimePackage
            : Runtimes.FirstOrDefault(r => r.Gpus.Contains(generation, StringComparer.OrdinalIgnoreCase))?.Package;

    /// <summary>The runtime the user picked, when this build lists it for the card; otherwise
    /// <see cref="RuntimePackageFor(string)"/>. A pick made for another card, or for a runtime
    /// the build has since dropped, is no pick at all: a runtime that was not meant for the card
    /// is worse than an old one.</summary>
    public string? RuntimePackageFor(string generation, string? preferred)
        => preferred is not null && ListsForCard(preferred, generation) ? preferred : RuntimePackageFor(generation);

    /// <summary>The runtime packages this build lists for the card, in the manifest's order:
    /// what a chooser can offer. A build written before there was a list offers the one
    /// package every older manifest had.</summary>
    public IReadOnlyList<string> RuntimesFor(string generation)
        => Runtimes is null
            ? [LegacyRuntimePackage]
            : Runtimes.Where(r => r.Gpus.Contains(generation, StringComparer.OrdinalIgnoreCase))
                .Select(r => r.Package)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToList();

    private bool ListsForCard(string package, string generation)
        => Runtimes?.Any(r => string.Equals(r.Package, package, StringComparison.OrdinalIgnoreCase)
                              && r.Gpus.Contains(generation, StringComparer.OrdinalIgnoreCase)) == true;
}

/// <summary>One runtime package and the GPU generations it is offered to.</summary>
public sealed record RuntimeChoice(string Package, IReadOnlyList<string> Gpus);

public sealed record IniSetting(string Section, string Key, string Value);

public sealed record CompatibilityRule(string Amdnr, IReadOnlyList<string> Runtime);

public sealed record ProxyOverride(string Exe, string Proxy, string? Reason);

public sealed record Manifest(
    int SchemaVersion,
    LauncherInfo Launcher,
    IReadOnlyList<PackageInfo> Packages,
    IReadOnlyList<CompatibilityRule> Compatibility,
    IReadOnlyList<string> ProxyDefaults,
    IReadOnlyList<ProxyOverride> ProxyOverrides,
    IReadOnlyList<SourceInfo>? Sources = null,
    IReadOnlyList<string>? HiddenSteamAppIds = null)
{
    /// <summary>What every manifest written before builds could be chosen offered: AMDNR, with
    /// the runtime and its forwarder. Such a manifest is still what an older cache holds when the
    /// launcher runs offline, and what the published URL serves until it gains the list.</summary>
    private static readonly SourceInfo LegacyAmdnr = new(
        Id: "amdnr", Name: "AMDNR", Author: "3zwr1",
        Summary: "OptiScaler with AMD Neural Rendering.",
        Homepage: "https://github.com/3zwr1/AMD-NR---OptiScaler",
        Package: "amdnr", Forwarder: "forwarder", RuntimeRequired: false,
        RuntimeNote: "Needed on RX 7000.");

    /// <summary>The builds a user can choose from. A manifest without the list is one written
    /// before there was a choice, and reading it as offering nothing would cost every AMDNR
    /// install its Doctor checks and its update, and leave Install with nothing to install. A
    /// manifest that has the list means it, even when AMDNR is not on it.</summary>
    public IReadOnlyList<SourceInfo> OfferedSources => Sources ?? [LegacyAmdnr];

    /// <summary>Null when the manifest does not offer that build. The id comes from a choice
    /// the user saved against an earlier manifest, and a record written by an earlier install,
    /// so either can name a build the current manifest has dropped.</summary>
    public SourceInfo? TrySource(string id)
        => OfferedSources.FirstOrDefault(s => string.Equals(s.Id, id, StringComparison.OrdinalIgnoreCase));

    public PackageInfo Package(string id)
        => TryPackage(id)
           ?? throw new KeyNotFoundException($"Manifest has no package with id '{id}'.");

    /// <summary>Null when the manifest does not list that package. A stale cached manifest or a
    /// hand-edited one can be missing any entry, and the code paths that run on every scan
    /// cannot afford to throw for it — there is no catch between them and the UI.</summary>
    public PackageInfo? TryPackage(string id)
        => Packages.FirstOrDefault(p => string.Equals(p.Id, id, StringComparison.OrdinalIgnoreCase));
}

public sealed class UnsupportedManifestException(int found, int supported)
    : Exception($"This manifest uses schemaVersion {found}; this launcher understands {supported}. " +
                "Please update the launcher.");
