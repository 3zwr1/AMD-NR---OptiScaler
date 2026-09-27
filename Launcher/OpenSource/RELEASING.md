# Releasing AMD NR Launcher

Every launcher, whatever version it is, reads one file at start-up:

    https://raw.githubusercontent.com/3zwr1/AMD-NR---OptiScaler/main/Launcher/manifest.json

**Where the launcher lives in the mod repository (owner decision, 27 September 2026):** the folder is `Launcher/`
with a capital L — raw.githubusercontent.com paths are case-sensitive and this address is compiled into every
launcher, so the folder must be spelled exactly so. `Launcher/manifest.json` is the manifest; `Launcher/OpenSource/` is
the published source snapshot (the contents of `launcher-public\`, with its own `LICENSE.txt` and `README.md`), which the
repository's root `AMDNR_NOTICE.txt` says is not covered by the root GPL-3.0 licence. `Desktop\dlss-amd\launcher-repo-upload\`
holds both in that layout, ready to upload; refresh its `manifest.json` from `launcher\manifest.json` after step 3.

That address is compiled in. Whatever `Launcher/manifest.json` on `main` of
`3zwr1/AMD-NR---OptiScaler` says is what every user downloads, so that file is the release. The steps
below exist to make sure every URL, size and hash in it is read from GitHub and never typed by hand.

## 0.3.4 — first public release

The launcher carries the mod's version number and ships in the mod's own release (owner decision,
27 September 2026): `AMDNR-Launcher.exe` is an asset of `Alpha0.3.4`, there is no separate launcher tag, and
`-LauncherTag` is simply `Alpha0.3.4`. The forwarder is hosted on the owner's own server rather than on GitHub:
`-ForwarderUrl https://files.catbox.moe/dvn8sw.dll` (the owner's upload of the 23 September build, 27 September 2026) with
`-ForwarderPath publish
vngx.dll_dlssnr.dll` for its hash and size. The name the file has on that server does not matter:
the launcher stores it under that name and places it in the game as `nvngx.dll_dlssnr.dll`. If the host ever goes away,
upload the same file anywhere reachable by https, change `-ForwarderUrl`, run the script and publish the manifest — no new
launcher. A later launcher without a mod release gets a fourth number (0.3.4.1) and its own tag.

The `manifest.json` committed in this repo today was made with `-SkipLauncher`: every package value is
real and checked against GitHub, but the launcher `sha256` is `PENDING_UPLOAD` and the two files below
are not on GitHub yet. **Do not publish it as it is.** Step 3 fills it in.

### 1. Test and build

From `C:\Users\Administrator\Desktop\dlss-amd\launcher`, delete the `publish` folder first. An older
build (`AmdnrLauncher.exe`, 1.0.0) may still be in it, and it must never be the file you upload. Then:

    dotnet test AmdnrLauncher.sln --nologo
    publish.cmd

The result is one self-contained file, `publish\AMDNR-Launcher.exe`. Check it is 0.4.0 (Windows
writes the file version with four parts, so this prints `0.4.0.0`):

    (Get-Item publish\AMDNR-Launcher.exe).VersionInfo.FileVersion

### 2. Create the GitHub release

On github.com/3zwr1/AMD-NR---OptiScaler → **Releases** → **Draft a new release**:

- Tag: **`Alpha0.3.4`** — the mod's own release. The launcher is one more asset of it; the forwarder is not uploaded here.
- **Upload exactly these two files, with exactly these names.** The manifest's URLs already point at
  them, so a different name is a broken download for every user:
  1. **`AMDNR-Launcher.exe`** — from `launcher\publish\`.
  2. **`nvngx.dll_dlssnr.dll`** — the 23 September build, kept as `launcher\publish\nvngx.dll_dlssnr.dll`
     (114,688 bytes, sha256 `5976915a7f59e57c6fd5ffe4742468dd9db19503c894ef21810a0ad0f941f419`; check with
     `Get-FileHash publish\nvngx.dll_dlssnr.dll`). **Not** `OptiScaler-DLSSNR-PreSR-Multipass-main\x64\Release\a\`
     any more: that folder was rebuilt on 26 September 17:53 and now holds a different file (`99810824…`) —
     if that newer build is the one 0.3.4 should ship, copy it over `publish\nvngx.dll_dlssnr.dll`, raise
     `-ForwarderVersion` (0.2.2), and the script takes it from there. Never the 19 September build in
     `…\release\` (`f5935dd3…`), and never a 1,254,400-byte Debug build like the one found in Forza's folder.
     The forwarder is not on any release yet, and the AMDNR build cannot install without it.
- Leave **Set as a pre-release** unticked: `make-manifest.ps1` refuses a pre-release for the launcher.
  (TheAutomatic's 1.9.1-alpha is a pre-release on their side; the script accepts that one on purpose.)
- Click **Publish release**, not **Save draft**. Nobody (including the script) can see a draft's files.

### 3. Fill in the manifest

The launcher's sha256 only exists once GitHub has the file, so it is filled in now, by the script.
In PowerShell, from the `launcher` folder:

    powershell -ExecutionPolicy Bypass -File tools\make-manifest.ps1 -AmdnrTag Alpha0.3.4 -LauncherTag Alpha0.3.4 -ForwarderPath publish
vngx.dll_dlssnr.dll -ForwarderUrl https://files.catbox.moe/dvn8sw.dll

`-AmdnrTag` names the AMDNR release users get: `Alpha0.3.3.2` today, `Alpha0.3.4` once that is published
(see *When AMDNR 0.3.4 is published*). `-TheAutomaticTag` defaults to `v1.9.1-alpha`, the TheAutomatic
build 0.4.0 offers. `-RuntimeTags` defaults to the AMDNR tag: `Alpha0.3.3.2` holds both runtime zips,
`v0.3.3-Runtime.zip` and `v0.4.0-Runtime.zip`, and the script refuses if either is missing (see *The two
runtimes*).

It reads each release's file list and the sha256 GitHub recorded for each upload, writes
`manifest.json`, and re-reads it to check it. With `-ForwarderPath` it also stops if the uploaded
forwarder is not the file on this PC. Any missing file, missing hash or unreadable release stops it
with a message, and the old `manifest.json` is left untouched. The launcher line must now show a real
hash, not `PENDING_UPLOAD`.

Commit it here: `git add manifest.json` then `git commit`. This repository is local only; it keeps the
copy the next run of the script replaces. Launchers read the copy on GitHub, which step 4 puts there.

**Never delete `%LOCALAPPDATA%\AMDNR\records` (or the whole `AMDNR` folder) in the steps below.** The
records are the only thing that knows which files in a game folder are the launcher's and where it moved
the game's own files (`AMDNR_backup\<date>`). Without them a test install cannot be uninstalled, and the
next INSTALL takes the launcher's own files for the game's. To make the launcher behave like a first run,
close it and delete only these three from `%LOCALAPPDATA%\AMDNR`: `settings.json` (the build choice),
`manifest-cache.json` and the `assets` folder (the downloads).

### 4. Try it before users see it

Put the new `manifest.json` on a branch of `3zwr1/AMD-NR---OptiScaler`, on github.com:

1. Open the repository. In the branch menu (top left, it says `main`), type `launcher-0.4.0` and click
   **Create branch launcher-0.4.0 from main**.
2. With `launcher-0.4.0` selected, open the file the launchers read:
   - **0.4.0, the first release:** `main` has no `Launcher/manifest.json` yet. Click **Add file** →
     **Create new file** and name it `Launcher/manifest.json` (typing the `/` makes the folder).
   - **Every release after that:** `Launcher/manifest.json` is already on `main`, so do not create it
     again. Open the `launcher` folder, click `manifest.json`, then click the pencil icon (**Edit this
     file**) at the top right of the file. Click into the editor, select all and delete the old text.
3. Open the new `manifest.json` from this folder in Notepad, select all, copy, and paste it into the page.
4. **Commit changes…** → **Commit directly to the `launcher-0.4.0` branch** → **Commit changes**.

Then, on a PC with an AMD GPU:

1. Close the launcher. In `%LOCALAPPDATA%\AMDNR`, delete `settings.json`, `manifest-cache.json` and the
   `assets` folder — not `records` (see above).
2. Run
   `AMDNR-Launcher.exe --manifest https://raw.githubusercontent.com/3zwr1/AMD-NR---OptiScaler/launcher-0.4.0/Launcher/manifest.json`
   — the caption shows a red **TEST MANIFEST** label. The address is used for that run only and never saved.
3. Go through `TESTING.md`.

raw.githubusercontent.com can serve the old copy of a file for a few minutes after a push; wait if
the launcher still sees the previous one.

### 5. Publish

On github.com, open **Pull requests** → **New pull request**, choose base `main` and compare
`launcher-0.4.0`, then **Create pull request** and **Merge pull request**. The file is now at
`Launcher/manifest.json` on **`main`**. Close the launcher and delete `settings.json`,
`manifest-cache.json` and the `assets` folder from `%LOCALAPPDATA%\AMDNR` again — not `records` —
because step 4 saved a build choice and cached every package there, so without this there is no
first-run chooser and nothing to download. Then run `AMDNR-Launcher.exe` with no arguments: no TEST
MANIFEST label, the first-run chooser appears, the downloads succeed, and the games installed while
testing still show as installed.

Until this step is done, a freshly downloaded 0.4.0 cannot find its manifest and cannot install
anything. Announce the release only after this check passes.

## The runtimes

The manifest offers three of danielblnc's runtimes as the packages `runtime-041`
(`v0.4.1-Runtime.zip`), `runtime-033` (`v0.3.3-Runtime.zip`) and `runtime-040` (`v0.4.0-Runtime.zip`). Every
build's `runtimes` list names them in the order of the `$Runtimes` table in `tools\make-manifest.ps1`, and
each entry says which GPU generations it is for (`rdna1`, `rdna2`, `rdna3`, `rdna4`, `unknown-amd`, `none`).
**The launcher installs the first entry whose list contains the detected generation.** Order decides, not
how specific an entry is.

> **Where the zips are today (26 September, evening):** `v0.4.1-Runtime.zip` and `v0.4.0-Runtime.zip` are on `Alpha0.3.3.2`;
> `v0.3.3-Runtime.zip` went missing from that release during the day and is read from `Alpha0.3.3.1`, so the manifest was
> generated with `-RuntimeTags Alpha0.3.3.2,Alpha0.3.3.1,Alpha0.3.3.2`. For 0.3.4 attach all three zips to `Alpha0.3.4` and
> the default (every zip from the AMDNR tag) is right again.

- **0.4.1 is the RX 9000 runtime** (`rdna4`), by the owner's decision of 26 September 2026, the day Daniel
  published it. It is listed first.
- **0.3.3 is the runtime for every other GPU** (`rdna2`, `rdna3`, `unknown-amd`, `none`); it also names `rdna4`,
  which is never reached because 0.4.1 comes first.
- **0.4.0 stays listed last**, RX 9000 only, so its package and hashes remain in the manifest; no card reaches
  it by default.
- `rdna1` (RX 5000) is on no list: no runtime runs there, so the launcher offers it none.

**How a newer runtime reaches players — no new launcher:** attach the zip, add its `$Runtimes` line first
for the cards it is for (see *New runtime*), run the script, publish through steps 4 and 5. On the next
start every game installed with another runtime for its card shows `Update available` with the line
`DLSSNR AMD files <new> are now the runtime for this card; this game has <old>.`, and REPAIR / UPDATE
installs it (backing up what it replaces, refusing while the game runs). Games installed without the files
(No at the chooser, the lmxxf path) and TheAutomatic installs are never moved. The reverse reorder puts a
runtime back the same way.

**Users pick too.** The chooser (first run and SETTINGS) offers every runtime the build lists for the card, in the
manifest's order, the first marked RECOMMENDED, then NO. Picking the recommended one is saved as no pick — that user
follows the manifest and moves when the card's first entry changes; picking another is saved by id (`RuntimeId` in
`settings.json`), is what downloads at start and what REPAIR / UPDATE installs, and the Doctor measures that user's games
against it, so a game on the picked runtime is never offered the card's default. A pick the build does not list for the
card (a GPU swap, a dropped runtime) is ignored and the card's first entry decides. So: to offer a runtime to a card, list
the card in its `gpus`; the order still decides what the launcher installs unasked.

