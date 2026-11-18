#pragma once
#include "d3d12_auto_pipeline_bundle.h"
#include "d3d12_auto_raygen.h"
#include "auto_guide_session.h"
#include <memory>
#include "d3d12_auto_timing.h"
namespace gi_reuse {
struct GIAutoPublishedTarget {
 Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
 UINT frame=0,epoch=0;
};
struct D3D12AutoScratch {
 Microsoft::WRL::ComPtr<ID3D12Device> device;
 Microsoft::WRL::ComPtr<ID3D12Resource> training,cells;
 D3D12AutoRaygenRecord record;
 UINT width=0,height=0;
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
 D3D12AutoTiming timing;
 Ptr<ID3D12Resource> training,cells,audit,pixelAudit;
 UINT64 pixelBytes=0,auditOrdinal=0,captureRequest=0;
 UINT roiX=0,roiY=0;
 GIAutoPublishedTarget output;
 UINT width=0,height=0,step=0,capacity=0;
 bool shaded=false,reusedScratch=false;
 Ptr<ID3D12Device> ownerDevice;
 std::unique_ptr<D3D12AutoScratch> ReleaseRetiredScratch()noexcept {
  if(!shaded||pixelBytes)return {};
  auto s=std::unique_ptr<D3D12AutoScratch>(new(std::nothrow) D3D12AutoScratch);if(!s)return {};
  s->device=ownerDevice;s->width=width;s->height=height;s->training=std::move(training);s->cells=std::move(cells);s->record=std::move(record);return s;
 }
 HRESULT Initialize(ID3D12Device* device,ID3D12StateObject* state,D3D12AutoPipelineBundle* bundle,
                    const GIAutoPublishedTarget& prior,UINT w,UINT h,UINT frame,UINT epoch,bool captureTargets=false,bool capturePixels=false,UINT rayCount=10,std::unique_ptr<D3D12AutoScratch> recycled={})noexcept {
  if(!device||!state||!bundle||!w||!h||w>16384||h>16384||!frame||!epoch)return E_INVALIDARG;
  ownerDevice=device;width=w;height=h;step=8;while(UINT64((w+step-1)/step)*((h+step-1)/step)>65535&&step<256)step*=2;
  capacity=((w+step-1)/step)*((h+step-1)/step);if(capacity>65535)return E_INVALIDARG;
  GIAutoPublishedTarget selected=prior;HRESULT hr=S_OK;
  if(!GIAutoPriorCompatible(selected.buffer.Get()!=nullptr,selected.epoch,selected.frame,epoch,frame)){
   selected={};selected.frame=frame;selected.epoch=epoch;
   hr=Buffer(device,sizeof(GIAutoTargets),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,selected.buffer);if(FAILED(hr))return hr;
   void* mapped=nullptr;const D3D12_RANGE empty{};hr=selected.buffer->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
   if(!mapped){selected.buffer->Unmap(0,&empty);return E_UNEXPECTED;}
   std::memset(mapped,0,sizeof(GIAutoTargets));const D3D12_RANGE written{0,sizeof(GIAutoTargets)};selected.buffer->Unmap(0,&written);
  }
  if(!target.Initialize(selected.buffer.Get(),{epoch,selected.frame,rayCount,rayCount==4?.5f:.4f,frame}))return E_INVALIDARG;
  if(recycled&&!capturePixels&&recycled->device.Get()==device&&recycled->width==w&&recycled->height==h){
   training=std::move(recycled->training);cells=std::move(recycled->cells);record=std::move(recycled->record);reusedScratch=true;
  }
  hr=reusedScratch?record.RewriteRetired(device,state,target):record.Initialize(device,state,target);if(FAILED(hr))return hr;
  pixelBytes=capturePixels?UINT64(GIAuditWidth(w))*GIAuditHeight(h)*sizeof(GIAutoPixelAudit):0;
  if(!training)hr=Buffer(device,UINT64(capacity)*sizeof(GIAutoTraining)+pixelBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true,training);if(FAILED(hr))return hr;
  if(!cells)hr=Buffer(device,4096*sizeof(GIAutoCell),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true,cells);if(FAILED(hr))return hr;
  hr=Buffer(device,sizeof(GIAutoTargets),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true,output.buffer);if(FAILED(hr))return hr;
  if(captureTargets){hr=Buffer(device,2*sizeof(GIAutoTargets),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,false,audit);if(FAILED(hr))return hr;}
  if(pixelBytes){hr=Buffer(device,pixelBytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,false,pixelAudit);if(FAILED(hr))return hr;}
  output.frame=frame;output.epoch=epoch;pipelines=bundle;return S_OK;
 }
 UINT64 Bytes()const noexcept{return UINT64(capacity)*sizeof(GIAutoTraining)+4096*sizeof(GIAutoCell)+sizeof(GIAutoTargets)+record.RetainedBytes()+target.RetainedBytes()+(audit?2*sizeof(GIAutoTargets):0)+2*pixelBytes;}
 bool Shade(ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& bindings)noexcept {
  if(shaded||!pipelines)return false;
  if(reusedScratch){
   D3D12_RESOURCE_BARRIER reset{};reset.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   reset.Transition={training.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS};list->ResourceBarrier(1,&reset);
  }
  timing.Mark(list,2);
  if(!pipelines->evaluator.Record(list,bindings,target,training.Get(),width,height,step,capacity,pixelBytes!=0,roiX,roiY))return false;
  timing.Mark(list,3);
  D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition={training.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&barrier);
  timing.Mark(list,4);
  shaded=pipelines->builder.Record(list,training.Get(),capacity,cells.Get(),output.buffer.Get(),4096,output.frame,output.epoch,.5f,512,1,bindings);
  timing.Mark(list,5);timing.Resolve(list,2,4);
  if(shaded&&audit){
   barrier.Transition={output.buffer.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
   list->CopyBufferRegion(audit.Get(),0,output.buffer.Get(),0,sizeof(GIAutoTargets));
   std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);
  }
  if(shaded&&audit){
   D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
   if(FAILED(target.Resource()->GetHeapProperties(&heap,&flags)))return false;
   const bool upload=heap.Type==D3D12_HEAP_TYPE_UPLOAD;
   barrier.Transition={target.Resource(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};
   if(!upload)list->ResourceBarrier(1,&barrier);
   list->CopyBufferRegion(audit.Get(),sizeof(GIAutoTargets),target.Resource(),0,sizeof(GIAutoTargets));
   if(!upload){std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);}
  }
  if(shaded&&pixelAudit){
   barrier.Transition={training.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
   list->CopyBufferRegion(pixelAudit.Get(),0,training.Get(),UINT64(capacity)*sizeof(GIAutoTraining),pixelBytes);
   std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);
  }
  return shaded;
 }
};
}
