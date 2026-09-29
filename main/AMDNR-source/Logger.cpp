// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#include "pch.h"
#include "Logger.h"
#include "Config.h"
#include <iostream>

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/callback_sink.h"
#include <include/spdlog_sink/debug_sink.h>

#include "Util.h"
#include "resource.h" // VER_PRODUCT_NAME for the session header
#include "misc/LogRotationNames.h" // FB-L8: the previous logs' names

#include <algorithm>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

// Set by BeginLogSession when another live process holds the log (0.3.3.2, NTE's crash reporter
// emptied the game's log). PrepareLogger then appends instead of truncating.
static bool s_logOpenElsewhere = false;

static bool InitializeConsole()
{
    // Allocate a console for this app
    if (!AllocConsole())
        return false;

    FILE* pFile;

    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&pFile, "CONIN$", "r", stdin) != 0)
            return false;
    }

    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&pFile, "CONOUT$", "w", stdout) != 0)
            return false;
    }

    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&pFile, "CONOUT$", "w", stderr) != 0)
            return false;
    }

    // Clear the error state for each of the C++ standard streams
    std::cin.clear();
    std::cout.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::wcout.clear();
    std::wcerr.clear();

    // Make C++ standard streams point to console as well.
    std::ios::sync_with_stdio();

    WaitForEnter();

    return true;
}

void WaitForEnter()
{
    if (Config::Instance()->DebugWait.value_or_default())
    {
        std::cout << "Press ENTER to continue..." << std::endl;
        std::cin.get();
    }
}

void PrepareLogger()
{
    try
    {
        if (spdlog::default_logger() != nullptr)
            spdlog::default_logger().reset();

        if (Config::Instance()->LogToConsole.value_or_default() || Config::Instance()->LogToFile.value_or_default() ||
            Config::Instance()->LogToNGX.value_or_default() || Config::Instance()->LogToDebug.value_or_default())
        {
            if (Config::Instance()->OpenConsole.value_or_default())
                InitializeConsole();

            std::shared_ptr<spdlog::logger> shared_logger = nullptr;

            if (Config::Instance()->LogAsync.value_or_default())
            {
                // Set the queue size for asynchronous logging
                spdlog::init_thread_pool(8192, Config::Instance()->LogAsyncThreads.value_or_default());
            }

            std::vector<spdlog::sink_ptr> sinks;

            if (Config::Instance()->LogToDebug.value_or_default())
            {
                auto debug_sink = std::make_shared<spdlog::sinks::debug_sink_mt>();
                debug_sink->set_level(spdlog::level::level_enum::trace);

#ifdef LOG_ASYNC
                debug_sink->set_pattern("%H:%M:%S.%f\t%L\t%v");
#else
                debug_sink->set_pattern("[%H:%M:%S.%f] [%L] %v");
                // file_sink->set_pattern("[%H:%M:%S.%f] [thread %t] [%L] %v");
#endif // LOG_ASYNC

                sinks.push_back(debug_sink);
            }

            if (Config::Instance()->LogToConsole.value_or_default())
            {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_level(spdlog::level::level_enum::info);
                console_sink->set_pattern("[%H:%M:%S.%f] [%L] %v");

                sinks.push_back(console_sink);
            }

            if (Config::Instance()->LogToFile.value_or_default())
            {
                // Truncate only a log no other live process writes (see BeginLogSession).
                auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                    Config::Instance()->LogFileName.value_or_default(), !s_logOpenElsewhere);
                file_sink->set_level(spdlog::level::level_enum::trace);
#ifdef LOG_ASYNC
                file_sink->set_pattern("%H:%M:%S.%f\t%L\t%v");
#else
                file_sink->set_pattern("[%H:%M:%S.%f] [%L] %v");
                // file_sink->set_pattern("[%H:%M:%S.%f] [thread %t] [%L] %v");
#endif // LOG_ASYNC

                sinks.push_back(file_sink);
            }

            auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
                [](const spdlog::details::log_msg& msg)
                {
                    if (Config::Instance()->LogToNGX.value_or_default() &&
                        State::Instance().NVNGX_Logger.LoggingCallback != nullptr &&
                        State::Instance().NVNGX_Logger.MinimumLoggingLevel != NVSDK_NGX_LOGGING_LEVEL_OFF &&
                        (State::Instance().NVNGX_Logger.MinimumLoggingLevel == NVSDK_NGX_LOGGING_LEVEL_VERBOSE ||
                         msg.level >= spdlog::level::info))
                    {
                        auto message = (char*) msg.payload.data();
                        State::Instance().NVNGX_Logger.LoggingCallback(message, NVSDK_NGX_LOGGING_LEVEL_ON,
                                                                       NVSDK_NGX_Feature_SuperSampling);
                    }
                });

            callback_sink->set_level(spdlog::level::level_enum::trace);
            callback_sink->set_pattern("[%H:%M:%S.%f] [%L] %v");

            sinks.push_back(callback_sink);

            if (Config::Instance()->LogAsync.value_or_default())
            {
                shared_logger =
                    std::make_shared<spdlog::async_logger>("multi_sink_logger", sinks.begin(), sinks.end(),
                                                           spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            }
            else
            {
                spdlog::logger logger("multi_sink", sinks.begin(), sinks.end());
                shared_logger = std::make_shared<spdlog::logger>(logger);
            }

            shared_logger->set_level((spdlog::level::level_enum) Config::Instance()->LogLevel.value_or_default());
            shared_logger->flush_on(spdlog::level::trace);

            spdlog::set_default_logger(shared_logger);
        }
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << ex.what() << std::endl;

        auto logger = spdlog::stdout_color_mt("xess");
        logger->set_pattern("[%H:%M:%S.%f] [%L] %v");
        logger->set_level((spdlog::level::level_enum) 2);
        spdlog::set_default_logger(logger);
    }
}

