// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class VdfReaderTests
{
    [Fact]
    public void Reads_nested_blocks_and_quoted_values()
    {
        var node = VdfReader.Parse("""
        "libraryfolders"
        {
            "0"
            {
                "path"      "C:\\Program Files (x86)\\Steam"
                "apps"
                {
                    "1234"  "567"
                }
            }
        }
        """);

        Assert.Equal(@"C:\Program Files (x86)\Steam", node["libraryfolders"]!["0"]!["path"]!.Value);
        Assert.Equal("567", node["libraryfolders"]!["0"]!["apps"]!["1234"]!.Value);
    }

    [Fact]
    public void Key_lookup_is_case_insensitive()
    {
        var node = VdfReader.Parse(""" "AppState" { "Name" "Silent Hill 2" } """);

        Assert.Equal("Silent Hill 2", node["appstate"]!["NAME"]!.Value);
    }

    [Fact]
    public void Skips_line_comments()
    {
        var node = VdfReader.Parse("""
        "root"
        {
            // a comment
            "k" "v"
        }
        """);

        Assert.Equal("v", node["root"]!["k"]!.Value);
    }

    [Fact]
    public void Missing_key_returns_null_rather_than_throwing()
    {
        Assert.Null(VdfReader.Parse(""" "root" { } """)["root"]!["nope"]);
    }
}
