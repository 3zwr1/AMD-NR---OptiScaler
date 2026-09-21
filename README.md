DLSS 5 Neural Rendering on AMD, built into OptiScaler so it works in the games OptiScaler
already hooks. Two downloads: `AMDNR-vX.X.X.zip` (this build) and `Runtime.zip` (danielblnc's
v0.3.1 runtime and weights, unmodified). Installation guide in `README.md`.

**Discord: <https://discord.gg/QzbzxfKYyh>** — support, `#bug-report`, test builds, and NVIDIA's
`nvngx_dlssnr.dll` if you need it (not in these archives; the AMD path does not need it).

**Support the project: <https://ko-fi.com/3zinr>**

Parts of this are still being worked on, and those parts say so below rather than in a
changelog nobody reads.

---

# New in v0.2.0

## Ghosting, properly this time

v0.1.0 shipped with ghosting behind moving objects, and the fix turned out not to be a
better detector — it was asking a question we had never asked.

Every guard in the temporal pass tried to *detect* content whose motion vectors do not
describe it: by depth, by vector divergence, by colour. None of them can, because that
content — smoke, particles, transparencies, contact shadows — is composited **after** the
velocity pass, so every vector involved is finite, smooth, agrees with its neighbours, and
wrong. There is no local evidence left to find.

The renderer already knows. A **reactive mask** is the channel where a game marks exactly
those pixels, FSR and DLSS both consume one for this purpose, and the mask was already
sitting in the parameter table unread. It is read now, and a filled frame simply does not
reuse history where the game says the pixel is unpredictable.

Three more, each a separate cause found from a separate report:

- **Velocity dilation** — a pixel uses the motion vector of the nearest-to-camera pixel in
  its neighbourhood. Without it a road pixel beside a car reprojects along the road, lands
  where the car was, and returns car colour.
- **Clamped Catmull-Rom on the residual upsample.** The dark rim beside characters was not
  temporal at all: Catmull-Rom carries negative weights, so at a hard edge it reconstructs
  values outside the range of every sample it read. It is now clamped back into that range,
  which costs nothing it was chosen for.
- **The carried edit is bounded against the current edit** in Residual temporal, which is
  what was drawing a car's shadow backwards along the road like a reflection.
- **The ghost bound no longer flickers when pushed.** A tester found that at the top of the
  slider the picture flickered and at 0 the ghost came back. The top of the range had
  become a 1-sigma box, and a legitimate one-pixel detail sits 2.83 sigma from its own
  neighbourhood, so the bound was clipping real detail on filled frames only. It now
  never goes below 3 sigma, and where it fires it replaces the pixel with the current
  frame - the same thing every other guard falls back to - instead of leaving a fainter
  ghost at the edge of the box.

## Model interleave: a new default that cannot ghost

Every interleave preset so far reprojected the last model frame and then argued with it:
depth, motion divergence, the reactive mask, a colour bound - each a guard deciding whether
the old picture may be shown at a pixel. Each guard was born from a ghost and each produced a
flicker, because a guard is a per-pixel decision and a decision can land differently on a
model frame than on a filled one. Ghosting and flicker were one bug seen from two sides.

**Guided fill** never shows the old picture. A skipped frame is a filter of *this* frame's raw
colour, where each neighbour's weight comes from how similar it looks in the reprojected
denoised history. Where the reprojection is right the result looks denoised; where it is
wrong the filter merely smooths in a slightly wrong direction. It cannot show a car where the
car used to be, because no colour of the old frame is ever copied. The model frame runs the
same filter on its own raw input, and what the model did beyond denoising - tone, structure -
is carried to the skipped frame and added back, bounded. Both frame types are the same kind
of image, built the same way, which is what stops the flicker.

"Guided fill" in the list is that arrangement under the name testers already know; the
experimental history-guided filter it was first named for is not active in this release.

