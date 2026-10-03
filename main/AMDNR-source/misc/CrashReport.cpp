// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
#include "pch.h"

#include "CrashReport.h"

#include <Util.h>
#include <resource.h>
#include <proxies/KernelBase_Proxy.h>
#include <proxies/Ntdll_Proxy.h>

#include <dbghelp.h>

#include <algorithm>
#include <atomic>
#include <cwctype>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

// CRASH-b (AMDNR 0.3.4, D22): an in-process crash note, only with [Log] CrashHandler=true.
//
// Install (DllMain, after the kernel proxies are set up) prepares everything: the append handle of
// amdnr_crash.log, the exe / OptiScaler module bases and names, and with [Log] CrashDump=true two events and a
// sleeping dump writer thread. It writes one START line, then registers a vectored handler (last in the chain) and an
// unhandled-exception filter that chains to the previous one. The writer thread loads System32's dbghelp itself: it
// starts running only after DllMain has returned, so nothing new is loaded under the loader lock (it writes one
// "DUMP ready" or "DUMP unavailable" line; a fault before that gets no dump).
//
// The handlers are passive: they always pass the exception on (EXCEPTION_CONTINUE_SEARCH, or the previous filter's
// answer), so the game's own handlers (crash reporters, anti-tamper traps) behave as without them. Inside a handler
// nothing allocates, opens a file, loads a library or takes the loader lock: a line is formatted into a stack
// buffer and appended with one WriteFile on the pre-opened handle; module names come from the memory map
// (VirtualQuery) and the image's PE export name, read under SEH. Only fault codes are noted (access violation,
// illegal instruction, stack overflow, heap corruption, ...; never C++ throws or debug prints), at most 4 lines for
// any module plus 4 more for AMD-NR related modules (AMD driver, HIP, the NR runtimes, FidelityFX, OptiScaler),
// each fault site once. A site that can add nothing more (already noted, or not AMD-NR related once the 4 general
// lines are used) joins a 16-entry list of addresses: a later fault there costs those compares, so a title that raises
// handled faults from a few sites all the time pays little. Any other fault takes a spin flag, one VirtualQuery and a
// PE header read before it is judged; a title with many changing fault sites pays that on each.
// The dump (optional) is written by the writer thread while the faulting thread waits at most 10 s, once per process:
// on the first fault in an AMD-NR related module or at the unhandled filter, whichever comes first. The writer gets
// copies of the exception and context records (the faulting thread's own ones are on its stack, which it may unwind
// after the wait).
// A __fastfail (C0000409 without an exception) skips every handler; the Windows event log has those.
namespace CrashReport
{
namespace
{
namespace fs = std::filesystem;

constexpr int kAnyCap = 4;  // EXCEPTION lines for any module
constexpr int kAmdCap = 4;  // and these more, only for AMD-NR related modules
constexpr int kFrames = 12; // stack frames per line
constexpr uint64_t kTrimAbove = 512 * 1024;
constexpr size_t kTrimKeep = 256 * 1024;
constexpr DWORD kDumpWaitMs = 10000;
constexpr int kDumpsKept = 3;
constexpr int kQuietSites = 16; // fault addresses that can add no line and no dump any more

// Prepared by Install, read by the handlers.
HANDLE g_log = INVALID_HANDLE_VALUE;
PVOID g_veh = nullptr;
LPTOP_LEVEL_EXCEPTION_FILTER g_previousUef = nullptr;
bool g_uefInstalled = false;
bool g_installed = false; // DllMain only
uintptr_t g_selfBase = 0;
uintptr_t g_exeBase = 0;
char g_exeName[64] = {};
char g_selfName[96] = {};

std::atomic_flag g_busy = ATOMIC_FLAG_INIT; // one thread formats at a time; the others skip their note
std::atomic<int> g_linesAny { 0 };
std::atomic<int> g_linesAmd { 0 };
std::atomic<bool> g_unhandledWritten { false };
struct Seen
{
    DWORD code;
    uintptr_t base;
    uintptr_t offset;
};
Seen g_seen[kAnyCap + kAmdCap] = {};
int g_seenCount = 0; // under g_busy
std::atomic<uintptr_t> g_quiet[kQuietSites] = {}; // read without g_busy; 0 = empty
int g_quietNext = 0;                              // under g_busy

using MiniDumpWriteDumpFn = BOOL(WINAPI*)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
                                          PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
std::atomic<MiniDumpWriteDumpFn> g_writeDump { nullptr }; // set by the writer thread once dbghelp is loaded
HANDLE g_dumpRequest = nullptr;
HANDLE g_dumpDone = nullptr;
HANDLE g_dumpThread = nullptr;
std::atomic<bool> g_dumpTaken { false };
std::atomic<bool> g_quit { false };
EXCEPTION_POINTERS* g_dumpPointers = nullptr; // &g_dumpCopy or nullptr
EXCEPTION_RECORD g_dumpRecord {};             // copies for the writer (the originals are on the faulting stack)
CONTEXT g_dumpContext {};
EXCEPTION_POINTERS g_dumpCopy {};
DWORD g_dumpThreadId = 0;
wchar_t g_dumpPrefix[MAX_PATH] = {}; // <folder>\AMDNR-crash-<exe stem>-

// One log line in a stack buffer (no allocation), appended with one WriteFile.
struct Line
{
    char buf[512]; // small: after a stack overflow little stack is left
    size_t n = 0;

