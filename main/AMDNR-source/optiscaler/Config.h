// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#pragma once

#include "SysUtils.h"
#include "State.h"

#include <optional>
#include <filesystem>

enum HasDefaultValue
{
    WithDefault,
    NoDefault,
    SoftDefault // Change always gets saved to the config
};

template <class T, HasDefaultValue defaultState = WithDefault> class CustomOptional : public std::optional<T>
{
  private:
    T _defaultValue;
    std::optional<T> _configIni;
    bool _volatile;

  public:
    CustomOptional(T defaultValue)
        requires(defaultState != NoDefault)
        : std::optional<T>(), _defaultValue(std::move(defaultValue)), _configIni(std::nullopt), _volatile(false)
    {
    }

    CustomOptional()
        requires(defaultState == NoDefault)
        : std::optional<T>(), _defaultValue(T {}), _configIni(std::nullopt), _volatile(false)
    {
    }

    // Prevents a change from being saved to ini
    constexpr void set_volatile_value(const T& value)
    {
        if (!_volatile)
        { // make sure the previously set value is saved
            if (this->has_value())
                _configIni = this->value();
            else
                _configIni = std::nullopt;
        }
        _volatile = true;
        std::optional<T>::operator=(value);
    }

    // Use this when first setting a CustomOptional
    constexpr void set_from_config(const std::optional<T>& opt)
    {
        if (!this->has_value())
        {
            _configIni = opt;
            std::optional<T>::operator=(opt);
        }
    }

    constexpr CustomOptional& operator=(const T& value)
    {
        _volatile = false;
        std::optional<T>::operator=(value);
        return *this;
    }

    constexpr CustomOptional& operator=(T&& value)
    {
        _volatile = false;
        std::optional<T>::operator=(std::move(value));
        return *this;
    }

    constexpr CustomOptional& operator=(const std::optional<T>& opt)
    {
        _volatile = false;
        std::optional<T>::operator=(opt);
        return *this;
    }

    constexpr CustomOptional& operator=(std::optional<T>&& opt)
    {
        _volatile = false;
        std::optional<T>::operator=(std::move(opt));
        return *this;
    }

    // Needed for string literals for some reason
    constexpr CustomOptional& operator=(const char* value)
        requires std::same_as<T, std::string>
    {
        _volatile = false;
        std::optional<T>::operator=(T(value));
        return *this;
    }

    constexpr T value_or_default() const&
        requires(defaultState != NoDefault)
    {
        return this->has_value() ? this->value() : _defaultValue;
    }

    constexpr T value_or_default() &&
        requires(defaultState != NoDefault) {
            return this->has_value() ? std::move(this->value()) : std::move(_defaultValue);
        }

        constexpr std::optional<T> value_for_config()
            requires(defaultState == WithDefault)
    {
        if (_volatile)
        {
            if (_configIni != _defaultValue)
                return _configIni;

            return std::nullopt;
        }

        if (!this->has_value() || *this == _defaultValue)
            return std::nullopt;

        return this->value();
    }

    constexpr std::optional<T> value_for_config()
        requires(defaultState != WithDefault)
    {
        if (_volatile)
            return _configIni;

        if (this->has_value())
            return this->value();

        return std::nullopt;
    }

    constexpr T value_for_config_or(T other)
    {
        auto option = value_for_config();

        if (option.has_value())
            return option.value();
        else
            return other;
    }

    // Like value_for_config(), but an explicit value that happens to equal the class
    // default still counts as set. value_for_config() folds those two together, which
    // is right for writing an ini - there is no point storing a line that changes
    // nothing - and wrong for asking "did anyone decide this?". The FSR-RR path has to
    // distinguish a title that declared its depth linear from one that never said.
    constexpr std::optional<T> value_for_config_ignore_default()
        requires(defaultState == WithDefault)
    {
        if (_volatile)
            return _configIni;

        if (this->has_value())
            return this->value();

        return std::nullopt;
    }
};

constexpr inline int UnboundKey = -1;
constexpr uint32_t NV_PRESET_LATEST = 0x00FFFFFF;

enum FpsOverlayPos : uint32_t
{
    FpsOverlayPos_TopLeft,
    FpsOverlayPos_TopRight,
    FpsOverlayPos_BottomLeft,
    FpsOverlayPos_BottomRight,
    FpsOverlayPos_COUNT,
};

enum FpsOverlay : uint32_t
{
    FpsOverlay_JustFPS,
    FpsOverlay_Simple,
    FpsOverlay_Detailed,
    FpsOverlay_DetailedGraph,
    FpsOverlay_Full,
    FpsOverlay_FullGraph,
    FpsOverlay_ReflexTimings,
    FpsOverlay_COUNT,
};

// Output scaling downscaler
enum class Scaler : uint32_t
{
    FSR1 = 0,
    Bicubic = 1,
    CatmullRom = 2,
    Lanczos2 = 3,
    Lanczos3 = 4,
    Kaiser2 = 5,
    Kaiser3 = 6,
    Magic = 7,
    Count
};

enum class ForceReflex : uint32_t
{
    InGame,
    ForceDisable,
    ForceEnable,
    Count
};

enum class LFXMode : uint32_t
{
    Conservative,
    Aggressive,
    ReflexIDs,
    Count
};

enum class LowLatencyInput : uint32_t
{
    None,
    Auto,
    AntiLag2,
    Reflex,
    XeLL,
    UeLowLatency,
    _
};

enum class LowLatencyMode : uint32_t
{
    None,
    Auto,
    LatencyFlex,
    AntiLag2,
    XeLL,
    AntiLagVk,
    Reflex
};

class Config
{
  public:
    Config();

    // Init flags
    CustomOptional<bool, NoDefault> DepthInverted;
    CustomOptional<bool, NoDefault> AutoExposure;
    CustomOptional<bool, NoDefault> HDR;
    CustomOptional<bool, NoDefault> JitterCancellation;
    CustomOptional<bool, NoDefault> DisplayResolution;
    CustomOptional<bool, NoDefault> DisableReactiveMask;
    CustomOptional<float> DlssReactiveMaskBias { 0.45f };

    // Logging
    CustomOptional<bool> LogToFile { false };
    CustomOptional<bool> LogToConsole { false };
    CustomOptional<bool> LogToDebug { false };
    CustomOptional<bool> LogToNGX { false };
    CustomOptional<bool> OpenConsole { false };
    CustomOptional<bool> DebugWait { false }; // not in ini
    CustomOptional<int> LogLevel { 0 };
    CustomOptional<std::wstring> LogFileName { L"OptiScaler.log" };
    CustomOptional<bool> LogSingleFile { true };
    CustomOptional<bool> LogAsync { false };
    CustomOptional<int> LogAsyncThreads { 4 };
    // AMDNR 0.3.4, [Log]. Read at startup, never written back by SaveIni (a key set by hand stays in the ini).
    // KeepPreviousLogs: how many earlier logs are kept beside the current one, all named OptiScaler.previous*.log
    //   (misc/LogRotationNames.h), 1..5. 1 = one previous log per exe, as in 0.3.3.2.
    CustomOptional<int> KeepPreviousLogs { 3 };
    // CrashHandler: an in-process crash note (misc/CrashReport.h). Off by default: titles with anti-tamper (RE Engine)
    //   make an exception handler risky. CrashDump: with CrashHandler on, also a small minidump. false = 0.3.3.2.
    CustomOptional<bool> CrashHandler { false };
    CustomOptional<bool> CrashDump { false };

    // XeSS
    CustomOptional<bool> BuildPipelines { true };
    CustomOptional<int32_t> NetworkModel { 0 };
    CustomOptional<bool> CreateHeaps { true };

