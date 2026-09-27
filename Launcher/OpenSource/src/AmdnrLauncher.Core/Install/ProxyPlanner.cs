// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Install;

/// <summary>The proxy name the launcher would use for a game, and why, in words the stage can
/// show as they are: "your earlier install used dbghelp.dll", "from the AMDNR list for
/// Spider-Man.exe", "re9.exe loads dxgi.dll itself when it starts", "first free name".</summary>
public sealed record ProxyChoice(string Name, string Reason)
{
    /// <summary>True when the name is first because an OptiScaler already loads through it.
    /// LauncherService reads it: with a record of its own for that name, "your earlier install"
    /// is the launcher's own install, and the row says so in those words instead.</summary>
    public bool FromEarlierInstall { get; init; }
}

public static class ProxyPlanner
{
    /// <summary>Every supported proxy name, best guess first. What decides, in order: an
    /// OptiScaler already loading through one of the names; the manifest's entry for this exe;
    /// the names the exe itself imports, which Windows will look for beside it the moment it
    /// starts, kept in the manifest's default order among themselves; then the rest in that
    /// order. (An earlier form of this rule used PayloadNames.ProxyNames order: the manifest
    /// decides here as it does for the rest of the list, and the two are the same six names
    /// today.) A name some
    /// other file already holds is demoted behind every free one but never dropped — the user
    /// may still know better — except the first two, which exist precisely for folders where
    /// that file is there. dinput8.dll is never on the list.</summary>
    public static IReadOnlyList<string> CandidateOrder(
        string exeName, string exeDirectory, Manifest manifest)
        => Rank(exeName, exeDirectory, manifest).Order;

    public static string Recommend(string exeName, string exeDirectory, Manifest manifest)
        => CandidateOrder(exeName, exeDirectory, manifest)[0];

    /// <summary>The first name in <see cref="CandidateOrder"/>, with the reason it is first.</summary>
    public static ProxyChoice Explain(string exeName, string exeDirectory, Manifest manifest)
    {
        var ranked = Rank(exeName, exeDirectory, manifest);
        return new ProxyChoice(ranked.Order[0], ranked.Reason) { FromEarlierInstall = ranked.FromEarlierInstall };
    }

    /// <summary>The next name TRY NEXT PROXY can move the installed DLL to: the first in
    /// <see cref="CandidateOrder"/> that has not been tried and has no file under it. Null when
    /// only tried or occupied names remain. An occupied name is kept in the order for the picker
    /// to grey out, but is no use here: the installer refuses to move over a file it did not
    /// place, before saving anything, so offering the name once would be offering it on every
    /// press after that — the second dead end, with the DLL stuck on whatever name came before.</summary>
    public static string? Next(
        string currentProxy,
        IReadOnlyList<string> alreadyTried,
        string exeName,
        string exeDirectory,
        Manifest manifest)
    {
        var tried = new HashSet<string>(alreadyTried, StringComparer.OrdinalIgnoreCase) { currentProxy };

        return CandidateOrder(exeName, exeDirectory, manifest)
            .FirstOrDefault(n => !tried.Contains(n) && !IsOccupied(exeDirectory, n));
    }

    private sealed record Ranking(IReadOnlyList<string> Order, string Reason, bool FromEarlierInstall);

    private static Ranking Rank(string exeName, string exeDirectory, Manifest manifest)
    {
        var defaults = manifest.ProxyDefaults.Count > 0
            ? manifest.ProxyDefaults
            : PayloadNames.ProxyNames;

        var ordered = defaults
            .Where(IsSupported)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();

        // IsSupported is applied to the override too: dinput8 must never reach the list,
        // and a manifest override is not a way around that.
        var over = manifest.ProxyOverrides
            .FirstOrDefault(o => string.Equals(o.Exe, exeName, StringComparison.OrdinalIgnoreCase)
                                 && IsSupported(o.Proxy));

        // An OptiScaler already loading through a proxy name — installed by hand, or by us —
        // outranks everything. Whoever put it there has already found a name this game loads,
        // often after trying several; taking it over replaces their build in place, where any
        // other name would move it aside and start that search again.
        var existing = ProxySlotProbe.FindOptiScalerProxy(exeDirectory);

        // The names the exe asks Windows for as it starts. Each is looked for in the game folder
        // first, so an OptiScaler under one of them loads without the game having to load that
        // DLL for any other reason. An exe that cannot be read, or has no name, imports nothing
        // as far as this is concerned, and the defaults decide as before.
        var imported = new HashSet<string>(
            exeName.Length == 0 ? [] : PeImports.Read(Path.Combine(exeDirectory, exeName)),
            StringComparer.OrdinalIgnoreCase);

        List<string> first = [];
        if (existing is not null) first.Add(existing);
        if (over is not null && !first.Contains(over.Proxy, StringComparer.OrdinalIgnoreCase)) first.Add(over.Proxy);

        // An occupied name is demoted, not dropped: the user may still know better.
        // The existing name and the override are exempt from that demotion and are prepended
        // afterwards — a folder that already holds a file of that name is precisely the case
        // each exists for (a previous attempt left one behind), so sorting it down would defeat it.
        // Among the free names, the ones the exe imports come first; the default order settles
        // the rest, and OrderBy is stable so it does.
        var tail = ordered
            .Where(n => !first.Contains(n, StringComparer.OrdinalIgnoreCase))
            .OrderBy(n => IsOccupied(exeDirectory, n) ? 1 : 0)
            .ThenBy(n => imported.Contains(n) ? 0 : 1);

        List<string> order = [.. first, .. tail];
        var top = order[0];

        var fromEarlierInstall = existing is not null && Same(top, existing);
        var reason =
            fromEarlierInstall ? $"your earlier install used {top}"
            : over is not null && Same(top, over.Proxy) ? $"from the AMDNR list for {exeName}"
            : imported.Contains(top) ? $"{exeName} loads {top} itself when it starts"
            : !IsOccupied(exeDirectory, top) ? "first free name"
            : $"every name is already taken here; {top} is first on the list";

        return new Ranking(order, reason, fromEarlierInstall);
    }

    private static bool IsOccupied(string exeDirectory, string name)
        => File.Exists(Path.Combine(exeDirectory, name));

    private static bool Same(string a, string b) => string.Equals(a, b, StringComparison.OrdinalIgnoreCase);

    /// <summary>The parser already drops anything else from a downloaded manifest; this holds
    /// for a Manifest built any other way. dinput8.dll is not on the list.</summary>
    private static bool IsSupported(string proxy) => PayloadNames.IsProxyName(proxy);
}
