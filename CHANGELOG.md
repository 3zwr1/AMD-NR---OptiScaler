# Changelog

Many thanks to **TheAutomatic** (DLSS 5 AMD project) — the releases, the HIP toolchain and the asset
layout that the lmxxf runtime integration in 0.3.0 builds on.

## 0.3.3.2 — 2026-09-26

Hotfix: lmxxf now runs on PCs with more than one AMD GPU (for example a Ryzen with its integrated graphics on) and is
faster on RX 9070 / 9070 XT (network 17.8 -> 15.3 ms at 1080p, same image), Ray Reconstruction is offered on RX 9000
(RDNA 4) only by default so RX 7000 keeps the game's own denoiser, danielblnc's Residual strength no longer jumps at
100% NR resolution or away from it, lmxxf no longer keeps about 100 MB of VRAM on every NR resolution change, Model
interleave no longer ghosts (new default: Edit accumulation), and Resident Evil Requiem warns when REFramework is
missing and gets a skin mask that leaves streets and walls alone. Replace `OptiScaler.dll`, `LmxxfNrRuntime.dll` and
`LmxxfNrRuntime.pak`: all three changed.

### Fixed
- **lmxxf did nothing, or stopped at once, on PCs where the game's GPU is not HIP device 0.** This hits a Ryzen
  desktop with its integrated graphics on, a laptop with an AMD APU and a Radeon, and PCs with two AMD GPUs, on
  RX 7000 and RX 9000 alike (reported on an RX 7900 XTX). HIP keeps its current GPU per thread. lmxxf built its
  network on the right GPU, but the game's submit thread still pointed at device 0, the integrated GPU. The first
  frame failed with `hipErrorInvalidHandle (400)`, and lmxxf stayed off for the whole session ("session is
  poisoned" in `lmxxf_backend.log`). `LmxxfNrRuntime.dll` 0.3.3.2 now selects the network's GPU on every call and
  gives the thread its old GPU back afterwards. On our single-GPU PC its output is identical to 0.3.3.1's, also
  when frames are sent from a second thread. Not yet tested on a PC with two AMD GPUs.
- **Ray Reconstruction on RX 7000 (RDNA 3) and older: no longer offered by default.** AMD ships Ray Regeneration
  for RDNA 4 only; the driver refused it on an RX 7800 XT in Resident Evil Requiem, and the game, having switched
  its own denoiser off for Ray Reconstruction, then showed an undenoised picture. AMDNR now answers "Ray
  Reconstruction supported" on RDNA 4 (RX 9000) only, so on other cards the game keeps its own denoiser.
  `[FSR-RR] FfxDenoiserAllowPreRdna4=true` offers it anyway; if the driver refuses it, AMDNR falls back to FSR 4
  INT8 / FSR 3.1 (not FSR 2.1.2), warns once and stops retrying for the session, and the ray-traced lighting is not
  denoised. Not yet tested in a game on RX 7000.
- **A Ray Reconstruction fallback was saved as your upscaler.** After that fallback, the next Save Settings (or a
  Neural runtime change) wrote `Dx12Upscaler=fsr21` into `OptiScaler.ini`, so every later start ran FSR 2.1.2,
  even on cards with FSR 4. That no longer happens. An ini that already says `Dx12Upscaler=fsr21` is not changed:
  if you did not choose FSR 2.1.2 yourself, set it to `auto` under `[Upscalers]`.
- **danielblnc: Residual strength jumped at 100% NR resolution, and the style changed when NR resolution left
  100%.** Moving strength from 1.00 to 0.99 switched on a hidden Residual limit over the whole look (RenoDX
  composition, look, stability and sharpening), so one step changed the style. Every NR style other than Default
  did the same. At 100% NR resolution the limit and the edge fade are now off: 0.99 gives 99% of 1.00, and the NR
  styles look the way their strength says. Below and above 100% NR resolution (also Dynamic NR steps and the
  Balanced / Performance presets) Residual strength and limit now act only on the model's own edit, before the
  look, temporal stability and sharpening, as lmxxf always has, so changing NR resolution no longer clips the look.
  With the default limit the model's edit stays capped there (not at 100%). Default is unchanged at 100%, bit for
  bit. lmxxf was not affected. `[DlssNr] AmdEditShaper=false` brings back the first 0.3.3.2 behaviour away from
  100%. Not yet tested in a game.
- **Dynamic NR resolution bounced between two steps.** It now steps down after 1.5 s over the target and back up
  only after 5 s of headroom, and waits a minute after a step up that could not hold. Its frame timer was too
  coarse above about 64 FPS and could push it to 50% at high targets; it now uses a precise timer, so at high
  targets you may see a higher NR resolution and a lower frame rate than before. With an older
  `LmxxfNrRuntime.dll` (no buffer reuse, below) it stops changing after 12 changes per session. Not yet tested in a
  game.
- **lmxxf: each NR resolution, DLSS mode or resolution change kept about 100 MB of VRAM and as much RAM until the
  game restarted.** An AMD HIP driver never gives back a D3D12 buffer that was imported and mapped, so
  `LmxxfNrRuntime.dll` now makes these buffers once per network size and reuses them (`lmxxf_backend.log`:
  `hip buffers ... imports N, reuses M`). A smaller rest of about 10-25 MB of VRAM per change remains (the first
  0.3.3.2 runtime has it too; its cause is not known yet), so after very many changes a restart still helps. With an
  older `LmxxfNrRuntime.dll` the menu says so. Verified with the leak probe (40 changes: about +0.4 GB instead of
  +1.8 GB); not yet tested in a game.
- **FPS limit with frame generation above 2X.** OptiScaler's own frame limiter (used when Reflex / Anti-Lag / XeLL
  does not limit) assumed 2X, so XeFG 4X with a 120 FPS limit ran at 240. The limit is the displayed frame rate at
  every multiplier now (4X: the game runs at 30). 2X and FSR FG are unchanged; if you set a limit for 3X or more
  before, it now binds as labelled.
- **lmxxf could stay off for a whole session** when the game never submitted the list of the first neural frame,
  or submitted it through a wrapper. It now watches a newer frame's list after 8 frames, matches by COM identity
  after 16, and logs why. After a dropped list lmxxf's temporal history restarts instead of carrying on from a job
  that never ran. Found by reading the code; not seen in a game.
- **A neural command list the game throws away** (reset or released without running it) left danielblnc waiting
  for it for the rest of the session ("previous Record still awaits submission"), and could let lmxxf start a job
  on inputs that were never copied. AMDNR now notices such a list (it watches `Reset` on the one list NR last
  recorded into; OptiScaler's own D3D11 / Vulkan bridges report a list they did not run): danielblnc finishes the
  lost job without it and NR comes back after about 4 s, and lmxxf drops the job as a lost frame.
  `[DlssNr] AmdNeuralListRecovery=false` turns this off. Not yet tested in a game: nothing reproduces a dropped list
  on demand.
- **Quitting the game while NR runs.** NR stops the moment the game starts to exit, and AMDNR now waits at most about
  2 s for its runtime (lmxxf could wait up to 30 s for its queue before), also when the exit comes from a crash
  handler on the rendering thread. lmxxf over the Vulkan bridge: a clean quit during lmxxf's warm-up no longer
  turns lmxxf off at the next start. Not yet tested in a game.
- **lmxxf when the GPU is removed** (a driver reset or crash): lmxxf now stops at once and says so in
  `lmxxf_backend.log` (nothing is released or waited for), as danielblnc already did.
- **XeSS, FSR 2.2 and FSR 2.1.2 changed the resource state of AMDNR's NR output** (in Unreal Engine titles a wrong
  transition in and out). They now leave NR's output alone, as the FSR 4 / FSR 3.1 path already did
  (`[DlssNr] AmdNrColourGuard=false` restores the old behaviour). Not yet tested in a game.
- **NR after Ray Regeneration ran again on a frame the upscaler skipped** (its first frames, a backend change, the
  FSR 2.1.2 fallback), editing the previous output a second time. Such frames now get no neural pass and NR history
  restarts (`[DlssNr] AmdSkipUnwrittenFrames=false` restores the old behaviour).
