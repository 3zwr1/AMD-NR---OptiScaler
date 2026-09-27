// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Net.Http;

namespace AmdnrLauncher.Core.Assets;

/// <summary>The one HttpClient the launcher downloads with. It says who it is: HttpClient sends
/// no User-Agent at all by default, and the host the owner keeps the forwarder on drops such
/// requests before answering — every AMDNR install failed at the forwarder with "An error
/// occurred while sending the request" until the client introduced itself. GitHub is
/// indifferent; some hosts are not.</summary>
public static class LauncherHttp
{
    public const string Project = "https://github.com/3zwr1/AMD-NR---OptiScaler";

    public static string UserAgent(string version) => $"AMD-NR-Launcher/{version} (+{Project})";

    public static HttpClient Create(string version)
    {
        var client = new HttpClient();
        client.DefaultRequestHeaders.UserAgent.ParseAdd(UserAgent(version));
        return client;
    }
}
