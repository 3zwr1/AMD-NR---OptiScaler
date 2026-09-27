// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Buffers.Binary;
using System.IO.Compression;
using System.Runtime.InteropServices;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class ExeIconTests
{
    /// <summary>Real images from real linkers that are known to carry an icon: the dotnet muxer
    /// this test runs under, then notepad. The test host itself (testhost.exe) has none, which
    /// is its own test below.</summary>
    internal static string? AnExeWithAnIcon()
    {
        var runtime = RuntimeEnvironment.GetRuntimeDirectory();
        string?[] candidates =
        [
            Path.GetFullPath(Path.Combine(runtime, "..", "..", "..", "dotnet.exe")),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "dotnet", "dotnet.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "notepad.exe"),
        ];
        return candidates.FirstOrDefault(File.Exists);
    }

    [Fact]
    public void Writes_the_icon_as_a_256_px_PNG_with_alpha_for_an_exe_that_has_one()
    {
        if (AnExeWithAnIcon() is not { } exe) return;

        using var dir = new TempDir();
        var png = Path.Combine(dir.Path, "art", "exe", "icon.png");

        Assert.True(ExeIcon.ExtractPng(exe, png));
        Assert.True(File.Exists(png));
        Assert.False(File.Exists(png + ".part"));

        Assert.Equal(new ImageInfo(ImageFormat.Png, 256, 256, HasAlpha: true), ImageHeader.Read(png));

        // An icon has transparent corners and an opaque middle, so its alpha is not one value.
        var pixels = Rgba(File.ReadAllBytes(png), 256, 256);
        var alphas = new HashSet<byte>();
        for (var i = 3; i < pixels.Length; i += 4) alphas.Add(pixels[i]);
        Assert.True(alphas.Count > 1, "every pixel has the same alpha: the mask was not applied");
        Assert.Contains((byte)0, alphas);
        Assert.Contains((byte)255, alphas);
    }

    [Fact]
    public void An_exe_without_an_icon_yields_nothing_and_writes_nothing()
    {
        using var dir = new TempDir();

        // A well-formed image with no resources at all.
        var bare = dir.Write("Bare.exe", PeImportsTests.Build(pe32Plus: true, ["KERNEL32.dll"]));
        var text = dir.Write("Text.exe", "stand-in for a game");
        var png = Path.Combine(dir.Path, "icon.png");

        Assert.False(ExeIcon.ExtractPng(bare, png));
        Assert.False(ExeIcon.ExtractPng(text, png));
        Assert.False(ExeIcon.ExtractPng(Path.Combine(dir.Path, "Missing.exe"), png));
        Assert.False(ExeIcon.ExtractPng(dir.Path, png));
        Assert.False(ExeIcon.ExtractPng("", png));

        Assert.False(File.Exists(png));
        Assert.False(File.Exists(png + ".part"));
    }

    [Fact]
    public void The_test_host_is_read_without_throwing_and_a_file_exists_exactly_when_it_says_so()
    {
        // testhost.exe carries no icon; dotnet.exe does. Which one hosts the tests depends on the
        // runner, so what is held here is the contract, not the answer.
        var host = Environment.ProcessPath;
        Assert.NotNull(host);

        using var dir = new TempDir();
        var png = Path.Combine(dir.Path, "host.png");

        var extracted = ExeIcon.ExtractPng(host, png);

        Assert.Equal(extracted, File.Exists(png));
        if (extracted) Assert.Equal(new ImageInfo(ImageFormat.Png, 256, 256, HasAlpha: true), ImageHeader.Read(png));
    }

    [Fact]
    public void The_PNG_writer_round_trips_pixels_in_a_file_a_header_reader_understands()
    {
        // Two by two, BGRA as GDI hands it over: red, half-transparent green / blue, clear.
        byte[] bgra =
        [
            0, 0, 255, 255,    0, 255, 0, 128,
            255, 0, 0, 255,    0, 0, 0, 0,
        ];

        var png = PngWriter.Encode(bgra, 2, 2);

        Assert.Equal(new ImageInfo(ImageFormat.Png, 2, 2, HasAlpha: true), ImageHeader.Read(new MemoryStream(png)));
        Assert.Equal(new byte[]
        {
            255, 0, 0, 255,    0, 255, 0, 128,
            0, 0, 255, 255,    0, 0, 0, 0,
        }, Rgba(png, 2, 2));

        // Every chunk's CRC is right: a decoder that checks them — WPF does — would refuse the file otherwise.
        AssertChunkCrcs(png);
    }

    /// <summary>The picture data of a PNG written with filter 0 on every line, as RGBA bytes.</summary>
    private static byte[] Rgba(byte[] png, int width, int height)
    {
        var idat = new MemoryStream();
        var at = 8;
        while (at + 8 <= png.Length)
        {
            var length = (int)BinaryPrimitives.ReadUInt32BigEndian(png.AsSpan(at));
            var type = System.Text.Encoding.ASCII.GetString(png, at + 4, 4);
            if (type == "IDAT") idat.Write(png, at + 8, length);
            if (type == "IEND") break;
            at += 12 + length;
        }

        idat.Position = 0;
        using var inflate = new ZLibStream(idat, CompressionMode.Decompress);
        var raw = new byte[height * (1 + width * 4)];
        inflate.ReadExactly(raw);

        var rgba = new byte[width * height * 4];
        for (var y = 0; y < height; y++)
        {
            Assert.Equal(0, raw[y * (1 + width * 4)]);   // filter type: none
            Array.Copy(raw, y * (1 + width * 4) + 1, rgba, y * width * 4, width * 4);
        }

        return rgba;
    }

    private static void AssertChunkCrcs(byte[] png)
    {
        var at = 8;
        while (at + 8 <= png.Length)
        {
            var length = (int)BinaryPrimitives.ReadUInt32BigEndian(png.AsSpan(at));
            var stored = BinaryPrimitives.ReadUInt32BigEndian(png.AsSpan(at + 8 + length));
            Assert.Equal(Crc32(png.AsSpan(at + 4, 4 + length)), stored);
            at += 12 + length;
        }
    }

    private static uint Crc32(ReadOnlySpan<byte> bytes)
    {
        var crc = 0xFFFFFFFFu;
        foreach (var b in bytes)
        {
            crc ^= b;
            for (var k = 0; k < 8; k++) crc = (crc & 1) != 0 ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }

        return ~crc;
    }
}
