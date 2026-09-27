// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Diagnostics;

/// <summary>BuildName is the name of the build the record was installed from, as the manifest
/// shows it: what REPAIR / UPDATE puts back, whatever build a new install would choose.
/// AvailableVersion is the manifest's version of that build when it differs from the installed
/// one — different, not newer: an owner who rolls a build back is offered it too.
/// AvailableRuntimeVersion is the version of the runtime the manifest now lists first for this
/// card, when the game was installed with another: the way a newer runtime reaches players
/// without a new launcher.</summary>
public sealed record DoctorReport(
    IReadOnlyList<DoctorFinding> Findings,
    string? BuildStamp,
    bool UpdateAvailable,
    string? BuildName = null,
    string? AvailableVersion = null,
    string? AvailableRuntimeVersion = null)
{
    public bool IsHealthy => Findings.All(f => f.Severity != DoctorSeverity.Error);
}

public static class Doctor
{
    /// <param name="generation">The GPU generation the runtime is chosen for — rdna4, rdna3 —
    /// or null when it is not known, in which case no runtime change is offered: a runtime that
    /// was not meant for the card is worse than an old one.</param>
    /// <param name="preferredRuntimeId">The runtime package the user picked in the chooser, when
    /// any: a game on it is left alone whatever the manifest lists first for the card, and a game
    /// on another runtime is offered the pick. Ignored when the build does not list it for the card.</param>
    public static DoctorReport Run(InstallRecord record, Manifest manifest, string? generation = null,
        string? preferredRuntimeId = null)
    {
        var findings = new List<DoctorFinding>();
        var game = record.ExeDirectory;
        var updateAvailable = false;
        string? availableVersion = null;

        // Every package is looked up leniently. Doctor runs for each installed game on every
        // scan, and the manifest it runs against may be a stale cache or a hand-edited file —
        // Package() throws KeyNotFoundException for a missing id, and nothing between here and
        // the UI catches it. A manifest we cannot check against is a finding, not a crash.
        //
        // The record names the build it came from, and only that build's package says whether
        // this install is current: a newer AMDNR is not an update for someone running
        // TheAutomatic's build, and offering it would install a build the user did not choose.
        var source = manifest.TrySource(record.Source);
        var modPackage = source is null ? null : manifest.TryPackage(source.Package);
        var buildName = source?.Name ?? record.Source;

        // Null versions are what the installer writes for parts it did not place. Checking for
        // them would report as missing the files the user chose not to install.
        var hasRuntime = record.RuntimeVersion is not null;
        var hasForwarder = record.ForwarderVersion is not null;
        var forwarderId = source?.Forwarder ?? "forwarder";

        // The package the record says it placed — runtime-033 on one machine, runtime-040 on
        // another — never the one the manifest would pick for this GPU today, and never a
        // fixed name: a manifest with two runtime packages has no package called "runtime",
        // and looking that up would call every new install unverifiable. A record from before
        // runtime packages had names placed the one there was.
        var runtimeId = record.RuntimeId ?? InstallRecordStore.LegacyRuntimeId;
        var runtimePackage = hasRuntime ? manifest.TryPackage(runtimeId) : null;
        var forwarderPackage = hasForwarder ? manifest.TryPackage(forwarderId) : null;

        var missingPackages = new[]
            {
                (source is null ? record.Source : source.Package, modPackage is null),
                (runtimeId, hasRuntime && runtimePackage is null),
                (forwarderId, hasForwarder && forwarderPackage is null),
            }
            .Where(p => p.Item2)
            .Select(p => p.Item1)
            .ToList();

        if (missingPackages.Count > 0)
        {
            findings.Add(new DoctorFinding("manifest.incomplete", DoctorSeverity.Warning,
                $"The update manifest does not list {string.Join(" or ", missingPackages)}, so " +
                "part of this install could not be checked.", null));
        }

        // The record is written before the first payload file lands and marked finished only
        // after the last one. One that survives still unfinished means the install died
        // part-way with no chance to roll back — a power cut, a killed process — so this folder
        // holds an unknown fraction of the payload. Everything below reports symptoms of that;
        // this says what actually happened.
        if (!record.Completed)
        {
            findings.Add(new DoctorFinding("install.incomplete", DoctorSeverity.Error,
                $"This install did not finish, so the game folder holds only part of {buildName}. " +
                "Uninstall to take it back out, or Repair to put the rest in.", "Repair"));
        }

        // --- proxy -------------------------------------------------------------
        var proxyPath = Path.Combine(game, record.Proxy);
        var recordedProxy = record.Files.FirstOrDefault(f =>
            string.Equals(f.RelativePath, record.Proxy, StringComparison.OrdinalIgnoreCase));

        if (!File.Exists(proxyPath))
        {
            findings.Add(new DoctorFinding("proxy.missing", DoctorSeverity.Error,
                $"{record.Proxy} is missing. The mod will not load.", "Repair"));
        }
        else if (recordedProxy is not null)
        {
            var actual = Hashing.TrySha256OfFile(proxyPath);

            if (actual is null)
                findings.Add(Unreadable(record.Proxy));
            else if (!Hashing.Matches(actual, recordedProxy.Sha256))
            {
                // Compared against what this launcher placed, never against the package hash:
                // the manifest hash covers the zip, this file is one entry inside it.
                findings.Add(new DoctorFinding("proxy.replaced", DoctorSeverity.Error,
                    $"{record.Proxy} is not the file this launcher installed. Something replaced it.",
                    "Repair"));
            }
        }

        // Both sides are publisher-authored: the manifest states what is current, the record
        // states what was installed. Different is the whole test. Package versions are whatever
        // their authors call them — TheAutomatic's 1.9.1-alpha is not a version any parser
        // agrees on — and a comparison that cannot read one would never offer it.
        if (modPackage is not null &&
            !string.Equals(modPackage.Version, record.AmdnrVersion, StringComparison.OrdinalIgnoreCase))
        {
            updateAvailable = true;
            availableVersion = modPackage.Version;
            findings.Add(new DoctorFinding("proxy.outdated", DoctorSeverity.Warning,
                $"Installed {buildName} {record.AmdnrVersion}; {modPackage.Version} is available.",
                "Update"));
        }

        // --- the runtime the card should have now -----------------------------
        // The manifest's order for a generation is the publisher's word on which runtime a card
        // gets, and reordering it is how a newer runtime reaches players without a new launcher.
        // A game installed while another runtime was first is offered the move. Not without the
        // generation — a runtime that was not meant for the card is worse than an old one — not
        // without a runtime installed (No at the chooser, or the lmxxf path), and not on a card
        // the build names no runtime for. REPAIR / UPDATE installs whatever is first today.
        string? availableRuntimeVersion = null;
        if (hasRuntime && generation is not null
            && source?.RuntimePackageFor(generation, preferredRuntimeId) is { } wanted
            && !string.Equals(wanted, runtimeId, StringComparison.OrdinalIgnoreCase)
            && manifest.TryPackage(wanted) is { } wantedPackage)
        {
            updateAvailable = true;
            availableRuntimeVersion = wantedPackage.Version;
            findings.Add(new DoctorFinding("runtime.outdated", DoctorSeverity.Warning,
                $"Installed DLSSNR AMD files {record.RuntimeVersion}; {wantedPackage.Version} is now the runtime for this card.",
                "Update"));
        }

        if (hasRuntime)
            CheckRuntime(game, buildName, runtimePackage, findings);
        else if (source?.RuntimeRequired == true)
        {
            // Not an error: the build loads and the game runs. But the one thing the user
            // installed it for is off, and without this the row would read as healthy.
            findings.Add(new DoctorFinding("runtime.notInstalled", DoctorSeverity.Warning,
                "The DLSSNR AMD files are not installed, so Neural Rendering cannot run with this build.",
                null));
        }

        if (hasForwarder)
            CheckForwarder(game, forwarderPackage, findings);

        if (!Directory.Exists(Path.Combine(game, "OptiScaler")))
        {
            findings.Add(new DoctorFinding("deps.missing", DoctorSeverity.Error,
                "The OptiScaler support folder is missing.", "Repair"));
        }

        // --- double injection --------------------------------------------------
        var others = PayloadNames.ProxyNames
            .Where(n => !string.Equals(n, record.Proxy, StringComparison.OrdinalIgnoreCase))
            .Where(n => File.Exists(Path.Combine(game, n)))
            .ToList();

        if (others.Count > 0)
        {
            findings.Add(new DoctorFinding("doubleInjection", DoctorSeverity.Warning,
                $"{string.Join(", ", others)} also present — two mods may load at once.",
                "Move aside"));
        }

        // --- RE Engine without REFramework ------------------------------------
        // The install is fine and the game closes itself 15-60 s after launch all the same, which
        // no other check here would explain; the report names the one file that fixes it.
        if (ReEngine.MissingReframeworkWarning(game) is { } reframework)
            findings.Add(new DoctorFinding("reframework.missing", DoctorSeverity.Warning, reframework, "Install REFramework"));

        // --- version compatibility --------------------------------------------
        // Without a runtime there is nothing for the build to be incompatible with.
        var rule = manifest.Compatibility.FirstOrDefault(c => c.Amdnr == record.AmdnrVersion);
        if (record.RuntimeVersion is { } runtimeVersion && rule is not null && !rule.Runtime.Contains(runtimeVersion))
        {
            findings.Add(new DoctorFinding("compat.mismatch", DoctorSeverity.Error,
                $"{buildName} {record.AmdnrVersion} does not work with runtime {record.RuntimeVersion}.",
                "Update"));
        }

        var ini = Path.Combine(game, PayloadNames.OptiScalerIni);
        findings.AddRange(IniHealth.Check(ini));

        // Only with the runtime in place is there something better for the ini to select.
        if (hasRuntime)
            findings.AddRange(IniHealth.CheckLmxxfModels(ini));

        return new DoctorReport(findings, ReadBuildStamp(game), updateAvailable, buildName, availableVersion,
            availableRuntimeVersion);
    }