void CloseLogger()
{
    spdlog::default_logger()->flush();
    spdlog::shutdown();
}

// SESSION BOOKKEEPING FOR OptiScaler.log (the TLOU report).
//
// PrepareLogger truncates the log at every start (basic_file_sink, truncate = true), unless another
// live process still holds it (then it appends, 0.3.3.2). The Last of Us
// runs two exes from one folder: when tlou-i.exe died without a word, starting tlou-i-l.exe emptied
// the only log of that death. So, in DLL_PROCESS_ATTACH before PrepareLogger:
//   - the old log is moved aside to "<stem>.previous.<exe that wrote it>.log", one previous log per
//     exe, so a second exe no longer overwrites the first one's. The writer is read from the session
//     header, the first line of every log from this build on; a log without one (an older build)
//     becomes "<stem>.previous.log". Only with LogToFile and SingleFile: per-session file names
//     (SingleFile=false) are never truncated.
//   - "optiscaler_running_<exe>.marker" beside the log exists from attach until DLL_PROCESS_DETACH.
//     Still there at the next start = that session recorded no clean exit. The wording is deliberate:
//     a crash, a hang ended from Task Manager and a game that ends itself with TerminateProcess all
//     look the same from here (a normal TLOU quit does reach DETACH).
// Only kernel32 file calls (CreateFileW, ReadFile, WriteFile, MoveFileExW, SetFileInformationByHandle),
// which are safe under the loader lock. The findings are logged by LogSessionHeader once the logger
// exists.
//   - (AMDNR 0.3.4, FB-L7) the AMD bridge's RtlExitUserProcess hook, which runs when the game quits through
//     ExitProcess, appends a stamp line to the marker (NoteSessionExitHook), and a second one once AMDNR's own exit
//     work (the NR runtime's shutdown) has returned (NoteSessionExitDone). A marker with both stamps at the next
//     start is a session that quit normally and then did not reach DLL_PROCESS_DETACH (another module's detach, a
//     hang later in the shutdown), so it is noted as "exited after the Exit hook", not as a missing clean exit. A
//     marker with the first stamp only is still warned about: AMDNR's own exit work did not finish.
//   - (AMDNR 0.3.4, FB-L8) [Log] KeepPreviousLogs (1..5, default 3) previous logs per exe: before the log is moved
//     to generation 0 (0.3.3.2's name), that exe's older ones move up one generation and the oldest is replaced
//     (names: misc/LogRotationNames.h, "<stem>.previous-<n>.<exe>.log"). 1 = 0.3.3.2 (only generation 0).
namespace
{
constexpr char kSessionTag[] = "AMDNR session start:";
constexpr char kExitHookTag[] = "AMDNR exit hook reached:"; // FB-L7: the stamp line's start
constexpr char kExitDoneTag[] = "AMDNR exit hook finished:"; // FB-L7: the second stamp, after AMDNR's exit work

struct SessionNote
{
    bool warn = false;
    std::string text;
};

// A running marker's text: the session header (its first line), then the Exit hook's stamp line when the game's own
// exit call ran in that session, then the second stamp when AMDNR's exit work in that hook finished (FB-L7).
struct MarkerText
{
    std::string header;
    bool exitHook = false;
    std::string exitDetails; // the stamp after its tag ("tick ..., exit code 0x..."), when exitHook
    bool exitDone = false;
    std::string exitDoneDetails; // the second stamp after its tag ("NR shutdown: stopped (12 ms)"), when exitDone
};

// The rest of the line after `tag` at `at`, spaces trimmed at the start.
std::string MarkerStampDetails(const std::string& text, size_t at, size_t tagSize)
{
    auto details = text.substr(at + tagSize);
    details = details.substr(0, details.find_first_of("\r\n"));
    const auto start = details.find_first_not_of(' ');
    return start == std::string::npos ? std::string("no details") : details.substr(start);
}

MarkerText SplitMarker(const std::string& text)
{
    MarkerText m;
    const auto eol = text.find_first_of("\r\n");
    m.header = text.substr(0, eol);
    if (eol == std::string::npos)
        return m;

    const auto tag = text.find(kExitHookTag, eol);
    if (tag == std::string::npos)
        return m;

    m.exitHook = true;
    m.exitDetails = MarkerStampDetails(text, tag, sizeof(kExitHookTag) - 1);

    const auto done = text.find(kExitDoneTag, tag);
    if (done != std::string::npos)
    {
        m.exitDone = true;
        m.exitDoneDetails = MarkerStampDetails(text, done, sizeof(kExitDoneTag) - 1);
    }
    return m;
}

std::wstring _sessionMarker;             // this session's marker; EndLogSession removes it
std::string _sessionHeader;              // the header line, also written into the marker
std::vector<SessionNote> _sessionNotes;  // said by LogSessionHeader

bool SessionFileExists(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// FB-L8: whether the log can be moved now. Another live process that writes it holds a handle without
// FILE_SHARE_DELETE (ERROR_SHARING_VIOLATION here), and then the move below fails too: the older generations must
// not move up for a log that stays where it is. kernel32 only.
bool SessionLogCanMove(const std::wstring& path)
{
    HANDLE file = CreateFileW(path.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    CloseHandle(file);
    return true;
}

// Up to maxBytes from the start of an open file, with trailing line ends and spaces trimmed. Shared by
// SessionReadHead and EndLogSession, so both compare a marker the same way.
std::string SessionReadOpen(HANDLE file, DWORD maxBytes)
{
    std::string text(maxBytes, '\0');
    DWORD read = 0;
    if (!ReadFile(file, text.data(), maxBytes, &read, nullptr))
        read = 0;

    text.resize(read);

    while (!text.empty() && (text.back() == '\r' || text.back() == '\n' || text.back() == ' '))
        text.pop_back();

    return text;
}

std::string SessionReadHead(const std::wstring& path, DWORD maxBytes)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return {};

    std::string text = SessionReadOpen(file, maxBytes);
    CloseHandle(file);
    return text;
}

bool SessionWriteFile(const std::wstring& path, const std::string& text)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    const bool ok = WriteFile(file, text.data(), (DWORD) text.size(), &written, nullptr) && written == text.size();
    CloseHandle(file);
    return ok;
}

std::wstring SessionMarkerPath(const std::filesystem::path& dir, const std::wstring& exe)
{
    return (dir / (L"optiscaler_running_" + exe + L".marker")).wstring();
}

// exe="..." from the session header at the top of an old log. Empty for a log without one, and for
// anything that is not a plain file name (it becomes part of a path).
std::wstring SessionWriterExe(const std::string& head)
{
    const auto tag = head.find(kSessionTag);
    if (tag == std::string::npos)
        return {};

    const auto eol = head.find('\n', tag);
    auto key = head.find("exe=\"", tag);
    if (key == std::string::npos || key > eol)
        return {};

    key += 5;
    const auto end = head.find('"', key);
    if (end == std::string::npos || end > eol || end == key)
        return {};

    const auto exe = string_to_wstring(head.substr(key, end - key));
    if (exe.empty() || exe == L"." || exe == L".." || exe.find_first_of(L"\\/:*?\"<>|") != std::wstring::npos)
        return {};

    return exe;
}
} // namespace

