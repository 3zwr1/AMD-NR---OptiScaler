// Copyright (c) 2026 3zwr1 (AMDNR)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// FINAL IMAGE MODE - neural rendering for games that give OptiScaler nothing to hook.
//
// The normal path runs the model on the colour a temporal upscaler is about to consume:
// it needs a DLSS / FSR 2+ / XeSS call to intercept, and with it the game's motion vectors,
// depth and jitter. A game with only FSR 1, or no upscaler at all, never makes that call, so
// ticking "Enable Neural Rendering" there does nothing and says nothing.
//
// This mode takes the frame at the last place it exists as one picture: the swapchain's
// back buffer, at Present. What it has is colour. What it does not have is motion, depth
// and jitter, and the whole design follows from not pretending otherwise:
//
//   - The model is fed a zero motion field, a constant depth and `reset = true` on every
//     frame, so it never samples its own history and never sees a motion vector. That is
//     exactly how lmxxf's port of the same network runs ("reset=true means the network never
//     samples motion/history"), and it turns the model into what it fundamentally is: a
//     per-frame neural denoiser and re-renderer, not an upscaler.
//   - Everything temporal in our own chain is forced off (interleave, temporal stability,
//     residual temporal): each of them reprojects with motion vectors that do not exist here.
//     NR resolution and the residual composition still work, because they need no motion.
//   - The HUD is in the picture and gets processed with it. A later version can take the
//     hudless capture the frame-generation path already knows how to find; this one is honest
//     about the limit rather than clever about it.
//   - D3D11 swapchains go through the same shared-texture bridge the D3D11-on-D3D12 upscaler
//     path uses (Dx11WithDx12): the back buffer is copied into a shared texture on the D3D11
//     side, opened on a D3D12 device, processed there, copied into a second shared texture,
//     and copied back into the back buffer on the D3D11 side, with a shared fence in each
//     direction. The game's own D3D11 queue waits for the neural work, which is the same
//     inline cost the upscaler path pays.
//
// LINEAGE. This file was a PCSX2-only experiment ("final image, synthetic guides, no
// history") that was paused after a corruption report. Three things about it were wrong for
// a real game and are different now:
//   1. It skipped the frame whenever the GPU was still on the previous one. A game keeps
//      the GPU busy, so that skipped most frames and alternated NR / no-NR - a flicker. The
//      command lists are a ring now, and a frame waits for the slot from three frames ago,
//      which is normally already done.
//   2. It refused BGRA back buffers, which are common. They are written through a BGRA UAV
//      when the device supports one (RDNA 4 does), and refused with a reason otherwise.
//   3. It would have run a second neural backend beside the upscaler bridge's in any game
//      that has an upscaler. The pass DLLs and the HIP runtime are process-global, so two
//      backends is a conflict, not a feature. This mode stays idle whenever the bridge's
//      backend exists, and the menu says so.
//
// EXCLUSIVITY IS THE RULE THAT MAKES TESTING POSSIBLE: to try this in a game that has an
// upscaler, switch the game's upscaler off first, so no OptiScaler feature exists and the
// bridge backend never comes up.
#include "AmdPreSr.h"
#include "AmdBridge.h"
#include <dlssnr/lmxxf/LmxxfBackend.h>
#include <Config.h>
#include <State.h>
#include <with_dx12/with_dx12.h>
#include <with_dx12/dx11_with_dx12.h>
#include <filesystem>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include "SystemCompiler.h"
#include <dxgi1_4.h>
#include <cstring>
#include <mutex>
#include <string>
#include <stdexcept>
#include <chrono>

