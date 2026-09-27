// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.Core.Install;

public sealed record InstallRequest(
    string ExeDirectory,
    string GameName,
    string Proxy,
    PackageInfo Mod,
    PackageInfo? Runtime,
    PackageInfo? Forwarder,
    string SourceId,
    IReadOnlyList<Conflict> ConflictsToMoveAside,
    bool AntiCheatAcknowledged,
    IReadOnlyList<IniSetting>? IniWhenRuntime = null);

public readonly record struct InstallProgress(string Stage, int FilesDone, int FilesTotal);

/// <summary>What an uninstall could not finish, in paths relative to the game folder.
/// LeftBehind is payload we could not remove because something holds it — the running game,
/// most often — and another Uninstall can finish the job. Changed is payload the user changed
/// after we placed it: theirs now, kept on purpose. NotRestored is the user's own files still
/// in AMDNR_backup, named by their path there, because something occupies where they came
/// from or the move back failed. All three are things the user has to be told about: a silent
/// partial success leaves them believing the folder is clean.
///
/// Superseded is different: an earlier OptiScaler build the install replaced, kept in
/// AMDNR_backup on purpose and named by its path there. It is not a failure, so it does not
/// make the uninstall incomplete — but the user still has to be told where it went.</summary>
public sealed record UninstallResult(
    IReadOnlyList<string> LeftBehind,
    IReadOnlyList<string> NotRestored,
    IReadOnlyList<string> Superseded,
    IReadOnlyList<string>? Changed = null)
{
    public IReadOnlyList<string> Changed { get; init; } = Changed ?? [];

    /// <summary>True when the record was kept because pressing Uninstall again, with the game
    /// closed, can get further: something of ours is still held, or a backup could not be put
    /// back or even read yet.</summary>
    public bool CanFinishLater { get; init; }

    public static UninstallResult Nothing { get; } = new([], [], []);

    public bool IsComplete => LeftBehind.Count == 0 && NotRestored.Count == 0 && Changed.Count == 0;
}

public sealed class Installer(IAssetStore store, IFilePlacer placer)
{
    internal const string BackupRootName = "AMDNR_backup";

