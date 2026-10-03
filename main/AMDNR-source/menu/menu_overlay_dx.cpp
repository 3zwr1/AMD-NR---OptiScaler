// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#include "pch.h"
#include <dlssnr/amd/PresentExperimental.h>
#include "menu_overlay_base.h"
#include "menu_overlay_dx.h"

#include <Util.h>
#include <Logger.h>
#include <Config.h>

#include <proxies/XeFGPacing.h>

#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_win32.h>
#include <misc/DiagSwitches.h>

// menu
static int const NUM_BACK_BUFFERS = 8;
static int const SRV_HEAP_SIZE = 64;
static bool _dx11Device = false;
static bool _dx12Device = false;

// for dx11
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static ID3D11RenderTargetView* g_pd3dRenderTarget = nullptr;

// for dx12
static ID3D12Device* g_pd3dDeviceParam = nullptr;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static DescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static ID3D12CommandAllocator* g_commandAllocators[NUM_BACK_BUFFERS] = {};
static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

// current command queue for dx12 swapchain
static IUnknown* currentSCCommandQueue = nullptr;

// AMDNR 0.3.3 (MFG stutter, D). The eight allocators above used to be picked by
// GetCurrentBackBufferIndex(), so with a three-buffer swapchain only three were ever used,
// and each was Reset with no fence - about every 10 ms at 300 presents/s, while the GPU
// was 10 ms or more behind on the present queue. All 43 of the exact 100 ms stalls in the
// Silent Hill 2 9X/10X logs fell inside this overlay. Now the eight are taken in turn and
// each carries the fence value its last submission signalled: an allocator is only reused
// once the GPU has passed that value, nothing ever waits on the fence, and when the next
// allocator is still in flight this present goes without the overlay instead.
//
// Only the next one in turn is ever considered. One queue retires in order, so if it is
// not done none of the newer ones is either; and ImGui's own eight vertex/index buffer
// sets (NumFramesInFlight = NUM_BACK_BUFFERS) advance once per draw too, so they stay
// covered by the same fences. If the fence cannot be created, or a Signal fails, the
// overlay goes back to the old per-back-buffer allocators.
//
// Known gap, left for 0.3.4 (review of 0.3.3; not a regression, the old code had no fence
// at all). ImGui's TEXTURES are not covered the same way. A skipped present still runs
// ImGui::NewFrame (RenderMenu) and ImGui::Render before the fence check, and ImGui 1.92
// counts every NewFrame toward a WantDestroy texture's UnusedFrames (imgui_draw.cpp), while
// the DX12 backend destroys it at UnusedFrames >= NumFramesInFlight inside RenderDrawData
// (imgui_impl_dx12.cpp). After skips, that draw's fence only proves the draw eight DRAWS
// back is done, not the last draw that sampled the texture. It needs a replaced font atlas
// (new glyphs or a new scale, e.g. the menu opening) while the GPU is more than eight
// overlay draws behind (9X/10X). Fix for 0.3.4: do the next-allocator check before
// ImGui_ImplDX12_NewFrame / RenderMenu (RenderMenu also runs the hotkeys and frame timing,
// so that needs its own pass), or hold a WantDestroy texture until the fence value of the
// last draw that used it has completed.
static ID3D12Fence* g_overlayFence = nullptr;
static UINT64 g_overlayFenceValue = 0;
static UINT64 g_allocatorFenceValues[NUM_BACK_BUFFERS] = {};
static UINT g_nextAllocator = 0;

// The rate-limited WARN timing (D): which step of an overlay present took over 20 ms,
// and how many presents went without the overlay. One line a second at most.
static constexpr double OverlaySlowStepMs = 20.0;
static int64_t g_overlayLastWarnQpc = 0;
static uint32_t g_overlaySlowSuppressed = 0;
static uint32_t g_overlaySkipped = 0;

static int64_t OverlayQpcNow()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return now.QuadPart;
}

static double OverlayQpcFrequency()
{
    static const double frequency = []()
    {
        LARGE_INTEGER f {};
        QueryPerformanceFrequency(&f);
        return f.QuadPart > 0 ? static_cast<double>(f.QuadPart) : 1.0;
    }();

    return frequency;
}

