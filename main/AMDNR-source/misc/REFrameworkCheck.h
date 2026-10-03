// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
#pragma once
// REFramework check (AMDNR 0.3.3.2). Capcom's anti-tamper in Resident Evil Requiem traps (ud2 from its runtime-filled
// .xtext) 15-60 s after launch unless REFramework is loaded; OptiScaler relies on REF getting past it. re9.exe imports
// DINPUT8.dll statically and the game copies its folder DLLs into _storage_ and loads them from there, so both places
// are looked at on disk, and the module actually loaded is the authority at the first menu frame. REF's dinput8.dll has
// no VERSIONINFO (exports: DirectInput8Create, direct_input8_create, cimgui), so it is recognised by location: any
// dinput8.dll that is not Windows' own. A reframework folder alone is not evidence (it survives a deleted or
// antivirus-quarantined dinput8.dll). Warn only: nothing is loaded, hooked or blocked.
#include "SysUtils.h"

#include <filesystem>
#include <iterator>
#include <string>

namespace REFrameworkCheck
{
inline constexpr const char* kNightlyUrl = "https://github.com/praydog/REFramework-nightly/releases";

// "dinput8.dll", "_storage_\dinput8.dll", both, or empty. folderOnly: no dinput8.dll but a reframework folder.
inline std::string FilesFound(const std::filesystem::path& gameDir, bool& folderOnly)
{
    folderOnly = false;
    try
    {
        std::error_code ec;
        std::string found;
        if (std::filesystem::is_regular_file(gameDir / L"dinput8.dll", ec))
            found = "dinput8.dll";
        ec.clear();
        if (std::filesystem::is_regular_file(gameDir / L"_storage_" / L"dinput8.dll", ec))
            found += std::string(found.empty() ? "" : ", ") + "_storage_\\dinput8.dll";
        if (found.empty())
        {
            ec.clear();
            folderOnly = std::filesystem::is_directory(gameDir / L"reframework", ec);
        }
        return found;
    }
    catch (...)
    {
        folderOnly = false;
        return {};
    }
}

// The loaded dinput8.dll's path when it is neither Windows' own nor OptiScaler itself; empty otherwise.
// loaded: any dinput8.dll is in the process.
inline std::wstring LoadedProxy(bool& loaded)
{
    loaded = false;
    try
    {
        HMODULE h = GetModuleHandleW(L"dinput8.dll");
        loaded = h != nullptr;
        if (!h)
            return {};
        HMODULE self = nullptr;
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&LoadedProxy), &self) &&
            self == h)
            return {};
        wchar_t path[MAX_PATH * 2] {};
        const DWORD n = GetModuleFileNameW(h, path, static_cast<DWORD>(std::size(path)));
        if (n == 0 || n >= std::size(path))
            return {};
        wchar_t sys[MAX_PATH] {};
        const UINT sn = GetSystemDirectoryW(sys, MAX_PATH);
        std::wstring p(path, n), s(sys, sn < MAX_PATH ? sn : 0);
        to_lower_in_place(p);
        to_lower_in_place(s);
        if (!s.empty() && p.size() > s.size() && p.compare(0, s.size(), s) == 0 &&
            (p[s.size()] == L'\\' || p[s.size()] == L'/'))
            return {};
        return std::wstring(path, n);
    }
    catch (...)
    {
        return {};
    }
}

inline std::string GameTitle(std::string exe)
{
    to_lower_in_place(exe);
    return exe == "pragmata.exe" ? "PRAGMATA" : "Resident Evil Requiem";
}

// Confirmed for Resident Evil Requiem (and its demo); PRAGMATA uses the same engine and anti-tamper, unconfirmed.
inline bool Confirmed(std::string exe)
{
    to_lower_in_place(exe);
    return exe != "pragmata.exe";
}

inline std::string Advice(const std::string& exe, bool folderOnly)
{
    try
    {
        return GameTitle(exe) + " needs REFramework with OptiScaler" +
               (Confirmed(exe) ? "" : " (probably; not confirmed for this game)") +
               ", or it crashes 15-60 s after launch (An unhandled exception occurred). Put dinput8.dll from "
               "REFramework.zip (latest nightly: " +
               kNightlyUrl + ") in the game folder next to " + exe +
               ", and change REFramework's menu key: it is also Insert." +
               (folderOnly ? " A reframework folder is here but its dinput8.dll is missing (deleted, or quarantined "
                             "by an antivirus?)."
                           : "");
    }
    catch (...)
    {
        return {};
    }
}
} // namespace REFrameworkCheck