    void Put(char c)
    {
        if (n < sizeof(buf) - 2)
            buf[n++] = c;
    }
    void Str(const char* s)
    {
        if (s != nullptr)
            while (*s)
                Put(*s++);
    }
    void Wide(const wchar_t* s) // ASCII only; anything else becomes '?'
    {
        if (s != nullptr)
            for (; *s; ++s)
                Put(*s < 0x80 ? static_cast<char>(*s) : '?');
    }
    void Hex(uint64_t v, int minDigits = 1)
    {
        char t[16];
        int k = 0;
        do
        {
            t[k++] = "0123456789ABCDEF"[v & 15];
            v >>= 4;
        } while (v != 0 && k < 16);
        while (k < minDigits && k < 16)
            t[k++] = '0';
        while (k > 0)
            Put(t[--k]);
    }
    void Dec(uint64_t v)
    {
        char t[20];
        int k = 0;
        do
        {
            t[k++] = static_cast<char>('0' + v % 10);
            v /= 10;
        } while (v != 0 && k < 20);
        while (k > 0)
            Put(t[--k]);
    }
    void Two(unsigned v)
    {
        Put(static_cast<char>('0' + (v / 10) % 10));
        Put(static_cast<char>('0' + v % 10));
    }
    void Time()
    {
        SYSTEMTIME st {};
        GetLocalTime(&st);
        Dec(st.wYear);
        Put('-');
        Two(st.wMonth);
        Put('-');
        Two(st.wDay);
        Put(' ');
        Two(st.wHour);
        Put(':');
        Two(st.wMinute);
        Put(':');
        Two(st.wSecond);
        Put('.');
        Put(static_cast<char>('0' + (st.wMilliseconds / 100) % 10));
        Two(st.wMilliseconds % 100);
    }
    void Head(const char* what) // "<local time> pid=<n> <what>"
    {
        Time();
        Str(" pid=");
        Dec(GetCurrentProcessId());
        Put(' ');
        Str(what);
    }
    void Write()
    {
        buf[n++] = '\r';
        buf[n++] = '\n';
        DWORD written = 0;
        if (g_log != INVALID_HANDLE_VALUE)
            WriteFile(g_log, buf, static_cast<DWORD>(n), &written, nullptr);
        n = 0;
    }
};

enum class Kind
{
    Unknown,
    Game,
    Self,
    Driver,
    Hip,
    Daniel,
    Lmxxf,
    Ffx,
    Xess,
    D3d12,
    Other,
};

const char* KindName(Kind k)
{
    switch (k)
    {
    case Kind::Game:
        return "game";
    case Kind::Self:
        return "AMDNR/OptiScaler";
    case Kind::Driver:
        return "AMD driver";
    case Kind::Hip:
        return "HIP";
    case Kind::Daniel:
        return "NR runtime danielblnc";
    case Kind::Lmxxf:
        return "NR runtime lmxxf";
    case Kind::Ffx:
        return "FidelityFX";
    case Kind::Xess:
        return "XeSS";
    case Kind::D3d12:
        return "D3D12";
    case Kind::Other:
        return "other";
    default:
        return "no module";
    }
}

bool AmdRelated(Kind k)
{
    return k == Kind::Self || k == Kind::Driver || k == Kind::Hip || k == Kind::Daniel || k == Kind::Lmxxf ||
           k == Kind::Ffx;
}

bool StartsWithI(const char* s, const char* prefix)
{
    for (; *prefix; ++s, ++prefix)
    {
        const char a = (*s >= 'A' && *s <= 'Z') ? static_cast<char>(*s + 32) : *s;
        if (a != *prefix) // prefixes are lower case
            return false;
    }
    return true;
}

Kind KindOf(const char* name)
{
    struct Rule
    {
        const char* prefix;
        Kind kind;
    };
    static constexpr Rule kRules[] = {
        { "amdxc64", Kind::Driver },         { "amdxx64", Kind::Driver },   { "amdxn64", Kind::Driver },
        { "atidxx64", Kind::Driver },        { "amdhip64", Kind::Hip },     { "amd_comgr", Kind::Hip },
        { "dlssnr_amd_pass", Kind::Daniel }, { "lmxxfnrruntime", Kind::Lmxxf }, { "amd_fidelityfx", Kind::Ffx },
        { "libxess", Kind::Xess },           { "d3d12", Kind::D3d12 },
    };
    for (const Rule& r : kRules)
        if (StartsWithI(name, r.prefix))
            return r.kind;
    return Kind::Other;
}

// The image that holds `address`, from the memory map: no loader lock. 0 when it is not in an image.
uintptr_t ImageBaseOf(uintptr_t address)
{
    MEMORY_BASIC_INFORMATION mbi {};
    if (address == 0 || VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0 ||
        mbi.Type != MEM_IMAGE)
        return 0;
    return reinterpret_cast<uintptr_t>(mbi.AllocationBase);
}

// The PE export-directory name of the image at `base` (DLLs carry one), copied into `out`; read under SEH.
bool ExportName(uintptr_t base, char* out, size_t size)
{
    __try
    {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return false;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return false;
        const IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (dir.VirtualAddress == 0 || dir.Size == 0)
            return false;
        const auto* exp = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + dir.VirtualAddress);
        if (exp->Name == 0)
            return false;
        const char* name = reinterpret_cast<const char*>(base + exp->Name);
        size_t i = 0;
        for (; i + 1 < size && name[i] != 0; ++i)
            out[i] = name[i];
        out[i] = 0;
        return i > 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        out[0] = 0;
        return false;
    }
}

struct Where
{
    uintptr_t base = 0;
    uintptr_t offset = 0;
    char name[64] = {};
    Kind kind = Kind::Unknown;
};

void CopyName(char* out, size_t size, const char* s)
{
    size_t i = 0;
    for (; s != nullptr && i + 1 < size && s[i] != 0; ++i)
        out[i] = s[i];
    out[i] = 0;
}

void Locate(uintptr_t address, Where& w)
{
    w.base = ImageBaseOf(address);
    w.offset = w.base != 0 ? address - w.base : address;
    if (w.base == 0)
    {
        w.kind = Kind::Unknown;
        CopyName(w.name, sizeof(w.name), "?");
    }
    else if (w.base == g_selfBase)
    {
        w.kind = Kind::Self;
        CopyName(w.name, sizeof(w.name), g_selfName);
    }
    else if (w.base == g_exeBase)
    {
        w.kind = Kind::Game;
        CopyName(w.name, sizeof(w.name), g_exeName);
    }
    else if (ExportName(w.base, w.name, sizeof(w.name)))
        w.kind = KindOf(w.name);
    else
    {
        w.kind = Kind::Other;
        CopyName(w.name, sizeof(w.name), "?");
    }
}

void AppendWhere(Line& l, const Where& w)
{
    if (w.base != 0)
    {
        l.Str(w.name);
        l.Str("+0x");
    }
    else
        l.Str("0x");
    l.Hex(w.offset);
}

#if defined(_M_X64)
// One unwind step of `c` (x64 unwind data, as the OS's own dispatcher uses); false at the end or on a bad frame.
bool Unwind(CONTEXT& c)
{
    __try
    {
        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(c.Rip, &imageBase, nullptr);
        if (fn == nullptr) // a leaf function: the return address is at [rsp]
        {
            c.Rip = *reinterpret_cast<const DWORD64*>(c.Rsp);
            c.Rsp += 8;
        }
        else
        {
            PVOID handlerData = nullptr;
            DWORD64 establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, c.Rip, fn, &c, &handlerData, &establisher, nullptr);
        }
        return c.Rip != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// The fault's stack, one line per frame. Its own frame (the CONTEXT copy) is only taken when a stack is walked.
__declspec(noinline) void WriteFrames(const CONTEXT* source)
{
    CONTEXT c = *source;
    for (int i = 0; i < kFrames && c.Rip != 0; ++i)
    {
        Where w;
        Locate(static_cast<uintptr_t>(c.Rip), w);
        Line l;
        l.Str("    #");
        l.Dec(static_cast<uint64_t>(i));
        l.Put(' ');
        AppendWhere(l, w);
        l.Write();
        if (!Unwind(c))
            break;
    }
}
#endif

bool IsFault(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:        // C0000005
    case EXCEPTION_IN_PAGE_ERROR:           // C0000006
    case EXCEPTION_ILLEGAL_INSTRUCTION:     // C000001D
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: // C0000025
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:   // C000008C .. C0000096
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW: // C00000FD
    case 0xC0000374:               // heap corruption
    case 0xC0000409:               // stack buffer overrun, when raised as an exception
    case EXCEPTION_BREAKPOINT:     // 80000003: how some crash reporters signal
        return true;
    default:
        return false;
    }
}

// Faults worth the one dump when they happen in an AMD-NR related module.
bool DumpWorthy(DWORD code)
{
    return code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION ||
           code == EXCEPTION_STACK_OVERFLOW || code == 0xC0000374 || code == 0xC0000409;
}

// Writes the EXCEPTION (capped, once per site) or UNHANDLED line and its stack. Caller holds g_busy.
// True when the site can add nothing more (no line now, never a dump from OnException): not AMD-NR related and
// either already noted or past the general cap.
bool Note(EXCEPTION_POINTERS* ep, bool unhandled, Where& w)
{
    const EXCEPTION_RECORD* er = ep->ExceptionRecord;
    const DWORD code = er->ExceptionCode;
    Locate(reinterpret_cast<uintptr_t>(er->ExceptionAddress), w);

    if (!unhandled)
    {
        for (int i = 0; i < g_seenCount; ++i)
            if (g_seen[i].code == code && g_seen[i].base == w.base && g_seen[i].offset == w.offset)
                return !AmdRelated(w.kind);
        if (g_linesAny.load(std::memory_order_relaxed) < kAnyCap)
            g_linesAny.fetch_add(1, std::memory_order_relaxed);
        else if (AmdRelated(w.kind) && g_linesAmd.load(std::memory_order_relaxed) < kAmdCap)
            g_linesAmd.fetch_add(1, std::memory_order_relaxed);
        else
            return !AmdRelated(w.kind);
        if (g_seenCount < static_cast<int>(std::size(g_seen)))
            g_seen[g_seenCount++] = { code, w.base, w.offset };
    }

    Line l;
    l.Head(unhandled ? "UNHANDLED " : "EXCEPTION first-chance ");
    l.Hex(code, 8);
    if ((code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR) && er->NumberParameters >= 2)
    {
        const ULONG_PTR access = er->ExceptionInformation[0];
        l.Str(access == 0 ? " read 0x" : access == 1 ? " write 0x" : access == 8 ? " execute 0x" : " access 0x");
        l.Hex(er->ExceptionInformation[1]);
    }
    l.Str(" at ");
    AppendWhere(l, w);
    l.Str(" [");
    l.Str(KindName(w.kind));
    l.Str("] tid=");
    l.Dec(GetCurrentThreadId());
    l.Write();

#if defined(_M_X64)
    if (code != EXCEPTION_STACK_OVERFLOW && ep->ContextRecord != nullptr) // little stack is left after an overflow
        WriteFrames(ep->ContextRecord);
#endif
    return false;
}

// The one dump of this process: the writer thread writes it while this thread waits (at most kDumpWaitMs).
void RequestDump(EXCEPTION_POINTERS* ep)
{
    if (g_writeDump.load(std::memory_order_acquire) == nullptr || g_dumpRequest == nullptr ||
        g_dumpTaken.exchange(true))
        return;
    // Static copies (no allocation): after a timeout this thread resumes the dispatch while the writer may still read.
    g_dumpPointers = nullptr;
    if (ep != nullptr && ep->ExceptionRecord != nullptr && ep->ContextRecord != nullptr)
    {
        g_dumpRecord = *ep->ExceptionRecord;
        g_dumpRecord.ExceptionRecord = nullptr; // a chained record is on the faulting stack as well
        g_dumpContext = *ep->ContextRecord;
        g_dumpContext.ContextFlags &= ~static_cast<DWORD>(CONTEXT_XSTATE & 0xFFFF); // the XSAVE area is not copied
        g_dumpCopy.ExceptionRecord = &g_dumpRecord;
        g_dumpCopy.ContextRecord = &g_dumpContext;
        g_dumpPointers = &g_dumpCopy;
    }
    g_dumpThreadId = GetCurrentThreadId();
    SetEvent(g_dumpRequest);
    WaitForSingleObject(g_dumpDone, kDumpWaitMs);
}

LONG CALLBACK OnException(EXCEPTION_POINTERS* ep)
{
    if (ep == nullptr || ep->ExceptionRecord == nullptr || !IsFault(ep->ExceptionRecord->ExceptionCode))
        return EXCEPTION_CONTINUE_SEARCH;
    const uintptr_t at = reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress);
    if (at != 0)
        for (const std::atomic<uintptr_t>& quiet : g_quiet)
            if (quiet.load(std::memory_order_relaxed) == at)
                return EXCEPTION_CONTINUE_SEARCH;
    const bool linesLeft = g_linesAny.load(std::memory_order_relaxed) < kAnyCap ||
                           g_linesAmd.load(std::memory_order_relaxed) < kAmdCap;
    const bool dumpLeft = g_writeDump.load(std::memory_order_relaxed) != nullptr &&
                          !g_dumpTaken.load(std::memory_order_relaxed);
    if (!linesLeft && !dumpLeft)
        return EXCEPTION_CONTINUE_SEARCH;
    if (g_busy.test_and_set(std::memory_order_acquire)) // another thread is writing (or a fault inside our note)
        return EXCEPTION_CONTINUE_SEARCH;
    Where w;
    if (Note(ep, false, w) && at != 0)
    {
        g_quiet[g_quietNext].store(at, std::memory_order_relaxed);
        g_quietNext = (g_quietNext + 1) % kQuietSites;
    }
    g_busy.clear(std::memory_order_release);
    if (dumpLeft && AmdRelated(w.kind) && DumpWorthy(ep->ExceptionRecord->ExceptionCode))
        RequestDump(ep);
    return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI OnUnhandled(EXCEPTION_POINTERS* ep)
{
    if (ep != nullptr && ep->ExceptionRecord != nullptr && !g_unhandledWritten.exchange(true) &&
        !g_busy.test_and_set(std::memory_order_acquire))
    {
        Where w;
        Note(ep, true, w);
        g_busy.clear(std::memory_order_release);
    }
    RequestDump(ep);
    return g_previousUef != nullptr ? g_previousUef(ep) : EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI DumpWriter(LPVOID)
{
    // System32's dbghelp (OptiScaler itself may be the dbghelp.dll proxy), loaded here, not in Install: this thread
    // runs only after DllMain has returned, so dbghelp's DllMain does not run under the loader lock we hold there.
    FARPROC fn = nullptr;
    if (HMODULE dbghelp = NtdllProxy::LoadLibraryExW_Ldr(L"dbghelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
    {
        const auto getProc = KernelBaseProxy::GetProcAddress_();
        fn = getProc != nullptr ? getProc(dbghelp, "MiniDumpWriteDump")
                                : ::GetProcAddress(dbghelp, "MiniDumpWriteDump");
    }
    {
        Line l;
        l.Head(fn != nullptr ? "DUMP ready" : "DUMP unavailable (System32 dbghelp.dll not loaded)");
        l.Write();
    }
    if (fn == nullptr || g_quit.load())
        return 0;
    g_writeDump.store(reinterpret_cast<MiniDumpWriteDumpFn>(fn), std::memory_order_release);

    while (WaitForSingleObject(g_dumpRequest, INFINITE) == WAIT_OBJECT_0 && !g_quit.load())
    {
        const MiniDumpWriteDumpFn writeDump = g_writeDump.load(std::memory_order_acquire);
        if (writeDump == nullptr)
            break;
        SYSTEMTIME st {};
        GetLocalTime(&st);
        wchar_t path[MAX_PATH + 32] {};
        swprintf_s(path, L"%s%04u%02u%02u-%02u%02u%02u.dmp", g_dumpPrefix, st.wYear, st.wMonth, st.wDay, st.wHour,
                   st.wMinute, st.wSecond);
        BOOL ok = FALSE;
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION info { g_dumpThreadId, g_dumpPointers, FALSE };
            ok = writeDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                             static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo |
                                                        MiniDumpWithUnloadedModules),
                             g_dumpPointers != nullptr ? &info : nullptr, nullptr, nullptr);
            CloseHandle(file);
            if (!ok)
                DeleteFileW(path);
        }
        Line l;
        l.Head(ok ? "DUMP written " : "DUMP failed");
        if (ok)
        {
            const wchar_t* name = wcsrchr(path, L'\\');
            l.Wide(name != nullptr ? name + 1 : path);
        }
        l.Write();
        SetEvent(g_dumpDone);
    }
    return 0;
}

// amdnr_crash.log over 512 KB keeps its last 256 KB (from a line start). Install time only.
void TrimLog(const fs::path& path)
{
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE)
        return;
    LARGE_INTEGER size {};
    std::string tail;
    if (GetFileSizeEx(f, &size) && static_cast<uint64_t>(size.QuadPart) > kTrimAbove)
    {
        LARGE_INTEGER at {};
        at.QuadPart = size.QuadPart - static_cast<LONGLONG>(kTrimKeep);
        tail.resize(kTrimKeep);
        DWORD got = 0;
        if (SetFilePointerEx(f, at, nullptr, FILE_BEGIN) &&
            ReadFile(f, tail.data(), static_cast<DWORD>(tail.size()), &got, nullptr))
            tail.resize(got);
        else
            tail.clear();
    }
    CloseHandle(f);
    if (tail.empty())
        return;
    const size_t nl = tail.find('\n');
    if (nl != std::string::npos)
        tail.erase(0, nl + 1);
    HANDLE out = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
    if (out == INVALID_HANDLE_VALUE)
        return;
    DWORD written = 0;
    WriteFile(out, tail.data(), static_cast<DWORD>(tail.size()), &written, nullptr);
    CloseHandle(out);
}