namespace AmdFinalImage
{
using Microsoft::WRL::ComPtr;

inline std::mutex mutex;
inline std::string status = "Final image mode: idle";
inline bool loggedActive = false;

inline void Check(HRESULT h, const char* what)
{
    if (FAILED(h))
        throw std::runtime_error(std::string("Final image mode: ") + what + " failed, HRESULT " +
                                 std::to_string((UINT) h));
}
inline void Transition(ID3D12GraphicsCommandList* c, ID3D12Resource* r, D3D12_RESOURCE_STATES a,
                       D3D12_RESOURCE_STATES b)
{
    if (a == b)
        return;
    D3D12_RESOURCE_BARRIER v {};
    v.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    v.Transition = { r, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, a, b };
    c->ResourceBarrier(1, &v);
}

// SHELVED FOR 0.2.0, BACK FOR TESTING IN 0.3.x. It ran in Stray (D3D11, 10-bit SDR) and the
// still frame was right, but the moment the camera moved the whole picture smeared into
// streaks: temporal accumulation with a zero motion field. Two things are different now.
// The runtimes' own history paths are forced off here rather than inherited from the user's
// settings (danielblnc's runtime takes no history inputs at all in every-frame mode), and
// the lmxxf runtime can carry the mode too, network history off, its edit carried in place.
// Whether the streaks are gone is what the Stray test decides; an estimated motion field
// (FfxOpticalFlow) stays the plan if they are not.
// 0.3.1: OFF for the public build (user decision: fix the 0.3.0 reports first). The code stays -
// the no-stall consume and the block-matching carry for lmxxf are the work in progress for the
// no-upscaler titles - and this one switch brings the checkbox and the ini key back.
inline constexpr bool kAvailable = false;

// Is the mode switched on at all, before any device state is consulted. The menu uses this
// to decide whether to show the status line.
inline bool Enabled()
{
    if (!kAvailable)
        return false;
    const auto& cfg = *Config::Instance();
    return cfg.AmdFinalImage.value_or_default() && cfg.DlssNrEnabled.value_or_default();
}

struct Context
{
    static constexpr UINT kRing = 3;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    struct Slot
    {
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> cmd;
        UINT64 fenceValue = 0;
    } ring[kRing];
    UINT ringIndex = 0;
    ComPtr<ID3D12Fence> fence;
    UINT64 serial = 0;
    HANDLE fenceEvent = nullptr;
    ComPtr<ID3D12Resource> input, motion, depth, output;
    ComPtr<ID3D12DescriptorHeap> heap, clearCpu, composeHeap;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pipeline;
    AmdPreSr::NeuralBackend* backend = nullptr; // danielblnc's or lmxxf's, by the runtime choice
    bool lmxxf = false;
    UINT width = 0, height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    bool stopped = false;
    bool guidesCleared = false;
    // Our own cost, for the status line: the longest wait for a ring slot and the longest
    // whole FrameFrom, per 600 frames (a stutter report needs to know whether it is ours).
    double waitMax = 0.0, frameMax = 0.0, waitShown = 0.0, frameShown = 0.0;
    UINT64 frameNo = 0;

