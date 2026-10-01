// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
// moved from dlssnr/DlssNr_Menu.cpp
#include "pch.h"
#include "NeuralUi.h"

#include <dlssnr/amd/AmdBridge.h>
#include <dlssnr/amd/PresentExperimental.h> // AmdFinalImage (Final image mode)
#include <dlssnr/lmxxf/LmxxfBackend.h>      // RuntimePath (the runtime DLL's version), LateSubmitNrShare
#include <dlssnr/lmxxf/LmxxfTierPolicy.h>   // PlanLmxxfSize: the Preset hover's lmxxf tiers (pure, header-only)
#include <misc/IdentifyGpu.h>
#include <Util.h>
#include <Config.h>
#include <menu/menu_common.h>
#include <shaders/fsrd_preprocess/FSRDRuntimeStatus.h> // RayRegenOffForSession (attention offer 9)

#include <imgui/imgui.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

// The top of the Neural tab (0.3.4 W2-M2a; menu plan section 4 A1-A7), no heading. (0.3.4 menu rework) Drawn as the
// owner-approved mock (the owner-approved menu mock, T1-T6 of SPEC-content.md):
//   A1 [x] Enable Neural Rendering [Home]                               DrawTop
//   A2 [lmxxf v] Neural runtime   running (dim tag; orange only when the player must act)   DrawTop
//   A3 the credit, one dim line (the author's part opens their page)   DrawTop
//   A4 Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms    DrawTop (was the Live section, M:695-836)
//   A4b > Live (a tree, closed by default; owner decision 3): rates and cadence, working size / dropped frames /
//       network ms, Streamline slInit and the runtime's version, the late-submission share   DrawTop
//   A5 the attention slot: at most one orange line, the rest in (+N)  DrawAttention
//   A6 [Quality][Balanced][Performance] Preset                          DrawPresetAndStyle
//      (+ [Handheld] on an lmxxf handheld APU only, owner decision 1)
//   A7 [Default v] NR style [Style slots]                               DrawPresetAndStyle
// No "(?)": each row's help opens on hovering its label (NeuralUi Help). With no runtime files on a non-NVIDIA GPU
// only A1 and the missing-runtime card are drawn. DrawPlacement and DrawLive draw nothing any more (the dispatcher
// still calls them): "AMD processing" moved into the status line's hover, and the Live section became the status
// line plus the attention slot. Every ini key and every write is 0.3.3.2's; only layout, labels, help and where a
// message is shown changed.
namespace DlssNr::NeuralUi
{

namespace
{
using NeuralRuntime = DlssNr::AmdBridge::NeuralRuntime;

// The orange of the attention slot (NeuralUi.cpp) and the menu's other warnings; a muted green for the Retry lmxxf
// confirmation (a state line the mock does not show). (0.3.4 menu rework) "running" is a dim tag now, as in the mock.
const ImVec4 kOrange(1.f, 0.55f, 0.2f, 1.f);
const ImVec4 kGreen(0.55f, 0.8f, 0.55f, 1.f);
// The status line's separator, the mock's " - " (0.3.4 W2 drew U+00B7).
const char* const kSep = " - ";

// ---- The mock's vertical rhythm (APPROVED_MOCK.png, measured with PIL) ------------------------------------------
// ImGui ends every row with the 6 px item spacing; the mock's two dim lines sit tight under the runtime row (cap tops
// 23 px under the combo's frame top) and 14 px apart, and the Preset buttons start 21 px under the second line's cap
// tops. (0.3.4 MENU match1) With the 14 px menu font (a text line is 14 + 6 px, the cap tops 2 px into it): -3 after
// the runtime row, -6 between the two dim lines (20 -> 14), +3 before Preset. Mock px, scaled by Menu Scale.
void NudgeY(float mockPx)
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::round(Px(mockPx)));
}