void BeginLogSession()
{
    _sessionNotes.clear();

    const std::wstring exe = Util::ExePath().filename().wstring();
    const std::filesystem::path logPath(Config::Instance()->LogFileName.value_or_default());
    std::filesystem::path logDir = logPath.parent_path();
    if (logDir.empty())
        logDir = Util::DllPath().parent_path();

    SYSTEMTIME now {};
    GetLocalTime(&now);
    _sessionHeader = std::format("{} {:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03} local, tick {}, pid {}, exe=\"{}\", {}",
                                 kSessionTag, now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
                                 now.wMilliseconds, GetTickCount64(), GetCurrentProcessId(), wstring_to_string(exe),
                                 VER_PRODUCT_NAME);

    // This exe's marker as the last session left it, before it is rewritten below
    const std::wstring ownMarker = SessionMarkerPath(logDir, exe);
    const bool ownUnclean = SessionFileExists(ownMarker);
    const MarkerText ownPrevious = ownUnclean ? SplitMarker(SessionReadHead(ownMarker, 4096)) : MarkerText();
    bool ownReported = false;

    if (Config::Instance()->LogToFile.value_or_default() && Config::Instance()->LogSingleFile.value_or_default() &&
        SessionFileExists(logPath.wstring()))
    {
        const std::wstring writer = SessionWriterExe(SessionReadHead(logPath.wstring(), 4096));
        // Generation 0 is 0.3.3.2's "<stem>.previous[.<writer stem>].log" (LogRotationNames.h)
        const std::wstring logStem = logPath.stem().wstring();
        const std::wstring writerStem = writer.empty() ? std::wstring() : std::filesystem::path(writer).stem().wstring();
        const std::wstring previousName = LogRotationNames::PreviousLogName(logStem, writerStem, 0);
        const std::wstring previousPath = (logDir / previousName).wstring();
        const std::string writerU8 = writer.empty() ? std::string("a build without a session header")
                                                    : wstring_to_string(writer);

        // FB-L8: this writer's older previous logs move up one generation (the oldest kept one is replaced), only
        // when the log itself can move now. A generation that cannot move stays, and the next one down replaces it
        // or is replaced, as generation 0 always was in 0.3.3.2. KeepPreviousLogs=1: nothing here (0.3.3.2).
        const unsigned keepPrevious = static_cast<unsigned>(
            std::clamp(Config::Instance()->KeepPreviousLogs.value_or_default(), 1, 5));
        if (keepPrevious > 1 && SessionLogCanMove(logPath.wstring()))
        {
            for (unsigned generation = keepPrevious - 1; generation >= 1; --generation)
            {
                const std::wstring from =
                    (logDir / LogRotationNames::PreviousLogName(logStem, writerStem, generation - 1)).wstring();
                const std::wstring to =
                    (logDir / LogRotationNames::PreviousLogName(logStem, writerStem, generation)).wstring();
                if (SessionFileExists(from))
                    MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING);
            }
        }

        if (MoveFileExW(logPath.c_str(), previousPath.c_str(), MOVEFILE_REPLACE_EXISTING))
        {
            const bool sameExe = !writer.empty() && _wcsicmp(writer.c_str(), exe.c_str()) == 0;
            const bool writerUnclean =
                sameExe ? ownUnclean : (!writer.empty() && SessionFileExists(SessionMarkerPath(logDir, writer)));
            ownReported = sameExe;
            // FB-L7: whether that session's marker carries the Exit hook's stamp
            const MarkerText writerMarker =
                !writerUnclean ? MarkerText()
                : sameExe      ? ownPrevious
                               : SplitMarker(SessionReadHead(SessionMarkerPath(logDir, writer), 4096));

            if (writerUnclean && writerMarker.exitHook && writerMarker.exitDone)
                _sessionNotes.push_back(
                    { false, std::format("Previous session log ({}) kept as {} - that session exited after the Exit "
                                         "hook (the game's own exit call ran: {}; AMDNR's exit work finished: {}) but "
                                         "did not reach DLL_PROCESS_DETACH; the game was quitting, not a crash "
                                         "during play",
                                         writerU8, wstring_to_string(previousName), writerMarker.exitDetails,
                                         writerMarker.exitDoneDetails) });
            else if (writerUnclean && writerMarker.exitHook)
                _sessionNotes.push_back(
                    { true, std::format("Previous session log ({}) kept as {} - no clean exit recorded for that "
                                        "session: the game's own exit call ran ({}), but AMDNR's exit work (the NR "
                                        "shutdown) did not finish (a crash or hang in it, or the process was ended "
                                        "from outside while quitting)",
                                        writerU8, wstring_to_string(previousName), writerMarker.exitDetails) });
            else if (writerUnclean)
                _sessionNotes.push_back(
                    { true, std::format("Previous session log ({}) kept as {} - no clean exit recorded for that "
                                        "session (it never reached DLL_PROCESS_DETACH: a crash, a hang ended from "
                                        "outside, or a game that terminates itself)",
                                        writerU8, wstring_to_string(previousName)) });
            else
                _sessionNotes.push_back(
                    { false, std::format("Previous session log ({}) kept as {}", writerU8,
                                         wstring_to_string(previousName)) });

            if (keepPrevious > 1)
            {
                std::string older = wstring_to_string(LogRotationNames::PreviousLogName(logStem, writerStem, 1));
                if (keepPrevious > 2)
                    older += " up to " + wstring_to_string(
                                             LogRotationNames::PreviousLogName(logStem, writerStem, keepPrevious - 1));
                _sessionNotes.push_back({ false, std::format("Older previous logs of {} kept as {} ([Log] "
                                                             "KeepPreviousLogs={})",
                                                             writerU8, older, keepPrevious) });
            }
        }
        else
        {
            // ERROR_SHARING_VIOLATION: another process still writes it (its handle has no
            // FILE_SHARE_DELETE), so it is not a dead session and it is not claimed as one.
            // That process's log is not truncated either (0.3.3.2): this session appends to it.
            const DWORD error = GetLastError();
            s_logOpenElsewhere = (error == ERROR_SHARING_VIOLATION);
            _sessionNotes.push_back(
                { true, s_logOpenElsewhere
                            ? std::format("Could not keep the previous log as {} (error {}, still open in another "
                                          "process); this session appends to it instead of overwriting it",
                                          wstring_to_string(previousName), error)
                            : std::format("Could not keep the previous log as {} (error {}); it is overwritten now",
                                          wstring_to_string(previousName), error) });
        }
    }

    // Not covered above: logging was off last time, or another exe wrote the log since (TLOU's
    // second exe). Its own previous log, if one was kept then, is still <stem>.previous.<exe>.log.
    if (ownUnclean && !ownReported && ownPrevious.exitHook && ownPrevious.exitDone)
        _sessionNotes.push_back(
            { false, std::format("The previous {} session exited after the Exit hook (the game's own exit call ran: "
                                 "{}; AMDNR's exit work finished: {}) but did not reach DLL_PROCESS_DETACH; the game "
                                 "was quitting, not a crash during play ({})",
                                 wstring_to_string(exe), ownPrevious.exitDetails, ownPrevious.exitDoneDetails,
                                 ownPrevious.header.empty() ? std::string("no details") : ownPrevious.header) });
    else if (ownUnclean && !ownReported && ownPrevious.exitHook)
        _sessionNotes.push_back(
            { true, std::format("No clean exit recorded for the previous {} session: the game's own exit call ran "
                                "({}), but AMDNR's exit work (the NR shutdown) did not finish (a crash or hang in it, "
                                "or the process was ended from outside while quitting) ({})",
                                wstring_to_string(exe), ownPrevious.exitDetails,
                                ownPrevious.header.empty() ? std::string("no details") : ownPrevious.header) });
    else if (ownUnclean && !ownReported)
        _sessionNotes.push_back({ true, std::format("No clean exit recorded for the previous {} session ({})",
                                                    wstring_to_string(exe),
                                                    ownPrevious.header.empty() ? std::string("no details")
                                                                               : ownPrevious.header) });

    if (SessionWriteFile(ownMarker, _sessionHeader + "\r\n"))
    {
        _sessionMarker = ownMarker;
    }
    else
    {
        const DWORD error = GetLastError();
        _sessionNotes.push_back(
            { false, std::format("Session marker not written ({}, error {}); the next start cannot tell whether "
                                 "this one ended cleanly",
                                 wstring_to_string(ownMarker), error) });
    }
}

