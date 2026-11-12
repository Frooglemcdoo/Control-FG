#define NOMINMAX
#define CONTROL_GI_LONG_SEQUENCE
#include <windows.h>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
static HMODULE verifiedRenderer=nullptr,verifiedD3d=nullptr;
static constexpr char kRendererHash[]="fixture",kD3dHash[]="fixture";
static bool HashMatches(HMODULE,const char*)noexcept{return true;}
static void Log(const char*,...)noexcept{}
static void* AllocateExecutableRelayNear(const void* origin)noexcept{
 SYSTEM_INFO si{};GetSystemInfo(&si);const auto gran=si.dwAllocationGranularity;
 const auto start=reinterpret_cast<std::uintptr_t>(origin)&~(std::uintptr_t(gran)-1);
 for(std::uintptr_t offset=gran;offset<0x10000000;offset+=gran)
  if(auto* p=VirtualAlloc(reinterpret_cast<void*>(start+offset),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))return p;
 return nullptr;
}
static bool Rel32Fits(const unsigned char* from,const void* to)noexcept{
 const auto d=reinterpret_cast<std::intptr_t>(to)-reinterpret_cast<std::intptr_t>(from);return d>=INT32_MIN&&d<=INT32_MAX;
}
#include "../src/gi_native_scene_window.h"
namespace GINativeSceneOwners { inline std::atomic<bool> ready{false}; }
#include "../src/gi_frozen_scene_contract.h"
namespace contract=gi_frozen_scene_contract;
namespace g=gi_native_scene_window;namespace rscope=gi_native_render_scope;namespace prep=gi_native_prepare_scope;
static unsigned calls=0;static bool throws=false;static void* expected=nullptr;
static void NativeJoin(void* scene){assert(scene==expected);++calls;if(throws)throw std::runtime_error("join failure");}
static void NativePrepare(void*,void*){assert(g::window.Inspect().preparations==1);}
int main(){
 auto* r=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x640000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
 auto* d=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x140000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(r&&d);
 verifiedRenderer=reinterpret_cast<HMODULE>(r);verifiedD3d=reinterpret_cast<HMODULE>(d);
 assert(!g::Install(verifiedRenderer,verifiedD3d));
 rscope::original=reinterpret_cast<rscope::Native>(r+0x13e860);prep::original=reinterpret_cast<prep::Native>(r+0x13de60);prep::d3d=d;
 *reinterpret_cast<rscope::U64*>(d+0x136c68)=11;
 const unsigned char tail[]={0xff,0x25,0,0,0,0};std::memcpy(r+0xa8bf0,tail,6);auto* target=reinterpret_cast<void*>(&NativeJoin);std::memcpy(r+0xa8bf6,&target,8);
 r[0x128f3b]=0xe8;const int delta=0xa8bf0-(0x128f3b+5);std::memcpy(r+0x128f3c,&delta,4);
 assert(g::Install(verifiedRenderer,verifiedD3d)&&!g::Install(verifiedRenderer,verifiedD3d));
 unsigned char owner[0x228]{};int scene=0;expected=&scene;std::memcpy(owner+0x220,&expected,8);
 void* pass=reinterpret_cast<void*>(200);void* closure[6]{};closure[5]=owner;
 rscope::current={9,11,owner,pass};assert(!g::TryAcquire());
 prep::original=&NativePrepare;prep::Invoke(closure,&pass);g::Join(expected);assert(calls==1&&g::TryAcquire());
 assert(!g::TryAcquire()&&!g::ReleaseAfterConsumerOrCancellation(10));assert(g::ReleaseAfterConsumerOrCancellation(9));
 // Reject a stale joined frame even if the next prepare callback has returned.
 g::Join(expected);prep::Invoke(closure,&pass);assert(!g::TryAcquire());
 g::Join(expected);rscope::current.token=10;assert(!g::TryAcquire());rscope::current.token=9;
 void* wrong=reinterpret_cast<void*>(300);std::memcpy(owner+0x220,&wrong,8);assert(!g::TryAcquire());std::memcpy(owner+0x220,&expected,8);
 throws=true;try{g::Join(expected);assert(false);}catch(const std::runtime_error&){}assert(!g::TryAcquire());throws=false;
 g::Join(expected);assert(g::TryAcquire()&&g::ReleaseAfterConsumerOrCancellation(9));
 // Independently exercise the exact-build frame/scene adapter. Fake COM
 // identities are never dereferenced: this fixture tests admission, not GPU use.
 prep::original=reinterpret_cast<prep::Native>(r+0x13de60);
 *reinterpret_cast<void**>(r+0x631da0)=reinterpret_cast<void*>(&rscope::Invoke);
 *reinterpret_cast<void**>(r+0x632270)=reinterpret_cast<void*>(&prep::Invoke);
 assert(!contract::Install(verifiedRenderer,verifiedD3d));
 GINativeSceneOwners::ready=true;
 assert(contract::Install(verifiedRenderer,verifiedD3d));
 unsigned char device[0x90]{};UINT64 fence[2]={17,0x1234};
 *reinterpret_cast<unsigned char**>(d+0x136d28)=device;
 auto* deviceIdentity=reinterpret_cast<ID3D12Device*>(0x1000);
 std::memcpy(device+0x78,&deviceIdentity,sizeof(deviceIdentity));
 auto* fenceAddress=fence;std::memcpy(device+0x88,&fenceAddress,sizeof(fenceAddress));
 gi_reuse::NativeGIRecording recording;
 recording.device=deviceIdentity;
 recording.list=reinterpret_cast<ID3D12CommandList*>(0x2000);
 recording.queue=reinterpret_cast<ID3D12CommandQueue*>(0x3000);
 recording.heaps.frame=11;recording.heaps.deviceEpoch=17;
 recording.heaps.deviceFenceRecord=reinterpret_cast<UINT64>(fence);
 recording.heaps.deviceFenceObject=fence[1];
 const auto scope=rscope::current;
 assert(!contract::Validate(scope,recording)); // No reader lease.
 g::Join(expected);assert(g::TryAcquire());
 assert(contract::Validate(scope,recording));
 ++fence[0];assert(!contract::Validate(scope,recording));--fence[0];
 ++fence[1];assert(!contract::Validate(scope,recording));--fence[1];
 ++*reinterpret_cast<UINT64*>(d+0x136c68);assert(!contract::Validate(scope,recording));
 --*reinterpret_cast<UINT64*>(d+0x136c68);
 ++rscope::current.token;assert(!contract::Validate(scope,recording));--rscope::current.token;
 ++g::joined.epoch;assert(!contract::Validate(scope,recording));--g::joined.epoch;
 std::memcpy(owner+0x220,&wrong,8);assert(!contract::Validate(scope,recording));
 std::memcpy(owner+0x220,&expected,8);
 assert(contract::Validate(scope,recording));
 assert(g::ReleaseAfterConsumerOrCancellation(scope.token));
 assert(!contract::Validate(scope,recording));
 contract::supported=false;contract::module=nullptr;
 assert(NativeCpuExchangeCall(&g::patch,false));g::ready=false;prep::entering=nullptr;prep::leaving=nullptr;
 VirtualFree(r,0,MEM_RELEASE);VirtualFree(d,0,MEM_RELEASE);
 std::puts("PASS native scene adapter: prepare boundary, real joined epoch, owner/invocation checks, exception invalidation and exact reader release.");
}
