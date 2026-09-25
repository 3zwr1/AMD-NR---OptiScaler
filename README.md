# AMDNR — DLSS 5 Neural Rendering on AMD (OptiScaler build) — v0.3.3.1

**English** | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md)

> **We need your support.** Join the Discord server — <https://discord.gg/AMDNR> — for
> help, bug reports and test builds; every report with a log makes the next build better.

DLSS 5 Neural Rendering running on AMD GPUs, built into OptiScaler so it works in any
Direct3D 12 game OptiScaler already hooks. On top of the neural pass: model interleave for a
large frame-rate gain, residual composition, XeSS frame generation unlocked up to 6X (up to 10X
opt-in in D3D12 games), and FSR Ray Regeneration for games that use DLSS Ray Reconstruction.

**Discord: <https://discord.gg/AMDNR>** — support, bug reports (`#bug-report`), test
builds.

**Support the project: <https://ko-fi.com/3zinr>**

> **The danielblnc runtime is Daniel Blanco's work.** The AMD neural runtime in `Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) is **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR ships it unmodified, with his permission; it is not AMDNR's work. Please support his project.
> Full credits for everyone else are at the end of this page.

---

## AMDNR - OptiScaler Installation Guide

Installation is pretty simple.

### 1. Download the files

Download these two files from GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>):

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. Extract both files

Extract the contents of both `.zip` files.

### 3. Copy everything to the game folder

First, copy all files from `AMDNR-vX.X.X` into the game's root folder — the same folder where
the game's `.exe` is located.

Then, do the same with all files from `Runtime`.

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

Test one name at a time. Do not create multiple copies of `OptiScaler.dll`.

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

