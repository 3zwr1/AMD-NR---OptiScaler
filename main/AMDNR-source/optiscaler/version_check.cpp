#include "pch.h"

#include "version_check.h"

#include "State.h"
#include "resource.h"

#include <json.hpp>

#include <winhttp.h>

#include <charconv>
#include <optional>
#include <string_view>
#include <thread>
#include <mutex>
#include <format>

namespace
{
struct LatestReleaseInfo
{
    std::string tag;
    std::string name; // "AMDNR-v0.3.3.1": read when the tag carries no version
    std::string url;
};

// AMDNR (0.3.3.2): the check asks AMDNR's own GitHub releases, not upstream OptiScaler's, and compares AMDNR's
// version (AMDNR_VERSION_STR). Upstream's releases have no neural rendering, so "updating" to one would remove it.
// AMDNR tags read "Alpha0.3.3.1" (a v prefix works too): the first run of digits, up to four dot-separated parts.
constexpr wchar_t kReleasesHost[] = L"api.github.com";
constexpr wchar_t kReleasesPath[] = L"/repos/3zwr1/AMD-NR---OptiScaler/releases/latest";

std::optional<feature_version> ParseAmdnrVersion(std::string_view text)
{
    size_t i = 0;
    while (i < text.size() && (text[i] < '0' || text[i] > '9'))
        ++i;

    unsigned int parts[4] {};
    int count = 0;
    while (count < 4 && i < text.size() && text[i] >= '0' && text[i] <= '9')
    {
        const auto [end, ec] = std::from_chars(text.data() + i, text.data() + text.size(), parts[count]);
        if (ec != std::errc {})
            return std::nullopt;

        ++count;
        i = static_cast<size_t>(end - text.data());
        if (i + 1 < text.size() && text[i] == '.' && text[i + 1] >= '0' && text[i + 1] <= '9')
            ++i;
        else
            break;
    }

    if (count < 2)
        return std::nullopt;

    return feature_version { parts[0], parts[1], parts[2], parts[3] };
}

feature_version CurrentVersion()
{
    static const feature_version version = ParseAmdnrVersion(AMDNR_VERSION_STR).value_or(feature_version {});
    return version;
}

std::optional<LatestReleaseInfo> FetchLatestRelease()
{
    HINTERNET session = nullptr;
    HINTERNET connection = nullptr;
    HINTERNET request = nullptr;

    auto cleanup = [&]()
    {
        if (request != nullptr)
        {
            WinHttpCloseHandle(request);
            request = nullptr;
        }
        if (connection != nullptr)
        {
            WinHttpCloseHandle(connection);
            connection = nullptr;
        }
        if (session != nullptr)
        {
            WinHttpCloseHandle(session);
            session = nullptr;
        }
    };

    // Wine is being stoopid and fetch deadlocks somewhere otherwise
    Sleep(1000);

    session = WinHttpOpen(L"AMDNR Version Check/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr)
    {
        LOG_WARN("Version check failed to open WinHTTP session: {}", GetLastError());
        cleanup();
        return std::nullopt;
    }

    WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);

    connection = WinHttpConnect(session, kReleasesHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (connection == nullptr)
    {
        LOG_WARN("Version check failed to connect: {}", GetLastError());
        cleanup();
        return std::nullopt;
    }

    request = WinHttpOpenRequest(connection, L"GET", kReleasesPath, nullptr, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (request == nullptr)
    {
        LOG_WARN("Version check failed to open request: {}", GetLastError());
        cleanup();
        return std::nullopt;
    }

    LPCWSTR headers = L"User-Agent: OptiScaler\r\nAccept: application/vnd.github+json\r\nAccept-Encoding: identity\r\n";
    if (!WinHttpSendRequest(request, headers, (DWORD) -1, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        LOG_WARN("Version check failed to send request: {}", GetLastError());
        cleanup();
        return std::nullopt;
    }

    if (!WinHttpReceiveResponse(request, nullptr))
    {
        LOG_WARN("Version check failed to receive response: {}", GetLastError());
        cleanup();
        return std::nullopt;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX))
    {
        LOG_WARN("Version check failed to read status code: {}", GetLastError());
        cleanup();
        return std::nullopt;
    }

    if (statusCode != HTTP_STATUS_OK)
    {
        LOG_WARN("Version check returned HTTP status {}", statusCode);
        cleanup();
        return std::nullopt;
    }

    std::string response;
    DWORD available = 0;
    do
    {
        if (!WinHttpQueryDataAvailable(request, &available))
        {
            LOG_WARN("Version check failed querying data availability: {}", GetLastError());
            cleanup();
            return std::nullopt;
        }

        if (available == 0)
            break;

        std::string buffer;
        buffer.resize(available);
        DWORD downloaded = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &downloaded))
        {
            LOG_WARN("Version check failed reading response: {}", GetLastError());
            cleanup();
            return std::nullopt;
        }

        response.append(buffer.data(), downloaded);
    } while (available > 0);

    cleanup();

    try
    {
        auto json = nlohmann::json::parse(response);
        LatestReleaseInfo info;
        info.tag = json.value("tag_name", std::string {});
        if (const auto name = json.find("name"); name != json.end() && name->is_string()) // null when unnamed
            info.name = name->get<std::string>();
        info.url = json.value("html_url", std::string {});

        if (info.tag.empty())
            return std::nullopt;

        return info;
    }
    catch (const std::exception& ex)
    {
        LOG_WARN("Version check failed to parse response: {}", ex.what());
        return std::nullopt;
    }
}

void FinishVersionCheck()
{
    auto& state = State::Instance();
    std::scoped_lock lock(state.versionCheckMutex);
    state.versionCheckInProgress = false;
    state.versionCheckCompleted = true;
}

void RunVersionCheck()
{
    struct Finalizer
    {
        ~Finalizer() { FinishVersionCheck(); }
    } finalize;

    auto release = FetchLatestRelease();
    if (!release.has_value())
    {
        auto& state = State::Instance();
        std::scoped_lock lock(state.versionCheckMutex);
        state.versionCheckError = "Unable to check for updates.";
        state.updateAvailable = false;
        return;
    }

    auto remoteParsed = ParseAmdnrVersion(release->tag);
    if (!remoteParsed.has_value())
        remoteParsed = ParseAmdnrVersion(release->name);
    const feature_version remoteVersion = remoteParsed.value_or(feature_version {});

    auto& state = State::Instance();
    {
        std::scoped_lock lock(state.versionCheckMutex);
        state.latestVersionTag = release->tag;
        state.latestVersionUrl = release->url;
    }

    if (remoteVersion == feature_version { 0, 0, 0 })
    {
        LOG_WARN("Version check received unrecognized tag format: {}", release->tag);
        std::scoped_lock lock(state.versionCheckMutex);
        state.versionCheckError = "Received an unknown version format from update server.";
        state.updateAvailable = false;
        return;
    }

    const auto localVersion = CurrentVersion();
    const bool updateAvailable = localVersion != feature_version {} && remoteVersion > localVersion;

    {
        std::scoped_lock lock(state.versionCheckMutex);
        state.updateAvailable = updateAvailable;
        state.versionCheckError.clear();
    }

    if (updateAvailable)
        LOG_WARN("New AMDNR release available: {} (current {}).", release->tag, AMDNR_VERSION_STR);
    else
        LOG_INFO("AMDNR is up to date (current {}, latest release {})", AMDNR_VERSION_STR, release->tag);
}
} // namespace

// The version the update check compares: AMDNR's, not the OptiScaler base's.
const std::string& VersionCheck::CurrentVersionString()
{
    static const std::string version = AMDNR_VERSION_STR;
    return version;
}

void VersionCheck::Start()
{
    auto& state = State::Instance();
    {
        std::scoped_lock lock(state.versionCheckMutex);
        if (state.versionCheckInProgress || state.versionCheckCompleted)
            return;

        state.versionCheckInProgress = true;
        state.versionCheckCompleted = false;
        state.updateAvailable = false;
        state.versionCheckError.clear();
        state.latestVersionTag.clear();
        state.latestVersionUrl.clear();
    }

    std::thread(
        []()
        {
            try
            {
                RunVersionCheck();
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR("Version check failed with exception: {}", ex.what());
                auto& state = State::Instance();
                std::scoped_lock lock(state.versionCheckMutex);
                state.versionCheckError = "Update check failed.";
                state.updateAvailable = false;
            }
            catch (...)
            {
                LOG_ERROR("Version check failed with unknown exception");
                auto& state = State::Instance();
                std::scoped_lock lock(state.versionCheckMutex);
                state.versionCheckError = "Update check failed.";
                state.updateAvailable = false;
            }
        })
        .detach();
}
