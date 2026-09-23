# Changelog

Many thanks to **TheAutomatic** (DLSS 5 AMD project) — the releases, the HIP toolchain and the asset
layout that the lmxxf runtime integration in 0.3.0 builds on.

## 0.3.2 — 2026-09-23

What you reported on 0.3.1 in its first hours, plus frame generation for Vulkan titles.

### Added
- **Frame generation for Vulkan titles: Nukem9's dlssg-to-fsr3 ships in the zip** as
  `OptiScaler\amdnr_dlssg_fsr3.dll` (the release binary, unmodified, renamed; GPLv3 like
  OptiScaler - see `Licenses\dlssg-to-fsr3_ATTRIBUTION.txt`). OptiScaler's own frame generation
  (OptiFG, FSR-FG and XeFG outputs) is D3D12-only; on a Vulkan title (Indiana Jones and the Great
  Circle) set `FGInput=DLSSG`, `FGOutput=DLSSG`, `FGNvngxReplacement=Nukems` with `Dxgi=true`, and
  the game's own DLSS Frame Generation option runs FSR 3 frame generation. A
  `dlssg_to_fsr3_amd_is_better.dll` you already have is still honoured.
- **lmxxf: auto-exposure when the game publishes no exposure texture** (`[DlssNr]
  AmdLmxxfAutoExposure`, default on; Neural > Image look > Appearance > lmxxf edit). danielblnc's
  runtime never feeds the network the colour as is: it moves an exposure until the encoded
  picture's mean sits near 0.5, in every game. lmxxf fed the colour as is, so the network ran
  off its operating point and its answer drifted in tone and colour - the "colour difference
  between daniel and lmxxf" (the community workaround, Edit colour 0, only threw the drifted
  chroma away). The same loop runs for lmxxf now, on the GPU; the edit is divided back, so the
  game's own tone is untouched. It holds when the fed picture is already within a quarter of the
  target (Forza, Stray keep their look) and corrects outside it. It also takes over when the
  game's own exposure is refused - and that refusal no longer fires on a black loading screen
  (RE Requiem log: muted on a black frame at start, then fed 13x too bright for the whole
  session; that was the report's colour drift). Off = the 0.3.0 behaviour.
- **Neural tab, Live: a title whose Ray Reconstruction FSR Ray Regeneration gave up on is named,
  with the reason** (Satisfactory: the game publishes no camera matrices). The game shows RR as
  on; this line says FSR upscaling runs in its place and NR keeps its pre-SR position.
- **FSR Ray Regeneration tuning in the Neural tab**, live: "AMD's default tuning" (AMD's own
  defaults against the fork's tuned set) and the disocclusion threshold - the knob that decides how
  fast history is dropped behind a moving face. For the skin / hair ghosting and flicker reports
  under path tracing + RR (RE Requiem). ini section `[FSR-RR]` documented.

### Fixed
- **Vulkan titles (Indiana Jones and the Great Circle): "Could not create the Vulkan device
  (VK_ERROR_EXTENSION_NOT_PRESENT)" before the first frame.** The inherited NVIDIA neural path
  appended two NVIDIA-only device extensions at `vkCreateDevice` whenever Neural Rendering was on;
  with Vulkan extension spoofing the driver's list appeared to contain them, AMD's driver does
  not. Nothing is appended on a non-NVIDIA GPU any more. The neural pass has no Vulkan path in
  this build (both AMD runtimes are D3D12): Vulkan titles start and run, without NR.
- **Switching the Neural runtime from the menu did not stick.** The combo needed Save Settings,
  and an lmxxf chosen with its files incomplete silently ran danielblnc's runtime. The choice is
  written to the ini the moment it is made (as the first-launch chooser does), the menu says when
  lmxxf is chosen but incomplete and what is missing, and `amd_bridge.log` says so too. Under
  danielblnc the row now says that lmxxf is installed and that the list above switches to it.
- **Vulkan titles with the lmxxf runtime chosen froze right after the first model frame**
  (Indiana Jones and the Great Circle, over OptiScaler's Vulkan-on-D3D12 bridge:
  `lmxxf_backend.log` ends at "first frame prepared"). The bridge executes its D3D12 list inside
  the game's vkQueueSubmit and waits on the CPU for the previous frame's D3D12 work; a queue-side
  wait on the HIP fence inside that timeline never resolves. On a Vulkan title the answer is now
  waited for on the CPU instead (bounded; the frame waits for the network, as under danielblnc's
  inline mode - Model interleave 2 halves the cost), the queue never waits, and if the answer does
  not come within a second the pass stops with a line in `lmxxf_backend.log` instead of a hang.


### Notes
- Where Winds Meet: DLSS spoofing does not make DLSS / DLSS Frame Generation appear. The log shows
  DXGI spoofing and the built-in nvapi active and the game never loading Streamline: the decision is
  taken before OptiScaler is asked (launcher / saved vendor). Multi-frame generation there goes
  through the game's own XeSS Frame Generation instead: `[XeFG] UnlockUnverifiedBuilds=true`
  patches its `libxess_fg.dll` 1.3.1.68 (a table derived offline, not yet confirmed in a game).
- The Last of Us Part I freeze report (danielblnc runtime, FSR-FG input -> XeFG 4X): no error in any
  log; the pass completed its last frame normally. Isolation asked for (NR off, then FG off, then a
  Task Manager dump of the frozen process).
- Frame generation on Vulkan titles (Indiana Jones): OptiFG, FSR-FG output and XeFG output are
  D3D12-only (the Frame Gen tab greys them out as "Unsupported API"); set in the ini they give a
  black screen with sound. On Vulkan use the game's own frame generation, or the shipped Nukem9
  DLL (Added above).


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
- **Where Winds Meet (danielblnc runtime): NR did nothing in 0.3.0.** Our own
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
- **Skyrim SE (Community Shaders D3D11-on-12 bridge): NR never activated - `Private AMD runtime hash
  mismatch: pass 1` on every launch.** The reporter's pass DLLs were danielblnc's 0.2.16, a build this
  host has no layout for, and nothing said so (the menu read `pass1?`, the status line blamed the
  upscaler's parameters). The log and the status line now name the build found, its size and digest,
  the supported builds (0.2.17, 0.3.0, 0.3.1) and what to install. Check the `Runtime.zip` you
  downloaded: `dlssnr_amd_pass1.dll` must be 7,304,192 bytes (SHA256 `b108d640...`).

### Notes
- Final image mode (games with FSR 1 or no upscaler at all) stays off in this release. The code
  carries the work in progress - a no-stall consume and a block-matching carry for the lmxxf
  runtime - behind a compile-time switch, for the no-upscaler titles of 0.4.0.
- The 0.3.0 zip shipped lmxxf alone, and lmxxf alone never ran (first fix above). If 0.3.0 did
  nothing for you, this is why.

## 0.3.0 — 2026-09-23

### Added
- **lmxxf neural runtime (HIP, RDNA 4)** as a second selectable runtime beside danielblnc's:
  built in-tree as `LmxxfNrRuntime.dll`, chosen on first launch or under Neural > Neural runtime
  (`[DlssNr] NrBackend = daniel | lmxxf`). Its edit is applied one frame late through the temporal
  carry, so no frame waits for the network.
- **`LmxxfNrRuntime.pak`**: the lmxxf weights, HIP modules and HLSL in one encrypted, authenticated
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
