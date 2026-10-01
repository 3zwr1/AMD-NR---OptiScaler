# Changelog

Many thanks to **TheAutomatic** (DLSS 5 AMD project) — the releases, the HIP toolchain and the asset
layout that the lmxxf runtime integration in 0.3.0 builds on.

**Attribution correction (2026-09-29).** AMDNR's Ray Regeneration path comes from *FSR Ray Regeneration
for OptiScaler* by **Zach Hembree (DarkHelmet)** (branch `ffx-denoise-experimental`, GPL-3.0), continued
by **burak113**, whose branch AMDNR ported from (commit `3da4808`, 2026-09-19). Earlier releases credited
burak113 only. The file headers, `Licenses\AMDNR_NOTICE.txt`, the README credits and the in-game credits
now name both. No entry below this line is rewritten.

## 0.3.5 — 2026-10-01

AMDNR Anywhere (preview), lmxxf 0.37 on RX 9000, faster on RX 7000 and Z1 Extreme-class handhelds, a Ray
Regeneration tab, the neural pass after upscaling, and a Frame Gen tab that says why nothing generates.
**Neural Rendering changes in this release:** lmxxf 0.37's kernels (by Kien, MIT) run by default on RX 9000, the
lmxxf module set for RX 7000 and Z1 Extreme-class handhelds is new (same picture, about 10 percent less network
time), `LmxxfNrRuntime.dll` changes on every card, and the resize and handheld-carry fixes touch both runtimes'
hosts. Coming from 0.3.4.x, **replace `OptiScaler.dll`, `LmxxfNrRuntime.dll` and `LmxxfNrRuntime.pak`
together** (the file you renamed, e.g. `dxgi.dll`, with the new `OptiScaler.dll` renamed the same way); your
danielblnc runtime files and your `OptiScaler.ini` stay. Launcher users: it updates for you. Requires AMD Software:
Adrenalin Edition 26.9.1 or newer.

**In short, for players:**
- **AMDNR Anywhere** (preview, RX 9000): Neural Rendering for games with no upscaler of their own, one PLAY in the
  launcher, nothing written into the game folder, with either runtime; since the first test builds it is paced to
  what the network can answer, shows each frame a steady time after its capture, leaves dark screens alone, runs
  its work off the host's render thread, and shows a hitch counter.
- **Faster:** lmxxf 0.37 on by default on RX 9000 (the RX 9060 / 9060 XT experimental, with off switches); about 10
  percent less network time on RX 7000 and Z1 Extreme-class handhelds, same picture.
- **Ray Regeneration** has its own tab; on RX 7000 it is off by default (an unsupported experimental opt-in).
- **Frame generation:** a game's own FSR frame generation keeps its own provider; the tab says why nothing
  generates.
- **Handhelds:** FSR 4 (INT8) as an experimental opt-in; no more blown highlights; one Neural pass; greyed rows.
- **Fixes:** dynamic resolution, Uncharted, GTA V Enhanced, Control Resonant, Assetto Corsa, F1 25, Kingdom Come II,
  mod loaders, HIP errors that now explain themselves; custom style slots keep the whole look.
- **AMDNR Launcher 0.3.5.1:** one PLAY with a route chooser, twelve languages, Rockstar games through their store.

### AMDNR Anywhere (preview)
- **Neural Rendering for games that have no DLSS, XeSS or FSR 2 of their own**, and nothing is written into the game
  folder. AMDNR runs inside a window-capture host - **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0),
  fetched from the author, not redistributed by AMDNR** - which captures the game's window, scales it to the screen
  with FSR 3 and hands the picture to Neural Rendering. In the AMDNR Launcher a game with no upscaler shows **PLAY
  ANYWHERE**: the launcher fetches the host from its author's release (467 MB, once), starts the game and captures
  its window. The menu draws inside the host (your menu key) with its own **Anywhere** tab; in an ordinary game
  nothing changes, row for row. **RX 9000 (RDNA 4) only in this release; RX 7000 follows once tested there.**
- **Host settings live in the Anywhere tab, not in the launcher:** the game window size (720p / 900p / 1080p; advice -
  the host captures what the game opens), the stage list (V1: one FSR 3 pass to the screen; V2: FSR 3 at 1x, then a
  fill pass), the NR tier (Auto / 720 / 900 / 1080), VRR, frame pacing and the **host frame rate** (Default = the
  display's refresh rate, at most 60; `auto` = the largest rate the network sustained in the last session, read from
  the `anywhere_budget.ini` the runtime writes beside its log; 30 to 120; Display refresh = no cap). They apply at
  the next PLAY ANYWHERE, and the launcher shows a read-only summary beside the button. Keys, in the host's own ini:
  `[DlssNr] AnywhereWindow`, `AnywhereEffect`, `AnywhereNrTier`, `AnywhereVrr`, `AnywherePacing`,
  `AnywhereHostFps`; Restore Anywhere defaults clears them; one Save Settings, at the bottom.
- **What a captured window cannot give the network** - the game's depth, motion vectors, jitter and exposure - the
  host says so instead of pretending: the Anywhere tab names what is estimated; the host's reactive mask (a constant
  0.2) is ignored for the carry and its exposure placeholder is treated as absent (no exposure scan there); Screen
  GI and RTGI are refused; **Model interleave is refused** (it would spend the carried edit on estimated motion; the
  row is greyed at Off with the reason, and your own `AmdInterleave` value is never rewritten); the carried edit
  fades where the host's flow does not explain the picture (`[DlssNr] AnywhereSyncEdit=true` cuts instead of fading,
  an A/B); full network is on; output smoothing runs at lmxxf's own Magpie values (0.8 at 10/255; a set
  `AmdLmxxfOutputSmooth` wins); Game exposure shows what runs. The Neural tab's rows that act in the host keep their
  help with a host footnote (Model interleave, Dynamic NR, Temporal stability, Game exposure).
- **The picture and the budget, honestly.** The NR resolution row shows the network's real size inside the host
  ("1.00 (network 1280x708, capture 2554x1412: 0.25x) - Fast mode: one tier lower"), and one line on the Anywhere
  page says when the network is over the host's frame budget - "Network X ms of Y ms per host frame (N fps)" with
  "the host holds at half rate" or "the network cannot keep up", where the rate came from, and what to lower. A
  window larger than 1920x1080 is fed below the network's ceiling and the page says it; when the picture was brought
  down before the network and upscaled back (the 1080 tier on a 1440p window) the page says so, with the share of
  the screen's pixels the network saw. **A 1920x1080 or smaller game window is the pixel-exact case.** The title bar
  and the report name the scaled game.
- **The host is kept out of the in-game hooks:** no window-procedure replacement, no D3D11 feature-level override,
  the Win32 detours wait for the first menu open, only the first upscale context goes to Neural Rendering, and a
  D3D11 context is never handed to a D3D12 upscaler feature. Menu input inside the host: the cursor is driven from
  raw mouse input (the game underneath recentres the system cursor), the arrow keys / Tab / Enter / Escape drive the
  menu, and keyboard and mouse stay inside the host while the menu is open. Frame generation and a motion cap
  (`AnywhereFg`, `AnywhereMotionMaxPx`) are offered in the tab; the frame-generation row says whose stage it asks
  for (the host's own XeSS frame generation) and applies at the next PLAY ANYWHERE.
- **The picture since the first test builds.** At 1:1 AMDNR writes the host's output itself and the host's FSR 3
  temporal pass is skipped (no CAS either: the launcher's V1 chain never adds it); by default each frame shows the
  captured picture with its own answer (the delayed exact edit), so the edit never lands on the wrong frame and the
  host never waits for the network; the network's history chains across repeated frames (the vectors are kept and
  accumulated since the last feed); the network's exposure eases toward the host's auto-exposure loop, and the loop
  keeps the highlights under the codec's shoulder (no washed-out car in bright scenes).
- **The pace since the first test builds.** The host's base rate is a rate on your display's refresh grid that the
  network answers every frame at, and the Anywhere page says the next one and why; inside a session it is paced
  live - a step down only for a scene that stays too heavy, a 10 s dwell, back up only with room - so it no longer
  bounces; at the floor the page offers the smaller tier (never silently). The network's input runs on its own
  high-priority compute queue and its HIP stream at the device's highest priority, and the launcher lowers the
  game's GPU priority, so the host and the network go first.
- **Nothing of ours holds the host's render thread:** the runtime loads, builds and warms the network on a thread of
  its own, the feed list is never waited for, the live pace never waits after a late frame nor over 50 ms, a
  still-picture gap keeps the history, and a **Hitches** counter on the page, in the stats line and in
  `anywhere_budget.ini` says what is left.
- **Start-up and Auto tier:** the host's start-up leaves its render thread (the first frame's setup ran there for
  about a second), the pace settles once, 2.5 s after the first answer, on a finer rate grid, and **Auto tier**
  (`[DlssNr] AnywhereAutoTier`, on by default) steps down to the next smaller NR tier once when the GPU cannot hold
  the chosen one.
- **Dark screens are left dark:** a black screen with a logo is no longer lifted by the host's auto-exposure, and the
  edit fades out on very dark pictures (a played scene is untouched); with frame generation the pace uses only the
  rates that frame generation can keep.
- **DLSS-NR on AMD by Daniel Blanco (danielblnc) inside Anywhere** gets what lmxxf has there: the settled pace, the
  hitch counter, the budget file (`Backend=daniel`) and the dark-screen fade; the Anywhere page says "same frame
  (danielblnc)" with his network line.
- **Timestamp pacing:** each frame is presented at its picture's capture time plus a steady latency (the 95th
  percentile of the pipeline's own capture-to-present time, moving at most 0.5 ms at a time), never past the next
  frame's slot; a repeated frame is never held. `[DlssNr] AnywhereTimestampPacing` (default true, ini only).
