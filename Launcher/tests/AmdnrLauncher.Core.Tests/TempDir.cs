// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Tests;

/// <summary>A temp directory that deletes itself. Names include an apostrophe and
/// brackets on purpose: real reference installs live under "Marvel's Spider-Man".</summary>
public sealed class TempDir : IDisposable
{
    public string Path { get; }

    public TempDir()
    {
        Path = System.IO.Path.Combine(
            System.IO.Path.GetTempPath(),
            "amdnr-test-" + Guid.NewGuid().ToString("N") + "-Marvel's [x]");
        Directory.CreateDirectory(Path);
    }

    public string Write(string relativePath, byte[] content)
    {
        var full = System.IO.Path.Combine(Path, relativePath);
        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(full)!);
        File.WriteAllBytes(full, content);
        return full;
    }

    public string Write(string relativePath, string content)
        => Write(relativePath, System.Text.Encoding.UTF8.GetBytes(content));

    public void Dispose()
    {
        try { Directory.Delete(Path, recursive: true); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
    }
}
