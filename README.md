# AMDNR — DLSS 5 Neural Rendering on AMD (OptiScaler build) — v0.3.4.2

**English** | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **We need your support.** Join the Discord server — <https://discord.gg/AMDNR> — for
> help, bug reports and test builds; every report with a log makes the next build better.

DLSS 5 Neural Rendering running on AMD GPUs, built into OptiScaler so it works in any
Direct3D 12 game OptiScaler already hooks. On top of the neural pass: model interleave for a
large frame-rate gain, residual composition, XeSS frame generation unlocked up to 6X (up to 10X
opt-in in D3D12 games), and FSR Ray Regeneration for games that use DLSS Ray Reconstruction.

**Discord: <https://discord.gg/AMDNR>** — support, bug reports (`#bug-report`), test
builds.

**Support the project: <https://ko-fi.com/3zinr>**

> **The danielblnc runtime is Daniel Blanco's work.** The AMD neural runtime in the `*Runtime.zip` files
> (`dlssnr_amd_pass1..3.dll`) is **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR ships it unmodified, with his permission; it is not AMDNR's work. Please support his project.
> Full credits for everyone else are at the end of this page.

> **New in 0.3.4.2 (hotfix):** the menu in Assetto Corsa: the menu key toggles once per press, clicks shorter than a
> frame are no longer lost, and the runtime chooser answers to `1` / `2` / `Enter` / `Esc`, has a title-bar X and
> closing the menu counts as Decide later. The runtime chooser no longer opens the menu by itself (one notice
> instead), the **Ray Regeneration** section of the Neural tab no longer hides - it is always there and one dim line
> says why it is not running - and the Wine / Proton text says that Ray Regeneration is a known issue there. AMDNR
> also **accepts one more danielblnc runtime layout**, so a newer danielblnc build can be driven without an AMDNR
> update. **Neural Rendering is byte-identical to 0.3.4.1 except that one accepted-layout row**
> (the neural pass, both runtimes and the pak are unchanged): coming from 0.3.4.1 or
> 0.3.4, replace `OptiScaler.dll` only; launcher users: it updates for you. Details: `CHANGELOG.md`.

> **New in 0.3.4.1 (hotfix):** Ray Regeneration is less soft on Windows (0.25 sharpening when the game sends none;
> to turn it off: Image > Sharpness, tick Override, slider 0); no false "Upscaler failed to run!" popup in Control
> Resonant; on Linux / Proton the menu works (confirmed by a player, also with frame generation on) and Ray
> Regeneration adds no default sharpening there (see "Linux / Proton"). **AMDNR Launcher 0.3.4.1**, built from your
> Discord feedback: nine languages, search, favourites, hide, rename, CHOOSE GAME .EXE, PLAY, a full UNINSTALL and
> more (see "AMDNR Launcher"). Neural Rendering is unchanged from 0.3.4 (same runtime and pak): coming from 0.3.4,
> replace `OptiScaler.dll` only; launcher users: it updates for you. Details: `CHANGELOG.md`.

> **New in 0.3.4:** a new menu (the Neural tab is rebuilt, every other tab follows the same look, and a
> **Save report** button zips your logs for a bug report); lmxxf is faster on RX 7000 (1440p FSR Quality:
> 73.3 -> 52.2 ms per network run on an RX 7800 XT, network time measured outside a game) and on RX 9070 / 9070 XT (lmxxf 0.31 kernels);
> lmxxf runs on handheld APUs (experimental; one tester's run, in a game: about 29 fps in Shadow of the Tomb Raider on a ROG Ally); an opt-in
> lmxxf **Fast mode**; **AMDNR Screen GI**, AMDNR's own screen-space GI (preview, off by default); and many fixes. Replace `OptiScaler.dll`, `LmxxfNrRuntime.dll`
> and `LmxxfNrRuntime.pak` together. Details: `CHANGELOG.md`.

---

## AMDNR - OptiScaler Installation Guide

Installation is pretty simple. **On Windows, the AMDNR Launcher does all of this for you** (see "AMDNR Launcher"
below). By hand:

### 1. Download the files

Download these files from the latest release on GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>;
0.3.4.2 is the tag Alpha0.3.4.2):

* `AMDNR-vX.X.X.zip` (for 0.3.4.2: `AMDNR-v0.3.4.2.zip`), with the complete lmxxf runtime.
* For the danielblnc runtime, one runtime zip: on **RX 9000 and RX 7000** alike, `v0.4.3-Runtime.zip`
  (recommended) from Alpha0.3.4.1; `v0.4.1-Runtime.zip` and `v0.4.0-Runtime.zip` from the same release are still
  accepted. The lmxxf runtime is
  in `AMDNR-vX.X.X.zip` and needs no runtime zip on RX 7000 and RX 9000; handheld APUs use lmxxf only. The AMDNR
  Launcher picks the right zip for your GPU. See "What is in the archives".

### 2. Extract both files

Extract the contents of both `.zip` files.

### 3. Copy everything to the game folder

First, copy all files from `AMDNR-vX.X.X` into the game's root folder — the same folder where
the game's `.exe` is located.

Then, do the same with all files from the runtime zip (e.g. `v0.4.3-Runtime` on RX 9000 and on RX 7000).

> **Updating from an older AMDNR?** Copy everything again and overwrite. In 0.3.4 three files changed
> together: `OptiScaler.dll` (replace the file you renamed, e.g. `dxgi.dll`, with the new one renamed the
> same way), `LmxxfNrRuntime.dll` and `LmxxfNrRuntime.pak` (440 MB, new in 0.3.4). Do not
> mix them with older copies. You can keep your own `OptiScaler.ini`: new settings use their defaults.
> **From 0.3.4 to 0.3.4.1 and 0.3.4.2 only `OptiScaler.dll` changed:** replace that one file; `LmxxfNrRuntime.dll`,
> `LmxxfNrRuntime.pak` and your runtime zip's files stay as they are. The AMDNR Launcher does this for you: press
> REPAIR / UPDATE on a game that shows "Update available".

### 4. Rename OptiScaler.dll

Inside the game folder, find:

`OptiScaler.dll`

Rename it to:

`dxgi.dll`

`dxgi.dll` is the recommended option.

If the game doesn't launch or the mod doesn't load, try renaming `OptiScaler.dll` to one of
these instead:

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`
* `winhttp.dll`
* `wininet.dll`

Test one name at a time. Do not create multiple copies of `OptiScaler.dll`. These are the names the mod loads under
(plus `OptiScaler.asi` with an ASI loader); `d3d11.dll` is not one of them.

> **Resident Evil Requiem (and its demo) needs REFramework.** A known requirement, not an AMDNR bug: OptiScaler relies on it to
> get past Capcom's anti-tamper ([OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)). Without it the game crashes 15-60 s
> after launch ("An unhandled exception occurred"). Put `dinput8.dll` from `REFramework.zip` in the latest nightly
> (<https://github.com/praydog/REFramework-nightly/releases>) next to `dxgi.dll`, and change REFramework's menu key (e.g. to Delete): it is also Insert.
> After a game update, expect crashes until REFramework is updated. PRAGMATA, Monster Hunter Wilds and Onimusha probably need it too (not confirmed).

### 5. Launch the game

`HOME` switches Neural Rendering on and off while playing (both runtimes; a small notice says
On / Off). Rebind it beside the Enable checkbox in the Neural tab or under Interface > Keybinds.

That's it.

Launch the game normally and press:

`INSERT`

This will open the OptiScaler / AMDNR menu, where you can configure the mod however you like.

### If it doesn't work

If the game still doesn't launch with any of the names above, please report it in the
`#bug-report` channel on Discord.

When reporting the issue, also upload any `.log` files that may have been generated in the
game's root folder.

These logs are very important and will help us identify the issue much faster.

