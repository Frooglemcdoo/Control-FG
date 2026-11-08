#pragma once
#include "pt_glass_descriptors.h"
#include <array>
#include <stdexcept>

namespace control_nrd {
// Frame-local guide export at the exact GI6 bind. It keeps the native root,
// sampler/global tables and heap, and borrows only observed CPU descriptors.
// The owner must serialize reuse after the preceding consumer was submitted.
// This component does not enable scene inventory or install observation hooks.
class NativeReceiver {
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 Ptr<ID3D12Device> device;
 Ptr<ID3D12RootSignature> root;
 Ptr<ID3D12PipelineState> pipeline;
 Ptr<ID3D12DescriptorHeap> views;
 std::array<Ptr<ID3D12Resource>,3> texture;
 std::array<D3D12_RESOURCE_STATES,3> states{};
 UINT width,height,step;
 bool abandoned=false;
 static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Native GI receiver export creation failed");}
 D3D12_CPU_DESCRIPTOR_HANDLE Handle(UINT i)const {auto h=views->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*step;return h;}
 void Transition(ID3D12GraphicsCommandList* list,D3D12_RESOURCE_STATES next){
  std::array<D3D12_RESOURCE_BARRIER,3> barriers{};UINT count=0;
  for(UINT i=0;i<3;++i){if(states[i]==next)continue;auto& b=barriers[count++];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   b.Transition={texture[i].Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,states[i],next};states[i]=next;}
  if(count)list->ResourceBarrier(count,barriers.data());
 }
public:
 struct Work {
  Ptr<ID3D12DescriptorHeap> heap;
  Ptr<ID3D12RootSignature> root;
  Ptr<ID3D12PipelineState> pipeline,native;
  std::array<Ptr<ID3D12Resource>,3> textures;
 };
 NativeReceiver(ID3D12Device* d,ID3D12RootSignature* nativeRoot,D3D12_SHADER_BYTECODE shader,UINT w,UINT h):device(d),root(nativeRoot),width(w),height(h){
  if(!d||!nativeRoot||!shader.pShaderBytecode||!w||!h||w>8192||h>8192)throw std::runtime_error("Invalid native receiver contract");
  step=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=nativeRoot;ps.CS=shader;Check(d->CreateComputePipelineState(&ps,IID_PPV_ARGS(&pipeline)));
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=6;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&views)));
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  for(UINT i=0;i<3;++i){D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=w;desc.Height=h;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;
   desc.Format=i<2?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R32_FLOAT;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&texture[i])));
   D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=desc.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Texture2D.MipLevels=1;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d->CreateShaderResourceView(texture[i].Get(),&srv,Handle(i));
   D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=desc.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;d->CreateUnorderedAccessView(texture[i].Get(),nullptr,&uav,Handle(i+3));
  }
 }
 // Must be called immediately after the matching GI6 dispatch, before
 // the binding changes. Work stays alive until fence AND Reset.
 bool Record(ID3D12GraphicsCommandList* list,ID3D12PipelineState* native,
             D3D12_GPU_DESCRIPTOR_HANDLE nativeTable,PTGlass::LightingTableShadow& shadow,
             UINT64 serial,const PTGlass::DescriptorRange& fresh,Work& work){
  if(abandoned||!list||!native||!nativeTable.ptr||work.heap)return false;
  // Reflection of nrd_diffuse_receiver: t0,t1,t3,t4,t12,t24.
  constexpr UINT64 required=(1ull<<5)|(1ull<<6)|(1ull<<8)|(1ull<<9)|(1ull<<17)|(1ull<<29);
  if(!shadow.Has(serial,required)||!shadow.Copy(serial,43,{views.Get(),3,3}))return false;
  D3D12_GPU_DESCRIPTOR_HANDLE table{};
  if(!shadow.PublishChecked(serial,fresh,required|(7ull<<43),&table))return false;
  work.heap=fresh.heap;work.root=root;work.pipeline=pipeline;work.native=native;work.textures=texture;
  Transition(list,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  {PTGlass::ScopedLightingBinding binding(list,native,nativeTable,pipeline.Get(),table);list->Dispatch((width+7)/8,(height+15)/16,1);}
  Transition(list,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);return true;
 }
 ID3D12Resource* Resource(unsigned i)const{return texture.at(i).Get();}
 D3D12_RESOURCE_STATES State(unsigned i)const{return states.at(i);}
 void AbandonAfterReset()noexcept{abandoned=true;}
};
}
