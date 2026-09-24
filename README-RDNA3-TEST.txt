AMDNR - lmxxf on RDNA 3 (RX 7000) - TEST BUILD
Discord: https://discord.gg/AMDNR

For RX 7900 XTX / XT / GRE, RX 7800 XT / 7700 XT, RX 7600 and Strix Halo (Radeon 8060S).
lmxxf's network uses RDNA 4 FP8 instructions; this build runs the same network on RDNA 3 with the
FP8 math done on RDNA 3's F16 matrix units. It is slower than on an RX 9000.

The RDNA 3 backend is AMDNR's own work by 3zwr1 (https://github.com/3zwr1/AMD-NR---OptiScaler); see
NOTICE-AMDNR-RDNA3.txt. Every RDNA 3 module in LmxxfNrRuntime.pak carries this credit and the runtime refuses
a module without it. lmxxf's network, weights and kernels remain lmxxf's (MIT).

STEP 1 - quick check, no game needed (1 minute)
  Double-click run_probe.bat. Send probe_result.txt (and probe_output.ppm) in #bug-report.

STEP 2 - in a game
  1. Install AMDNR v0.3.3 as usual (AMDNR-v0.3.3.zip + Runtime.zip).
  2. Copy OptiScaler.dll, LmxxfNrRuntime.dll and LmxxfNrRuntime.pak from this folder over the
     0.3.3 ones (rename OptiScaler.dll to dxgi.dll again if that is how you run it).
  3. INSERT > Neural > Neural runtime: lmxxf, restart the game.
  4. Start with NR resolution at 67% (or lower); the Neural tab shows hip_ms (network time).
  5. Send: what you see compared with the danielblnc runtime, the hip_ms number, and the logs
     from the game folder: lmxxf_backend.log, amd_bridge.log, OptiScaler.log.