    // --- DLSS 5 Neural Rendering (OptiScaler/dlssnr) --- removable as one block -----------------
    // DLSS Neural Rendering: a detail-synthesis pass over the upscaler's output. Off by default -- it is
    // an undocumented feature driven directly through its snippet, not something NVIDIA exposes.
    CustomOptional<bool> DlssNrEnabled { false };
    // Run the NR pass on the upscaler's colour input, at render resolution, immediately before SR.
    // Off preserves the v0.2.0 post-upscale placement.
    CustomOptional<bool> DlssNrRunBeforeSr { false };
    CustomOptional<bool> DlssNrApplyAfterRR { false };
    CustomOptional<unsigned int> DlssNrRRPasses { 1 };
    CustomOptional<float> DlssNrRRWorkingScale { 0.5f };
    // Toggles the pass in game. Unbound by default -- a key that does something unexpected is worse
    // than one that does nothing.
    CustomOptional<int> DlssNrToggleKey { VK_HOME }; // toggles Neural Rendering in-game, both runtimes; Backspace in the binder unbinds
    // Press in-game, no menu: the AMD backend writes the next 8 frames to a time-stamped
    // amd-nr-capture folder. The menu button cannot be pressed while moving fast; a key can.
    CustomOptional<int> DlssNrCaptureKey { UnboundKey };
    CustomOptional<uint32_t> DlssNrPreset { 0 };
    CustomOptional<float> DlssNrIntensity { 1.0f };
    // 0 default (standard), 1 natural, 2 cinematic -- the model's own processing profiles.
    CustomOptional<uint32_t> DlssNrStyle { 0 };
    // Optional per-pass model profiles. Pass 1 uses Preset/Style above; an absent override inherits
    // pass 1. Keeping inheritance explicit preserves every existing configuration and lets changing
    // the base profile update the whole stack unless a later pass was deliberately specialised.
    CustomOptional<uint32_t, NoDefault> DlssNrPass2Preset;
    CustomOptional<uint32_t, NoDefault> DlssNrPass2Style;
    CustomOptional<uint32_t, NoDefault> DlssNrPass3Preset;
    CustomOptional<uint32_t, NoDefault> DlssNrPass3Style;
    CustomOptional<float> DlssNrLocalStructure { 1.0f };
    CustomOptional<float> DlssNrLocalTone { 1.0f };
    CustomOptional<bool> AmdNeuralLighting { true };
    // 1.0, not 0.5. This is the network's LocalToneStrength and the binaries' own default
    // is 1 - we were shipping every user half the tonal effect the model was built to
    // apply, which is a large part of why "turning Neural Rendering on" did not look like
    // turning anything on.
    CustomOptional<float> AmdNeuralLightingStrength { 1.0f };
    CustomOptional<int> AmdEncoding { 0 };
    // Hybrid highlight proxy (experimental, OFF; danielblnc runtime only). The Forza capture
    // measured the model returning bright content at about half its brightness - it compresses
    // highlights it is shown as linear HDR. With 1, the frame handed to the model is squeezed
    // above the knee with a reversible curve (identity below it), so it never sees an extreme
    // value and cannot crush it, and its answer is brought back with each pixel's ORIGINAL
    // scale (RenoDX's principle): exact wherever the model changes nothing, its changes kept in
    // proportion; where it removed a sparkle, its answer is kept. Above about 5x the knee a
    // highlight's brightness follows the game's frame (the upscaler after this pass resolves
    // its noise, as with the proxy off). Two tiny in-place passes. Off until the capture
    // confirms it. See AmdPreSr.cpp, ProxyShader.
    CustomOptional<int> AmdHighlightProxy { 0 };
    CustomOptional<float> AmdHighlightProxyKnee { 1.0f };
    CustomOptional<float> AmdHighlightProxyRange { 1.0f };
    // 1-5 in the ini; the menu offers 2-5. Too few and a frame that finds every
    // buffer busy carries no NR at all, so this decides whether the mode works
    // rather than how fast it runs. See AmdPreSr.cpp for the measurements.
    //
    // The default has to follow AMD_SINGLESLOT as well, or the control build
    // contradicts its own banner: AmdBridge overwrites the backend's default
    // from here on the first frame, so a control build with this left at 3
    // would announce "default=1" and then run three.
#ifdef AMD_SINGLESLOT
    CustomOptional<int> AmdSlots { 1 };
#else
    CustomOptional<int> AmdSlots { 3 };
#endif
    CustomOptional<float> AmdNrScale { 1 };
    // leak audit R4 (0.3.3.2 rebuild): away from 100% NR and above about 1 MP, danielblnc's working size is snapped to
    // this many pixels so its runtime reuses a few sizes (each new size keeps 75-350 MB of VRAM per pass until the game
    // restarts). 0 = exact sizes, as in 0.3.3.2's first build. lmxxf never reads it.
    CustomOptional<int> AmdNrSizeStep { 64 };
    // Temporal stabilisation strength for the AMD pre-SR colour, 0..1. Default
    // (auto in the ini) is 0.6: on out of the box to suppress flicker/shimmer,
    // by blending the motion-reprojected, neighbourhood-clamped previous frame.
    // The host also nudges this up automatically on frames near a denoise skip.
    // 0 turns it off (byte-identical). Read every frame; no model rebuild.
    // Under Model interleave with Edit accumulation (AmdInterleavePreset 10) it is the damping of
    // the model's answers across calls on BOTH runtimes (danielblnc ignored it under interleave
    // before), and the glide of the tone curve.
    CustomOptional<float> AmdTemporalStability { 0.6f };
    // Temporal stability mode: 0 = variance clamp (TAA-style, fills interleave
    // frames), 1 = difference-gated (blends only where reprojected history already
    // matches the frame; ghost-free, after lmxxf's native_output_smooth). Default 0
    // (variance clamp): it suppresses flicker/shimmer more strongly and, with the
    // neighbourhood clamp, stays ghost-free - the difference-gated mode smooths less
    // so faint flicker can survive. Both modes are clamped, so neither ghosts.
    CustomOptional<int> AmdStabilityMode { 0 };
    // Difference-gated sensitivity in tonemapped units; below this the pixel is
    // treated as shimmer and smoothed, above it passes through as real motion. Kept
    // low (like lmxxf's few/255) so only genuine shimmer is touched and nothing ghosts.
    CustomOptional<float> AmdStabilityThreshold { 0.04f };
    // #5 Still-surface steadiness (both runtimes, 0.3.3.2 rebuild): widens the temporal clamp by a brightness-proportional
    // floor on pixels that provably did not move (motion, depth and the reactive mask agree), so shadows and flat areas that
    // pulse on still geometry are damped. 0 = off (byte-identical). danielblnc: not used with Model interleave. lmxxf: the
    // carried edit.
    CustomOptional<float> AmdStabilityStaticRelax { 0.0f }; // O-4: 0.5 after the in-game A/B
    // Diagnostic overlay for it (green = relaxed, red = relaxed but rejected as a real change). Read-only, never saved.
    CustomOptional<bool> AmdStabilityStaticDebug { false };
    // Detail / Colour strength for the AMD in-place denoise, 0..1, default 1 (full
    // model, pass skipped = byte-identical). Detail = how far brightness moves to
    // the model's answer; Colour = model hue (1) vs original hue at model brightness
    // (0). Read every frame; no model rebuild. See DetailColourMix.h.
    CustomOptional<float> AmdDetailStrength { 1.0f };
    CustomOptional<float> AmdColourStrength { 1.0f };
    // Colour composition on the AMD path, both runtimes. 0 (default) = Classic: today's picture,
    // byte-identical (no new dispatch, same runtime strengths). 1 = RenoDX (experimental): the
    // composition tail of dlssnr.hlsl on the host (dlssnr/amd/NrCompose.h) - Detail strength,
    // the Highlight guard (DlssNrMaxRatio), Colour strength and the skin / environment edit
    // (the DlssNrSkin* keys) - bounding the model's answer against the original before each
    // runtime's usual residual controls. Any other value is Classic. Display-referred input is
    // refused (Classic for that frame, one log line, a menu note) on both runtimes.
    CustomOptional<uint32_t> AmdComposition { 0 };
    // Detail / Colour strength of the RenoDX mode, separate from the Classic keys above so a value
    // above 1 set here never lands in Classic's 0..1 range. Detail 0..2 (above 1 the luminance
    // ratio is raised to that power, still bounded by the guard); Colour 0..4 (above 1 OkLab
    // chroma is scaled, pulled back into gamut toward neutral). Clamped where they are used
    // (AmdBridge BuildSettings); NR styles and presets leave them alone.
    CustomOptional<float> AmdComposeDetail { 1.0f };
    CustomOptional<float> AmdComposeColour { 1.0f };
    // Contrast Adaptive Sharpening on the AMD denoised colour, 0..1. 0 (default)
    // is off/skipped. Applied before Super Resolution. See amd/Sharpen.h.
    CustomOptional<float> AmdSharpness { 0.0f };
    // Dynamic NR resolution: hold a frame-time target by nudging the model's
    // working scale in a few discrete, debounced steps (each step rebuilds the
    // model, so changes are deliberately rare). Off by default. The manual NR
    // resolution acts as the ceiling. Target FPS clamps to 30..240.
    CustomOptional<bool> AmdDynamicRes { false };
    CustomOptional<int> AmdDynamicTargetFps { 60 };
    // Experimental: run the neural model every Nth frame and fill the rest by
    // temporal reprojection (forces the temporal stabilizer on). 0 = off (every
    // frame); >1 sets the average cadence and need not be an integer - e.g. 1.4
    // runs the model on most frames and skips roughly 2 in every 5 (a much
    // gentler duty cycle than 2 = every other frame). Big FPS win at any NR
    // resolution; can add motion ghosting and disrupts the model's temporal
    // state. See amd/AmdPreSr.cpp.
    CustomOptional<float> AmdInterleave { 0.f };
    // How an interleave FILL frame is produced. 0 = pure reprojected history (the
    // un-denoised current frame is never mixed in, so a fill frame cannot differ from
    // the model frame it follows - this is the combination originally reported clean,
    // with ghosting as its only cost). 1 = smart fill, which blends in a blurred copy
    // of the un-denoised current frame to break that ghost, at the price of the fill
    // frame no longer matching the model frame - i.e. cadence-locked flicker.
    // Detail kept from the current frame on interleave frames, 0..3 -> 0.25/0.5/0.75/1.0.
    // Default 0. Raising it was tried to cure softness and made the flicker worse: the
    // formula is the same on both frame types, but its CONTENT is not - the detail is
    // denoised on a model frame and raw sensor noise on a skipped one - so scaling it up
    // magnifies that difference, and the difference is what is visible at the cadence.
    // Softness here is the price of interleave, not a bug to tune out.
    CustomOptional<int> AmdInterleaveFill { 0 };
    // Interleave: take a skipped frame's fine detail from the reprojected denoised
    // history (true) instead of scaling the un-denoised current frame's (false).
    // True gives full sharpness with no grain on both frame types; false is immune to
    // reprojection error but trades blur against grain with one number.
    CustomOptional<bool> AmdInterleaveSharp { true };
    // Interleave diagnostic overlay: 0 off, 1 trust, 2 frame type, 3 clamp, 4 raw on skipped frames.
    CustomOptional<int> AmdInterleaveDebug { 0 };
    // Interleave preset: 0 = standard, 1 = extra temporal effect on the frames the
    // model skipped (they lean harder on the reprojected denoised history).
    // 3 = Held frame: gates nothing at all, which is the only arrangement in which a
    // per-pixel decision cannot come out differently on a model frame than on a filled
    // one. Its cost is that it shows the old picture, so it ghosts, and every guard added
    // to catch the ghost flickers. 1 = Standard keeps every guard; 2 = Extra temporal is
    // the experimental lane; 4 = Residual temporal (retired from the menu).
    // 5 = Guided fill: never shows the old picture. A filled frame is a
    // joint bilateral filter of THIS frame's raw colour whose weights come from the
    // reprojected denoised history, plus the model's carried non-noise edit. The model
    // frame runs the same operator, so both frame types are the same kind of image. See
    // the GUIDED FILL note in TemporalStability.h.
    // 6 = Guided fill v2: the Held picture (the `53d42bc0` arrangement the user kept, which
    // 5 still maps to) with the filled frame's raw fallbacks wearing the model's tone and
    // the ghost bound comparing raw against raw - the measured cause of the lamp/sign/sky
    // pulse at 2-3 passes and >115% NR. See TemporalStability.h, preset 6.
    // 10 = Edit accumulation, BOTH runtimes (the lmxxf host runs it as 11; any other value is
    // lmxxf's classic carry, 9). No picture ever travels: every frame is THIS frame's raw plus a
    // carried local correction - on danielblnc a per-channel gain and a luminance slope fitted to
    // the model's answer on model frames, on lmxxf the edit itself. Validity is geometry times a
    // raw-against-raw test run on both frame types, with a per-pixel noise tolerance learned over
    // time; where it fails both frame types show the same position-free tone curve (on danielblnc
    // with this frame's detail at the model's measured detail ratio for that brightness). Answers
    // are damped by Temporal stability and each is spread over two frames. It is the owner's
    // 2026-09-26 report (ghosting and flicker left under interleave, clearly on danielblnc) that
    // this answers: the Held picture's one-frame ghosts and its 30 Hz grain are gone by
    // construction. The price, on danielblnc: model frames show the fitted correction too, not the
    // model's exact picture (a Forza frame's fit reproduced ~55% of the model's local change; the
    // real model keeps the raw's grain - Silent Hill 2 0.95x, Forza 1.05x - so the fit does too).
    // Same model runs; the pass has its own pipeline and costs no more than 6 (2560x1440, RX 9070
    // XT: 0.56 / 0.41 ms per model / skipped frame, 6: 0.59 / 0.59). See TemporalStability.h,
    // EDIT ACCUMULATION. 7 runs as 8; 9 and 11 run as 10 (AmdBridge). Default 10 in the test
    // build - set 6 for the old picture; the release default is decided after the owner's A/B.
    CustomOptional<int> AmdInterleavePreset { 10 };
    // Interleave pacing, 0..1. Model frames cost more than filled ones, so the frame
    // TIME alternates even when the image does not - and an engine that steps its
    // simulation with the previous frame's duration then shows every step for the wrong
    // length of time. That mismatch is the "feels choppy while the image looks smooth"
    // report, and no amount of filtering could ever have addressed it.
    //
    // 0 leaves the sawtooth alone. 1 pads every filled frame out to the model frame's
    // cost: perfectly even, and exactly the frame rate you get with interleave switched
    // off. It is a dial because the trade is continuous and the right point is the
    // player's to pick. Off by default - this changes frame rate, and a default that
    // quietly does that is how the 125% NR resolution surprise happened.
    // -1 = automatic (engages in proportion to the measured model/fill split, see
    // InterleavePacing.h), 0 = off, 0..1 = manual strength. Automatic by the user's
    // request; the slider is gone from the menu, the readout stays.
    CustomOptional<float> AmdInterleavePacing { -1.f };
    // Held frame's ghost bound, 0..1.
    //
    // Every other guard in that preset is geometric - it asks where a pixel came from.
    // That cannot catch a car ghosted across the smoke behind it, because smoke is a
    // particle pass that usually writes no motion vectors at all: the whole region
    // reprojects by the road's motion, lands where the car was, and every vector involved
    // is finite and smooth, so nothing geometric has anything to object to.
    //
    // What is wrong is the colour - the history carries a colour that occurs nowhere
    // around that pixel now, which is what a ghost IS. Where the history sits outside the
    // neighbourhood's mean +/- k sigma the pixel is handed to the same fallback the other
    // guards use (the current frame). The slider scales k from 6 (0) down to 3 (1), and
    // never below 3: a legitimate one-pixel detail sits 2.83 sigma from its own 3x3, so
    // anything tighter clips real detail on filled frames only and that alternation is
    // the cadence flicker a tester reported at the old top of the range. It is the
    // identity on every pixel whose history already agrees with its surroundings.
    //
    // ON by default at 0.6 - unlike frame rate, this one has a defensible default, and
    // the alternative is shipping a known artefact with a switch next to it.
    CustomOptional<float> AmdInterleaveGhostBound { 0.6f };
    // Clear the model's internal temporal history on every model frame while
    // interleaving: removes the smear the network builds around content its motion
    // vectors do not describe, at the cost of the noise averaging it normally does.
    // OFF, and it stays off: it costs far more frame time than it is worth.
    //
    // It was briefly defaulted on as the second half of the anti-ghost fix. It works -
    // but clearing `historyValid` also nulls `historyView`, and the runtime answers that
    // by BUILDING A NEW HISTORY RESOURCE. At cadence 2 on a 60 fps frame that is thirty
    // allocations a second inside HIP, and the frame rate drop was immediate and severe.
    //
    // It turned out not to be needed. The real fault was the guard's own floor constant
    // in TemporalStability.h, which capped ghost removal at 45% no matter how far out
    // the history was; with that fixed the trail goes on the skipped frames and the
    // model-frame residue is diluted enough not to read as a pulse. Left here because it
    // is still the only lever that reaches inside the network, and it costs nothing while
    // it is off - but it is an ini key now, not a menu item, because the frame cost makes
    // it the wrong thing to offer someone casually.
    CustomOptional<bool> AmdInterleaveFreshHistory { false };
    // Whether the model keeps its own temporal history across skipped frames while
    // interleaving. OFF by default: every model frame is computed on its own, exactly as
    // every-frame mode does with interleave off. Interleave used to force the model's
    // temporal path on; that path reprojects a history two frames old and the ghost it
    // built there was in the model frames themselves, which is why no fill preset could
    // remove the ghost on a moving character. ON is the old behaviour, for comparison.
    CustomOptional<bool> AmdInterleaveModelHistory { false };
    CustomOptional<bool> AmdLmxxfHistory { true };
    // lmxxf only, "Full network": run all 71 blocks of the network instead of skipping 42, 43 and
    // 46 (lmxxf's production schedule) - slightly more faithful, about 0.5 ms slower at 1080p. OFF by
    // default; a change rebuilds the runtime's network at the next model frame. danielblnc's
    // runtime is closed, its block schedule cannot be chosen.
    CustomOptional<bool> LmxxfFullNetwork { false };
    CustomOptional<float> AmdLmxxfEditDetail { 1.0f };
    CustomOptional<float> AmdLmxxfEditSaturation { 1.0f };
    CustomOptional<bool> AmdLmxxfAutoExposure { true };
    // lmxxf only, 0.3.3 colour fix; LmxxfBackend.cpp reads both straight from here (the bridge's
    // Settings are unchanged). HighlightChromaGuard: where the copy the network is fed passes the
    // codec's shoulder (max channel above 0.75, fully from 1.5), the answer keeps its light and takes
    // the game's colour - the shoulder squashes such a pixel toward white and the decode hands it back
    // grey. AutoExposureHighlightCap: auto-exposure stops raising the exposure while over 8% of its
    // samples are fed past 0.75, and may lower it up to 4x per feed over 25%.
    // Both ON by default in 0.3.3: with both on, the grey highlights in Silent Hill 2 are fixed
    // (tested in Silent Hill 2). Off, the guard's weight is 0 and the auto-exposure clamp is the old
    // one (the 0.3.2 picture). In Colour composition RenoDX the guard runs on the answer before the
    // composition (NrCompose.h GuardAnswer; new in this build, not yet tested in a game), so
    // Composition colour still acts on guarded pixels.
    CustomOptional<bool> AmdLmxxfHighlightChromaGuard { true };
    CustomOptional<bool> AmdLmxxfAutoExposureHighlightCap { true };
    CustomOptional<float> AmdLmxxfEdgeGuard { 0.5f };
    CustomOptional<float> AmdLmxxfOutputSmooth { 0.6f };
    // Adaptive interleave: run the model every frame while the picture changes, interleave
    // only while it stands still (the temporal pass measures both). On by default.
    CustomOptional<bool> AmdInterleaveAdaptive { false };
    // 0 = Low (most fps), 1 = Medium, 2 = High (least fps, least visible change tolerated).
    CustomOptional<int> AmdInterleaveAdaptiveSensitivity { 1 };
    // Jitter compensation in the temporal pass: 1 = on (prev = p + motion + jitterPrev -
    // jitterCur), -1 = opposite sign convention, 0 = off. Measured on Silent Hill 2: the raw
    // shifts by up to 0.8 px between consecutive frames standing still, and without this a
    // skipped frame sat at the PREVIOUS frame's jitter - a sub-pixel wobble at half rate.
    CustomOptional<int> AmdJitterSign { 1 };
    // Show the network's own output, with nothing of ours after it: no appearance
    // filter, no experimental lighting, no detail/colour mix, no temporal stability, no
    // sharpening. This is the "Network Output" the RenoDX-based mod exposes, and the
    // reason to have it is that it settles an argument instead of continuing it - if the
    // raw result looks dramatically better than what normally reaches the screen, the
    // difference is something in our chain and can be named; if it looks the same, the
    // model is simply doing what it does and no amount of tuning here will change that.
    CustomOptional<bool> AmdNetworkOutput { false };
    // Residual composition controls. danielblnc: at exactly 100% NR resolution strength scales
    // the whole NR result against the pre-model frame (1.0 = unchanged) and limit / fade do not
    // act (0.3.3.2 A-min); at any other NR size strength and limit act on the model's edit before
    // the Look / stability / sharpening (AmdPreSr.cpp EditShapeShader) and fade rolls off the
    // lifted edit at the border. lmxxf: strength and limit act on the model's edit at every NR
    // resolution; fade is not used.
    CustomOptional<float> AmdResidualIntensity { 1.0f };
    // 0.25, lowered from 0.5. The residual is not a smooth few-percent field: the network
    // works in tiles, and a tile where it extrapolated rather than saw returns something
    // nothing like the rest. dlss5-neural-amd measured one run at a mean of 0.072 and a
    // MAXIMUM of 4.16, in a picture whose own mean was 0.13 - and every extra pass runs on
    // top of the last one's blown tile, so passes compound the outliers instead of
    // averaging them away. At 0.5 a clipped outlier still moves its pixel by half its own
    // brightness, which is a visible blotch; this is the number that decides how much of
    // that survives.
    CustomOptional<float> AmdResidualLimit { 0.25f };
    CustomOptional<float> AmdResidualFade { 0.0f };
    // SJ-1 (0.3.3.2 rebuild), danielblnc: away from 100% NR, strength and limit act on the model's composed edit (the
    // edit shaper, before the Look, stability and sharpening) and the resolve only lifts it to the frame; at exactly 100%
    // nothing changes. false = the first 0.3.3.2 build's whole-result dial away from 100%. Read, never saved.
    // OFF by default: in the owner's Forza Horizon 6 test (Classic, 115% NR, strength 1.2, limit 0.565) the shaper
    // washed the highlights out and AmdEditShaper=false fixed it, so it is an opt-in (true) until the limit rule
    // (plan O-1) is redone. lmxxf never reads it (it shapes the edit at every size already).
    CustomOptional<bool> AmdEditShaper { false };
    // AMDNR 0.3.4, danielblnc edit shaper modes; they act only with AmdEditShaper=true. Read, never saved (kill-switch
    // style, like AmdEditShaper). lmxxf never reads them.
    // AmdEditShaperLimit: 0 literal (Residual limit caps the edit, as in 0.3.3.2), 1 F1 (the edit is never capped),
    //   2 F2 (the cap ramps from none at 100% NR to Residual limit at 50%). See dlssnr/amd/EditShapeRules.h.
    // AmdEditShaperScope: 0 both sides of 100% NR (0.3.3.2), 1 below 100% only.
    // AmdEditShaperCarryCap: in shape mode, Edit accumulation's carry cap is the shaper's effective limit instead
    //   of 4. false = 0.3.3.2.
    CustomOptional<int> AmdEditShaperLimit { 0 };
    CustomOptional<int> AmdEditShaperScope { 0 };
    CustomOptional<bool> AmdEditShaperCarryCap { false };
    // AMDNR 0.3.4, danielblnc highlight colour guard (experimental, off): where the frame fed to the model passes the
    // codec's shoulder, the answer keeps its light and takes the game's colour (lmxxf's HighlightChromaGuard rule).
    // false = 0.3.3.2. Saved like its neighbours.
    CustomOptional<bool> AmdDanielHighlightGuard { false };
    // AMDNR 0.3.4, danielblnc runtime knobs. -1 = auto: the host writes nothing and the runtime keeps its own value
    // (0.3.3.2). Only runtimes whose layout maps the field take them. Saved like their neighbours.
    // AmdRuntimeStyle -1..2 (the runtime's own Style, not the NVIDIA path's [DlssNr] Style).
    // AmdToneCurve -1..1 (0 Reinhard, 1 ACES). AmdToneLift -1 or 0..0.25 (black lift).
    // AmdUseGameExposure -1..1 (-1 auto = the title's exposure is used when it publishes one; 0 ignore it; 1 use it).
    //   Shared by both runtimes (lmxxf: its title-exposure gate).
    CustomOptional<int> AmdRuntimeStyle { -1 };
    CustomOptional<int> AmdToneCurve { -1 };
    CustomOptional<float> AmdToneLift { -1.f };
    CustomOptional<int> AmdUseGameExposure { -1 };
    // AMDNR 0.3.4, danielblnc only: the runtime's quality mode, on the builds that have one (AmdBridge
    // DanielFastModeSupported). Unset / auto = the host writes nothing: the runtime keeps its own mode (Fast unless its
    // dlssnr_on_amd.ini says otherwise), as 0.3.3.2. true = Fast, false = Reference (explicit values only: read it
    // with has_value(), not value_or_default()). Live, per frame. Saved like its neighbours, an explicit false included
    // (SaveIni: value_for_config_ignore_default).
    CustomOptional<bool> AmdDanielFastMode { false };
    // AMDNR 0.3.4, lmxxf only: snap the NR working size to the network's size tiers. false = 0.3.3.2's sizing. Unset =
    // on for RDNA 3 (gfx11: RX 7000, Radeon 8060S, the APUs), off elsewhere; an explicit true / false wins. The default
    // below is only the unset reading of value_or_default(): read the value that applies through
    // AmdBridge::LmxxfTierSnapOn(). Ini only, read, never saved.
    CustomOptional<bool> AmdLmxxfTierSnap { false };
    // AMDNR 0.3.4, lmxxf only: the largest network size tier lmxxf's NR size may use. 0 = auto (360 on an lmxxf
    // handheld APU, no cap elsewhere), 360, 576, 720, 900 or 1080; any other value = auto. Applied by LmxxfBackend
    // (dlssnr/lmxxf/LmxxfTierPolicy.h). Ini only, read at game start, never saved.
    CustomOptional<int> AmdLmxxfTierCap { 0 };
    // AMDNR 0.3.4, lmxxf only: Fast mode. true = the network runs one size tier below the one the sizing above feeds
    // (1080 -> 900, 900 -> 720; 720 -> 576, 576 -> 360 with the small tiers; LmxxfTierPolicy.h PlanLmxxfFastSize): a
    // faster network, softer fine detail. Host only, live (next frame). false = off. Saved like its neighbours.
    CustomOptional<bool> AmdLmxxfFastMode { false };
    // Temporal smoothing of the model's EDIT rather than of the picture, with the model
    // running every frame. Independent of Model interleave - it is the same idea the
    // interleave preset uses, made available to people who do not want frames skipped.
    CustomOptional<bool> AmdResidualTemporal { false };
    // EXPERIMENTAL, and unavailable in this build: the runtime's 0.3.1 graphics wait (1-pixel
    // draws) instead of the compute spin. The draws disturb rasteriser and output-merger state,
    // and GraphicsSnapshotHooks.h may only admit a command list whose state it fully knows - but
    // it never learns the graphics root signature, so it refused every list (no ADMITTED line in
    // any tester log) while its hooks cost twelve mid-frame detours and a global lock on every
    // hooked call. Record therefore no longer installs them or asks, and the compute wait runs
    // whatever this says. Still ON by default, as in every earlier build: the key sets SpinDraw
    // before the runtime's Init (InitPass), and the runtime builds its graphics root signature
    // and one-pixel-draw PSO at Init from that value, so the default keeps the runtime's Init
    // exactly as it was. The hooks are gone because of kGraphicsAdmissionPossible, not because
    // of this key. On, it also logs "graphics wait unavailable in this build" once; off, the
    // runtime no longer builds that pipeline at Init (never A/B-tested in a game). See
    // AmdPreSr.cpp, kGraphicsAdmissionPossible.
    CustomOptional<bool> AmdGraphicsWaitExperimental { true };
    // Every-frame is the only configuration under test, so it is the default:
    // enabling neural rendering is then the single switch a test run needs.
    CustomOptional<bool> AmdEveryFrame { true };
    // Final image mode: run the neural pass on the swapchain back buffer at Present, for
    // games with FSR 1 or no upscaler (nothing for the pre-SR path to hook). No motion, no
    // depth, no history; HUD included; D3D12 directly, D3D11 through the shared-texture
    // bridge. Idle whenever the bridge's backend exists. See dlssnr/amd/PresentExperimental.h.
    CustomOptional<bool> AmdFinalImage { false };
    // Retained for existing INIs. The host currently forces compute (0) because
    // graphics waiting changes state the host cannot completely restore.
    CustomOptional<int> AmdSpinDraw { 0 };
    // Vulkan titles (Vulkan-on-D3D12 bridge), EXPERIMENTAL, default false. With NR on, the bridge's
    // D3D12 queue Wait on the game's texture copies is queued right before the bridge list executes,
    // instead of before the upscaler and NR record, so queue work the runtimes issue while recording
    // (danielblnc's staging drain and inline flag check) no longer waits for the game's own
    // vkQueueSubmit. Off: the upstream order, unchanged. See IFeature_VkwDx12.cpp, FlushCopyWait.
    CustomOptional<bool> AmdVkLateCopyWait { false };
    // Competitor-audit fixes (0.3.3.2 rebuild), both runtimes, on by default. Kill switches: read, never saved.
    // AmdNrColourGuard (C3-A): XeSS, FSR 2.2 and FSR 2.1.2 leave AMDNR's NR replacement alone - no ColorResourceBarrier
    // and no Unreal RENDER_TARGET transition on it, in or out - as the FFX path already does: it arrives in
    // NON_PIXEL_SHADER_RESOURCE and must leave in it. The title's own colour is unchanged. false = 0.3.3.2 behaviour.
    CustomOptional<bool> AmdNrColourGuard { true };
    // AmdSkipUnwrittenFrames (C6-A): no neural pass after the upscaler (after Ray Regeneration on AMD) on a frame it did
    // not write - SkipFirstFrames, a root signature it cannot restore, a backend change or recreate, the FSR 2.1.2
    // fallback - and NR history restarts. The NVIDIA path's post pass follows the same rule. false = 0.3.3.2 behaviour
    // (the pass edits the previous output a second time).
    CustomOptional<bool> AmdSkipUnwrittenFrames { true };
    // (0.3.3.2) A neural command list the game discards. Both runtimes record their work into the
    // game's list and act when it is executed; a list reset or released without being executed left
    // danielblnc's runtime waiting for it for the rest of the session ("previous Record still awaits
    // submission"), and could let lmxxf launch a job on inputs that were never copied. On: the
    // bridge watches ID3D12GraphicsCommandList::Reset for the one list NR last recorded into (a
    // pointer compare per Reset, nothing more), OptiScaler's D3D11 / Vulkan bridges report a list
    // they did not execute, and a list only NR still holds counts as released. danielblnc then
    // completes the dropped job without the game's list and NR resumes when it retires (about 4 s,
    // the runtime's own wait for a capture that never lands); lmxxf drops the job as a lost frame.
    // false = no hook and no recovery, as before. See AmdBridge.cpp (ResetHook) and AmdPreSr.cpp
    // (RecoverDiscarded). Not yet tested in a game: nothing reproduces a discarded list on demand.
    CustomOptional<bool> AmdNeuralListRecovery { true };
    // (0.3.3.2) NR runs only on the D3D12 device its backend was built on. A frame recorded on
    // another device (a second adapter, or a game that re-creates its device) goes to the upscaler
    // untouched, logged once per adapter, instead of reaching resources that belong to the first
    // device. false = no check, as before. See AmdBridge.cpp (Run, the device gate).
    CustomOptional<bool> AmdDeviceGate { true };
    // (0.3.3.2) One neural stream per presented frame. When more than one NR entry per presented
    // frame is measured (split view, a scope or picture-in-picture, a second upscaler context), NR
    // runs on one of them - the largest - and the others pass through untouched, so they no longer
    // restart or mix its history. A game with one entry per frame is not affected. false = every
    // entry runs NR, as before. See AmdBridge.cpp (OneStreamPerFrame).
    CustomOptional<bool> AmdOneStreamPerFrame { true };
    // (0.3.4, P3) Late submission (The Last of Us Part II; likely other job-system engines): the game submits a frame's
    // command list only after the next frame's Evaluate. lmxxf keeps such a job one frame (that frame feeds nothing; the
    // answer comes one frame later, measured against a copy of the frame it was fed and carried on by the motion)
    // instead of dropping it and restarting the network history; still unsubmitted a frame later, it is dropped as
    // before. false = drop at once, as 0.3.3.2. Read at load, never saved. See dlssnr/lmxxf/LateSubmitGrace.h.
    CustomOptional<bool> AmdLateSubmitGrace { true };
    // (0.3.4, P1 Marvel's Midnight Suns) danielblnc's runtime runs on the device behind the game's D3D12 device when that
    // is a proven proxy of the device of the game's queue (a Streamline interposer proxy: its base-object GUID or a fence
    // probe), so its own command lists match the queue it submits them to; two devices with no proof: it is not started
    // (it would crash its first submission). lmxxf and every other game unchanged. false = the 0.3.3.2 pairing (the
    // proxy device with the native queue). Read at load, never saved. See dlssnr/amd/ComIdentity.h (ChooseNrDevice).
    CustomOptional<bool> AmdStreamlineDeviceFix { true };
    CustomOptional<bool> AmdRtgiEnabled { false };
    CustomOptional<uint32_t> AmdRtgiQuality { 2 };
    CustomOptional<uint32_t> AmdRtgiDenoiser { 1 };
    CustomOptional<uint32_t> AmdRtgiInspect { 0 };
    CustomOptional<float> AmdRtgiContact { 0 };
    CustomOptional<float> AmdRtgiSaturation { 1 };
    CustomOptional<float> AmdRtgiRadius { 1 };
    CustomOptional<float> AmdRtgiMix { 1 };
    CustomOptional<float> AmdRtgiLighting { 5 };
    CustomOptional<float> AmdRtgiOcclusion { 1 };
    CustomOptional<float> AmdRtgiAmbient { 1 };
    CustomOptional<float> AmdRtgiThickness { .1f };
    CustomOptional<float> AmdRtgiSmoothness { .5f };
    CustomOptional<float> AmdRtgiFade { .3f };
    CustomOptional<float> AmdRtgiFov { 60 };
    CustomOptional<float> AmdRtgiFarPlane { 600 };
    // (0.3.4 preview) [AmdGi] AMDNR Screen GI (dlssnr/gi; design: the AMDNR SSGI design note 6.1).
    // Off by default; with Enabled=false nothing runs and nothing is allocated. Saved like the neighbours except
    // DebugView and the hidden experiment keys (ini only, never written by Save Settings).
    CustomOptional<bool> AmdGiEnabled { false };
    CustomOptional<int> AmdGiQuality { 2 };          // 0 Low, 1 Medium, 2 High, 3 Ultra, 4 Auto; unset = 0 on APUs
    CustomOptional<float> AmdGiIntensity { 1.0f };   // Bounce light 0..3
    CustomOptional<float> AmdGiOcclusion { 1.0f };   // Ambient occlusion 0..2
    CustomOptional<float> AmdGiRadius { 1.0f };      // 0.25..4, multiplier on the tier's screen radius
    CustomOptional<float> AmdGiThickness { 0.0f };   // 0 = auto, 0.01..1 (fraction of view depth)
    CustomOptional<float> AmdGiSaturation { 1.0f };  // Bounce colour 0..2
    CustomOptional<float> AmdGiSky { 0.0f };         // Sky light 0..2
    CustomOptional<float> AmdGiFeedback { 0.5f };    // Multi-bounce 0..0.8
    CustomOptional<int> AmdGiPlacement { 0 };        // 0 before NR, 1 after NR
    CustomOptional<float> AmdGiFov { 0.0f };         // 0 = auto (camera chain), else 20..140 degrees
    CustomOptional<int> AmdGiFovAxis { 0 };          // 0 vertical, 1 horizontal
    CustomOptional<float> AmdGiNearFade { -1.0f };   // -1 auto, 0 off, > 0 value
    CustomOptional<float> AmdGiDistanceFade { -1.0f };
    CustomOptional<int> AmdGiEncoding { -1 };        // -1 auto, 0 linear, 1 sRGB, 2 gamma 2.2
    CustomOptional<int> AmdGiDebugView { 0 };        // 0..10, not saved
    CustomOptional<int> AmdGiDepthConvention { -1 }; // hidden: -1 auto, 0 standard, 1 reversed, 2 linear
    CustomOptional<float> AmdGiTraceCap { 0.0f };    // hidden: trace-grid Mpx, 0 = tier
    CustomOptional<int> AmdGiAlbedoMode { 0 };       // hidden: 0..2
    CustomOptional<float> AmdGiAoLitProtect { 1.0f }; // hidden: 0..1
    CustomOptional<int> AmdGiTranslucency { 0 };     // hidden: 0 off, 1 heuristic
    CustomOptional<bool> AmdLookEnabled { false };
    CustomOptional<uint32_t> AmdLookAppearance { 2 };
    CustomOptional<float> AmdLookMix { 1.0f };
    CustomOptional<float> AmdLookMaterialDetail { 1.15f };
    CustomOptional<float> AmdLookShapeDefinition { 1.20f };
    CustomOptional<float> AmdLookLocalLighting { 1.15f };
    CustomOptional<float> AmdLookSkinDetail { 1.10f };
    CustomOptional<float> AmdLookSkinSoftness { 0.486f };
    CustomOptional<bool> AmdLookDetectSkin { true };
    CustomOptional<float> AmdLookSpecularControl { 0.58f };
    CustomOptional<float> AmdLookHighlightRollOff { 0.9f };
    CustomOptional<float> AmdLookColourSeparation { 0.0f };
    CustomOptional<float> AmdLookShadowDepth { 0.2f };
    CustomOptional<float> AmdLookAntiHalo { 0.901f };
    CustomOptional<float> AmdLookFlatAreaProtection { 0.0f };
    CustomOptional<uint32_t> AmdLookInspect { 0 };
    CustomOptional<float> AmdLookTone { 0.0f };
    CustomOptional<float> AmdLookExposureEV { 1.0f };
    CustomOptional<float> AmdLookContrast { 1.0f };
    CustomOptional<float> AmdLookSaturation { 1.0f };
    CustomOptional<float> AmdLookHighlightCompression { 0.0f };
    // -1 means follow local structure, which is the model's own default. It is not a strength of zero.
    CustomOptional<float> DlssNrSkinStructure { -1.0f };
    CustomOptional<bool> DlssNrAutoMask { true };
    // Optional final-composition filter, not NVIDIA's semantic auto mask.
    CustomOptional<bool> DlssNrSkinProtection { false };
    CustomOptional<bool> DlssNrSkinToneEnabled { true };
    CustomOptional<float> DlssNrSkinDetail { 1.0f };
    CustomOptional<float> DlssNrSkinColour { 1.0f };
    CustomOptional<float> DlssNrEnvironmentDetail { 1.0f };
    CustomOptional<float> DlssNrEnvironmentColour { 1.0f };
    CustomOptional<bool> DlssNrShowSkinMask { false };
    CustomOptional<float, NoDefault> DlssNrPass2Intensity;
    CustomOptional<float, NoDefault> DlssNrPass2LocalStructure;
    CustomOptional<float, NoDefault> DlssNrPass2LocalTone;
    CustomOptional<float, NoDefault> DlssNrPass2SkinStructure;
    CustomOptional<bool, NoDefault> DlssNrPass2AutoMask;
    CustomOptional<float, NoDefault> DlssNrPass3Intensity;
    CustomOptional<float, NoDefault> DlssNrPass3LocalStructure;
    CustomOptional<float, NoDefault> DlssNrPass3LocalTone;
    CustomOptional<float, NoDefault> DlssNrPass3SkinStructure;
    CustomOptional<bool, NoDefault> DlssNrPass3AutoMask;