Every listed zip must be in the release named by `-RuntimeTags` (default: the AMDNR tag; one tag for all,
or one per runtime in the table's order), or the script refuses. A manifest that names a runtime nobody
can download fails the install of everyone whose GPU picks it. All packages carry the same eight
`passSha256` values (see *New runtime* below): the Doctor checks installed pass DLLs against that list.

## When AMDNR 0.3.4 is published

The launcher does not change; the manifest does. Once the `Alpha0.3.4` release is up — `AMDNR-v0.3.4.zip`
and both runtime zips in it, or `-RuntimeTags Alpha0.3.3.2` if the runtimes stay where they are — run
step 3 with `-AmdnrTag Alpha0.3.4`, test the result on a branch through `--manifest` (step 4), merge it
(step 5). Every game installed from 0.3.3.2 then shows `Update available`, and REPAIR / UPDATE installs
0.3.4 over it, keeping the DLL name and the user's `OptiScaler.ini`. Before running the script, check two
tables in `tools\make-manifest.ps1` against the 0.3.4 source: `$AcceptedPassSha256` must hold every hash
in `kAmdLayouts[]` of `OptiScaler\dlssnr\amd\AmdLayout.h` (a runtime the mod accepts but the list lacks
reads as "could not be verified" in the Doctor), and `$Runtimes` must name every runtime zip 0.3.4 ships
(see *New runtime*). If the forwarder was rebuilt for 0.3.4, see step 2.

## Publishing the source

The launcher's source is published to be read, not reused: `LICENSE.txt` at the root is the licence
(source-available, all rights reserved), `README.md` summarises it, every source file carries the
copyright header, and ABOUT in the launcher shows the one-line notice. When the repository goes on
GitHub, do **not** pick a licence from GitHub's list — those are open-source licences and would
contradict `LICENSE.txt` — and keep `LICENSE.txt` and `README.md` at the root of what is uploaded. The
mods the launcher installs keep their own licences; the AMDNR mod itself is GPL-3.0 and its source is
published separately, as planned.

## New runtime

danielblnc published DLSS-NR on AMD **0.4.1** on 26 September 2026 and it became the RX 9000 runtime the same day
(its pass-DLL hash, `823063eb…`, was already in `$AcceptedPassSha256`). The next one follows the same steps: the
owner attaches `v<version>-Runtime.zip` to the AMDNR release the manifest reads (`-AmdnrTag`, or a tag named in
`-RuntimeTags`), makes sure its pass hash is in `$AcceptedPassSha256` (from `kAmdLayouts[]`), and adds one line to
`$Runtimes` in `tools\make-manifest.ps1`, first for the cards it is for:

    [ordered]@{ id = 'runtime-<digits>'; asset = 'v<version>-Runtime.zip'; gpus = @('rdna4') }

The script refuses if the zip is not on the release. `ManifestParserTests` pins the runtime list (today
`runtime-041`, `runtime-033`, `runtime-040`): update `manifest_json_lists_eight_accepted_pass_hashes_and_the_runtimes_by_card`
with the new id, version, URL, size and hash, then run the suite. A runtime Daniel has not published is never named in the
manifest, the script, the tests or these notes, until he releases it.

## Later releases

Always regenerate `manifest.json` with the script; never hand-edit it. The descriptions, TheAutomatic
exclude list, accepted pass hashes, proxy defaults and overrides, and hidden Steam app ids are the
tables at the top of `tools\make-manifest.ps1`. Change them there and run the script again, or the
next run undoes your edit.

**Whatever changed, the new `manifest.json` reaches users only through steps 4 and 5**: a branch named
after what you are releasing (for example `amdnr-0.3.3` instead of `launcher-0.4.0`, and the same name in
the `--manifest` test address), edit the existing `Launcher/manifest.json` there with the pencil icon,
test with `--manifest`, then merge the pull request into `main`. A `manifest.json` changed only on this
PC changes nothing for anyone.

- **New AMDNR build:** publish a release with `AMDNR-v<version>.zip`. The script only accepts that file
  name. Both runtime zips named in `$Runtimes` (`v0.3.3-Runtime.zip`, `v0.4.0-Runtime.zip`) must be in
  that release too, or pass `-RuntimeTags` with the tag of the release that has them. Run the script with
  the new `-AmdnrTag` and the current `-LauncherTag`. Installed games show *Update available*.
- **New runtime:** first add its pass DLL hash to `$AcceptedPassSha256` in the script, copied in full from
  `kAmdLayouts[]` in `OptiScaler\dlssnr\amd\AmdLayout.h` of the AMDNR source (eight layouts today, 0.2.17
  to the newest layout the mod accepts; every runtime package carries the whole list). The Doctor checks installed pass DLLs against
  it, so it can report the mod's "Private AMD runtime hash mismatch" before the game does, and a list one
  hash short has it call a good runtime unverified. Then add a `$Runtimes` entry: id `runtime-<version
  without dots>`, the zip's exact name, and the generations it runs on; where you put it decides which
  GPUs get it (see *The two runtimes*). A runtime that is no longer offered comes out of `$Runtimes`, not
  out of `$AcceptedPassSha256`: games that already have it are still checked.
- **New TheAutomatic build:** pass the new `-TheAutomaticTag` (or change its default in the script).
  Their files are always downloaded from their own release. Their repository has no licence, so never
  re-upload their zip anywhere. Their releases are accepted even when GitHub marks them pre-release,
  because the tag is always chosen by hand. Before publishing, open the new zip's `OptiScaler.ini`:
  `$TheAutomaticIniWhenRuntime` in the script sets `[DlssNr] Enabled=true` and `NrBackend=daniel` when
  the DLSSNR AMD files are installed, because their packaged ini ships Neural Rendering switched off and
  selects `lmxxf`, whose model files are not in their release. Compare the `[DlssNr]` keys their
  `Setup.ps1` writes with the packaged ini: any other key it changes belongs in that table too. If a new
  build ships those models or renames a setting, change that table first. Check the exclude
  list against the zip's root too.
  **Next candidate: `v1.9.3-alpha`** (2026-09-25, 134,356,192 bytes, sha256 `fac8f5bb…3c6a`; a 1.9.2-alpha
  exists too, and all of them are pre-releases). 0.4.0 keeps 1.9.1-alpha, the only zip inspected this way.
  Offer 1.9.3 only after its zip has had the same inspection: root layout against the exclude list, the
  `[DlssNr]` keys of its `OptiScaler.ini` against what its `Setup.ps1` writes, and whether the lmxxf model
  files are in it this time.
- **New launcher:** raise `<Version>` in `src\AmdnrLauncher.App\AmdnrLauncher.App.csproj`, then repeat
  steps 1–5 with a new tag (`Launcher<version>`) and a new branch (`launcher-<version>`). Upload **both**
  files again: the script reads the forwarder from the launcher release. In step 4,
  `Launcher/manifest.json` is already on `main`: open it on the new branch and edit it with the pencil
  icon (**Edit this file**), not **Add file** → **Create new file**. Running launchers see the higher
  version, download the new exe, check its sha256 against the manifest, and swap themselves.
- **New forwarder build:** upload it with the next launcher release and pass a higher
  `-ForwarderVersion` (the default is `0.2.1`). Launchers already re-download a cached forwarder whose
  hash no longer matches the manifest, but the version is what install records and bug reports name,
  so two different builds should never share one.

`-SkipLauncher` is only for making a manifest before the launcher release exists. Its output always
says `PENDING_UPLOAD` and is never published. The script calls the GitHub API three times per run (the
AMDNR release, TheAutomatic's and the launcher's), plus once for every `-RuntimeTags` tag that is not the
AMDNR tag. GitHub allows 60 calls an hour without signing in.