    public async Task<InstallRecord> InstallAsync(
        InstallRequest request, IProgress<InstallProgress>? progress, CancellationToken ct)
    {
        RequireProxyName(request.Proxy);

        // An unplugged drive makes every backup look deleted, so the carry-forward below would
        // prune the record's only pointers to the user's own files, and a first install would
        // create a fresh game folder somewhere nothing will ever load it from. Checked before the
        // downloads, so nothing is fetched for an install that cannot happen.
        if (!CanList(request.ExeDirectory))
        {
            throw new IOException(
                "The game folder is not reachable (is its drive connected?). Nothing was changed.");
        }

        await store.EnsureAsync(request.Mod, null, ct);
        if (request.Runtime is not null) await store.EnsureAsync(request.Runtime, null, ct);
        if (request.Forwarder is not null) await store.EnsureAsync(request.Forwarder, null, ct);

        // Read before anything moves. A Repair rolled back to "no record at all" would take the
        // first install's backup pointers with it, stranding the user's own files.
        var previous = InstallRecordStore.Load(request.ExeDirectory);
        var bin = new BackupBin(request.ExeDirectory);
        var placed = new List<PlacedFile>();

        // Destinations that already held a file before this install reached them, and still do:
        // an earlier install's own, or somebody's byte-identical copy of the build. Neither went
        // into the backup, so a rollback that deleted them would delete the only copy — the
        // earlier build's proxy, or a hand-made install that loaded fine until INSTALL failed.
        var wasThere = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        // Outside the try so the rollback has the last one saved: a failed first install whose
        // backup survives keeps a record, and it is built from this one.
        var record = new InstallRecord(
            ExeDirectory: request.ExeDirectory,
            GameName: request.GameName,
            Proxy: request.Proxy,
            Source: request.SourceId,
            AmdnrVersion: request.Mod.Version,
            // Null is what tells Doctor not to look for files that were never placed.
            RuntimeVersion: request.Runtime?.Version,
            ForwarderVersion: request.Forwarder?.Version,
            // Named unconditionally: the directory does not exist yet, so the carry-forward
            // that prunes a missing one cannot be used here — and the conflicts land in it
            // on the very next line.
            BackupDirectories: WithBin(previous, bin),
            InstalledAtUtc: DateTimeOffset.UtcNow,
            AntiCheatAcknowledged: request.AntiCheatAcknowledged,
            // The earlier install's files stay listed until each is replaced. Nothing may be
            // missing from this list at any moment: if the process dies, it is what Uninstall
            // deletes by and what a Repair takes for ours — an empty one deletes nothing, and
            // has the Repair move the earlier build's files into AMDNR_backup as the user's.
            Files: previous?.Files ?? [])
        {
            Completed = false,
            // Which runtime package, not just which version: Doctor checks the pass DLLs
            // against this package's accepted hashes, and two packages can share a version.
            RuntimeId = request.Runtime?.Id,
            // The names TRY NEXT PROXY has been through, kept while the DLL keeps its name: a
            // repair is a new build, not a new round. Under another name the round is new.
            TriedProxies = previous is not null
                           && string.Equals(previous.Proxy, request.Proxy, StringComparison.OrdinalIgnoreCase)
                ? previous.TriedProxies
                : [],
        };

        try
        {
            // Written before anything in the game folder moves, exactly as §7 requires — and
            // the conflicts move first, before any payload file lands. Two things depend on it:
            // the rollback below, and the case no rollback can reach — a power cut or a killed
            // process — where this unfinished record is the only evidence that a game folder
            // holds part of a payload, or somebody's own DLL sits in AMDNR_backup. Doctor
            // reports it and offers Uninstall or Repair.
            InstallRecordStore.Save(record);

            MoveConflictsAside(request, bin, ct);

            // Now the bin either holds something or was never created, so the pointer to it can
            // be pruned: an install that moved nothing must not leave a record naming a
            // directory that does not exist.
            record = record with { BackupDirectories = CarryBackupsForward(previous, bin) };
            InstallRecordStore.Save(record);

            var plan = BuildPlan(request, previous);

            // What an earlier install of ours put in this folder and nobody has changed since. A
            // Repair overwriting its own files is the point of a Repair; anything else at a
            // destination is the user's — a file an earlier install disowned, and a file that
            // still has a recorded name but not the recorded bytes, because somebody dropped their
            // own build over it. Uninstall already refuses to delete that one; writing over it
            // here would destroy it with no copy. A file that cannot be read proves nothing, so it
            // is not claimed either: it goes to the backup, where nothing is lost.
            var ours = new HashSet<string>(
                previous?.Files
                    .Where(f => !f.Disowned && IsUnchangedOrGone(request.ExeDirectory, f))
                    .Select(f => f.RelativePath) ?? [],
                StringComparer.OrdinalIgnoreCase);

            for (var i = 0; i < plan.Count; i++)
            {
                ct.ThrowIfCancellationRequested();
                var (source, relative) = plan[i];
                var destination = Path.Combine(request.ExeDirectory, relative);

                PlacedFile file;

                if (File.Exists(destination) && !ours.Contains(relative))
                {
                    // Already the very file we would write: adopted where it stands. Copying over
                    // it gains nothing, and costs the one guarantee a rollback needs — that what
                    // it deletes was not in this folder before.
                    if (IdenticalHash(source, destination) is { } same)
                    {
                        file = new PlacedFile(relative, same, PlacementMode.Copy);
                        wasThere.Add(relative);
                    }
                    else
                    {
                        // Nothing else stands between a user's own file and this copy:
                        // ConflictScanner skips the chosen proxy name, and the payload carries
                        // names no scan looks for. §7 step 3 says never silently overwrite, so a
                        // destination we did not place goes into the same backup the conflict
                        // flow uses.
                        bin.MoveAside(destination);

                        // The record has to learn where that file went before we write over its
                        // old home — for the same reason it is saved before the first file lands.
                        record = record with { BackupDirectories = CarryBackupsForward(previous, bin) };
                        InstallRecordStore.Save(record);

                        var mode = placer.Place(source, destination, ct);
                        file = new PlacedFile(relative, Hashing.Sha256OfFile(destination), mode);
                    }
                }
                else
                {
                    if (File.Exists(destination)) wasThere.Add(relative);

                    var mode = placer.Place(source, destination, ct);
                    file = new PlacedFile(relative, Hashing.Sha256OfFile(destination), mode);
                }

                placed.Add(file);
                ours.Add(relative);

                // Saved as each file lands, so a record that outlives the process names every
                // file this install put in, with the hash that proves it is ours.
                record = record with { Files = Replacing(record.Files, file) };
                InstallRecordStore.Save(record);

                progress?.Report(new InstallProgress("Installing files", i + 1, plan.Count));
            }

            var kept = TakeOutWhatThisInstallNoLongerPlaces(request.ExeDirectory, previous, plan);

            record = record with
            {
                Files = [.. placed, .. kept],
                Completed = true,
                BackupDirectories = CarryBackupsForward(previous, bin),
            };

            InstallRecordStore.Save(record);
            return record;
        }
        catch
        {
            RollBack(request.ExeDirectory, placed, wasThere, bin, previous, record);
            throw;
        }
    }

    /// <summary>The proxy is joined onto the game folder and written over with only the backup
    /// bin between it and someone's file, and the bin can only hold what is inside the folder.
    /// Checked before anything is downloaded or moved, so a bad name costs nothing.</summary>
    private static void RequireProxyName(string proxy)
    {
        if (!PayloadNames.IsProxyName(proxy))
        {
            throw new InvalidOperationException(
                $"\"{proxy}\" is not a DLL name this launcher installs under. " +
                $"Use one of {string.Join(", ", PayloadNames.ProxyNames)}.");
        }
    }

    private static List<PlacedFile> Replacing(IReadOnlyList<PlacedFile> files, PlacedFile file)
        =>
        [
            .. files.Where(f => !string.Equals(f.RelativePath, file.RelativePath, StringComparison.OrdinalIgnoreCase)),
            file,
        ];

    /// <summary>The destination's hash when it is already a byte-for-byte copy of what we are
    /// about to put there, and null otherwise. The record is the first answer to "did we place
    /// this?", but it is not the only one: Load deliberately returns null for a record that is
    /// missing, corrupt, schema-newer or written by a build that is no longer current, and
    /// without this every destination of an earlier install of ours would be taken for the
    /// user's own and moved into a fresh backup — from where the next uninstall would move the
    /// whole payload back in.
    ///
    /// A file we cannot read is one we cannot prove is ours, and so is a source we cannot read.
    /// Either way the caller falls through to moving the destination aside, which is the branch
    /// that loses nothing.</summary>
    private static string? IdenticalHash(string source, string destination)
    {
        var there = Hashing.TrySha256OfFile(destination);
        if (there is null) return null;

        var wanted = Hashing.TrySha256OfFile(source);
        return wanted is not null && Hashing.Matches(there, wanted) ? there : null;
    }

