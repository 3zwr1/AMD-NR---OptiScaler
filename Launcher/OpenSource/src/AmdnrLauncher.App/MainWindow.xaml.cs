// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Windows;
using AmdnrLauncher.App.ViewModels;

namespace AmdnrLauncher.App;

public partial class MainWindow : Window
{
    private readonly MainViewModel _viewModel;

    public MainWindow()
    {
        // The command line is read here and nowhere else: a --manifest address lives in this
        // Settings instance for this run, and Settings never writes it out.
        var settings = Settings.Load();
        settings.ApplyCommandLine(Environment.GetCommandLineArgs().Skip(1));
        _viewModel = new MainViewModel(settings);

        InitializeComponent();
        DataContext = _viewModel;

        // No is the default button: Enter, or a click meant for the window behind, must not be
        // the thing that puts a mod into a game that bans for it.
        _viewModel.ConfirmAntiCheat = message => MessageBox.Show(
            this, message + "\n\nInstall anyway?", "Anti-cheat detected",
            MessageBoxButton.YesNo, MessageBoxImage.Warning, MessageBoxResult.No) == MessageBoxResult.Yes;

        // No is the default here too: the Alt+F4 or stray click on the X that this exists to
        // catch should not be answered by the next key press.
        _viewModel.ConfirmCloseMidChange = message => MessageBox.Show(
            this, message + "\n\nClose anyway?", Brand.Name,
            MessageBoxButton.YesNo, MessageBoxImage.Warning, MessageBoxResult.No) == MessageBoxResult.Yes;

        // Every way the window closes comes through here — the X, Alt+F4, the taskbar, the
        // system menu — not only the caption button's click handler.
        Closing += (_, e) => e.Cancel = !_viewModel.MayClose();

        // The caption is ours, but the frame around it is still the desktop window
        // manager's: without this the resize border and the snap-layouts flyout are drawn
        // in the light theme against a dark window.
        SourceInitialized += (_, _) => DarkTitleBar.Apply(this);

        // The maximise glyph has to follow the state, and the state changes by routes this
        // window never sees as a click: the snap gesture, Win+Up, the taskbar, a second
        // monitor being unplugged.
        StateChanged += (_, _) => SyncMaximiseGlyph();

        Loaded += async (_, _) => await _viewModel.StartAsync();
    }

    private void SyncMaximiseGlyph()
    {
        var maximised = WindowState == WindowState.Maximized;

        MaximiseButton.Content = FindResource(maximised ? "GlyphRestore" : "GlyphMaximise");
        MaximiseButton.ToolTip = maximised ? "Restore" : "Maximise";
    }

    private void OnMinimise(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;

    private void OnMaximiseRestore(object sender, RoutedEventArgs e)
        => WindowState = WindowState == WindowState.Maximized
            ? WindowState.Normal
            : WindowState.Maximized;

    private void OnClose(object sender, RoutedEventArgs e) => Close();

    /// <summary>The caption's Discord mark: the owner's server, in the default browser.</summary>
    private void OnDiscord(object sender, RoutedEventArgs e) => Services.Links.Open(Brand.DiscordUri);

    private void OnAddFolder(object sender, RoutedEventArgs e)
    {
        var dialog = new Microsoft.Win32.OpenFolderDialog
        {
            Title = "Select the folder that contains the game .exe",
        };

        if (dialog.ShowDialog(this) == true) _viewModel.AddManual(dialog.FolderName);
    }

    private void OnApplyUpdate(object sender, RoutedEventArgs e)
    {
        // Null means the new launcher is running and this one is closing. Anything else is a
        // message: wait for the task in progress, or what happened to the launcher they have
        // and where to get the new one.
        if (_viewModel.ApplyUpdate(Services.SelfUpdater.ApplyAndRestart) is { } problem)
            MessageBox.Show(this, problem, "Update", MessageBoxButton.OK, MessageBoxImage.Warning);
    }

    private void OnRestartElevated(object sender, RoutedEventArgs e)
    {
        // The button is bound to CanRestartElevated; this catches a click queued behind the one
        // that started an install, which Application.Shutdown below would cut short.
        if (!_viewModel.CanRestartElevated) return;

        try
        {
            var start = new System.Diagnostics.ProcessStartInfo
            {
                FileName = Environment.ProcessPath!,
                UseShellExecute = true,
                Verb = "runas",
            };

            // A tester on a test manifest must come back on the same one.
            foreach (var arg in Environment.GetCommandLineArgs().Skip(1)) start.ArgumentList.Add(arg);

            System.Diagnostics.Process.Start(start);
            Application.Current.Shutdown();
        }
        catch (System.ComponentModel.Win32Exception)
        {
            // The user declined the UAC prompt; stay where we are.
        }
    }
}