// A state word after a row's label that needs the player ("not installed", "restart the game to switch", "not for
// this GPU"): DimTag's place, in the menu's warning orange. Every other state word is DimTag.
void OrangeTag(const char* text)
{
    ImGui::SameLine(0.0f, std::floor(Px(16.0f))); // DimTag's gap
    ImGui::PushStyleColor(ImGuiCol_Text, kOrange);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

// NR STYLES (user request, 0.3.1). A style is a set of values for the appearance controls -
// Residual strength and limit, Temporal stability, Sharpening, Detail and Colour strength, Tone
// and Structure intensity, and lmxxf's Edit detail / Edit colour / Edge guard / Output smoothing.
// Five named styles are starting points, not measurements; three custom slots keep the user's
// own as "key=value;..." strings in the ini ([DlssNr] StyleSlot1..3). The combo reads "Custom"
// whenever the sliders match no named style, so it tells the truth after any manual change.
// The colour composition (AmdComposition, AmdComposeDetail / AmdComposeColour, and the guard and
// skin keys the RenoDX mode shares with the NVIDIA path) is deliberately not a style field: a slot
// captured in one mode would mean something else recalled in the other. The menu says so.
struct StyleField
{
    const char* key;
    CustomOptional<float> Config::*member;
};
static constexpr int kStyleCount = 12;
static const StyleField kStyleFields[kStyleCount] = {
    { "residual", &Config::AmdResidualIntensity },     { "limit", &Config::AmdResidualLimit },
    { "stability", &Config::AmdTemporalStability },    { "sharpen", &Config::AmdSharpness },
    { "detail", &Config::AmdDetailStrength },          { "colour", &Config::AmdColourStrength },
    { "tone", &Config::AmdNeuralLightingStrength },    { "structure", &Config::DlssNrLocalStructure },
    { "editdetail", &Config::AmdLmxxfEditDetail },     { "editcolour", &Config::AmdLmxxfEditSaturation },
    { "edgeguard", &Config::AmdLmxxfEdgeGuard },       { "smooth", &Config::AmdLmxxfOutputSmooth },
};
struct StylePreset
{
    const char* name;
    float v[kStyleCount];
};
//                 residual limit  stab   sharp  detail colour tone   struct editD  editC  edge   smooth
static const StylePreset kStylePresets[] = {
    { "Default",   { 1.00f, 0.25f, 0.60f, 0.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.50f, 0.60f } },
    { "Cinematic", { 0.85f, 0.20f, 0.75f, 0.00f, 1.00f, 1.00f, 1.15f, 0.90f, 0.85f, 0.90f, 0.60f, 0.80f } },
    { "Crisp",     { 1.20f, 0.35f, 0.50f, 0.30f, 1.00f, 1.00f, 1.00f, 1.25f, 1.30f, 1.00f, 0.40f, 0.40f } },
    { "Natural",   { 0.70f, 0.15f, 0.70f, 0.00f, 1.00f, 1.00f, 0.90f, 0.90f, 0.75f, 0.75f, 0.70f, 0.70f } },
    { "Vivid",     { 1.10f, 0.30f, 0.60f, 0.15f, 1.00f, 1.00f, 1.25f, 1.10f, 1.10f, 1.40f, 0.50f, 0.50f } },
};
static constexpr int kStylePresetCount = 5;
// The hover of each style in the NR style combo (0.3.3.2 listed them in the combo's help).
static const char* const kStyleHelp[kStylePresetCount] = {
    "The shipped values.",
    "Softer edit, more stability and smoothing, a little more tone.",
    "Stronger edit and structure, some sharpening, less smoothing.",
    "A restrained edit with a wider edge guard.",
    "Stronger colour and tone.",
};
// (0.3.4, plan A7) The runtime capability each style field belongs to; Cap::Count = a field every runtime uses.
// MatchingStyle compares only the fields the menu's runtime uses (Yes / Always), so a named style reads as itself
// on both runtimes: 0.3.3.2 compared all 12 and read "Custom" once the other runtime's fields had moved.
// ApplyStyleValues still writes all 12, so a switch of runtime keeps the style honest.
static const RuntimeCaps::Cap kStyleFieldCap[kStyleCount] = {
    RuntimeCaps::Cap::Count,      RuntimeCaps::Cap::Count,     RuntimeCaps::Cap::Count,
    RuntimeCaps::Cap::Count,      RuntimeCaps::Cap::Count,     RuntimeCaps::Cap::Count,
    RuntimeCaps::Cap::Tone,       RuntimeCaps::Cap::Structure, RuntimeCaps::Cap::EditShaper,
    RuntimeCaps::Cap::EditShaper, RuntimeCaps::Cap::EditShaper, RuntimeCaps::Cap::OutputSmoothing,
};
static bool StyleFieldUsed(int i)
{
    const RuntimeCaps::Cap cap = kStyleFieldCap[i];
    if (cap == RuntimeCaps::Cap::Count)
        return true;
    const RuntimeCaps::Support s = RuntimeCaps::Get(cap);
    return s == RuntimeCaps::Support::Yes || s == RuntimeCaps::Support::Always;
}
static void ApplyStyleValues(Config& c, const float* v)
{
    for (int i = 0; i < kStyleCount; ++i)
        (c.*kStyleFields[i].member) = v[i];
}
static int MatchingStyle(Config& c)
{
    for (int p = 0; p < kStylePresetCount; ++p)
    {
        bool all = true;
        for (int i = 0; i < kStyleCount && all; ++i)
            if (StyleFieldUsed(i))
                all = std::fabs((c.*kStyleFields[i].member).value_or_default() - kStylePresets[p].v[i]) < 0.005f;
        if (all)
            return p;
    }
    return -1;
}

// D1 (plan 10.1, owner OK): Preset writes only NR resolution and Dynamic NR off, so it no longer fights NR style.
// Revert: true restores 0.3.3.2's writes (Temporal stability, Sharpening, Detail and Colour strength as well).
constexpr bool kPresetWritesLookValues = false;
static std::string CaptureStyle(Config& c)
{
    std::string s;
    char buf[64];
    for (int i = 0; i < kStyleCount; ++i)
    {
        std::snprintf(buf, sizeof buf, "%s=%.4f;", kStyleFields[i].key, (c.*kStyleFields[i].member).value_or_default());
        s += buf;
    }
    return s;
}
static void ApplyStyleString(Config& c, const std::string& s)
{
    size_t at = 0;
    while (at < s.size())
    {
        const size_t end = s.find(';', at);
        const std::string item = s.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? s.size() : end + 1;
        const size_t eq = item.find('=');
        if (eq == std::string::npos)
            continue;
        const std::string key = item.substr(0, eq);
        const float v = std::strtof(item.c_str() + eq + 1, nullptr);
        if (!std::isfinite(v))
            continue;
        for (int i = 0; i < kStyleCount; ++i)
            if (key == kStyleFields[i].key)
                (c.*kStyleFields[i].member) = v;
    }
}

// ---- A4/A5 helpers (local to this file: NeuralUi.h is frozen) ----------------------------------------------

// The first sentence of a status text, at most `maxLen` characters (cut at a space, "..." appended): the line the
// attention slot shows (plan A5.7; the plan's StatusHeadline, kept menu-side over AmdBridge::Status()). The full
// text goes into the line's hover.
std::string Headline(const std::string& full, size_t maxLen = 90)
{
    size_t end = full.size();
    for (size_t i = 0; i < full.size(); ++i)
    {
        if (full[i] == '\n' || (full[i] == '.' && (i + 1 == full.size() || full[i + 1] == ' ')))
        {
            end = i;
            break;
        }
    }
    std::string s = full.substr(0, end);
    if (s.size() > maxLen)
    {
        size_t cut = s.rfind(' ', maxLen - 3);
        if (cut == std::string::npos || cut < maxLen / 2)
            cut = maxLen - 3;
        while (cut > 0 && (static_cast<unsigned char>(s[cut]) & 0xC0) == 0x80) // never inside a UTF-8 sequence
            --cut;
        s.erase(cut);
        s += "...";
    }
    return s;
}

// The hover of an attention line: the full text, or nothing when the line already says all of it.
std::string RestOf(const std::string& full, const std::string& line)
{
    return full == line ? std::string() : full;
}

void OpenUrl(const char* url)
{
    // As the header's Discord / GitHub buttons and the credits link (menu_common.cpp): the platform shell.
    auto& pio = ImGui::GetPlatformIO();
    if (pio.Platform_OpenInShellFn)
        pio.Platform_OpenInShellFn(ImGui::GetCurrentContext(), url);
}

const char* WithoutScheme(const char* url)
{
    return std::strncmp(url, "https://", 8) == 0 ? url + 8 : url;
}

// D3D11 GAMES REACH THIS PASS ONLY THROUGH THE w/Dx12 BACKENDS, and that is not
// guessable from the menu.
//
// The neural work is D3D12; a D3D11 game gets to it over OptiScaler's D3D11-on-D3D12
// bridge (IFeature_Dx11wDx12), which is the ONLY D3D11 path that calls into it - the
// native D3D11 features run entirely on the game's own device and never touch it.
//
// So a D3D11 player who ticks the box above and leaves the backend on "FSR 3.1" or
// "XeSS" gets a working upscaler, no neural rendering at all, and nothing anywhere
// telling them why. That is the same silent-refusal shape as the NR resolution cost
// and the missing denoiser DLL, and it is worth one line to close.
// Vulkan is the same shape: IFeature_VkwDx12 makes the identical hand-off, the native
// Vulkan backends never do. One hint serves both. (0.3.4: the line is attention item 5; the NVIDIA layout keeps
// 0.3.3.2's line, see DrawTop.)
struct ApiHint
{
    bool show = false;
    std::string line, hover;
};
// withoutFeature: also before the game has created an upscaler (0.3.3.2 said it then too; the NVIDIA layout keeps
// that). The attention slot passes false (G2 review): with the upscaler unset in the ini, FSR 3.X/4 w/Dx12 is picked
// when the feature is created, so on a title screen the line was a false alarm that took the slot.
ApiHint ApiHintFor(bool enabled, bool withoutFeature)
{
    ApiHint h;
    if (!enabled || (State::Instance().api != DX11 && State::Instance().api != Vulkan))
        return h;
    const bool vulkan = State::Instance().api == Vulkan;
    IFeature* const feature = State::Instance().currentFeature; // read once: another thread may reset it
    if (feature == nullptr && !withoutFeature)
        return h;
    const auto backend = feature != nullptr ? feature->GetUpscalerType() : Upscaler::Reset;
    const bool viaDx12 = backend == Upscaler::XeSS_on12 || backend == Upscaler::FSR21_on12 ||
                         backend == Upscaler::FSR22_on12 || backend == Upscaler::FFX_on12 ||
                         backend == Upscaler::DLSS_on12;
    if (viaDx12)
        return h;
    // Named the way the Upscaling list names it: "FSR 3.X/4 w/Dx12" ("FSR 3.X w/Dx12" on a GPU
    // without FSR 4), the backend picked automatically when the ini leaves the upscaler unset.
    // The old text sent Vulkan players to "XeSS w/Dx12" and "DLSS w/Dx12", which the Vulkan list
    // does not have; DLSS w/Dx12 needs an NVIDIA GPU on D3D11 as well. (G2) On a non-NVIDIA GPU with NR on the
    // list ends these items in " - Neural" (MenuCommon::RenderUpscalerCombo), so the line names it that way too.
    const bool tagged = IdentifyGpu::getPrimaryGpu().vendorId != VendorId::Nvidia;
    const std::string ffx12 =
        UpscalerDisplayName(Upscaler::FFX_on12, State::Instance().api) + (tagged ? " - Neural" : "");
    h.show = true;
    h.line = std::string(vulkan ? "Vulkan" : "D3D11") + " game: pick " + ffx12 + " in Upscaling, or Neural does nothing";
    // R4 (G2 review): at most 4 lines.
    h.hover = "The neural model runs on D3D12; a D3D11 or Vulkan game reaches it only through a \"w/Dx12\" upscaler."
              " Pick " +
              ffx12 + (vulkan ? " (or FSR 2.1.2 w/Dx12)" : " (or XeSS w/Dx12, FSR 2.1.2 / 2.2.1 w/Dx12)") +
              " in Upscaling and press Change Upscaler. Left unset, it is picked for you while NR is on.";
    return h;
}

// What the top block and the attention slot read, worked out once per ImGui frame (DrawTop and DrawAttention both
// ask). Rates, the idle timer and the status debounce advance here, once per frame, as they did in the Live
// section.
struct TopFrame
{
    int frame = -1;
    bool anyRuntime = false;
    NeuralRuntime active = NeuralRuntime::Unchosen; // the backend the process built (Unchosen = none yet)
    // The runtime the combo shows: the choice ([DlssNr] NrBackend); with no choice, the one built or, before that,
    // the one the bridge will build (danielblnc when its files are there, else lmxxf). 0.3.3.2 showed danielblnc
    // for no choice, so the lmxxf-only package read "danielblnc" and, once lmxxf ran, "restart to switch".
    NeuralRuntime shown = NeuralRuntime::Daniel;
    bool shownInstalled = false;
    bool hipMissing = false; // lmxxf shown, danielblnc built because HIP failed (0.3.3.2's HIP line)
    bool vkHeld = false;     // lmxxf held back on this Vulkan game by lmxxf_vk_launch.pending (0.3.3.2 rule)
    bool restart = false;    // the saved choice differs from the built backend: it switches at the next start
    bool stopped = false;    // AmdBridge::RuntimeStopped()
    // (0.3.4, P1-host request 1) Why danielblnc's runtime was not started in this session although its files are
    // installed: the game's device and its queue's device could not be paired (AmdBridge::NrDeviceRefusal; empty
    // otherwise). No backend is built then, so RuntimeStopped() stays false.
    std::string refusal;
    // (0.3.4, P3) Late submission of the built runtime (Lmxxf::LateSubmitNrShare / AmdPreSr::LateSubmitNrShare): -1 =
    // nothing late in the last window of about 600 frames, else the share of that window's frames NR (danielblnc) or
    // the model (lmxxf) ran on, 0..1. Changes at most every ~10 s.
    float lateShare = -1.f;
    decltype(DlssNr::AmdBridge::Stats()) st {};
    bool rateValid = false;  // nrFps / modelFps were measured at least once since the backend's counters began
    std::string status;      // the backend's status when it is not the routine one, debounced (M:814-836)
    bool c32w = false;       // lmxxf's network runs AMDNR's RDNA 4 c32w kernels (the credit line's suffix)
    // (0.3.4 owner decision 2) Each runtime's version as the player's own files say it (RuntimeVersionOf; danielblnc's
    // credit line shows it), and the combo's "danielblnc 0.4.0" / "lmxxf 0.3.4" (the name alone while the version is
    // unknown or not a plain version number: see PlainVersion). Indexed by IsLmxxf.
    std::string version[2];
    std::string label[2];
    const std::string& Version(NeuralRuntime id) const { return version[id == NeuralRuntime::Lmxxf ? 1 : 0]; }
    const std::string& Label(NeuralRuntime id) const { return label[id == NeuralRuntime::Lmxxf ? 1 : 0]; }
};

// (0.3.4 owner decision 2) The version of the LmxxfNrRuntime.dll the bridge loads (Lmxxf::RuntimePath: beside
// OptiScaler.dll, else in DLSS5-AMD\), from its version resource: FILEVERSION 0,3,4,0 reads "0.3.4" (a fourth part only
// when it is not 0: "0.3.3.2"). Empty when the file carries no version (then the menu shows the name alone). Read once
// the file is found; looked for again every 5 s while it is missing, so the menu never stats it every frame. Menu
// thread only.
const std::string& LmxxfRuntimeVersion()
{
    static std::string version;
    static bool read = false;
    static double nextLook = -1.0;
    if (read)
        return version;
    const double now = ImGui::GetTime();
    if (nextLook >= 0.0 && now < nextLook)
        return version;
    nextLook = now + 5.0;
    std::error_code ec;
    const std::filesystem::path dll = Lmxxf::RuntimePath(Util::DllPath().parent_path());
    if (!std::filesystem::is_regular_file(dll, ec))
        return version;
    read = true;
    version_t v {};
    if (Util::GetFileVersion(dll.wstring(), &v) && (v.major | v.minor | v.patch | v.reserved) != 0)
    {
        char buf[48];
        if (v.reserved != 0)
            std::snprintf(buf, sizeof buf, "%u.%u.%u.%u", unsigned(v.major), unsigned(v.minor), unsigned(v.patch),
                          unsigned(v.reserved));
        else
            std::snprintf(buf, sizeof buf, "%u.%u.%u", unsigned(v.major), unsigned(v.minor), unsigned(v.patch));
        version = buf;
    }
    return version;
}

// A runtime's version from the player's own files: danielblnc's the build its pass DLL was identified as
// (AmdBridge::RuntimeName through RuntimeInfo::buildName: the loaded layout, else the file identified once; empty when
// the files are missing or the build is unknown), lmxxf's the runtime DLL's version resource. Never a fixed string.
std::string RuntimeVersionOf(const RuntimeCaps::RuntimeInfo& r)
{
    if (r.id == NeuralRuntime::Lmxxf)
        return LmxxfRuntimeVersion();
    const char* build = r.buildName != nullptr ? r.buildName() : nullptr;
    return build != nullptr ? std::string(build) : std::string();
}

// A version the combo, the status line's hover and the Live tree may name: digits and dots only ("0.4.0", "0.3.3.2").
// For a danielblnc build this host does not drive, RuntimeName is a sentence ("<version> (sha ...) - not driven by this
// build: use ..."); it stays on the credit line (0.3.3.2 showed it there) and the combo shows the name alone (owner
// decision 2: the name only when the version is unknown).
bool PlainVersion(const std::string& v)
{
    if (v.empty() || v.size() > 16 || !std::isdigit(static_cast<unsigned char>(v[0])))
        return false;
    for (const char c : v)
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.')
            return false;
    return true;
}

const TopFrame& Evaluate(Config& config)
{
    static TopFrame f;
    static bool rateValid = false;
    const int frame = ImGui::GetFrameCount();
    if (f.frame == frame)
        return f;
    f.frame = frame;
    NeuralState& ns = Shared();

    f.anyRuntime = DlssNr::AmdBridge::AnyRuntimePresent();
    f.active = DlssNr::AmdBridge::ActiveRuntime();
    const auto chosen = DlssNr::AmdBridge::ChosenRuntime();
    if (chosen != NeuralRuntime::Unchosen)
        f.shown = chosen;
    else if (f.active != NeuralRuntime::Unchosen)
        f.shown = f.active;
    else
        f.shown = DlssNr::AmdBridge::LmxxfReady() && !DlssNr::AmdBridge::HasFiles() ? NeuralRuntime::Lmxxf
                                                                                     : NeuralRuntime::Daniel;
    f.shownInstalled = RuntimeCaps::Row(f.shown).installed();
    // The versions once a second, not every frame: before danielblnc's backend loads, RuntimeName stats the pass DLL
    // on every call (the identification itself runs once).
    static double nextVersions = -1.0;
    if (const double t = ImGui::GetTime(); nextVersions < 0.0 || t >= nextVersions)
    {
        nextVersions = t + 1.0;
        for (const auto& r : RuntimeCaps::All())
        {
            const int i = r.id == NeuralRuntime::Lmxxf ? 1 : 0;
            f.version[i] = RuntimeVersionOf(r);
            f.label[i] = PlainVersion(f.version[i]) ? std::string(r.name) + " " + f.version[i] : std::string(r.name);
        }
    }

    // lmxxf held back on this Vulkan game by lmxxf_vk_launch.pending (AmdBridge.h,
    // LmxxfVkLaunchPending). Said wherever lmxxf would otherwise run: NrBackend=lmxxf, or the
    // main archive's lmxxf-only layout. There NrBackend stays unset and no backend gets built.
    // LmxxfWanted() answers the same question but logs, so the menu asks its parts. vkRetried keeps
    // the answer after Retry, since LmxxfVkLaunchPending() caches for about a second. (0.3.3.2's rule.)
    f.vkHeld = State::Instance().api == API::Vulkan && f.active != NeuralRuntime::Lmxxf &&
               DlssNr::AmdBridge::LmxxfReady() &&
               (chosen == NeuralRuntime::Lmxxf || !DlssNr::AmdBridge::HasFiles()) &&
               (ns.vkRetried || DlssNr::AmdBridge::LmxxfVkLaunchPending());

    // 0.3.3.2's order: a built backend that is not the shown runtime is explained by HIP, then the Vulkan hold,
    // then missing files; only otherwise is it a saved switch waiting for a restart. HipRuntimeVersion() is asked
    // only here, as in 0.3.3.2 (its first call loads HIP and runs hipInit on this thread).
    f.hipMissing = f.restart = false;
    if (f.active != NeuralRuntime::Unchosen && f.active != f.shown)
    {
        if (f.shown == NeuralRuntime::Lmxxf && DlssNr::AmdBridge::HipRuntimeVersion() < 0)
            f.hipMissing = true;
        else if (f.shown == NeuralRuntime::Lmxxf && f.vkHeld)
            ; // the attention slot, with Retry lmxxf
        else if (!f.shownInstalled)
            ; // the combo's "not installed"
        else
            f.restart = true;
    }
    f.stopped = DlssNr::AmdBridge::RuntimeStopped();
    f.refusal = DlssNr::AmdBridge::NrDeviceRefusal();
    f.lateShare = f.active == NeuralRuntime::Lmxxf    ? Lmxxf::LateSubmitNrShare()
                  : f.active == NeuralRuntime::Daniel ? AmdPreSr::LateSubmitNrShare()
                                                      : -1.f;

    // Rates are worked out here rather than in the backend, which only keeps monotonic counters. Averaged over
    // a rolling window so the numbers are readable instead of flickering, and held between updates so a slow
    // window does not blank the display. (0.3.3.2's Live readout.)
    f.st = DlssNr::AmdBridge::Stats();
    const auto& st = f.st;
    if (ns.liveLastTick == 0 || st.tick < ns.liveLastTick)
    {
        ns.liveLastTick = st.tick;
        ns.liveLastRec = st.recorded;
        ns.liveLastModel = st.modelFrames;
        ns.liveLastSkip = st.skips;
        rateValid = false;
    }
    else if (st.tick - ns.liveLastTick >= 500)
    {
        const float secs = float(st.tick - ns.liveLastTick) / 1000.f;
        ns.nrFps = float(st.recorded - ns.liveLastRec) / secs;
        ns.modelFps = float(st.modelFrames - ns.liveLastModel) / secs;
        ns.skipRate = st.skips - ns.liveLastSkip;
        ns.liveLastTick = st.tick;
        ns.liveLastRec = st.recorded;
        ns.liveLastModel = st.modelFrames;
        ns.liveLastSkip = st.skips;
        rateValid = true;
    }
    f.rateValid = rateValid;

    // NOTHING HAS REACHED THE PASS (0.3.3, design item 2): after 5 s with NR on and no recorded frame the status
    // line says "Idle" instead of leaving "Starting" to read as a fault.
    const double now = ImGui::GetTime();
    if (!config.DlssNrEnabled.value_or_default() || st.recorded != 0)
        ns.idleSince = -1.0;
    else if (ns.idleSince < 0.0)
        ns.idleSince = now;

    // The backend's own status, only when it is not the routine one: waiting, warming up, a refusal, a failure -
    // the things worth a line. Two rules keep it from flickering. The healthy heartbeat ("Completed AMD pre-SR
    // ...", whose counters change every frame) is never shown, and the counters suffix is cut off anything that
    // is. And a line has to hold for half a second before it appears or disappears, so a state that flips on
    // alternate frames cannot make the window height jump. (0.3.3.2's rules, M:814-836.)
    // (0.3.4, G2 review) The bridge's start text ("AMD pre-SR: waiting for the first upscaler frame", AmdBridge.cpp)
    // is routine too: only Run() replaces it, and Run() is not reached while NR is off or before the first upscaler
    // call, so it stood as the page's orange line in the default state. The status line says Off / Starting / Idle.
    {
        std::string status = DlssNr::AmdBridge::Status();

        // (0.3.4 menu rework, mock T3) AMDNR's RDNA 4 one-wave C32 kernels (lmxxf runtime, Network::C32wInit). Once its
        // network is built, lmxxf's status carries " c32w=on" or " c32w=off:<reason>"; until then gfx1201 stands in,
        // the one architecture C32wGate turns them on for by default. Latched: later texts (idle, a refusal, a status
        // too long for the word) leave it out, and the kernels do not change within a process.
        static int c32wSeen = -1;
        if (f.active == NeuralRuntime::Lmxxf)
        {
            if (status.find("c32w=on") != std::string::npos)
                c32wSeen = 1;
            else if (status.find("c32w=off") != std::string::npos)
                c32wSeen = 0;
        }
        const auto& gpu = DlssNr::AmdBridge::GpuSupportInfo();
        f.c32w = c32wSeen == 1 || (c32wSeen < 0 && gpu.lmxxfOk && gpu.target == "gfx1201");

        const bool routine = status.empty() || status.find("Completed AMD pre-SR") != std::string::npos ||
                             status.rfind("AMD isolated neural command list", 0) == 0 ||
                             status.rfind("AMD pre-SR: waiting for the first upscaler frame", 0) == 0 ||
                             status == "AMD pre-SR: idle" || status == "AMD pre-SR: busy";
        if (routine)
            status.clear();
        else if (const auto cut = status.find(" | completed frames="); cut != std::string::npos)
            status.erase(cut);
        std::string &pending = ns.statusPending, &shown = ns.statusShown;
        double &pendingSince = ns.statusPendingSince, &shownAt = ns.statusShownAt;
        if (status != pending)
        {
            pending = status;
            pendingSince = now;
        }
        else if (now - pendingSince >= 0.5)
        {
            shown = pending;
            shownAt = now;
        }
        if (now - shownAt > 2.0 && shown != pending)
            shown.clear(); // nothing stable for 2 s
        f.status = shown;
    }
    return f;
}

// One entry of the runtime combo: can it run here? (plan A2; absorbs 0.3.3.2's GPU line and file lines)
void RuntimeItemTooltip(const RuntimeCaps::RuntimeInfo& r)
{
    const auto& g = DlssNr::AmdBridge::GpuSupportInfo();
    const bool installed = r.installed();
    const std::string missing = r.missing();
    const bool gpuBad = g.amd && g.known && !r.gpuOk(g);
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
    if (!installed)
        ImGui::Text("Not installed: %s", missing.empty() ? "its files are missing" : missing.c_str());
    else if (gpuBad)
        ImGui::Text("This GPU cannot run it: %s", g.note.c_str());
    else if (!missing.empty())
        ImGui::Text("Installed, but %s", missing.c_str());
    else if (g.amd && g.known)
        ImGui::TextUnformatted("Installed, runs on this GPU");
    else
        ImGui::TextUnformatted("Installed");
    if (!g.name.empty())
        ImGui::TextDisabled("GPU: %s%s%s%s", g.name.c_str(), g.target.empty() ? "" : " (", g.target.c_str(),
                            g.target.empty() ? "" : ")");
    if (!g.known && !g.note.empty())
        ImGui::TextDisabled("%s", g.note.c_str());
    // lmxxf on RDNA 3 is AMDNR's own backend: say whose, wherever it can run (0.3.3.2's GPU line said it too)
    if (r.id == NeuralRuntime::Lmxxf && g.lmxxfOk && g.target.rfind("gfx11", 0) == 0)
        ImGui::TextDisabled("%s", "lmxxf on RDNA 3: AMDNR's RDNA 3 backend by 3zwr1");
    // (0.3.4 runtime work HB request 5) lmxxf on a 12+ CU handheld APU runs, but slow and untested on the chip
    // (AmdBridge GpuSupport::lmxxfExperimental); the one orange line of this tooltip.
    if (r.id == NeuralRuntime::Lmxxf && g.lmxxfExperimental)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, kOrange);
        ImGui::TextUnformatted("Experimental on this GPU: slow (estimate: 60-125 ms per network run on a 780M, 45-95 ms"
                               " on an 890M, at 360p). The NR cost below shows the real number.");
        ImGui::PopStyleColor();
    }
    if (!installed || gpuBad || !missing.empty())
        ImGui::TextDisabled("%s", "It can still be picked; it will not start until that is fixed.");
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// A2: the combo (174 px, the mock's), its help on the label and the state on the same row.
void DrawRuntimeRow(Config* config, const TopFrame& f)
{
    const RuntimeCaps::RuntimeInfo& shownRow = RuntimeCaps::Row(f.shown);
    // (0.3.4 owner decision 2) The preview and the items name the exact version of the player's files ("danielblnc
    // 0.4.0", "lmxxf 0.3.4"; the name alone while it is unknown). "###" keeps each item's ID on its ini value, so the
    // ID does not change when the version appears.
    if (BeginCombo("Neural runtime", f.Label(f.shown).c_str()))
    {
        for (const auto& r : RuntimeCaps::All())
        {
            const bool selected = r.id == f.shown;
            const std::string item = f.Label(r.id) + "###nrRuntime_" + r.iniValue;
            if (ImGui::Selectable(item.c_str(), selected))
            {
                // Written to the ini at once, like the first-launch chooser does: the choice only
                // takes effect on the next start, and a player who changed it here, restarted and
                // found the old runtime again (the "cannot switch to lmxxf" report) had not
                // pressed Save Settings. (0.3.3.2: a click on the shown entry writes it too.)
                config->DlssNrBackend = std::string(r.iniValue);
                config->SaveIni();
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                RuntimeItemTooltip(r);
        }
        ImGui::EndCombo();
    }
    Help("Which neural runtime runs the model. Every setting on this page works with both; what a runtime cannot"
         " use yet is greyed with a reason. Saved at once, used from the next game start. Hover an entry for its"
         " state here.",
         { "danielblnc: Daniel Blanco's runtime, shipped unmodified with his permission.",
           "lmxxf: Kien's open HIP runtime (MIT); its edit lands one frame late, so the frame never waits." },
         "[DlssNr] NrBackend");

    // The state (plan A2): at most one word or phrase, the mock's dim tag; orange only when something needs the
    // player (SPEC-content section 5).
    const auto& g = DlssNr::AmdBridge::GpuSupportInfo();
    const RuntimeCaps::RuntimeInfo& other =
        RuntimeCaps::Row(f.shown == NeuralRuntime::Lmxxf ? NeuralRuntime::Daniel : NeuralRuntime::Lmxxf);
    const bool shownGpuBad = g.amd && g.known && !shownRow.gpuOk(g);
    const char* text = nullptr;
    bool orange = true;
    std::string hover;
    if (!f.shownInstalled)
    {
        text = "not installed";
        const std::string missing = shownRow.missing();
        hover = (missing.empty() ? std::string("Its files are missing") : missing) +
                ". Copy them next to OptiScaler.dll; until then " +
                (other.installed() ? std::string(other.name) + " runs." : std::string("no neural runtime runs."));
    }
    else if (f.hipMissing || f.vkHeld)
    {
        // the attention slot says it (HIP / Retry lmxxf)
    }
    else if (f.restart)
    {
        text = "restart the game to switch";
        hover = std::string("Saved. ") + RuntimeCaps::Row(f.active).name +
                " keeps running until the next game start.";
    }
    else if (shownGpuBad && g.amd && g.known && other.gpuOk(g))
    {
        text = "not for this GPU";
        hover = g.note + ". Pick " + other.name + " instead.";
    }
    else if (f.active == f.shown && f.stopped)
    {
        text = "stopped";
        orange = false;
        hover = "It stopped for this session after an error; the status line below says why.";
    }
    else if (f.active == f.shown && config->DlssNrEnabled.value_or_default())
    {
        text = "running";
        orange = false;
    }
    if (text != nullptr)
    {
        if (orange)
            OrangeTag(text);
        else
            DimTag(text);
        if (!hover.empty())
            HelpForLastItem(hover.c_str());
    }
}

// A3: the active runtime's credit, one plain dim line as in the mock (Daniel's permission to ship his runtime asks for
// prominent attribution; the header's row 2 names both runtimes on every tab). The credit opens the author's page:
// the hand cursor shows it, as on the header's credits row (0.3.4 W2 drew an ImGui::TextLink, which the theme
// colours red and underlines).
void DrawCreditLine(const TopFrame& f)
{
    const RuntimeCaps::RuntimeInfo& menu = RuntimeCaps::Menu();
    // The runtime the tab talks to, or the combo's when that one is not installed (a Vulkan hold on the lmxxf-only
    // package runs nothing, and danielblnc's credit would name a runtime that is not there).
    const RuntimeCaps::RuntimeInfo& r = menu.installed() ? menu : RuntimeCaps::Row(f.shown);
    ImGui::TextDisabled("%s", r.credit);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemClicked())
            OpenUrl(r.url);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
        if (r.id == NeuralRuntime::Daniel)
            ImGui::TextUnformatted("Daniel Blanco's DLSS-NR on AMD runtime, shipped unmodified with his permission.");
        else
            ImGui::TextUnformatted("Kien's open-source HIP runtime, MIT licence.");
        ImGui::TextDisabled("Click to open %s", WithoutScheme(r.url));
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    const auto& g = DlssNr::AmdBridge::GpuSupportInfo();
    if (r.id == NeuralRuntime::Daniel)
    {
        // The installed build, as 0.3.3.2 and the approved mock show it (" - build <x.y.z>"): f.Version, danielblnc's
        // identified build (AmdBridge::RuntimeName: the loaded pass DLL, else the file identified once); for a build
        // not driven here RuntimeName's whole sentence, as 0.3.3.2 showed it (the combo then shows the name alone).
        // Only for an installed runtime; "pass1?" read as an error.
        // (top review) lmxxf's line stays the mock's ("lmxxf runtime by Kien (MIT)" + the AMDNR suffixes): its version
        // is the combo's (owner decision 2 names the combo only), not the credit's, where a build number after Kien's
        // name would read as his release.
        if (DlssNr::AmdBridge::HasFiles())
        {
            const std::string& version = f.Version(r.id);
            ImGui::SameLine(0.0f, 0.0f);
            if (!version.empty())
                ImGui::TextDisabled(" - build %s", version.c_str());
            else
                ImGui::TextDisabled("%s", " - build not identified");
        }
        return;
    }
    if (g.lmxxfOk && g.target.rfind("gfx11", 0) == 0)
    {
        // lmxxf on RDNA 3 is AMDNR's own backend: say whose, wherever it runs (0.3.3.2's GPU line, M:323-325). On a
        // 12+ CU handheld APU it runs but is slow and untested (runtime work HB request 6): dim, not orange.
        ImGui::SameLine(0.0f, 0.0f);
        if (g.lmxxfExperimental)
            ImGui::TextDisabled("%s", " - RDNA 3 backend by 3zwr1 (AMDNR), experimental");
        else
            ImGui::TextDisabled("%s", " - RDNA 3 backend by 3zwr1 (AMDNR)");
    }
    else if (f.c32w)
    {
        // (0.3.4 menu rework, mock T3) lmxxf on RDNA 4 runs AMDNR's one-wave C32 kernels (Evaluate: the runtime's
        // c32w word, else gfx1201): the owner's credit, as the RDNA 3 backend's above.
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextDisabled("%s", " - c32w kernels by 3zwr1 (AMDNR)");
    }
}

