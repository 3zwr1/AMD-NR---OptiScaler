// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace AmdnrLauncher.Core.Install;

public sealed record PlacedFile(string RelativePath, string Sha256, PlacementMode Mode)
{
    /// <summary>True for a file an earlier install placed, that a later install stopped placing
    /// and could not take out because it had changed since, or could not be read. It stays in
    /// the record so uninstall can report it, but it is the user's now: an install that places
    /// that path again must move it aside rather than write over it. Absent in records written
    /// before this member existed, which is right — those only ever listed files as placed.</summary>
    public bool Disowned { get; init; }
}

public sealed record InstallRecord(
    string ExeDirectory,
    string GameName,
    string Proxy,
    string Source,
    string AmdnrVersion,
    string? RuntimeVersion,
    string? ForwarderVersion,
    IReadOnlyList<string> BackupDirectories,
    DateTimeOffset InstalledAtUtc,
    bool AntiCheatAcknowledged,
    IReadOnlyList<PlacedFile> Files)
{
    /// <summary>Bumped when a member changes meaning rather than merely being added. A record
    /// stamped higher than this is one written by a launcher that knows something this build
    /// does not, so this build must not act on it — guessing would mean deleting files from a
    /// game folder on the strength of a document it cannot read properly.</summary>
    public const int CurrentSchemaVersion = 1;

    public int SchemaVersion { get; init; } = CurrentSchemaVersion;

    /// <summary>False between the moment the record is written and the moment the last payload
    /// file lands. A record that survives with this still false is one whose install died
    /// part-way with no chance to roll back — a power cut, a killed process — so the game
    /// folder holds an unknown fraction of the payload. Doctor reports that.
    ///
    /// It defaults to true because the only writer that sets it false sets it explicitly; the
    /// default is what a document written before this member existed reads back as, and such a
    /// document could only have been written by the old code, which wrote on success alone.</summary>
    public bool Completed { get; init; } = true;

    /// <summary>The manifest package the runtime files came from — runtime-033, runtime-040 —
    /// or null when no runtime was placed. Doctor checks the pass DLLs against this package's
    /// accepted hashes, so it has to be the one that was actually installed, not whichever the
    /// manifest would pick for this GPU today. Absent in records written before builds could
    /// list their runtimes; InstallRecordStore reads those as the one package there was then.</summary>
    public string? RuntimeId { get; init; }

    /// <summary>The proxy names this install has already been through, so TRY NEXT PROXY can
    /// carry on where it left off. The documented workflow — press, launch the game, look, come
    /// back — invites closing the launcher in between, and a list held in the window alone
    /// forgot and offered the name that had just failed. Once every name has been tried the
    /// list is cleared and the round starts again: there is no dead end. Empty for a record
    /// written before the member existed, which is also what such a record means.</summary>
    public IReadOnlyList<string> TriedProxies { get; init; } = [];
}

public static class InstallRecordStore
{
    public static string RootDirectory { get; set; } = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDNR", "records");

    private static readonly JsonSerializerOptions Options = new() { WriteIndented = true };

    /// <summary>The build every record predating the Source member was installed from.</summary>
    public const string LegacySource = "amdnr";

    /// <summary>The runtime package every record predating the RuntimeId member placed, when it
    /// placed one: the only runtime package a manifest had before builds listed theirs.</summary>
    public const string LegacyRuntimeId = Assets.SourceInfo.LegacyRuntimePackage;

    /// <summary>Windows paths are case-insensitive, so the key is too.</summary>
    private static string KeyFor(string exeDirectory)
    {
        var normalized = Path.TrimEndingDirectorySeparator(exeDirectory).ToLowerInvariant();
        var hash = SHA256.HashData(Encoding.UTF8.GetBytes(normalized));
        return Convert.ToHexString(hash)[..16] + ".json";
    }

    private static string PathFor(string exeDirectory) => Path.Combine(RootDirectory, KeyFor(exeDirectory));

    public static void Save(InstallRecord record)
    {
        Directory.CreateDirectory(RootDirectory);

        // This record is the only thing that lets Uninstall take ~470 MB back out of a game
        // folder. Written in place, a failure part-way through — a full disk, a power cut —
        // leaves a truncated document that reads back as "nothing is installed", and the user
        // keeps files the launcher can no longer remove. Write beside it and move: the move is
        // the one step the filesystem either does or does not do.
        var path = PathFor(record.ExeDirectory);
        var temp = path + ".tmp";

        try
        {
            File.WriteAllText(temp, JsonSerializer.Serialize(record, Options));
            File.Move(temp, path, overwrite: true);
        }
        finally
        {
            // A move that did not happen leaves the sibling behind. The *.json glob ignores it,
            // so it costs nothing but disk — and one accumulates per failed save in a directory
            // nobody ever cleans. The delete is best effort for the same reason the write was
            // attempted at all: the caller needs the real failure, not one from the tidying.
            try { File.Delete(temp); }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
        }
    }

