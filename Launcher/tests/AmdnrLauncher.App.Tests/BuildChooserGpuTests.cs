// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.App.ViewModels;
using AmdnrLauncher.Core;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.App.Tests;

/// <summary>What the chooser says about the graphics card it found. Every note informs and
/// none blocks: the manifest decides what is installed, and a wrong guess about a card must
/// never stop somebody whose card is fine.</summary>
public class BuildChooserGpuTests
{
    private static readonly Manifest Manifest = ManifestParser.Parse("""
        {
          "schemaVersion": 1,
          "launcher": { "version": "0.4.0", "url": "https://example.test/AMDNR-Launcher.exe", "sha256": "00" },
          "sources": [
            { "id": "amdnr", "name": "AMDNR", "author": "3zwr1", "summary": "s", "homepage": "https://github.com/3zwr1/AMD-NR---OptiScaler",
              "package": "amdnr", "forwarder": null, "runtimeRequired": false, "runtimeNote": "n",
              "runtimes": [
                { "package": "runtime-033", "gpus": ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"] },
                { "package": "runtime-040", "gpus": ["rdna4"] } ] },
            { "id": "theautomatic", "name": "OptiScaler AMD pre-SR", "author": "TheAutomatic", "summary": "s", "homepage": "https://github.com/TheAutomatic/dlss-5-amd-project",
              "package": "theautomatic", "forwarder": null, "runtimeRequired": true, "runtimeNote": "Required.",
              "runtimes": [
                { "package": "runtime-033", "gpus": ["rdna2", "rdna3", "rdna4", "unknown-amd", "none"] },
                { "package": "runtime-040", "gpus": ["rdna4"] } ] }
          ],
          "packages": [
            { "id": "amdnr", "version": "0.3.3.1", "url": "https://example.test/a.zip", "sha256": "00", "size": 1 },
            { "id": "theautomatic", "version": "1.9.1-alpha", "url": "https://example.test/t.zip", "sha256": "00", "size": 1 },
            { "id": "runtime-033", "version": "0.3.3", "url": "https://example.test/r033.zip", "sha256": "00", "size": 109730870 },
            { "id": "runtime-040", "version": "0.4.0", "url": "https://example.test/r040.zip", "sha256": "00", "size": 200000000 }
          ],
          "proxyDefaults": ["dxgi.dll"],
          "proxyOverrides": []
        }
        """);

    /// <summary>This PC, as the display class key describes it.</summary>
    private static readonly GpuInfo Rdna4 =
        new("AMD Radeon RX 9070 XT", "rdna4", "32.0.31041.3013", @"PCI\VEN_1002&DEV_7550&REV_C0");

    private static BuildChooserViewModel Chooser(GpuInfo gpu)
        => new(Manifest, null, isFirstRun: true, gpu);

    [Fact]
    public void The_detected_card_is_named_with_its_generation_and_a_supported_one_gets_no_note()
    {
        var chooser = Chooser(Rdna4);

        Assert.Equal("Detected: AMD Radeon RX 9070 XT (RDNA 4)", chooser.DetectedGpu);
        Assert.False(chooser.HasGpuNote);
        Assert.Equal("", chooser.GpuNote);

        var rdna3 = Chooser(new GpuInfo("AMD Radeon RX 7900 XTX", "rdna3", "32.0.21001.1", null));
        Assert.Equal("Detected: AMD Radeon RX 7900 XTX (RDNA 3)", rdna3.DetectedGpu);
        Assert.False(rdna3.HasGpuNote);
    }

    [Theory]
    [InlineData("AMD Radeon RX 6800 XT", "rdna2",
        "Detected: AMD Radeon RX 6800 XT (RDNA 2)",
        "RX 6000 is not supported yet — Neural Rendering will not run. AMDNR is working on it.")]
    [InlineData("AMD Radeon RX 5700 XT", "rdna1",
        "Detected: AMD Radeon RX 5700 XT (RDNA 1)",
        "RX 5000 is not supported — Neural Rendering will not run on it.")]
    [InlineData("AMD Radeon(TM) Graphics", "unknown-amd",
        "Detected: AMD Radeon(TM) Graphics (unknown generation)",
        "Could not tell which generation this AMD card is; installing anyway.")]
    [InlineData("NVIDIA GeForce RTX 4080", "none",
        "Detected: no AMD graphics card",
        "No AMD graphics card was found. AMDNR is for AMD GPUs; Neural Rendering will not run here.")]
    public void A_card_that_will_not_run_Neural_Rendering_gets_an_honest_note_that_does_not_block(
        string name, string generation, string detected, string note)
    {
        var chooser = Chooser(new GpuInfo(name, generation, "32.0.1", null));

        Assert.Equal(detected, chooser.DetectedGpu);
        Assert.True(chooser.HasGpuNote);
        Assert.Equal(note, chooser.GpuNote);

        // Informs, never blocks: CONTINUE is still there, and answers as it always did.
        Assert.True(chooser.ContinueCommand.CanExecute(null));
        chooser.Continue();
        Assert.True(chooser.Answer.IsCompletedSuccessfully);
    }

