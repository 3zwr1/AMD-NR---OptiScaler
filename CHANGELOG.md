# AMDNR 0.3.5.8R1

## Launcher 0.3.5.52
- Anywhere lmxxf selection releases stale Daniel NR ownership while preserving independent SR/FG choices.
- Store/Xbox ReShade installation allows an existing, scanned executable with an unknown graphics API; incompatible API, ownership and file-access checks remain.
- The new Anywhere host is paired for Model E; the previously qualified host remains accepted during updates.
- Retains compact grouped model/runtime choices, Game Settings and Anywhere Settings, installation/window behavior, report collection, German launcher support and 14 menu languages. Some diagnostic text remains English.

## Mod and Anywhere
- Daniel 0.8.0 automatic host lookup resolves relative to the installed mod and keeps identity checks; unique temporary settings writes preserve prior settings on failure.
- FSR FG and XeFG normalize wrapped command-list submission. FSR FG resources use the native device domain; the Witcher/ReShade heap mismatch was reproduced and corrected.
- Explicit FFX reselection clears an incompatible saved provider-version constraint. This does not invent a missing FSR version.
- Host-side camera-motion rejection was narrowed; temporal history and lmxxf Model B network/weights are retained. Controlled movement tests do not establish a general ghosting cure.
- Anywhere includes the FSR frame-generation implementation and required runtime assets, with producer-fence/retirement handling.
- Model E retains completed SR output for an unchanged captured image/input revision rather than redispatching an invalid repeated temporal pair.
- Mod menu/log version is 0.3.5.8R1. Retains prior Mochi worker/ACO, guides, colour transport, interleave 2x/4x/8x/10x, menu, Vulkan and report-collection work; remaining limits are explicit below.

## Validation
Pinned v145 / MSVC 14.44.35207 mod and host builds succeeded. Launcher tests: 722 passed, zero failed; after host-pair update, nine capability tests passed. Final signed host/mod pairing completed 480 consumed Model E frames. FSR FG restart fixture produced successful generation dispatches with zero failed dispatches. Earlier actual-NR motion suite includes 300 images and 240 successful outputs across lmxxf A/B and Mochi A/B, using the pre-version-stamp DLL with the same NR implementation.

Build warnings remain (including C4250, C4744, LNK4098); this is not a warning-free build or blanket in-game certification. See KNOWN-LIMITS.md. Collect a fresh report after reproducing a problem.

Credits: OptiScaler contributors; Daniel Blanco (danielblnc); lmxxf by Kien; separately acquired runtime and upscaler authors retain their licences and attribution.
