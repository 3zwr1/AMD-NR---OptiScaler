// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text;
using AmdnrLauncher.Core.Scanning;

namespace AmdnrLauncher.Core.Tests;

public class PeImportsTests
{
    /// <summary>A complete, minimal Windows executable image: a DOS stub, the PE signature, the
    /// COFF and optional headers, and one .rdata section that holds the import directory, an
    /// optional delay-load directory and the DLL name strings they point at. Nothing else — no
    /// code, no entry point — because PeImports reads nothing else. Shared with ProxyPlannerTests,
    /// which drops one of these into a temp game folder as the game's exe.</summary>
    public static byte[] Build(bool pe32Plus, IReadOnlyList<string> imports, IReadOnlyList<string>? delayImports = null)
    {
        const int fileAlignment = 0x200;
        const int sectionRva = 0x1000;
        const int peOffset = 0x40;
        const int sizeOfHeaders = fileAlignment;
        var optionalSize = pe32Plus ? 0xF0 : 0xE0;
        var delays = delayImports ?? [];

        // Section layout, in RVA order: import descriptors, delay-load descriptors, one zeroed
        // thunk the descriptors can point at, then the names.
        var importDescriptors = 0;
        var delayDescriptors = (imports.Count + 1) * 20;
        var thunk = delayDescriptors + (delays.Count > 0 ? (delays.Count + 1) * 32 : 0);
        var names = thunk + 8;

        var nameRvas = new List<int>();
        var cursor = names;
        foreach (var name in imports.Concat(delays))
        {
            nameRvas.Add(sectionRva + cursor);
            cursor += name.Length + 1;
        }
        var contentSize = cursor;
        var content = new byte[(contentSize + fileAlignment - 1) / fileAlignment * fileAlignment];

        for (var i = 0; i < imports.Count; i++)
        {
            var at = importDescriptors + i * 20;
            Put32(content, at, sectionRva + thunk);          // OriginalFirstThunk
            Put32(content, at + 12, nameRvas[i]);            // Name
            Put32(content, at + 16, sectionRva + thunk);     // FirstThunk
        }
        for (var j = 0; j < delays.Count; j++)
        {
            var at = delayDescriptors + j * 32;
            Put32(content, at, 1);                           // Attributes: RVA-based
            Put32(content, at + 4, nameRvas[imports.Count + j]);
        }
        cursor = names;
        foreach (var name in imports.Concat(delays))
        {
            Encoding.ASCII.GetBytes(name).CopyTo(content, cursor);
            cursor += name.Length + 1;
        }

        var file = new byte[sizeOfHeaders + content.Length];
        file[0] = (byte)'M';
        file[1] = (byte)'Z';
        Put32(file, 0x3C, peOffset);

        file[peOffset] = (byte)'P';
        file[peOffset + 1] = (byte)'E';
        var coff = peOffset + 4;
        Put16(file, coff, pe32Plus ? 0x8664 : 0x014C);      // Machine
        Put16(file, coff + 2, 1);                            // NumberOfSections
        Put16(file, coff + 16, optionalSize);                // SizeOfOptionalHeader
        Put16(file, coff + 18, 0x0002);                      // Characteristics: executable image

        var optional = coff + 20;
        Put16(file, optional, pe32Plus ? 0x20B : 0x10B);     // Magic
        Put32(file, optional + 32, 0x1000);                  // SectionAlignment
        Put32(file, optional + 36, fileAlignment);           // FileAlignment
        Put32(file, optional + 56, 0x2000);                  // SizeOfImage
        Put32(file, optional + 60, sizeOfHeaders);           // SizeOfHeaders
        Put16(file, optional + 68, 3);                       // Subsystem: console
        var directories = optional + (pe32Plus ? 112 : 96);
        Put32(file, directories - 4, 16);                    // NumberOfRvaAndSizes
        Put32(file, directories + 1 * 8, sectionRva + importDescriptors);
        Put32(file, directories + 1 * 8 + 4, (imports.Count + 1) * 20);
        if (delays.Count > 0)
        {
            Put32(file, directories + 13 * 8, sectionRva + delayDescriptors);
            Put32(file, directories + 13 * 8 + 4, (delays.Count + 1) * 32);
        }

        var section = optional + optionalSize;
        Encoding.ASCII.GetBytes(".rdata").CopyTo(file, section);
        Put32(file, section + 8, contentSize);               // VirtualSize
        Put32(file, section + 12, sectionRva);               // VirtualAddress
        Put32(file, section + 16, content.Length);           // SizeOfRawData
        Put32(file, section + 20, sizeOfHeaders);            // PointerToRawData
        Put32(file, section + 36, 0x40000040);               // initialised data, readable

        content.CopyTo(file, sizeOfHeaders);
        return file;
    }

    private static void Put16(byte[] into, int at, int value) => BitConverter.GetBytes((ushort)value).CopyTo(into, at);
    private static void Put32(byte[] into, int at, int value) => BitConverter.GetBytes(value).CopyTo(into, at);

    [Fact]
    public void Reads_the_DLL_names_a_PE32_plus_exe_imports_including_delay_loaded_ones()
    {
        using var dir = new TempDir();
        var exe = dir.Write("Game.exe", Build(pe32Plus: true, ["KERNEL32.dll", "d3d12.dll"], ["dxgi.dll"]));

        Assert.Equal(["KERNEL32.dll", "d3d12.dll", "dxgi.dll"], PeImports.Read(exe));
    }

    [Fact]
    public void Reads_the_DLL_names_a_PE32_exe_imports()
    {
        using var dir = new TempDir();
        var exe = dir.Write("Game.exe", Build(pe32Plus: false, ["KERNEL32.dll", "winmm.dll"]));

        Assert.Equal(["KERNEL32.dll", "winmm.dll"], PeImports.Read(exe));
    }

