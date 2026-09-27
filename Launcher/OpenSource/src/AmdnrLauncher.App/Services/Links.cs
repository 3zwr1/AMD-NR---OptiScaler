// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.ComponentModel;
using System.Diagnostics;

namespace AmdnrLauncher.App.Services;

/// <summary>Opens a web address in the user's browser — and only a web address. A build's
/// homepage comes from the manifest, and the shell runs whatever it is handed, a path to a
/// program included; so anything that is not http(s) is dropped here, before Process.Start.</summary>
public static class Links
{
    public static bool IsWeb(Uri uri)
        => uri.IsAbsoluteUri && (uri.Scheme == Uri.UriSchemeHttps || uri.Scheme == Uri.UriSchemeHttp);

    public static void Open(Uri uri)
    {
        if (!IsWeb(uri)) return;

        try
        {
            Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true });
        }
        // No browser registered, or a policy that blocks starting one. The address is on
        // screen or in the tooltip, so the user can still copy it by hand.
        catch (Win32Exception) { }
    }
}