    [Fact]
    public void A_card_that_could_not_be_read_is_said_to_be_unread_not_absent()
    {
        // The registry could not be read, or listed no adapter at all. "No AMD graphics card
        // was found" is a claim the launcher cannot make then; what it knows is that it could
        // not look. Still a note, still CONTINUE.
        var chooser = Chooser(GpuInfo.Unknown);

        Assert.Equal("Detected: the graphics card could not be read", chooser.DetectedGpu);
        Assert.True(chooser.HasGpuNote);
        Assert.Equal("The graphics card could not be read, so the launcher cannot say whether Neural Rendering " +
                     "will run here. AMDNR is for AMD GPUs.", chooser.GpuNote);
        Assert.True(chooser.ContinueCommand.CanExecute(null));
    }

    [Fact]
    public async Task When_the_build_lists_no_runtime_for_the_card_the_question_gives_way_to_a_note()
    {
        // Nothing in the manifest lists rdna1. Yes to a download that Yes would not make is a
        // question with no honest answer, so the page says what is the case instead. The
        // preference itself passes through untouched: the user declined nothing, and a
        // manifest that one day lists the card should get the default they never changed.
        var chooser = Chooser(new GpuInfo("AMD Radeon RX 5700 XT", "rdna1", null, null));

        Assert.False(chooser.OffersRuntime);
        Assert.Equal("No DLSSNR AMD files are offered for this card with this build.", chooser.NoRuntimeNote);

        // Nothing offered is nothing declined, even on the build that requires the runtime.
        chooser.SelectedOption = chooser.Options.Single(o => o.Source.Id == "theautomatic");
        chooser.IncludeRuntime = false;
        Assert.False(chooser.OffersRuntime);
        Assert.False(chooser.ShowRuntimeWarning);

        chooser.IncludeRuntime = true;
        chooser.Continue();
        Assert.Equal(new InstallChoice("theautomatic", IncludeRuntime: true), await chooser.Answer);

        // A card the list names gets the question, and the warning when it declines.
        var offered = Chooser(Rdna4);
        Assert.True(offered.OffersRuntime);
        offered.SelectedOption = offered.Options.Single(o => o.Source.Id == "theautomatic");
        offered.IncludeRuntime = false;
        Assert.True(offered.ShowRuntimeWarning);
    }

    [Fact]
    public void The_selected_build_decides_whether_a_runtime_is_offered_and_says_so_when_it_changes()
    {
        var oneBuildOffersNone = Manifest with
        {
            Sources = Manifest.OfferedSources
                .Select(s => s.Id == "theautomatic" ? s with { Runtimes = [] } : s)
                .ToList(),
        };
        var chooser = new BuildChooserViewModel(oneBuildOffersNone, null, isFirstRun: true, Rdna4);
        var raised = new List<string?>();
        chooser.PropertyChanged += (_, e) => raised.Add(e.PropertyName);

        Assert.True(chooser.OffersRuntime);
        chooser.SelectedOption = chooser.Options.Single(o => o.Source.Id == "theautomatic");

        Assert.False(chooser.OffersRuntime);
        Assert.Contains(nameof(BuildChooserViewModel.OffersRuntime), raised);

        // The size line follows the build too: the other build's runtime is another download.
        Assert.Contains(nameof(BuildChooserViewModel.RuntimeSize), raised);
        Assert.Contains(nameof(BuildChooserViewModel.HasRuntimeSize), raised);
        Assert.False(chooser.HasRuntimeSize);
    }

    [Fact]
    public async Task The_runtimes_listed_for_the_card_are_offered_in_the_manifests_order_then_No()
    {
        // RDNA 4 is on both lists: 0.3.3 first — the card's default, marked so — then 0.4.0, then No.
        var chooser = Chooser(Rdna4);
        Assert.Equal(["0.3.3 · RECOMMENDED", "0.4.0", "NO"], chooser.RuntimeOptions.Select(o => o.Label));
        Assert.Equal(["runtime-033", "runtime-040", null], chooser.RuntimeOptions.Select(o => o.Id));
        Assert.Equal(["0.3.3 · RECOMMENDED"], chooser.RuntimeOptions.Where(o => o.IsSelected).Select(o => o.Label));
        Assert.True(chooser.IncludeRuntime);

        // Picking 0.4.0: it is what downloads, its size is quoted, and it is remembered by id.
        var raised = new List<string?>();
        chooser.PropertyChanged += (_, e) => raised.Add(e.PropertyName);
        chooser.PickRuntime("runtime-040");
        Assert.True(chooser.IncludeRuntime);
        Assert.Equal(["0.4.0"], chooser.RuntimeOptions.Where(o => o.IsSelected).Select(o => o.Label));
        Assert.Equal("About 200 MB.", chooser.RuntimeSize);
        Assert.Contains(nameof(BuildChooserViewModel.RuntimeOptions), raised);
        Assert.Contains(nameof(BuildChooserViewModel.RuntimeSize), raised);
        chooser.Continue();
        Assert.Equal(new InstallChoice("amdnr", IncludeRuntime: true, RuntimeId: "runtime-040"), await chooser.Answer);

        // RDNA 3 is on one list only: its runtime and No.
        var rdna3 = Chooser(new GpuInfo("AMD Radeon RX 7800 XT", "rdna3", null, null));
        Assert.Equal(["0.3.3 · RECOMMENDED", "NO"], rdna3.RuntimeOptions.Select(o => o.Label));

        // A card no runtime is listed for is offered only the note; there is nothing to pick.
        Assert.Empty(Chooser(new GpuInfo("AMD Radeon RX 5700 XT", "rdna1", null, null)).RuntimeOptions.Where(o => o.Id is not null));
    }

