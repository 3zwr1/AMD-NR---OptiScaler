// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Windows;
using System.Windows.Threading;

namespace AmdnrLauncher.App;

public partial class App : Application
{
    private static int _crashShown;

    protected override void OnStartup(StartupEventArgs e)
    {
        // First, so that nothing after it — the window included — can fail without a trace.
        DispatcherUnhandledException += OnDispatcherUnhandledException;
        AppDomain.CurrentDomain.UnhandledException += (_, args) =>
        {
            if (args.ExceptionObject is Exception exception) ReportCrash(exception);
        };

        base.OnStartup(e);

        // A self-update leaves the launcher it replaced beside us as <exe>.old; nothing else
        // ever removes it.
        Services.SelfUpdater.RemoveLeftoverOldExeInBackground();
    }

    /// <summary>Handled, then the app closes: carrying on after an exception nobody planned for
    /// could mean carrying on half-way through a change to a game folder.</summary>
    private void OnDispatcherUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
    {
        e.Handled = true;
        ReportCrash(e.Exception);
        Shutdown(1);
    }

    /// <summary>One message, however many handlers fire and however many exceptions follow the
    /// first: a cascade of dialogs is worse than the crash.</summary>
    private static void ReportCrash(Exception exception)
    {
        var path = Services.CrashLog.DefaultPath;
        var written = Services.CrashLog.Write(path, exception);

        if (Interlocked.Exchange(ref _crashShown, 1) != 0) return;

        MessageBox.Show(
            Brand.Name + " ran into a problem it could not recover from and has to close.\n\n" +
            (written
                ? $"The details were saved to\n{path}\nPlease include that file when you report the problem."
                : $"The details could not be saved: {exception.Message}"),
            Brand.Name, MessageBoxButton.OK, MessageBoxImage.Error);
    }
}