    private static void CheckRuntime(
        string game, string buildName, PackageInfo? runtimePackage, List<DoctorFinding> findings)
    {
        // --- runtime passes ----------------------------------------------------
        var accepted = runtimePackage?.PassSha256 ?? [];
        var passHashes = new List<string>();

        // Said once, before any pass DLL is looked at. This is the check that exists to catch
        // spec §2's "Private AMD runtime hash mismatch", the most common real breakage — and a
        // manifest that omits passSha256, or misspells it (ManifestParser drops unknown members
        // by design, which is the forward-compatibility rule), made every pass DLL pass
        // unconditionally with no indication. Two harmless-looking behaviours composing into a
        // Doctor that reports everything is fine when it cannot check anything.
        if (accepted.Count == 0)
        {
            findings.Add(new DoctorFinding("pass.unverifiable", DoctorSeverity.Warning,
                "The update manifest does not say which runtime builds are accepted, so the " +
                "runtime build in this game could not be verified. If the game fails with " +
                "'Private AMD runtime hash mismatch', this is the check that could not warn you.",
                null));
        }

        foreach (var pass in PayloadNames.PassDlls)
        {
            var path = Path.Combine(game, pass);
            if (!File.Exists(path))
            {
                findings.Add(new DoctorFinding("pass.missing", DoctorSeverity.Error,
                    $"{pass} is missing.", "Repair"));
                continue;
            }

            var hash = Hashing.TrySha256OfFile(path);
            if (hash is null)
            {
                findings.Add(Unreadable(pass));
                continue;
            }

            passHashes.Add(hash);

            // The runtime accepts three builds; any one of them is fine.
            if (accepted.Count > 0 && !accepted.Any(a => Hashing.Matches(hash, a)))
            {
                findings.Add(new DoctorFinding("pass.unknownBuild", DoctorSeverity.Error,
                    $"{pass} is not a runtime build {buildName} accepts. The game would fail with " +
                    "'Private AMD runtime hash mismatch'.", "Repair"));
            }
        }

        if (passHashes.Distinct(StringComparer.OrdinalIgnoreCase).Count() > 1)
        {
            findings.Add(new DoctorFinding("pass.mixedVersions", DoctorSeverity.Error,
                "The three runtime files are not the same build. The game would fail with " +
                "'Mixed AMD runtime versions across passes'.", "Repair"));
        }

        // --- weights ------------------------------------------------------------
        var weights = Path.Combine(game, PayloadNames.Weights);
        if (FileFacts.TryLength(weights) == 0)
        {
            findings.Add(new DoctorFinding("weights.missing", DoctorSeverity.Error,
                $"{PayloadNames.Weights} is missing or empty.", "Repair"));
        }
    }

