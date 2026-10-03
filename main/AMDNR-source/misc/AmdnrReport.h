// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// "Save report": one zip in the game folder with report.txt, the logs (the current one and every
// OptiScaler.previous*.log generation), the ini and a system summary, user names masked. Written on a worker thread;
// nothing is uploaded and no network API is used. The menu's button calls SaveAsync() and shows LastResult().
// The text helpers below (masking, caps) are header-only so the unit tests (tests\034) build them without the DLL.
//
// Signatures of Result / SaveAsync / LastResult / MaskUserProfile fixed by W0-2 (a change is a gate request);
// bodies W1-F (the helpers here, the rest in AmdnrReport.cpp).

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace AmdnrReport
{
// The last save, for the menu's result line.
struct Result
{
    bool running = false; // a save is in progress (the button is disabled)
    bool done = false;    // at least one save finished this session
    bool ok = false;      // the last finished save wrote its zip
    std::string text;     // UTF-8: the zip's path, or why the last save failed; empty before the first save
};

// Starts one save on a worker thread. False (and nothing started) while a save is running. Menu thread; cheap.
bool SaveAsync();

// The state of the last save. Safe to call from the menu every frame; never waits on the worker.
Result LastResult();

namespace detail
{
inline char FoldPathChar(char c)
{
    if (c == '/')
        return '\\';
    if (c >= 'A' && c <= 'Z')
        return static_cast<char>(c + 32);
    return c;
}

inline char LowerAscii(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; }

inline bool IsSeparator(char c) { return c == '\\' || c == '/'; }

// A byte that belongs to a word for MaskWord: ASCII letters, digits, '_' and every UTF-8 byte of a non-ASCII letter.
inline bool IsWordByte(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '_' || u >= 0x80;
}

// Where a folder name inside a path ends: a separator, a quote, a line break or a character Windows forbids in names.
inline bool EndsPathPart(char c)
{
    switch (c)
    {
    case '\\':
    case '/':
    case '"':
    case '\'':
    case '\r':
    case '\n':
    case '\t':
    case '\0':
    case '<':
    case '>':
    case '|':
    case '*':
    case '?':
    case ':':
        return true;
    default:
        return false;
    }
}

inline bool EqualsIgnoringAsciiCase(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (LowerAscii(a[i]) != LowerAscii(b[i]))
            return false;
    return true;
}
} // namespace detail

// Every occurrence of `profileDir` in `text` (compared case-insensitively, '\\' and '/' alike) becomes
// "%USERPROFILE%": with profileDir C:\Users\Player, the text C:\Users\Player\x becomes %USERPROFILE%\x.
// Only where the folder name ends (a separator, a quote, a line break, a forbidden name character or the text's end):
// C:\Users\Player2\x is left to MaskUserFolders.
// An empty `profileDir` leaves the text as it is (so does one shorter than 3 characters after its trailing
// separators are dropped, which could only be a drive). UTF-8 in, UTF-8 out.
inline std::string MaskUserProfile(std::string_view text, std::string_view profileDir)
{
    while (!profileDir.empty() && detail::IsSeparator(profileDir.back()))
        profileDir.remove_suffix(1);
    if (profileDir.size() < 3)
        return std::string(text);

    std::string out;
    out.reserve(text.size());
    const char first = detail::FoldPathChar(profileDir.front());
    size_t i = 0;
    while (i < text.size())
    {
        if (detail::FoldPathChar(text[i]) == first && text.size() - i >= profileDir.size())
        {
            size_t k = 1;
            while (k < profileDir.size() && detail::FoldPathChar(text[i + k]) == detail::FoldPathChar(profileDir[k]))
                ++k;
            // Only a whole folder name: with profile C:\Users\Ali, C:\Users\Alice\x is another user (MaskUserFolders).
            if (k == profileDir.size() && (i + k == text.size() || detail::EndsPathPart(text[i + k])))
            {
                out += "%USERPROFILE%";
                i += k;
                continue;
            }
        }
        out.push_back(text[i]);
        ++i;
    }
    return out;
}

// Other people's folders: the name after "\Users\" or "/Users/" (any drive, any case, doubled separators too) and
// after "/home/" or "\home\" (Wine / Proton) becomes "<user>". Public, Default, Default User and All Users stay, and
// so does a name that is already masked (it starts with '%' or '<').
inline std::string MaskUserFolders(std::string_view text)
{
    static constexpr std::string_view kKeys[] = { "users", "home" };
    static constexpr std::string_view kKeep[] = { "public", "default", "default user", "all users" };

    std::string out;
    out.reserve(text.size());
    size_t i = 0;
    while (i < text.size())
    {
        out.push_back(text[i]);
        if (!detail::IsSeparator(text[i]))
        {
            ++i;
            continue;
        }
        ++i;
        // text[i - 1] is a separator: is a key folder next?
        for (std::string_view key : kKeys)
        {
            if (text.size() - i <= key.size() || !detail::EqualsIgnoringAsciiCase(text.substr(i, key.size()), key) ||
                !detail::IsSeparator(text[i + key.size()]))
                continue;
            size_t nameStart = i + key.size();
            while (nameStart < text.size() && detail::IsSeparator(text[nameStart]))
                ++nameStart;
            size_t nameEnd = nameStart;
            while (nameEnd < text.size() && !detail::EndsPathPart(text[nameEnd]))
                ++nameEnd;
            const std::string_view name = text.substr(nameStart, nameEnd - nameStart);
            bool keep = name.empty() || name.front() == '%' || name.front() == '<';
            for (std::string_view k : kKeep)
                keep = keep || detail::EqualsIgnoringAsciiCase(name, k);
            if (keep)
                break;
            out.append(text.substr(i, nameStart - i)); // the key folder and its separators
            out += "<user>";
            i = nameEnd;
            break;
        }
    }
    return out;
}

// Every whole-word occurrence of `word` (ASCII case ignored) becomes `replacement`. A word boundary is anything but
// an ASCII letter, digit, '_' or a non-ASCII byte. An empty `word` leaves the text as it is.
inline std::string MaskWord(std::string_view text, std::string_view word, std::string_view replacement)
{
    if (word.empty())
        return std::string(text);
    std::string out;
    out.reserve(text.size());
    size_t i = 0;
    while (i < text.size())
    {
        if (text.size() - i >= word.size() && detail::LowerAscii(text[i]) == detail::LowerAscii(word.front()) &&
            (i == 0 || !detail::IsWordByte(text[i - 1])) &&
            detail::EqualsIgnoringAsciiCase(text.substr(i, word.size()), word) &&
            (i + word.size() == text.size() || !detail::IsWordByte(text[i + word.size()])))
        {
            out.append(replacement);
            i += word.size();
            continue;
        }
        out.push_back(text[i]);
        ++i;
    }
    return out;
}

// The text as it is when it has at most head + tail bytes; else its first `head` and last `tail` bytes, both cut at a
// line break where one is near, with a "[... N bytes omitted ...]" line between them.
inline std::string CapHeadTail(std::string_view text, size_t head, size_t tail)
{
    if (text.size() <= head + tail)
        return std::string(text);
    size_t headEnd = head;
    if (const size_t nl = text.rfind('\n', head > 0 ? head - 1 : 0); nl != std::string_view::npos && nl + 1 >= head / 2)
        headEnd = head > 0 ? nl + 1 : 0;
    size_t tailStart = text.size() - tail;
    if (const size_t nl = text.find('\n', tailStart); nl != std::string_view::npos && nl + 1 - tailStart <= tail / 2)
        tailStart = nl + 1;
    std::string out(text.substr(0, headEnd));
    if (!out.empty() && out.back() != '\n')
        out.push_back('\n');
    out += "[... " + std::to_string(tailStart - headEnd) + " bytes omitted ...]\n";
    out.append(text.substr(tailStart));
    return out;
}

// The last `sessions` sessions of a log that marks each session start with `marker`: the text from the start of the
// line holding the sessions-th last marker. The whole text when it has that many or fewer. `omitted` = bytes cut.
inline std::string_view LastSessions(std::string_view text, std::string_view marker, size_t sessions, size_t& omitted)
{
    omitted = 0;
    if (marker.empty() || sessions == 0)
        return text;
    std::vector<size_t> starts;
    for (size_t pos = text.find(marker); pos != std::string_view::npos; pos = text.find(marker, pos + marker.size()))
        starts.push_back(pos);
    if (starts.size() <= sessions)
        return text;
    const size_t at = starts[starts.size() - sessions];
    const size_t nl = at > 0 ? text.rfind('\n', at - 1) : std::string_view::npos;
    const size_t lineStart = nl == std::string_view::npos ? 0 : nl + 1;
    omitted = lineStart;
    return text.substr(lineStart);
}

// A file-name-safe form of an exe stem for the zip's name: [A-Za-z0-9._-] kept, anything else '_'; "game" if empty.
inline std::string SafeFileStem(std::string_view stem)
{
    std::string out;
    for (char c : stem)
    {
        const bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' ||
                          c == '_' || c == '-';
        out.push_back(keep ? c : '_');
    }
    while (!out.empty() && out.back() == '.')
        out.pop_back();
    return out.empty() ? std::string("game") : out;
}
} // namespace AmdnrReport
