#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <atomic>
struct RRAlbedoCallPatch{};struct RRIndirectCallPatch{};
namespace control_rr {enum class RRFrameStage {Lighting,Ready};}
struct Frame {bool selected=true,failed=false;control_rr::RRFrameStage stage=control_rr::RRFrameStage::Lighting;
 bool Selected(){return selected;}auto Stage(){return stage;}void Fail(const char*){failed=true;selected=false;}} rrNativeFrame;
static bool rrNativeFrameEnabled=true,rrNativeGIKeepThisFrame=true;
static void* verifiedRenderer=reinterpret_cast<void*>(0x10000000);
static unsigned fault=0,binds=0,dispatches=0,spatials=0,clones=0,filters=0;
static void* record=nullptr;static void* bound=nullptr;
static std::atomic<bool> rrGIMotionStorageReady{true};
static constexpr int kGIMotionHistoryStorageId=0x6F474931;
#include "../build/gi_motion_history.generated.h"
namespace RRAlbedoShader {
struct NativeDescriptor {std::uintptr_t shaderBase;unsigned stage;int identifier;std::uint64_t size;};
bool ReadDescriptor(std::uintptr_t p,size_t slot,NativeDescriptor* d){assert(slot==0x60);if(fault==9)return false;*d={p,5,1,5076};return true;}
bool CopyResident(const NativeDescriptor*,unsigned char*,size_t){return true;}
}
bool PTReflectionSha256(const unsigned char*,size_t,char* out){std::strcpy(out,fault==3?"wrong":"d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15");return true;}
bool PTReflectionCloneShaderRecord(void*,size_t slot,unsigned stage,int id,size_t size,unsigned char (&)[0xA8],unsigned char (&)[0x100]){
 ++clones;assert(slot==0x60&&stage==5&&id==kGIMotionHistoryStorageId&&size==5076);return fault!=4;
}
namespace native_lighting {template<class... T>void Trace(const char*,T...){SetLastError(99);}}
static void Bind(void*);static void Dispatch(void*,const void*);static void Filter(void*,void*,unsigned);
static auto rrNativeTemporalBindOriginal=&Bind;static auto rrNativeDispatchOriginal=&Dispatch;static auto rrNativeDiffuseFilterOriginal=&Filter;
#include "../src/rr_gi_temporal_native.h"
static void Bind(void* p){++binds;bound=p;if(fault==1)RaiseException(0xE0420001,0,0,nullptr);SetLastError(18);}
static void Dispatch(void*,const void*){++dispatches;if(fault==2)RaiseException(0xE0420002,0,0,nullptr);SetLastError(19);}
static void Filter(void*,void*,unsigned passes){
 ++filters;assert(passes==6);RRNativeHookGITemporalBind(record);RRNativeHookGITemporalDispatch(nullptr,nullptr);++spatials;SetLastError(20);
}
int main(){
 auto* history=reinterpret_cast<void*>(std::uintptr_t(verifiedRenderer)+0x90eac8);
 for(fault=0;fault<11;++fault){
  binds=dispatches=spatials=clones=filters=0;bound=nullptr;rrGIMotionSource=nullptr;rrNativeFrame={};rrNativeFrameEnabled=true;rrNativeGIKeepThisFrame=true;rrNativeGIHistoryZeroThisFrame=true;rrGIMotionStorageReady=true;
  record=reinterpret_cast<void*>(0x2000+fault*0x100);bool caught=false;
  if(fault==5)rrNativeGIHistoryZeroThisFrame=false;
  if(fault==6)rrNativeFrame.selected=false;
  if(fault==7)rrNativeFrame.stage=control_rr::RRFrameStage::Ready;
  if(fault==8)rrNativeGIKeepThisFrame=false;
  if(fault==10)rrGIMotionStorageReady=false;
  __try {RRNativeRunGIFilter(verifiedRenderer,history,6);}
  __except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
  assert(caught==(fault==1||fault==2));assert(!rrNativeGIHistoryScope&&!rrNativeGIMotionBound&&filters==1);
  assert(!rrNativeFrame.failed&&rrNativeFrame.selected==(fault!=6));
  if(fault==0)assert(rrNativeGIHistoryDispatches==1&&bound==rrGIMotionRecord&&clones==1);
  if(fault>=3)assert(bound==record);
  if(!caught)assert(spatials==1&&GetLastError()==20);
 }
 // A prepared clone is reused, but never rewritten for another native descriptor.
 fault=0;rrGIMotionStorageReady=true;rrGIMotionSource=nullptr;record=reinterpret_cast<void*>(0x9000);clones=0;
 assert(RRNativeGIPrepareMotion(record));assert(RRNativeGIPrepareMotion(record));assert(clones==1);
 assert(!RRNativeGIPrepareMotion(reinterpret_cast<void*>(0x9100)));assert(clones==1);
 puts("PASS GI adaptive clone selection, immutable reuse, native fallback preserves RR, exceptions and mode bypass");
}