static double OverlayMs(int64_t qpc) { return (qpc * 1000.0) / OverlayQpcFrequency(); }

// True when a WARN may go out now; otherwise counts it for the next one.
static bool OverlayMayWarn(int64_t nowQpc)
{
    if (g_overlayLastWarnQpc != 0 && (nowQpc - g_overlayLastWarnQpc) < static_cast<int64_t>(OverlayQpcFrequency()))
        return false;

    g_overlayLastWarnQpc = nowQpc;
    return true;
}

static void WarnOverlayTimes(int64_t menu, int64_t render, int64_t reset, int64_t draw, int64_t submit)
{
    if (OverlayMs(menu) <= OverlaySlowStepMs && OverlayMs(render) <= OverlaySlowStepMs &&
        OverlayMs(reset) <= OverlaySlowStepMs && OverlayMs(draw) <= OverlaySlowStepMs &&
        OverlayMs(submit) <= OverlaySlowStepMs)
        return;

    if (!OverlayMayWarn(OverlayQpcNow()))
    {
        g_overlaySlowSuppressed++;
        return;
    }

    LOG_WARN("Overlay present step over {:.0f} ms: menu {:.1f}, ImGui::Render {:.1f}, allocator reset {:.1f}, "
             "draw {:.1f} (texture uploads included), submit {:.1f} ms; {} more slow and {} overlay draws "
             "skipped since the last warning",
             OverlaySlowStepMs, OverlayMs(menu), OverlayMs(render), OverlayMs(reset), OverlayMs(draw),
             OverlayMs(submit), g_overlaySlowSuppressed, g_overlaySkipped);

    g_overlaySlowSuppressed = 0;
    g_overlaySkipped = 0;
}

static void NoteOverlaySkipped()
{
    g_overlaySkipped++;

    if (!OverlayMayWarn(OverlayQpcNow()))
        return;

    LOG_WARN("Overlay skipped on this present: all {} command allocators are still in flight on the GPU "
             "({} skipped since the last warning)",
             NUM_BACK_BUFFERS, g_overlaySkipped);

    g_overlaySkipped = 0;
}

// Once a second, while XeFG runs at 3X or above: what DXGI says it displayed (D). The
// deltas over five seconds give the real refresh rate and how many presents made it to
// the screen, next to how many were made. One INFO line per five seconds; nothing at 2X
// or without frame generation.
static void SamplePresentStatistics(IDXGISwapChain* swapChain)
{
    static int64_t nextSampleQpc = 0;
    static int32_t samples = 0;
    static uint32_t failures = 0;
    static uint32_t presents = 0;
    static bool haveFirst = false;
    static DXGI_FRAME_STATISTICS first {};

    if (swapChain == nullptr || !XeFGPacing::MfgActive())
    {
        nextSampleQpc = 0;
        samples = 0;
        failures = 0;
        presents = 0;
        haveFirst = false;
        return;
    }

    presents++;

    const int64_t now = OverlayQpcNow();

    if (nextSampleQpc != 0 && now < nextSampleQpc)
        return;

    nextSampleQpc = now + static_cast<int64_t>(OverlayQpcFrequency());

    DXGI_FRAME_STATISTICS stats {};

    if (swapChain->GetFrameStatistics(&stats) != S_OK)
    {
        failures++;
        return;
    }

    if (!haveFirst)
    {
        first = stats;
        haveFirst = true;
        samples = 0;
        presents = 0;
        failures = 0;
        return;
    }

    if (++samples < 5)
        return;

    const double seconds =
        static_cast<double>(stats.SyncQPCTime.QuadPart - first.SyncQPCTime.QuadPart) / OverlayQpcFrequency();
    const UINT refreshes = stats.SyncRefreshCount - first.SyncRefreshCount;

    LOG_INFO("XeFG present statistics: {} presents made, {} counted by DXGI, {} refreshes in {:.2f} s ({:.1f} Hz); "
             "{} samples failed",
             presents, stats.PresentCount - first.PresentCount, refreshes, seconds,
             seconds > 0.0 ? refreshes / seconds : 0.0, failures);

    first = stats;
    samples = 0;
    presents = 0;
    failures = 0;
}