    private static void CheckForwarder(string game, PackageInfo? forwarderPackage, List<DoctorFinding> findings)
    {
        var forwarder = Path.Combine(game, PayloadNames.Forwarder);
        if (!File.Exists(forwarder))
        {
            findings.Add(new DoctorFinding("forwarder.stale", DoctorSeverity.Error,
                $"{PayloadNames.Forwarder} is missing.", "Repair"));
        }
        else
        {
            var hash = Hashing.TrySha256OfFile(forwarder);

            if (hash is null)
                findings.Add(Unreadable(PayloadNames.Forwarder));
            else if (forwarderPackage is not null && !Hashing.Matches(hash, forwarderPackage.Sha256))
            {
                findings.Add(new DoctorFinding("forwarder.stale", DoctorSeverity.Warning,
                    $"{PayloadNames.Forwarder} is an outdated build.", "Repair"));
            }
        }
    }

    /// <summary>The first line of OptiScaler.log carries version and build timestamp.</summary>
    public static string? ReadBuildStamp(string exeDirectory)
    {
        foreach (var candidate in new[]
                 {
                     Path.Combine(exeDirectory, "OptiScaler.log"),
                     Path.Combine(exeDirectory, "_storage_", "OptiScaler.log"),
                 })
        {
            if (!File.Exists(candidate)) continue;
            try
            {
                using var reader = new StreamReader(new FileStream(
                    candidate, FileMode.Open, FileAccess.Read, FileShare.ReadWrite));
                var line = reader.ReadLine();
                if (!string.IsNullOrWhiteSpace(line)) return line.Trim();
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException)
            {
                /* the game may hold the log open */
            }
        }
        return null;
    }

    /// <summary>Not proof of a fault — proof we could not check. The ordinary cause is the
    /// game still running, so it is a warning with an action, not an error.</summary>
    private static DoctorFinding Unreadable(string fileName) => new(
        "file.unreadable", DoctorSeverity.Warning,
        $"{fileName} could not be read, so it could not be checked. " +
        "If the game is running, close it and check again.",
        "Check again");
}
