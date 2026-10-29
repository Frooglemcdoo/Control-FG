#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
struct RRAlbedoCallPatch{};struct RRIndirectCallPatch{};
namespace control_rr {enum class RRFrameStage {Lighting,Ready};}
struct Frame {bool selected=true,failed=false;control_rr::RRFrameStage stage=control_rr::RRFrameStage::Lighting;
 bool Selected(){return selected;}auto Stage(){return stage;}void Fail(const char*){failed=true;}} rrNativeFrame;
static bool rrNativeFrameEnabled=true,rrNativeGIKeepThisFrame=true;
static void* verifiedRenderer=reinterpret_cast<void*>(0x10000000);
static unsigned fault=0,provider=0,binds=0,dispatches=0,spatials=0,arms=0,restores=0,filters=0;
static void* record=nullptr;
namespace control_rr_native_clamp {
struct Lease {bool armed=false,bindReturned=false;unsigned original=0;};
struct Api {
 bool Arm(Lease& l){++arms;if(fault==4)return false;l.original=provider;provider=1;l.armed=true;SetLastError(71);return true;}
 bool StillForced(Lease& l){return l.armed&&provider==1;}
 bool Restore(Lease& l){++restores;if(l.armed){provider=l.original;l.armed=false;}SetLastError(72);return true;}
};}
static control_rr_native_clamp::Api rrNativeClampApi;
namespace RRAlbedoShader {
struct NativeDescriptor {std::uintptr_t shaderBase;unsigned stage;int identifier;std::uint64_t size;};
bool ReadDescriptor(std::uintptr_t p,size_t slot,NativeDescriptor* d){if(slot!=0x50)return false;*d={p,2,1,5076};return true;}
bool CopyResident(const NativeDescriptor*,unsigned char*,size_t){return true;}
}
bool PTReflectionSha256(const unsigned char*,size_t,char* out){std::strcpy(out,fault==3?"wrong":"d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15");return true;}
namespace native_lighting {template<class... T>void Trace(const char*,T...){SetLastError(99);}}
static void Bind(void*);static void Dispatch(void*,const void*);static void Filter(void*,void*,unsigned);
static auto rrNativeTemporalBindOriginal=&Bind;
static auto rrNativeDispatchOriginal=&Dispatch;
static auto rrNativeDiffuseFilterOriginal=&Filter;
#include "../src/rr_gi_temporal_native.h"
static void Bind(void*){++binds;if(rrNativeGIHistoryScope&&fault!=3&&fault!=4)assert(provider==1);if(fault==1)RaiseException(0xE0420001,0,0,nullptr);SetLastError(18);}
static void Dispatch(void*,const void*){++dispatches;if(rrNativeGIHistoryScope&&fault!=3&&fault!=4)assert(provider==1);if(fault==2)RaiseException(0xE0420002,0,0,nullptr);SetLastError(19);}
static void Filter(void*,void*,unsigned passes){
 ++filters;assert(passes==6);RRNativeHookGITemporalBind(record);RRNativeHookGITemporalDispatch(nullptr,nullptr);
 assert(provider==0);++spatials;SetLastError(20);
}
int main(){
 auto* history=reinterpret_cast<void*>(std::uintptr_t(verifiedRenderer)+0x90eac8);
 for(fault=0;fault<9;++fault){
  provider=binds=dispatches=spatials=arms=restores=filters=0;rrNativeFrame={};rrNativeFrameEnabled=true;rrNativeGIKeepThisFrame=true;rrNativeGIHistoryZeroThisFrame=true;
  record=reinterpret_cast<void*>(0x2000+fault*0x100);bool caught=false;
  if(fault==5)rrNativeGIHistoryZeroThisFrame=false;
  if(fault==6)rrNativeFrame.selected=false;
  if(fault==7)rrNativeFrame.stage=control_rr::RRFrameStage::Ready;
  if(fault==8)rrNativeGIKeepThisFrame=false;
  __try {RRNativeRunGIFilter(verifiedRenderer,history,6);}
  __except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
  assert(caught==(fault==1||fault==2));assert(!rrNativeGIHistoryScope&&provider==0&&filters==1);
  if(fault>=5)assert(arms==0&&!rrNativeFrame.failed);
  if(fault>=1&&fault<=4)assert(rrNativeFrame.failed);
  if(!caught)assert(spatials==1&&GetLastError()==20);
 }
 puts("PASS scoped GI bind/dispatch, spatial provider restoration, exception cleanup, unknown shader and RR/mode bypass");
}