// status
static bool _isInited = false;
static bool _d3d12Captured = false;

// for showing
static bool _showRenderImGuiDebugOnce = true;

static IID streamlineRiid {};
static bool CheckForRealObject(std::string functionName, IUnknown* pObject, IUnknown** ppRealObject)
{
    if (streamlineRiid.Data1 == 0)
    {
        auto iidResult = IIDFromString(L"{ADEC44E2-61F0-45C3-AD9F-1B37379284FF}", &streamlineRiid);

        if (iidResult != S_OK)
            return false;
    }

    auto qResult = pObject->QueryInterface(streamlineRiid, (void**) ppRealObject);

    if (qResult == S_OK && *ppRealObject != nullptr)
    {
        LOG_INFO("{} Streamline proxy found!", functionName);
        (*ppRealObject)->Release();
        return true;
    }

    return false;
}

static int GetCorrectDXGIFormat(int eCurrentFormat)
{
    switch (eCurrentFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    return eCurrentFormat;
}

static void CreateRenderTargetDx12(ID3D12Device* device, IDXGISwapChain* pSwapChain)
{
    LOG_FUNC();

    DXGI_SWAP_CHAIN_DESC sd;
    HRESULT hr = pSwapChain->GetDesc(&sd);

    if (hr != S_OK)
    {
        LOG_ERROR("pSwapChain->GetDesc: {0:X}", (unsigned long) hr);
        return;
    }

    for (UINT i = 0; i < sd.BufferCount; ++i)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        auto result = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));

        if (pBackBuffer != nullptr)
            pBackBuffer->Release();

        if (result != S_OK)
        {
            LOG_ERROR("pSwapChain->GetBuffer: {:X}", (unsigned long) result);
            return;
        }

        if (pBackBuffer != nullptr)
        {
            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = static_cast<DXGI_FORMAT>(GetCorrectDXGIFormat(sd.BufferDesc.Format));
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            device->CreateRenderTargetView(pBackBuffer, &desc, g_mainRenderTargetDescriptor[i]);
            g_mainRenderTargetResource[i] = pBackBuffer;
        }
    }

    LOG_INFO("done!");
}

static void CleanupRenderTargetDx12(bool clearQueue)
{
    if (!_isInited || !_dx12Device || State::Instance().isShuttingDown)
        return;

    LOG_TRACE("clearQueue: {}", clearQueue);

    for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i)
    {
        if (g_mainRenderTargetResource[i])
            g_mainRenderTargetResource[i] = nullptr;
    }

    if (clearQueue)
    {
        if (MenuOverlayBase::IsInited() && g_pd3dDeviceParam != nullptr && g_pd3dSrvDescHeap != nullptr &&
            ImGui::GetIO().BackendRendererUserData)
        {
            // std::this_thread::sleep_for(std::chrono::milliseconds(500));
            ImGui_ImplDX12_Shutdown(false);
        }

        SAFE_RELEASE(g_pd3dRtvDescHeap);
        SAFE_RELEASE(g_pd3dSrvDescHeap);

        for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i)
        {
            SAFE_RELEASE(g_commandAllocators[i]);
            g_allocatorFenceValues[i] = 0;
        }

        // AMDNR 0.3.3: the allocators' fence goes with them.
        SAFE_RELEASE(g_overlayFence);
        g_overlayFenceValue = 0;
        g_nextAllocator = 0;

        SAFE_RELEASE(g_pd3dCommandList);

        if (g_pd3dCommandQueue != nullptr)
            g_pd3dCommandQueue = nullptr;

        g_pd3dSrvDescHeapAlloc.Destroy();

        // SAFE_RELEASE(g_pd3dDeviceParam);

        _dx12Device = false;
        _isInited = false;
    }
}

