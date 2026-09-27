// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;

namespace AmdnrLauncher.App;

/// <summary>
/// Paints the window's title bar dark to match the rest of the launcher.
///
/// WPF does not own the title bar — the desktop window manager draws it, and it follows
/// the system's app theme, not the window's Background. On a machine set to the light
/// theme that leaves a white caption bar sitting on top of a dark window. The alternative
/// is <c>WindowStyle="None"</c> with a hand-drawn caption, which means re-implementing
/// drag, snap, maximise, the resize borders and the system menu — a large amount of
/// fragile code to own, for a cosmetic result this achieves in one call.
/// </summary>
internal static class DarkTitleBar
{
    [DllImport("dwmapi.dll", CharSet = CharSet.Unicode)]
    private static extern int DwmSetWindowAttribute(
        IntPtr hwnd, int attribute, ref int value, int size);

    /// <summary>Windows 10 2004 and later, including Windows 11.</summary>
    private const int UseImmersiveDarkMode = 20;

    /// <summary>Windows 10 1809 to 1909 used a different, undocumented number for the same
    /// attribute. Trying it second costs one failed call on the builds that take 20.</summary>
    private const int UseImmersiveDarkModeBefore20H1 = 19;

    /// <summary>Call from the window's SourceInitialized handler: the HWND does not exist
    /// before then, and after the window is shown the caption has already been painted
    /// light once, which the user sees as a flash.</summary>
    public static void Apply(Window window)
    {
        if (new WindowInteropHelper(window).Handle is var handle && handle == IntPtr.Zero) return;

        var enabled = 1;

        // Failure is expected and fine: on Windows 8.1, on a Server SKU without dwmapi, or
        // with the desktop composition disabled, the launcher simply keeps the system's
        // title bar. A cosmetic call must never be able to stop the app from starting.
        try
        {
            if (DwmSetWindowAttribute(handle, UseImmersiveDarkMode, ref enabled, sizeof(int)) != 0)
                DwmSetWindowAttribute(handle, UseImmersiveDarkModeBefore20H1, ref enabled, sizeof(int));
        }
        catch (DllNotFoundException) { }
        catch (EntryPointNotFoundException) { }
    }
}
