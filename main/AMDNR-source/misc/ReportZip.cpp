// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
#include "pch.h"

#include "ReportZip.h"

// Stored ZIP (APPNOTE 6.3): per entry a local header (0x04034b50) + name + data, then one central directory
// record (0x02014b50) per entry and the end record (0x06054b50). Version 2.0, method 0, flag bit 11 (UTF-8
// names), no data descriptor (the CRC and sizes are known before the header is written), no extra fields.
namespace ReportZip
{
namespace
{
constexpr size_t kBufferSize = 64 * 1024;
constexpr uint16_t kVersion = 20;      // 2.0: stored entries in folders
constexpr uint16_t kFlagUtf8 = 0x0800; // general-purpose bit 11
constexpr uint32_t kMaxZip32 = 0xFFFFFFFEu;
constexpr size_t kMaxEntries = 0xFFFF;
constexpr char kComment[] = "AMDNR report";

void Le16(std::vector<uint8_t>& out, uint16_t v)
{
    out.push_back(static_cast<uint8_t>(v));
    out.push_back(static_cast<uint8_t>(v >> 8));
}
void Le32(std::vector<uint8_t>& out, uint32_t v)
{
    for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<uint8_t>(v >> (8 * i)));
}
void Bytes(std::vector<uint8_t>& out, std::string_view s) { out.insert(out.end(), s.begin(), s.end()); }

char LowerAscii(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; }

bool SameNameIgnoringAsciiCase(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (LowerAscii(a[i]) != LowerAscii(b[i]))
            return false;
    return true;
}

// '/'-separated relative name: no '\\', ':', control characters, empty or ".." parts, leading or trailing '/'.
bool ValidName(std::string_view name)
{
    if (name.empty() || name.size() > 0xFFFF || name.front() == '/' || name.back() == '/')
        return false;
    size_t partStart = 0;
    for (size_t i = 0; i <= name.size(); ++i)
    {
        if (i == name.size() || name[i] == '/')
        {
            const std::string_view part = name.substr(partStart, i - partStart);
            if (part.empty() || part == "..")
                return false;
            partStart = i + 1;
            continue;
        }
        const unsigned char c = static_cast<unsigned char>(name[i]);
        if (c < 0x20 || c == 0x7F || c == '\\' || c == ':')
            return false;
    }
    return true;
}

// MS-DOS date and time of a UTC FILETIME in local time; 1980-01-01 00:00 when it cannot be expressed.
void DosTime(FILETIME utc, uint16_t& dosTime, uint16_t& dosDate)
{
    dosTime = 0;
    dosDate = (0 << 9) | (1 << 5) | 1; // 1980-01-01
    FILETIME local {};
    WORD d = 0, t = 0;
    if ((utc.dwHighDateTime != 0 || utc.dwLowDateTime != 0) && FileTimeToLocalFileTime(&utc, &local) &&
        FileTimeToDosDateTime(&local, &d, &t))
    {
        dosTime = t;
        dosDate = d;
    }
}
} // namespace

Writer::~Writer() { Abandon(); }

void Writer::Abandon()
{
    if (file_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file_);
        file_ = INVALID_HANDLE_VALUE;
    }
    buffer_.clear();
}

bool Writer::Open(const std::wstring& tmpPath)
{
    Abandon();
    entries_.clear();
    offset_ = 0;
    failed_ = false;

    file_ = CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_ == INVALID_HANDLE_VALUE)
    {
        failed_ = true;
        return false;
    }
    buffer_.reserve(kBufferSize);
    return true;
}

bool Writer::Flush()
{
    size_t done = 0;
    while (done < buffer_.size())
    {
        DWORD written = 0;
        const DWORD chunk = static_cast<DWORD>(buffer_.size() - done);
        if (!WriteFile(file_, buffer_.data() + done, chunk, &written, nullptr) || written == 0)
        {
            failed_ = true;
            return false;
        }
        done += written;
    }
    buffer_.clear();
    return true;
}