// The events and the writer thread (it loads dbghelp once DllMain has returned); keeps the newest kDumpsKept - 1
// older dumps of this exe so the next one makes kDumpsKept. True when the writer thread was started.
bool PrepareDump(const fs::path& dir, const std::wstring& exeStem)
{
    std::wstring stem;
    for (wchar_t c : exeStem)
        stem.push_back((c < 0x80 && (iswalnum(c) || c == L'-' || c == L'_' || c == L'.')) ? c : L'_');
    if (stem.empty())
        stem = L"game";
    const std::wstring prefix = (dir / (L"AMDNR-crash-" + stem + L"-")).wstring();
    if (prefix.size() + 24 >= MAX_PATH)
        return false;
    wcscpy_s(g_dumpPrefix, prefix.c_str());

    struct Old
    {
        fs::path path;
        FILETIME time;
    };
    std::vector<Old> old;
    WIN32_FIND_DATAW fd {};
    HANDLE find = FindFirstFileW((prefix + L"*.dmp").c_str(), &fd);
    if (find != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                old.push_back({ dir / fd.cFileName, fd.ftLastWriteTime });
        } while (FindNextFileW(find, &fd));
        FindClose(find);
    }
    std::sort(old.begin(), old.end(),
              [](const Old& a, const Old& b) { return CompareFileTime(&a.time, &b.time) > 0; });
    for (size_t i = kDumpsKept - 1; i < old.size(); ++i)
        DeleteFileW(old[i].path.c_str());

    g_dumpRequest = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    g_dumpDone = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (g_dumpRequest == nullptr || g_dumpDone == nullptr)
        return false;
    g_dumpThread = CreateThread(nullptr, 256 * 1024, DumpWriter, nullptr, 0, nullptr);
    return g_dumpThread != nullptr;
}

