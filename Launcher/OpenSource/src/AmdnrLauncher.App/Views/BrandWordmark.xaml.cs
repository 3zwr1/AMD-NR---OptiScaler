// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace AmdnrLauncher.App.Views;

/// <summary>The owner's wordmark, drawn from FontSize alone, or the owner's pictures when they
/// are in the build. See the XAML for the shape and <see cref="Brand"/> for the pictures.</summary>
public partial class BrandWordmark : UserControl
{
    /// <summary>The owner's whole wordmark as a picture. Set, it replaces the drawn line.</summary>
    public static readonly DependencyProperty PictureProperty = DependencyProperty.Register(
        nameof(Picture), typeof(ImageSource), typeof(BrandWordmark),
        new PropertyMetadata(null, (d, _) => ((BrandWordmark)d).Apply()));

    /// <summary>The owner's square mark as a picture. Set, it replaces the drawn arrow alone.</summary>
    public static readonly DependencyProperty MarkProperty = DependencyProperty.Register(
        nameof(Mark), typeof(ImageSource), typeof(BrandWordmark),
        new PropertyMetadata(null, (d, _) => ((BrandWordmark)d).Apply()));

    public BrandWordmark()
    {
        InitializeComponent();
        Apply();
    }

    public ImageSource? Picture
    {
        get => (ImageSource?)GetValue(PictureProperty);
        set => SetValue(PictureProperty, value);
    }

    public ImageSource? Mark
    {
        get => (ImageSource?)GetValue(MarkProperty);
        set => SetValue(MarkProperty, value);
    }

    /// <summary>FontSize arrives by inheritance from the page as well as from a setter, so the
    /// sizes follow it from here rather than from a setter alone.</summary>
    protected override void OnPropertyChanged(DependencyPropertyChangedEventArgs e)
    {
        base.OnPropertyChanged(e);
        if (e.Property == FontSizeProperty) Apply();
    }

    private void Apply()
    {
        // Until InitializeComponent has run the elements do not exist; the constructor's own
        // call is the one that first sizes them.
        if (Amd is null) return;

        var size = FontSize;

        PicturePane.Source = Picture;
        PicturePane.Height = size * 1.5;
        PicturePane.Visibility = Picture is null ? Visibility.Collapsed : Visibility.Visible;
        VectorPane.Visibility = Picture is null ? Visibility.Visible : Visibility.Collapsed;

        // The arrow stands cap-high after the letters AMD, as it does in the owner's logo and in
        // the exe's icon, with the gap make-icon.ps1 leaves before it, in the mark's own units.
        var mark = size * 0.72;
        MarkSlot.Width = MarkSlot.Height = mark;
        MarkSlot.Margin = new Thickness(mark * 0.24, 0, 0, 0);
        MarkPicture.Source = Mark;
        MarkPicture.Visibility = Mark is null ? Visibility.Collapsed : Visibility.Visible;
        MarkVector.Visibility = Mark is null ? Visibility.Visible : Visibility.Collapsed;

        // NR follows the mark as a word of its own, a word space after it.
        Amd.FontSize = Nr.FontSize = size;
        Nr.Margin = new Thickness(size * 0.3, 0, 0, 0);
    }
}
