// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// AMDNR Screen GI: the seam between OptiScaler and the module (design 4.3). Reads [AmdGi] and the NGX parameter
// block, resolves the camera chain (design 5.2), keeps one Effect on the first device, and owns GI's colour
// replacement in the parameter block.
#pragma once

#include <d3d12.h>
#include <nvsdk_ngx.h>

#include <string>

#include "AmdnrGi.h"

namespace AmdnrGi::Seam
{
// [AmdGi] Enabled and an AMD GPU. With the key off this is one config read and the hook does nothing at all:
// no allocation, no pass, the frame byte for byte as before.
bool Wanted();

// The pre-upscale call: DlssNr_Dx12.cpp EvaluateInternal, before the "NR enabled" gate, inside
// ScopedNrStateEnvelope. `epoch` = the presented-frame counter (the AMD path's rule).
void BeforeNr(ID3D12GraphicsCommandList* cmdList, NVSDK_NGX_Parameter* params, ID3D12CommandQueue* timingQueue,
              unsigned long long epoch);

// The hook skipped GI this frame (the state-restore precondition failed); a dim status line.
void NoteSkipped(const char* reason);

// Colour chain (the AmdBridge side is a host-side request, see the internal design note for it):
//   - Restore: puts the game's colour back when GI replaced it in this block (this thread). AmdBridge::Restore calls
//     it AFTER its own restore. The first call marks the chain as wired; until then BeforeNr never replaces the
//     colour and does not run the passes (status "waiting for the colour chain").
//   - Replacement: GI's output in this block (in NON_PIXEL_SHADER_RESOURCE, must stay there), or nullptr.
//   - Original: the game's colour GI replaced, or nullptr.
void Restore(NVSDK_NGX_Parameter* params);
ID3D12Resource* Replacement(const NVSDK_NGX_Parameter* params);
ID3D12Resource* Original(const NVSDK_NGX_Parameter* params);
bool ChainWired();

// Menu readout.
Stats GetStats();
// "GI: High - 0.82 ms - trace 960x540 - depth reversed (flag)", or why it passes through. Empty while disabled.
std::string StatusLine();
// "Reset GI" button, and any caller that knows the history is invalid.
void ResetHistory();
} // namespace AmdnrGi::Seam
