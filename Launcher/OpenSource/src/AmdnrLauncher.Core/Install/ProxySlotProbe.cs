// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Diagnostics;

namespace AmdnrLauncher.Core.Install;

public enum SlotContent { Empty, OptiScaler, Other, Unreadable }

/// <summary>What sits under a proxy name in a game folder. People install OptiScaler by hand
/// long before they find this launcher, and a hand-made install leaves no record — only a DLL
/// under dxgi.dll or winmm.dll whose version resource still says what it really is. Every
/// OptiScaler build is compiled as OptiScaler.dll and keeps that OriginalFilename whatever it
/// is renamed to; ReShade, DXVK and the game's own DLLs say something else.</summary>
public static class ProxySlotProbe
{
    public static SlotContent Classify(string path)
    {
        if (!File.Exists(path)) return SlotContent.Empty;

        try
        {
            // FileVersionInfo reports a file it cannot open as one with no version resource,
            // which would read as "some other DLL". Opening it first is what tells a file the
            // running game or an antivirus holds apart from one that really is something else.
            using (new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete)) { }

            var original = FileVersionInfo.GetVersionInfo(path).OriginalFilename;
            return string.Equals(original?.Trim(), PayloadNames.OptiScalerDll, StringComparison.OrdinalIgnoreCase)
                ? SlotContent.OptiScaler
                : SlotContent.Other;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return SlotContent.Unreadable;
        }
    }

    /// <summary>The proxy name an OptiScaler build already loads through, in the launcher's own
    /// order. dinput8.dll is not among them: it is never a name this launcher installs under,
    /// so an OptiScaler found there is not one it can take over.</summary>
    public static string? FindOptiScalerProxy(string exeDirectory)
        => PayloadNames.ProxyNames.FirstOrDefault(
            name => Classify(Path.Combine(exeDirectory, name)) == SlotContent.OptiScaler);

    /// <summary>The proxy names some other file holds in this folder — ReShade's dxgi.dll, a
    /// DXVK build, the game's own copy — or a file that could not be read, which proves nothing
    /// either way. These are the names the stage greys out: switching an install onto one would
    /// destroy that file, and Core refuses it, so the picker says so first.
    ///
    /// A name another OptiScaler loads through is deliberately not among them: on a folder
    /// with somebody's hand-made install and no record, that name is the one INSTALL takes
    /// over, and it has to stay pickable after the user has tried another on the stage. On an
    /// installed game the same name is refused when picked — the switch will not move over any
    /// file it did not place — and the picker snaps back with the installer's own words.</summary>
    public static IReadOnlyList<string> FindOccupied(string exeDirectory)
        => PayloadNames.ProxyNames
            .Where(name => Classify(Path.Combine(exeDirectory, name)) is SlotContent.Other or SlotContent.Unreadable)
            .ToList();
}
