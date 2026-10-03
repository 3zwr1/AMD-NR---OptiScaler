// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
#pragma once
// A small stored-only (method 0) ZIP writer for the "Save report" zip (misc/AmdnrReport.h), with its own IEEE CRC-32.
// Never ImGui's CRC: ImHashData is CRC-32C (Castagnoli) and would fail every entry's check. No compression, no
// ZIP64, no dependency. Crc32 is header-only so the unit tests (tests\034) build it without the DLL.
//
// Signatures fixed by W0-2 (a change is a gate request); bodies W1-F (Crc32 here, the writer in ReportZip.cpp).

#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ReportZip
{
namespace detail
{
// The 256-entry table of the reflected IEEE polynomial, built at compile time.
constexpr std::array<uint32_t, 256> MakeCrc32Table()
{
    std::array<uint32_t, 256> table {};
    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t c = i;
        for (int bit = 0; bit < 8; ++bit)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        table[i] = c;
    }
    return table;
}
inline constexpr std::array<uint32_t, 256> kCrc32Table = MakeCrc32Table();
} // namespace detail

// IEEE 802.3 CRC-32 (reflected polynomial 0xEDB88320, initial value and final xor 0xFFFFFFFF), as ZIP stores it.
// Incremental: pass the previous result as `crc` to continue (0 to start). Crc32("123456789", 9) == 0xCBF43926.
inline uint32_t Crc32(const void* data, size_t size, uint32_t crc = 0)
{
    if (data == nullptr || size == 0)
        return crc;
    const auto* p = static_cast<const uint8_t*>(data);
    uint32_t c = ~crc;
    for (size_t i = 0; i < size; ++i)
        c = detail::kCrc32Table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    return ~c;
}

// Writes one archive. Each entry is handed over whole, so its CRC and sizes go into its local header (no data
// descriptor). Names are UTF-8 (general-purpose flag bit 11), '/' separated, never absolute, never "..".
class Writer
{
  public:
    Writer() = default;
    ~Writer(); // closes an archive left open (the file is then incomplete; the caller deletes it)
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    // Creates `tmpPath` (replacing it). The caller moves it to its final name after Close() succeeded.
    bool Open(const std::wstring& tmpPath);
    // One stored entry; `data` may be null when `size` is 0. `mtime` is the entry's time (UTC FILETIME).
    // False (and the archive is failed, see Close) for a name that is empty, absolute, has '\\', ':', a control
    // character, an empty or ".." part, a trailing '/', repeats an earlier name (ASCII case ignored), or for an
    // entry that would need ZIP64 (4 GiB, 65535 entries).
    bool Add(std::string_view utf8Name, const uint8_t* data, size_t size, FILETIME mtime);
    // Central directory and end record, then closes the file. False when anything failed since Open().
    bool Close();

  private:
    struct Entry
    {
        std::string name;
        uint32_t crc = 0;
        uint32_t size = 0;
        uint32_t offset = 0; // of the local header
        uint16_t dosTime = 0;
        uint16_t dosDate = 0;
    };

    bool Put(const void* data, size_t size); // buffered; direct for large blocks
    bool Flush();
    void Abandon(); // closes the handle without writing the directory

    HANDLE file_ = INVALID_HANDLE_VALUE;
    std::vector<uint8_t> buffer_;
    std::vector<Entry> entries_;
    uint64_t offset_ = 0; // bytes handed to Put so far = the archive's size once flushed
    bool failed_ = false;
};
} // namespace ReportZip
