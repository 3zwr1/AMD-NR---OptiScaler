// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Assets;

public static class ManifestUrl
{
    /// <summary>Whether the launcher can actually fetch a manifest from this address. It lives
    /// here rather than in the UI because the answer is a property of the downloader, not of the
    /// settings dialog: HttpClient serves http and https and throws NotSupportedException for
    /// anything else — including the file:// URL spec open item 2 contemplates but nothing
    /// implements yet. The setting is user-editable and persisted, so an address nothing can
    /// fetch is a state the launcher must not be able to enter at all: saved once, it crashes
    /// every startup before the user can get back into Settings to undo it.</summary>
    public static bool IsSupported(string? url)
        => Uri.TryCreate(url, UriKind.Absolute, out var uri)
           && (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps);
}