bool Writer::Put(const void* data, size_t size)
{
    if (size == 0)
        return true;
    const auto* p = static_cast<const uint8_t*>(data);
    offset_ += size;
    if (buffer_.size() + size <= kBufferSize)
    {
        buffer_.insert(buffer_.end(), p, p + size);
        return true;
    }
    if (!Flush())
        return false;
    if (size < kBufferSize)
    {
        buffer_.insert(buffer_.end(), p, p + size);
        return true;
    }
    size_t done = 0;
    while (done < size)
    {
        DWORD written = 0;
        const size_t left = size - done;
        const DWORD chunk = static_cast<DWORD>(left > (1u << 24) ? (1u << 24) : left);
        if (!WriteFile(file_, p + done, chunk, &written, nullptr) || written == 0)
        {
            failed_ = true;
            return false;
        }
        done += written;
    }
    return true;
}

bool Writer::Add(std::string_view utf8Name, const uint8_t* data, size_t size, FILETIME mtime)
{
    if (file_ == INVALID_HANDLE_VALUE || failed_)
    {
        failed_ = true;
        return false;
    }
    if (!ValidName(utf8Name) || (data == nullptr && size != 0) || entries_.size() >= kMaxEntries ||
        size > kMaxZip32 || offset_ + 30 + utf8Name.size() + size > kMaxZip32)
    {
        failed_ = true;
        return false;
    }
    for (const Entry& e : entries_)
    {
        if (SameNameIgnoringAsciiCase(e.name, utf8Name))
        {
            failed_ = true;
            return false;
        }
    }

    Entry entry;
    entry.name = std::string(utf8Name);
    entry.crc = Crc32(data, size);
    entry.size = static_cast<uint32_t>(size);
    entry.offset = static_cast<uint32_t>(offset_);
    DosTime(mtime, entry.dosTime, entry.dosDate);

    std::vector<uint8_t> header;
    header.reserve(30 + entry.name.size());
    Le32(header, 0x04034b50u);
    Le16(header, kVersion);
    Le16(header, kFlagUtf8);
    Le16(header, 0); // stored
    Le16(header, entry.dosTime);
    Le16(header, entry.dosDate);
    Le32(header, entry.crc);
    Le32(header, entry.size); // compressed = uncompressed
    Le32(header, entry.size);
    Le16(header, static_cast<uint16_t>(entry.name.size()));
    Le16(header, 0); // no extra field
    Bytes(header, entry.name);

    if (!Put(header.data(), header.size()) || !Put(data, size))
        return false;
    entries_.push_back(std::move(entry));
    return true;
}

bool Writer::Close()
{
    if (file_ == INVALID_HANDLE_VALUE)
        return false;

    bool ok = !failed_;
    if (ok)
    {
        const uint64_t directoryStart = offset_;
        std::vector<uint8_t> record;
        for (const Entry& e : entries_)
        {
            record.clear();
            Le32(record, 0x02014b50u);
            Le16(record, kVersion); // made by: MS-DOS host (upper byte 0), 2.0
            Le16(record, kVersion);
            Le16(record, kFlagUtf8);
            Le16(record, 0); // stored
            Le16(record, e.dosTime);
            Le16(record, e.dosDate);
            Le32(record, e.crc);
            Le32(record, e.size);
            Le32(record, e.size);
            Le16(record, static_cast<uint16_t>(e.name.size()));
            Le16(record, 0); // extra
            Le16(record, 0); // comment
            Le16(record, 0); // disk number
            Le16(record, 0); // internal attributes
            Le32(record, 0); // external attributes
            Le32(record, e.offset);
            Bytes(record, e.name);
            if (!Put(record.data(), record.size()))
                break;
        }
        const uint64_t directorySize = offset_ - directoryStart;
        if (!failed_ && directoryStart <= kMaxZip32 && directorySize <= kMaxZip32)
        {
            record.clear();
            Le32(record, 0x06054b50u);
            Le16(record, 0); // this disk
            Le16(record, 0); // disk of the directory
            Le16(record, static_cast<uint16_t>(entries_.size()));
            Le16(record, static_cast<uint16_t>(entries_.size()));
            Le32(record, static_cast<uint32_t>(directorySize));
            Le32(record, static_cast<uint32_t>(directoryStart));
            Le16(record, static_cast<uint16_t>(sizeof(kComment) - 1));
            Bytes(record, std::string_view(kComment, sizeof(kComment) - 1));
            Put(record.data(), record.size());
            Flush();
        }
        else
            failed_ = true;
        ok = !failed_;
    }

    if (!CloseHandle(file_))
        ok = false;
    file_ = INVALID_HANDLE_VALUE;
    buffer_.clear();
    entries_.clear();
    return ok;
}
} // namespace ReportZip