static void CreateRenderTargetDx11(IDXGISwapChain* pSwapChain)
{
    ID3D11Texture2D* pBackBuffer = NULL;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    if (pBackBuffer)
    {
        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);

        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format = static_cast<DXGI_FORMAT>(GetCorrectDXGIFormat(sd.BufferDesc.Format));
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pd3dRenderTarget);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTargetDx11(bool shutDown)
{
    if (!_isInited || !_dx11Device || State::Instance().isShuttingDown)
        return;

    if (!shutDown)
        LOG_FUNC();

    SAFE_RELEASE(g_pd3dRenderTarget);

    if (g_pd3dDevice != nullptr)
        g_pd3dDevice = nullptr;

    _dx11Device = false;
    _isInited = false;
}

static void RenderImGui_DX11(IDXGISwapChain* pSwapChain)
{
    // Final image mode for D3D11 games: the back buffer crosses to D3D12 and back before
    // the menu is drawn, so the menu is not in the picture the model sees.
    if (AmdFinalImage::Enabled())
        AmdFinalImage::RenderDx11(pSwapChain, Util::DllPath().parent_path());

    bool drawMenu = false;

    do
    {
        if (!MenuOverlayBase::IsInited())
            break;

        // Draw only when menu activated
        // if (!MenuOverlayBase::IsVisible())
        //    break;

        if (!_dx11Device || g_pd3dDevice == nullptr)
            break;

        drawMenu = true;

    } while (false);

    if (!drawMenu)
    {
        MenuOverlayBase::HideMenu();
        return;
    }

    LOG_FUNC();

    ImGuiIO& io = ImGui::GetIO();
    (void) io;

    if (io.BackendRendererUserData == nullptr)
    {
        if (pSwapChain->GetDevice(IID_PPV_ARGS(&g_pd3dDevice)) == S_OK)
        {
            g_pd3dDevice->Release();
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
            g_pd3dDeviceContext->Release();
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        }
    }

    if (_isInited)
    {
        if (!g_pd3dRenderTarget)
            CreateRenderTargetDx11(pSwapChain);

        if (ImGui::GetCurrentContext() && g_pd3dRenderTarget)
        {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();

            if (MenuOverlayBase::RenderMenu())
            {
                ImGui::Render();

                g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pd3dRenderTarget, NULL);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            }
        }
    }
}

