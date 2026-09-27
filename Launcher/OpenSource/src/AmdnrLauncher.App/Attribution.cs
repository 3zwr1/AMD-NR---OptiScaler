// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.App;

/// <summary>Who made what, in the words each of them asked for.
///
/// danielblnc let his runtime ship with the launcher (2026-09-25) on one condition: prominent
/// attribution everywhere it is offered — his name, a link to his repository, the words
/// "shipped unmodified, with his permission", and never a hint that it is AMDNR's work. The
/// chooser's question and ABOUT draw their words from here, so no page can say less.
///
/// The launcher's own work is the owner's, under the holder string the owner chose: a handle
/// and the project's name, never a real name or an e-mail address. The same string is in the
/// csproj for the exe's Details tab, and a test holds the two to one value.</summary>
public static class Attribution
{
    public const string LauncherCopyright = "Copyright (c) 2026 3zwr1 (AMDNR)";

    public const string RuntimeAuthor = "Daniel Blanco (danielblnc)";
    public const string RuntimeProject = "DLSS-NR on AMD";
    public static readonly Uri RuntimeProjectUrl = new("https://github.com/danielblnc/DLSS-NR-on-AMD");
    public const string RuntimeCopyright = "Copyright (c) 2026 Daniel Blanco. Shipped unmodified, with permission.";

    /// <summary>The runtime question in three pieces, because the middle one is a link on the
    /// page. WPF keeps one space between inlines written on separate lines, so read together
    /// they are <see cref="RuntimeQuestion"/>.</summary>
    public const string RuntimeQuestionLead = "Download the DLSSNR AMD files? —";
    public const string RuntimeQuestionTail = "by " + RuntimeAuthor + ", shipped unmodified with his permission";
    public const string RuntimeQuestion = RuntimeQuestionLead + " " + RuntimeProject + " " + RuntimeQuestionTail;
}
