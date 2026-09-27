// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text;

namespace AmdnrLauncher.Core.Scanning;

public sealed class VdfNode
{
    private readonly Dictionary<string, VdfNode> _children =
        new(StringComparer.OrdinalIgnoreCase);

    public string? Value { get; init; }
    public IReadOnlyDictionary<string, VdfNode> Children => _children;
    public VdfNode? this[string key] => _children.GetValueOrDefault(key);

    internal void Add(string key, VdfNode child) => _children[key] = child;
}

/// <summary>Minimal reader for Valve KeyValues text (libraryfolders.vdf, *.acf).</summary>
public static class VdfReader
{
    public static VdfNode Parse(string text)
    {
        var i = 0;
        var root = new VdfNode();
        ParseInto(text, ref i, root);
        return root;
    }

    private static void ParseInto(string s, ref int i, VdfNode parent)
    {
        while (true)
        {
            SkipTrivia(s, ref i);
            if (i >= s.Length || s[i] == '}') { i++; return; }

            var key = ReadToken(s, ref i);
            SkipTrivia(s, ref i);
            if (i >= s.Length) return;

            if (s[i] == '{')
            {
                i++;
                var child = new VdfNode();
                ParseInto(s, ref i, child);
                parent.Add(key, child);
            }
            else
            {
                parent.Add(key, new VdfNode { Value = ReadToken(s, ref i) });
            }
        }
    }

    private static void SkipTrivia(string s, ref int i)
    {
        while (i < s.Length)
        {
            if (char.IsWhiteSpace(s[i])) { i++; continue; }
            if (s[i] == '/' && i + 1 < s.Length && s[i + 1] == '/')
            {
                while (i < s.Length && s[i] != '\n') i++;
                continue;
            }
            return;
        }
    }

    private static string ReadToken(string s, ref int i)
    {
        var quoted = s[i] == '"';
        if (quoted) i++;

        var sb = new StringBuilder();
        while (i < s.Length)
        {
            var c = s[i];
            if (quoted && c == '"') { i++; break; }
            if (!quoted && (char.IsWhiteSpace(c) || c == '{' || c == '}')) break;

            if (c == '\\' && i + 1 < s.Length)
            {
                // Valve escapes backslashes in paths; \n and \t also appear.
                i++;
                sb.Append(s[i] switch { 'n' => '\n', 't' => '\t', var e => e });
            }
            else
            {
                sb.Append(c);
            }
            i++;
        }
        return sb.ToString();
    }
}
