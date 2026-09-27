// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Runtime.InteropServices;

namespace AmdnrLauncher.Core.Scanning;

/// <summary>The picture of last resort: the game's own icon, out of its exe, as a PNG the view
/// can bind to. Every Windows program carries one, so a game from no store, a folder added by
/// hand, or a Steam game whose art the CDN does not have still gets a tile that is recognisably
/// its own rather than an initial on a coloured plate.
///
/// <para>Windows does the extraction (PrivateExtractIconsW picks the best frame and scales it to
/// the size asked for), GDI hands the pixels over, and <see cref="PngWriter"/> writes them. No
/// UI assembly is involved: Core stays free of them, and this runs on a pool thread during the
/// scan. Nothing here throws — an exe with no icon, a file that is not an exe, a name that is a
/// folder or does not exist, a folder that cannot be written: each is false and no file.</para></summary>
public static class ExeIcon
{
    /// <summary>The largest frame a modern icon carries. Asked for at this size, Windows returns
    /// that frame as it is and scales a smaller icon up; the view draws it at 42 px and, blurred,
    /// as the backdrop, so a scaled-up 32 px icon still reads.</summary>
    public const int DefaultSize = 256;

    /// <summary>Writes the exe's icon at <paramref name="size"/> pixels square as a PNG at
    /// <paramref name="destinationPath"/>, through a .part file so a reader never sees half of
    /// one. False, and nothing written, when there is no icon to write or it could not be.</summary>
    public static bool ExtractPng(string exePath, string destinationPath, int size = DefaultSize)
    {
        if (!OperatingSystem.IsWindows()) return false;
        if (string.IsNullOrEmpty(exePath) || string.IsNullOrEmpty(destinationPath) || size <= 0) return false;

        try
        {
            if (!File.Exists(exePath)) return false;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException)
        {
            return false;
        }

        byte[]? pixels;
        try
        {
            pixels = Extract(exePath, size);
        }
        // A Windows without these entry points is not one the launcher runs on, but the answer
        // to that is no icon, not a crash in the middle of a scan.
        catch (Exception e) when (e is DllNotFoundException or EntryPointNotFoundException)
        {
            return false;
        }

        if (pixels is null) return false;

        var part = destinationPath + ".part";
        try
        {
            var directory = Path.GetDirectoryName(destinationPath);
            if (!string.IsNullOrEmpty(directory)) Directory.CreateDirectory(directory);

            File.WriteAllBytes(part, PngWriter.Encode(pixels, size, size));
            File.Move(part, destinationPath, overwrite: true);
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException
                                    or ArgumentException or NotSupportedException)
        {
            try { File.Delete(part); }
            catch (Exception cleanup) when (cleanup is IOException or UnauthorizedAccessException) { }
            return false;
        }
    }

    /// <summary>The icon's pixels as top-down BGRA, <paramref name="size"/> square, or null when
    /// the file has no icon or GDI would not describe it.</summary>
    private static byte[]? Extract(string exePath, int size)
    {
        var icons = new IntPtr[1];
        var ids = new uint[1];

        // The count extracted, or all ones when the file could not be opened as an image.
        var count = PrivateExtractIconsW(exePath, 0, size, size, icons, ids, 1, 0);
        if (count == 0 || count == uint.MaxValue || icons[0] == IntPtr.Zero) return null;

        try
        {
            return Pixels(icons[0], size);
        }
        finally
        {
            DestroyIcon(icons[0]);
        }
    }

