#pragma once
#include "d3d12_native_recorded_heaps.h"
#include <d3d12.h>
#include <intrin.h>
namespace gi_reuse {
// Borrowed exact-build context from the ORIGINAL recording thread. No global
// renderer feature initialization dependency and no submission-thread lookup.
// Host must retain queue/fence objects and validate bound heaps and actual
// recording generation before admission. This is not a scene/content lease.
struct NativeGIRecording {
 ID3D12CommandList* list=nullptr;
 ID3D12CommandQueue* queue=nullptr;
 ID3D12Device* device=nullptr;
 const void* sbtContext=nullptr;
 GIRecordedHeapIdentity::Sample heaps{};
};
inline bool ReadNativeGIRecording(const unsigned char* d3d,const unsigned char* tls,
                                 ID3D12CommandList* expectedList,UINT64 frame,
                                 NativeGIRecording* output)noexcept {
 if(!output)return false;*output={};
 if(!d3d||!tls||!expectedList||!frame)return false;
 __try {
  if(*reinterpret_cast<const UINT64*>(d3d+0x136c68)!=frame)return false;
  const auto* state=*reinterpret_cast<const unsigned char*const*>(tls+0x20);
  const auto* context=*reinterpret_cast<const unsigned char*const*>(tls+8);
  const auto* queue=*reinterpret_cast<const unsigned char*const*>(tls+0x18);
  if(!state)state=*reinterpret_cast<const unsigned char*const*>(d3d+0x111c10);
  if(!context)context=*reinterpret_cast<const unsigned char*const*>(d3d+0x111c18);
  if(!queue)queue=*reinterpret_cast<const unsigned char*const*>(d3d+0x111c38);
  const auto* device=*reinterpret_cast<const unsigned char*const*>(d3d+0x136d28);
  if(!state||!context||!queue||!device)return false;
  const auto* graphics=*reinterpret_cast<ID3D12CommandList*const*>(context+0x18);
  const auto* rays=*reinterpret_cast<ID3D12CommandList*const*>(context+0x20);
  if(expectedList!=graphics&&expectedList!=rays)return false;
  NativeGIRecording next;next.list=expectedList;
  next.queue=*reinterpret_cast<ID3D12CommandQueue*const*>(queue+0x10);
  next.device=*reinterpret_cast<ID3D12Device*const*>(device+0x78);
  next.sbtContext=*reinterpret_cast<const void*const*>(device+0x220);
  const auto* fence=*reinterpret_cast<const unsigned char*const*>(device+0x88);
  if(!next.queue||!next.device||!next.sbtContext||!fence)return false;
  auto& h=next.heaps;h.frame=frame;h.deviceFenceRecord=reinterpret_cast<UINT64>(fence);
  std::memcpy(&h.deviceEpoch,fence,8);std::memcpy(&h.deviceFenceObject,fence+8,8);
  if(!h.deviceEpoch||!h.deviceFenceObject)return false;
  for(unsigned i=0;i<2;++i){
   const auto* record=*reinterpret_cast<const unsigned char*const*>(state+0x50+i*8);
   if(!record)return false;auto& heap=h.heaps[i];heap.record=reinterpret_cast<UINT64>(record);
   std::memcpy(&heap.object,record,8);std::memcpy(&heap.cursor,record+0x18,4);
   std::memcpy(&heap.epoch,record+0x20,8);std::memcpy(&heap.generation,record+0x30,8);
   if(!heap.object||heap.epoch!=h.deviceEpoch)return false;
  }
  if(*reinterpret_cast<const UINT64*>(d3d+0x136c68)!=frame)return false;
  *output=next;return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool ReadCurrentNativeGIRecording(const unsigned char* d3d,ID3D12CommandList* expectedList,
                                        UINT64 frame,NativeGIRecording* output)noexcept {
 if(!output)return false;*output={};if(!d3d)return false;
 __try {
  const auto index=*reinterpret_cast<const DWORD*>(d3d+0x1115fc);
  const auto* slots=reinterpret_cast<const unsigned char*const*>(__readgsqword(0x58));
  if(!slots)return false;
  return ReadNativeGIRecording(d3d,slots[index],expectedList,frame,output);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