std::string LocalNow()
{
    SYSTEMTIME st {};
    GetLocalTime(&st);
    return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", st.wYear, st.wMonth, st.wDay, st.wHour,
                       st.wMinute, st.wSecond, st.wMilliseconds);
}
} // namespace

bool Install(bool withDump)
{
    if (g_installed)
        return true;
    try
    {
        fs::path dir = fs::path(Config::Instance()->LogFileName.value_or_default()).parent_path();
        if (dir.empty())
            dir = Util::DllPath().parent_path();
        const fs::path crashLog = dir / L"amdnr_crash.log";
        TrimLog(crashLog);
        g_log = CreateFileW(crashLog.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (g_log == INVALID_HANDLE_VALUE)
        {
            LOG_WARN("Crash note: amdnr_crash.log could not be opened (error {}); [Log] CrashHandler stays off",
                     GetLastError());
            return false;
        }

        HMODULE self = dllModule;
        if (self == nullptr)
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&Install), &self);
        g_selfBase = reinterpret_cast<uintptr_t>(self);
        g_exeBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const std::string exe = wstring_to_string(Util::ExePath().filename().wstring());
        const std::string proxy = wstring_to_string(Util::DllPath().filename().wstring());
        CopyName(g_exeName, sizeof(g_exeName), exe.c_str());
        CopyName(g_selfName, sizeof(g_selfName), ("OptiScaler (" + proxy + ")").c_str());

        const bool dumpArmed = withDump && PrepareDump(dir, Util::ExePath().stem().wstring());

        // dump=pending: the writer thread loads dbghelp after DllMain and writes "DUMP ready" or "DUMP unavailable".
        const std::string start = std::format("{} pid={} START {} {} proxy={} tick={} dump={}\r\n", LocalNow(),
                                              GetCurrentProcessId(), exe, VER_PRODUCT_NAME, proxy, GetTickCount64(),
                                              dumpArmed ? "pending" : "off");
        DWORD written = 0;
        WriteFile(g_log, start.data(), static_cast<DWORD>(start.size()), &written, nullptr);

        g_veh = AddVectoredExceptionHandler(0, OnException); // 0 = last: the game's own handlers run first
        if (g_veh == nullptr)
        {
            LOG_WARN("Crash note: AddVectoredExceptionHandler failed; [Log] CrashHandler stays off");
            CloseHandle(g_log);
            g_log = INVALID_HANDLE_VALUE;
            return false;
        }
        g_previousUef = SetUnhandledExceptionFilter(OnUnhandled);
        g_uefInstalled = true;
        g_installed = true;
        LOG_INFO("Crash note on ([Log] CrashHandler=true): {}{}", wstring_to_string(crashLog.wstring()),
                 dumpArmed ? ", minidump armed ([Log] CrashDump=true)"
                           : (withDump ? ", minidump unavailable (no writer thread)" : ""));
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_WARN("Crash note not installed: {}", e.what());
    }
    catch (...)
    {
        LOG_WARN("Crash note not installed: unknown error");
    }
    if (g_log != INVALID_HANDLE_VALUE && !g_installed)
    {
        CloseHandle(g_log);
        g_log = INVALID_HANDLE_VALUE;
    }
    return false;
}

void Uninstall()
{
    if (!g_installed)
        return;
    g_installed = false;
    if (g_veh != nullptr)
    {
        RemoveVectoredExceptionHandler(g_veh);
        g_veh = nullptr;
    }
    if (g_uefInstalled)
    {
        // Put the previous filter back only if ours is still the current one (a game's later filter stays).
        const LPTOP_LEVEL_EXCEPTION_FILTER current = SetUnhandledExceptionFilter(g_previousUef);
        if (current != OnUnhandled)
            SetUnhandledExceptionFilter(current);
        g_uefInstalled = false;
    }
    if (g_dumpThread != nullptr)
    {
        // Not waited for: DllMain holds the loader lock (at process exit the thread is already gone).
        g_quit.store(true);
        SetEvent(g_dumpRequest);
        CloseHandle(g_dumpThread);
        g_dumpThread = nullptr;
        g_writeDump.store(nullptr, std::memory_order_release);
    }
    Line l;
    l.Head("END clean exit");
    l.Write();
    const HANDLE log = g_log;
    g_log = INVALID_HANDLE_VALUE;
    CloseHandle(log);
}
} // namespace CrashReport