**The easy way: Save report.** If the menu opens, click **Save report** (the last row of Neural > Diagnostics, or the first row of Advanced > Logging). It writes one zip,
`AMDNR-report-<game exe>-<date>.zip`, into the game folder (on the Desktop if the game folder is read-only,
else in `%TEMP%`), with `report.txt`, the logs and the ini files, and the menu shows where it went. Your Windows
user name and PC name are replaced by placeholders; a name inside a game path outside `C:\Users\` is not.
Attach the zip in `#bug-report`.

> The `.exe` is usually not where the shortcut points. Unreal games keep it under
> `<Game>\Binaries\Win64\`.

---

### The lmxxf runtime (0.3.0, optional)

A second neural runtime (MIT-licensed, by lmxxf) can carry the pass instead of
danielblnc's. RDNA 4 runs it natively; RDNA 3 (RX 7000, Strix Halo) runs it through AMDNR's RDNA 3
backend by 3zwr1 - slower there, see "RX 7000" below: start at NR resolution 70% or lower. Handheld
APUs run it too, experimental (see "Handheld APUs" below). It needs two things next to the game:

1. `LmxxfNrRuntime.dll` - in this archive, beside `OptiScaler.dll` (it is copied with the rest).
2. `LmxxfNrRuntime.pak` (440 MB, included in the AMDNR zip) beside `LmxxfNrRuntime.dll` - lmxxf's
   weight files, HIP modules and HLSL in one encrypted, authenticated file. The runtime opens it in
   memory; nothing is unpacked to disk.

On the first launch that finds a runtime installed and no choice made, the menu asks which one
to use (`[DlssNr] NrBackend = daniel | lmxxf` in the ini records it; Neural > Neural runtime
changes it, on the next game start). The lmxxf edit is applied one frame late, carried by the
motion vectors, so the frame never waits for the network (about 14.1 ms of network time at 1080p
on an RX 9070 XT). Its log is `lmxxf_backend.log` next to the game.

**Compatibility (lmxxf).** The runtime sees only what DLSS sees, so what varies per title is
a short list: colour format and HDR, motion vectors and their scale, depth and its direction,
the reactive mask, the exposure texture, the Reset flag, and where the pass sits (before Super
Resolution, or after Ray Reconstruction). Tested so far:

| Title | API / placement | Notes |
|---|---|---|
| Silent Hill 2 | D3D12, before SR | reference title; Unreal's padded colour allocation handled |
| Forza Horizon 6 | D3D12, before SR | |
| Stray | D3D11 through the D3D12 bridge, before SR | |
| GTA V Enhanced | D3D12, before SR, HDR, one-channel reactive mask | fixed in 0.3.0: the mask used to read as "everything reactive" and the edit never landed |
| Any title with Ray Reconstruction | D3D12, after RR (written back into the output) | supported since 0.3.0; not yet confirmed in a game |

If a title shows no effect: `lmxxf_backend.log` has an `lmxxf inputs:` line (formats, sizes,
motion scale, depth direction, mask, exposure) and an `lmxxf stats @N:` line every 600 frames
(exposure, fed brightness, the model's edit, the carried edit, keep, reactive mean, vector
length and rejected fraction). Attach the log to a report; those two lines usually say why.

Both runtimes share one Neural tab (see "The menu" below). Controls the running runtime does not
have are greyed with a short tag or hidden with a count. lmxxf-only: **Full network**, **Output
smoothing** (Quality > More quality options, needs Network history), **Edit detail**, **Edit colour** and
**Edge guard** (Image look > Model strength: gain on the fine part of the model's edit, its colour against
its brightness change, and a fade of the edit across depth edges) and the auto-exposure highlight cap. New
on lmxxf in 0.3.4: Network output, Encoding, Residual edge fade, Game exposure, Fast mode, the
interleave pacing readout, and the model's native character mask with Structure intensity and
Character structure (each change rebuilds the network: about a 1 s hitch).

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, lmxxf only) runs all 71 of the
network's blocks instead of skipping 42, 43 and 46: slightly more faithful, about 0.5 ms slower at 1080p
(16.6 -> 17.1 ms on an RX 9070 XT, measured in 0.3.3). Off by default.

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf, opt-in, off by default) runs the network
one size tier lower (1080 -> 900, 900 -> 720): about 29% less network time at 1080p (RX 9070 XT, measured outside a
game), with fine detail a little softer. danielblnc builds that have their own Fast mode get a Fast mode row there
too (`[DlssNr] AmdDanielFastMode`); the runtimes in this release's runtime zips do not have it, so the row is hidden.

### RX 7000 (RDNA 3): faster with the network size tier (new in 0.3.4)

lmxxf's network runs at a few fixed sizes (tiers): 720 (1280x720), 900 (1600x900) and 1080 (1920x1080),
plus 576 and 360 (new, used on handhelds). A tier costs the same whatever part of it the picture fills. On
RDNA 3 (RX 7000, Radeon 8060S / 8050S and the handheld APUs) lmxxf's NR size now moves onto a tier by
default: down to the next smaller tier when it is nearer to it (cheaper), else grown to fill its own tier
(same cost, a little more detail), never above the frame's own size.

Network time per run on an RX 7800 XT (measured by a tester with lmxxf's probe; network only, mean of 30
runs; the 900-tier time was measured at 1600x900):

| Game setting | 0.3.3.2 | 0.3.4 on RX 7000 |
|---|---|---|
| 1440p, FSR Quality (1706x960 render), NR 100% | 1080 tier: 73.3 ms | 900 tier: 52.2 ms |
| 1080p render, NR 85% | 1080 tier: 73.2 ms | 900 tier: 52.2 ms |
| 1080p render, NR 70% | 900 tier: 52.2 ms | 720 tier: 34.4 ms |
| 1080p render, NR 80% | 900 tier: 52.2 ms | 900 tier, filled: 52.2 ms (more detail) |
| 1080p render, NR 100% | 1080 tier: 73.2 ms | unchanged |

- In game the gain per displayed frame is smaller: with Model interleave the network runs every 2nd frame,
  and the game has its own cost. Not yet measured in a game.
- The network sees a slightly smaller picture (at 1440p Quality about 6% fewer pixels per side), so fine
  detail can be a little softer. `[DlssNr] AmdLmxxfTierSnap=false` restores 0.3.3.2's sizes. RX 9000 keeps
  0.3.3.2's sizes unless you set it to `true`.
- **RX 9000:** `[DlssNr] AmdLmxxfTierSnap=true` (off by default there) moves lmxxf's NR size to a network size at
  every render resolution: some sizes drop a tier (1440p FSR Quality, 1707x960 -> the 900 size: network 14.08 ->
  9.96 ms per run on an RX 9070 XT, measured outside a game, a slightly softer image), others grow inside their tier
  (80% of a 1080p render -> 1600x900: same cost, a little more detail). Left unset, RX 9000 keeps 0.3.3.2's sizes.
- Start at NR resolution 70% or lower (the 720 tier at a 1080p render; the Performance preset is 70%). The
  cost beside NR resolution is priced by the tier the network runs; its hover names the tier.

### Handheld APUs (experimental, new in 0.3.4)

lmxxf runs on handheld APUs with 12 or more compute units, through AMDNR's RDNA 3 backend by 3zwr1:
**Z1 Extreme, Z2 and Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M and 880M** (gfx1150). It is
experimental and slow. The Neural runtime row says "experimental" after the RDNA 3 credit.
First tester results (ROG Ally, Z1 Extreme): lmxxf's probe outside a game, 54.7 ms per network run at the 360p size,
110.9 ms at 576p; in a game, one tester's run (Shadow of the Tomb Raider, 1280x720 with XeSS, Handheld preset), 62 ms per network
run on average at 360p with the model every 4th frame, about 29 fps with NR on.

- **Not supported:** Z1 and Radeon 740M (4 compute units), Radeon 760M (8), Radeon 860M / 840M. danielblnc's
  runtime does not run on handheld APUs. RX 6000 (RDNA 2) is planned for 0.4.0; the Steam Deck and other
  RDNA 2 APUs are not supported.
- **What it does by itself** (only while your ini has no value of its own): the network runs at its smallest
  size, 360p (640x360), and the model runs every 4th frame (Model interleave; not saved). Neural passes stay
  at 1.
- **Speed, honestly:** For scale: an
  RX 7800 XT (60 compute units) needs 34.4 ms per network run at the 720 size; these chips have 12 to 16 and
  run at lower clocks. Expect a large frame-rate cost even at 360p with the model every 4th frame, some
  ghosting from the long interleave, and a softer look than on a desktop GPU. The NR cost at the end of the
  Neural tab's status line (and in Diagnostics) shows the real number on your device.
- **Settings:**
  - Sharper but slower: `[DlssNr] AmdLmxxfTierCap=576` (the 1024x576 network size).
  - At a 720p or 800p render, NR resolution 100% already feeds the 360p size, so a lower NR resolution does
    not make it cheaper.
  - Model interleave Off is saved as `[DlssNr] AmdInterleave=1` (also off), so the handheld default does not
    come back at the next start. To turn it off by hand, write 1, not 0.
  - Preset > **Handheld** sets NR resolution 100%, Dynamic NR off, the model every 4th frame, 1 Neural pass and Full network off. The button shows only on these APUs; Quality, Balanced and Performance keep the 360p network size here too (the menu says so).
- **FSR 4:** on these chips (and the Radeon 780M / 760M / 740M in general) FSR 4 INT8 is no longer switched on
  by itself: with `Dx12Upscaler=auto` the upscaler is XeSS, and FSR 3.1 stays FSR 3.1. `[FSR] Fsr4ForceModel=2`
  still forces it (experimental).
- **Shadow of the Tomb Raider** (and games that make their D3D12 device twice) no longer crash when the upscaler
  starts (fixed in 0.3.4).
- **Driver:** use AMD's own Adrenalin driver. lmxxf needs HIP (`amdhip64_7.dll`), which some handheld makers'
  drivers leave out; `amd_bridge.log` then says HIP is not available.
- **Use the 0.3.4 files together** (0.3.4.1 and 0.3.4.2 change only `OptiScaler.dll`): only the 0.3.4 pak has the handheld
  modules, and the 0.3.4 `LmxxfNrRuntime.dll` refuses a handheld when `OptiScaler.dll` is older than 0.3.4 ("this
  handheld needs OptiScaler.dll 0.3.4 or newer").
- **Testers with a handheld:** ask on Discord for the handheld test kit (`handheld-test.zip`). Its
  `run_probe.bat` measures the network on your device and writes `handheld_result.txt` (your Windows user name
  is masked).

## Linux / Proton (Steam Deck, desktop Linux)

AMDNR runs under Proton and Wine as an OptiScaler build. **Neural Rendering does not run on Linux (Windows only):**
both NR runtimes need AMD HIP, and Proton and Wine do not provide HIP. With NR switched on under Proton, NR does not
run, and the Neural tab does not always say why. That is expected, not a crash or a broken install. The AMDNR
Launcher is a Windows program that can run under Proton (experimental, not tested by us yet, see below); installing
by hand works without it.

**What works:** the FSR upscalers (FSR 3.1, and FSR 4 on GPUs and drivers that support it), the menu (`INSERT`) and
**Save report**. A player confirmed on Steam Proton (RX 9070 XT, vkd3d-proton, Resident Evil Requiem) that the game
boots, the menu opens and takes the mouse, and Save report works, also with frame generation on.

**Ray Regeneration and frame generation:**
- Known issue: Ray Regeneration can show pink / magenta patches on Proton; the default sharpening is off there now, but if you still see them use plain FSR (FSR 4 on RX 9000) and send a Save report.
  On Proton, AMDNR adds no sharpening after Ray Regeneration when the game sends none (on Windows it adds 0.25):
  Image > Sharpness shows "RR default 0 (off on Proton)", and Override still sets your own value.
- **Frame generation** now turns on without crashing, but fps counters count the generated frames too: under a
  60 fps cap or 60 Hz V-Sync that is 30 real frames, which looks like 30. Leave it off on Proton for now
  (`[FrameGen] FGOutput=nofg`), or use it only when the game reaches about 60 fps without it, on a screen faster
  than 60 Hz.

**Requirements:** a current Proton or Wine (tested: Proton 11, which is Wine 11), with the game on vkd3d-proton
(D3D12) or DXVK (D3D11), which is Proton's default. Older versions are not tested.

**The AMDNR Launcher on Linux (experimental, not tested by us yet).** The launcher is the same Windows program,
`AMDNR-Launcher.exe` (self-contained: no .NET or other runtime to install). Under Wine / Proton it detects Wine and
shows a notice with what to do. It also looks for your Linux Steam library through Wine's `Z:` drive
(`~/.steam/steam` and `~/.local/share/Steam`, and the library folders in `libraryfolders.vdf`). We have not tested
this ourselves yet: if you try it, tell us on Discord whether it works. To try it:

1. In Steam, add `AMDNR-Launcher.exe` as a non-Steam game (**Games > Add a Non-Steam Game to My Library**).
2. In its **Properties > Compatibility**, force a Proton version (Proton Experimental), then start it from Steam.
3. If the game is not in **LIBRARY**, press **ADD** and choose the game's folder (your Linux folders are on the
   `Z:` drive); if the launcher picks the wrong `.exe`, use **CHOOSE GAME .EXE**.
4. Select the game and press **INSTALL**.
5. In the game's **Properties > General > Launch Options**, enter `WINEDLLOVERRIDES="dxgi=n,b" %command%` (if the
   launcher used another DLL name for the game, put that name in place of `dxgi`), then continue with steps 5 and
   6 of the install by hand below (start the game from Steam; **PLAY** in the launcher is off under Wine / Proton).

**Install by hand** (without the launcher):

1. Download `AMDNR-vX.X.X.zip` from the releases page (for 0.3.4.2: `AMDNR-v0.3.4.2.zip`). The danielblnc runtime
   zips (`v0.4.3-Runtime.zip` and the others) are only used by Neural Rendering, so you do not need them on Linux
   (copying one does no harm).
2. Extract the zip and copy everything into the game folder, next to the game's `.exe`.
3. Rename `OptiScaler.dll` to `dxgi.dll`.
4. In Steam, open the game's **Properties > General > Launch Options** and enter:

   ```
   WINEDLLOVERRIDES="dxgi=n,b" %command%
   ```

   This tells Wine to load the `dxgi.dll` in the game folder instead of its own; without it AMDNR does not load.
   If you used another name (for example `winmm.dll` or `version.dll`), put that name in place of `dxgi`, e.g.
   `WINEDLLOVERRIDES="winmm=n,b" %command%`. Lutris, Heroic and Bottles: add the same override (`dxgi` =
   `native,builtin`) in the runner's DLL overrides or environment settings.
5. In `OptiScaler.ini`, set `[FrameGen] FGOutput=nofg` (frame generation off, see above).
6. Start the game and press `INSERT` to open the menu. Set up the upscaler there.

**The menu.** In 0.3.4, with frame generation on, the menu could open without taking mouse or keyboard input, or
not open at all. 0.3.4.1 attaches the menu to the game window; a player confirmed on Proton that the menu opens and
takes the mouse, also with frame generation on. If it still happens on your setup, AMDNR shows the warning "Menu
window lost". Then, in `OptiScaler.ini`, set `[FrameGen] FGOutput=nofg`; if the menu still does not respond, also
set `[Menu] OverlayMenu=false` (the classic menu, which does not depend on the overlay window).

**HDR.** AMDNR does not switch HDR on under Proton. HDR comes from your Proton and desktop setup: a Proton build
with HDR support and a session that can show HDR (for example gamescope, or a Wayland desktop with HDR on). If
HDR works in the game without AMDNR, it keeps working with AMDNR; if the game's HDR option is greyed out, the fix
is in your Proton or desktop setup.

**Reporting a Linux problem:** use the menu's **Save report** button (keep `[Log] LogToFile=true`, the default, so
the report has this session's log); the report shows whether the game ran under Wine/Proton, vkd3d-proton or DXVK.
Please add your distribution, GPU, Mesa version and Proton version.

## Requirements

- Windows 10 or 11 (64-bit) for Neural Rendering. The AMDNR Launcher is a Windows program that can run under Proton
  (experimental, not tested by us yet). Under Linux / Proton AMDNR runs as an OptiScaler build without NR (see
  "Linux / Proton").
- An AMD GPU with a current driver. The neural runtime uses HIP through the driver; no HIP
  SDK and no developer mode are needed. Which chips:
  - RX 9000 (RDNA 4): both runtimes.
  - RX 7000 (RDNA 3, desktop and mobile): both runtimes - lmxxf through AMDNR's RDNA 3 backend, slower than on
    RDNA 4 (the network size tier is on by default, see above).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - Handheld APUs with 12+ compute units (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M / 880M): lmxxf,
    experimental and slow. Z1 (4 CU), 760M / 740M and 860M / 840M: not supported.
  - RX 6000 (RDNA 2): not supported yet, planned for 0.4.0. Steam Deck and RDNA 2 APUs: not supported (for
    Neural Rendering; for the upscalers under Proton see "Linux / Proton").

  The Neural tab says what your GPU can run (hover the runtime entries, or the GPU line in Diagnostics).
- A Direct3D 12, Direct3D 11 or Vulkan game. The AMD neural path itself is D3D12; D3D11 and
  Vulkan titles reach it through OptiScaler's D3D12 bridge, which means the upscaler must be
  one of the "w/Dx12" backends (`ffx_12`). Leave `Dx11Upscaler` / `VulkanUpscaler` on `auto`
  and this build picks it for you when neural rendering is on. With Neural Rendering on, the Upscaling
  list names them "... w/Dx12 - Neural".
- About 2 GB of spare VRAM at 1080p-class render resolutions.

## What is in the archives

**AMDNR-vX.X.X.zip**

| File | What it is |
|---|---|
| `OptiScaler.dll` | OptiScaler with the DLSS-NR AMD backend (AMDNR 0.3.4.2). Rename it as the guide says. |
| `OptiScaler.ini` | Settings. Neural Rendering is enabled; logging is on so a bug report has something to attach. |
| `LmxxfNrRuntime.dll` | The lmxxf neural runtime (0.3.4, unchanged in 0.3.4.1 and 0.3.4.2: lmxxf's kernels, including lmxxf 0.31's, AMDNR's c32w kernels, the small network sizes and the native character mask). Used only when chosen; reads `LmxxfNrRuntime.pak` beside it, see "The lmxxf runtime". |
| `LmxxfNrRuntime.pak` | The lmxxf runtime's weights, HIP modules and shaders in one encrypted file (440 MB, unchanged in 0.3.4.1 and 0.3.4.2; new in 0.3.4: the handheld modules and lmxxf 0.31's kernels). Only the lmxxf runtime reads it; harmless to keep with the danielblnc runtime. |
| `OptiScaler\` | FSR, XeSS, the FidelityFX denoiser and the D3D12 Agility SDK OptiScaler uses. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | Nukem9's dlssg-to-fsr3, unmodified and renamed: the game's DLSS Frame Generation calls served by FSR 3 frame generation, also on Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, see `Licenses/`. |
| `Licenses\`, `LICENSE` | Third-party licences, AMDNR's notice (`AMDNR_NOTICE.txt`) and the GPL-3.0 licence of this build. |
| `SHA256SUMS.txt` | Checksums of every file in this zip, and of the files in the danielblnc runtime zips it lists. |

**The danielblnc runtime zips** (DLSS-NR on AMD by Daniel Blanco, unmodified, with his permission; use one)

Which one: on **RX 9000 and RX 7000** alike, `v0.4.3-Runtime.zip` (recommended) from Alpha0.3.4.1;
`v0.4.1-Runtime.zip` and `v0.4.0-Runtime.zip` from the same release are still accepted. The lmxxf runtime needs
no runtime zip on RX 7000 and RX 9000; handheld APUs use lmxxf only. The AMDNR Launcher offers 0.4.3
(recommended), 0.4.1 and 0.4.0, and picks it for you.

| Zip | Release | danielblnc runtime |
|---|---|---|
| `v0.4.3-Runtime.zip` | Alpha0.3.4.1 | 0.4.3, **recommended on RX 9000 and RX 7000**; the danielblnc runtime settings work with it |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (and Alpha0.3.4) | 0.4.1, still accepted. Network style, Tone curve, Black lift and Game exposure are greyed with it |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (and Alpha0.3.4) | 0.4.0, still accepted; the danielblnc runtime settings work with it |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, retired: no longer recommended. It still runs if you already have it; the danielblnc runtime settings work with it |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; the danielblnc runtime settings are greyed with it |

**danielblnc 0.4.3 (published 2026-09-28) is supported day one:** its layout was already accepted in the host, so
AMDNR 0.3.4, 0.3.4.1 and 0.3.4.2 all drive it with no AMDNR update. His own numbers for it: +20% Reference /
+18% Fast vs 0.4.2, his measurement.

Each one holds:

| File | What it is |
|---|---|
| `dlssnr_amd_pass1..3.dll` | The AMD neural runtime, unmodified. Three copies so multi-pass has one per pass. |
| `dlssnr_on_amd_weights.bin` | The network weights the runtime loads. |
| `danielblnc_ATTRIBUTION.txt` | Daniel Blanco's credit and the terms AMDNR ships his runtime under. |

## The menu (new in 0.3.4)

Press `INSERT`. Every tab has the same look: text tabs, a header row with Discord and GitHub (it opens this page),
a credits row (Daniel Blanco's name opens his GitHub page), the **Components** line (how many of OptiScaler's seven
components are active; click it for the list), and a footer with Menu
Scale, Save Settings and Close. Help opens when you hover a control's label.

**The Neural tab, top to bottom:**

- **Enable Neural Rendering** and its key (the button, e.g. `Home`: click it, then press another key to rebind).
- **Neural runtime** (danielblnc / lmxxf, with the exact version of your files, e.g. `lmxxf 0.3.4`) with one state word: running, restart the game to switch, not
  installed, not for this GPU, or stopped. Under it the running runtime's credit and one status line, e.g.
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (the last number is the NR cost), and a closed **Live** row with more detail. When
  something needs you, one orange line follows, with a button when there is a fix (Retry lmxxf, Switch to
  danielblnc, Open Upscaling). In the default state there is none.
- **Preset**: Quality / Balanced / Performance set NR resolution to 100 / 85 / 70% and turn Dynamic NR off;
  nothing else. On handheld APUs a fourth button, **Handheld** (see "Handheld APUs"). **NR style**, and **Style
  slots** (Store / Apply / Clear).
- **Performance**: NR resolution (%) with its cost, Neural passes, Full network, Fast mode, Dynamic NR resolution, Model
  interleave (Interleave preset and the pacing line appear under it while it is on).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS), and **More quality
  options** (Network history - one checkbox for both runtimes -, Output smoothing, Stability mode, Residual
  temporal, Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail and Colour strength, and three folds: **Model strength** (Tone and
  Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask, and
  danielblnc's Network style, Tone curve and Black lift), **Exposure and highlights** (Auto-exposure, its
  highlight cap, Highlight colour guard, Game exposure) and **Appearance filter** (its off / on word after
  the name). A dim "default" or "custom" after a fold's name says whether you changed something inside it.
- **Ray Regeneration**: its own section. Since 0.3.4.2 it is always there: while FSR Ray Regeneration is not
  running, a dim line says why, and the controls stay on screen,
  greyed out, until it runs.
- **The tools row**, closed at start: **Diagnostics** (Network output, Debug view, the RR debug view, Edit
  shaper A/B, NR cost, the ghost and self-tuning readouts, the GPU line, **Save report**), **Runtime options** (Encoding,
  Every-frame NR, NR slots, Highlight proxy) and **Experimental** (AMDNR Screen-space GI, a preview).

A control the running runtime does not have is greyed with a short tag (e.g. "not in lmxxf yet") or hidden
with a count ("3 danielblnc-only options hidden"); switching runtime moves no other row.

**The other tabs:** Upscaling starts with the upscaler, one status line and Render resolution (the former
Upscale Ratio Override and Output Scaling); on a non-NVIDIA card "DLSS w/Dx12" is no longer listed. Image
holds Sharpness, Textures, Init Flags and the Magnifier. Frame Gen opens with FG Input and FG Output.
Interface has the FPS overlay and Keybinds (one button per key). Advanced starts with Active Quirks, then Display (V-Sync),
Compatibility and Logging. The settings, keys and what Save Settings writes are unchanged, except where
`CHANGELOG.md` says so.

## Settings worth knowing

Open the **Neural** tab. The defaults are the most recent tested arrangement, so the useful first
move is to change one thing at a time.

- **NR resolution** — the main quality/cost lever. Below 100% the model works on a smaller
  picture and only its *correction* is carried back up to the full-resolution frame, so the
  frame keeps its own detail. Above 100% cost grows with the square (150% is 2.25x). The slider
  moves in 5% steps: each new NR size can keep VRAM until the game restarts, so restart the game
  after many changes. The cost beside it reads 1.00x at 100%; on lmxxf it is the price of the network size
  tier it runs (its hover names the tier). The Preset buttons set it to 100 / 85 / 70%.
- **Residual strength** — how much of the model's edit is applied; above 1 it amplifies. This is
  the control that changes the picture most.
- **Residual limit** — a ceiling on how far one pixel may move. Blotchy patches: **lower** it.
- **Model interleave** — runs the model every second frame for a large frame-rate gain. The
  skipped frames are filled by the **Interleave preset**; *Edit accumulation* (preset 10, both
  runtimes) is the default: every frame is that frame's own picture plus the model's carried
  correction, so no picture is held over. *Guided fill v2* (preset 6, danielblnc) and *Classic
  carry* (lmxxf) are the older fills. Pacing of the two frame types is automatic on danielblnc and off on
  lmxxf (`[DlssNr] AmdInterleavePacing` 0..1 paces both, at a frame-rate cost); a dim line under the preset
  shows the measurement. Adaptive interleave is switched off in this build.
- **Neural passes** — 2 and 3 stack the model, with diminishing returns. Under lmxxf the
  network's history stays its first pass; the extra passes are spatial refinement only.
  danielblnc runs 1 pass on Vulkan titles (a note under the slider says so).
- **Colour composition** (Neural > Image look, both runtimes) — *Classic* (default) is the picture
  you had before. *RenoDX (experimental)* runs RenoDX's colour composition after the model, as the
  NVIDIA path does: Composition detail and colour, a two-sided **Highlight guard** (2x by default)
  that bounds the model's answer against the original, and optional skin / environment controls.
  On a display-referred (SDR) frame, with Network output, or with Encoding sRGB / Gamma 2.2 it falls back to
  Classic on both runtimes; the menu's note then offers a button that turns the blocker off. NR styles
  and presets leave it alone.
- **Native character mask** (Image look > Model strength, `[DlssNr] AutoMask`, on by default) — the model's own
  treatment of faces and skin. Unticking it now acts on both runtimes (on lmxxf it rebuilds the network: about
  a 1 s hitch); on lmxxf, Structure intensity and Character structure now act too.
- **Frame generation is off in a fresh ini.** Frame Gen tab: choose the FG Input (e.g. "DLSSG via
  Streamline" in a game with DLSS frame generation) and FG Output (XeFG), then tick **Active** in
  the Frame Generation (XeFG) section and press Save Settings. A 0.1.0 ini that had it on is not
  carried over when you install 0.2.0's ini.
- **XeFG multi-frame generation** — 3X to 6X is built in and on by default (`XeFG\UnlockMFG`),
  for OptiScaler's copy and the game's own. **Delete `XeFGUnlock.asi`** from `OptiScaler\plugins`
  if you still have it: two copies of the same patch crash the game.
  **Up to 10X is opt-in** (D3D12 games only): set *XeFG ceiling (restart)* under FG Output in the
  Frame Gen tab (4X, 6X default, 8X or 10X; `[XeFG] MaxInterpolatedFrames`), restart, then pick the
  multiplier in the MFG combo. Above 6X it needs OptiScaler's own XeFG provider with Extra pacing on;
  a game's own XeSS 3 copy stays at 6X at most. 10X needs a 360 Hz+ display and a frame cap at
  refresh / 10; latency is high, and the provider reserves about 128 MiB more VRAM at 4K.
  7X-10X is not yet confirmed in a game: testers, please send `OptiScaler.log`.
- **FSR Ray Regeneration** — RX 9000 and RX 7000 (RDNA 4 and RDNA 3) by default; only in games that use DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), with the game running DLSS (spoofing on), ray tracing and Ray Reconstruction
  enabled in its own settings. Neural Rendering then runs after it, on its output, which costs
  more: lower the NR resolution if the frame rate drops. Its controls have their own section,
  **Neural > Ray Regeneration**, always shown since 0.3.4.2: while Ray Regeneration is not running, a dim line says
  why (the game has not turned Ray Reconstruction on, Ray Regeneration gave this title up and why, or when it last
  ran), and, where your card has something to add, a third dim line says it; the controls
  stay on screen, greyed out, until it runs. The **path-traced profile**
  (less grain on faces under path tracing) is opt-in since 0.3.3.1: tick it there to try it in Resident
  Evil Requiem or PRAGMATA. The same section has the bias mask strength and **skin
  smoothing** (experimental, for games that publish an SSS guide; off by default, but on by default in
  Resident Evil Requiem since 0.3.3.2); the temporal tuning sliders are under *More Ray Regeneration options*,
  and the RR debug view is in Diagnostics. On RX 7000 (RDNA 3) FSR Ray Regeneration is offered by default again (0.3.3.2 offered it on RDNA 4
  only). AMD ships it for RDNA 4 only: if the driver refuses it, the game gets FSR without the denoiser.
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false` keeps it to RDNA 4; RX 6000 and older get it only with `true` (Upscaling
  tab: **Offer FSR Ray Regeneration on this GPU (restart)**). **Sharpening after RR** (0.3.4.1): when the game
  sends no sharpness, AMDNR sharpens by 0.25 after RR on Windows (0 on Linux / Proton); to turn it off: Image >
  Sharpness, tick Override, slider 0. Since 0.3.4.2 that number is its own ini key,
  `[Sharpness] RrDefaultSharpness` (same 0.25 default): set 0.15, 0.10 or 0 there without touching Override, and a
  value your ini kept under `[Sharpness] Sharpness` while Override is off is tagged in the menu as waiting.