// A4: one dim line and its (?). The body (R4: at most 4 short lines) says where the pass runs and the cadence; the
// game's Streamline and the Vulkan note of the runtime that runs follow as footnotes, and a key only in the dim ini
// footer. The Ray Regeneration fallback reason is the attention slot's (offer 9) and the Ray Regeneration section's;
// the backend's full status text is the hover of attention offer 7b (G2 review: the hover had grown past 6 lines).
void DrawStatusLine(Config* config, const TopFrame& f, bool enabled)
{
    const NeuralState& ns = Shared();
    const auto& st = f.st;
    const auto& state = State::Instance();
    std::string text;
    std::string body;
    auto addBody = [&body](const std::string& line) {
        if (line.empty())
            return;
        if (!body.empty())
            body += "\n";
        body += line;
    };
    std::string slNote, vkNote;
    const char* ini = nullptr;
    char buf[256];

    if (!enabled)
    {
        const int key = config->DlssNrToggleKey.value_or_default();
        text = key == UnboundKey ? std::string("Off - tick Enable Neural Rendering to turn it on")
                                 : "Off - press " + MenuCommon::KeyName(key) + " to turn it on";
    }
    else if (f.stopped)
    {
        text = "Stopped - the neural runtime stopped for this session";
        addBody("It stopped after an error; frames pass through without NR until the next game start.");
        if (!f.status.empty())
            addBody(Headline(f.status));
    }
    else if (!f.refusal.empty())
    {
        // (0.3.4, P1-host request 1) Not "Idle" with its pick-an-upscaler advice: frames arrive, danielblnc was refused
        // them. The orange line under it (attention offer 3b) says it for the player; the full text is its hover.
        text = "Not running - danielblnc cannot run in this game";
        addBody("The game's D3D12 device and its queue's device could not be paired; danielblnc's runtime would crash"
                " on them, so it is not started. The game runs without NR.");
    }
    else if (st.recorded == 0 || (f.rateValid && ns.nrFps < 0.5f))
    {
        const bool idle = st.recorded != 0 || (ns.idleSince >= 0.0 && ImGui::GetTime() - ns.idleSince > 5.0);
        text = !idle ? "Starting"
                     : st.recorded == 0 ? "Idle - no upscaled frame has reached the pass yet"
                                        : "Idle - no upscaled frame is reaching the pass now";
        if (idle)
            addBody("Usually the game runs no upscaler OptiScaler can take over: pick DLSS, FSR or XeSS in the game"
                    " (not Off, TAA or native).");
    }
    else
    {
        text = "Running";
        std::snprintf(buf, sizeof buf, "%s%ux%u", kSep, st.width, st.height);
        text += buf;
        if (config->AmdDynamicRes.value_or_default())
            text += " (Dynamic NR)";
        else
        {
            std::snprintf(buf, sizeof buf, " at %.0f%%", config->AmdNrScale.value_or_default() * 100.f);
            text += buf;
        }
        std::snprintf(buf, sizeof buf, "%sNR %.0f/s%s", kSep, ns.nrFps, kSep);
        text += buf;
        if (ns.modelFps > 0.5f)
            std::snprintf(buf, sizeof buf, "model %.0f/s", ns.modelFps);
        else
            std::snprintf(buf, sizeof buf, "%s", "model idle");
        text += buf;
        if (ns.skipRate)
        {
            std::snprintf(buf, sizeof buf, "%s%llu frames without NR", kSep, ns.skipRate);
            text += buf;
        }
        // (0.3.4 W3-D, mock T4) NR's GPU time per model frame, AmdPreSr::Stats::nrGpuMs (P7.10), on both runtimes;
        // -1 = not measured (danielblnc on D3D11 / Vulkan, or not timed in the last 2 s): the text is left out.
        const bool showMs =
            st.nrGpuMs >= 0.f && RuntimeCaps::Get(RuntimeCaps::Cap::NrCostReadout) == RuntimeCaps::Support::Yes;
        if (showMs)
        {
            std::snprintf(buf, sizeof buf, "%s%.1f ms", kSep, st.nrGpuMs);
            text += buf;
        }

        // WHERE THE PASS RUNS right now - there is no placement to choose on AMD (0.3.3; 0.3.3.2's
        // "AMD processing" line, M:575-594). With a DLSS, FSR or XeSS upscaler the pass runs before it; when the
        // game runs Ray Reconstruction (DX12) it runs after it automatically.
        const ULONGLONG rrLast = state.fsrRrLastDispatchMs;
        if (rrLast != 0 && GetTickCount64() - rrLast < 2000)
            addBody("Placement: after Ray Regeneration (the game's Ray Reconstruction).");
        else
            addBody("Placement: before Super Resolution (input: " + ApiUpscalerInputName(state.currentInputApiName) +
                    ").");
        // The cadence actually being delivered, which is not always the cadence that was asked for - a refused
        // record or a busy slot moves it.
        if (ns.modelFps > 0.5f)
            std::snprintf(buf, sizeof buf, "NR/s: frames with NR; model/s: network runs, every %.2f frames.",
                          ns.nrFps / ns.modelFps);
        else
            std::snprintf(buf, sizeof buf, "%s", "NR/s: frames with NR; model/s: network runs.");
        addBody(buf);
        if (ns.skipRate)
            addBody("Frames without NR are shown raw: normal at a scene change; if steady, lower NR resolution.");
        if (showMs)
            addBody("ms: NR's GPU time per model frame (lmxxf: the network's time per job; danielblnc: the game's"
                    " queue inside NR).");
    }

    if (enabled)
    {
        // THE GAME'S STREAMLINE (0.3.3, F3: State::slInitResult, set by hkslInit). A failure while nothing reaches
        // the pass is an attention offer; the rest is one footnote here ("unknown" = hkslInit not installed, e.g.
        // [FrameGen] External=true).
        const int32_t slInit = state.slInitResult.load();
        const char* slName = state.slInitResultName.load();
        if (slInit > 0 && st.recorded != 0)
        {
            std::snprintf(buf, sizeof buf, "Streamline did not start (slInit %s): no DLSS; NR runs on FSR/XeSS.",
                          slName != nullptr ? slName : "failed");
            slNote = buf;
        }
        else if (slInit <= 0 && state.streamlineVersion.major > 0)
        {
            if (slInit < 0 || slName == nullptr)
                std::snprintf(buf, sizeof buf, "%s", "Streamline slInit: unknown");
            else
                std::snprintf(buf, sizeof buf, "Streamline slInit: %s in %u ms", slName, state.slInitMs.load());
            slNote = buf;
        }
        // The Vulkan note follows the runtime that runs (RuntimeCaps::Menu(), as the controls below do), not the
        // combo (M:370-397 at 0.3.3.2, dim lines under the combo).
        if (state.api == API::Vulkan)
        {
            if (MenuRuntimeIsLmxxf())
                vkNote = "Vulkan: a one-time 1-2 s hitch loads the network, then each frame waits for its answer"
                         " (Model interleave halves the cost).";
            else if (DlssNr::AmdBridge::HasFiles())
            {
                // danielblnc's counterpart (AmdBridge.cpp says the same once in amd_bridge.log). The runtime reads
                // its own ini (dlssnr_on_amd.ini), whose async mode passes pre-upscale colour through untouched.
                // With [DlssNr] AmdVkLateCopyWait on (experimental, IFeature_VkwDx12.cpp) the first-frame pause is
                // expected gone; the key is named only in the footer (R4).
                const bool lateCopyWait = config->AmdVkLateCopyWait.value_or_default();
                vkNote = lateCopyWait ? "Vulkan: NR runs inline; no first-frame pause expected (late copy wait,"
                                        " experimental). Keep danielblnc's own ini as shipped (async shows no NR)."
                                      : "Vulkan: NR runs inline; the first NR frame pauses about 5 s. Keep"
                                        " danielblnc's own ini as shipped (async shows no NR).";
                if (lateCopyWait)
                    ini = "[DlssNr] AmdVkLateCopyWait";
            }
        }
    }

    // (0.3.4 owner decision 2) The runtime with its exact version: the one built, else the one the tab talks to.
    const NeuralRuntime runtimeNow = f.active != NeuralRuntime::Unchosen ? f.active : RuntimeCaps::Menu().id;
    std::string rtNote = "Runtime: " + f.Label(runtimeNow);

    // As StatusLine (NeuralUi.cpp) draws it, with the notes as footnotes and the ini footer. A note becomes the body
    // when the state has none (Starting, Off), so the tooltip does not open with a blank line.
    if (body.empty())
        body.swap(!slNote.empty() ? slNote : !vkNote.empty() ? vkNote : rtNote);
    ImGui::TextDisabled("%s", text.c_str());
    if (!body.empty())
        Help(body.c_str(),
             { slNote.empty() ? nullptr : slNote.c_str(), vkNote.empty() ? nullptr : vkNote.c_str(),
               rtNote.empty() ? nullptr : rtNote.c_str() },
             ini);
}

