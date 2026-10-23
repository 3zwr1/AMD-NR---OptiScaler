> Latest local refresh: shared NR quality selector matches Friendly use; Handheld appears only on supported handheld APUs. Mochi A/B receive normalized depth/motion guides and the overlay counts returned NR outputs. Classic carry is OFF by default for lmxxf Model A and mochi A/B, remains optional in Neural > Quality, and uses the original menu widgets. Network history is separate. Explicit Model interleave still controls its fill path. Carry colour transport corrects the reproduced warm-to-cyan subtraction defect; universal absence of ghosting or colour artifacts is not claimed. Updated host shaders are embedded at build time.

# AMDNR 0.3.5.8 — complete local candidate

Neural Rendering for AMD Radeon, built on OptiScaler. Launcher: 0.3.5.36.

This assembly contains the latest implemented mod, launcher, Anywhere and optional runtime components together. It has not been published or approved as an all-game-stable release. Read KNOWN-LIMITS.md before distribution. The public and supporter launchers still contact their existing live update channels; placing draft manifests beside them does not switch those channels.

## Included

- AMDNR-v0.3.5.8.zip: complete mod files, production lmxxf 0.4.4, licences and manuals; latest menu and crash-path corrections.
- AMDNR-Launcher.exe: signed public launcher 0.3.5.36, automatic component preparation, runtime repair, cache cleanup and proxy selection.
- SupporterPreview/AMDNR-Launcher-SupporterPreview.exe: corresponding signed supporter launcher.
- AMDNR-Anywhere-0.3.5.8.zip: verified Model E host integration and driver-based FSR provider selection.
- AMDNR-Mochi-0.0.7-component.zip: original mochi component with updated AMDNR worker; no weights included.
- v0.8.0-Runtime.zip: Daniel Blanco's original authorized host and attribution, unchanged.
- Server/AMDNR-Server-0358-prepared.zip: dry-run Worker bundle and migration files, not deployed.
- Manifests/: proposed GitHub/server manifests and their inventory; URLs for new assets are planned, not uploaded or download-back verified.
- Docs/: current notes, multilingual README updates, Discord text and video brief.

## Main changes

- Accept RE Engine signed normalized motion vectors correctly, fixing the demonstrated unsupported-format refusal for R16G16B16A16_SNORM (13).
- Cleaner original menu; Friendly use mode with the original theme, NR/upscaling/FG and shared 1–3-pass controls where supported. Mochi remains single-pass. Return to Advanced and persist the menu choice.
- Advanced > Reset all settings queues a backed-up reset for the next game start, retaining installed runtime identity and library paths.
- Automatically fetch approved components and recover interrupted downloads/incomplete caches; manual Import remains optional.
- Repair missing Daniel host/dependency/index files without replacing valid player settings.
- Keep one selected prepared runtime copy per channel cache; prune eligible older copies only after successful verification. Busy, unknown and linked folders are protected; current mochi A/B and required dependencies remain.
- Select proxies using executable imports, current loading evidence and supported platform/game exceptions. Resolve Unreal game executables instead of Engine tool folders.
- Fix Model E SR/flow handoff and prevent runtime files from falsely moving games out of Anywhere.
- HelixSR 2.0.0 is prepared from its author's official source. New AMD SR/FG implementations depend on the installed driver.
- Correct FFX callback ownership, Streamline initialization races, menu/feature lifetime, DX12 GPU retirement, discarded scene resets and predicate-state leakage. Scope and measured limits are in CHANGELOG.md.

## Local use

Keep a backup and close the game before changing its mod files. Use the inner mod ZIP for an existing AMDNR installation, preserving the player's INI and current proxy filename. The optional runtime ZIPs are component-store inputs with distinct layouts; do not extract every archive into the game root. Obtain dependencies through the launcher. This bundle is not a fully offline installer. No NVIDIA DLL, NR weights, driver DLL or author pass DLL is redistributed.

The owner signature and timestamp are present. The signing root is not trusted by Windows on this machine; no public certificate trust claim is made.

## Credits

OptiScaler contributors (GPL-3.0); lmxxf by Kien (MIT); DLSS-NR on AMD by Daniel Blanco (danielblnc), host distributed unchanged with permission; mochizuki0323's original runtime (MIT); HelixSR by lonewolf0622, fetched separately. Keep Licenses/AMDNR_NOTICE.txt and each component's licences. Copyright (c) 2026 3zwr1 (AMDNR). Not affiliated with AMD or NVIDIA.
