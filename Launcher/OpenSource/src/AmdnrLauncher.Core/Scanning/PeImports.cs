// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text;

namespace AmdnrLauncher.Core.Scanning;

/// <summary>The DLL names a program asks Windows for when it starts, read out of its import
/// directory. A game that imports dxgi.dll itself will load a dxgi.dll sitting beside it, which
/// is how an OptiScaler proxy gets in — so the import list is the closest thing to knowing the
/// right proxy name before the game has ever been run with one.
///
/// <para>Only the headers and the import tables are read, through a FileStream, a few hundred
/// bytes at a time: re9.exe is 587 MB and a game folder can be on a slow drive, so the file is
/// never loaded and never read past what the headers point at. Both PE32 and PE32+ images are
/// understood, and delay-load imports are included when they are RVA-based, which every
/// current linker writes. Anything that is not a readable, well-formed image reads as importing
/// nothing: a wrong or missing answer here costs a less good first guess at the proxy name,
/// never a failed scan.</para></summary>
public static class PeImports
{
    /// <summary>The PE format allows 96 sections; a header claiming more is not one.</summary>
    private const int MaxSections = 96;

    /// <summary>Descriptors are read until a zero terminator, so a table without one has to
    /// end somewhere. No real program imports anywhere near this many DLLs.</summary>
    private const int MaxDescriptors = 1024;

    /// <summary>A DLL name is a file name: MAX_PATH bounds it.</summary>
    private const int MaxNameLength = 260;

    /// <summary>The PE header follows a DOS stub that is a few hundred bytes in every real
    /// image; a pointer a megabyte in is corruption, and a seek past the file's end would follow.</summary>
    private const int MaxHeaderOffset = 1 << 20;

    private const int MaxOptionalHeaderSize = 4096;

    private const int ImportDirectory = 1;
    private const int DelayImportDirectory = 13;

    public static IReadOnlyList<string> Read(string exePath)
    {
        try
        {
            using var stream = new FileStream(exePath, FileMode.Open, FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete, bufferSize: 4096);
            return ReadImports(stream);
        }
        // The game may be running, the folder may be on an unplugged drive, the exe may be a
        // folder or an empty name: all ordinary during a scan, and none of them is this
        // reader's problem to report. UnauthorizedAccessException is not an IOException.
        catch (Exception e) when (e is IOException or UnauthorizedAccessException
                                    or ArgumentException or NotSupportedException)
        {
            return [];
        }
    }

    private static IReadOnlyList<string> ReadImports(FileStream stream)
    {
        var dos = ReadAt(stream, 0, 64);
        if (dos is null || dos[0] != (byte)'M' || dos[1] != (byte)'Z') return [];

        var peOffset = BitConverter.ToInt32(dos, 0x3C);
        if (peOffset < 0x40 || peOffset > MaxHeaderOffset) return [];

        // Signature and the 20-byte COFF header together.
        var coff = ReadAt(stream, peOffset, 24);
        if (coff is null || coff[0] != (byte)'P' || coff[1] != (byte)'E' || coff[2] != 0 || coff[3] != 0) return [];

        int sectionCount = BitConverter.ToUInt16(coff, 4 + 2);
        int optionalSize = BitConverter.ToUInt16(coff, 4 + 16);
        if (sectionCount == 0 || sectionCount > MaxSections) return [];

        // Two bytes is the least that holds the magic number read first from it.
        if (optionalSize < 2 || optionalSize > MaxOptionalHeaderSize) return [];

        long optionalOffset = peOffset + 24L;
        var optional = ReadAt(stream, optionalOffset, optionalSize);
        if (optional is null) return [];

        // PE32 and PE32+ agree on every field up to SizeOfHeaders; the data directories sit
        // sixteen bytes later in PE32+ because its four stack and heap sizes are eight bytes each.
        var directoriesAt = BitConverter.ToUInt16(optional, 0) switch
        {
            0x10B => 96,
            0x20B => 112,
            _ => 0,
        };
        if (directoriesAt == 0 || optionalSize < directoriesAt) return [];

        var sizeOfHeaders = BitConverter.ToUInt32(optional, 60);
        var directoryCount = BitConverter.ToUInt32(optional, directoriesAt - 4);

        var table = ReadAt(stream, optionalOffset + optionalSize, sectionCount * 40);
        if (table is null) return [];

        // A section maps only as many bytes as it has in the file. VirtualSize can be larger
        // (the tail is zero-filled at load time, so nothing can be read from it) or zero in an
        // odd image; SizeOfRawData is what the file offset can be trusted for either way, and
        // an address past it would otherwise land in whatever follows the section in the file.
        var sections = new List<Section>(sectionCount);
        for (var i = 0; i < sectionCount; i++)
        {
            var at = i * 40;
            sections.Add(new Section(
                VirtualAddress: BitConverter.ToUInt32(table, at + 12),
                RawSize: BitConverter.ToUInt32(table, at + 16),
                RawPointer: BitConverter.ToUInt32(table, at + 20)));
        }

        var image = new Image(stream, sections, sizeOfHeaders, optional, directoriesAt, directoryCount);
        var names = new List<string>();
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        // IMAGE_IMPORT_DESCRIPTOR: 20 bytes, the name's RVA at 12, FirstThunk at 16.
        foreach (var name in image.Names(ImportDirectory, entrySize: 20, nameAt: 12, thunkAt: 16, rvaFlagAt: null))
            if (seen.Add(name)) names.Add(name);

        // IMAGE_DELAYLOAD_DESCRIPTOR: 32 bytes, attributes at 0 whose low bit says the fields
        // are RVAs, the name's RVA at 4. The old VA-based form cannot be resolved without the
        // image base and is skipped.
        foreach (var name in image.Names(DelayImportDirectory, entrySize: 32, nameAt: 4, thunkAt: null, rvaFlagAt: 0))
            if (seen.Add(name)) names.Add(name);

        return names;
    }