- **Smaller resource-state fixes:** FSR 4's auto-exposure padding workaround left the game's colour in the wrong
  state; FSR Ray Regeneration issued a transition to the state a resource was already in; OptiScaler's Vulkan
  bridge in Unreal Engine titles transitioned its own copy of the colour from the wrong state.
- **Two neural streams in one frame** (split screen, a scope or picture-in-picture view, a second upscaler context)
  restarted or mixed each other's NR history. When AMDNR measures more than one NR entry per presented frame, NR
  runs on the largest one and the others pass through untouched (`[DlssNr] AmdOneStreamPerFrame=false` turns this
  off). Games with one entry per frame are not affected. Not yet tested in a game.
- **A frame from another D3D12 device** (a second GPU, or a game that re-creates its device) could reach NR
  resources that belong to the first device. It now goes to the upscaler untouched and is logged once
  (`[DlssNr] AmdDeviceGate=false` turns the check off). Not yet tested on a PC with two GPUs.
- **Linux / Proton (DXVK): Vulkan instances and devices OptiScaler creates for itself** were taken for the game's,
  which could overwrite the game's Vulkan instance and its Anti-Lag 2 state. They are now told apart
  (`[Hooks] SkipOwnVulkanObjects=false` restores the upstream behaviour). Not yet tested on Proton.
- **Model interleave: ghosting and 30 Hz grain**, see Changed (Edit accumulation). Also fixed for every preset: the
  "Model-frame ghost" meter and the Self-tuning totals never showed a value; a model call the runtime refused was
  treated as a model frame; and a wrong barrier state in interleave's motion accumulation on titles whose motion
  vectors are not in NON_PIXEL_SHADER_RESOURCE.
- **Neverness to Everness: the game's crash reporter loaded OptiScaler and wiped the game's log.** OptiScaler now
  skips `CrashClientReporter.exe`, `UnrealCefSubProcess.exe` and danielblnc's installer (`dlssnr_on_amd_setup.exe`),
  so they no longer load it or replace the game's `OptiScaler.log`.
- **`OptiScaler.log` was emptied while another process still wrote to it.** When another running process holds
  the log, the new session now appends to it instead.
- **Save Settings dropped a value you set when it equalled the default** (the FSR-RR path-traced profile keys, skin
  smoothing on/off, radius and guide thresholds, the skin classifier keys, and RR bias mask strength). It was saved
  as `auto`, which follows the game's default, so skin smoothing turned off in Resident Evil Requiem would have come
  back on. Save Settings now keeps the value.
- **Neural tab: the orange Streamline (slInit) message stayed up while NR ran** (The Last of Us Part I, where NR
  runs on the game's FSR 3.1). It now goes away once NR receives frames, and it says "DLSS is not available in
  this game" instead of "the game switched DLSS off".
- **The HOME notice said "On" while the neural runtime was stopped.** It now says "On - but the neural runtime is
  stopped (see the Neural tab)" when danielblnc was refused or failed, or lmxxf failed.
- **Ray Regeneration's log said "GPU is RDNA 4" on RX 7000**, and its version query wrote a warning on every menu
  frame. Both are fixed.
- **The update notice pointed at upstream OptiScaler**, which has no neural rendering, and could tell AMDNR players
  to "update" to it. It now checks AMDNR's own GitHub releases and compares against the AMDNR version
  (`[Hotfix] CheckForUpdate=false` turns it off).

### Added
- **lmxxf is faster on RX 9070 / RX 9070 XT (RDNA 4): network 17.8 -> 15.3 ms at 1080p on an RX 9070 XT (720p 8.0
  -> 6.9 ms), the same image bit for bit.** New "c32w" one-wave kernels for the network's C32 layers ship in
  `LmxxfNrRuntime.pak`. `LmxxfNrRuntime.dll` checks their SHA-256 before it loads them and runs the old kernels if
  they do not match. lmxxf's status line and `lmxxf_backend.log` show `c32w=on` when they run (`c32w=off:<reason>`
  otherwise). With the old `LmxxfNrRuntime.pak` everything works as before, at the old speed (`c32w=off:nofile`).
  RX 9060 XT keeps the old kernels for now (the environment variable `AMDNR_C32W=1` turns them on there, untested);
  `AMDNR_C32W=0` turns them off everywhere. RX 7000 is not affected. The c32w kernels are AMDNR's own work
  (Copyright (c) 2026 3zwr1 (AMDNR)); they run lmxxf's network (Kien, MIT). Acknowledgement: AMD's public RDNA 4
  WMMA documentation (AMD GPUOpen, the ROCm matrix instruction calculator), used for ideas only.
- **Resident Evil Requiem / PRAGMATA without REFramework: a warning.** When re9.exe, re9demo.exe or pragmata.exe
  runs without REFramework's dinput8.dll (none in the game folder, or Windows' own dinput8.dll was loaded),
  OptiScaler.log gets a warning, a notice shows for 20 s and the menu shows an orange line with the download link.
  Without REFramework, Resident Evil Requiem crashes 15-60 s after launch.
- **Resident Evil Requiem skin smoothing: a Robust skin classifier (the new default).** The SSS guide alone marked
  25-39% of a night street as skin (walls, coats, umbrellas); the Robust classifier also asks how much the game's
  SSS pass moved the pixel, whether the surface around it agrees (held from the previous frame, so faces do not
  flicker) and whether the albedo is skin-toned: 0.5-2.3% of the frame in the same scenes, with faces and hands
  still covered. Neural > Quality > Ray Regeneration > Skin classifier; "Guide only (0.3.3)" is the old mask.
  Keys `[FSR-RR] FfxDenoiserSkinSmoothingClassifier` (1 Robust, 0 guide only), `...MovedLow` / `...MovedHigh`
  (0.05 / 0.12) and `...ShowCues`.
- **Still-surface steadiness** (Neural > Quality, `[DlssNr] AmdStabilityStaticRelax`, both runtimes): steadies
  shadows and flat areas that pulse while nothing moves; only pixels that provably did not move are affected. Off by
  default in this release; try 0.5 if shadows flicker on still walls. Not used while Model interleave is on
  (Edit accumulation, the default) or at Temporal stability 0.
- **DLSS Enabler options say where the DLL comes from.** AMDNR does not ship `dlss-enabler-headless.dll`; the Frame
  Gen options that need it now show how to get it (DLSS Enabler 4.9.0 or newer by Artur Graniszewski, its
  `version.dll` renamed) with a link, and the log says the same.
- **danielblnc 0.4.2 is supported, ahead of its release** (not yet tested in a game). This build drives danielblnc
  0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0, 0.4.1 and 0.4.2. The newest public build is 0.4.0: get it in
  `v0.4.0-Runtime.zip`.
- **Clearer message when your danielblnc files are not a build this AMDNR drives.** The Neural tab and
  `amd_presr.log` name the file and its version, say whether it is newer, older, a re-published copy or missing,
  and what to do (usually: use `v0.4.0-Runtime.zip` from the AMDNR release, all three pass DLLs from one zip). The
  long line now wraps. The runtime combo reads "danielblnc (0.3.x / 0.4.x)".
- **Warning when danielblnc's standalone DLSS-NR on AMD is installed next to AMDNR** (his `version.dll`,
  `winhttp.dll` or `dxgi.dll`, or `dlssnr_on_amd_setup.exe`). AMDNR already runs his runtime as
  `dlssnr_amd_pass1..3.dll`. The standalone copy hooks the game on its own, shares `dlssnr_on_amd.ini` and
  `dlssnr_on_amd.log`, and its End / Enter overlay only switches itself, so toggling there changes nothing. Remove
  it. The warning shows in orange in the Neural tab and in the logs.
- **lmxxf says why it stopped:** the Neural tab shows "lmxxf stopped after an error: ...". `amd_bridge.log` lists
  every HIP device and marks the game's GPU, and lmxxf's status line names the HIP device it uses. While lmxxf's
  runtime is not up yet, its status line says so.
