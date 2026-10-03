// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "pch.h"

#include "AmdnrReport.h"
#include "ReportZip.h"

#include <State.h>
#include <Util.h>
#include <dlssnr/amd/AmdBridge.h>
#include <misc/IdentifyGpu.h>

#include <magic_enum.hpp>

#include <bcrypt.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

// "Save report" (CRASH-a, AMDNR 0.3.4). The menu thread takes a snapshot of what the menu already shows (cheap
// getters only: no file access, no backend lock), then one worker thread does all the disk work at background
// priority: reads the logs through shared handles (spdlog keeps OptiScaler.log open), caps them, hashes the AMDNR
// files, masks user and computer names in every text entry, writes a stored zip as <name>.zip.tmp and renames it.
// Output: the game folder; if that cannot be written, the Desktop, then %TEMP%. Nothing is uploaded; no network API.
// Layout: report.txt, logs\ (OptiScaler.log, every OptiScaler.previous*.log generation, the AMD logs), config\ (the
// two ini files). Caps keep the worst case under about 9.5 MB (Discord's 10 MB limit).
namespace AmdnrReport
{
namespace
{
namespace fs = std::filesystem;
using DlssNr::AmdBridge::NeuralRuntime;

constexpr size_t kKiB = 1024;
constexpr size_t kMiB = 1024 * 1024;
constexpr size_t kBudget = 9 * kMiB + 512 * kKiB; // every entry together, report.txt included
constexpr size_t kReportReserve = 256 * kKiB;     // kept free for report.txt
constexpr size_t kSessionWindow = 8 * kMiB;       // how much of a multi-session log is searched for its sessions
constexpr uint64_t kHashLimit = 64 * kMiB;        // larger files get size and date only
constexpr char kSessionMarker[] = "AMDNR session start:";

std::atomic<bool> g_running { false };
std::mutex g_resultMutex;
Result g_last; // done / ok / text of the last finished save (running lives in g_running)

// What the menu shows, copied on the menu thread.
struct Snapshot
{
    std::string product;
    uint64_t tick = 0;
    FILETIME madeUtc {};
    DWORD pid = 0;

    std::string exe, gameName, gameVersion, engine, api, swapchainApi;
    bool wine = false;
    fs::path exePath, dllPath, logFile, mainDllDir;
    bool logToFile = false, logSingleFile = true;

    bool nrEnabled = false;
    NeuralRuntime chosen = NeuralRuntime::Unchosen, active = NeuralRuntime::Unchosen;
    std::string runtimeName;
    bool hasFiles = false, danielWeights = false, lmxxfReady = false, vkPending = false;
    bool backendActive = false, runtimeStopped = false;
    std::string status, compositionNote, foreignStandalone;
    AmdPreSr::Stats stats {};
    AmdPreSr::Settings settings {};
    DlssNr::AmdBridge::GpuSupport gpu {};
    std::vector<GpuInformation> gpus; // IdentifyGpu's list (its mutex is taken here, not by the worker)

    std::string feature;
    unsigned renderW = 0, renderH = 0, displayW = 0, displayH = 0, targetW = 0, targetH = 0;
    std::string fgInput, fgOutput, fgInputActive, fgOutputActive;
    bool fgEnabled = false;
    std::string rrFallbackReason;

