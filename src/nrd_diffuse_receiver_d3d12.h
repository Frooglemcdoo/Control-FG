#pragma once
#include "pt_glass_descriptors.h"
#include "nrd_diffuse_native_transaction.h"
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
 Ptr<ID3D12PipelineState> pipeline,privatePipeline;
 Ptr<ID3D12RootSignature> privateRoot;
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
  // Receiver shader uses t0..35, u2..4, b0/b1 and the existing s6/space1
  // sampler. It never indexes the native bindless material texture table.
  D3D12_DESCRIPTOR_RANGE ranges[3]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,5},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,2,0,43},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,0,1,0}};
  D3D12_ROOT_PARAMETER params[4]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={2,ranges};
  for(UINT i=1;i<=2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor={i-1,0};}
  params[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[3].DescriptorTable={1,&ranges[2]};
  D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=4;rd.pParameters=params;Ptr<ID3DBlob> bytes,error;
  Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
  Check(d->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&privateRoot)));
  ps.pRootSignature=privateRoot.Get();Check(d->CreateComputePipelineState(&ps,IID_PPV_ARGS(&privatePipeline)));
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
 // Rollover-only path: stage CPU descriptors into a private heap, then
 // restore both native heap bindings and every known graphics/compute table.
 bool RecordPrivate(ID3D12GraphicsCommandList4* list,const gi_reuse::ComputeBindingSnapshot& native,
                    const gi_reuse::GraphicsTableState& graphics,PTGlass::LightingTableShadow& shadow,
                    UINT64 serial,Work& work){
  if(abandoned||!list||!native.Ready()||!graphics.known||!native.heaps[1]||work.heap)return false;
  struct Extract {
   UINT64 cb0=0,cb1=0,sampler=0;
   void Constants(unsigned,unsigned,unsigned,const std::uint32_t*)noexcept{}
   void Descriptor(unsigned i,gi_reuse::RootArgumentKind kind,std::uint64_t address)noexcept{
    if(kind==gi_reuse::RootArgumentKind::CBV){if(i==2)cb0=address;if(i==3)cb1=address;}
    if(kind==gi_reuse::RootArgumentKind::Table&&i==6)sampler=address;
   }
  } input;native.arguments.Replay(input);
  if(!input.cb0||!input.cb1||!input.sampler)return false;
  const auto samplerBase=native.heaps[1]->GetGPUDescriptorHandleForHeapStart().ptr;
  const UINT samplerStep=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  const UINT samplerSize=native.heaps[1]->GetDesc().NumDescriptors;
  if(!samplerStep||input.sampler<samplerBase||(input.sampler-samplerBase)%samplerStep||samplerSize<64||
     (input.sampler-samplerBase)/samplerStep>samplerSize-64)return false;
  constexpr UINT64 required=(1ull<<5)|(1ull<<6)|(1ull<<8)|(1ull<<9)|(1ull<<17)|(1ull<<29);
  if(!shadow.Has(serial,required)||!shadow.Copy(serial,43,{views.Get(),3,3}))return false;
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=49;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  Ptr<ID3D12DescriptorHeap> heap;if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap))))return false;
  const auto cpu=shadow.Snapshot();
  device->CopyDescriptorsSimple(49,heap->GetCPUDescriptorHandleForHeapStart(),cpu.heap->GetCPUDescriptorHandleForHeapStart(),hd.Type);
  work.heap=heap;work.root=privateRoot;work.pipeline=privatePipeline;work.native=native.pipeline;work.textures=texture;
  struct Restore {ID3D12GraphicsCommandList4* list;const gi_reuse::ComputeBindingSnapshot& native;const gi_reuse::GraphicsTableState& graphics;
   ~Restore(){RestoreNativeHeaps(list,native,graphics);native.Restore(list);}
  } restore{list,native,graphics};
  ID3D12DescriptorHeap* heaps[]={heap.Get(),native.heaps[1].Get()};list->SetDescriptorHeaps(2,heaps);
  list->SetComputeRootSignature(privateRoot.Get());list->SetPipelineState(privatePipeline.Get());
  list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());
  list->SetComputeRootConstantBufferView(1,input.cb0);list->SetComputeRootConstantBufferView(2,input.cb1);list->SetComputeRootDescriptorTable(3,{input.sampler});
  Transition(list,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);list->Dispatch((width+7)/8,(height+15)/16,1);
  Transition(list,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);return true;
 }
 ID3D12Resource* Resource(unsigned i)const{return texture.at(i).Get();}
 D3D12_RESOURCE_STATES State(unsigned i)const{return states.at(i);}
 void AbandonAfterReset()noexcept{abandoned=true;}
};
}