    // How much of the model's edit reaches the frame. Separated because detail synthesis is a luminance
    // edit and any colour shift is usually the part you do not want, and allowed past 1.0 because
    // exaggerating an edit is the only honest way to see whether there is one.
    CustomOptional<float> DlssNrTransferStrength { 1.0f };
    CustomOptional<float> DlssNrColourStrength { 1.0f };

    // The RenoDX reversible proxy mode. 0 = today's soft-knee encode + our composition (default,
    // byte-identical); 1 = unclipped Neutwo proxy + our composition; 2 = Neutwo proxy + pure-inverse
    // replace. An in-game A/B and a way back. Default 0 = byte-identical to before.
    CustomOptional<uint32_t> DlssNrReversibleMode { 0 };

    // Whether the model's edit is applied. Off keeps the pass running (so Hold frame works) but shows
    // the clean upscaler frame -- for A/B'ing NR on/off on a frozen frame. Default true.
    CustomOptional<bool> DlssNrApplyModel { true };

    // Frame hold: freeze the NR pass's input so a live setting change re-renders the SAME frame -- the
    // only clean way to A/B our settings. A live testing toggle, not really a saved preference; off by
    // default. See dlssnr/design/frame-hold.md.
    CustomOptional<bool> DlssNrHoldFrame { false };