    std::vector<std::string> configLog;
    std::string snapshotError;
};

// One zip entry, masked and capped, in memory.
struct Entry
{
    std::string name;
    std::string data;
    FILETIME mtime {};
};

const char* RuntimeText(NeuralRuntime r)
{
    switch (r)
    {
    case NeuralRuntime::Daniel:
        return "danielblnc";
    case NeuralRuntime::Lmxxf:
        return "lmxxf";
    default:
        return "none";
    }
}

std::string U8(const fs::path& p) { return wstring_to_string(p.wstring()); }

std::string LocalTimeText(FILETIME utc)
{
    FILETIME local {};
    SYSTEMTIME st {};
    if (!FileTimeToLocalFileTime(&utc, &local) || !FileTimeToSystemTime(&local, &st))
        return "?";
    return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
                       st.wSecond);
}

// "UTC+03:00": local time minus UTC at `utc`.
std::string UtcOffsetText(FILETIME utc)
{
    FILETIME local {};
    if (!FileTimeToLocalFileTime(&utc, &local))
        return "UTC?";
    const auto u = (static_cast<int64_t>(utc.dwHighDateTime) << 32) | utc.dwLowDateTime;
    const auto l = (static_cast<int64_t>(local.dwHighDateTime) << 32) | local.dwLowDateTime;
    const int64_t minutes = (l - u) / (10'000'000LL * 60);
    const int64_t a = minutes < 0 ? -minutes : minutes;
    return std::format("UTC{}{:02}:{:02}", minutes < 0 ? '-' : '+', a / 60, a % 60);
}

std::string StampForName(FILETIME utc)
{
    FILETIME local {};
    SYSTEMTIME st {};
    if (!FileTimeToLocalFileTime(&utc, &local) || !FileTimeToSystemTime(&local, &st))
        return "unknown";
    return std::format("{:04}{:02}{:02}-{:02}{:02}{:02}", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
                       st.wSecond);
}

std::string SizeText(uint64_t bytes)
{
    if (bytes >= 10 * kMiB)
        return std::format("{} MB", bytes / kMiB);
    if (bytes >= 10 * kKiB)
        return std::format("{} KB", bytes / kKiB);
    return std::format("{} B", bytes);
}

bool FileInfo(const fs::path& p, uint64_t& size, FILETIME& mtime)
{
    WIN32_FILE_ATTRIBUTE_DATA a {};
    if (!GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &a) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return false;
    size = (static_cast<uint64_t>(a.nFileSizeHigh) << 32) | a.nFileSizeLow;
    mtime = a.ftLastWriteTime;
    return true;
}

bool IsDirectory(const fs::path& p)
{
    const DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

bool SamePath(const fs::path& a, const fs::path& b)
{
    std::error_code ec;
    if (a.empty() || b.empty())
        return false;
    const bool same = fs::equivalent(a, b, ec);
    if (!ec)
        return same;
    return Util::ToLower(a.lexically_normal().wstring()) == Util::ToLower(b.lexically_normal().wstring());
}

// A file that another process may be writing: shared read, delete allowed.
class SharedFile
{
  public:
    explicit SharedFile(const fs::path& p)
    {
        h_ = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        LARGE_INTEGER s {};
        if (h_ != INVALID_HANDLE_VALUE && GetFileSizeEx(h_, &s))
            size_ = static_cast<uint64_t>(s.QuadPart);
        FILETIME created {}, accessed {};
        if (h_ != INVALID_HANDLE_VALUE)
            GetFileTime(h_, &created, &accessed, &mtime_);
    }
    ~SharedFile()
    {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }
    SharedFile(const SharedFile&) = delete;
    SharedFile& operator=(const SharedFile&) = delete;

    bool Ok() const { return h_ != INVALID_HANDLE_VALUE; }
    uint64_t Size() const { return size_; }
    FILETIME MTime() const { return mtime_; }

    // Appends up to `length` bytes from `offset`; false on a read error.
    bool Read(uint64_t offset, size_t length, std::string& out) const
    {
        LARGE_INTEGER at {};
        at.QuadPart = static_cast<LONGLONG>(offset);
        if (!SetFilePointerEx(h_, at, nullptr, FILE_BEGIN))
            return false;
        const size_t start = out.size();
        out.resize(start + length);
        size_t done = 0;
        while (done < length)
        {
            DWORD got = 0;
            const size_t left = length - done;
            const DWORD chunk = static_cast<DWORD>(left > 4 * kMiB ? 4 * kMiB : left);
            if (!ReadFile(h_, out.data() + start + done, chunk, &got, nullptr))
            {
                out.resize(start + done);
                return false;
            }
            if (got == 0)
                break; // the file shrank since the size was read
            done += got;
        }
        out.resize(start + done);
        return true;
    }

  private:
    HANDLE h_ = INVALID_HANDLE_VALUE;
    uint64_t size_ = 0;
    FILETIME mtime_ {};
};

// First 16 hex digits of the file's SHA-256 (BCrypt, streamed in 1 MB blocks), or "" when it cannot be read.
std::string Sha256Prefix(const fs::path& p)
{
    SharedFile f(p);
    if (!f.Ok() || f.Size() > kHashLimit)
        return {};
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return {};
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::string result;
    if (BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) >= 0)
    {
        bool ok = true;
        std::string block;
        for (uint64_t offset = 0; ok && offset < f.Size(); offset += kMiB)
        {
            block.clear();
            const uint64_t left = f.Size() - offset;
            ok = f.Read(offset, static_cast<size_t>(left > kMiB ? kMiB : left), block) && !block.empty();
            // BCryptHashData on an empty buffer is not needed (and a null one is an invalid parameter)
            if (ok)
                ok = BCryptHashData(hash, reinterpret_cast<PUCHAR>(block.data()), static_cast<ULONG>(block.size()),
                                    0) >= 0;
        }
        UCHAR digest[32] {};
        if (ok && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0)
        {
            for (int i = 0; i < 8; ++i)
                result += std::format("{:02x}", digest[i]);
        }
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(alg, 0);
    return result;
}

// The profile folder, the account name and the computer name, for masking (UTF-8).
struct Masker
{
    std::string profile, user, computer;

    Masker()
    {
        wchar_t buffer[MAX_PATH + 1] {};
        DWORD n = GetEnvironmentVariableW(L"USERPROFILE", buffer, MAX_PATH);
        if (n > 0 && n < MAX_PATH)
            profile = wstring_to_string(buffer);
        else if (SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, buffer) == S_OK)
            profile = wstring_to_string(buffer);

        wchar_t name[257] {};
        DWORD len = 257;
        if (GetUserNameW(name, &len))
            user = wstring_to_string(name);
        // Generic account names hide nothing and would turn common words of the logs into "<user>".
        static constexpr std::string_view kGeneric[] = { "user",  "admin", "administrator", "guest", "default",
                                                         "public", "owner", "gamer",        "pc",    "steamuser" };
        bool generic = user.size() < 3;
        for (std::string_view g : kGeneric)
            generic = generic || detail::EqualsIgnoringAsciiCase(user, g);
        if (generic)
            user.clear();

        wchar_t pc[MAX_COMPUTERNAME_LENGTH + 1] {};
        DWORD pcLen = MAX_COMPUTERNAME_LENGTH + 1;
        if (GetComputerNameW(pc, &pcLen))
            computer = wstring_to_string(pc);
        if (computer.size() < 3)
            computer.clear();
    }

    std::string operator()(std::string_view text) const
    {
        std::string out = MaskUserProfile(text, profile);
        out = MaskUserFolders(out);
        if (!user.empty())
            out = MaskWord(out, user, "<user>");
        if (!computer.empty())
            out = MaskWord(out, computer, "<pc>");
        return out;
    }
};

Snapshot TakeSnapshot()
{
    Snapshot s;
    s.tick = GetTickCount64();
    GetSystemTimeAsFileTime(&s.madeUtc);
    s.pid = GetCurrentProcessId();
    s.product = DlssNr::AmdBridge::ProductName();
    try
    {
        s.exePath = Util::ExePath();
        s.dllPath = Util::DllPath();

        auto* config = Config::Instance();
        s.logFile = fs::path(config->LogFileName.value_or_default());
        s.logToFile = config->LogToFile.value_or_default();
        s.logSingleFile = config->LogSingleFile.value_or_default();
        if (config->MainDllPath.has_value())
            s.mainDllDir = fs::path(config->MainDllPath.value());
        s.nrEnabled = config->DlssNrEnabled.value_or_default();
        s.configLog = config->GetConfigLog();
        s.fgInput = std::string(magic_enum::enum_name(config->FGInput.value_or_default()));
        s.fgOutput = std::string(magic_enum::enum_name(config->FGOutput.value_or_default()));
        s.fgEnabled = config->FGEnabled.value_or_default();

        auto& state = State::Instance();
        s.exe = state.gameExe;
        s.gameName = state.gameName;
        s.gameVersion = state.gameVersion;
        s.engine = std::string(magic_enum::enum_name(state.gameEngine));
        s.api = std::string(magic_enum::enum_name(state.api));
        s.swapchainApi = std::string(magic_enum::enum_name(state.swapchainApi));
        s.wine = state.isRunningOnLinux;
        s.fgInputActive = std::string(magic_enum::enum_name(state.activeFgInput));
        s.fgOutputActive = std::string(magic_enum::enum_name(state.activeFgOutput));
        s.rrFallbackReason = state.rrFallbackReason;
        if (IFeature* f = state.currentFeature)
        {
            s.feature = f->Name();
            s.renderW = f->RenderWidth();
            s.renderH = f->RenderHeight();
            s.displayW = f->DisplayWidth();
            s.displayH = f->DisplayHeight();
            s.targetW = f->TargetWidth();
            s.targetH = f->TargetHeight();
        }

        namespace Bridge = DlssNr::AmdBridge;
        s.chosen = Bridge::ChosenRuntime();
        s.active = Bridge::ActiveRuntime();
        if (const char* name = Bridge::RuntimeName())
            s.runtimeName = name;
        s.hasFiles = Bridge::HasFiles();
        s.danielWeights = Bridge::DanielWeightsPresent();
        s.lmxxfReady = Bridge::LmxxfReady();
        s.vkPending = Bridge::LmxxfVkLaunchPending();
        s.backendActive = Bridge::BackendActive();
        s.runtimeStopped = Bridge::RuntimeStopped();
        s.status = Bridge::Status();
        s.compositionNote = Bridge::CompositionNote();
        s.foreignStandalone = Bridge::ForeignStandalone();
        s.stats = Bridge::Stats();
        s.settings = Bridge::CurrentSettings();
        s.gpu = Bridge::GpuSupportInfo();
        s.gpus = IdentifyGpu::getAllGpus();
    }
    catch (const std::exception& e)
    {
        s.snapshotError = e.what();
    }
    catch (...)
    {
        s.snapshotError = "unknown exception";
    }
    if (s.exe.empty())
        s.exe = U8(s.exePath.filename());
    return s;
}

// Reads a log: the first `head` and last `tail` bytes (a line marks the cut). With `marker`, only the last
// `sessions` sessions of the last kSessionWindow bytes count, then the same cap applies.
struct ReadLog
{
    bool found = false;
    uint64_t size = 0;
    FILETIME mtime {};
    std::string text;
    bool capped = false;
};

ReadLog ReadCapped(const fs::path& p, size_t head, size_t tail, std::string_view marker = {}, size_t sessions = 0)
{
    ReadLog r;
    SharedFile f(p);
    if (!f.Ok())
        return r;
    r.found = true;
    r.size = f.Size();
    r.mtime = f.MTime();
    if (!marker.empty())
    {
        const uint64_t window = r.size > kSessionWindow ? kSessionWindow : r.size;
        std::string all;
        f.Read(r.size - window, static_cast<size_t>(window), all);
        size_t omitted = 0;
        const std::string_view last = LastSessions(all, marker, sessions, omitted);
        std::string text;
        if (r.size > window || omitted > 0)
            text = std::format("[... {} bytes of older sessions omitted ...]\n", r.size - window + omitted);
        text += CapHeadTail(last, head, tail);
        r.capped = last.size() > head + tail || r.size > window || omitted > 0;
        r.text = std::move(text);
        return r;
    }
    if (r.size <= head + tail)
    {
        f.Read(0, static_cast<size_t>(r.size), r.text);
        return r;
    }
    // Head and tail only (a multi-GB log is never read whole), each cut at a line break near the cut, as CapHeadTail.
    std::string headText, tailText;
    f.Read(0, head, headText);
    if (tail > 0)
        f.Read(r.size - tail, tail, tailText);
    size_t headKeep = headText.size();
    if (const size_t nl = headText.rfind('\n'); nl != std::string::npos && nl + 1 >= head / 2)
        headKeep = nl + 1;
    size_t tailSkip = 0;
    if (const size_t nl = tailText.find('\n'); nl != std::string::npos && nl + 1 <= tail / 2)
        tailSkip = nl + 1;
    const uint64_t kept = headKeep + (tailText.size() - tailSkip);
    const uint64_t omitted = r.size > kept ? r.size - kept : 0; // 0 if the file shrank while it was read
    r.text = headText.substr(0, headKeep);
    if (!r.text.empty() && r.text.back() != '\n')
        r.text.push_back('\n');
    r.text += std::format("[... {} bytes omitted ...]\n", omitted);
    r.text.append(tailText, tailSkip);
    r.capped = true;
    return r;
}

// Entries in priority order, within kBudget; what does not fit or is capped goes to the "Not included" list.
class Collector
{
  public:
    explicit Collector(const Masker& mask) : mask_(mask) {}

