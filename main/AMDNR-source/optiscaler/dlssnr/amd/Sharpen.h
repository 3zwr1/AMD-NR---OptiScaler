// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include "SystemCompiler.h"
#include <wrl/client.h>
#include <stdexcept>
#include <string>
namespace AmdPreSr {
// Clamped unsharp-mask sharpening applied to the denoised colour before Super
// Resolution. The neural denoise softens fine detail; a light sharpen restores
// crispness that reads closer to a hardware DLSS result. The sharpened value is
// clamped to the pixel's own neighbourhood, so it can never leave the local
// colour range - no ringing, no NaN, no colour corruption, and it is safe in
// any colour space (SDR or linear HDR), unlike a [0,1]-assuming CAS. Strength 0
// skips the pass entirely. Self-contained, run on the render queue.
inline constexpr char SharpenShader[] = R"(
Texture2D<float4> src : register(t0);
RWTexture2D<float4> dst : register(u0);
cbuffer P : register(b0) { uint w; uint h; float sharpness; uint pad; }
float3 tap(int2 p){ return src.Load(int3(clamp(p, int2(0,0), int2(int(w)-1,int(h)-1)),0)).rgb; }
[numthreads(8,8,1)] void main(uint3 tid:SV_DispatchThreadID) {
 if (tid.x>=w || tid.y>=h) return;
 int2 p = int2(tid.xy);
 float4 centre = src.Load(int3(p,0));
 float3 e = centre.rgb;
 if (sharpness <= 0.0 || !all(isfinite(e))) { dst[p]=centre; return; }
 float3 b=tap(p+int2(0,-1)), d=tap(p+int2(-1,0)), f=tap(p+int2(1,0)), hh=tap(p+int2(0,1));
 // Also sample the diagonals so the detail estimate is a full 3x3, giving a
 // stronger, more visible sharpen.
 float3 a=tap(p+int2(-1,-1)), cc=tap(p+int2(1,-1)), gg=tap(p+int2(-1,1)), ii=tap(p+int2(1,1));
 float3 blur = (b + d + f + hh) * 0.125 + (a + cc + gg + ii) * 0.0625 + e * 0.25;
 float amount = sharpness * 2.5;               // visible gain on the high-frequency detail
 float3 sharp = e + (e - blur) * amount;
 // Bound the result to the local neighbourhood plus a small overshoot, so it is
 // clearly sharp but can never explode into a wrong/corrupted colour (HDR-safe).
 float3 lo = min(min(min(a,b),min(cc,d)), min(min(f,gg),min(hh,min(ii,e))));
 float3 hi = max(max(max(a,b),max(cc,d)), max(max(f,gg),max(hh,max(ii,e))));
 float3 rng = hi - lo;
 float3 outc = clamp(sharp, lo - rng*0.25, hi + rng*0.25);
 dst[p] = float4(outc, centre.a);
}
)";
class Sharpen {
 using Res = Microsoft::WRL::ComPtr<ID3D12Resource>;
 static constexpr UINT kRegions = 5;   // one region per in-flight NR frame
 static constexpr UINT kPerRegion = 2; // t0 src, u0 out
 Microsoft::WRL::ComPtr<ID3D12Device> device;
 Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
 Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
 Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
 Res output;
 UINT width = 0, height = 0, region = 0, stride = 0;
 static void Check(HRESULT hr) { if (FAILED(hr)) throw std::runtime_error("AMD sharpen D3D12 error " + std::to_string((UINT)hr)); }
 static void Barrier(ID3D12GraphicsCommandList* c, ID3D12Resource* r, D3D12_RESOURCE_STATES a, D3D12_RESOURCE_STATES b) {
  if (a == b || !r) return;
  D3D12_RESOURCE_BARRIER v {}; v.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  v.Transition = { r, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, a, b }; c->ResourceBarrier(1, &v);
 }
 public:
 Sharpen(ID3D12Device* d) : device(d) {
  D3D12_DESCRIPTOR_RANGE ranges[2] = { { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0 },
                                       { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 1 } };
  D3D12_ROOT_PARAMETER params[2] {};
  params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; params[0].DescriptorTable = { 2, ranges };
  params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; params[1].Constants = { 0, 0, 4 };
  D3D12_ROOT_SIGNATURE_DESC rd {}; rd.NumParameters = 2; rd.pParameters = params;
  Microsoft::WRL::ComPtr<ID3DBlob> b, e;
  Check(D3D12SerializeRootSignature(&rd, D3D_ROOT_SIGNATURE_VERSION_1, &b, &e));
  Check(d->CreateRootSignature(0, b->GetBufferPointer(), b->GetBufferSize(), IID_PPV_ARGS(&root)));
  Check(DlssNr::SysCompiler::Compile(SharpenShader, sizeof(SharpenShader), "AMD CAS", nullptr, nullptr, "main", "cs_5_0", 0, 0, &b, &e));
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd {}; pd.pRootSignature = root.Get(); pd.CS = { b->GetBufferPointer(), b->GetBufferSize() };
  Check(d->CreateComputePipelineState(&pd, IID_PPV_ARGS(&pipeline)));
  D3D12_DESCRIPTOR_HEAP_DESC hd {}; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.NumDescriptors = kRegions * kPerRegion;
  hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  Check(d->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&heap)));
  stride = d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 }
 // Returns the sharpened buffer, or `colour` unchanged when inert. `colourState` restored.
 ID3D12Resource* Run(ID3D12GraphicsCommandList* c, ID3D12Resource* colour, D3D12_RESOURCE_STATES colourState,
                     UINT w, UINT h, float sharpness) {
  if (!c || !colour || !w || !h || sharpness <= 0.f) return colour;
  if (!output || width != w || height != h) {
   D3D12_HEAP_PROPERTIES hp {}; hp.Type = D3D12_HEAP_TYPE_DEFAULT;
   D3D12_RESOURCE_DESC rd {}; rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; rd.Width = w; rd.Height = h;
   rd.DepthOrArraySize = 1; rd.MipLevels = 1; rd.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; rd.SampleDesc.Count = 1;
   rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; output.Reset();
   Check(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&output)));
   width = w; height = h;
  }
  const UINT base = region * kPerRegion;
  region = (region + 1) % kRegions;
  auto cpu = heap->GetCPUDescriptorHandleForHeapStart();
  cpu.ptr += static_cast<SIZE_T>(base) * stride;
  D3D12_SHADER_RESOURCE_VIEW_DESC s {}; s.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; s.Texture2D.MipLevels = 1;
  device->CreateShaderResourceView(colour, &s, cpu); cpu.ptr += stride;
  D3D12_UNORDERED_ACCESS_VIEW_DESC u {}; u.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; u.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(output.Get(), nullptr, &u, cpu);
  Barrier(c, colour, colourState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Barrier(c, output.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  auto hh = heap.Get(); c->SetDescriptorHeaps(1, &hh);
  c->SetComputeRootSignature(root.Get()); c->SetPipelineState(pipeline.Get());
  auto gpu = heap->GetGPUDescriptorHandleForHeapStart(); gpu.ptr += static_cast<UINT64>(base) * stride;
  c->SetComputeRootDescriptorTable(0, gpu);
  struct { UINT w, h; float sharpness; UINT pad; } cb { w, h, sharpness, 0 };
  c->SetComputeRoot32BitConstants(1, 4, &cb, 0);
  c->Dispatch((w + 7) / 8, (h + 7) / 8, 1);
  Barrier(c, output.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Barrier(c, colour, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, colourState);
  return output.Get();
 }
};
}