    [Fact]
    public async Task The_recommended_runtime_is_stored_as_no_pick_so_the_user_follows_the_manifest()
    {
        // Back on the card's default: saved as no pick, so when the owner moves the card to a newer
        // runtime this user moves with it. An explicit pick stays until they change it.
        var chooser = Chooser(Rdna4);
        chooser.PickRuntime("runtime-040");
        chooser.PickRuntime("runtime-033");
        chooser.Continue();
        Assert.Equal(new InstallChoice("amdnr", IncludeRuntime: true, RuntimeId: null), await chooser.Answer);

        // No is No, whatever was picked before it.
        var declined = Chooser(Rdna4);
        declined.PickRuntime("runtime-040");
        declined.PickRuntime(null);
        Assert.False(declined.IncludeRuntime);
        Assert.Equal(["NO"], declined.RuntimeOptions.Where(o => o.IsSelected).Select(o => o.Label));
        declined.Continue();
        Assert.Equal(new InstallChoice("amdnr", IncludeRuntime: false), await declined.Answer);

        // The old Yes / No property still drives the same state: No selects NO, Yes the default.
        var yesNo = Chooser(Rdna4);
        yesNo.IncludeRuntime = false;
        Assert.Equal(["NO"], yesNo.RuntimeOptions.Where(o => o.IsSelected).Select(o => o.Label));
        yesNo.IncludeRuntime = true;
        Assert.Equal(["0.3.3 · RECOMMENDED"], yesNo.RuntimeOptions.Where(o => o.IsSelected).Select(o => o.Label));

        // A saved pick comes back selected; one the build does not list for this card falls back.
        var saved = new BuildChooserViewModel(Manifest, new InstallChoice("amdnr", true, "runtime-040"), isFirstRun: false, Rdna4);
        Assert.Equal(["0.4.0"], saved.RuntimeOptions.Where(o => o.IsSelected).Select(o => o.Label));
        var moved = new BuildChooserViewModel(Manifest, new InstallChoice("amdnr", true, "runtime-040"), isFirstRun: false,
            new GpuInfo("AMD Radeon RX 7800 XT", "rdna3", null, null));
        Assert.Equal(["0.3.3 · RECOMMENDED"], moved.RuntimeOptions.Where(o => o.IsSelected).Select(o => o.Label));
        moved.Continue();
        Assert.Equal(new InstallChoice("amdnr", IncludeRuntime: true, RuntimeId: null), await moved.Answer);
    }

    [Fact]
    public void The_runtime_size_is_that_of_the_runtime_this_card_would_get()
    {
        // 0.3.3 first in the list, so RDNA 4 and RDNA 3 both get it and its ~110 MB. The size
        // is its own line under the question, which names the author and nothing else.
        Assert.Equal("About 110 MB.", Chooser(Rdna4).RuntimeSize);
        Assert.True(Chooser(Rdna4).HasRuntimeSize);
        Assert.Equal("About 110 MB.", Chooser(new GpuInfo("AMD Radeon RX 7800 XT", "rdna3", null, null)).RuntimeSize);

        // No manifest to read a size from: no line, rather than a size made up. The question
        // is still asked, because the one runtime package every older manifest had is offered.
        var unsized = new BuildChooserViewModel(null, null, isFirstRun: true, Rdna4);
        Assert.True(unsized.OffersRuntime);
        Assert.Equal("", unsized.RuntimeSize);
        Assert.False(unsized.HasRuntimeSize);
    }

    [Fact]
    public void The_runtime_size_follows_the_manifest_order_for_the_card()
    {
        // The owner makes 0.4.0 the RDNA 4 default by reordering the manifest; the chooser
        // then quotes 0.4.0's size to an RDNA 4 user and still 0.3.3's to everyone else.
        var reordered = Manifest with
        {
            Sources = Manifest.OfferedSources
                .Select(s => s with { Runtimes = s.Runtimes!.Reverse().ToList() })
                .ToList(),
        };

        Assert.Equal("About 200 MB.",
            new BuildChooserViewModel(reordered, null, isFirstRun: true, Rdna4).RuntimeSize);
        Assert.Equal("About 110 MB.",
            new BuildChooserViewModel(reordered, null, isFirstRun: true,
                new GpuInfo("AMD Radeon RX 7800 XT", "rdna3", null, null)).RuntimeSize);
    }
}