    void AddLog(const fs::path& p, const std::string& zipName, size_t head, size_t tail, std::string_view marker = {},
                size_t sessions = 0)
    {
        if (HaveName(zipName))
            return;
        ReadLog r = ReadCapped(p, head, tail, marker, sessions);
        if (!r.found)
            return;
        AddText(zipName, std::move(r.text), r.mtime, r.size, r.capped);
    }

    void AddText(const std::string& zipName, std::string text, FILETIME mtime, uint64_t originalSize, bool capped)
    {
        std::string masked = mask_(text);
        if (used_ + masked.size() + kReportReserve > kBudget)
        {
            notIncluded_.push_back(std::format("{} ({}): left out, the zip would pass its size limit", zipName,
                                               SizeText(originalSize)));
            return;
        }
        if (capped)
            notIncluded_.push_back(std::format("{}: capped, {} of {} kept", zipName, SizeText(masked.size()),
                                               SizeText(originalSize)));
        used_ += masked.size();
        entries_.push_back({ zipName, std::move(masked), mtime });
    }

    bool HaveName(const std::string& zipName) const
    {
        for (const Entry& e : entries_)
            if (detail::EqualsIgnoringAsciiCase(e.name, zipName))
                return true;
        return false;
    }

    std::vector<Entry>& Entries() { return entries_; }
    std::vector<std::string>& NotIncluded() { return notIncluded_; }

