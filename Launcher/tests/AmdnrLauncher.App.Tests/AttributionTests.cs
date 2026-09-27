// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Diagnostics;
using System.Reflection;
using AmdnrLauncher.App.ViewModels;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>Who made what, in the words each of them asked for. danielblnc let his runtime
/// ship (2026-09-25) only with prominent attribution everywhere it is offered: his name, a
/// link to his repository, the words "shipped unmodified, with his permission", and never a
/// hint that it is AMDNR's work. The launcher's own work is the owner's, under the holder
/// string the owner chose — a handle and the project's name, never a real name or an
/// e-mail address.</summary>
public class AttributionTests
{
    private const string Holder = "Copyright (c) 2026 3zwr1 (AMDNR)";
    private const string DanielsRepository = "https://github.com/danielblnc/DLSS-NR-on-AMD";

    private static readonly Manifest Manifest = ManifestParser.Parse("""
        {
          "schemaVersion": 1,
          "launcher": { "version": "0.4.0", "url": "https://example.test/AMDNR-Launcher.exe", "sha256": "00" },
          "sources": [
            { "id": "amdnr", "name": "AMDNR", "author": "3zwr1", "summary": "s", "homepage": "https://github.com/3zwr1/AMD-NR---OptiScaler",
              "package": "amdnr", "forwarder": null, "runtimeRequired": false, "runtimeNote": "n",
              "runtimes": [ { "package": "runtime-033", "gpus": ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"] } ] }
          ],
          "packages": [
            { "id": "amdnr", "version": "0.3.3.1", "url": "https://example.test/a.zip", "sha256": "00", "size": 1 },
            { "id": "runtime-033", "version": "0.3.3", "url": "https://example.test/r033.zip", "sha256": "00", "size": 109730870 }
          ],
          "proxyDefaults": ["dxgi.dll"],
          "proxyOverrides": []
        }
        """);

    /// <summary>This PC, as the display class key describes it.</summary>
    private static readonly GpuInfo Rdna4 =
        new("AMD Radeon RX 9070 XT", "rdna4", "32.0.31041.3013", @"PCI\VEN_1002&DEV_7550&REV_C0");

    [Fact]
    public void The_exe_names_the_owner_as_the_copyright_holder_in_its_details_tab()
    {
        var assembly = typeof(App).Assembly;

        Assert.Equal(Holder, assembly.GetCustomAttribute<AssemblyCopyrightAttribute>()?.Copyright);
        Assert.Equal(Brand.Name, assembly.GetCustomAttribute<AssemblyProductAttribute>()?.Product);
        Assert.Equal("3zwr1 (AMDNR)", assembly.GetCustomAttribute<AssemblyCompanyAttribute>()?.Company);

        // The Details tab reads the Win32 version resource, not the managed attributes. The
        // compiler writes the one from the other; this is the check that it did, on the file
        // the launcher is built from.
        var details = FileVersionInfo.GetVersionInfo(assembly.Location);
        Assert.Equal(Holder, details.LegalCopyright);
        Assert.Equal(Brand.Name, details.ProductName);
        Assert.Equal("3zwr1 (AMDNR)", details.CompanyName);
        Assert.Equal(Brand.Name, details.FileDescription);
    }

    [Fact]
    public void The_launcher_is_called_AMD_NR_Launcher_and_its_files_and_ids_stay_AMDNR()
    {
        // The owner's name for it, as two words, everywhere a person reads it: the window, the
        // exe's Details tab, every dialog. Only what is read changes: the exe the manifest URLs
        // point at keeps its file name, and the data folder and the source ids keep AMDNR.
        Assert.Equal("AMD NR Launcher", Brand.Name);
        Assert.Equal("AMDNR-Launcher", typeof(App).Assembly.GetName().Name);
    }

    [Fact]
    public void The_chooser_asks_for_the_runtime_by_its_authors_full_name()
    {
        var chooser = new BuildChooserViewModel(Manifest, null, isFirstRun: true, Rdna4);

        Assert.Equal(
            "Download the DLSSNR AMD files? — DLSS-NR on AMD by Daniel Blanco (danielblnc), shipped unmodified with his permission",
            chooser.RuntimeQuestion);
    }

    [Fact]
    public void The_question_links_his_repository_from_its_middle_piece()
    {
        var chooser = new BuildChooserViewModel(Manifest, null, isFirstRun: true, Rdna4);

        Assert.Equal(DanielsRepository, chooser.RuntimeProjectUrl.AbsoluteUri);
        Assert.Equal("DLSS-NR on AMD", chooser.RuntimeProject);

        // The page draws the question from three pieces so that the middle one can be the
        // link. Read with single spaces between them they are the question — no more, no less.
        Assert.Equal(chooser.RuntimeQuestion,
            $"{chooser.RuntimeQuestionLead} {chooser.RuntimeProject} {chooser.RuntimeQuestionTail}");
    }

    [Fact]
    public void About_credits_Daniel_Blanco_with_his_copyright_and_the_launcher_with_the_owners()
    {
        var settings = new BuildChooserViewModel(Manifest, null, isFirstRun: false, Rdna4);

        Assert.Equal("Daniel Blanco (danielblnc)", settings.RuntimeAuthor);
        Assert.Equal("Copyright (c) 2026 Daniel Blanco. Shipped unmodified, with permission.", settings.RuntimeCopyright);
        Assert.Equal(DanielsRepository, settings.RuntimeProjectUrl.AbsoluteUri);
        Assert.Equal(Holder, settings.LauncherCopyright);

        // One holder string: what ABOUT shows is what the exe's Details tab says.
        Assert.Equal(typeof(App).Assembly.GetCustomAttribute<AssemblyCopyrightAttribute>()?.Copyright,
            settings.LauncherCopyright);
    }
}