- **AMDNR Screen GI** (preview, new in 0.3.4, off by default; Neural > Experimental, or `[AmdGi] Enabled=true`) — AMDNR's own screen-space bounce light and ambient occlusion, from the game's depth, before NR and the upscaler; works with NR on or off; about 1 ms at High for a 1080p render on an RX 9070 XT (measured outside a game). Screen-space: light from off-screen is missing. See `CHANGELOG.md`.
- **Save report** (Neural > Diagnostics, or Advanced > Logging) — one zip with every log and the ini files for a bug report; see "If it
  doesn't work" above.

## If something goes wrong

`OptiScaler.log` appears in the game folder. Attach it in `#bug-report`, and say which game and
which GPU; **Save report** (Neural > Diagnostics, or Advanced > Logging) zips it with everything else. The AMD backend also writes
`amd_presr.log` and `amd_bridge.log`, which are the useful ones when the neural pass specifically
misbehaves. The last three sessions' logs are kept as `OptiScaler.previous.<exe>.log` (newest),
`OptiScaler.previous-1.<exe>.log` and `OptiScaler.previous-2.<exe>.log` (`[Log] KeepPreviousLogs`, 1 keeps only
one as before). After a crash, attach them too: the new log then says "no clean exit recorded" (since 0.3.4 no
longer after a normal quit).

