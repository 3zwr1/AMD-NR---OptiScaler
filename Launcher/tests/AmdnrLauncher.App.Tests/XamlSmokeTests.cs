// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Runtime.ExceptionServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using AmdnrLauncher.App.ViewModels;
using AmdnrLauncher.App.Views;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>XAML is only compiled for its syntax; a StaticResource that does not resolve, or a
/// template that does not fit its control, fails when the element is built. These build the
/// chooser, in both of its modes, against the application's own resources — without running
/// the app, showing a window, or starting anything.</summary>
public class XamlSmokeTests
{
    private static readonly Manifest Manifest = ManifestParser.Parse("""
        {
          "schemaVersion": 1,
          "launcher": { "version": "0.4.0", "url": "https://example.test/AMDNR-Launcher.exe", "sha256": "00" },
          "sources": [
            { "id": "amdnr", "name": "AMDNR", "author": "3zwr1", "summary": "s", "homepage": "https://github.com/3zwr1/AMD-NR---OptiScaler",
              "package": "amdnr", "forwarder": null, "runtimeRequired": false, "runtimeNote": "n",
              "runtimes": [ { "package": "runtime", "gpus": ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"] } ] },
            { "id": "theautomatic", "name": "OptiScaler AMD pre-SR", "author": "TheAutomatic", "summary": "s", "homepage": "C:\\Windows\\notepad.exe",
              "package": "theautomatic", "forwarder": null, "runtimeRequired": true, "runtimeNote": "r" }
          ],
          "packages": [
            { "id": "amdnr", "version": "1", "url": "https://example.test/a.zip", "sha256": "00", "size": 1 },
            { "id": "theautomatic", "version": "1", "url": "https://example.test/t.zip", "sha256": "00", "size": 1 },
            { "id": "runtime", "version": "1", "url": "https://example.test/r.zip", "sha256": "00", "size": 110992515 }
          ],
          "proxyDefaults": ["dxgi.dll"],
          "proxyOverrides": []
        }
        """);

    /// <summary>This PC, as the display class key describes it.</summary>
    private static readonly GpuInfo Rdna4 =
        new("AMD Radeon RX 9070 XT", "rdna4", "32.0.31041.3013", @"PCI\VEN_1002&DEV_7550&REV_C0");

    private static readonly GpuInfo Rdna2 = new("AMD Radeon RX 6800 XT", "rdna2", "32.0.13031.3015", null);