    // The most the pass may multiply or divide a pixel by. A detail pass has no business restyling a
    // light source, whatever the model returns.
    CustomOptional<float> DlssNrMaxRatio { 2.0f };

    // How a model that worked below the frame's size is brought back. 0 classic, 1 matched
    // residual. Only has an effect when Model resolution is under 100%.
    CustomOptional<uint32_t> DlssNrTransfer { 1 };

    // Measure the white point from the frame instead of taking it from the slider. On a frame the
    // game already tone mapped there is nothing to measure and this has no effect.
    //
    // Off by default, because it is not finished. The pass writes its result back into the same buffer
    // the meter reads, so with the pass running the meter is partly measuring its own output and the
    // two chase each other: Enshrouded, one session, 1545 samples spanning 0.01 to 97.9 with 57 jumps
    // beyond 1.5x in a single frame. Measured in the same spot seconds apart, 41.31 with the pass off
    // against 0.46 with it on. That is visible as the picture pumping and occasionally flickering.
    //
    // The slider is the supported control until the loop is broken. This stays as an opt-in so the
    // behaviour can still be looked at.


    // Take the white point from the game's own exposure texture instead of measuring or guessing.
    // Off by default until it has been seen to work in more than one game.
    CustomOptional<bool> DlssNrWhitePointFromExposure { true };

    // Ask the model, once, whether it will run on Direct3D 11 without the bridge.
    //
    // Off by default and deliberately so. Everything else this pass does reads memory it already owns;
    // this one initialises an NVIDIA subsystem on the game's live D3D11 device, in a process where the
    // D3D12 NGX instance is already running. It should return an error code and nothing more, but
    // "should" is doing work in that sentence and it ships into games nobody can test first.
    CustomOptional<bool> DlssNrProbeD3D11 { false };

    // 0 off, 1 the picture the model was shown, 2 its raw answer, 3 what it changed, amplified.
    CustomOptional<uint32_t> DlssNrDebugView { 0 };

    // Showing the pass against itself, without having to toggle it and remember what the last frame
    // looked like. 0 off, 1 side by side, 2 a wipe.
    //
    // Side by side squeezes the whole frame into each half, so it is a comparison rather than
    // something to play in. The wipe cuts one frame and resamples nothing, so it is; the split is a
    // stored setting and stays where it was put once the menu closes.
    CustomOptional<uint32_t> DlssNrCompare { 0 };
    CustomOptional<float> DlssNrCompareSplit { 0.5f };

    // Side by side only. 1 fits the whole frame at its right shape and accepts the bars; 2 fills
    // the half and crops the sides off instead.
    CustomOptional<float> DlssNrCompareZoom { 1.0f };

    // Which side the edited frame sits on, in both comparison modes.
    CustomOptional<bool> DlssNrCompareSwap { false };

    // Labels drawn onto the two sides of a comparison, so a screenshot still says which is which.
    // Drawn into the frame's own plane with a clip per side: in the wipe they are revealed and hidden
    // by the split exactly as the images are, and there is nothing to drag.
    CustomOptional<bool> DlssNrCompareTags { false };
    CustomOptional<float> DlssNrTagScale { 1.5f };

    // The fraction of the frame's resolution the model works at. The frame itself is never reduced --
    // only the model's contribution is computed small and enlarged, so the picture underneath is
    // untouched whatever this is set to. 1.0 is full resolution and behaves exactly as before.
    CustomOptional<float> DlssNrWorkingScale { 1.0f };

    // Filter used for NR supersampling (working scale > 1): the model runs above native, and this is
    // the downscaler that averages its answer back to native. Independent of OutputScalingDownscaler
    // so NR and Output Scaling can run different filters at once. Lanczos3 is the sharp default.
    CustomOptional<Scaler> DlssNrScalingDownscaler { Scaler::Lanczos3 };

    // Ask the driver's own nvngx.dll whether it will dispatch Neural Rendering, once per session.
    //
    // Everything here drives the model's DLL directly through a forwarder, because the model refuses
    // callers whose module path does not contain "nvngx.dll". But the model ships inside the driver
    // store, and NVIDIA does not ship a feature DLL that no dispatcher can reach -- so the driver's
    // nvngx.dll may well know feature 18 already. If it does, the forwarder is unnecessary, the
    // signature question disappears, and users stop needing a 165 MB copy in every game folder.
    //
    // Off by default: it is a diagnostic, not a feature.
    CustomOptional<bool> DlssNrProxyProbe { false };

    // Run Neural Rendering through the driver's own nvngx.dll rather than through the forwarder.
    //
    // This is how DLSS itself is called. The forwarder exists only because driving the model
    // directly trips its caller check, and a probe showed the driver dispatches feature 18 already:
    // asking for 18 answers differently from asking for a feature that does not exist. OptiScaler
    // also already tells the driver where to look, since NVNGX_FeatureInfo_Paths carries the game
    // and OptiScaler folders into Init_Ext.
    //
    // Off until it is shown to produce the same picture. If it does, the forwarder can go.
    CustomOptional<bool> DlssNrUseProxy { false };

    // Look for the exposure the game computed but never handed to the upscaler.
    //
    // Off by default, and it has to be. Reading a resource the game owns means assuming what state
    // it is in, and unlike depth and motion vectors -- where NGX documents the contract -- a buffer
    // found by its shape comes with no promise at all. UNORDERED_ACCESS is the reasonable
    // assumption, since every candidate got here by having a UAV made on it, but it is an
    // assumption, and nobody who has not asked for the scan should be carrying that risk.
    //
    // It decides nothing either way. It watches and it reports, because the last two times a number
    // was inferred here it went straight into the interface and was wrong.
    CustomOptional<bool> DlssNrScanExposure { false };

    // Anchoring the scan: the white point that looked right, and the scan's value at that moment.
    //
    // The absolute white point cannot be derived from a buffer whose units are unknown. What CAN be
    // derived is every value after the first: if the scan's number halves, the scene got twice as
    // bright, and the white point follows -- whatever the number actually means, because only the
    // ratio is used and the units cancel.
    //
    // So the user sets it once, in one lighting condition, and presses a button. After that it stays
    // correct through every cave and every noon without being touched again. Which is also the shape
    // that makes per-game profiles work: one person anchors a game, everybody else gets the number.
    //
    // Zero means not anchored, and then nothing happens at all.
    // A lamp in the corner showing what the scan currently thinks the light is doing: red for dark,
    // green for full light, and the shades between. Off by default; it is for watching the thing
    // work, not for playing with.
    // Where the white point comes from. One control, because there is one answer.
    //
    //   0  the paper white slider, and nothing else
    //   1  the exposure the game hands the upscaler
    //   2  a buffer the scan found, anchored to a white point the user chose once
    //
    // This replaces two independent checkboxes that could both be on. They were made exclusive by
    // greying, which deadlocked -- each disabled the other, so once both were set the only way out
    // was a button the notice never mentioned -- and then by clearing, which silently undid a
    // setting the user had made. Both were attempts to stop an illegal state being REACHED. A single
    // choice cannot reach it: there is nothing to keep consistent, because there is only one value.
    CustomOptional<uint32_t> DlssNrWhitePointSource { 1 };

    CustomOptional<bool> DlssNrScanMeter { false };

