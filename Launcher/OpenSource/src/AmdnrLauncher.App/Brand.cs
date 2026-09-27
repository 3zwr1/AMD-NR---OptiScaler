// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using System.IO.Packaging;
using System.Windows.Media.Imaging;

namespace AmdnrLauncher.App;

/// <summary>What the launcher is called, and the owner's mark.
///
/// The owner named it AMD NR Launcher, two words, and that is what every person reads: the
/// window and its taskbar entry, the exe's Details tab (the csproj's Product and AssemblyTitle,
/// held to <see cref="Name"/> by a test) and every dialog. Nothing a machine reads changed with
/// it: the exe is still AMDNR-Launcher.exe because the manifest's URLs point at that file, the
/// data folder is still %LOCALAPPDATA%\AMDNR, and the source and package ids are still amdnr.
///
/// The mark is the owner's logo: AMD in white, then the AMD arrow, then NR in the accent, the
/// order the exe's icon draws as well (tools\make-icon.ps1). Until the owner
/// drops the real pictures into the build, BrandWordmark draws it; once they are there, they are
/// used instead. They are never generated here: a picture the owner did not make is not the
/// owner's mark.</summary>
public static class Brand
{
    public const string Name = "AMD NR Launcher";

    /// <summary>The owner's Discord server. The caption's mark and the ABOUT link both open this
    /// one address, so there is one place to change it.</summary>
    public const string DiscordUrl = "https://discord.gg/AMDNR";
    public static readonly Uri DiscordUri = new(DiscordUrl);

    /// <summary>What the published source may be used for, under the copyright line in ABOUT.
    /// The launcher is not open source: LICENSE.txt has the terms, this is the one-line version.</summary>
    public const string LicenceNotice =
        "Source published to be read, not reused — all rights reserved. See LICENSE.txt.";

    // Read before the two addresses below are built. PackUriHelper's type initialiser registers
    // the pack scheme, and until an Application exists (as in the tests) nothing else has, so a
    // pack address would be refused as malformed. Static fields initialise in this order.
    private static readonly string PackScheme = PackUriHelper.UriSchemePack;

    /// <summary>Where the owner's pictures go, as the csproj compiles them into the exe from
    /// Assets\brand: logo.png is the square mark, wordmark.png the line "NEURAL RENDERING - AMD".</summary>
    public static readonly Uri LogoUri = PictureUri("Assets/brand/logo.png");
    public static readonly Uri WordmarkUri = PictureUri("Assets/brand/wordmark.png");

    private static readonly Lazy<BitmapSource?> LazyLogo = new(() => TryLoad(LogoUri));
    private static readonly Lazy<BitmapSource?> LazyWordmark = new(() => TryLoad(WordmarkUri));

    /// <summary>The owner's square mark, or null while it is not in the build. Decoded once, on
    /// first use, and frozen, so every page draws the one copy.</summary>
    public static BitmapSource? Logo => LazyLogo.Value;

    /// <summary>The owner's wordmark, or null while it is not in the build.</summary>
    public static BitmapSource? Wordmark => LazyWordmark.Value;

    /// <summary>The pack address of a file compiled into this assembly as a Resource. Named by
    /// assembly rather than left to "the application", so it resolves the same under a test host.</summary>
    public static Uri PictureUri(string relativePath)
        => new($"{PackScheme}://application:,,,/{typeof(Brand).Assembly.GetName().Name};component/{relativePath}");

    /// <summary>The picture at an address, or null: not there, not readable, not a picture. OnLoad
    /// reads the bytes at once and lets the source go; Freeze makes the one copy shareable.</summary>
    public static BitmapSource? TryLoad(Uri uri)
    {
        try
        {
            var image = new BitmapImage();
            image.BeginInit();
            image.CacheOption = BitmapCacheOption.OnLoad;
            image.CreateOptions = BitmapCreateOptions.IgnoreColorProfile;
            image.UriSource = uri;
            image.EndInit();
            image.Freeze();
            return image;
        }
        // A resource that is not in the build surfaces as IOException; bytes that are not a
        // picture as NotSupportedException or FileFormatException. None of them is news worth a
        // dialog: the wordmark is drawn instead.
        catch (Exception e) when (e is IOException
                                    or UnauthorizedAccessException
                                    or NotSupportedException
                                    or FileFormatException
                                    or ArgumentException
                                    or UriFormatException)
        {
            return null;
        }
    }
}
