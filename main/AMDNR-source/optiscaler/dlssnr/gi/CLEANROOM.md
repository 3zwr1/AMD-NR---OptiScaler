<!-- Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt). -->
# AMDNR Screen GI - clean-room record

AMDNR Screen GI (this folder) is written from public research only. No code was copied, translated or consulted from
the inherited experimental screen-space GI (its host code and its shaders), from ReShade / iMMERSE / qUINT (Pascal
Gilcher, "Marty McFly"), from NVIDIA NRD, Shadertoy, or any other non-MIT source. MIT reference code may be opened
only after a kernel has been written from the papers, to debug a measured problem; every such use is logged in
our internal clean-room log (file, why, "no code copied").

## Idea sources (papers, no code)
- Therrien, Levesque, Gilet 2023, visibility bitmask screen-space indirect lighting (arXiv 2301.11376).
- Jimenez, Wu, Pesce, Jarabo 2016, GTAO and its multibounce fit.
- Schied et al. 2017, SVGF (variance-guided filtering, history length).
- Dammertz et al. 2010, edge-avoiding a-trous.
- Kopf et al. 2007, joint bilateral upsampling.
- Karis 2014, Salvi 2016, history clamping.
- Turanszki 2019, Wu 2020, normals from depth.
- Hartley 1997, rotating-camera self-calibration.
- Roberts, the R2 sequence; Jimenez 2014, interleaved gradient noise.
- IEC 61966-2-1, the sRGB transfer function.

## Per task
| Task | Track | Read | MIT code opened |
|---|---|---|---|
| Module skeleton (host, stub passes, build script, lab) | A+B | the SSGI design plan; tree wiring only | none |
| Shader passes P1-P7, pass wiring, lab scenes (2026-09-26) | A | the design plan sections 0-6 and 8; the skeleton; the papers above | none |
| Shader track second pass: temporal normal test, debug view 10, lab tolerance (2026-09-26) | A | the design plan sections 0-6 and 8; the skeleton; this module's own files; our own gilab harness | none |
| Verify pass: hardware runs, trace fixes (full front hemisphere of the slice, exact slide), Ultra cap refit (2026-09-26) | A | the design plan sections 0-6 and 8; the skeleton; this module's own files; our own gilab harness (with two new options) and a scratch copy of the module (outside the source tree, never shipped) for diagnostics | none |
| NR colour chain in AmdBridge (Restore/Replacement/HasReplacement fall through to the seam; NR input state for GI's texture) (2026-09-26 23:00) | host | the design plan 4.3 item 2; `ssgi_skeleton.md` section 6; tree wiring only (`AmdBridge.h/.cpp`, `GiSeam.h/.cpp`, the upscalers' restore and barrier contract, `LmxxfBackend.cpp` colour use) | none |
