// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Buffers.Binary;
using System.Text;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

/// <summary>Hand-built PNG and JPEG files: only the bytes a header reader looks at, and none of
/// the picture. Shared with ArtworkTests, which drops them into a pretend Steam cache under
/// hash names and expects them told apart by size alone.</summary>
public static class Pictures
{
    /// <summary>A PNG signature and an IHDR chunk of the given size and colour type (6 = RGBA,
    /// 4 = grey with alpha, 2 = RGB, 3 = palette), then any extra chunks and IEND. No picture
    /// data at all: nothing here decodes, and nothing here needs to.</summary>
    public static byte[] Png(int width, int height, byte colorType = 6, params (string Type, byte[] Data)[] extra)
    {
        var bytes = new List<byte> { 0x89, (byte)'P', (byte)'N', (byte)'G', 0x0D, 0x0A, 0x1A, 0x0A };

        var ihdr = new byte[13];
        BinaryPrimitives.WriteUInt32BigEndian(ihdr.AsSpan(0), (uint)width);
        BinaryPrimitives.WriteUInt32BigEndian(ihdr.AsSpan(4), (uint)height);
        ihdr[8] = 8;            // bit depth
        ihdr[9] = colorType;
        Chunk(bytes, "IHDR", ihdr);
        foreach (var (type, data) in extra) Chunk(bytes, type, data);
        Chunk(bytes, "IEND", []);
        return [.. bytes];
    }

    private static void Chunk(List<byte> into, string type, byte[] data)
    {
        var length = new byte[4];
        BinaryPrimitives.WriteUInt32BigEndian(length, (uint)data.Length);
        into.AddRange(length);
        into.AddRange(Encoding.ASCII.GetBytes(type));
        into.AddRange(data);
        into.AddRange(new byte[4]);   // CRC: not checked by a header reader
    }

    /// <summary>SOI, the given leading segments, one SOFn frame header of the given size, then
    /// EOI. Steam's art is baseline (SOF0); a phone photo is progressive (SOF2) and carries a
    /// large APP1 (EXIF) segment first, which the reader has to step over, not read.</summary>
    public static byte[] Jpeg(int width, int height, byte sof = 0xC0, params byte[][] leadingSegments)
    {
        var bytes = new List<byte> { 0xFF, 0xD8 };
        foreach (var segment in leadingSegments) bytes.AddRange(segment);

        // SOFn: length 17, precision 8, height, width, 3 components of 3 bytes each.
        bytes.AddRange([0xFF, sof, 0x00, 0x11, 0x08]);
        bytes.AddRange([(byte)(height >> 8), (byte)height, (byte)(width >> 8), (byte)width]);
        bytes.AddRange([0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01]);
        bytes.AddRange([0xFF, 0xD9]);
        return [.. bytes];
    }

    /// <summary>An APPn segment of the given payload size, as a JFIF or EXIF header is.</summary>
    public static byte[] Segment(byte marker, int payloadLength)
    {
        var length = payloadLength + 2;
        var bytes = new byte[2 + length];
        bytes[0] = 0xFF;
        bytes[1] = marker;
        bytes[2] = (byte)(length >> 8);
        bytes[3] = (byte)length;
        return bytes;
    }
}

public class ImageHeaderTests
{
    [Fact]
    public void A_PNG_header_gives_its_size_and_whether_it_has_alpha()
    {
        var rgba = ImageHeader.Read(new MemoryStream(Pictures.Png(640, 360, 6)));
        Assert.Equal(new ImageInfo(ImageFormat.Png, 640, 360, HasAlpha: true), rgba);

        var greyAlpha = ImageHeader.Read(new MemoryStream(Pictures.Png(640, 360, 4)));
        Assert.Equal(new ImageInfo(ImageFormat.Png, 640, 360, HasAlpha: true), greyAlpha);

        var rgb = ImageHeader.Read(new MemoryStream(Pictures.Png(1920, 1080, 2)));
        Assert.Equal(new ImageInfo(ImageFormat.Png, 1920, 1080, HasAlpha: false), rgb);
    }

    [Fact]
    public void A_palette_PNG_has_alpha_only_when_it_carries_a_transparency_chunk()
    {
        var opaque = ImageHeader.Read(new MemoryStream(Pictures.Png(150, 150, 3, ("PLTE", new byte[768]))));
        Assert.Equal(new ImageInfo(ImageFormat.Png, 150, 150, HasAlpha: false), opaque);

        var transparent = ImageHeader.Read(new MemoryStream(
            Pictures.Png(150, 150, 3, ("PLTE", new byte[768]), ("tRNS", new byte[256]), ("IDAT", new byte[10]))));
        Assert.Equal(new ImageInfo(ImageFormat.Png, 150, 150, HasAlpha: true), transparent);
    }

