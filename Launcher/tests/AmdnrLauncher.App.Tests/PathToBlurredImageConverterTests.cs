// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>The backdrop behind the window and the stage when Steam has no pre-blurred hero:
/// blurred once, small, in the converter, and drawn large — never an Effect over the whole window
/// at every redraw. What the plain converter refuses, this refuses too.</summary>
public sealed class PathToBlurredImageConverterTests : IDisposable
{
    private readonly string _root = Path.Combine(Path.GetTempPath(), "amdnr-blur-" + Guid.NewGuid().ToString("N"));
    private readonly PathToBlurredImageConverter _converter = new();

    public PathToBlurredImageConverterTests() => Directory.CreateDirectory(_root);

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

    private object? Convert(string? path, string? radius = null)
        => _converter.Convert(path, typeof(ImageSource), radius, CultureInfo.InvariantCulture);

    [Fact]
    public void The_backdrop_is_blurred_once_over_the_small_decode_and_handed_over_frozen_and_opaque()
    {
        // A hard edge — left half black, right half white — at twice the decode width, so the
        // decode itself does some work too.
        const int width = 480, height = 270;
        var pixels = new byte[width * height * 4];
        for (var y = 0; y < height; y++)
        {
            for (var x = 0; x < width; x++)
            {
                var at = (y * width + x) * 4;
                var shade = x < width / 2 ? (byte)0 : (byte)255;
                pixels[at] = shade;
                pixels[at + 1] = shade;
                pixels[at + 2] = shade;
                pixels[at + 3] = 255;
            }
        }
        var edge = Write("edge.png", PngWriter.Encode(pixels, width, height));

        var image = Assert.IsAssignableFrom<BitmapSource>(Convert(edge, "12"));

        Assert.True(image.IsFrozen);
        Assert.Equal(PathToBlurredImageConverter.DecodeWidth, image.PixelWidth);
        Assert.Equal(135, image.PixelHeight);

        var row = new byte[image.PixelWidth * 4];
        image.CopyPixels(new Int32Rect(0, image.PixelHeight / 2, image.PixelWidth, 1), row, row.Length, 0);
        byte Blue(int x) => row[x * 4];
        byte Alpha(int x) => row[x * 4 + 3];

        Assert.InRange(Blue(120), 48, 208);     // the edge, blurred to grey: the blur ran, in software
        Assert.InRange(Blue(30), 0, 8);         // far into the black, still black
        Assert.InRange(Blue(210), 247, 255);    // far into the white, still white
        Assert.All(new[] { 0, 1, 120, 238, 239 }, x => Assert.Equal(255, Alpha(x)));   // opaque to the edges
    }

    [Fact]
    public void A_radius_of_nothing_leaves_the_picture_as_it_was_and_a_bad_radius_falls_back()
    {
        var pixels = new byte[4 * 4 * 4];
        for (var i = 0; i < pixels.Length; i += 4) { pixels[i] = 0x10; pixels[i + 1] = 0x20; pixels[i + 2] = 0x30; pixels[i + 3] = 0xFF; }
        var flat = Write("flat.png", PngWriter.Encode(pixels, 4, 4));

        foreach (var radius in new[] { "0", null, "not a number", "-5" })
        {
            var image = Assert.IsAssignableFrom<BitmapSource>(Convert(flat, radius));
            var row = new byte[image.PixelWidth * 4];
            image.CopyPixels(new Int32Rect(0, 0, image.PixelWidth, 1), row, row.Length, 0);
            Assert.Equal(0x10, row[0]);
            Assert.Equal(0x20, row[1]);
            Assert.Equal(0x30, row[2]);
            Assert.Equal(0xFF, row[3]);
        }
    }

    [Fact]
    public void What_is_no_picture_is_no_backdrop_either()
    {
        byte[] headerOnly =
        [
            0xFF, 0xD8,
            0xFF, 0xC0, 0x00, 0x11, 0x08, 0x03, 0x84, 0x02, 0x58,
            0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01,
            0xFF, 0xD9,
        ];

        Assert.Null(Convert(Write("header-only.jpg", headerOnly), "12"));
        Assert.Null(Convert(Write("text.png", "not a picture"u8.ToArray())));
        Assert.Null(Convert(Path.Combine(_root, "missing.png")));
        Assert.Null(Convert(_root));
        Assert.Null(Convert(""));
        Assert.Null(Convert(null));
    }
}