    [Fact]
    public void The_chooser_builds_against_the_app_resources_in_both_modes()
    {
        RunOnSta(() =>
        {
            var app = Application.Current as App ?? new App();
            app.InitializeComponent();

            foreach (var firstRun in new[] { true, false })
            {
                var chooser = new BuildChooserViewModel(Manifest, null, firstRun, Rdna2) { IncludeRuntime = false };
                var view = new BuildChooserView { DataContext = chooser };

                view.Measure(new Size(1200, 800));
                view.Arrange(new Rect(0, 0, 1200, 800));
                view.UpdateLayout();

                Assert.True(view.DesiredSize.Height > 0);

                // The card and the amber note are on the page, in both modes.
                var shown = Descendants(view).OfType<TextBlock>().Where(t => IsShown(t, view)).ToList();
                Assert.Contains(shown, t => TextOf(t) == chooser.DetectedGpu);
                var note = Assert.Single(shown, t => TextOf(t) == chooser.GpuNote);
                Assert.Same(app.FindResource("StatusWarn"), note.Foreground);

                // A card the build lists a runtime for is asked the question, Yes and No.
                var question = Assert.Single(shown, t => TextOf(t) == chooser.RuntimeQuestion);
                Assert.Equal(2, Descendants(view).OfType<RadioButton>().Count(r => IsShown(r, view)));
                Assert.DoesNotContain(shown, t => TextOf(t) == chooser.NoRuntimeNote);

                // The question names danielblnc in full and links his repository from the page
                // itself, on the first run and in Settings alike; the size is its own line.
                var projectLink = Assert.Single(Links(question));
                Assert.Equal("https://github.com/danielblnc/DLSS-NR-on-AMD", projectLink.NavigateUri.AbsoluteUri);
                Assert.Equal(chooser.RuntimeProject, TextOf(projectLink.Inlines));
                Assert.Contains(shown, t => TextOf(t) == chooser.RuntimeSize);

                // ABOUT is Settings-only. There, his credit is his linked name with his
                // copyright line, and the launcher's own copyright line is beside it.
                var credits = shown.Where(t => TextOf(t).Contains(chooser.RuntimeCopyright, StringComparison.Ordinal)).ToList();
                var holders = shown.Where(t => TextOf(t).Contains(chooser.LauncherCopyright, StringComparison.Ordinal)).ToList();
                if (firstRun)
                {
                    Assert.Empty(credits);
                    Assert.Empty(holders);
                }
                else
                {
                    var credit = Assert.Single(credits);
                    Assert.Single(holders);
                    var hisLink = Assert.Single(Links(credit));
                    Assert.Equal("https://github.com/danielblnc/DLSS-NR-on-AMD", hisLink.NavigateUri.AbsoluteUri);
                    Assert.Equal(chooser.RuntimeAuthor, TextOf(hisLink.Inlines));
                }

                // The owner's wordmark heads the page: the PNGs once they are in the build, the
                // vector until then. In Settings a second one heads ABOUT, beside the owner's handle.
                var marks = Descendants(view).OfType<BrandWordmark>().Where(m => IsShown(m, view)).ToList();
                Assert.Equal(firstRun ? 1 : 2, marks.Count);
                Assert.Same(Brand.Wordmark, marks[0].Picture);
                Assert.Same(Brand.Logo, marks[0].Mark);
                Assert.Contains(shown, t => TextOf(t) == chooser.Heading);
                if (!firstRun)
                {
                    Assert.Null(marks[1].Picture);
                    Assert.Same(Brand.Logo, marks[1].Mark);
                    Assert.Contains(shown, t => TextOf(t) == "by 3zwr1");
                }

                // ABOUT says what the published source may be used for — read, not reused — and
                // links the owner's Discord server by the one address the caption's mark opens.
                var notices = shown.Where(t => TextOf(t) == Brand.LicenceNotice).ToList();
                var discordLinks = Descendants(view).OfType<TextBlock>().Where(t => IsShown(t, view))
                    .SelectMany(Links).Where(l => l.NavigateUri == Brand.DiscordUri).ToList();
                Assert.Equal(firstRun ? 0 : 1, notices.Count);
                Assert.Equal(firstRun ? 0 : 1, discordLinks.Count);

                ContinueStaysInViewHoweverShortTheWindow(view);
            }

            // A card no runtime is listed for is not asked — Yes would download nothing — and
            // is told so in the question's place.
            var rdna1 = new BuildChooserViewModel(Manifest, null, true,
                new GpuInfo("AMD Radeon RX 5700 XT", "rdna1", null, null));
            var unoffered = new BuildChooserView { DataContext = rdna1 };
            unoffered.Measure(new Size(1200, 800));
            unoffered.Arrange(new Rect(0, 0, 1200, 800));
            unoffered.UpdateLayout();
            Assert.False(rdna1.OffersRuntime);
            Assert.DoesNotContain(Descendants(unoffered).OfType<RadioButton>(), r => IsShown(r, unoffered));
            var unofferedTexts = Descendants(unoffered).OfType<TextBlock>().Where(t => IsShown(t, unoffered)).Select(TextOf).ToList();
            Assert.Contains(rdna1.NoRuntimeNote, unofferedTexts);
            Assert.DoesNotContain(rdna1.RuntimeQuestion, unofferedTexts);

            // On a supported card there is nothing to warn about, so no amber line is shown.
            var quiet = new BuildChooserView { DataContext = new BuildChooserViewModel(Manifest, null, true, Rdna4) };
            quiet.Measure(new Size(1200, 800));
            quiet.Arrange(new Rect(0, 0, 1200, 800));
            quiet.UpdateLayout();
            Assert.DoesNotContain(Descendants(quiet).OfType<TextBlock>().Where(t => IsShown(t, quiet)),
                t => ReferenceEquals(t.Foreground, app.FindResource("StatusWarn")));

            // Built, never shown: StartAsync runs from Loaded, which only a shown window
            // raises, so nothing here reaches the network or a game folder.
            var window = new MainWindow();
            Assert.Equal("AMD NR Launcher", window.Title);
            Assert.Equal(Brand.Name, window.Title);

            // Here rather than in a test of its own: the Application and its resources belong
            // to the thread that built them, and a second STA thread cannot use them.
            TheStageAndRailShowTheirNewLines(window, app);
            EveryGameHasACoverToBindTo(window);
            TheCaptionCarriesTheOwnersMark(window, app);
            TheCaptionLinksToTheDiscordServer(window, app);
            ThePicturesStandInForTheVectorOnlyWhenTheyExist();
            TheWindowDoesNotEndAGameFolderChangeUnasked(window);
            window.Close();
        });
    }