static void RenderImGui_DX12(IDXGISwapChain* pSwapChainPlain)
{
    bool drawMenu = false;
    IDXGISwapChain3* pSwapChain = nullptr;

    do
    {
        if (pSwapChainPlain->QueryInterface(IID_PPV_ARGS(&pSwapChain)) != S_OK || pSwapChain == nullptr)
            return;

        // Final image mode: the neural pass over the back buffer, for games that give the
        // upscaler path nothing to hook. Runs before the menu is drawn so the menu is not
        // in the picture the model sees.
        if (AmdFinalImage::Enabled())
            AmdFinalImage::Render(pSwapChain, (ID3D12CommandQueue*) currentSCCommandQueue,
                                  Util::DllPath().parent_path());

        if (!MenuOverlayBase::IsInited())
            break;

        // Draw only when menu activated
        // if (!MenuOverlayBase::IsVisible())
        //    break;

        if (!_dx12Device || currentSCCommandQueue == nullptr || g_pd3dDeviceParam == nullptr)
            break;

        drawMenu = true;

    } while (false);

    if (!drawMenu)
    {
        MenuOverlayBase::HideMenu();
        auto releaseResult = pSwapChain->Release();
        return;
    }

    LOG_FUNC();

    // Get device from swapchain
    ID3D12Device* device = g_pd3dDeviceParam;

    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    // Generate ImGui resources
    if (!io.BackendRendererUserData && currentSCCommandQueue != nullptr)
    {
        LOG_DEBUG("ImGui::GetIO().BackendRendererUserData == nullptr");

        HRESULT result;

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.NumDescriptors = NUM_BACK_BUFFERS;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            desc.NodeMask = 1;

            {
                ScopedSkipHeapCapture skipHeapCapture {};
                result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap));
            }

            if (result != S_OK)
            {
                LOG_ERROR("CreateDescriptorHeap(g_pd3dRtvDescHeap): {0:X}", (unsigned long) result);
                MenuOverlayBase::HideMenu();
                CleanupRenderTargetDx12(true);
                pSwapChain->Release();
                return;
            }

            SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

            for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i)
            {
                g_mainRenderTargetDescriptor[i] = rtvHandle;
                rtvHandle.ptr += rtvDescriptorSize;
            }
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = SRV_HEAP_SIZE;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            {
                ScopedSkipHeapCapture skipHeapCapture {};
                result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap));
            }

            if (result != S_OK)
            {
                LOG_ERROR("CreateDescriptorHeap(g_pd3dSrvDescHeap): {0:X}", (unsigned long) result);
                MenuOverlayBase::HideMenu();
                CleanupRenderTargetDx12(true);
                pSwapChain->Release();
                return;
            }

            g_pd3dSrvDescHeapAlloc.Create(device, g_pd3dSrvDescHeap);
        }

        for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i)
        {
            result =
                device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocators[i]));

            if (result != S_OK)
            {
                LOG_ERROR("CreateCommandAllocator[{0}]: {1:X}", i, (unsigned long) result);
                MenuOverlayBase::HideMenu();
                CleanupRenderTargetDx12(true);
                pSwapChain->Release();
                return;
            }

            g_allocatorFenceValues[i] = 0;
        }

        // AMDNR 0.3.3 (MFG stutter, D): one fence value per allocator, see g_overlayFence.
        // Without the fence the overlay keeps the old per-back-buffer allocators.
        g_overlayFenceValue = 0;
        g_nextAllocator = 0;
        SAFE_RELEASE(g_overlayFence);

        if (device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_overlayFence)) != S_OK)
        {
            g_overlayFence = nullptr;
            LOG_WARN("Overlay fence could not be created, command allocators stay per back buffer");
        }

        result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocators[0], NULL,
                                           IID_PPV_ARGS(&g_pd3dCommandList));
        if (result != S_OK)
        {
            LOG_ERROR("CreateCommandList: {0:X}", (unsigned long) result);
            MenuOverlayBase::HideMenu();
            CleanupRenderTargetDx12(true);
            pSwapChain->Release();
            return;
        }

        result = g_pd3dCommandList->Close();
        if (result != S_OK)
        {
            LOG_ERROR("g_pd3dCommandList->Close: {0:X}", (unsigned long) result);
            MenuOverlayBase::HideMenu();
            CleanupRenderTargetDx12(false);
            pSwapChain->Release();
            return;
        }

        DXGI_SWAP_CHAIN_DESC scDesc;
        pSwapChain->GetDesc(&scDesc);

        ImGui_ImplDX12_InitInfo initInfo {};
        initInfo.Device = device;
        initInfo.CommandQueue = (ID3D12CommandQueue*) currentSCCommandQueue;
        initInfo.NumFramesInFlight = NUM_BACK_BUFFERS;
        initInfo.RTVFormat = scDesc.BufferDesc.Format;
        initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
        initInfo.SrvDescriptorHeap = g_pd3dSrvDescHeap;
        initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
                                           D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
        { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
        initInfo.SrvDescriptorFreeFn =
            [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
        { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };

        ImGui_ImplDX12_Init(&initInfo);

        pSwapChain->Release();
        return;
    }

    if (_isInited && currentSCCommandQueue != nullptr)
    {
        // Generate render targets
        if (!g_mainRenderTargetResource[0])
        {
            CreateRenderTargetDx12(device, pSwapChain);
            pSwapChain->Release();
            return;
        }

        // If everything is ready render the frame
        if (ImGui::GetCurrentContext() && g_mainRenderTargetResource[0])
        {
            _showRenderImGuiDebugOnce = true;

            ImGui_ImplDX12_NewFrame();

            // AMDNR 0.3.3 (D): timed, for the rate-limited WARN; and the total is handed to
            // the XeFG pacing engine, which takes it back out of its present timing.
            const int64_t menuStart = OverlayQpcNow();

            if (MenuOverlayBase::RenderMenu())
            {
                const int64_t renderStart = OverlayQpcNow();

                ImGui::Render();

                const int64_t allocatorStart = OverlayQpcNow();

                UINT backBufferIdx = pSwapChain->GetCurrentBackBufferIndex();

                // AMDNR 0.3.3 (D): the next allocator in turn, if the GPU is done with it;
                // otherwise no overlay on this present. Never waits. Without the fence,
                // the old per-back-buffer choice.
                int allocatorSlot = -1;
                ID3D12CommandAllocator* commandAllocator = nullptr;

                if (g_overlayFence != nullptr)
                {
                    const UINT slot = g_nextAllocator % NUM_BACK_BUFFERS;

                    if (g_allocatorFenceValues[slot] > g_overlayFence->GetCompletedValue())
                    {
                        NoteOverlaySkipped();
                        XeFGPacing::NoteOverlayTime(OverlayQpcNow() - menuStart);
                        pSwapChain->Release();
                        return;
                    }

                    allocatorSlot = static_cast<int>(slot);
                    commandAllocator = g_commandAllocators[slot];
                }
                else
                {
                    commandAllocator = g_commandAllocators[backBufferIdx];
                }

                auto result = commandAllocator->Reset();
                if (result != S_OK)
                {
                    LOG_ERROR("commandAllocator->Reset: {0:X}", (unsigned long) result);
                    CleanupRenderTargetDx12(false);
                    pSwapChain->Release();
                    return;
                }

                const int64_t drawStart = OverlayQpcNow();

                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

                result = g_pd3dCommandList->Reset(commandAllocator, nullptr);
                if (result != S_OK)
                {
                    LOG_ERROR("g_pd3dCommandList->Reset: {0:X}", (unsigned long) result);
                    pSwapChain->Release();
                    return;
                }

                g_pd3dCommandList->ResourceBarrier(1, &barrier);
                g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
                g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);

                ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);

                const int64_t submitStart = OverlayQpcNow();

                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                g_pd3dCommandList->ResourceBarrier(1, &barrier);

                result = g_pd3dCommandList->Close();
                if (result != S_OK)
                {
                    LOG_ERROR("g_pd3dCommandList->Close: {0:X}", (unsigned long) result);
                    CleanupRenderTargetDx12(true);
                    pSwapChain->Release();
                    return;
                }

                ID3D12CommandList* ppCommandLists[] = { g_pd3dCommandList };
                ((ID3D12CommandQueue*) currentSCCommandQueue)->ExecuteCommandLists(1, ppCommandLists);

                // AMDNR 0.3.3 (D): mark the allocator's work, and move on to the next one.
                // A Signal that fails would leave the allocator waiting on a value that never
                // comes, so the fence is dropped and the old per-back-buffer choice returns.
                if (allocatorSlot >= 0 && g_overlayFence != nullptr)
                {
                    const UINT64 value = g_overlayFenceValue + 1;

                    if (((ID3D12CommandQueue*) currentSCCommandQueue)->Signal(g_overlayFence, value) == S_OK)
                    {
                        g_overlayFenceValue = value;
                        g_allocatorFenceValues[allocatorSlot] = value;
                        g_nextAllocator = (static_cast<UINT>(allocatorSlot) + 1) % NUM_BACK_BUFFERS;
                    }
                    else
                    {
                        LOG_WARN("Overlay fence Signal failed, command allocators go back to per back buffer");
                        SAFE_RELEASE(g_overlayFence);
                    }
                }

                const int64_t submitEnd = OverlayQpcNow();

                XeFGPacing::NoteOverlayTime(submitEnd - menuStart);
                WarnOverlayTimes(renderStart - menuStart, allocatorStart - renderStart, drawStart - allocatorStart,
                                 submitStart - drawStart, submitEnd - submitStart);
            }
            else
            {
                XeFGPacing::NoteOverlayTime(OverlayQpcNow() - menuStart);
            }
        }
        else
        {
            if (_showRenderImGuiDebugOnce)
                LOG_INFO("!(ImGui::GetCurrentContext() && currentSCCommandQueue && g_mainRenderTargetResource[0])");

            MenuOverlayBase::HideMenu();
            _showRenderImGuiDebugOnce = false;
        }
    }

    pSwapChain->Release();
}

