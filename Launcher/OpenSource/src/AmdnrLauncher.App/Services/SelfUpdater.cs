// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Net.Http;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.App.Services;

public sealed class SelfUpdater(HttpClient http)
{
    public static string CurrentVersion =>
        typeof(SelfUpdater).Assembly.GetName().Version?.ToString(3) ?? "0.0.0";

    public async Task<string?> DownloadIfNewerAsync(
        LauncherInfo info, string currentVersion, CancellationToken ct)
    {
        if (!VersionComparison.IsNewer(info.Version, currentVersion)) return null;

        var staged = Path.Combine(Path.GetTempPath(), $"AmdnrLauncher-{info.Version}.exe");
        await new ResumableDownloader(http)
            .DownloadAsync(info.Url, staged, info.Sha256, 0, null, ct);

        return staged;
    }

    /// <summary>Puts the staged download in place of the running exe, starts it and shuts this
    /// process down. Returns null on success — the app is closing at that point — or a message
    /// to show the user. The swap is done here, in-process, with renames: the earlier batch
    /// file handed the paths to cmd.exe, which re-reads them in the console code page and so
    /// could not find a launcher sitting in an Arabic, Chinese or accented folder at all.
    /// Every failure leaves the user with the launcher they already had.</summary>
    public static string? ApplyAndRestart(string stagedExe, LauncherInfo update)
    {
        var exe = Environment.ProcessPath;
        if (exe is null)
            return ManualUpdateMessage("Could not determine the launcher's own location.", "the launcher", update);

        // The first click either succeeded, and the app is on its way down, or failed and was
        // told why. Repeating that answer is all a second click can safely get: a second swap
        // would start from whatever state the first one left behind.
        if (!ScheduleTheOneSwap()) return _swapProblem;

        if (Swap(exe, stagedExe, update) is { } problem) return _swapProblem = problem;

        try
        {
            var start = new ProcessStartInfo(exe) { UseShellExecute = false };

            // A tester started with a test manifest must come back on the same one.
            foreach (var arg in Environment.GetCommandLineArgs().Skip(1)) start.ArgumentList.Add(arg);

            Process.Start(start);
        }
        catch (Exception e) when (e is Win32Exception
                                    or IOException
                                    or UnauthorizedAccessException)
        {
            // The new exe is in place but will not run — an antivirus or a policy is blocking
            // it. Shutting down now would leave the user with a launcher that does not start.
            return _swapProblem = Unswap(exe, stagedExe)
                ? ManualUpdateMessage(
                    $"The new launcher could not be started ({e.Message}). The one you had is back in place.",
                    Path.GetFileName(exe), update)
                : CouldNotRestoreMessage(exe, update);
        }

        System.Windows.Application.Current.Shutdown();
        return null;
    }

    /// <summary>Runs at every startup. The launcher an update replaced is normally still
    /// running at that moment — it started us and is on its way down — and Windows will not
    /// delete an exe a process is running from, so this tries for a while, off the UI thread,
    /// rather than once.</summary>
    public static void RemoveLeftoverOldExeInBackground()
    {
        if (Environment.ProcessPath is not { } exe) return;

        _ = Task.Run(async () =>
        {
            for (var attempt = 0; attempt < 20 && !RemoveLeftoverOldExe(exe); attempt++)
                await Task.Delay(TimeSpan.FromMilliseconds(500));
        });
    }

    /// <summary>True when there is no leftover any more. Best-effort by design: a leftover that
    /// cannot be deleted costs disk space, nothing else, and is tried again next startup.</summary>
    internal static bool RemoveLeftoverOldExe(string exe)
    {
        try
        {
            File.Delete(OldPathFor(exe));
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    /// <summary>The file operations of an update, without the process start and shutdown
    /// around them. Returns null when the new exe is in place and the old one is beside it
    /// as <c>&lt;exe&gt;.old</c>; otherwise a message, with the old exe back under its own name
    /// whenever that is at all possible.</summary>
    internal static string? Swap(string exe, string staged, LauncherInfo update)
    {
        var name = Path.GetFileName(exe);

        // Checked again here, not only when the download finished: it has sat in %TEMP% since
        // then, and the next thing that happens to it is that it gets run.
        if (Hashing.TrySha256OfFile(staged) is not { } actual || !Hashing.Matches(actual, update.Sha256))
        {
            return ManualUpdateMessage(
                "The downloaded update is missing or no longer matches its checksum.", name, update);
        }

        var old = OldPathFor(exe);
        try
        {
            // Windows refuses to overwrite or delete a running exe but lets it be renamed; the
            // process carries on from the renamed file. Overwrite clears a leftover from an
            // earlier update that could not be deleted at the time.
            File.Move(exe, old, overwrite: true);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            // Typically a launcher kept under Program Files, run without elevation.
            return ManualUpdateMessage(
                $"The launcher could not be replaced where it is ({e.Message}).", name, update);
        }

        try
        {
            File.Move(staged, exe);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return PutOldBack(exe, old)
                ? ManualUpdateMessage(
                    $"The new launcher could not be put in place ({e.Message}). The one you had is back where it was.",
                    name, update)
                : CouldNotRestoreMessage(exe, update);
        }

        return null;
    }

    /// <summary>Reverses a Swap that completed. The new exe goes back to where it was staged,
    /// so the path the app is holding still points at a verified download; if that fails it
    /// is simply overwritten, because the old exe returning matters more than the download.</summary>
    internal static bool Unswap(string exe, string staged)
    {
        try
        {
            File.Move(exe, staged, overwrite: true);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            // PutOldBack overwrites it instead.
        }

        return PutOldBack(exe, OldPathFor(exe));
    }

    private static bool PutOldBack(string exe, string old)
    {
        try
        {
            // Overwrite, because a move that failed part-way across drives can leave a partial
            // copy under the exe's name.
            File.Move(old, exe, overwrite: true);
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    internal static string OldPathFor(string exe) => exe + ".old";

    private static string? _swapProblem;
    private static int _swapScheduled;

    /// <summary>True for the first caller only. A second click on "Restart to update the
    /// launcher" after a successful swap would move the new exe aside on top of the old one
    /// this process is still running from.</summary>
    internal static bool ScheduleTheOneSwap()
        => Interlocked.Exchange(ref _swapScheduled, 1) == 0;

    private static string ManualUpdateMessage(string what, string exeName, LauncherInfo update)
        => $"{what}\n\nThe launcher you have still works. To update it yourself, download " +
           $"version {update.Version} from\n{update.Url}\nand put it in place of {exeName}.";

    private static string CouldNotRestoreMessage(string exe, LauncherInfo update)
        => $"The update failed, and the launcher you had could not be moved back under its own " +
           $"name. It is at\n{OldPathFor(exe)}\nRename it to {Path.GetFileName(exe)} to use it " +
           $"again, or download version {update.Version} from\n{update.Url}";
}
