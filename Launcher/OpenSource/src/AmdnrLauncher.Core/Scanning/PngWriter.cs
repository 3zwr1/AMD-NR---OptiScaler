// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;

namespace AmdnrLauncher.Core.Scanning;

/// <summary>Writes a picture as an 8-bit RGBA PNG. Core has no image library and takes none:
/// the one picture it ever produces is an exe's icon, which GDI hands over as bare pixels and the
/// view needs as a file it can bind to. The format is small — a signature, three chunks, zlib —
/// and the runtime already has zlib.</summary>
public static class PngWriter
{
    private static readonly uint[] CrcTable = BuildCrcTable();

    /// <summary>The PNG of <paramref name="width"/> × <paramref name="height"/> pixels given as
    /// top-down BGRA, which is how GDI lays a 32-bit DIB out.</summary>
    public static byte[] Encode(ReadOnlySpan<byte> bgra, int width, int height)
    {
        if (width <= 0 || height <= 0 || bgra.Length != width * height * 4)
            throw new ArgumentException("The pixel buffer does not match the size given.", nameof(bgra));

        // One filter byte (0: none) in front of each line, and the channels in PNG order.
        var stride = width * 4;
        var raw = new byte[height * (1 + stride)];
        for (var y = 0; y < height; y++)
        {
            var from = y * stride;
            var to = y * (1 + stride) + 1;
            for (var x = 0; x < width; x++)
            {
                raw[to + x * 4 + 0] = bgra[from + x * 4 + 2];   // R
                raw[to + x * 4 + 1] = bgra[from + x * 4 + 1];   // G
                raw[to + x * 4 + 2] = bgra[from + x * 4 + 0];   // B
                raw[to + x * 4 + 3] = bgra[from + x * 4 + 3];   // A
            }
        }

        var ihdr = new byte[13];
        BinaryPrimitives.WriteUInt32BigEndian(ihdr.AsSpan(0), (uint)width);
        BinaryPrimitives.WriteUInt32BigEndian(ihdr.AsSpan(4), (uint)height);
        ihdr[8] = 8;    // bit depth
        ihdr[9] = 6;    // colour type: RGBA
        // compression 0, filter 0, interlace 0

        using var output = new MemoryStream();
        output.Write([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]);
        WriteChunk(output, "IHDR", ihdr);
        WriteChunk(output, "IDAT", Deflate(raw));
        WriteChunk(output, "IEND", []);
        return output.ToArray();
    }

    /// <summary>A zlib stream — header, deflate, Adler-32 — which is what IDAT holds.</summary>
    private static byte[] Deflate(byte[] raw)
    {
        using var compressed = new MemoryStream();
        using (var zlib = new ZLibStream(compressed, CompressionLevel.Optimal, leaveOpen: true))
            zlib.Write(raw);
        return compressed.ToArray();
    }

    private static void WriteChunk(Stream output, string type, byte[] data)
    {
        var typeBytes = Encoding.ASCII.GetBytes(type);
        var length = new byte[4];
        BinaryPrimitives.WriteUInt32BigEndian(length, (uint)data.Length);

        var crc = new byte[4];
        BinaryPrimitives.WriteUInt32BigEndian(crc, Crc32(typeBytes, data));

        output.Write(length);
        output.Write(typeBytes);
        output.Write(data);
        output.Write(crc);
    }

    /// <summary>The CRC-32 the specification asks for, over the chunk type and its data.</summary>
    private static uint Crc32(byte[] type, byte[] data)
    {
        var crc = 0xFFFFFFFFu;
        foreach (var b in type) crc = CrcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
        foreach (var b in data) crc = CrcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
        return ~crc;
    }

    private static uint[] BuildCrcTable()
    {
        var table = new uint[256];
        for (uint n = 0; n < 256; n++)
        {
            var c = n;
            for (var k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[n] = c;
        }

        return table;
    }
}
