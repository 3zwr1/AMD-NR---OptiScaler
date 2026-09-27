// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Collections;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class InstallerTests : IDisposable
{
    private readonly TempDir _dir = new();
    private readonly string _originalRecordRoot = InstallRecordStore.RootDirectory;
    private readonly string _game;
    private readonly StubAssetStore _store;

    public InstallerTests()
    {
        InstallRecordStore.RootDirectory = Path.Combine(_dir.Path, "records");
        _game = Path.Combine(_dir.Path, "game");
        Directory.CreateDirectory(_game);
        _store = new StubAssetStore(Path.Combine(_dir.Path, "assets"));
    }

    public void Dispose()
    {
        InstallRecordStore.RootDirectory = _originalRecordRoot;
        _dir.Dispose();
    }

    private InstallRequest Request(string proxy = "dxgi.dll", IReadOnlyList<Conflict>? conflicts = null)
        => new(_game, "Test Game", proxy, _store.Amdnr, _store.Runtime, _store.Forwarder, "amdnr",
               conflicts ?? [], AntiCheatAcknowledged: false);

    /// <summary>TheAutomatic's build as the manifest describes it: its own package, no
    /// forwarder, and the runtime only when the user asked for it.</summary>
    private InstallRequest TheAutomaticRequest(bool withRuntime = true)
        => new(_game, "Test Game", "dxgi.dll", _store.TheAutomatic,
               withRuntime ? _store.Runtime : null, Forwarder: null, "theautomatic",
               [], AntiCheatAcknowledged: false);

    private Installer NewInstaller() => new(_store, new CopyFilePlacer());

    /// <summary>Every file under the folder with its hash, so "byte-identical" means what it
    /// says rather than "the same names are present".</summary>
    private static string[] Snapshot(string root) =>
        [.. Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories)
            .Select(f => Path.GetRelativePath(root, f) + "  " + Hashing.Sha256OfFile(f))
            .Order(StringComparer.OrdinalIgnoreCase)];

    /// <summary>Places files normally until the nth one, then fails the way a full disk or an
    /// antivirus lock does — part-way through a ~470 MB payload, with files already on disk.</summary>
    private sealed class FailsAfter(int fileCount) : IFilePlacer
    {
        private readonly CopyFilePlacer _real = new();
        private int _done;

        public PlacementMode Place(string source, string destination, CancellationToken ct)
        {
            if (_done++ >= fileCount) throw new IOException("There is not enough space on the disk.");
            return _real.Place(source, destination, ct);
        }
    }

    /// <summary>Fails the same way, and first puts a directory where a moved-aside file has to
    /// go back to — so the rollback's restore fails the way an antivirus lock or a running game
    /// makes it fail, and the backup it could not empty survives.</summary>
    private sealed class FailsAfterAndBlocks(int fileCount, string blocked) : IFilePlacer
    {
        private readonly CopyFilePlacer _real = new();
        private int _done;

        public PlacementMode Place(string source, string destination, CancellationToken ct)
        {
            Directory.CreateDirectory(blocked);
            if (_done++ >= fileCount) throw new IOException("There is not enough space on the disk.");
            return _real.Place(source, destination, ct);
        }
    }

    /// <summary>Places files normally until it reaches this destination file name, then fails
    /// there: every file the plan puts in before it has landed, whatever order that is.</summary>
    private sealed class FailsAt(string fileName) : IFilePlacer
    {
        private readonly CopyFilePlacer _real = new();

        public PlacementMode Place(string source, string destination, CancellationToken ct)
        {
            if (string.Equals(Path.GetFileName(destination), fileName, StringComparison.OrdinalIgnoreCase))
                throw new IOException("There is not enough space on the disk.");
            return _real.Place(source, destination, ct);
        }
    }

    /// <summary>Places files normally until the nth one, then holds the last file it placed open
    /// the way a running game or an antivirus scan does — so the rollback cannot delete it —
    /// and fails. Dispose Held to let go.</summary>
    private sealed class FailsAfterHoldingTheLast(int fileCount) : IFilePlacer
    {
        private readonly CopyFilePlacer _real = new();
        private int _done;
        private string? _last;

        public FileStream? Held { get; private set; }

        public PlacementMode Place(string source, string destination, CancellationToken ct)
        {
            if (_done++ >= fileCount)
            {
                Held = new FileStream(_last!, FileMode.Open, FileAccess.Read, FileShare.Read);
                throw new IOException("There is not enough space on the disk.");
            }

            _last = destination;
            return _real.Place(source, destination, ct);
        }
    }

    /// <summary>Places files normally until it has placed this destination file name. Then, before
    /// the next one, something else writes its own bytes over that file and holds it shut — a
    /// game's own updater, or a hand copy still open — and the install fails. Nothing can read
    /// that file, and nothing can delete it. Dispose Held to let go.</summary>
    private sealed class FailsAfterSomebodyTakesOver(string fileName) : IFilePlacer
    {
        private readonly CopyFilePlacer _real = new();
        private string? _taken;

        public FileStream? Held { get; private set; }

        public PlacementMode Place(string source, string destination, CancellationToken ct)
        {
            if (_taken is not null)
            {
                File.WriteAllText(_taken, "somebody else's bytes");
                Held = new FileStream(_taken, FileMode.Open, FileAccess.Read, FileShare.None);
                throw new IOException("There is not enough space on the disk.");
            }

            if (string.Equals(Path.GetFileName(destination), fileName, StringComparison.OrdinalIgnoreCase))
                _taken = destination;

            return _real.Place(source, destination, ct);
        }
    }

    private Manifest ManifestOfTheStore() => new(
        1, new LauncherInfo("1.0.0", "https://h/l.exe", "AA"),
        [_store.Amdnr, _store.TheAutomatic, _store.Runtime, _store.Forwarder],
        [], PayloadNames.ProxyNames, [], Sources: StubAssetStore.Sources);

    /// <summary>The caller's conflict list, with a callback run as the first conflict is handed
    /// over — the instant before the game folder is mutated for the first time. There is no
    /// other way to stand in that window: every in-process failure triggers the rollback, which
    /// tidies away the evidence a power cut or a killed process would leave.</summary>
    private sealed class ConflictsWatched(IReadOnlyList<Conflict> inner, Action onFirst)
        : IReadOnlyList<Conflict>
    {
        public int Count => inner.Count;

        public Conflict this[int index] => inner[index];

        public IEnumerator<Conflict> GetEnumerator()
        {
            var first = true;
            foreach (var conflict in inner)
            {
                if (first) { onFirst(); first = false; }
                yield return conflict;
            }
        }

        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
    }

    /// <summary>Cancels the install from inside the placement loop, which is where a Cancel
    /// button would land.</summary>
    private sealed class CancelsAfter(int fileCount, CancellationTokenSource source) : IFilePlacer
    {
        private readonly CopyFilePlacer _real = new();
        private int _done;

        public PlacementMode Place(string path, string destination, CancellationToken ct)
        {
            var mode = _real.Place(path, destination, ct);
            if (++_done >= fileCount) source.Cancel();
            return mode;
        }
    }

    /// <summary>Places files normally, and just before the nth placement runs a callback: the
    /// instant a power cut or a closed window would leave the install frozen at.</summary>
    private sealed class Watches(int atCall, Action onCall) : IFilePlacer
    {
        private readonly CopyFilePlacer _real = new();
        private int _done;

        public PlacementMode Place(string source, string destination, CancellationToken ct)
        {
            if (_done++ == atCall) onCall();
            return _real.Place(source, destination, ct);
        }
    }

    [Fact]
    public async Task A_failed_install_over_a_hand_made_install_leaves_its_files_where_they_were()
    {
        // Somebody unzipped the same build by hand and renamed OptiScaler.dll to dxgi.dll. Every
        // one of those files is byte-identical to ours, so none is moved aside — and a rollback
        // that deletes whatever this install "placed" would delete the user's own install with
        // them, leaving a game that no longer loads and a message about disk space.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        File.WriteAllText(Path.Combine(_game, "dxgi.dll"), "optiscaler-payload");
        File.WriteAllText(Path.Combine(_game, PayloadNames.OptiScalerIni), "; packaged defaults");
        Directory.CreateDirectory(Path.Combine(_game, "OptiScaler"));
        File.WriteAllText(Path.Combine(_game, "OptiScaler", "libxess.dll"), "xess");
        Directory.CreateDirectory(Path.Combine(_game, "Licenses"));
        File.WriteAllText(Path.Combine(_game, "Licenses", "XeSS_LICENSE.txt"), "licence");
        var before = Snapshot(_game);

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, new FailsAfter(2)).InstallAsync(Request(), null, default));

        Assert.Equal(before, Snapshot(_game));
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task A_failed_update_keeps_the_earlier_installs_files_and_they_still_uninstall()
    {
        // The update writes over the earlier build's proxy first. Deleting it on the way out
        // leaves the game with no proxy at all — the earlier build no longer loads either.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var before = Snapshot(_game);

        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);

        var package = _store.PathFor(_store.Amdnr);
        File.WriteAllText(Path.Combine(package, "OptiScaler.dll"), "optiscaler-payload v2");
        File.WriteAllText(Path.Combine(package, "OptiScaler", "libxess.dll"), "xess v2");
        var update = Request() with { Mod = _store.Amdnr with { Version = "0.3.3" } };

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, new FailsAfter(1)).InstallAsync(update, null, default));

        Assert.True(File.Exists(Path.Combine(_game, "dxgi.dll")));

        // Whatever the update left in place is still recorded as ours, so nothing is stranded.
        var record = InstallRecordStore.Load(_game);
        Assert.NotNull(record);
        var result = await installer.UninstallAsync(record!, default);

        Assert.True(result.IsComplete);
        Assert.Equal(before, Snapshot(_game));
    }

    [Theory]
    [InlineData(false)]   // a first install
    [InlineData(true)]    // an update that also stops placing the runtime
    public async Task An_install_that_dies_part_way_leaves_a_record_listing_every_file_in_the_folder(bool update)
    {
        // No rollback runs after a power cut or a window closed mid-copy. The unfinished record
        // is then all Uninstall and Repair have to go on, and one that lists nothing lets
        // Uninstall delete nothing, and a Repair take the earlier build's files for the user's.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var request = Request();

        if (update)
        {
            await NewInstaller().InstallAsync(Request(), null, default);

            var package = _store.PathFor(_store.Amdnr);
            File.WriteAllText(Path.Combine(package, "OptiScaler.dll"), "optiscaler-payload v2");
            File.WriteAllText(Path.Combine(package, "OptiScaler", "libxess.dll"), "xess v2");
            request = request with { Mod = _store.Amdnr with { Version = "0.3.3" }, Runtime = null };
        }

        InstallRecord? survivor = null;
        string[] onDisk = [];
        var placer = new Watches(2, () =>
        {
            survivor = InstallRecordStore.Load(_game);
            onDisk = Snapshot(_game);
        });

        await new Installer(_store, placer).InstallAsync(request, null, default);

        Assert.NotNull(survivor);
        Assert.False(survivor!.Completed);

        var listed = survivor.Files
            .Select(f => f.RelativePath + "  " + f.Sha256)
            .ToHashSet(StringComparer.OrdinalIgnoreCase);

        Assert.All(onDisk.Where(line => !line.StartsWith("game.exe", StringComparison.Ordinal)),
            line => Assert.Contains(line, listed));
    }

    [Theory]
    [InlineData(@"..\dxgi.dll")]
    [InlineData("dinput8.dll")]
    [InlineData("OptiScaler.ini")]
    public async Task An_install_refuses_a_proxy_name_that_is_not_a_proxy_dll(string proxy)
    {
        // The proxy name is joined onto the game folder. Anything but one of the known DLL
        // names can point outside it, where nothing is backed up before it is written over.
        var outside = Path.Combine(_dir.Path, "dxgi.dll");
        File.WriteAllText(outside, "a file outside the game folder");
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var before = Snapshot(_game);

        await Assert.ThrowsAsync<InvalidOperationException>(
            () => NewInstaller().InstallAsync(Request(proxy), null, default));

        Assert.Equal("a file outside the game folder", File.ReadAllText(outside));
        Assert.Equal(before, Snapshot(_game));
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task SwitchProxy_refuses_a_name_that_is_not_a_proxy_dll()
    {
        var outside = Path.Combine(_dir.Path, "dxgi.dll");
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(), null, default);

        await Assert.ThrowsAsync<InvalidOperationException>(
            () => installer.SwitchProxyAsync(record, @"..\dxgi.dll", default));

        Assert.False(File.Exists(outside));
        Assert.True(File.Exists(Path.Combine(_game, "dxgi.dll")));
    }

    [Fact]
    public async Task A_conflict_outside_the_game_folder_stops_the_install_instead_of_being_skipped()
    {
        // Skipped quietly, the file stays where it is — and anything the install then places
        // at that path goes over it with no backup behind it.
        var outside = Path.Combine(_dir.Path, "version.dll");
        File.WriteAllText(outside, "not in the game folder");
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var before = Snapshot(_game);

        await Assert.ThrowsAsync<InvalidOperationException>(() => NewInstaller().InstallAsync(
            Request(conflicts: [new Conflict(outside, ConflictKind.InjectionDll, "stray", true)]),
            null, default));

        Assert.Equal("not in the game folder", File.ReadAllText(outside));
        Assert.Equal(before, Snapshot(_game));
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task A_failure_mid_placement_puts_the_game_folder_back()
    {
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var before = Directory.EnumerateFileSystemEntries(_game).Order().ToArray();

        var installer = new Installer(_store, new FailsAfter(3));

        await Assert.ThrowsAsync<IOException>(() => installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default));

        Assert.Equal(before, Directory.EnumerateFileSystemEntries(_game).Order().ToArray());
        Assert.Equal("another mod", File.ReadAllText(stray));
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task A_cancellation_mid_placement_leaves_no_partial_install_behind()
    {
        var before = Directory.EnumerateFileSystemEntries(_game).Order().ToArray();
        using var cts = new CancellationTokenSource();
        var installer = new Installer(_store, new CancelsAfter(2, cts));

        await Assert.ThrowsAnyAsync<OperationCanceledException>(
            () => installer.InstallAsync(Request(), null, cts.Token));

        Assert.Equal(before, Directory.EnumerateFileSystemEntries(_game).Order().ToArray());
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task A_failed_repair_leaves_the_previous_install_recorded()
    {
        // Rolling a repair back to "no record at all" would throw away the pointer to the
        // first install's backup — the very thing that lets Uninstall put the user's file
        // back — and leave a game folder full of our files the launcher no longer knows about.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");

        var first = await NewInstaller().InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        await Assert.ThrowsAsync<IOException>(() => new Installer(_store, new FailsAfter(3))
            .InstallAsync(Request(), null, default));

        var stillThere = InstallRecordStore.Load(_game);
        Assert.NotNull(stillThere);
        Assert.Equal(first.BackupDirectories, stillThere!.BackupDirectories);
    }

    [Fact]
    public async Task A_rollback_that_cannot_empty_its_backup_still_records_where_the_file_went()
    {
        // PutEverythingBack is best effort, so a file it cannot move back stays in the new
        // stamped directory and that directory is not removed. Saving the previous record
        // verbatim leaves the user's file sitting in AMDNR_backup with nothing pointing at it:
        // Doctor cannot see it and Uninstall will never restore it.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");

        var installer = NewInstaller();
        await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        File.WriteAllText(stray, "and another");
        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, new FailsAfterAndBlocks(3, stray)).InstallAsync(
                Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
                null, default));

        var recorded = InstallRecordStore.Load(_game)!.BackupDirectories;
        var onDisk = Directory.EnumerateDirectories(Path.Combine(_game, "AMDNR_backup"));

        Assert.Equal(onDisk.Order(), recorded.Order());
        Assert.Equal("and another", File.ReadAllText(Path.Combine(recorded[^1], "version.dll")));
    }

    [Fact]
    public async Task A_first_install_that_cannot_empty_its_backup_keeps_the_record_naming_it()
    {
        // Worse than the repair case: with no previous record the rollback deleted this one
        // outright, so not even Doctor knew the user's file had been moved anywhere.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, new FailsAfterAndBlocks(3, stray)).InstallAsync(
                Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
                null, default));

        var record = InstallRecordStore.Load(_game);
        Assert.NotNull(record);
        Assert.False(record!.Completed);
        var backup = Assert.Single(record.BackupDirectories);
        Assert.Equal("another mod", File.ReadAllText(Path.Combine(backup, "version.dll")));
    }

    [Fact]
    public async Task A_failed_switch_of_build_is_not_left_recorded_as_a_finished_install()
    {
        // TheAutomatic's build is installed and the user switches to AMDNR. Its proxy and ini go
        // over TheAutomatic's — ours, so there is no backup of them — and then the forwarder
        // fails. The rollback cannot bring TheAutomatic's bytes back, so the folder now loads
        // AMDNR's OptiScaler beside TheAutomatic's leftovers. A record that still says "finished"
        // has Doctor call that a healthy TheAutomatic install, with nothing offering Repair.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var installer = NewInstaller();
        await installer.InstallAsync(TheAutomaticRequest(), null, default);

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, new FailsAt(PayloadNames.Forwarder)).InstallAsync(Request(), null, default));

        Assert.Equal("optiscaler-payload", File.ReadAllText(Path.Combine(_game, "dxgi.dll")));

        var record = InstallRecordStore.Load(_game)!;
        Assert.Equal("theautomatic", record.Source);
        Assert.False(record.Completed);

        var report = Diagnostics.Doctor.Run(record, ManifestOfTheStore());
        Assert.False(report.IsHealthy);
        Assert.Contains(report.Findings, f => f.Id == "install.incomplete");

        // Repair puts the recorded build back over the mixture, and it is whole again.
        var repaired = await installer.InstallAsync(TheAutomaticRequest(), null, default);
        Assert.True(repaired.Completed);
        Assert.Equal("presr-payload", File.ReadAllText(Path.Combine(_game, "dxgi.dll")));
    }

    [Fact]
    public async Task A_failed_repair_that_changed_no_bytes_leaves_the_install_finished()
    {
        // The same build over itself: every file it wrote before failing is the one that was
        // there, so the folder is exactly as it was and nothing is unfinished.
        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, new FailsAt(PayloadNames.Forwarder)).InstallAsync(Request(), null, default));

        Assert.True(InstallRecordStore.Load(_game)!.Completed);
    }

    [Fact]
    public async Task A_failed_first_install_whose_backup_survives_does_not_record_the_users_own_files_as_ours()
    {
        // A hand-made install of the same build: every one of its files is adopted where it
        // stands, never copied over. The install then fails, and the rollback cannot put the
        // user's other DLL back out of AMDNR_backup, so the record has to stay to point at it.
        // Listing the adopted files in it would have the Uninstall it offers delete the user's
        // own install — their hashes match.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        File.WriteAllText(Path.Combine(_game, "dxgi.dll"), "optiscaler-payload");
        Directory.CreateDirectory(Path.Combine(_game, "OptiScaler"));
        File.WriteAllText(Path.Combine(_game, "OptiScaler", "libxess.dll"), "xess");
        Directory.CreateDirectory(Path.Combine(_game, "Licenses"));
        File.WriteAllText(Path.Combine(_game, "Licenses", "XeSS_LICENSE.txt"), "licence");
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var before = Snapshot(_game);

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, new FailsAfterAndBlocks(2, stray)).InstallAsync(
                Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
                null, default));

        var record = InstallRecordStore.Load(_game);
        Assert.NotNull(record);
        Assert.False(record!.Completed);
        Assert.Single(record.BackupDirectories);
        Assert.DoesNotContain(record.Files, f => f.RelativePath is "dxgi.dll"
            or @"OptiScaler\libxess.dll" or @"Licenses\XeSS_LICENSE.txt");

        // Whatever held version.dll's place lets go, and the Uninstall the row offers puts the
        // folder back exactly as it was.
        Directory.Delete(stray);
        var result = await NewInstaller().UninstallAsync(record, default);

        Assert.True(result.IsComplete);
        Assert.Equal(before, Snapshot(_game));
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task A_failed_first_install_keeps_a_record_of_a_file_it_could_not_take_back_out()
    {
        // Deleting the record then would leave our file in the game with nothing that knows
        // it is ours: no Uninstall, and the next install takes it for the user's.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var before = Snapshot(_game);
        var placer = new FailsAfterHoldingTheLast(2);

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, placer).InstallAsync(Request(), null, default));

        string held;
        using (placer.Held)
        {
            held = Path.GetRelativePath(_game, placer.Held!.Name);
            Assert.True(File.Exists(placer.Held.Name));
        }

        var record = InstallRecordStore.Load(_game);
        Assert.NotNull(record);
        Assert.False(record!.Completed);
        Assert.Equal([held], record.Files.Select(f => f.RelativePath));
        Assert.Empty(record.BackupDirectories);

        var result = await NewInstaller().UninstallAsync(record, default);

        Assert.True(result.IsComplete);
        Assert.Equal(before, Snapshot(_game));
    }

    [Fact]
    public async Task A_rollback_does_not_claim_a_file_it_cannot_read_and_the_next_repair_backs_it_up()
    {
        // The rollback cannot delete a file it added, and cannot read it either. Recorded as ours
        // with our hash, it is a file the next Repair writes straight over with no backup — and
        // what is there may not be ours at all.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var placer = new FailsAfterSomebodyTakesOver("dxgi.dll");

        await Assert.ThrowsAsync<IOException>(() =>
            new Installer(_store, placer).InstallAsync(Request("dxgi.dll"), null, default));

        placer.Held!.Dispose();

        Assert.DoesNotContain(InstallRecordStore.Load(_game)?.Files ?? [], f =>
            string.Equals(f.RelativePath, "dxgi.dll", StringComparison.OrdinalIgnoreCase));

        // Not ours, so the Repair moves it aside like any file it did not place.
        var repaired = await NewInstaller().InstallAsync(Request("dxgi.dll"), null, default);

        var backup = Assert.Single(repaired.BackupDirectories);
        Assert.Equal("somebody else's bytes", File.ReadAllText(Path.Combine(backup, "dxgi.dll")));
    }

    [Fact]
    public async Task Places_every_payload_file_and_renames_OptiScaler_to_the_proxy()
    {
        var record = await NewInstaller().InstallAsync(Request("winmm.dll"), null, default);

        Assert.Equal("optiscaler-payload", File.ReadAllText(Path.Combine(_game, "winmm.dll")));
        Assert.False(File.Exists(Path.Combine(_game, "OptiScaler.dll")));
        Assert.True(File.Exists(Path.Combine(_game, "OptiScaler", "libxess.dll")));
        Assert.True(File.Exists(Path.Combine(_game, "Licenses", "XeSS_LICENSE.txt")));
        Assert.True(File.Exists(Path.Combine(_game, PayloadNames.Weights)));
        Assert.True(File.Exists(Path.Combine(_game, PayloadNames.Forwarder)));
        Assert.All(PayloadNames.PassDlls, p => Assert.True(File.Exists(Path.Combine(_game, p))));
        Assert.Equal("winmm.dll", record.Proxy);
    }

    [Fact]
    public async Task Writes_the_packaged_ini_only_when_none_exists()
    {
        File.WriteAllText(Path.Combine(_game, PayloadNames.OptiScalerIni), "; MY settings");

        await NewInstaller().InstallAsync(Request(), null, default);

        Assert.Equal("; MY settings", File.ReadAllText(Path.Combine(_game, PayloadNames.OptiScalerIni)));
    }

    [Fact]
    public async Task Never_touches_the_author_runtime_ini()
    {
        File.WriteAllText(Path.Combine(_game, PayloadNames.AuthorIni), "[DlssNrOnAmd]\nScale=0.03125");

        var record = await NewInstaller().InstallAsync(Request(), null, default);

        Assert.Equal("[DlssNrOnAmd]\nScale=0.03125",
            File.ReadAllText(Path.Combine(_game, PayloadNames.AuthorIni)));
        Assert.DoesNotContain(record.Files, f => f.RelativePath == PayloadNames.AuthorIni);
    }

    [Fact]
    public async Task The_record_names_the_backup_before_the_first_conflict_moves_into_it()
    {
        // The conflicts are moved before any record exists, so a power cut or a kill in that
        // window leaves the user's ReShade DLL — and the stale XeFGUnlock.asi, and any stale
        // forwarder — inside AMDNR_backup with no record anywhere and nothing for Doctor to
        // report. §7's "write the record before the first file lands" covers these too: they
        // are what lands first.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");

        IReadOnlyList<string>? namedWhenTheFirstConflictMoved = null;
        var conflicts = new ConflictsWatched(
            [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)],
            () => namedWhenTheFirstConflictMoved = InstallRecordStore.Load(_game)?.BackupDirectories);

        var record = await NewInstaller().InstallAsync(Request(conflicts: conflicts), null, default);

        var backup = Assert.Single(record.BackupDirectories);
        Assert.Equal([backup], namedWhenTheFirstConflictMoved);
        Assert.Equal("another mod", File.ReadAllText(Path.Combine(backup, "version.dll")));
    }

    [Fact]
    public async Task Moves_conflicts_aside_into_a_timestamped_backup()
    {
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var conflicts = new[] { new Conflict(stray, ConflictKind.InjectionDll, "double injection", true) };

        var record = await NewInstaller().InstallAsync(Request(conflicts: conflicts), null, default);

        Assert.False(File.Exists(stray));
        var backup = Assert.Single(record.BackupDirectories);
        Assert.Equal("another mod", File.ReadAllText(Path.Combine(backup, "version.dll")));
    }

    [Fact]
    public async Task Repairing_keeps_the_first_installs_backup_directory()
    {
        // Install (the user's ReShade dxgi.dll moved aside), an update ships, Repair,
        // Uninstall — four ordinary actions. The repair finds no conflicts left, so its own
        // record carries no backup; if that record replaced the first one wholesale, the
        // user's DLL would sit in AMDNR_backup forever with nobody told where.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");

        var installer = NewInstaller();
        await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        var repaired = await installer.InstallAsync(Request(), null, default);
        Assert.Single(repaired.BackupDirectories);

        await installer.UninstallAsync(repaired, default);

        Assert.Equal("another mod", File.ReadAllText(stray));
    }

    [Fact]
    public async Task A_repair_carries_the_proxy_names_already_tried_forward()
    {
        // TRY NEXT PROXY twice, then REPAIR / UPDATE for a new build: the round carries on where
        // it was, instead of offering the two names that have just failed all over again.
        var installer = NewInstaller();
        var first = await installer.InstallAsync(Request(), null, default);
        InstallRecordStore.Save(first with { TriedProxies = ["winmm.dll", "version.dll"] });

        var repaired = await installer.InstallAsync(Request(), null, default);

        Assert.Equal(["winmm.dll", "version.dll"], repaired.TriedProxies);
        Assert.Equal(["winmm.dll", "version.dll"], InstallRecordStore.Load(_game)!.TriedProxies);
    }

    [Fact]
    public async Task A_backed_up_file_from_a_subdirectory_goes_back_to_that_subdirectory()
    {
        // ConflictScanner produces exactly one nested conflict path, and it is this one.
        // Flattening the backup relocates it to the folder root on uninstall: debris, and a
        // folder that is no longer byte-identical to its pre-install state.
        var asi = Path.Combine(_game, "OptiScaler", "plugins", "XeFGUnlock.asi");
        Directory.CreateDirectory(Path.GetDirectoryName(asi)!);
        File.WriteAllText(asi, "the superseded plugin");

        var installer = NewInstaller();
        var record = await installer.InstallAsync(
            Request(conflicts: [new Conflict(asi, ConflictKind.StaleArtifact, "superseded", true)]),
            null, default);

        Assert.False(File.Exists(asi));

        await installer.UninstallAsync(record, default);

        Assert.Equal("the superseded plugin", File.ReadAllText(asi));
        Assert.False(File.Exists(Path.Combine(_game, "XeFGUnlock.asi")));
    }

    [Fact]
    public async Task Uninstall_restores_the_oldest_backup_of_a_file_backed_up_twice()
    {
        var installer = NewInstaller();
        var stray = Path.Combine(_game, "version.dll");

        File.WriteAllText(stray, "the user's own");
        await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        File.WriteAllText(stray, "whatever turned up later");
        var second = await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        await installer.UninstallAsync(second, default);

        // The earliest backup holds the folder as it was before the launcher ever touched it.
        Assert.Equal("the user's own", File.ReadAllText(stray));
    }

    [Fact]
    public async Task Uninstall_restores_the_folder_to_its_previous_state()
    {
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var conflicts = new[] { new Conflict(stray, ConflictKind.InjectionDll, "double injection", true) };
        var before = Directory.EnumerateFileSystemEntries(_game).Order().ToArray();

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(conflicts: conflicts), null, default);
        await installer.UninstallAsync(record, default);

        Assert.Equal(before, Directory.EnumerateFileSystemEntries(_game).Order().ToArray());
        Assert.Equal("another mod", File.ReadAllText(stray));
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task Uninstall_does_not_delete_empty_directories_the_game_owns()
    {
        // The game shipped an empty folder. It is not ours and must survive.
        var gameOwned = Path.Combine(_game, "Movies");
        Directory.CreateDirectory(gameOwned);

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(), null, default);
        await installer.UninstallAsync(record, default);

        Assert.True(Directory.Exists(gameOwned));
        // Directories we created for our own payload are gone.
        Assert.False(Directory.Exists(Path.Combine(_game, "OptiScaler")));
    }

    [Fact]
    public async Task Uninstall_keeps_a_file_the_user_modified_and_reports_it()
    {
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(), null, default);
        var pass1 = Path.Combine(_game, PayloadNames.PassDlls[0]);
        File.WriteAllText(pass1, "user edited this");

        var result = await installer.UninstallAsync(record, default);

        Assert.True(File.Exists(pass1));
        Assert.Equal("user edited this", File.ReadAllText(pass1));

        // Kept on purpose, not stuck: telling the user to close the game and try again would
        // send them after a problem that does not exist. And pressing Uninstall again would
        // change nothing, so no record is kept for it.
        Assert.Equal([PayloadNames.PassDlls[0]], result.Changed);
        Assert.Empty(result.LeftBehind);
        Assert.False(result.IsComplete);
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task An_uninstall_the_running_game_gets_in_the_way_of_can_be_finished_later()
    {
        // The game holds our dxgi.dll, so it cannot go, and the user's ReShade dxgi.dll cannot
        // come back from AMDNR_backup while it is there. Deleting the record anyway turns the
        // row into "Earlier install found" with Uninstall greyed out, and nothing ever leads
        // back to that backup.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var reshade = Path.Combine(_game, "dxgi.dll");
        File.WriteAllText(reshade, "ReShade");
        var before = Snapshot(_game);

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);
        var backup = Assert.Single(record.BackupDirectories);

        UninstallResult first;
        using (new FileStream(reshade, FileMode.Open, FileAccess.Read, FileShare.Read))
            first = await installer.UninstallAsync(record, default);

        Assert.Equal(["dxgi.dll"], first.LeftBehind);
        Assert.True(first.CanFinishLater);
        // Named by where it waits, so the user can find it.
        Assert.Equal([Path.GetRelativePath(_game, Path.Combine(backup, "dxgi.dll"))], first.NotRestored);

        var remaining = InstallRecordStore.Load(_game);
        Assert.NotNull(remaining);
        Assert.False(remaining!.Completed);
        Assert.Equal(["dxgi.dll"], remaining.Files.Select(f => f.RelativePath));
        Assert.Equal([backup], remaining.BackupDirectories);

        var second = await installer.UninstallAsync(remaining, default);

        Assert.True(second.IsComplete);
        Assert.Equal(before, Snapshot(_game));
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Theory]
    [InlineData(false)]   // the external drive is unplugged: the folder is not there at all
    [InlineData(true)]    // its name still answers, but it cannot be listed
    public async Task An_uninstall_with_the_game_folder_unreachable_changes_nothing_and_keeps_the_record(
        bool nameStillAnswers)
    {
        // Every File.Exists in an unreachable folder is false, so nothing was removed, the record
        // — the only way back to the user's version.dll in AMDNR_backup — was deleted, and the
        // result read as a clean uninstall.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var installer = NewInstaller();
        var record = await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]), null, default);
        var before = Snapshot(_game);
        var recorded = System.Text.Json.JsonSerializer.Serialize(InstallRecordStore.Load(_game));

        var unplugged = Path.Combine(_dir.Path, "unplugged");
        var notThere = Path.Combine(_dir.Path, "not-there");
        Directory.Move(_game, unplugged);
        if (nameStillAnswers) Directory.CreateSymbolicLink(_game, notThere);

        var problem = await Assert.ThrowsAsync<IOException>(() => installer.UninstallAsync(record, default));

        Assert.Equal("The game folder is not reachable (is its drive connected?). Nothing was changed.",
            problem.Message);
        Assert.Equal(recorded, System.Text.Json.JsonSerializer.Serialize(InstallRecordStore.Load(_game)));

        // Plugged back in, the folder is as it was, and the Uninstall the kept record offers
        // finishes the job.
        if (nameStillAnswers)
        {
            // A link to nothing cannot be deleted; one to an empty folder goes on its own.
            Directory.CreateDirectory(notThere);
            Directory.Delete(_game);
        }

        Directory.Move(unplugged, _game);
        Assert.Equal(before, Snapshot(_game));

        var result = await installer.UninstallAsync(InstallRecordStore.Load(_game)!, default);
        Assert.True(result.IsComplete);
        Assert.Equal("another mod", File.ReadAllText(stray));
    }

    [Fact]
    public async Task An_uninstall_that_cannot_read_a_backup_keeps_the_record_and_names_the_backup()
    {
        // The user's own version.dll waits in a backup the uninstall cannot list. Skipping it
        // quietly deleted the record and said "Uninstalled." — with nothing left that points
        // at the backup, and nobody told it is there.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var installer = NewInstaller();
        var record = await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]), null, default);
        var backup = Assert.Single(record.BackupDirectories);

        // The backup's name still answers Directory.Exists, but listing it fails — the way an
        // unreadable one does: here it is a link to a folder that is not there.
        var aside = Path.Combine(_dir.Path, "backup-aside");
        var target = Path.Combine(_dir.Path, "not-there-yet");
        Directory.Move(backup, aside);
        Directory.CreateSymbolicLink(backup, target);

        var result = await installer.UninstallAsync(record, default);

        Assert.False(result.IsComplete);
        Assert.True(result.CanFinishLater);
        Assert.Contains(Path.GetRelativePath(_game, backup), result.NotRestored);
        Assert.Equal([backup], InstallRecordStore.Load(_game)!.BackupDirectories);

        // With the backup readable again, the next Uninstall finishes the job.
        Directory.Move(aside, target);
        var second = await installer.UninstallAsync(InstallRecordStore.Load(_game)!, default);
        Assert.Equal("another mod", File.ReadAllText(stray));
        Assert.False(second.CanFinishLater);
        Assert.Null(InstallRecordStore.Load(_game));
    }

    [Fact]
    public async Task An_uninstall_that_cannot_tell_whether_a_backup_is_there_keeps_the_record_and_names_it()
    {
        // Directory.Exists answers false for every failure, not only for a folder that is gone:
        // a share gone offline, a drive not ready, a folder it may not look at. Taken as gone,
        // the backup was skipped, the record pointing at it deleted, and the uninstall called
        // clean. A name Windows refuses to look up fails the same way, and is the one a test
        // can produce on demand.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var installer = NewInstaller();
        var installed = await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]), null, default);
        var readable = Assert.Single(installed.BackupDirectories);
        var unknown = Path.Combine(_game, "AMDNR_backup", "20260924-101500|");
        var record = installed with { BackupDirectories = [readable, unknown] };
        InstallRecordStore.Save(record);

        var result = await installer.UninstallAsync(record, default);

        // The backup it could read went back as usual.
        Assert.Equal("another mod", File.ReadAllText(stray));

        Assert.True(result.CanFinishLater);
        Assert.False(result.IsComplete);
        Assert.Equal([@"AMDNR_backup\20260924-101500|"], result.NotRestored);
        Assert.Equal([unknown], InstallRecordStore.Load(_game)!.BackupDirectories);
    }

    [Fact]
    public async Task Uninstall_continues_past_a_file_it_cannot_delete()
    {
        // The ordinary trigger: the user launched the game between running Doctor and pressing
        // Uninstall. Stopping here leaves the folder half-stripped with nobody told which half.
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(), null, default);

        var held = Path.Combine(_game, PayloadNames.PassDlls[0]);
        using var game = new FileStream(held, FileMode.Open, FileAccess.Read, FileShare.Read);

        var result = await installer.UninstallAsync(record, default);

        Assert.True(File.Exists(held));
        Assert.Contains(PayloadNames.PassDlls[0], result.LeftBehind);

        // Everything it could take out, it took out.
        Assert.False(File.Exists(Path.Combine(_game, PayloadNames.Weights)));
        Assert.False(File.Exists(Path.Combine(_game, "dxgi.dll")));
    }

    [Fact]
    public async Task Uninstall_continues_past_a_file_it_cannot_read()
    {
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(), null, default);

        var locked = Path.Combine(_game, PayloadNames.PassDlls[1]);
        using var scanner = new FileStream(locked, FileMode.Open, FileAccess.Read, FileShare.None);

        var result = await installer.UninstallAsync(record, default);

        Assert.Contains(PayloadNames.PassDlls[1], result.LeftBehind);
        Assert.False(File.Exists(Path.Combine(_game, PayloadNames.Weights)));
    }

    [Fact]
    public async Task Uninstall_reports_a_backup_it_could_not_put_back()
    {
        var forwarder = Path.Combine(_game, PayloadNames.Forwarder);
        File.WriteAllText(forwarder, "a stale forwarder");

        var installer = NewInstaller();
        var record = await installer.InstallAsync(
            Request(conflicts:
                [new Conflict(forwarder, ConflictKind.StaleArtifact, "outdated forwarder", true)]),
            null, default);

        File.WriteAllText(forwarder, "user edited this");

        var result = await installer.UninstallAsync(record, default);

        Assert.Equal(
            [Path.GetRelativePath(_game, Path.Combine(record.BackupDirectories.Single(), PayloadNames.Forwarder))],
            result.NotRestored);
        Assert.Equal([PayloadNames.Forwarder], result.Changed);
        Assert.False(result.IsComplete);
    }

    [Fact]
    public async Task A_clean_uninstall_reports_nothing_left_behind()
    {
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(), null, default);

        Assert.True((await installer.UninstallAsync(record, default)).IsComplete);
    }

    [Fact]
    public async Task SwitchProxy_moves_one_file_and_leaves_the_rest_alone()
    {
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);
        var weightsWrittenAt = File.GetLastWriteTimeUtc(Path.Combine(_game, PayloadNames.Weights));

        var updated = await installer.SwitchProxyAsync(record, "d3d12.dll", default);

        Assert.False(File.Exists(Path.Combine(_game, "dxgi.dll")));
        Assert.Equal("optiscaler-payload", File.ReadAllText(Path.Combine(_game, "d3d12.dll")));
        Assert.Equal("d3d12.dll", updated.Proxy);
        Assert.Equal(weightsWrittenAt, File.GetLastWriteTimeUtc(Path.Combine(_game, PayloadNames.Weights)));
    }

    [Fact]
    public async Task Uninstall_does_not_restore_a_backup_over_a_file_the_user_modified()
    {
        // A stale forwarder is backed up under the same filename our own forwarder is then
        // placed under. If the user later edits that file, uninstall preserves it — and must
        // not restore the backup on top, destroying what the hash check just protected.
        var forwarder = Path.Combine(_game, PayloadNames.Forwarder);
        File.WriteAllText(forwarder, "a stale forwarder");
        var conflicts = new[]
        {
            new Conflict(forwarder, ConflictKind.StaleArtifact, "outdated forwarder", true),
        };

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(conflicts: conflicts), null, default);
        File.WriteAllText(forwarder, "user edited this");

        await installer.UninstallAsync(record, default);

        Assert.Equal("user edited this", File.ReadAllText(forwarder));
        // The original is not lost either — it stays in the backup.
        Assert.Equal("a stale forwarder",
            File.ReadAllText(Path.Combine(record.BackupDirectories.Single(), PayloadNames.Forwarder)));
    }

    [Fact]
    public async Task Two_installs_within_the_same_second_get_separate_backup_directories()
    {
        var installer = NewInstaller();
        var stray = Path.Combine(_game, "version.dll");

        File.WriteAllText(stray, "first");
        var first = await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        File.WriteAllText(stray, "second");
        var second = await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        var firstBackup = Assert.Single(first.BackupDirectories);
        Assert.Equal(2, second.BackupDirectories.Count);
        Assert.Equal(firstBackup, second.BackupDirectories[0]);

        Assert.NotEqual(firstBackup, second.BackupDirectories[1]);
        Assert.Equal("first", File.ReadAllText(Path.Combine(firstBackup, "version.dll")));
        Assert.Equal("second",
            File.ReadAllText(Path.Combine(second.BackupDirectories[1], "version.dll")));
    }

    [Fact]
    public async Task SwitchProxy_refuses_when_the_current_proxy_file_is_missing()
    {
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);
        File.Delete(Path.Combine(_game, "dxgi.dll"));

        await Assert.ThrowsAsync<InvalidOperationException>(
            () => installer.SwitchProxyAsync(record, "d3d12.dll", default));
    }

    [Fact]
    public async Task SwitchProxy_refuses_a_proxy_the_record_does_not_list_as_placed()
    {
        // A failed install keeps its record when a backup survives, and that record names the
        // proxy the install was going to use while listing none of it as placed: the dxgi.dll
        // in the folder is the user's own ReShade, put back by the rollback. Moving it to another
        // name would break their ReShade and leave the record claiming it as ours.
        File.WriteAllText(Path.Combine(_game, "dxgi.dll"), "ReShade");
        var kept = new InstallRecord(_game, "Test Game", "dxgi.dll", "amdnr", "0.3.2", null, null,
            [Path.Combine(_game, "AMDNR_backup", "20260924-101500")], DateTimeOffset.UtcNow, false, [])
        {
            Completed = false,
        };
        InstallRecordStore.Save(kept);

        var problem = await Assert.ThrowsAsync<InvalidOperationException>(
            () => NewInstaller().SwitchProxyAsync(kept, "d3d12.dll", default));

        Assert.Contains("Repair the install before trying another name.", problem.Message);
        Assert.Equal("ReShade", File.ReadAllText(Path.Combine(_game, "dxgi.dll")));
        Assert.False(File.Exists(Path.Combine(_game, "d3d12.dll")));
        Assert.Equal("dxgi.dll", InstallRecordStore.Load(_game)!.Proxy);
    }

    [Fact]
    public async Task SwitchProxy_refuses_a_proxy_that_is_no_longer_the_file_it_placed()
    {
        // The user put their own dxgi.dll over ours. It is theirs now, the rule uninstall keeps.
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);
        File.WriteAllText(Path.Combine(_game, "dxgi.dll"), "ReShade");

        var problem = await Assert.ThrowsAsync<InvalidOperationException>(
            () => installer.SwitchProxyAsync(record, "d3d12.dll", default));

        Assert.Contains("Repair the install before trying another name.", problem.Message);
        Assert.Equal("ReShade", File.ReadAllText(Path.Combine(_game, "dxgi.dll")));
        Assert.False(File.Exists(Path.Combine(_game, "d3d12.dll")));
    }

    [Fact]
    public async Task SwitchProxy_refuses_to_destroy_a_file_it_did_not_place()
    {
        // ProxyPlanner.CandidateOrder only demotes an occupied name, it never drops one, so
        // once the free names run out TryNextProxy hands this a name a foreign DLL holds.
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);

        var foreign = Path.Combine(_game, "d3d12.dll");
        File.WriteAllText(foreign, "somebody else's mod");

        var problem = await Assert.ThrowsAsync<InvalidOperationException>(
            () => installer.SwitchProxyAsync(record, "d3d12.dll", default));

        Assert.Contains("d3d12.dll", problem.Message);
        Assert.Equal("somebody else's mod", File.ReadAllText(foreign));
        Assert.True(File.Exists(Path.Combine(_game, "dxgi.dll")));
    }

    [Fact]
    public async Task SwitchProxy_refuses_a_name_another_recorded_file_already_holds()
    {
        // The "is ours" branch exists for the proxy and nothing else. Any other recorded file
        // at the target name is destroyed by an overwriting move, and the record then carries
        // two entries for the same relative path — the duplicate that makes uninstall decide
        // its own file is user-modified and refuse to delete it, forever. Today's payload
        // carries no such name, so this is latent rather than live.
        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);

        var other = Path.Combine(_game, "winmm.dll");
        File.WriteAllText(other, "a payload file that happens to carry a proxy name");
        var withOther = record with
        {
            Files = [.. record.Files,
                     new PlacedFile("winmm.dll", Hashing.Sha256OfFile(other), PlacementMode.Copy)],
        };

        var problem = await Assert.ThrowsAsync<InvalidOperationException>(
            () => installer.SwitchProxyAsync(withOther, "winmm.dll", default));

        Assert.Contains("winmm.dll", problem.Message);
        Assert.Equal("a payload file that happens to carry a proxy name", File.ReadAllText(other));
        Assert.True(File.Exists(Path.Combine(_game, "dxgi.dll")));
    }

    [Fact]
    public async Task Package_metadata_is_not_installed_into_the_game_folder()
    {
        // AMDNR's archive ships LICENSE, README.md and SHA256SUMS.txt at its root. None of
        // them is part of the mod, and a game with its own README.md beside its exe would have
        // it overwritten with no backup and no conflict entry — then deleted on uninstall,
        // because by then the hash matches what we recorded.
        var package = _store.PathFor(_store.Amdnr);
        File.WriteAllText(Path.Combine(package, "README.md"), "the package's readme");
        File.WriteAllText(Path.Combine(package, "LICENSE"), "the package's licence");
        File.WriteAllText(Path.Combine(package, "SHA256SUMS.txt"), "hashes");

        File.WriteAllText(Path.Combine(_game, "README.md"), "the game's own readme");

        var record = await NewInstaller().InstallAsync(Request(), null, default);

        Assert.Equal("the game's own readme", File.ReadAllText(Path.Combine(_game, "README.md")));
        Assert.False(File.Exists(Path.Combine(_game, "LICENSE")));
        Assert.False(File.Exists(Path.Combine(_game, "SHA256SUMS.txt")));
        Assert.DoesNotContain(record.Files, f => f.RelativePath == "README.md");
    }

    [Fact]
    public async Task A_destination_we_did_not_place_is_moved_aside_rather_than_overwritten()
    {
        // ConflictScanner skips the chosen proxy name, so a foreign DLL already sitting at
        // that name reaches the placement loop with no conflict entry behind it.
        var theirs = Path.Combine(_game, "dxgi.dll");
        File.WriteAllText(theirs, "somebody else's dxgi");

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);

        Assert.Equal("optiscaler-payload", File.ReadAllText(theirs));
        var backup = Assert.Single(record.BackupDirectories);
        Assert.Equal("somebody else's dxgi", File.ReadAllText(Path.Combine(backup, "dxgi.dll")));

        await installer.UninstallAsync(record, default);

        Assert.Equal("somebody else's dxgi", File.ReadAllText(theirs));
    }

    [Fact]
    public async Task A_relative_path_shipped_by_two_packages_gets_one_plan_entry()
    {
        // Two entries for one destination place both, record two PlacedFiles for it with
        // different hashes, and at uninstall the stale entry's hash will not match — so the
        // launcher decides its own file is user-modified and refuses to delete it, forever,
        // from a manifest change no user can see.
        File.WriteAllText(Path.Combine(_store.PathFor(_store.Amdnr), "shared.dll"), "from the mod");
        File.WriteAllText(Path.Combine(_store.PathFor(_store.Runtime), "shared.dll"), "from runtime");

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request(), null, default);

        Assert.Single(record.Files, f =>
            string.Equals(f.RelativePath, "shared.dll", StringComparison.OrdinalIgnoreCase));

        await installer.UninstallAsync(record, default);

        Assert.False(File.Exists(Path.Combine(_game, "shared.dll")));
    }

    [Fact]
    public async Task A_repair_does_not_move_its_own_earlier_files_aside()
    {
        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);

        var repaired = await installer.InstallAsync(Request(), null, default);

        Assert.Empty(repaired.BackupDirectories);
        Assert.False(Directory.Exists(Path.Combine(_game, "AMDNR_backup")));
    }

    [Fact]
    public async Task A_repair_over_a_file_the_user_replaced_backs_it_up_and_uninstall_brings_it_back()
    {
        // A recorded name is not proof the file is still ours: the user may have dropped their
        // own OptiScaler build over it since. Uninstall already refuses to delete such a file;
        // a Repair that wrote over it would destroy it with no copy — and "Repair the install"
        // is exactly what TRY NEXT PROXY tells the user to do when the proxy has changed.
        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);
        var proxy = Path.Combine(_game, "dxgi.dll");
        File.WriteAllText(proxy, "the user's own build");

        var repaired = await installer.InstallAsync(Request(), null, default);

        var backup = Assert.Single(repaired.BackupDirectories);
        Assert.Equal("the user's own build", File.ReadAllText(Path.Combine(backup, "dxgi.dll")));
        Assert.NotEqual("the user's own build", File.ReadAllText(proxy));

        await installer.UninstallAsync(repaired, default);

        Assert.Equal("the user's own build", File.ReadAllText(proxy));
    }

    [Fact]
    public async Task A_repair_with_the_game_folder_unreachable_changes_nothing_and_keeps_every_backup_pointer()
    {
        // Every backup looks missing on an unplugged drive, so the carry-forward pruned them all,
        // and the record came out complete with no way back to the user's version.dll. Plugged
        // back in, Uninstall then reported a clean folder while it sat in AMDNR_backup for good.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var installer = NewInstaller();
        await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]), null, default);
        var recorded = System.Text.Json.JsonSerializer.Serialize(InstallRecordStore.Load(_game));
        var unplugged = Path.Combine(_dir.Path, "unplugged");
        Directory.Move(_game, unplugged);

        var problem = await Assert.ThrowsAsync<IOException>(() => installer.InstallAsync(Request(), null, default));

        Assert.Equal("The game folder is not reachable (is its drive connected?). Nothing was changed.",
            problem.Message);
        Assert.Equal(recorded, System.Text.Json.JsonSerializer.Serialize(InstallRecordStore.Load(_game)));
        Assert.False(Directory.Exists(_game));

        Directory.Move(unplugged, _game);
    }

    [Fact]
    public async Task A_repair_keeps_a_backup_pointer_whose_existence_cannot_be_told()
    {
        // Directory.Exists says false for a share gone offline or a folder it may not look at,
        // and the carry-forward took that as "already emptied". A name Windows refuses to look
        // up fails the same way, and is the one a test can produce on demand.
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");
        var installer = NewInstaller();
        var installed = await installer.InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]), null, default);
        var readable = Assert.Single(installed.BackupDirectories);
        var unknown = Path.Combine(_game, "AMDNR_backup", "20260924-101500|");
        InstallRecordStore.Save(installed with { BackupDirectories = [readable, unknown] });

        var repaired = await installer.InstallAsync(Request(), null, default);

        Assert.Contains(readable, repaired.BackupDirectories);
        Assert.Contains(unknown, repaired.BackupDirectories);
    }

    [Fact]
    public async Task An_install_that_lost_its_record_still_uninstalls_the_folder_clean()
    {
        // Load returns null for a record that is missing, corrupt, schema-newer or pre-fix, and
        // "ours" was read from that record alone. With no record every destination our own
        // earlier install placed looks like somebody else's file: moved into a fresh backup,
        // then moved back by the uninstall that has just deleted the payload it recorded. The
        // folder ends up holding a complete, loading install while the launcher says "Not
        // installed", and repeating Install -> Uninstall reproduces the identical state.
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var before = Snapshot(_game);

        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);

        InstallRecordStore.Delete(_game);

        var second = await installer.InstallAsync(Request(), null, default);
        await installer.UninstallAsync(second, default);

        Assert.Equal(before, Snapshot(_game));
    }

    [Fact]
    public async Task Records_an_acknowledged_anticheat_warning()
    {
        var request = Request() with { AntiCheatAcknowledged = true };

        var record = await NewInstaller().InstallAsync(request, null, default);

        Assert.True(record.AntiCheatAcknowledged);
        Assert.True(InstallRecordStore.Load(_game)!.AntiCheatAcknowledged);
    }

    [Fact]
    public async Task Reports_progress_across_the_whole_payload()
    {
        var seen = new List<InstallProgress>();

        await NewInstaller().InstallAsync(Request(), new Progress<InstallProgress>(seen.Add), default);

        for (var i = 0; i < 50 && seen.Count == 0; i++) await Task.Delay(10);
        Assert.Equal(seen[^1].FilesTotal, seen[^1].FilesDone);
    }

    [Fact]
    public async Task A_package_nested_in_one_top_folder_installs_from_inside_it()
    {
        // Placed as it stands, the whole build lands one folder down, where the game never
        // looks — and OptiScaler.dll is not renamed to the proxy, so nothing loads at all.
        var record = await NewInstaller().InstallAsync(TheAutomaticRequest(), null, default);

        Assert.Equal("presr-payload", File.ReadAllText(Path.Combine(_game, "dxgi.dll")));
        Assert.Equal("; presr defaults", File.ReadAllText(Path.Combine(_game, PayloadNames.OptiScalerIni)));
        Assert.True(File.Exists(Path.Combine(_game, "OptiScaler", "libxess.dll")));
        Assert.True(File.Exists(Path.Combine(_game, "Licenses", "OptiScaler_LICENSE.txt")));
        Assert.False(Directory.Exists(Path.Combine(_game, StubAssetStore.TheAutomaticTopFolder)));
        Assert.DoesNotContain(record.Files, f =>
            f.RelativePath.StartsWith(StubAssetStore.TheAutomaticTopFolder, StringComparison.OrdinalIgnoreCase));
        Assert.Equal("theautomatic", record.Source);
    }

    [Theory]
    [InlineData(false)]    // the manifest's own spelling
    [InlineData(true)]     // another case, and Windows separators
    public async Task Exclude_entries_are_not_placed_files_and_folders(bool otherSpelling)
    {
        // TheAutomatic's archive carries its own installer and tools. Placed in a game folder
        // they are clutter at best, and a Setup.bat beside the exe invites someone to run it
        // over the launcher's install.
        var request = TheAutomaticRequest();
        if (otherSpelling)
        {
            request = request with
            {
                Mod = request.Mod with
                {
                    Exclude = ["SETUP.BAT", "setup.PS1", @"TOOLS\",
                               "uninstall_optiscaler_nr.BAT", @"\Uninstall_OptiScaler_NR.ps1"],
                },
            };
        }

        var record = await NewInstaller().InstallAsync(request, null, default);

        foreach (var name in new[] { "Setup.bat", "Setup.ps1", "Uninstall_OptiScaler_NR.bat", "Uninstall_OptiScaler_NR.ps1" })
            Assert.False(File.Exists(Path.Combine(_game, name)), name);

        Assert.False(File.Exists(Path.Combine(_game, "tools", "x.ps1")));
        Assert.False(Directory.Exists(Path.Combine(_game, "tools")));
        Assert.DoesNotContain(record.Files, f =>
            f.RelativePath.StartsWith("tools", StringComparison.OrdinalIgnoreCase) ||
            f.RelativePath.StartsWith("Setup", StringComparison.OrdinalIgnoreCase));

        // Excluding is by entry, not by guess: the rest of the build still lands.
        Assert.True(File.Exists(Path.Combine(_game, "OptiScaler", "libxess.dll")));
    }

    [Fact]
    public async Task Root_markdown_LICENSE_and_checksums_are_not_placed_but_Licenses_folder_is()
    {
        // AMDNR 0.3.2 ships a CHANGELOG and a README per language at its root. Any of them can
        // share a name with a game's own file, and uninstall would delete the game's copy once
        // the hash matched what we recorded. The Licenses folder is different: OptiScaler is
        // GPL-3.0 and its notices go wherever the binaries go.
        var package = _store.PathFor(_store.Amdnr);
        File.WriteAllText(Path.Combine(package, "CHANGELOG.md"), "changes");
        File.WriteAllText(Path.Combine(package, "README.es.md"), "léame");
        File.WriteAllText(Path.Combine(package, "LICENSE"), "gpl");
        File.WriteAllText(Path.Combine(package, "SHA256SUMS.txt"), "hashes");
        File.WriteAllText(Path.Combine(package, "Licenses", "LICENSE"), "a notice inside the payload");
        File.WriteAllText(Path.Combine(package, "OptiScaler", "notes.md"), "part of the mod");

        var record = await NewInstaller().InstallAsync(Request(), null, default);

        foreach (var name in new[] { "CHANGELOG.md", "README.es.md", "LICENSE", "SHA256SUMS.txt" })
        {
            Assert.False(File.Exists(Path.Combine(_game, name)), name);
            Assert.DoesNotContain(record.Files, f => f.RelativePath == name);
        }

        Assert.True(File.Exists(Path.Combine(_game, "Licenses", "XeSS_LICENSE.txt")));
        Assert.True(File.Exists(Path.Combine(_game, "Licenses", "LICENSE")));
        Assert.True(File.Exists(Path.Combine(_game, "OptiScaler", "notes.md")));
    }

    [Theory]
    [InlineData("README.md", true)]
    [InlineData("readme.PT-BR.MD", true)]
    [InlineData("CHANGELOG.md", true)]
    [InlineData("license", true)]
    [InlineData("SHA256SUMS.txt", true)]
    [InlineData(@"Licenses\LICENSE", false)]
    [InlineData(@"OptiScaler\README.md", false)]
    [InlineData("LICENSE.dll", false)]
    [InlineData("OptiScaler.ini", false)]
    public void IsPackageMetadata_is_root_level_paperwork_only(string relativePath, bool expected)
    {
        Assert.Equal(expected, PayloadNames.IsPackageMetadata(relativePath));
    }

    [Fact]
    public async Task Installing_without_the_runtime_places_no_pass_dlls_and_records_null_runtime()
    {
        var record = await NewInstaller().InstallAsync(Request() with { Runtime = null }, null, default);

        Assert.All(PayloadNames.PassDlls, p => Assert.False(File.Exists(Path.Combine(_game, p)), p));
        Assert.False(File.Exists(Path.Combine(_game, PayloadNames.Weights)));
        Assert.Null(record.RuntimeVersion);

        // The rest of AMDNR still goes in, forwarder included.
        Assert.Equal("optiscaler-payload", File.ReadAllText(Path.Combine(_game, "dxgi.dll")));
        Assert.True(File.Exists(Path.Combine(_game, PayloadNames.Forwarder)));

        // And the record that says so is one the store will read back.
        var stored = InstallRecordStore.Load(_game);
        Assert.NotNull(stored);
        Assert.Null(stored!.RuntimeVersion);
        Assert.Equal("amdnr", stored.Source);
    }

    [Fact]
    public async Task A_source_without_a_forwarder_places_none()
    {
        var record = await NewInstaller().InstallAsync(TheAutomaticRequest(), null, default);

        Assert.False(File.Exists(Path.Combine(_game, PayloadNames.Forwarder)));
        Assert.Null(record.ForwarderVersion);
        Assert.Equal("0.3.1", record.RuntimeVersion);
        Assert.All(PayloadNames.PassDlls, p => Assert.True(File.Exists(Path.Combine(_game, p)), p));
    }

    [Fact]
    public async Task The_backup_folder_in_the_game_directory_is_named_AMDNR_backup()
    {
        var stray = Path.Combine(_game, "version.dll");
        File.WriteAllText(stray, "another mod");

        var record = await NewInstaller().InstallAsync(
            Request(conflicts: [new Conflict(stray, ConflictKind.InjectionDll, "stray", true)]),
            null, default);

        var backup = Assert.Single(record.BackupDirectories);
        Assert.Equal(Path.Combine(_game, "AMDNR_backup"), Path.GetDirectoryName(backup));
        Assert.Equal("another mod", File.ReadAllText(Path.Combine(backup, "version.dll")));
    }

    [Fact]
    public async Task Reinstalling_without_the_runtime_takes_the_earlier_runtime_files_back_out()
    {
        // The new record no longer lists them, so leaving them would strand ~150 MB in the
        // game folder that no uninstall could ever find again — and Doctor, told there is no
        // runtime, would stop checking the very DLLs the game is still loading.
        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);

        var second = await installer.InstallAsync(Request() with { Runtime = null }, null, default);

        Assert.All(PayloadNames.PassDlls, p => Assert.False(File.Exists(Path.Combine(_game, p)), p));
        Assert.False(File.Exists(Path.Combine(_game, PayloadNames.Weights)));
        Assert.DoesNotContain(second.Files, f => f.RelativePath == PayloadNames.Weights);
        Assert.Equal(second.Files.Count, InstallRecordStore.Load(_game)!.Files.Count);
    }

    [Fact]
    public async Task Switching_builds_leaves_nothing_of_the_earlier_build_behind()
    {
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var before = Snapshot(_game);

        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);
        var switched = await installer.InstallAsync(TheAutomaticRequest(), null, default);

        // AMDNR's forwarder has no place beside TheAutomatic's build.
        Assert.False(File.Exists(Path.Combine(_game, PayloadNames.Forwarder)));
        Assert.Equal("presr-payload", File.ReadAllText(Path.Combine(_game, "dxgi.dll")));

        // The ini AMDNR seeded was never touched, so it was ours, not the user's: it makes way
        // for the new build's defaults instead of being kept as if somebody had tuned it.
        Assert.Equal("; presr defaults", File.ReadAllText(Path.Combine(_game, PayloadNames.OptiScalerIni)));

        var result = await installer.UninstallAsync(switched, default);

        Assert.True(result.IsComplete);
        Assert.Equal(before, Snapshot(_game));
    }

    [Fact]
    public async Task Switching_builds_keeps_an_ini_the_user_changed()
    {
        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);
        File.WriteAllText(Path.Combine(_game, PayloadNames.OptiScalerIni), "; tuned in the overlay");

        await installer.InstallAsync(TheAutomaticRequest(), null, default);

        Assert.Equal("; tuned in the overlay", File.ReadAllText(Path.Combine(_game, PayloadNames.OptiScalerIni)));
    }

    [Fact]
    public async Task A_leftover_the_user_changed_is_kept_and_still_recorded()
    {
        // Not ours to delete any more — the same rule uninstall follows. Dropping it from the
        // record instead would hide it from the uninstall that should report it.
        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);
        File.WriteAllText(Path.Combine(_game, PayloadNames.Weights), "weights the user swapped in");

        var second = await installer.InstallAsync(Request() with { Runtime = null }, null, default);

        Assert.Equal("weights the user swapped in", File.ReadAllText(Path.Combine(_game, PayloadNames.Weights)));
        Assert.Contains(second.Files, f => f.RelativePath == PayloadNames.Weights);

        var result = await installer.UninstallAsync(second, default);
        Assert.Contains(PayloadNames.Weights, result.Changed);
    }

    [Fact]
    public async Task A_leftover_the_user_changed_is_moved_aside_by_a_later_install_not_overwritten()
    {
        // The install that kept it decided it was not ours any more. Recorded under the path
        // alone, the next install would take it for its own and write over it with no backup:
        // weights the user swapped in, gone because they switched the runtime back on.
        var weights = Path.Combine(_game, PayloadNames.Weights);
        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);
        File.WriteAllText(weights, "weights the user swapped in");
        await installer.InstallAsync(Request() with { Runtime = null }, null, default);

        var third = await installer.InstallAsync(Request(), null, default);

        Assert.Equal("weights-bytes", File.ReadAllText(weights));
        var backup = Assert.Single(third.BackupDirectories);
        Assert.Equal("weights the user swapped in", File.ReadAllText(Path.Combine(backup, PayloadNames.Weights)));

        await installer.UninstallAsync(third, default);

        Assert.Equal("weights the user swapped in", File.ReadAllText(weights));
    }

    [Theory]
    [InlineData(true)]     // a switch to a build that keeps it under OptiScaler\
    [InlineData(false)]    // an update to an AMDNR that no longer ships it at the root
    public async Task Taking_out_a_file_this_install_no_longer_places_puts_the_games_own_copy_back(bool switchBuild)
    {
        // Older AMDNR builds placed amd_fidelityfx_dx12.dll beside the exe, and the game's own
        // copy went into AMDNR_backup to make way. Deleting ours without putting that back
        // leaves the game with no amd_fidelityfx_dx12.dll at all until the user uninstalls —
        // and a game that imports it does not start.
        const string shared = "amd_fidelityfx_dx12.dll";
        var ownCopy = Path.Combine(_game, shared);
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        File.WriteAllText(ownCopy, "the game's own FidelityFX");
        var before = Snapshot(_game);

        var packaged = Path.Combine(_store.PathFor(_store.Amdnr), shared);
        File.WriteAllText(packaged, "an older AMDNR's FidelityFX");

        var installer = NewInstaller();
        await installer.InstallAsync(Request(), null, default);
        Assert.Equal("an older AMDNR's FidelityFX", File.ReadAllText(ownCopy));

        File.Delete(packaged);
        var second = await installer.InstallAsync(
            switchBuild ? TheAutomaticRequest() : Request(), null, default);

        Assert.Equal("the game's own FidelityFX", File.ReadAllText(ownCopy));
        Assert.DoesNotContain(second.Files, f => f.RelativePath == shared);

        // The backup held nothing else, so it goes, and the record stops pointing at it.
        Assert.Empty(second.BackupDirectories);
        Assert.False(Directory.Exists(Path.Combine(_game, "AMDNR_backup")));

        var result = await installer.UninstallAsync(second, default);

        Assert.True(result.IsComplete);
        Assert.Equal(before, Snapshot(_game));
    }

    [Fact]
    public async Task A_games_own_copy_that_cannot_be_put_back_leaves_ours_in_place()
    {
        // Deleting ours first and then failing to move the original back would leave the game
        // with neither. Ours is still byte-identical to what was placed, so it stays ours: the
        // record keeps it, and the original stays in the backup for uninstall to restore.
        const string shared = "amd_fidelityfx_dx12.dll";
        var ownCopy = Path.Combine(_game, shared);
        File.WriteAllText(ownCopy, "the game's own FidelityFX");
        File.WriteAllText(Path.Combine(_store.PathFor(_store.Amdnr), shared), "an older AMDNR's FidelityFX");

        var installer = NewInstaller();
        var first = await installer.InstallAsync(Request(), null, default);
        var original = Path.Combine(first.BackupDirectories.Single(), shared);

        InstallRecord second;
        using (new FileStream(original, FileMode.Open, FileAccess.Read, FileShare.Read))
            second = await installer.InstallAsync(TheAutomaticRequest(), null, default);

        Assert.Equal("an older AMDNR's FidelityFX", File.ReadAllText(ownCopy));
        Assert.Contains(second.Files, f => f.RelativePath == shared);
        Assert.Equal(first.BackupDirectories, second.BackupDirectories);

        var result = await installer.UninstallAsync(second, default);

        Assert.True(result.IsComplete);
        Assert.Equal("the game's own FidelityFX", File.ReadAllText(ownCopy));
    }

    [Theory]
    [InlineData("dxgi.dll")]      // the hand-made install sat under the name we installed under
    [InlineData("version.dll")]   // it sat under another name and was moved aside as a conflict
    public async Task Uninstall_does_not_switch_an_old_OptiScaler_build_back_on_and_reports_it_as_superseded(
        string handMadeName)
    {
        // The user replaced it when they pressed Install. Putting it back would switch an old
        // build on again, with nothing to show for it but a game that loads the wrong OptiScaler.
        var handMade = ProxySlotProbeTests.PlaceHandMadeProxy(_game, handMadeName);
        var oldBytes = File.ReadAllBytes(handMade);
        IReadOnlyList<Conflict> conflicts = handMadeName == "dxgi.dll"
            ? []
            : [new Conflict(handMade, ConflictKind.InjectionDll, "double injection", true)];

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll", conflicts), null, default);
        var backup = Assert.Single(record.BackupDirectories);

        var result = await installer.UninstallAsync(record, default);

        Assert.False(File.Exists(handMade));
        Assert.False(File.Exists(Path.Combine(_game, "dxgi.dll")));

        var kept = Path.Combine(backup, handMadeName);
        Assert.Equal(oldBytes, File.ReadAllBytes(kept));
        Assert.Equal([Path.GetRelativePath(_game, kept)], result.Superseded);
        Assert.StartsWith("AMDNR_backup", result.Superseded[0]);
        Assert.Empty(result.NotRestored);
        Assert.True(result.IsComplete);
    }

    [Fact]
    public async Task Uninstall_still_restores_a_non_OptiScaler_dll_such_as_ReShade()
    {
        // A real PE whose version resource names something else. Only OptiScaler is held back.
        var reshade = Path.Combine(_game, "dxgi.dll");
        File.Copy(typeof(Installer).Assembly.Location, reshade);
        var reshadeBytes = File.ReadAllBytes(reshade);

        var installer = NewInstaller();
        var record = await installer.InstallAsync(Request("dxgi.dll"), null, default);
        var result = await installer.UninstallAsync(record, default);

        Assert.Equal(reshadeBytes, File.ReadAllBytes(reshade));
        Assert.Empty(result.Superseded);
        Assert.True(result.IsComplete);
        Assert.False(Directory.Exists(Path.Combine(_game, "AMDNR_backup")));
    }

    [Fact]
    public async Task Moving_to_another_proxy_name_does_not_switch_the_old_OptiScaler_back_on()
    {
        // The reinstall stops placing winmm.dll, and the earliest backup holds a winmm.dll of
        // its own: the hand-made OptiScaler. Put back, it would load beside ours as dxgi.dll.
        var handMade = ProxySlotProbeTests.PlaceHandMadeProxy(_game, "winmm.dll");

        var installer = NewInstaller();
        var first = await installer.InstallAsync(Request("winmm.dll"), null, default);
        var second = await installer.InstallAsync(Request("dxgi.dll"), null, default);

        Assert.False(File.Exists(handMade));
        Assert.Equal(first.BackupDirectories, second.BackupDirectories);

        var result = await installer.UninstallAsync(second, default);

        Assert.False(File.Exists(handMade));
        Assert.Single(result.Superseded);
    }

    private static readonly IniSetting[] DanielBackend = [new("DlssNr", "NrBackend", "daniel")];

    [Fact]
    public async Task No_ini_setting_is_applied_without_the_runtime()
    {
        const string packaged = "[DlssNr]\r\nEnabled=false\r\nNrBackend=lmxxf\r\n";
        File.WriteAllText(_store.TheAutomaticPackagedIni, packaged);

        await NewInstaller().InstallAsync(
            TheAutomaticRequest(withRuntime: false) with { IniWhenRuntime = DanielBackend }, null, default);

        Assert.Equal(packaged, File.ReadAllText(Path.Combine(_game, PayloadNames.OptiScalerIni)));
    }

    [Fact]
    public async Task A_setting_missing_from_the_section_is_added_and_nothing_else_changes()
    {
        // LF endings, odd spacing and a comment that names the key: all of it stays as it was.
        const string packaged =
            "; NrBackend=lmxxf is what upstream ships\n[DlssNr]\nEnabled = false\n\n[Upscalers]\nDx12Upscaler=auto\n";
        File.WriteAllText(_store.TheAutomaticPackagedIni, packaged);

        await NewInstaller().InstallAsync(
            TheAutomaticRequest() with { IniWhenRuntime = DanielBackend }, null, default);

        Assert.Equal(
            "; NrBackend=lmxxf is what upstream ships\n[DlssNr]\nEnabled = false\nNrBackend=daniel\n\n[Upscalers]\nDx12Upscaler=auto\n",
            File.ReadAllText(Path.Combine(_game, PayloadNames.OptiScalerIni)));

        // Staged, never edited in place: the packaged file is what the next install starts from.
        Assert.Equal(packaged, File.ReadAllText(_store.TheAutomaticPackagedIni));
    }

    [Fact]
    public async Task An_update_still_recognises_the_staged_seed_as_ours()
    {
        // The seed in the game folder matches neither packaged ini, so only a record of the
        // staged bytes says it is ours. Without that it would be kept as if somebody had tuned
        // it, and the update's own defaults would never arrive.
        File.WriteAllText(_store.TheAutomaticPackagedIni, "[DlssNr]\r\nNrBackend=lmxxf\r\nVersion=1\r\n");
        var installer = NewInstaller();
        var request = TheAutomaticRequest() with { IniWhenRuntime = DanielBackend };
        var first = await installer.InstallAsync(request, null, default);

        File.WriteAllText(_store.TheAutomaticPackagedIni, "[DlssNr]\r\nNrBackend=lmxxf\r\nVersion=2\r\n");
        var updated = await installer.InstallAsync(
            request with { Mod = _store.TheAutomatic with { Version = "1.9.1-alpha" } }, null, default);

        var ini = Path.Combine(_game, PayloadNames.OptiScalerIni);
        Assert.Equal("[DlssNr]\r\nNrBackend=daniel\r\nVersion=2\r\n", File.ReadAllText(ini));

        var recorded = Assert.Single(updated.Files, f => f.RelativePath == PayloadNames.OptiScalerIni);
        Assert.Equal(Hashing.Sha256OfFile(ini), recorded.Sha256);
        Assert.NotEqual(first.Files.Single(f => f.RelativePath == PayloadNames.OptiScalerIni).Sha256,
            recorded.Sha256);
    }

    [Fact]
    public async Task A_repair_without_its_record_still_recognises_the_staged_seed_as_ours()
    {
        File.WriteAllText(Path.Combine(_game, "game.exe"), "the game itself");
        var before = Snapshot(_game);
        File.WriteAllText(_store.TheAutomaticPackagedIni, "[DlssNr]\r\nNrBackend=lmxxf\r\n");
        var installer = NewInstaller();
        var request = TheAutomaticRequest() with { IniWhenRuntime = DanielBackend };
        await installer.InstallAsync(request, null, default);

        InstallRecordStore.Delete(_game);

        var second = await installer.InstallAsync(request, null, default);
        Assert.Contains(second.Files, f => f.RelativePath == PayloadNames.OptiScalerIni);

        await installer.UninstallAsync(second, default);
        Assert.Equal(before, Snapshot(_game));
    }
}
