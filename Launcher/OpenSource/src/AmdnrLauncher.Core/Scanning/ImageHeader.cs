// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Buffers.Binary;

namespace AmdnrLauncher.Core.Scanning;

public enum ImageFormat { Png, Jpeg }

/// <summary>What a picture's header says about it: its format, its size in pixels, and whether
/// it carries transparency. A Steam logo is a PNG with alpha; every other Steam picture is an
/// opaque JPEG whose size says what it is.</summary>
public readonly record struct ImageInfo(ImageFormat Format, int Width, int Height, bool HasAlpha);

/// <summary>Reads a picture's size out of its header without decoding any of it. A Steam cache
/// holds a few files per game under names that say nothing (content hashes), and the launcher
/// has a thousand of them to look through on this PC: decoding each to learn its size would
/// mean decoding the whole library at every scan. The PNG size sits in the first chunk; the
/// JPEG size sits in the frame header, which follows a handful of segments the reader steps
/// over by their declared length.
///
/// <para>Nothing here throws for a file that is not a picture, is cut short, or lies about a
/// length: it reads as nothing, and nothing is drawn for it. A file that cannot be opened
/// reads the same way.</para></summary>
public static class ImageHeader
{
    /// <summary>A JPEG carries a few segments before its frame header; a photo with a large EXIF
    /// block and several tables is still under this. A file that has not shown a frame by then
    /// is not one the launcher can size, whatever it is.</summary>
    private const int MaxJpegSegments = 64;

    /// <summary>A palette PNG says whether it is transparent in a chunk that can sit anywhere
    /// before the picture data. PLTE, tRNS and a few text chunks are all that is ever there.</summary>
    private const int MaxPngChunks = 32;

    /// <summary>Longer than any header chunk a real encoder writes before the picture data. A
    /// length past this is a corrupt file, and the reader stops rather than seeking by it.</summary>
    private const int MaxPngChunkLength = 1 << 20;

    public static ImageInfo? Read(string path)
    {
        try
        {
            using var stream = new FileStream(path, FileMode.Open, FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete, bufferSize: 4096);
            return Read(stream);
        }
        // A file the Steam client is writing right now, an empty name, a folder given as a
        // file, or a drive that is gone: none of them is a picture, and none is an error the
        // scan should hear about. UnauthorizedAccessException is not an IOException.
        catch (Exception e) when (e is IOException or UnauthorizedAccessException
                                    or ArgumentException or NotSupportedException)
        {
            return null;
        }
    }

    /// <summary>The header of the picture the stream holds, or null when it holds none the
    /// reader knows. The stream is read from its current position and must be seekable, which
    /// a file and a memory stream both are.</summary>
    public static ImageInfo? Read(Stream stream)
    {
        var start = stream.Position;
        var magic = new byte[8];
        if (!Fill(stream, magic)) return null;

        if (magic.AsSpan().SequenceEqual(PngSignature)) return ReadPng(stream);

        if (magic[0] == 0xFF && magic[1] == 0xD8)
        {
            // Back to the first segment: the eight bytes taken were two of signature and six of
            // whatever follows, and the JPEG reader walks segments from just after the signature.
            stream.Seek(start + 2, SeekOrigin.Begin);
            return ReadJpeg(stream);
        }

        return null;
    }

    private static ReadOnlySpan<byte> PngSignature => [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];

    /// <summary>IHDR first, by the specification: width, height, bit depth, colour type. Colour
    /// types 4 and 6 carry an alpha channel. A palette picture (type 3) is transparent only when
    /// a tRNS chunk follows, which has to come before the picture data.</summary>
    private static ImageInfo? ReadPng(Stream stream)
    {
        var chunk = new byte[8];
        if (!Fill(stream, chunk)) return null;

        var length = BinaryPrimitives.ReadUInt32BigEndian(chunk);
        if (length != 13 || !chunk.AsSpan(4).SequenceEqual("IHDR"u8)) return null;

        var ihdr = new byte[13 + 4];   // the data and its CRC
        if (!Fill(stream, ihdr)) return null;

        var width = BinaryPrimitives.ReadUInt32BigEndian(ihdr.AsSpan(0));
        var height = BinaryPrimitives.ReadUInt32BigEndian(ihdr.AsSpan(4));
        if (width == 0 || height == 0 || width > int.MaxValue || height > int.MaxValue) return null;

        var colorType = ihdr[9];
        var hasAlpha = colorType is 4 or 6;

        if (colorType == 3) hasAlpha = HasTransparencyChunk(stream);

        return new ImageInfo(ImageFormat.Png, (int)width, (int)height, hasAlpha);
    }