    /// <summary>Whether a deserialised record can be acted on. Every string here is declared
    /// non-nullable, but System.Text.Json's parameterised-constructor path fills an absent
    /// member with null and reports nothing — so a hand-edited or half-written document
    /// produces a record that throws only later, part-way through deleting files from someone's
    /// game folder. A record we cannot fully read is an unknown install, not a usable one.</summary>
    private static bool IsUsable(InstallRecord? record) =>
        record is not null
        && record.SchemaVersion <= InstallRecord.CurrentSchemaVersion
        && !string.IsNullOrEmpty(record.ExeDirectory)
        && !string.IsNullOrEmpty(record.GameName)
        && !string.IsNullOrEmpty(record.Proxy)
        && !string.IsNullOrEmpty(record.Source)
        && !string.IsNullOrEmpty(record.AmdnrVersion)
        // Null here is a statement, not a gap: the runtime is the user's choice and not every
        // build has a forwarder. An empty string is neither, so it is still refused.
        && (record.RuntimeVersion is null || record.RuntimeVersion.Length > 0)
        && (record.RuntimeId is null || record.RuntimeId.Length > 0)
        && (record.ForwarderVersion is null || record.ForwarderVersion.Length > 0)
        && record.BackupDirectories is not null
        && record.BackupDirectories.All(d => !string.IsNullOrEmpty(d))
        && record.TriedProxies is not null
        && record.TriedProxies.All(p => !string.IsNullOrEmpty(p))
        && record.Files is not null
        && record.Files.All(f => f is not null
                                 && !string.IsNullOrEmpty(f.RelativePath)
                                 && !string.IsNullOrEmpty(f.Sha256));

    /// <summary>A record written before BackupDirectories replaced a single nullable
    /// BackupDirectory deserialises with the list absent, which IsUsable — rightly — rejects.
    /// Every record written by every build before that one is in this shape, so rejecting them
    /// means an ordinary update turns every existing install into "no previous install",
    /// orphaning the backup that holds the user's own conflicting file. Read the old member
    /// instead; the next Save writes the record back in the current shape.</summary>
    private static InstallRecord? MigrateFromBackupDirectory(InstallRecord? record, string document)
    {
        if (record is null || record.BackupDirectories is not null) return record;

        using var parsed = JsonDocument.Parse(document);
        if (!parsed.RootElement.TryGetProperty("BackupDirectory", out var legacy)) return record;

        return legacy.ValueKind switch
        {
            JsonValueKind.Null => record with { BackupDirectories = [] },
            JsonValueKind.String => record with { BackupDirectories = [legacy.GetString()!] },
            _ => record,
        };
    }

    /// <summary>Every record written before the launcher offered a second build is an AMDNR
    /// install, and has no Source member. Rejecting it for that would turn each of them into
    /// "nothing installed", and the next install would take the launcher's own files for the
    /// user's. Only an absent member is that legacy shape: a Source written as null is a
    /// document no build produced, and stays unusable.</summary>
    private static InstallRecord? MigrateFromNoSource(InstallRecord? record, string document)
    {
        if (record is null || record.Source is not null) return record;

        using var parsed = JsonDocument.Parse(document);
        return parsed.RootElement.ValueKind == JsonValueKind.Object &&
               !parsed.RootElement.TryGetProperty(nameof(InstallRecord.Source), out _)
            ? record with { Source = LegacySource }
            : record;
    }

    /// <summary>A record written before runtime packages had names, with a runtime in it,
    /// placed the one package the manifest then had. Doctor checks the pass DLLs against the
    /// package the record names, so left null the record would say "a runtime, from nowhere"
    /// and the check would look up nothing. Only an absent member is that legacy shape, and
    /// only a record that has a runtime at all; without one there is nothing to name.</summary>
    private static InstallRecord? MigrateFromNoRuntimeId(InstallRecord? record, string document)
    {
        if (record is null || record.RuntimeId is not null || record.RuntimeVersion is null) return record;

        using var parsed = JsonDocument.Parse(document);
        return parsed.RootElement.ValueKind == JsonValueKind.Object &&
               !parsed.RootElement.TryGetProperty(nameof(InstallRecord.RuntimeId), out _)
            ? record with { RuntimeId = LegacyRuntimeId }
            : record;
    }

    /// <summary>Null for a record this build cannot act on: absent, unreadable, unparseable,
    /// stamped by a newer launcher, or missing a member it declares non-nullable.</summary>
    private static InstallRecord? Read(string path)
    {
        // Read runs for every game on every scan. The record lives in %LOCALAPPDATA%, which is
        // OneDrive-synced and AV-swept on a real machine, so a momentarily locked or
        // permission-denied record is ordinary — and UnauthorizedAccessException is not an
        // IOException, so both have to be named.
        try
        {
            var document = File.ReadAllText(path);
            var record = MigrateFromNoRuntimeId(
                MigrateFromNoSource(
                    MigrateFromBackupDirectory(JsonSerializer.Deserialize<InstallRecord>(document), document),
                    document),
                document);

            return IsUsable(record) ? record : null;
        }
        catch (Exception e) when (e is JsonException or IOException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    public static InstallRecord? Load(string exeDirectory)
    {
        var path = PathFor(exeDirectory);
        return File.Exists(path) ? Read(path) : null;
    }

    public static void Delete(string exeDirectory)
    {
        var path = PathFor(exeDirectory);
        if (File.Exists(path)) File.Delete(path);
    }

    public static IReadOnlyList<InstallRecord> LoadAll()
    {
        if (!Directory.Exists(RootDirectory)) return [];

        var records = new List<InstallRecord>();

        List<string> files;
        try { files = Directory.EnumerateFiles(RootDirectory, "*.json").ToList(); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { return []; }

        // A record we cannot read or parse is skipped; the rest are still worth having.
        foreach (var file in files)
        {
            var record = Read(file);
            if (record is not null) records.Add(record);
        }
        return records;
    }
}
