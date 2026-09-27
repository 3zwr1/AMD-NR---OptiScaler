// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using AmdnrLauncher.Core.Diagnostics;

namespace AmdnrLauncher.Core.Tests;

public class IniHealthTests
{
    [Fact]
    public void Flags_AmdGraphicsWaitExperimental_set_to_true()
    {
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini", "[DlssNr]\nAmdGraphicsWaitExperimental=true\n");

        var finding = Assert.Single(IniHealth.Check(ini));
        Assert.Equal("ini.AmdGraphicsWaitExperimental", finding.Id);
        Assert.Equal(DoctorSeverity.Warning, finding.Severity);
    }

    [Fact]
    public void Flags_AmdInterleaveModelHistory_and_LogToFile()
    {
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini",
            "[DlssNr]\nAmdInterleaveModelHistory=true\n[Log]\nLogToFile=false\n");

        var ids = IniHealth.Check(ini).Select(f => f.Id).Order().ToArray();

        Assert.Equal(["ini.AmdInterleaveModelHistory", "ini.LogToFile"], ids);
    }

    [Fact]
    public void The_shipped_defaults_produce_no_findings()
    {
        using var dir = new TempDir();
        // These are the values the packaged ini actually ships with.
        var ini = dir.Write("OptiScaler.ini",
            "AmdGraphicsWaitExperimental=auto\nAmdInterleaveModelHistory=auto\nLogToFile=true\n");

        Assert.Empty(IniHealth.Check(ini));
    }

    [Fact]
    public void Comparison_ignores_case_and_surrounding_whitespace()
    {
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini", "   LogToFile = FALSE   \n");

        Assert.Single(IniHealth.Check(ini), f => f.Id == "ini.LogToFile");
    }

    [Fact]
    public void A_commented_out_line_is_not_a_finding()
    {
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini", "; LogToFile=false\n# AmdGraphicsWaitExperimental=true\n");

        Assert.Empty(IniHealth.Check(ini));
    }

    [Fact]
    public void A_missing_ini_produces_no_findings()
    {
        Assert.Empty(IniHealth.Check(@"Z:\nope\OptiScaler.ini"));
    }

    [Fact]
    public void An_ini_that_cannot_be_read_is_reported_rather_than_thrown()
    {
        // Runs on every scan of an installed game. An ini the game (or an AV scanner) holds
        // open must produce a finding, not an exception that kills the whole rescan.
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini", "LogToFile=false\n");
        using var hold = new FileStream(ini, FileMode.Open, FileAccess.Read, FileShare.None);

        var finding = Assert.Single(IniHealth.Check(ini));

        Assert.Equal("ini.unreadable", finding.Id);
        Assert.Equal(DoctorSeverity.Warning, finding.Severity);
    }

    [Fact]
    public void Reset_backs_up_the_old_ini_and_writes_the_packaged_one()
    {
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini", "LogToFile=false\n");
        var packaged = dir.Write("packaged.ini", "; packaged defaults\n");

        var backup = IniHealth.ResetToDefaults(ini, packaged);

        Assert.NotNull(backup);
        Assert.Equal("; packaged defaults\n", File.ReadAllText(ini));
        Assert.Equal("LogToFile=false\n", File.ReadAllText(backup!));
        Assert.StartsWith("OptiScaler.ini.bak-", Path.GetFileName(backup!));
    }

    [Fact]
    public void Reset_returns_null_when_there_was_no_ini_to_back_up()
    {
        using var dir = new TempDir();
        var ini = Path.Combine(dir.Path, "OptiScaler.ini");
        var packaged = dir.Write("packaged.ini", "; packaged defaults\n");

        // Nothing was backed up, so there is no backup path to report.
        Assert.Null(IniHealth.ResetToDefaults(ini, packaged));
        Assert.Equal("; packaged defaults\n", File.ReadAllText(ini));
    }

    [Fact]
    public void Two_resets_in_the_same_second_keep_both_backups()
    {
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini", "first\n");
        var packaged = dir.Write("packaged.ini", "; packaged defaults\n");

        var firstBackup = IniHealth.ResetToDefaults(ini, packaged);
        File.WriteAllText(ini, "second\n");
        var secondBackup = IniHealth.ResetToDefaults(ini, packaged);

        Assert.NotEqual(firstBackup, secondBackup);
        Assert.Equal("first\n", File.ReadAllText(firstBackup!));
        Assert.Equal("second\n", File.ReadAllText(secondBackup!));
    }

    [Theory]
    [InlineData("[DlssNr]\nNrBackend=lmxxf\n", null, true)]
    [InlineData("[dlssnr]\r\n nrbackend = LMXXF \r\n", null, true)]
    [InlineData("[DlssNr]\nNrBackend=daniel\n", null, false)]
    [InlineData("[DlssNr]\n;NrBackend=lmxxf\n", null, false)]
    [InlineData("[Other]\nNrBackend=lmxxf\n", null, false)]
    [InlineData("NrBackend=lmxxf\n", null, false)]
    // lmxxf's models, in each place a build that selects it looks for them: TheAutomatic's
    // layout, lmxxf's own release, and the pak AMDNR 0.3.2 ships beside LmxxfNrRuntime.dll.
    [InlineData("[DlssNr]\nNrBackend=lmxxf\n", "native-game-tiled-assets/", false)]
    [InlineData("[DlssNr]\nNrBackend=lmxxf\n", "DLSS5-AMD/native-game-tiled-assets/", false)]
    [InlineData("[DlssNr]\nNrBackend=lmxxf\n", "LmxxfNrRuntime.pak", false)]
    public void Lmxxf_without_its_models_is_flagged(string content, string? models, bool flagged)
    {
        using var dir = new TempDir();
        var ini = dir.Write("OptiScaler.ini", content);
        if (models is not null && models.EndsWith('/'))
            Directory.CreateDirectory(Path.Combine(dir.Path, models));
        else if (models is not null)
            dir.Write(models, "pak");

        var findings = IniHealth.CheckLmxxfModels(ini);

        Assert.Equal(flagged, findings.Any(f => f.Id == "ini.lmxxfWithoutModels"));
        Assert.All(findings, f => Assert.Equal(DoctorSeverity.Warning, f.Severity));
    }

    [Fact]
    public void Lmxxf_check_has_nothing_to_say_about_a_missing_ini()
    {
        using var dir = new TempDir();

        Assert.Empty(IniHealth.CheckLmxxfModels(Path.Combine(dir.Path, "OptiScaler.ini")));
    }
}
