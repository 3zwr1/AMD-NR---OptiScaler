// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO.Compression;
using System.Net;
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.Core.Tests;

public class AssetStoreTests
{
    private sealed class BytesHandler(byte[] body) : HttpMessageHandler
    {
        public int Calls { get; private set; }

        protected override Task<HttpResponseMessage> SendAsync(
            HttpRequestMessage request, CancellationToken cancellationToken)
        {
            Calls++;
            return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK)
            {
                Content = new ByteArrayContent(body),
            });
        }
    }

    private static byte[] MakeZip(params (string Name, string Content)[] entries)
    {
        using var ms = new MemoryStream();
        using (var zip = new ZipArchive(ms, ZipArchiveMode.Create, leaveOpen: true))
        {
            foreach (var (name, content) in entries)
            {
                using var writer = new StreamWriter(zip.CreateEntry(name).Open());
                writer.Write(content);
            }
        }
        return ms.ToArray();
    }

    private static (AssetStore Store, BytesHandler Handler) Make(string root, byte[] body)
    {
        var handler = new BytesHandler(body);
        return (new AssetStore(root, new ResumableDownloader(new HttpClient(handler))), handler);
    }

    private static PackageInfo Pkg(string id, string url, byte[] body) => new(
        id, "1.0.0", url,
        Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(body)),
        body.Length, null);

    [Fact]
    public async Task Zip_package_is_extracted_into_a_versioned_directory()
    {
        using var dir = new TempDir();
        var body = MakeZip(("OptiScaler.dll", "x"), ("OptiScaler/libxess.dll", "y"));
        var (store, _) = Make(dir.Path, body);
        var pkg = Pkg("amdnr", "https://h/AMDNR-v0.2.1.zip", body);

        await store.EnsureAsync(pkg, null, default);

        var extracted = store.PathFor(pkg);
        Assert.True(File.Exists(Path.Combine(extracted, "OptiScaler.dll")));
        Assert.True(File.Exists(Path.Combine(extracted, "OptiScaler", "libxess.dll")));
    }

    [Fact]
    public async Task Bare_file_package_is_stored_as_a_file_not_extracted()
    {
        using var dir = new TempDir();
        var body = "forwarder-bytes"u8.ToArray();
        var (store, _) = Make(dir.Path, body);
        var pkg = Pkg("forwarder", "https://h/nvngx.dll_dlssnr-0.2.1.dll", body);

        await store.EnsureAsync(pkg, null, default);

        var path = store.PathFor(pkg);
        Assert.True(File.Exists(path));
        Assert.Equal("nvngx.dll_dlssnr-0.2.1.dll", Path.GetFileName(path));
        Assert.Equal("forwarder-bytes", File.ReadAllText(path));
    }

    [Fact]
    public async Task Second_Ensure_does_not_download_again()
    {
        using var dir = new TempDir();
        var body = MakeZip(("a.txt", "x"));
        var (store, handler) = Make(dir.Path, body);
        var pkg = Pkg("amdnr", "https://h/a.zip", body);

        await store.EnsureAsync(pkg, null, default);
        await store.EnsureAsync(pkg, null, default);

        Assert.Equal(1, handler.Calls);
        Assert.True(store.Has(pkg));
    }

    [Fact]
    public async Task An_interrupted_extraction_is_not_reported_as_present()
    {
        using var dir = new TempDir();
        var body = MakeZip(("a.txt", "x"));
        var (store, _) = Make(dir.Path, body);
        var pkg = Pkg("amdnr", "https://h/a.zip", body);

        // Simulate a crash mid-extract: content exists, completion marker does not.
        Directory.CreateDirectory(store.PathFor(pkg));
        File.WriteAllText(Path.Combine(store.PathFor(pkg), "a.txt"), "partial");

        Assert.False(store.Has(pkg));

        await store.EnsureAsync(pkg, null, default);

        Assert.True(store.Has(pkg));
        Assert.Equal("x", File.ReadAllText(Path.Combine(store.PathFor(pkg), "a.txt")));
    }

    [Fact]
    public async Task The_completion_marker_is_not_stored_inside_the_directory_handed_to_the_installer()
    {
        // PathFor's directory is copied wholesale into a game folder by the installer.
        // Anything bookkeeping-related living in there would be copied into every game.
        using var dir = new TempDir();
        var body = MakeZip(("OptiScaler.dll", "x"));
        var (store, _) = Make(dir.Path, body);
        var pkg = Pkg("amdnr", "https://h/a.zip", body);

        await store.EnsureAsync(pkg, null, default);

        Assert.Equal(
            ["OptiScaler.dll"],
            Directory.EnumerateFileSystemEntries(store.PathFor(pkg), "*", SearchOption.AllDirectories)
                .Select(Path.GetFileName)
                .Order());
    }

    [Fact]
    public void A_marker_whose_payload_was_deleted_does_not_report_the_package_as_present()
    {
        using var dir = new TempDir();
        var body = MakeZip(("a.txt", "x"));
        var (store, _) = Make(dir.Path, body);
        var pkg = Pkg("amdnr", "https://h/a.zip", body);

        // Marker present, payload gone — e.g. the user cleared the store by hand.
        Directory.CreateDirectory(Path.Combine(dir.Path, pkg.Id));
        File.WriteAllText(Path.Combine(dir.Path, pkg.Id, pkg.Version + ".complete"), pkg.Sha256);

        Assert.False(store.Has(pkg));
    }

    [Fact]
    public async Task A_rebuilt_file_published_under_the_same_version_is_downloaded_again()
    {
        // The forwarder's version is typed by hand, not derived from its bytes. A rebuild
        // published under the old version would otherwise never reach anyone who has that
        // version cached: every INSTALL and REPAIR would put the old file back, and Doctor
        // would call it outdated forever.
        using var dir = new TempDir();
        var old = "forwarder built 19 September"u8.ToArray();
        var rebuilt = "forwarder built 23 September"u8.ToArray();

        var (store, _) = Make(dir.Path, old);
        await store.EnsureAsync(Pkg("forwarder", "https://h/nvngx.dll_dlssnr.dll", old), null, default);

        var current = Pkg("forwarder", "https://h/nvngx.dll_dlssnr.dll", rebuilt);
        var (again, handler) = Make(dir.Path, rebuilt);

        Assert.False(again.Has(current));
        await again.EnsureAsync(current, null, default);

        Assert.Equal(1, handler.Calls);
        Assert.Equal("forwarder built 23 September", File.ReadAllText(again.PathFor(current)));
        Assert.True(again.Has(current));
    }

    [Fact]
    public async Task A_file_package_whose_url_does_not_name_a_plain_file_is_never_stored()
    {
        // The parser drops such a package; this is the store refusing it on its own, before
        // anything is downloaded or deleted. Joined onto the version folder, "C:version.dll"
        // is a path relative to the current directory on C:, not a file inside the store.
        using var dir = new TempDir();
        var body = "forwarder-bytes"u8.ToArray();
        var (store, handler) = Make(dir.Path, body);
        var pkg = Pkg("forwarder", "https://h/C:version.dll", body);

        Assert.Throws<InvalidOperationException>(() => store.PathFor(pkg));
        Assert.False(store.Has(pkg));
        await Assert.ThrowsAsync<InvalidOperationException>(() => store.EnsureAsync(pkg, null, default));

        Assert.Equal(0, handler.Calls);
        Assert.False(Directory.Exists(Path.Combine(dir.Path, "forwarder")));
    }

    [Fact]
    public async Task A_download_that_fails_does_not_leave_the_old_marker_beside_an_emptied_folder()
    {
        // The rebuilt package under the same version fails to download, after its old copy was
        // cleared out. Should the manifest then go back to the old hash — a reverted upload — a
        // marker still naming it would call the empty folder that package, and every INSTALL
        // would place no OptiScaler at all.
        using var dir = new TempDir();
        var first = MakeZip(("OptiScaler.dll", "first"));
        var (store, _) = Make(dir.Path, first);
        var original = Pkg("amdnr", "https://h/a.zip", first);
        await store.EnsureAsync(original, null, default);

        var rebuilt = Pkg("amdnr", "https://h/a.zip", MakeZip(("OptiScaler.dll", "rebuilt")));
        var (broken, _) = Make(dir.Path, "not the rebuilt zip"u8.ToArray());
        await Assert.ThrowsAsync<HashMismatchException>(() => broken.EnsureAsync(rebuilt, null, default));

        Assert.False(broken.Has(original));
    }

    [Fact]
    public async Task Different_versions_of_one_package_coexist()
    {
        using var dir = new TempDir();
        var body = MakeZip(("a.txt", "x"));
        var (store, _) = Make(dir.Path, body);
        var v1 = Pkg("amdnr", "https://h/a.zip", body);
        var v2 = v1 with { Version = "2.0.0" };

        await store.EnsureAsync(v1, null, default);
        await store.EnsureAsync(v2, null, default);

        Assert.True(store.Has(v1));
        Assert.True(store.Has(v2));
        Assert.NotEqual(store.PathFor(v1), store.PathFor(v2));
    }
}