    /// <summary>Undoes everything this install did, in reverse. Deliberately best effort with
    /// narrow filters throughout: a real exception is already on its way out — a full disk, a
    /// cancellation — and a failure while cleaning up must not replace it with a misleading one.
    /// What the user needs to see is why the install stopped, not why the tidying did.</summary>
    private static void RollBack(
        string exeDirectory, List<PlacedFile> placed, HashSet<string> wasThere, BackupBin bin,
        InstallRecord? previous, InstallRecord unfinished)
    {
        var added = placed.Where(f => !wasThere.Contains(f.RelativePath)).ToList();

        foreach (var file in added)
        {
            try { File.Delete(Path.Combine(exeDirectory, file.RelativePath)); }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
        }

        RemoveEmptyDirectoriesWePlacedInto(exeDirectory, added);
        bin.PutEverythingBack();

        // A file this install added that would not go — the game or an antivirus holding it —
        // is still ours and still in the folder, and a record that forgets it strands it. But
        // only a file that can still be read as ours is claimed: one held so tightly it cannot
        // even be hashed proves nothing, and claiming it would let the next Repair overwrite
        // what may be the user's own file with no backup. Checked after the backup went back,
        // which may have put the user's own file over it.
        var stuck = added.Where(f => IsStillThere(exeDirectory, f)).ToList();

        // An earlier install's file this one wrote over stays, and stays ours: the old bytes
        // are gone either way, and a record still holding their hash would have the next
        // uninstall take ours for the user's and leave it in the game.
        var rewritten = placed
            .Where(f => wasThere.Contains(f.RelativePath))
            .ToDictionary(f => f.RelativePath, StringComparer.OrdinalIgnoreCase);

        try
        {
            if (previous is not null)
            {
                var files = previous.Files
                    .Select(f => !f.Disowned && rewritten.TryGetValue(f.RelativePath, out var now)
                        ? f with { Sha256 = now.Sha256, Mode = now.Mode }
                        : f)
                    .ToList();
                foreach (var file in stuck) files = Replacing(files, file);

                // A failed repair leaves the record that was already there, so the files the
                // earlier install did place stay accounted for — but not verbatim:
                // PutEverythingBack is best effort, and a file it could not move back is still
                // in the new stamped directory, which is then not removed. Carrying that
                // directory forward is what keeps a pointer to it; the carry-forward prunes a
                // bin that a clean rollback did empty and remove, so nothing is added in the
                // ordinary case.
                //
                // And it is not finished once any of those files holds other bytes than the
                // earlier install left: a failed switch of build has put the new build's proxy
                // among the old build's files, with no backup of the old ones to put back. Still
                // "finished", Doctor would call that mixture a healthy install of the old build;
                // unfinished, it says so and offers Repair, which puts the old build back whole.
                InstallRecordStore.Save(previous with
                {
                    Files = files,
                    BackupDirectories = CarryBackupsForward(previous, bin),
                    Completed = previous.Completed && files.All(f => IsAsBefore(previous, f)),
                });
                return;
            }

            // A failed first install leaves no record — unless its bin survived, or a file it
            // placed would not go. Then a record is the only thing that can lead anyone back to
            // them. It names those and nothing else: never a file that was already there, such
            // as a hand-made install adopted where it stood, whose matching hash would have the
            // Uninstall this record offers delete somebody's own files.
            var kept = unfinished with
            {
                Files = stuck,
                BackupDirectories = CarryBackupsForward(null, bin),
                Completed = false,
            };

            if (kept.Files.Count > 0 || kept.BackupDirectories.Count > 0)
            {
                InstallRecordStore.Save(kept);
                return;
            }

            try { InstallRecordStore.Delete(exeDirectory); }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException)
            {
                // The record the loop saved lists the adopted files too. If it cannot go, it
                // must at least stop naming them.
                InstallRecordStore.Save(kept);
            }
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    /// <summary>Whether a file this install added is still at its place with the bytes it put
    /// there. One that cannot be read is not provably ours, so it does not count: recorded with
    /// our hash, it is a file the next Repair takes for its own and writes over with no backup —
    /// and what sits there may be the user's, put back by the rollback, or anybody's.</summary>
    private static bool IsStillThere(string exeDirectory, PlacedFile file)
    {
        var path = Path.Combine(exeDirectory, file.RelativePath);
        if (!File.Exists(path)) return false;

        var actual = Hashing.TrySha256OfFile(path);
        return actual is not null && Hashing.Matches(actual, file.Sha256);
    }

    private static bool IsAsBefore(InstallRecord previous, PlacedFile file)
        => previous.Files.Any(p =>
            string.Equals(p.RelativePath, file.RelativePath, StringComparison.OrdinalIgnoreCase)
            && p.Disowned == file.Disowned
            && Hashing.Matches(p.Sha256, file.Sha256));

    /// <summary>(source path, path relative to the game folder) for every file to place.</summary>
    private List<(string Source, string Relative)> BuildPlan(InstallRequest request, InstallRecord? previous)
    {
        var plan = new List<(string Source, string Relative)>();

        // The rules below are about OptiScaler, not about whose build it is: every source this
        // launcher offers is an OptiScaler build, so each rule applies to the mod package.
        foreach (var (source, relative) in PackageFiles(store.PathFor(request.Mod), request.Mod))
        {
            // OptiScaler.dll is installed under the proxy name the user chose.
            if (string.Equals(relative, PayloadNames.OptiScalerDll, StringComparison.OrdinalIgnoreCase))
            {
                plan.Add((source, request.Proxy));
                continue;
            }

            // A user's existing ini is theirs; only seed one when the game has none — or when
            // the one sitting there is provably our own: byte-identical to the seed, or
            // untouched since an earlier install of ours placed it. Skipping our own leaves it
            // out of the record, and the uninstall that reports a clean folder leaves it behind;
            // after a switch of build it would also keep the other build's defaults as if
            // somebody had tuned them.
            if (string.Equals(relative, PayloadNames.OptiScalerIni, StringComparison.OrdinalIgnoreCase))
            {
                // The seed, not the packaged file, is what both checks compare against and what
                // gets placed and recorded: an ini staged with the build's runtime settings
                // matches no packaged file, and would otherwise be taken for the user's own.
                var seed = Stage(source, store.PathFor(request.Mod), request.SourceId,
                    request.Runtime is null ? null : request.IniWhenRuntime);

                var existing = Path.Combine(request.ExeDirectory, PayloadNames.OptiScalerIni);
                if (File.Exists(existing) &&
                    IdenticalHash(seed, existing) is null &&
                    !IsUnchangedSinceWePlacedIt(previous, relative, existing))
                {
                    continue;
                }

                plan.Add((seed, relative));
                continue;
            }

            // The author runtime's own config is user-owned unconditionally. No current
            // package ships it, but the guarantee should not depend on that staying true.
            if (string.Equals(relative, PayloadNames.AuthorIni, StringComparison.OrdinalIgnoreCase))
                continue;

            plan.Add((source, relative));
        }

        if (request.Runtime is not null)
            plan.AddRange(PackageFiles(store.PathFor(request.Runtime), request.Runtime));

        if (request.Forwarder is not null)
            plan.Add((store.PathFor(request.Forwarder), PayloadNames.Forwarder));

        // One destination, one entry. Two entries for the same path place both, record two
        // PlacedFiles for it with different hashes, and at uninstall the stale one will not
        // match — so the launcher decides its own file is user-modified and refuses to delete
        // it, forever, triggered by a manifest change no user can see. Today's packages do not
        // collide; that is luck, not a guarantee. The later entry wins, because the runtime and
        // the manifest's explicitly named forwarder are more specific than whatever the mod
        // archive happens to also carry.
        return plan
            .GroupBy(entry => entry.Relative, StringComparer.OrdinalIgnoreCase)
            .Select(group => group.Last())
            .ToList();
    }

    /// <summary>The OptiScaler.ini an install of this build seeds: the packaged one, or a staged
    /// copy with the build's settings applied. Pass the settings only when the runtime is
    /// installed — they are what a build needs to use it. RESET INI writes this, so that a reset
    /// lands on the same ini an install would have placed, not on one that switches NR off.</summary>
    public string SeedIni(PackageInfo mod, string sourceId, IReadOnlyList<IniSetting>? settings)
    {
        var extracted = store.PathFor(mod);
        return Stage(Path.Combine(ContentRoot(extracted), PayloadNames.OptiScalerIni), extracted, sourceId, settings);
    }

    /// <summary>Written beside the extracted package, never inside it: everything inside is
    /// copied into the game folder. Named for the package version (the folder it sits beside)
    /// and the build, and rewritten whenever its bytes would differ, so it always follows the
    /// packaged ini it was made from.</summary>
    private static string Stage(
        string packagedIni, string extracted, string sourceId, IReadOnlyList<IniSetting>? settings)
    {
        if (settings is not { Count: > 0 }) return packagedIni;

        var package = Path.TrimEndingDirectorySeparator(Path.GetFullPath(extracted));
        var beside = Path.GetDirectoryName(package)
                     ?? throw new InvalidOperationException($"The package at {package} has no folder to stage its ini beside.");

        var invalid = Path.GetInvalidFileNameChars();
        var name = string.Concat(sourceId.Select(c => invalid.Contains(c) ? '_' : c));
        var staged = Path.Combine(beside, $"{Path.GetFileName(package)}.{PayloadNames.OptiScalerIni}.{name}-runtime");

        var bytes = IniSeed.Apply(File.ReadAllBytes(packagedIni), settings);
        if (!File.Exists(staged) || !File.ReadAllBytes(staged).AsSpan().SequenceEqual(bytes))
            File.WriteAllBytes(staged, bytes);

        return staged;
    }

    /// <summary>Every file a package would put in the game folder, relative to its content root,
    /// minus the archive's own paperwork and whatever the manifest excludes. TheAutomatic's
    /// archive carries its own installer and tools: placed beside the exe they are clutter at
    /// best, and a Setup.bat there invites someone to run it over the launcher's install.</summary>
    private static IEnumerable<(string Source, string Relative)> PackageFiles(
        string extractedDirectory, PackageInfo package)
    {
        var root = ContentRoot(extractedDirectory);

        foreach (var source in Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories))
        {
            var relative = Path.GetRelativePath(root, source);
            if (PayloadNames.IsPackageMetadata(relative) || IsExcluded(relative, package.Exclude))
                continue;

            yield return (source, relative);
        }
    }

