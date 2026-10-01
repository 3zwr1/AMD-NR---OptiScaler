// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// AMDNR Screen GI: the D3D12 compute module. See AmdnrGi.h for the contract and
// the AMDNR SSGI design note (sections 3.1-3.5) for the design. No pch: the offline lab
// (dlssnr/gi/lab) compiles this file on its own.
//
// Frame outline (Record):
//   prepare (may throw; nothing recorded yet): validate inputs, pick the tier and trace grid, (re)allocate, read the
//   probe and timestamps of 4 Records ago, decide the depth convention, fill the constants
//   record (no throw): heap + root signature + b0, game inputs to NON_PIXEL_SHADER_RESOURCE when needed, clear the
//   probe, P1 prepass, P2 pyramid x6, P3 trace, P4 temporal, P5 a-trous x0-2, P6 composite, P7 debug (optional),
//   out to NON_PIXEL_SHADER_RESOURCE, probe copy to the readback ring, timestamps, game inputs back.

// No pch here, so windows.h (through d3d12.h) would bring its min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "AmdnrGi.h"
#include "GiShared.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "precompiled/gi_prepass.h"
#include "precompiled/gi_pyramid.h"
#include "precompiled/gi_trace.h"
#include "precompiled/gi_temporal.h"
#include "precompiled/gi_spatial.h"
#include "precompiled/gi_composite.h"
#include "precompiled/gi_debug.h"