// A4b (0.3.4 owner decision 3) THE LIVE TREE, right under the status line and closed by default: the numbers 0.3.3.2's
// Live section showed, one dim line each, with " | " between the parts; each line's hover says what its parts mean.
// Reads what Evaluate worked out this frame (the rates advance there, once per frame), nothing new per frame but two
// atomics.
void DrawLiveTree(const TopFrame& f)
{
    if (!TreeNode("Live"))
        return;
    const NeuralState& ns = Shared();
    const auto& st = f.st;
    const auto& state = State::Instance();
    char buf[256];

    // 1. NR frames N/s | model N/s | every N frames (the delivered cadence: frames per network run)
    if (!f.rateValid)
        std::snprintf(buf, sizeof buf, "%s", "NR frames: not measured yet (no NR frame recorded lately)");
    else if (ns.modelFps > 0.5f)
        std::snprintf(buf, sizeof buf, "NR frames %.0f/s | model %.0f/s | every %.2f frames", ns.nrFps, ns.modelFps,
                      ns.nrFps / ns.modelFps);
    else
        std::snprintf(buf, sizeof buf, "NR frames %.0f/s | model idle", ns.nrFps);
    StatusLine(buf, "NR frames: frames with NR per second. model: network runs per second. every: frames per network"
                    " run as delivered (1 = every frame; Model interleave raises it).");

    // 2. Working at WxH | dropped frames N | network N ms
    {
        std::string line;
        if (st.width != 0 && st.height != 0)
            std::snprintf(buf, sizeof buf, "Working at %ux%u", st.width, st.height);
        else
            std::snprintf(buf, sizeof buf, "%s", "Working size not known yet");
        line = buf;
        std::snprintf(buf, sizeof buf, " | dropped frames %llu", f.rateValid ? ns.skipRate : 0ull);
        line += buf;
        const bool timed =
            st.nrGpuMs >= 0.f && RuntimeCaps::Get(RuntimeCaps::Cap::NrCostReadout) == RuntimeCaps::Support::Yes;
        if (timed)
            std::snprintf(buf, sizeof buf, " | network %.1f ms", st.nrGpuMs);
        else
            std::snprintf(buf, sizeof buf, "%s", " | network not timed");
        line += buf;
        StatusLine(line.c_str(),
                   "Working at: the size the model works at. dropped frames: frames without NR in the last half second,"
                   " shown raw (normal at a scene change). network: NR's GPU time per model frame; not timed on"
                   " danielblnc in D3D11 / Vulkan games.");
    }

    // 3. Streamline slInit: ... | runtime <name> <version>
    {
        const int32_t slInit = state.slInitResult.load();
        const char* slName = state.slInitResultName.load();
        std::string line;
        if (slInit < 0 || slName == nullptr)
            line = state.streamlineVersion.major > 0 ? "Streamline slInit: unknown" : "Streamline: not seen";
        else if (slInit > 0 && !state.slInitIsSl1.load())
        {
            std::snprintf(buf, sizeof buf, "Streamline slInit: %s (error 0x%X) in %u ms", slName, (unsigned) slInit,
                          state.slInitMs.load());
            line = buf;
        }
        else
        {
            std::snprintf(buf, sizeof buf, "Streamline slInit: %s in %u ms", slName, state.slInitMs.load());
            line = buf;
        }
        const NeuralRuntime runtimeNow = f.active != NeuralRuntime::Unchosen ? f.active : RuntimeCaps::Menu().id;
        line += " | runtime " + f.Label(runtimeNow);
        StatusLine(line.c_str(),
                   "Streamline slInit: how the game's Streamline started (unknown: its slInit was not seen, e.g."
                   " [FrameGen] External=true). runtime: the neural runtime that runs, with its files' version.");
    }

    // 4. The late-submission share (P3), while the last window had late frames
    if (f.lateShare >= 0.f)
    {
        const int pct = static_cast<int>(std::lround(f.lateShare * 100.f));
        std::snprintf(buf, sizeof buf, "Late submission: %s ran on %d%% of the last ~600 frames",
                      f.active == NeuralRuntime::Lmxxf ? "the model" : "NR", pct);
        StatusLine(buf, "The game hands some frames' work to the GPU only after the next frame has started. lmxxf keeps"
                        " such a job one frame ([DlssNr] AmdLateSubmitGrace); danielblnc's runtime skips those frames."
                        " Shown while the last window of about 600 frames had a late frame.");
    }
    ImGui::TreePop();
}
} // namespace

