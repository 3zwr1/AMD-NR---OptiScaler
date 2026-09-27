// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Install;

public static class PayloadNames
{
    /// <summary>The six valid proxy names, in recommendation order.
    /// dinput8.dll is never valid for this build.</summary>
    public static IReadOnlyList<string> ProxyNames { get; } =
    [
        "dxgi.dll", "d3d12.dll", "winmm.dll", "version.dll", "dbghelp.dll", "d3d11.dll",
    ];

    /// <summary>Whether a name is one the launcher installs OptiScaler under. A proxy name is
    /// joined onto the game folder, so this is also what keeps a manifest from pointing the
    /// install anywhere else: "..\dxgi.dll" or a rooted path is not on the list.</summary>
    public static bool IsProxyName(string? name)
        => name is not null && ProxyNames.Contains(name, StringComparer.OrdinalIgnoreCase);

    public const string OptiScalerDll = "OptiScaler.dll";
    public const string OptiScalerIni = "OptiScaler.ini";
    public const string Forwarder = "nvngx.dll_dlssnr.dll";
    public const string Weights = "dlssnr_on_amd_weights.bin";

    /// <summary>The author runtime's own config. User-owned: never written, never deleted.</summary>
    public const string AuthorIni = "dlssnr_on_amd.ini";

    /// <summary>The archive's own paperwork, which is not part of the mod: every Markdown file,
    /// LICENSE and SHA256SUMS.txt — at the package root only, because a Licenses\ folder inside
    /// the payload is something the mod does ship. AMDNR 0.3.2 alone carries a CHANGELOG and a
    /// README per language. Placing any of them would silently overwrite a game's own file of
    /// the same name sitting beside its exe — and games do ship those — and uninstall would then
    /// delete it, because by that point its hash matches what we recorded.</summary>
    public static bool IsPackageMetadata(string relativePath)
    {
        if (relativePath.IndexOfAny(['\\', '/']) >= 0) return false;

        return relativePath.EndsWith(".md", StringComparison.OrdinalIgnoreCase)
               || string.Equals(relativePath, "LICENSE", StringComparison.OrdinalIgnoreCase)
               || string.Equals(relativePath, "SHA256SUMS.txt", StringComparison.OrdinalIgnoreCase);
    }

    public static IReadOnlyList<string> PassDlls { get; } =
    [
        "dlssnr_amd_pass1.dll", "dlssnr_amd_pass2.dll", "dlssnr_amd_pass3.dll",
    ];

    public static IReadOnlyList<string> LogFiles { get; } =
    [
        "OptiScaler.log", "amd_bridge.log", "amd_presr.log", "dlssnr_on_amd.log",
    ];
}