    /// <summary>Both routes out of the process: the window's own close, and RESTART AS
    /// ADMINISTRATOR, which ends in Application.Shutdown.</summary>
    private static void TheWindowDoesNotEndAGameFolderChangeUnasked(MainWindow window)
    {
        var viewModel = (MainViewModel)window.DataContext;
        var root = (FrameworkElement)window.Content;

        var restart = Descendants(root).OfType<Button>()
            .Single(b => b.Content as string == "RESTART AS ADMINISTRATOR");
        Assert.True(IsShown(restart, root));
        Assert.True(restart.IsEnabled);

        viewModel.Busy = true;
        Assert.False(restart.IsEnabled);

        var asked = new List<string>();
        viewModel.ConfirmCloseMidChange = message => { asked.Add(message); return false; };
        viewModel.ChangingGame = "SILENT HILL 2";

        var closed = false;
        window.Closed += (_, _) => closed = true;
        window.Close();

        Assert.False(closed);
        Assert.Single(asked);

        viewModel.ChangingGame = null;
        viewModel.Busy = false;
        Assert.True(restart.IsEnabled);
    }

    private static GameEntry Row(
        string name, string? exeDirectory, GameStatus status, GateResult? gate,
        IReadOnlyList<string>? occupied = null) => new(
        new GameCandidate(name, GameStore.Steam, @"D:\Games\" + name),
        ExeDirectory: exeDirectory,
        ExeName: exeDirectory is null ? null : "SHProto-Win64-Shipping.exe",
        Status: status,
        Record: null,
        Report: null,
        Gate: gate,
        RecommendedProxy: "dxgi.dll",
        ProxyReason: "SHProto-Win64-Shipping.exe loads dxgi.dll itself when it starts",
        OccupiedProxies: occupied ?? []);

    /// <summary>The rail's templates are built only once there are rows, and a resource or a
    /// trigger that does not resolve inside one fails only then — so this lays the window out
    /// with rows in it and reads back what a user would see.</summary>
    private static void TheStageAndRailShowTheirNewLines(MainWindow window, Application app)
    {
        var viewModel = (MainViewModel)window.DataContext;
        // NotWritable puts RESTART AS ADMINISTRATOR on screen for the check after this one.
        var gate = new GateResult(
        [
            new GateFinding(GateCode.AntiCheatDetected, "m", Blocking: true, ["EasyAntiCheat"]),
            new GateFinding(GateCode.NotWritable, "m", Blocking: true),
        ]);

        viewModel.ReplaceGames(
        [
            Row("SILENT HILL 2", @"D:\Games\SH2\SHProto\Binaries\Win64", GameStatus.ExistingInstall, gate,
                occupied: ["winmm.dll"]),
            Row("Unresolved", null, GameStatus.NotInstalled, null),
        ]);
        var earlier = viewModel.Games[0];
        var unresolved = viewModel.Games[1];
        viewModel.Selected = earlier;

        var root = (FrameworkElement)window.Content;
        root.Measure(new Size(1240, 760));
        root.Arrange(new Rect(0, 0, 1240, 760));
        root.UpdateLayout();

        var shown = Descendants(root).OfType<TextBlock>().Where(t => IsShown(t, root)).ToList();

        Assert.Contains(shown, t => TextOf(t) == earlier.AntiCheatWarning);
        Assert.Contains(shown, t => TextOf(t) == earlier.EarlierInstallNote);
        Assert.Contains(shown, t => TextOf(t) == "exe SHProto-Win64-Shipping.exe");

        // The rail row and the stage both name the DLL for a resolved game, and nothing does
        // for the one with no exe folder — not even the separator in front of the name.
        Assert.Equal(2, shown.Count(t => t.DataContext == earlier && TextOf(t) == " · dxgi.dll"));
        Assert.DoesNotContain(shown, t => t.DataContext == unresolved && TextOf(t).StartsWith(" ·", StringComparison.Ordinal));

        var dot = Descendants(root).OfType<Ellipse>().Single(e => e.Name == "Dot" && e.DataContext == earlier);
        Assert.Same(app.FindResource("StatusInfo"), dot.Fill);

        // The six DLL names sit under the facts as a row of choices: the one in use marked, a
        // name another file holds greyed with the reason in its tooltip — shown though the
        // control is disabled — and under them why the launcher picked what it picked.
        var radios = Descendants(root).OfType<RadioButton>().Where(r => IsShown(r, root)).ToList();
        Assert.Equal(PayloadNames.ProxyNames, radios.Select(r => (string)r.Content));
        Assert.Equal(["dxgi.dll"], radios.Where(r => r.IsChecked == true).Select(r => (string)r.Content));

        var taken = Assert.Single(radios, r => !r.IsEnabled);
        Assert.Equal("winmm.dll", taken.Content);
        Assert.Contains("winmm.dll", (string)taken.ToolTip);
        Assert.True(ToolTipService.GetShowOnDisabled(taken));
        Assert.All(radios.Where(r => r.IsEnabled), r => Assert.Null(r.ToolTip));

        Assert.Same(viewModel.ChooseProxyCommand, taken.Command);
        Assert.Equal("winmm.dll", taken.CommandParameter);
        Assert.Contains(shown, t => TextOf(t) == earlier.ProxyReason);
        Assert.Equal("dxgi.dll — SHProto-Win64-Shipping.exe loads dxgi.dll itself when it starts.", earlier.ProxyReason);
    }

    /// <summary>The rail and the stage bind to every kind of cover the row can have: box art
    /// cropped to a spine, a square logo or icon letterboxed on the plate with the initial gone,
    /// and — where Steam's pre-blurred hero is missing — a backdrop the window blurs itself,
    /// behind the stage and behind the window, each with a blur of its own. Bindings, not
    /// pictures: the rows here have none, and a Source that does not resolve draws nothing.</summary>
    private static void EveryGameHasACoverToBindTo(MainWindow window)
    {
        var root = (FrameworkElement)window.Content;
        var viewModel = (MainViewModel)window.DataContext;
        var earlier = viewModel.Games[0];
        var images = Descendants(root).OfType<Image>().ToList();

        // The rail: one tile of each shape per row, and the initial only while there is no tile.
        var portrait = Assert.Single(images, i => i.DataContext == earlier && BindingPath(i, Image.SourceProperty) == nameof(GameRowViewModel.PortraitPath));
        Assert.Equal(Stretch.UniformToFill, portrait.Stretch);
        var square = Assert.Single(images, i => i.DataContext == earlier && BindingPath(i, Image.SourceProperty) == nameof(GameRowViewModel.SquareTilePath));
        Assert.Equal(Stretch.Uniform, square.Stretch);
        Assert.Null(square.Effect);
        var initial = Assert.Single(Descendants(root).OfType<TextBlock>(),
            t => t.DataContext == earlier && BindingPath(t, TextBlock.TextProperty) == nameof(GameRowViewModel.Initial));
        Assert.Equal(nameof(GameRowViewModel.HasTile), BindingPath(initial, UIElement.VisibilityProperty));
        Assert.True(IsShown(initial, root));

        // The stage: the hero as it is, and the square tile blurred behind it when there is no
        // hero — blurred once by the converter over the small decode, so no effect runs over
        // the stage-sized surface at every redraw.
        Assert.Single(images, i => i.DataContext == earlier && BindingPath(i, Image.SourceProperty) == nameof(GameRowViewModel.HeroPath) && i.Effect is null);
        var stageBackdrop = Assert.Single(images, i => i.DataContext == earlier && BindingPath(i, Image.SourceProperty) == nameof(GameRowViewModel.StageBackdropPath));
        Assert.Null(stageBackdrop.Effect);
        Assert.IsType<PathToBlurredImageConverter>(BindingOperations.GetBinding(stageBackdrop, Image.SourceProperty)?.Converter);
        Assert.Equal(Stretch.UniformToFill, stageBackdrop.Stretch);

        // The window: Steam's pre-blurred hero drawn as it is, or a backdrop blurred here — the
        // same way, once and small, never as a blur over the whole window.
        Assert.Single(images, i => BindingPath(i, Image.SourceProperty) == "Selected.HeroBlurPath" && i.Effect is null);
        var windowBackdrop = Assert.Single(images, i => BindingPath(i, Image.SourceProperty) == "Selected.BackdropToBlurPath");
        Assert.Null(windowBackdrop.Effect);
        Assert.IsType<PathToBlurredImageConverter>(BindingOperations.GetBinding(windowBackdrop, Image.SourceProperty)?.Converter);
        Assert.Equal(Stretch.UniformToFill, windowBackdrop.Stretch);
        Assert.DoesNotContain(Descendants(root), d => d is Image { Effect: BlurEffect });
    }

    private static string? BindingPath(DependencyObject element, DependencyProperty property)
        => BindingOperations.GetBinding(element, property)?.Path.Path;

    /// <summary>Discord's mark sits in the caption, on every page — the first-run chooser
    /// included, which is when a newcomer needs the server most — and one click opens the
    /// owner's server. Hit-testable in the chrome, or the click would drag the window instead.</summary>
    private static void TheCaptionLinksToTheDiscordServer(MainWindow window, Application app)
    {
        var root = (FrameworkElement)window.Content;
        var caption = Assert.Single(Descendants(root).OfType<Grid>().Where(g => g.Name == "Caption"));

        var discord = Assert.Single(Descendants(caption).OfType<Button>().Where(b => b.Name == "DiscordButton"));
        Assert.True(IsShown(discord, root));
        Assert.True(System.Windows.Shell.WindowChrome.GetIsHitTestVisibleInChrome(discord));
        Assert.Equal("https://discord.gg/AMDNR", Brand.DiscordUrl);
        Assert.Equal(Brand.DiscordUrl, discord.Tag);
        Assert.Contains("discord.gg/AMDNR", Assert.IsType<string>(discord.ToolTip));

        discord.ApplyTemplate();
        var mark = Assert.Single(Descendants(discord).OfType<Path>().Where(p => IsShown(p, discord)));
        Assert.Same(app.FindResource("DiscordMark"), mark.Data);
        Assert.NotNull(mark.Fill);
    }

    /// <summary>The buttons are a footer under the scrolling text, not its last lines: at the
    /// window's default height the page was taller than the window, and CONTINUE — the one
    /// control a first run has to find — sat below the fold with a scrollbar as the only hint.</summary>
    private static void ContinueStaysInViewHoweverShortTheWindow(BuildChooserView view)
    {
        var scroll = Assert.Single(Descendants(view).OfType<ScrollViewer>());
        var buttons = Descendants(view).OfType<Button>().Where(b => b.Content is "CONTINUE" or "CANCEL").ToList();
        Assert.NotEmpty(buttons);
        Assert.All(buttons, b => Assert.DoesNotContain(b, Descendants(scroll)));

        view.Measure(new Size(1000, 560));
        view.Arrange(new Rect(0, 0, 1000, 560));
        view.UpdateLayout();

        var go = buttons.Single(b => b.Content is "CONTINUE");
        var bounds = go.TransformToAncestor(view).TransformBounds(new Rect(go.RenderSize));
        Assert.True(go.RenderSize.Height > 0, "CONTINUE has no size");
        Assert.True(bounds.Top >= 0 && bounds.Bottom <= 560, $"CONTINUE lies at {bounds} in a 1000x560 view");
    }

    /// <summary>The caption carries the owner's mark, drawn: AMD in white, then the arrow, then NR
    /// in the accent — the order of the owner's logo and of the exe's icon — then LAUNCHER small
    /// and muted. Never a picture: at 46 px a scaled PNG blurs where the vector stays sharp. With
    /// the chooser closed, the caption's is the only wordmark shown.</summary>
    private static void TheCaptionCarriesTheOwnersMark(MainWindow window, Application app)
    {
        var root = (FrameworkElement)window.Content;

        var caption = Assert.Single(Descendants(root).OfType<BrandWordmark>().Where(m => IsShown(m, root)));
        Assert.Null(caption.Picture);
        Assert.Null(caption.Mark);

        var arrow = Assert.Single(Descendants(caption).OfType<Path>().Where(p => IsShown(p, caption)));
        Assert.Same(app.FindResource("AmdArrowMark"), arrow.Data);
        Assert.Same(app.FindResource("BrandInk"), arrow.Fill);
        Assert.DoesNotContain(Descendants(caption).OfType<Image>(), i => IsShown(i, caption));

        var words = Descendants(caption).OfType<TextBlock>().Where(t => IsShown(t, caption)).ToList();
        Assert.Equal(["AMD", "NR"], words.Select(TextOf));
        ReadsAmdThenTheMarkThenNr(words[0], arrow, words[1], caption);
        Assert.Same(app.FindResource("BrandInk"), words[0].Foreground);
        Assert.Same(app.FindResource("Accent"), words[1].Foreground);
        Assert.All(words, word =>
        {
            Assert.Equal(FontWeights.Bold, word.FontWeight);
            Assert.Same(app.FindResource("BrandFont"), word.FontFamily);
            Assert.Equal(caption.FontSize, word.FontSize);
        });

        var launcher = Assert.Single(Descendants(root).OfType<TextBlock>().Where(t => IsShown(t, root) && TextOf(t) == "LAUNCHER"));
        Assert.Same(app.FindResource("TextMuted"), launcher.Foreground);
        Assert.True(launcher.FontSize < caption.FontSize);
    }

    /// <summary>The owner's pictures stand in only when they exist: a Picture replaces the whole
    /// line, a Mark replaces the arrow alone, and without either the vector draws, and draws again
    /// when a picture is taken away. Everything is sized from FontSize alone.</summary>
    private static void ThePicturesStandInForTheVectorOnlyWhenTheyExist()
    {
        var picture = new WriteableBitmap(4, 4, 96, 96, PixelFormats.Bgra32, null);
        picture.Freeze();

        var vector = Lay(new BrandWordmark());
        var arrow = Assert.Single(Descendants(vector).OfType<Path>().Where(p => IsShown(p, vector)));
        var drawnWords = Descendants(vector).OfType<TextBlock>().Where(t => IsShown(t, vector)).ToList();
        Assert.Equal(["AMD", "NR"], drawnWords.Select(TextOf));
        Assert.DoesNotContain(Descendants(vector).OfType<Image>(), i => IsShown(i, vector));
        ReadsAmdThenTheMarkThenNr(drawnWords[0], arrow, drawnWords[1], vector);

        var withMark = Lay(new BrandWordmark { Mark = picture });
        var mark = Assert.Single(Descendants(withMark).OfType<Image>().Where(i => IsShown(i, withMark)));
        Assert.Same(picture, mark.Source);
        Assert.DoesNotContain(Descendants(withMark).OfType<Path>(), p => IsShown(p, withMark));
        var markedWords = Descendants(withMark).OfType<TextBlock>().Where(t => IsShown(t, withMark)).ToList();
        Assert.Equal(["AMD", "NR"], markedWords.Select(TextOf));
        ReadsAmdThenTheMarkThenNr(markedWords[0], mark, markedWords[1], withMark);

        var withPicture = Lay(new BrandWordmark { Picture = picture });
        var whole = Assert.Single(Descendants(withPicture).OfType<Image>().Where(i => IsShown(i, withPicture)));
        Assert.Same(picture, whole.Source);
        Assert.DoesNotContain(Descendants(withPicture).OfType<TextBlock>(), t => IsShown(t, withPicture));
        Assert.DoesNotContain(Descendants(withPicture).OfType<Path>(), p => IsShown(p, withPicture));

        withPicture.Picture = null;
        Assert.DoesNotContain(Descendants(withPicture).OfType<Image>(), i => IsShown(i, withPicture));
        Assert.Single(Descendants(withPicture).OfType<Path>().Where(p => IsShown(p, withPicture)));

        var small = Lay(new BrandWordmark { FontSize = 15 });
        var large = Lay(new BrandWordmark { FontSize = 30 });
        Assert.All(Descendants(small).OfType<TextBlock>(), t => Assert.Equal(15, t.FontSize));
        Assert.All(Descendants(large).OfType<TextBlock>(), t => Assert.Equal(30, t.FontSize));
        Assert.True(Descendants(large).OfType<Viewbox>().Single().ActualHeight
                    > Descendants(small).OfType<Viewbox>().Single().ActualHeight);
    }

    private static T Lay<T>(T element) where T : FrameworkElement
    {
        element.Measure(new Size(600, 200));
        element.Arrange(new Rect(0, 0, 600, 200));
        element.UpdateLayout();
        return element;
    }

    /// <summary>The line reads as the owner's logo does and as the exe's icon draws it: AMD, then
    /// the mark, then NR, each wholly before the next. Judged by where each lands once laid out,
    /// which is what a reader sees, not by the order in the tree.</summary>
    private static void ReadsAmdThenTheMarkThenNr(FrameworkElement amd, FrameworkElement mark, FrameworkElement nr, Visual line)
    {
        var (_, amdRight) = SpanOf(amd, line);
        var (markLeft, markRight) = SpanOf(mark, line);
        var (nrLeft, _) = SpanOf(nr, line);
        Assert.True(amdRight <= markLeft, $"AMD ends at {amdRight:F1} but the mark starts at {markLeft:F1}: the mark should follow AMD");
        Assert.True(markRight <= nrLeft, $"the mark ends at {markRight:F1} but NR starts at {nrLeft:F1}: NR should follow the mark");
    }

    /// <summary>An element's left and right edges in the line's own coordinates, through whatever
    /// a Viewbox scales it by.</summary>
    private static (double Left, double Right) SpanOf(FrameworkElement element, Visual line)
    {
        var toLine = element.TransformToAncestor(line);
        return (toLine.Transform(new Point(0, 0)).X, toLine.Transform(new Point(element.ActualWidth, 0)).X);
    }

    /// <summary>What the TextBlock draws. Text does not follow bound Runs, so a line built from
    /// Runs is read from its Runs.</summary>
    private static string TextOf(TextBlock block)
        => block.Inlines.Count == 0 ? block.Text : TextOf(block.Inlines);

    /// <summary>Runs, the Runs inside a Hyperlink or any other Span, and the one space WPF
    /// keeps between two inlines written on separate lines: what the reader reads.</summary>
    private static string TextOf(InlineCollection inlines)
        => string.Concat(inlines.Select(inline => inline switch
        {
            Run run => run.Text,
            Span span => TextOf(span.Inlines),
            LineBreak => "\n",
            _ => "",
        }));

    /// <summary>Hyperlinks are inlines, not visuals, so the visual tree does not list them;
    /// this reads them out of a TextBlock at any depth.</summary>
    private static IEnumerable<Hyperlink> Links(TextBlock block) => Links(block.Inlines);

    private static IEnumerable<Hyperlink> Links(InlineCollection inlines)
    {
        foreach (var inline in inlines)
        {
            if (inline is Hyperlink link) yield return link;
            else if (inline is Span span) foreach (var below in Links(span.Inlines)) yield return below;
        }
    }

    private static IEnumerable<DependencyObject> Descendants(DependencyObject parent)
    {
        for (var i = 0; i < VisualTreeHelper.GetChildrenCount(parent); i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);
            yield return child;
            foreach (var below in Descendants(child)) yield return below;
        }
    }

