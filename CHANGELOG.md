## Latest local refresh — 23 October 2026

- Original-menu styling for Classic carry; shared full/Friendly NR quality selector and supported-handheld option.
- Mochi A/B depth and motion guides reach the worker in canonical formats. Overlay activity counts completed model outputs rather than pass-through frames.
- Correct source-relative colour transport for Classic carry. Prior controlled host fixtures reduced detected warm-to-cyan pixels; a universal in-game fix is not claimed.
- Classic carry remains optional and defaults OFF independently for lmxxf Model A and mochi A/B. Neural > Quality controls it; network history is separate. Explicit Model interleave retains its own fill preset.
- Updated temporal shaders embedded at build time; macro-containing source extraction is checked against actual compiled C++ shader literals.

# AMDNR 0.3.5.8 — current local changes

Not published. Launcher 0.3.5.36. This supersedes older candidate notes bundled during development.

## Menu
- Cleaner original menu: detailed explanations folded into Live/details and tooltips; actionable errors stay visible.
- Friendly use menu shares the primary theme and widgets, with NR, upscaling, FG, quality presets and an Advanced mode return.
- Neural passes 1–3 uses the same native Daniel or legacy/lmxxf writer as the full menu; unsupported combinations retain their existing limits and mochi remains one pass.
- Reset all settings backs up settings and applies only on the next game startup, preserving installation identity and paths.
- Overlay Guide remains available with its work-in-progress disclaimer. Gallery, window restoration and startup updating remain included; they are not all new 0.3.5.8 features.

## Launcher
- Automatic verified component preparation in public and supporter builds; optional manual import.
- Bounded transient download retries, validated resume ranges, incomplete-cache recovery and per-package preparation serialization.
- Partial Daniel 0.8.0 folder repair from NR and independent SR/RR/FG choices, preserving verified files and player settings.
- One selected prepared copy per package/channel; older eligible copies pruned after success, including cache hits. Failed downloads retain old files; busy/linked/unknown content is skipped. Keep both current mochi networks and distinct dependencies.
- Proxy selection uses current executable and loading evidence; respects supported catalog shortlists and foreign DLLs; improves Vulkan/Xbox/WinGDK and Unreal directory handling.
- Runtime libraries no longer count as native game upscaler support, preventing erroneous Anywhere routing.
- HelixSR acquisition/preparation updated to official 2.0.0.
- Anti-cheat acknowledgement replaces unconditional installation refusal. This does not bypass protection or certify online compatibility.
- Fresh installs use LogToFile=true; existing INIs stay intact. Correct auto/missing logging diagnostics.
- Numeric protocol version fixes developer chat creation without a selected game. Intro completion is bounded while the startup update gate remains enforced.

## Runtime / Anywhere
- Accept R16G16B16A16_SNORM motion without float reinterpretation; existing typeless-family handling remains. Earlier RE3 rejection reproduced in supplied logs; regression now passes.
- Retain native FFX callback ownership, protect synthetic handles at native fallback boundaries, preserve ownership on hot-swap, and reject missing upscale command lists.
- Retain the active menu/feature safely across backend changes; prevent concurrent Streamline hook mutation and retain required module ownership.
- Use recording ownership and real queue fences for DX12 feature retirement instead of a fixed two-second timeout. Release-during-initialization and retained/replayable-list cases are covered.
- Carry scene-reset intent into the next submitted NR/SR evaluation after a command list is discarded.
- Suspend known game predication during common SR/NR work, restoring exact state afterwards.
- FSR FG uses completion tracking for prepare allocator reuse and retirement; deferred Present retirement is retried before admission.
- Expand Vulkan dynamic-state replay. The requested Bloodborne HUD game retest is still pending.
- Anywhere Model E consumes SR and flow with the required synchronization; no host hash-only bypass.
- Select a query-reported FSR provider instead of hardcoding 4.1.1. Driver SR 4.1.2 and mod FG 4.0.2 were exercised; bundled SDK files remain 4.1.1/4.0.1. No new FSR FG renderer in Anywhere.
- Mochi worker honors the selected compiler path. The driver-specific default ACO failure remains unresolved.

## Community / server preparation
- Public Chat Enter-to-send, Shift+Enter newline, search, text mentions and persisted reactions.
- Admin bulk close/Trash with confirmation and partial failure counts; export available retained original report attachments with an index.
- AI support distinguishes enablement from actual output and reports narrower, evidence-based guidance.
- New server routes require the matching Worker and outstanding migrations (including 0018). No remote migration or deployment was performed by this assembly.

## Evidence scope
- Previous controlled 60-switch / 360-frame test: VRAM peak with Daniel 8.992 -> 3.215 GB (64.2% lower); NR off 8.176 -> 1.893 GB (76.8% lower). Not game FPS figures.
- Predicate reproduction: frozen adjacent pairs 5/9 -> 0/9; ten output pairs matched the unpredicated control. Discarded-reset cases matched explicit-reset controls.
- New assembly uses v145 / MSVC 14.44.35207. Four host contracts pass; source hashes unchanged throughout the build. Latest signed-binary identity and current test outputs are in Validation/.
- Latest menu interaction fixture exercises 1–3-pass writes/readback and 720p/1080p/small layouts using real widgets with controlled runtime substitutes, not an in-game screenshot.

Read KNOWN-LIMITS.md; no universal crash, flicker, blur, performance or all-architecture claim.
