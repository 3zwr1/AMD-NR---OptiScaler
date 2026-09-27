// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text;
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.Core.Install;

/// <summary>Sets keys in a packaged OptiScaler.ini and changes nothing else. The result is
/// hashed and compared byte for byte to decide whether an ini in a game folder is still our
/// own seed, so the edit must be deterministic and leave every other byte alone: comments, blank
/// lines, spacing, line endings, a byte-order mark, text in any language.</summary>
public static class IniSeed
{
    /// <summary>Latin-1 maps every byte to one char and back, so the text round-trips exactly
    /// whatever the file's real encoding is. Only the lines this writes are new, and those are
    /// encoded as UTF-8 before being mapped the same way.</summary>
    private static readonly Encoding Bytes = Encoding.Latin1;

    /// <summary>ASCII spacing only. Decoded as Latin-1, the continuation bytes of UTF-8 text
    /// include 0x85 and 0xA0, which .NET counts as whitespace — "voilà" would lose its last byte
    /// to a plain Trim.</summary>
    private static readonly char[] Spacing = [' ', '\t'];

    private static bool IsBlank(string text) => text.Trim(Spacing).Length == 0;

    public static byte[] Apply(byte[] ini, IReadOnlyList<IniSetting> settings)
    {
        var lines = Split(Bytes.GetString(ini));
        var newline = lines.Select(l => l.Ending).FirstOrDefault(e => e.Length > 0) ?? "\r\n";

        foreach (var setting in settings)
            Set(lines, setting, newline);

        return Bytes.GetBytes(string.Concat(lines.Select(l => l.Text + l.Ending)));
    }

    private sealed record Line(string Text, string Ending);

    private static List<Line> Split(string text)
    {
        var lines = new List<Line>();
        var start = 0;

        for (var i = 0; i < text.Length; i++)
        {
            if (text[i] is not ('\r' or '\n')) continue;

            var ending = text[i] == '\r' && i + 1 < text.Length && text[i + 1] == '\n' ? "\r\n" : text[i].ToString();
            lines.Add(new Line(text[start..i], ending));
            i += ending.Length - 1;
            start = i + 1;
        }

        if (start < text.Length) lines.Add(new Line(text[start..], ""));
        return lines;
    }

    private static void Set(List<Line> lines, IniSetting setting, string newline)
    {
        // Every assignment of the key in every section of that name: whichever one the reader
        // honours, it is ours.
        var replaced = false;
        var inSection = false;

        for (var i = 0; i < lines.Count; i++)
        {
            if (SectionName(lines[i].Text) is { } name)
            {
                inSection = IsSection(name, setting);
                continue;
            }

            if (inSection && KeyOf(lines[i].Text) is { } key &&
                string.Equals(key, setting.Key, StringComparison.OrdinalIgnoreCase))
            {
                lines[i] = lines[i] with { Text = WithValue(lines[i].Text, setting.Value) };
                replaced = true;
            }
        }

        if (replaced) return;

        var assignment = Bytes.GetString(Encoding.UTF8.GetBytes($"{setting.Key}={setting.Value}"));
        var header = lines.FindIndex(l => SectionName(l.Text) is { } n && IsSection(n, setting));

        if (header >= 0)
        {
            // After the section's last non-blank line, so the new key does not land beyond the
            // blank line that closes the section, visually inside the next one.
            var after = header;
            for (var i = header + 1; i < lines.Count && SectionName(lines[i].Text) is null; i++)
            {
                if (!IsBlank(lines[i].Text)) after = i;
            }

            Terminate(lines, after, newline);
            lines.Insert(after + 1, new Line(assignment, newline));
            return;
        }

        // No such section: appended, set off by a blank line from whatever came before.
        if (lines.Count > 0)
        {
            Terminate(lines, lines.Count - 1, newline);
            if (!IsBlank(lines[^1].Text)) lines.Add(new Line("", newline));
        }

        lines.Add(new Line($"[{setting.Section}]", newline));
        lines.Add(new Line(assignment, newline));
    }

    private static bool IsSection(string name, IniSetting setting)
        => string.Equals(name, setting.Section, StringComparison.OrdinalIgnoreCase);

    /// <summary>A last line with no ending gets one before anything is put after it.</summary>
    private static void Terminate(List<Line> lines, int index, string newline)
    {
        if (lines[index].Ending.Length == 0) lines[index] = lines[index] with { Ending = newline };
    }

    private static string? SectionName(string text)
    {
        var line = text.Trim(Spacing);
        if (line.Length < 2 || line[0] != '[') return null;

        var close = line.IndexOf(']');
        return close < 0 ? null : line[1..close].Trim(Spacing);
    }

    private static string? KeyOf(string text)
    {
        var line = text.TrimStart(Spacing);
        if (line.Length == 0 || line[0] is ';' or '#') return null;

        var separator = line.IndexOf('=');
        return separator < 0 ? null : line[..separator].Trim(Spacing);
    }

    /// <summary>The key as the file spells it, and the spacing after the '=' if there was any.</summary>
    private static string WithValue(string text, string value)
    {
        var separator = text.IndexOf('=');
        var rest = text[(separator + 1)..];
        var spacing = rest[..(rest.Length - rest.TrimStart(Spacing).Length)];

        return text[..(separator + 1)] + spacing + Bytes.GetString(Encoding.UTF8.GetBytes(value));
    }
}