**Lights, sky and signs pulsing with interleave, worse above 115% NR resolution or with 2-3
passes.** Both presets were cutting the model's legitimate tonal lift on filled frames only:
Held frame's ghost bound has no tolerance on flat content (where the lift is largest), and
Guided fill clipped its carried edit to the Residual limit. Held frame's bound now fires only
where the history carries edges the current frame lacks - a ghost - and never on a flat
offset; Guided fill no longer clips the edit, and its model frames are damped against the
previous model frame's edit so the network's own per-call re-decision on emissive content
(which extra passes compound) no longer lands at half the frame rate.

**Correction:** the first build that listed Guided fill clamped the preset back to Held frame
on its way to the backend, so every "Guided fill" test before this build was Held frame under
a different label. The clamp is fixed here; this is the first build in which Guided fill runs.

A first tester clip (The Last of Us, camera turning) showed filled frames coming out softer
than model frames under motion: the history that guides the filter was being read bilinearly
at fractional positions, which blurs the one image whose edges decide the weights. The guide
is now read at whole texels, and the carried edit is resampled with the same clamped
Catmull-Rom the history uses instead of bilinear. At rest the two frame types were already
identical; this is about motion. If it still looks worse than Held frame in your game, switch
back and say so.

## Highlight proxy (experimental, off)

**Bounded decode.** A Silent Hill 2 capture with the proxy on showed the overcast sky coming back
at the decode ceiling (knee + 199 x range) on every model frame, and the tone LUT then learned
that - grey blocks in the sky on skipped frames. The inverse curve is steep near the top of its
range, so a small nudge by the model to a compressed value decodes to a huge one. The decode is
now bounded to twice the brightest raw value in the pixel's 3x3 neighbourhood: the model may
brighten a highlight, but cannot create a light from nothing.

`AmdHighlightProxy=1` (menu: Neural, advanced, "Highlight proxy") squeezes the frame above a knee
with a reversible curve before the model sees it and decodes the model's answer with the exact
inverse; below the knee nothing changes. It exists because a capture showed the model returning
bright content at about half its brightness when shown linear HDR. Off by default until measured.
The frame-capture tool these measurements come from has no menu buttons in this release; testers
reach it through `[DlssNr] CaptureKey` in the ini (unbound by default) or an `amd-nr-capture.trigger`
file, and every capture lands in its own time-stamped `amd-nr-capture-<date>-<time>` folder.

## XeFG multi-frame generation, built in

3X and 4X XeSS frame generation on AMD (and any non-Intel card) no longer needs the third-party
`XeFGUnlock.asi` plugin: the same unlock is built into this build (`XeFG\UnlockMFG`, on by
default), and it reaches **every** `libxess_fg.dll` in the process - the copy OptiScaler uses and
a game's own copy, so a game's native XeSS 3 menu (Cyberpunk 2077) offers 3X/4X on AMD as well.
Each copy is patched in memory the moment it loads, every site verified before and after the
write and the whole set rolled back on any mismatch; the file on disk is never touched. Two
provider builds are known, 1.3.1.78 (XeSS SDK 3.0.1/3.0.2, shipped here) and 1.3.1.68 (SDK
3.0.0); anything else is refused and stays at 2X. **Delete `XeFGUnlock.asi` from your plugins
folder**: the loader now skips it, because two copies of the same patch in one process is a
crash at swapchain init (a copy the plugin already patched is recognised and left alone). The
ceiling is `XeFG\MaxInterpolatedFrames` (5 = 6X by default; 3 = 4X is the value the plugin's users
ran, so if 5X/6X misbehaves that is the number to go back to; at those multipliers cap the frame
rate or the extra frames only add latency).

**3X/4X pacing.** Above 2X the provider presents a burst of generated frames back to back and
then holds the last one, which reads as judder, and it is told a render time that includes its
own burst, which is a feedback loop: input lag grows and the frame rate drops periodically (the
Resident Evil report). `XeFG\ExtraPacing` (on by default, checkbox under the XeFG heading) routes
each generated frame through the provider's own scheduler and hands it a render time with the
burst taken out. It applies to OptiScaler's own 1.3.1.78 copy and takes effect on the next launch.
Both pieces are ported from Coldwood1026's XeFGUnlock (Dedushque/OptiScaler pull request 1, GPL-3.0).