> The `.exe` is usually not where the shortcut points. Unreal games keep it under
> `<Game>\Binaries\Win64\`.

---

### The lmxxf runtime (0.3.0, optional)

A second neural runtime (MIT-licensed, by lmxxf) can carry the pass instead of
danielblnc's. RDNA 4, and RDNA 3 (RX 7000, Strix Halo) through AMDNR's RDNA 3 backend - slower there:
start at NR resolution 67%. It needs two things next to the game:

1. `LmxxfNrRuntime.dll` - in this archive, beside `OptiScaler.dll` (it is copied with the rest).
2. `LmxxfNrRuntime.pak` (416 MB, included in the AMDNR zip) beside `LmxxfNrRuntime.dll` - lmxxf's weight
   files, HIP modules and HLSL in one encrypted, authenticated file. The runtime opens it in
   memory; nothing is unpacked to disk.

On the first launch that finds a runtime installed and no choice made, the menu asks which one
to use (`[DlssNr] NrBackend = daniel | lmxxf` in the ini records it; Neural > Neural runtime
changes it, on the next game start). The lmxxf edit is applied one frame late, carried by the
motion vectors, so the frame never waits for the network (about 17 ms at 1080p
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

Under lmxxf the Neural runtime block has **Network history** (the model's own temporal input),
and Image look has the **lmxxf edit** group: the edit shaper (Edit detail,
Edit colour, Edge guard: gain on the fine part of the model's edit, its colour against its
brightness change, and a fade of the edit across depth edges) and **Output smoothing**
(upstream's output-side pass, needs Network history). Neural passes, Residual strength/limit,
sharpening, Debug view 1 and the Appearance filter apply under both runtimes.

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, lmxxf only) runs all 71 of the
network's blocks instead of skipping 42, 43 and 46: slightly more faithful, about 0.5 ms slower at 1080p
(16.6 -> 17.1 ms on an RX 9070 XT). Off by default.

## Requirements

- An AMD GPU with a current driver. The neural runtime uses HIP through the driver; no HIP
  SDK and no developer mode are needed. Which chips: RX 9000 (RDNA 4) runs both runtimes;
  RX 7000 (RDNA 3, desktop and mobile) runs both too - lmxxf through AMDNR's RDNA 3 backend, slower
  than on RDNA 4; Strix Halo (8060S / 8050S) runs lmxxf; handheld APUs (Z1 Extreme / 780M, Z2 Extreme /
  890M) and RDNA 2 (RX 6000, Steam Deck) are not supported by either.
  The Neural tab says what your GPU can run.
- A Direct3D 12, Direct3D 11 or Vulkan game. The AMD neural path itself is D3D12; D3D11 and
  Vulkan titles reach it through OptiScaler's D3D12 bridge, which means the upscaler must be
  one of the "w/Dx12" backends (`ffx_12`). Leave `Dx11Upscaler` / `VulkanUpscaler` on `auto`
  and this build picks it for you when neural rendering is on.
- About 2 GB of spare VRAM at 1080p-class render resolutions.

## What is in the two archives

**AMDNR-vX.X.X.zip**

| File | What it is |
|---|---|
| `OptiScaler.dll` | OptiScaler with the DLSS-NR AMD backend. Rename it as the guide says. |
| `OptiScaler.ini` | Settings. Neural Rendering is enabled; logging is on so a bug report has something to attach. |
| `LmxxfNrRuntime.dll` | The lmxxf neural runtime (lmxxf 0.29 kernels). Used only when chosen; reads `LmxxfNrRuntime.pak` beside it, see "The lmxxf runtime". |
| `LmxxfNrRuntime.pak` | The lmxxf runtime's weights, HIP modules and shaders in one encrypted file (416 MB). Only the lmxxf runtime reads it; harmless to keep with the danielblnc runtime. |
| `OptiScaler\` | FSR, XeSS, the FidelityFX denoiser and the D3D12 Agility SDK OptiScaler uses. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | Nukem9's dlssg-to-fsr3, unmodified and renamed: the game's DLSS Frame Generation calls served by FSR 3 frame generation, also on Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, see `Licenses/`. |
| `Licenses\`, `LICENSE` | Third-party licences, AMDNR's notice (`AMDNR_NOTICE.txt`) and the GPL-3.0 licence of this build. |
| `SHA256SUMS.txt` | Checksums of every shipped file, both archives. |

**Runtime.zip**

| File | What it is |
|---|---|
| `dlssnr_amd_pass1..3.dll` | The AMD neural runtime, danielblnc's v0.3.1, unmodified. Three copies so multi-pass has one per pass. |
| `dlssnr_on_amd_weights.bin` | The network weights the runtime loads. |

## Settings worth knowing

Open the **Neural** tab. The defaults are the most recent tested arrangement, so the useful first
move is to change one thing at a time.

- **NR resolution** — the main quality/cost lever. Below 100% the model works on a smaller
  picture and only its *correction* is carried back up to the full-resolution frame, so the
  frame keeps its own detail. Above 100% cost grows with the square (150% is 2.25x). The slider
  moves in 5% steps: each new NR size can keep VRAM until the game restarts, so restart the game
  after many changes.
- **Residual strength** — how much of the model's edit is applied; above 1 it amplifies. This is
  the control that changes the picture most.
- **Residual limit** — a ceiling on how far one pixel may move. Blotchy patches: **lower** it.
- **Model interleave** — runs the model every second frame for a large frame-rate gain. The
  skipped frames are filled by the **Interleave preset**; *Guided fill v2* is the default and
  the one under active work. Pacing of the two frame types is automatic, and **Adaptive
  interleave** (on by default) runs the model on every frame while the picture is moving, so
  the skips - and their artefacts - only happen while the picture stands still.
- **Neural passes** — 2 and 3 stack the model, with diminishing returns. Under lmxxf the
  network's history stays its first pass; the extra passes are spatial refinement only.
  danielblnc runs 1 pass on Vulkan titles (a note under the slider says so).
- **Colour composition** (Neural > Image look, both runtimes) — *Classic* (default) is the picture
  you had before. *RenoDX (experimental)* runs RenoDX's colour composition after the model, as the
  NVIDIA path does: Composition detail and colour, a two-sided **Highlight guard** (2x by default)
  that bounds the model's answer against the original, and optional skin / environment controls.
  On a display-referred (SDR) frame it falls back to Classic, with a note in the menu. NR styles
  and presets leave it alone.
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
- **FSR Ray Regeneration** — only in games that use DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), with the game running DLSS (spoofing on), ray tracing and Ray Reconstruction
  enabled in its own settings. Neural Rendering then runs after it, on its output, which costs
  more: lower the NR resolution if the frame rate drops. Its controls (Neural > Quality > Ray
  Regeneration) appear only while the game is running Ray Reconstruction. Resident Evil Requiem and
  PRAGMATA get the **path-traced profile** automatically (less grain on faces under path tracing).
  The same place has the bias mask strength, an RR debug view and **skin smoothing**
  (experimental, off by default; for games that publish an SSS guide, such as Resident Evil Requiem).

## If something goes wrong

`OptiScaler.log` appears in the game folder. Attach it in `#bug-report`, and say which game and
which GPU. The AMD backend also writes `amd_presr.log` and `amd_bridge.log`, which are the useful
ones when the neural pass specifically misbehaves. The previous session's log is kept as
`OptiScaler.previous.<exe>.log`; after a crash, attach it too (the new log then says "no clean exit
recorded").

**NR frames 0/s, the runtime combo reads `pass1?`, and `amd_presr.log` says the pass DLL is a build this
OptiScaler does not drive?** Your `dlssnr_amd_pass1..3.dll` are not danielblnc's 0.3.1 (a 0.2.16 set was
seen in the wild). Use `Runtime.zip` from this release: `dlssnr_amd_pass1.dll` is 7,304,192 bytes, SHA256
starting `b108d640`. Supported builds: 0.2.17, 0.3.0, 0.3.1.

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
fps). What remains: each NR resolution or DLSS mode change keeps about 97 MB of VRAM and as much RAM
under lmxxf at the 1080 tier (an AMD driver leak; the fix is planned for 0.3.4), and danielblnc keeps VRAM for each new
NR size above about 1 MP. Restart the game after many changes.

