// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#pragma once

// Crash triage, second set (RE Requiem 1.3.1.0 on RDNA 4).
//
// DiagSwitches: accessors for the [Hotfix] Diag* kill switches declared in Config.h. Each one
// reads its key once, logs a single "[Diag]" INFO line the first time it is true and returns the
// cached value afterwards. All default to false = unchanged behaviour.
//
// DiagProbe: read-only helpers for the swapchain lines logged by the DiagNoSwapchainWrap path only
// (nothing is logged while every Diag* key is false). They only read memory and reference counts
// (AddRef immediately followed by Release); they never change what the game or DXGI get back.

#include <SysUtils.h>
#include <Config.h>

#include <Windows.h>
#include <dxgi1_4.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace DiagSwitches
{
inline bool NoFfxProxies()
{
    static const bool value = []
    {
        const bool v = Config::Instance()->DiagNoFfxProxies.value_or_default();
        if (v)
            LOG_INFO("[Diag] DiagNoFfxProxies=true: OptiScaler loads none of its FidelityFX DLLs, leaves the game's "
                     "amd_fidelityfx_* exports undetoured and lets the game load its own FidelityFX files (no FSR / "
                     "FSR-RR / FSR-FG inside OptiScaler, no FSR 4 model-selection hooks)");
        return v;
    }();

    return value;
}

inline bool NoXessProxies()
{
    static const bool value = []
    {
        const bool v = Config::Instance()->DiagNoXessProxies.value_or_default();
        if (v)
            LOG_INFO("[Diag] DiagNoXessProxies=true: no OptiScaler libxess/libxess_dx11/libxess_fg/libxell, no "
                     "detours or export redirects on the game's copies, no libxell GetModuleHandleExA spoof, no "
                     "XeSS-FG unlock patch (no XeSS / XeFG / XeLL inside OptiScaler)");
        return v;
    }();

    return value;
}

inline bool NoSwapchainWrap()
{
    static const bool value = []
    {
        const bool v = Config::Instance()->DiagNoSwapchainWrap.value_or_default();
        if (v)
            LOG_INFO("[Diag] DiagNoSwapchainWrap=true: the DXGI factory detours stay, but every swapchain goes back "
                     "to the game exactly as DXGI created it (no wrapper: no menu, no frame generation, no Present "
                     "work)");
        return v;
    }();

    return value;
}

inline bool NoOverlay()
{
    static const bool value = []
    {
        const bool v = Config::Instance()->DiagNoOverlay.value_or_default();
        if (v)
            LOG_INFO("[Diag] DiagNoOverlay=true: the swapchain wrapper stays, but its Present does no overlay work "
                     "(no ImGui, no render targets or back-buffer references, no overlay command lists on the "
                     "game's queue); the menu cannot be shown");
        return v;
    }();

    return value;
}

inline bool NoD3D12Hooks()
{
    static const bool value = []
    {
        const bool v = Config::Instance()->DiagNoD3D12Hooks.value_or_default();
        if (v)
            LOG_INFO("[Diag] DiagNoD3D12Hooks=true: no detours on d3d12.dll exports, D3D12Core D3D12GetInterface, "
                     "ID3D12Device methods or command lists (no DLSS-NR exposure scan, no root-signature restore, "
                     "no resource tracking); diagnosis only");
        return v;
    }();

    return value;
}

inline bool NoAmdGpuProbe()
{
    static const bool value = []
    {
        const bool v = Config::Instance()->DiagNoAmdGpuProbe.value_or_default();
        if (v)
            LOG_INFO("[Diag] DiagNoAmdGpuProbe=true: the GPU capability probe does not load amdxc64.dll, does not call "
                     "AmdExtD3DCreateInterface / CheckSupport(Float8Conversion) and does not force-release its probe "
                     "device; FSR 4 support comes from the card table");
        return v;
    }();

    return value;
}
} // namespace DiagSwitches