**NR frames 0/s, and the Neural tab or `amd_presr.log` says the pass DLL is a build this AMDNR does not
drive?** Your `dlssnr_amd_pass1..3.dll` are a danielblnc build this AMDNR does not know (a 0.2.16 set was
seen in the wild), or one of the three is missing. Since 0.3.3.2 the Neural tab names the file and its
version and says what to do. Use the recommended runtime, all three pass DLLs from the same zip: on **RX 9000 and
RX 7000** alike, `v0.4.3-Runtime.zip` from Alpha0.3.4.1 (116,484,918 bytes, SHA256 starting `07dd7774`);
`v0.4.1-Runtime.zip` and `v0.4.0-Runtime.zip` from the same release are still accepted (the `dlssnr_amd_pass1.dll`
in `v0.4.1-Runtime.zip` is 9,916,928 bytes, SHA256 starting `823063eb`; in `v0.4.0-Runtime.zip`: 10,027,008 bytes,
`d62be3d8`). Supported builds: 0.2.17,
0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0, the runtime zips named above, and newer builds already accepted here, ahead
of their release. Do not install danielblnc's own setup or its `dxgi.dll` / `version.dll` /
`winhttp.dll` next to AMDNR: AMDNR already runs his runtime. **0.3.4.2 accepts one more layout:** a newer danielblnc
build can be driven here without waiting for an AMDNR update - the Neural tab names the file and its version as
usual, and the danielblnc-only settings (Network
style, Tone curve, Black lift, Game exposure, Fast mode) work with it. Nothing changed for the runtime files you
already have.

