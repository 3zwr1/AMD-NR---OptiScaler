// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Security;
using System.Text.RegularExpressions;
using Microsoft.Win32;

namespace AmdnrLauncher.Core.Scanning;

/// <summary>The graphics card the launcher found, and the RDNA generation it took it for.
/// Generation is one of the six strings a manifest's runtime list can name — rdna1, rdna2,
/// rdna3, rdna4, unknown-amd for an AMD adapter of no generation the table knows, and none for
/// a machine with no AMD adapter or nothing readable — because that string is what selects a
/// runtime. Name, DriverVersion and DeviceId are what the display class key said, kept for the
/// report; for none, Name is whatever adapter was found, so a support thread can see it is an
/// NVIDIA machine rather than a blank.</summary>
public sealed record GpuInfo(string Name, string Generation, string? DriverVersion, string? DeviceId)
{
    public const string Rdna1 = "rdna1";
    public const string Rdna2 = "rdna2";
    public const string Rdna3 = "rdna3";
    public const string Rdna4 = "rdna4";
    public const string UnknownAmd = "unknown-amd";
    public const string None = "none";

    /// <summary>Nothing readable: no adapter, or the registry could not be read.</summary>
    public static GpuInfo Unknown => new("Unknown", None, null, null);

    /// <summary>False for none only: an AMD adapter of unknown generation is still AMD.</summary>
    public bool IsAmd => Generation != None;

    /// <summary>True when nothing could be read at all — the registry refused, or listed no
    /// adapter — as opposed to a machine whose card was read and is not AMD. The chooser says
    /// "could not be read" for the one and "no AMD graphics card" for the other.</summary>
    public bool IsUnread => Equals(Unknown);

    /// <summary>The generation as a person would say it, for the line the chooser shows.</summary>
    public string GenerationLabel => Generation switch
    {
        Rdna1 => "RDNA 1",
        Rdna2 => "RDNA 2",
        Rdna3 => "RDNA 3",
        Rdna4 => "RDNA 4",
        UnknownAmd => "unknown generation",
        _ => "no AMD GPU",
    };
}

/// <summary>One display adapter as its key under the display class describes it. Any of the
/// three can be absent: a key can belong to a driver that never finished installing.</summary>
public sealed record DisplayAdapter(string? Description, string? MatchingDeviceId, string? DriverVersion);

/// <summary>Where the adapters come from — the registry in the app, a list in a test. In the
/// order Windows set them up, the oldest key first: the probe relies on it to tell a card that
/// was swapped out from the one that replaced it.</summary>
public interface IDisplayAdapterReader
{
    IReadOnlyList<DisplayAdapter> Read();
}

/// <summary>Finds the AMD graphics card and its generation from the registry alone. Never WMI:
/// a Win32_VideoController query can hang for the whole of a driver reset, and this runs before
/// the window is drawn.</summary>
public static class GpuProbe
{
    public static GpuInfo Detect() => Detect(new RegistryDisplayAdapterReader());

    /// <summary>Never throws. The probe informs a note and a report line; the manifest, not the
    /// probe, decides what is installed, so a registry that cannot be read is an unknown GPU
    /// and nothing more.</summary>
    public static GpuInfo Detect(IDisplayAdapterReader reader)
    {
        IReadOnlyList<DisplayAdapter> adapters;
        try
        {
            adapters = reader.Read();
        }
        catch (Exception e) when (e is SecurityException
                                    or IOException
                                    or UnauthorizedAccessException
                                    or ObjectDisposedException)
        {
            return GpuInfo.Unknown;
        }

        // A key with neither a name nor a device id describes nothing that can be reported.
        var described = adapters
            .Where(a => a is not null && (!string.IsNullOrWhiteSpace(a.Description) || !string.IsNullOrWhiteSpace(a.MatchingDeviceId)))
            .ToList();

        var amd = described.Where(IsAmd).ToList();
        if (amd.Count == 0)
        {
            // Not AMD, but not nothing: the report says which card this machine does have.
            return described.FirstOrDefault(a => !string.IsNullOrWhiteSpace(a.Description)) is { } other
                ? new GpuInfo(other.Description!, GpuInfo.None, other.DriverVersion, other.MatchingDeviceId)
                : GpuInfo.Unknown;
        }

        // The class key lists every adapter Windows has ever set up, present or not, an APU
        // often first. When a discrete card is there too, it is the one the game will render on
        // and the one the runtime is chosen for. When two discrete cards are, the later key is
        // the one still in the machine far more often than not: a swapped-out card leaves its
        // key behind at the lower number, and the card that replaced it took the next.
        var chosen = amd.LastOrDefault(a => IsDiscrete(a.Description))
                     ?? amd.LastOrDefault(a => GenerationOf(a.Description, a.MatchingDeviceId) != GpuInfo.UnknownAmd)
                     ?? amd.LastOrDefault(a => !string.IsNullOrWhiteSpace(a.Description))
                     ?? amd[^1];

        return new GpuInfo(
            string.IsNullOrWhiteSpace(chosen.Description) ? "AMD graphics adapter" : chosen.Description,
            GenerationOf(chosen.Description, chosen.MatchingDeviceId),
            chosen.DriverVersion,
            chosen.MatchingDeviceId);
    }

    private static bool IsAmd(DisplayAdapter adapter)
        => Contains(adapter.MatchingDeviceId, "VEN_1002")
           || Contains(adapter.Description, "Radeon")
           || Regex.IsMatch(adapter.Description ?? "", @"\bAMD\b", RegexOptions.IgnoreCase);