namespace DiagProbe
{
inline bool ReadBytes(const void* address, void* out, size_t size)
{
    SIZE_T read = 0;
    return address != nullptr && ReadProcessMemory(GetCurrentProcess(), address, out, size, &read) != FALSE &&
           read == size;
}

// "name.dll+0x1234", or "<no module>+0x...": anonymous memory such as a hook's relay page.
inline std::string AddressName(const void* address)
{
    if (address == nullptr)
        return "null";

    HMODULE module = nullptr;
    char buffer[400] {};

    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(address), &module) ||
        module == nullptr)
    {
        std::snprintf(buffer, sizeof(buffer), "<no module>@0x%llX", (unsigned long long) (uintptr_t) address);
        return buffer;
    }

    wchar_t path[MAX_PATH] {};
    const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
    std::wstring name = length > 0 ? std::wstring(path, length) : std::wstring(L"<unnamed>");
    if (const auto slash = name.find_last_of(L"\\/"); slash != std::wstring::npos)
        name = name.substr(slash + 1);

    std::snprintf(buffer, sizeof(buffer), "%s+0x%llX", wstring_to_string(name).c_str(),
                  (unsigned long long) ((uintptr_t) address - (uintptr_t) module));
    return buffer;
}

// Module of a COM object's vtable: dxgi.dll for a genuine DXGI object, something else for a proxy.
inline std::string VtableName(IUnknown* object)
{
    void* vtable = nullptr;
    if (object == nullptr || !ReadBytes(object, &vtable, sizeof(vtable)))
        return "unreadable";

    return AddressName(vtable);
}

inline void* VtableSlot(IUnknown* object, size_t slot)
{
    void* vtable = nullptr;
    void* entry = nullptr;
    if (object == nullptr || !ReadBytes(object, &vtable, sizeof(vtable)) ||
        !ReadBytes(static_cast<char*>(vtable) + slot * sizeof(void*), &entry, sizeof(entry)))
        return nullptr;

    return entry;
}

// Where a function really starts executing: the module of its address, its first bytes and, when
// those bytes are a jump (what a hook writes at a function entry), the chain of jump targets.
inline std::string DescribeCode(const void* function)
{
    std::string out;
    const unsigned char* p = static_cast<const unsigned char*>(function);

    for (int hop = 0; hop < 5 && p != nullptr; hop++)
    {
        unsigned char b[16] {};
        const bool readable = ReadBytes(p, b, sizeof(b));

        if (hop > 0)
            out += " -> ";

        out += AddressName(p);

        if (!readable)
        {
            out += " [unreadable]";
            break;
        }

        char bytes[64] {};
        std::snprintf(bytes, sizeof(bytes), " [%02X %02X %02X %02X %02X %02X %02X %02X]", b[0], b[1], b[2], b[3], b[4],
                      b[5], b[6], b[7]);
        out += bytes;

        const unsigned char* next = nullptr;

        if (b[0] == 0xE9) // jmp rel32
        {
            int32_t rel = 0;
            std::memcpy(&rel, b + 1, sizeof(rel));
            next = p + 5 + rel;
        }
        else if (b[0] == 0xEB) // jmp rel8
        {
            next = p + 2 + static_cast<int8_t>(b[1]);
        }
        else if (b[0] == 0xFF && b[1] == 0x25) // jmp [rip+disp32]
        {
            int32_t disp = 0;
            std::memcpy(&disp, b + 2, sizeof(disp));
            void* target = nullptr;
            if (ReadBytes(p + 6 + disp, &target, sizeof(target)))
                next = static_cast<const unsigned char*>(target);
        }
        else if (b[0] == 0x48 && b[1] == 0xB8 && b[10] == 0xFF && b[11] == 0xE0) // mov rax, imm64; jmp rax
        {
            std::memcpy(&next, b + 2, sizeof(next));
        }
        else if (b[0] == 0x49 && b[1] == 0xBB && b[10] == 0x41 && b[11] == 0xFF && b[12] == 0xE3) // mov r11; jmp r11
        {
            std::memcpy(&next, b + 2, sizeof(next));
        }
        else if (b[0] == 0x68 && b[5] == 0xC7 && b[6] == 0x44 && b[7] == 0x24 && b[8] == 0x04 &&
                 b[13] == 0xC3) // push imm32; mov dword [rsp+4], imm32; ret
        {
            uint32_t low = 0;
            uint32_t high = 0;
            std::memcpy(&low, b + 1, sizeof(low));
            std::memcpy(&high, b + 9, sizeof(high));
            next = reinterpret_cast<const unsigned char*>((uintptr_t) low | ((uintptr_t) high << 32));
        }

        if (next == nullptr)
            break;

        p = next;
    }

    return out;
}