bool DrawTop(const Ctx& ctx)
{
    Config* config = ctx.config;

    // A1: [x] Enable Neural Rendering [Home]. Each part has its own hover (the help attaches to the last item).
    bool enabled = config->DlssNrEnabled.value_or_default();
    if (Checkbox("Enable Neural Rendering", &enabled))
    {
        config->DlssNrEnabled = enabled;
        // Logged like the hotkey (0.3.3.2): a tester's menu switches went unrecorded
        LOG_INFO("Neural Rendering checkbox: DlssNrEnabled {}", enabled);
        DlssNr::AmdBridge::InvalidateHistory();
        DlssNr::AmdBridge::NoteToggle(enabled, "menu");
    }
    Help("Turns the neural pass on or off, on every runtime. The key beside it does the same while you play (menu"
         " closed); a small notice says On or Off.",
         {}, "[DlssNr] Enabled");
    ImGui::SameLine();
    MenuCommon::RenderDlssNrToggleKeybind();
    Help("The key that turns Neural Rendering on or off while you play. Click it, then press a key: Backspace"
         " unbinds, Esc cancels. R, shown once the key is changed, puts the default back.",
         {}, "[DlssNr] ToggleKey");

    const TopFrame& f = Evaluate(*config);
    if (!f.anyRuntime)
    {
        // No runtime files: on an AMD (or any non-NVIDIA) GPU the missing-runtime card follows and nothing else;
        // on NVIDIA the NVIDIA layout follows, and the D3D11/Vulkan hint stays as 0.3.3.2 drew it there.
        if (IdentifyGpu::getPrimaryGpu().vendorId == VendorId::Nvidia)
        {
            if (const ApiHint hint = ApiHintFor(enabled, true); hint.show)
            {
                ImGui::TextColored(kOrange, "   %s", hint.line.c_str());
                Help(hint.hover.c_str());
            }
        }
        return enabled;
    }

    DrawRuntimeRow(config, f);          // A2
    NudgeY(-3.0f);                      // the dim lines sit tight under the row (see NudgeY)
    DrawCreditLine(f);                  // A3
    NudgeY(-6.0f);                      // 14 px apart, as the mock's
    DrawStatusLine(config, f, enabled); // A4
    DrawLiveTree(f);                    // A4b (owner decision 3)

    // FINAL IMAGE MODE, for games with FSR 1 or no upscaler at all. The normal path needs
    // a DLSS / FSR 2+ / XeSS call to intercept; this takes the back buffer at Present.
    // Hidden while AmdFinalImage::kAvailable is false (see PresentExperimental.h); back for
    // testing in 0.3.x with both runtimes' history paths forced off.
    if (AmdFinalImage::kAvailable)
    {
        bool finalImage = config->AmdFinalImage.value_or_default();
        if (Checkbox("Final image mode (games with FSR 1 or no upscaler)", &finalImage))
            config->AmdFinalImage = finalImage;
        HelpMarker("For games that give OptiScaler nothing to hook: FSR 1 only, or no"
                   "\nupscaler at all. The neural pass runs on the finished frame at Present"
                   "\ninstead of on the upscaler's input.\n"
                   "\nWhat it costs, honestly: there are no motion vectors and no depth, so the"
                   "\nmodel runs with no temporal history (every frame from scratch), Model"
                   "\ninterleave and Temporal stability are off in this mode, and the HUD is"
                   "\nprocessed with the scene. NR resolution and Residual strength still work.\n"
                   "\nRuns the lmxxf runtime (danielblnc's only when Neural runtime says daniel),"
                   "\nhistory off. With lmxxf the frame never waits for the network (the answer is"
                   "\ntaken when it is done), the motion is estimated by block matching so the"
                   "\nedit follows the picture, Temporal stability smooths it between answers, and"
                   "\nModel interleave is the network's rest in frames between two feeds (more ="
                   "\nless GPU taken from the game). NR resolution defaults to half size here.\n"
                   "\nIt stays idle whenever a DLSS/FSR/XeSS input is active - one neural"
                   "\nbackend per game - so to try it in a game that has an upscaler, switch"
                   "\nthe game's upscaler off first. D3D11 games cross to a D3D12 device"
                   "\nand back through shared textures, like the upscaler bridge does.");
        if (finalImage)
            ImGui::TextWrapped("%s", AmdFinalImage::Status().c_str());
    }

    return enabled;
}

