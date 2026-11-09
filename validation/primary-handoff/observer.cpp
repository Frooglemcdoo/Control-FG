#include "fixture.h"
#include "../../src/rr_user_control.h"
#include <cstring>
#include <stdexcept>
#include <cwchar>
using DWORD=unsigned long;
using HMODULE=void*;
struct SRWLOCK {bool held=false;};
static SRWLOCK rrReflectionLock;
static unsigned locks=0,unlocks=0,nativeCalls=0,prepareCalls=0,exchangeCalls=0;
static DWORD threadId=17,lastError=13;
static unsigned long long engineFrame=1;
static bool frameReadable=true,prepareOK=true,exchangeOK=true,writeHealthy=true,nativeThrows=false;
static bool rrReflectionCaptureEnabled=true;
static std::atomic<unsigned long long> presentCount{51};
static std::vector<unsigned char> d3d(0x112000),queue(0x80),closed(0x80);
static void* verifiedD3d=d3d.data();
static std::uintptr_t closedArgument=0;
static std::vector<std::pair<std::uintptr_t,std::size_t>> ranges;
static bool TryAcquireSRWLockExclusive(SRWLOCK* lock){if(lock->held)return false;lock->held=true;++locks;return true;}
static void ReleaseSRWLockExclusive(SRWLOCK* lock){assert(lock->held);lock->held=false;++unlocks;}
static DWORD GetCurrentThreadId(){return threadId;}
static DWORD GetLastError(){return lastError;}
static void SetLastError(DWORD value){lastError=value;}
static bool ReadEngineFrameSafe(unsigned long long* frame,DWORD*){*frame=engineFrame;return frameReadable;}
static void Log(const char*,...){lastError=999;}
namespace control_rr_reflection {
struct WindowsAccess {
 static bool Read(void*,std::uintptr_t address,void* out,std::size_t size){
  for(const auto& range:ranges)if(address>=range.first&&size<=range.second&&address-range.first<=range.second-size){
   std::memcpy(out,reinterpret_cast<void*>(address),size);return true;
  }
  return false;
 }
};
}
struct RRReflectionOwner {
 Api api;Backend<Api> backend;Capture<Backend<Api>> capture;PresentLedger presents;
 struct Input {bool valid=false;Context context{};} distanceInputs[3];
 void* queue=reinterpret_cast<void*>(91);DWORD recordingThread=17;bool stopped=false;
 RRReflectionOwner():backend(api),capture(backend){assert(backend.Prepare(92,91,94,testMaterialShape,testPositionShape)&&capture.Bind(backend.Key()));}
};
static RRReflectionOwner* rrReflection=nullptr;
struct RRAlbedoCallPatch {bool writeHealthy=false;};
static bool RRAlbedoPrepareCallPatch(void* site,void* original,void* hook,RRAlbedoCallPatch* patch){
 ++prepareCalls;assert(site==d3d.data()+0x32991&&original==d3d.data()+0x24e0&&hook&&patch);return prepareOK;
}
static bool RRAlbedoExchangeCall(RRAlbedoCallPatch* patch,bool install){
 ++exchangeCalls;if(install){patch->writeHealthy=writeHealthy;return exchangeOK;}return true;
}
// Exercise the actual observer's normal/early-exit branches on a bounded memory
// model. This deliberately does not emulate or claim Windows SEH validation.
#pragma push_macro("__try")
#undef __try
#define __try do
#define __except(x) while(false);if(false)
#define __leave break
#include "../../src/rr_reflection_handoff_windows.h"
#undef __try
#pragma pop_macro("__try")
#undef __except
#undef __leave
template<class T>static void put(std::vector<unsigned char>& memory,std::size_t offset,T value){std::memcpy(memory.data()+offset,&value,sizeof(value));}
static std::uintptr_t nativeAppend(void* vector,void* argument){
 ++nativeCalls;assert(vector==queue.data()+0x28&&argument==&closedArgument&&lastError==13);
 if(nativeThrows)throw std::runtime_error("native exception");
 lastError=27;return UINT64_C(0xabcdeffedcba4321);
}
int main(){
 ranges={{reinterpret_cast<std::uintptr_t>(d3d.data()),d3d.size()},
  {reinterpret_cast<std::uintptr_t>(queue.data()),queue.size()},
  {reinterpret_cast<std::uintptr_t>(closed.data()),closed.size()},
  {reinterpret_cast<std::uintptr_t>(&closedArgument),sizeof(closedArgument)}};
 RRReflectionOwner owner;rrReflection=&owner;
 control_rr::RRUserSetMode(control_rr::RRUserMode::Full);
 const auto s=snapshot(owner.api);control_rr::Lease lease{};
 assert(owner.backend.SetRecording(s.context)&&owner.capture.Record(s,1,owner.backend.Destinations(),lease));
 assert(owner.presents.Record(lease,presentCount.load()));owner.distanceInputs[lease.index]={true,s.context};
 closedArgument=reinterpret_cast<std::uintptr_t>(closed.data());
 put(d3d,0x111c38,reinterpret_cast<std::uintptr_t>(queue.data()));put(queue,0x10,std::uintptr_t(91));
 put(closed,0x18,std::uintptr_t(90));put(closed,0x28,static_cast<unsigned char>(1));
 auto consumer=s.context;consumer.list=190;
 auto rejected=[&](void* vector=queue.data()+0x28,void* argument=&closedArgument){
  const auto before=rrPrimaryQueuedCount;RRReflectionObservePrimaryAppend(vector,argument);
  assert(rrPrimaryQueuedCount==before&&owner.capture.PrimaryHandoffReason(lease,consumer,1));
 };
 rrReflectionCaptureEnabled=false;rejected();rrReflectionCaptureEnabled=true;
 rrReflectionLock.held=true;rejected();rrReflectionLock.held=false;
 rrReflection=nullptr;rejected();rrReflection=&owner;
 owner.stopped=true;rejected();owner.stopped=false;
 ++threadId;rejected();--threadId;
 rejected(queue.data()+0x18);rejected(queue.data()+0x28,reinterpret_cast<void*>(1));
 put(queue,0x10,std::uintptr_t(92));rejected();put(queue,0x10,std::uintptr_t(91));
 put(closed,0x28,static_cast<unsigned char>(0));rejected();put(closed,0x28,static_cast<unsigned char>(1));
 put(closed,0x18,std::uintptr_t(89));rejected();put(closed,0x18,std::uintptr_t(90));
 frameReadable=false;rejected();frameReadable=true;
 ++engineFrame;rejected();--engineFrame;
 ++presentCount;rejected();--presentCount;
 owner.distanceInputs[lease.index].valid=false;rejected();owner.distanceInputs[lease.index].valid=true;
 owner.distanceInputs[lease.index].context.queue=92;rejected();owner.distanceInputs[lease.index].context.queue=91;
 assert(locks==unlocks&&!rrReflectionLock.held);
 // The hook must preserve native return bits and error even when the observer logs.
 rrPrimaryAppendOriginal=&nativeAppend;lastError=13;
 assert(RRReflectionHookPrimaryAppend(queue.data()+0x28,&closedArgument)==UINT64_C(0xabcdeffedcba4321));
 assert(nativeCalls==1&&lastError==27&&rrPrimaryQueuedCount==1&&!owner.capture.PrimaryHandoffReason(lease,consumer,1));
 RRReflectionObservePrimaryAppend(queue.data()+0x28,&closedArgument);assert(rrPrimaryQueuedCount==1);
 nativeThrows=true;lastError=13;const auto beforeLocks=locks;
 try{RRReflectionHookPrimaryAppend(queue.data()+0x28,&closedArgument);assert(false);}catch(const std::runtime_error&){}
 assert(nativeCalls==2&&locks==beforeLocks);nativeThrows=false;
 assert(owner.capture.TakeOnPrimaryQueue(lease,consumer,1)&&owner.capture.SubmitCaptured(lease));
 owner.api.completed=owner.api.signal;assert(owner.capture.Collect()&&owner.capture.Idle()&&owner.backend.Dispose(true));
 // Installer address/prefix gates and failed-write rollback.
 assert(!RRReflectionInstallPrimaryAppend(nullptr));
 assert(!RRReflectionInstallPrimaryAppend(queue.data()));
 assert(!RRReflectionInstallPrimaryAppend(verifiedD3d)&&prepareCalls==0);
 const unsigned char prefix[]{0x48,0x8d,0x4e,0x28};std::memcpy(d3d.data()+0x3298d,prefix,4);
 prepareOK=false;assert(!RRReflectionInstallPrimaryAppend(verifiedD3d)&&exchangeCalls==0);prepareOK=true;
 exchangeOK=false;assert(!RRReflectionInstallPrimaryAppend(verifiedD3d)&&exchangeCalls==1);exchangeOK=true;
 writeHealthy=false;assert(!RRReflectionInstallPrimaryAppend(verifiedD3d)&&exchangeCalls==3);writeHealthy=true;
 assert(RRReflectionInstallPrimaryAppend(verifiedD3d)&&exchangeCalls==4&&rrPrimaryAppendOriginal==reinterpret_cast<RRPrimaryAppendFn>(d3d.data()+0x24e0));
 control_rr::RRUserRequest(true);control_rr::RRUserPublish(control_rr::RRUserStatus::Waiting);
 control_rr::RRUserDistanceDispatch(false,"producer_list_not_queued");control_rr::RRUserWaitingStatus(true,true,1);
 assert(std::wcscmp(control_rr::RRUserLabel(),L"Blocked: reflection handoff; see log")==0);
 control_rr::RRUserDistanceDispatch(false,"pipeline_prepare_requested");control_rr::RRUserWaitingStatus(true,true,1);
 assert(std::wcscmp(control_rr::RRUserLabel(),L"Preparing reflection distance pipeline")==0);
 puts("PASS actual enqueue observer: identity/closed/frame/Present/thread/lock rejection, exact native forwarding, LastError, native exceptions, installer rollback and user status; Windows SEH not exercised");
}