    /// <summary>The folder inside an extracted package that corresponds to the game folder. An
    /// archive that wraps everything in one top-level folder would otherwise land a folder
    /// down, where the game never looks — with OptiScaler.dll not renamed to the proxy, so
    /// nothing loads at all. Descends while the folder holds exactly one directory and no
    /// files, and no more than three levels: past that it is not a wrapper but a layout this
    /// launcher does not understand, and guessing deeper would be worse than stopping.</summary>
    public static string ContentRoot(string extractedDirectory)
    {
        var root = extractedDirectory;

        for (var depth = 0; depth < 3; depth++)
        {
            if (Directory.EnumerateFiles(root).Any()) break;

            var directories = Directory.GetDirectories(root);
            if (directories.Length != 1) break;

            root = directories[0];
        }

        return root;
    }

    /// <summary>Manifest entries are relative to the content root and are typed by hand, so they
    /// match without regard to case or separator. One ending in a separator excludes that whole
    /// folder.</summary>
    private static bool IsExcluded(string relative, IReadOnlyList<string>? exclude)
    {
        if (exclude is null) return false;

        var path = relative.Replace('\\', '/');

        foreach (var raw in exclude)
        {
            // A blank entry — or a lone separator — must not become a prefix that matches the
            // whole package.
            var entry = (raw ?? "").Trim().Replace('\\', '/').TrimStart('/');
            if (entry.Length == 0) continue;

            var matches = entry.EndsWith('/')
                ? path.StartsWith(entry, StringComparison.OrdinalIgnoreCase)
                : string.Equals(path, entry, StringComparison.OrdinalIgnoreCase);

            if (matches) return true;
        }

        return false;
    }