    private static byte[]? Pixels(IntPtr icon, int size)
    {
        if (!GetIconInfo(icon, out var info)) return null;

        try
        {
            // No colour bitmap means a monochrome icon: the mask alone, doubled in height. Not
            // worth drawing on a game tile.
            if (info.hbmColor == IntPtr.Zero) return null;

            if (GetObjectW(info.hbmColor, Marshal.SizeOf<BITMAP>(), out var bitmap) == 0) return null;
            if (bitmap.bmWidth != size || bitmap.bmHeight != size) return null;

            var hdc = GetDC(IntPtr.Zero);
            if (hdc == IntPtr.Zero) return null;

            try
            {
                var color = ReadDib(hdc, info.hbmColor, size);
                if (color is null) return null;

                // A 32-bit icon carries its own alpha. One that does not — an older icon, or one
                // scaled from a 24-bit frame — comes through with every alpha byte zero, and its
                // transparency is in the mask: white where the screen shows through, black where
                // the icon paints.
                if (HasAlpha(color)) return color;

                var mask = info.hbmMask == IntPtr.Zero ? null : ReadDib(hdc, info.hbmMask, size);
                for (var i = 0; i < color.Length; i += 4)
                {
                    var opaque = mask is null || (mask[i] == 0 && mask[i + 1] == 0 && mask[i + 2] == 0);
                    color[i + 3] = opaque ? (byte)255 : (byte)0;
                }

                return color;
            }
            finally
            {
                ReleaseDC(IntPtr.Zero, hdc);
            }
        }
        finally
        {
            if (info.hbmColor != IntPtr.Zero) DeleteObject(info.hbmColor);
            if (info.hbmMask != IntPtr.Zero) DeleteObject(info.hbmMask);
        }
    }

    private static bool HasAlpha(byte[] bgra)
    {
        for (var i = 3; i < bgra.Length; i += 4)
        {
            if (bgra[i] != 0) return true;
        }

        return false;
    }

    /// <summary>The bitmap as a top-down 32-bit BGRA DIB, whatever depth it has: GDI converts.
    /// A negative height is how a top-down layout is asked for.</summary>
    private static byte[]? ReadDib(IntPtr hdc, IntPtr bitmap, int size)
    {
        var info = new BITMAPINFO
        {
            bmiHeader = new BITMAPINFOHEADER
            {
                biSize = (uint)Marshal.SizeOf<BITMAPINFOHEADER>(),
                biWidth = size,
                biHeight = -size,
                biPlanes = 1,
                biBitCount = 32,
                biCompression = 0,   // BI_RGB
            },
        };

        var pixels = new byte[size * size * 4];
        var lines = GetDIBits(hdc, bitmap, 0, (uint)size, pixels, ref info, 0 /* DIB_RGB_COLORS */);
        return lines > 0 ? pixels : null;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct ICONINFO
    {
        public int fIcon;
        public int xHotspot;
        public int yHotspot;
        public IntPtr hbmMask;
        public IntPtr hbmColor;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct BITMAP
    {
        public int bmType;
        public int bmWidth;
        public int bmHeight;
        public int bmWidthBytes;
        public ushort bmPlanes;
        public ushort bmBitsPixel;
        public IntPtr bmBits;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct BITMAPINFOHEADER
    {
        public uint biSize;
        public int biWidth;
        public int biHeight;
        public ushort biPlanes;
        public ushort biBitCount;
        public uint biCompression;
        public uint biSizeImage;
        public int biXPelsPerMeter;
        public int biYPelsPerMeter;
        public uint biClrUsed;
        public uint biClrImportant;
    }

    /// <summary>The header with room for the colour masks GDI may write after it.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct BITMAPINFO
    {
        public BITMAPINFOHEADER bmiHeader;
        public uint bmiColors0;
        public uint bmiColors1;
        public uint bmiColors2;
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    private static extern uint PrivateExtractIconsW(
        string lpszFile, int nIconIndex, int cxIcon, int cyIcon,
        [Out] IntPtr[] phicon, [Out] uint[] piconid, uint nIcons, uint flags);

    [DllImport("user32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DestroyIcon(IntPtr hIcon);

    [DllImport("user32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetIconInfo(IntPtr hIcon, out ICONINFO piconinfo);

    [DllImport("user32.dll", ExactSpelling = true)]
    private static extern IntPtr GetDC(IntPtr hWnd);

    [DllImport("user32.dll", ExactSpelling = true)]
    private static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);

    [DllImport("gdi32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DeleteObject(IntPtr ho);

    [DllImport("gdi32.dll", ExactSpelling = true)]
    private static extern int GetObjectW(IntPtr h, int c, out BITMAP pv);

    [DllImport("gdi32.dll", ExactSpelling = true)]
    private static extern int GetDIBits(
        IntPtr hdc, IntPtr hbm, uint start, uint cLines, byte[] lpvBits, ref BITMAPINFO lpbmi, uint usage);
}