void LogSessionHeader()
{
    // Warn level, like the banner after it, so the line is there at every LogLevel up to 3. It is the
    // file's first line, where the next start's BeginLogSession looks for the writer's exe.
    if (!_sessionHeader.empty())
        spdlog::warn("{}", _sessionHeader);

    for (const auto& note : _sessionNotes)
    {
        if (note.warn)
            spdlog::warn("{}", note.text);
        else
            spdlog::info("{}", note.text);
    }

    _sessionNotes.clear();
}

// The header line for the AMD logs' first line in this process (AmdBridge::SessionHeader, 0.3.3.2).
// Set once in DLL_PROCESS_ATTACH and never changed after, so any thread may read it.
std::string SessionHeaderText() { return _sessionHeader; }

// FB-L7 (AMDNR 0.3.4): the AMD bridge's RtlExitUserProcess hook (AmdBridge.cpp, Exit) calls this as the game quits.
// It appends "AMDNR exit hook reached: tick <t>, exit code 0x<c>" to this session's running marker, so a session
// that then does not reach DLL_PROCESS_DETACH is told apart at the next start (BeginLogSession) from one that
// crashed. Only a marker that still holds this session's header is stamped (a relaunched copy of the exe may own it
// by then); the check and the append go through one handle that shares reading only, as in EndLogSession. kernel32
// calls and stack buffers only: no allocation and no lock (the marker path and the header are fixed since
// DLL_PROCESS_ATTACH), nothing that can throw.
void NoteSessionExitHook(long exitCode) noexcept
{
    if (_sessionMarker.empty() || _sessionHeader.empty())
        return;

    HANDLE file = CreateFileW(_sessionMarker.c_str(), GENERIC_READ | FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;

    char onDisk[2048];
    DWORD read = 0;
    if (!ReadFile(file, onDisk, sizeof(onDisk), &read, nullptr))
        read = 0;

    const bool endsWithLine = read > 0 && onDisk[read - 1] == '\n'; // BeginLogSession wrote "<header>\r\n"
    while (read > 0 && (onDisk[read - 1] == '\r' || onDisk[read - 1] == '\n' || onDisk[read - 1] == ' '))
        --read;

    if (read == _sessionHeader.size() && std::memcmp(onDisk, _sessionHeader.data(), read) == 0)
    {
        char stamp[128];
        char* out = stamp;
        char* const end = stamp + sizeof(stamp);
        const auto put = [&out, end](const char* text, size_t size)
        {
            if (size <= static_cast<size_t>(end - out))
            {
                std::memcpy(out, text, size);
                out += size;
            }
        };
        if (!endsWithLine)
            put("\r\n", 2);
        put(kExitHookTag, sizeof(kExitHookTag) - 1);
        put(" tick ", 6);
        out = std::to_chars(out, end, GetTickCount64()).ptr;
        put(", exit code 0x", 14);
        out = std::to_chars(out, end, static_cast<unsigned long>(exitCode), 16).ptr;
        put("\r\n", 2);

        LARGE_INTEGER atEnd {};
        DWORD written = 0;
        if (SetFilePointerEx(file, atEnd, nullptr, FILE_END))
            WriteFile(file, stamp, static_cast<DWORD>(out - stamp), &written, nullptr);
    }

    CloseHandle(file);
}

// FB-L7 (G1 review): the same hook calls this once AMDNR's own exit work (the NR runtime's Shutdown and its
// amd_bridge.log line) has returned. It appends "AMDNR exit hook finished: <what> (<ms> ms)" under the first stamp,
// only to a marker that holds this session's header followed by that stamp line and nothing else (so never twice,
// never on a relaunched copy's marker). A marker with the first stamp only is AMDNR's exit work not finishing - a
// crash or hang in it, or the process ended from outside meanwhile - and the next start warns about it
// (BeginLogSession). Same constraints as NoteSessionExitHook: kernel32 calls and stack buffers only.
void NoteSessionExitDone(const char* what, unsigned long long ms) noexcept
{
    if (_sessionMarker.empty() || _sessionHeader.empty() || what == nullptr)
        return;

    HANDLE file = CreateFileW(_sessionMarker.c_str(), GENERIC_READ | FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;

    char onDisk[2048];
    DWORD read = 0;
    if (!ReadFile(file, onDisk, sizeof(onDisk), &read, nullptr))
        read = 0;

    // "<header>" + line end(s) + "AMDNR exit hook reached: ...\r\n", the stamp line complete and the last one
    const std::string_view text(onDisk, read);
    const size_t headerSize = _sessionHeader.size();
    bool stampOnly = text.size() > headerSize && text.compare(0, headerSize, _sessionHeader) == 0 &&
                     (text[headerSize] == '\r' || text[headerSize] == '\n');
    if (stampOnly)
    {
        const size_t stampLine = text.find_first_not_of("\r\n", headerSize);
        stampOnly = stampLine != std::string_view::npos &&
                    text.compare(stampLine, sizeof(kExitHookTag) - 1, kExitHookTag) == 0 &&
                    text.find('\n', stampLine) == text.size() - 1;
    }

    if (stampOnly)
    {
        char stamp[256];
        char* out = stamp;
        char* const end = stamp + sizeof(stamp);
        const auto put = [&out, end](const char* bytes, size_t size)
        {
            if (size <= static_cast<size_t>(end - out))
            {
                std::memcpy(out, bytes, size);
                out += size;
            }
        };
        put(kExitDoneTag, sizeof(kExitDoneTag) - 1);
        put(" ", 1);
        put(what, std::strlen(what));
        put(" (", 2);
        out = std::to_chars(out, end, ms).ptr;
        put(" ms)\r\n", 6);

        LARGE_INTEGER atEnd {};
        DWORD written = 0;
        if (SetFilePointerEx(file, atEnd, nullptr, FILE_END))
            WriteFile(file, stamp, static_cast<DWORD>(out - stamp), &written, nullptr);
    }

    CloseHandle(file);
}

void EndLogSession()
{
    if (_sessionMarker.empty())
        return;

    const std::wstring marker = std::move(_sessionMarker);
    _sessionMarker.clear();

    // The marker is per exe, not per process. A game that relaunches itself with the same exe can
    // attach the child before the parent reaches DLL_PROCESS_DETACH, and the child's BeginLogSession
    // has then rewritten the marker with its own header: deleting it here would erase the child's
    // running marker, and a later crash of the child would go unreported. So it is removed only
    // while it still holds this session's header (local time, tick and pid, which no other session
    // writes). The check and the delete use one handle that shares reading only, so nothing can
    // rewrite the marker in between. A session that starts at that moment cannot write its marker
    // (sharing violation), and its own log says so. A marker that is already gone still counts as a
    // clean exit.
    HANDLE file = CreateFileW(marker.c_str(), GENERIC_READ | DELETE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
            spdlog::info("Clean exit recorded (session marker removed)");
        else if (error == ERROR_SHARING_VIOLATION)
            spdlog::warn("Session marker is open in another process (error {}: another session of this exe writing "
                         "it, or a scanner); left in place, so the next start may report no clean exit",
                         error);
        else
            spdlog::warn("Session marker could not be removed (error {}); the next start will report no clean exit",
                         error);
        return;
    }

    // This session's header, alone or followed by this session's Exit hook stamps (FB-L7): NoteSessionExitHook only
    // stamps a marker that holds this header, NoteSessionExitDone only one that holds the header and that stamp.
    const std::string onDisk = SessionReadOpen(file, 4096);
    const size_t stampLine = onDisk.find_first_not_of("\r\n", _sessionHeader.size());
    const bool ownMarker =
        onDisk == _sessionHeader ||
        (onDisk.size() > _sessionHeader.size() && onDisk.compare(0, _sessionHeader.size(), _sessionHeader) == 0 &&
         (onDisk[_sessionHeader.size()] == '\r' || onDisk[_sessionHeader.size()] == '\n') &&
         stampLine != std::string::npos && onDisk.compare(stampLine, sizeof(kExitHookTag) - 1, kExitHookTag) == 0);
    if (!ownMarker)
    {
        CloseHandle(file);
        spdlog::info("Clean exit, but the session marker now belongs to another session of this exe ({}); "
                     "left in place",
                     onDisk.empty() ? std::string("no details") : onDisk);
        return;
    }

    // Delete through the same handle (the file goes when the last handle to it closes, as with
    // DeleteFileW). The struct's one member is DeleteFile = TRUE.
    FILE_DISPOSITION_INFO disposition = { TRUE };
    const bool removed =
        SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof(disposition)) != FALSE;
    const DWORD error = removed ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);

    if (removed)
        spdlog::info("Clean exit recorded (session marker removed)");
    else
        spdlog::warn("Session marker could not be removed (error {}); the next start will report no clean exit",
                     error);
}
