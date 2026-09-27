// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text;
using AmdnrLauncher.Core.Assets;
using AmdnrLauncher.Core.Install;

namespace AmdnrLauncher.Core.Tests;

public class IniSeedTests
{
    private static readonly IniSetting[] Daniel = [new("DlssNr", "NrBackend", "daniel")];

    private static string Apply(string ini, params IniSetting[] settings)
        => Encoding.UTF8.GetString(IniSeed.Apply(Encoding.UTF8.GetBytes(ini), settings));

    [Theory]
    // Replaced in place, the key's own spelling and spacing kept, other sections untouched.
    [InlineData("[DlssNr]\r\nNrBackend=lmxxf\r\n[B]\r\nNrBackend=x\r\n",
                "[DlssNr]\r\nNrBackend=daniel\r\n[B]\r\nNrBackend=x\r\n")]
    [InlineData("[ dlssnr ]\nnrbackend = lmxxf\n", "[ dlssnr ]\nnrbackend = daniel\n")]
    // Added after the section's last setting, not after the blank line that closes it.
    [InlineData("[DlssNr]\r\nEnabled=false\r\n\r\n[B]\r\n", "[DlssNr]\r\nEnabled=false\r\nNrBackend=daniel\r\n\r\n[B]\r\n")]
    [InlineData("[DlssNr]", "[DlssNr]\r\nNrBackend=daniel\r\n")]
    [InlineData("[DlssNr]\nEnabled=false", "[DlssNr]\nEnabled=false\nNrBackend=daniel\n")]
    // A section the file does not have is appended, in the file's own line ending.
    [InlineData("[A]\nX=1\n", "[A]\nX=1\n\n[DlssNr]\nNrBackend=daniel\n")]
    [InlineData("[A]\r\nX=1", "[A]\r\nX=1\r\n\r\n[DlssNr]\r\nNrBackend=daniel\r\n")]
    [InlineData("", "[DlssNr]\r\nNrBackend=daniel\r\n")]
    // Commented-out keys are documentation, not settings.
    [InlineData("[DlssNr]\n;NrBackend=daniel\n", "[DlssNr]\n;NrBackend=daniel\nNrBackend=daniel\n")]
    public void Settings_are_applied_and_nothing_else_changes(string before, string after)
    {
        Assert.Equal(after, Apply(before, Daniel));
    }

    [Fact]
    public void A_byte_order_mark_and_non_ASCII_text_survive()
    {
        var before = new byte[] { 0xEF, 0xBB, 0xBF }
            .Concat(Encoding.UTF8.GetBytes("; Ação 游戏\r\n[DlssNr]\r\nNrBackend=lmxxf\r\n")).ToArray();

        var after = IniSeed.Apply(before, Daniel);

        Assert.Equal(
            new byte[] { 0xEF, 0xBB, 0xBF }.Concat(Encoding.UTF8.GetBytes("; Ação 游戏\r\n[DlssNr]\r\nNrBackend=daniel\r\n")),
            after);
    }

    [Fact]
    public void Several_settings_apply_in_order()
    {
        var after = Apply("[DlssNr]\nEnabled=false\n",
            new IniSetting("DlssNr", "Enabled", "true"), new IniSetting("DlssNr", "NrBackend", "daniel"));

        Assert.Equal("[DlssNr]\nEnabled=true\nNrBackend=daniel\n", after);
    }
}
