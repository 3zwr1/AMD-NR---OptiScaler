// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Security;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class GpuProbeTests
{
    private sealed class StubReader(params DisplayAdapter[] adapters) : IDisplayAdapterReader
    {
        public IReadOnlyList<DisplayAdapter> Read() => adapters;
    }

    private sealed class ThrowingReader(Exception failure) : IDisplayAdapterReader
    {
        public IReadOnlyList<DisplayAdapter> Read() => throw failure;
    }

    private static DisplayAdapter Amd(string description, string device = "7550", string driver = "32.0.31041.3013")
        => new(description, $@"PCI\VEN_1002&DEV_{device}&REV_C0", driver);

    // The names as the display class key writes them (DriverDesc), including this PC's exact
    // string. The generation comes from the name first, so a device id the table has never
    // seen still gets the right answer for every card whose name says what it is.
    [Theory]
    [InlineData("AMD Radeon RX 9070 XT", "rdna4")]
    [InlineData("AMD Radeon RX 9070", "rdna4")]
    [InlineData("AMD Radeon RX 9060 XT", "rdna4")]
    [InlineData("AMD Radeon RX 7900 XTX", "rdna3")]
    [InlineData("AMD Radeon RX 7800 XT", "rdna3")]
    [InlineData("AMD Radeon RX 7600", "rdna3")]
    [InlineData("AMD Radeon RX 7900M", "rdna3")]
    [InlineData("AMD Radeon 780M Graphics", "rdna3")]
    [InlineData("AMD Radeon 760M Graphics", "rdna3")]
    [InlineData("AMD Radeon 890M Graphics", "rdna3")]
    [InlineData("AMD Radeon 880M Graphics", "rdna3")]
    [InlineData("AMD Radeon 8060S Graphics", "rdna3")]
    [InlineData("AMD Radeon 8050S Graphics", "rdna3")]
    [InlineData("AMD Ryzen Z1 Extreme Graphics", "rdna3")]
    [InlineData("AMD Ryzen Z2 Extreme Graphics", "rdna3")]
    [InlineData("AMD Ryzen AI Z2 Extreme Graphics", "rdna3")]
    [InlineData("AMD Radeon 8040S Graphics", "rdna3")]
    [InlineData("AMD Ryzen Z2 Go Graphics", "rdna2")]
    [InlineData("AMD Ryzen Z2 A Graphics", "rdna2")]
    [InlineData("AMD Custom GPU 0932", "rdna2")]
    [InlineData("AMD Radeon RX 6800 XT", "rdna2")]
    [InlineData("AMD Radeon RX 6600", "rdna2")]
    [InlineData("AMD Radeon RX 6400", "rdna2")]
    [InlineData("AMD Radeon 680M Graphics", "rdna2")]
    [InlineData("AMD Radeon 610M", "rdna2")]
    [InlineData("AMD Custom GPU 0405", "rdna2")]
    [InlineData("AMD Radeon RX 5700 XT", "rdna1")]
    [InlineData("AMD Radeon RX 5500 XT", "rdna1")]
    [InlineData("AMD Radeon RX 580", "unknown-amd")]
    [InlineData("AMD Radeon RX 590", "unknown-amd")]
    [InlineData("Radeon RX Vega 64", "unknown-amd")]
    [InlineData("AMD Radeon VII", "unknown-amd")]
    [InlineData("AMD Radeon(TM) Graphics", "unknown-amd")]
    [InlineData("amd radeon rx 9070 xt", "rdna4")]
    public void The_generation_is_read_from_the_adapter_name(string name, string generation)
    {
        // An unknown device id, so only the name can answer.
        var gpu = GpuProbe.Detect(new StubReader(Amd(name, device: "0000")));

        Assert.Equal(generation, gpu.Generation);
        Assert.Equal(name, gpu.Name);
        Assert.Equal("32.0.31041.3013", gpu.DriverVersion);
        Assert.Equal(@"PCI\VEN_1002&DEV_0000&REV_C0", gpu.DeviceId);
    }

    [Fact]
    public void A_generic_name_falls_back_to_the_device_id()
    {
        // An APU, and some driver packages, report only "AMD Radeon(TM) Graphics". The device
        // id is the one thing left that says which silicon it is — and DEV_7550 is this PC's
        // RX 9070 XT, the one id verified against a real machine.
        var gpu = GpuProbe.Detect(new StubReader(Amd("AMD Radeon(TM) Graphics", device: "7550")));

        Assert.Equal("rdna4", gpu.Generation);
        Assert.Equal("AMD Radeon(TM) Graphics", gpu.Name);
    }

    [Fact]
    public void The_name_wins_over_the_device_id()
    {
        // The table of device ids is the best-effort half; a name that says RX 7900 is not
        // overruled by an id the table happens to know as something else.
        var gpu = GpuProbe.Detect(new StubReader(Amd("AMD Radeon RX 7900 XTX", device: "7550")));

        Assert.Equal("rdna3", gpu.Generation);
    }

    [Fact]
    public void A_lower_case_device_id_is_read_too()
    {
        var gpu = GpuProbe.Detect(new StubReader(
            new DisplayAdapter("AMD Radeon(TM) Graphics", @"pci\ven_1002&dev_7550&subsys_00000000", "32.0.1")));

        Assert.Equal("rdna4", gpu.Generation);
    }

    [Fact]
    public void No_AMD_adapter_is_none_and_still_names_what_was_found()
    {
        // The report is read in a support thread: "none" alone hides that this is an NVIDIA
        // machine, which is the whole explanation of why Neural Rendering did not run.
        var gpu = GpuProbe.Detect(new StubReader(
            new DisplayAdapter("NVIDIA GeForce RTX 4080", @"PCI\VEN_10DE&DEV_2704", "32.0.15.6094")));

        Assert.Equal("none", gpu.Generation);
        Assert.Equal("NVIDIA GeForce RTX 4080", gpu.Name);
        Assert.Equal("32.0.15.6094", gpu.DriverVersion);
        Assert.False(gpu.IsAmd);
        Assert.False(gpu.IsUnread);
    }

    [Fact]
    public void No_adapters_at_all_is_Unknown_and_none()
    {
        Assert.Equal(new GpuInfo("Unknown", "none", null, null), GpuProbe.Detect(new StubReader()));
        Assert.True(GpuProbe.Detect(new StubReader()).IsUnread);
    }

    [Theory]
    [InlineData(typeof(IOException))]
    [InlineData(typeof(UnauthorizedAccessException))]
    [InlineData(typeof(SecurityException))]
    public void A_registry_failure_never_throws(Type failure)
    {
        // The probe runs first thing on startup. A policy-locked hive, a key marked for
        // deletion or a denied ACL must cost a support note at most, never the window.
        var reader = new ThrowingReader((Exception)Activator.CreateInstance(failure)!);

        Assert.Equal(new GpuInfo("Unknown", "none", null, null), GpuProbe.Detect(reader));
        Assert.True(GpuProbe.Detect(reader).IsUnread);
    }

    [Fact]
    public void An_AMD_adapter_is_preferred_over_others_and_a_discrete_one_over_an_APU()
    {
        // A laptop or a desktop with both: the class key lists every adapter, the APU often
        // first. The runtime that runs on the RX 7000 is what the user wants, not the 780M's.
        var gpu = GpuProbe.Detect(new StubReader(
            new DisplayAdapter("Intel(R) UHD Graphics 770", @"PCI\VEN_8086&DEV_4680", "31.0.101.4502"),
            Amd("AMD Radeon 780M Graphics", device: "15BF"),
            Amd("AMD Radeon RX 7900 XTX", device: "744C")));

        Assert.Equal("AMD Radeon RX 7900 XTX", gpu.Name);
        Assert.Equal("rdna3", gpu.Generation);
    }

    [Fact]
    public void Of_two_discrete_AMD_keys_the_one_set_up_later_is_the_card_in_the_machine()
    {
        // The class key keeps a subkey for every adapter Windows has ever set up, not only the
        // ones present. A card that was swapped out leaves its key at the lower number, and the
        // one that replaced it gets the next. Read oldest first, the later key is the answer —
        // whichever way the swap went: with 0.4.0 made the RDNA 4 default, a stale RX 9070 key
        // on a machine now holding an RX 7800 XT would otherwise pull the RX 9000-only kernels.
        var upgraded = GpuProbe.Detect(new StubReader(
            Amd("AMD Radeon RX 6800 XT", device: "73BF"),
            Amd("AMD Radeon RX 9070 XT", device: "7550")));

        Assert.Equal("AMD Radeon RX 9070 XT", upgraded.Name);
        Assert.Equal("rdna4", upgraded.Generation);

        var downgraded = GpuProbe.Detect(new StubReader(
            Amd("AMD Radeon RX 9070 XT", device: "7550"),
            Amd("AMD Radeon RX 7800 XT", device: "747E")));

        Assert.Equal("AMD Radeon RX 7800 XT", downgraded.Name);
        Assert.Equal("rdna3", downgraded.Generation);
        Assert.Equal(@"PCI\VEN_1002&DEV_747E&REV_C0", downgraded.DeviceId);
    }

    [Fact]
    public void An_adapter_key_without_a_name_is_skipped_for_one_with()
    {
        // A key left behind by a driver that never finished installing has an id and nothing
        // else; the card that is actually described is the answer.
        var gpu = GpuProbe.Detect(new StubReader(
            new DisplayAdapter(null, @"PCI\VEN_1002&DEV_0000", null),
            Amd("AMD Radeon RX 6800 XT", device: "73BF")));

        Assert.Equal("AMD Radeon RX 6800 XT", gpu.Name);
        Assert.Equal("rdna2", gpu.Generation);
    }

    [Fact]
    public void An_AMD_adapter_known_only_by_its_id_is_still_reported()
    {
        var gpu = GpuProbe.Detect(new StubReader(new DisplayAdapter(null, @"PCI\VEN_1002&DEV_7550&REV_C0", "32.0.1")));

        Assert.Equal("rdna4", gpu.Generation);
        Assert.True(gpu.IsAmd);
        Assert.False(string.IsNullOrWhiteSpace(gpu.Name));
    }

    [Theory]
    [InlineData("rdna1", "RDNA 1")]
    [InlineData("rdna2", "RDNA 2")]
    [InlineData("rdna3", "RDNA 3")]
    [InlineData("rdna4", "RDNA 4")]
    [InlineData("unknown-amd", "unknown generation")]
    [InlineData("none", "no AMD GPU")]
    public void The_generation_has_a_label_a_person_would_say(string generation, string label)
    {
        Assert.Equal(label, new GpuInfo("x", generation, null, null).GenerationLabel);
    }

    [Fact]
    public void The_real_registry_is_read_without_throwing_and_answers_in_the_manifest_vocabulary()
    {
        // Read-only, and whatever machine runs the tests: the answer must be one of the six
        // strings a manifest's runtime list can name, because that is what selects a runtime.
        var gpu = GpuProbe.Detect();

        Assert.Contains(gpu.Generation, new[] { "rdna1", "rdna2", "rdna3", "rdna4", "unknown-amd", "none" });
        Assert.False(string.IsNullOrWhiteSpace(gpu.Name));
    }
}