    private static bool HasTransparencyChunk(Stream stream)
    {
        var header = new byte[8];
        for (var i = 0; i < MaxPngChunks; i++)
        {
            if (!Fill(stream, header)) return false;

            var length = BinaryPrimitives.ReadUInt32BigEndian(header);
            var type = header.AsSpan(4);
            if (type.SequenceEqual("tRNS"u8)) return true;
            if (type.SequenceEqual("IDAT"u8) || type.SequenceEqual("IEND"u8)) return false;
            if (length > MaxPngChunkLength) return false;

            if (!Skip(stream, length + 4L)) return false;   // the data and its CRC
        }

        return false;
    }

    /// <summary>Segments, each a marker and a big-endian length that counts itself. The frame
    /// header — any SOFn marker — holds the precision, then the height, then the width. A scan
    /// header (SOS) or the end marker before any frame means there is no size to read.</summary>
    private static ImageInfo? ReadJpeg(Stream stream)
    {
        var pair = new byte[2];
        for (var i = 0; i < MaxJpegSegments; i++)
        {
            var first = stream.ReadByte();
            if (first != 0xFF) return null;

            // Fill bytes: any number of 0xFF may precede a marker.
            int marker;
            do { marker = stream.ReadByte(); } while (marker == 0xFF);
            if (marker < 0) return null;

            // Markers that carry no length: start of image, restart markers, temporary.
            if (marker is 0xD8 or 0x01 || marker is >= 0xD0 and <= 0xD7) continue;

            // End of image, or the picture data itself: no frame header was seen.
            if (marker is 0xD9 or 0xDA) return null;

            if (!Fill(stream, pair)) return null;
            var length = BinaryPrimitives.ReadUInt16BigEndian(pair);
            if (length < 2) return null;

            if (IsFrameHeader(marker))
            {
                // Precision (1), height (2), width (2).
                var frame = new byte[5];
                if (length < 2 + frame.Length || !Fill(stream, frame)) return null;

                var height = BinaryPrimitives.ReadUInt16BigEndian(frame.AsSpan(1));
                var width = BinaryPrimitives.ReadUInt16BigEndian(frame.AsSpan(3));
                if (width == 0 || height == 0) return null;

                return new ImageInfo(ImageFormat.Jpeg, width, height, HasAlpha: false);
            }

            if (!Skip(stream, length - 2)) return null;
        }

        return null;
    }

    /// <summary>SOF0 through SOF15, less the ones that are not frame headers at all: DHT (C4),
    /// JPG (C8) and DAC (CC) share the range.</summary>
    private static bool IsFrameHeader(int marker)
        => marker is >= 0xC0 and <= 0xCF && marker is not (0xC4 or 0xC8 or 0xCC);

    private static bool Fill(Stream stream, byte[] buffer)
    {
        var read = 0;
        while (read < buffer.Length)
        {
            var n = stream.Read(buffer, read, buffer.Length - read);
            if (n <= 0) return false;
            read += n;
        }

        return true;
    }

    /// <summary>Moves past <paramref name="count"/> bytes, and says so only when they exist: a
    /// declared length that runs past the end of the file is the file lying, not a size.</summary>
    private static bool Skip(Stream stream, long count)
    {
        if (count < 0 || stream.Position + count > stream.Length) return false;

        stream.Seek(count, SeekOrigin.Current);
        return true;
    }
}