    Context(ID3D12Device* d, ID3D12CommandQueue* q) : device(d), queue(q)
    {
        for (auto& s : ring)
        {
            Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s.allocator)),
                  "command allocator");
            Check(d->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s.allocator.Get(), nullptr,
                                       IID_PPV_ARGS(&s.cmd)),
                  "command list");
            Check(s.cmd->Close(), "initial close");
        }
        Check(d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "fence");
        fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        D3D12_DESCRIPTOR_HEAP_DESC hd {};
        hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hd.NumDescriptors = 2;
        hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Check(d->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&heap)), "clear heap");
        Check(d->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&composeHeap)), "compose heap");
        hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Check(d->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&clearCpu)), "clear cpu heap");
        D3D12_DESCRIPTOR_RANGE ranges[2] {};
        ranges[0] = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0 };
        ranges[1] = { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 1 };
        D3D12_ROOT_PARAMETER params[2] {};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[0].DescriptorTable = { 2, ranges };
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[1].Constants = { 0, 0, 2 };
        D3D12_ROOT_SIGNATURE_DESC rd {};
        rd.NumParameters = 2;
        rd.pParameters = params;
        ComPtr<ID3DBlob> b, e;
        Check(D3D12SerializeRootSignature(&rd, D3D_ROOT_SIGNATURE_VERSION_1, &b, &e), "root signature blob");
        Check(d->CreateRootSignature(0, b->GetBufferPointer(), b->GetBufferSize(), IID_PPV_ARGS(&root)),
              "root signature");
        // The compose is a copy: the backend's FP16 result into the back buffer's own format.
        // A UAV store clamps to the target format's range, which is the right thing for an
        // 8-bit SDR target and a no-op for a float one.
        const char* shader = "Texture2D<float4> src:register(t0); RWTexture2D<float4> dst:register(u0); "
                             "cbuffer C:register(b0){uint w,h;} [numthreads(8,8,1)] void main(uint3 p:SV_DispatchThreadID)"
                             "{if(p.x<w&&p.y<h)dst[p.xy]=src.Load(int3(p.xy,0));}";
        Check(DlssNr::SysCompiler::Compile(shader, strlen(shader), "final image compose", nullptr, nullptr, "main", "cs_5_0", 0, 0, &b, &e),
              "compose shader");
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd {};
        pd.pRootSignature = root.Get();
        pd.CS = { b->GetBufferPointer(), b->GetBufferSize() };
        Check(d->CreateComputePipelineState(&pd, IID_PPV_ARGS(&pipeline)), "compose pipeline");
    }
    ~Context()
    {
        if (fenceEvent)
            CloseHandle(fenceEvent);
    }

    ComPtr<ID3D12Resource> Texture(DXGI_FORMAT f, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial)
    {
        D3D12_HEAP_PROPERTIES hp {};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC r {};
        r.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        r.Width = width;
        r.Height = height;
        r.DepthOrArraySize = 1;
        r.MipLevels = 1;
        r.Format = f;
        r.SampleDesc.Count = 1;
        r.Flags = flags;
        ComPtr<ID3D12Resource> out;
        Check(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &r, initial, nullptr, IID_PPV_ARGS(&out)),
              "texture");
        return out;
    }

    // Can this device store through a UAV of the back buffer's format? BGRA8 is optional in
    // D3D12; RDNA 4 supports it, and refusing with a reason beats a device removal.
    bool UavStoreSupported(DXGI_FORMAT f)
    {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT s { f };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &s, sizeof(s))))
            return false;
        return (s.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0;
    }

    void WaitFor(UINT64 value)
    {
        if (fence->GetCompletedValue() >= value)
            return;
        Check(fence->SetEventOnCompletion(value, fenceEvent), "fence event");
        WaitForSingleObject(fenceEvent, 2000);
    }

    // Resources follow the back buffer. Any change drains the GPU first: the textures
    // being replaced may still be read by the last list, and the backend keeps its own
    // process-lifetime state so it needs no drain of its own.
    void EnsureResources(const D3D12_RESOURCE_DESC& desc)
    {
        if (input && desc.Width == width && desc.Height == height && desc.Format == format)
            return;
        WaitFor(serial);
        width = (UINT) desc.Width;
        height = desc.Height;
        format = desc.Format;
        input = Texture(format, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
        output = Texture(format, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        motion = Texture(DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                         D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        depth = Texture(DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        guidesCleared = false;
    }

    // The synthetic guides are written once per allocation and left in the shader-read
    // state; nothing ever changes them again.
    void ClearGuides(ID3D12GraphicsCommandList* cmd)
    {
        ID3D12DescriptorHeap* heaps[] = { heap.Get() };
        cmd->SetDescriptorHeaps(1, heaps);
        auto cpu = heap->GetCPUDescriptorHandleForHeapStart();
        auto gpu = heap->GetGPUDescriptorHandleForHeapStart();
        auto clear = clearCpu->GetCPUDescriptorHandleForHeapStart();
        const UINT inc = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        for (ID3D12Resource* r : { motion.Get(), depth.Get() })
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC u {};
            u.Format = r->GetDesc().Format;
            u.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            device->CreateUnorderedAccessView(r, nullptr, &u, cpu);
            device->CreateUnorderedAccessView(r, nullptr, &u, clear);
            // Depth 0.5 rather than 0 or 1: a constant depth carries no information, and a
            // mid value keeps every depth-relative tolerance in the chain finite.
            float values[4] = { r == depth.Get() ? 0.5f : 0.f, 0, 0, 0 };
            cmd->ClearUnorderedAccessViewFloat(gpu, clear, r, values, 0, nullptr);
            Transition(cmd, r, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            clear.ptr += inc;
            cpu.ptr += inc;
            gpu.ptr += inc;
        }
        guidesCleared = true;
    }

    void Compose(ID3D12GraphicsCommandList* cmd, ID3D12Resource* back, D3D12_RESOURCE_STATES backState,
                 ID3D12Resource* result)
    {
        auto cpu = composeHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_SHADER_RESOURCE_VIEW_DESC srv {};
        srv.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(result, &srv, cpu);
        cpu.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_UNORDERED_ACCESS_VIEW_DESC u {};
        u.Format = format;
        u.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(output.Get(), nullptr, &u, cpu);
        ID3D12DescriptorHeap* h = composeHeap.Get();
        cmd->SetDescriptorHeaps(1, &h);
        cmd->SetComputeRootSignature(root.Get());
        cmd->SetPipelineState(pipeline.Get());
        cmd->SetComputeRootDescriptorTable(0, composeHeap->GetGPUDescriptorHandleForHeapStart());
        UINT dims[] = { width, height };
        cmd->SetComputeRoot32BitConstants(1, 2, dims, 0);
        cmd->Dispatch((width + 7) / 8, (height + 7) / 8, 1);
        Transition(cmd, output.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        Transition(cmd, back, backState, D3D12_RESOURCE_STATE_COPY_DEST);
        cmd->CopyResource(back, output.Get());
        Transition(cmd, back, D3D12_RESOURCE_STATE_COPY_DEST, backState);
        Transition(cmd, output.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    void Frame(ID3D12Resource* back, const std::filesystem::path& directory)
    {
        FrameFrom(back, D3D12_RESOURCE_STATE_PRESENT, back, D3D12_RESOURCE_STATE_PRESENT, directory);
    }

    // True when the composed result was written into `dst`. A false return leaves `dst`
    // untouched, and the caller must present the frame as it was: the D3D11 path used to copy
    // its output texture back regardless, and an output nothing had written is a black frame -
    // the black screen in Stray with the lmxxf runtime (see the readiness note below).

    // `src` is read (copied into the working input), `dst` receives the composed result.
    // For a D3D12 swapchain both are the back buffer in PRESENT; for D3D11 they are two
    // shared textures that must be handed back in COMMON.
    bool FrameFrom(ID3D12Resource* src, D3D12_RESOURCE_STATES srcState, ID3D12Resource* dst,
                   D3D12_RESOURCE_STATES dstState, const std::filesystem::path& directory)
    {
        if (stopped)
            return false;
        const auto desc = src->GetDesc();
        // 10-bit UNORM (R10G10B10A2, format 24) is what Unreal games such as Stray present
        // with. Without HDR it holds the same sRGB-encoded picture an 8-bit target does, at
        // more precision, so it is decoded the same way. With HDR active it is PQ-encoded,
        // which the encoding pass cannot undo yet, so that case is refused with the reason.
        const bool sdr8 = desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM ||
                          (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM && !State::Instance().isHdrActive);
        const bool hdr16 = desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT;
        if (!sdr8 && !hdr16)
        {
            status = "Final image mode: back buffer format " + std::to_string((int) desc.Format) +
                     (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM
                          ? " with HDR (PQ) is not supported yet; switch the game to SDR"
                          : " is not supported yet (8-bit RGBA/BGRA, 10-bit SDR and 16-bit float are)");
            return false;
        }
        if (!UavStoreSupported(desc.Format))
        {
            status = "Final image mode: this device cannot write the back buffer's format through a UAV";
            return false;
        }
        EnsureResources(desc);
        if (!backend)
        {
            // lmxxf carries this mode unless danielblnc's runtime was chosen explicitly
            // ([DlssNr] NrBackend=daniel) or lmxxf is not installed. Not the bridge's rule
            // (unchosen with both installed = ask): here the user's word is lmxxf first.
            const bool danielChosen =
                DlssNr::AmdBridge::ChosenRuntime() == DlssNr::AmdBridge::NeuralRuntime::Daniel;
            lmxxf = DlssNr::AmdBridge::LmxxfReady() && (!danielChosen || !DlssNr::AmdBridge::HasFiles());
            if (lmxxf)
                backend = new Lmxxf::Backend(device.Get(), queue.Get(), directory);
            else
                backend = new AmdPreSr::Backend(device.Get(), queue.Get(), directory);
            LOG_INFO("DLSS-NR final image mode: {} runtime", lmxxf ? "lmxxf" : "danielblnc");
        }
        // danielblnc's backend loads in its constructor, so not-ready means failed. lmxxf's binds
        // its runtime at the first SUBMISSION of a list its Record has seen (that is how it learns
        // the queue), so before the first frame it is not ready by design; gating on Ready() here
        // never let that first Record happen, and the mode sat at "not ready" with the D3D11
        // path copying an unwritten output over the picture. After the first frame, not-ready
        // means the load failed and the backend's status says why.
        if (!backend->Ready() && !(lmxxf && backend->RecordedFrames() == 0))
        {
            status = "Final image mode: " + backend->Status();
            return false;
        }

        // The slot used three frames ago. Normally complete; if not, the wait is short and
        // bounded, and it is the price of never skipping a frame - a skipped frame here is
        // one un-processed picture between two processed ones, which is a flicker.
        const auto tf0 = std::chrono::steady_clock::now();
        Slot& slot = ring[ringIndex];
        ringIndex = (ringIndex + 1) % kRing;
        WaitFor(slot.fenceValue);
        waitMax = (std::max)(waitMax, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tf0).count());
        Check(slot.allocator->Reset(), "allocator reset");
        Check(slot.cmd->Reset(slot.allocator.Get(), nullptr), "list reset");
        ID3D12GraphicsCommandList* cmd = slot.cmd.Get();

        if (!guidesCleared)
            ClearGuides(cmd);

        Transition(cmd, src, srcState, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cmd->CopyResource(input.Get(), src);
        Transition(cmd, src, D3D12_RESOURCE_STATE_COPY_SOURCE, srcState);
        Transition(cmd, input.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        AmdPreSr::Frame f {};
        f.colour = input.Get();
        f.motion = motion.Get();
        f.depth = depth.Get();
        f.width = width;
        f.height = height;
        // No history, ever: there are no motion vectors to reproject it with. danielblnc's
        // runtime is told so with the reset flag on every frame; lmxxf's has its network
        // history switched off below instead (its backend ignores a Reset flag held up for
        // more than eight frames, and a per-frame reset would also erase the carried edit).
        f.reset = !lmxxf;
        f.colourState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        // The user's own settings, with everything that needs motion switched off.
        AmdPreSr::Settings s = DlssNr::AmdBridge::CurrentSettings();
        // lmxxf: no-motion mode. The answer is consumed when it is done (the frame never waits
        // for the network), the edit is carried in place by an EMA gated on colour change;
        // Temporal stability is that EMA's weight and Model interleave is the feed cadence
        // (no fill shaders are involved), so both stay the user's. danielblnc's path keeps
        // everything temporal off, as before.
        s.lmxxfNoMotion = lmxxf;
        if (!lmxxf)
        {
            s.interleave = 0.f;
            s.stability = 0.f;
        }
        // Without motion vectors the age of the answer is what shows, so the network is fed at
        // half size here unless NR resolution was set explicitly: GTA V at 160 fps got one answer
        // per ~7 frames from a 1080p feed. The edit is lifted to the frame either way.
        if (lmxxf && !Config::Instance()->AmdNrScale.has_value())
            s.modelScale = (std::min)(s.modelScale, 0.5f);
        s.residualTemporal = false;
        s.rtgi.enabled = false; // needs real depth
        // The runtimes' own temporal paths, off explicitly rather than by inheritance: with a
        // zero motion field they accumulate, and the whole picture streaks on camera motion
        // (the Stray report that shelved this mode). Every-frame mode is where danielblnc's
        // runtime takes no history inputs; lmxxf's network history is a flag of its own.
        s.everyFrame = true;
        s.interleaveModelHistory = false;
        s.lmxxfHistory = false;
        // An 8-bit swapchain holds sRGB-encoded colour; the model wants linear. Auto (0) means
        // "the input is already linear", which is right for the upscaler path and wrong here.
        if (sdr8 && s.encoding == 0)
            s.encoding = 2;

        ID3D12Resource* result = backend->Record(cmd, f, s);
        Transition(cmd, input.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        const bool composed = result != nullptr;
        if (composed)
            Compose(cmd, dst, dstState, result);

        Check(cmd->Close(), "close");
        ID3D12CommandList* lists[] = { cmd };
        backend->Submitting(queue.Get(), 1, lists);
        queue->ExecuteCommandLists(1, lists);
        backend->Submitted(queue.Get(), 1, lists);
        slot.fenceValue = ++serial;
        Check(queue->Signal(fence.Get(), serial), "signal");
        frameMax = (std::max)(frameMax, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tf0).count());
        if (++frameNo % 600 == 0)
        {
            waitShown = waitMax; frameShown = frameMax;
            waitMax = frameMax = 0.0;
        }
        char ours[96];
        std::snprintf(ours, sizeof ours, " | ours: max %.1f ms per frame (slot wait %.1f) over the last 600", frameShown, waitShown);
        status = std::string(composed ? "Final image mode active (" : "Final image mode waiting (") +
                 (lmxxf ? "lmxxf" : "danielblnc") + " runtime, no motion, no depth, HUD included): " +
                 backend->Status() + ours;
        if (!loggedActive && composed)
        {
            loggedActive = true;
            LOG_INFO("DLSS-NR final image mode engaged: {}x{} format {} ({})", width, height, (int) format,
                     sdr8 ? "SDR sRGB-encoded, decoded for the model" : "16-bit float, taken as linear");
        }
        return composed;
    }
};

inline Context* context = nullptr;

inline std::string Status()
{
    std::lock_guard g(mutex);
    return status;
}

// Called from the D3D12 overlay present path with the swapchain and the queue it presents on.
inline void Render(IDXGISwapChain3* sc, ID3D12CommandQueue* queue, const std::filesystem::path& directory)
{
    std::lock_guard g(mutex);
    if (!Enabled() || !queue || !sc)
        return;
    if (!DlssNr::AmdBridge::AnyRuntimePresent())
    {
        status = "Final image mode: no neural runtime files beside OptiScaler (LmxxfNrRuntime.dll + .pak, or dlssnr_amd_pass1.dll)";
        return;
    }
    // EXCLUSIVITY. One neural backend per process: the pass DLLs and the HIP runtime are
    // global. If the upscaler bridge has one, this mode has nothing to add and stays idle.
    if (DlssNr::AmdBridge::BackendActive())
    {
        status = "Final image mode idle: an upscaler input is active, so the normal pre-SR path is in charge. "
                 "To test this mode here, switch the game's upscaler off.";
        return;
    }
    try
    {
        ComPtr<ID3D12Device> d;
        Check(sc->GetDevice(IID_PPV_ARGS(&d)), "swapchain device");
        if (!context)
            context = new Context(d.Get(), queue);
        if (context->device.Get() != d.Get() || context->queue.Get() != queue)
        {
            status = "Final image mode: the device or queue changed; restart the game";
            return;
        }
        ComPtr<ID3D12Resource> b;
        Check(sc->GetBuffer(sc->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&b)), "back buffer");
        context->Frame(b.Get(), directory);
    }
    catch (const std::exception& e)
    {
        status = std::string("Final image mode stopped: ") + e.what();
        LOG_ERROR("{}", status);
        if (context)
            context->stopped = true;
    }
}

// ---------------------------------------------------------------- D3D11 swapchains
inline Dx11WithDx12::D3D11_TEXTURE2D_RESOURCE_C sharedIn {}, sharedOut {};
inline UINT64 dx11FrameId = 0;
inline bool dx11Inited = false;

// Called from the D3D11 overlay present path. The back buffer crosses to D3D12 and back
// through the same shared-texture bridge the D3D11-on-D3D12 upscaler path uses.
inline void RenderDx11(IDXGISwapChain* sc, const std::filesystem::path& directory)
{
    std::lock_guard g(mutex);
    if (!Enabled() || !sc)
        return;
    if (!DlssNr::AmdBridge::AnyRuntimePresent())
    {
        status = "Final image mode: no neural runtime files beside OptiScaler (LmxxfNrRuntime.dll + .pak, or dlssnr_amd_pass1.dll)";
        return;
    }
    if (DlssNr::AmdBridge::BackendActive())
    {
        status = "Final image mode idle: an upscaler input is active, so the normal pre-SR path is in charge. "
                 "To test this mode here, switch the game's upscaler off.";
        return;
    }
    try
    {
        ComPtr<ID3D11Device> dev;
        Check(sc->GetDevice(IID_PPV_ARGS(&dev)), "D3D11 device");
        ComPtr<ID3D11DeviceContext> ctx;
        dev->GetImmediateContext(&ctx);
        if (!ctx)
            throw std::runtime_error("no immediate context");

        if (!dx11Inited)
        {
            // A D3D12 device and queue on the same adapter, and the helper's shared fences.
            if (!WithDx12::PrepareD3D12ForD3D11(dev.Get(), D3D_FEATURE_LEVEL_11_0))
                throw std::runtime_error("could not create a D3D12 device beside the D3D11 one");
            Dx11WithDx12::Init(dev.Get(), ctx.Get(), WithDx12::GetD3D12Device(), WithDx12::GetD3D12CommandQueue());
            dx11Inited = true;
            LOG_INFO("DLSS-NR final image mode: D3D11 swapchain, D3D12 device and queue prepared");
        }
        ID3D12Device* d12 = WithDx12::GetD3D12Device();
        ID3D12CommandQueue* q12 = WithDx12::GetD3D12CommandQueue();
        if (!d12 || !q12)
            throw std::runtime_error("D3D12 device or queue missing");
        if (!context)
            context = new Context(d12, q12);
        if (context->device.Get() != d12 || context->queue.Get() != q12)
        {
            status = "Final image mode: the device or queue changed; restart the game";
            return;
        }

        ComPtr<ID3D11Texture2D> back;
        Check(sc->GetBuffer(0, IID_PPV_ARGS(&back)), "D3D11 back buffer");
        const UINT64 frameId = ++dx11FrameId;
        // In: the back buffer copied into a shared texture on the D3D11 queue, opened on D3D12.
        if (!Dx11WithDx12::PrepareTextureFrom11To12("FinalImageIn", d12, back.Get(), &sharedIn, true, false, false, frameId))
            throw std::runtime_error("shared input texture");
        // Out: a second shared texture of the back buffer's shape, written on D3D12.
        if (!Dx11WithDx12::PrepareTextureFrom11To12("FinalImageOut", d12, back.Get(), &sharedOut, false, false, false, frameId))
            throw std::runtime_error("shared output texture");
        if (!sharedIn.Dx12Resource || !sharedOut.Dx12Resource || !sharedOut.SharedTexture)
            throw std::runtime_error("shared resources not opened");

        // D3D11 copy done before D3D12 reads; D3D12 work done before D3D11 copies back.
        if (!Dx11WithDx12::SyncDx11ToDx12())
            throw std::runtime_error("D3D11 -> D3D12 fence");
        // Only a composed frame goes back into the back buffer; anything else presents the
        // game's own picture. Copying the output texture unconditionally put an unwritten
        // texture (black) on screen whenever the backend produced nothing.
        const bool composed = context->FrameFrom(sharedIn.Dx12Resource, D3D12_RESOURCE_STATE_COMMON,
                                                 sharedOut.Dx12Resource, D3D12_RESOURCE_STATE_COMMON, directory);
        if (!composed)
            return;
        if (!Dx11WithDx12::SyncDx12ToDx11())
            throw std::runtime_error("D3D12 -> D3D11 fence");
        ctx->CopyResource(back.Get(), sharedOut.SharedTexture);
    }
    catch (const std::exception& e)
    {
        status = std::string("Final image mode stopped: ") + e.what();
        LOG_ERROR("{}", status);
        if (context)
            context->stopped = true;
    }
}
} // namespace AmdFinalImage