## Ray Regeneration (experimental)

AMD **FSR Ray Regeneration** driven from a title's DLSS Ray Reconstruction inputs, selectable
in Upscaling beside the other backends.

**It appears on any non-NVIDIA card** with the 1.2 denoiser provider loaded. AMD documents Ray
Regeneration for RDNA 4 (RX 9000); on other cards the denoiser is tried and, if it will not
initialise, the upscaler falls back to FSR 2.1.2 with the reason in the log. And if the title
turns out not to publish Ray Reconstruction inputs it falls back to FSR with the reason written
to the log rather than failing silently.

**Honest status: this has never run in a shipping game.** It compiles, it is wired, its gates
are tested, and no frame has ever gone through it. Treat it as something to try and report on,
not something to rely on. It needs `amd_fidelityfx_denoiser_dx12.dll` from AMD's FidelityFX
SDK 2.3 in the game's `OptiScaler` folder; this archive ships that file (MIT, see `Licenses`).

**It now works with DLSS spoofing.** A game asks Streamline whether Ray Reconstruction is
supported before it offers the option, and Streamline answered from the adapter - so on an
AMD card the option never appeared and the feature Ray Regeneration is driven from was never
created. Where Ray Regeneration can serve it (a non-NVIDIA card + the 1.2 denoiser), the `sl.dlss_d`
plugin now loads under a spoofed architecture and Streamline reports RR as supported; the
game creates the RR feature, and on a card that can serve it (non-NVIDIA + the denoiser DLL) that
feature IS FSR Ray Regeneration - no `Dx12Upscaler=fsr_rr` needed any more. A game that asks NGX
directly (not Streamline) whether RR is supported is now told yes on the same cards.
Untested in a shipping game, like the rest of RR. (A first cut of this broke the game's own DLSS
frame generation on AMD cards that could serve RR; that is fixed in this build.)

**Guided fill v2 (new default preset).** With 2 or 3 passes, or NR resolution above about 115%,
lights, signs, sky and car lamps pulsed at half the frame rate under model interleave. Measured on
a capture: the model darkens highlights (about half the raw brightness on flat bright content), and
Guided fill's ghost bound compared the model's picture against the raw frame, so on those pixels a
skipped frame was handed back to the raw: dark lamp, bright lamp, dark lamp. v2 is the same picture
with that removed: the bound compares raw against raw with a tolerance that follows the frame's own
noise (in a grainy, ray-traced title the old box was so wide that a whole jacket held over the road
passed as the same surface - the Silent Hill 2 ghost), and every skipped-frame fallback wears the
model's tone, measured on the last model frame as a luminance curve rather than carried from another
position, so a fallback patch is never a brighter patch and never a ghost. Real ghosts still fail the
test. The Ghost bound and Interleave pacing sliders are gone from the menu: v2 needs no bound
setting, and pacing now engages on its own in proportion to the measured model/fill split (the
readout stays; `AmdInterleavePacing` in the ini: -1 automatic, 0 off, 0..1 fixed). The previous "Guided fill" stays in the list for comparison.

**Character ghost under v2 (fixed).** Captures taken mid-walk in Silent Hill 2 showed James's
jacket and hair holding the PREVIOUS model frame's pose on every skipped frame while the raw
already showed the next one - the character animating at half rate, read as ghosting. The
raw-against-raw tolerance was blind there for three measured reasons: it used the raw's own 3x3
sigma, which on hair and folds is the texture's contrast rather than the grain; its constant floor
(0.02 in tonemapped units) was most of the signal on a dark jacket; and it needed twice the
tolerance to fall back fully. Now the noise term is the raw's grain in excess of the denoised
history (texture cancels), the floor scales with the luminance, a small allowance covers the raw's
own jitter on textured surfaces, and the fallback is complete at 1.5x the tolerance. Skipped frames
show slightly more of the raw's grain on moving textured content; they no longer show an old pose.