ID3D12GraphicsCommandList* MenuOverlayDx::MenuCommandList() { return g_pd3dCommandList; }

void MenuOverlayDx::CleanupRenderTarget(bool clearQueue, HWND hWnd)
{
    LOG_FUNC();

    auto fg = State::Instance().currentFG;
    if (fg != nullptr && fg->FrameGenerationContext() != nullptr && fg->IsActive())
    {
        State::Instance().fgChanged = true;
        fg->UpdateTarget();
        fg->Deactivate();
    }

    if (_dx11Device)
        CleanupRenderTargetDx11(false);
    else
        CleanupRenderTargetDx12(clearQueue);
}

void MenuOverlayDx::Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags,
                            const DXGI_PRESENT_PARAMETERS* pPresentParameters, IUnknown* pDevice, HWND hWnd, bool isUWP)
{
    // AMDNR 0.3.3 (MFG stutter, D): XeFG at 3X and above only, whether or not the overlay
    // is drawn; does nothing otherwise.
    SamplePresentStatistics(pSwapChain);

    // [Hotfix] DiagNoOverlay (crash triage): same as OverlayMenu=false from here on, but the DXGI
    // hooks and the swapchain wrapper stay in place.
    if (!Config::Instance()->OverlayMenu.value_or_default() || DiagSwitches::NoOverlay())
    {
        MenuOverlayBase::Present();
        return;
    }

    LOG_DEBUG("");

    ID3D12CommandQueue* cq = nullptr;
    ID3D11Device* device = nullptr;
    ID3D12Device* device12 = nullptr;

    // try to obtain directx objects and find the path
    if (pDevice->QueryInterface(IID_PPV_ARGS(&device)) == S_OK)
    {
        if (!_dx11Device)
            LOG_DEBUG("D3D11Device captured");

        _dx11Device = true;
    }
    else if (pDevice->QueryInterface(IID_PPV_ARGS(&cq)) == S_OK)
    {
        if (!_dx12Device)
            LOG_DEBUG("D3D12CommandQueue captured");

        if (!CheckForRealObject(__FUNCTION__, pDevice, &currentSCCommandQueue))
            currentSCCommandQueue = pDevice;

        if (((ID3D12CommandQueue*) currentSCCommandQueue)->GetDevice(IID_PPV_ARGS(&device12)) == S_OK)
        {
            if (!_dx12Device)
                LOG_DEBUG("D3D12Device captured");

            _dx12Device = true;
        }
    }

    // Process window handle changed, update base
    if (MenuOverlayBase::Handle() != hWnd)
    {
        LOG_DEBUG("Handle changed {:X} -> {:X}", (size_t) MenuOverlayBase::Handle(), (size_t) hWnd);

        if (MenuOverlayBase::IsInited())
            MenuOverlayBase::Shutdown();

        MenuOverlayBase::Init(hWnd, isUWP);

        _isInited = false;
    }

    // Init
    if (!_isInited)
    {
        if (_dx11Device)
        {
            CleanupRenderTargetDx11(false);

            g_pd3dDevice = device;

            CreateRenderTargetDx11(pSwapChain);
            MenuOverlayBase::Dx11Ready();
            _isInited = true;
        }
        else if (_dx12Device && (g_pd3dDeviceParam != nullptr || device12 != nullptr))
        {
            if (g_pd3dDeviceParam != nullptr && device12 == nullptr)
                device12 = g_pd3dDeviceParam;

            CleanupRenderTargetDx12(true);

            g_pd3dCommandQueue = cq;
            g_pd3dDeviceParam = device12;

            MenuOverlayBase::Dx12Ready();
            _isInited = true;
        }
    }

    {
        ScopedSkipHeapCapture skipHeapCapture {};

        // Render menu
        if (_dx11Device)
            RenderImGui_DX11(pSwapChain);
        else if (_dx12Device)
            RenderImGui_DX12(pSwapChain);
    }

    // release used objects
    if (cq != nullptr)
        cq->Release();

    if (device != nullptr)
        device->Release();

    if (device12 != nullptr)
        device12->Release();
}
