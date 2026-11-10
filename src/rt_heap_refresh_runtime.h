#pragma once
#include "rt_heap_refresh.h"
#include "rt_descriptor_lease_bytes.h"
#include "rt_allocator_probe_patch.h"
namespace RTHeapRefresh {
using rt_owned_assembly::Read;
inline unsigned char* base=nullptr;
inline SRWLOCK mutex=SRWLOCK_INIT;
inline rt_heap_refresh::Tracker tracker;
inline bool ready=false;
inline UINT64 calls=0,fullCopies=0,partialCopies=0,skips=0,nativeDescriptors=0,copiedDescriptors=0,driverCalls=0,fallbacks=0;
inline void Publish(void* manager,std::uint64_t index,std::uint64_t source){
 const DWORD error=GetLastError();AcquireSRWLockExclusive(&mutex);
 const auto version=Read<std::uint64_t>(manager,8);
 // Preserve the complete native leaf operation, including same-handle publications.
 std::memcpy(static_cast<unsigned char*>(manager)+0x1f8+index*8,&source,8);
 const auto next=version+1;std::memcpy(static_cast<unsigned char*>(manager)+8,&next,8);
 tracker.Publish(reinterpret_cast<std::uintptr_t>(manager),version,index<rt_heap_refresh::Capacity?static_cast<std::uint32_t>(index):rt_heap_refresh::Capacity);
 ReleaseSRWLockExclusive(&mutex);SetLastError(error);
}
inline void Copy(ID3D12Device* device,UINT destinations,const D3D12_CPU_DESCRIPTOR_HANDLE* dest,const UINT* destSizes,
 UINT sources,const D3D12_CPU_DESCRIPTOR_HANDLE* src,const UINT* sourceSizes,D3D12_DESCRIPTOR_HEAP_TYPE type){
 const DWORD error=GetLastError();AcquireSRWLockExclusive(&mutex);
 auto* manager=Read<void*>(base,0x111bb8);auto* heap=manager?Read<void*>(manager,0x60):nullptr;
 const auto count=manager?Read<std::uint32_t>(manager,0x10):0;
 const auto oldCount=heap?Read<std::uint32_t>(heap,0x38):0;
 const auto version=manager?Read<std::uint64_t>(manager,8):0;
 const auto heapVersion=heap?Read<std::uint64_t>(heap,0x30):0;
 const auto step=Read<std::uint32_t>(base,0x111bc0);
 const rt_heap_refresh::Key key{reinterpret_cast<std::uintptr_t>(heap),heap?reinterpret_cast<std::uintptr_t>(Read<void*>(heap)):0,heap?Read<std::uintptr_t>(heap,8):0};
 ++calls;
 const bool valid=ready&&manager&&heap&&destinations==1&&dest&&destSizes&&src&&!sourceSizes&&type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV&&
  sources==std::max(count,oldCount)&&sources<=rt_heap_refresh::Capacity&&*destSizes==sources&&dest->ptr==key.cpu&&
  reinterpret_cast<const void*>(src)==static_cast<unsigned char*>(manager)+0x1f8&&step;
 if(!valid){
  ++fallbacks;device->CopyDescriptors(destinations,dest,destSizes,sources,src,sourceSizes,type);
  // Unknown copy shape invalidates history, including in-place changes.
  tracker.Observe(0,0);
 }else{
  auto plan=tracker.Prepare(reinterpret_cast<std::uintptr_t>(manager),key,version,heapVersion,count,oldCount);
  nativeDescriptors+=sources;copiedDescriptors+=plan.copied;driverCalls+=plan.rangeCount;
  if(plan.full)++fullCopies;else if(!plan.rangeCount)++skips;else ++partialCopies;
  for(std::uint32_t i=0;i<plan.rangeCount;++i){
   const auto& range=plan.ranges[i];const D3D12_CPU_DESCRIPTOR_HANDLE destination{dest->ptr+std::size_t(range.start)*step};
   device->CopyDescriptors(1,&destination,&range.count,range.count,src+range.start,nullptr,type);
  }
  tracker.Commit(key,version,count);
 }
 ReleaseSRWLockExclusive(&mutex);SetLastError(error);
}
inline void Report(){
 UINT64 n,full,partial,skip,requested,copied,drivers,fallback,updates,resets;
 const DWORD error=GetLastError();AcquireSRWLockShared(&mutex);
 n=calls;full=fullCopies;partial=partialCopies;skip=skips;requested=nativeDescriptors;copied=copiedDescriptors;drivers=driverCalls;fallback=fallbacks;updates=tracker.publications;resets=tracker.resets;
 ReleaseSRWLockShared(&mutex);
 Log("RT_HEAP_R410 calls=%llu full=%llu partial=%llu skipped=%llu native_descriptors=%llu copied_descriptors=%llu driver_calls=%llu fallbacks=%llu publications=%llu resets=%llu page_size=256 native_retirement=1",n,full,partial,skip,requested,copied,drivers,fallback,updates,resets);SetLastError(error);
}
struct Patch {unsigned char* site=nullptr;unsigned size=0;unsigned char before[13]{},after[13]{};};
inline Patch patches[2];
inline bool Prepare(unsigned char* site,unsigned size,void* hook,bool jump,Patch& patch){
 auto* relay=static_cast<unsigned char*>(AllocateExecutableRelayNear(site));if(!relay)return false;
 unsigned char call[6]{};if(!rt_allocator_probe::Call6(reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(relay),call))return false;
 const unsigned char stub[]={0xff,0x25,0,0,0,0};std::memcpy(relay,stub,6);std::memcpy(relay+6,&hook,8);DWORD old=0;
 if(!VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),relay,14))return false;
 patch.site=site;patch.size=size;std::memcpy(patch.before,site,size);std::memset(patch.after,0x90,size);std::memcpy(patch.after,call,5);if(jump)patch.after[0]=0xe9;return true;
}
inline bool Exchange(Patch& patch,bool install){
 const auto* before=install?patch.before:patch.after;const auto* after=install?patch.after:patch.before;
 if(!patch.site||std::memcmp(patch.site,before,patch.size))return false;DWORD old=0;if(!VirtualProtect(patch.site,patch.size,PAGE_EXECUTE_READWRITE,&old))return false;
 std::memcpy(patch.site,after,patch.size);const bool flush=FlushInstructionCache(GetCurrentProcess(),patch.site,patch.size)!=FALSE;DWORD ignored=0;
 return VirtualProtect(patch.site,patch.size,old,&ignored)!=FALSE&&flush;
}
inline bool Install(HMODULE module){
 if(!module||module!=verifiedD3d||!HashMatches(module,kD3dHash))return false;base=reinterpret_cast<unsigned char*>(module);
 const unsigned char setter[]={0x4c,0x89,0x84,0xd1,0xf8,0x01,0,0,0x48,0xff,0x41,8,0xc3};
 if(std::memcmp(base+0x1aa00,rt_lease_bytes::Allocate,sizeof(rt_lease_bytes::Allocate))||std::memcmp(base+0x1a9c0,setter,sizeof(setter)))return false;
 if(!Prepare(base+0x1a9c0,13,reinterpret_cast<void*>(&Publish),true,patches[0])||!Prepare(base+0x1ac12,7,reinterpret_cast<void*>(&Copy),false,patches[1]))return false;
 if(Exchange(patches[0],true)&&Exchange(patches[1],true)){ready=true;return true;}
 for(auto& patch:patches)if(patch.site&&!std::memcmp(patch.site,patch.after,patch.size))Exchange(patch,false);return false;
}
}
