# OptiScaler AMD-NR — v0.1.0

DLSS 5 Neural Rendering running on AMD, wired into OptiScaler so it works in any
Direct3D 12 game that OptiScaler already hooks.

Everything needed is in this archive. There is no second download.

---

## Install

1. Extract every file into the game folder, next to the game's `.exe`.
2. Run `install.bat` **only if the game does not load `dxgi.dll`** — it renames the
   proxy for you. Most D3D12 games need nothing.
3. Start the game and press **INSERT** for the menu. Neural Rendering is already on.

The `.exe` is usually not where the shortcut points. Unreal games keep it under
`<Game>\Binaries\Win64\`.

## Requirements

- An AMD GPU with a current driver. The neural runtime uses HIP through the driver;
  no HIP SDK and no developer mode are needed.
- A Direct3D 12 game. D3D11 and Vulkan titles reach OptiScaler through its bridges,
  but the AMD neural path itself is D3D12.
- About 2 GB of spare VRAM at 1080p-class render resolutions.


## What is in the box

| File | What it is |
|---|---|
| `dxgi.dll` | OptiScaler with the DLSS-NR AMD backend. Rename it if your game needs another proxy. |
| `OptiScaler.ini` | Settings. Neural Rendering is enabled; logging is on so a bug report has something to attach. |
| `dlssnr_amd_pass1..3.dll` | The AMD neural runtime, v0.3.1. Three copies so multi-pass has one per pass. |
| `dlssnr_on_amd_weights.bin` | The network weights the runtime loads. |
| `nvngx.dll_dlssnr.dll` | Forwarder for the NVIDIA path. Harmless on AMD. |
| `OptiScaler\` | FSR and XeSS libraries OptiScaler uses for upscaling and frame generation. |

## Settings worth knowing

Open the **Neural** tab. The defaults are the most recent tested arrangement, so the
useful first move is to change one thing at a time rather than five.

**NR resolution** — the main quality/cost lever. Below 100% the model works on a
smaller picture and only its *correction* is carried back up to the full-resolution
frame, so the frame keeps its own detail. Above 100% the model works on a *larger*
picture; cost grows with the square, so 150% is 2.25× the model time.

**Residual strength** — how much of the model's edit is applied. Above 1 it
amplifies. This is the effect-strength control the AMD path does not otherwise have,
and it is the one that changes the picture most.

**Residual limit** — a ceiling on how far one pixel may move. If you see blotchy
patches, **lower** this. It is a ceiling, so a higher number lets more through. The
patches come from the network itself: it works in tiles, and a tile where it
extrapolated rather than saw returns something far outside its usual range. Extra
passes make it worse, because each pass runs on top of the last one's blown tile.

**Model interleave** — runs the model every second frame for a large frame-rate gain.
The frames it skips have to be filled, and how they are filled is the **Interleave
preset**. This is the part still under active work; if it looks worse than leaving it
off in your game, that is worth reporting, not a sign you configured it wrongly.

**Residual temporal (no interleave)** — smooths the model's *edit* over time instead
of the picture. The network re-decides each pixel on every call, and on emissive
content its answer moves even when the scene does not; that is the flicker. Averaging
the picture to damp it is what makes ghosts. Averaging only the correction damps the
same jitter and cannot ghost, because every pixel of geometry on screen is still this
frame's.

**Neural passes** — 2 and 3 stack the model. Expect diminishing returns: it is a
denoiser, and a second pass finds much less left to do than the first.

## If something goes wrong

`OptiScaler.log` appears in the game folder. Attach it, and say which game and which
GPU. The AMD backend also writes `amd_presr.log` and `amd_bridge.log`, which are the
useful ones when the neural pass specifically misbehaves.

`SHA256SUMS.txt` lists every shipped file if you want to check the download.

---

## Credits

This build stands on three projects. It is a wiring job over their work, not a
replacement for it — if you find this useful, the thanks belong upstream.

**DLSS-NR on AMD** — *danielblnc*
<https://github.com/danielblnc/DLSS-NR-on-AMD/releases/tag/v0.3.1>
The neural runtime itself: `dlssnr_amd_pass1..3.dll` and `dlssnr_on_amd_weights.bin`
in this archive are his v0.3.1 release, redistributed unmodified so the package works
without a second download. Everything the network actually computes is his.


**DLSS 5 AMD project** — *TheAutomatic*
<https://github.com/TheAutomatic/dlss-5-amd-project>
Reference and groundwork for running DLSS 5 Neural Rendering on AMD hardware.

## Legal

OptiScaler is distributed under the licence in `LICENSE`; third-party library licences
are in `Licenses\`.

The AMD neural runtime and its weights are redistributed here under their original
authorship as credited above. They are included for convenience only — no ownership is
claimed over them, and no warranty is offered for them.

**NVIDIA's `nvngx_dlssnr.dll` is not in this archive and will not be.** It is NVIDIA's
file. The AMD path does not need it; it is only relevant if you run the NVIDIA
backend, in which case you supply your own copy.

None of this is endorsed by, affiliated with, or supported by NVIDIA, AMD, or any game
publisher. It drives an undocumented feature directly. Use it at your own risk.
