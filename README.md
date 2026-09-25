# AMDNR — DLSS 5 Neural Rendering on AMD (OptiScaler build) — v0.3.2

**English** | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md)

> **We need your support.** Join the Discord server — <[AMDNR](https://discord.gg/AMDNR)> — for
> help, bug reports and test builds; every report with a log makes the next build better.

DLSS 5 Neural Rendering running on AMD GPUs, built into OptiScaler so it works in any
Direct3D 12 game OptiScaler already hooks. On top of the neural pass: model interleave for a
large frame-rate gain, residual composition, XeSS frame generation unlocked up to 6X, and FSR
Ray Regeneration for games that use DLSS Ray Reconstruction.

**Discord: https://discord.gg/AMDNR** — support, bug reports (`#bug-report`), test
builds. If you need NVI's `nvn.dll` for anything, it is available there; it is not
in these archives and the AMD path does not need it.

**Support the project: <https://ko-fi.com/3zinr>**

---

## AMDNR - OptiScaler Installation Guide

Installation is pretty simple.

### 1. Download the files

Download these two files from GitHub:

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
danielblnc's. RDNA 4 only. It needs two things next to the game:

1. `LmxxfNrRuntime.dll` - in this archive, beside `OptiScaler.dll` (it is copied with the rest).
2. `LmxxfNrRuntime.pak` (382 MB, included in the AMDNR zip) beside `LmxxfNrRuntime.dll` - lmxxf's weight
   files, HIP modules and HLSL in one encrypted, authenticated file. The runtime opens it in
   memory; nothing is unpacked to disk. The older folder layout still works instead of the

On the first launch that finds a runtime installed and no choice made, the menu asks which one
to use (`[DlssNr] NrBackend = daniel | lmxxf` in the ini records it; Neural > Neural runtime
changes it, on the next game start). The lmxxf edit is applied one frame late, carried by the
motion vectors, so the frame never waits for the network (about 30 ms at 1080p, 15 ms at 720p
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
| GTA V Enhanced | D3D12, before SR, HDR, one-channel reactive mask | fixed in this build: the mask used to read as "everything reactive" and the edit never landed |
| Any title with Ray Reconstruction | D3D12, after RR (written back into the output) | supported from this build; not yet confirmed in a game |

If a title shows no effect: `lmxxf_backend.log` has an `lmxxf inputs:` line (formats, sizes,
motion scale, depth direction, mask, exposure) and an `lmxxf stats @N:` line every 600 frames
(exposure, fed brightness, the model's edit, the carried edit, keep, reactive mean, vector
length and rejected fraction). Attach the log to a report; those two lines usually say why.

Under lmxxf the Neural runtime block has **Network history** (the model's own temporal input),
and Image look has the **lmxxf edit** group: the edit shaper (Edit detail,
Edit colour, Edge guard: gain on the fine part of the model's edit, its colour against its
brightness change, and a fade of the edit across depth edges) and **Output smoothing**
(upstream's output-side pass, needs Network history). Neural passes, Residual strength/limit,
sharpening, Debug view 1 and the Appearance filter apply under both runtimes. Ini keys: `AmdLmxxfHistory`, `AmdLmxxfEditDetail`,
`AmdLmxxfEditSaturation`, `AmdLmxxfEdgeGuard`, `AmdLmxxfOutputSmooth` under `[DlssNr]`.

## Requirements

- An AMD GPU with a current driver. The neural runtime uses HIP through the driver; no HIP
  SDK and no developer mode are needed. Which chips: RX 9000 (RDNA 4) runs both runtimes;
  RX 7000 (RDNA 3, desktop and mobile) runs danielblnc's only; handheld APUs (Z1 Extreme /
  780M, Z2 Extreme / 890M) and RDNA 2 (RX 6000, Steam Deck) are not supported by either yet.
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
| `LmxxfNrRuntime.dll` | The lmxxf neural runtime (0.3.0). Used only when chosen; reads `LmxxfNrRuntime.pak` beside it, see "The lmxxf runtime". |
| `LmxxfNrRuntime.pak` | The lmxxf runtime's weights, HIP modules and shaders in one encrypted file (382 MB). Only the lmxxf runtime reads it; harmless to keep with the danielblnc runtime. |
| `OptiScaler\` | FSR, XeSS, the FidelityFX denoiser and the D3D12 Agility SDK OptiScaler uses. |
| `Licenses\`, `LICENSE` | Third-party licences and the GPL-3.0 licence of this build. |
| `SHA256SUMS.txt` | Checksums of every shipped file, both archives. |

**Runtime.zip**

| File | What it is |
|---|---|
| `OptiScaler/amdnr_dlssg_fsr3.dll` | Nukem9's dlssg-to-fsr3, unmodified and renamed: the game's DLSS Frame Generation calls served by FSR 3 frame generation, also on Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, see `Licenses/`. |
| `dlssnr_amd_pass1..3.dll` | The AMD neural runtime, danielblnc's v0.3.1, unmodified. Three copies so multi-pass has one per pass. |
| `dlssnr_on_amd_weights.bin` | The network weights the runtime loads. |

## Settings worth knowing

Open the **Neural** tab. The defaults are the most recent tested arrangement, so the useful first
move is to change one thing at a time.

- **NR resolution** — the main quality/cost lever. Below 100% the model works on a smaller
  picture and only its *correction* is carried back up to the full-resolution frame, so the
  frame keeps its own detail. Above 100% cost grows with the square (150% is 2.25x).
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
- **Frame generation is off in a fresh ini.** Frame Gen tab: choose the FG Input (e.g. "DLSSG via
  Streamline" in a game with DLSS frame generation) and FG Output (XeFG), then tick **Active** in
  the Frame Generation (XeFG) section and press Save Settings. A 0.1.0 ini that had it on is not
  carried over when you install 0.2.0's ini.
- **XeFG multi-frame generation** — 3X to 6X is built in and on by default (`XeFG\UnlockMFG`),
  for OptiScaler's copy and the game's own. **Delete `XeFGUnlock.asi`** from `OptiScaler\plugins`
  if you still have it: two copies of the same patch crash the game.
- **FSR Ray Regeneration** — only in games that use DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), with the game running DLSS (spoofing on), ray tracing and Ray Reconstruction
  enabled in its own settings. Neural Rendering then runs after it, on its output, which costs
  more: lower the NR resolution if the frame rate drops.

## If something goes wrong

`OptiScaler.log` appears in the game folder. Attach it in `#bug-report`, and say which game and
which GPU. The AMD backend also writes `amd_presr.log` and `amd_bridge.log`, which are the useful
ones when the neural pass specifically misbehaves.

**NR frames 0/s, the runtime combo reads `pass1?`, and `amd_presr.log` says the pass DLL is a build this
OptiScaler does not drive?** Your `dlssnr_amd_pass1..3.dll` are not danielblnc's 0.3.1 (a 0.2.16 set was
seen in the wild). Use `Runtime.zip` from this release: `dlssnr_amd_pass1.dll` is 7,304,192 bytes, SHA256
starting `b108d640`. Supported builds: 0.2.17, 0.3.0, 0.3.1.

**A Vulkan game (Indiana Jones and the Great Circle) stops at start with "Could not create the Vulkan
device (VK_ERROR_EXTENSION_NOT_PRESENT)"?** Fixed in this build: the inherited NVIDIA neural path asked the
AMD driver for two NVIDIA-only device extensions. Note that the neural pass has no Vulkan path yet - both
AMD runtimes are D3D12 - so Vulkan titles start and run without NR.

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

Full notes for this version: `RELEASE-NOTES.md` in the repository.

## Roadmap

- **0.3.2** (this build) — the 0.3.1 reports: Vulkan titles start and run with lmxxf, lmxxf colours
  matched to danielblnc's (auto-exposure), the runtime combo, Ray Reconstruction status and tuning;
  Nukem9's dlssg-to-fsr3 in the zip for frame generation on Vulkan.
- **0.3.1** — fixes from the first 0.3.0 reports (lmxxf alone never ran, Where Winds

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

- **DLSS-NR on AMD** — *danielblnc* — <https://github.com/danielblnc/DLSS-NR-on-AMD>
  The AMD neural runtime and weights in `Runtime.zip` are his v0.3.1 release, redistributed
  unmodified. Everything the network actually computes is his.
- **DLSS 5 AMD project** — *TheAutomatic* — <https://github.com/TheAutomatic/dlss-5-amd-project>
  Groundwork and reference for DLSS 5 Neural Rendering on AMD hardware; the lmxxf runtime
  integration shipped in 0.3.0 follows his work. Many thanks.
- **lmxxf / dlss5-on-amd-9070xt-porting** — <https://github.com/lmxxf/dlss5-on-amd-9070xt-porting>
  Open-source HIP neural rendering runtime (MIT); the difference-gated temporal mode here follows
  his `native_output_smooth`.
- **OptiScaler** — *Overclockers* and contributors — <https://github.com/Overclockers/OptiScaler-Releases>
  The framework this is built into: the hooking, the FSR/XeSS/frame-generation plumbing, the
  menu, and the game compatibility that makes any of it reachable.

Code lineage: OptiScaler → Dagherbou / OptiScaler_DLSSNR → wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass
→ Matheus / dlss-5-amd → this build. The XeFG unlock and pacing are ported from Coldwood1026's
XeFGUnlock (GPL-3.0); the `CubeScale` gamut handling is *hhkbble*'s.

## Legal

This build is distributed under the GPL-3.0 licence in `LICENSE`; third-party library licences are
in `Licenses\`. The AMD neural runtime and its weights are redistributed under their original
authorship as credited above, for convenience only, with no ownership claimed and no warranty
offered.

.
