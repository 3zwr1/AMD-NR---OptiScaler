# AMDNR

**AMDNR is based on [OptiScaler](https://github.com/optiscaler/OptiScaler) (GPL-3.0) and keeps its
licence.** It is a fork that adds DLSS Neural Rendering ("DLSS-NR") support on AMD Radeon GPUs: the
NR pipeline, the runtime host and bridge, the Neural tab of the menu, and the AMD-side shader work
around them. Everything else is OptiScaler.

This repository is the **corresponding source** of the OptiScaler.dll shipped in AMDNR **0.3.4.2**,
published for GPL-3.0 compliance. It is a snapshot, not the development history.

---

## What this repository contains

* `OptiScaler/` — the full source of the shipped `OptiScaler.dll`: upstream OptiScaler plus AMDNR's
  changes, and AMDNR's own modules:
  * `dlssnr/amd/` — the AMD NR pipeline: the runtime bridge, pre-SR multipass, interleave, temporal
    stability, residual and colour work, the graphics-state hooks.
  * `dlssnr/lmxxf/` — the **host side** of the lmxxf-based runtime (the loader, the tier policy, the
    frame submission), plus `runtime/LmxxfNrApi.h`, the C ABI header the host compiles against.
  * `dlssnr/menu/`, `menu/`, `menu/input/` — the AMDNR menu and its input layer.
  * `dlssnr/gi/` — AMDNR Screen GI, a clean-room screen-space GI module (see
    `dlssnr/gi/CLEANROOM.md`); the HLSL sources and the generated headers are both here.
  * `dlssnr/forwarder/` — the small `nvngx.dll_dlssnr.dll` forwarder.
  * `upscalers/`, `framegen/`, `hooks/`, `shaders/`, … — OptiScaler's own code, with AMDNR's
    modifications marked by a `Modifications Copyright (c) 2026 3zwr1 (AMDNR)` line.
* `external/` — the third-party libraries the build needs, as their authors ship them, each under its
  own licence. Upstream OptiScaler carries these as git submodules; they are vendored here so the
  snapshot builds as-is.
* `OptiScaler/library/` — the prebuilt FidelityFX FSR 2 / FSR 2.1.2 / FSR 3.1, Vulkan, D3DX and
  Detours import libraries the linker needs. These are third-party build inputs, unchanged.
* `redist/`, `OptiScaler.ini`, `setup_windows.bat`, `setup_linux.sh` — packaging inputs used by the
  project's post-build step.
* `Licenses/` — `AMDNR_NOTICE.txt` (AMDNR's copyright and its GPL section-7 terms),
  `AMDNR_RDNA3_BACKEND.txt`, `RenoDX_ATTRIBUTION.txt`, `lmxxf_LICENSE.txt`.
* `LICENSE` — the GNU GPL version 3.
* `Changelog.md` — AMDNR's release history.

## What this repository does **not** contain

The neural runtimes are **separate programs** with their own licences. `OptiScaler.dll` does not
contain them: it loads the runtime DLL at run time through the C ABI in
`OptiScaler/dlssnr/lmxxf/runtime/LmxxfNrApi.h`, so the DLL here builds and links without any of them.

* **danielblnc's runtime** — *DLSS-NR on AMD* by Daniel Blanco (danielblnc) is his own closed work.
  AMDNR ships it with his permission, under his terms. No part of it is in this repository.
* **The lmxxf-based runtime** — lmxxf's network port and kernels (Kien, MIT, including the portions
  contributed to lmxxf by TheAutomatic, MIT) together with AMDNR's RDNA 3 and RDNA 4 kernel work are
  a separate package (`LmxxfNrRuntime.dll` and `LmxxfNrRuntime.pak`). Its sources
  (`OptiScaler/dlssnr/lmxxf/runtime/**`), every HIP kernel, and every `.pak` / `.hsaco` / weight
  file are **excluded** here; only the API header the host needs to compile is kept, with its
  credit line intact.

Also not here: AMDNR's private tests, research notes, capture-analysis tooling, release packaging
scripts and the offline shader lab. None of them is part of `OptiScaler.dll`.

## Building

Requirements:

* Visual Studio 2022 with the **Desktop development with C++** workload (MSVC v143, Windows 10/11
  SDK). The project builds as C++ latest.
* Windows 10 or 11, x64.

From a *x64 Native Tools Command Prompt for VS 2022* (or after running `vcvars64.bat`) at the
repository root:

```
msbuild OptiScaler.sln /t:Build /p:Configuration=Release /p:Platform=x64 /m
```

The result is `x64\Release\a\OptiScaler.dll`, together with the packaging layout the post-build step
assembles next to it. `Debug` and `ReleaseDebug` configurations are also defined.

Two notes:

* The pre-build step writes `OptiScaler/resource_build_date.h` and
  `OptiScaler/resource_build_commit.h` from the clock and from `git rev-parse`. They are committed
  here with the placeholder value `"unknown"`, so a build from this snapshot differs from the
  released binary only in that version stamp.
* The shaders under `OptiScaler/shaders/**/precompile/` and `OptiScaler/dlssnr/gi/precompiled/` are
  committed as generated headers. The scripts that regenerate them
  (`shaders/shader_tools/`, `dlssnr/gi/shaders/build_gi_shaders.py`) need the Windows SDK's `dxc` /
  `fxc`; point `AMDNR_DXC` at `dxc.exe` or put it on `PATH`.

## Licence

GPL-3.0-or-later. AMDNR's own files carry
`SPDX-License-Identifier: GPL-3.0-or-later` and `Copyright (c) 2026 3zwr1 (AMDNR)`; upstream files
AMDNR changed carry a `Modifications Copyright (c) 2026 3zwr1 (AMDNR)` line under their existing
notice. Upstream notices are unchanged.

`Licenses/AMDNR_NOTICE.txt` adds the three terms that GPL-3.0 section 7 permits, and nothing else:
**(b)** a copy or derivative that contains AMDNR material must preserve this notice, AMDNR's
copyright lines, and the credit *AMDNR by 3zwr1 — https://github.com/3zwr1/AMD-NR---OptiScaler*
along with the credits of the authors listed in the notice, in its README and wherever it shows
credits or an About section; **(c)** modified versions must be marked as changed and must not be
presented as the original AMDNR; **(e)** no rights are granted to the AMDNR or 3zwr1 names or logo,
so using them to endorse or promote a modified version needs permission — removing them from a
modified version is always allowed. These are additional permitted terms, not restrictions, and
section 7 lets you remove them from any copy you convey.

## Credits

TheAutomatic (DLSS 5 AMD project) · danielblnc (DLSS-NR on AMD) · lmxxf
(dlss5-on-amd-9070xt-porting) · Matheus (dlss-5-amd) · Zach Hembree (DarkHelmet) and burak113 (FSR
Ray Regeneration for OptiScaler, branch ffx-denoise-experimental) · OptiScaler (Overclockers). Licence
GPL-3.0; third-party licences in `Licenses\`. The "c32w" RDNA 4 kernels (0.3.3.2) are AMDNR's own work,
Copyright (c) 2026 3zwr1 (AMDNR); they run lmxxf's network (Kien, MIT), with thanks to AMD's public
RDNA 4 WMMA documentation (AMD GPUOpen, ROCm matrix instruction calculator) for ideas.
