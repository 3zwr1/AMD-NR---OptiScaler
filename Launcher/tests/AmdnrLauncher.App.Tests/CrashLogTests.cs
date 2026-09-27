// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using AmdnrLauncher.App.Services;

namespace AmdnrLauncher.App.Tests;

public sealed class CrashLogTests : IDisposable
{
    private readonly string _root = Path.Combine(
        Path.GetTempPath(), "amdnr-crash-" + Guid.NewGuid().ToString("N"));

    public void Dispose()
    {
        try { Directory.Delete(_root, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    [Fact]
    public void A_crash_is_appended_with_its_stack_and_the_launcher_version()
    {
        var path = Path.Combine(_root, "AMDNR", "crash.log");

        Assert.True(CrashLog.Write(path, new InvalidOperationException("first")));
        Assert.True(CrashLog.Write(path, new IOException("second")));

        var text = File.ReadAllText(path);
        Assert.Contains("System.InvalidOperationException: first", text);
        Assert.Contains("System.IO.IOException: second", text);
        Assert.Contains(SelfUpdater.CurrentVersion, text);
    }

    [Fact]
    public void An_unwritable_log_is_reported_rather_than_thrown()
    {
        // The handler runs while the app is already failing; a second exception from inside
        // it would replace the message the user was about to see.
        Directory.CreateDirectory(Path.Combine(_root, "crash.log"));

        Assert.False(CrashLog.Write(Path.Combine(_root, "crash.log"), new InvalidOperationException("x")));
    }

    [Fact]
    public void The_log_lives_in_the_AMDNR_data_folder()
    {
        Assert.Equal(
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDNR", "crash.log"),
            CrashLog.DefaultPath);
    }
}