    /// <summary>IsVisible is false for everything in a window that was never shown, so this
    /// walks up and asks each element instead.</summary>
    private static bool IsShown(DependencyObject element, DependencyObject root)
    {
        for (var at = element; at is not null && at != root; at = VisualTreeHelper.GetParent(at))
        {
            if (at is UIElement { Visibility: not Visibility.Visible }) return false;
        }

        return true;
    }

    [Fact]
    public void A_homepage_that_is_not_a_web_address_is_not_offered_as_a_link()
    {
        var chooser = new BuildChooserViewModel(Manifest, null, isFirstRun: true, Rdna4);

        Assert.Equal("https://github.com/3zwr1/AMD-NR---OptiScaler", chooser.Options[0].Homepage?.AbsoluteUri);
        Assert.Null(chooser.Options[1].Homepage);
        Assert.Equal("About 110 MB.", chooser.RuntimeSize);
    }

    [Fact]
    public async Task Settings_can_be_closed_without_an_answer_but_the_first_run_cannot()
    {
        var first = new BuildChooserViewModel(Manifest, null, isFirstRun: true, Rdna4);
        first.Cancel();
        Assert.False(first.Answer.IsCompleted);
        Assert.False(first.CancelCommand.CanExecute(null));

        var settings = new BuildChooserViewModel(Manifest, new Core.InstallChoice("theautomatic", false), isFirstRun: false, Rdna4);
        Assert.Equal("theautomatic", settings.SelectedOption?.Source.Id);
        Assert.False(settings.IncludeRuntime);
        settings.Cancel();
        Assert.True(settings.Answer.IsCompletedSuccessfully);
        Assert.Null(await settings.Answer);
    }

    private static void RunOnSta(Action action)
    {
        ExceptionDispatchInfo? failure = null;
        var thread = new Thread(() =>
        {
            try { action(); }
            catch (Exception e) { failure = ExceptionDispatchInfo.Capture(e); }
        });
        thread.SetApartmentState(ApartmentState.STA);
        thread.Start();
        thread.Join();
        failure?.Throw();
    }
}