**lmxxf does nothing, or stops at once, on a PC with integrated graphics?** Fixed in 0.3.3.2. On a Ryzen
desktop with its integrated graphics on, a laptop with an AMD APU and a Radeon, or a PC with two AMD GPUs,
the game's GPU is often not HIP device 0. lmxxf then failed on its first frame
(`hipErrorInvalidHandle (400)`, then "session is poisoned" in `lmxxf_backend.log`) and stayed off. Replace
both `OptiScaler.dll` (the file you renamed, e.g. `dxgi.dll`) and `LmxxfNrRuntime.dll` with the files of
0.3.3.2 or newer. Not yet tested on such a PC: if lmxxf still stops, the Neural tab now says why; send
`lmxxf_backend.log` and `amd_bridge.log` (it lists the HIP devices).

**A PC with integrated graphics and a Radeon (a Ryzen desktop with its integrated GPU on, or a laptop): NR never
starts, and the Neural tab or the GPU line names the integrated GPU?** AMDNR's neural pass runs on the GPU the game
draws with. If Windows started the game on the integrated GPU, NR does not run on your Radeon at all. Set the game
to the discrete GPU: Windows Settings > System > Display > Graphics, add the game's `.exe`, Options, High
performance; then restart the game and check the GPU line in Neural > Diagnostics, which names the adapter NR runs
on (`OptiScaler.log` has an `AMD neural: NR runs on ...` line when that adapter is not the primary GPU). Seen in
Starfield on a Ryzen desktop.