    [Fact]
    public void A_JPEG_frame_header_gives_its_size_wherever_it_sits()
    {
        var baseline = ImageHeader.Read(new MemoryStream(Pictures.Jpeg(600, 900, 0xC0, Pictures.Segment(0xE0, 14))));
        Assert.Equal(new ImageInfo(ImageFormat.Jpeg, 600, 900, HasAlpha: false), baseline);

        // A 60 KB EXIF block in front, and a progressive frame: stepped over by length, not read.
        var progressive = ImageHeader.Read(new MemoryStream(
            Pictures.Jpeg(1920, 620, 0xC2, Pictures.Segment(0xE1, 60_000), Pictures.Segment(0xDB, 65))));
        Assert.Equal(new ImageInfo(ImageFormat.Jpeg, 1920, 620, HasAlpha: false), progressive);

        var bare = ImageHeader.Read(new MemoryStream(Pictures.Jpeg(32, 32)));
        Assert.Equal(new ImageInfo(ImageFormat.Jpeg, 32, 32, HasAlpha: false), bare);

        // From wherever the stream stands, not from its start: a picture behind a prefix.
        byte[] prefix = [1, 2, 3, 4, 5];
        var offset = new MemoryStream([.. prefix, .. Pictures.Jpeg(300, 450)]) { Position = prefix.Length };
        Assert.Equal(new ImageInfo(ImageFormat.Jpeg, 300, 450, HasAlpha: false), ImageHeader.Read(offset));
        var offsetPng = new MemoryStream([.. prefix, .. Pictures.Png(640, 360)]) { Position = prefix.Length };
        Assert.Equal(new ImageInfo(ImageFormat.Png, 640, 360, HasAlpha: true), ImageHeader.Read(offsetPng));
    }

    [Fact]
    public void Anything_that_is_not_a_readable_picture_header_reads_as_nothing()
    {
        Assert.Null(ImageHeader.Read(new MemoryStream([])));
        Assert.Null(ImageHeader.Read(new MemoryStream(Encoding.ASCII.GetBytes("<html>Not Found</html>"))));
        Assert.Null(ImageHeader.Read(new MemoryStream([0xFF, 0xD8, 0xFF, 0xD9])));                 // JPEG with no frame
        Assert.Null(ImageHeader.Read(new MemoryStream(Pictures.Png(640, 360)[..20])));            // truncated IHDR
        Assert.Null(ImageHeader.Read(new MemoryStream(Pictures.Jpeg(600, 900)[..6])));            // truncated segment
        Assert.Null(ImageHeader.Read(new MemoryStream(Pictures.Png(0, 360))));                    // no picture is 0 wide

        // A segment whose length points past the end of the file.
        Assert.Null(ImageHeader.Read(new MemoryStream([0xFF, 0xD8, 0xFF, 0xE1, 0xFF, 0xFF, 0x00])));
    }

    [Fact]
    public void A_file_that_cannot_be_opened_reads_as_nothing_rather_than_throwing()
    {
        using var dir = new TempDir();
        Assert.Null(ImageHeader.Read(Path.Combine(dir.Path, "no-such.jpg")));
        Assert.Null(ImageHeader.Read(dir.Path));   // a folder
        Assert.Null(ImageHeader.Read(""));

        var real = dir.Write("art.jpg", Pictures.Jpeg(460, 215));
        Assert.Equal(new ImageInfo(ImageFormat.Jpeg, 460, 215, HasAlpha: false), ImageHeader.Read(real));
    }

    /// <summary>The sizes Steam's cache holds, as found on this PC: the 600×900 capsule is cached
    /// at 300×450, the hero at 1920×620 with its blur at 192×62, the header at 460×215, and the
    /// logo as a 640×360 PNG with alpha. The 32×32 JPEG icon beside them is nothing to show.</summary>
    [Theory]
    [InlineData(600, 900, ImageFormat.Jpeg, false, ArtKind.Tile)]
    [InlineData(300, 450, ImageFormat.Jpeg, false, ArtKind.Tile)]
    [InlineData(1920, 620, ImageFormat.Jpeg, false, ArtKind.Hero)]
    [InlineData(3840, 1240, ImageFormat.Jpeg, false, ArtKind.Hero)]
    [InlineData(460, 215, ImageFormat.Jpeg, false, ArtKind.Header)]
    [InlineData(920, 430, ImageFormat.Jpeg, false, ArtKind.Header)]
    [InlineData(192, 62, ImageFormat.Jpeg, false, ArtKind.Blur)]
    [InlineData(640, 360, ImageFormat.Png, true, ArtKind.Logo)]
    [InlineData(1280, 720, ImageFormat.Png, true, ArtKind.Logo)]
    [InlineData(600, 900, ImageFormat.Png, true, ArtKind.Tile)]
    [InlineData(1920, 620, ImageFormat.Png, true, ArtKind.Hero)]
    [InlineData(1600, 900, ImageFormat.Png, true, null)]      // too wide for a logo
    [InlineData(640, 360, ImageFormat.Png, false, null)]      // no alpha: not a wordmark
    [InlineData(640, 360, ImageFormat.Jpeg, false, null)]
    [InlineData(32, 32, ImageFormat.Jpeg, false, null)]
    [InlineData(64, 64, ImageFormat.Png, true, null)]         // an icon, not a logo
    public void Steam_art_is_told_apart_by_its_size(int width, int height, ImageFormat format, bool alpha, ArtKind? expected)
    {
        Assert.Equal(expected, Artwork.Classify(new ImageInfo(format, width, height, alpha)));
    }
}