    /// <summary>Whether the file at this path is still exactly what an earlier install of ours
    /// recorded placing there. Unreadable proves nothing, so it answers no; and a file an
    /// earlier install disowned is the user's, whatever its bytes are now.</summary>
    private static bool IsUnchangedSinceWePlacedIt(InstallRecord? previous, string relative, string path)
    {
        var recorded = previous?.Files.FirstOrDefault(f =>
            !f.Disowned && string.Equals(f.RelativePath, relative, StringComparison.OrdinalIgnoreCase));
        if (recorded is null) return false;

        var actual = Hashing.TrySha256OfFile(path);
        return actual is not null && Hashing.Matches(actual, recorded.Sha256);
    }

    /// <summary>What the earlier install placed that this one does not: the runtime after a
    /// reinstall without it, the other build's forwarder and extras after a switch of build.
    /// The new record replaces the old one, so anything left in the folder and not carried
    /// forward is stranded — ~150 MB no uninstall can find again, and DLLs the game still loads
    /// while Doctor, told they are not installed, stops checking them. A file still exactly as
    /// we placed it is ours to take out. One that changed, or that cannot be read, is not — the
    /// rule uninstall follows — so it stays, and stays in the record, disowned, where the next
    /// uninstall reports it instead of forgetting it. One we cannot take out stays ours.
    ///
    /// Taking ours out is not the whole job when an earlier install moved the game's own file
    /// of that name into AMDNR_backup to make way for it. The path is not ours any more, so the
    /// original goes back now rather than at some later uninstall: older AMDNR builds placed
    /// amd_fidelityfx_dx12.dll beside the exe and 0.3.2 does not, and a game that imports its
    /// own copy does not start without it.</summary>
    private static List<PlacedFile> TakeOutWhatThisInstallNoLongerPlaces(
        string exeDirectory, InstallRecord? previous, List<(string Source, string Relative)> plan)
    {
        var kept = new List<PlacedFile>();
        if (previous is null) return kept;

        var planned = new HashSet<string>(plan.Select(p => p.Relative), StringComparer.OrdinalIgnoreCase);
        var removed = new List<PlacedFile>();

        foreach (var file in previous.Files)
        {
            if (planned.Contains(file.RelativePath)) continue;

            var path = Path.Combine(exeDirectory, file.RelativePath);
            if (!File.Exists(path)) continue;

            var actual = Hashing.TrySha256OfFile(path);
            if (actual is null || !Hashing.Matches(actual, file.Sha256))
            {
                kept.Add(file with { Disowned = true });
                continue;
            }

            try
            {
                var original = OriginalOf(previous, file.RelativePath);
                if (original is null)
                {
                    File.Delete(path);
                    removed.Add(file);
                }
                else
                {
                    // Moved over ours in one step rather than delete-then-restore: a restore
                    // that failed after the delete would leave the game with neither copy.
                    File.Move(original.Value.File, path, overwrite: true);
                    RemoveBackupIfEmpty(original.Value.Backup);
                }

                continue;
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }

            kept.Add(file);
        }

        RemoveEmptyDirectoriesWePlacedInto(exeDirectory, removed);
        RemoveBackupRootIfEmpty(exeDirectory);
        return kept;
    }

    /// <summary>The copy of this path the earliest backup holds: the folder as it was before
    /// AMDNR ever touched it, by the same first-restore-wins rule uninstall follows. A later
    /// backup's copy is a snapshot taken after we were already there, and stays where it is.
    /// An OptiScaler build the install replaced is not an original to put back, for the reason
    /// uninstall does not restore one either.</summary>
    private static (string Backup, string File)? OriginalOf(InstallRecord previous, string relative)
    {
        foreach (var backup in previous.BackupDirectories)
        {
            var file = Path.Combine(backup, relative);
            if (File.Exists(file))
                return IsSupersededOptiScaler(relative, file) ? null : (backup, file);
        }

        return null;
    }

    /// <summary>An OptiScaler build that sat under a proxy name before we installed — by hand,
    /// or with another tool. Putting it back would switch an old build on again, beside ours or
    /// in place of the clean folder the user asked for, so it stays in its backup. Only the
    /// proxy names at the root are checked: that is the only place a build gets loaded from,
    /// and anything else a hand-made install left is ordinary support files.</summary>
    private static bool IsSupersededOptiScaler(string relative, string backedUp)
        => PayloadNames.ProxyNames.Contains(relative, StringComparer.OrdinalIgnoreCase)
           && ProxySlotProbe.Classify(backedUp) == SlotContent.OptiScaler;