**lmxxf's status line says `c32w=off:nofile` on an RX 9070 / 9070 XT?** An old
`DLSS5-AMD\native-game-tiled-assets` folder next to the game's `.exe` (left from an earlier lmxxf setup) is used
instead of `LmxxfNrRuntime.pak`. It has no c32w kernels, so lmxxf runs at the old speed. Remove or rename the
`DLSS5-AMD` folder: the pak holds everything lmxxf needs. A `LmxxfNrRuntime.pak` older than 0.3.3.2 shows the
same status; replace it with the one in this release.
`fk=fff-` in the same line means the same (an old pak or a loose folder): lmxxf still runs, at the
old speed.

**danielblnc: the NR style still changes when NR resolution leaves 100%?** Still open in 0.3.4, 0.3.4.1 and 0.3.4.2,
and the default is unchanged. At 100% Residual strength 0.99 gives 99% of 1.00 (fixed in 0.3.3.2); away from 100%
(also Dynamic NR steps and the Balanced / Performance presets) strength, limit and edge fade still act on the
whole result, so the look can change. 0.3.4 adds an A/B to find the right fix: Neural > Diagnostics > **Edit
shaper (A/B, not saved)** with Literal, F1 and F2, plus Only below 100% and Carry cap (danielblnc only; Save
Settings does not keep it; the ini keys are `[DlssNr] AmdEditShaper`, `AmdEditShaperLimit`,
`AmdEditShaperScope` and `AmdEditShaperCarryCap`). If one of them makes 85% look like 100% in your game, tell
us on Discord with screenshots. lmxxf is not affected.

**The menu opened and closed twice per key press, or keyboard and mouse stopped working on the whole desktop
while the menu was open (Assetto Corsa)?** Fixed in 0.3.4: a second menu or NR key press within 400 ms is
ignored (`[Hotfix] MenuToggleDebounceMs`, 0 = the old behaviour), and while the menu is open a game's
low-level keyboard or mouse hook is skipped but the key still reaches Windows
(`[Hotfix] MenuLowLevelHookPassThrough=false` = the old behaviour). Not yet confirmed in Assetto Corsa: send
the report zip if it still happens.

**The menu opened by itself on the runtime chooser, clicks did nothing, or the menu key hid the menu only while it
was held (Assetto Corsa)?** Fixed in 0.3.4.2: the menu key toggles once per physical press (a key message that
arrives late is ignored), clicks and menu keys shorter than a frame are replayed, the runtime chooser answers to
`1` / `2` / `Enter` / `Esc` and its title-bar X, and closing the menu counts as Decide later; the chooser no longer
opens the menu by itself. Not yet confirmed by the Assetto Corsa player: send the report zip if it still
happens. To get 0.3.4.1's menu key and clicks back: add `DiagInputHooksSkip=presslatch,clickreplay` under
`[Hotfix]` in your `OptiScaler.ini` yourself (no new key; the shipped ini only describes the line in a comment).

**Uncharted: Legacy of Thieves Collection crashes a few seconds after start on RX 9000 with Neural Rendering on?**
Known, not fixed in 0.3.4.2: the game runs its work on small 192 KiB fibers, and HIP's first initialisation (the
driver compiles its helper kernels inside the game) overflows that stack at the first NR frame. Keep
`[DlssNr] Enabled=false` in that game's `OptiScaler.ini` until 0.3.5. Switching runtime does not help: both runtimes
start HIP on the same thread.

**A Vulkan game (Indiana Jones and the Great Circle) stops at start with "Could not create the Vulkan
device (VK_ERROR_EXTENSION_NOT_PRESENT)"?** Fixed in 0.3.2: the inherited NVIDIA neural path asked the
AMD driver for two NVIDIA-only device extensions. Vulkan titles reach the neural pass through
OptiScaler's D3D12 bridge (see Requirements).

