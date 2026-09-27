// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

/// <summary>Lays out an asset store the way AssetStore would after a real download.
/// Shared by InstallerTests and LauncherServiceTests.</summary>
public sealed class StubAssetStore : IAssetStore
{
    public PackageInfo Amdnr { get; }
    public PackageInfo TheAutomatic { get; }
    public PackageInfo Runtime { get; }
    public PackageInfo Forwarder { get; }

    /// <summary>The folder TheAutomatic's archive wraps everything in. Their README does not
    /// say whether the zip nests its contents, so the fixture takes the harder case.</summary>
    public const string TheAutomaticTopFolder = "OptiScaler-AMD-PreSR-1.8.6-0.3.1";

    private readonly Dictionary<string, string> _paths = new();

    public StubAssetStore(string root)
    {
        Amdnr = new PackageInfo("amdnr", "0.2.1", "https://h/a.zip", "AA", 1, null);
        Forwarder = new PackageInfo("forwarder", "0.2.1", "https://h/nvngx.dll_dlssnr.dll", "CC", 1, null);

        var amdnrDir = Path.Combine(root, "amdnr");
        Directory.CreateDirectory(Path.Combine(amdnrDir, "OptiScaler", "plugins"));
        Directory.CreateDirectory(Path.Combine(amdnrDir, "Licenses"));
        File.WriteAllText(Path.Combine(amdnrDir, "OptiScaler.dll"), "optiscaler-payload");
        File.WriteAllText(Path.Combine(amdnrDir, "OptiScaler.ini"), "; packaged defaults");
        File.WriteAllText(Path.Combine(amdnrDir, "OptiScaler", "libxess.dll"), "xess");
        File.WriteAllText(Path.Combine(amdnrDir, "Licenses", "XeSS_LICENSE.txt"), "licence");
        _paths["amdnr"] = amdnrDir;

        TheAutomatic = new PackageInfo("theautomatic", "1.8.6-0.3.1", "https://h/t.zip", "DD", 1, null,
            Exclude: ["Setup.bat", "Setup.ps1", "tools/", "Uninstall_OptiScaler_NR.bat", "Uninstall_OptiScaler_NR.ps1"]);

        var automaticDir = Path.Combine(root, "theautomatic");
        var content = Path.Combine(automaticDir, TheAutomaticTopFolder);
        Directory.CreateDirectory(Path.Combine(content, "OptiScaler"));
        Directory.CreateDirectory(Path.Combine(content, "Licenses"));
        Directory.CreateDirectory(Path.Combine(content, "tools"));
        File.WriteAllText(Path.Combine(content, "OptiScaler.dll"), "presr-payload");
        File.WriteAllText(Path.Combine(content, "OptiScaler.ini"), "; presr defaults");
        File.WriteAllText(Path.Combine(content, "OptiScaler", "libxess.dll"), "xess");
        File.WriteAllText(Path.Combine(content, "Licenses", "OptiScaler_LICENSE.txt"), "gpl");
        File.WriteAllText(Path.Combine(content, "Setup.bat"), "@echo off");
        File.WriteAllText(Path.Combine(content, "Setup.ps1"), "Write-Host setup");
        File.WriteAllText(Path.Combine(content, "Uninstall_OptiScaler_NR.bat"), "@echo off");
        File.WriteAllText(Path.Combine(content, "Uninstall_OptiScaler_NR.ps1"), "Write-Host bye");
        File.WriteAllText(Path.Combine(content, "tools", "x.ps1"), "Write-Host tool");
        File.WriteAllText(Path.Combine(content, "README.en.md"), "their readme");
        _paths["theautomatic"] = automaticDir;

        var runtimeDir = Path.Combine(root, "runtime");
        Directory.CreateDirectory(runtimeDir);
        string? passHash = null;
        foreach (var pass in PayloadNames.PassDlls)
        {
            var passPath = Path.Combine(runtimeDir, pass);
            File.WriteAllText(passPath, "pass-bytes");
            passHash ??= Hashing.Sha256OfFile(passPath);
        }
        File.WriteAllText(Path.Combine(runtimeDir, PayloadNames.Weights), "weights-bytes");
        _paths["runtime"] = runtimeDir;

        // Doctor validates each pass dll's hash against this list, so it must be the real
        // hash of the bytes written above rather than an arbitrary placeholder — a fixture
        // shared with tests that run Doctor (LauncherServiceTests) needs it to actually match.
        Runtime = new PackageInfo("runtime", "0.3.1", "https://h/r.zip", "BB", 1, [passHash!]);
        _runtimeDir = runtimeDir;

        var forwarder = Path.Combine(root, "forwarder", PayloadNames.Forwarder);
        Directory.CreateDirectory(Path.GetDirectoryName(forwarder)!);
        File.WriteAllText(forwarder, "forwarder-bytes");
        _paths["forwarder"] = forwarder;
    }

    /// <summary>The two builds the manifest offers, shaped as the real manifest describes them.</summary>
    public static IReadOnlyList<SourceInfo> Sources { get; } =
    [
        new SourceInfo("amdnr", "AMDNR", "3zwr1", "AMDNR summary", "https://h/amdnr",
            Package: "amdnr", Forwarder: "forwarder", RuntimeRequired: false,
            RuntimeNote: "Needed on RX 7000."),
        new SourceInfo("theautomatic", "OptiScaler AMD pre-SR", "TheAutomatic", "pre-SR summary",
            "https://h/theautomatic", Package: "theautomatic", Forwarder: null, RuntimeRequired: true,
            RuntimeNote: "Required.",
            IniWhenRuntime: [new IniSetting("DlssNr", "Enabled", "true"), new IniSetting("DlssNr", "NrBackend", "daniel")]),
    ];

    private readonly string _runtimeDir;

    /// <summary>The same runtime files under another package id and version — runtime-033 and
    /// runtime-040 as the 0.3.3.1 manifest names them. Its passSha256 is the real hash of the
    /// fixture's pass DLLs, so Doctor accepts an install made from it.</summary>
    public PackageInfo RuntimeNamed(string id, string version)
    {
        _paths[id] = _runtimeDir;
        return Runtime with { Id = id, Version = version, Url = $"https://h/{id}.zip" };
    }

    /// <summary>Where the fixture's TheAutomatic archive keeps its OptiScaler.ini.</summary>
    public string TheAutomaticPackagedIni
        => Path.Combine(_paths["theautomatic"], TheAutomaticTopFolder, PayloadNames.OptiScalerIni);

    public bool Has(PackageInfo p) => true;
    public string PathFor(PackageInfo p) => _paths[p.Id];
    public Task EnsureAsync(PackageInfo p, IProgress<DownloadProgress>? _, CancellationToken __)
        => Task.CompletedTask;
}