    /// <summary>Only once no file is left in it: anything still there is one we could not put
    /// back, and deleting it would lose the user's original. Recursive is safe precisely
    /// because of that check — all that remains are the directories we created to mirror the
    /// game folder's layout. Best effort: every file in it has already gone where it belongs,
    /// and an empty folder left behind costs nothing.</summary>
    private static void RemoveBackupIfEmpty(string backup)
    {
        try
        {
            if (Directory.Exists(backup) &&
                !Directory.EnumerateFiles(backup, "*", SearchOption.AllDirectories).Any())
            {
                Directory.Delete(backup, recursive: true);
            }
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    private static void RemoveBackupRootIfEmpty(string exeDirectory)
    {
        try
        {
            var backupRoot = Path.Combine(exeDirectory, BackupRootName);
            if (Directory.Exists(backupRoot) && !Directory.EnumerateFileSystemEntries(backupRoot).Any())
                Directory.Delete(backupRoot);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    private static void MoveConflictsAside(InstallRequest request, BackupBin bin, CancellationToken ct)
    {
        foreach (var conflict in request.ConflictsToMoveAside)
        {
            ct.ThrowIfCancellationRequested();
            bin.MoveAside(conflict.Path);
        }
    }

    /// <summary>Every backup directory this game folder has produced, oldest first. A record is
    /// keyed by the exe directory, so saving a new one replaces the old: a Repair — which
    /// normally finds no conflicts left to move, and so produces no backup of its own — would
    /// otherwise drop the pointer to the first install's backup, and the user's original file
    /// would sit in AMDNR_backup forever with nobody able to find it.</summary>
    private static IReadOnlyList<string> CarryBackupsForward(InstallRecord? previous, BackupBin bin)
    {
        var all = WithBin(previous, bin);

        // A directory that does not exist is either a backup uninstall already emptied and
        // removed, or a bin nothing was ever put into. Neither is worth a pointer. Only a
        // definite "not there" prunes: Directory.Exists also says false for a share gone
        // offline or a folder it may not look at, and dropping that pointer strands whatever
        // the user's own files are waiting in it.
        all.RemoveAll(directory => DirectoryExists(directory) == false);
        return all;
    }

    /// <summary>A recorded file that is gone is still ours to put back; one that is there has
    /// to still carry the hash we recorded for it.</summary>
    private static bool IsUnchangedOrGone(string exeDirectory, PlacedFile file)
    {
        var path = Path.Combine(exeDirectory, file.RelativePath);
        if (!File.Exists(path)) return true;

        var actual = Hashing.TrySha256OfFile(path);
        return actual is not null && Hashing.Matches(actual, file.Sha256);
    }

    /// <summary>The same list without the pruning, for the one caller that has to name a bin
    /// before anything has been put into it.</summary>
    private static List<string> WithBin(InstallRecord? previous, BackupBin bin)
    {
        var all = previous?.BackupDirectories.ToList() ?? [];
        if (!all.Contains(bin.StampedDirectory, StringComparer.OrdinalIgnoreCase))
            all.Add(bin.StampedDirectory);

        return all;
    }

    /// <summary>The one place anything is moved out of the way during an install: the conflicts
    /// the caller listed, and any destination we are about to write over that we did not place
    /// ourselves. Its directory is named up front but created on first use, so an install with
    /// nothing to move leaves no empty folder in someone's game directory — and it remembers
    /// every move, because a failed install has to put them all back.</summary>
    private sealed class BackupBin(string exeDirectory)
    {
        private readonly List<(string From, string To)> _moved = [];

        /// <summary>Second-level resolution alone is not enough: two installs into the same
        /// folder within one second would merge into one backup and overwrite each other's
        /// originals.</summary>
        public string StampedDirectory { get; } = NextFreeStamp(exeDirectory);

        private static string NextFreeStamp(string exeDirectory)
        {
            var stamped = Path.Combine(
                exeDirectory, BackupRootName, DateTime.Now.ToString("yyyyMMdd-HHmmss"));

            var candidate = stamped;
            for (var n = 2; Directory.Exists(candidate); n++) candidate = $"{stamped}-{n}";
            return candidate;
        }

        public void MoveAside(string path)
        {
            if (!File.Exists(path)) return;

            // Stored under its path relative to the game folder, not its bare filename.
            // ConflictScanner produces exactly one nested conflict — OptiScaler\plugins\
            // XeFGUnlock.asi — and flattening it here relocates it to the folder root on
            // uninstall: debris, and a folder no longer byte-identical to its previous state.
            var relative = Path.GetRelativePath(exeDirectory, path);

            // The conflict list comes from the caller, so a path outside the game folder is
            // possible in principle, and it has no place inside that folder's backup. Skipping
            // it quietly would leave the caller free to write over a file nobody backed up, so
            // the install stops here instead, and rolls back.
            if (Path.IsPathRooted(relative) ||
                relative.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
                        .Contains("..", StringComparer.Ordinal))
            {
                throw new InvalidOperationException(
                    $"{path} is outside the game folder, so it cannot be backed up before the install replaces it.");
            }

            var target = Path.Combine(StampedDirectory, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(target)!);
            File.Move(path, target, overwrite: true);
            _moved.Add((path, target));
        }

        /// <summary>Best effort: this runs during a rollback, with the real exception already on
        /// its way out. Newest move first, so that if two moves ever shared a destination the
        /// earlier — and therefore more original — file is the one that ends up back in place.</summary>
        public void PutEverythingBack()
        {
            for (var i = _moved.Count - 1; i >= 0; i--)
            {
                var (from, to) = _moved[i];
                try
                {
                    if (!File.Exists(to)) continue;
                    Directory.CreateDirectory(Path.GetDirectoryName(from)!);
                    File.Move(to, from, overwrite: true);
                }
                catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
            }

            try
            {
                // Only once no file is left: anything still here is one we could not put back,
                // and deleting it would lose the user's original.
                if (Directory.Exists(StampedDirectory) &&
                    !Directory.EnumerateFiles(StampedDirectory, "*", SearchOption.AllDirectories).Any())
                {
                    Directory.Delete(StampedDirectory, recursive: true);
                }

                var root = Path.Combine(exeDirectory, BackupRootName);
                if (Directory.Exists(root) && !Directory.EnumerateFileSystemEntries(root).Any())
                    Directory.Delete(root);
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
        }
    }

    /// <summary>This is the only path that deletes files out of a real game folder, and the
    /// ordinary way it goes wrong is the user launching the game between running Doctor and
    /// pressing Uninstall. It never stops at the first file it cannot handle — that would leave
    /// the folder half-stripped with nobody told which half — and it reports what it left, so
    /// the caller can say so instead of claiming a clean removal.</summary>
    public Task<UninstallResult> UninstallAsync(InstallRecord record, CancellationToken ct)
    {
        // Everything below reads "not there" as "nothing to do". In a folder that cannot be
        // reached — an external drive unplugged, a share offline — every file is "not there":
        // nothing would be removed, the record that is the only way back to the user's backup
        // would be deleted, and the result would read as a clean uninstall. Refused before
        // anything is touched, so pressing Uninstall again once the drive is back still works.
        if (!CanList(record.ExeDirectory))
            throw new IOException("The game folder is not reachable (is its drive connected?). Nothing was changed.");

        var leftBehind = new List<PlacedFile>();
        var changed = new List<string>();

        foreach (var file in record.Files)
        {
            ct.ThrowIfCancellationRequested();
            var path = Path.Combine(record.ExeDirectory, file.RelativePath);
            if (!File.Exists(path)) continue;

            // A file we cannot read is one we cannot prove is ours, and deleting it on that
            // basis would be exactly the destruction the hash check exists to prevent. The
            // ordinary reason is the running game holding it, so it waits for another try.
            var actual = Hashing.TrySha256OfFile(path);
            if (actual is null)
            {
                leftBehind.Add(file);
                continue;
            }

            // A file whose hash moved is no longer ours to delete: the user changed it, and it
            // stays for good — no second attempt would treat it any differently.
            if (!Hashing.Matches(actual, file.Sha256))
            {
                changed.Add(file.RelativePath);
                continue;
            }

            try { File.Delete(path); }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException)
            {
                leftBehind.Add(file);
            }
        }

        RemoveEmptyDirectoriesWePlacedInto(record.ExeDirectory, record.Files);

        // Oldest first. The earliest backup holds the folder as it was before AMDNR ever
        // touched it, which is the state §10 promises to restore; a later backup's copy of the
        // same name is a newer snapshot taken after we were already there. The first restore to
        // a destination wins, so working forwards in time puts the true original back and
        // leaves the rest in their backup directories rather than overwriting it.
        var notRestored = new List<string>();
        var superseded = new List<string>();

        // Whether pressing Uninstall again, with the game closed, could get further.
        var worthAnotherTry = leftBehind.Count > 0;

        // Backups that could not be read. They stay in the record whatever a second look says:
        // by then the tidying below may have changed what that look sees.
        var unread = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        // Only a backup known to be gone is skipped. One whose existence cannot be told goes on
        // to the listing, which fails for it too and reports it as not read.
        foreach (var backup in record.BackupDirectories.Where(d => DirectoryExists(d) != false))
        {
            List<string> saved;
            try { saved = Directory.EnumerateFiles(backup, "*", SearchOption.AllDirectories).ToList(); }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException)
            {
                // Whatever is in it is the user's and still waiting. Skipping it quietly would
                // delete the only record that points here and report a clean uninstall.
                notRestored.Add(Path.GetRelativePath(record.ExeDirectory, backup));
                unread.Add(backup);
                worthAnotherTry = true;
                continue;
            }

            foreach (var file in saved)
            {
                var relative = Path.GetRelativePath(backup, file);
                var destination = Path.Combine(record.ExeDirectory, relative);

                if (IsSupersededOptiScaler(relative, file))
                {
                    superseded.Add(Path.GetRelativePath(record.ExeDirectory, file));
                    continue;
                }

                // Anything still sitting at the destination survived the hash-gated delete
                // above, which means we deliberately preserved it. Restoring over it would
                // destroy the very file that check protected — the backed-up copy stays put
                // instead, so neither version is lost. This is reachable: a stale forwarder
                // is backed up under the same filename our own forwarder is placed under.
                // Named by where it waits, not where it came from: the user has to find it.
                if (File.Exists(destination))
                {
                    notRestored.Add(Path.GetRelativePath(record.ExeDirectory, file));
                    continue;
                }

                try
                {
                    // The directory the file came from may have gone with our payload.
                    Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                    File.Move(file, destination, overwrite: false);
                }
                catch (Exception e) when (e is IOException or UnauthorizedAccessException)
                {
                    notRestored.Add(Path.GetRelativePath(record.ExeDirectory, file));
                    worthAnotherTry = true;
                }
            }

            RemoveBackupIfEmpty(backup);
        }

        RemoveBackupRootIfEmpty(record.ExeDirectory);

        try
        {
            if (worthAnotherTry)
            {
                // Deleting the record here would leave our locked proxy loading in a folder the
                // launcher then calls "Earlier install found", with Uninstall greyed out, and the
                // user's own file stranded in a backup nothing points to any more. What is left
                // is recorded instead, unfinished, so the row offers Uninstall again and the next
                // one picks up exactly here.
                InstallRecordStore.Save(record with
                {
                    Files = leftBehind,
                    BackupDirectories = record.BackupDirectories.Where(d => unread.Contains(d) || DirectoryExists(d) != false).ToList(),
                    Completed = false,
                });
            }
            else
            {
                // Nothing a second attempt could change: what remains is the user's, and the
                // result is what tells them about it.
                InstallRecordStore.Delete(record.ExeDirectory);
            }
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }

        return Task.FromResult(new UninstallResult(
            [.. leftBehind.Select(f => f.RelativePath)], notRestored, superseded, changed)
        {
            CanFinishLater = worthAnotherTry,
        });
    }

    /// <summary>Whether the directory is there and its contents can be listed. Directory.Exists
    /// alone answers the same false for a folder that is gone and one it could not look at.</summary>
    private static bool CanList(string directory)
    {
        try
        {
            using var entries = Directory.EnumerateFileSystemEntries(directory).GetEnumerator();
            entries.MoveNext();
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    /// <summary>True or false only when the answer is known; null when it cannot be told — a
    /// share gone offline, a drive not ready, a path it may not look at — for all of which
    /// Directory.Exists says false, as if the folder were gone.</summary>
    private static bool? DirectoryExists(string path)
    {
        try
        {
            return (File.GetAttributes(path) & FileAttributes.Directory) != 0;
        }
        catch (Exception e) when (e is FileNotFoundException or DirectoryNotFoundException)
        {
            return false;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    /// <summary>Only directories this install created are candidates for removal. Walking the
    /// whole game folder for empty directories would delete ones the game itself shipped.</summary>
    private static void RemoveEmptyDirectoriesWePlacedInto(
        string exeDirectory, IEnumerable<PlacedFile> files)
    {
        var ours = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        foreach (var file in files)
        {
            var relativeDirectory = Path.GetDirectoryName(file.RelativePath);
            while (!string.IsNullOrEmpty(relativeDirectory))
            {
                ours.Add(Path.Combine(exeDirectory, relativeDirectory));
                relativeDirectory = Path.GetDirectoryName(relativeDirectory);
            }
        }

        // Deepest first, so a parent becomes empty only after its children are gone. Best
        // effort: this also runs during a rollback, where a failure to tidy must not replace
        // the exception that is already on its way out.
        foreach (var directory in ours.OrderByDescending(d => d.Length))
        {
            try
            {
                if (Directory.Exists(directory) &&
                    !Directory.EnumerateFileSystemEntries(directory).Any())
                {
                    Directory.Delete(directory);
                }
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
        }
    }

    public Task<InstallRecord> SwitchProxyAsync(
        InstallRecord record, string newProxy, CancellationToken ct)
    {
        ct.ThrowIfCancellationRequested();
        RequireProxyName(newProxy);

        var oldPath = Path.Combine(record.ExeDirectory, record.Proxy);
        var newPath = Path.Combine(record.ExeDirectory, newProxy);

        // Renaming the record entry without moving a file would leave the record claiming a
        // file exists where none was placed. Say so instead of drifting silently.
        if (!File.Exists(oldPath))
        {
            throw new InvalidOperationException(
                $"{record.Proxy} is missing from the game folder. Repair the install before trying another name.");
        }

        // The file moved is only ever one this launcher placed and nobody has changed since. A
        // record kept by a failed install names the proxy it meant to use while listing none of
        // it as placed, and the DLL at that name is then the user's own, put back by the
        // rollback. One changed after we placed it is theirs too. A file that cannot be read
        // proves nothing either way, so it is refused with them.
        var placed = record.Files.FirstOrDefault(f =>
            !f.Disowned && string.Equals(f.RelativePath, record.Proxy, StringComparison.OrdinalIgnoreCase));
        var actual = placed is null ? null : Hashing.TrySha256OfFile(oldPath);

        if (placed is null || actual is null || !Hashing.Matches(actual, placed.Sha256))
        {
            throw new InvalidOperationException(
                $"{record.Proxy} in the game folder is not the file this launcher put there, or could not be " +
                "read. Repair the install before trying another name.");
        }

        // ProxyPlanner.CandidateOrder only demotes an occupied name, it never drops one, so once
        // the free names run out this is handed a name a foreign DLL holds — and an overwriting
        // move would destroy it with no backup and no way back.
        //
        // The current proxy is the only file at the target name this may move over: switching
        // to the name already in use is the no-op the user asked for. Any other recorded file
        // there would be destroyed by the move, and the record would then carry two entries for
        // the same relative path — after which uninstall refuses to delete that file forever.
        // No payload file carries a proxy name today; that is luck, not a guarantee.
        var isTheCurrentProxy =
            string.Equals(record.Proxy, newProxy, StringComparison.OrdinalIgnoreCase);

        if (File.Exists(newPath) && !isTheCurrentProxy)
        {
            throw new InvalidOperationException(
                $"{newProxy} already exists in the game folder and was not put there by this " +
                "launcher. Move it aside yourself before trying that name.");
        }

        File.Move(oldPath, newPath, overwrite: true);

        var files = record.Files
            .Select(f => string.Equals(f.RelativePath, record.Proxy, StringComparison.OrdinalIgnoreCase)
                ? f with { RelativePath = newProxy }
                : f)
            .ToList();

        var updated = record with { Proxy = newProxy, Files = files };
        InstallRecordStore.Save(updated);
        return Task.FromResult(updated);
    }
}