**lmxxf froze a Vulkan game at the first NR frame?** Fixed in 0.3.3; expect one hitch of about 1 s when NR
starts. If a Vulkan session ever stops before lmxxf's first answer, the next start runs danielblnc's runtime
and the Neural tab says why; press **Retry lmxxf** there (it removes `lmxxf_vk_launch.pending` beside
`OptiScaler.dll`) to try lmxxf again.

**danielblnc paused for seconds, then stopped NR, on a Vulkan game (Indiana Jones) with 2-3 Neural
passes?** Fixed in 0.3.3: on Vulkan titles it runs 1 pass, and its 80 ms post-submit wait is gone. The
first NR frame of a session still pauses about 5 s; a note under the runtime choice explains its log
lines. Testers: `[DlssNr] AmdVkLateCopyWait=true` (experimental, off by default, not yet tested in a
game) is expected to remove that pause; send `OptiScaler.log`, `amd_presr.log` and `dlssnr_on_amd.log`.

**lmxxf's RAM use climbed for as long as NR ran?** Fixed in 0.3.3 (it was about 45 GB an hour at 60 NR
fps). What remains: danielblnc keeps VRAM for each new NR size above about 1 MP (0.3.3.2 rounds its sizes
to 64 px away from 100%, so there are only a few); restart the game after many changes with danielblnc. Since
0.3.3.2 lmxxf no longer keeps about 97 MB per NR resolution or DLSS mode change: it makes its network buffers once
per network size and reuses them (a small rest of about 10-25 MB of VRAM per change remains).

**A Streamline game fails at start with slInit error 0x18 (seen with NBA 2K27 on AMD)?** 0.3.3 closes
one way OptiScaler's Streamline plugin hooks could cause it, but that is not confirmed as NBA 2K27's
cause. `OptiScaler.log` now records `slInit returned ...` and `[SLINIT]` lines: send the log with the
report.

**You cannot find the Ray Regeneration settings?** Since 0.3.4.2 the section is always in the menu: press
**Insert**, open the **Neural** tab and scroll to **Ray Regeneration** (in 0.3.4.1 and older the section was drawn
only while Ray Regeneration was running, or when it had given the game up, which is what an RX 7000 player ran into).
While it is not running, one dim line under the heading says why: the game has not turned Ray Reconstruction on, Ray
Regeneration gave this title up and why, or it last ran N seconds ago. On RX 7000 a third dim line adds that Ray
Regeneration is offered by default there but the driver may refuse it (then the game keeps its own denoiser); on
RX 6000 and older it says that it is not offered on this GPU, that AMD ships it for RDNA 4, and that
`[FSR-RR] FfxDenoiserAllowPreRdna4=true` offers it anyway. The controls stay on screen, greyed out, and act again as
soon as it runs. To get it running, in the game's own graphics settings: pick **DLSS** as
the upscaler (not FSR, not XeSS), turn **ray tracing** or path tracing on, and turn **Ray Reconstruction** (DLSS-RR)
on; the Upscaling tab then reads "FSR Ray Regeneration". Which setting to change for which problem is in
the Ray Regeneration settings guide **RR-BEST-SETTINGS.md** (not in the zip).

**Ray Regeneration looks grainy or noisy?** Judge it with **Neural Rendering switched off** first (untick **Enable Neural Rendering** at the top of the Neural tab, press Home while you play, or set
`[DlssNr] Enabled=false`): the neural pass runs after Ray Regeneration, on its output, so a
screenshot taken with NR on tells you nothing about the denoiser. Then, per kind of grain - crawling grain in a still
scene, bright specks, grain on faces, trails behind moving people - the settings to try are in
the same guide, **RR-BEST-SETTINGS.md**. **No denoiser default and no sharpening default changed in
0.3.4.2**: the numbers are 0.3.4.1's. What did change is that the sharpening AMDNR adds after Ray Regeneration is
its own ini key now, `[Sharpness] RrDefaultSharpness` (same 0.25 default), so 0.15, 0.10 or 0 is an ini edit
instead of a new build. Check your ini before you chase grain with the sharpness slider: a value under
`[Sharpness] Sharpness` does nothing while `OverrideSharpness` is off, and it applies at once the moment you tick
**Override** in the menu - which is why Image > Sharpness now tags it ("ini Sharpness 1.00 waits for Override"). Two things we will not pretend away: a part of the grain is the game's own ray sampling -
AMD's denoiser is not built to repair noise that arrives correlated, and a game that offers DLSS Ray Reconstruction
switches its own denoiser off and hands us the raw signal - and the grain that crawls in a still scene has a
structural cause on our side that no slider removes completely. That one is a known issue; the work for it is
0.3.5.

**The game's Ray Reconstruction is on but the Neural tab says "Ray Regeneration is off in this title"?**
The game does not publish what FSR Ray Regeneration needs: its DLSS plugin passes empty camera matrices
(Satisfactory), which NVIDIA's Ray Reconstruction treats as optional and FSR Ray Regeneration needs. FSR
upscaling runs in its place and NR takes its normal pre-SR position; the Upscaling tab says the same. Since
0.3.4 it stays off for the whole session in an Unreal title with this signature. Turn Ray Reconstruction off
in the game and restore the engine's own denoiser settings.

**A Ubisoft Anvil game (AC Black Flag Resynced, Shadows, Mirage) shows "DX12 Error 0x80070057"?**
Those games carry their own XeSS Frame Generation. This build leaves it to them (OptiScaler's XeFG
output stands down there and the Frame Gen tab says so); use the game's own XeSS FG option. If
it still happens, set `[FrameGen] Enabled=false` and `[fakenvapi] ForceXeLL=false` and report
with the log.

**The Last of Us Part I crashes on boot?** That is the game's own Streamline init, a known
OptiScaler issue: rename `sl.common.dll` in the game folder to `sl.common.dll.bak` and pick
**FSR 3.1** in the game's settings instead of DLSS.

Full notes for every version: `CHANGELOG.md` (in the zip and in the repository).

## Roadmap

- **0.3.4.2** (this build) — hotfix: the Assetto Corsa menu (the menu key toggles once per press, clicks shorter
  than a frame are replayed, the runtime chooser answers to keys and closing the menu counts as Decide later), the
  runtime chooser no longer opens the menu by itself, the Ray Regeneration section is always in the Neural tab and
  says why it is not running, one more accepted danielblnc runtime layout, Wine / Proton text
  corrections, README additions (proxy names, Uncharted, hybrid PCs, `AmdLmxxfTierSnap` on RX 9000, the two Ray
  Regeneration FAQ entries); NR byte-identical to 0.3.4.1 except that one accepted-layout row.
- **0.3.4.1** — hotfix: Ray Regeneration sharpening when the game sends none (Windows; none by
  default on Linux / Proton), Control Resonant's false "Upscaler failed to run!" popup, the Linux / Proton menu
  attaches to the game window, Save report names vkd3d-proton / DXVK; AMDNR Launcher 0.3.4.1 (nine languages,
  search, favourites, hide, rename, CHOOSE GAME .EXE, PLAY, a full UNINSTALL); NR unchanged.
- **0.3.4** — the new menu (the Neural tab rebuilt, the same look on every tab, Save report);
  lmxxf faster on RX 7000 (the network size tier by default) and on RX 9070 / 9070 XT (lmxxf 0.31
  kernels); lmxxf on handheld APUs (experimental; new 360p and 576p network sizes); lmxxf gets
  Network output, Encoding, Residual edge fade, the native character mask and an opt-in Fast mode; AMDNR Screen GI
  (preview); danielblnc
  runtime settings (Network style, Tone curve, Black lift, Game exposure) and a highlight colour guard; Ray
  Regeneration tuning and diagnostics; fixes for Assetto Corsa's menu input, Shadow of the Tomb Raider, Marvel's
  Midnight Suns and The Last of Us Part II, clean exit and logs.