  private:
    const Masker& mask_;
    std::vector<Entry> entries_;
    std::vector<std::string> notIncluded_;
    size_t used_ = 0;
};

bool EndsWithI(const std::wstring& s, std::wstring_view suffix)
{
    if (s.size() < suffix.size())
        return false;
    return Util::ToLower(s.substr(s.size() - suffix.size())) == Util::ToLower(std::wstring(suffix));
}

bool StartsWithI(const std::wstring& s, std::wstring_view prefix)
{
    if (s.size() < prefix.size())
        return false;
    return Util::ToLower(s.substr(0, prefix.size())) == Util::ToLower(std::wstring(prefix));
}

struct DirFile
{
    fs::path path;
    uint64_t size = 0;
    FILETIME mtime {};
};

// Regular files of one folder (not recursive) whose name passes `match`, newest first.
template <typename Match> std::vector<DirFile> FilesIn(const fs::path& dir, Match match)
{
    std::vector<DirFile> out;
    std::error_code ec;
    if (dir.empty() || !IsDirectory(dir))
        return out;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
    {
        const fs::path p = it->path();
        if (!match(p.filename().wstring()))
            continue;
        DirFile f;
        f.path = p;
        if (FileInfo(p, f.size, f.mtime))
            out.push_back(std::move(f));
    }
    std::sort(out.begin(), out.end(),
              [](const DirFile& a, const DirFile& b) { return CompareFileTime(&a.mtime, &b.mtime) > 0; });
    return out;
}

std::string FileLine(const fs::path& p, const std::string& label, bool hash)
{
    uint64_t size = 0;
    FILETIME mtime {};
    if (!FileInfo(p, size, mtime))
        return std::format("  {}: not present\n", label);
    std::string line = std::format("  {}: {} bytes, {}", label, size, LocalTimeText(mtime));
    if (hash)
    {
        if (size > kHashLimit)
            line += ", not hashed (large)";
        else if (const std::string h = Sha256Prefix(p); !h.empty())
            line += ", sha256 " + h;
    }
    return line + "\n";
}

void AppendSettings(std::string& out, const AmdPreSr::Settings& s)
{
#define AMDNR_KV(field) out += std::format("    " #field "={}\n", s.field)
    AMDNR_KV(encoding);
    AMDNR_KV(everyFrame);
    AMDNR_KV(slots);
    AMDNR_KV(toneChannels);
    AMDNR_KV(modelScale);
    AMDNR_KV(sizeStep);
    AMDNR_KV(stability);
    AMDNR_KV(stabilityMode);
    AMDNR_KV(stabilityThreshold);
    AMDNR_KV(stabilityStaticRelax);
    AMDNR_KV(detail);
    AMDNR_KV(colour);
    AMDNR_KV(sharpness);
    AMDNR_KV(interleave);
    AMDNR_KV(interleaveFill);
    AMDNR_KV(interleaveSharp);
    AMDNR_KV(interleavePreset);
    AMDNR_KV(interleavePacing);
    AMDNR_KV(interleaveGhostBound);
    AMDNR_KV(interleaveFreshHistory);
    AMDNR_KV(interleaveModelHistory);
    AMDNR_KV(interleaveAdaptive);
    AMDNR_KV(interleaveAdaptiveSensitivity);
    AMDNR_KV(proxy);
    AMDNR_KV(lmxxfHistory);
    AMDNR_KV(lmxxfFullNetwork);
    AMDNR_KV(lmxxfEditDetail);
    AMDNR_KV(lmxxfEditSaturation);
    AMDNR_KV(lmxxfEdgeGuard);
    AMDNR_KV(lmxxfOutputSmooth);
    AMDNR_KV(lmxxfAutoExposure);
    AMDNR_KV(lmxxfCpuWait);
    AMDNR_KV(vulkanBridge);
    AMDNR_KV(lmxxfNoMotion);
    AMDNR_KV(jitterSign);
    AMDNR_KV(networkOutput);
    AMDNR_KV(residualIntensity);
    AMDNR_KV(residualLimit);
    AMDNR_KV(residualFade);
    AMDNR_KV(residualTemporal);
    AMDNR_KV(editShaper);
    AMDNR_KV(editShaperLimit);
    AMDNR_KV(editShaperBelowOnly);
    AMDNR_KV(editShaperCarryCap);
    AMDNR_KV(passes);
    AMDNR_KV(graphicsWait);
    AMDNR_KV(tone);
    AMDNR_KV(structure);
    AMDNR_KV(skin);
    AMDNR_KV(composition);
    AMDNR_KV(composeDetail);
    AMDNR_KV(composeColour);
    AMDNR_KV(maxRatio);
    AMDNR_KV(skinProtection);
    AMDNR_KV(danielHighlightGuard);
    AMDNR_KV(autoMask);
    AMDNR_KV(runtimeStyle);
    AMDNR_KV(toneCurve);
    AMDNR_KV(toneLift);
    AMDNR_KV(useGameExposure);
    AMDNR_KV(lmxxfTierSnap);
#undef AMDNR_KV
}

std::string BuildReportText(const Snapshot& s, const std::vector<std::string>& fileLines,
                            const std::vector<std::string>& conflicts, std::vector<std::string>& notIncluded,
                            const std::vector<Entry>& entries)
{
    std::string r;
    r += s.product + "\n";
    r += "AMDNR report, made by the in-game \"Save report\" button\n";
    r += std::format("Made: {} local ({}), tick {}\n", LocalTimeText(s.madeUtc), UtcOffsetText(s.madeUtc), s.tick);
    r += "User names and the computer name are masked. Nothing was uploaded.\n";
    if (!s.snapshotError.empty())
        r += "Snapshot incomplete: " + s.snapshotError + "\n";

    r += "\n[Game]\n";
    r += std::format("  folder: {}\n", U8(s.exePath.parent_path()));
    if (!SamePath(s.exePath.parent_path(), s.dllPath.parent_path()))
        r += std::format("  OptiScaler folder: {}\n", U8(s.dllPath.parent_path()));
    r += std::format("  exe: {}  pid {}\n", s.exe, s.pid);
    if (!s.gameName.empty() || !s.gameVersion.empty())
        r += std::format("  name: {}  version: {}\n", s.gameName, s.gameVersion);
    r += std::format("  engine: {}  api: {}  swapchain api: {}\n", s.engine, s.api, s.swapchainApi);
    r += std::format("  proxy: {}\n", U8(s.dllPath.filename()));
    {
        uint64_t logSize = 0;
        FILETIME logTime {};
        const bool logThere = !s.logFile.empty() && FileInfo(s.logFile, logSize, logTime);
        std::string logNote;
        if (!logThere)
            logNote = ", no such file";
        else if (!s.logToFile)
            logNote = ", the file there is from " + LocalTimeText(logTime) + " (zipped as *.NOT-THIS-SESSION.log)";
        r += std::format("  {} written this session: {}{}\n",
                         s.logFile.empty() ? std::string("log") : U8(s.logFile.filename()),
                         s.logToFile ? "yes" : "no ([Log] LogToFile=false)", logNote);
    }
    r += std::format("  Wine/Proton: {}\n", s.wine ? "yes" : "no");
    // (0.3.4.1) The translation layers, as IdentifyGpu found them on the primary adapter (the first in s.gpus)
    r += std::format("  vkd3d-proton (D3D12): {}\n",
                     !s.gpus.empty() && s.gpus.front().usesVkd3dProton ? "yes" : "no");
    r += std::format("  DXVK: {}\n", !s.gpus.empty() && s.gpus.front().usesDxvk ? "yes" : "no");
    OSVERSIONINFOW winVer {};
    if (Util::GetRealWindowsVersion(winVer))
        r += std::format("  Windows: {} ({}.{}.{})\n", Util::GetWindowsName(winVer), winVer.dwMajorVersion,
                         winVer.dwMinorVersion, winVer.dwBuildNumber);

    r += "\n[GPU]\n";
    for (const GpuInformation& g : s.gpus)
    {
        r += std::format("  {}: VEN {:04X} DEV {:04X} SUBSYS {:08X} REV {:02X}, VRAM {} MB{}\n", g.name,
                         static_cast<uint32_t>(g.vendorId), g.deviceId, g.subsystemId, g.revisionId,
                         g.dedicatedVramInBytes / kMiB, g.softwareAdapter ? ", software" : "");
        if (!g.driverStore.empty())
            r += std::format("    driver package: {}\n", U8(g.driverStore.filename()));
    }
    r += std::format("  NR runs on: {} ({}), danielblnc {}, lmxxf {}\n", s.gpu.name.empty() ? "?" : s.gpu.name,
                     s.gpu.target.empty() ? "?" : s.gpu.target, s.gpu.danielOk ? "yes" : "no",
                     s.gpu.lmxxfOk ? "yes" : "no");
    if (!s.gpu.note.empty())
        r += "  note: " + s.gpu.note + "\n";
    wchar_t system[MAX_PATH] {};
    if (GetSystemDirectoryW(system, MAX_PATH) > 0)
    {
        r += FileLine(fs::path(system) / L"amdhip64_7.dll", "System32 amdhip64_7.dll", false);
        r += FileLine(fs::path(system) / L"amdhip64_6.dll", "System32 amdhip64_6.dll", false);
    }

    r += "\n[AMDNR]\n";
    r += std::format("  NR enabled: {}\n", s.nrEnabled ? "yes" : "no");
    r += std::format("  runtime: chosen {}, built {}{}\n", RuntimeText(s.chosen), RuntimeText(s.active),
                     s.runtimeStopped ? " (stopped for this session)" : "");
    r += std::format("  danielblnc files: {}, weights: {}, build: {}\n", s.hasFiles ? "yes" : "no",
                     s.danielWeights ? "yes" : "no", s.runtimeName.empty() ? "unknown" : s.runtimeName);
    r += std::format("  lmxxf ready: {}, Vulkan launch marker: {}\n", s.lmxxfReady ? "yes" : "no",
                     s.vkPending ? "present" : "no");
    r += std::format("  backend active: {}\n", s.backendActive ? "yes" : "no");
    r += "  status: " + (s.status.empty() ? std::string("-") : s.status) + "\n";
    if (!s.compositionNote.empty())
        r += "  composition: " + s.compositionNote + "\n";
    if (!s.foreignStandalone.empty())
        r += "  standalone check: " + s.foreignStandalone + "\n";
    r += std::format("  stats: recorded {}, model frames {}, skips {}, working size {}x{}, interleaving {}, first "
                     "model frame {}, NR GPU ms {}\n",
                     s.stats.recorded, s.stats.modelFrames, s.stats.skips, s.stats.width, s.stats.height,
                     s.stats.interleaving ? "yes" : "no", s.stats.modelFrameSeen ? "yes" : "no", s.stats.nrGpuMs);
    r += "  settings the bridge hands the backend:\n";
    AppendSettings(r, s.settings);

    r += "\n[Upscaler and frame generation]\n";
    if (!s.feature.empty())
        r += std::format("  upscaler: {}, render {}x{} -> output {}x{} (target {}x{})\n", s.feature, s.renderW,
                         s.renderH, s.displayW, s.displayH, s.targetW, s.targetH);
    else
        r += "  upscaler: none created yet\n";
    r += std::format("  FG input: {} (active {}), FG output: {} (active {}), FG enabled: {}\n", s.fgInput,
                     s.fgInputActive, s.fgOutput, s.fgOutputActive, s.fgEnabled ? "yes" : "no");
    r += "  Ray Regeneration fallback: " + (s.rrFallbackReason.empty() ? std::string("none") : s.rrFallbackReason) +
         "\n";

    r += "\n[Ini values read at start]\n";
    if (s.configLog.empty())
        r += "  (none)\n";
    for (const std::string& line : s.configLog)
        r += "  " + line + "\n";

    r += "\n[Files]\n";
    for (const std::string& line : fileLines)
        r += line;

    r += "\n[Possible conflicts in the game folder]\n";
    if (conflicts.empty())
        r += "  none found\n";
    for (const std::string& line : conflicts)
        r += line;

    r += "\n[Not included]\n";
    if (notIncluded.empty())
        r += "  nothing\n";
    for (const std::string& line : notIncluded)
        r += "  " + line + "\n";

    r += "\n[Zip contents]\n  report.txt\n";
    for (const Entry& e : entries)
        r += std::format("  {} ({})\n", e.name, SizeText(e.data.size()));
    return r;
}

// Writes the zip into `dir`; the full path on success, else "" and `error` says why.
std::wstring WriteZip(const fs::path& dir, const std::string& stem, const std::string& stamp,
                      const std::vector<Entry>& entries, std::string& error)
{
    if (dir.empty() || !IsDirectory(dir))
    {
        error = "folder not found";
        return {};
    }
    // Leftovers of a save that died (older than 10 minutes, so a save running in another process is never touched)
    FILETIME nowFt {};
    GetSystemTimeAsFileTime(&nowFt);
    const auto now = (static_cast<uint64_t>(nowFt.dwHighDateTime) << 32) | nowFt.dwLowDateTime;
    for (const DirFile& f : FilesIn(dir, [](const std::wstring& n)
                                    { return StartsWithI(n, L"AMDNR-report-") && EndsWithI(n, L".zip.tmp"); }))
    {
        const auto t = (static_cast<uint64_t>(f.mtime.dwHighDateTime) << 32) | f.mtime.dwLowDateTime;
        if (now > t && now - t > 10ull * 60 * 10'000'000)
            DeleteFileW(f.path.c_str());
    }

    const std::wstring base = string_to_wstring("AMDNR-report-" + stem + "-" + stamp);
    fs::path final = dir / (base + L".zip");
    for (int n = 2; n < 100 && GetFileAttributesW(final.c_str()) != INVALID_FILE_ATTRIBUTES; ++n)
        final = dir / (base + L"-" + std::to_wstring(n) + L".zip");
    const fs::path tmp = fs::path(final.wstring() + L".tmp");

    ReportZip::Writer zip;
    bool ok = zip.Open(tmp.wstring());
    for (const Entry& e : entries)
        ok = ok && zip.Add(e.name, reinterpret_cast<const uint8_t*>(e.data.data()), e.data.size(), e.mtime);
    ok = zip.Close() && ok;
    if (ok && MoveFileExW(tmp.c_str(), final.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return final.wstring();
    error = std::format("could not write here (error {})", GetLastError());
    DeleteFileW(tmp.c_str());
    return {};
}

void Publish(bool ok, std::string text)
{
    {
        std::scoped_lock lock(g_resultMutex);
        g_last.done = true;
        g_last.ok = ok;
        g_last.text = std::move(text);
    }
    g_running.store(false, std::memory_order_release);
}

// What Work did; logged and published by the caller at normal priority.
struct Outcome
{
    bool ok = false;
    std::string text;   // the zip's path, or why it failed
    size_t entries = 0; // entries in the zip
};

// The disk work (background priority, see SaveAsync). No LOG_* in here: spdlog's sink mutex, held by a
// background-priority thread, would make the render thread's next log line wait on it.
Outcome Work(const Snapshot& s)
{
    const Masker mask;
    Collector c(mask);

    const fs::path dllDir = s.dllPath.parent_path();
    const fs::path exeDir = s.exePath.parent_path();
    fs::path logDir = s.logFile.parent_path();
    if (logDir.empty())
        logDir = dllDir;
    const std::wstring logStem = s.logFile.stem().wstring();

    // 1. OptiScaler.log of this session, then the previous generations (W1-B's rotation: <stem>.previous*.log).
    // [Log] LogToFile=false writes no log: a file of that name is from an earlier session and is named so in the zip.
    if (!s.logFile.empty())
        c.AddLog(s.logFile,
                 "logs/" + (s.logToFile ? U8(s.logFile.filename())
                                        : U8(s.logFile.stem()) + ".NOT-THIS-SESSION" + U8(s.logFile.extension())),
                 256 * kKiB, 2 * kMiB);
    int generation = 0;
    for (const DirFile& f : FilesIn(logDir, [&](const std::wstring& n)
                                    { return StartsWithI(n, logStem + L".previous") && EndsWithI(n, L".log"); }))
    {
        if (generation >= 5)
            break;
        const bool newest = generation++ == 0;
        c.AddLog(f.path, "logs/" + U8(f.path.filename()), newest ? 128 * kKiB : 32 * kKiB,
                 newest ? kMiB : 224 * kKiB);
    }
    if (!s.logSingleFile) // SingleFile=false: per-session names <stem>_<ticks>.log; the newest other one
    {
        // LogFileName already holds this session's <stem>_<ticks>.log (Config): drop the _<ticks> for the pattern.
        std::wstring baseStem = logStem;
        if (const size_t cut = baseStem.rfind(L'_'); cut != std::wstring::npos && cut + 1 < baseStem.size() &&
                                                      std::all_of(baseStem.begin() + cut + 1, baseStem.end(),
                                                                  [](wchar_t ch) { return ch >= L'0' && ch <= L'9'; }))
            baseStem.resize(cut);
        for (const DirFile& f : FilesIn(logDir, [&](const std::wstring& n)
                                        { return StartsWithI(n, baseStem + L"_") && EndsWithI(n, L".log"); }))
        {
            if (SamePath(f.path, s.logFile))
                continue;
            c.AddLog(f.path, "logs/" + U8(f.path.filename()), 128 * kKiB, kMiB);
            break;
        }
    }

    // 2. The AMD logs (beside OptiScaler.dll): the last 3 sessions each
    static constexpr std::pair<const wchar_t*, const char*> kAmdLogs[] = {
        { L"amd_bridge.log", kSessionMarker },
        { L"amd_presr.log", kSessionMarker },
        { L"lmxxf_backend.log", kSessionMarker },
        { L"dlssnr_on_amd.log", "loaded into" },
    };
    for (const auto& [name, marker] : kAmdLogs)
    {
        const bool daniel = std::wstring_view(name) == L"dlssnr_on_amd.log";
        c.AddLog(dllDir / name, "logs/" + wstring_to_string(name), 128 * kKiB, daniel ? 384 * kKiB : 640 * kKiB,
                 marker, 3);
    }
    c.AddLog(logDir / L"amdnr_crash.log", "logs/amdnr_crash.log", 0, 256 * kKiB);

    // RE Engine loads OptiScaler from _storage_: the game folder may hold older copies of the AMD logs
    if (!SamePath(exeDir, dllDir))
    {
        for (const auto& [name, marker] : kAmdLogs)
            c.AddLog(exeDir / name, "logs/game-folder/" + wstring_to_string(name), 32 * kKiB, 96 * kKiB, marker, 1);
    }

    // 3. Settings
    c.AddLog(dllDir / L"OptiScaler.ini", "config/OptiScaler.ini", 256 * kKiB, 0);
    c.AddLog(dllDir / L"dlssnr_on_amd.ini", "config/dlssnr_on_amd.ini", 64 * kKiB, 0);
    if (!SamePath(exeDir, dllDir))
        c.AddLog(exeDir / L"OptiScaler.ini", "config/game-folder/OptiScaler.ini", 256 * kKiB, 0);

    // 4. Files: the AMDNR set and OptiScaler's own DLLs
    std::vector<std::string> fileLines;
    fileLines.push_back(FileLine(s.dllPath, "proxy " + U8(s.dllPath.filename()), true));
    static constexpr const wchar_t* kFiles[] = {
        L"OptiScaler.ini",        L"nvngx.dll_dlssnr.dll",       L"dlssnr_amd_pass1.dll",
        L"dlssnr_amd_pass2.dll",  L"dlssnr_amd_pass3.dll",       L"dlssnr_on_amd_weights.bin",
        L"dlssnr_on_amd.ini",     L"LmxxfNrRuntime.dll",         L"LmxxfNrRuntime.pak",
        L"lmxxf_vk_launch.pending",
    };
    for (const wchar_t* name : kFiles)
        fileLines.push_back(FileLine(dllDir / name, wstring_to_string(name), true));
    const fs::path labDir = dllDir / L"DLSS5-AMD";
    fileLines.push_back(std::format("  DLSS5-AMD folder: {}\n", IsDirectory(labDir) ? "present" : "not present"));
    if (IsDirectory(labDir))
        fileLines.push_back(FileLine(labDir / L"LmxxfNrRuntime.dll", "DLSS5-AMD\\LmxxfNrRuntime.dll", true));
    if (!s.mainDllDir.empty() && !SamePath(s.mainDllDir, dllDir))
    {
        for (const DirFile& f : FilesIn(s.mainDllDir, [](const std::wstring& n) { return EndsWithI(n, L".dll"); }))
            fileLines.push_back(FileLine(f.path, U8(s.mainDllDir.filename() / f.path.filename()), true));
    }

    // 5. Other mods and proxies beside the game; game crash dumps are listed only (they hold process memory)
    std::vector<std::string> conflicts;
    std::vector<fs::path> dirs { exeDir };
    if (!SamePath(exeDir, dllDir))
        dirs.push_back(dllDir);
    static constexpr std::wstring_view kProxyNames[] = { L"dxgi.dll",    L"d3d12.dll",   L"d3d11.dll",
                                                         L"winmm.dll",   L"version.dll", L"dbghelp.dll",
                                                         L"winhttp.dll", L"wininet.dll", L"dinput8.dll",
                                                         L"reshade.ini" };
    for (const fs::path& dir : dirs)
    {
        const std::string where = SamePath(dir, exeDir) ? "" : "OptiScaler folder: ";
        for (const DirFile& f : FilesIn(dir,
                                        [](const std::wstring& n)
                                        {
                                            for (std::wstring_view p : kProxyNames)
                                                if (Util::ToLower(n) == p)
                                                    return true;
                                            return EndsWithI(n, L".asi") || StartsWithI(n, L"nvngx_dlss") ||
                                                   (StartsWithI(n, L"sl.") && EndsWithI(n, L".dll"));
                                        }))
        {
            const bool self = SamePath(f.path, s.dllPath);
            conflicts.push_back(std::format("  {}{}: {} bytes, {}{}\n", where, U8(f.path.filename()), f.size,
                                            LocalTimeText(f.mtime), self ? " (this OptiScaler)" : ""));
        }
        for (const DirFile& f : FilesIn(dir, [](const std::wstring& n) { return EndsWithI(n, L".dmp"); }))
            c.NotIncluded().push_back(std::format("{}crash dump {}: {} bytes, {}", where, U8(f.path.filename()),
                                                  f.size, LocalTimeText(f.mtime)));
    }

    // 6. report.txt first in the zip, masked like every entry
    std::string report = BuildReportText(s, fileLines, conflicts, c.NotIncluded(), c.Entries());
    std::vector<Entry> entries;
    entries.reserve(c.Entries().size() + 1);
    entries.push_back({ "report.txt", mask(report), s.madeUtc });
    for (Entry& e : c.Entries())
        entries.push_back(std::move(e));

    // 7. The game folder, else the Desktop, else %TEMP%
    const std::string stem = SafeFileStem(U8(s.exePath.stem()));
    const std::string stamp = StampForName(s.madeUtc);
    std::vector<fs::path> targets { exeDir };
    wchar_t buffer[MAX_PATH + 1] {};
    if (SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT, buffer) == S_OK)
        targets.emplace_back(buffer);
    if (const DWORD n = GetTempPathW(MAX_PATH, buffer); n > 0 && n < MAX_PATH)
        targets.emplace_back(buffer);

    std::string errors;
    for (const fs::path& dir : targets)
    {
        std::string error;
        const std::wstring written = WriteZip(dir, stem, stamp, entries, error);
        if (!written.empty())
            return { true, wstring_to_string(written), entries.size() };
        errors += std::format("{}{}: {}", errors.empty() ? "" : "; ", mask(U8(dir)), error);
    }
    return { false, errors, entries.size() };
}

// Background priority (CPU and I/O) for the disk work only.
struct BackgroundScope
{
    bool on = false;
    BackgroundScope() { on = SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN) != FALSE; }
    ~BackgroundScope()
    {
        if (on)
            SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
    }
};
} // namespace