    CustomOptional<float> DlssNrScanAnchorValue { 0.0f };       // legacy single anchor, migrated then unused
    CustomOptional<float> DlssNrScanAnchorWhitePoint { 0.0f };  // legacy single anchor, migrated then unused

    // The multi-point anchor table, serialised as "scan:white;scan:white;..." ascending. See
    // dlssnr/design/multi-point-anchoring.md. Replaces the single pair above; a pre-existing single
    // anchor is migrated into a one-row table on first load.
    CustomOptional<std::string> DlssNrScanAnchors { std::string() };
    // Which AMD neural runtime carries the pass ([DlssNr] NrBackend): "daniel" - danielblnc's
    // closed 0.3.x / 0.4.x runtime (dlssnr_amd_pass1..3.dll + dlssnr_on_amd_weights.bin) - or "lmxxf" -
    // the open-source HIP runtime (DLSS5-AMD\native-game-tiled-assets next to the game). Empty
    // = not chosen yet: the menu opens on the first launch that finds either runtime installed
    // and asks. The lmxxf backend itself lands in 0.3.0; until then a choice of lmxxf is
    // recorded, and this build runs danielblnc's runtime when that is installed.
    CustomOptional<std::string> DlssNrBackend { std::string() };
    // NR style slots (Neural tab > NR style > Custom style slots): the appearance controls
    // captured as "key=value;..." strings, saved with the rest of the ini.
    CustomOptional<std::string> AmdStyleSlot1 { std::string() };
    CustomOptional<std::string> AmdStyleSlot2 { std::string() };
    CustomOptional<std::string> AmdStyleSlot3 { std::string() };

    // Whether the scan's number rises or falls with the light.
    //
    // A found buffer carries no contract. Most engines store an exposure -- a multiplier that goes
    // DOWN as the scene gets brighter -- but some store its reciprocal, and nothing in the buffer
    // says which. Rather than guess and be silently wrong in half the games, this is one click: if
    // the picture moves the wrong way, flip it.
    CustomOptional<bool> DlssNrScanInverted { false };







    // The trim on an exposure-derived white point, kept apart from the manual divisor on purpose.
    //
    // These are two different quantities that happened to share one slider: the manual path wants an
    // absolute divisor on an open-ended linear buffer, which in Nioh 3 is about 240, and the exposure
    // path wants a multiplier on a number the game already supplied, where anything far from 1 is
    // a sign the read is wrong rather than a preference. Sharing one stored value meant touching the
    // slider in one mode silently destroyed the number found in the other.
    //
    // 1.0 is the identity: take the game's exposure exactly as given. That is the "safe value", and
    // it is safe by construction rather than by being written down somewhere.
    CustomOptional<float> DlssNrWhitePointTrim { 1.0f };

    // The scan's trim, kept apart from the exposure texture's.
    //
    // They are trims on different things and a value found against one is meaningless against the
    // other. Sharing one slider meant switching source silently carried a number across, so a
    // picture that had been tuned came back wrong for a reason nothing on screen explained.
    CustomOptional<float> DlssNrScanTrim { 1.0f };

    // How many sequential model layers to run between one encode and one final composition. Each extra
    // layer consumes the preceding model output and owns a persistent feature/history. The implementation
    // deliberately caps this at three and never evaluates a feature on the command list that created it.
    //
    // 1 is what the model was trained for and what every published number describes. Above that it
    // is being asked to enhance its own output, which is outside its training distribution: detail
    // compounds, and so does anything it got wrong. Two often looks richer; three is the guarded
    // ceiling because further layers converge while still paying the full cost.
    //
    // The cost is exactly linear -- the model is 98% of the frame's expense and every pass pays it
    // again -- so 3 costs three times, near enough. There is no shortcut and no amortisation: the
    // passes are sequential and each one needs the last one's output.
    CustomOptional<uint32_t> DlssNrPasses { 1 };

    // Which depth convention the model is told the guide uses.
    //
    //   0  what the game's own DLSS feature was created with, which is what it means for the upscaler
    //   1  force normal
    //   2  force inverted
    //
    // Writes one set of matched before/after frames per session, without anyone having to ask. The
    // folder is cleared at the start of each run, so it holds one session's worth and never grows.
    //
    // OFF by default now. It was on, and the AMD path did not even read it: every session
    // copied eight frames of before/after colour to readback heaps, mapped them, scanned them,
    // wrote 60-130 MB to disk on the render thread, then did it four more times - and when the
    // sample was dark it re-armed with no limit. In a dark game (Resident Evil Requiem) that
    // loop ran every half second for the whole session. (The periodic FPS drops that led here
    // turned out to be the XeFG unlocker plugin, not this - but this was real waste on every
    // install regardless.) A diagnostic that costs frame time has to be asked for.
    CustomOptional<bool> DlssNrAutoCapture { false };





    // Multiplies the (auto or manual) white point before the encode: what the model considers "white".
    // Higher means highlights sit lower on the curve and the model treats them as less extreme.
    CustomOptional<float> DlssNrWhitePointScale { 1.0f };





    // --- end DLSS 5 Neural Rendering -------------------------------------------------------------

    // DLSS
    CustomOptional<bool> DLSSEnabled { true };
    CustomOptional<bool> RenderPresetOverride { false };
    CustomOptional<uint32_t> RenderPresetForAll { 0 };
    CustomOptional<uint32_t> RenderPresetDLAA { 0 };
    CustomOptional<uint32_t> RenderPresetUltraQuality { 0 };
    CustomOptional<uint32_t> RenderPresetQuality { 0 };
    CustomOptional<uint32_t> RenderPresetBalanced { 0 };
    CustomOptional<uint32_t> RenderPresetPerformance { 0 };
    CustomOptional<uint32_t> RenderPresetUltraPerformance { 0 };

    // DLSSD
    CustomOptional<bool> DLSSDRenderPresetOverride { false };
    CustomOptional<uint32_t> DLSSDRenderPresetForAll { 0 };
    CustomOptional<uint32_t> DLSSDRenderPresetDLAA { 0 };
    CustomOptional<uint32_t> DLSSDRenderPresetUltraQuality { 0 };
    CustomOptional<uint32_t> DLSSDRenderPresetQuality { 0 };
    CustomOptional<uint32_t> DLSSDRenderPresetBalanced { 0 };
    CustomOptional<uint32_t> DLSSDRenderPresetPerformance { 0 };
    CustomOptional<uint32_t> DLSSDRenderPresetUltraPerformance { 0 };

    // Nukems
    CustomOptional<bool> NvngxFGMakeDepthCopy { false };

    // Libraries
    CustomOptional<std::wstring, NoDefault> MainDllPath;
    CustomOptional<std::wstring, NoDefault> FfxDx12Path;
    CustomOptional<std::wstring, NoDefault> FfxDx12SRPath;
    CustomOptional<std::wstring, NoDefault> FfxDx12FGPath;
    CustomOptional<std::wstring, NoDefault> FfxDx12RRPath;
    CustomOptional<std::wstring, NoDefault> FfxDx12RCPath;
    CustomOptional<std::wstring, NoDefault> FfxVkPath;
    CustomOptional<std::wstring, NoDefault> XeSSLibrary;
    CustomOptional<std::wstring, NoDefault> XeFGLibrary;
    CustomOptional<std::wstring, NoDefault> XeLLLibrary;
    CustomOptional<std::wstring, NoDefault> XeSSDx11Library;
    CustomOptional<std::wstring, NoDefault> NvngxPath;
    CustomOptional<std::wstring, NoDefault> NVNGX_DLSS_Library;
    CustomOptional<std::wstring, NoDefault> DLSSFeaturePath;
    CustomOptional<std::wstring, NoDefault> NvapiDllPath;

    // Sharpness
    CustomOptional<SharpenShader> SharpnessShader { SharpenShader::RCAS };
    CustomOptional<bool> OverrideSharpness { false };
    CustomOptional<float> Sharpness { 0.4f };
    // (AMDNR 0.3.4.2, RN3) The sharpening FSR SR applies after FSR Ray Regeneration when the title sends no NGX
    // sharpness and OverrideSharpness is off. It is FSR SR's own sharpness, not a denoiser parameter, so it lives
    // here with its neighbours and not in [FSR-RR]. The default is the shipped one, unchanged, and the sibling
    // constant FSRDFeatureDx12::kRrDefaultSharpness (upscalers/fsr31/FSRDFeature_Dx12.h) holds the same number - one
    // of them is the source of truth and a test asserts they agree. 0 = nothing sharpens RR's output; Wine/Proton
    // stays at 0 whatever this says (hooks/RrHardwareGate.h), and an explicit OverrideSharpness + Sharpness wins.
    CustomOptional<float> RrDefaultSharpness { 0.25f };

    // RCAS
    CustomOptional<bool> RcasEnabled { false };
    CustomOptional<bool> ContrastEnabled { false };
    CustomOptional<float> Contrast { -0.3f };

    // DA Sharpening
    CustomOptional<float, NoDefault> DADepthScale;
    CustomOptional<float, NoDefault> DADepthBias;
    CustomOptional<bool, NoDefault> DAClampOutput;
    // The title's depth-aware sharpening depth is already linear rather than an NDC
    // value. Read by the FSR-RR path, which has to know a depth's convention before it
    // can hand it to the denoiser.
    CustomOptional<bool> DADepthIsLinear { false };

    // MAS
    CustomOptional<bool> MotionSharpnessEnabled { false };
    CustomOptional<bool> MotionSharpnessDebug { false };
    CustomOptional<float> MotionSharpness { 0.2f };
    CustomOptional<float> MotionThreshold { 0.0f };
    CustomOptional<float> MotionScaleLimit { 10.0f };

    // Magnifier
    CustomOptional<bool> MagnifierEnabled { false };
    CustomOptional<float> MagnifierSize { 15.f }; // % of screen Height
    CustomOptional<int> MagnifierZoomFactor { 4 };
    CustomOptional<float> MagnifierBorderSize { 0.3f };   // % of screen Height
    CustomOptional<float> MagnifierCursorOffsetX { 0.f }; // Pixels
    CustomOptional<float> MagnifierCursorOffsetY { 0.f }; // Pixels
    CustomOptional<float, NoDefault> MagnifierStaticPosX; // % of screen Width, static pos enabled if both are defined
    CustomOptional<float, NoDefault> MagnifierStaticPosY; // % of screen Height

    // Menu
    CustomOptional<float, NoDefault> MenuScale;
    CustomOptional<bool> OverlayMenu { true };
    CustomOptional<int> ShortcutKey { VK_INSERT };
    CustomOptional<bool> ExtendedLimits { false };
    CustomOptional<bool> ShowFps { false };
    /// 0 Top Left, 1 Top Right, 2 Bottom Left, 3 Bottom Right
    CustomOptional<FpsOverlayPos> FpsOverlayPosition { FpsOverlayPos_TopLeft };
    /// 0 Only FPS, 1 +Avg FPS & Upscaler info 2 +Frame Time,
    /// 3 +Upscaler Time, 4 +Frame Time Graph, 5 +Upscaler Time Graph
    /// 6 +Reflex timings
    CustomOptional<FpsOverlay> FpsOverlayType { FpsOverlay_JustFPS };
    CustomOptional<int> FpsShortcutKey { VK_PRIOR };
    CustomOptional<int> FpsCycleShortcutKey { VK_NEXT };
    CustomOptional<bool> FpsOverlayHorizontal { false };
    CustomOptional<float> FpsOverlayAlpha { 0.4f };
    CustomOptional<float, NoDefault> FpsScale; // No value means same as MenuScale
    CustomOptional<bool> UseHQFont { true };
    CustomOptional<bool> DisableSplash { false };
    CustomOptional<float> FontSize { 14.0f };
    CustomOptional<std::wstring, NoDefault> TTFFontPath;
    CustomOptional<int> FGShortcutKey { VK_END };
    CustomOptional<bool> LightTheme { false };
    CustomOptional<bool> OverlaysUseTheme { false };
    // Red on near-black. The blue was stock ImGui's, which is exactly why every tool
    // built on ImGui looks the same; still fully overridable from the Interface tab.
    CustomOptional<float> MenuAccentColorR { 0.86f };
    CustomOptional<float> MenuAccentColorG { 0.14f };
    CustomOptional<float> MenuAccentColorB { 0.18f };
    CustomOptional<float> MenuBGColorR { 0.0f };
    CustomOptional<float> MenuBGColorG { 0.0f };
    CustomOptional<float> MenuBGColorB { 0.0f };
    CustomOptional<float> MenuBGColorA { 0.99f };

    // Hooks
    CustomOptional<bool> HookOriginalNvngxOnly { false };
    CustomOptional<bool> EarlyHooking { false };
    CustomOptional<bool> UseNtdllHooks { true };
    // (AMDNR 0.3.3.2) Vulkan instances and devices OptiScaler creates for itself (under Proton or DXVK,
    // inside its own D3D12 / DXGI calls: the GPU probe, the Vulkan bridge's D3D12 device, Anti-Lag 2)
    // are passed through without being taken for the game's: State::VulkanInstance and the Anti-Lag 2
    // state stay the game's. Decided per create call on the calling thread (ScopedOwnVulkanObjects,
    // State.h). false = the upstream behaviour. See hooks/Vulkan_Hooks.cpp.
    CustomOptional<bool> SkipOwnVulkanObjects { true };

    // Upscale Ratio Override
    CustomOptional<bool> UpscaleRatioOverrideEnabled { false };
    CustomOptional<float> UpscaleRatioOverrideValue { 1.3f };

    // DRS
    CustomOptional<bool> DrsMinOverrideEnabled { false };
    CustomOptional<bool> DrsMaxOverrideEnabled { false };

    // Quality Overrides
    CustomOptional<bool> QualityRatioOverrideEnabled { false };
    CustomOptional<float> QualityRatio_DLAA { 1.0f };
    CustomOptional<float> QualityRatio_UltraQuality { 1.3f };
    CustomOptional<float> QualityRatio_Quality { 1.5f };
    CustomOptional<float> QualityRatio_Balanced { 1.7f };
    CustomOptional<float> QualityRatio_Performance { 2.0f };
    CustomOptional<float> QualityRatio_UltraPerformance { 3.0f };

