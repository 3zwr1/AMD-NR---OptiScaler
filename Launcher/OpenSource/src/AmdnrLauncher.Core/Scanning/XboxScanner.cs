// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace AmdnrLauncher.Core.Scanning;

/// <summary>Games the Xbox app installed. Each fixed drive that hosts a library has a
/// .GamingRoot file at its root naming the library folder; each game in it is a folder whose
/// Content holds MicrosoftGame.config, the Xbox app's own description of the game. Nothing
/// here writes, and nothing here throws: a drive or a game that cannot be read is simply not
/// in the list, the same way an unreadable Steam library is not.</summary>
public sealed class XboxScanner(IReadOnlyList<string>? driveRoots = null) : IGameScanner
{
    private const string ConfigName = "MicrosoftGame.config";
    private const string DefaultLibrary = "XboxGames";

    /// <summary>"RGBX", then a 4-byte version, then NUL-terminated UTF-16LE folder names. Every
    /// name after the header is read, because nothing says a drive holds only one library.
    /// Anything without the header is not a GamingRoot file, and names nothing.</summary>
    public static IReadOnlyList<string> ParseGamingRoot(ReadOnlySpan<byte> bytes)
    {
        const int header = 8;
        if (bytes.Length < header || !bytes[..4].SequenceEqual("RGBX"u8)) return [];

        var body = bytes[header..];
        return Encoding.Unicode.GetString(body[..(body.Length / 2 * 2)])
            .Split('\0', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .ToList();
    }

    public IEnumerable<GameCandidate> Scan()
    {
        var games = new List<GameCandidate>();

        foreach (var drive in driveRoots ?? FixedDrives())
        {
            foreach (var library in Libraries(drive))
                games.AddRange(GamesIn(library));
        }

        return games;
    }

    /// <summary>Every fixed drive, not only C: — the Xbox app puts a library wherever the user
    /// told it to, and this PC has games on E:.</summary>
    private static List<string> FixedDrives()
    {
        try
        {
            return DriveInfo.GetDrives()
                .Where(d => d.DriveType == DriveType.Fixed && d.IsReady)
                .Select(d => d.RootDirectory.FullName)
                .ToList();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }

    /// <summary>The folders the drive's .GamingRoot names. One that is missing or unreadable
    /// still leaves the default folder worth a look: the file is the Xbox app's bookkeeping,
    /// and the games are there whether or not it survived.</summary>
    private static List<string> Libraries(string drive)
    {
        List<string> named;
        try
        {
            var file = Path.Combine(drive, ".GamingRoot");
            named = File.Exists(file) ? [.. ParseGamingRoot(File.ReadAllBytes(file))] : [];
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            named = [];
        }

        if (named.Count == 0) named.Add(DefaultLibrary);

        return named
            .Select(name => Path.Combine(drive, name))
            .Where(Directory.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static List<GameCandidate> GamesIn(string library)
    {
        List<string> folders;
        try { folders = Directory.EnumerateDirectories(library).ToList(); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { return []; }

        var games = new List<GameCandidate>();
        foreach (var folder in folders)
        {
            if (Read(folder) is { } game) games.Add(game);
        }
        return games;
    }

    /// <summary>A folder without a config is not a game (GameSave is one the Xbox app keeps),
    /// and one whose config does not parse is skipped rather than guessed at.</summary>
    private static GameCandidate? Read(string folder)
    {
        var content = Path.Combine(folder, "Content");

        try
        {
            if (!Directory.Exists(content)) return null;

            // Matched without regard to case: Call of Duty ships MicrosoftGame.Config.
            var config = Directory.EnumerateFiles(content)
                .FirstOrDefault(f => string.Equals(Path.GetFileName(f), ConfigName, StringComparison.OrdinalIgnoreCase));
            if (config is null) return null;

            var game = Load(config).Root;
            if (game is null) return null;

            var identity = game.Descendants("Identity").FirstOrDefault();
            var executable = game.Descendants("Executable").FirstOrDefault(e => Attribute(e, "Name") is not null);
            var visuals = game.Descendants("ShellVisuals").FirstOrDefault();

            // The raw folder name last of all: one made only of ® and ™ has nothing Readable
            // keeps, and First would throw out of a scan that must never throw.
            var name = new[]
                {
                    Attribute(visuals, "DefaultDisplayName"),
                    Attribute(executable, "OverrideDisplayName"),
                    Path.GetFileName(folder),
                }
                .Select(Readable)
                .FirstOrDefault(n => n is not null)
                ?? Path.GetFileName(folder);

            return new GameCandidate(name, GameStore.Xbox, content,
                StoreId: Attribute(identity, "Name"),
                ExeHint: Attribute(executable, "Name"));
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or XmlException)
        {
            return null;
        }
    }

    /// <summary>The config comes from a game folder anyone can write to, so it gets no DTD: an
    /// entity expansion there would be a way to make every launcher start hang.</summary>
    private static XDocument Load(string path)
    {
        var settings = new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null };
        using var reader = XmlReader.Create(path, settings);
        return XDocument.Load(reader);
    }

    private static string? Attribute(XElement? element, string name)
        => element?.Attribute(name)?.Value is { Length: > 0 } value ? value : null;

    /// <summary>A name a person can read, or null. The ® and ™ go (Call of Duty® Modern
    /// Warfare 3), and an "ms-resource:" reference is a key into a string table, not a name.</summary>
    private static string? Readable(string? name)
    {
        if (name is null || name.StartsWith("ms-resource:", StringComparison.OrdinalIgnoreCase)) return null;

        var cleaned = name.Replace("®", "").Replace("™", "").Trim();
        return cleaned.Length > 0 ? cleaned : null;
    }
}
