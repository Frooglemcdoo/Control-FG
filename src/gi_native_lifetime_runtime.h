#pragma once
#include "gi_reuse/d3d12_submission_metadata.h"
#include "gi_reuse/native_as_retirement.h"
namespace GINativeLifetimeRuntime {
using Release=void(*)(void*);
static Release originalRelease=nullptr;
static std::atomic<void(*)(void*) noexcept> liveRetire{nullptr};
static RRAlbedoCallPatch calls[16]{};
static std::atomic<UINT64> observed{0},retired{0},untracked{0},stale{0},invalid{0};
static void ReleaseHook(void* wrapper) {
 const DWORD before=GetLastError();if(const auto retire=liveRetire.load())retire(wrapper);SetLastError(before);
 const auto result=gi_reuse::ForwardNativeASRelease(gi_reuse::runtime::asOwners,wrapper,originalRelease);
 const DWORD error=GetLastError();
 switch(result){
 case gi_reuse::NativeASRetirement::Retired:++retired;break;
 case gi_reuse::NativeASRetirement::Untracked:++untracked;break;
 case gi_reuse::NativeASRetirement::Stale:++stale;break;
 case gi_reuse::NativeASRetirement::InvalidWrapper:++invalid;break;
 }
 const auto n=++observed;
 if(n<=8||n%256==0){UINT64 count=0,bytes=0,rejected=0;gi_reuse::runtime::asOwners.Stats(count,bytes,rejected);
  Log("GI_AS_RETIRE_R306 calls=%llu retired=%llu untracked=%llu stale=%llu invalid=%llu owners=%llu bytes=%llu rejected=%llu native_deferred_release_preserved=1 extra_gpu_work=0",n,retired.load(),untracked.load(),stale.load(),invalid.load(),count,bytes,rejected);
 }
 SetLastError(error);
}
static bool Install(HMODULE d3d) noexcept {
 if(!gi_reuse::runtime::Enabled()||!HashMatches(d3d,kD3dHash))return false;
 auto* b=reinterpret_cast<unsigned char*>(d3d);originalRelease=reinterpret_cast<Release>(b+0x25f00);
 unsigned i=0;bool ready=true;
 for(unsigned rva:{0x17c25u,0x17c67u,0x17d6eu,0x17f0bu,0x25200u,0x25268u,0x252d8u,0x25350u,0x253b8u,0x25430u,0x254a0u,0x25508u,0x2565du,0x28233u,0x2ae20u,0x35752u}){
  if(!RRAlbedoPrepareCallPatch(b+rva,reinterpret_cast<void*>(originalRelease),reinterpret_cast<void*>(&ReleaseHook),&calls[i++])){ready=false;break;}
 }
 if(ready)for(auto& p:calls)if(!RRAlbedoExchangeCall(&p,true)){ready=false;break;}
 if(!ready)for(auto& p:calls)if(p.site)RRAlbedoExchangeCall(&p,false);
 Log("GI_AS_RETIRE_INSTALL ready=%u sites=16 registry_only=1 native_forwarding=1 replacement_gi=0",unsigned(ready));return ready;
}
}