    private sealed record Section(uint VirtualAddress, uint RawSize, uint RawPointer);

    private sealed class Image(
        FileStream stream,
        IReadOnlyList<Section> sections,
        uint sizeOfHeaders,
        byte[] optional,
        int directoriesAt,
        uint directoryCount)
    {
        /// <summary>The names in one descriptor table. The table ends where the loader stops:
        /// at the first descriptor with no name — or, with <paramref name="thunkAt"/> given, no
        /// thunk either — whatever its other fields hold. Requiring every byte to be zero walked
        /// past a terminator with a stray TimeDateStamp in it, into whatever followed the table.</summary>
        public IEnumerable<string> Names(int directory, int entrySize, int nameAt, int? thunkAt, int? rvaFlagAt)
        {
            if (directory >= directoryCount) yield break;
            var at = directoriesAt + directory * 8;
            if (at + 8 > optional.Length) yield break;

            var rva = BitConverter.ToUInt32(optional, at);
            if (rva == 0) yield break;

            var offset = ToFileOffset(rva);
            if (offset is null) yield break;

            for (var i = 0; i < MaxDescriptors; i++)
            {
                var entry = ReadAt(stream, offset.Value + (long)i * entrySize, entrySize);
                if (entry is null) yield break;

                var nameRva = BitConverter.ToUInt32(entry, nameAt);
                if (nameRva == 0) yield break;
                if (thunkAt is { } thunk && BitConverter.ToUInt32(entry, thunk) == 0) yield break;

                if (rvaFlagAt is { } flag && (BitConverter.ToUInt32(entry, flag) & 1) == 0) continue;

                var nameOffset = ToFileOffset(nameRva);
                if (nameOffset is null) continue;

                var name = ReadName(stream, nameOffset.Value);
                if (name is not null) yield return name;
            }
        }

        /// <summary>Where in the file a virtual address lands: inside the section that maps it,
        /// or — for an address below the end of the headers, which are mapped as they are — at
        /// the address itself. Null for an address no section maps, which a corrupt or lying
        /// header can hold.</summary>
        private long? ToFileOffset(uint rva)
        {
            foreach (var section in sections)
            {
                if (rva >= section.VirtualAddress && rva - section.VirtualAddress < section.RawSize)
                    return section.RawPointer + (long)(rva - section.VirtualAddress);
            }

            return rva < sizeOfHeaders ? rva : null;
        }
    }

    /// <summary>Exactly <paramref name="count"/> bytes at <paramref name="offset"/>, or null when
    /// the file is not that long: a truncated image is answered, not thrown over.</summary>
    private static byte[]? ReadAt(FileStream stream, long offset, int count)
    {
        if (offset < 0 || count < 0 || offset + count > stream.Length) return null;

        var buffer = new byte[count];
        stream.Seek(offset, SeekOrigin.Begin);
        stream.ReadExactly(buffer);
        return buffer;
    }

    /// <summary>A NUL-terminated ASCII file name, or null when what is there is not one: no
    /// terminator within MAX_PATH, an empty string, or a byte no file name contains.</summary>
    private static string? ReadName(FileStream stream, long offset)
    {
        if (offset < 0 || offset >= stream.Length) return null;

        var count = (int)Math.Min(MaxNameLength, stream.Length - offset);
        var buffer = new byte[count];
        stream.Seek(offset, SeekOrigin.Begin);
        stream.ReadExactly(buffer);

        var end = Array.IndexOf(buffer, (byte)0);
        if (end <= 0) return null;

        for (var i = 0; i < end; i++)
        {
            if (buffer[i] < 0x20 || buffer[i] > 0x7E) return null;
        }

        return Encoding.ASCII.GetString(buffer, 0, end);
    }
}
