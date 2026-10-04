# AMDNR Anywhere capture host — source and changes (GPL-3.0)

The capture host AMDNR Anywhere runs in is **Magpie**, by **Blinue**, in the **experimental fork by SAOG0721**.
Both are **GPL-3.0**. From this release AMDNR builds and distributes that fork itself instead of fetching the
author's binary, so this file is the corresponding-source notice GPL-3.0 section 6 requires.

**Nothing here is AMDNR's own program.** It is somebody else's work, rebuilt with three fixes, and the upstream
projects are where it should be credited:

- Magpie — Blinue — <https://github.com/Blinue/Magpie>
- Experimental fork — SAOG0721 (Jay) — <https://github.com/SAOG0721/Magpie>

## What this binary is, exactly

| | |
|---|---|
| Base | `SAOG0721/Magpie` tag **`v0.6.8-experimental.1`**, commit **`06095c17c16923108645027503de1de2a119d02f`** |
| On top | the three patches in `patches\`, and nothing else |
| Build options | `patches\BuildOptions.props.user` (the fork's own `BuildOptions.props` defaults every `Enable*` to false; this file is what turns the shipped features on) |
| Toolchain | Visual Studio 2022+, x64 Release, the fork's own `.sln` |

Anyone can reproduce it: clone the fork, check out that commit, apply the three patches in order, drop
`BuildOptions.props.user` beside `BuildOptions.props`, and build x64 Release.

## The three changes, and why

All three are fixes to bugs AMDNR players hit. None adds a feature, and none changes what Magpie is for.

**`0001` — an unreadable UI font no longer kills scaling.**
A game whose UI font the overlay could not read took the whole scaling session down with it. Found on Need for
Speed Heat.

**`0002` — match a class rule that holds the full window class.**
The profile's class-name rule was compared against a raw rule string rather than the parsed window class, so the
game's profile never matched, no profile was found, and auto-scaling was never armed. This is the "Anywhere did
nothing at all" report: the window would open and simply never scale.

**`0003` — a stop nobody asked for no longer turns scaling off for good.**
An involuntary stop (a screenshot tool taking the foreground, a capture or NGX fault) set a state that was never
cleared, so scaling stayed off for the rest of the session even after the cause went away. The fix clears that
state instead of avoiding it, which is why it covers every cause and not just screenshots.

## Offered upstream

These three are being offered to the fork's author. If they are merged and released, AMDNR goes back to fetching
the author's own binary and this directory stops being needed — that is the preferred outcome, and the reason the
patches are kept as three separate, upstreamable commits rather than folded into one AMDNR build.

## Licences

The fork ships its own `LICENSE` / `LICENSE-Magpie.txt` and the third-party notices for the SDKs it carries
(AMD FidelityFX, Intel XeSS, NVIDIA). Those files are in the host package beside the binary and are unchanged.
AMDNR's own licence does not cover any of it.