- [PENDING j13] **Fine detail kept sharp:** the NR edit is lifted with a detail-aware upsample and output smoothing
  acts on the edit only (the picture's own detail is never softened).
- **Preview limits:** the host's motion is estimated over a captured window, so a light flicker or judder in fast
  motion can remain (lower the NR tier to 720, cap the game at 60-90 fps with its own limiter, keep Model interleave
  off); the game window must be windowed or borderless; a 1440p or 4K window is fed below the network's ceiling. The
  capture host is fetched from its author's GitHub release, not from ours.

### Neural Rendering
- **Neural pass after upscaling.** `[DlssNr] AmdPlacement = pre | post` (Neural tab > Performance > Placement, both
  runtimes, through the shared bridge). `post` runs the network on the upscaler's finished display-resolution
  picture instead of the render-resolution input, and writes its answer back into it: sharper, because the upscaler
  no longer re-filters the edit, and more expensive - the network runs at display size up to its 1920x1080 ceiling,
  so a 1080p screen pays the top tier at any NR resolution and a 1440p or 4K screen gets a 1080-line edit lifted
  back - a little less forgiving in motion (render-grid vectors on a display grid), and the HUD is included if the
  game composites it before upscaling. The NR resolution row says both sizes while `post` runs ("after upscaling:
  network 1920x1080 of 2560x1440"). Refused, with one line in `amd_bridge.log`, inside AMDNR Anywhere, in final image
  mode and once Ray Regeneration has run in the title (its own hand-off already runs the pass after the upscaler;
  a title that turns Ray Reconstruction on halfway through moves to `pre` once and never back); an unknown value
  reads as `pre` and says so once. **Default unchanged: `pre`** - with the key unset every session routes exactly
  as 0.3.4 did.
- **Faster on RX 7000 and Z1 Extreme-class handhelds, same picture.** The 0.3.5 module set in `LmxxfNrRuntime.pak`
  (the g2 operand-assembly build of lmxxf's kernels for gfx1100 / gfx1101 / gfx1102 / gfx1103) takes about 10 percent
  off the network time - about 11 percent on an RX 7800 XT, about 10 percent on a Z1 Extreme at the 360p and 576p
  sizes - with a bit-identical picture, measured and hash-checked by testers on both. The Z2 Extreme / 890M / 880M
  (RDNA 3.5) and Strix Halo modules are byte for byte 0.3.4.2's; so are RX 9000's base modules, with lmxxf 0.37's
  added beside them (next point). The runtime's handheld hints now carry
  the measured Z1 Extreme numbers (about 50 ms at the 360p size, about 105 ms at 576p).
- **lmxxf 0.37 by Kien (MIT), on by default on RX 9000.** lmxxf 0.37's gfx12 modules load beside the base set as
  one pinned group (all or nothing, SHA-256 checked, with a silent fallback to the base set), together with its host
  launch paths for the new kernels (four of six on by default). On an RX 9070 XT, measured outside a game, the
  network time drops by about 20 percent at the 1080 size (14.0 -> about 11.3 ms) and about 15 percent at 900 and
  720; the picture is not bit-identical to 0.3.4.2's (the kernels round differently). The lmxxf status line's `l37=`
  token names what runs. Off switch: `[DlssNr] AmdLmxxfL37=false` (or `AMDNR_L37=0`).
- **RX 9060 / 9060 XT (gfx1200), experimental on that card:** lmxxf 0.37, AMDNR's one-wave C32 kernels (c32w) and
  the FastK kernel groups are on by default there too, as on the RX 9070 / 9070 XT; none of them has run on a
  9060 yet. Off switches: `[DlssNr] AmdLmxxfL37=false`, `AmdLmxxfC32w=false`, `AmdLmxxfFastK=false`.
- **Custom style slots keep the whole look**: a slot now stores 62 typed settings under their ini keys - the
  Quality sliders, stability, history, interleave, model strength, exposure and highlights, encoding, colour
  composition with its RenoDX and skin keys, and the whole appearance filter with its tone curve - not only the
  quality sliders; a 0.3.4 slot string still reads, and performance and backend keys are never stored.
- **Dynamic resolution no longer rebuilds the network at every step** (both runtimes). The Last of Us Part II made
  233 size changes in 32 s under lmxxf, each restarting the 300 ms settle and resetting the history (NR on one frame
  in five); under danielblnc every Dynamic NR step was a full resize. A frame that stays inside the allocation - no
  larger, above 75 percent of it, the same NR cost band - is now held: no settle, no warm-up, no history reset, the
  network size kept; a larger frame or a real drop reallocates at once, a lower band after 30 stable frames, and an
  NR scale step moves only the network size. One rate-limited log line; the report's stats line and the Live row
  count the held steps. Fixed-size titles are unchanged.
- **Handhelds: the carried edit no longer vanishes when you move.** With Model interleave on a Z1 Extreme the
  network runs on frames of unequal length; the carry's divergence guard assumed equal frames and threw two thirds
  of the edit away on every moving frame ("when you move the effect is gone"). It now reads the previous frame's
  vectors at this frame's duration (both runtimes; only upward, 1 with Model interleave off, on a reset frame and
  after a pause, clamped at 4x); every frame no longer than the previous one is what it was. One log line the first
  time the sawtooth exceeds 1.5x.
- **Handhelds: bright coloured highlights no longer blow out** with the appearance filter's tone curve (fire in
  Shadow of the Tomb Raider): Saturation above 1 eases back in bright highlights and no channel ends above its
  pixel's own brightest channel or about 3.8x paper white (both runtimes). **Neural passes run as 1** on a handheld
  APU (the row says so and the NR cost counts one pass), and the rows that cannot act are greyed with the reason.
- **FSR 4 (INT8), experimental opt-in on RDNA 3 handhelds and APUs** (gfx1103 / gfx1150 / gfx1151): the Upscaling
  tab's box **FSR 4 (INT8) - Experimental on this GPU (restart)** writes `[FSR] Fsr4ForceModel=2`; unticked by
  default, not validated by AMD, about 1.5-3 ms per frame on a Z1 Extreme (estimate); the log names the upscaler
  DLL, the forced model and the FSR 3 fallback reason.
- **danielblnc runtime: your own `Async` key reaches it.** The host forced the same-frame mode on every hosted
  danielblnc build; on danielblnc 0.5.0 that mode failed from the second job on (RX 9060 XT, Control Resonant: every
  later job a sticky launch failure, then a crash), while the same runtime loaded by the game as his own proxy ran
  clean. The `Async` key of `dlssnr_on_amd.ini` (and the older `Inline` spelling) is read and passed through; with
  neither key the byte is what 0.3.4.1 wrote, so a default install is unchanged. **danielblnc 0.5.0**
  (`v0.5.0-Runtime.zip` on the Alpha0.3.4.2 release) is the recommended danielblnc runtime on RX 9000 and RX 7000
  and the launcher's first choice; 0.4.3, 0.4.1 and 0.4.0 stay accepted. A danielblnc build newer than 0.5.0 is
  not driven by this release.
- **"No HIP adapter matches D3D12 LUID" explains itself.** Both runtimes write the same refusal: the game's D3D12
  adapter (name, LUID), every HIP device (ordinal, name, gfx, LUID) and the likely cause - the game on the
  integrated GPU or another card, a HIP runtime without identity (a stray `amdhip64_7.dll`), no HIP device, the same
  card under another identity, not an AMD adapter - with what to do (the Windows Graphics setting, the driver). The
  bridge reads every DXGI adapter once and writes the list to `amd_bridge.log`; the Neural tab says when the game
  renders on the wrong one, and on a laptop APU beside a Radeon it no longer claims NR is off while NR runs on the
  APU (three wordings: no runtime yet, a runtime on the integrated GPU, a runtime on another adapter).
- **One runtime folder for many games.** `[DlssNr] AmdRuntimePath` names a folder that holds the danielblnc pass
  DLLs and weights, or `LmxxfNrRuntime.dll` + `.pak`; every game reads from it (about 455 MB saved per game). Unset
  = the game folder, exactly as before; a relative value is taken from the game folder, `%VARIABLES%` are expanded,
  a value that is not a folder is refused with one log line, and a file the shared folder does not hold is taken
  from the game folder with a WARN line naming it. Read at game start, ini only.
- **The runtime checks its modules.** `LmxxfNrRuntime.dll` hashes every HIP module in the pak against the pak's own
  digest list before use and refuses a mismatch by name instead of running an unknown module (about 4-11 ms once per
  start; `AMDNR_VERIFY_MODULES=0` for the lab; the status line reads `vm=0` while it is off). It also stops reading
  lmxxf's picture-changing environment variables: every default is 0.3.4.1's, so a host that sets nothing gets
  0.3.4.1's output. The lmxxf rebuild lines carry a one-word reason (`tier`, `colour`, `controls`, `policy`).
- **Typeless display-sized motion vectors are accepted** by the danielblnc host (Tainted Grail: The Fall of Avalon,
  Dead Space; UE5 / Frostbite hand an `R16G16_TYPELESS` velocity target) instead of latching NR off for the whole
  run; the four typeless families read through their float view, as every other guide path already did.
- **The GPU verdict comes first.** On a chip neither runtime runs on (RX 5000 / 6000, Steam Deck, the small APUs)
  the status line no longer blames a format, and the runtime combo's "not for this GPU" tag shows for both runtimes,
  with the reason on hover ("Pick <other> instead." when the other runtime runs on this chip, else "No neural
  runtime runs on this GPU.").
- **Uncharted: Legacy of Thieves Collection (RX 9000)** no longer crashes at the first NR frame: the neural bridge's
  first HIP use runs on its own large stack (`[DlssNr] BigStackCall`, auto) instead of the game's 192 KiB job fiber.
  Built and covered by this release's tests; a player's confirmation on RX 9000 is pending. If you set
  `[DlssNr] Enabled=false` there for 0.3.4.2, turn it back on.
- **DX11 games that never present through D3D12** get a bootstrap D3D12 queue for the neural pass
  (`[DlssNr] AmdBootstrapQueue`, auto).
- **New splash lines** on the menu toast: 108 one-liners about AMDNR itself - no version, no date, no promise,
  nobody named.

### Ray Regeneration
- **Its own tab, right after Upscaling.** Ray Regeneration is the denoiser that answers a game's DLSS Ray
  Reconstruction call, not Neural Rendering, and with 22 keys it was the longest section on a tab it did not belong
  to. The tab is drawn while Ray Regeneration is running in the title (latched for the session, so the tabs do not
  shift under your cursor when the game leaves a ray-traced scene) and never inside AMDNR Anywhere. Nothing was
  rewritten to move it: the same rows, the same defaults, the same ini keys. The Neural tab keeps one dim line and
  an **Open Ray Regeneration** button while the tab is up (a real button, for keyboard-only players) and, while the
  tab is down, the whole four-state status from 0.3.4.2. The RR debug view moved with it, as the tab's own
  Diagnostics block.
- **The status lines tell the truth per card and per API.** On RX 7000 Ray Regeneration is not offered by default
  any more (next point) and the game keeps its own denoiser; the refusal reason when Ray Regeneration could not start
  on a GPU is named on both tabs. On a **Vulkan** title (RTX Remix games such as Half-Life 2 RTX, id Tech 8) Ray Reconstruction is
  answered "not supported" by design - the denoiser is D3D12 and the ray-tracing buffers stay on the Vulkan device -
  and the tab says so instead of asking you to turn Ray Reconstruction on in a game that greys it.
- **RX 7000: not supported, off by default, an experimental opt-in.** AMD's denoiser has one provider, for RDNA 4:
  on an RX 7000 every start ended in "No FSR-RR denoisers were found" and FSR without any denoiser, noisier than the
  game's own (RE Requiem and Crimson Desert reports, RX 7800 XT, with the key unset and forced `true` alike). With
  `[FSR-RR] FfxDenoiserAllowPreRdna4` unset (`auto`, as shipped) Ray Regeneration is no longer offered on RDNA 3 /
  3.5: the game keeps its own denoiser, and there is no popup and no un-denoised picture. The Upscaling tab's box on
  those cards is now **Experimental: Ray Regeneration on this card (restart)**, with a dim "experimental - not
  supported" tag, unticked by default, and its hover says what it does today: ticked, the denoiser still refuses to
  start and the picture falls back to the upscaler without a denoiser. For testing only; AMDNR's own denoiser for
  this card is planned. RX 9000 is unchanged, and an explicit `true` / `false` in your ini still wins (a `true` you
  set yourself stays the experiment: untick the box to go back). The log no longer calls `false` the default on
  RX 6000 and older (`auto` is).
- **Denoiser backend: Automatic / Off.** `[FSR-RR] RrBackend = auto | off`: `off` tells the game Ray Reconstruction
  is unsupported at every gate a title asks (both Streamline answers, the NGX capability table, the feature
  creation), so it keeps its own denoiser - what the status line used to ask you to do inside the game, done from
  here. The game decides when it starts, so the row says "after a restart" while the choice differs from what this
  run was told; an unknown spelling reads as `auto` and is logged once. Save Settings keeps it. The card gate is
  untouched.
- **The noise number.** While the tab is open (or `[FSR-RR] FfxDenoiserDiagnostics` is on) a probe measures, on a
  64x64 grid, the grain of the colour the game handed in and of the picture that leaves Ray Regeneration for the
  upscaler, and the still-camera flicker between two captures while the view held still; the tab's Diagnostics block
  shows grain in -> out with its band and the flicker line, and `[RR_NOISE]` logs it every 5 s. A closed tab costs
  nothing. No denoiser tuning changed and no default moved: every Ray Regeneration number in this build is
  0.3.4.2's.
- **More Ray Regeneration options are live on the tab** (greyed while `FfxDenoiserUseAmdDefaults` is on); a value
  typed with Ctrl+Click is clamped to the slider's range.
- **Credits:** Zach Hembree (DarkHelmet) is named as the author of FSR Ray Regeneration for OptiScaler in the
  headers, the menu credits and the NOTICE; burak113 continued the branch AMDNR ported from.

### Frame generation
- **A game's own FSR frame generation keeps its own provider.** On RX 9000 a game that runs its own FSR 3.1 frame
  generation through the FidelityFX API had it swapped for the driver's ML frame generation by AMDNR's provider
  override; the game's own frame-generation context now keeps its FidelityFX DLL's implementation, and the log
  names it once per session (with a warning when an AMDNR FG pair is set as well).
- **The tab and the log say why nothing generates.** The audit of every frame-generation report found one crash and
  nine "FG never switched on". The Frame Gen tab now names all five steps - pick FG Input and FG Output, **Save
  Settings**, **close the game fully and start it again**, turn the game's **own** frame generation on (DLSS FG
  with DLSS as the upscaler for the DLSSG input, FSR frame generation for the FSR 3.1 FG input), then tick
  **Active** under Frame Generation - and one orange line says which one is missing: one half of a pair, a pair this
  game's API cannot run (DX11: only OptiFG; Vulkan: no FSR FG / XeFG output), or a live swapchain with Active
  unticked. The log writes it too, once after 600 presents and again only when the reason changes: *"XeFG swapchain
  is live but [FrameGen] Enabled is off ...: no frames are generated. Frame generation off, not broken."* A game
  without frame generation says so at debug level instead of the old warning.
- **A game that loads its own XeSS Frame Generation** (RE Engine, Crimson Desert, F1 25) gets an advice note - leave
  the game's own XeSS FG off, or two generators share one window - and AMDNR's XeFG output still runs there (until
  now the note switched the output off, and never fired in any shipped build because it was tested before the
  proxies were checked). `AllowXeFGWithNativeXeFG=true` hides the note.
- **Control Resonant no longer dies with frame generation.** A second swapchain on the same window took the wrong
  path and one swapchain ended up with two owners (the picture died 0.4 s after the game's second swapchain call).
  The title now runs without DXGI spoofing and without the preserved swapchain. A player's confirmation is pending.
- **Vulkan titles:** the Frame Generation tab says that AMDNR's own frame generation is D3D12 only and the game's own
  is the way (DLSSG via Nvngx only hands the call to a replacement dll); nothing is created on the Vulkan path that
  was crashing before.

### Fixed
- **Assetto Corsa**: AMDNR steps aside from a foreign `nvngx.dll` in the game folder and guards D3D11On12 device
  creation. Built and tested; a player's confirmation is pending.
- **F1 25, Kingdom Come: Deliverance II (Game Pass)**: a full-path `System32` request for AMDNR's own proxy name is
  answered with the genuine system DLL in every game (first written for F1 25 alone, now unconditional), and AMDNR
  pins itself once at start, so the Xbox overlay's probe can no longer unmap the mod while its hooks are live
  (access violations inside D3D12Core / amdxc64). Kingdom Come: Deliverance II also runs without DXGI spoofing
  (CryEngine: no Reflex, no DLSS FG; on Game Pass the 4090 name died inside the game's own D3D12CreateDevice).
- **GTA V Enhanced** runs without the DXGI spoof: the NVIDIA name made RAGE re-run its auto-config and switch ray
  tracing on (`ERR_GFX_STATE`, written back into `settings.xml`). FSR 3.1 picked in the game is the input; Neural
  Rendering runs before it; the game keeps its own FSR frame generation. A player's own `[Spoofing] Dxgi` still
  wins.
- **Anti-cheat and crash-reporter processes** (EA Javelin, Easy Anti-Cheat, BattlEye, Vanguard, the Unreal / Unity /
  crashpad reporters, REDengine's pre-launcher and error reporter, Steam's reporters and overlay - eighteen exact
  exe names) get the plain pass-through: the genuine DLL is served and no log, hook or report is written there; no
  ini setting can put AMDNR back inside one of them. Seen with EA SPORTS FC 27, where AMDNR ran a whole session
  inside EA's anti-cheat launcher; that game has no DLSS files where AMDNR looks, so Neural Rendering never ran
  there - Anywhere is the route.
- **Linux / Proton**: a count query through the emulated AMD device (the FSR 4 upgrade path with
  `[Spoofing] Dxgi=true`) no longer faults at start - the two-call API's first call passes no array.
- The build-time DXBC for the host passes (identical shader bytes; `AMDNR_HOST_DXBC=0` compiles at run time, and the
  build says when the embedded DXBC is stale).
- Line endings and project-file listings for the new headers (no behaviour change).

### Linux / Proton
- The Neural tab and the report say that both runtimes need the Windows AMD driver's HIP and cannot run under Wine /
  Proton, instead of "Idle". Ray Regeneration stays a known issue there (magenta patches in some games); Ray
  Reconstruction and AMDNR's own frame generation on a Vulkan title are refused by design, as on Windows.

### Diagnostics
- **HIP error 100 (no device) names its cause** in `amd_presr.log` and the report's `[GPU]` lines: a HIP runtime
  out of step with its compiler after a driver update (or a pending reboot), a `*_VISIBLE_DEVICES` variable left
  by an AI tool, or a shim DLL - with what to do.
- **Other mods' loaders** (Cyberpunk 2077 and friends): RED4ext (`winmm.dll`), Cyber Engine Tweaks (`version.dll`
  through an ASI loader) and the other proxy-name loaders are named in `report.txt` (`Mod loaders:`) and the log -
  by the module's version resource, never by the file name; a same-named dll chained from `OptiScaler\plugins`
  keeps a mod alive - and a warning fires when AMDNR holds a known loader's name of this game unchained. Every
  proxy dll sitting in `AMDNR_backup` is listed as "moved aside by the launcher".
- **ReShade beside AMDNR** is detected by the module's version resource or ReShade's add-on exports, never by a file
  name (a bare proxy name is no evidence: `dxgi.dll`, `d3d12.dll`, `winmm.dll`, `version.dll` and `dbghelp.dll` are
  the names AMDNR, the DLSS Enabler, SpecialK, Luma and REFramework all use), and named in the report and the log:
  `[Game] ReShade detected: <module>`. Nothing is loaded, hooked or blocked; making the two share the D3D12 device
  is designed, not built.
- The report's build line reads `AMDNR 0.3.5`; inside Anywhere the title names the scaled game; `amd_bridge.log`
  lists every adapter once and names the one the network was built on; the lmxxf status line carries `proc=` and
  the rebuild reason.

### AMDNR Launcher 0.3.5.1
- **PLAY ANYWHERE** on a game with no upscaler of its own (see above) and a read-only summary of the host settings
  beside it; games the mod cannot load into (32-bit, DirectX 9 / OpenGL without an upscaler) are refused at INSTALL
  with a clear line and offered Anywhere.
- **UPDATE ALL** and update-at-start; package mirrors; RETRY / OPEN DOWNLOAD / IMPORT PACKAGE when a download fails.
- **COLLECT LOGS** turns the crash handler on for one run and carries `amdnr_crash.log`, the newest dump and
  danielblnc's `dlssnr_on_amd.ini`.
- **PLAY** starts Xbox / Microsoft Store games by their app id.
- **One PLAY** with a route chooser (the game's own upscaler or AMDNR Anywhere; it remembers your pick per game)
  and the graphics API tag (DX9 / 10 / 11 / 12 / Vulkan / OpenGL) on the game page.
- **Twelve languages**: Turkish, Korean and Hungarian join English, Arabic, Chinese (Simplified), French, Spanish,
  Portuguese, Italian, Russian and Polish.
- **Rockstar titles start through their store** (Steam, Epic or the Rockstar Games Launcher), never by their exe,
  and the launcher waits for the real game window (GTA V `ERR_NO_LAUNCHER`).
- **AMDNR Anywhere:** the game runs below normal GPU priority while the host and the network go first
  (`AnywhereGamePriority`); the capture host's tray icon is hidden (the AMDNR menu controls it).
- **Resident Evil 2 / 3 / 4 (2023) / Village:** a setup notice - they need PureDark's upscaler plugin with
  REFramework (linked, not included).
- **More games marked unsupported** (no DLSS, XeSS or FSR 2+, among them Skyrim, Fallout 3 / New Vegas / 4, Elden
  Ring, Sekiro and Assassin's Creed Valhalla): the reason shows on the game page and INSTALL refuses before any
  download.
- **FSR 4 (INT8) opt-in** on the game page of RDNA 3 handhelds (the same key as the menu box).
- Only the danielblnc runtimes he has published are offered.
- **PLAY ANYWHERE is the player's choice:** INSTALL / DON'T INSTALL at first start and in SETTINGS; not installed,
  nothing of it is downloaded; turned off, the host is stopped and what it downloaded is removed (your Anywhere
  settings are kept if you tick "Keep my Anywhere settings").
- **A Fix button on each Doctor finding** that names a fix (REPAIR / UPDATE, RESET INI with its backup, UNINSTALL
  with its question, RESCAN, the REFramework page).
- **Anywhere starts at last session's settled rate** only when the same runtime and NR tier ran then, and the
  session log says how fast the game draws, with a note to cap the game when it draws far more than the host shows.
- **Other mods' loaders are never taken or moved**: the launcher picks another proxy name (Cyberpunk 2077:
  `dxgi.dll`) and its Doctor names any loader an earlier INSTALL moved aside.
- **GTA V Enhanced**: the game page carries the route line and a note about `settings.xml`.
- The launcher itself updates first; then every AMDNR game shows "Update available" and UPDATE ALL installs 0.3.5
  over 0.3.4.x, keeping the DLL name and your `OptiScaler.ini`; the danielblnc runtime files are never touched.

### Docs
- README: an **AMDNR Anywhere** section; the Ray Regeneration tab; the Placement row; the frame-generation FAQ (the
  five steps); FAQ entries for other mods / ReShade, "No HIP adapter matches D3D12 LUID", Uncharted (fixed),
  Assetto Corsa (0.3.5) and dynamic-resolution games; the hybrid-PC entry says when NR runs on the integrated GPU;
  the Linux / Proton section; the Roadmap (0.3.6: RX 6000, the AMDNR denoiser preview); the Magpie credit; the
  launcher section for 0.3.5.1. All nine languages.
- `OptiScaler.ini` documents every 0.3.5 key: `AmdPlacement`, `AmdRuntimePath`, `BigStackCall`, `AmdBootstrapQueue`,
  `RrBackend` and the `Anywhere*` host keys.
- `Licenses\AMDNR_NOTICE.txt`: the 0.3.5 highlights, the Magpie line (the host is fetched from its author, never
  included) and the ReShade line.

### Known issues
- AMDNR Anywhere is a preview (its limits are listed above). In heavy, uncapped games (GTA V) cap the game with
  Radeon Chill (Min = Max) for steady fps.
- Capture adds delay: inside Anywhere each frame is shown a little after the game drew it.
- Fast mode and a high Output smoothing make the picture softer.
- Frame generation needs the game's own frame generation as the input (DLSS FG or FSR 3.1 FG), or OptiFG in a DX12
  game without any; DX11: OptiFG only; Vulkan: no FSR FG / XeFG output. XeFG needs borderless, not exclusive
  fullscreen. No fps gain = FG is off (the tab and the log say which step), not broken.
- RX 7000 and Ray Regeneration: not supported. AMD's denoiser is RDNA 4 only, so Ray Regeneration is off on RX 7000
  by default and the game keeps its own denoiser. The Upscaling tab's experimental opt-in
  (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) is for testing only: the denoiser refuses to start and the picture falls
  back to FSR without a denoiser (the tab says what happened). AMDNR's own denoiser for RX 7000 is 0.3.6 work.
- Linux / Proton: Neural Rendering needs the Windows AMD driver's HIP; FSR upscaling works; Ray Regeneration can show
  magenta patches in some games (unchanged).
- A danielblnc build newer than 0.5.0 is not driven by this release; 0.5.0, 0.4.3, 0.4.1 and 0.4.0 are.
- The post-upscaling placement and the Control Resonant, Uncharted, Assetto Corsa, Fall of Avalon and GTA V Enhanced
  fixes are built and covered by this release's tests; player confirmations are pending where marked above.

## 0.3.4.2 — 2026-09-28

Hotfix for the menu in Assetto Corsa (the menu key toggles once per press, clicks land, and the runtime chooser can
no longer trap you), the runtime chooser no longer opens the menu by itself, the **Ray Regeneration** section of the
Neural tab is always there now and says why it is not running, and the Wine / Proton text says what the 0.3.4.1 notes
say. AMDNR also **accepts one more danielblnc runtime layout**, so a newer danielblnc
build can be driven without an AMDNR update. **Neural Rendering is byte-identical to 0.3.4.1 except that one
accepted-layout row: the neural pass, both runtimes and the pak are unchanged; only `OptiScaler.dll` and the
documents change.** Coming from 0.3.4.1 or 0.3.4, replace `OptiScaler.dll` only (the file you renamed, e.g.
`dxgi.dll`); `LmxxfNrRuntime.dll`, `LmxxfNrRuntime.pak` and your danielblnc runtime files stay as they are. Launcher
users: it updates for you.

### Fixed
- **Assetto Corsa: the menu.** The second report (an RX 7800 XT at a low frame rate): the menu opened by itself on
  the runtime chooser, clicks did nothing, and Insert hid the menu only while it was held.
  - **The menu key toggles once per physical press.** It acts on a fresh press, never on the release, and a key
    message that arrives late (Assetto Corsa hands them over up to about 2 s late) is stale and ignored. The 0.3.4
    400 ms debounce stays as a backstop, and a key that closed the menu is held back from the game until it is
    released.
  - **Clicks and menu keys shorter than a frame are no longer lost.** At a low frame rate a click could begin and
    end between two frames, so the menu never saw it. Both edges are now kept and replayed inside one frame, for the
    mouse buttons and the menu's navigation keys while the menu is open.
  - **The runtime chooser** ("Choose the neural runtime") answers to the keys `1` / `2` (pick a runtime), `Enter`
    (use it) and `Esc` (Decide later), has an X in its title bar, preselects the runtime that is running now, and
    **closing the menu (Insert) counts as Decide later**. It comes back at the next game start until you pick one.
  - **Found in review, fixed before release:** with `[Menu] OverlayMenu=false` and frame generation on, the menu's
    input poll ran between frames, so a replayed click could arrive twice (a double-click); the replay now counts
    presses since the key's previous poll. Also from that review: a menu key message sent from another thread is
    timed correctly (the key could look dead), and while the menu key is still held after closing the menu it is
    hidden from the game's DirectInput reads and keyboard hooks too, until it is released.
  - To get 0.3.4.1's menu key and clicks back (no new key): add `DiagInputHooksSkip=presslatch,clickreplay` under
    `[Hotfix]` in your `OptiScaler.ini` yourself; the shipped ini only describes the line in a comment.
    The fix is built and covered by this release's tests; confirmation from the Assetto Corsa player who
    reported it is still pending.
- **The runtime chooser no longer opens the menu by itself.** With files for both runtimes present and
  `[DlssNr] NrBackend` not set, 0.3.4 and 0.3.4.1 opened the menu on the chooser at the first NR frame, which trapped players at a low
  frame rate (Assetto Corsa, F1 25). Now the menu stays closed and one notice per game start says which runtime runs
  until you pick one and which key opens the menu ("A second neural runtime was found. <runtime> runs until you pick
  one: open the menu (<key>) ..."); the chooser appears the first time you open the menu, and Decide later still
  works. Which runtime runs while no choice is made is exactly 0.3.4.1's rule.
- **The Ray Regeneration settings could not be found.** An RX 7000 player went looking for the **Ray Regeneration**
  section in the Neural tab and there was nothing there: the section was drawn only while FSR Ray Regeneration had
  denoised a frame in the last 3 seconds, or when Ray Regeneration had given the game up, so a game that never turned
  Ray Reconstruction on, and a driver that refused the denoiser, both left an empty page and no explanation. The
  section is now **always** there, with one dim line that says which of four cases it is: it is denoising now, the
  game has not turned Ray Reconstruction on (that line gives the three steps in the game), Ray Regeneration gave this
  title up and why, or it last ran N seconds ago. A card that needs a word about itself gets a third dim line: on
  RX 7000 Ray Regeneration is offered by default but the driver may refuse it (then the game keeps its own denoiser);
  on RX 6000 and older it is not offered, because AMD ships it for RDNA 4, and
  `[FSR-RR] FfxDenoiserAllowPreRdna4=true` offers it anyway. The controls stay on screen, greyed out, while it is not
  denoising, and act again as soon as it runs. No setting and no default changed with this.

### Ray Regeneration
- **The sharpening AMDNR adds after Ray Regeneration is a real ini key now:** `[Sharpness] RrDefaultSharpness`,
  **default unchanged at 0.25**. That is the number AMDNR uses when the game itself sends no sharpness (Resident Evil
  Requiem sends none), applied by FSR to the denoised picture. Sharpening after a denoiser also lifts the grain the
  denoiser left, so if you see grain on faces or textures you can now set 0.15, 0.10 or 0 in the ini (or in the
  menu's Image > Sharpness with Override, as before) instead of rebuilding anything. A game that sends its own value
  keeps it, an explicit Override still wins over both, on Wine / Proton nothing is added whatever the key says, and
  Save Settings keeps your number.
- **A `[Sharpness] Sharpness` your ini kept while Override is off is named as waiting.** It does nothing at all until
  you tick Override, and then it applies at once: one Ray Regeneration report carried 1.00 there, four times the RR
  default, with no way for the player to see it. Image > Sharpness now shows a dim tag on the Ray Regeneration path
  ("ini Sharpness 1.00 waits for Override"), and the `[RR_POST]` log line says the same.
- **No denoiser tuning changed and no default moved**: every Ray Regeneration number in this build is 0.3.4.1's.

### Neural Rendering
- **danielblnc 0.4.3 (published 2026-09-28) is supported day one.** His new public runtime needs no AMDNR update:
  its layout was already accepted in the host, so AMDNR 0.3.4, 0.3.4.1 and 0.3.4.2 all drive it. It is now the
  **recommended danielblnc runtime on RX 9000 and RX 7000** - `v0.4.3-Runtime.zip` on the Alpha0.3.4.1 release,
  which the AMDNR Launcher offers as the recommended one; `v0.4.1-Runtime.zip` and `v0.4.0-Runtime.zip` stay
  accepted, and 0.3.3 is retired (no longer recommended for RX 7000). His own numbers for it: +20% Reference /
  +18% Fast vs 0.4.2, his measurement. Nothing in this build changed for it.
- **This build accepts one more danielblnc runtime layout.** AMDNR recognises a danielblnc pass DLL by the file
  itself (its size and its SHA256) and reads its knobs from a table of accepted layouts; a build that is not in that
  table is not driven at all. This release adds one more row to that table, so a newer danielblnc build can be driven
  here without waiting for an AMDNR update: the Neural tab names the file and its version as usual, and Network
  style, Tone curve, Black lift, Game exposure and Fast mode work with it. The layouts accepted before
  are untouched and no default moved, so the runtime you have today behaves exactly as it did in 0.3.4.1. That one
  row is the only change on the neural side of `OptiScaler.dll`: the neural pass itself, `LmxxfNrRuntime.dll`,
  `LmxxfNrRuntime.pak` and the danielblnc runtime zips are the 0.3.4.1 files, byte for byte.

### Linux / Proton
- **Text corrections.** The Neural tab's Wine / Proton line and the `[RR_DIAG]` log line still said that FSR
  Ray Regeneration works there. Both now say what the 0.3.4.1 notes say: FSR upscaling works under Wine / Proton, and
  FSR Ray Regeneration is a known issue there (pink / magenta patches in some games). Nothing else changed on Proton.

### Diagnostics
- For support, the log names the overlay DLL a game loads (Steam, Discord, Epic, GOG Galaxy, Overwolf, Ubisoft and
  the like) and the caller of `D3D11CreateDevice` at INFO, once per distinct name (DEBUG after that). An overlay
  AMDNR blocks instead of loading keeps its line at DEBUG (`[Menu] DisableOverlays=true`, or the Epic overlay while
  frame generation is running).
- Menu input at INFO (from the Assetto Corsa fix): one line per menu-key edge (up to 40 per game start), one line
  per second while the menu is visible (after two minutes every 30 s), a summary per open, and, per source, how many
  clicks and keys arrived and how late the game's own messages were.

### Docs
- README, "Rename OptiScaler.dll": the list now names every proxy name the mod accepts (`dxgi.dll`, `d3d12.dll`,
  `winmm.dll`, `version.dll`, `dbghelp.dll`, `winhttp.dll`, `wininet.dll`, plus `OptiScaler.asi` with an ASI loader).
  `d3d11.dll` is not one of them: the mod never loads under that name.
- README FAQ: **Uncharted: Legacy of Thieves Collection** crashes at the first NR frame on RX 9000 (a stack overflow
  at HIP's first initialisation on the game's 192 KiB job fiber). Keep `[DlssNr] Enabled=false` in that game until
  0.3.5; switching runtime does not help.
- README FAQ: **hybrid PCs** (a Ryzen with integrated graphics and a Radeon): if the game renders on the integrated
  GPU, NR never runs. Set the game to the discrete GPU in Windows Settings > System > Display > Graphics; the GPU
  line in Neural > Diagnostics names the adapter NR runs on.
- README: one line on `[DlssNr] AmdLmxxfTierSnap=true` on RX 9000 (off by default; the lmxxf NR size moves to a
  network size at every render resolution: some sizes drop a tier, 1440p Quality's 1707x960 -> the 900 size,
  network 14.08 -> 9.96 ms per run measured outside a game, a slightly softer image). The cost hover beside NR
  resolution (Neural > Performance) carries the same hint on RX 9000 while the snap is off and no tier cap is set.
- README FAQ: **"You cannot find the Ray Regeneration settings?"** - where the section is, what its dim line says while
  Ray Regeneration is not running, and the three switches a game needs (DLSS as the upscaler, ray tracing or path
  tracing, Ray Reconstruction). All nine languages.
- README: the sharpening after Ray Regeneration names its new key, and the ini documents it.
- README FAQ: **Ray Regeneration looks grainy or noisy** - judge it with Neural Rendering switched off, because the
  neural pass runs after Ray Regeneration, on its output, so a shot taken with NR on says nothing about RR. Which
  setting to try for which kind of grain is in the Ray Regeneration settings guide **RR-BEST-SETTINGS.md**
  (not in the zip). One ini trap is named there too: a value under `[Sharpness] Sharpness`
  does nothing while `OverrideSharpness` is off, and it applies the moment you tick Override in the menu (the menu
  tags it now). **No denoiser default and no sharpening default moved in 0.3.4.2** - the numbers are 0.3.4.1's. And two honest limits: a part of the grain is
  the game's own ray sampling, which AMD's denoiser is not built to repair (a game that offers DLSS Ray
  Reconstruction switches its own denoiser off and hands us the raw signal), and the grain that crawls in a still
  scene has a structural cause on our side that no slider removes completely - that one is a known issue, and the
  work for it is 0.3.5.
- Proton: Ray Regeneration stays a known issue (magenta patches in some games); the README and the Neural tab say so.

## 0.3.4.1 — 2026-09-27

Hotfix: Ray Regeneration is less soft on Windows, Control Resonant no longer shows a false "Upscaler failed to run!"
popup, and on Linux / Proton the menu works (confirmed by a player, also with frame generation on). **Neural
Rendering is unchanged from 0.3.4 (same runtime and pak).** Coming from 0.3.4, replace `OptiScaler.dll` only (the
file you renamed, e.g. `dxgi.dll`): `LmxxfNrRuntime.dll` and `LmxxfNrRuntime.pak` are the same files as in 0.3.4.
Launcher users: it updates for you. AMDNR Launcher 0.3.4.1 ships on the same release (see below).

### Fixed
- **Ray Regeneration looked soft** in games that send no sharpness (Resident Evil Requiem sends 0). On Windows, with
  FSR Ray Regeneration, when the game sends no NGX sharpness, AMDNR now sharpens by **0.25** after RR: FSR's own
  sharpening, or OptiScaler's RCAS when RCAS is ticked (never both). A game that sends its own value keeps it; the
  path-traced profile keeps 0; plain FSR without RR is unchanged; on Linux / Proton the default is 0 (see below).
  Image > Sharpness shows the tag "RR default: the game sends none". **To turn it off:** Image > Sharpness, tick
  Override, slider 0 (`[Sharpness] OverrideSharpness=true`, `Sharpness=0`). No new ini key.
- **Control Resonant: no false "Upscaler failed to run!" popup.** At 1440p Quality the game renders one pixel wider
  than the size it asked for, so FSR Ray Regeneration re-creates itself at the new size and skips that one frame.
  That skip is now a warning in the log, not a popup (it was not a 0.3.4 regression). Streamline's tag diagnostics
  (`[RR_TAG_DIAG]`) no longer log two lines every frame.

### Linux / Proton
- **The menu works.** The menu could bind to a helper window that was then destroyed, so it opened without mouse or
  keyboard input, or not at all; it now binds to its swapchain's own window and falls back to the game window. If
  the window is still lost, you get one warning, "Menu window lost", with the fix (a popup on Proton only), instead
  of a "SetTargetWindow rejected" line every frame. Confirmed by a player on Steam Proton (RX 9070 XT, vkd3d-proton,
  Resident Evil Requiem): the game boots, the menu opens and takes the mouse, and Save report works, also with frame
  generation switched on.
- Known issue: Ray Regeneration can show pink / magenta patches on Proton; the default sharpening is off there now, but if you still see them use plain FSR (FSR 4 on RX 9000) and send a Save report.
  Under Wine / Proton, AMDNR adds no sharpening after Ray Regeneration when the game sends none (Windows keeps 0.25);
  your own value (`[Sharpness] OverrideSharpness=true` with `Sharpness`) still applies there. Image > Sharpness then
  shows "RR default 0 (off on Proton)", and the log has one "[RR_POST] Wine/Proton: no default RR sharpening" line.
- **Frame generation** now turns on without crashing (the 0.3.4 crashes are gone in the player's tests), but fps
  counters count the generated frames too: under a 60 fps cap or 60 Hz V-Sync that is 30 real frames, which looks
  like 30. Leave it off on Proton for now (`[FrameGen] FGOutput=nofg`), or use it only when the game reaches about
  60 fps without it, on a screen faster than 60 Hz.
- **Save report** now says whether the game ran on vkd3d-proton or DXVK.
- **Neural Rendering does not run on Linux** (Windows only). The AMDNR Launcher is a Windows program that can run
  under Proton (experimental, not tested by us yet; see the launcher list below); installing by hand works as before
  (README, "Linux / Proton").

### AMDNR Launcher 0.3.4.1
You asked, we built it: the first Discord feedback, in `AMDNR-Launcher.exe` on the Alpha0.3.4.1 release.
- Nine languages: English, Arabic, Chinese (Simplified), French, Spanish, Portuguese, Italian, Russian and Polish.
  The launcher follows your Windows language (else English); pick another under LANGUAGE or in SETTINGS, where it is
  also offered on first start. Doctor findings, install messages and the COLLECT LOGS report stay in English so
  support can read them.
- Search the library.
- Favourites: starred games come first.
- Hide games (HIDDEN shows them again).
- Rename a game.
- CHOOSE GAME .EXE: pick the game's exe yourself when the launcher picked the wrong one or none.
- REDengine: Cyberpunk 2077 and The Witcher 3 (its DX12 build) are found in the right folder.
- PLAY and OPEN FOLDER on every game's page.
- STORES: switch whole stores on and off.
- Folders you add by hand stay listed, also while their drive is unplugged, until you remove them.
- UNINSTALL asks first and removes everything the mod placed, including what it wrote while the game ran (logs,
  caches, crash dumps, unfinished reports); it keeps finished Save report zips and a DLL under the proxy name that
  is no longer an OptiScaler (the game's own).
- COLLECT LOGS on any game, installed or not, with a scan report; one store that fails no longer stops the scan.
- Handheld / APU recognition (ROG Ally Z1 Extreme and other Ryzen APUs), with a note that AMDNR on them is still in
  testing.
- Linux (experimental, not tested by us yet): the same Windows exe can run under Proton, shows a notice with what
  to do there, and also looks for games in your Linux Steam library. Steps: README, "Linux / Proton". Tell us on
  Discord if it works.

### Other
- Skin classifier: "Guide only" now carries the warning tag "can blur non-skin".
- The log names the FSR upscaler in use and its version, once per feature creation (with Ray Regeneration: the one
  that runs after it).
- An FSR FG log warning that said "XeFG" now says "FSR FG".
- README: a new **Linux / Proton** section (what works, Ray Regeneration and frame generation there, the AMDNR
  Launcher under Proton (experimental), install by hand, the menu, HDR, reporting), which danielblnc runtime zip for
  which GPU and on which release, and the AMDNR Launcher 0.3.4.1 list.
- Not in this hotfix: the Assetto Corsa report (the mouse in the menu) is still being looked into.

## 0.3.4 — 2026-09-27

A new menu: the Neural tab is rebuilt (runtime, status and Preset at the top, then Performance, Quality, Image
look, Ray Regeneration and a tools row with Diagnostics, Runtime options and Experimental), and every other tab
follows the same look. lmxxf is faster on RX 7000: its network now runs the cheaper size tier by default (1440p FSR
Quality: 73.3 -> 52.2 ms per network run on an RX 7800 XT, network time measured by a tester outside a game). lmxxf is also faster on
RX 9070 / 9070 XT with lmxxf 0.31's kernels (network time 15.35 -> 14.08 ms at 1080p on an RX 9070 XT, measured outside a game, the same output).
lmxxf runs on handheld APUs (Z1 Extreme / Z2 / Radeon 780M, Z2 Extreme / Radeon 890M / 880M), experimental and slow.
lmxxf gains Network output, Encoding, Residual edge fade, the model's native character mask and an opt-in Fast mode
(about 29% less network time at 1080p);
danielblnc gains its runtime settings (Network style, Tone curve, Black lift, Game exposure) and a highlight colour
guard. **AMDNR Screen GI** is new as a preview, off by default: AMDNR's own screen-space bounce light and
ambient occlusion. A **Save report** button zips every log for a bug report, and Assetto Corsa's menu input, the false "no
clean exit" warning, Shadow of the Tomb Raider's crash when the upscaler starts, Marvel's Midnight Suns on danielblnc,
The Last of Us Part II's late frames on lmxxf and many logs are fixed. Replace `OptiScaler.dll`, `LmxxfNrRuntime.dll` and
`LmxxfNrRuntime.pak`: all three changed. Most of this is not yet tested in a game (see "Known issues / not
tested").

### New menu
- **The Neural tab is rebuilt** (both runtimes; the approved mock). Top: Enable Neural Rendering with its key
  button; **Neural runtime** (danielblnc / lmxxf) with one state word (running, restart the game to switch, not
  installed, not for this GPU, stopped) and the running runtime's credit (Daniel Blanco's opens his GitHub page;
  on RDNA 4 lmxxf's names AMDNR's c32w kernels, on RDNA 3 AMDNR's RDNA 3 backend); one status line
  (`Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms`, the last number being the NR cost), with the
  placement, Streamline and Vulkan notes in its hover. The old Live section is now a closed **Live** row under the
  status line (NR and model rate, working size, dropped frames and network ms, Streamline and the runtime in use; a
  fourth line in a game that submits its frames late); the "AMD processing" line is gone.
  At most one orange line follows when something needs you, with a button when there is a fix (Retry lmxxf, Switch
  to danielblnc, Open Upscaling); none in the default state.
- **Preset is three buttons** (Quality / Balanced / Performance): they set only NR resolution (100 / 85 / 70%) and
  turn Dynamic NR off. They no longer change Temporal stability, Sharpening or Detail / Colour strength, so NR
  style keeps its value; "Custom" shows when no button matches. NR style reads the style from the sliders the
  running runtime uses, so "Default" reads Default on both runtimes. **Style slots** is a button with a popup:
  Store / Apply / Clear (were Save / Load / Clear; same `StyleSlot1..3` keys).
- **Performance**: NR resolution (%) with its cost beside it (`N.NNx cost`; on lmxxf the price of the network size
  tier it runs, named in the hover), Neural passes, Full network, Fast mode (see "Speed"), Dynamic NR resolution (Target FPS only while it
  is ticked), Model interleave (Interleave preset and a pacing line appear under it only while it is on).
  **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS) and *More quality options*:
  Network history (one checkbox for both runtimes instead of "Network history (lmxxf)" and "Model history
  (interleave)"; same keys), Output smoothing, Stability mode, Residual temporal, Residual edge fade and
  Still-surface steadiness. A row that cannot act says why (e.g. "needs Temporal stability", "not with Model
  interleave"). The long notes under the residual sliders moved into their hover. The locked-off Adaptive
  interleave rows are gone (the keys are still read).
- **Image look**: Colour composition, Detail / Colour strength and three folds: *Model strength* (Tone intensity,
  Structure intensity, Character structure with Follow Structure, Edit detail, Edit colour, Edge guard, Native
  character mask, and danielblnc's Network style, Tone curve and Black lift), *Exposure and highlights*
  (Auto-exposure, its highlight cap, Highlight colour guard, Game exposure) and *Appearance filter* (moved here from
  the tools row; its off / on word after the name; labels Specular reduction, Contact shadows, Halo protection,
  Flat-area protection, Tonemap strength). Fixed: Character structure could not go back to "follow Structure"
  (-1); ticking Follow Structure now saves `SkinStructure=auto`. **Reset appearance filter now resets only
  `[AmdLook]`**: 0.3.3.2 also reset Neural passes, NR resolution, Encoding, Structure, Character structure and Tone
  intensity. A RenoDX refusal note now carries a fix button ("Turn off Network output", "Encoding: Auto").
- **Ray Regeneration has its own section** (Neural > Ray Regeneration, after Image look; was Neural > Quality >
  Ray Regeneration), shown only while the game runs FSR Ray Regeneration. See "Ray Regeneration" below.
- **The tools row** is closed when the game starts and has three buttons: **Diagnostics** (Network output, Debug
  view 1-5, the RR debug view in plain words, Edit shaper A/B, NR cost, the ghost / self-tuning / edit
  accumulation readouts, a GPU line naming what the GPU runs, and **Save report**), **Runtime options** (was Advanced: Encoding,
  Every-frame NR, NR slots, Highlight proxy) and **Experimental** (AMDNR Screen-space GI, a preview: see
  "AMDNR Screen GI" below; the inherited Screen-space GI is retired from the menu). The Graphics wait row is gone (`AmdGraphicsWaitExperimental`
  is still read and saved).
- **Rows the running runtime does not have** are greyed with a short tag ("not in lmxxf yet", "not in this
  danielblnc build") or hidden with a count ("3 danielblnc-only options hidden"); a value you saved stays clickable
  so it can be turned back. Switching runtime moves no other row.
- **Help:** hover a control's label (no more (?) markers); help texts are a few plain lines with the ini key in a
  dim last line, and no version history.
- **Before the first upscaled frame** the Neural tab and the Home-key notice name the runtime that will run (0.3.3.2
  showed danielblnc's rows on an lmxxf-only install until the first frame). The notice reads "Neural Rendering: On
  (lmxxf)" / "On (danielblnc)" / "Off".
- **No runtime files on an AMD or Intel GPU:** the Neural tab shows only the "No neural runtime found" card (with
  the folder and a Discord link), plus Ray Regeneration while it runs. The inherited NVIDIA layout is no longer
  drawn there (its Auto skin mask checkbox wrote `[DlssNr] AutoMask`). On a GPU that can run neither runtime the card
  says so, with the reason in its hover.
- **First-launch chooser:** two rows, each with its credit ("DLSS-NR on AMD by Daniel Blanco (danielblnc)", "lmxxf
  runtime by Kien (MIT)"), installed or not and whether it runs on this GPU; the files are in the row's hover.
- **Header and footer:** the header reads AMDNR (hover: version and build) with Discord and GitHub buttons and the
  copyright line; a second row credits the neural runtimes ("DLSS-NR on AMD by Daniel Blanco (danielblnc) - lmxxf by
  Kien (MIT)"; hover it for the full credits). The footer's "Open Wiki" (upstream OptiScaler's wiki) is gone: the
  header's GitHub button opens this project's README. The footer holds the frame-time graph, Menu Scale, Save
  Settings and Close. **Save report** is the last row of Neural > Diagnostics and the first row of Advanced > Logging.
  The window title reads "AMDNR v0.3.4 - <exe> - <game>", and the start-up splash "AMDNR - Insert for menu".
- **The other tabs** follow the same look. Upscaling: the upscaler first, one status line (GPU, API, input,
  spoofing), a render / display / upscaler-time line, and *Render resolution* (the former Upscale Ratio Override and
  Output Scaling); on a non-NVIDIA card "DLSS w/Dx12" is no longer listed; in a D3D11 or Vulkan game with Neural
  Rendering on the w/Dx12 items read e.g. "FSR 3.X/4 w/Dx12 - Neural". Image: Sharpness (one Sharpener combo),
  Textures (moved from Advanced), Init Flags (one per row; "Upscaler auto exposure"), Magnifier. Frame Gen: FG Input
  and FG Output first, external frame generation in a Compatibility fold, "Low latency" (was "fakenvapi").
  Interface: FPS Overlay and Keybinds (a button per key, Reset only on a rebound key). Advanced: one intro line,
  Active Quirks, Display (V-Sync: Game / On / Off in one combo, same `[V-Sync] ForceVsync` values), Compatibility,
  Logging (with the build stamp). Keys, defaults and what Save Settings writes are unchanged.
- **Smaller menu fixes:** tooltips no longer end in "##2" (Frame Gen > XeFG > Rectangle Settings); "Motion
  Threshold" (was MotionThreshod); Root Signatures says it can pause Neural Rendering on some frames; the large
  header messages before an upscaler runs wrap inside the window; the Menu Scale combo shows "Auto (1.0)" in full;
  collapsing sections no longer draw a box inside a box; the FPS overlay and the splash draw their text as plain
  text.
- **The Neural runtime combo names the exact version of your files**, e.g. "lmxxf 0.3.4" or "danielblnc 0.4.0"
  (only the name when the version is unknown); the running state stays the dim word after it.
- **Fold tags:** More quality options, Model strength and Exposure and highlights show a dim "default" or "custom"
  after their name, so a changed value inside a closed fold is easy to find (Appearance filter keeps its off / on).
- **Components: N of 7 active** sits in the header of every tab, under the credits line (the seven component chips
  no longer sit above every tab, and the row left the Advanced tab): click it for the pills, grouped Upscalers /
  Frame gen / Hooks / NVIDIA, each with a hover; "nvngx.dll not present (normal on AMD)".
- **Preset has a fourth button, Handheld**, on handheld APUs only (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M / 880M):
  NR resolution 100%, Dynamic NR off, the model every 4th frame (Model interleave), 1 Neural pass and Full network
  off. On these chips a dim line under Preset says the network stays at 360p whatever the Preset.
- **Fast mode** in Neural > Performance, right after Full network (see "Speed").
- **Late frames:** in a game that submits its frames late (The Last of Us Part II) the Neural tab says "This game
  submits frames late: the model ran on N% of frames" (see "Fixes").

### AMDNR Screen GI (preview, off by default)
AMDNR's own screen-space global illumination, new in 0.3.4, by 3zwr1. It is a preview: off by default, not yet
tested in a game, and its look will change.
- **What it does:** ambient occlusion (darkening in creases and where objects touch) and one bounce of light from
  nearby surfaces (a lit red wall tints the floor beside it), plus an optional share of the last frame's bounce for
  more than one bounce. It is traced from the game's depth and colour before Neural Rendering, the upscaler and the
  game's UI, so the UI is never lit. It works with NR on (both runtimes, danielblnc and lmxxf, take GI's picture as
  their input) and with NR off (the upscaler takes it). The light comes from the game's own image; GI adds no lights.
- **Turn it on:** Neural > tools row > **Experimental** > **AMDNR Screen-space GI** (dim tag "preview - by 3zwr1"),
  or `[AmdGi] Enabled=true` in `OptiScaler.ini`. While it is ticked: GI quality (Low / Medium / High / Ultra / Auto;
  High by default, Low on APUs and handhelds; its measured GPU time shows beside it), Bounce light, Ambient
  occlusion, Radius, Object thickness, Camera FOV (auto reads it from FSR, Streamline or DLSS-FG; the tag says from
  where), *More GI options* (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding), Debug view (not
  saved), Reset GI and a status line. Only one screen-space GI at a time: ticking it unticks the inherited one. The
  Experimental drawer is drawn only while an NR runtime is installed.
- **Cost, measured in AMDNR's GPU lab on an RX 9070 XT** (synthetic scenes, not in a game; the whole GI pass per
  frame with the GPU clocked up; the range goes from an outdoor scene to a closed room; "render" is the game's
  resolution before the upscaler):

  | GI quality | 1920x1080 render | 1280x720 render |
  |---|---|---|
  | Low | 0.39-0.42 ms | 0.28-0.30 ms |
  | Medium | 0.55-0.60 ms | 0.32-0.36 ms |
  | High (default) | 0.95-1.05 ms | 0.47-0.52 ms |
  | Ultra | 1.57-1.76 ms | 1.30-1.38 ms |

  In a game expect somewhat more (the game shares the GPU's caches and bandwidth), and the cost depends on what is
  on screen: read the ms tag beside GI quality. Not yet measured on RX 7000 or a handheld.
- **Limits:** it is screen-space. Only what is on screen can cast or block light: light from off-screen, behind the
  camera or behind an object is missing, and GI changes as things enter or leave the view. Thin objects can show a
  dark halo or let light leak behind them (Object thickness trades one for the other). Preview quality: the bounce
  into shadowed areas is weaker than it should be, ambient occlusion can darken lit surfaces a little, and still
  scenes can shimmer slightly. Direct3D 12 games with an upscaler (DLSS, FSR or XeSS inputs) on an AMD GPU only: not
  D3D11 or Vulkan games in this preview, and not where NR runs on the upscaled image (final image mode, after FSR
  Ray Regeneration). The status line says why when GI does not run.
- **AMDNR's own work**, Copyright (c) 2026 3zwr1 (AMDNR), written clean-room from published papers only: Therrien,
  Levesque and Gilet 2023 (screen-space indirect lighting with a visibility bitmask); Jimenez, Wu, Pesce and Jarabo
  2016 (GTAO and its multi-bounce fit); Jimenez 2014 (interleaved gradient noise); Roberts (the R2 sequence);
  Schied et al. 2017 (SVGF); Dammertz et al. 2010 (edge-avoiding a-trous); Kopf et al. 2007 (joint bilateral
  upsampling); Karis 2014 and Salvi 2016 (history clamping); Turanszki 2019 and Wu 2020 (normals from depth); Hartley
  1997 (rotating-camera self-calibration); IEC 61966-2-1 (the sRGB transfer function). No code from the inherited
  Screen-space GI or any other project was used. The inherited Screen-space GI (OptiScaler-AMD-PreSR lineage, credit
  to its original authors) is a separate effect: retired from the menu in 0.3.4 (its checkbox shows, tagged
  "retired", only while your ini has it on), and it still runs from `[AmdRtgi] Enabled` in the ini.

### Handhelds (experimental)
- **lmxxf runs on handheld APUs with 12 or more compute units**, through AMDNR's RDNA 3 backend by 3zwr1:
  Z1 Extreme, Z2 and Radeon 780M (gfx1103), Z2 Extreme, Radeon 890M and 880M (gfx1150). Experimental and slow.
  First tester results (ROG Ally, Z1 Extreme): lmxxf's probe outside a game, 54.7 ms per network run at 360p,
  110.9 ms at 576p (168.0 ms at 720p); in a game, one tester's run (Shadow of the Tomb Raider, 1280x720 with XeSS, Handheld preset),
  62 ms per network run on average at the 360p size with the model every 4th frame, about 29 fps with NR on. The Neural
  runtime row says "experimental" after the RDNA 3 credit, and the lmxxf entry's hover has an orange "Experimental on
  this GPU" line. The NR cost readout shows the real number on the device.
- **Refused, with a note:** Z1 and Radeon 740M (4 compute units), Radeon 760M (8), Radeon 860M / 840M (gfx1152) and
  a chip whose compute-unit count is unknown. danielblnc's runtime does not run on handheld APUs. RX 6000 (RDNA 2) is
  planned for 0.4.0.
- **Handheld defaults, only while your ini has no value of its own:** the
  network runs at the new 360p size (640x360) and the model runs every 4th frame (`AmdInterleave` 4, never saved).
  Picking Model interleave Off is saved as `[DlssNr] AmdInterleave=1` (also off), so the default does not come back
  at the next start; to turn it off by hand, write 1. `[DlssNr] AmdLmxxfTierCap=576` gives the 1024x576 size (sharper,
  slower).
- **What to expect on a handheld:** the network sees a 360p picture and runs every 4th frame, so its effect is
  lighter than on a desktop card, and more Neural passes mostly cost frame rate. On these chips the network stays at
  360p whatever the Preset, so Quality, Balanced and Performance look like Handheld; the menu says so. For a stronger,
  much slower effect set `[DlssNr] AmdLmxxfTierCap=576` and `AmdInterleave=2` in the ini.
- `amd_bridge.log`'s GPU line names the compute units and the verdict, e.g. "AMD neural: GPU <name> (gfx1103, 12
  CUs) - ... lmxxf yes (experimental): <note>".
- `LmxxfNrRuntime.dll` 0.3.4 refuses a handheld chip with fewer than 12 compute units, and a handheld whose
  `OptiScaler.dll` did not ask for the small network sizes ("this handheld needs OptiScaler.dll 0.3.4 or newer");
  `AMDNR_ALLOW_SLOW_GPU=1` skips both (testers only). Neural passes 2-3 also have their kernel on these chips.
- The handheld modules (48: `hip/gfx1103` and `hip/gfx1150`) are in the new `LmxxfNrRuntime.pak`. Use AMD's own
  Adrenalin driver: lmxxf needs HIP (`amdhip64_7.dll`), which some handheld makers' drivers leave out.

### Speed (RX 9000 / RX 7000)
- **RX 7000 (RDNA 3): lmxxf's NR size now snaps to the cheaper network size tier by default**
  (`[DlssNr] AmdLmxxfTierSnap` unset = on for RX 7000, Radeon 8060S / 8050S and the handheld APUs; off on RX 9000; an
  explicit `true` / `false` wins; `false` restores 0.3.3.2's sizes). The network runs at a few fixed sizes (720, 900,
  1080) and a size costs the same whatever part of it the picture fills: a size nearer the next smaller tier is
  lowered to it, a size that fills 90% or more of its own tier is grown to fill it, never above the frame's own size.
  Network time per run on an RX 7800 XT (a tester's run of lmxxf's probe, mean of 30 runs, network only):
  1440p FSR Quality (1706x960, NR 100%) **73.3 -> 52.2 ms** (1080 tier -> 900 tier); 1080p at NR 85% **73.2 ->
  52.2 ms**; 1080p at NR 70% **52.2 -> 34.4 ms** (900 -> 720 tier); 1080p at 80% stays in the 900 tier, grown to
  fill it (same cost, more detail). The gain per displayed frame is smaller (Model interleave, the game's own cost)
  and not yet measured in a game. Not bit-exact on RX 7000 by design: the network sees a slightly smaller picture
  (about 6% per side at 1440p Quality). The RX 7000 and Strix Halo advice now reads "start with NR resolution at
  70% or lower".
- **Two smaller network sizes, 360p (640x360) and 576p (1024x576)**, in `LmxxfNrRuntime.dll` 0.3.4, used only when
  `OptiScaler.dll` asks for them: on handheld APUs by default, and on any GPU with `[DlssNr] AmdLmxxfTierCap=360` or
  `576` (for testing; the look at these sizes is not yet judged in a game). `AmdLmxxfTierCap` = 720 / 900 / 1080 caps
  the size without the small sizes. Without the key every size and picture is 0.3.3.2's (RX 9070 XT output checked
  unchanged, with c32w on and off).
- **RX 9000: `[DlssNr] AmdLmxxfTierSnap=true` is an opt-in** (off by default, output unchanged). From a 1920x1080
  render: 85-90% -> 1600x900 (the 900 tier, cheaper, a little softer), 75-80% -> grown to 1600x900 (same cost, more
  detail), 70% -> 1280x720, 60% -> grown to 1280x720; 100% unchanged. `lmxxf_backend.log`'s frame line names the
  tier and what the snap did.
- **lmxxf is faster on RX 9070 / 9070 XT: network 15.35 -> 14.08 ms at 1080p, 10.79 -> 9.96 ms at 900p and
  6.97 -> 6.63 ms at 720p on an RX 9070 XT (measured outside a game), the same network output (bit-identical).** This uses lmxxf's own new
  kernels from lmxxf 0.31 (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2); Kien, MIT) on top of AMDNR's c32w. RX 9060 (gfx1200) keeps the previous
  kernels until a tester confirms them. The lmxxf runtime no longer reads lmxxf's `DLSS5_VIT_*` environment
  switches. Replace `LmxxfNrRuntime.dll` **and** `LmxxfNrRuntime.pak`; with an old pak (or a loose `DLSS5-AMD`
  folder) the status line says `fk=fff-` and lmxxf runs at the old speed.
- **Fast mode for lmxxf (opt-in, off by default):** Neural > Performance > Fast mode, or
  `[DlssNr] AmdLmxxfFastMode=true` (saved). The network runs one size tier lower (1080 -> 900, 900 -> 720; 720 -> 576
  and 576 -> 360 where the small sizes are on): about 29% less network time at 1080p (RX 9070 XT, measured outside a
  game), with fine detail a little softer. The NR resolution cost tag and its hover follow it. Off, lmxxf gets
  exactly the input it got without it; no runtime change.
- **Fast mode for danielblnc builds that have one** (`[DlssNr] AmdDanielFastMode`, saved; auto = the runtime keeps its
  own mode): the row shows in Neural > Performance only with a danielblnc build that has the mode (true = Fast, false
  = Reference, live, no restart). The runtimes in this release's runtime zips do not have it, so the row stays hidden
  and a key set in the ini logs once that it is not available.
- **NR cost readout, both runtimes:** the status line ends with the NR cost in ms and Diagnostics shows "NR
  cost: N ms of GPU time per model frame". lmxxf: the network's GPU time from its stats window (about 10 s after NR
  starts). danielblnc: the game queue's GPU time inside NR, D3D12 only (none on Vulkan and D3D11 titles), timed
  only while the menu shows it, so with the menu closed the game's command lists are 0.3.3.2's.

### Ray Regeneration
- **Its own section, Neural > Ray Regeneration**, after Image look, shown only while the game runs FSR Ray
  Regeneration: a status line, Path-traced profile (+ Profile temporal values and Texture route while it is ticked),
  Disocclusion threshold, Bias mask strength, Skin smoothing (Strength, Radius, Skin classifier) and the fold *More
  Ray Regeneration options*. The RR debug view is in Diagnostics while an NR runtime is installed, and in this
  section while none is. Keys unchanged.
- **New sliders** in *More Ray Regeneration options*: Stability bias, Cross-bilateral normal strength, Gaussian
  kernel relaxation, Radiance clip and Max radiance (`[FSR-RR]` keys that were ini-only), live; greyed while AMD's
  default tuning is on. A value typed with Ctrl+Click is clamped to the slider's range.
- **Profile temporal values** (`[FSR-RR] FfxDenoiserPathTracedTemporal`, on = 0.3.3.2): off applies only the
  path-traced profile's routing half; its six temporal values stay the fork's / yours.
- **RX 7000 / RDNA 3: FSR Ray Regeneration is offered by default again** (0.3.3.2 offered it on RDNA 4 only): RX
  7000, the RDNA 3 APUs and Strix Halo, while `[FSR-RR] FfxDenoiserAllowPreRdna4` is unset (auto, as shipped).
  `false` = RDNA 4 only; RX 6000 and older (and Intel) get it only with `true`. AMD ships Ray Regeneration for RDNA 4:
  when the driver refuses it on RDNA 3, the log and a notice say "FSR Ray Regeneration could not start" and the game
  gets FSR without the denoiser. The Upscaling tab has the switch as **Offer FSR Ray Regeneration on this GPU
  (restart)** (D3D12 games, AMD cards that are not RDNA 4), under the upscaler combo.
- **Fixed: the Ray Reconstruction option could be missing on RDNA 4** in Streamline titles that load
  `sl.interposer` before OptiScaler (the GPU check ran before the GPU was known): the RR hooks now attach and decide
  per call.
- **Bias mask strength is greyed "no bias mask in this game"** where the game publishes no DLSS bias mask
  (Resident Evil Requiem): its 0 default there changes nothing.
- **"Ray Regeneration is off in this title"** now says why in plain words (the game's DLSS plugin passes empty
  camera matrices; NVIDIA's Ray Reconstruction treats them as optional, FSR Ray Regeneration needs them) and what to
  do (turn RR off in the game and restore the engine's denoiser settings). It shows in the Neural tab (also without
  an NR runtime) and in the Upscaling tab under the upscaler combo.
- **Fixed: re-picking FSR Ray Regeneration after a fallback** clears "Ray Regeneration is off in this title" (it
  comes back after 30 frames if the inputs are still missing).
- **Changed: in an Unreal title whose DLSS plugin passes null camera matrices** (Ray Reconstruction evaluated
  through NGX directly, no Streamline camera, on all 30 frames) Ray Regeneration now stays off for the whole session:
  later RR features are created as FSR, and re-picking it in the menu keeps FSR with a notice. Other fallbacks retry
  as before.
- **Fixed: the game textures kept for Ray Regeneration** (up to 18 Streamline tags) are released once RR goes off;
  not on an in-game resize or quality change.
- **Opt-in experiments, ini only, off by default:** `[FSR-RR] FfxDenoiserAlbedoFp16=true` stores the albedo handed
  to Ray Regeneration as FP16 instead of 8-bit (for dark low-albedo surfaces and hair; whether AMD's denoiser accepts
  it is unproven: set it back if FFX errors appear); `[FSR-RR] FfxDenoiserNgxDirectSLConstants=true` takes the camera
  from the Streamline constants whose jitter matches, for NGX-direct titles that also send them (unproven on a real
  title).
- **The path-traced profile is retired (not recommended):** every Resident Evil Requiem report found it worse than
  the normal route (etched textures, lamps and emissive light too bright, thinner hair), also with Texture route 1.
  Its row shows only while `[FSR-RR] FfxDenoiserPathTracedProfile` is on, tagged "retired" (untick it, then Save
  Settings); while it is on, the log has one warning and every `[RR_PROFILE]` line says so. Resident Evil Requiem and
  PRAGMATA no longer carry the "path-traced title (profile opt-in)" quirk tag.
- **Diagnostic GPU work at default**, only on Ray Regeneration frames whose camera is missing, once per handle: one
  shader compile, up to 3 small dispatches and about 0.8 MB of buffers, to read the depth and normals format for a
  later fix. Nothing runs where the camera resolves (Resident Evil Requiem). Not yet run on a real GPU.
- **Logs:** `[RR_PROFILE]` names where the routing and temporal halves come from and whether a bias mask is
  published; `[RR_POST]` is logged again when a post setting changes; `[RR_CFG] settings changed` once per slider
  move; `[RR_GUIDES]` adds the disocclusion masks; `[RR_CAM]` and `[RR_INPUT] ... snapshot` lines once per handle; the
  per-frame "View matrix missing!", "slEvaluateFeature" and specular-hit-distance lines are logged a few times, then
  every 1000th. The RR dispatch snapshot (about 60 lines) is logged on a feature's first dispatch and on history resets
  1-3 and every 20th, not on every reset (Resident Evil Requiem resets on camera cuts), and one line names every reset.
  One line confirms the Ray Regeneration version with the game's device ("FSR Ray Regeneration - 1.2.0"); the
  earlier device-less version queries are INFO, no longer a false version warning. The denoiser provider's
  `0x80070057` line is INFO (the denoiser uses its own providers). One line says FSR Ray Regeneration does not need
  NR (it works the same with NR off) and, under Wine / Proton, that it works there while NR cannot.
- In a title where Ray Regeneration gave up, the Upscaling tab's orange line now points to Neural > Ray Regeneration
  for its options.

### danielblnc
danielblnc's runtime is **DLSS-NR on AMD by Daniel Blanco (danielblnc)** —
<https://github.com/danielblnc/DLSS-NR-on-AMD>. AMDNR ships it unmodified, with his permission; this release
changes only how AMDNR drives it.
- **Runtime settings in the menu** (Image look > Model strength: *Network style*, *Tone curve* (Reinhard / ACES) and
  *Black lift*; Exposure and highlights: *Game exposure*). Auto (the default) writes nothing, so the runtime's own
  value stays (0.3.3.2). Only danielblnc's public 0.3.3 and 0.4.0 builds take them; with another build the rows are
  greyed "not in this danielblnc build" and a key set in the ini logs once that it is ignored. A change resets the
  model history; back to Auto restores the runtime's value without a restart. Keys `[DlssNr] AmdRuntimeStyle`,
  `AmdToneCurve`, `AmdToneLift`, `AmdUseGameExposure`.
- **Highlight colour guard for danielblnc** (`[DlssNr] AmdDanielHighlightGuard`, off by default): bright areas (sky,
  lamps, fog lit by a flashlight) keep the game's colour instead of going grey; also inside the RenoDX composition.
  The first enable compiles a shader (a short hitch). In the menu: Exposure and highlights >
  Highlight colour guard (one row for both runtimes; it writes the running runtime's key).
- **Native character mask:** `[DlssNr] AutoMask=false` (or unticking the row) now turns danielblnc's character mask
  off on every pass; 0.3.3.2 forced it on. Default on, unchanged.
- **Network history with Model interleave off:** `[DlssNr] AmdInterleaveModelHistory=true` (the Network history
  checkbox) now keeps danielblnc's network history with Model interleave off too. Default off, unchanged.
- **Edit shaper A/B** (the "NR style changes away from 100%" issue, still open, default unchanged): Diagnostics >
  *Edit shaper (A/B, not saved)* = Off / Literal / F1 no limit / F2 ramped, with *Only below 100%* and *Carry cap*,
  in memory only (Save Settings writes nothing). Ini keys `AmdEditShaperLimit` (0 literal, 1 F1, 2 F2; any other value
  = 0), `AmdEditShaperScope` (1 = below 100% only) and `AmdEditShaperCarryCap` (never above 4x), read only with
  `AmdEditShaper=true`. `amd_presr.log` names the mode on each change, and says why when the shaper does not act.
- The model-frame ghost readout (Diagnostics) says it is waiting for the first model frame instead of "n/a".
- The warning about danielblnc's standalone installer in the game folder now also catches renamed installers
  (`dlssnr_on_amd_setup*.exe`) and names the file.

### lmxxf
- **`LmxxfNrRuntime.dll` 0.3.4:** takes the model's native character mask controls and the two small network sizes,
  and guards handheld chips. At default the host sends 0.3.3.2's frame byte for byte, and the output equals
  0.3.3.2's. It also accepts an older `OptiScaler.dll`. With 0.3.3.2's runtime and the new `OptiScaler.dll` the
  controls are refused once, NR keeps running, and the Native character mask row says "refused: runtime DLL too old".
- **Native character mask on lmxxf, with Structure intensity and Character structure:** lmxxf now
  follows Native character mask (`[DlssNr] AutoMask`), Structure intensity and Character structure, and the
  NR styles that set structure (Cinematic, Crisp, Natural, Vivid). Each change rebuilds the network (about a
  1 s hitch). Defaults send the 0.3.3.2 frame.
- **Network output** (Diagnostics, `[DlssNr] AmdNetworkOutput`, off): the model's answer reaches the frame untouched
  (one frame late, as every lmxxf answer): no look, no sharpening, full strength. The RenoDX composition stays
  refused under it, as on danielblnc.
- **Encoding** (Runtime options, `[DlssNr] AmdEncoding`, Auto): sRGB / Gamma 2.2 decode the game's colour to linear
  light before the network and encode the result back, as on danielblnc. Upscaler path only (final image mode keeps
  0.3.3.2's feed). The RenoDX composition stays refused with sRGB / Gamma 2.2 on both runtimes.
- **Residual edge fade** (`[DlssNr] AmdResidualFade`, 0..0.25, 0 default): fades the model's edit toward the screen
  edges, only while the edit is lifted (NR resolution not 100%, a render above 1920x1080, or the tier snap moving the
  size) and not under Network output.
- **Screen-space GI** (the inherited effect, `[AmdRtgi] Enabled`, off; an effect AMDNR inherited from the
  OptiScaler-AMD-PreSR lineage, credit to its original authors; retired from the menu in 0.3.4, see "AMDNR Screen
  GI"): needs the `experimental_lighting` folder of the
  danielblnc package beside the game (else one log line and the network runs on). It runs on the carried frame
  before the Image look. While on it costs GPU time and 64 bytes of VRAM per pixel (about 130 MB at 1080p, 530 MB at
  4K after Ray Regeneration), released about 8 frames after it stops.
- **Interleave pacing:** a saved `[DlssNr] AmdInterleavePacing` of 0..1 now also paces lmxxf with Model interleave
  on (it pads the filled frames, so FPS drops); -1 (auto, the default) stays off on lmxxf. The menu shows "Interleave
  pacing: off with lmxxf" with the measured model / fill ms, and each `lmxxf stats` line shows the pacing readout.
- **Game exposure** (`[DlssNr] AmdUseGameExposure`, -1 auto = use the title's exposure texture, 0 ignore, 1 use): 0
  feeds lmxxf by its auto-exposure instead; logged once, and again only when the key moves to or from 0.
- **Classic carry is saved as `AmdInterleavePreset=1`** (0.3.3.2 wrote 6): switching to danielblnc then reads
  Standard. An old ini with 6 still reads Classic carry under lmxxf and Guided fill v2 under danielblnc.
- **Fixed: the lmxxf stats window cleared 8 of its 12 counters**, so shoulder / blown / rejected summed over the
  session and could latch a false self-heal (a colour switch mid-session).
- Logs: the "runtime up" line says "modules pak" when the pak is used; the loose-folder warning no longer tells you
  to delete a `native-game-tiled-assets` folder beside the exe (it may be another mod's); the stats line adds
  `release_marks=`.

### Fixes
- **Assetto Corsa: the menu input.** While the menu blocks the game's input, a game's low-level keyboard or mouse
  hook is now skipped and the event passed on to Windows; before, it was swallowed for the whole desktop (Alt+Tab
  and the Windows key dead). Kill switch `[Hotfix] MenuLowLevelHookPassThrough=false`. And a second menu or NR key
  press within 400 ms of the last one is ignored, so one press toggles once (`[Hotfix] MenuToggleDebounceMs`, 0 = the
  old behaviour). Not yet confirmed in Assetto Corsa.
- **Clean exit:** after a normal quit the next start no longer warns "No clean exit recorded". AMDNR stamps its
  marker when the game's own exit call runs and again when its NR shutdown has finished; with both stamps the new log
  says the game was quitting, not crashing. With only the first stamp (a crash or hang in AMDNR's shutdown) it still
  warns, and says so.
- **Previous logs:** three previous logs are kept per game exe (`[Log] KeepPreviousLogs=3`; 1 = one, as in
  0.3.3.2): `OptiScaler.previous.<exe>.log` (newest, the old name), `OptiScaler.previous-1.<exe>.log` and
  `OptiScaler.previous-2.<exe>.log`.
- **Report zip: Save report** (Neural > Diagnostics, last row, or Advanced > Logging, first row) writes `AMDNR-report-<exe>-<date>.zip` into the game folder
  (on the Desktop if it is read-only, else in `%TEMP%`), with `report.txt` (an `[AMDNR]` section), `logs/`
  (`OptiScaler.log`, every previous log, `amd_bridge.log`, `amd_presr.log`, `lmxxf_backend.log`, `dlssnr_on_amd.log`)
  and `config/` (`OptiScaler.ini`, `dlssnr_on_amd.ini`). Your Windows user name and PC name are masked (a name inside
  a game path outside `C:\Users\` is not); the result line masks the user folder too. With `[Log] LogToFile=false`
  the old log is zipped as `OptiScaler.NOT-THIS-SESSION.log` and `report.txt` says so. It saves in the background.
- **Crash log (opt-in):** `[Log] CrashHandler=true` writes `amdnr_crash.log` beside `OptiScaler.log` (START, the
  module of a crash with its kind, e.g. "[AMD driver]", END on a clean exit); `[Log] CrashDump=true` adds a minidump
  ("DUMP ready" or "DUMP unavailable"). Off by default: nothing is registered. Save report collects it.
- **Crash reporters no longer empty the game's `OptiScaler.log`:** OptiScaler also passes through crash reporters
  and CEF helpers by name (crashpad_handler, crashreport, crashhandler, cefsubprocess) and danielblnc's installer
  under any name. Games are not affected.
- **Logs:** "waiting for resolution settings to settle" once per settle instead of about 45 times; the RDR2 flood of
  `CreateCommittedResource result: 80070057` and the frame-generation flip / create errors are logged 5 times per
  resource, then every 1000th, and name the resource; one `amd_bridge.log` line "AMD settings: <key> a -> b" per
  settled change of a picture setting (both runtimes); the colour line mentions the RenoDX fallback only when RenoDX
  is selected.
- **REFramework warning can be silenced:** `[Hotfix] WarnMissingREFramework=false` hides the start-up notice and the
  menu line (one log line remains). Default on, unchanged.
- **`[Anisotropy] ModifyComparison` / `ModifyMinMax` saved by Save Settings are now read back** (they were read under
  other names, so a saved `false` was ignored).
- **Frame generation:** after a failed depth / velocity copy re-create the next frame no longer reads the freed copy.
- **Marvel's Midnight Suns (and other titles whose D3D12 device is a Streamline proxy): danielblnc no longer
  crashes.** Its backend is now built on the device behind the proxy (the device of the game's queue), proven by
  Streamline's own identity or a probe; with two devices and no proof danielblnc is not started, the status line says
  why and the game runs untouched. The lmxxf runtime also asks Streamline for the real device. Kill switch
  `[DlssNr] AmdStreamlineDeviceFix=false` (0.3.3.2). Not yet confirmed in the game.
- **The Last of Us Part II (games that submit a frame's work late):** lmxxf ran on a fraction of the frames and
  restarted its network history about every other frame. A job whose command list arrives one frame late is now kept
  for that frame, and its answer is used one frame later, warped into place by the motion; still late a frame later,
  it is dropped as before. `lmxxf stats` lines count late and graced jobs. Kill switch
  `[DlssNr] AmdLateSubmitGrace=false` (0.3.3.2). Games that submit on time are unchanged. danielblnc: `amd_presr.log`
  says how often NR ran ("AMD late submission: NR ran on X% of the last N frames"); its fix is planned for 0.3.5, so
  use lmxxf in such a game. The Neural tab shows the share for both runtimes.
- **An NR resolution change alone no longer pauses NR for 300 ms** (the slider, a Preset or NR style, a Dynamic NR
  step): the history restarts and NR is back on the next frame. A change of the game's input size still settles as
  before, and so does a burst of changes. lmxxf still has its short hitch when the network size tier changes.
- **Forza Horizon 6: no false "No clean exit recorded"** after a normal quit. A game that ends itself with exit code
  0 now counts as a quit; crash exits still warn.
- **lmxxf after an error that stops it for the session** no longer calls the runtime every frame (each call failed
  and filled the log): the game's frame passes through untouched and the Neural tab still offers Switch to
  danielblnc. A device removal (driver timeout) is still logged as such.
- **Wine / Proton:** the Neural tab says NR cannot run there (both runtimes need AMD's Windows HIP runtime,
  `amdhip64_7.dll`), while FSR Ray Regeneration and FSR upscaling still work; before, it said to reinstall the driver.
- **Logs:** the menu's input-health lines now count every input channel and warn at most once per 10 s with no
  input while the menu is open, instead of firing on healthy sessions; `hkslSetTag only supports DX12` in D3D11 /
  Vulkan Streamline titles is INFO, 5 times, then every 1000th.
- **Handhelds and laptops with a Radeon 780M / 760M / 740M (ROG Ally, Legion Go and similar), on Windows: no automatic
  FSR 4.** These chips got the FSR 4 INT8 upgrade on their own, and AMD does not support FSR 4 on them yet. The
  upscaler now stays on its non-FSR 4 path: with `Dx12Upscaler=auto` that is XeSS, and FSR 3.1 stays FSR 3.1 (`Dx12Upscaler=fsr22` is the
  lightest choice on these chips). `[FSR] Fsr4ForceModel=2` still forces FSR 4 INT8 if you want to try it
  (experimental on these chips). RX 7000 desktop cards and RX 9000 are unchanged.
- **Shadow of the Tomb Raider (and games that make their D3D12 device twice) no longer crash when the upscaler
  starts.** The game hands its first device to NGX, releases it, and renders on a new one; OptiScaler kept the
  released device and the upscaler's first start crashed (seen on a ROG Ally, with XeSS and with FSR alike). OptiScaler
  now checks the device of the game's command list when it creates the upscaler and uses that one. The log shows
  `[CREATE]` lines for each step.

### Before you update: values in your own ini that now act differently
If your `OptiScaler.ini` holds these (non-default) values, the picture or the frame rate changes:
- `[DlssNr] AutoMask=false`: now turns danielblnc's character mask off, and lmxxf's (each change rebuilds lmxxf's
  network: about 1 s).
- `DlssNrLocalStructure` (Structure intensity) other than 1, an own Character structure (`DlssNrSkinStructure` other than
  -1), or an NR style that sets structure (Cinematic, Crisp, Natural, Vivid): now changes lmxxf's picture on every GPU
  (0.3.3.2's lmxxf ignored them); each change rebuilds the network (about 1 s).
- `AmdInterleaveModelHistory=true`: now also keeps danielblnc's network history with Model interleave off.
- `AmdEncoding` sRGB / Gamma 2.2, `AmdResidualFade` above 0, `AmdNetworkOutput=true` and `AmdRtgiEnabled=true` saved
  while on danielblnc: now also act under lmxxf (Screen-space GI adds GPU and VRAM cost).
- `AmdInterleavePacing` 0..1 (older builds had a slider): now also paces lmxxf with Model interleave on, so FPS drops.
  -1 (the default) stays off on lmxxf.
- `AmdLmxxfTierSnap` unset on RX 7000 / Radeon 8060S / handhelds: now on (above). Set `false` for 0.3.3.2's sizes.
- `AmdInterleave` unset on a handheld APU: the model runs every 4th frame.

### Defaults that differ from 0.3.3.2
- `[Log] KeepPreviousLogs=3` (0.3.3.2 kept one previous log); the "no clean exit" note no longer follows a normal
  quit; crash reporters and danielblnc's setup under any name are passed through.
- Menu: a second toggle key press within 400 ms is ignored; a game's low-level input hook is skipped (not swallowed)
  while the menu is open.
- Preset writes only NR resolution and Dynamic NR off; lmxxf's Classic carry is saved as 1; "DLSS w/Dx12" is hidden on
  non-NVIDIA cards; the Graphics wait row is gone; Reset appearance filter resets only `[AmdLook]`; the menu layout and
  labels above.
- Ray Regeneration: a fallback on the Unreal null-camera signature stays for the session; offered by default on
  RX 7000 / RDNA 3 again (0.3.3.2: RDNA 4 only; `[FSR-RR] FfxDenoiserAllowPreRdna4=false` restores that).
- RX 7000 / Radeon 8060S / handhelds: the lmxxf network size tier snap is on. Handhelds: 360p network size and the
  model every 4th frame.
- lmxxf keeps a job the game submits one frame late (`AmdLateSubmitGrace`); danielblnc runs on the device behind a
  Streamline proxy (`AmdStreamlineDeviceFix`); an NR resolution change alone no longer pauses NR.

### New ini keys
`AmdLmxxfTierSnap` (on RDNA 3), `AmdLateSubmitGrace`, `AmdStreamlineDeviceFix`, `KeepPreviousLogs`,
`MenuToggleDebounceMs` and `MenuLowLevelHookPassThrough` change the behaviour at their default (see "Defaults that differ"); the others keep 0.3.3.2's behaviour at their default.
"Not saved" = read at start, never written by Save Settings: set it by hand.
- `[DlssNr]` `AmdLmxxfTierSnap` (unset: on for RDNA 3, off elsewhere; not saved), `AmdLmxxfTierCap` (0 auto = 360 on
  handhelds, no cap elsewhere; 360 / 576 / 720 / 900 / 1080; not saved), `AmdDanielHighlightGuard` (false),
  `AmdRuntimeStyle`, `AmdToneCurve`, `AmdToneLift`, `AmdUseGameExposure` (-1 auto), `AmdEditShaperLimit` (0),
  `AmdEditShaperScope` (0), `AmdEditShaperCarryCap` (false; the three not saved), `AmdLmxxfFastMode` (false),
  `AmdDanielFastMode` (auto = the runtime's own mode), `AmdLateSubmitGrace` (true; not saved),
  `AmdStreamlineDeviceFix` (true; not saved).
- `[Hotfix]` `MenuToggleDebounceMs` (400, used as 0..1000), `MenuLowLevelHookPassThrough` (true),
  `WarnMissingREFramework` (true); not saved.
- `[Log]` `KeepPreviousLogs` (3, 1..5), `CrashHandler` (false), `CrashDump` (false); not saved.
- `[FSR-RR]` `FfxDenoiserPathTracedTemporal` (true), `FfxDenoiserAlbedoFp16` (false, not saved),
  `FfxDenoiserNgxDirectSLConstants` (false, not saved).
- `[AmdGi]` (AMDNR Screen GI; off at the defaults): `Enabled` (false), `Quality` (unset: High, Low on APUs),
  `Intensity` (1), `Occlusion` (1), `Radius` (1), `Thickness` (0 = auto), `Saturation` (1), `Sky` (0), `Feedback`
  (0.5), `Placement` (0 = before NR, the only one in the preview), `Fov` (0 = auto), `FovAxis` (0 = vertical),
  `NearFade` and `DistanceFade` (-1 = no fade), `Encoding` (-1 = auto); not saved: `DebugView` (0) and
  `DepthConvention`, `TraceCap`, `AlbedoMode`, `AoLitProtect`, `Translucency`.
- `OptiScaler.ini` documents all of them.

### Files
- `OptiScaler.dll`: AMD-NR v0.3.4. `LmxxfNrRuntime.dll`: 0.3.4 (Properties > Details).
- `LmxxfNrRuntime.pak` is new (440 MB, md5 `522fab211dd51916b81f4cbcde6effdb`): every entry of the 0.3.3.2 pak unchanged (the
  c32w kernels included), plus the handheld modules for gfx1103 and gfx1150 and lmxxf 0.31's
  kernels (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2); Kien, MIT). Replace all three files together; the forwarder and
  `Runtime.zip` are unchanged.
- The new `OptiScaler.dll` also runs with 0.3.3.2's `LmxxfNrRuntime.dll` (without the native character mask and the
  small sizes; handhelds need the new one), and the new `LmxxfNrRuntime.dll` accepts an older `OptiScaler.dll` on
  desktop GPUs.

### Known issues / not tested
- **Not yet tested in a game:** this build was checked on the desk (full builds, unit tests, the lmxxf probe's
  image hashes on an RX 9070 XT, the menu in a capture host). The in-game test is still to come, most of all
  for the new menu, the RX 7000 tier snap's look, Ray Regeneration by default on RX 7000, Assetto Corsa's input,
  Save report, the Fast modes, Midnight Suns and The Last of Us Part II.
- **AMDNR Screen GI (preview)** has run only in AMDNR's GPU lab (synthetic scenes on an RX 9070 XT, with D3D12
  GPU-based validation: no errors), not yet in a game. Known from the lab: the bounce into shadow is weak, ambient
  occlusion can darken lit surfaces a little, and still scenes can shimmer slightly.
- **Handhelds:** lmxxf has run on one handheld so far, a ROG Ally (Z1 Extreme): in lmxxf's probe (HIP found the chip
  with ASUS's Adrenalin driver, all modules loaded, no errors) and in one tester's run in a game, Shadow of the Tomb Raider (about 29 fps with NR
  on, see "Handhelds"). Not yet on a gfx1150 chip (Z2 Extreme / Radeon 890M / 880M). If a handheld maker's driver leaves out `amdhip64_7.dll`, the
  Neural tab says HIP is not available: install AMD's own Adrenalin driver.
- **The 360p and 576p sizes:** their look is not yet judged in a game.
- **danielblnc: the NR style can still change when NR resolution leaves 100%** (default unchanged; use the Edit
  shaper A/B and tell us which matches).
- **Ticking Network history mid-game on danielblnc** can freeze the picture for about 4 s (now also with Model
  interleave off); fix planned for 0.3.5.
- The Ray Regeneration diagnostic probe, `FfxDenoiserAlbedoFp16` and `FfxDenoiserNgxDirectSLConstants` have not run in
  a real game.
- The menu window still scrolls as a whole once it reaches its height cap.
- Frame generation: the stale-copy write-back and the "CopyResource error!" throttle in the FSR / XeFG / DLSSG copy
  paths are planned for 0.3.5.
- lmxxf keeps about 10-25 MB of VRAM per NR size change (known since 0.3.3.2).
- danielblnc in games that submit late (The Last of Us Part II) still runs NR on about half the frames: only the
  counter and the menu line are in 0.3.4; the fix is planned for 0.3.5.
- RX 6000 (RDNA 2) is planned for 0.4.0.


## 0.3.3.2 — 2026-09-26

Hotfix: lmxxf now runs on PCs with more than one AMD GPU (for example a Ryzen with its integrated graphics on) and is
faster on RX 9070 / 9070 XT (network 17.8 -> 15.3 ms at 1080p, same network output), Ray Reconstruction is offered on RX 9000
(RDNA 4) only by default so RX 7000 keeps the game's own denoiser, danielblnc's Residual strength no longer jumps at
100% NR resolution (leaving 100% still can change the look; that part is planned for 0.3.4), lmxxf keeps about 10-25 MB instead of about 100 MB of
VRAM per NR resolution change, Model interleave should no longer ghost (new default: Edit accumulation), and
Resident Evil Requiem warns when REFramework is missing and gets a skin mask that leaves streets and walls alone.
Replace `OptiScaler.dll`, `LmxxfNrRuntime.dll` and `LmxxfNrRuntime.pak`: all three changed.

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
  styles look the way their strength says. Away from 100% NR resolution (also Dynamic NR steps and the Balanced / Performance presets) strength, limit and
  edge fade still act on the whole result, as in the first 0.3.3.2 build: the new edit shaper, which applies them
  to the model's own edit instead, washed highlights out in a test (Forza Horizon 6, Classic, 115% NR), so it is
  now off by default (opt-in with `[DlssNr] AmdEditShaper=true`). So the style can still change when NR resolution leaves 100%;
  a full fix for that is planned for 0.3.4. Default at 100% runs the same code as before (not yet
  compared with a capture). lmxxf was not affected.
- **Dynamic NR resolution bounced between two steps.** It now steps down after 1.5 s over the target and back up
  only after 5 s of headroom, and waits a minute after a step up that could not hold. Its frame timer was too
  coarse above about 64 FPS and could push it to 50% at high targets; it now uses a precise timer, so at high
  targets you may see a higher NR resolution and a lower frame rate than before. At a target it can only just hold,
  it still tries a step up about once a minute. With an older `LmxxfNrRuntime.dll` (no buffer reuse, below) it stops
  changing after 12 changes per session; with this one there is no change cap, and on lmxxf each change still keeps
  about 10-25 MB of VRAM (below). Not yet tested in a game.
- **lmxxf: each NR resolution, DLSS mode or resolution change kept about 100 MB of VRAM and as much RAM until the
  game restarted.** An AMD HIP driver never gives back a D3D12 buffer that was imported and mapped, so
  `LmxxfNrRuntime.dll` now makes these buffers once per network size and reuses them (`lmxxf_backend.log`:
  `hip buffers ... imports N, reuses M`). A smaller rest of about 10-25 MB of VRAM per change remains (the first
  0.3.3.2 runtime has it too; its cause is not known yet), so after very many changes a restart still helps. With an
  older `LmxxfNrRuntime.dll` the menu says so. Leak probe on the desk: about 90 MB of VRAM and 70 MB of RAM per
  change with the first 0.3.3.2 runtime, about 10-25 MB of VRAM and no RAM with this one (40 changes: +0.45 GB).
  Not yet tested in a game.
- **FPS limit with frame generation above 2X.** OptiScaler's own frame limiter (used when Reflex / Anti-Lag / XeLL
  does not limit) assumed 2X, so XeFG 4X with a 120 FPS limit ran at 240. The limit is the displayed frame rate at
  every multiplier now (4X: the game runs at 30). 2X and FSR FG are unchanged; if you set a limit for 3X or more
  before, it now binds as labelled. Not yet tested in a game.
- **lmxxf could stay off for a whole session** when the game never submitted the list of the first neural frame,
  or submitted it through a wrapper. It now watches a newer frame's list after 8 frames, matches by COM identity
  after 16, and logs why. After a dropped list lmxxf's temporal history restarts instead of carrying on from a job
  that never ran. Found by reading the code; not seen in a game.
- **A neural command list the game throws away** (reset or released without running it) left danielblnc waiting
  for it for the rest of the session ("previous Record still awaits submission"), and could let lmxxf start a job
  on inputs that were never copied. AMDNR now notices such a list (it watches `Reset` on the one list NR last
  recorded into; OptiScaler's own D3D11 / Vulkan bridges report a list they did not run): danielblnc finishes the
  lost job without it and NR should come back after about 4 s, and lmxxf drops the job as a lost frame.
  `[DlssNr] AmdNeuralListRecovery=false` turns this off. Not yet tested in a game, so not confirmed as a fix yet:
  nothing reproduces a dropped list on demand. If `amd_presr.log` says "its job is completed without it", please
  send the log.
- **Quitting the game while NR runs.** NR stops the moment the game starts to exit, and AMDNR now waits at most about
  2 s for its runtime (lmxxf could wait up to 30 s for its queue before), also when the exit comes from a crash
  handler on the rendering thread. lmxxf over the Vulkan bridge: a clean quit during lmxxf's warm-up no longer
  turns lmxxf off at the next start. No ini switch. Not yet tested in a game.
- **lmxxf when the GPU is removed** (a driver reset or crash): lmxxf now stops at once and says so in
  `lmxxf_backend.log` (nothing is released or waited for), as danielblnc already did. No ini switch. Not yet
  tested in a game.
- **XeSS, FSR 2.2 and FSR 2.1.2 changed the resource state of AMDNR's NR output** (in Unreal Engine titles a wrong
  transition in and out). They now leave NR's output alone, as the FSR 4 / FSR 3.1 path already did
  (`[DlssNr] AmdNrColourGuard=false` restores the old behaviour). Not yet tested in a game.
- **NR after Ray Regeneration ran again on a frame the upscaler skipped** (its first frames, a backend change, the
  FSR 2.1.2 fallback), editing the previous output a second time. Such frames now get no neural pass and NR history
  restarts (`[DlssNr] AmdSkipUnwrittenFrames=false` restores the old behaviour). Not yet tested in a game.
- **Smaller resource-state fixes:** FSR 4's auto-exposure padding workaround left the game's colour in the wrong
  state; FSR Ray Regeneration issued a transition to the state a resource was already in; OptiScaler's Vulkan
  bridge in Unreal Engine titles transitioned its own copy of the colour from the wrong state. Not yet tested in a
  game.
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
  -> 6.9 ms), the same network output bit for bit.** New "c32w" one-wave kernels for the network's C32 layers ship in
  `LmxxfNrRuntime.pak`. `LmxxfNrRuntime.dll` checks their SHA-256 before it loads them and runs the old kernels if
  they do not match. lmxxf's status line and `lmxxf_backend.log` show `c32w=on` when they run (`c32w=off:<reason>`
  otherwise). With the old `LmxxfNrRuntime.pak` everything works as before, at the old speed (`c32w=off:nofile`).
  An old `DLSS5-AMD\native-game-tiled-assets` folder next to the game's exe does the same (see Notes).
  RX 9060 XT keeps the old kernels for now (the environment variable `AMDNR_C32W=1` turns them on there, untested);
  `AMDNR_C32W=0` turns them off everywhere. RX 7000 is not affected. Measured on the desk; not yet timed in a game.
  The c32w kernels are AMDNR's own work
  (Copyright (c) 2026 3zwr1 (AMDNR)); they run lmxxf's network (Kien, MIT). Acknowledgement: AMD's public RDNA 4
  WMMA documentation (AMD GPUOpen, the ROCm matrix instruction calculator), used for ideas only.
- **Resident Evil Requiem / PRAGMATA without REFramework: a warning.** When re9.exe, re9demo.exe or pragmata.exe
  runs without REFramework's dinput8.dll (none in the game folder, or Windows' own dinput8.dll was loaded),
  OptiScaler.log gets a warning, a notice shows for 20 s and the menu shows an orange line with the download link.
  Without REFramework, Resident Evil Requiem crashes 15-60 s after launch. The warning is not yet tested in a game.
- **Resident Evil Requiem skin smoothing: a Robust skin classifier (the new default).** The SSS guide alone marked
  25-39% of a night street as skin (walls, coats, umbrellas); the Robust classifier also asks how much the game's
  SSS pass moved the pixel, whether the surface around it agrees (held from the previous frame, so faces should not
  flicker; the hold is not yet confirmed in the game) and whether the albedo is skin-toned: 0.5-2.3% of the frame
  in the same scenes, with faces and hands still covered. Neural > Quality > Ray Regeneration > Skin classifier;
  "Guide only (0.3.3)" is the old mask.
  Keys `[FSR-RR] FfxDenoiserSkinSmoothingClassifier` (1 Robust, 0 guide only), `...MovedLow` / `...MovedHigh`
  (0.05 / 0.12) and `...ShowCues`.
- **Still-surface steadiness** (Neural > Quality, `[DlssNr] AmdStabilityStaticRelax`, both runtimes): steadies
  shadows and flat areas that pulse while nothing moves; only pixels that provably did not move are affected. Off by
  default in this release; try 0.5 if shadows flicker on still walls. Not used at Temporal stability 0, on
  danielblnc while Model interleave is on, or on lmxxf under Edit accumulation (the default interleave preset;
  lmxxf's Classic carry uses it). Not yet tested in a game.
- **DLSS Enabler options say where the DLL comes from.** AMDNR does not ship `dlss-enabler-headless.dll`; the Frame
  Gen options that need it now show how to get it (DLSS Enabler 4.9.0 or newer by Artur Graniszewski, its
  `version.dll` renamed) with a link, and the log says the same.
- **Newer danielblnc 0.4.x builds are supported, ahead of their release** (not yet tested in a game). This build drives danielblnc
  0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0 and the newer 0.4.x builds. The newest public build is 0.4.0: get it in
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
  every new one (`[DlssNr] AmdNrSizeStep`, 0 = exact sizes as before). 100% is unchanged. Not yet tested in a game.
- **Resident Evil Requiem, FSR Ray Regeneration: RR bias mask strength 0 by default.** Pixels the game flags in its
  DLSS bias mask are now denoised like the rest (a tester saw less blotching and grain at 0; not yet confirmed from a
  log that the game publishes the mask - if it does not, this changes nothing). A value you set in
  `[FSR-RR] FfxDenoiserBiasMaskStrength` wins, also 1; Save Settings now keeps an explicit 1 in every game.
- **Menu Scale:** the menu window, its sliders and combos now follow the Menu Scale (a smaller scale shrinks the
  window, not only the text); long notes (Frame Gen, XeFG 6X, native XeSS FG) wrap instead of running off the
  right edge; the FrameTime graph ends with "fps" again; and a window you dragged is pulled back on screen when it
  grows (a larger scale or a taller tab). The wrapping, the "fps" label and the pull-back are not yet tested in a
  game.
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
- **Remove an old `DLSS5-AMD\native-game-tiled-assets` folder.** A loose `DLSS5-AMD\native-game-tiled-assets`
  folder next to the game's exe (left from an earlier lmxxf setup) takes precedence over `LmxxfNrRuntime.pak`. It
  has no c32w kernels, so lmxxf runs at the old speed and its status line says `c32w=off:nofile`. Remove or rename
  the folder; the pak holds everything lmxxf needs.
- **Not yet tested in a game:** every entry above marked so was built and checked on the desk only (build,
  shader compile, the lmxxf probe's image hashes and timing, the leak probe); the in-game test of this build is
  still to come. Some cannot be reproduced on demand: recovery from a dropped neural list, the device gate on PCs
  with two GPUs, one stream per frame, the Proton / DXVK Vulkan change, and Ray Reconstruction on RX 7000 with
  `FfxDenoiserAllowPreRdna4=true`. Most new behaviour has an ini switch to turn it off (see above); the bounded
  exit, the GPU-removed stop and the FPS limit change have none.

## 0.3.3.1 — 2026-09-25

Hotfix: danielblnc's runtimes 0.3.3 and 0.4.0 are supported (and one newer 0.4.x build ahead of its release), and FSR Ray Regeneration's path-traced profile is
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
- **danielblnc 0.4.0 is supported, and one newer 0.4.x build ahead of its release.** Daniel shared both early, so AMDNR
  drove 0.4.0 the day he published it (2026-09-25; the public build is byte-identical to the one we mapped):
  the release has a new `v0.4.0-Runtime.zip` (his 0.4.0 runtime, unmodified, with his permission, plus the
  weights). His release notes claim 42% more speed than 0.3.3; its new fast kernels run on RX 9000 only.
  That newer build will work the day it comes out, with no AMDNR update needed. Each build was derived twice,
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
