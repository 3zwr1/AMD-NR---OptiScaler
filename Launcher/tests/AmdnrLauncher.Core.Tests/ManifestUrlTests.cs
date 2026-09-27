// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.Core.Tests;

public class ManifestUrlTests
{
    [Theory]
    [InlineData("https://example.invalid/amdnr/manifest.json")]
    [InlineData("http://example.invalid/amdnr/manifest.json")]
    public void Accepts_what_the_downloader_can_actually_fetch(string url)
    {
        Assert.True(ManifestUrl.IsSupported(url));
    }

    [Theory]
    [InlineData("file:///C:/manifest.json")]
    [InlineData(@"C:\manifest.json")]
    [InlineData("ftp://example.invalid/manifest.json")]
    [InlineData("manifest.json")]
    [InlineData("")]
    [InlineData(null)]
    public void Rejects_an_address_that_would_brick_the_launcher(string? url)
    {
        // The setting is user-editable and persisted. HttpClient throws NotSupportedException
        // for an unsupported scheme — a file:// URL, which spec open item 2 contemplates but
        // nothing implements yet, is exactly that — and once it is saved the launcher crashes
        // on every startup, before the user can reach Settings to undo it.
        Assert.False(ManifestUrl.IsSupported(url));
    }
}