- **Resident Evil Requiem, FSR Ray Regeneration:**
  - **Texture route for the path-traced profile** (`[FSR-RR] FfxDenoiserProfileTextureRoute`, default 1.0; slider
    under the profile checkbox). With the profile on, textured and very dark surfaces (brick, tiles, fabric,
    signs) go back to the normal routing, and flat surfaces such as faces keep the profile. It is meant to remove
    the etched look the profile gave textures in 0.3.3; 0 is the profile as in 0.3.3. The profile itself stays
    opt-in (off by default). Not yet confirmed in the game. New RR debug view *ProfileTextureRoute*.
  - **Skin smoothing is on by default in Resident Evil Requiem**, with the settings that tested well there:
    strength 1.0, radius 16, guide thresholds 0.0265 / 0.0414. It reduces flicker on faces. A value you set
    yourself wins; `FfxDenoiserSkinSmoothing=false` turns it off. Other games keep it off by default.
- **Upscaling tab hints:** "Not applied yet - press Change Upscaler" when the combo shows a choice that is not
  running, and a hint when FSR 2.1.2 runs on a card that has FSR 4.
- **Logs for bug reports:** NR on/off from the hotkey and from the Neural tab checkbox is logged. `amd_bridge.log`
  and `amd_presr.log` start with a session header (time, process, AMDNR version). `dlssnr_on_amd.log` marks the
  copy AMDNR hosts. `amd_presr.log` lists the `dlssnr_on_amd.ini` keys AMDNR does not set, and warns about ones
  that change the picture. On PCs with two GPUs the GPU line names the adapter NR runs on.
- **Cost hints:** danielblnc on RX 7000 (about 27 ms per frame even at 320x180, about 52 ms at 1280x720; 0.4.0 on
  an RX 7900 XTX), and danielblnc with an NR input of 1440p or more (about 45 ms per frame; 0.3.1 on an RX 9070
  XT): lower the game's render resolution, or set NR resolution to 67-85%.

### Changed
- **Model interleave: new default preset "Edit accumulation"** (`[DlssNr] AmdInterleavePreset=10`, both runtimes;
  lmxxf runs it as 11). No picture is carried over any more: every frame, model frame and skipped frame alike, is
  this frame's raw picture plus the model's carried correction, checked raw against raw on every frame; where it
  does not fit, both frame types show the model's overall tone curve, so nothing pulses. This removes the held
  picture's one-frame ghost and its 30 Hz grain, and costs no more than Guided fill v2. On danielblnc the model
  frames show the fitted correction too, not the model's exact picture: for that, pick "Guided fill v2"
  (Neural > Performance > Interleave preset, or `AmdInterleavePreset=6`); on lmxxf "Classic carry" is the old
  fill. Not yet tested in a game.
- **danielblnc: away from 100% NR resolution, working sizes above about 1 MP are rounded to 64-pixel steps**, so
  dynamic resolution, DLSS modes and Dynamic NR reuse a few sizes instead of keeping 75-350 MB of VRAM per pass for
  every new one (`[DlssNr] AmdNrSizeStep`, 0 = exact sizes as before). 100% is unchanged.
- **Resident Evil Requiem, FSR Ray Regeneration: RR bias mask strength 0 by default.** Pixels the game flags in its
  DLSS bias mask are now denoised like the rest (a tester saw less blotching and grain at 0; not yet confirmed from a
  log that the game publishes the mask - if it does not, this changes nothing). A value you set in
  `[FSR-RR] FfxDenoiserBiasMaskStrength` wins, also 1; Save Settings now keeps an explicit 1 in every game.
- **Menu Scale:** the menu window, its sliders and combos now follow the Menu Scale (a smaller scale shrinks the
  window, not only the text); long notes (Frame Gen, XeFG 6X, native XeSS FG) wrap instead of running off the
  right edge; the FrameTime graph ends with "fps" again; and a window you dragged is pulled back on screen when it
  grows (a larger scale or a taller tab).
- **Version:** `OptiScaler.dll` reports AMD-NR v0.3.3.2. `LmxxfNrRuntime.dll` is 0.3.3.2 r2 (Properties >
  Details; the first 0.3.3.2 runtime said 0.3.3.2), and the runtime combo reads "lmxxf (0.3.3.2)". It works with
  the old `LmxxfNrRuntime.pak` too, without the speed-up.
- **danielblnc Residual notes:** the notes under the Residual sliders say what strength, limit and edge fade do at
  100% and away from it, and the Edge fade slider is greyed out only at exactly 100% with Dynamic NR off. The NR
  resolution tooltip and the note while you drag it say what a new NR size keeps with each runtime.
- **Logs:** the Resident Evil Requiem crash-triage `[Probe]` log lines are gone; the default-off `[Hotfix] Diag*`
  switches stay for testers.
- **`OptiScaler.ini`:** `[FSR-RR]` documents the texture route and the Resident Evil Requiem defaults, and
  `NrBackend` names danielblnc 0.3.x / 0.4.x. With your own ini, the new keys use their defaults.

