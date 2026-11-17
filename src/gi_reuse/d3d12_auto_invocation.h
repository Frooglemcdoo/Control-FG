#pragma once
#include "d3d12_auto_pipeline_bundle.h"
#include "d3d12_auto_raygen.h"
namespace gi_reuse {
struct GIAutoPublishedTarget {
 Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
 UINT frame=0,epoch=0;
};
class D3D12AutoInvocation {
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 static HRESULT Buffer(ID3D12Device* device,UINT64 bytes,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state,bool uav,Ptr<ID3D12Resource>& out)noexcept {
  D3D12_HEAP_PROPERTIES hp{};hp.Type=heap;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;
  rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=uav?D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:D3D12_RESOURCE_FLAG_NONE;
  auto hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&out));
  if(SUCCEEDED(hr)&&!RetainCreatedStorage(out.Get()))return E_FAIL;return hr;
 }
public:
 Ptr<D3D12AutoPipelineBundle> pipelines;
 D3D12AutoTargetBinding target;
 D3D12AutoRaygenRecord record;
 Ptr<ID3D12Resource> training,cells,audit;
 GIAutoPublishedTarget output;
 UINT width=0,height=0,step=0,capacity=0;
 bool shaded=false;
 HRESULT Initialize(ID3D12Device* device,ID3D12StateObject* state,D3D12AutoPipelineBundle* bundle,
                    const GIAutoPublishedTarget& prior,UINT w,UINT h,UINT frame,UINT epoch,bool captureTargets=false)noexcept {
  if(!device||!state||!bundle||!w||!h||w>16384||h>16384||!frame||!epoch)return E_INVALIDARG;
  width=w;height=h;step=8;while(UINT64((w+step-1)/step)*((h+step-1)/step)>65535&&step<256)step*=2;
  capacity=((w+step-1)/step)*((h+step-1)/step);if(capacity>65535)return E_INVALIDARG;
  GIAutoPublishedTarget selected=prior;HRESULT hr=S_OK;
  if(!selected.buffer||selected.epoch!=epoch||selected.frame>=frame){
   selected={};selected.frame=frame;selected.epoch=epoch;
   hr=Buffer(device,sizeof(GIAutoTargets),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,selected.buffer);if(FAILED(hr))return hr;
   void* mapped=nullptr;const D3D12_RANGE empty{};hr=selected.buffer->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
   if(!mapped){selected.buffer->Unmap(0,&empty);return E_UNEXPECTED;}
   std::memset(mapped,0,sizeof(GIAutoTargets));const D3D12_RANGE written{0,sizeof(GIAutoTargets)};selected.buffer->Unmap(0,&written);
  }
  if(!target.Initialize(selected.buffer.Get(),{epoch,selected.frame,10,.4f}))return E_INVALIDARG;
  hr=record.Initialize(device,state,target);if(FAILED(hr))return hr;
  hr=Buffer(device,UINT64(capacity)*sizeof(GIAutoTraining),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true,training);if(FAILED(hr))return hr;
  hr=Buffer(device,4096*sizeof(GIAutoCell),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true,cells);if(FAILED(hr))return hr;
  hr=Buffer(device,sizeof(GIAutoTargets),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true,output.buffer);if(FAILED(hr))return hr;
  if(captureTargets){hr=Buffer(device,sizeof(GIAutoTargets),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,false,audit);if(FAILED(hr))return hr;}
  output.frame=frame;output.epoch=epoch;pipelines=bundle;return S_OK;
 }
 UINT64 Bytes()const noexcept{return UINT64(capacity)*sizeof(GIAutoTraining)+4096*sizeof(GIAutoCell)+sizeof(GIAutoTargets)+record.RetainedBytes()+target.RetainedBytes()+(audit?sizeof(GIAutoTargets):0);}
 bool Shade(ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& bindings)noexcept {
  if(shaded||!pipelines||!pipelines->evaluator.Record(list,bindings,target,training.Get(),width,height,step,capacity))return false;
  D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition={training.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&barrier);
  shaded=pipelines->builder.Record(list,training.Get(),capacity,cells.Get(),output.buffer.Get(),4096,output.frame,output.epoch,.5f,512,1,bindings);
  if(shaded&&audit){
   barrier.Transition={output.buffer.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
   list->CopyBufferRegion(audit.Get(),0,output.buffer.Get(),0,sizeof(GIAutoTargets));
   std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);
  }
  return shaded;
 }
};
}