**It only works in a game that uses DLSS Ray Reconstruction.** A game that only has DLSS
Super Resolution (Silent Hill 2, for one) publishes none of the inputs a denoiser needs, and
picking it there used to freeze the picture under a stack of "Upscaler failed to run" errors.
Now the menu refuses with a message and uses FSR instead; and if a game's RR inputs stop
arriving mid-session the frame is still upscaled and it switches to FSR after 30 frames, with
the reason in the log. Setting `Dx12Upscaler=fsr_rr` in the ini is honoured in RR titles.

**Neural Rendering now runs with Ray Regeneration.** The AMD path used to refuse a Ray
Reconstruction feature outright ("select Super Resolution"). Now, when a game's DLSS Ray
Reconstruction is served by FSR Ray Regeneration, the model runs AFTER it: on the finished
output at display resolution, with the render-resolution depth and motion vectors resampled
onto the model's grid, and the result is written back into the game's output (a copy for
half-float outputs, a typed write otherwise). Everything else - model interleave, Guided fill
v2, the encoding, the highlight proxy, captures - works as before. Because the model now sees
the display-sized frame it costs more per frame than before Super Resolution; lower the NR
resolution in the menu if the frame rate drops. Proof line in `amd_presr.log`:
`Recorded post-RR WxH passes=N (output WxH, guides WxH)`.

## Direct3D 11

D3D11 games reach the neural pass through OptiScaler's D3D11-on-D3D12 bridge. **Pick a
backend marked "w/Dx12"** — the native D3D11 backends run on the game's own device and never
reach the pass. The menu now says so when you are in a D3D11 game with the wrong one selected,
instead of quietly doing nothing.

## Also fixed

- **Multi-frame generation in Streamline titles.** The stub DLSSG that stands in on AMD
  reported a maximum of 1 generated frame, so games like Cyberpunk never offered multi-frame
  generation at all and no setting could undo it. It now reports what the output backend
  actually supports.
- **The NR cost readout prices resolution and passes together.** 150% resolution with 3 passes
  is 6.75× the model's time; the old line said 2.25×, which was true about the resolution and a
  third of the bill.
- A name collision in the tree that made any file including the resource tracker fail to
  compile if it also named the upscaler type.
- **The diagnostic frame capture no longer runs unasked.** It was running on every AMD
  install: five times a session it copied eight frames to readback memory and wrote
  60-130 MB to disk on the render thread, and when the sample was dark it retried with no
  limit - in a dark game, every half second. It is now off by default, honours the
  `AutoCapture` setting when on, and gives up after a few dark samples.