**A Streamline game fails at start with slInit error 0x18 (seen with NBA 2K27 on AMD)?** 0.3.3 closes
one way OptiScaler's Streamline plugin hooks could cause it, but that is not confirmed as NBA 2K27's
cause. `OptiScaler.log` now records `slInit returned ...` and `[SLINIT]` lines: send the log with the
report.

**The game's Ray Reconstruction is on but the Neural tab says "Ray Regeneration is off in this title"?**
The game does not publish what FSR Ray Regeneration needs (Satisfactory: no camera matrices). FSR
upscaling runs in its place and NR takes its normal pre-SR position; nothing in the ini changes this.

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

- **0.3.3** (this build) — lmxxf on RDNA 3 (RX 7000; AMDNR's own backend); RenoDX colour
  composition (experimental, opt-in) on both runtimes; lmxxf: Full network option, the RAM leak
  fixed, Vulkan titles fixed (lazy weight upload inside the Vulkan bridge), 0.29 kernels (bit-exact,
  faster); danielblnc on Vulkan titles: 1 Neural pass, clearer messages, an opt-in late copy wait;
  XeFG up to 10X (opt-in, D3D12); Streamline start-up hardening and diagnostics; FSR Ray
  Regeneration path-traced profile and skin smoothing; UE5 robustness.
- **0.3.2** — the 0.3.1 reports: Vulkan titles start and run with lmxxf, lmxxf colours
  matched to danielblnc's (auto-exposure), the runtime combo, Ray Reconstruction status and tuning;
  Nukem9's dlssg-to-fsr3 in the zip for frame generation on Vulkan.
- **0.3.1** — fixes from the first 0.3.0 reports (lmxxf alone never ran, Where Winds
  Meet's silent NR, the crash on a DLSS-quality change) and NR style presets with
  Meet's silent NR, the crash on a DLSS-quality change) and NR style presets with
  three custom slots.
- **0.3.0** — the **lmxxf** HIP neural runtime (RDNA 4) as a selectable runtime
  beside danielblnc's, shipped as `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: network history,
  real Neural passes, the edit shaper, the after-Ray-Regeneration placement, per-title
  diagnostics and self-healing. Many thanks to TheAutomatic, whose DLSS 5 AMD project work
  this integration builds on.
- **0.4.0** — the AMDNR Launcher (one-click install of the runtimes and the pak, updates) and
  support for titles without an upscaler of their own (Stray-class), where OptiScaler supplies
  the upscaler and the neural pass together.

---

## Credits

This build is a wiring job over other people's work. If you find it useful, the thanks belong
upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`, unmodified)
- **lmxxf** — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (the HIP runtime, MIT)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, unmodified)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (colour composition maths, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), the base of the built-in XeFG multi-frame unlock and its pacing
- **burak113** — the FSR Ray Regeneration preprocessor (OptiScaler branch ffx-denoise-experimental, GPL-3.0)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## Copyright / License

AMDNR is Copyright (c) 2026 3zwr1 (AMDNR). It is a fork of OptiScaler, distributed under the GPL-3.0
licence in `LICENSE`.

AMDNR's own work carries an additional term under GPL-3.0 section 7(b) (see
`Licenses/AMDNR_NOTICE.txt`): any copy, fork or derivative that uses it must keep its notices and
credit **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

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
