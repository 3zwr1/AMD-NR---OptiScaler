// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;

namespace AmdnrLauncher.App;

/// <summary>Loads a picture from a path, or returns null so the view falls back.
///
/// Two things here are not optional. OnLoad decodes the file immediately and closes the
/// handle — the default keeps it open, and these files belong to the Steam client, which
/// would then be unable to refresh its own cache while the launcher is running. Freeze
/// makes the result shareable and, more to the point, lets WPF reuse one decoded copy of
/// an image bound in several places instead of decoding it per binding.</summary>
public sealed class PathToImageConverter : IValueConverter
{
    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        // Box art is 600x900 and the row draws it at 44 wide. Decoding at the size we
        // need keeps a 200-game library from holding 200 full-resolution bitmaps.
        => Load(value as string, parameter is string width && int.TryParse(width, out var pixels) ? pixels : null);

    /// <summary>The picture at <paramref name="path"/>, decoded <paramref name="decodeWidth"/>
    /// pixels wide when that is given, frozen — or null for anything that is not a picture this
    /// process can show, whatever the reason.</summary>
    internal static BitmapImage? Load(string? path, int? decodeWidth)
    {
        if (string.IsNullOrEmpty(path)) return null;

        try
        {
            var image = new BitmapImage();
            image.BeginInit();
            image.CacheOption = BitmapCacheOption.OnLoad;
            image.CreateOptions = BitmapCreateOptions.IgnoreColorProfile;
            if (decodeWidth is { } pixels) image.DecodePixelWidth = pixels;
            image.UriSource = new Uri(path, UriKind.Absolute);
            image.EndInit();
            image.Freeze();
            return image;
        }
        // A truncated or half-written file in Steam's cache is an ordinary thing to find, and
        // so is a CDN body whose header passed the launcher's own size check. WPF raises
        // NotSupportedException for bytes no decoder claims, FileFormatException — a
        // FormatException, which also covers UriFormatException — for a file whose header a
        // decoder took but whose body it could not finish, and a bare COMException for the WIC
        // failures it maps to nothing better. None of these may reach the binding engine: an
        // exception thrown inside a converter during layout is caught by nothing short of the
        // dispatcher's last handler, which ends the launcher.
        catch (Exception e) when (e is IOException
                                    or UnauthorizedAccessException
                                    or NotSupportedException
                                    or FormatException
                                    or ArgumentException
                                    or System.Runtime.InteropServices.COMException)
        {
            return null;
        }
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

/// <summary>Blurs a picture once, small, for the view to draw large — the way Steam's own
/// library_hero_blur.jpg, 192 px wide and drawn over the whole window, already is.
///
/// A BlurEffect on the on-screen Image would not do this: an Effect runs over the element's
/// rendered surface at its on-screen size — the window, or the stage — at every redraw, and in
/// software rendering (a remote desktop, a Tier 0 machine) that is the whole window through a
/// Gaussian each time. This runs the same blur over a bitmap <see cref="DecodeWidth"/> pixels
/// wide, once per picture, in software, on the thread that binds; the frozen result is what
/// every binding shares, and the Image that draws it carries no effect at all. The parameter is
/// the blur's radius at that width — what a radius over the on-screen surface looked like,
/// divided by how many times the small bitmap is enlarged to fill it.</summary>
public sealed class PathToBlurredImageConverter : IValueConverter
{
    /// <summary>Wide enough that, enlarged over a 1400 px window, a blur this soft shows no
    /// blocks; small enough that the blur costs nothing worth measuring.</summary>
    public const int DecodeWidth = 240;

    public const double DefaultRadius = 12;

    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (PathToImageConverter.Load(value as string, DecodeWidth) is not { } source) return null;

        var radius = parameter is string text
                     && double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out var given)
                     && given >= 0
            ? given
            : DefaultRadius;

        try
        {
            var visual = new DrawingVisual
            {
                Effect = new BlurEffect { Radius = radius, KernelType = KernelType.Gaussian, RenderingBias = RenderingBias.Performance },
            };
            using (var drawing = visual.RenderOpen())
            {
                // Drawn two radii past every edge, so at the edges the blur samples the picture
                // and not the void beyond it: the result is opaque to its last pixel, and the
                // view has no soft border to hide.
                var overscan = 2 * radius;
                drawing.DrawImage(source, new Rect(-overscan, -overscan, source.PixelWidth + 2 * overscan, source.PixelHeight + 2 * overscan));
            }

            var target = new RenderTargetBitmap(source.PixelWidth, source.PixelHeight, 96, 96, PixelFormats.Pbgra32);
            target.Render(visual);
            target.Freeze();
            return target;
        }
        // A backdrop that cannot be rendered is a backdrop that is not drawn — the plate, as for
        // any other picture that could not be shown — never an exception out of a binding.
        catch (Exception e) when (e is ArgumentException
                                    or InvalidOperationException
                                    or System.Runtime.InteropServices.COMException)
        {
            return null;
        }
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

/// <summary>Gives each game's tile its own colour, derived from its name, so a list of
/// twenty games is scannable by shape and colour rather than by reading every line.
///
/// The hash is FNV-1a rather than <see cref="string.GetHashCode()"/> on purpose: .NET
/// randomises string hashing per process, so the stock hash would repaint every tile a
/// different colour on every launch — which would read as a rendering bug.</summary>
public sealed class NameToTileBrushConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        var name = value as string ?? "";

        uint hash = 2166136261;
        foreach (var c in name)
        {
            hash ^= c;
            hash *= 16777619;
        }

        // Saturation and lightness are fixed and deliberately low: the tiles have to sit
        // behind the accent in the visual hierarchy, not compete with it. Only hue varies.
        var hue = hash % 360;

        return new LinearGradientBrush(
            FromHsl(hue, 0.32, 0.30),
            FromHsl((hue + 26) % 360, 0.34, 0.19),
            new Point(0, 0), new Point(1, 1));
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();

    private static Color FromHsl(double hue, double saturation, double lightness)
    {
        var chroma = (1 - Math.Abs(2 * lightness - 1)) * saturation;
        var second = chroma * (1 - Math.Abs(hue / 60 % 2 - 1));
        var match = lightness - chroma / 2;

        var (r, g, b) = hue switch
        {
            < 60  => (chroma, second, 0d),
            < 120 => (second, chroma, 0d),
            < 180 => (0d, chroma, second),
            < 240 => (0d, second, chroma),
            < 300 => (second, 0d, chroma),
            _     => (chroma, 0d, second),
        };

        return Color.FromRgb(
            (byte)Math.Round((r + match) * 255),
            (byte)Math.Round((g + match) * 255),
            (byte)Math.Round((b + match) * 255));
    }
}

/// <summary>Visible when the bound value is set. Used for the launcher-update banner and
/// for the detail panel, both of which have nothing to say until something exists.</summary>
public sealed class NullToVisibilityConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => value is null ? Visibility.Collapsed : Visibility.Visible;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

/// <summary>The opposite: visible only while the bound value is null. This is what shows the
/// "select a game" placeholder, which exists precisely when the detail panel does not.
/// Inverting with a parameter on the converter above would make every call site read
/// ambiguously at a glance, which is the wrong trade for two extra lines.</summary>
public sealed class NullToInverseVisibilityConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => value is null ? Visibility.Visible : Visibility.Collapsed;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

/// <summary>Visible only while the bound flag is false. This draws the game's name in
/// place of its wordmark: exactly one of the two appears, never both.</summary>
public sealed class BoolToInverseVisibilityConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => value is true ? Visibility.Collapsed : Visibility.Visible;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

/// <summary>Visible only when the bound count is zero — the empty-list explanation.
/// Anything that is not a number counts as "not empty": a binding that fails to resolve
/// must not put an empty-state message on top of a populated list.</summary>
public sealed class ZeroToVisibilityConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => value is int count && count == 0 ? Visibility.Visible : Visibility.Collapsed;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