- **0.3.3.x** — lmxxf on RDNA 3 (RX 7000; AMDNR's own backend); RenoDX colour
  composition (experimental, opt-in) on both runtimes; lmxxf: Full network option, the RAM leak
  fixed, Vulkan titles fixed (lazy weight upload inside the Vulkan bridge), 0.29 kernels (bit-exact,
  faster); danielblnc on Vulkan titles: 1 Neural pass, clearer messages, an opt-in late copy wait;
  XeFG up to 10X (opt-in, D3D12); Streamline start-up hardening and diagnostics; FSR Ray
  Regeneration path-traced profile and skin smoothing; UE5 robustness.
- **0.3.2** — the 0.3.1 reports: Vulkan titles start and run with lmxxf, lmxxf colours
  matched to danielblnc's (auto-exposure), the runtime combo, Ray Reconstruction status and tuning;
  Nukem9's dlssg-to-fsr3 in the zip for frame generation on Vulkan.
- **0.3.1** — fixes from the first 0.3.0 reports (lmxxf alone never ran, Where Winds
  Meet's silent NR, the crash on a DLSS-quality change, GTA V Legacy) and NR style presets with
  three custom slots.
- **0.3.0** — the **lmxxf** HIP neural runtime (RDNA 4) as a selectable runtime
  beside danielblnc's, shipped as `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: network history,
  real Neural passes, the edit shaper, the after-Ray-Regeneration placement, per-title
  diagnostics and self-healing. Many thanks to TheAutomatic, whose DLSS 5 AMD project work
  this integration builds on.
- **0.4.0** — RX 6000
  (RDNA 2), and support for titles without an upscaler of their own (Stray-class), where OptiScaler
  supplies the upscaler and the neural pass together.

---

## Credits

This build is a wiring job over other people's work. If you find it useful, the thanks belong
upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (the `*Runtime.zip` files, unmodified)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (the network port, kernels and HIP runtime, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h`: portions contributed to lmxxf by TheAutomatic (MIT)
- **lmxxf 0.31 kernels** in `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — lmxxf's (Kien, MIT), built by AMDNR from lmxxf's sources and build recipe; AMDNR's part is the loading, SHA-256 pins, per-GPU gating and fallbacks
- **c32w kernels** (0.3.3.2) — AMDNR's own one-wave RDNA 4 kernels for lmxxf's network, Copyright (c) 2026 3zwr1 (AMDNR); ideas from AMD's public RDNA 4 WMMA docs (GPUOpen, ROCm matrix instruction calculator)
- **AMDNR's RDNA 3 backend** (0.3.3; the handheld builds gfx1103 / gfx1150 in 0.3.4), the network size tier policy and the small network sizes (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, unmodified)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (colour composition maths, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), the base of the built-in XeFG multi-frame unlock and its pacing
- **burak113** — the FSR Ray Regeneration preprocessor (OptiScaler branch ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (the inherited effect; retired from the menu in 0.3.4, `[AmdRtgi] Enabled` in the ini) — an effect AMDNR inherited from the OptiScaler-AMD-PreSR lineage; the credit belongs to its original authors. It needs the `experimental_lighting` folder of the danielblnc package, which AMDNR does not ship.
- **AMDNR Screen GI** (0.3.4 preview) — AMDNR's own, Copyright (c) 2026 3zwr1 (AMDNR), written from published papers (Therrien, Levesque and Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; and the others listed in `CHANGELOG.md` and `Licenses/AMDNR_NOTICE.txt`)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (new in 0.3.4) is a Windows 10 / 11 program that can also run under Proton on Linux
(experimental, not tested by us yet: see "Linux / Proton"). It installs and updates AMDNR per game: it finds your
games (Steam, Epic, the Xbox app, Ubisoft Connect, the EA app, GOG, Rockstar, Battle.net and Amazon Games), picks
the DLL name, downloads the build and the danielblnc runtime you choose, checks every install with its Doctor, and
updates itself. Download `AMDNR-Launcher.exe`, the AMDNR Launcher from the latest release, from the releases page:
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**New in Launcher 0.3.4.1: you asked, we built it** (from the first Discord feedback):

- Nine languages: English, Arabic, Chinese (Simplified), French, Spanish, Portuguese, Italian, Russian and Polish.
  The launcher follows your Windows language (else English); pick another under LANGUAGE or in SETTINGS, where it is
  also offered on first start. Doctor findings, install messages and the COLLECT LOGS report stay in English so
  support can read them.
- Search the library; favourites (a star, and starred games first); hide games (HIDDEN shows them again); rename a
  game.
- CHOOSE GAME .EXE: pick the game's exe yourself when the launcher picked the wrong one or none. Cyberpunk 2077 and
  The Witcher 3 (REDengine) are now found in the right folder without it.
- PLAY and OPEN FOLDER on every game's page; STORES switches whole stores on and off; folders you add by hand stay
  listed, also while their drive is unplugged, until you remove them.
- UNINSTALL asks first and removes everything the mod placed, including what it wrote while the game ran (logs,
  caches, crash dumps, unfinished reports); it keeps finished Save report zips and a DLL under the proxy name that
  is no longer an OptiScaler (the game's own).
- COLLECT LOGS on any game, installed or not, with a scan report.
- Handhelds and APUs (ROG Ally Z1 Extreme and other Ryzen APUs) are recognised, with a note that AMDNR on them is
  still in testing.
- Linux (experimental, not tested by us yet): the same Windows exe can run under Proton, shows a notice with what
  to do there and also looks for games in your Linux Steam library; steps in "Linux / Proton". Tell us on Discord if
  it works.

Its source is in `Launcher/OpenSource/` of this project's GitHub repository, with its own licence,
`Launcher/OpenSource/LICENSE.txt`. It is **not** covered by this repository's GPL-3.0 `LICENSE`: it is
source-available, all rights reserved, Copyright (c) 2026 3zwr1 (AMDNR). The launcher's manifest is
`Launcher/manifest.json`. See also section 7 of `Licenses/AMDNR_NOTICE.txt`.

## Copyright / License

AMDNR is Copyright (c) 2026 3zwr1 (AMDNR). It is a fork of OptiScaler, distributed under the GPL-3.0
licence in `LICENSE`.

AMDNR's own work carries an additional term under GPL-3.0 section 7(b) (see
`Licenses/AMDNR_NOTICE.txt`): any copy, fork or derivative that uses it must keep its notices and
credit **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

**AMDNR menu copyright.** The AMDNR menu — its layout, design, texts and the code AMDNR added for it — is Copyright (c) 2026 3zwr1 (AMDNR). It is part of this GPL-3.0 fork, with these additional terms (GPL-3.0 section 7): (b) anyone who reuses any part of it must keep this copyright line and credit AMDNR by 3zwr1 visibly, in the menu and the README; (c) you may not present it, or a modified copy, as your own work; modified versions must be clearly marked as changed; (e) no rights are granted to the AMDNR name or logo; other projects may not use them.

The upstream work credited above stays with its authors, under their own licences; AMDNR claims no
copyright over it.

The source code will be published with AMDNR 0.5.0.

## Legal

This build is distributed under the GPL-3.0 licence in `LICENSE`; third-party library licences are
in `Licenses\`. The AMD neural runtime and its weights are redistributed under their original
authorship as credited above, for convenience only, with no ownership claimed and no warranty
offered.

NVIDIA's `nvngx_dlssnr.dll` is not in these archives. None of this is endorsed by, affiliated
with, or supported by NVIDIA, AMD, or any game publisher. It drives an undocumented feature
directly. Use it at your own risk.
