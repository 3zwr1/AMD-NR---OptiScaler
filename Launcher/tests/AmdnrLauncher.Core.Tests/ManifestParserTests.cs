// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class ManifestParserTests
{
    private const string Sample = """
    {
      "schemaVersion": 1,
      "launcher": { "version": "1.0.0", "url": "https://h/l.exe", "sha256": "AA" },
      "packages": [
        { "id": "amdnr",   "version": "0.2.1", "url": "https://h/a.zip", "sha256": "BB", "size": 314572800 },
        { "id": "runtime", "version": "0.3.1", "url": "https://h/r.zip", "sha256": "CC", "size": 178000000,
          "passSha256": ["0217AA", "03BB", "031CC"] },
        { "id": "forwarder", "version": "0.2.1", "url": "https://h/f.dll", "sha256": "F5935DD3", "size": 114688 }
      ],
      "compatibility": [ { "amdnr": "0.2.1", "runtime": ["0.3.1", "0.3.0"] } ],
      "proxyDefaults": ["dxgi.dll", "d3d12.dll", "winmm.dll", "version.dll", "dbghelp.dll", "d3d11.dll"],
      "proxyOverrides": [ { "exe": "SomeGame.exe", "proxy": "winmm.dll", "reason": "overlay owns dxgi" } ]
    }
    """;

    [Fact]
    public void The_launcher_block_is_read_from_a_manifest_that_does_not_parse()
    {
        // A newer schema is refused as a whole, but its launcher block is the update that
        // would read it.
        var future = Sample.Replace("\"schemaVersion\": 1", "\"schemaVersion\": 2");
        Assert.Throws<UnsupportedManifestException>(() => ManifestParser.Parse(future));

        Assert.Equal(new LauncherInfo("1.0.0", "https://h/l.exe", "AA"), ManifestParser.TryReadLauncher(future));
        Assert.Equal(new LauncherInfo("2.0.0", "https://h/2.exe", "DD"),
            ManifestParser.TryReadLauncher("""{ "LAUNCHER": { "Version": "2.0.0", "url": "https://h/2.exe", "sha256": "DD" }, "packages": 7 }"""));
    }

    [Theory]
    [InlineData("<html>captive portal</html>")]
    [InlineData("""{ "launcher": { "version": "1.0.0", "url": "https://h/l.exe" } }""")]
    [InlineData("""{ "launcher": { "version": "", "url": "https://h/l.exe", "sha256": "AA" } }""")]
    [InlineData("""{ "launcher": "1.0.0" }""")]
    [InlineData("[1, 2]")]
    public void No_launcher_block_is_read_from_a_document_without_a_complete_one(string json)
        => Assert.Null(ManifestParser.TryReadLauncher(json));

    [Fact]
    public void Parses_all_sections()
    {
        var m = ManifestParser.Parse(Sample);

        Assert.Equal(1, m.SchemaVersion);
        Assert.Equal("1.0.0", m.Launcher.Version);
        Assert.Equal(3, m.Packages.Count);
        Assert.Equal(178000000, m.Package("runtime").Size);
        Assert.Equal(6, m.ProxyDefaults.Count);
        Assert.Equal("winmm.dll", m.ProxyOverrides.Single().Proxy);
    }

    [Fact]
    public void PassSha256_is_a_list_of_all_accepted_runtime_builds()
    {
        // The runtime accepts three builds (kAmdLayouts[]); a single value
        // would reject two valid installs.
        var runtime = ManifestParser.Parse(Sample).Package("runtime");

        Assert.Equal(new[] { "0217AA", "03BB", "031CC" }, runtime.PassSha256);
    }

    [Fact]
    public void PassSha256_is_null_for_packages_that_do_not_carry_pass_dlls()
    {
        Assert.Null(ManifestParser.Parse(Sample).Package("amdnr").PassSha256);
    }

    [Fact]
    public void Unknown_fields_are_ignored()
    {
        var json = Sample.Replace("\"schemaVersion\": 1,",
                                  "\"schemaVersion\": 1, \"somethingNew\": { \"a\": [1,2] },");

        Assert.Equal(3, ManifestParser.Parse(json).Packages.Count);
    }

    [Fact]
    public void Higher_schemaVersion_is_refused_with_a_clear_error()
    {
        var json = Sample.Replace("\"schemaVersion\": 1", "\"schemaVersion\": 2");

        var ex = Assert.Throws<UnsupportedManifestException>(() => ManifestParser.Parse(json));
        Assert.Contains("update", ex.Message, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void Missing_package_throws_with_the_id_in_the_message()
    {
        var ex = Assert.Throws<KeyNotFoundException>(() => ManifestParser.Parse(Sample).Package("nope"));
        Assert.Contains("nope", ex.Message);
    }

    /// <summary>The shape the release manifest has: two sources, per-package excludes, a
    /// hidden-app list, and no compatibility table.</summary>
    private const string WithSources = """
    {
      "schemaVersion": 1,
      "launcher": { "version": "0.4.0", "url": "https://h/AMDNR-Launcher.exe", "sha256": "AA" },
      "sources": [
        { "id": "amdnr", "name": "AMDNR", "author": "3zwr1",
          "summary": "OptiScaler with AMD Neural Rendering.",
          "homepage": "https://github.com/3zwr1/AMD-NR---OptiScaler",
          "package": "amdnr", "forwarder": "forwarder",
          "runtimeRequired": false,
          "runtimeNote": "Needed on RX 7000." },
        { "id": "theautomatic", "name": "OptiScaler AMD pre-SR", "author": "TheAutomatic",
          "summary": "Multi-slot pre-SR build.",
          "homepage": "https://github.com/TheAutomatic/dlss-5-amd-project",
          "package": "theautomatic", "forwarder": null,
          "runtimeRequired": true,
          "runtimeNote": "Required: this build ships no neural runtime of its own." }
      ],
      "packages": [
        { "id": "amdnr", "version": "0.3.2", "url": "https://h/f.zip", "sha256": "BB", "size": 1, "exclude": [] },
        { "id": "theautomatic", "version": "1.8.6-0.3.1", "url": "https://h/t.zip", "sha256": "CC", "size": 2,
          "exclude": ["Setup.bat", "Setup.ps1", "tools/"] },
        { "id": "runtime", "version": "0.3.1", "url": "https://h/r.zip", "sha256": "DD", "size": 3,
          "passSha256": ["01", "02", "03"] },
        { "id": "forwarder", "version": "0.2.1", "url": "https://h/n.dll", "sha256": "EE", "size": 114688 }
      ],
      "proxyDefaults": ["dxgi.dll", "d3d12.dll", "winmm.dll", "version.dll", "dbghelp.dll", "d3d11.dll"],
      "proxyOverrides": [ { "exe": "Spider-Man.exe", "proxy": "dbghelp.dll", "reason": "dxgi.dll does not load" } ],
      "hiddenSteamAppIds": ["228980"]
    }
    """;

    [Fact]
    public void Manifest_with_sources_parses_and_TrySource_finds_by_id_case_insensitively()
    {
        var m = ManifestParser.Parse(WithSources);

        Assert.Equal(2, m.Sources!.Count);

        var amdnr = m.TrySource("AMDNR");
        Assert.NotNull(amdnr);
        Assert.Equal("AMDNR", amdnr!.Name);
        Assert.Equal("3zwr1", amdnr.Author);
        Assert.Equal("amdnr", amdnr.Package);
        Assert.Equal("forwarder", amdnr.Forwarder);
        Assert.False(amdnr.RuntimeRequired);
        Assert.Equal("Needed on RX 7000.", amdnr.RuntimeNote);

        var automatic = m.TrySource("TheAutomatic");
        Assert.NotNull(automatic);
        Assert.Null(automatic!.Forwarder);
        Assert.True(automatic.RuntimeRequired);
        Assert.Equal("https://github.com/TheAutomatic/dlss-5-amd-project", automatic.Homepage);

        Assert.Null(m.TrySource("nope"));

        Assert.Equal(["Setup.bat", "Setup.ps1", "tools/"], m.Package("theautomatic").Exclude!);
        Assert.Empty(m.Package("amdnr").Exclude!);
        Assert.Equal(["228980"], m.HiddenSteamAppIds!);
    }

    [Fact]
    public void A_source_names_the_ini_settings_it_needs_with_the_runtime()
    {
        var json = WithSources.Replace(
            "\"runtimeNote\": \"Required: this build ships no neural runtime of its own.\" }",
            "\"runtimeNote\": \"Required.\", \"iniWhenRuntime\": [ " +
            "{ \"section\": \"DlssNr\", \"key\": \"NrBackend\", \"value\": \"daniel\" }, " +
            // Each of these would write a broken line into someone's ini, or a key into no section.
            "{ \"section\": \"\", \"key\": \"K\", \"value\": \"v\" }, " +
            "{ \"section\": \"S\", \"key\": \" \", \"value\": \"v\" }, " +
            "{ \"section\": \"S\", \"key\": \"K\" }, " +
            "{ \"section\": \"S\", \"key\": \"K\", \"value\": \"two\\nlines\" }, " +
            "{ \"section\": \"S]\", \"key\": \"K\", \"value\": \"v\" }, " +
            "{ \"section\": \"S\", \"key\": \"K=\", \"value\": \"v\" } ] }");

        var m = ManifestParser.Parse(json);

        Assert.Equal([new IniSetting("DlssNr", "NrBackend", "daniel")], m.TrySource("theautomatic")!.IniWhenRuntime!);
        Assert.Null(m.TrySource("amdnr")!.IniWhenRuntime);
    }

    [Fact]
    public void Manifest_without_sources_or_hidden_ids_parses_with_nulls()
    {
        // Forward compatibility runs both ways: a manifest that predates these members is
        // still a manifest, and every reader has to cope with them being absent.
        var m = ManifestParser.Parse(Sample);

        Assert.Null(m.Sources);
        Assert.Null(m.HiddenSteamAppIds);
        Assert.Null(m.Package("amdnr").Exclude);
    }

    [Fact]
    public void A_manifest_without_sources_still_offers_AMDNR_as_every_earlier_manifest_did()
    {
        // An older cached manifest used offline, or the published one before it gains the list.
        // Every such manifest offered exactly one build — AMDNR, with the runtime and its
        // forwarder — and reading it as offering none would cost every AMDNR install its Doctor
        // checks and its update detection, and leave Install with nothing to install.
        var m = ManifestParser.Parse(Sample);

        var amdnr = m.TrySource("AMDNR");
        Assert.NotNull(amdnr);
        Assert.Equal("amdnr", amdnr!.Id);
        Assert.Equal("AMDNR", amdnr.Name);
        Assert.Equal("amdnr", amdnr.Package);
        Assert.Equal("forwarder", amdnr.Forwarder);
        Assert.False(amdnr.RuntimeRequired);
        Assert.Equal([amdnr], m.OfferedSources);
        Assert.Null(m.TrySource("theautomatic"));
    }

    [Fact]
    public void A_manifest_that_lists_sources_offers_only_those()
    {
        // Once a manifest names its builds, a missing AMDNR is one the publisher withdrew.
        var m = ManifestParser.Parse(WithSources) with { Sources = [] };

        Assert.Empty(m.OfferedSources);
        Assert.Null(m.TrySource("amdnr"));
    }

    [Fact]
    public void A_manifest_without_a_compatibility_table_reads_it_as_empty()
    {
        // The release manifest has none. Left null, the Doctor check that walks it would throw
        // for every installed game on every scan.
        Assert.Empty(ManifestParser.Parse(WithSources).Compatibility);
    }

    [Fact]
    public void A_source_missing_its_id_or_package_is_dropped_rather_than_offered()
    {
        // Offered anyway, it is a choice that fails only at install time, after the user
        // has picked it and waited for the download.
        var json = WithSources.Replace("\"package\": \"theautomatic\", ", "");

        var m = ManifestParser.Parse(json);

        Assert.Single(m.Sources!);
        Assert.Null(m.TrySource("theautomatic"));
    }

    [Fact]
    public void Proxy_names_that_are_not_proxy_dlls_are_dropped()
    {
        // A proxy name becomes a path in the game folder. "..\dxgi.dll" or a rooted path would
        // have the installer write outside it, over a file it keeps no backup of — and the
        // launcher often runs as administrator.
        var json = Sample
            .Replace("\"proxyDefaults\": [\"dxgi.dll\",",
                @"""proxyDefaults"": [""..\\dxgi.dll"", ""C:\\Windows\\System32\\dxgi.dll"", ""dxgi.dll"",")
            .Replace("\"proxy\": \"winmm.dll\"", @"""proxy"": ""..\\winmm.dll""");

        var m = ManifestParser.Parse(json);

        Assert.Equal(PayloadNames.ProxyNames, m.ProxyDefaults);
        Assert.Empty(m.ProxyOverrides);
    }

    [Fact]
    public void A_manifest_without_proxy_lists_or_packages_reads_them_as_empty()
    {
        var m = ManifestParser.Parse("""
            { "schemaVersion": 1, "launcher": { "version": "1.0.0", "url": "https://h/l.exe", "sha256": "AA" } }
            """);

        Assert.Empty(m.Packages);
        Assert.Empty(m.ProxyDefaults);
        Assert.Empty(m.ProxyOverrides);
        Assert.Null(m.TryPackage("amdnr"));
    }

    [Theory]
    [InlineData("\"id\": \"..\"")]
    [InlineData(@"""id"": ""..\\..\\x""")]
    [InlineData(@"""version"": ""D:\\SteamLibrary""")]
    [InlineData("\"version\": \"0.2.1/../..\"")]
    [InlineData("\"version\": \"\"")]
    public void A_package_whose_id_or_version_is_not_one_folder_name_is_dropped(string member)
    {
        // Both become folder names under the download cache, and a stale version folder there
        // is deleted recursively before a download: a rooted or ".." one points that delete
        // anywhere on the disk.
        var name = member[1..member.IndexOf('"', 1)];
        var json = Sample.Replace(
            name == "id" ? "\"id\": \"amdnr\"" : "\"version\": \"0.2.1\", \"url\": \"https://h/a.zip\"",
            name == "id" ? member : member + ", \"url\": \"https://h/a.zip\"");

        var m = ManifestParser.Parse(json);

        Assert.Equal(2, m.Packages.Count);
        Assert.Null(m.TryPackage("amdnr"));
    }

    [Theory]
    [InlineData("https://h/C:version.dll")]
    [InlineData("https://h/C%3Aversion.dll")]
    [InlineData("https://h/files/")]
    [InlineData("f.dll")]
    public void A_file_package_whose_url_does_not_end_in_one_plain_file_name_is_dropped(string url)
    {
        // A package that is not a zip is stored under the last segment of its URL, joined onto
        // its folder in the download cache. "C:version.dll" is a drive-relative path: joined, it
        // is the whole result, and the download lands beside the launcher's own exe, which loads
        // a version.dll from there.
        var m = ManifestParser.Parse(Sample.Replace("\"url\": \"https://h/f.dll\"", $"\"url\": \"{url}\""));

        Assert.Null(m.TryPackage("forwarder"));
        Assert.Equal(2, m.Packages.Count);
    }

    [Fact]
    public void The_sample_manifest_in_the_repository_parses_and_names_packages_it_lists()
    {
        var m = ManifestParser.Parse(File.ReadAllText(FindUpwards("manifest.sample.json")));

        Assert.NotEmpty(m.Sources!);
        foreach (var source in m.Sources!)
        {
            Assert.NotNull(m.TryPackage(source.Package));
            if (source.Forwarder is not null) Assert.NotNull(m.TryPackage(source.Forwarder));
            Assert.NotEmpty(source.Runtimes!);
            foreach (var runtime in source.Runtimes!) Assert.Equal(8, m.Package(runtime.Package).PassSha256!.Count);
        }
    }

    [Fact]
    public void manifest_json_parses_and_its_theautomatic_and_forwarder_entries_match_the_plan()
    {
        // The file every launcher reads. A wrong hash or size here fails the download for every
        // user at once, and the wrong forwarder is the 19 September build the owner withdrew.
        var text = File.ReadAllText(FindUpwards("manifest.json"));
        var m = ManifestParser.Parse(text);

        var automatic = m.Package("theautomatic");
        Assert.Equal("1.9.1-alpha", automatic.Version);
        Assert.Equal(
            "https://github.com/TheAutomatic/dlss-5-amd-project/releases/download/v1.9.1-alpha/OptiScaler-AMD-PreSR-1.9.1-alpha.zip",
            automatic.Url);
        Assert.Equal(132_549_025, automatic.Size);
        Assert.Equal("bc0799ee408f50a0766f250118022a6c030f46a3d2bf75e9715266ad2a3c247a", automatic.Sha256,
            ignoreCase: true);
        Assert.Equal(
            ["Setup.bat", "Setup.ps1", "Uninstall_OptiScaler_NR.bat", "Uninstall_OptiScaler_NR.ps1"],
            automatic.Exclude!);

        var source = m.TrySource("theautomatic")!;
        Assert.True(source.RuntimeRequired);
        Assert.Null(source.Forwarder);
        // Their packaged ini also ships NR switched off, and their own Setup.ps1 switches it on
        // with the backend: selecting daniel alone leaves Neural Rendering off.
        Assert.Equal(
            [new IniSetting("DlssNr", "Enabled", "true"), new IniSetting("DlssNr", "NrBackend", "daniel")],
            source.IniWhenRuntime!);

        var forwarder = m.Package(m.TrySource("amdnr")!.Forwarder!);
        Assert.Equal("5976915A7F59E57C6FD5FFE4742468DD9DB19503C894EF21810A0AD0F941F419", forwarder.Sha256,
            ignoreCase: true);
        Assert.Equal(114_688, forwarder.Size);

        foreach (var offered in m.Sources!) Assert.NotNull(m.TryPackage(offered.Package));
        Assert.DoesNotContain("FSRNR", text, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void manifest_json_lists_eight_accepted_pass_hashes_and_the_runtimes_by_card()
    {
        // The mod moved to 0.3.3.2 with three danielblnc runtimes, and its AmdLayout.h accepts
        // eight pass-DLL layouts. A manifest still listing three has the Doctor call a good 0.3.3
        // or 0.4.0 runtime unverified, and a wrong hash or size fails the download for everyone.
        var m = ManifestParser.Parse(File.ReadAllText(FindUpwards("manifest.json")));

        var amdnr = m.Package("amdnr");
        Assert.Equal("0.3.3.2", amdnr.Version);
        Assert.Equal(
            "https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.3.2/AMDNR-v0.3.3.2.zip",
            amdnr.Url);
        Assert.Equal(577_549_740, amdnr.Size);
        Assert.Equal("00bb375d5d18d239dcb4820f59e4dd2c744445d783fae7f7df1b2dc24ac7e0c9", amdnr.Sha256, ignoreCase: true);
        // lmxxf runs on RDNA 3 since 0.3.3, so the .pak is needed on RX 7000 as well: nothing excluded.
        Assert.Empty(amdnr.Exclude!);

        var runtime033 = m.Package("runtime-033");
        Assert.Equal("0.3.3", runtime033.Version);
        Assert.Equal(
            // Read from Alpha0.3.3.1: the zip went missing from Alpha0.3.3.2 on 26 September, and the
            // manifest is generated with -RuntimeTags until the owner attaches it again.
            "https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.3.1/v0.3.3-Runtime.zip",
            runtime033.Url);
        Assert.Equal(109_730_870, runtime033.Size);
        Assert.Equal("a44a70736ba248d6c5b4b41ccb8d8924aed784a6fc1b58b74f5999b3994a206c", runtime033.Sha256, ignoreCase: true);

        var runtime040 = m.Package("runtime-040");
        Assert.Equal("0.4.0", runtime040.Version);
        Assert.Equal(
            "https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.3.2/v0.4.0-Runtime.zip",
            runtime040.Url);
        Assert.Equal(113_039_884, runtime040.Size);
        Assert.Equal("15ebed60b7946f4eb616a8b89a6c8d8038886a2bfead2f9cf299175d0ec5b973", runtime040.Sha256, ignoreCase: true);

        var runtime041 = m.Package("runtime-041");
        Assert.Equal("0.4.1", runtime041.Version);
        Assert.Equal(
            "https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.3.2/v0.4.1-Runtime.zip",
            runtime041.Url);
        Assert.Equal(112_707_880, runtime041.Size);
        Assert.Equal("d6aa1ff24f4d1379b2914e31403c8027bf556d59f87ec265ea79502b60e4d492", runtime041.Sha256, ignoreCase: true);

        // kAmdLayouts[] in OptiScaler\dlssnr\amd\AmdLayout.h, in its order, by the first and last
        // byte its static_asserts state: 0.2.17, 0.3, 0.3.1, 0.3.2, 0.3.3, 0.4.0, 0.4.1, then one
        // layout the mod already accepts for a runtime that is not published yet.
        (string First, string Last)[] layouts =
            [("bc", "4e"), ("83", "38"), ("b1", "54"), ("b9", "1e"), ("90", "12"), ("d6", "80"), ("82", "76"), ("8a", "5a")];
        foreach (var runtime in new[] { runtime033, runtime040, runtime041 })
        {
            var hashes = runtime.PassSha256!;
            Assert.Equal(8, hashes.Count);
            Assert.All(hashes, h => Assert.Matches("^[0-9a-f]{64}$", h));
            Assert.Equal(8, hashes.Distinct().Count());
            Assert.Equal(layouts, hashes.Select(h => (h[..2], h[^2..])));
        }
        Assert.Equal(runtime033.PassSha256, runtime040.PassSha256);
        Assert.Equal(runtime033.PassSha256, runtime041.PassSha256);

        // 0.4.1 first, RX 9000 only: the owner's runtime for that card since 26 September 2026.
        // 0.3.3 next, for every generation the launcher can name. 0.4.0 stays listed behind both,
        // so its package and hashes remain, but no card reaches it by default.
        Assert.Equal(2, m.Sources!.Count);
        foreach (var source in m.Sources!)
        {
            var runtimes = source.Runtimes!;
            Assert.Equal(["runtime-041", "runtime-033", "runtime-040"], runtimes.Select(r => r.Package));
            Assert.Equal(["rdna4"], runtimes[0].Gpus);
            Assert.Equal(["rdna2", "rdna3", "rdna4", "unknown-amd", "none"], runtimes[1].Gpus);
            Assert.Equal(["rdna4"], runtimes[2].Gpus);
            Assert.Equal("runtime-033", source.RuntimePackageFor("rdna3"));
            Assert.Equal("runtime-041", source.RuntimePackageFor("rdna4"));
            Assert.Null(source.RuntimePackageFor("rdna1"));
        }
    }

    [Fact]
    public void manifest_json_names_the_DLL_for_the_games_the_owner_has_running()
    {
        // Each from the owner's own working install, so the first name offered is the one known
        // to load there. Resident Evil Requiem is the one that sent the owner round every name:
        // REFramework holds dinput8.dll, and OptiScaler loads as dxgi.dll. The script table has
        // to agree, or the next regenerated manifest silently loses them.
        var m = ManifestParser.Parse(File.ReadAllText(FindUpwards("manifest.json")));
        var script = File.ReadAllText(FindUpwards(Path.Combine("tools", "make-manifest.ps1")));

        var expected = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["re9.exe"] = "dxgi.dll",
            ["Spider-Man.exe"] = "dbghelp.dll",
            ["forzahorizon6.exe"] = "dxgi.dll",
            ["GTA5_Enhanced.exe"] = "dxgi.dll",
            ["SHProto-Win64-Shipping.exe"] = "dxgi.dll",
            ["Stray-Win64-Shipping.exe"] = "dxgi.dll",
        };

        var overrides = m.ProxyOverrides.ToDictionary(o => o.Exe, o => o, StringComparer.OrdinalIgnoreCase);
        Assert.Equal(expected.Count, m.ProxyOverrides.Count);
        foreach (var (exe, proxy) in expected)
        {
            Assert.Equal(proxy, overrides[exe].Proxy);
            Assert.False(string.IsNullOrWhiteSpace(overrides[exe].Reason));
            Assert.Contains($"exe = '{exe}'; proxy = '{proxy}'", script);
        }

        Assert.Contains("REFramework", overrides["re9.exe"].Reason);
        Assert.Contains("dinput8.dll", overrides["re9.exe"].Reason);
    }

    /// <summary>AMDNR's source from WithSources, offering danielblnc's two runtimes the way the
    /// release manifest does: 0.3.3 first, for every generation; 0.4.0 second, for RX 9000 only.</summary>
    private const string WithRuntimes = """
        "runtimeNote": "Needed on RX 7000.",
        "runtimes": [
          { "package": "runtime-033", "gpus": ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"] },
          { "package": "runtime-040", "gpus": ["rdna4"] } ] }
    """;

    [Fact]
    public void Runtimes_pick_the_first_entry_that_lists_the_gpu()
    {
        // 0.3.3 runs on RX 7000 and RX 9000 and is the default everywhere. 0.4.0's fast kernels
        // are RX 9000 only and, in the owner's words, not yet tested in a game: it is listed second,
        // so rdna4 still gets 0.3.3 until the owner reorders the manifest. Order, not specificity,
        // decides, because the manifest is the one place the owner can change the default from.
        var amdnr = ManifestParser.Parse(WithSources.Replace("\"runtimeNote\": \"Needed on RX 7000.\" }", WithRuntimes))
            .TrySource("amdnr")!;

        Assert.Equal(["runtime-033", "runtime-040"], amdnr.Runtimes!.Select(r => r.Package));
        Assert.Equal(["rdna2", "rdna3", "rdna4", "unknown-amd", "none"], amdnr.Runtimes![0].Gpus);
        Assert.Equal(["rdna4"], amdnr.Runtimes![1].Gpus);

        Assert.Equal("runtime-033", amdnr.RuntimePackageFor("rdna3"));
        Assert.Equal("runtime-033", amdnr.RuntimePackageFor("rdna4"));
        Assert.Equal("runtime-033", amdnr.RuntimePackageFor("none"));
        Assert.Equal("runtime-033", amdnr.RuntimePackageFor("RDNA4"));
        // Nothing lists rdna1: that generation is offered no runtime, rather than a wrong one.
        Assert.Null(amdnr.RuntimePackageFor("rdna1"));

        // The owner makes 0.4.0 the RX 9000 default by moving it first. Every other generation
        // still gets 0.3.3, because 0.4.0's list names only rdna4.
        var reordered = ManifestParser.Parse(WithSources.Replace("\"runtimeNote\": \"Needed on RX 7000.\" }",
            "\"runtimeNote\": \"Needed on RX 7000.\", \"runtimes\": [ " +
            "{ \"package\": \"runtime-040\", \"gpus\": [\"rdna4\"] }, " +
            "{ \"package\": \"runtime-033\", \"gpus\": [\"rdna2\", \"rdna3\", \"rdna4\", \"unknown-amd\", \"none\"] } ] }"))
            .TrySource("amdnr")!;

        Assert.Equal(["runtime-040", "runtime-033"], reordered.Runtimes!.Select(r => r.Package));
        Assert.Equal("runtime-040", reordered.RuntimePackageFor("rdna4"));
        Assert.Equal("runtime-033", reordered.RuntimePackageFor("rdna3"));
    }

    [Fact]
    public void A_source_without_runtimes_falls_back_to_the_runtime_package()
    {
        // Every manifest before this one carried a single "runtime" package and no list. A source
        // that predates the list, and the AMDNR that a manifest without sources implies, still
        // offer that package to every GPU, or every such manifest would lose its runtime.
        var amdnr = ManifestParser.Parse(WithSources).TrySource("amdnr")!;

        Assert.Null(amdnr.Runtimes);
        Assert.Equal("runtime", amdnr.RuntimePackageFor("rdna4"));
        Assert.Equal("runtime", amdnr.RuntimePackageFor("none"));
        Assert.Equal("runtime", ManifestParser.Parse(Sample).TrySource("amdnr")!.RuntimePackageFor("rdna3"));

        // A list that is there but empty is a build the publisher gave no runtime, not a build
        // written before there was a list.
        var none = ManifestParser.Parse(WithSources.Replace("\"runtimeNote\": \"Needed on RX 7000.\" }",
            "\"runtimeNote\": \"Needed on RX 7000.\", \"runtimes\": [] }")).TrySource("amdnr")!;

        Assert.Empty(none.Runtimes!);
        Assert.Null(none.RuntimePackageFor("rdna4"));
    }

    [Fact]
    public void A_runtime_choice_without_a_package_or_gpus_is_dropped()
    {
        // An entry with no package would win the pick and then name nothing to download; one with
        // no generations can never win. Either is a hand-edit that should not decide anything.
        var amdnr = ManifestParser.Parse(WithSources.Replace("\"runtimeNote\": \"Needed on RX 7000.\" }",
            "\"runtimeNote\": \"Needed on RX 7000.\", \"runtimes\": [ " +
            "{ \"package\": \"\", \"gpus\": [\"rdna4\"] }, " +
            "{ \"package\": \"no-gpus\" }, " +
            "{ \"package\": \"empty-gpus\", \"gpus\": [] }, " +
            "{ \"package\": \"blank-gpus\", \"gpus\": [\"\", \" \"] }, " +
            "{ \"gpus\": [\"rdna4\"] }, " +
            "{ \"package\": \"runtime-033\", \"gpus\": [\"\", \"rdna4\", \" \"] } ] }"))
            .TrySource("amdnr")!;

        Assert.Equal(["runtime-033"], amdnr.Runtimes!.Select(r => r.Package));
        Assert.Equal(["rdna4"], amdnr.Runtimes![0].Gpus);
        Assert.Equal("runtime-033", amdnr.RuntimePackageFor("rdna4"));
    }

    private static string FindUpwards(string fileName)
    {
        for (var dir = new DirectoryInfo(AppContext.BaseDirectory); dir is not null; dir = dir.Parent)
        {
            var candidate = Path.Combine(dir.FullName, fileName);
            if (File.Exists(candidate)) return candidate;
        }
        throw new FileNotFoundException($"{fileName} was not found above the test output folder.");
    }
}