- **A depth view could remove the device (`DXGI_ERROR_INVALID_CALL`)** in games whose depth is
  `D32_FLOAT_S8X24` (the Nixxes ports, The Last of Us Part I and II): a depth copy that already
  carried the depth-plane view format was run through the typeless translator, which handed
  back the depth-stencil format, and that is not a legal shader-resource format. The view
  format is now kept as-is (the same fix the fork's author landed upstream as #1157). And a depth
  buffer the game created without shader access is now copied into a readable twin each frame
  instead of being handed to the model as-is. If Part II still shows that dialog on this build,
  send `OptiScaler.log` from the crashing run.
- **Neural Rendering silently did nothing in games with a typed depth-stencil buffer**
  (The Last of Us Part I allocates `D32_FLOAT_S8X24_UINT`). With NR resolution below 100%
  the pass refused the depth, and the refusal switched the backend off for the whole
  session with the reason in a log nobody reads. Typed depth formats are read now, a
  depth the scaled path really cannot read forces 100% instead of stopping, and a
  stopped backend says so in the menu's status line.

---

## Game notes

**Periodic frame-rate drops in RE Engine games (Resident Evil Requiem, Pragmata) with the
XeFG unlocker.** A tester traced "FPS normal for a while, then drops hard, back up, drops
again" to the `XeFGUnlock.asi` plugin: removing it and switching frame generation to FSR FG
made the drops go away. That pattern is the 3X/4X frame-time feedback loop described under
"XeFG multi-frame generation, built in"; this build skips the plugin and paces the burst itself,
so XeFG at 3X/4X is worth trying again there. If the drops return, FSR FG remains the way out.

**The Last of Us Part I crashes on boot.** This is the game's own Streamline 1.x init and is
a known OptiScaler issue, not something this build adds - the crash happens before any of
the neural code runs. OptiScaler's wiki page for the game gives the fix: rename
`sl.common.dll` in the game folder to `sl.common.dll.bak`, select **FSR 3.1** in the game's
settings (OptiScaler takes that input), and enable frame generation only after the game
boots cleanly once.

## What this adds that the standalone runtime does not

### Residual composition

The model's **change** is carried to the full-resolution frame instead of its picture.
The frame keeps its own detail; the reduced raster contributes the edit and nothing
else. This is what makes a low NR resolution usable rather than just soft — the usual
complaint about lowering it.

Three controls, all of which act on the edit rather than the image:

- **Residual strength** — above 1 it *amplifies* what the model changed. The AMD runtime
  exposes no intensity parameter of its own, so this is the only way to ask it for more,
  and unlike a contrast or saturation slider it touches only what the model actually did.
- **Residual limit** — a ceiling on how far one pixel may move, applied to the whole
  colour rather than per channel. A per-channel clamp lets whichever channel hits the
  limit first decide the hue of the rest, which turns an over-large edit into a
  *wrong-coloured* one.
- **Residual edge fade** — rolls the edit off at the frame border, where the reduced
  raster has no neighbour and the cubic resample rings on the missing data.

The residual is resampled with Catmull-Rom, not bilinear. It is the only thing carried
up from the reduced raster, and a 2×2 average destroys the finest of it first — which is
precisely what the model exists to synthesise.

### Model interleave

Runs the model every second frame and fills the rest by reprojection, for a large
frame-rate gain. How the skipped frames are filled is selectable:

- **Held frame** (default) — nothing is gated. Both frame types are pictures of the same
  kind, so no per-pixel decision exists that could land differently on one than the
  other. A decision that flips at the cadence *is* flicker, which is why the guards that
  were supposed to prevent it were producing it.
- **Residual temporal** — reprojects the model's edit instead of its picture. The
  geometry on screen is always this frame's; only the correction is a frame behind.
- **Standard** / **Extra temporal** — the earlier approaches, kept for comparison.

### Interleave pacing

The reason interleave could feel choppy while the picture looked smooth, and the dial
that fixes it.

A model frame costs more than a filled one, so with interleave on the frame *time*
alternates long/short even though both frames look equally good. Engines step their
simulation with the **previous** frame's duration, so every long camera step gets shown
for a short time and every short step for a long one — the motion on screen is wrong by
the difference, alternating in sign, every frame. That is the judder. It was never an
image problem, which is why no image setting had ever moved it.

The pacer measures the two frame types separately and pads the filled frame out. The
trade is real and it is yours to set: `0` leaves the sawtooth alone, `1` pads every
filled frame to a model frame's cost — perfectly even, and exactly the frame rate you
get with interleave switched off. What makes the middle worth having is that the win is
not proportional: once the interval stops oscillating the engine's own timing settles,
and that happens well before the fully-even end.

Off by default, because it changes your frame rate. The menu prints the measurement live
under the slider — your model frame, your filled frame, the paced interval and the frame
rate it lands on — so the cost of the next notch is never a guess. Inactive with frame
generation on: FG paces its own presents and two pacers fight.

### Residual temporal (without interleave)

Smooths the model's **edit** over time and leaves the picture strictly alone.

The network re-decides every pixel on each call, and on emissive content its answer moves
between calls even when the scene does not — that is the flicker. Averaging the *picture*
to damp it is what makes ghosts, because averaging a picture means showing an old one.
Averaging only the *correction* damps the same jitter and cannot ghost: every pixel of
geometry on screen is still this frame's, at full detail.

### Reprojection that knows when it is wrong

Three guards, all geometric. None of them looks at colour — a colour test compares a
denoised frame against an un-denoised one and answers differently depending on which it
got, and that difference is the cadence flicker.

- **Velocity dilation** — each pixel uses the motion vector of the nearest-to-camera pixel
  in its neighbourhood, not its own. Without it, a road pixel against a car's silhouette
  reprojects along the road, lands where the car was, and comes back with car colour —
  a second copy of the car smeared beside it, every frame.
- **Temporal motion divergence** — follows the pixel's vector back and reads the vector
  that was there a frame ago. A pixel that claims to have come from somewhere that was
  standing still is contradicting itself, and that contradiction is how a light's trail
  gets seeded.
- **Depth disocclusion**, ramped rather than binary. A boolean reject draws the outline of
  the region it rejected, and that outline is its own artefact.

### NR resolution 25–150%

Below 100% the model works small and the residual carries its work up. Above 100% it
works on a *larger* picture than the frame. Cost grows with the square, and the menu says
so live under the slider — 125% is 1.56× the model's time, which is not obvious from the
number.

### Also

- **1–3 neural passes.**
- **Dynamic NR resolution** — stepped, holds a frame-time target.
- **Graphics wait** — the runtime's one-pixel-draw wait, with the rasteriser and
  output-merger state recorded and restored. It refuses outright on any command list whose
  state is not fully known and falls back to the compute wait, so the worst case is the
  old behaviour rather than a glitch.
- **Game profiles** and quality presets.
- **Live readout** — what the pass is actually doing: NR frames/s, model frames/s, the
  cadence you are really getting, and frames that carried no NR at all.

---

## Credits

This build is a wiring job over other people's work. If you find it useful, the thanks belong
upstream.

- **[DLSS-NR on AMD](https://github.com/danielblnc/DLSS-NR-on-AMD)** — *danielblnc*
  The AMD neural runtime and weights in `Runtime.zip` are his v0.3.1 release, redistributed
  unmodified. Everything the network actually computes is his.
- **[DLSS 5 AMD project](https://github.com/TheAutomatic/dlss-5-amd-project)** — *TheAutomatic*
  Groundwork and reference for DLSS 5 Neural Rendering on AMD hardware; the lmxxf backend
  integration planned for 0.3.0 follows his work.
- **[Matheus / dlss-5-amd](https://github.com/MatheusGViana/dlss-5-amd-project)**
  The AMD pre-SR bridge this tree descends from.
- **[lmxxf / dlss5-on-amd-9070xt-porting](https://github.com/lmxxf/dlss5-on-amd-9070xt-porting)**
  Open-source HIP neural rendering runtime (MIT); the difference-gated temporal mode here
  follows his `native_output_smooth`.
- **[OptiScaler](https://github.com/Overclockers/OptiScaler-Releases)** — *Overclockers* and contributors
  The framework this is built into: the hooking, the FSR/XeSS/frame-generation plumbing,
  the menu, and the game compatibility that makes any of it reachable.

Code lineage: OptiScaler → Dagherbou / OptiScaler_DLSSNR → wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass
→ Matheus / dlss-5-amd → this build. The temporal guards are adapted from AMD's FidelityFX FSR2,
the colour clip is Playdead's, the XeFG unlock and pacing are ported from Coldwood1026's
XeFGUnlock (GPL-3.0), and the cube-scaling gamut handling is *hhkbble*'s.

## Legal

`nvngx_dlssnr.dll` is **not** in these archives — it is NVIDIA's file, and the AMD path does
not need it; if you need it for anything, ask in the Discord. The AMD runtime and its weights are redistributed under
their original authorship as credited above, for convenience only, with no ownership
claimed and no warranty offered.

Not endorsed by, affiliated with, or supported by NVIDIA, AMD, or any game publisher.
This drives an undocumented feature directly. Use at your own risk.

## Reporting

Logging is on by default in this build. `OptiScaler.log` appears in the game folder —
attach it in `#bug-report` on the Discord, and say which game and which GPU. `amd_presr.log` and `amd_bridge.log` are the
useful ones when the neural pass specifically misbehaves.
