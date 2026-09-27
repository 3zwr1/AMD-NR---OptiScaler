// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Diagnostics;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;
using AmdnrLauncher.Core.State;

namespace AmdnrLauncher.Core;

/// <summary>Which build the user picked, and whether they want the DLSSNR AMD files with it.</summary>
/// <summary>Which build, whether to fetch the DLSSNR AMD files with it, and — when the user
/// picked one in the chooser — which runtime package. Null <paramref name="RuntimeId"/> means the
/// one the manifest lists first for the card, which moves when the owner reorders the manifest;
/// a pick stays until the user changes it.</summary>
public sealed record InstallChoice(string SourceId, bool IncludeRuntime, string? RuntimeId = null);

public enum GameStatus
{
    NotInstalled,
    Installed,
    UpdateAvailable,
    Problem,
    Unsupported,

    /// <summary>No record of ours, but an OptiScaler build already loads through one of the
    /// proxy names: somebody installed it by hand, or with another tool.</summary>
    ExistingInstall,
}

/// <summary><paramref name="RecommendedProxy"/> is the name in use when something is installed,
/// and the name INSTALL would use when nothing is; <paramref name="ProxyReason"/> says why, in
/// ProxyPlanner's words, and is empty when no exe folder was found. <paramref name="OccupiedProxies"/>
/// are the names some other file already holds in the folder, which the stage greys out.</summary>
public sealed record GameEntry(
    GameCandidate Candidate,
    string? ExeDirectory,
    string? ExeName,
    GameStatus Status,
    InstallRecord? Record,
    DoctorReport? Report,
    GateResult? Gate,
    string RecommendedProxy,
    string ProxyReason,
    IReadOnlyList<string> OccupiedProxies);

/// <summary>The refreshed row, plus what the uninstall could not finish. The two travel
/// together because the row alone cannot say "gone, except for these three files".</summary>
public sealed record UninstallOutcome(GameEntry Entry, UninstallResult Result);

/// <summary>The refreshed row after TRY NEXT PROXY, and whether this press was the one that
/// found every name tried and began the round again from the first — which the status line
/// has to say, because the DLL just moved to a name the user has seen fail before.</summary>
public sealed record ProxySwitchOutcome(GameEntry Entry, bool StartedOver);