### Notes
- **Resident Evil Requiem (and its demo) needs REFramework: a known requirement, not an AMDNR bug.** OptiScaler relies on it
  to get past Capcom's anti-tamper (<https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem>). Without it the game crashes 15-60 s after
  launch ("An unhandled exception occurred", `re9.exe+0x9fb347e`). Put `dinput8.dll` from `REFramework.zip` in the latest
  nightly (<https://github.com/praydog/REFramework-nightly/releases>) next to `dxgi.dll`, and change REFramework's menu key (e.g. to Delete):
  it is also Insert. After a game update, expect crashes until REFramework is updated. PRAGMATA, Monster Hunter Wilds and Onimusha probably need it too (not confirmed).
  Since this release AMDNR warns when it is missing (Added).
- **Not yet tested in a game** (built and checked on the desk only): recovery from a dropped neural list, the
  device gate on PCs with two GPUs, one stream per frame, the Proton / DXVK Vulkan change, the bounded exit, the
  still-surface slider, and Ray Reconstruction on RX 7000 with `FfxDenoiserAllowPreRdna4=true`. Each has an ini
  switch to turn it off (see above).

## 0.3.3.1 — 2026-09-25

Hotfix: danielblnc's runtimes 0.3.3 and 0.4.0 are supported (and 0.4.1 ahead of its release), and FSR Ray Regeneration's path-traced profile is
opt-in again (it corrupted the picture in Resident Evil Requiem). Only `OptiScaler.dll` and the ini changed;
`LmxxfNrRuntime.dll` and `LmxxfNrRuntime.pak` are the same as in 0.3.3.

### Added
- **danielblnc 0.3.3 is supported (new, not yet tested in a game).** AMDNR now drives danielblnc's 0.3.3
  runtime (pass DLL 7,607,296 bytes, SHA256 907b30a6...) as well as 0.2.17, 0.3.0, 0.3.1 and 0.3.2. In
  0.3.3 every address AMDNR uses moved (its data section moved and grew). They were worked out twice,
  independently, from the file, and both results agreed. The build has not run on a GPU yet. The existing
  `dlssnr_on_amd_weights.bin` works with it unchanged. The release has a new `v0.3.3-Runtime.zip`
  (danielblnc's 0.3.3 runtime, unmodified, with his permission, plus the weights); `Runtime.zip` (0.3.1) and
  `v0.3.2-Runtime.zip` still work.
- **danielblnc 0.4.0 is supported, and 0.4.1 ahead of its release.** Daniel shared both early, so AMDNR
  drove 0.4.0 the day he published it (2026-09-25; the public build is byte-identical to the one we mapped):
  the release has a new `v0.4.0-Runtime.zip` (his 0.4.0 runtime, unmodified, with his permission, plus the
  weights). His release notes claim 42% more speed than 0.3.3; its new fast kernels run on RX 9000 only.
  0.4.1 will work the day it comes out, with no AMDNR update needed. Each build was derived twice,
  independently, and both results agreed; the host contract is unchanged from 0.3.3. Not yet run on a GPU.
  The existing weights file works with both.

### Fixed
- **FSR Ray Regeneration in Resident Evil Requiem and PRAGMATA: the path-traced profile is opt-in again.**
  In 0.3.3 it turned on by itself in these two games. A tester's report from Resident Evil Requiem (RX 9070 XT)
  shows that it cleared most of the grain on faces but badly corrupted the rest of the picture. It is now off
  unless you tick *FSR Ray Regeneration: path-traced profile* (Neural > Quality) or set
  `[FSR-RR] FfxDenoiserPathTracedProfile=true`, so by default these games look as they did in 0.3.2. The
  profile itself is unchanged. The cause was found later; 0.3.3.2 adds a texture route for it.

## 0.3.3 — 2026-09-24

lmxxf on RDNA 3 (RX 7000), an optional RenoDX colour composition on both runtimes, the real fix for lmxxf
on Vulkan, lmxxf's newest kernels (faster), a path-traced profile for FSR Ray Regeneration, paced XeFG up
to 10X (opt-in), a fix for lmxxf's RAM use that climbed for as long as NR ran (now flat in GPU-bound games
too), and an lmxxf fix for colours that turn grey (its two new options are on by default; tested in Silent
Hill 2). Late additions, not yet tested in a game: a fix for multi-frame generation's lock and stalls at
3X-10X, and support for danielblnc's runtime 0.3.2.

**Our Discord has a new server: <https://discord.gg/AMDNR>.** The menu's Discord button and every
link in this release point to it; the old invite still works.

### Added
- **danielblnc 0.3.2 is supported (new, not yet tested in a game).** AMDNR now drives danielblnc's
  0.3.2 runtime (pass DLL 6,788,096 bytes, SHA256 b92f7481...) as well as 0.2.17, 0.3.0 and 0.3.1, so both
  0.3.1 and 0.3.2 work and nothing needs to change for anyone on 0.3.1. In 0.3.2 only the GPU kernels
  changed; the host side is the same as in 0.3.1 apart from the Init entry, which moved by 0x30. Every
  address AMDNR uses was worked out from the file and then checked again independently, but the build has not
  run on a GPU yet. The existing `dlssnr_on_amd_weights.bin` works with it unchanged. As before, a pass DLL
  AMDNR does not recognise is refused by its size and hash, and the menu names it.
- **lmxxf runs on RDNA 3** (RX 7900 XTX / XT / GRE, RX 7800 XT / 7700 XT, RX 7600, Strix Halo) through
  AMDNR's own RDNA 3 backend, by **3zwr1**. lmxxf's kernels use RDNA 4 FP8 matrix instructions that RDNA 3
  does not have; the backend carries that work on RDNA 3's F16 matrix units (FP8 to F16 is exact) and
  arranges the matrix operands for RDNA 3's layout, with lmxxf's kernels themselves unchanged. The network's
  output matches the RDNA 4 result (identical at 720p, within rounding at 1080p). Confirmed on an RX 7800 XT: same picture as RDNA 4 (edit correlation 0.987, 99.9% of values within 8/255, no structured error).
- Slower than on RDNA 4 (about 2.3x the work at the same GPU size): on an RX 7800 XT one network run takes
  about 34 ms at 1280x720 and 73 ms at 1920x1080, so start with NR resolution at 67% or lower.
  The Neural tab now lists lmxxf as available on these GPUs; handheld APUs (780M, 890M) stay unsupported.
- Every RDNA 3 module in `LmxxfNrRuntime.pak` carries the backend's credit, and the runtime refuses an RDNA 3
  module without it.

- **Colour composition: RenoDX (experimental), on both runtimes** (Neural > Image look). *Classic*, the
  default, is the picture you had. *RenoDX* runs RenoDX's colour composition after the model - its
  two-branch luminance ratio and hue step and its neutral-axis gamut compression (clshortfuse, MIT) -
  with its own Composition detail (0-2) and Composition colour (0-4), the Highlight guard (default 2x)
  and wilsjo2's skin / environment final edit. danielblnc: the runtime's answer is composed against the
  copy it was handed, the white point from the title's exposure or estimated. lmxxf: the runtime
  returns its RenoDX answer (strengths 1/1) and the host composes it against what the network was
  fed, latched per job. Refused, with a note in the menu and Classic running, when Network output is
  on or the colour is display-referred. New ini keys `AmdComposition`, `AmdComposeDetail`,
  `AmdComposeColour`; styles and presets leave them alone.
  - What the two sliders can do: Composition colour above 1 multiplies the saturation that is there, so
    in near-grey scenes such as Silent Hill 2's fog it stays subtle (2 is barely visible there; try 3-4);
    between 0 and 1 it only matters where the model changed the colour. Composition detail only scales
    the model's own change in brightness, so where the model changed little its effect is subtle too.
  - lmxxf: the highlight colour guard (now on by default, see Fixed) would have put the game's colour
    back on every pixel fed past 1.5 whatever Composition colour was set to. It now runs on the model's
    answer *before* the composition, so Composition colour still acts on those pixels. This ordering is
    new in this build and not yet tested in a game. With the guard off the RenoDX picture is unchanged,
    bit for bit. danielblnc has no highlight colour guard, so nothing cancels its sliders; its runtime
    is closed, and this change is lmxxf-only.
  - lmxxf_backend.log: a line on every change of Composition detail, colour, Highlight guard, skin edit
    or the highlight colour guard while RenoDX composes (up to 24 per session), and the `lmxxf stats`
    line's `RenoDX composed` part now carries the values in force, so a log shows that a slider move
    arrived.
- **`LmxxfNrRuntime.pak` v2 carries a plaintext credit notice** at the top of the file (lmxxf's network
  and kernels, and the RDNA 3 backend by 3zwr1), bound into the pack's authentication: a pack whose
  notice was edited or removed is refused by the runtime. `LmxxfNrRuntime.dll` shows the copyright
  under Properties > Details; `Licenses\AMDNR_RDNA3_BACKEND.txt`. The new pak needs the new
  `LmxxfNrRuntime.dll` - both are in this zip; replace them together.
- **lmxxf: Full network option** ([DlssNr] LmxxfFullNetwork, Neural tab): runs all 71 blocks instead of skipping 42, 43 and 46 - slightly more faithful, about 0.5 ms slower at 1080p (network time 16.6 -> 17.1 ms on an RX 9070 XT). An LmxxfNrRuntime.dll older than the option refuses it; the default network then runs and the menu says so under the checkbox. danielblnc's runtime is closed, so it does not apply there.
- **XeFG up to 10X, opt-in (6X default); confirmed in a game: pending** (`[XeFG] MaxInterpolatedFrames=9`,
  or *XeFG ceiling (restart)* under FG Output in the Frame Gen tab: 4X, 6X (default), 8X or 10X; restart,
  then pick the multiplier in the MFG combo). 10X: opt-in, needs a 360 Hz+ display and a frame cap;
  +128 MiB at 4K. Nothing changes unless you raise the ceiling. D3D12 games only (the XeFG output is
  D3D12-only). Above 6X the ceiling is written only into OptiScaler's own 1.3.1.78 provider with XeFG pacing
  installed. With `ExtraPacing=false`, another provider build, or a game's own XeSS 3 copy, it stays at 6X,
  and the log and the menu say why. The provider reserves VRAM for the ceiling at init, whatever multiplier
  runs: about +128 MiB at 4K over 6X (+60 at 1440p, +34 at 1080p; half that at 8X). Cap the frame rate at
  refresh / 10 (48 fps at 480 Hz); latency is high (about a 21 ms base frame at a 480 Hz cap). 7X-10X is
  not yet confirmed in a game: testers, please send OptiScaler.log. The same on both runtimes (no NR code
  involved).
- **XeFG: Allow dilated motion vectors (experimental, off by default)** (`[XeFG] AllowDilatedMV`, Frame Gen
  tab). When a game's frame-generation input sent dilated (display-resolution) motion vectors, the Frame Gen
  tab showed "Requires disabling dilated motion vectors" and kept XeFG off; the only way past it was Ignore
  Init Checks, which skips every check. A checkbox under that line now skips only the motion-vector check:
  the fullscreen and HDR checks still apply, and XeFG runs in Intel's high-res motion-vector mode as before.
  Generated frames can show artefacts in some games. The key is documented in OptiScaler.ini. Not yet
  tested in a game.
- **MFG diagnostics in OptiScaler.log (kept at LogLevel=2).** The pacing lines and DXGI's present statistics
  are written at 3X and above only; the overlay WARNs at any multiplier.
  - Every 5 s, a second line, "XeFG pacing detail": the step and what bounded it (ring, ceiling, ours or
    estimate), the float at ring+0x1B8, ctx+0x340/0x341, present time per burst and the worst present, and
    the share of frame gaps under 0.6 ms and over 8 ms. The scheduler and deadline counts of the first stats
    line are now per 5-s window instead of cumulative.
  - A "warm-up" line for each of the first 4 bursts after FG turns on or the multiplier changes.
  - "XeFG pacing diag" for the first 3 bursts: which call presented each frame, and when.
  - A WARN, at most one a second, for a present or an overlay step over 20 ms, or for an overlay draw skipped
    because all its command allocators are still in flight. The two overlay WARNs are not limited to 3X and
    above: at 2X or with FG off they can fire too, e.g. once when the menu first opens (ImGui's font upload).
  - DXGI's present statistics (presents, refreshes, refresh rate) every 5 s.
  - When the XeFG swapchain is created: the display's exact refresh rate (QueryDisplayConfig) and the
    swapchain XeFG presents on (size, buffers, flags).
  Testers at 4X-10X: send OptiScaler.log (the shipped LogLevel=2 records these lines).
- **`[XeFG] PaceOnSwapchain` (experimental, off by default; Frame Gen tab, "Pace on swapchain")**: before each
  paced generated frame, wait (never past that frame's deadline) until XeFG's swapchain has room, so a
  blocking present is taken before the frame is due instead of bunching the frames after it. Needs Extra
  pacing; turning it on needs a restart. It gives the swapchain's count straight back, and switches itself
  off for the session (logged) if the swapchain's waitable object is not what it expects. Not yet tested in
  a game.
- **FSR Ray Regeneration: bias mask routing, a debug view and skin smoothing (experimental)** (Neural >
  Quality > Ray Regeneration, all live; `[FSR-RR] FfxDenoiserBiasMaskStrength`, `FfxDenoiserDebugMode`,
  `FfxDenoiserSkinSmoothing*`). Bias mask strength decides whether the pixels a game flags in its DLSS bias
  mask go around Ray Regeneration or through it; the RR debug view picks one of RR's internal images by
  name, to see where face grain comes from; skin smoothing (AMDNR, off by default) evens out the
  low-frequency blotches on faces in games that publish an SSS guide (Resident Evil Requiem), changing only
  RR's diffuse lighting on the pixels the guide marks as skin. `[RR_SKIN]` in OptiScaler.log records every
  change of skin smoothing with its frame number (off, on with no usable SSS guide, unable to run, each
  start or change of the pass), so a tester's on/off comparison can be read from the log. If the pass
  cannot be created, the menu disables its controls and shows "(could not start - [RR_SKIN] in the log)";
  a ticked box stays clickable so it can be turned off. The debug view list reads a fixed table, so
  re-creating Ray Regeneration (a resolution, quality or backend change) with the overlay open cannot crash
  it. Neither NR runtime is involved.
- **Memory figures in the logs, both runtimes**: VRAM in use and the OS budget (DXGI, the adapter's local
  memory) and the process's private commit (Task Manager's "Commit size"), so a tester log shows whether
  memory grows at fixed settings or steps up with a new NR size. danielblnc (`amd_presr.log`, as
  `vramMB= budgetMB= privateMB=`): on the heartbeat, on every NR size change ("AMD resize: ... -> ...") and
  once after the rebuild at the new size ("AMD memory after the rebuild"). lmxxf (`lmxxf_backend.log`, as
  `mem: vram <used> MiB used of <budget> MiB budget, private <n> MiB`): at the end of the `lmxxf stats`
  line (every 600 frames) and of the resize line.
- **The previous session's OptiScaler.log is kept.** At start, before the new log begins, the old one is
  moved to `OptiScaler.previous.<exe>.log`. There is one per game exe, so The Last of Us's two exes no
  longer erase each other's log. While the game runs, a small `optiscaler_running_<exe>.marker` sits beside
  the log and is removed when OptiScaler unloads. If it is still there at the next start, the new log says
  **no clean exit recorded** for that session: a crash, a hang ended from outside, or a game that ends
  itself that way. A session removes the marker only while it still holds that session's own line (checked
  and deleted in one step), so a game that relaunches its own exe cannot erase the running copy's marker.
  Every log now starts with a session line: local time, tick, pid, exe and the AMDNR version. This works
  with `LogToFile` and `SingleFile` on, as shipped.
- **Retry lmxxf button (Neural tab, Vulkan titles).** When `lmxxf_vk_launch.pending` holds lmxxf back, the
  tab says so, now also on lmxxf-only installs, where the line never appeared. The button removes the file,
  so it no longer has to be deleted by hand. If no neural runtime has started yet, lmxxf starts at the next
  upscaler frame; otherwise it starts on the next game start.
- **danielblnc on Vulkan titles explains its own log.** At 1 Neural pass danielblnc never froze in Indiana
  Jones and had no timeouts; one line in amd_bridge.log and a note under the runtime choice now say what
  its logs mean:
  - NR runs inline every frame.
  - The first NR frame of a session pauses about 5 s.
  - The runtime's "inline flag check ... NOT seen ... update the driver, or set Inline=0" line comes from the
    Vulkan bridge's queue order, not from the driver.
  - Its "HIP runtime 0" reads 0 on every machine under OptiScaler.
  - Leave dlssnr_on_amd.ini as it is: AMDNR runs this runtime inline, and async mode would pass the colour
    through untouched.
  - With `AmdVkLateCopyWait` on (below), the amd_bridge.log line says the key is on and leaves out the
    "5 s" first-frame pause and "NOT seen" sentences, as the note in the Neural tab does.
- **A Vulkan gap line for both runtimes (diagnostic).** When more than a second passes between two NR
  frames on a Vulkan title, the log says "AMD vk: no Record for N ms - either the game did not reach
  Evaluate or the bridge's previous-frame wait held it" ("AMD vk (lmxxf): ..." under lmxxf). danielblnc
  writes it to amd_presr.log with a slot snapshot; lmxxf writes it to amd_bridge.log. The first 10 are
  logged, then every 10th. Together with OptiScaler.log's "has not completed after" warning, it tells a
  game pause from a bridge stall behind the 3-4 s SPIKE lines in dlssnr_on_amd.log. Testers: please send
  it with any 3-4 s hitch on a Vulkan title.
- **slInit result in the log.** Every Streamline game logs `slInit returned N (name) in X ms`. For error
  0x18 the line also says where Streamline wrote its minidump. The result is kept for the Neural tab.
- **[SLINIT] diagnostics for Streamline start-up.** They log at INFO, so the shipped LogLevel=2 records
  them, and only while the game's slInit runs. They record: the preferences the game passed (features,
  flags, plugin folders, log folder); each Streamline plugin DLL that was loaded, from where, and whether
  OptiScaler hooked it; each plugin's onLoad call and result, and whether its JSON was set; the adapter
  list, and Streamline's SystemCaps before, during and after the architecture spoof; and every NGX feature
  Streamline asks about. After a failed slInit, the log also lists the sl.*.dll files in the plugin folders
  with their file versions. OptiScaler's loader checks are off for each file while it is read, so reading
  them loads nothing. Testers with a Streamline start-up error: send OptiScaler.log and search it for
  `[SLINIT]` and `slInit returned`.
