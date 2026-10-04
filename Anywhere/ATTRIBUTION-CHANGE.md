# The Magpie line has to change — the old one stops being true

Today AMDNR says, in the release notes, the credits and the README:

> **AMDNR Anywhere** runs inside **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the
> author, not redistributed by AMDNR** - <https://github.com/SAOG0721/Magpie>.

From the release that ships our patched `Magpie.exe`, the second half is false. We still fetch the host package
from the author and still do not re-host his zip — but we do distribute one modified binary out of it, and GPL-3.0
asks that a modified version say so, carry its date, and come with its corresponding source.

## Replacement — use this everywhere the old line appears

> **AMDNR Anywhere** runs inside **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** -
> <https://github.com/SAOG0721/Magpie>. The host package is fetched from the fork's own release and is not
> re-hosted by AMDNR. **One file in it is modified: `Magpie.exe` is that fork's `v0.6.8-experimental.1` rebuilt by
> AMDNR with three fixes** (an unreadable UI font no longer ends scaling; a class-name rule that holds the full
> window class matches again, so a profile is found and auto-scaling arms; a stop nobody asked for no longer turns
> scaling off for the rest of the session). The three patches, the base commit and the build options are in
> `Anywhere-host-source\` beside the host, and have been offered upstream - if they are merged and released,
> AMDNR goes back to the author's own binary.

Short form, where there is no room for all of it (in-app, a tight credits panel):

> **Magpie** by **Blinue**, experimental fork by **SAOG0721** (GPL-3.0). `Magpie.exe` is that fork rebuilt by
> AMDNR with three scaling fixes; the patches ship beside it in `Anywhere-host-source\`.

## Where it appears, to be changed together

- The GitHub release body (the attribution blockquote) — `GITHUB-RELEASE-BODY.md` carries the old wording now.
- `Licenses\AMDNR_NOTICE.txt` in the zip.
- The README credits.
- The in-app credits / the Anywhere page, wherever the fork is named.
- `make-manifest.ps1`'s comment at line ~171 and ~630 ("from its author's release, never re-hosted") — the
  comment describes `anywhere-host`, which is still true of the zip, so it needs one clause rather than a rewrite.

## What does NOT change

- `anywhere-host` still points at `SAOG0721/Magpie`'s own release asset, at the pinned sha256. We are not
  re-hosting 490 MB and not forking his distribution.
- `$AnywhereRepo` and `$AnywhereSha256` keep their defaults.
- The patched `Magpie.exe` rides in the AMDNR package the launcher already stages over the host folder, which is
  a package we have always hosted and re-hashed ourselves.