inline ULONG RefCount(IUnknown* object)
{
    if (object == nullptr)
        return 0;

    object->AddRef();
    return object->Release();
}

// Two "[Diag]" INFO lines per swapchain the DiagNoSwapchainWrap path hands back unwrapped, logged
// straight after DXGI returned it: who asked, what the object really is, whether its Present /
// Present1 / ResizeBuffers entries are already hooked by someone below OptiScaler, its reference
// count and its description. Nothing calls this while every Diag* key is false.
inline void Swapchain(const char* path, IUnknown* swapchain, void* callerReturnAddress)
{
    if (swapchain == nullptr)
        return;

    // Count first, before any QueryInterface below.
    const ULONG refCount = RefCount(swapchain);

    std::string description = "no desc";
    IDXGISwapChain1* sc1 = nullptr;
    IDXGISwapChain* sc = nullptr;

    if (swapchain->QueryInterface(IID_PPV_ARGS(&sc1)) == S_OK && sc1 != nullptr)
    {
        DXGI_SWAP_CHAIN_DESC1 desc {};
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc {};
        const bool hasFs = sc1->GetFullscreenDesc(&fsDesc) == S_OK;

        if (sc1->GetDesc1(&desc) == S_OK)
        {
            char buffer[256] {};
            std::snprintf(buffer, sizeof(buffer),
                          "%ux%u fmt %u flags 0x%X buffers %u swapEffect %u scaling %u alpha %u usage 0x%X windowed %s",
                          desc.Width, desc.Height, (unsigned) desc.Format, desc.Flags, desc.BufferCount,
                          (unsigned) desc.SwapEffect, (unsigned) desc.Scaling, (unsigned) desc.AlphaMode,
                          (unsigned) desc.BufferUsage, hasFs ? (fsDesc.Windowed ? "yes" : "no") : "n/a");
            description = buffer;
        }

        sc1->Release();
    }
    else if (swapchain->QueryInterface(IID_PPV_ARGS(&sc)) == S_OK && sc != nullptr)
    {
        DXGI_SWAP_CHAIN_DESC desc {};
        if (sc->GetDesc(&desc) == S_OK)
        {
            char buffer[256] {};
            std::snprintf(buffer, sizeof(buffer), "%ux%u fmt %u flags 0x%X buffers %u swapEffect %u windowed %s",
                          desc.BufferDesc.Width, desc.BufferDesc.Height, (unsigned) desc.BufferDesc.Format, desc.Flags,
                          desc.BufferCount, (unsigned) desc.SwapEffect, desc.Windowed ? "yes" : "no");
            description = buffer;
        }

        sc->Release();
    }

    LOG_INFO("[Diag] {}: swapchain {:X} from caller {} | vtable {} | refCount {} right after DXGI returned it | {}",
             path, (uintptr_t) swapchain, AddressName(callerReturnAddress), VtableName(swapchain), refCount,
             description);
    LOG_INFO("[Diag] {}: swapchain {:X} Present = {} | Present1 = {} | ResizeBuffers = {}", path,
             (uintptr_t) swapchain, DescribeCode(VtableSlot(swapchain, 8)), DescribeCode(VtableSlot(swapchain, 22)),
             DescribeCode(VtableSlot(swapchain, 13)));
}
} // namespace DiagProbe
