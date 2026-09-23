# Changelog

Many thanks to **TheAutomatic** — the releases, the HIP toolchain and the asset
layout that the lmxxf runtime integration in 0.3.0 builds on.

## 0.3.0 — 2026-09-23

### Added
- **lmxxf neural runtime (HIP, RDNA 4)** as a second selectable runtime beside danielblnc's:
  built in-tree as `LmxxfNrRuntime.dll`, chosen on first launch or under Neural > Neural runtime
  (`[DlssNr] NrBackend = daniel | lmxxf`). Its edit is applied one frame late through the temporal
  carry, so no frame waits for the network.
- **`LmxxfNrRuntime.pak`**: the lmxxf weights, HIP modules and HLSL in one encrypted, authenticated
  file (382 MB) beside the runtime DLL, shipped inside the AMDNR zip; decrypted in memory only.
  folder still works and takes precedence when present.
- **Network history** (the model's own temporal input) with a two-frame vector chain under Model
  interleave, upstream's history guard, and **Output smoothing** (``).
- **Real Neural passes** (2 and 3 launches per frame on the HIP stream); the history loop stays the
  first pass, so passes do not compound.
- **Edit shaper** (lmxxf, under Image look): Edit detail (``), Edit colour
  (``), Edge guard (``).
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
  
### Changed
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
- Edge garbage and blur from sampling over Unreal's padded colour allocation (pixel-exact blits).
- HDR / exposure handling for lmxxf (fed copy times the NGX exposure, edit divided back).
- danielblnc path: the silent early return now logs why (`AMD idle: ...`).

### Known
- RX 7000,RX9000 (RDNA 3, RDNA4) runs danielblnc's runtime only; handheld APUs (Z1 Extreme, Z2) and RX 6000 are
  not supported by either runtime yet.
- lmxxf after RR and in Vulkan titles: implemented / bridged, not yet confirmed in a game.
- Open reports awaiting logs: TLOU Part I crash, Where Winds Meet "model idle", RE Requiem RR noise.

## 0.2.1 — 2026-09-21


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