    [Fact]
    public void Reads_notepads_imports_when_it_is_present()
    {
        // A real image from a real linker, with seven sections and a delay-load table. The
        // Windows 11 stub imports no KERNEL32.dll at all — only api-ms-win-* sets beside
        // USER32 and GDI32 — so those two are what every notepad build is known to name.
        var notepad = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "notepad.exe");
        if (!File.Exists(notepad)) return;

        var imports = PeImports.Read(notepad);

        Assert.Contains(imports, n => string.Equals(n, "USER32.dll", StringComparison.OrdinalIgnoreCase));
        Assert.Contains(imports, n => string.Equals(n, "GDI32.dll", StringComparison.OrdinalIgnoreCase));
        // Bare file names, each once: WINSPOOL.DRV is among them, so ".dll" is not a rule.
        Assert.All(imports, n =>
        {
            Assert.Equal(n, Path.GetFileName(n));
            Assert.Contains('.', n);
        });
        Assert.Equal(imports.Count, imports.Distinct(StringComparer.OrdinalIgnoreCase).Count());
    }

    [Fact]
    public void A_name_in_a_sections_virtual_only_tail_is_not_read_from_the_bytes_that_follow()
    {
        // VirtualSize may exceed SizeOfRawData: the tail is zero-filled when the image is loaded
        // and has no bytes in the file. An address in that tail used to be mapped all the same,
        // to a file offset that belongs to whatever follows the section in the file, and a name
        // could be read from those bytes. Here the section's raw size is cut back to end just
        // before the name string: the string is still in the file, but no section maps it.
        using var dir = new TempDir();
        var image = Build(pe32Plus: true, ["dxgi.dll"]);
        var section = 0x40 + 4 + 20 + 0xF0;              // the first section header
        var nameAt = 2 * 20 + 8;                          // two import descriptors and the thunk
        Put32(image, section + 16, nameAt);               // SizeOfRawData, was the aligned content
        var exe = dir.Write("Game.exe", image);

        Assert.Empty(PeImports.Read(exe));
    }

    [Fact]
    public void An_exe_with_an_empty_import_directory_imports_nothing()
    {
        using var dir = new TempDir();
        var exe = dir.Write("Game.exe", Build(pe32Plus: true, []));

        Assert.Empty(PeImports.Read(exe));
    }

    [Fact]
    public void The_same_name_in_two_spellings_is_reported_once()
    {
        using var dir = new TempDir();
        var exe = dir.Write("Game.exe", Build(pe32Plus: true, ["dxgi.dll", "DXGI.dll"], ["Dxgi.dll"]));

        Assert.Equal(["dxgi.dll"], PeImports.Read(exe));
    }

    [Fact]
    public void Returns_nothing_rather_than_throwing_for_what_is_not_a_readable_PE()
    {
        using var dir = new TempDir();

        // Missing, not a PE at all, a folder in the exe's place, and no path at all.
        Assert.Empty(PeImports.Read(Path.Combine(dir.Path, "missing.exe")));
        Assert.Empty(PeImports.Read(dir.Write("text.exe", "MZ but nothing after it that makes sense")));
        Assert.Empty(PeImports.Read(dir.Path));
        Assert.Empty(PeImports.Read(""));

        // Cut off inside the headers, and cut off inside the section the names live in.
        var whole = Build(pe32Plus: true, ["KERNEL32.dll"]);
        Assert.Empty(PeImports.Read(dir.Write("headers.exe", whole[..0x100])));
        Assert.Empty(PeImports.Read(dir.Write("section.exe", whole[..0x208])));

        // An import directory that points far past the end of the file: a lie the reader must
        // not follow into a read that never returns anything.
        var beyond = Build(pe32Plus: true, ["KERNEL32.dll"]);
        BitConverter.GetBytes(0x7FFF0000).CopyTo(beyond, 0x40 + 4 + 20 + 112 + 8);
        Assert.Empty(PeImports.Read(dir.Write("beyond.exe", beyond)));

        // A SizeOfOptionalHeader too small to hold even the magic number.
        var noOptional = Build(pe32Plus: true, ["KERNEL32.dll"]);
        Put16(noOptional, 0x40 + 4 + 16, 0);
        Assert.Empty(PeImports.Read(dir.Write("no-optional.exe", noOptional)));
    }

    [Fact]
    public void The_walk_stops_where_the_loader_stops_at_a_descriptor_with_no_name_or_no_thunk()
    {
        // Windows stops at the first descriptor whose Name or FirstThunk is zero, whatever its
        // other fields hold. One with a stray TimeDateStamp used to be walked past, and every
        // descriptor after it was then read as a real import.
        using var dir = new TempDir();
        var image = Build(pe32Plus: true, ["dxgi.dll", "d3d12.dll"]);
        const int first = 0x200;                          // the section's first descriptor
        Put32(image, first + 4, 0x12345678);              // TimeDateStamp
        Put32(image, first + 12, 0);                      // Name
        Put32(image, first + 16, 0);                      // FirstThunk
        var exe = dir.Write("Game.exe", image);

        Assert.Empty(PeImports.Read(exe));
    }

    [Fact]
    public void A_file_something_else_holds_open_exclusively_reads_as_importing_nothing()
    {
        using var dir = new TempDir();
        var exe = dir.Write("Game.exe", Build(pe32Plus: true, ["KERNEL32.dll"]));

        using (new FileStream(exe, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
            Assert.Empty(PeImports.Read(exe));
    }
}
