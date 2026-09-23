# Changelog

Many thanks to **TheAutomatic** (DLSS 5 AMD project) — the releases, the HIP toolchain and the asset
layout that the lmxxf runtime integration in 0.3.0 builds on.

## 0.3.1 — 2026-09-23

Fixes from the first 0.3.0 reports, plus one request.

### Added
- **NR style presets** (Neural tab, under Preset): Default, Cinematic, Crisp, Natural, Vivid set the
  look controls together (Residual strength and limit, Temporal stability, Sharpening, Detail and
  Colour strength, Tone and Structure intensity, lmxxf's Edit detail / Edit colour / Edge guard /
  Output smoothing), and **three custom style slots** keep your own (`[DlssNr] StyleSlot1..3`,
  written by Save Settings). The combo reads Custom when the sliders match no style.

### Fixed
- **lmxxf installed on its own never ran (any game).** The pass entry opened the AMD path only
  when danielblnc's `dlssnr_amd_pass1.dll` was present, and the main archive ships lmxxf
  without it, so an lmxxf-only install fell through to the NVIDIA path and the log ended in
  `nvngx.dll_dlssnr.dll is missing` while the menu stayed at "waiting for a DirectX 12 SR frame"
  (reported in Neverness to Everness, RX 9070 XT). Either runtime's files open the path now,
  lmxxf runs by itself when it is the only runtime installed (the chooser only asks when both
  are), and an AMD card with no runtime files logs the folder it searched.
- **Where Winds Meet: NR did nothing in 0.3.0.** Our own
  shaders were compiled with whatever `d3dcompiler_47.dll` the game keeps beside its exe (the
  import binds to the game folder first); an old compiler fails on the temporal pass's shader as
  it is since 0.2.x (`AMD temporal D3D12 error 2147500037`), and that latched the whole backend off.
  Every AMD-path shader now compiles with the system's own compiler (the runtime module already
  did), the temporal pass is optional if anything still fails, and `amd_bridge.log` names the
  compiler in use and any copy the game carries.
- **Crash on a DLSS quality change with the lmxxf runtime** (GTA V Enhanced / RDR1 reports): the
  temporal pass reallocated its textures while the GPU could still be reading them. The queue is
  drained once per resolution change before the reallocation.
- **Satisfactory (UE5) with DLSS Ray Reconstruction on: NR did nothing.** The game creates a Ray
  Reconstruction feature but publishes none of RR's inputs (no camera matrices, no guides), so
  FSR-RR gives up after 30 frames and the handle runs plain FSR - and the neural pass kept
  treating it as RR (post-RR placement behind the ApplyAfterRR gate), i.e. it never ran. Such a
  handle is a Super Resolution handle for the neural pass now: pre-SR placement, as in any SR
  title.

### Notes
- The 0.3.0 zip shipped lmxxf alone, and lmxxf alone never ran (first fix above). If 0.3.0 did
  nothing for you, this is why.

## 0.3.0 — 2026-09-23

### Added
- **lmxxf neural runtime (HIP, RDNA 4)** as a second selectable runtime beside danielblnc's:
  built in-tree as `LmxxfNrRuntime.dll`, chosen on first launch or under Neural > Neural runtime
  (`[DlssNr] NrBackend = daniel | lmxxf`). Its edit is applied one frame late through the temporal
  carry, so no frame waits for the network.
- **`LmxxfNrRuntime.pak`**: the lmxxf weights, HIP modules and HLSL in one encrypted, authenticated
  file (382 MB) beside the runtime DLL, shipped inside the AMDNR zip.
