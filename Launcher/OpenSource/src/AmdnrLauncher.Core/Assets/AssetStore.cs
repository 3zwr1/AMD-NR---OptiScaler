// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO.Compression;

namespace AmdnrLauncher.Core.Assets;

public interface IAssetStore
{
    bool Has(PackageInfo package);
    string PathFor(PackageInfo package);
    Task EnsureAsync(PackageInfo package, IProgress<DownloadProgress>? progress, CancellationToken ct);
}

public sealed class AssetStore(string rootDirectory, ResumableDownloader downloader) : IAssetStore
{
    public static string DefaultRoot => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDNR", "assets");

    internal static bool IsZip(PackageInfo p)
        => p.Url.EndsWith(".zip", StringComparison.OrdinalIgnoreCase);

    /// <summary>The name a package that is not a zip is stored under: the last segment of its
    /// URL, when that is one plain file name, and null otherwise. It is joined onto the
    /// package's folder in the store, and a name such as "C:version.dll" survives
    /// Path.GetFileName and is rooted — joined, it replaces the folder instead of going into
    /// it, and the download lands in the current directory on C:.</summary>
    internal static string? StoredFileName(PackageInfo p)
    {
        if (!Uri.TryCreate(p.Url, UriKind.Absolute, out var uri)) return null;

        var name = Path.GetFileName(uri.LocalPath);
        return ManifestParser.IsFolderName(name) ? name : null;
    }

    private string VersionDirectory(PackageInfo p)
        => Path.Combine(rootDirectory, p.Id, p.Version);

    /// <summary>A SIBLING of the version directory, never inside it. `PathFor` hands the version
    /// directory to the installer, which copies everything under it into a game folder — a marker
    /// stored inside would be copied into every game and recorded as an installed file.</summary>
    private string MarkerPath(PackageInfo p)
        => Path.Combine(rootDirectory, p.Id, p.Version + ".complete");

    public string PathFor(PackageInfo p)
        => IsZip(p)
            ? VersionDirectory(p)
            : Path.Combine(VersionDirectory(p), StoredFileName(p)
                ?? throw new InvalidOperationException(
                    $"The download for \"{p.Id}\" does not end in a file name ({p.Url}), so it cannot be stored."));

    /// <summary>All three halves matter: the marker proves the download finished, the payload
    /// check keeps a marker left behind by a hand-deleted store from reporting a package that is
    /// gone, and the hash in the marker ties the cache to the bytes rather than to a version typed
    /// by hand. A rebuilt forwarder published under the old version would otherwise never reach
    /// anyone who had that version cached, and every Repair would put the old one back.</summary>
    public bool Has(PackageInfo p)
        => MarkedHash(p) is { } hash && Hashing.Matches(hash, p.Sha256)
           && (IsZip(p) ? Directory.Exists(PathFor(p)) : StoredFileName(p) is not null && File.Exists(PathFor(p)));

    private string? MarkedHash(PackageInfo p)
    {
        try
        {
            var marker = MarkerPath(p);
            return File.Exists(marker) ? File.ReadAllText(marker).Trim() : null;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    public async Task EnsureAsync(PackageInfo p, IProgress<DownloadProgress>? progress, CancellationToken ct)
    {
        if (Has(p)) return;

        // Worked out before anything is deleted, so a package with nowhere to go costs nothing.
        var target = PathFor(p);

        // The marker goes first. Left beside a folder emptied for a download that then failed,
        // it would still name the old hash — and should the manifest ever go back to that hash,
        // the empty folder would pass for the package, and every install would place no
        // OptiScaler at all.
        var marker = MarkerPath(p);
        if (File.Exists(marker)) File.Delete(marker);

        var versionDir = VersionDirectory(p);
        if (Directory.Exists(versionDir)) Directory.Delete(versionDir, recursive: true);
        Directory.CreateDirectory(versionDir);

        if (IsZip(p))
        {
            var archive = Path.Combine(rootDirectory, p.Id, p.Version + ".zip");
            await downloader.DownloadAsync(p.Url, archive, p.Sha256, p.Size, progress, ct);
            ZipFile.ExtractToDirectory(archive, versionDir, overwriteFiles: true);
            File.Delete(archive);
        }
        else
        {
            await downloader.DownloadAsync(p.Url, target, p.Sha256, p.Size, progress, ct);
        }

        File.WriteAllText(marker, p.Sha256);
    }
}
