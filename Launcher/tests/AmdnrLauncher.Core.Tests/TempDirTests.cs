// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Tests;

public class TempDirTests
{
    [Fact]
    public void Disposing_a_folder_it_may_not_delete_does_not_fail_the_test_that_used_it()
    {
        // A read-only file makes the recursive delete throw UnauthorizedAccessException, which
        // is not an IOException. Game folders hold read-only files, and a test that copies one
        // must not fail in its cleanup for it.
        var dir = new TempDir();
        var readOnly = dir.Write("readonly.dll", "stand-in");
        File.SetAttributes(readOnly, FileAttributes.ReadOnly);

        try
        {
            dir.Dispose();
        }
        finally
        {
            File.SetAttributes(readOnly, FileAttributes.Normal);
            Directory.Delete(dir.Path, recursive: true);
        }
    }
}