    // ProcessFilter
    CustomOptional<std::wstring, NoDefault> TargetProcess;
    // 0.3.3.2: crashclientreporter.exe (Neverness to Everness's crash reporter emptied the game's
    // log), unrealcefsubprocess.exe (UE CEF helper, preventive), dlssnr_on_amd_setup.exe (danielblnc's
    // installer rotated the game's log away).
    CustomOptional<std::wstring> ProcessExclusionList = {
        L"crashpad_handler.exe|crashreport.exe|crashreporter.exe|crs-handler.exe|crs-uploader.exe|crs-video.exe|"
        L"unitycrashhandler64.exe|idtechlauncher.exe|cefviewwing.exe|ace-setup64.exe|ace-service64.exe|"
        L"qtwebengineprocess.exe|platformprocess.exe|bugsplathd64.exe|bssndrpt64.exe|pspcsdkappmgr.exe|pspcsdkcore.exe|"
        L"pspcsdkstttts.exe|pspcsdktelemetry.exe|pspcsdkui.exe|pspcsdkupdatechecker.exe|pspcsdkvoicechat.exe|"
        L"pspcsdkwebview.exe|windhawk.exe|vscodium.exe|crash_reporter.exe|steamerrorreporter64.exe|crashreportclient."
        L"exe|edcefcrashpadprocess.exe|edcefrenderprocess.exe|crashclientreporter.exe|unrealcefsubprocess.exe|"
        L"dlssnr_on_amd_setup.exe"
    };

    // Hotfixes
    // AMDNR 0.3.3.2: asks AMDNR's GitHub releases (3zwr1/AMD-NR---OptiScaler) and compares the AMD-NR version, not
    // upstream OptiScaler's (version_check.cpp)
    CustomOptional<bool> CheckForUpdate { true };
    CustomOptional<bool, SoftDefault> DisableOverlays { false };

    CustomOptional<bool> SimulateWaitableObject { false };

    CustomOptional<float, NoDefault> MipmapBiasOverride; // disabled by default
    CustomOptional<bool> MipmapBiasFixedOverride { false };
    CustomOptional<bool> MipmapBiasScaleOverride { false };
    CustomOptional<bool> MipmapBiasOverrideAll { false };

    CustomOptional<int, NoDefault> AnisotropyOverride; // disabled by default
    CustomOptional<bool> OverrideShaderSampler { true };
    CustomOptional<bool> AnisotropyModifyComp { true };
    CustomOptional<bool> AnisotropyModifyMinMax { true };
    CustomOptional<bool> AnisotropySkipPointFilter { true };

    CustomOptional<int, NoDefault> RoundInternalResolution; // disabled by default

    CustomOptional<int, NoDefault> SkipFirstFrames; // disabled by default
    CustomOptional<bool> RestoreComputeSignature { false };
    CustomOptional<bool> RestoreGraphicSignature { false };
    CustomOptional<bool> ExtendedStateRestore { false };

    CustomOptional<bool> UsePrecompiledShaders { true };

    CustomOptional<bool> UseGenericAppIdWithDlss { false };
    CustomOptional<bool> PreferDedicatedGpu { true };
    CustomOptional<bool> PreferFirstDedicatedGpu { false };

    CustomOptional<int32_t, NoDefault> ColorResourceBarrier;    // disabled by default
    CustomOptional<int32_t, NoDefault> MVResourceBarrier;       // disabled by default
    CustomOptional<int32_t, NoDefault> DepthResourceBarrier;    // disabled by default
    CustomOptional<int32_t, NoDefault> ExposureResourceBarrier; // disabled by default
    CustomOptional<int32_t, NoDefault> MaskResourceBarrier;     // disabled by default
    CustomOptional<int32_t, NoDefault> OutputResourceBarrier;   // disabled by default

    CustomOptional<bool> CreateD3D12DeviceForLuma { false };

    // Crash-triage kill switches ([Hotfix] Diag*). All default to today's behaviour; each one
    // that is set logs a single "[Diag]" INFO line where it takes effect. They are read at
    // startup and are not written back by SaveIni (a key the user set stays in the ini).
    // DiagNoInputHooks: OptiInput never starts (no WndProc subclass, no Win32/HID/raw/
    //   SetWindowsHookEx/cursor/XInput/DirectInput/GameInput hooks). The menu gets no input.
    CustomOptional<bool> DiagNoInputHooks { false };
    // DiagInputHooksSkip: comma list of OptiInput hook groups to leave uninstalled:
    //   message, keystate, msgpos, clip, sendpost, hid, raw, winhook, cursor, xinput, dinput,
    //   gameinput, subclass. "minimal" = everything except message and subclass.
    //   AMDNR 0.3.4.2, two more names that are not hook groups ("minimal" leaves them on): "presslatch" = the menu
    //   key toggles on its release again, as in 0.3.4.1 (menu/input/KeyPressLatch.h); "clickreplay" = no replay of
    //   clicks and nav keys shorter than a frame (menu/input/EdgeReplay.h).
    CustomOptional<std::string> DiagInputHooksSkip { std::string() };
    // DiagNoAmdxc64Hooks: amdxc64.dll's AmdExtD3DCreateInterface is neither loaded early nor
    //   detoured; calls reach the driver unchanged (no FSR 4 upgrade factory, no AL2 proxy).
    CustomOptional<bool> DiagNoAmdxc64Hooks { false };
    // DiagNoStreamlineHooks: no detours into sl.interposer/sl.dlss/sl.dlss_d/sl.dlss_g/
    //   sl.reflex/sl.pcl/sl.common (no NVIDIA spoof inside Streamline, no DLSS/RR on AMD).
    CustomOptional<bool> DiagNoStreamlineHooks { false };
    // DiagNoAntiLag2: OptiScaler does not start Anti-Lag 2 (low-latency falls back to LatencyFlex).
    CustomOptional<bool> DiagNoAntiLag2 { false };
    // DiagNoTrustHooks: no process-wide detours on WinVerifyTrust, CryptQueryObject and
    //   gdi32 D3DKMTQueryAdapterInfo.
    CustomOptional<bool> DiagNoTrustHooks { false };
    // Second set (RE Requiem 1.3.1.0 on RDNA 4). Accessors with the one-time "[Diag]" line are
    // in misc/DiagSwitches.h.
    // DiagNoFfxProxies: OptiScaler loads none of its FidelityFX DLLs, does not detour the game's
    //   amd_fidelityfx_* exports and no longer redirects the game's FidelityFX loads to its copies.
    CustomOptional<bool> DiagNoFfxProxies { false };
    // DiagNoXessProxies: no OptiScaler libxess/libxess_dx11/libxess_fg/libxell, no detours or export
    //   redirects on the game's copies, no libxell GetModuleHandleExA spoof, no XeSS-FG unlock patch.
    CustomOptional<bool> DiagNoXessProxies { false };
    // DiagNoSwapchainWrap: the DXGI factory detours stay, but every swapchain is returned to the
    //   game exactly as DXGI created it (no wrapper: no menu, no FG, no Present work).
    CustomOptional<bool> DiagNoSwapchainWrap { false };
    // DiagNoOverlay: the swapchain wrapper stays, but its Present does no overlay work (no ImGui,
    //   no render targets or back-buffer references, no overlay command lists on the game's queue).
    CustomOptional<bool> DiagNoOverlay { false };
    // DiagNoD3D12Hooks: no detours on d3d12.dll exports, D3D12Core D3D12GetInterface, device
    //   methods or command lists (also removes the DLSS-NR exposure scan). Diagnosis only.
    CustomOptional<bool> DiagNoD3D12Hooks { false };
    // DiagNoAmdGpuProbe: the GPU capability probe does not load amdxc64 or query FP8 through
    //   AmdExtD3DCreateInterface on its probe device; FSR 4 support comes from the card table.
    CustomOptional<bool> DiagNoAmdGpuProbe { false };
    // AMDNR 0.3.4, [Hotfix]. Read at startup, not written back by SaveIni.
    // MenuToggleDebounceMs: a second menu or NR toggle edge within this many ms of an accepted one is ignored (one
    //   press arrived as two release edges 16-316 ms apart in Assetto Corsa). 0 = every edge toggles, as in 0.3.3.2.
    //   See menu/input/ToggleGate.h. AMDNR 0.3.4.2: the menu key toggles on its press through a latch first
    //   (menu/input/KeyPressLatch.h); this gate stays behind it as a backstop.
    CustomOptional<int> MenuToggleDebounceMs { 400 };
    // MenuLowLevelHookPassThrough: while the menu blocks game input, OptiScaler's wrapper of the game's low-level
    //   keyboard / mouse hook passes the event on (CallNextHookEx) instead of returning 1, so Windows still gets it
    //   (Alt+Tab, the Windows key). false = 0.3.3.2 (the event is swallowed).
    CustomOptional<bool> MenuLowLevelHookPassThrough { true };
    // WarnMissingREFramework: the RE Engine "REFramework is missing" warning (log and menu). false silences it.
    CustomOptional<bool> WarnMissingREFramework { true };

    // Upscalers
    CustomOptional<Upscaler, SoftDefault> Dx11Upscaler { Upscaler::FSR22 };
    CustomOptional<Upscaler, SoftDefault> Dx12Upscaler { Upscaler::XeSS };
    CustomOptional<Upscaler, SoftDefault> VulkanUpscaler { Upscaler::FSR22 };

    // Output Scaling
    CustomOptional<bool> OutputScalingEnabled { false };
    CustomOptional<float> OutputScalingMultiplier { 1.5f };
    CustomOptional<Scaler> OutputScalingDownscaler { Scaler::FSR1 };

    // FSR
    CustomOptional<bool> FsrDebugView { false };
    // FSR 4's own debug visualisation, which is a different flag from FSR 3.1's
    // FsrDebugView above and is gated on a different provider version.
    CustomOptional<bool> Fsr4EnableDebugView { false };
    CustomOptional<int> FfxUpscalerIndex { 0 };
    CustomOptional<int> FfxFGIndex { 0 };
    CustomOptional<bool> FsrUseMaskForTransparency { true };
    CustomOptional<bool> FsrNonLinearColorSpace { false };
    CustomOptional<bool> FsrNonLinearSRGB { false };
    CustomOptional<bool> FsrNonLinearPQ { false };
    CustomOptional<bool> FsrAgilitySDKUpgrade { false };

    // These default values will be overwritten at upscaler init time with optimized values
    CustomOptional<float> FsrVelocity { 1.0f };
    CustomOptional<float> FsrReactiveScale { 1.0f };
    CustomOptional<float> FsrShadingScale { 1.0f };
    CustomOptional<float> FsrAccAddPerFrame { 0.333f };
    CustomOptional<float> FsrMinDisOccAcc { -0.333f };

    // FSR4
    CustomOptional<FSR4Support> Fsr4ForceModel { FSR4Support::None };
    CustomOptional<uint32_t, NoDefault> Fsr4Preset;
    // FSR-RR
    CustomOptional<int> FfxDenoiserIndex { 0 };
    // A 64-bit view id: the composition views (SkipSignal, DenoiserOutput, Correlation, ...) sit
    // above bit 32, so the ini value is parsed as an unsigned 64-bit number (decimal or 0x hex).
    CustomOptional<uint64_t> FfxDenoiserDebugMode { 0 };
    // -1: overview, 0..FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS-1: fullscreen viewport
    CustomOptional<int> FfxDenoiserDebugViewport { -1 };
    // Enables AMD's internal RR debug descriptors. Requires context recreation.
    CustomOptional<bool> FfxDenoiserInternalDebugViews { false };
    CustomOptional<int> FfxDenoiserDiffuseSignalType { 0 };  // 0: Direct, 1: Indirect
    CustomOptional<int> FfxDenoiserSpecularSignalType { 1 }; // 0: Direct, 1: Indirect
    // Single-signal denoising: dispatch only the enabled signals. A title whose raw
    // signal content only makes sense for one of the two can denoise the surviving
    // signal alone; the disabled signal's output stays zero and composition falls
    // back to the floor/raw-correlation path for it. Requires context recreation.
    CustomOptional<bool> FfxDenoiserDenoiseDiffuse { true };
    CustomOptional<bool> FfxDenoiserDenoiseSpecular { true };
    // Uses only semantic Streamline AO noisy/denoised tags. Resource-inspector
    // candidates are deliberately never promoted to signal inputs.
    CustomOptional<bool> FfxDenoiserTaggedAmbientOcclusion { false };
    // Disabled preserves the existing contract that RR input normals are world-space.
    CustomOptional<bool> FfxDenoiserNormalsInViewSpace { false };
    // Prefer the title's own linearised view depth when it publishes one, instead of
    // deriving it from the title's depth buffer and the projection. Off by default: the
    // derived path is the one every title so far has been validated against.
    CustomOptional<bool> FfxDenoiserUseTitleLinearDepth { false };
    // Responsivity values on the unstable side of this threshold route the specular
    // radiance through the spatial path as well. Zero disables the test.
    CustomOptional<float> FfxDenoiserResponsivityThreshold { 0.0f };
    // Selects which side of that threshold counts as unstable, since the polarity of the
    // title's mask is a title property.
    CustomOptional<bool> FfxDenoiserResponsivityInvert { false };

    // Routes pixels flagged by the DLSS bias-current-color mask (particles, alpha layers,
    // animated and video textures) around the denoiser via the floor and skip signal.
    // 0 restores the behaviour where the mask was bound but unused.
    // 0 by default in RE Requiem (GameQuirk::RrBiasMaskDefaultOff) while the key has no value; SaveIni keeps an
    // explicit value, also 1.0.
    CustomOptional<float> FfxDenoiserBiasMaskStrength { 1.0f };

    // Fraction of the floor filter's high-frequency luminance residual pushed back into the
    // floor on the final pass, so texture microcontrast bypasses the denoiser.
    CustomOptional<float> FfxDenoiserFloorDetailBoost { 0.0f };

    // Exponent on the floor filter's normal edge-stopping weight. Higher stops harder at
    // creases and silhouettes; 0 disables the term.
    CustomOptional<float> FfxDenoiserFloorNormalSharpness { 16.0f };

    // Fraction of the floor's luminance edge stop released where diffuse albedo says two taps
    // share a material, so shadows and reflections reach the denoiser instead of the floor.
    CustomOptional<float> FfxDenoiserFloorAlbedoGuide { 1.0f };