namespace AmdnrGi
{
using Microsoft::WRL::ComPtr;

static_assert(sizeof(GiFrameConstants) % 16 == 0, "cbuffer layout: 16-byte vectors only");
static_assert(sizeof(GiPassConstants) == 32, "b1 is eight root constants");

namespace
{
std::atomic<LogSink> g_sink { nullptr };

void Log(int level, const std::string& text)
{
    if (auto sink = g_sink.load(std::memory_order_acquire))
        sink(level, text.c_str());
}

std::string Hex(HRESULT hr)
{
    char b[16];
    std::snprintf(b, sizeof(b), "0x%08X", static_cast<unsigned>(hr));
    return b;
}

void Check(HRESULT hr, const char* what)
{
    if (FAILED(hr))
        throw std::runtime_error(std::string(what) + " failed (" + Hex(hr) + ")");
}

// Ring depth: a descriptor block, a constant slot or a retired resource is reused / freed only after this many
// Records (design 3.5: 8 frames).
constexpr uint32_t kRing = 8;
constexpr uint32_t kSrvSlots = 12, kUavSlots = 16, kTable = kSrvSlots + kUavSlots;
// P1 + P2 x6 + P3 + P4 + P5 x2 + P6 + P7 = 13 dispatches; 3 spare for the shader lane.
constexpr uint32_t kMaxDispatches = 16;
constexpr uint32_t kDescPerRecord = kMaxDispatches * kTable + 1; // + the probe clear view
constexpr uint32_t kCbStride = (sizeof(GiFrameConstants) + 255) & ~255u;
constexpr uint32_t kTimestamps = 2; // per Record: start, end
constexpr uint32_t kReadbackTimestampBase = kRing * GI_READBACK_BYTES;
constexpr uint32_t kReadbackBytes = kReadbackTimestampBase + kRing * kTimestamps * sizeof(uint64_t);
constexpr uint32_t kProbeLag = 4; // Records between writing the probe/timestamps and reading them
constexpr uint32_t kTraceAlign = 32; // trace allocations round up to 32, so the 6-level pyramid always fits
constexpr uint32_t kShrinkAfter = 300;

enum PassId
{
    kPrepass,
    kPyramid,
    kTrace,
    kTemporal,
    kSpatial,
    kComposite,
    kDebug,
    kPassCount
};

struct ShaderBytes
{
    const unsigned char* data;
    size_t size;
    const char* name;
};

const ShaderBytes kShaders[kPassCount] = {
    { g_gi_prepass, sizeof(g_gi_prepass), "prepass" },       { g_gi_pyramid, sizeof(g_gi_pyramid), "pyramid" },
    { g_gi_trace, sizeof(g_gi_trace), "trace" },             { g_gi_temporal, sizeof(g_gi_temporal), "temporal" },
    { g_gi_spatial, sizeof(g_gi_spatial), "spatial" },       { g_gi_composite, sizeof(g_gi_composite), "composite" },
    { g_gi_debug, sizeof(g_gi_debug), "debug" },
};

// Design 6.3 (starting points; the ms targets are the contract and the lab refits these). thicknessAuto: 0.06 at
// High/Ultra keeps the halo beside a thin pillar under B4's 0.05 (gilab s1_pillar: 0.043 at 0.06, 0.079 at 0.10)
// while the crease AO stays strong (Cornell crease 0.66 vs 0.63); Low/Medium keep 0.10 (their smaller radius already
// passes, and fewer steps would make thin shells porous). Measured on the RX 9070 XT (lab --hw --perf, corner scene,
// 1920x1080 render, 2026-09-26): Low 0.31 ms, Medium 0.41, High 0.70; Ultra with 2 slices x 16 steps 2.91 ms, so
// Ultra runs 1 slice (the history integrates the directions over its 32 frames). Ultra with 1 slice measured 2.62 ms
// at 1080p (overlapping another GPU job, an upper bound): still over its 2.0 ms target, refit in a clean perf window.
// Low keeps 16 frames of history (design: 12): with 12 it failed B2a's Low gate in gilab (Cornell p95 8.1 %, s7 mean
// 3.1 % / p95 8.5 %), with 16 every scene passes (Cornell 2.6 % / 7.4 %, s7 2.7 % / 7.5 %); no cost change.
// Verify pass (gilab --hw, Cornell box at 1920x1080 render, effect's own timestamps, 2026-09-26 22:00): Low 0.42 ms,
// Medium 0.61, High 1.01; Ultra at full res (2.07 Mpx, 1 x 16) 3.3-3.7 ms (trace alone 2.27), so Ultra's cap is
// refit to 1.15 Mpx (1430x805 at 1080p): 1.85-1.90 ms, under its 2.0 ms target (an upper bound: a concurrent GPU
// probe started near the end of that run).
//  capMpx minRatio slices steps normals maxHistory atrous radiusFraction thicknessAuto
const TierInfo kTiers[4] = {
    { 0.15f, 2, 1, 6, false, 16, 1, 0.10f, 0.10f },  // Low (APU default)
    { 0.30f, 2, 1, 8, false, 20, 1, 0.12f, 0.10f },  // Medium
    { 0.55f, 2, 1, 12, true, 28, 2, 0.15f, 0.06f },  // High (desktop default)
    { 1.15f, 1, 1, 16, true, 32, 2, 0.15f, 0.06f },  // Ultra (1 x 16; full res measured 3.4 ms at 1080p, see above)
};

DXGI_FORMAT TypedColour(DXGI_FORMAT f)
{
    switch (f)
    {
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return f;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

bool IsSrgb(DXGI_FORMAT f)
{
    return f == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || f == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
}

bool IsUnorm(DXGI_FORMAT f)
{
    switch (f)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return true;
    default:
        return false;
    }
}

DXGI_FORMAT DepthSrvFormat(DXGI_FORMAT f)
{
    switch (f)
    {
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R16_FLOAT:
        return DXGI_FORMAT_R16_FLOAT;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT TypedTwoChannel(DXGI_FORMAT f)
{
    switch (f)
    {
    case DXGI_FORMAT_R16G16_TYPELESS: return DXGI_FORMAT_R16G16_FLOAT;
    case DXGI_FORMAT_R32G32_TYPELESS: return DXGI_FORMAT_R32G32_FLOAT;
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_UNORM:
        return f;
    // Some titles hand four-channel MV textures; .rg is read.
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return f;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT TypedScalar(DXGI_FORMAT f)
{
    switch (f)
    {
    case DXGI_FORMAT_R8_TYPELESS: return DXGI_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R16_TYPELESS: return DXGI_FORMAT_R16_FLOAT;
    case DXGI_FORMAT_R32_TYPELESS: return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R32_FLOAT:
        return f;
    // A multi-channel mask or exposure: .r is read.
    case DXGI_FORMAT_R8G8_TYPELESS: return DXGI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return f;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

struct Tex
{
    ComPtr<ID3D12Resource> res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint16_t mips = 1;
};

bool Readable(D3D12_RESOURCE_STATES s)
{
    return (s & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0;
}

// A batch of barriers, flushed before each dispatch.
struct Barriers
{
    ID3D12GraphicsCommandList* cl = nullptr;
    D3D12_RESOURCE_BARRIER b[32] {};
    UINT n = 0;

    void Add(const D3D12_RESOURCE_BARRIER& x)
    {
        if (n == 32)
            Flush();
        b[n++] = x;
    }
    void Raw(ID3D12Resource* r, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
    {
        if (!r || before == after)
            return;
        D3D12_RESOURCE_BARRIER x {};
        x.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        x.Transition.pResource = r;
        x.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        x.Transition.StateBefore = before;
        x.Transition.StateAfter = after;
        Add(x);
    }
    void To(Tex& t, D3D12_RESOURCE_STATES s)
    {
        if (!t.res || t.state == s)
            return;
        Raw(t.res.Get(), t.state, s);
        t.state = s;
    }
    void Uav(ID3D12Resource* r)
    {
        D3D12_RESOURCE_BARRIER x {};
        x.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        x.UAV.pResource = r;
        Add(x);
    }
    void Flush()
    {
        if (n)
            cl->ResourceBarrier(n, b);
        n = 0;
    }
};

float R2(uint64_t n, int axis)
{
    // Roberts' R2 sequence: fractional parts of n * (1/g, 1/g^2), g the plastic number.
    const double a = axis == 0 ? 0.75487766624669276 : 0.56984029099805327;
    const double v = 0.5 + a * static_cast<double>(n % 1000003ull);
    return static_cast<float>(v - std::floor(v));
}

uint32_t Hash32(uint64_t x)
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdull;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ull;
    x ^= x >> 33;
    return static_cast<uint32_t>(x);
}

uint32_t AlignUp(uint32_t v, uint32_t a)
{
    return (v + a - 1) / a * a;
}
} // namespace

void SetLogSink(LogSink sink)
{
    g_sink.store(sink, std::memory_order_release);
}

const TierInfo& Tier(int quality)
{
    return kTiers[std::clamp(quality, 0, 3)];
}

void TraceSize(uint32_t renderW, uint32_t renderH, int quality, float traceCapMpx, uint32_t& traceW, uint32_t& traceH)
{
    const TierInfo& t = Tier(quality);
    const double cap = (traceCapMpx > 0.0f ? std::clamp(traceCapMpx, 0.05f, 8.0f) : t.capMpx) * 1.0e6;
    const double px = static_cast<double>(renderW) * renderH;
    const double ratio = std::max<double>(t.minRatio, std::sqrt(px / cap));
    traceW = std::max(1u, static_cast<uint32_t>(std::ceil(renderW / ratio)));
    traceH = std::max(1u, static_cast<uint32_t>(std::ceil(renderH / ratio)));
}

const char* QualityName(int quality)
{
    static const char* names[] = { "Low", "Medium", "High", "Ultra", "Auto" };
    return names[std::clamp(quality, 0, 4)];
}

const char* DepthName(int convention)
{
    switch (convention)
    {
    case GI_DEPTH_STANDARD: return "standard";
    case GI_DEPTH_REVERSED: return "reversed";
    case GI_DEPTH_LINEAR: return "linear";
    default: return "unknown";
    }
}

struct Effect::Impl
{
    ComPtr<ID3D12Device> device;
    bool typedUavLoads = false;

    // Pipelines, built on a worker thread (design 3.4). 0 not started, 1 building, 2 ready, 3 failed.
    std::thread worker;
    std::atomic<int> pipelines { 0 };
    std::string pipelineError;
    ComPtr<ID3D12RootSignature> rootSig;
    ComPtr<ID3D12PipelineState> pso[kPassCount];

    // Rings (created by the first Record that runs the passes).
    ComPtr<ID3D12DescriptorHeap> heap;    // shader-visible, kRing * kDescPerRecord
    ComPtr<ID3D12DescriptorHeap> cpuHeap; // not shader-visible: kTable null descriptors + the probe clear view
    UINT descSize = 0;
    ComPtr<ID3D12Resource> cbRing;
    uint8_t* cbMapped = nullptr;
    ComPtr<ID3D12Resource> readback;
    const uint8_t* readbackMapped = nullptr;
    ComPtr<ID3D12QueryHeap> queries;
    uint64_t tsFrequency = 0;
    bool tsTried = false;

    // Resources. Full = the render-grid output; Trace = everything on the trace grid, plus the state buffer.
    struct FullSet
    {
        uint32_t w = 0, h = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        Tex out;
    } full;
    struct TraceSet
    {
        uint32_t w = 0, h = 0;
        Tex zHalf[2], nHalf[2], zMip, radMip, trace, gi, histGI[2], histFast[2], histMom[2], histLen[2], bounce;
        ComPtr<ID3D12Resource> state;
        D3D12_RESOURCE_STATES stateState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    } tr;
    uint32_t smallFrames = 0;

    struct Retired
    {
        ComPtr<ID3D12Resource> res;
        uint64_t record;
    };
    std::vector<Retired> graveyard;

    // Formats checked once each (typed UAV store for `out`).
    struct FormatCheck
    {
        DXGI_FORMAT f;
        bool store;
    };
    std::vector<FormatCheck> formatChecks;

    // Frame to frame.
    uint64_t records = 0;
    bool historyValid = false;
    bool resetRequested = false;
    bool haveLast = false;
    int lastQuality = -1;
    float lastRadius = 0.0f, lastThickness = 0.0f;
    uint32_t lastRenderW = 0, lastRenderH = 0, lastTraceW = 0, lastTraceH = 0;
    float prevPreExposure = 0.0f, prevJitterX = 0.0f, prevJitterY = 0.0f;
    int depthConvention = -1, depthSource = 0, contradictFrames = 0;
    bool lastDepthFlag = false;
    uint32_t probe[GI_PROBE_WORDS] = {};
    float gpuMs = -1.0f;

    bool failed = false;
    mutable std::mutex statsMutex;
    Stats stats;

    // Dispatch bookkeeping of the Record in progress.
    uint32_t recordBase = 0;
    uint32_t dispatches = 0;

    ~Impl()
    {
        if (worker.joinable())
            worker.join();
        if (cbRing && cbMapped)
            cbRing->Unmap(0, nullptr);
        if (readback && readbackMapped)
            readback->Unmap(0, nullptr);
    }

    void SetStatus(const std::string& s, bool ran)
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        if (stats.status != s && !s.empty())
            Log(0, "AMDNR Screen GI: " + s);
        stats.status = s;
        stats.ranLastFrame = ran;
    }

    void Fail(const std::string& why)
    {
        failed = true;
        Log(2, "AMDNR Screen GI: turned off for this session - " + why);
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.failed = true;
        stats.ranLastFrame = false;
        stats.status = "turned off after an error (see log)";
    }

    // ---------------------------------------------------------------- pipelines

    void BuildPipelines()
    {
        try
        {
            D3D12_DESCRIPTOR_RANGE ranges[2] {};
            ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            ranges[0].NumDescriptors = kSrvSlots;
            ranges[0].BaseShaderRegister = 0;
            ranges[0].OffsetInDescriptorsFromTableStart = 0;
            ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            ranges[1].NumDescriptors = kUavSlots;
            ranges[1].BaseShaderRegister = 0;
            ranges[1].OffsetInDescriptorsFromTableStart = kSrvSlots;

            D3D12_ROOT_PARAMETER params[3] {};
            params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[0].Descriptor.ShaderRegister = 0;
            params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            params[1].Constants.ShaderRegister = 1;
            params[1].Constants.Num32BitValues = sizeof(GiPassConstants) / 4;
            params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[2].DescriptorTable.NumDescriptorRanges = 2;
            params[2].DescriptorTable.pDescriptorRanges = ranges;
            params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            D3D12_STATIC_SAMPLER_DESC samplers[2] {};
            for (UINT i = 0; i < 2; ++i)
            {
                samplers[i].Filter = i == 0 ? D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                samplers[i].AddressU = samplers[i].AddressV = samplers[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                samplers[i].MaxLOD = D3D12_FLOAT32_MAX;
                samplers[i].ShaderRegister = i;
                samplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                samplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            }

            D3D12_ROOT_SIGNATURE_DESC desc {};
            desc.NumParameters = 3;
            desc.pParameters = params;
            desc.NumStaticSamplers = 2;
            desc.pStaticSamplers = samplers;
            desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

            ComPtr<ID3DBlob> blob, error;
            const HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
            if (FAILED(hr))
            {
                std::string msg = "root signature serialisation failed (" + Hex(hr) + ")";
                if (error)
                    msg += std::string(": ") + static_cast<const char*>(error->GetBufferPointer());
                throw std::runtime_error(msg);
            }
            Check(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig)),
                  "CreateRootSignature");

            for (int i = 0; i < kPassCount; ++i)
            {
                D3D12_COMPUTE_PIPELINE_STATE_DESC pd {};
                pd.pRootSignature = rootSig.Get();
                pd.CS.pShaderBytecode = kShaders[i].data;
                pd.CS.BytecodeLength = kShaders[i].size;
                const HRESULT phr = device->CreateComputePipelineState(&pd, IID_PPV_ARGS(&pso[i]));
                if (FAILED(phr))
                    throw std::runtime_error(std::string("pipeline gi_") + kShaders[i].name + " failed (" + Hex(phr) +
                                             ")");
            }
            pipelines.store(2, std::memory_order_release);
            Log(0, "AMDNR Screen GI: pipelines ready (7 compute passes, cs_6_2)");
        }
        catch (const std::exception& e)
        {
            pipelineError = e.what();
            pipelines.store(3, std::memory_order_release);
        }
    }

    // ---------------------------------------------------------------- resources

    void Retire(ComPtr<ID3D12Resource>& r)
    {
        if (r)
            graveyard.push_back({ std::move(r), records });
        r.Reset();
    }
    void Retire(Tex& t)
    {
        Retire(t.res);
        t = Tex {};
    }

    Tex MakeTex(uint32_t w, uint32_t h, DXGI_FORMAT f, uint16_t mips, const wchar_t* name)
    {
        D3D12_HEAP_PROPERTIES hp {};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC d {};
        d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width = w;
        d.Height = h;
        d.DepthOrArraySize = 1;
        d.MipLevels = mips;
        d.Format = f;
        d.SampleDesc.Count = 1;
        d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        d.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Tex t;
        Check(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &d, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                              nullptr, IID_PPV_ARGS(&t.res)),
              "CreateCommittedResource (texture)");
        t.res->SetName(name);
        t.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        t.format = f;
        t.mips = mips;
        return t;
    }

    ComPtr<ID3D12Resource> MakeBuffer(uint64_t bytes, D3D12_HEAP_TYPE type, D3D12_RESOURCE_FLAGS flags,
                                      D3D12_RESOURCE_STATES state, const wchar_t* name)
    {
        D3D12_HEAP_PROPERTIES hp {};
        hp.Type = type;
        D3D12_RESOURCE_DESC d {};
        d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        d.Width = bytes;
        d.Height = 1;
        d.DepthOrArraySize = 1;
        d.MipLevels = 1;
        d.Format = DXGI_FORMAT_UNKNOWN;
        d.SampleDesc.Count = 1;
        d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        d.Flags = flags;
        ComPtr<ID3D12Resource> r;
        Check(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, IID_PPV_ARGS(&r)),
              "CreateCommittedResource (buffer)");
        r->SetName(name);
        return r;
    }

    bool CanStore(DXGI_FORMAT f)
    {
        for (const auto& c : formatChecks)
            if (c.f == f)
                return c.store;
        D3D12_FEATURE_DATA_FORMAT_SUPPORT fs {};
        fs.Format = f;
        bool store = false;
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fs, sizeof(fs))))
            store = (fs.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE) != 0 &&
                    (fs.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0;
        formatChecks.push_back({ f, store });
        return store;
    }

    void EnsureRings(ID3D12CommandQueue* timingQueue)
    {
        if (!heap)
        {
            D3D12_DESCRIPTOR_HEAP_DESC hd {};
            hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            hd.NumDescriptors = kRing * kDescPerRecord;
            hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            Check(device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&heap)), "CreateDescriptorHeap (ring)");
            heap->SetName(L"AMDNR GI descriptor ring");
            descSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_DESCRIPTOR_HEAP_DESC cd {};
            cd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            cd.NumDescriptors = kTable + 1;
            cd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            Check(device->CreateDescriptorHeap(&cd, IID_PPV_ARGS(&cpuHeap)), "CreateDescriptorHeap (cpu)");
            // The null table: SRV slots then UAV slots, copied into every dispatch's table first.
            for (uint32_t i = 0; i < kTable; ++i)
                WriteNull(CpuOf(cpuHeap.Get(), i), i >= kSrvSlots);
        }
        if (!cbRing)
        {
            cbRing = MakeBuffer(uint64_t(kRing) * kCbStride, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
                                D3D12_RESOURCE_STATE_GENERIC_READ, L"AMDNR GI constants ring");
            D3D12_RANGE none { 0, 0 };
            Check(cbRing->Map(0, &none, reinterpret_cast<void**>(&cbMapped)), "Map (constants)");
        }
        if (!readback)
        {
            readback = MakeBuffer(kReadbackBytes, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE,
                                  D3D12_RESOURCE_STATE_COPY_DEST, L"AMDNR GI readback ring");
            D3D12_RANGE all { 0, kReadbackBytes };
            void* p = nullptr;
            Check(readback->Map(0, &all, &p), "Map (readback)");
            readbackMapped = static_cast<const uint8_t*>(p);
        }
        if (!tsTried && timingQueue)
        {
            // Timestamps need the executing queue's frequency. No queue known yet: no readout, try again next frame.
            // (No queue of our own: OptiScaler's hooks watch queue creation.)
            tsTried = true;
            if (FAILED(timingQueue->GetTimestampFrequency(&tsFrequency)))
                tsFrequency = 0;
            if (tsFrequency)
            {
                D3D12_QUERY_HEAP_DESC qh {};
                qh.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
                qh.Count = kRing * kTimestamps;
                if (FAILED(device->CreateQueryHeap(&qh, IID_PPV_ARGS(&queries))))
                    queries.Reset();
            }
        }
    }

