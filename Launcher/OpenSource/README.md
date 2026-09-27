# AMD NR Launcher

The installer for **AMDNR** — OptiScaler with AMD Neural Rendering — by 3zwr1. It finds the games on
your PC (Steam, Epic, the Xbox app, Ubisoft Connect, the EA app, GOG, Rockstar, Battle.net and Amazon
Games, on every drive), picks the right DLL name for each one, downloads
the build you choose and puts it in place; a Doctor checks every install and a report can be sent for
support. It replaces the manual "extract two zips and rename OptiScaler.dll" guide.

- **Download:** the latest `AMDNR-Launcher.exe` from the
  [releases](https://github.com/3zwr1/AMD-NR---OptiScaler/releases) of the AMDNR repository.
- **Help:** the [AMD NR Discord](https://discord.gg/AMDNR) — the Discord mark in the launcher's
  title bar opens it.
- **Requirements:** Windows 10/11 x64 and an AMD graphics card. DX11 and DX12 games only.

## What it installs

On first start the launcher asks which build to install — **AMDNR** by 3zwr1, or **OptiScaler AMD
pre-SR** by [TheAutomatic](https://github.com/TheAutomatic/dlss-5-amd-project) — and whether to
download the **DLSSNR AMD files**: [DLSS-NR on AMD](https://github.com/danielblnc/DLSS-NR-on-AMD) by
Daniel Blanco (danielblnc), Copyright (c) 2026 Daniel Blanco, shipped unmodified with his permission.
Everything is fetched from the authors' own GitHub releases and verified by SHA-256 before a single
file is placed. Every file the launcher moves aside goes into `AMDNR_backup` inside the game folder,
and UNINSTALL puts the folder back.

## Source code

The source is published so that anyone can read what the launcher does. It is **not open source**:
Copyright (c) 2026 3zwr1 (AMDNR), all rights reserved. You may read it and build it unmodified for
yourself; you may not copy it, adapt it, reuse any part of it in another program, use it as a
template for a similar one, redistribute it, or feed it to an AI model. The full terms are in
[LICENSE.txt](LICENSE.txt). The mods the launcher installs keep their own licences (OptiScaler and
AMDNR are GPL-3.0).

Building: .NET 8 SDK, then `dotnet test AmdnrLauncher.sln` and `publish.cmd`. `RELEASING.md` is the
release procedure and `TESTING.md` the manual test matrix.

## Credits

Daniel Blanco (danielblnc) — DLSS-NR on AMD · lmxxf — HIP runtime (MIT) · TheAutomatic — pre-SR build
and asset layout · Matheus — dlss-5-amd · Nukem9 — dlssg-to-fsr3 (GPLv3) · OptiScaler — the upscaler
all of this builds on. The Discord mark is Discord's, used to link to the server.

Not affiliated with or endorsed by AMD or NVIDIA. Use at your own risk.