bool SaveAsync()
{
    bool expected = false;
    if (!g_running.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return false;

    try
    {
        Snapshot s = TakeSnapshot();
        std::thread(
            [s = std::move(s)]()
            {
                using SetDescriptionFn = HRESULT(WINAPI*)(HANDLE, PCWSTR);
                if (HMODULE k32 = GetModuleHandleW(L"kernel32.dll"))
                    if (auto fn = reinterpret_cast<SetDescriptionFn>(GetProcAddress(k32, "SetThreadDescription")))
                        fn(GetCurrentThread(), L"AMDNR report");
                // At normal priority: the flush takes spdlog's sink mutex (so does every LOG_* below).
                if (auto logger = spdlog::default_logger())
                    logger->flush();
                Outcome out;
                {
                    BackgroundScope background;
                    try
                    {
                        out = Work(s);
                    }
                    catch (const std::exception& e)
                    {
                        out = { false, e.what(), 0 };
                    }
                    catch (...)
                    {
                        out = { false, "unknown error", 0 };
                    }
                }
                if (out.ok)
                {
                    LOG_INFO("Save report: {} written ({} entries)", out.text, out.entries);
                    Publish(true, out.text);
                }
                else
                {
                    LOG_WARN("Save report failed: {}", out.text);
                    Publish(false, "The report could not be saved: " + out.text);
                }
            })
            .detach();
    }
    catch (...)
    {
        g_running.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

Result LastResult()
{
    thread_local Result cache;
    std::unique_lock lock(g_resultMutex, std::try_to_lock);
    if (lock.owns_lock())
        cache = g_last;
    Result r = cache;
    r.running = g_running.load(std::memory_order_acquire);
    return r;
}
} // namespace AmdnrReport
