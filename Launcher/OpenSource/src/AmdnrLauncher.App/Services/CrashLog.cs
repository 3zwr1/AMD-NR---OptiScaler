// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;

namespace AmdnrLauncher.App.Services;

/// <summary>Where an exception nothing else caught is written down. Appended, not replaced:
/// the crash a user reports is often the second one, and the first explains it.</summary>
internal static class CrashLog
{
    public static string DefaultPath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDNR", "crash.log");

    /// <summary>False when the log could not be written. Never throws: this runs while the app
    /// is already failing, and a second exception from here would replace the message the user
    /// was about to see with nothing at all.</summary>
    public static bool Write(string path, Exception exception)
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.AppendAllText(path,
                $"==== {DateTimeOffset.Now:yyyy-MM-dd HH:mm:ss zzz}  {Brand.Name} {SelfUpdater.CurrentVersion}" +
                $"{Environment.NewLine}{exception}{Environment.NewLine}{Environment.NewLine}");
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }
}
