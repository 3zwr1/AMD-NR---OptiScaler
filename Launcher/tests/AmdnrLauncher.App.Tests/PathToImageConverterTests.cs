// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Globalization;
using System.IO;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>The one converter that opens files strangers' machines hold. WPF does not catch what
/// a converter throws: an exception here goes to the dispatcher's last handler, which ends the
/// launcher. So every way a picture can fail to load has to come back as null — the plate and
/// the initial — and a real picture still has to load.</summary>
public sealed class PathToImageConverterTests : IDisposable
{
    private readonly string _root = Path.Combine(Path.GetTempPath(), "amdnr-conv-" + Guid.NewGuid().ToString("N"));
    private readonly PathToImageConverter _converter = new();

    public PathToImageConverterTests() => Directory.CreateDirectory(_root);

    public void Dispose()
    {
        try { Directory.Delete(_root, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    private string Write(string name, byte[] bytes)
    {
        var path = Path.Combine(_root, name);
        File.WriteAllBytes(path, bytes);
        return path;
    }

    /// <summary>Only the bytes a header reader looks at: SOI, one SOF0 frame of the given size,
    /// EOI. Enough for the launcher's own size check to pass it, not enough to decode.</summary>
    private static byte[] Jpeg(int width, int height) =>
    [
        0xFF, 0xD8,
        0xFF, 0xC0, 0x00, 0x11, 0x08, (byte)(height >> 8), (byte)height, (byte)(width >> 8), (byte)width,
        0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01,
        0xFF, 0xD9,
    ];

    private object? Convert(string path, string? width = null)
        => _converter.Convert(path, typeof(ImageSource), width, CultureInfo.InvariantCulture);

    [Fact]
    public void A_file_whose_header_parses_but_whose_body_does_not_decode_is_no_picture_rather_than_an_exception()
    {
        // What a Steam cache file looks like while the client is still writing it, and what a
        // CDN body looks like when only its header was checked. WPF raises FileFormatException —
        // a FormatException, not a NotSupportedException — from EndInit for both.
        var headerOnly = Write("header-only.jpg", Jpeg(600, 900));
        var garbageBody = Write("garbage.jpg", [.. Jpeg(600, 900)[..^2], .. Enumerable.Repeat((byte)0x5A, 4096), 0xFF, 0xD9]);

        Assert.Null(Convert(headerOnly, "44"));
        Assert.Null(Convert(headerOnly));
        Assert.Null(Convert(garbageBody, "240"));
        Assert.Null(Convert(garbageBody));
    }

    [Fact]
    public void A_missing_file_a_folder_and_a_file_that_is_no_picture_at_all_are_no_picture_either()
    {
        Assert.Null(Convert(Path.Combine(_root, "missing.png"), "44"));
        Assert.Null(Convert(_root));
        Assert.Null(Convert(Write("text.png", "not a picture"u8.ToArray()), "44"));
        Assert.Null(Convert("relative\\path.png"));
        Assert.Null(Convert(""));
        Assert.Null(_converter.Convert(null, typeof(ImageSource), null, CultureInfo.InvariantCulture));
    }

    [Fact]
    public void A_real_picture_loads_frozen_and_at_the_width_asked_for()
    {
        // The writer Core uses for icons makes a PNG WPF decodes: 8 by 8, one colour.
        var pixels = new byte[8 * 8 * 4];
        for (var i = 0; i < pixels.Length; i += 4) { pixels[i] = 0x20; pixels[i + 1] = 0x40; pixels[i + 2] = 0x80; pixels[i + 3] = 0xFF; }
        var good = Write("good.png", PngWriter.Encode(pixels, 8, 8));

        var image = Assert.IsType<BitmapImage>(Convert(good));
        Assert.Equal(8, image.PixelWidth);
        Assert.True(image.IsFrozen);

        var small = Assert.IsType<BitmapImage>(Convert(good, "4"));
        Assert.Equal(4, small.PixelWidth);
    }
}