- **Vulkan titles: `[DlssNr] AmdVkLateCopyWait` (experimental, off by default).** With NR on, the
  Vulkan-on-D3D12 bridge queues its wait for the game's texture copies right before its own D3D12 list
  runs, instead of before the upscaler and NR record. The work the runtimes queue while recording then no
  longer waits for the game's own vkQueueSubmit. For danielblnc on Indiana Jones this is expected to remove
  the ~5 s pause on the first NR frame, the ~0.3 s hitch at each staging rebuild and the "inline flag check
  ... NOT seen" line. It has not been tested in a game yet: testers, please set it to true and send
  OptiScaler.log, amd_presr.log and dlssnr_on_amd.log. The proof line in OptiScaler.log is
  "AmdVkLateCopyWait is on", and the danielblnc note under the runtime choice changes to say the key is on.
  lmxxf is unchanged: its runtime load and HIP input signal already run after the bridge's list. With the
  key off, or with NR off, the bridge runs exactly as before. The code is in the bridge, so it is the same
  for both runtimes. D3D12 and D3D11 games are not affected: the D3D11 bridge has no such wait. With the
  key on, the bridge keeps its own record of the held copy wait: on the rare frame whose copy fence is
  missing (a failed fence creation) it skips the frame, as the key-off path does, and never runs its D3D12
  list without that wait.