    /// <summary>Every discrete Radeon of the last three generations is an "RX"; APUs are
    /// "Radeon 780M Graphics" and the like. Good enough to rank two AMD adapters.</summary>
    private static bool IsDiscrete(string? description)
        => Regex.IsMatch(description ?? "", @"\bRX\b", RegexOptions.IgnoreCase);

    // ---- the generation table -----------------------------------------------------------
    //
    // Best effort, and deliberately in one place. Name first, because the marketing name says
    // the generation for every card that has one; device id second, for the APUs and driver
    // packages whose DriverDesc is only "AMD Radeon(TM) Graphics". Anything AMD the table does
    // not know is unknown-amd, never a guess. The manifest, not this table, decides what is
    // installed: it names which runtime each generation string gets, and the launcher installs
    // exactly that. A wrong row here costs a wrong support note, not a wrong file.

    private static readonly (Regex Name, string Generation)[] ByName =
    [
        (Pattern(@"\bRX\s?90\d\d(?!\d)"), GpuInfo.Rdna4),           // RX 9070 XT, RX 9070, RX 9060 XT
        (Pattern(@"\bRX\s?7\d{3}(?!\d)"), GpuInfo.Rdna3),           // RX 7900 XTX … RX 7600, RX 7900M
        (Pattern(@"\b7\d0M\b"), GpuInfo.Rdna3),                     // Radeon 740M / 760M / 780M / 790M
        (Pattern(@"\b8\d0M\b"), GpuInfo.Rdna3),                     // Radeon 880M / 890M (RDNA 3.5)
        (Pattern(@"\b80[456]0S\b"), GpuInfo.Rdna3),                 // Radeon 8060S / 8050S / 8040S (Strix Halo)
        (Pattern(@"\bZ2 (?:Go|A)\b"), GpuInfo.Rdna2),               // Ryzen Z2 Go (Rembrandt), Z2 A (Van Gogh): RDNA 2, so before the Z row
        (Pattern(@"\bZ[12]\b"), GpuInfo.Rdna3),                     // Ryzen Z1 / Z1 Extreme / Z2 / Z2 Extreme handhelds
        (Pattern(@"\bRX\s?6\d{3}(?!\d)"), GpuInfo.Rdna2),           // RX 6900 XT … RX 6400
        (Pattern(@"\b6\d0M\b"), GpuInfo.Rdna2),                     // Radeon 610M / 660M / 680M
        (Pattern(@"Custom GPU 0(?:405|932)"), GpuInfo.Rdna2),       // Steam Deck LCD (Van Gogh) / OLED (Sephiroth)
        (Pattern(@"\bRX\s?5\d{3}(?!\d)"), GpuInfo.Rdna1),           // RX 5700 XT … RX 5500 XT
    ];

    /// <summary>PCI device ids, upper-case hex, only those verified against a real machine.
    /// DEV_7550 is the RX 9070 XT this launcher was developed on.</summary>
    private static readonly Dictionary<string, string> ByDeviceId = new(StringComparer.OrdinalIgnoreCase)
    {
        ["7550"] = GpuInfo.Rdna4,
    };

    private static readonly Regex DeviceIdPattern = Pattern(@"DEV_([0-9A-F]{4})");

    /// <summary>The generation of an AMD adapter, from its name first and its device id second;
    /// unknown-amd when neither says.</summary>
    public static string GenerationOf(string? description, string? matchingDeviceId)
    {
        foreach (var (name, generation) in ByName)
        {
            if (name.IsMatch(description ?? "")) return generation;
        }

        var device = DeviceIdPattern.Match(matchingDeviceId ?? "");
        return device.Success && ByDeviceId.TryGetValue(device.Groups[1].Value, out var byId)
            ? byId
            : GpuInfo.UnknownAmd;
    }

    private static Regex Pattern(string pattern)
        => new(pattern, RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

    private static bool Contains(string? text, string value)
        => text?.Contains(value, StringComparison.OrdinalIgnoreCase) == true;
}

/// <summary>The display class key, one subkey per adapter Windows has ever set up, read in
/// key order so the oldest comes first. Only the three values the probe needs are read, and
/// only from the four-digit adapter keys: the class key's Properties subkey is ACL-locked and
/// opening it throws.</summary>
internal sealed class RegistryDisplayAdapterReader : IDisplayAdapterReader
{
    private const string DisplayClassKey =
        @"SYSTEM\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}";

    public IReadOnlyList<DisplayAdapter> Read()
    {
        if (!OperatingSystem.IsWindows()) return [];

        using var classKey = Registry.LocalMachine.OpenSubKey(DisplayClassKey);
        if (classKey is null) return [];

        var adapters = new List<DisplayAdapter>();
        foreach (var name in classKey.GetSubKeyNames().Where(IsAdapterKey).Order(StringComparer.Ordinal))
        {
            try
            {
                using var key = classKey.OpenSubKey(name);
                if (key is null) continue;

                adapters.Add(new DisplayAdapter(
                    key.GetValue("DriverDesc") as string,
                    key.GetValue("MatchingDeviceId") as string,
                    key.GetValue("DriverVersion") as string));
            }
            // One adapter key that cannot be opened must not hide the others: the card that is
            // actually in the machine is usually a different key.
            catch (Exception e) when (e is SecurityException or IOException or UnauthorizedAccessException)
            {
            }
        }

        return adapters;
    }

    private static bool IsAdapterKey(string name) => name.Length == 4 && name.All(char.IsAsciiDigit);
}
