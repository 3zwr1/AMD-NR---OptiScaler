// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace AmdnrLauncher.App.Tests;

/// <summary>The owner's mark: the pictures the owner may drop into the build, and the icon the
/// exe carries. No picture is ever invented — without one, the wordmark is drawn.</summary>
public sealed class BrandTests : IDisposable
{
    private readonly string _root = Path.Combine(
        Path.GetTempPath(), "amdnr-brand-" + Guid.NewGuid().ToString("N"));

    private static readonly byte[] PngSignature = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];

    public BrandTests() => Directory.CreateDirectory(_root);

    public void Dispose()
    {
        try { Directory.Delete(_root, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    [Fact]
    public void A_picture_that_is_not_in_the_build_is_null_never_an_exception()
    {
        Assert.Null(Brand.TryLoad(Brand.PictureUri("Assets/brand/not-in-this-build.png")));

        // Whatever the owner has dropped in so far, asking cannot fail: the pages ask on load.
        _ = Brand.Logo;
        _ = Brand.Wordmark;
    }

    [Fact]
    public void The_Discord_server_and_the_licence_notice_are_the_owners_words()
    {
        Assert.Equal("https://discord.gg/AMDNR", Brand.DiscordUrl);
        Assert.Equal(Brand.DiscordUrl, Brand.DiscordUri.AbsoluteUri);
        Assert.Contains("all rights reserved", Brand.LicenceNotice, StringComparison.OrdinalIgnoreCase);
        Assert.Contains("read", Brand.LicenceNotice, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void Only_web_addresses_are_opened_and_nothing_is_ever_started()
    {
        // The shell would run whatever it was handed — a program's path included — so a link
        // that is not http(s) is dropped before it reaches Process.Start.
        Assert.True(Services.Links.IsWeb(Brand.DiscordUri));
        Assert.True(Services.Links.IsWeb(new Uri("http://127.0.0.1:8765/manifest-test.json")));
        Assert.False(Services.Links.IsWeb(new Uri(@"C:\Windows\notepad.exe")));
        Assert.False(Services.Links.IsWeb(new Uri("file:///C:/Windows/notepad.exe")));
        Assert.False(Services.Links.IsWeb(new Uri("Assets/brand/logo.png", UriKind.Relative)));
    }

    [Fact]
    public void The_pictures_are_looked_for_inside_the_exe_at_the_paths_the_owner_was_given()
    {
        Assert.Equal("pack://application:,,,/AMDNR-Launcher;component/Assets/brand/logo.png", Brand.LogoUri.AbsoluteUri);
        Assert.Equal("pack://application:,,,/AMDNR-Launcher;component/Assets/brand/wordmark.png", Brand.WordmarkUri.AbsoluteUri);
    }

    [Fact]
    public void A_picture_is_decoded_on_load_and_frozen_so_every_page_shares_one_copy()
    {
        var path = Path.Combine(_root, "wordmark.png");
        WritePng(path, width: 3, height: 2);

        var picture = Brand.TryLoad(new Uri(path));

        Assert.NotNull(picture);
        Assert.True(picture.IsFrozen);
        Assert.Equal((3, 2), (picture.PixelWidth, picture.PixelHeight));

        // OnLoad: the file is read once and let go, so nothing holds it open afterwards.
        File.Delete(path);
        Assert.False(File.Exists(path));
    }

    [Fact]
    public void A_file_that_is_not_a_picture_is_null_never_an_exception()
    {
        var path = Path.Combine(_root, "logo.png");
        File.WriteAllBytes(path, "not a png"u8.ToArray());

        Assert.Null(Brand.TryLoad(new Uri(path)));
    }

    [Fact]
    public void The_icon_is_a_real_multi_image_ico_with_the_sizes_Windows_asks_for()
    {
        var bytes = File.ReadAllBytes(Path.Combine(RepoRoot(), "src", "AmdnrLauncher.App", "Assets", "app.ico"));

        // ICONDIR: reserved, type 1 for an icon, then the count.
        Assert.Equal(0, BitConverter.ToUInt16(bytes, 0));
        Assert.Equal(1, BitConverter.ToUInt16(bytes, 2));
        var count = BitConverter.ToUInt16(bytes, 4);
        Assert.True(count >= 6, $"only {count} images");

        var sizes = new List<int>();
        for (var i = 0; i < count; i++)
        {
            // ICONDIRENTRY: width, height (0 stands for 256), colours, reserved, planes, bits,
            // then the image's size and offset.
            var entry = 6 + 16 * i;
            var width = bytes[entry] == 0 ? 256 : bytes[entry];
            var height = bytes[entry + 1] == 0 ? 256 : bytes[entry + 1];
            var size = BitConverter.ToInt32(bytes, entry + 8);
            var offset = BitConverter.ToInt32(bytes, entry + 12);

            Assert.Equal(width, height);
            Assert.Equal(1, BitConverter.ToUInt16(bytes, entry + 4));
            Assert.Equal(32, BitConverter.ToUInt16(bytes, entry + 6));
            Assert.InRange(offset, 6 + 16 * count, bytes.Length - size);
            sizes.Add(width);

            // Each image is a PNG of that size, or a 32-bit DIB whose header carries the doubled
            // height an icon's colour-plus-mask bitmap has.
            if (bytes.AsSpan(offset, 8).SequenceEqual(PngSignature))
            {
                Assert.Equal(width, BigEndian(bytes, offset + 16));
                Assert.Equal(height, BigEndian(bytes, offset + 20));
            }
            else
            {
                Assert.Equal(40, BitConverter.ToInt32(bytes, offset));
                Assert.Equal(width, BitConverter.ToInt32(bytes, offset + 4));
                Assert.Equal(height * 2, BitConverter.ToInt32(bytes, offset + 8));
                Assert.Equal(32, BitConverter.ToUInt16(bytes, offset + 14));
            }
        }

        Assert.Equal(sizes.Count, sizes.Distinct().Count());
        Assert.All(new[] { 16, 24, 32, 48, 64, 128, 256 }, wanted => Assert.Contains(wanted, sizes));
    }

    [Fact]
    public void The_built_assembly_carries_the_icon()
    {
        // The taskbar, Explorer and the Details tab read the Win32 icon group, which the compiler
        // writes from ApplicationIcon. An index of -1 asks how many icons the file holds.
        Assert.True(ExtractIconExW(typeof(App).Assembly.Location, -1, IntPtr.Zero, IntPtr.Zero, 0) >= 1);
    }

    private static int BigEndian(byte[] bytes, int at)
        => (bytes[at] << 24) | (bytes[at + 1] << 16) | (bytes[at + 2] << 8) | bytes[at + 3];

    private static void WritePng(string path, int width, int height)
    {
        var encoder = new PngBitmapEncoder();
        encoder.Frames.Add(BitmapFrame.Create(new WriteableBitmap(width, height, 96, 96, PixelFormats.Bgra32, null)));
        using var file = File.Create(path);
        encoder.Save(file);
    }

    private static string RepoRoot()
    {
        for (var dir = new DirectoryInfo(AppContext.BaseDirectory); dir is not null; dir = dir.Parent)
        {
            if (File.Exists(Path.Combine(dir.FullName, "AmdnrLauncher.sln"))) return dir.FullName;
        }

        throw new FileNotFoundException("AmdnrLauncher.sln was not found above the test output folder.");
    }

    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    private static extern uint ExtractIconExW(string file, int iconIndex, IntPtr large, IntPtr small, uint count);
}