### Fixed
- **lmxxf: colours that suddenly turn grey (Silent Hill 2) or stay grey (Control Resonant).** **Silent
  Hill 2: fixed by *Highlight colour guard* and *Auto-exposure highlight cap* (Neural tab, lmxxf), both on
  by default - tested in Silent Hill 2.** Control Resonant: not yet tested. The self-heal and the lower
  floor, also on, act only on a feed blown out as a whole (the likely Control Resonant case) or an
  auto-exposure below 1/256. The cause, read from the code and the logs: the network's codec
  squashes each colour channel above 0.75 (at the exposure the network is fed with) on its own. A bright pixel therefore
  reached the network close to white, and the decode (Classic, and RenoDX's answer) took that pixel's colour
  from the network: grey at the right brightness. Silent Hill 2 publishes no exposure texture, and there
  auto-exposure stepped up 1.5x at a time in dark fog (3.3 to 25 within one stats window), pushing the bright
  areas past that point. lmxxf only, host-side: the runtime, its network and the probe hash (51249b6b) are
  unchanged.
  - **Exposure self-heal** (on): a blown-out feed is now measured directly: more than half the fed pixels
    above paper white in every channel, or a fed mean above 12, in two stats windows in a row. The heal also
    fires when the game's exposure is exactly 1 or refused, as long as auto-exposure is on; the network is
    then fed by auto-exposure. The log line gives the raw exposure value, and the menu status says
    `self-heal: auto-exposure`. A healthy feed meets neither rule.
  - **Auto-exposure floor 1/65536** (was 1/256), for linear HDR games without an exposure texture that need
    an exposure below 1/256.
  - **Highlight colour guard** (new `[DlssNr] AmdLmxxfHighlightChromaGuard`, **on by default**; tested in
    Silent Hill 2; Neural tab, lmxxf edit): where a pixel fed to the network has its brightest channel above
    0.75 (fully from 1.5), the model's answer keeps its brightness and takes the game's own colour. Darker
    pixels are untouched, bit for bit. Classic and RenoDX composition (in RenoDX it now acts before the
    composition, so Composition colour still applies; that ordering is new in this build and not yet tested
    in a game). Off = the 0.3.2 colour.
  - **Auto-exposure highlight cap** (new `[DlssNr] AmdLmxxfAutoExposureHighlightCap`, **on by default**;
    tested in Silent Hill 2): auto-exposure no longer raises the exposure while more than 8% of its samples
    are fed above 0.75. Above 25% it may lower it up to 4x per model frame instead of 1.5x.
  - On screen, with the defaults: highlights keep their colour, and dark scenes lit by bright lights are
    exposed slightly less while auto-exposure feeds the network; pixels fed below 0.75 are untouched. The
    self-heal and the lower floor act only on a black or blown-out feed, or where auto-exposure needs an
    exposure below 1/256. Silent Hill 2 publishes no exposure texture, so the self-heal cannot act there,
    and its auto-exposure (3.3 to 25) is far above the floor; the two new options are what fix it. To get
    the 0.3.2 picture back, turn both off.
  - lmxxf_backend.log: one `lmxxf exposure source` line per session (the game's exposure texture accepted,
    refused, or none, with the raw value). The stats line adds the fed picture's shoulder and blown shares and
    prints the exposure and its raw value in full. Testers who still see grey colours: please send
    lmxxf_backend.log.
  - **danielblnc: the equivalent fix is planned for 0.3.4.** In 0.3.3 the colour fix is lmxxf-only.
- **Menu: the lmxxf skin help said the model's native character mask does not exist under lmxxf.** It does:
  lmxxf runs NVIDIA's auto mask on, at fixed default strengths. Controls for it are planned for 0.3.4.
- **Multi-frame generation (3X-10X): the ~1 fps lock after turning XeFG on, and its return after FG off/on or
  a multiplier change** (Silent Hill 2, RX 9070 XT). Not yet tested in a game. The first interpolated frames
  of a session can take 1-3 s. XeFG's pacing kept that stall as its measure of a frame, spread every burst
  over it and never cleared it, so the base frame rate sat near 1 fps for 20-30 s, and toggling FG did not
  help. Now a burst is never spread over more than 1/15 s (6.7 ms per frame at 10X, 22 ms at 3X). Our own
  hitch-free frame-time median replaces a provider figure that is three times it, and a render estimate
  breaks a lock the ceiling is already holding. Each activation and multiplier change restarts the pacing's
  own state with a 4-burst warm-up, and every frame's deadline is placed by its index in the burst. In a
  model of the Silent Hill 2 lock the base rate recovers in under 1 s (was 20-125 s). A healthy burst is
  paced exactly as before, and 2X is unchanged. Below 15 fps base at the chosen multiplier the burst is
  capped at 1/15 s. Same on both runtimes (no NR code involved).
- **The overlay's command allocators are no longer reset while the GPU still uses them** (the likely cause
  of the stalls of exactly 100 ms at high multipliers; not yet tested in a game). The overlay's eight
  command allocators were chosen by back buffer, so only three were used, and each was reset with no fence
  while the GPU was still behind. They are now used in turn, and each is reused only once its fence has
  passed. The overlay never waits: if the next allocator is still in flight, that one present goes without
  the overlay. If the fence cannot be created, the old behaviour stays. The game image is not touched.
- **The NR resolution help under lmxxf read "100 0.000000eeds the network"**: a literal percent sign in
  a format string. It reads "100% feeds the network" again.