void DrawMissingRuntimeCard(const Ctx&)
{
    // THE MISSING-FILES CASE SAYS SO. A tester on an AMD card whose bin folder had the proxy
    // but not the AMD runtime saw the NVIDIA layout below with "the NGX core would not
    // initialise", which reads as a bug rather than as four files that are not there.
    // (0.3.4, mock 5.8) The card is all the tab shows then; the NVIDIA layout is for NVIDIA GPUs only.
    if (!DlssNr::AmdBridge::AnyRuntimePresent() && IdentifyGpu::getPrimaryGpu().vendorId != VendorId::Nvidia)
    {
        const auto expected = Util::DllPath().parent_path().string();
        ImGui::TextColored(kOrange, "%s", "No neural runtime found.");
        // (G2 review) 0.3.3.2 drew the GPU line above the card: an RX 6000 or handheld owner learns here that no
        // runtime file would start on this GPU (dim: the card's headline is the page's orange line).
        const auto& g = DlssNr::AmdBridge::GpuSupportInfo();
        if (g.amd && g.known && !g.danielOk && !g.lmxxfOk)
        {
            ImGui::TextDisabled("%s", "This GPU cannot run a neural runtime");
            const std::string why = g.name.empty() ? g.note : g.name + ": " + g.note;
            if (!why.empty())
                Help(why.c_str());
        }
        ImGui::TextUnformatted("Copy the runtime files next to OptiScaler.dll, in:");
        Help("lmxxf: LmxxfNrRuntime.dll and LmxxfNrRuntime.pak. danielblnc: dlssnr_amd_pass1.dll,"
             " dlssnr_amd_pass2.dll, dlssnr_amd_pass3.dll and dlssnr_on_amd_weights.bin. Either set is enough; the"
             " settings appear as soon as the files are there.");
        ImGui::TextWrapped("%s", expected.c_str());
        ImGui::TextDisabled("%s", "Help and downloads:");
        ImGui::SameLine();
        if (ImGui::TextLink("discord.gg/AMDNR##missingRuntime"))
            OpenUrl("https://discord.gg/AMDNR");
    }
}

