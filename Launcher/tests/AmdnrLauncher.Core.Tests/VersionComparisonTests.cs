// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.Core.Tests;

public class VersionComparisonTests
{
    [Theory]
    [InlineData("1.1.0", "1.0.0", true)]
    [InlineData("1.0.1", "1.0.0", true)]
    [InlineData("2.0.0", "1.9.9", true)]
    [InlineData("1.0.0", "1.0.0", false)]
    [InlineData("1.0.0", "1.1.0", false)]
    [InlineData("1.10.0", "1.9.0", true)]      // numeric, not lexicographic
    [InlineData("0.3.0", "0.2.1", true)]
    public void Compares_semantic_versions(string candidate, string current, bool expected)
        => Assert.Equal(expected, VersionComparison.IsNewer(candidate, current));

    [Theory]
    [InlineData("1.8.7-0.3.1", "1.8.6-0.3.1", true)]
    [InlineData("1.8.6-0.3.2", "1.8.6-0.3.1", true)]
    [InlineData("1.9.0-0.3.1", "1.8.10-0.3.1", true)]
    [InlineData("1.8.6-0.3.1", "1.8.6-0.3.1", false)]
    [InlineData("1.8.6-0.3.1", "1.8.7-0.3.1", false)]
    [InlineData("1.8.6-0.3.1", "1.8.6", true)]
    public void Compares_TheAutomatics_build_and_runtime_versions(string candidate, string current, bool expected)
    {
        // Their releases are named <OptiScaler build>-<runtime build>. System.Version cannot
        // read that, and "cannot read" answers "not newer" — so without this no update to
        // their build would ever be offered.
        Assert.Equal(expected, VersionComparison.IsNewer(candidate, current));
    }

    [Theory]
    [InlineData("0.4.0.0", "0.4.0")]
    [InlineData("0.4.0", "0.4.0.0")]
    public void A_trailing_zero_is_not_a_newer_version(string candidate, string current)
        => Assert.False(VersionComparison.IsNewer(candidate, current));

    [Fact]
    public void An_unparseable_version_is_never_treated_as_newer()
    {
        Assert.False(VersionComparison.IsNewer("not-a-version", "1.0.0"));
        Assert.False(VersionComparison.IsNewer("2.0.0", ""));
    }

    [Theory]
    [InlineData("0.4.0", "0.4.0-rc1", true)]        // a release beats its own pre-release
    [InlineData("0.4.0-rc1", "0.4.0", false)]
    [InlineData("0.4.0-rc2", "0.4.0-rc1", true)]    // two pre-releases compare ordinally
    [InlineData("0.4.0-rc1", "0.4.0-rc1", false)]
    [InlineData("0.4.1-rc1", "0.4.0", true)]        // the numbers decide first
    [InlineData("0.4.0-rc1", "0.3.9", true)]
    [InlineData("0.3.9", "0.4.0-rc1", false)]
    [InlineData("1.9.1-alpha", "1.8.6-0.3.1", true)]
    public void Launcher_version_comparison_handles_prerelease_suffixes(string candidate, string current, bool expected)
    {
        // A test build published as 0.4.1-rc1 read as "not a version" would never be offered,
        // and 0.4.0 would never replace the 0.4.0-rc1 a tester is running.
        Assert.Equal(expected, VersionComparison.IsNewer(candidate, current));
    }
}
