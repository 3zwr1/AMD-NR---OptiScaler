// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text.Json;
using System.Text.Json.Nodes;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class InstallRecordStoreTests : IDisposable
{
    private readonly TempDir _dir = new();
    private readonly string _originalRoot = InstallRecordStore.RootDirectory;

    public InstallRecordStoreTests() => InstallRecordStore.RootDirectory = _dir.Path;

    public void Dispose()
    {
        InstallRecordStore.RootDirectory = _originalRoot;
        _dir.Dispose();
    }

    private static InstallRecord Sample(string exeDirectory) => new(
        ExeDirectory: exeDirectory,
        GameName: "SILENT HILL 2",
        Proxy: "dxgi.dll",
        Source: "amdnr",
        AmdnrVersion: "0.2.1",
        RuntimeVersion: "0.3.1",
        ForwarderVersion: "0.2.1",
        BackupDirectories: [],
        InstalledAtUtc: DateTimeOffset.UnixEpoch,
        AntiCheatAcknowledged: false,
        Files: [new PlacedFile("dxgi.dll", "AA", PlacementMode.Copy)]);

    [Fact]
    public void Round_trips_a_record()
    {
        var record = Sample(@"D:\SteamLibrary\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64");

        InstallRecordStore.Save(record);
        var loaded = InstallRecordStore.Load(record.ExeDirectory);

        // Compared field by field rather than with `Assert.Equal(record, loaded)`, because a
        // record's synthesized equality treats IReadOnlyList<T> by reference, and the loaded
        // list is a different instance. Fixing that by overriding Equals on InstallRecord
        // would be production code shaped by a test — so the test does the work instead.
        Assert.NotNull(loaded);
        Assert.Equal(record.ExeDirectory, loaded!.ExeDirectory);
        Assert.Equal(record.GameName, loaded.GameName);
        Assert.Equal(record.Proxy, loaded.Proxy);
        Assert.Equal(record.Source, loaded.Source);
        Assert.Equal(record.AmdnrVersion, loaded.AmdnrVersion);
        Assert.Equal(record.RuntimeVersion, loaded.RuntimeVersion);
        Assert.Equal(record.ForwarderVersion, loaded.ForwarderVersion);
        Assert.Equal(record.BackupDirectories, loaded.BackupDirectories);
        Assert.Equal(record.InstalledAtUtc, loaded.InstalledAtUtc);
        Assert.Equal(record.AntiCheatAcknowledged, loaded.AntiCheatAcknowledged);
        Assert.Equal(record.Files, loaded.Files);   // element-wise; PlacedFile is a record
    }

    [Fact]
    public void Load_returns_null_for_an_unknown_directory()
    {
        Assert.Null(InstallRecordStore.Load(@"D:\nothing\here"));
    }

    [Fact]
    public void Two_games_get_two_records()
    {
        InstallRecordStore.Save(Sample(@"D:\a"));
        InstallRecordStore.Save(Sample(@"D:\b"));

        Assert.Equal(2, InstallRecordStore.LoadAll().Count);
    }

    [Fact]
    public void Record_keys_are_case_insensitive_like_windows_paths()
    {
        InstallRecordStore.Save(Sample(@"D:\Games\Foo"));

        Assert.NotNull(InstallRecordStore.Load(@"d:\games\FOO"));
    }

    [Fact]
    public void Delete_removes_the_record()
    {
        var record = Sample(@"D:\a");
        InstallRecordStore.Save(record);

        InstallRecordStore.Delete(record.ExeDirectory);

        Assert.Null(InstallRecordStore.Load(record.ExeDirectory));
        Assert.Empty(InstallRecordStore.LoadAll());
    }

    [Fact]
    public void A_corrupt_record_file_is_skipped_rather_than_throwing()
    {
        InstallRecordStore.Save(Sample(@"D:\a"));
        File.WriteAllText(Path.Combine(InstallRecordStore.RootDirectory, "corrupt.json"), "{ not json");

        Assert.Single(InstallRecordStore.LoadAll());
    }

    private static string RecordFile() =>
        Directory.EnumerateFiles(InstallRecordStore.RootDirectory, "*.json").Single();

    [Fact]
    public void A_record_missing_a_member_is_rejected_rather_than_read_as_null()
    {
        // System.Text.Json's parameterised-constructor path fills an absent member with null,
        // with no error — and every consumer then treats that null as a real path or name.
        // UninstallAsync would throw part-way through deleting from a real game folder.
        InstallRecordStore.Save(Sample(@"D:\a"));
        var document = File.ReadAllText(RecordFile());
        File.WriteAllText(RecordFile(), RemoveMember(document, "Proxy"));

        Assert.Null(InstallRecordStore.Load(@"D:\a"));
        Assert.Empty(InstallRecordStore.LoadAll());
    }

    [Fact]
    public void A_record_written_by_a_newer_launcher_is_treated_as_an_unknown_install()
    {
        InstallRecordStore.Save(Sample(@"D:\a"));
        File.WriteAllText(RecordFile(), File.ReadAllText(RecordFile())
            .Replace($"\"SchemaVersion\": {InstallRecord.CurrentSchemaVersion}",
                     $"\"SchemaVersion\": {InstallRecord.CurrentSchemaVersion + 1}"));

        Assert.Null(InstallRecordStore.Load(@"D:\a"));
    }

    [Fact]
    public void A_truncated_record_is_skipped_rather_than_half_read()
    {
        InstallRecordStore.Save(Sample(@"D:\a"));
        var document = File.ReadAllText(RecordFile());
        File.WriteAllText(RecordFile(), document[..(document.Length / 2)]);

        Assert.Null(InstallRecordStore.Load(@"D:\a"));
    }

    [Fact]
    public void A_save_that_fails_part_way_leaves_the_previous_record_readable()
    {
        // This record is the only thing that lets Uninstall take ~470 MB back out of a game
        // folder. Written in place, a failure part-way leaves a truncated document that reads
        // back as "nothing is installed" — so the write goes to a sibling and is then moved
        // into place, which is the one step the filesystem either does or does not do.
        InstallRecordStore.Save(Sample(@"D:\a"));
        var path = RecordFile();
        Directory.CreateDirectory(path + ".tmp");   // blocks the sibling, not the record

        Assert.ThrowsAny<UnauthorizedAccessException>(
            () => InstallRecordStore.Save(Sample(@"D:\a") with { Proxy = "winmm.dll" }));
        Assert.Equal("dxgi.dll", InstallRecordStore.Load(@"D:\a")!.Proxy);
    }

    [Fact]
    public void A_save_that_could_not_be_moved_into_place_leaves_no_sibling_behind()
    {
        // Harmless — the *.json glob ignores it — but one accumulates per failed save in a
        // directory nobody ever cleans.
        InstallRecordStore.Save(Sample(@"D:\a"));
        var path = RecordFile();

        using (new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.None))
        {
            // A move blocked by a sharing violation surfaces as UnauthorizedAccessException,
            // which does not derive from IOException.
            Assert.ThrowsAny<UnauthorizedAccessException>(
                () => InstallRecordStore.Save(Sample(@"D:\a") with { Proxy = "winmm.dll" }));
        }

        Assert.False(File.Exists(path + ".tmp"));
    }

    /// <summary>Rewrites a saved record into the shape the build before BackupDirectories
    /// wrote: one nullable BackupDirectory instead of the list.</summary>
    private static void MakeItAPreFixRecord(string backupDirectory)
    {
        var singular = backupDirectory is null ? "null" : $"\"{backupDirectory.Replace(@"\", @"\\")}\"";
        File.WriteAllText(RecordFile(), File.ReadAllText(RecordFile())
            .Replace("\"BackupDirectories\": []", $"\"BackupDirectory\": {singular}"));
    }

    [Fact]
    public void A_record_written_before_BackupDirectories_is_migrated_rather_than_rejected()
    {
        // Every tester and dev install carries one of these. Rejecting it means the launcher
        // sees no previous install at all: it moves its own payload into a fresh backup, and
        // the pre-fix backup holding the user's own conflicting file is orphaned with nothing
        // left pointing at it.
        InstallRecordStore.Save(Sample(@"D:\a"));
        MakeItAPreFixRecord(@"D:\a\AMDNR_backup\20250101-120000");

        var loaded = InstallRecordStore.Load(@"D:\a");

        Assert.NotNull(loaded);
        Assert.Equal([@"D:\a\AMDNR_backup\20250101-120000"], loaded!.BackupDirectories);
        Assert.Equal("dxgi.dll", loaded.Proxy);
        Assert.Single(InstallRecordStore.LoadAll());
    }

    [Fact]
    public void A_pre_fix_record_that_backed_nothing_up_migrates_to_no_backup_directories()
    {
        // The member was nullable: an install that found no conflict to move wrote null there.
        InstallRecordStore.Save(Sample(@"D:\a"));
        MakeItAPreFixRecord(null!);

        var loaded = InstallRecordStore.Load(@"D:\a");

        Assert.NotNull(loaded);
        Assert.Empty(loaded!.BackupDirectories);
    }

    /// <summary>Drops one member from a serialised record, the way a hand edit or a partial
    /// write from an older build would.</summary>
    /// <summary>As JSON, not as a line: the last member of the document has no trailing comma,
    /// so dropping its line would leave a document that does not parse — and a legacy record
    /// is a well-formed one that simply never had the member.</summary>
    private static string RemoveMember(string json, string name)
    {
        var document = JsonNode.Parse(json)!.AsObject();
        document.Remove(name);
        return document.ToJsonString(new JsonSerializerOptions { WriteIndented = true });
    }

    [Fact]
    public void A_record_file_that_cannot_be_read_is_skipped_rather_than_throwing()
    {
        // Load runs for every game on every scan, and %LOCALAPPDATA% is routinely synced by
        // OneDrive or swept by an AV scanner — a momentarily locked record must not crash it.
        var record = Sample(@"D:\a");
        InstallRecordStore.Save(record);
        var path = Directory.EnumerateFiles(InstallRecordStore.RootDirectory, "*.json").Single();
        using var hold = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.None);

        Assert.Null(InstallRecordStore.Load(record.ExeDirectory));
        Assert.Empty(InstallRecordStore.LoadAll());
    }

    [Fact]
    public void A_legacy_record_without_source_reads_as_amdnr()
    {
        // Every record written before the launcher offered a second build is an AMDNR
        // install. Rejecting it for the missing member would turn each of them into "nothing
        // installed", and the next install would take the launcher's own files for the user's.
        InstallRecordStore.Save(Sample(@"D:\a") with { Source = "theautomatic" });
        File.WriteAllText(RecordFile(), RemoveMember(File.ReadAllText(RecordFile()), "Source"));

        var loaded = InstallRecordStore.Load(@"D:\a");

        Assert.NotNull(loaded);
        Assert.Equal("amdnr", loaded!.Source);
    }

    [Fact]
    public void A_record_whose_source_is_null_is_rejected()
    {
        // No build ever wrote this. Only an absent member is the legacy shape; a null one is a
        // document we cannot read properly, and guessing would mean acting on it anyway.
        InstallRecordStore.Save(Sample(@"D:\a"));
        File.WriteAllText(RecordFile(), File.ReadAllText(RecordFile())
            .Replace("\"Source\": \"amdnr\"", "\"Source\": null"));

        Assert.Null(InstallRecordStore.Load(@"D:\a"));
    }

    [Fact]
    public void A_record_without_the_runtime_or_a_forwarder_is_usable()
    {
        // Both are optional now: TheAutomatic's build has no forwarder, and the runtime is the
        // user's choice. The null means "not installed", not "could not be read".
        InstallRecordStore.Save(Sample(@"D:\a") with { RuntimeVersion = null, ForwarderVersion = null });

        var loaded = InstallRecordStore.Load(@"D:\a");

        Assert.NotNull(loaded);
        Assert.Null(loaded!.RuntimeVersion);
        Assert.Null(loaded.ForwarderVersion);
    }

    [Theory]
    [InlineData("AmdnrVersion")]
    [InlineData("GameName")]
    public void Only_the_runtime_and_forwarder_versions_may_be_null(string member)
    {
        InstallRecordStore.Save(Sample(@"D:\a"));
        File.WriteAllText(RecordFile(), RemoveMember(File.ReadAllText(RecordFile()), member));

        Assert.Null(InstallRecordStore.Load(@"D:\a"));
    }

    [Fact]
    public void The_runtime_package_a_record_names_round_trips()
    {
        InstallRecordStore.Save(Sample(@"D:\a") with { RuntimeId = "runtime-040", RuntimeVersion = "0.4.0" });

        var loaded = InstallRecordStore.Load(@"D:\a");

        Assert.Equal("runtime-040", loaded!.RuntimeId);
        Assert.Equal("0.4.0", loaded.RuntimeVersion);
    }

    [Fact]
    public void A_legacy_record_with_a_runtime_and_no_RuntimeId_reads_as_the_runtime_package()
    {
        // Every record written before builds could list their runtimes placed the one package
        // the manifest then had, "runtime". Doctor checks the pass DLLs against the package the
        // record names, so a legacy record has to name that one rather than nothing.
        InstallRecordStore.Save(Sample(@"D:\a") with { RuntimeId = "runtime-033" });
        File.WriteAllText(RecordFile(), RemoveMember(File.ReadAllText(RecordFile()), "RuntimeId"));

        var loaded = InstallRecordStore.Load(@"D:\a");

        Assert.NotNull(loaded);
        Assert.Equal("runtime", loaded!.RuntimeId);
    }

    [Fact]
    public void A_record_without_the_runtime_names_no_runtime_package()
    {
        // Null for the version already means "not installed"; the id must not contradict it,
        // whether the member was written as null or, in an older record, not at all.
        InstallRecordStore.Save(Sample(@"D:\a") with { RuntimeVersion = null });
        Assert.Null(InstallRecordStore.Load(@"D:\a")!.RuntimeId);

        File.WriteAllText(RecordFile(), RemoveMember(File.ReadAllText(RecordFile()), "RuntimeId"));
        Assert.Null(InstallRecordStore.Load(@"D:\a")!.RuntimeId);
    }

    [Fact]
    public void The_names_tried_so_far_round_trip()
    {
        // TRY NEXT PROXY has to know which names it has already been through, and the workflow
        // — launch the game, see, come back — invites closing the launcher in between. A list
        // held in the window alone forgot, and offered the name that had just failed.
        InstallRecordStore.Save(Sample(@"D:\a") with { TriedProxies = ["dxgi.dll", "d3d12.dll"] });

        Assert.Equal(["dxgi.dll", "d3d12.dll"], InstallRecordStore.Load(@"D:\a")!.TriedProxies);
    }

    [Fact]
    public void A_record_written_before_tried_names_were_kept_reads_as_none_tried()
    {
        InstallRecordStore.Save(Sample(@"D:\a") with { TriedProxies = ["dxgi.dll"] });
        File.WriteAllText(RecordFile(), RemoveMember(File.ReadAllText(RecordFile()), "TriedProxies"));

        var loaded = InstallRecordStore.Load(@"D:\a");

        Assert.NotNull(loaded);
        Assert.Empty(loaded!.TriedProxies);
    }

    [Fact]
    public void A_record_whose_tried_names_are_null_or_blank_is_rejected()
    {
        // No build writes either; a document that has them is one that was edited or damaged,
        // and the rule for those is the rule for every other member.
        InstallRecordStore.Save(Sample(@"D:\a"));
        var document = JsonNode.Parse(File.ReadAllText(RecordFile()))!.AsObject();

        document["TriedProxies"] = null;
        File.WriteAllText(RecordFile(), document.ToJsonString());
        Assert.Null(InstallRecordStore.Load(@"D:\a"));

        document["TriedProxies"] = new JsonArray("dxgi.dll", "");
        File.WriteAllText(RecordFile(), document.ToJsonString());
        Assert.Null(InstallRecordStore.Load(@"D:\a"));
    }
}