void DrawAttention(const Ctx& ctx)
{
    Config* config = ctx.config;
    const TopFrame& f = Evaluate(*config);
    NeuralState& ns = Shared();
    bool &vkRetried = ns.vkRetried, &vkRetryNow = ns.vkRetryNow, &vkRetryFailed = ns.vkRetryFailed;

    // A5 (plan 4 A5): the first offer is the orange line; the others are counted in its (+N) hover. The button
    // of the first offer is the only one drawn, so remember what it does.
    enum class Action
    {
        None,
        RetryLmxxf,
        SwitchToDaniel,
        OpenUpscaling,
    };
    AttentionSlot slot;
    Action firstAction = Action::None;
    auto offer = [&](std::string text, std::string hover, const char* button, Action action) {
        if (slot.Empty())
            firstAction = action;
        slot.Offer(std::move(text), std::move(hover), button);
    };

    // 2. The GPU can run no runtime at all (0.3.3.2's orange GPU note). A GPU missing from the device table says
    //    nothing here: the runtime itself decides.
    const auto& g = DlssNr::AmdBridge::GpuSupportInfo();
    if (g.amd && g.known && !g.danielOk && !g.lmxxfOk)
        offer("This GPU cannot run a neural runtime", g.note, nullptr, Action::None);

    // 3. lmxxf chosen, but HIP cannot be used (danielblnc's runtime was built instead). 0.3.3.2's rule and order.
    //    (0.3.4, RR-27) Under Wine/Proton (State::isRunningOnLinux, IsRunningOnWine in dllmain.cpp) HIP never loads:
    //    amdhip64_7.dll comes with the Windows AMD driver only, so "reinstall the driver" would send the player the
    //    wrong way. Both runtimes need HIP there; FSR upscaling does not. (0.3.4.2, H3) FSR Ray Regeneration is not
    //    claimed to work there any more: 0.3.4.1's docs list it as a known issue under Wine/Proton (pink / magenta
    //    patches in some games), and the hover repeats that wording.
    if (f.hipMissing)
    {
        if (State::Instance().isRunningOnLinux)
            offer("Running under Wine/Proton: Neural Rendering cannot run here",
                  "Both NR runtimes need AMD's HIP runtime (amdhip64_7.dll), which comes with the Windows AMD driver"
                  " and is not available under Wine/Proton. FSR upscaling works under Wine/Proton. FSR Ray"
                  " Regeneration is a known issue there: it can show pink / magenta patches in some games; if you see"
                  " them, use plain FSR (FSR 4 on RX 9000) and send a Save report.",
                  nullptr, Action::None);
        else
            offer("HIP is not available on this system, so lmxxf cannot run here",
                  "The AMD driver's HIP runtime did not load or start (the log says which step). danielblnc's runtime"
                  " runs instead. Reinstalling the AMD driver usually fixes it.",
                  nullptr, Action::None);
    }

    // 3b. (0.3.4, P1-host request 1) danielblnc was refused this game's device (AmdBridge::NrDeviceRefusal): one line
    //     instead of the backend status (7b skips it), the full text in the hover. No Switch offer: lmxxf would refuse
    //     the same game until P1-B lands. Only while NR is on (with NR off the game runs without it anyway).
    const bool refusalOffered = ctx.enabled && !f.refusal.empty();
    if (refusalOffered)
    {
        constexpr const char* kPrefix = "AMD neural: ";
        const bool prefixed = f.refusal.rfind(kPrefix, 0) == 0;
        offer("danielblnc cannot run in this game: its D3D12 device is wrapped by a layer AMDNR cannot see through. The"
              " game runs without NR.",
              prefixed ? f.refusal.substr(std::strlen(kPrefix)) : f.refusal, nullptr, Action::None);
    }

    // (G2 review) The offers with a button (6 Retry lmxxf, 7a Switch to danielblnc) come before the text-only ones
    // (4, 5): Draw() gives only the first offer its button, while the (+N) hover still lists the others' text.
    // The plan's order had the D3D11/Vulkan hint first, which hid Retry lmxxf on a Vulkan title.

    // 6. lmxxf held back on this Vulkan game (the Retry lmxxf label is kept: the guides name it).
    if (f.vkHeld && !vkRetried)
    {
        // R4 (G2 review): four short lines; the marker file is named only when it could not be removed (the line).
        std::string hover = std::string("Its last session here stopped before its first answer (a freeze, or the game"
                                        " closed first). ") +
                            (DlssNr::AmdBridge::HasFiles() ? "danielblnc runs this time." : "No NR this time.") +
                            "\nRetry lmxxf: it starts at the next upscaler frame, or at the next game start if a"
                            " runtime already runs.";
        offer(vkRetryFailed ? "Could not remove lmxxf_vk_launch.pending: delete it beside OptiScaler.dll by hand"
                            : "lmxxf was held back: its last session in this Vulkan game stopped early",
              std::move(hover), "Retry lmxxf##vkLaunchPending", Action::RetryLmxxf);
    }

    // 7a. FX-N4: lmxxf stopped after an error in this session, and danielblnc's runtime is complete (pass DLLs and
    //     weights) and can run on this GPU: offer the switch in place of the status line (7b). It writes the same
    //     [DlssNr] NrBackend the combo writes, saved at once; it acts at the next game start. Not offered once
    //     danielblnc is the saved choice.
    bool switchOffered = false;
    if (f.active == NeuralRuntime::Lmxxf && f.stopped &&
        DlssNr::AmdBridge::ChosenRuntime() != NeuralRuntime::Daniel && (!g.known || g.danielOk) &&
        RuntimeCaps::Row(NeuralRuntime::Daniel).missing().empty())
    {
        std::string hover = f.status.empty() ? std::string() : f.status + "\n\n";
        hover += "Switch to danielblnc saves Neural runtime = danielblnc; it runs from the next game start. lmxxf"
                 " stays installed and can be picked again in Neural runtime.";
        offer("lmxxf stopped after an error", std::move(hover), "Switch to danielblnc##nrSwitch",
              Action::SwitchToDaniel);
        switchOffered = true;
    }

    // 4. danielblnc's standalone found beside the game (0.3.3.2, AmdBridge::ForeignStandalone):
    //    its own End/Enter overlay switches only itself, which testers read as "NR does nothing".
    if (const std::string foreign = DlssNr::AmdBridge::ForeignStandalone(); !foreign.empty())
    {
        std::string line = Headline(foreign);
        std::string rest = RestOf(foreign, line);
        offer(std::move(line), std::move(rest), nullptr, Action::None);
    }

    // 5. A D3D11 or Vulkan game on an upscaler that never reaches the pass (once the game has created one). The button
    //    opens the Upscaling tab (T11g, MenuCommon::RequestTab; the menu plan's "Open Upscaling").
    if (ApiHint hint = ApiHintFor(ctx.enabled, false); hint.show)
        offer(std::move(hint.line), std::move(hint.hover), "Open Upscaling##nrOpenUpscaling", Action::OpenUpscaling);

    // 7b. The backend's status when it is not the routine one (debounced in Evaluate); the full text in the hover.
    //     Only while NR is on: with NR off Run() is not reached, so the text is whatever the last NR frame left
    //     (the status line says Off).
    if (ctx.enabled && !switchOffered && !refusalOffered && !f.status.empty())
    {
        std::string line = Headline(f.status);
        std::string rest = RestOf(f.status, line);
        offer(std::move(line), std::move(rest), nullptr, Action::None);
    }

    // 8. THE GAME'S STREAMLINE FAILED TO START (0.3.3, F3). The game then switches DLSS off and no upscaler call
    //    reaches the pass. Orange only while nothing reaches the pass (0.3.3.2, TLOU Part I: sl.dlss refuses an
    //    AMD GPU in every build, and NR ran fine on the game's FSR 3.1); after that the status hover says it.
    {
        const auto& slState = State::Instance();
        const int32_t slInit = slState.slInitResult.load();
        const char* slName = slState.slInitResultName.load();
        if (slInit > 0 && f.st.recorded == 0)
        {
            char buf[256];
            if (slState.slInitIsSl1.load())
                std::snprintf(buf, sizeof buf, "The game's Streamline failed to start (slInit %s): no DLSS in this game",
                              slName != nullptr ? slName : "failed");
            else
                std::snprintf(buf, sizeof buf,
                              "The game's Streamline failed to start (slInit error 0x%X, %s): no DLSS in this game",
                              (unsigned) slInit, slName != nullptr ? slName : "?");
            offer(buf, "Pick FSR or XeSS in the game's settings; Neural Rendering runs before them the same way.",
                  nullptr, Action::None);
        }
    }

    // 9. A Ray Reconstruction handle FSR Ray Regeneration gave up on (0.3.3.2 said it in orange in the Live
    //    section, because the game shows RR as on and the player sees "nothing happens").
    if (const auto& why = State::Instance().rrFallbackReason; !why.empty())
        offer(FSRD::RuntimeStatus::RayRegenOffForSession.load(std::memory_order_relaxed)
                  ? "Ray Regeneration is off in this title (stays off for this game session)"
                  : "Ray Regeneration is off in this title",
              why + "\n\nFSR upscaling runs in its place; the neural pass takes its normal pre-SR position.", nullptr,
              Action::None);

    // 10. (0.3.4, P3-late request 1) The game submits frames late (TLOU II): while NR runs and the built runtime ran on
    //     less than 80% of the last window's frames. The orange line only when the slot is free, else a dim line under
    //     it (at most one orange line). The share changes at most every ~10 s, so the line does not flicker.
    std::string lateLine, lateHover;
    if (ctx.enabled && !f.stopped && f.st.recorded != 0 && f.lateShare >= 0.f && f.lateShare < 0.8f)
    {
        const int pct = static_cast<int>(std::lround(f.lateShare * 100.f));
        char buf[96];
        if (f.active == NeuralRuntime::Lmxxf)
        {
            std::snprintf(buf, sizeof buf, "This game submits frames late: the model ran on %d%% of frames", pct);
            lateHover = "The game hands each frame's work to the GPU only after the next frame has started. lmxxf keeps"
                        " such a frame's job one frame and uses its answer one frame later ([DlssNr]"
                        " AmdLateSubmitGrace, on by default); the carried edit covers the frames in between. The"
                        " lmxxf_backend.log stats line counts late / graced frames.";
        }
        else
        {
            std::snprintf(buf, sizeof buf, "This game submits frames late: NR ran on %d%% of frames", pct);
            lateHover = "The game hands each frame's work to the GPU only after the next frame has started."
                        " danielblnc's runtime can hold only one frame that has not reached the GPU yet, so the frames"
                        " in between get no NR (NR / no-NR flicker). lmxxf keeps NR on these frames: pick lmxxf in"
                        " Neural runtime for this game. amd_presr.log has an 'AMD late submission' line every ~600"
                        " frames.";
        }
        lateLine = buf;
    }
    const bool lateDim = !lateLine.empty() && !slot.Empty();
    if (!lateLine.empty() && slot.Empty())
        offer(lateLine, lateHover, nullptr, Action::None);

    const bool clicked = slot.Draw() == 0;
    if (lateDim)
        StatusLine(lateLine.c_str(), lateHover.c_str());
    if (clicked && firstAction == Action::OpenUpscaling)
        MenuCommon::RequestTab(MenuCommon::MenuTab::Upscaling);
    if (clicked && firstAction == Action::SwitchToDaniel)
    {
        config->DlssNrBackend = std::string("daniel");
        config->SaveIni();
        LOG_INFO("AMD neural: Neural runtime set to danielblnc from the menu after lmxxf stopped (saved; runs from the "
                 "next game start)");
    }
    if (clicked && firstAction == Action::RetryLmxxf)
    {
        // Only our own file: AmdBridge.cpp's kVkLaunchMarker, in its Directory() (the folder
        // of OptiScaler.dll). With no backend built yet (the lmxxf-only layout, or before the
        // first upscaler frame) the bridge finds the file gone at the next upscaler frame and
        // builds lmxxf then; with danielblnc's runtime already running, at the next start.
        std::error_code ec;
        std::filesystem::remove(Util::DllPath().parent_path() / L"lmxxf_vk_launch.pending", ec);
        vkRetryFailed = (bool) ec;
        if (!vkRetryFailed)
        {
            vkRetried = true;
            vkRetryNow = f.active == NeuralRuntime::Unchosen;
            LOG_INFO("AMD neural: lmxxf_vk_launch.pending removed from the menu - lmxxf is tried again {}",
                     vkRetryNow ? "at the next upscaler frame" : "on the next game start");
        }
        else
        {
            LOG_WARN("AMD neural: could not remove lmxxf_vk_launch.pending: {}", ec.message());
        }
    }
    // After Retry lmxxf: what happens next, in green (not an attention line).
    if (f.vkHeld && vkRetried)
        ImGui::TextColored(kGreen, "%s",
                           vkRetryNow ? "lmxxf_vk_launch.pending removed: lmxxf starts at the next upscaler frame (a"
                                        " 1-2 s hitch)"
                                      : "lmxxf_vk_launch.pending removed: restart the game to run lmxxf");
}

void DrawPlacement(const Ctx&)
{
    // (0.3.4, plan A4) Nothing is drawn here any more: where the pass runs ("AMD processing" at 0.3.3.2) is the
    // first line of the status line's hover (DrawStatusLine above). Kept for the dispatcher's call.
}