    // Blends the floor's luminance normaliser from centre-only (0) to max(centre, tap) (1).
    CustomOptional<float> FfxDenoiserFloorLumSymmetry { 1.0f };

    // Additional normal edge-stop exponent in proportion to screen-space surface slope.
    CustomOptional<float> FfxDenoiserFloorGrazingSharpness { 0.0f };
    // How far each a-trous pass returns a downward-biased estimate instead of the bilateral
    // mean, bounding the floor below the raw colour. Off by default; the crossing it removes
    // measures ~0.1% of a typical frame, so it is a bound rather than a fix. The knob that
    // fixed this title's floor softness is FloorDetailBoost.
    CustomOptional<float> FfxDenoiserFloorEnvelopeBias { 0.0f };

    // Soft knee on the opt-in floor/raw ceiling clamp. 0 is the exact min().
    CustomOptional<float> FfxDenoiserFloorSoftMin { 0.0f };

    // Overrides the DLSS.Use.HW.Depth interpretation. Unset follows NGX, which
    // defaults to linear when the title publishes nothing - and reading a hardware
    // depth buffer as linear collapses the whole scene to sub-unit distances.
    CustomOptional<bool, NoDefault> FfxDenoiserHardwareDepth;

    // Pushes AMD's own queried baseline for the six tunable RR keys instead of the
    // values below. A/B reference only - the fork's defaults remain the shipping
    // configuration, and the sliders keep their values while this is enabled.
    CustomOptional<bool> FfxDenoiserUseAmdDefaults { false };

    // Offer FSR Ray Regeneration (answer DLSS-RR "supported") on non-NVIDIA cards that are not RDNA 4. AMDNR 0.3.4
    // (RR-7000): unset (auto) = RDNA 4 + RDNA 3 / 3.5; true = any non-NVIDIA card; false = RDNA 4 only. The gate reads
    // has_value() (hooks/RrHardwareGate.h), not value_or_default(); SaveIni keeps an explicit false (ignore_default).
    CustomOptional<bool> FfxDenoiserAllowPreRdna4 { false };

    // Path-traced profile (AMD's sample contract): all lit radiance goes into RR except pixels
    // the game's bias mask flags, which still take the floor/skip path
    // (FfxDenoiserBiasMaskStrength); no raw re-injection after RR; and the temporal keys move
    // to AMD's documented defaults, except disocclusion 0.05 (the top of AMD's sample range;
    // AMD's own default is 0.01). The title's NGX sharpness is also not applied to FSR SR after RR
    // unless [Sharpness] OverrideSharpness=true (RCAS stays the player's switch). Acts as custom
    // defaults for the keys it covers - an explicit value of any of them still wins, also one
    // equal to the fork's default (SaveIni writes the ten keys and this flag with
    // value_for_config_ignore_default(); the profile's volatile values still save as auto).
    // Opt-in since 0.3.3.1: GameQuirk::PathTracedRayReconstruction (RE Requiem, PRAGMATA) only
    // labelled the checkbox in the menu (no entry sets it since 0.3.4). Retired from the menu in 0.3.4
    // (RR-25: worse in every RE Requiem report, also with the texture route); the key, its default
    // and the ten values still work from the ini.
    CustomOptional<bool> FfxDenoiserPathTracedProfile { false };

    // Path-traced profile only: how far textured albedo takes back the fork's floor and raw blend
    // (FloorIsolation / FloorRawBlend) while flat albedo (faces) keeps the profile. The route is
    // smooth (an albedo texture indicator, max-filtered and blurred) so that it does not draw an
    // outline of its own. CorrelationBias and FloorHandover keep the profile's values. 0..1; 0 is
    // the 0.3.3 profile (etched textures in RE Requiem). Not read while the profile is off.
    CustomOptional<float> FfxDenoiserProfileTextureRoute { 1.0f };

    // AMDNR 0.3.4, path-traced profile only: whether the profile also sets its six temporal values. true = the whole
    // profile, as in 0.3.3.2; false = the profile keeps its routing half and the temporal keys keep their values
    // without the profile. Not read while the profile is off. Saved like the profile's keys
    // (value_for_config_ignore_default).
    CustomOptional<bool> FfxDenoiserPathTracedTemporal { true };
    // AMDNR 0.3.4, experimental, off: store RR's demodulation albedo as FP16 instead of the quantized format. Takes
    // effect at feature (re)create. Read at startup, not saved.
    CustomOptional<bool> FfxDenoiserAlbedoFp16 { false };
    // AMDNR 0.3.4, experimental, off: NGX-direct Ray Regeneration takes its camera constants from Streamline, matched
    // by jitter; inert when nothing matches. Read at startup, not saved.
    CustomOptional<bool> FfxDenoiserNgxDirectSLConstants { false };

    // Skin smoothing (SSS guide) - AMDNR, experimental, off by default; on by quirk for RE Requiem
    // (GameQuirk::SkinSmoothingDefault), which also sets radius 16 and guide thresholds
    // 0.0265 / 0.0414 there - each only while the key has no value. For titles that publish
    // DLSSD.ScreenSpaceSubsurfaceScatteringGuide: after RR's composition and before FSR SR, RR's
    // demodulated diffuse on the pixels the guide marks as skin is filtered with an edge-aware
    // a-trous kernel of the given reach, and only the change is added back (x diffuse albedo x skin
    // weight x strength) - specular, skip and emissive are untouched. Pixels the guide does not mark,
    // and every title without the guide, are untouched; with both switches off nothing runs and the
    // guide is not read. Strength 0..1, radius in render pixels 1..16. ShowMask replaces the picture
    // with the skin weight (a check of what the guide marks). GuideLow / GuideHigh map the guide's
    // |guide| / luminance to the skin weight (smoothstep; High above Low, 0..10); set them from the
    // [RR_GUIDES] sss-range log line. All apply live.
    // Classifier: 1 (default) = robust - the guide weight above times three cues: the share of the
    // pixel the SSS pass moved (against the title's colour, mapped by MovedLow / MovedHigh,
    // smoothstep; High above Low, 0..1), how much of the surface around it shows that (held from the
    // previous frame, so faces do not flicker), and skin-toned diffuse albedo. It never marks a pixel
    // the guide weight alone leaves alone; RE Requiem's guide is non-zero on almost every material,
    // and the guide weight alone smoothed whole night streets (25-39 % of the frame, robust 0.5-2.3 %
    // in the same scenes). 0 = the guide weight alone (0.3.3), for comparison. ShowCues makes the
    // mask view draw the robust classifier's three cues instead of the weight.
    CustomOptional<bool> FfxDenoiserSkinSmoothing { false };
    CustomOptional<float> FfxDenoiserSkinSmoothingStrength { 1.0f };
    CustomOptional<int> FfxDenoiserSkinSmoothingRadius { 6 };
    CustomOptional<bool> FfxDenoiserSkinSmoothingShowMask { false };
    CustomOptional<float> FfxDenoiserSkinSmoothingGuideLow { 0.004f };
    CustomOptional<float> FfxDenoiserSkinSmoothingGuideHigh { 0.04f };
    CustomOptional<int> FfxDenoiserSkinSmoothingClassifier { 1 };
    CustomOptional<float> FfxDenoiserSkinSmoothingMovedLow { 0.05f };
    CustomOptional<float> FfxDenoiserSkinSmoothingMovedHigh { 0.12f };
    CustomOptional<bool> FfxDenoiserSkinSmoothingShowCues { false };

    CustomOptional<float> FfxDenoiserDisocThreshold { 0.1f };
    CustomOptional<float> FfxDenoiserCrossBlNormStr { 0.5f };
    CustomOptional<float> FfxDenoiserStabilityBias { 0.5f };
    CustomOptional<float> FfxDenoiserMaxRadiance { 4e4f };
    CustomOptional<float> FfxDenoiserRadianceClip { 40.0f };
    CustomOptional<float> FfxDenoiserGaussKernRelax { 0.5f };
    CustomOptional<float> FfxDenoiserDebugDepthMax { 1024.0f };

    // Records the probe readbacks: seven render targets per input probe interval plus two per
    // denoiser output probe, and the log lines that report them. Off by default - the numbers
    // are for diagnosis, and an always-on probe wrote a gigabyte of log in a session.
    CustomOptional<bool> FfxDenoiserDiagnostics { false };

    CustomOptional<float> FfxDenoiserCorrelationBias { 1.0f };
    // Binds the title's diffuse ray length into the diffuse signal's alpha. Without
    // it that alpha is a constant FP16-max "ray miss", which is what RR's non-PSR
    // reflection handling reads. No effect when the title provides no such resource.
    CustomOptional<bool> FfxDenoiserDiffuseHitDistance { true };
    CustomOptional<float> FfxDenoiserFloorIsolation { 1.0f };
    CustomOptional<float> FfxDenoiserRoughnessFloor { 0.1f };
    // Hands exact-zero-roughness (type-1) pixels to the spatial floor instead of RR.
    // Which pixels take the floor handover - the graft that recombines RR's low frequencies
    // with the floor's high frequencies in composition. Because that combination happens
    // after denoising it never removes anything from the denoiser's input, which is what
    // makes it safe to widen beyond the zero-roughness pixels it was written for.
    // 0 = off, 1 = exact-zero-roughness (type-1) pixels only, 2 = every pixel.
    CustomOptional<int> FfxDenoiserFloorHandover { 1 };
    // Scales the graft weight so the handover can be applied partially. 1.0 is the full
    // handover and 0.0 is inert.
    CustomOptional<float> FfxDenoiserFloorHandoverStrength { 1.0f };

    // Scales the raw-preserving blend inside the floor. 1.0 keeps the floor's microcontrast
    // where the guide allows it; 0.0 leaves a pure spatial floor, which is also how the
    // blend's contribution to the image can be removed outright for comparison.
    CustomOptional<float> FfxDenoiserFloorRawBlend { 1.0f };

    // Floor on the albedo used as the demodulation divisor. The floor caps the gain on
    // dark surfaces, but everything it cannot represent is handed to the skip signal, which
    // reaches the screen without passing the denoiser - so a high floor trades amplified
    // noise inside the denoiser for unfiltered noise beside it. Lowering it keeps the
    // demodulate/remodulate round trip faithful.
    CustomOptional<float> FfxDenoiserDemodDivisorFloor { 8e-3f };

    // Opt-in energy guard on the floor/raw clamp: the floor may not exceed the raw, so the
    // share that does is replaced with a low pass of the raw. The replacement is the raw's own
    // noise and the skip signal publishes it unfiltered, so enabling this republishes the raw's
    // grain on exactly the pixels it clamps; averaging the ceiling attenuates that but cannot
    // remove it. 0 - the default - leaves the floor unclamped and closes the residual instead.
    CustomOptional<float> FfxDenoiserFloorClampSmoothing { 0.0f };

    // How far the diffuse albedo's local structure suppresses that blend. The blend's test is
    // whether the raw sample resembles the floor, which on a noisy input the noise itself
    // answers, so flat surfaces pass raw grain through while the floor stays smooth. Where
    // albedo shows structure the raw sample carries real detail and is kept; where it does not,
    // the variation is not material and is refused. 0.0 reproduces the ungated behaviour.
    CustomOptional<float> FfxDenoiserFloorStructureGate { 1.0f };
    // Blends that handover between FloorSeed's isotropic floor (0), which erases thin
    // structure, and a directional hybrid median that preserves panel text (1).
    CustomOptional<float> FfxDenoiserFloorHandoverDetail { 1.0f };
    // Band-split only. Replaces the single mid band with the floor chain's own five
    // a-trous detail levels, each shrunk against its own threshold. The chain is
    // already this decomposition, so the mid band is a one-boundary approximation of
    // a five-boundary split that exists either way.
    // Handover refinements, each inert at zero and composing with the blend mode
    // rather than replacing it.
    CustomOptional<float> FfxDenoiserFloorHandoverAnchorClamp { 2.0f };
    CustomOptional<float> FfxDenoiserFloorHandoverCorrelationMix { 1.0f };
    // Per-level thresholds, finest (a-trous stride 1) first, in the same multiples of
    // the floor's local spread as the thresholds above. Noise is broadband and panel
    // structure is not, so the noise-to-signal ratio is worst at the finest level -
    // hence the descending defaults.

    CustomOptional<bool> Fsr4EnableWatermark { false };
    CustomOptional<bool> Fsr4DoNotLoadAmdxc64 { false };

    // FSR Common
    CustomOptional<float> FsrVerticalFov { 60.0f };
    CustomOptional<float> FsrHorizontalFov { 0.0f }; // off by default
    CustomOptional<float> FsrCameraNear { 0.1f };
    CustomOptional<float> FsrCameraFar { 100000.0f };
    CustomOptional<bool> FsrUseFsrInputValues { true };

    // dx11wdx12
    CustomOptional<bool> Dx11DelayedInit { false };
    CustomOptional<bool> DontUseNTShared { true };

    // vulkanwdx12
    CustomOptional<bool> VulkanUseCopyForInputs { false };
    CustomOptional<bool> VulkanUseCopyForOutput { false };

    // NVAPI Override
    CustomOptional<bool> DisableFlipMetering { false };

    // Spoofing
    CustomOptional<bool, SoftDefault> DxgiSpoofing { true };
    CustomOptional<bool> DxgiFactoryWrapping { false };
    CustomOptional<bool> StreamlineSpoofing { true };
    CustomOptional<std::string, NoDefault> DxgiBlacklist; // disabled by default
    CustomOptional<int, NoDefault> DxgiVRAM;              // disabled by default
    CustomOptional<bool> VulkanSpoofing { false };
    CustomOptional<bool> VulkanExtensionSpoofing { false };
    CustomOptional<int, NoDefault> VulkanVRAM; // disabled by default
    CustomOptional<bool> SpoofHAGS { false };
    CustomOptional<bool> SpoofFeatureLevel { false };
    CustomOptional<uint32_t> SpoofedVendorId { VendorId::Nvidia };
    CustomOptional<uint32_t> SpoofedDeviceId { 0x2684 };
    CustomOptional<uint32_t, NoDefault> TargetVendorId;
    CustomOptional<uint32_t, NoDefault> TargetDeviceId;
    CustomOptional<std::wstring> SpoofedGPUName { L"NVIDIA GeForce RTX 4090" };
    CustomOptional<bool> UESpoofIntelAtomics64 { false };
    CustomOptional<bool> SpoofRegistry { false };
    CustomOptional<bool> SpoofUser32 { false };
    CustomOptional<std::wstring> SpoofedDriver { L"32.0.15.9155" };

