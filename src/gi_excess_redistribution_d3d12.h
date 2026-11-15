#pragma once
#include "pt_glass_descriptors.h"
#include <stdexcept>

namespace gi_excess {
// Frame-local work is returned to the pool only after fence completion AND
// command-list Reset. No history, scene inventory or CPU readback is introduced.
class Pipeline {
 template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
 Ptr<ID3D12Device> device;
 Ptr<ID3D12RootSignature> root;
 Ptr<ID3D12PipelineState> classify,gather;
 static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("GI excess pipeline creation failed");}
public:
 struct Work {
  Ptr<ID3D12Resource> donors,raw;
  Ptr<ID3D12DescriptorHeap> view,recordingHeap;
  Ptr<ID3D12RootSignature> root;
  Ptr<ID3D12PipelineState> classify,gather,native;
  UINT width=0,height=0;
  void Retire(){raw.Reset();recordingHeap.Reset();native.Reset();}
 };
 Pipeline(ID3D12Device* d,ID3D12RootSignature* r,D3D12_SHADER_BYTECODE a,D3D12_SHADER_BYTECODE b):device(d),root(r){
  D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=r;ps.CS=a;Check(d->CreateComputePipelineState(&ps,IID_PPV_ARGS(&classify)));
  ps.CS=b;Check(d->CreateComputePipelineState(&ps,IID_PPV_ARGS(&gather)));
 }
 bool Matches(ID3D12RootSignature* r)const{return root.Get()==r;}
 bool Record(ID3D12GraphicsCommandList* list,ID3D12PipelineState* native,D3D12_GPU_DESCRIPTOR_HANDLE nativeTable,
             ID3D12Resource* raw,PTGlass::LightingTableShadow& shadow,UINT64 serial,
             const PTGlass::DescriptorRange& fresh,Work& work){
  if(!list||!native||!raw||work.recordingHeap)return false;
  const auto desc=raw->GetDesc();
  if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!desc.MipLevels||desc.DepthOrArraySize!=1||
     desc.SampleDesc.Count!=1||!desc.Width||!desc.Height||desc.Width>8192||desc.Height>8192||
     !(desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))return false;
  D3D12_FEATURE_DATA_FORMAT_SUPPORT support{desc.Format};
  if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support)))||
     !(support.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD)||!(support.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))return false;
  // t0 GBuffer1, t1 GBuffer2, t2 linear depth, t24 validity, u0 GI.
  constexpr UINT64 required=(7ull<<5)|(1ull<<29)|(1ull<<41);
  if(!shadow.Has(serial,required))return false;
  const UINT w=UINT(desc.Width),h=desc.Height;
  if(work.width!=w||work.height!=h){
   Work replacement;
   auto d=desc;d.MipLevels=1;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
   Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&replacement.donors)));
   D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;
   Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&replacement.view)));
   D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=d.Format;u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
   device->CreateUnorderedAccessView(replacement.donors.Get(),nullptr,&u,replacement.view->GetCPUDescriptorHandleForHeapStart());
   replacement.width=w;replacement.height=h;work=std::move(replacement);
  }
  if(!shadow.Copy(serial,43,{work.view.Get(),0,1}))return false;
  D3D12_GPU_DESCRIPTOR_HANDLE table{};
  if(!shadow.PublishChecked(serial,fresh,required|(1ull<<43),&table))return false;
  // Retain every referenced object before recording the first GPU command.
  work.raw=raw;work.root=root;work.classify=classify;work.gather=gather;work.native=native;work.recordingHeap=fresh.heap;
  D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.UAV.pResource=raw;list->ResourceBarrier(1,&barrier);
  {PTGlass::ScopedLightingBinding binding(list,native,nativeTable,classify.Get(),table);
   list->Dispatch((w+7)/8,(h+7)/8,1);
   barrier.UAV.pResource=work.donors.Get();list->ResourceBarrier(1,&barrier);
   // Finish all raw reads in Classify before any Gather writes to raw.
   barrier.UAV.pResource=raw;list->ResourceBarrier(1,&barrier);
   list->SetPipelineState(gather.Get());list->Dispatch((w+7)/8,(h+7)/8,1);
   list->ResourceBarrier(1,&barrier);
  }
  return true;
 }
};
}