void DrawPresetAndStyle(const Ctx& ctx)
{
    Config* config = ctx.config;
    NudgeY(3.0f); // the mock's margin above the Preset row (0.3.4 W2: ImGui::Spacing, 6 px)

    // A6 PRESET (plan A6): three buttons for NR resolution in one click, the pressed one (red) following the settings
    // (a preset is recognised by its NR resolution with Dynamic NR off), so it reads the truth after an ini load
    // and after a manual change; otherwise the dim tag "Custom". D1: it writes NR resolution and Dynamic NR only, so
    // the look (NR style) is left as it is; kPresetWritesLookValues restores 0.3.3.2's writes. (0.3.3.2: a three-stop
    // slider.) The sliders below still fine-tune afterwards.
    // (0.3.4 owner decision 1, 2026-09-26) HANDHELD, a 4th button only on an lmxxf handheld APU
    // (GpuSupport::lmxxfExperimental: gfx1103 / gfx1150 with 12+ CUs); desktop GPUs keep the three. It writes NR
    // resolution 100%, Dynamic NR off, Model interleave every 4th frame (AmdInterleave = 4 as the player's own value, so
    // Save Settings writes it; the bridge's APU default is the same 4 but volatile), Neural passes 1 and Full network
    // off. The 360p network size is the APU's automatic AmdLmxxfTierCap and is not written. It reads as pressed while
    // all five values hold; it wins over Quality then (Quality's rule, 100% with Dynamic NR off, holds too).
    {
        static const char* const kPresetNames[] = { "Quality", "Balanced", "Performance", "Handheld" };
        static constexpr float kPresetScale[] = { 1.00f, 0.85f, 0.70f, 1.00f };
        constexpr int kHandheld = 3;
        constexpr float kHandheldInterleave = 4.f;
        const bool handheldGpu = DlssNr::AmdBridge::GpuSupportInfo().lmxxfExperimental;
        // (0.3.4 MENU handheld presets) The network size a handheld really runs at: the running lmxxf backend's cap
        // (Lmxxf::TierCapInUse, 720 when the runtime refused the small tiers), else the backend's rule for a handheld
        // (LmxxfTierPolicy.h DecideLmxxfTierPolicy: 360 when [DlssNr] AmdLmxxfTierCap is 0 = auto, else the key's tier).
        const Lmxxf::TierPolicy hhPolicy =
            Lmxxf::DecideLmxxfTierPolicy(config->AmdLmxxfTierCap.value_or_default(), true, false, false);
        const int hhCap = handheldGpu ? (Lmxxf::TierCapInUse() != 0 ? Lmxxf::TierCapInUse() : hhPolicy.cap) : 0;
        const bool hhAuto360 = handheldGpu && hhCap == 360 && hhPolicy.autoCap; // an ini 360 is not "(automatic)"
        // The owner's text (2026-09-26), exactly as given; only while the network size is the automatic 360p (it would be
        // false with AmdLmxxfTierCap=576, or any AmdLmxxfTierCap, set).
        static const char* const kHandheldSameLook =
            "On this handheld the network stays at 360p (automatic), so Quality, Balanced and Performance look like"
            " Handheld. For a stronger, much slower effect set AmdLmxxfTierCap=576 and AmdInterleave=2.";
        char hhFootnote[256] = {};
        if (handheldGpu)
        {
            char size[96] = {};
            if (hhCap == 360 && hhPolicy.autoCap)
                std::snprintf(size, sizeof size, "%s", "The 360p network size stays automatic.");
            else if (hhCap == 0)
                std::snprintf(size, sizeof size, "%s", "The network size stays automatic.");
            else
                std::snprintf(size, sizeof size, "The network size stays %dp (not changed by Handheld).", hhCap);
            std::snprintf(hhFootnote, sizeof hhFootnote,
                          "Handheld (this handheld only): NR resolution 100%%, Dynamic NR off, the model every 4th"
                          " frame, 1 neural pass, Full network off. %s",
                          size);
        }
        const float nrNow = config->AmdNrScale.value_or_default();
        const bool dynamicOff = !config->AmdDynamicRes.value_or_default();
        int active = -1;
        if (dynamicOff)
            for (int i = 0; i < 3; ++i)
                if (std::fabs(nrNow - kPresetScale[i]) < 0.01f)
                    active = i;
        if (handheldGpu && dynamicOff && std::fabs(nrNow - kPresetScale[kHandheld]) < 0.01f &&
            std::fabs(config->AmdInterleave.value_or_default() - kHandheldInterleave) < 0.01f &&
            config->DlssNrPasses.value_or_default() == 1u && !config->LmxxfFullNetwork.value_or_default())
            active = kHandheld;
        const int clicked = Segmented("nrPreset", std::span<const char* const>(kPresetNames, handheldGpu ? 4 : 3), active);
        if (clicked == kHandheld)
        {
            config->AmdNrScale = kPresetScale[kHandheld];
            config->AmdDynamicRes = false;
            config->AmdInterleave = kHandheldInterleave;
            config->DlssNrPasses = 1u;
            config->LmxxfFullNetwork = false;
            LOG_INFO("AMD neural: Preset Handheld from the menu (NR resolution 100%, Dynamic NR off, AmdInterleave 4, "
                     "Passes 1, LmxxfFullNetwork off; the network cap stays {})",
                     hhCap == 0 ? std::string("automatic") : std::to_string(hhCap) + "p");
        }
        else if (clicked >= 0)
        {
            config->AmdNrScale = kPresetScale[clicked];
            config->AmdDynamicRes = false;
            if constexpr (kPresetWritesLookValues)
            {
                // 0.3.3.2's stops: Temporal stability, Sharpening, and Detail/Colour strength back to full
                static constexpr float kStab[] = { 0.50f, 0.65f, 0.80f };
                static constexpr float kSharp[] = { 0.30f, 0.40f, 0.50f };
                config->AmdTemporalStability = kStab[clicked];
                config->AmdSharpness = kSharp[clicked];
                config->AmdDetailStrength = 1.0f;
                config->AmdColourStrength = 1.0f;
            }
        }
        // MN5 / runtime work HB request 4: the lmxxf network tiers Balanced and Performance run from a 1920x1080 input,
        // by the backend's own sizing rule (LmxxfTierPolicy.h PlanLmxxfSize: the tier snap as it applies, which is on
        // by default on RDNA 3, and a handheld's cap), so the footnote stays true whichever of them is on.
        char lmxxfTiers[160] = {};
        if (MenuRuntimeIsLmxxf())
        {
            const int cap = Lmxxf::TierCapInUse();
            const bool smallTiers = Lmxxf::SmallTierPolicy();
            const bool snap = DlssNr::AmdBridge::LmxxfTierSnapOn();
            // (0.3.4, lmxxf Fast mode) With [DlssNr] AmdLmxxfFastMode on, each plan one tier lower, as the backend does
            // (LmxxfTierPolicy.h PlanLmxxfFastSize from the tier the default size lands in; unchanged at the lowest).
            const bool fast = config->AmdLmxxfFastMode.value_or_default();
            auto presetTier = [&](float scale) {
                Lmxxf::TierPlan plan = Lmxxf::PlanLmxxfSize(1920u, 1080u, scale, cap, smallTiers, snap);
                Lmxxf::TierPlan fp;
                if (fast && Lmxxf::PlanLmxxfFastSize(1920u, 1080u, scale,
                                                     static_cast<unsigned>(Lmxxf::LmxxfNetworkTier(plan.w, plan.h, smallTiers)),
                                                     smallTiers, snap, fp))
                    plan = fp;
                return plan.to;
            };
            const unsigned balanced = presetTier(kPresetScale[1]);
            const unsigned performance = presetTier(kPresetScale[2]);
            std::snprintf(lmxxfTiers, sizeof lmxxfTiers,
                          "lmxxf: from a 1920x1080 input Balanced (85%%) %s the %u tier, Performance (70%%) the %u tier%s.",
                          balanced == 1080u ? "still runs" : "runs", balanced, performance,
                          fast ? " (Fast mode on)" : "");
        }
        LabelWithHelp("Preset",
                      kPresetWritesLookValues
                          ? "Sets NR resolution in one click (Quality 100%, Balanced 85%, Performance 70%, the best"
                            " frame rate) with Temporal stability and Sharpening, resets Detail and Colour strength and"
                            " turns Dynamic NR off. Custom: NR resolution was set by hand."
                          : "Sets NR resolution in one click: Quality 100%, Balanced 85%, Performance 70% (the best"
                            " frame rate). Turns Dynamic NR off. The look (NR style) is left as it is. Custom: NR"
                            " resolution was set by hand.",
                      { RuntimeCaps::Menu().noteNrResolution, lmxxfTiers[0] != '\0' ? lmxxfTiers : nullptr,
                        handheldGpu ? hhFootnote : nullptr, hhAuto360 ? kHandheldSameLook : nullptr },
                      handheldGpu ? "[DlssNr] AmdModelScale, AmdDynamicRes; Handheld also AmdInterleave, Passes,"
                                    " LmxxfFullNetwork"
                                  : "[DlssNr] AmdModelScale, AmdDynamicRes");
        if (active < 0)
            DimTag("Custom");
        // (0.3.4 MENU handheld presets, owner text) On a handheld at the 360p network size Quality / Balanced /
        // Performance look like Handheld: one dim line under the row says so, wrapped to the row as the other dim
        // wrapped lines (NeuralLook's Ray Regeneration notes). Desktop GPUs: nothing.
        if (hhAuto360 && active >= 0 && active < kHandheld)
        {
            NudgeY(-3.0f); // a dim line right under a frame row (NeuralUi.cpp DimLineFlow's rule)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", kHandheldSameLook);
            ImGui::PopStyleColor();
        }
    }

    // A7 NR STYLE (plan A7): named looks over the appearance controls; each style's hover says what it does. The
    // combo reads Custom when the sliders the menu's runtime uses match no style. Style slots: the three custom
    // slots in a popup (0.3.3.2: the "Custom style slots" tree; Save / Load are now Store / Apply, so Store is not
    // mistaken for Save Settings).
    {
        const int match = MatchingStyle(*config);
        if (BeginCombo("NR style", match < 0 ? "Custom" : kStylePresets[match].name))
        {
            for (int p = 0; p < kStylePresetCount; ++p)
            {
                if (ImGui::Selectable(kStylePresets[p].name, p == match))
                    ApplyStyleValues(*config, kStylePresets[p].v);
                if (p == match)
                    ImGui::SetItemDefaultFocus();
                HelpForLastItem(kStyleHelp[p]);
            }
            ImGui::EndCombo();
        }
        Help("Named looks: a style sets the Residual, Temporal stability, Sharpening, Detail, Colour and Model"
             " strength sliders and Output smoothing together (not Colour composition). Each runtime uses the parts"
             " it has. Custom: the sliders match no style.");
        ImGui::SameLine();
        if (Button("Style slots"))
            ImGui::OpenPopup("nrStyleSlots");
        Help("Three slots for your own looks: Store keeps the current one, Apply brings it back, Clear empties the"
             " slot. Save Settings writes the slots to the ini.",
             {}, "[DlssNr] StyleSlot1..3");
        if (ImGui::BeginPopup("nrStyleSlots"))
        {
            CustomOptional<std::string>* slots[3] = { &config->AmdStyleSlot1, &config->AmdStyleSlot2, &config->AmdStyleSlot3 };
            const float gap = ImGui::GetStyle().ItemSpacing.x * 2.0f;
            const float stateX = ImGui::CalcTextSize("Slot 3").x + gap;
            const float buttonsX = stateX + (std::max)(ImGui::CalcTextSize("saved").x, ImGui::CalcTextSize("empty").x) + gap;
            for (int i = 0; i < 3; ++i)
            {
                ImGui::PushID(i);
                const bool saved = !slots[i]->value_or_default().empty();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Slot %d", i + 1);
                ImGui::SameLine(stateX);
                ImGui::TextDisabled("%s", saved ? "saved" : "empty");
                ImGui::SameLine(buttonsX);
                if (Button("Store"))
                    *slots[i] = CaptureStyle(*config);
                ImGui::SameLine();
                ImGui::BeginDisabled(!saved);
                if (Button("Apply"))
                    ApplyStyleString(*config, slots[i]->value_or_default());
                ImGui::SameLine();
                if (Button("Clear"))
                    *slots[i] = std::string();
                ImGui::EndDisabled();
                ImGui::PopID();
            }
            ImGui::Spacing();
            ImGui::TextDisabled("%s", "Store keeps the current look in the slot;");
            ImGui::TextDisabled("%s", "Save Settings writes the slots to the ini.");
            ImGui::EndPopup();
        }
    }

    // The per-game profile combo that used to sit here is gone (user request): its
    // numbers were mostly starting points nobody had measured, and a second row of
    // presets under the first one read as two ways of saying the same thing.
}

void DrawLive(const Ctx&)
{
    // (0.3.4, plan A4/A5) The Live section is gone: its rates and working size are the status line (DrawTop), its
    // RR fallback reason and Streamline lines are in that line's hover and the attention slot, and the backend's
    // status is attention offer 7 (DrawAttention). Kept for the dispatcher's call.
}
} // namespace DlssNr::NeuralUi