    // Plugins
    CustomOptional<std::wstring, NoDefault> PluginPath;
    CustomOptional<bool> LoadSpecialK { false };
    CustomOptional<bool> LoadReShade { false };
    CustomOptional<bool> LoadCustomAmdxc64OnRdna2 { false };
    CustomOptional<bool> LoadAsiPlugins { false };
    CustomOptional<int> LateAsiPluginsDelay { 30 };

    // Frame Generation
    CustomOptional<FGInput> FGInput { FGInput::NoFG };
    CustomOptional<bool> ExternalFrameGeneration { false };
    CustomOptional<FGOutput> FGOutput { FGOutput::NoFG };
    CustomOptional<FGNvngxReplacement> FGNvngxReplacement { FGNvngxReplacement::None };
    CustomOptional<bool> FGDrawUIOverFG { false };
    // A title that loads its own libxess_fg.dll (native XeSS FG) gets OptiScaler's XeFG output
    // switched off: two frame-generation swapchains on one window is a DX12 Error 0x80070057 at
    // launch (AC Black Flag Resynced). True forces the old behaviour.
    CustomOptional<bool> FGAllowXeFGWithNativeXeFG { false };
    CustomOptional<bool> FGUIPremultipliedAlpha { true };
    CustomOptional<bool> FGDisableHudless { false };
    CustomOptional<bool> FGDisableUI { false };
    CustomOptional<bool> FGSkipReset { false };
    CustomOptional<int> FGAllowedFrameAhead { 1 };
    CustomOptional<bool> FGDepthValidNow { false };
    CustomOptional<bool> FGVelocityValidNow { false };
    CustomOptional<bool> FGHudlessValidNow { false };
    CustomOptional<bool> FGOnlyAcceptFirstHudless { false };
    CustomOptional<bool> FGPreserveSwapChain { true };
    CustomOptional<bool> FGSkipResizeBuffers { false };
    CustomOptional<bool> FGModifyBufferState { false };
    CustomOptional<bool> FGModifySCIndex { false };
    CustomOptional<float> FGHudCutoff { 0.0f };
    CustomOptional<FrameTimeSource> FTInput { FrameTimeSource::Input };

    // OptiFG
    CustomOptional<bool> FGEnabled { false };
    CustomOptional<bool> FGUseMutexForSwapchain { true };
    CustomOptional<bool> FGMakeMVCopy { true };
    CustomOptional<bool> FGMakeDepthCopy { true };
    CustomOptional<bool> FGResourceFlip { false };
    CustomOptional<bool> FGResourceFlipOffset { false };
    CustomOptional<bool> FGAlwaysCaptureFSRFGSwapchain { false };

    CustomOptional<int, NoDefault> FGRectLeft;
    CustomOptional<int, NoDefault> FGRectTop;
    CustomOptional<int, NoDefault> FGRectWidth;
    CustomOptional<int, NoDefault> FGRectHeight;

    // OptiFG - Hudfix
    CustomOptional<bool> FGDisableHUDFix { false };
    CustomOptional<bool> FGHUDFix { false };
    CustomOptional<int> FGHUDLimit { 1 };
    CustomOptional<bool> FGHUDFixExtended { false };
    CustomOptional<bool> FGImmediateCapture { false };
    CustomOptional<bool> FGDontUseSwapchainBuffers { false };
    CustomOptional<bool> FGRelaxedResolutionCheck { false };
    CustomOptional<bool> FGHudfixDisableRTV { false };
    CustomOptional<bool> FGHudfixDisableSRV { false };
    CustomOptional<bool> FGHudfixDisableUAV { false };
    CustomOptional<bool> FGHudfixDisableOM { false };
    CustomOptional<bool> FGHudfixDisableDispatch { false };
    CustomOptional<bool> FGHudfixDisableDI { false };
    CustomOptional<bool> FGHudfixDisableDII { false };
    CustomOptional<bool> FGHudfixDisableSCR { true };
    CustomOptional<bool> FGHudfixDisableSGR { true };

    // OptiFG - Resource Tracking
    CustomOptional<bool> FGAlwaysTrackHeaps { false };
    CustomOptional<bool> FGResourceBlocking { false };
    CustomOptional<bool> FGUseShards { false };

    // OptiFG - DLSS-D Depth scale
    CustomOptional<bool> FGEnableDepthScale { false };
    CustomOptional<float> FGDepthScaleMax { 10000.0f };

    // FSR-FG
    CustomOptional<bool> FGDebugView { false };
    CustomOptional<bool> FGDebugResetLines { false };
    CustomOptional<bool> FGDebugTearLines { false };
    CustomOptional<bool> FGDebugPacingLines { false };
    CustomOptional<bool> FGAsync { false };
    CustomOptional<bool> FGFramePacingTuning { true };
    CustomOptional<float> FGFPTSafetyMarginInMs { 0.01f };
    CustomOptional<float> FGFPTVarianceFactor { 0.3f };
    CustomOptional<bool> FGFPTAllowHybridSpin { false };
    CustomOptional<int> FGFPTHybridSpinTime { 2 };
    CustomOptional<bool> FGFPTAllowWaitForSingleObjectOnFence { false };

    CustomOptional<bool> FSRFGSkipConfigForHudless { false };
    CustomOptional<bool> FSRFGSkipDispatchForHudless { false };
    CustomOptional<bool> FSRFGEnableWatermark { false };

    // XeFG
    // The ceiling for the whole XeFG path, in interpolated frames: 9 is 10X, opt-in (the
    // default below stays 5 = 6X). One number decides what the unlock may write into the
    // provider, what the ini may ask for and what the menu offers. The provider takes 9
    // (SetNumInterpolatedFrames(10) is refused), XeFGPacing paces bursts up to 9 (a
    // static_assert in XeFGUnlock.h holds the two together), and above 6X the unlock writes
    // it only into a copy the pacing engine runs on. 10X: opt-in, needs a 360 Hz+ display and
    // a frame cap; +128 MiB of VRAM at 4K over 6X. Offered in this build so it can be tested in
    // a game before release: so far only create/init/SetNum have run at 9, and 7 (8X, the
    // ceiling earlier builds shipped) is the way back if that test fails. The menu's MFG list
    // has one entry per count up to this (static_assert in menu_common.cpp).
    static constexpr int32_t XeFGMaxInterpolations = 9;
    CustomOptional<bool> FGXeFGIgnoreInitChecks { false };
    // AMDNR: lets XeFG run with the game's dilated / display-resolution motion vectors (Intel's
    // high-res MV mode, XEFG_SWAPCHAIN_INIT_FLAG_HIGH_RES_MV, which XeFG\HighResMV already sets).
    // Skips only the motion-vector check, in the Frame Gen tab and in XeFG_Dx12::Activate; the
    // fullscreen and HDR checks still apply (IgnoreInitChecks skips all of them). Experimental.
    CustomOptional<bool> FGXeFGAllowDilatedMV { false };
    CustomOptional<int> FGXeFGInterpolationCount { 1 };
    // Built-in XeFG multi-frame unlock (proxies/XeFGUnlock.h): five verified byte
    // patches on the mapped libxess_fg.dll so it reports MFG on non-Intel cards.
    // Replaces the third-party XeFGUnlock.asi, which the plugin loader now skips.
    CustomOptional<bool> FGXeFGUnlockEnabled { true };
    // The game's OWN provider copy is patched only when its build table was confirmed in a
    // game; a table derived offline (1.3.1.68) is left alone unless this is true.
    CustomOptional<bool> FGXeFGUnlockUnverified { false };
    // What the unlock reports as the provider's maximum, in interpolated frames: 5 = 6X by
    // default (3 = 4X is what the testers' plugin reported); up to XeFGMaxInterpolations
    // (9 = 10X) as an opt-in. The provider sizes its buffers from this patched maximum at
    // init, whatever multiplier then runs: about +32 MiB of VRAM per step at 4K, +15 at
    // 1440p, +8.5 at 1080p (10X is +128 MiB over 6X at 4K). Above 5 it applies only to
    // OptiScaler's own copy with XeFG pacing installed; a game's own copy, or ours without
    // the pacing, stays at 6X. A failed init at a high value leaves XeFG off on that
    // swapchain (logged, and shown in the Frame Gen tab), and 5 is the way back. Read when
    // the provider loads: a change needs a restart.
    CustomOptional<int> FGXeFGMaxInterpolatedFrames { 5 };
    // Coldwood1026's pacing engine for 3X and above (proxies/XeFGPacing.h): routes every
    // generated frame through the provider's own scheduler so a burst is spaced evenly
    // instead of clumped, and hands the provider a render time with the burst's own
    // blocking taken out (the feedback loop behind the latency and the periodic drops).
    // Installed on the proxy's own 1.3.1.78 copy only, at load; a change needs a restart.
    CustomOptional<bool> FGXeFGExtraPacing { true };
    // AMDNR 0.3.3 (MFG stutter, experimental): before each paced generated frame, wait -
    // never past that frame's deadline - until XeFG's real swapchain has room (its frame
    // latency waitable object), so the swapchain's queue is paced instead of the frames
    // clumping behind one blocking present. Needs ExtraPacing. Read when the XeFG swapchain
    // is created (turning it on needs a restart; turning it off applies at once). Off by
    // default: nothing is queried or waited on unless this is true.
    CustomOptional<bool> FGXeFGPaceOnSwapchain { false };
    CustomOptional<bool> FGXeFGUIComposition { false };
    CustomOptional<bool> FGXeFGDepthInverted { true };
    CustomOptional<bool> FGXeFGJitteredMV { false };
    CustomOptional<bool> FGXeFGHighResMV { false };
    CustomOptional<bool> FGXeFGDebugView { false };
    CustomOptional<bool> FGXeFGForceBorderless { false };

    // DLSSG
    CustomOptional<int> FGDLSSGInterpolationCount { 1 }; // For Opti's own SL instance
    CustomOptional<bool> FGDLSSGUseGamesReflexMarkers { true };
    CustomOptional<int, NoDefault>
        FGDLSSGOverrideInterpolationCount; // For overriding game's value sent to SL, could be Nvngx FG, could be noFG
                                           // but someone just uses real DLSSG
    CustomOptional<bool> FGDLSSGOverrideForceDMFG { false };   // Overrides game's DLSSG mode to Dynamic
    CustomOptional<bool> FGDLSSGForceDMFG { false };           // Overrides Opti's DLSSG mode to Dynamic
    CustomOptional<float> FGDLSSGFramerateTargetDMFG { 0.0f }; // 0.0 means auto-detects the display refresh rate

    // As per
    // https://github.com/artur-graniszewski/dlss-enabler-main/blob/a92464d468eb0d91ae17befa66c6bf6229f20b9f/Utils/DlssgProxy.cpp#L1033
    CustomOptional<uint32_t> NvngxFGDispatchFlags { 0x10000000 }; // IGNORE_UI_TEXTURE
    CustomOptional<bool> NvngxFGShowDebug { false };
    CustomOptional<bool> NvngxFGDisableHudless { false };

    // fakenvapi
    CustomOptional<bool> UseFakenvapi { true };
    CustomOptional<bool> ForceXeLL { false };
    CustomOptional<bool> FN_ForceLatencyFlex { false };
    CustomOptional<LFXMode> FN_LatencyFlexMode { LFXMode::Conservative };
    CustomOptional<ForceReflex> FN_ForceReflex { ForceReflex::InGame };
    CustomOptional<LowLatencyInput> LowLatencyInput { LowLatencyInput::Auto }; // TODO: no reading/saving to config
    CustomOptional<LowLatencyMode> LowLatencyOutput { LowLatencyMode::Auto };

    // Inputs
    CustomOptional<bool> EnableDlssInputs { true };
    CustomOptional<bool> EnableXeSSInputs { true };
    CustomOptional<bool> UseFsr2Inputs { true };
    CustomOptional<bool> UseFsr2Dx11Inputs { false };
    CustomOptional<bool> UseFsr2VulkanInputs { false };
    CustomOptional<bool> Fsr2Pattern { false };
    CustomOptional<bool> UseFsr3Inputs { true };
    CustomOptional<bool> Fsr3Pattern { false };
    CustomOptional<bool> UseFfxInputs { true };
    CustomOptional<bool> EnableHotSwapping { false };
    CustomOptional<bool> EnableFsr2Inputs { true };
    CustomOptional<bool> EnableFsr3Inputs { true };
    CustomOptional<bool> EnableFfxInputs { true };

    // Framerate
    CustomOptional<float> FramerateLimit { 0.0f };

    // HDR
    CustomOptional<bool> ForceHDR { false };
    CustomOptional<bool> UseHDR10 { false };
    CustomOptional<bool> SkipColorSpace { false };

    // V-Sync
    CustomOptional<bool> OverrideVsync { false };
    CustomOptional<bool, NoDefault> ForceVsync;
    CustomOptional<UINT> VsyncInterval { 0 };

    // Old configs for compat reasons
    CustomOptional<bool, NoDefault> _DONTUSE_Fsr4ForceEnableInt8;

    bool LoadFromPath(const wchar_t* InPath);
    bool SaveIni();
    bool SaveXeFG();

    void CheckUpscalerFiles();

    std::vector<std::string> GetConfigLog();

    static Config* Instance();

  private:
    inline static Config* _config;
    inline static std::vector<std::string> _log;

    std::filesystem::path absoluteFileName;
    std::wstring fileName = L"OptiScaler.ini";

    bool Reload(std::filesystem::path iniPath);

    std::optional<std::string> readString(std::string section, std::string key, bool lowercase = false);
    std::optional<std::wstring> readWString(std::string section, std::string key, bool lowercase = false);
    std::optional<float> readFloat(std::string section, std::string key);
    std::optional<int> readInt(std::string section, std::string key);
    std::optional<uint32_t> readUInt(std::string section, std::string key);
    std::optional<bool> readBool(std::string section, std::string key);

    template <typename Enum> std::optional<Enum> readEnum(std::string section, std::string key);
};