    // Returns true when the history no longer matches (a new trace set).
    bool EnsureTrace(uint32_t needW, uint32_t needH)
    {
        const bool fits = tr.state && needW <= tr.w && needH <= tr.h;
        bool shrink = false;
        if (fits)
        {
            // Shrink after 300 Records at no more than half the allocated area (design 3.5).
            if (uint64_t(needW) * needH * 2 > uint64_t(tr.w) * tr.h)
            {
                smallFrames = 0;
                return false;
            }
            if (++smallFrames < kShrinkAfter)
                return false;
            shrink = true;
        }
        smallFrames = 0;
        uint32_t w = std::max(AlignUp(needW, kTraceAlign), kTraceAlign);
        uint32_t h = std::max(AlignUp(needH, kTraceAlign), kTraceAlign);
        if (!shrink && tr.state)
        {
            // Growing: keep the larger extent of the old set too, so a DRS swing does not reallocate back and forth.
            w = std::max(w, tr.w);
            h = std::max(h, tr.h);
        }
        TraceSet fresh;
        fresh.w = w;
        fresh.h = h;
        const uint16_t levels = GI_PYRAMID_LEVELS;
        for (int i = 0; i < 2; ++i)
        {
            fresh.zHalf[i] = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R32G32_FLOAT, 1, L"AMDNR GI zHalf");
            fresh.nHalf[i] = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R16G16_SNORM, 1, L"AMDNR GI nHalf");
            fresh.histGI[i] = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, L"AMDNR GI histGI");
            fresh.histFast[i] = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, L"AMDNR GI histFast");
            fresh.histMom[i] = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R16G16_FLOAT, 1, L"AMDNR GI histMom");
            fresh.histLen[i] = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R8_UINT, 1, L"AMDNR GI histLen");
        }
        fresh.zMip = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R32G32_FLOAT, levels, L"AMDNR GI zMip");
        fresh.radMip = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R16G16B16A16_FLOAT, levels, L"AMDNR GI radMip");
        fresh.trace = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, L"AMDNR GI trace");
        fresh.gi = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, L"AMDNR GI gi");
        fresh.bounce = MakeTex(fresh.w, fresh.h, DXGI_FORMAT_R11G11B10_FLOAT, 1, L"AMDNR GI bounceHalf");
        // Buffers start in COMMON whatever the initial state says; the first Record transitions it.
        fresh.state = MakeBuffer(GI_STATE_BYTES, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                 D3D12_RESOURCE_STATE_COMMON, L"AMDNR GI state");
        fresh.stateState = D3D12_RESOURCE_STATE_COMMON;

        RetireTrace();
        tr = std::move(fresh);

        // The probe clear view lives in the CPU heap (ClearUnorderedAccessViewUint needs a CPU handle).
        WriteProbeView(CpuOf(cpuHeap.Get(), kTable));

        const double mb = (double(tr.w) * tr.h * (2 * (8 + 4 + 8 + 8 + 4 + 1) + 8 * 4 / 3.0 + 8 * 4 / 3.0 + 8 + 8 + 4)) /
                          (1024.0 * 1024.0);
        Log(0, "AMDNR Screen GI: trace-grid resources " + std::to_string(tr.w) + "x" + std::to_string(tr.h) + " (about " +
                   std::to_string(int(mb + 0.5)) + " MB)");
        return true;
    }

    void RetireTrace()
    {
        for (int i = 0; i < 2; ++i)
        {
            Retire(tr.zHalf[i]);
            Retire(tr.nHalf[i]);
            Retire(tr.histGI[i]);
            Retire(tr.histFast[i]);
            Retire(tr.histMom[i]);
            Retire(tr.histLen[i]);
        }
        Retire(tr.zMip);
        Retire(tr.radMip);
        Retire(tr.trace);
        Retire(tr.gi);
        Retire(tr.bounce);
        Retire(tr.state);
        tr.w = tr.h = 0;
    }

    void EnsureFull(uint32_t w, uint32_t h, DXGI_FORMAT f)
    {
        if (full.out.res && full.w == w && full.h == h && full.format == f)
            return;
        Retire(full.out);
        full.out = MakeTex(w, h, f, 1, L"AMDNR GI out");
        full.w = w;
        full.h = h;
        full.format = f;
        Log(0, "AMDNR Screen GI: output " + std::to_string(w) + "x" + std::to_string(h) + " format " +
                   std::to_string(int(f)));
    }

    void FreeRetired()
    {
        graveyard.erase(std::remove_if(graveyard.begin(), graveyard.end(),
                                       [&](const Retired& r) { return records - r.record >= kRing; }),
                        graveyard.end());
    }

    // ---------------------------------------------------------------- descriptors

    D3D12_CPU_DESCRIPTOR_HANDLE CpuOf(ID3D12DescriptorHeap* h, uint32_t i) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE c = h->GetCPUDescriptorHandleForHeapStart();
        c.ptr += SIZE_T(i) * descSize;
        return c;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GpuOf(uint32_t i) const
    {
        D3D12_GPU_DESCRIPTOR_HANDLE g = heap->GetGPUDescriptorHandleForHeapStart();
        g.ptr += UINT64(i) * descSize;
        return g;
    }

    void WriteNull(D3D12_CPU_DESCRIPTOR_HANDLE h, bool uav)
    {
        if (uav)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC u {};
            u.Format = DXGI_FORMAT_R32_FLOAT;
            u.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            device->CreateUnorderedAccessView(nullptr, nullptr, &u, h);
        }
        else
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC s {};
            s.Format = DXGI_FORMAT_R32_FLOAT;
            s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            s.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(nullptr, &s, h);
        }
    }

    void WriteProbeView(D3D12_CPU_DESCRIPTOR_HANDLE h)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC u {};
        u.Format = DXGI_FORMAT_R32_TYPELESS;
        u.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        u.Buffer.FirstElement = GI_STATE_PROBE / 4;
        u.Buffer.NumElements = GI_PROBE_WORDS;
        u.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        device->CreateUnorderedAccessView(tr.state.Get(), nullptr, &u, h);
    }

    struct Table
    {
        uint32_t base = 0;
    };

    // The next dispatch's table, pre-filled with null descriptors.
    Table NextTable()
    {
        Table t;
        t.base = recordBase + std::min(dispatches, kMaxDispatches - 1) * kTable;
        ++dispatches;
        device->CopyDescriptorsSimple(kTable, CpuOf(heap.Get(), t.base), CpuOf(cpuHeap.Get(), 0),
                                      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return t;
    }

    void Srv(const Table& t, uint32_t slot, ID3D12Resource* r, DXGI_FORMAT f, uint32_t mips = 1)
    {
        if (!r || f == DXGI_FORMAT_UNKNOWN)
            return;
        D3D12_SHADER_RESOURCE_VIEW_DESC s {};
        s.Format = f;
        s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        s.Texture2D.MipLevels = mips;
        device->CreateShaderResourceView(r, &s, CpuOf(heap.Get(), t.base + slot));
    }
    void Srv(const Table& t, uint32_t slot, const Tex& x)
    {
        Srv(t, slot, x.res.Get(), x.format, x.mips);
    }
    void Uav(const Table& t, uint32_t slot, const Tex& x, uint32_t mip = 0)
    {
        if (!x.res)
            return;
        D3D12_UNORDERED_ACCESS_VIEW_DESC u {};
        u.Format = x.format;
        u.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        u.Texture2D.MipSlice = mip;
        device->CreateUnorderedAccessView(x.res.Get(), nullptr, &u, CpuOf(heap.Get(), t.base + kSrvSlots + slot));
    }
    void UavState(const Table& t, uint32_t slot)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC u {};
        u.Format = DXGI_FORMAT_R32_TYPELESS;
        u.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        u.Buffer.NumElements = GI_STATE_BYTES / 4;
        u.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        device->CreateUnorderedAccessView(tr.state.Get(), nullptr, &u, CpuOf(heap.Get(), t.base + kSrvSlots + slot));
    }

    void Dispatch(ID3D12GraphicsCommandList* cl, Barriers& b, PassId pass, const Table& t, uint32_t w, uint32_t h,
                  uint32_t a0 = 0, uint32_t a1 = 0)
    {
        b.Flush();
        GiPassConstants pc {};
        pc.a.x = a0;
        pc.a.y = a1;
        cl->SetPipelineState(pso[pass].Get());
        cl->SetComputeRoot32BitConstants(1, sizeof(pc) / 4, &pc, 0);
        cl->SetComputeRootDescriptorTable(2, GpuOf(t.base));
        cl->Dispatch((w + 7) / 8, (h + 7) / 8, 1);
    }

    // ---------------------------------------------------------------- the frame

    void UpdateDepthVerdict(bool flagInverted, int setting)
    {
        if (setting >= 0 && setting <= 2)
        {
            depthConvention = setting;
            depthSource = 2;
            return;
        }
        // A new flag value (another title stream, a re-created feature) starts over from the flag.
        if (depthConvention < 0 || depthSource == 2 || flagInverted != lastDepthFlag)
        {
            lastDepthFlag = flagInverted;
            depthConvention = flagInverted ? GI_DEPTH_REVERSED : GI_DEPTH_STANDARD;
            depthSource = 0;
            contradictFrames = 0;
        }
        const double total = probe[GI_PROBE_TOTAL];
        if (total < 64.0)
            return;
        const double zeros = probe[GI_PROBE_ZERO] / total, ones = probe[GI_PROBE_ONE] / total;
        const double above = probe[GI_PROBE_ABOVE_ONE] / total;
        if (above > 0.01)
        {
            if (depthConvention != GI_DEPTH_LINEAR)
                Log(1, "AMDNR Screen GI: depth values above 1 - the title stores linear depth; using it");
            depthConvention = GI_DEPTH_LINEAR;
            depthSource = 1;
            contradictFrames = 0;
            return;
        }
        if (depthConvention == GI_DEPTH_LINEAR && above == 0.0)
        {
            depthConvention = flagInverted ? GI_DEPTH_REVERSED : GI_DEPTH_STANDARD;
            depthSource = 0;
        }
        // Evidence against the current convention: the far plane shows up at the other end of the range.
        const bool contradicts = (depthConvention == GI_DEPTH_REVERSED && ones > 0.02 && zeros < 0.001) ||
                                 (depthConvention == GI_DEPTH_STANDARD && zeros > 0.02 && ones < 0.001);
        contradictFrames = contradicts ? contradictFrames + 1 : 0;
        if (contradictFrames >= 30)
        {
            depthConvention = depthConvention == GI_DEPTH_REVERSED ? GI_DEPTH_STANDARD : GI_DEPTH_REVERSED;
            depthSource = 1;
            contradictFrames = 0;
            Log(1, std::string("AMDNR Screen GI: the depth data contradicts the DepthInverted flag for 30 frames; "
                               "using ") +
                       DepthName(depthConvention) + " depth");
        }
    }

    void ReadBack()
    {
        if (!readbackMapped || records < kProbeLag)
            return;
        const uint32_t slot = uint32_t((records - kProbeLag) % kRing);
        std::memcpy(probe, readbackMapped + slot * GI_READBACK_BYTES + GI_STATE_PROBE, sizeof(probe));
        if (queries && tsFrequency)
        {
            uint64_t ts[kTimestamps];
            std::memcpy(ts, readbackMapped + kReadbackTimestampBase + slot * kTimestamps * sizeof(uint64_t), sizeof(ts));
            if (ts[1] > ts[0])
            {
                const float ms = float(double(ts[1] - ts[0]) * 1000.0 / double(tsFrequency));
                gpuMs = gpuMs < 0.0f ? ms : gpuMs + (ms - gpuMs) * 0.1f;
            }
        }
    }

    ID3D12Resource* RecordFrame(ID3D12GraphicsCommandList* cl, const Inputs& in, const Settings& sIn)
    {
        // ---------- prepare (may throw; nothing recorded yet) ----------
        const int ps = pipelines.load(std::memory_order_acquire);
        if (ps == 0)
        {
            if (!typedUavLoads)
            {
                Fail("the GPU cannot read RGBA16F/RG32F UAVs (TypedUAVLoadAdditionalFormats)");
                return nullptr;
            }
            pipelines.store(1, std::memory_order_release);
            worker = std::thread([this] { BuildPipelines(); });
            SetStatus("shaders not ready yet", false);
            return nullptr;
        }
        if (ps == 1)
        {
            SetStatus("shaders not ready yet", false);
            return nullptr;
        }
        if (ps == 3)
        {
            Fail(pipelineError);
            return nullptr;
        }

        const D3D12_COMMAND_LIST_TYPE type = cl->GetType();
        if (type != D3D12_COMMAND_LIST_TYPE_DIRECT && type != D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            SetStatus("skipped: the upscaler's command list cannot run compute", false);
            return nullptr;
        }
        if (!in.colour || !in.depth)
        {
            SetStatus(in.colour ? "no depth" : "no colour", false);
            return nullptr;
        }
        const D3D12_RESOURCE_DESC cd = in.colour->GetDesc();
        const D3D12_RESOURCE_DESC dd = in.depth->GetDesc();
        const DXGI_FORMAT colourFmt = TypedColour(cd.Format);
        const DXGI_FORMAT depthFmt = DepthSrvFormat(dd.Format);
        if (cd.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || cd.SampleDesc.Count != 1 ||
            cd.DepthOrArraySize != 1 || colourFmt == DXGI_FORMAT_UNKNOWN)
        {
            SetStatus("colour format " + std::to_string(int(cd.Format)) + " not supported", false);
            return nullptr;
        }
        if (dd.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || dd.SampleDesc.Count != 1 ||
            depthFmt == DXGI_FORMAT_UNKNOWN)
        {
            SetStatus("depth format " + std::to_string(int(dd.Format)) + " not supported", false);
            return nullptr;
        }
        if (dd.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)
        {
            SetStatus("depth is not shader-readable (no readable copy in this build)", false);
            return nullptr;
        }
        const uint32_t rw = in.renderWidth ? in.renderWidth : uint32_t(cd.Width);
        const uint32_t rh = in.renderHeight ? in.renderHeight : cd.Height;
        if (rw < 16 || rh < 16 || rw > cd.Width || rh > cd.Height || rw > dd.Width || rh > dd.Height)
        {
            SetStatus("render size " + std::to_string(rw) + "x" + std::to_string(rh) + " does not fit the inputs",
                      false);
            return nullptr;
        }
        const bool unormIn = IsUnorm(colourFmt);
        if (in.isHdr && unormIn && sIn.encoding < 0 && !IsSrgb(colourFmt))
        {
            SetStatus("HDR flag on an 8/10-bit colour: set [AmdGi] Encoding to use it", false);
            return nullptr;
        }
        DXGI_FORMAT motionFmt = DXGI_FORMAT_UNKNOWN;
        if (in.motion)
            motionFmt = TypedTwoChannel(in.motion->GetDesc().Format);
        const bool hasMotion = in.motion && motionFmt != DXGI_FORMAT_UNKNOWN;
        const DXGI_FORMAT reactiveFmt = in.reactive ? TypedScalar(in.reactive->GetDesc().Format) : DXGI_FORMAT_UNKNOWN;
        const DXGI_FORMAT exposureFmt = in.exposure ? TypedScalar(in.exposure->GetDesc().Format) : DXGI_FORMAT_UNKNOWN;

        Settings s = sIn;
        s.quality = std::clamp(s.quality, 0, 3);
        const TierInfo& tier = Tier(s.quality);
        uint32_t tw = 0, th = 0;
        TraceSize(rw, rh, s.quality, s.traceCap, tw, th);

        // out: the game's format when compute can store it (the copy pass is then bit-exact), else RGBA16F. sRGB
        // inputs get RGBA16F holding the linear values the upscaler's SRV would have read.
        DXGI_FORMAT outFmt = colourFmt;
        if (IsSrgb(outFmt) || !CanStore(outFmt))
            outFmt = DXGI_FORMAT_R16G16B16A16_FLOAT;

        EnsureRings(in.timingQueue);
        EnsureFull(uint32_t(cd.Width), cd.Height, outFmt);
        bool reset = EnsureTrace(tw, th);
        FreeRetired();

        // History reset triggers (design 3.5).
        reset = reset || in.reset || !historyValid || resetRequested;
        if (haveLast)
        {
            reset = reset || s.quality != lastQuality || s.radius != lastRadius || s.thickness != lastThickness;
            reset = reset || tw != lastTraceW || th != lastTraceH;
            const float jx = std::abs(float(rw) / float(std::max(lastRenderW, 1u)) - 1.0f);
            const float jy = std::abs(float(rh) / float(std::max(lastRenderH, 1u)) - 1.0f);
            reset = reset || jx > 0.25f || jy > 0.25f;
        }
        resetRequested = false;

        ReadBack();
        UpdateDepthVerdict(in.depthInverted, s.depthConvention);

        // Encoding: display-referred UNORM colour is decoded, composed and re-encoded (design 5.1).
        int encoding = s.encoding;
        if (encoding < 0 || encoding > 2)
            encoding = (!unormIn || IsSrgb(colourFmt) || in.isHdr) ? GI_ENC_LINEAR : GI_ENC_SRGB;

        // ---------- constants ----------
        GiFrameConstants c {};
        c.renderSize = { rw, rh, tw, th };
        c.invSizes = { 1.0f / rw, 1.0f / rh, 1.0f / tw, 1.0f / th };
        c.allocSize = { full.w, full.h, tr.w, tr.h };
        c.traceRatio = { float(rw) / tw, float(rh) / th, float(tw) / rw, float(th) / rh };
        const float fovY = std::clamp(in.camera.fovY, 0.1f, 3.0f);
        const float aspect = float(rw) / float(rh);
        const float tanY = std::tan(fovY * 0.5f);
        const bool nearFar = in.camera.nearZ > 0.0f && in.camera.farZ > in.camera.nearZ;
        c.camera = { tanY * aspect, tanY, nearFar ? in.camera.nearZ : 0.0f, nearFar ? in.camera.farZ : 0.0f };
        c.cameraInfo = { fovY, aspect, float(in.camera.source), nearFar ? 1.0f : 0.0f };
        c.depthInfo = { depthConvention, in.depthInverted ? 1 : 0, depthSource, 0 };
        c.depthParams = { 1.0e-6f, 1.0f, 0.0f, 0.0f };
        const uint32_t mw = hasMotion ? (in.motionWidth ? in.motionWidth : rw) : rw;
        const uint32_t mh = hasMotion ? (in.motionHeight ? in.motionHeight : rh) : rh;
        c.motion = { in.mvScaleX, in.mvScaleY, float(mw) / rw, float(mh) / rh };
        c.motionInfo = { hasMotion ? 1 : 0, in.mvJittered ? 1 : 0, in.mvLowRes ? 1 : 0, 0 };
        c.jitter = { in.jitterX, in.jitterY, prevJitterX, prevJitterY };
        const float pre = in.preExposure > 0.0f ? in.preExposure : 1.0f;
        const float ratio = (prevPreExposure > 0.0f && in.preExposure > 0.0f) ? in.preExposure / prevPreExposure : 1.0f;
        c.exposure = { pre, std::isfinite(ratio) ? ratio : 1.0f, in.exposureScale > 0.0f ? in.exposureScale : 1.0f,
                       exposureFmt != DXGI_FORMAT_UNKNOWN ? 1.0f : 0.0f };
        c.colourInfo = { encoding, in.isHdr ? 1 : 0, IsUnorm(outFmt) ? 1 : 0, reactiveFmt != DXGI_FORMAT_UNKNOWN ? 1 : 0 };
        c.frame = { uint32_t(records), reset ? 1u : 0u, Hash32(records), uint32_t(std::clamp(s.debugView, 0, 10)) };
        c.noise = { R2(records, 0), R2(records, 1), 0.0f, 0.0f };
        c.tier = { s.quality, tier.slices, tier.stepsPerSide, tier.sampleNormals ? 1 : 0 };
        const float thickness = s.thickness > 0.0f ? std::clamp(s.thickness, 0.01f, 1.0f) : tier.thicknessAuto;
        c.trace = { tier.radiusFraction * std::clamp(s.radius, 0.25f, 4.0f) * float(th), s.radius, thickness,
                    float(GI_PYRAMID_LEVELS - 1) };
        c.temporal = { float(tier.maxHistory), 1.0f / float(tier.maxHistory), 3.0f, 2.0f };
        c.spatial = { tier.atrousIterations, std::clamp(s.albedoMode, 0, 2), std::clamp(s.translucency, 0, 1), 0 };
        const float intensity = std::clamp(s.intensity, 0.0f, 3.0f);
        // Multi-bounce loop gain <= 0.8 at any dial setting (design 2.1 item 9).
        const float feedbackEff = std::min(std::clamp(s.feedback, 0.0f, 0.8f), 0.8f / (0.9f * std::max(intensity, 1.0f)));
        c.composite0 = { intensity, std::clamp(s.occlusion, 0.0f, 2.0f), std::clamp(s.saturation, 0.0f, 2.0f),
                         std::clamp(s.sky, 0.0f, 2.0f) };
        c.composite1 = { feedbackEff, s.nearFade, s.distanceFade, std::clamp(s.aoLitProtect, 0.0f, 1.0f) };
        for (int r = 0; r < 4; ++r)
        {
            const float* v = in.camera.viewToClip + r * 4;
            const float* q = in.camera.clipToPrevClip + r * 4;
            c.viewToClip[r] = { v[0], v[1], v[2], v[3] };
            c.clipToPrevClip[r] = { q[0], q[1], q[2], q[3] };
        }
        c.matrixInfo = { in.camera.hasViewToClip ? 1 : 0, in.camera.hasClipToPrevClip ? 1 : 0, 0, 0 };

        const uint32_t slot = uint32_t(records % kRing);
        std::memcpy(cbMapped + slot * kCbStride, &c, sizeof(c));
        recordBase = slot * kDescPerRecord;
        dispatches = 0;

        // ---------- record (no throw from here) ----------
        const int cur = int(records & 1), prev = cur ^ 1;
        Barriers b;
        b.cl = cl;

        ID3D12DescriptorHeap* heaps[] = { heap.Get() };
        cl->SetDescriptorHeaps(1, heaps);
        cl->SetComputeRootSignature(rootSig.Get());
        cl->SetComputeRootConstantBufferView(0, cbRing->GetGPUVirtualAddress() + UINT64(slot) * kCbStride);
        if (queries)
            cl->EndQuery(queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slot * kTimestamps + 0);

        // Game inputs: read as NON_PIXEL_SHADER_RESOURCE; left as they came.
        struct GameIn
        {
            ID3D12Resource* r;
            D3D12_RESOURCE_STATES s;
        };
        GameIn games[5] = { { in.colour, in.colourState },
                            { in.depth, in.depthState },
                            { hasMotion ? in.motion : nullptr, in.motionState },
                            { reactiveFmt != DXGI_FORMAT_UNKNOWN ? in.reactive : nullptr, in.reactiveState },
                            { exposureFmt != DXGI_FORMAT_UNKNOWN ? in.exposure : nullptr, in.exposureState } };
        for (int i = 0; i < 5; ++i)
            for (int j = 0; j < i; ++j)
                if (games[j].r == games[i].r)
                    games[i].r = nullptr; // the same texture twice: one transition
        for (auto& g : games)
            if (g.r && !Readable(g.s))
                b.Raw(g.r, g.s, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        ID3D12Resource* colour = in.colour;
        ID3D12Resource* depth = in.depth;
        ID3D12Resource* motion = hasMotion ? in.motion : nullptr;
        ID3D12Resource* reactive = reactiveFmt != DXGI_FORMAT_UNKNOWN ? in.reactive : nullptr;
        ID3D12Resource* exposure = exposureFmt != DXGI_FORMAT_UNKNOWN ? in.exposure : nullptr;

        // Probe clear (16 words), before P1 counts into it.
        b.Raw(tr.state.Get(), tr.stateState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        tr.stateState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        b.Flush();
        {
            const uint32_t clearSlot = recordBase + kDescPerRecord - 1;
            WriteProbeView(CpuOf(heap.Get(), clearSlot));
            const UINT zero[4] = { 0, 0, 0, 0 };
            cl->ClearUnorderedAccessViewUint(GpuOf(clearSlot), CpuOf(cpuHeap.Get(), kTable), tr.state.Get(), zero, 0,
                                             nullptr);
            b.Uav(tr.state.Get());
        }

        // P1 prepass
        {
            b.To(tr.zHalf[cur], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.To(tr.nHalf[cur], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            const Table t = NextTable();
            Srv(t, 0, colour, colourFmt);
            Srv(t, 1, depth, depthFmt);
            Srv(t, 2, reactive, reactiveFmt);
            Uav(t, 0, tr.zHalf[cur]);
            Uav(t, 1, tr.nHalf[cur]);
            UavState(t, 2);
            Dispatch(cl, b, kPrepass, t, tw, th);
        }
        // P2 pyramid, level by level (each reads the level above through its UAV)
        {
            b.To(tr.zHalf[cur], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.zHalf[prev], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.nHalf[cur], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.bounce, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.zMip, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.To(tr.radMip, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.Uav(tr.state.Get());
            for (uint32_t level = 0; level < GI_PYRAMID_LEVELS; ++level)
            {
                const Table t = NextTable();
                Srv(t, 0, colour, colourFmt);
                Srv(t, 1, depth, depthFmt);
                Srv(t, 2, reactive, reactiveFmt);
                Srv(t, 3, motion, motionFmt);
                Srv(t, 4, tr.zHalf[cur]);
                Srv(t, 5, tr.zHalf[prev]);
                Srv(t, 6, tr.bounce);
                Srv(t, 7, tr.nHalf[cur]);
                for (uint32_t m = 0; m < GI_PYRAMID_LEVELS; ++m)
                {
                    Uav(t, m, tr.zMip, m);
                    Uav(t, GI_PYRAMID_LEVELS + m, tr.radMip, m);
                }
                UavState(t, 12);
                if (level > 0)
                {
                    b.Uav(tr.zMip.res.Get());
                    b.Uav(tr.radMip.res.Get());
                }
                // Level size rounded up (LevelSize in gi_common.hlsli), so an odd trace grid keeps its last row.
                const uint32_t lw = std::max((tw + (1u << level) - 1u) >> level, 1u);
                const uint32_t lh = std::max((th + (1u << level) - 1u) >> level, 1u);
                Dispatch(cl, b, kPyramid, t, lw, lh, level);
            }
        }
        // P3 trace
        {
            b.To(tr.zMip, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.radMip, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.trace, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.Uav(tr.state.Get());
            const Table t = NextTable();
            Srv(t, 0, tr.zHalf[cur]);
            Srv(t, 1, tr.nHalf[cur]);
            Srv(t, 2, tr.zMip);
            Srv(t, 3, tr.radMip);
            Uav(t, 0, tr.trace);
            UavState(t, 1);
            Dispatch(cl, b, kTrace, t, tw, th);
        }
        // P4 temporal
        {
            b.To(tr.trace, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histGI[prev], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histFast[prev], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histMom[prev], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histLen[prev], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.nHalf[prev], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histGI[cur], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.To(tr.histFast[cur], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.To(tr.histMom[cur], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.To(tr.histLen[cur], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.Uav(tr.state.Get());
            const Table t = NextTable();
            Srv(t, 0, tr.trace);
            Srv(t, 1, tr.histGI[prev]);
            Srv(t, 2, tr.histFast[prev]);
            Srv(t, 3, tr.histMom[prev]);
            Srv(t, 4, tr.histLen[prev]);
            Srv(t, 5, tr.zHalf[cur]);
            Srv(t, 6, tr.zHalf[prev]);
            Srv(t, 7, tr.nHalf[cur]);
            Srv(t, 8, tr.nHalf[prev]);
            Srv(t, 9, motion, motionFmt);
            Srv(t, 10, reactive, reactiveFmt);
            Uav(t, 0, tr.histGI[cur]);
            Uav(t, 1, tr.histFast[cur]);
            Uav(t, 2, tr.histMom[cur]);
            Uav(t, 3, tr.histLen[cur]);
            UavState(t, 4);
            Dispatch(cl, b, kTemporal, t, tw, th);
        }
        // P5 spatial: histGI[cur] -> gi (-> trace). History keeps the pre-spatial value (SpatialFeedback off).
        Tex* final = &tr.histGI[cur];
        {
            b.To(tr.histGI[cur], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histMom[cur], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histLen[cur], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(tr.histFast[cur], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            const int iterations = std::clamp(tier.atrousIterations, 0, 2);
            Tex* dsts[2] = { &tr.gi, &tr.trace };
            for (int it = 0; it < iterations; ++it)
            {
                Tex* dst = dsts[it];
                b.To(*final, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                b.To(*dst, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                const Table t = NextTable();
                Srv(t, 0, *final);
                Srv(t, 1, tr.histMom[cur]);
                Srv(t, 2, tr.histLen[cur]);
                Srv(t, 3, tr.zHalf[cur]);
                Srv(t, 4, tr.nHalf[cur]);
                Uav(t, 0, *dst);
                Dispatch(cl, b, kSpatial, t, tw, th, uint32_t(it), 1u << it);
                final = dst;
            }
        }
        // P6 upsample + composite
        {
            b.To(*final, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            b.To(full.out, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.To(tr.bounce, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            b.Uav(tr.state.Get());
            const Table t = NextTable();
            Srv(t, 0, *final);
            Srv(t, 1, tr.zHalf[cur]);
            Srv(t, 2, tr.nHalf[cur]);
            Srv(t, 3, depth, depthFmt);
            Srv(t, 4, colour, colourFmt);
            Srv(t, 5, reactive, reactiveFmt);
            Srv(t, 6, tr.radMip);
            Srv(t, 7, exposure, exposureFmt);
            Srv(t, 8, tr.histLen[cur]);
            Uav(t, 0, full.out);
            Uav(t, 1, tr.bounce);
            UavState(t, 2);
            Dispatch(cl, b, kComposite, t, rw, rh);
        }
        // P7 debug
        if (c.frame.w != 0)
        {
            b.Uav(full.out.res.Get());
            const Table t = NextTable();
            Srv(t, 0, *final);
            Srv(t, 1, tr.zHalf[cur]);
            Srv(t, 2, tr.nHalf[cur]);
            Srv(t, 3, tr.histLen[cur]);
            Srv(t, 4, colour, colourFmt);
            Srv(t, 5, depth, depthFmt);
            // The raw P3 trace; with two a-trous iterations the second one has overwritten it.
            Srv(t, 6, tr.trace);
            Srv(t, 7, motion, motionFmt);
            Srv(t, 8, tr.radMip); // view 10 (lit protection) reads the neighbourhood statistics like P6
            Uav(t, 0, full.out);
            Dispatch(cl, b, kDebug, t, rw, rh, c.frame.w);
        }

        // Out for the upscaler; the probe and stats to the readback ring; game inputs back.
        b.To(full.out, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        b.Raw(tr.state.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        tr.stateState = D3D12_RESOURCE_STATE_COPY_SOURCE;
        b.Flush();
        cl->CopyBufferRegion(readback.Get(), UINT64(slot) * GI_READBACK_BYTES, tr.state.Get(), 0, GI_READBACK_BYTES);
        if (queries)
        {
            cl->EndQuery(queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slot * kTimestamps + 1);
            cl->ResolveQueryData(queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slot * kTimestamps, kTimestamps,
                                 readback.Get(), kReadbackTimestampBase + UINT64(slot) * kTimestamps * sizeof(uint64_t));
        }
        for (auto& g : games)
            if (g.r && !Readable(g.s))
                b.Raw(g.r, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, g.s);
        b.Flush();

        // ---------- bookkeeping ----------
        ++records;
        historyValid = true;
        haveLast = true;
        lastQuality = s.quality;
        lastRadius = s.radius;
        lastThickness = s.thickness;
        lastRenderW = rw;
        lastRenderH = rh;
        lastTraceW = tw;
        lastTraceH = th;
        prevPreExposure = in.preExposure;
        prevJitterX = in.jitterX;
        prevJitterY = in.jitterY;
        {
            std::lock_guard<std::mutex> lock(statsMutex);
            stats.ready = true;
            stats.traceWidth = tw;
            stats.traceHeight = th;
            stats.renderWidth = rw;
            stats.renderHeight = rh;
            stats.gpuMs = gpuMs;
            stats.depthConvention = depthConvention;
            stats.depthSource = depthSource;
            stats.cameraSource = in.camera.source;
            stats.fovYDegrees = fovY * 57.2957795f;
            stats.quality = s.quality;
            stats.frames = records;
            if (reset)
                ++stats.historyResets;
            std::memcpy(stats.probe, probe, sizeof(probe));
        }
        SetStatus("", true);
        return full.out.res.Get();
    }
};

Effect::Effect(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

Effect::~Effect() = default;

std::unique_ptr<Effect> Effect::Create(ID3D12Device* device)
{
    if (!device)
        return nullptr;
    auto impl = std::make_unique<Impl>();
    impl->device = device;
    D3D12_FEATURE_DATA_D3D12_OPTIONS o {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &o, sizeof(o))))
        impl->typedUavLoads = o.TypedUAVLoadAdditionalFormats != FALSE;
    return std::unique_ptr<Effect>(new Effect(std::move(impl)));
}

ID3D12Resource* Effect::Record(ID3D12GraphicsCommandList* cmdList, const Inputs& in, const Settings& s)
{
    Impl& m = *impl_;
    if (m.failed || !cmdList)
        return nullptr;
    try
    {
        return m.RecordFrame(cmdList, in, s);
    }
    catch (const std::exception& e)
    {
        // Every throw happens in the prepare phase, before anything is recorded.
        m.Fail(e.what());
    }
    catch (...)
    {
        m.Fail("unknown exception");
    }
    return nullptr;
}

void Effect::ResetHistory()
{
    impl_->resetRequested = true;
}

Stats Effect::GetStats() const
{
    std::lock_guard<std::mutex> lock(impl_->statsMutex);
    Stats s = impl_->stats;
    s.failed = impl_->failed;
    return s;
}

bool Effect::Failed() const
{
    return impl_->failed;
}

ID3D12Device* Effect::Device() const
{
    return impl_->device.Get();
}
} // namespace AmdnrGi