- **The waiting status no longer names DirectX 12** ("AMD pre-SR: waiting for the first upscaler
  frame"): D3D11 and Vulkan titles reach the pass through the D3D12 bridge too, and the old wording
  read like an error on them.
- **lmxxf froze Vulkan titles at the first model frame (Indiana Jones and the Great Circle) - found
  and fixed.** It was not the driver (0.3.2's "HIP runtime 0" explanation was wrong: that 0 is a
  field danielblnc's runtime never fills when OptiScaler hosts it, on every machine). OptiScaler's
  Vulkan-on-D3D12 bridge queues its D3D12 work behind a fence the game only signals at its next
  vkQueueSubmit, and lmxxf's first job loaded its weights lazily with a synchronous HIP call inside
  that frame - a circular wait. On Vulkan titles the runtime now uploads the weights and runs the
  network once before the first fence wait (a one-time hitch of about 1.2 s, measured on an RX 9070
  XT; the first launch then takes 0.4 ms instead of 1.2 s), and it skips the queue drains the bridge
  cannot satisfy (a 30 s stall on every NR-resolution change is gone too). If a Vulkan session ever
  stops before its first answer, the next start runs danielblnc's runtime and says why
  (`lmxxf_vk_launch.pending` beside OptiScaler.dll; the Neural tab's **Retry lmxxf** button removes it). The 0.3.2 "HIP runtime 0"
  gate is removed; `amd_bridge.log` states the real HIP runtime and driver versions.
- **danielblnc on Vulkan titles (Indiana Jones and the Great Circle): Neural passes 2-3 now run as 1
  pass.** Each extra pass waited on the CPU for work that OptiScaler's Vulkan-on-D3D12 bridge runs only
  after the game submits its frame. That meant about 5 s per pass, and then NR stopped for the session.
  amd_presr.log says so once ("AMD vk: Neural passes N -> 1"). The menu shows a note under the slider and
  prices the cost line at 1 pass. D3D12 and D3D11 titles still run 2-3 passes. lmxxf runs all its passes
  on Vulkan titles, inside one runtime job, so it needed no change.
- **danielblnc on Vulkan titles: the 80 ms post-submit wait is gone.** On the Vulkan bridge the wait could
  never see the frame's job finish, because the job runs only after the game submits. It always ran out
  without retiring anything. It cost 80 ms on every frame with AmdSlots=1, and on the runtime's rebuild
  frames with the default 3 slots. amd_presr.log says so once. lmxxf has no such wait: on Vulkan it takes
  each answer with a bounded CPU wait.
- **The FSR Ray Regeneration controls were invisible on AMD** (they sat below a section the AMD menu
  never reaches). They are in the Neural tab's Quality section now, and show only while the game
  is running Ray Reconstruction (they hide a few seconds after the last Ray Regeneration frame).
- **lmxxf's RAM use climbed for as long as NR ran**: about 212 KB per network launch, roughly 45 GB an hour
  at 60 NR fps with one Neural pass and twice that with two. The HIP runtime keeps memory for every launch
  until the stream passes a marker, and the runtime never gave it one while frames ran. **RAM stays flat in
  GPU-bound games too.** A synchronize when the previous job has already finished was not enough on its
  own: a GPU-bound game queues the next job before the last one ends, so the stream is almost never idle
  (Silent Hill 2: 9035 syncs against 48563 skips, 14.8 GB private). The runtime now also records an event
  behind every job - it only queues a marker, never waits on the GPU, and changes no output byte. On a
  pipelined probe (RX 9070 XT, 1080p, 1-3 frames in flight, no idle syncs at all): 218 KB per launch
  before, flat over 18000 frames after, same picture (probe hash 51249b6b), median frame time about 0.1 ms
  higher. The lmxxf status line in the menu shows `idle_syncs`, `idle_sync_skips` and `release_marks`
  (one per job), and the `lmxxf stats` line in `lmxxf_backend.log` (every 600 frames, just before the
  `mem:` figures) shows the first two, counted since the network was last built; a skip means the network
  was still running at the next launch. In a GPU-bound game skips climb faster than syncs, and that is
  expected now - watch `private` in the `mem:` figures instead, and if it keeps climbing at fixed
  settings, please report it with `lmxxf_backend.log`. danielblnc is not affected: its closed runtime
  already synchronizes after every job.
- **lmxxf kept about 106 MB of VRAM for good on every network rebuild at the 1080 tier** (a change of NR
  resolution, game resolution, DLSS mode or dynamic resolution that changes the network size, or a queue
  change): a buffer list inherited from upstream held an extra reference to every buffer of 32 MB or more
  and never released it. It is gone (on the probe, about 70-100 MB less VRAM per resize cycle). What each
  rebuild still keeps is an AMD driver leak (Known issues).
- **lmxxf: a temporal history chain that failed to build leaked, and was rebuilt, on every frame** (up to
  about 115 MB per frame; never seen on a working install). What was built is freed now, and the failure
  is remembered for that motion geometry: the network runs without history and the status reads
  `hist=unavailable`.
- **lmxxf: a pixel whose alpha was not finite went black.** When any channel of the game's colour was NaN
  or Inf, the host's copy of it turned the whole pixel black. So a bad alpha alone could black a pixel whose
  colour was fine, both in the output and in what the network was fed. An FP32 alpha above 65504 did the
  same, because it becomes Inf once stored in FP16. Colour and alpha are now cleaned separately: a
  non-finite colour becomes black and keeps its alpha, and a non-finite alpha becomes 1 while the colour
  stays. The same copy handles 4-channel motion vectors, so a 4-channel motion texel whose only non-finite
  channel is .a now keeps its vector, as danielblnc always did; a non-finite x, y or z still zeroes it. RG
  motion formats and finite frames are unchanged. lmxxf only: the defect exists only in lmxxf's copy.
- **Highlight proxy (danielblnc, experimental) is decoded by the original, not by inverting the curve on
  the model's answer.** The answer is brought back with each pixel's original scale (RenoDX's bridge
  principle, clshortfuse, MIT). Low in the curve it hands over to the model's own answer decoded through
  the curve (AMDNR's). Results: exact where the model changes nothing (1 FP16 ulp); highlights above 200 no
  longer clip to 200; small model changes are no longer magnified (a 0.9x dimming at 100 now stays about
  0.86x instead of 0.05x); sparkles and fireflies the model removes stay removed. Above about 5x the knee a
  highlight's brightness follows the game's frame. Off by default, so the default picture is unchanged.
  lmxxf: no change, and none possible. Its host encode is linear (raw x exposure) and is already undone by
  the stored exposure. Its tonal proxy and decode live inside `LmxxfNrRuntime.pak`, which is not changed;
  that decode adds the original's headroom back above the proxy, so a highlight the network suppresses
  comes back there too.
- **danielblnc: the graphics wait no longer installs its state hooks.** Its admission could never pass:
  the graphics root signature is never tracked, and no log ever showed ADMITTED. Yet sessions on the 0.3.1
  runtime still paid for twelve mid-frame detours and a global lock on common command-list calls. The
  compute wait, which every session actually ran, is unchanged. `AmdGraphicsWaitExperimental` stays on by
  default, so the runtime starts exactly as before (it still builds its graphics-wait pipeline at init);
  amd_presr.log then says once "graphics wait unavailable in this build; compute wait used". lmxxf has no
  graphics wait.
- **Post-RR shimmer (danielblnc)**: after Ray Reconstruction (FSR Ray Regeneration) the temporal pass no
  longer shifts its history by the TAA jitter. RR's output is already resolved and carries no jitter, so
  the shift misregistered the history by up to a display pixel per frame. lmxxf skips it the same way.
- **XeFG pacing stopped at 6X**: its hooks assumed the provider caps there. It does not, so the opt-in
  7X/8X ran unpaced without a word. The hooks now pace every burst up to 10X.
- **A failed XeFG swapchain init kept its context** (roughly 0.8-1 GB of VRAM at 4K), and the next
  swapchain re-initialised that same failed context. The context is now destroyed, so the next swapchain
  starts from a fresh one. Only a context the failing call created is destroyed. The game still gets its
  plain swapchain, as before. One trigger is FP16 HDR swapchains, which XeFG refuses.
- **XeFG pacing kept a pointer into the provider's context after the context was destroyed** (resize,
  swapchain recreation), so its deadline hook could read freed memory until the next scheduled frame. The
  pointer and the burst schedule are now cleared on destroy.
- **XeFG no longer saves a lowered MFG count over yours.** When a session offers fewer frames than
  `[XeFG] InterpolationCount` asks for (for example 10X set, but the session held at 6X because Extra
  pacing was off), the count was lowered as a saved setting, and the next save wrote the lower count to
  the ini, so 10X stayed lost after pacing came back. The lower count now holds for that session only.
- **Frame Gen tab: no false "needs XeFG pacing" note** when OptiScaler's XeFG provider is the same
  libxess_fg.dll the game loaded first. That copy is unlocked as the game's own (6X at most), and the note
  blamed missing pacing even when pacing was installed. It now says the provider is the game's copy, and
  OptiScaler.log says so once.
- **Native D3D11 upscalers no longer over-release textures when a feature is re-created.** This affected a
  quality or resolution change with `Dx11Upscaler` set, with NR off, or on the FSR 2.2 fallback.
  OptiScaler's D3D11 helpers (reactive-mask bias, RCAS sharpening, output scaling, depth transfer,
  magnifier) released the game's colour, motion-vector and depth textures, which they had only borrowed.
  The bias helper also released its own buffer after it was already freed. The helpers now release only
  what they own. This is not on the default D3D11 NR route (FSR 3.X/4 w/Dx12), and neither runtime is
  involved.
- **A failed frame capture no longer leaves half-allocated readback buffers behind.** Before, the next
  capture reused them along with their stale footprints. This affects danielblnc only; lmxxf has no frame
  capture.
- **The `[RR_POST]` log line said `rcas=off` while RCAS was sharpening Ray Regeneration's output.** It read
  the sharpness after OptiScaler had set it to 0 to stop FSR sharpening a second time. It now reports RCAS
  as it runs: on or off, the strength used, and motion adaptive sharpening.
- **Streamline games: OptiScaler's plugin hooks can no longer turn a plugin that refuses to load into
  slInit error 0x18.** The six slOnPluginLoad wrappers (sl.dlss, sl.dlss_d, sl.dlss_g and OptiScaler's own
  sl.dlss_g, sl.reflex, sl.common) read the plugin's JSON without checking that it was set. For a plugin
  that refused to load, that was an access violation inside slInit, which Streamline reports as error
  0x18. They now share one guarded path. A missing or unreadable JSON makes Streamline skip that plugin,
  as it already did when the patch threw. The one exception is an sl.common JSON that cannot be read: it
  keeps Streamline's own JSON and version. Seen with
  NBA 2K27 on AMD, but not confirmed as its cause; the new [SLINIT] log lines are there to find it. Both
  runtimes (no NR code involved).
- **The loader hook's check for OptiScaler's own Streamline folder can no longer throw** on a path it
  cannot resolve. It answers "outside" instead; every other answer is unchanged. It runs for every DLL
  load, including Streamline's plugin loads inside slInit.
- **The NGX feature queries** (GetFeatureRequirements on D3D12, D3D11 and Vulkan, and the Vulkan instance
  and device extension queries), which Streamline calls during slInit, no longer dereference a null
  request: they answer InvalidParameter. The three GetFeatureRequirements also no longer write through a
  null result pointer.
- **Streamline's adapter table (SystemCaps) is looked up again at every plugin load** instead of being kept
  from the first one, so a second slInit or a plugin reload cannot write through a stale pointer.
- **On an AMD card, a Ray Reconstruction frame the AMD runtime did not process no longer logs "enable
  ApplyAfterRR"**, a switch for the NVIDIA path that cannot help there. The log gives the AMD reason
  instead: no runtime installed, GPU not supported, or the runtime did not run. NVIDIA cards keep the old
  line. Both runtimes.
- **Neural tab: the (?) help tooltips now open inside greyed-out sections**, such as the NVIDIA-only
  placement controls on AMD and Intel GPUs.
- **OptiScaler.ini and the menu's NVIDIA-path note said this package ships `nvngx.dll_dlssnr.dll` and
  `INSTALL-DLSSNR.md`.** It ships neither. The `[DlssNr] Enabled` comment now says what runs on AMD (lmxxf
  from this zip, danielblnc from Runtime.zip) and what the NVIDIA path would need.

### Changed
- **Interleave pacing is held off while XeFG runs at 3X or above**: its sleep could land in the middle of a
  burst. 2X and FG off are unchanged (and at the shipped default, interleave pacing does not sleep at all).
- **lmxxf updated to lmxxf 0.29's kernels**: four new kernels for each architecture (RX 9070 and RX
  9060), plus 0.29's production byte-stream options. Bit-exact: the network output is identical
  byte for byte. Network time at 1920x1080 on an RX 9070 XT: median 18.4 -> 16.7 ms with the
  options, about 20 -> 16.7 ms against 0.3.2. `LmxxfNrRuntime.pak` is rebuilt with them.
- **FSR Ray Regeneration: path-traced profile** (`[FSR-RR] FfxDenoiserPathTracedProfile`, menu
  checkbox; on automatically for Resident Evil Requiem and PRAGMATA). The fork's FSR-RR path was
  tuned on hybrid raster+RT games: part of every pixel went around RR as a spatial estimate and raw
  noisy colour was blended back after it - on full path tracing that puts grain back on faces. With
  the profile, all lit light goes into Ray Regeneration (as in AMD's own sample) and the temporal
  set moves to AMD's documented defaults (disocclusion 0.05). Values you set yourself still win.
  New log lines `[RR_PROFILE]` and `[RR_GUIDES]` say what ran and which guides the game publishes
  (skin/SSS guide, depth type, exposure) - the next RE9 log decides the skin-specific step.
- **UE5 robustness (lmxxf)**: a neural list or queue that reaches
  ExecuteCommandLists through a wrapper (Streamline and other proxies UE5 titles load) is matched
  by COM identity instead of being dropped or treated as a queue change; on a real queue change the
  old lmxxf session is released once its GPU work is done instead of leaking its VRAM; fence and
  event handling after a timeout is hardened.
- **The NR resolution slider moves in 5% steps** (both runtimes; rounded to the nearest step when you let
  go), because each new NR working size can hold VRAM until the game restarts (Known issues): fine tuning
  now lands on a few reusable sizes. Its help says so, and while it is dragged a line under it says what
  the runtime in use keeps. Values from the ini, NR styles and Dynamic NR are not rounded.
- **The NR hotkey is logged at INFO** (it was DEBUG), so every in-game NR on/off appears in a report.
- **"Frame count jumped too much!" is a warning only while frame generation is active** (debug
  otherwise). With FG off it made up about 87% of a TLOU log, one flushed write every other frame on the
  render thread. Frame pacing is unchanged.
- **Neural tab, D3D11/Vulkan hint:** it now names the backend to pick, "FSR 3.X/4 w/Dx12", as the
  Upscaling list shows it. It no longer sends Vulkan players to XeSS or DLSS w/Dx12, which the Vulkan list
  does not have.
- **DLSS-G titles are never told more than 6X**: the Streamline stand-in reports at most 5 generated
  frames, and its presented-frame count is held to 6. XeFG's opt-in 7X-10X therefore never reaches a
  title's DLSS-G logic. Unchanged at the default 6X.
- **A game's own XeSS 3 provider copy is unlocked up to 6X at most.** It used to get whatever
  MaxInterpolatedFrames said, up to 8X, but OptiScaler's pacing never runs on it. This applies when the
  game's copy is a separate libxess_fg.dll. An out-of-range MaxInterpolatedFrames set from the menu now
  falls back to 6X, not to the ceiling.
- **RunBeforeSR and ApplyAfterRR (with RRPasses and RRWorkingScale) are marked as NVIDIA-path-only** in the
  ini and the Neural tab. On AMD the pass always runs before Super Resolution, and after Ray Regeneration
  automatically when the game uses Ray Reconstruction. The AMD processing line shows the current placement
  and input. The Live section explains a failed Streamline slInit, and a pass that has had no frame for
  5 s. The missing-runtime note names LmxxfNrRuntime.dll + .pak.
- **Copyright:** OptiScaler.dll's version info (Properties > Details, LegalCopyright) reads "Copyright (c)
  2026 3zwr1 (AMDNR). OptiScaler (c) its contributors, GPL-3.0", and the menu header shows "AMDNR (c) 2026
  3zwr1 - see Licenses\AMDNR_NOTICE.txt" under the Discord / GitHub row. `Licenses\AMDNR_NOTICE.txt` is new
  in this zip; upstream notices are kept.

### Known issues
- lmxxf: changing NR resolution or DLSS mode many times raises VRAM until the game restarts (an AMD HIP driver leak on imported D3D12 buffers; 0.3.3.2 reuses these buffers, see there).
  Each change at the 1080 tier keeps about 97 MB of VRAM and as much RAM; restart the game after many
  changes.
- danielblnc: each new NR working size above about 1 MP keeps about 75-350 MB of VRAM per Neural pass
  until the game restarts (the closed runtime pools its interop buffers and never frees them; a size used
  before costs nothing). Read from the runtime's code and an RE Requiem log, not yet measured in a game.
  The slider's 5% steps (Changed) keep the number of sizes down; restart the game after many changes.
  0.3.3.2 rounds these sizes to 64 px away from 100%.

## 0.3.2 — 2026-09-23

What you reported on 0.3.1 in its first hours, plus frame generation for Vulkan titles.

### Added
- **Discord and GitHub buttons at the top of the menu**, under the status chips: support, reports
  and test builds on the Discord; releases on GitHub. One quiet line, opened in the browser.
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
- **GTA V (Legacy): `ERR_GFX_D3D_NOFEATURELEVEL_1` at start with OptiScaler loaded.** The D3D11 hook
  elevated the game's requested feature level 11_0 to 11_1 and the game refused the device. `gta5.exe`
  now keeps its own level (quirk `SkipD3D11FeatureLevelElevation`). Note: `GTA5.exe` never loads a
  local `dxgi.dll`; use `OptiScaler.asi` (ASI loader) or `winmm.dll` / `version.dll`.

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
  file (382 MB) beside the runtime DLL, shipped inside the AMDNR zip; decrypted in memory only. The `DLSS5-AMD\native-game-tiled-assets`
  folder still works and takes precedence when present.
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
third-party licences in `Licenses\`. The "c32w" RDNA 4 kernels (0.3.3.2) are AMDNR's own work, Copyright (c)
2026 3zwr1 (AMDNR); they run lmxxf's network (Kien, MIT), with thanks to AMD's public RDNA 4 WMMA documentation
(AMD GPUOpen, ROCm matrix instruction calculator) for ideas.
