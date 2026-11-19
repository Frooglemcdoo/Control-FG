#pragma once
#include "nrd_diffuse_native_transaction.h"
namespace control_nrd {
class SpecularGuides {
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 Ptr<ID3D12Device> device;Ptr<ID3D12RootSignature> root;Ptr<ID3D12PipelineState> pipeline;
 std::array<Ptr<ID3D12Resource>,4> textures;
 std::array<D3D12_RESOURCE_STATES,4> states{};
 UINT width,height,step;
 static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("REBLUR specular guide creation failed");}
public:
 using External=D3D12Backend::External;
 struct Work {
  Ptr<ID3D12DescriptorHeap> heap;Ptr<ID3D12RootSignature> root;Ptr<ID3D12PipelineState> pipeline;
  std::array<Ptr<ID3D12Resource>,8> retained;
  bool recorded=false;
 };
 SpecularGuides(ID3D12Device* d,UINT w,UINT h,D3D12_SHADER_BYTECODE shader):device(d),width(w),height(h){
  step=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,4,0,0,4}};
  D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[0].DescriptorTable={2,ranges};
  p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={0,0,36};
  D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=2;rd.pParameters=p;Ptr<ID3DBlob> blob,error;
  Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));Check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
  D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=root.Get();ps.CS=shader;Check(d->CreateComputePipelineState(&ps,IID_PPV_ARGS(&pipeline)));
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  for(UINT i=0;i<4;++i){D3D12_RESOURCE_DESC t{};t.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;t.Width=w;t.Height=h;t.DepthOrArraySize=t.MipLevels=1;t.SampleDesc.Count=1;t.Format=(i==1||i==2)?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R16G16B16A16_FLOAT;t.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&t,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&textures[i])));}
 }
 bool Record(ID3D12GraphicsCommandList4* list,const gi_reuse::ComputeBindingSnapshot& native,const gi_reuse::GraphicsTableState& graphics,const std::array<External,4>& inputs,const void* constants,Work& work){
  if(!list||!native.Ready()||!graphics.known||work.recorded||!constants)return false;
  for(UINT i=0;i<4;++i){if(!inputs[i].resource||!(inputs[i].state&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))return false;
   auto d=inputs[i].resource->GetDesc();if(d.Width!=width||d.Height!=height||d.SampleDesc.Count!=1||d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d.MipLevels)return false;
   if((i<2&&(d.DepthOrArraySize<1||d.DepthOrArraySize>32))||(i>=2&&d.DepthOrArraySize!=1))return false;
   if((i==0&&d.Format!=DXGI_FORMAT_R16_UINT)||(i==1&&d.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT)||(i==2&&d.Format!=DXGI_FORMAT_R32_FLOAT&&d.Format!=DXGI_FORMAT_R32_TYPELESS&&d.Format!=DXGI_FORMAT_D32_FLOAT)||(i==3&&d.Format!=DXGI_FORMAT_R8G8B8A8_UNORM))return false;
  }
  if(!work.heap){D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=8;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&work.heap)));}
  auto handle=work.heap->GetCPUDescriptorHandleForHeapStart();
  for(UINT i=0;i<4;++i){auto d=inputs[i].resource->GetDesc();D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=i==2?DXGI_FORMAT_R32_FLOAT:d.Format;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   if(i<2){v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=d.DepthOrArraySize;}else{v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=1;}
   device->CreateShaderResourceView(inputs[i].resource,&v,handle);handle.ptr+=step;work.retained[i]=inputs[i].resource;
  }
  for(UINT i=0;i<4;++i){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;v.Format=textures[i]->GetDesc().Format;device->CreateUnorderedAccessView(textures[i].Get(),nullptr,&v,handle);handle.ptr+=step;work.retained[4+i]=textures[i];}
  work.root=root;work.pipeline=pipeline;
  struct Restore{ID3D12GraphicsCommandList4* list;const gi_reuse::ComputeBindingSnapshot& native;const gi_reuse::GraphicsTableState& graphics;~Restore(){RestoreNativeHeaps(list,native,graphics);native.Restore(list);}}restore{list,native,graphics};
  std::array<D3D12_RESOURCE_BARRIER,4> barriers{};
  for(UINT i=0;i<4;++i){barriers[i].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barriers[i].Transition={textures[i].Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,states[i],D3D12_RESOURCE_STATE_UNORDERED_ACCESS};}
  work.recorded=true;list->ResourceBarrier(4,barriers.data());auto* heap=work.heap.Get();list->SetDescriptorHeaps(1,&heap);list->SetComputeRootSignature(root.Get());list->SetPipelineState(pipeline.Get());list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());list->SetComputeRoot32BitConstants(1,36,constants,0);list->Dispatch((width+7)/8,(height+7)/8,1);
  for(UINT i=0;i<4;++i){barriers[i].Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;barriers[i].Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;states[i]=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}list->ResourceBarrier(4,barriers.data());return true;
 }
 ID3D12Resource* Resource(UINT i)const{return textures.at(i).Get();}
 D3D12_RESOURCE_STATES State(UINT i)const{return states.at(i);}
};
}
