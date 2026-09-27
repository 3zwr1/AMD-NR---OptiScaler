// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.IO;
using AmdnrLauncher.App.Services;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.App.Tests;

public sealed class SelfUpdaterTests : IDisposable
{
    private const string ManualUrl =
        "https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/v0.4.1/AMDNR-Launcher.exe";

    // The folder names people actually install into. A swap that went through cmd.exe or any
    // other re-encoding step turned these into '?' and copied to a path that did not exist.
    private readonly string _root = Path.Combine(
        Path.GetTempPath(), "amdnr-selfupdate-" + Guid.NewGuid().ToString("N"));

    private readonly string _exe;
    private readonly string _staged;

    public SelfUpdaterTests()
    {
        var installDir = Path.Combine(_root, "ألعاب ÇÃO 游戏");
        var tempDir = Path.Combine(_root, "Temp 下载");
        Directory.CreateDirectory(installDir);
        Directory.CreateDirectory(tempDir);

        _exe = Path.Combine(installDir, "AMDNR-Launcher.exe");
        _staged = Path.Combine(tempDir, "AmdnrLauncher-0.4.1.exe");
        File.WriteAllText(_exe, "old launcher");
        File.WriteAllText(_staged, "new launcher");
    }

    public void Dispose()
    {
        try { Directory.Delete(_root, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }

    private LauncherInfo Update(string? sha256 = null)
        => new("0.4.1", ManualUrl, sha256 ?? Hashing.Sha256OfFile(_staged));

    [Fact]
    public void Swap_replaces_the_exe_and_keeps_the_old_one_aside_under_a_non_ASCII_folder()
    {
        var problem = SelfUpdater.Swap(_exe, _staged, Update());

        Assert.Null(problem);
        Assert.Equal("new launcher", File.ReadAllText(_exe));
        Assert.Equal("old launcher", File.ReadAllText(_exe + ".old"));
        Assert.False(File.Exists(_staged));
    }

    [Fact]
    public void A_failed_placement_puts_the_old_exe_back()
    {
        var update = Update();

        // Readable, so the hash check passes, but not deletable, so it cannot be moved: the
        // current exe has already been renamed aside by the time the placement fails. An
        // antivirus scanner holding the fresh download does exactly this.
        string? problem;
        using (new FileStream(_staged, FileMode.Open, FileAccess.Read, FileShare.Read))
            problem = SelfUpdater.Swap(_exe, _staged, update);

        Assert.NotNull(problem);
        Assert.Contains(ManualUrl, problem);
        Assert.Equal("old launcher", File.ReadAllText(_exe));
        Assert.False(File.Exists(_exe + ".old"));
        Assert.True(File.Exists(_staged));
    }

    [Fact]
    public void Undoing_a_swap_puts_the_old_exe_back_and_the_new_one_where_it_was_staged()
    {
        // What happens when the new exe is in place but will not start: shutting down then
        // would leave the user with a launcher that does not run.
        Assert.Null(SelfUpdater.Swap(_exe, _staged, Update()));

        Assert.True(SelfUpdater.Unswap(_exe, _staged));

        Assert.Equal("old launcher", File.ReadAllText(_exe));
        Assert.Equal("new launcher", File.ReadAllText(_staged));
        Assert.False(File.Exists(_exe + ".old"));
    }

    [Fact]
    public void Leftover_old_exe_is_removed_on_startup()
    {
        File.WriteAllText(_exe + ".old", "the launcher before the last update");

        SelfUpdater.RemoveLeftoverOldExe(_exe);

        Assert.False(File.Exists(_exe + ".old"));
        Assert.Equal("old launcher", File.ReadAllText(_exe));
    }

    [Fact]
    public void Removing_a_leftover_that_is_not_there_is_not_an_error()
        => Assert.True(SelfUpdater.RemoveLeftoverOldExe(_exe));

    [Fact]
    public void The_failure_message_names_the_manual_download_url()
    {
        var update = Update();

        // An exe that cannot be renamed is what a Program Files install looks like to a
        // launcher running without elevation: the swap cannot even start, so the only way
        // forward is for the user to fetch the new exe themselves.
        string? problem;
        using (new FileStream(_exe, FileMode.Open, FileAccess.Read, FileShare.Read))
            problem = SelfUpdater.Swap(_exe, _staged, update);

        Assert.NotNull(problem);
        Assert.Contains(ManualUrl, problem);
        Assert.Equal("old launcher", File.ReadAllText(_exe));
        Assert.Equal("new launcher", File.ReadAllText(_staged));
    }

    [Fact]
    public void A_staged_download_that_no_longer_matches_its_hash_is_never_swapped_in()
    {
        // The download was verified when it landed, but it then sits in %TEMP% until the user
        // clicks, and it is about to be run. It is checked again right before it replaces us.
        var problem = SelfUpdater.Swap(_exe, _staged, Update(sha256: new string('0', 64)));

        Assert.NotNull(problem);
        Assert.Contains(ManualUrl, problem);
        Assert.Equal("old launcher", File.ReadAllText(_exe));
        Assert.False(File.Exists(_exe + ".old"));
    }

    [Fact]
    public void Only_the_first_click_schedules_a_swap()
    {
        // A second click lands after the first swap has already put the new exe in place and
        // asked the app to shut down; running the swap again would move the new exe aside
        // over the old one the process is still running from.
        Assert.True(SelfUpdater.ScheduleTheOneSwap());
        Assert.False(SelfUpdater.ScheduleTheOneSwap());
    }
}
