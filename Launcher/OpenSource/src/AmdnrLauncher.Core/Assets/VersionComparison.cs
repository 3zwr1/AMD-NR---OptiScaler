// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Assets;

public static class VersionComparison
{
    /// <summary>For the launcher's own version. Numeric, component by component, with '-' read
    /// as one more separator: TheAutomatic names releases &lt;OptiScaler build&gt;-&lt;runtime
    /// build&gt;, and System.Version cannot read that. A missing trailing component counts as
    /// zero, so 0.4.0 and 0.4.0.0 are the same version.
    ///
    /// The first component that is not a plain number starts a pre-release suffix (0.4.1-rc1,
    /// 1.9.1-alpha). The numbers decide first; with equal numbers a release is newer than its own
    /// pre-release, and two pre-releases compare ordinally. A string with no leading number at
    /// all is incomparable, and incomparable is never newer: an update offered on a guess would
    /// install a build nobody asked for.</summary>
    public static bool IsNewer(string candidate, string current)
    {
        var a = Parse(candidate);
        var b = Parse(current);
        if (a is null || b is null) return false;

        var (x, xSuffix) = a.Value;
        var (y, ySuffix) = b.Value;

        for (var i = 0; i < Math.Max(x.Count, y.Count); i++)
        {
            var p = i < x.Count ? x[i] : 0;
            var q = i < y.Count ? y[i] : 0;
            if (p != q) return p > q;
        }

        return (xSuffix, ySuffix) switch
        {
            (null, null) => false,
            (null, _) => true,
            (_, null) => false,
            _ => string.CompareOrdinal(xSuffix, ySuffix) > 0,
        };
    }

    private static (List<long> Numbers, string? Suffix)? Parse(string version)
    {
        if (string.IsNullOrWhiteSpace(version)) return null;

        var text = version.Trim();
        var numbers = new List<long>();
        var start = 0;

        while (start < text.Length)
        {
            var end = text.IndexOfAny(['.', '-'], start);
            var part = end < 0 ? text[start..] : text[start..end];

            if (!long.TryParse(part, System.Globalization.NumberStyles.None,
                    System.Globalization.CultureInfo.InvariantCulture, out var number))
            {
                return numbers.Count == 0 ? null : (numbers, text[start..]);
            }

            numbers.Add(number);
            if (end < 0) break;
            start = end + 1;
        }

        return numbers.Count == 0 ? null : (numbers, null);
    }
}