- **Network history** (the model's own temporal input) with a two-frame vector chain under Model
  interleave, upstream's history guard, and **Output smoothing** (`AmdLmxxfOutputSmooth`).
- **Real Neural passes** (2 and 3 launches per frame on the HIP stream); the history loop stays the
  first pass, so passes do not compound.
- **Edit shaper** (lmxxf, under Image look): Edit detail (`AmdLmxxfEditDetail`), Edit colour
  (`AmdLmxxfEditSaturation`), Edge guard (`AmdLmxxfEdgeGuard`).
- **Appearance filter** under lmxxf (the same shader as the danielblnc path).
- **After Ray Reconstruction placement** for lmxxf: the result is written back into the RR output
  (depth resampled to the display grid, jitter ignored). Not yet confirmed in a shipping title.
- **Diagnostics**: `lmxxf inputs:` line once per allocation; `lmxxf stats @N:` line every 600 frames
  (exposure and raw exposure, fed brightness, fresh and carried edit, keep, reactive mean, vector
  length and rejected fraction, model / fresh / skip / reset / history counters).
- **Self-healing** (lmxxf): vectors mostly rejected → carry without reprojection; depth test alone
  refusing the carry → depth test off; exposure blacking out or blowing out the feed → exposure 1.
  Each decision once per session, logged, shown in the Live line.
- **Debug view 5 — Direct edit** (lmxxf): the model's fresh edit added with no reprojection.
- **GPU support readout** in the Neural tab and the runtime chooser (which runtime each chip runs).
- **Toggle hotkey**: `HOME` switches Neural Rendering on and off in-game for both runtimes, with an
  On / Off notice; rebind it beside the Enable checkbox (Neural tab) or under Interface > Keybinds
  (`[DlssNr] ToggleKey`). The key now also fires in titles whose input arrives through the window
  procedure, and toggling restarts the neural history.
- **Compatibility table** for lmxxf in the README, with what to attach to a report.
- Tools: `lmxxf_pak` (pack / list / verify / extract), `lmxxf_gpu_probe`.

### Changed
- Product name: `AMD-NR v0.3.0 / OptiScaler v11.0.0`.
- Neural passes moved under NR resolution (both runtimes); the four lmxxf sliders sit directly under
  Image look; the lmxxf "what applies here" header removed.
- NR resolution above 100% allowed again under lmxxf (the network is fed an upscaled copy fitted
  into 1920x1080).
- The `hip_ms` readout is measured on the D3D12 fence (real numbers with history on).
- Menu texts no longer describe runtimes as "open source".

### Fixed
- **Reactive / bias mask read as "everything reactive"** when a title publishes a one-channel mask
  (Direct3D returns alpha = 1 for the missing component): keep fell to 0 over the whole frame and no
  edit landed. GTA V Enhanced showed no effect because of this. Fixes the danielblnc path's temporal
  pass in the same titles. The mask is now read by the channels its format has.
- **Network history never engaged under Model interleave** (the chain rule wanted an answer from
  the same frame); it engages now in every game with `AmdInterleave = 2`.
- **Crash on every NR-resolution change** (lmxxf): the job in flight is abandoned, not consumed.
- **NR % slider snapping back to its old value** while dragging.
- **Neural passes 2 / 3 ghosting**: the multi-pass answer was fed back as the network's history and
  compounded every frame; the history is the first pass now.
- **Reset flag held up by the game** erased the edit every frame; ignored after 9 frames in a row.
- **Ubisoft Anvil titles (AC Black Flag Resynced): `DX12 Error 0x80070057` at launch.** The game
  loads its own `libxess_fg.dll` (native XeSS Frame Generation) before OptiScaler asks for one;
  with XeFG selected as FG output (also silently by `ForceXeLL`) a second FG swapchain was wrapped
  around the same window, and the built-in MFG unlock patched the game's 1.3.1.68 provider from a
  table derived offline. Now: a native XeSS FG title keeps its own frame generation (OptiScaler's
  XeFG output stands down, the Frame Gen tab says why; `[FrameGen] AllowXeFGWithNativeXeFG`
  overrides), and the game's own provider copy is only patched from a table confirmed in a game
  (`[XeFG] UnlockUnverifiedBuilds` overrides). OptiScaler's own 1.3.1.78 copy is unaffected.
- Edge garbage and blur from sampling over Unreal's padded colour allocation (pixel-exact blits).
- HDR / exposure handling for lmxxf (fed copy times the NGX exposure, edit divided back).
- danielblnc path: the silent early return now logs why (`AMD idle: ...`).

### Known
- RX 7000 (RDNA 3) runs danielblnc's runtime only; handheld APUs (Z1 Extreme, Z2) and RX 6000 are
  not supported by either runtime yet.
- lmxxf after RR and in Vulkan titles: implemented / bridged, not yet confirmed in a game.
- Open reports awaiting logs: TLOU Part I crash, Where Winds Meet "model idle", RE Requiem RR noise.

## 0.2.1 — 2026-09-21
- Trace build for testers (diagnostic logging); no functional change over 0.2.0.

## 0.2.0 — 2026-09-21
- Ghosting fixed at the source (reactive mask, velocity dilation, clamped residual upsample, bounded
  carried edit); Guided fill v2 as the Model interleave default; XeSS frame generation up to 6X built
  in; FSR Ray Regeneration (experimental); highlight proxy (experimental); TLOU depth-format device
  removal fixed; NR cost readout; D3D11 backend hint. See `RELEASE-NOTES.md`.

## 0.1.0 — 2026-09-20
- First public build: DLSS 5 Neural Rendering on AMD through OptiScaler with danielblnc's runtime,
  model interleave, residual composition.

---

Credits: TheAutomatic (DLSS 5 AMD project) · danielblnc (DLSS-NR on AMD) · lmxxf
(dlss5-on-amd-9070xt-porting) · Matheus (dlss-5-amd) · OptiScaler (Overclockers). Licence GPL-3.0;
third-party licences in `Licenses\`.