public sealed class LauncherService(
    Manifest manifest,
    IAssetStore store,
    Installer installer,
    IReadOnlyList<IGameScanner> scanners,
    GpuInfo gpu)
{
    public Manifest Manifest => manifest;

    /// <summary>The graphics card the runtime is chosen for. Detected once, at startup: a
    /// build's runtime list is read with its generation, and the same one for every game.</summary>
    public GpuInfo Gpu => gpu;

    /// <summary>The store the service was built with. Exposed so callers that need a path into
    /// the downloaded payload use this one rather than constructing a second store pointed at
    /// the same directory.</summary>
    public IAssetStore Assets => store;

    public IReadOnlyList<GameEntry> Scan()
    {
        // Dropped before Describe, not after: resolving a tool's folder walks its files for
        // nothing, and SteamVR's install is large.
        var candidates = scanners
            .SelectMany(s => s.Scan())
            .Where(c => !HiddenApps.IsHidden(c, manifest.HiddenSteamAppIds))
            .ToList();

        foreach (var manual in InstalledGamesDb.Load().Where(Directory.Exists))
            candidates.Add(new GameCandidate(new DirectoryInfo(manual).Name, GameStore.Manual, manual));

        return candidates
            .DistinctBy(c => c.InstallRoot, StringComparer.OrdinalIgnoreCase)
            .Select(Describe)
            .OrderBy(e => e.Candidate.Name, StringComparer.CurrentCultureIgnoreCase)
            .ToList();
    }

    public GameEntry AddManual(string exeDirectory)
    {
        InstalledGamesDb.AddManual(exeDirectory);
        return Describe(new GameCandidate(new DirectoryInfo(exeDirectory).Name, GameStore.Manual, exeDirectory));
    }

    public GameEntry Refresh(GameEntry entry) => Describe(entry.Candidate);

    private GameEntry Describe(GameCandidate candidate)
    {
        // A manually added folder is already the exe directory; a store root is not. A store that
        // names the exe has already answered the question, when its answer is the game itself.
        var resolution = candidate.Store == GameStore.Manual
            ? ResolveManual(candidate.InstallRoot)
            : ExeDirectoryResolver.FromHint(candidate.InstallRoot, candidate.ExeHint)
              ?? ExeDirectoryResolver.Resolve(candidate.InstallRoot);

        var exeDirectory = resolution.ExeDirectory;
        if (exeDirectory is null)
        {
            // WindowsApps is TrustedInstaller-owned and typically cannot even be enumerated, so
            // exe resolution routinely fails there before ever reaching the exe directory. Check
            // the install root itself so that case still surfaces as Unsupported, not a silent
            // "not installed yet" for a target that can never be actioned. A Ricochet game is the
            // same: "Not installed" would enable INSTALL, whose answer is "Add game folder…" —
            // and that leads to a subfolder of a game no consent can make safe.
            var rootGate = InstallGate.Evaluate(candidate.InstallRoot);
            if (IsPermanentlyBlocked(rootGate))
            {
                return new GameEntry(candidate, null, null, GameStatus.Unsupported,
                    null, null, rootGate, PayloadNames.ProxyNames[0], "", []);
            }

            return new GameEntry(candidate, null, null, GameStatus.NotInstalled,
                null, null, null, PayloadNames.ProxyNames[0], "", []);
        }

        var gate = InstallGate.Evaluate(exeDirectory, candidate.InstallRoot);
        var record = InstallRecordStore.Load(exeDirectory);

        // Without a record of ours, an OptiScaler under a proxy name is somebody's own install.
        // "Not installed" would be wrong about the game, and the name it loads under is the one
        // to install under: ProxyPlanner ranks it first and says so.
        var existing = record is null ? ProxySlotProbe.FindOptiScalerProxy(exeDirectory) : null;
        var choice = ProxyPlanner.Explain(resolution.ExeName ?? "", exeDirectory, manifest);
        var occupied = ProxySlotProbe.FindOccupied(exeDirectory);

        // Installed, the name in use is the one to show, whatever the planner would pick for an
        // empty folder today. The planner's reason is kept when it explains that very name for
        // a reason of its own (the manifest, the exe's imports) and otherwise replaced by the one
        // thing that is certainly true of the record. "Your earlier install used dxgi.dll" is
        // the planner recognising our own DLL: right, but the words for a hand-made install found
        // without a record, and odd a moment after INSTALL — so with a record it is said as ours.
        var recommended = record?.Proxy ?? choice.Name;
        var reason =
            record is null ? choice.Reason
            : !string.Equals(record.Proxy, choice.Name, StringComparison.OrdinalIgnoreCase)
                ? $"this install was placed as {record.Proxy}"
            : choice.FromEarlierInstall ? $"installed here as {record.Proxy}"
            : choice.Reason;

        // Both are permanent: nothing the user can do in the folder makes either go away, so the
        // row says so rather than offering an INSTALL that can only fail.
        if (IsPermanentlyBlocked(gate))
        {
            return new GameEntry(candidate, exeDirectory, resolution.ExeName, GameStatus.Unsupported,
                record, null, gate, recommended, reason, occupied);
        }

        if (record is null)
        {
            return new GameEntry(candidate, exeDirectory, resolution.ExeName,
                existing is null ? GameStatus.NotInstalled : GameStatus.ExistingInstall,
                null, null, gate, recommended, reason, occupied);
        }

        var report = Doctor.Run(record, manifest, gpu.Generation, PreferredRuntimeId);
        var status = !report.IsHealthy ? GameStatus.Problem
            : report.UpdateAvailable ? GameStatus.UpdateAvailable
            : GameStatus.Installed;

        return new GameEntry(candidate, exeDirectory, resolution.ExeName, status,
            record, report, gate, recommended, reason, occupied);
    }

    /// <summary>WindowsApps and Ricochet: nothing done in the folder, and no answer to a prompt,
    /// makes either go away.</summary>
    private static bool IsPermanentlyBlocked(GateResult gate)
        => gate.Findings.Any(f => f.Code is GateCode.WindowsAppsRejected or GateCode.AntiCheatBlocked);

    /// <summary>A manually added folder is already the exe directory, so it skips resolution —
    /// but it can still vanish between being added and being refreshed: deleted, renamed, or on
    /// a drive that is no longer connected. Absorbing that here is the whole point of this
    /// facade; the UI should not need a try/catch around `Refresh`.</summary>
    private static ExeResolution ResolveManual(string exeDirectory)
    {
        if (!Directory.Exists(exeDirectory)) return new ExeResolution(null, null, []);

        try
        {
            var exe = Directory
                .EnumerateFiles(exeDirectory, "*.exe", new EnumerationOptions { IgnoreInaccessible = true })
                .Select(Path.GetFileName)
                .FirstOrDefault();

            return new ExeResolution(exeDirectory, exe, []);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return new ExeResolution(null, null, []);
        }
    }

    public async Task<GameEntry> InstallAsync(
        GameEntry entry,
        string proxy,
        InstallChoice choice,
        bool antiCheatAcknowledged,
        IProgress<InstallProgress>? progress,
        CancellationToken ct)
    {
        // Resolved before anything else so that a choice the manifest no longer offers fails
        // with nothing touched.
        var packages = Resolve(choice);

        // Naming the way out matters: the Install button stays enabled for this row, so without
        // it the user gets a statement of fact and no idea what to do about it.
        if (entry.ExeDirectory is null)
        {
            // Unless no folder would do: sending the user to pick a subfolder of a game that
            // can never be installed into is how a hard block turns into a bypass.
            var rootGate = InstallGate.Evaluate(entry.Candidate.InstallRoot);
            if (IsPermanentlyBlocked(rootGate))
            {
                throw new InvalidOperationException(string.Join(" ", rootGate.Findings
                    .Where(f => f.Code is GateCode.WindowsAppsRejected or GateCode.AntiCheatBlocked)
                    .Select(f => f.Message)));
            }

            throw new InvalidOperationException(
                "The folder holding the game executable could not be worked out. Use " +
                "\"Add game folder…\" and pick the folder that contains the game's .exe.");
        }

        var gate = InstallGate.Evaluate(entry.ExeDirectory, entry.Candidate.InstallRoot);
        if (gate.IsHardBlocked)
        {
            throw new InvalidOperationException(
                string.Join(" ", gate.Findings.Where(f => f.Blocking).Select(f => f.Message)));
        }

        var antiCheat = gate.Findings.Any(f => f.Code == GateCode.AntiCheatDetected);
        if (antiCheat && !antiCheatAcknowledged)
            throw new InvalidOperationException("Anti-cheat was detected and has not been acknowledged.");

        var conflicts = ConflictScanner
            .Scan(entry.ExeDirectory, proxy, packages.Forwarder?.Sha256)
            .Where(c => c.RecommendMoveAside)
            .ToList();

        await installer.InstallAsync(
            new InstallRequest(entry.ExeDirectory, entry.Candidate.Name, proxy,
                packages.Mod, packages.Runtime, packages.Forwarder, packages.Source.Id,
                conflicts, antiCheat && antiCheatAcknowledged, packages.Source.IniWhenRuntime),
            progress, ct);

        return Describe(entry.Candidate);
    }

    /// <summary>Everything an install of this choice needs downloaded, mod package first. The
    /// same resolution InstallAsync uses, so what is fetched up front is exactly what the
    /// install will ask for — and nothing it will not.</summary>
    public IReadOnlyList<PackageInfo> PackagesFor(InstallChoice choice)
    {
        var packages = Resolve(choice);

        List<PackageInfo> all = [packages.Mod];
        if (packages.Runtime is not null) all.Add(packages.Runtime);
        if (packages.Forwarder is not null) all.Add(packages.Forwarder);
        return all;
    }

    /// <summary>What an install of this row should put in. The saved choice is for games with
    /// nothing installed; Repair and Update on an installed row put back what the record says
    /// is there. Otherwise Update on a TheAutomatic row would silently switch the game to
    /// AMDNR, and Repair on a row installed without the runtime would place the ~110 MB the
    /// user declined.</summary>
    public static InstallChoice ChoiceFor(GameEntry entry, InstallChoice forNewInstall)
        => entry.Record is { } record
            ? new InstallChoice(record.Source, IncludeRuntime: record.RuntimeVersion is not null, forNewInstall.RuntimeId)
            : forNewInstall;

    /// <summary>The runtime package the user picked in the chooser, or null to follow the one
    /// the manifest lists first for the card. Set from the saved choice at start and whenever it
    /// changes: the Doctor measures every install against it, so a game on the picked runtime
    /// is not offered the card's default, and a game on another runtime is offered the pick.</summary>
    public string? PreferredRuntimeId { get; set; }

    private sealed record ResolvedChoice(
        SourceInfo Source, PackageInfo Mod, PackageInfo? Runtime, PackageInfo? Forwarder);

    /// <summary>The runtime only when the user asked for it, and then the one the build lists
    /// for this GPU's generation — none at all when the build lists runtimes and names none for
    /// it, because a runtime that was not meant for the card is worse than no runtime. The
    /// forwarder only when the build names one. A choice is saved against the manifest of the
    /// day, so the current one can have dropped the build or a package it names — said plainly,
    /// because the user can do something about it, and a KeyNotFoundException tells them
    /// nothing.</summary>
    private ResolvedChoice Resolve(InstallChoice choice)
    {
        var source = manifest.TrySource(choice.SourceId)
            ?? throw new InvalidOperationException(
                $"The build \"{choice.SourceId}\" is no longer offered. Open Settings and choose a build again.");

        var runtimeId = choice.IncludeRuntime ? source.RuntimePackageFor(gpu.Generation, choice.RuntimeId) : null;

        return new ResolvedChoice(
            source,
            Require(source.Package, source),
            runtimeId is null ? null : Require(runtimeId, source),
            source.Forwarder is null ? null : Require(source.Forwarder, source));
    }

    private PackageInfo Require(string packageId, SourceInfo source)
        => manifest.TryPackage(packageId)
           ?? throw new InvalidOperationException(
               $"The update manifest offers {source.Name} but does not list its \"{packageId}\" " +
               "download, so it cannot be installed right now. Try again after the next launcher update.");

    /// <summary>Puts back the OptiScaler.ini an install of this row's build would seed, and
    /// returns where the replaced one was kept (null when there was none). The seed, not the
    /// packaged file: for a build whose packaged ini selects a backend the launcher cannot
    /// supply, a reset to the packaged file would switch Neural Rendering off again.</summary>
    public string? ResetIni(GameEntry entry)
    {
        if (entry.Record is not { } record)
            throw new InvalidOperationException("Nothing is installed for this game yet.");

        var source = manifest.TrySource(record.Source)
            ?? throw new InvalidOperationException(
                $"The build \"{record.Source}\" is no longer offered, so its default OptiScaler.ini is not available.");

        var seed = installer.SeedIni(Require(source.Package, source), source.Id,
            record.RuntimeVersion is null ? null : source.IniWhenRuntime);

        return IniHealth.ResetToDefaults(Path.Combine(record.ExeDirectory, PayloadNames.OptiScalerIni), seed);
    }

    public async Task<UninstallOutcome> UninstallAsync(GameEntry entry, CancellationToken ct)
    {
        var result = entry.Record is null
            ? UninstallResult.Nothing
            : await installer.UninstallAsync(entry.Record, ct);

        return new UninstallOutcome(Describe(entry.Candidate), result);
    }

    /// <summary>Moves the installed DLL to the next name in the planner's order that this
    /// install has not been through, and notes the one it leaves in the record. Once every name
    /// has been tried the list is cleared and the DLL goes back to the first name the planner
    /// would pick now — not to a refusal. The owner met that refusal on Resident Evil Requiem,
    /// with the game left on d3d11.dll and the name that works there, dxgi.dll, out of reach.
    /// A name another file holds — ReShade's dxgi.dll, say — counts as tried for both steps:
    /// the installer would refuse to move over it, and refusing the same name on every press
    /// is the same dead end by another route.</summary>
    public async Task<ProxySwitchOutcome> TryNextProxyAsync(GameEntry entry, CancellationToken ct)
    {
        var record = RequireInstalled(entry);
        RefuseWhenRicochetGuards(entry, record);

        var exeName = entry.ExeName ?? "";
        var exeDirectory = entry.ExeDirectory ?? record.ExeDirectory;

        var next = ProxyPlanner.Next(record.Proxy, record.TriedProxies, exeName, exeDirectory, manifest);
        if (next is not null)
        {
            await installer.SwitchProxyAsync(record with { TriedProxies = Remembering(record) }, next, ct);
            return new ProxySwitchOutcome(Describe(entry.Candidate), StartedOver: false);
        }

        // The planner ranks our own DLL's name first, because it is an OptiScaler already
        // loading through a proxy name; the round starts again at the first free name after it.
        // Nothing free at all means every other name has a file under it, which no press can
        // change — said so, with what would.
        var again = ProxyPlanner.Next(record.Proxy, [], exeName, exeDirectory, manifest)
            ?? throw new InvalidOperationException(
                "Every other proxy name already has a file under it in the game folder, so there is " +
                "nothing to move the DLL to. Move one aside yourself before trying another name.");

        await installer.SwitchProxyAsync(record with { TriedProxies = [] }, again, ct);
        return new ProxySwitchOutcome(Describe(entry.Candidate), StartedOver: true);
    }

    /// <summary>Moves the installed DLL to the name the user chose on the stage, under the same
    /// guards as TRY NEXT PROXY, and counts the name it leaves as tried so TRY NEXT PROXY does
    /// not offer it straight back. Choosing the name already in use changes nothing.</summary>
    public async Task<GameEntry> SwitchProxyAsync(GameEntry entry, string proxy, CancellationToken ct)
    {
        var record = RequireInstalled(entry);
        if (string.Equals(proxy, record.Proxy, StringComparison.OrdinalIgnoreCase)) return Describe(entry.Candidate);

        RefuseWhenRicochetGuards(entry, record);

        await installer.SwitchProxyAsync(record with { TriedProxies = Remembering(record) }, proxy, ct);
        return Describe(entry.Candidate);
    }

    private static InstallRecord RequireInstalled(GameEntry entry)
        => entry.Record ?? throw new InvalidOperationException("Nothing is installed for this game yet.");

    /// <summary>Checked again here, not read off the entry: a game update can bring Ricochet
    /// into a folder installed before it, and a DLL moved to a name the game does load is an
    /// injection no consent can make safe.</summary>
    private static void RefuseWhenRicochetGuards(GameEntry entry, InstallRecord record)
    {
        var gate = InstallGate.Evaluate(entry.ExeDirectory ?? record.ExeDirectory, entry.Candidate.InstallRoot);
        if (gate.Findings.FirstOrDefault(f => f.Code == GateCode.AntiCheatBlocked) is { } blocked)
            throw new InvalidOperationException(blocked.Message);
    }

    /// <summary>The record's tried names with the one now in use added: the installer writes
    /// back whatever record it is handed, with the new proxy in it.</summary>
    private static IReadOnlyList<string> Remembering(InstallRecord record)
        => record.TriedProxies.Append(record.Proxy).Distinct(StringComparer.OrdinalIgnoreCase).ToList();
}
