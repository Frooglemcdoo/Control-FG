// Host mocks exercise the actual gateway and installation control flow.
// They do not simulate Windows SEH or instruction-cache behavior.
#include <atomic>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#define __try try
#define __except(x) catch(...)
using DWORD=unsigned long;using HMODULE=void*;using UINT=unsigned;
constexpr unsigned D3D12_RESOURCE_DIMENSION_TEXTURE2D=3;
constexpr unsigned DXGI_FORMAT_R16_FLOAT=54;
struct FakeDesc {unsigned Dimension=3;unsigned long long Width=2560;unsigned Height=1440,DepthOrArraySize=1,MipLevels=1,Format=DXGI_FORMAT_R16_FLOAT,Flags=0;struct{unsigned Count=1;}SampleDesc;};
using D3D12_RESOURCE_DESC=FakeDesc;
struct ID3D12GraphicsCommandList {};struct ID3D12Resource {FakeDesc GetDesc(){return {};}};
static DWORD lastError=0;
static DWORD GetLastError(){return lastError;}
static void SetLastError(DWORD x){lastError=x;}
static DWORD GetExceptionCode(){return 123;}
static unsigned long long currentFrame=42;
static bool ReadEngineFrameSafe(unsigned long long* f,DWORD* e){*f=currentFrame;*e=0;return true;}
template<class... T> static void Log(const char*,T...){SetLastError(999);}
static bool failGet=false;static unsigned gets=0;
static unsigned getResource(void*,const char*,ID3D12Resource** r){++gets;if(failGet)throw std::runtime_error("getter");*r=reinterpret_cast<ID3D12Resource*>(0x1234);return 1;}
static unsigned getFloat(void*,const char*,float* f){*f=.25f;return 1;}
static unsigned getInt(void*,const char*,int* i){*i=1;return 1;}
static auto ngxGetResource=&getResource;static auto ngxGetFloat=&getFloat;static auto ngxGetInt=&getInt;
static unsigned char module[0x53000],relays[2][4096];static unsigned allocations=0,writes=0,failWrite=0;
static HMODULE verifiedD3d=module;
static void* GetProcAddress(HMODULE,const char*){return module+0x52a20;}
static void* AllocateExecutableRelayNear(void*){return relays[allocations++%2];}
constexpr unsigned PAGE_EXECUTE_READ=0x20;
static bool VirtualProtect(void*,unsigned,unsigned,DWORD* p){*p=4;return true;}
static void* GetCurrentProcess(){return nullptr;}
static bool FlushInstructionCache(void*,void*,std::size_t){return true;}
struct RRAlbedoCallPatch{unsigned char* site;unsigned char original[5],replacement[5];void* relay;bool writeHealthy;};
static bool RRAlbedoExchangeCall(RRAlbedoCallPatch* p,bool install){
 ++writes;if(writes==failWrite)return false;
 auto expected=install?p->original:p->replacement;auto next=install?p->replacement:p->original;
 if(std::memcmp(p->site,expected,5))return false;
 std::memcpy(p->site,next,5);p->writeHealthy=true;return true;
}
#include "../../src/rr_live_frame_input.h"
static unsigned liveCalls=0;
static void RRLiveBeforeEvaluation(ID3D12GraphicsCommandList*,const control_rr::LiveFrameInput& in) {
 ++liveCalls;
 if(failGet) assert(!in.Matches(currentFrame));
 else assert(in.Matches(currentFrame)&&in.jitterX==.25f&&in.jitterY==.25f&&in.reset==1&&in.MatchesExtent(2560,1440));
}
#include "../../src/rr_frame_coordinator.h"
#include "../../src/rr_evaluation_inputs.h"
static control_rr::RRFrameCoordinator rrNativeFrame;
static bool rrNativeFrameEnabled=false,rrNativeLightingComplete=false,missingGuides=false,setFailure=false;
static unsigned distanceCalls=0,testPreset=6;
static bool testProjection=false;
static ID3D12Resource* testDistance=nullptr;
static ID3D12Resource* lastDistance=nullptr;
static ID3D12Resource* RRDistanceBeforeEvaluation(ID3D12GraphicsCommandList*,ID3D12Resource*,unsigned long long,bool){++distanceCalls;return testDistance;}
static ID3D12Resource* rrDistanceLastStatus=nullptr;
static void RRInputCaptureBeforeEvaluation(ID3D12GraphicsCommandList*,unsigned long long,unsigned,ID3D12Resource*,ID3D12Resource*,ID3D12Resource*,ID3D12Resource*,ID3D12Resource*,const float*,bool){}
static bool RRNativePartialActive(){return false;}
struct RRNativeGuideBindings {float viewToClip[16]{};bool projectionValid=false;ID3D12Resource*normal=nullptr;ID3D12Resource*diffuse=nullptr;ID3D12Resource*specular=nullptr;ID3D12Resource*responsivity=nullptr;};
static bool RRNativeResolveEvaluationPreset(ID3D12GraphicsCommandList*,void* feature,void*,void** out,bool* reset,unsigned* active){
 *out=feature;*reset=false;*active=testPreset;return true;
}
static ID3D12Resource* RRDiffuseCharacterCurrent(unsigned long long,UINT,UINT){return nullptr;}
static ID3D12Resource* RRSkinMaskBeforeEvaluation(ID3D12GraphicsCommandList*,ID3D12Resource*,UINT,UINT,unsigned long long){return nullptr;}
static bool RRNativeTakeGuides(const RREvaluationInputs&,RRNativeGuideBindings& b){b.projectionValid=testProjection;return !missingGuides;}
#include "../../src/rr_rt_stack.h"
static control_rr::RRRTFrame rrNativeRTFrame;
static bool checkRTStack=false;
static unsigned currentEffects=1;
static bool RRNativeLightingReady(unsigned long long frame){return checkRTStack?rrNativeRTFrame.Complete(frame,currentEffects,rrNativeLightingComplete):rrNativeLightingComplete;}
static bool lastRRReset=false;
static bool RRNativeSetGuides(void*,const RRNativeGuideBindings*,ID3D12Resource* distance,bool reset){lastDistance=distance;lastRRReset=reset;return !setFailure;}
static UINT RRDiffuseFamilyCount(unsigned long long,UINT){return 0;}
struct RRDiffuseCharacterSummary {UINT batches=0;unsigned long long instances=0,fingerprint=0;};
static RRDiffuseCharacterSummary RRDiffuseCharacterInfo(unsigned long long){return {};}
// Profiling must preserve the gateway arguments/result/LastError even if its helpers touch it.
struct RRGuideInputSnapshot {ID3D12GraphicsCommandList* commandList=nullptr;unsigned long long engineFrame=0;};
struct RRPerfTicket {};
enum class RRPerfStage {NativeEvaluation};
static long long RRPerfClock(){SetLastError(888);return 1;}
static double RRPerfElapsed(long long){SetLastError(887);return 0;}
static bool RRGuideReadRendererContext(RRGuideInputSnapshot*,const char**){return false;}
static RRPerfTicket RRPerfBegin(const RRGuideInputSnapshot&,RRPerfStage){return {};}
static void RRPerfEnd(RRPerfTicket){SetLastError(886);}
static unsigned GetFGUserMultiplier(){return 0;}
static bool IsHdr10BridgeActive(){return false;}
static void RRPerfEvaluation(unsigned long long,bool,bool,bool,long long,double,double,unsigned,unsigned){SetLastError(885);}
#include "../../src/rr_evaluation_entry.h"
static unsigned nativeCalls=0,nativeResult=0xbad00007u;
static unsigned native(ID3D12GraphicsCommandList* a,void* b,void* c,void* d){
 assert(a==reinterpret_cast<ID3D12GraphicsCommandList*>(1));assert(b==reinterpret_cast<void*>(2));
 assert(c==reinterpret_cast<void*>(3));assert(d==reinterpret_cast<void*>(4));
 assert(GetLastError()==17);++nativeCalls;SetLastError(18);return nativeResult;
}
static void reset(){
 std::memset(module,0,sizeof(module));allocations=writes=failWrite=0;
 for(auto site:control_rr::EvaluationTailSites){control_rr::Jump5 j{};assert(control_rr::RelativeJump(reinterpret_cast<std::uintptr_t>(module+site),reinterpret_cast<std::uintptr_t>(module+0x52a20),j));std::memcpy(module+site,j.data(),5);}
}
int main(){
 reset();module[0x1e156]=0xe8;assert(!RRInstallEvaluationEntry(module));assert(writes==0&&allocations==0);
 reset();failWrite=2;assert(!RRInstallEvaluationEntry(module));assert(writes==3);
 for(auto site:control_rr::EvaluationTailSites){control_rr::Jump5 j{};assert(control_rr::RelativeJump(reinterpret_cast<std::uintptr_t>(module+site),reinterpret_cast<std::uintptr_t>(module+0x52a20),j));assert(!std::memcmp(module+site,j.data(),5));}
 reset();assert(RRInstallEvaluationEntry(module));assert(writes==2);
 rrEvaluationOriginal=&native;
 for(unsigned i=0;i<10000;++i){SetLastError(17);failGet=(i%2)!=0;
 assert(RREvaluationEntry(i%2,reinterpret_cast<ID3D12GraphicsCommandList*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),reinterpret_cast<void*>(4))==0xbad00007u);assert(GetLastError()==18);}
 assert(nativeCalls==10000&&gets==25000&&liveCalls==0);
 control_rr::RRUserSetMode(control_rr::RRUserMode::Full);
 rrNativeFrameEnabled=true;failGet=false;nativeResult=1;
 for(unsigned fault=0;fault<5;++fault){
  rrNativeFrame={};assert(rrNativeFrame.Begin({42,3840,2160,2560,1440},true,true));assert(rrNativeFrame.FeatureResult(true,true));assert(rrNativeFrame.BeginLighting(42));
  missingGuides=fault==1;rrNativeLightingComplete=fault!=2;setFailure=fault==3;
  const unsigned calls=nativeCalls;SetLastError(17);
  const auto result=RREvaluationEntry(fault==4?0:1,reinterpret_cast<ID3D12GraphicsCommandList*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
  assert(result==(fault?0xbad00001u:1u));assert(nativeCalls==calls+(fault?0:1));assert(rrNativeFrame.Stopped()==(fault!=0));
 }
 missingGuides=setFailure=false;
 for(unsigned fault=0;fault<5;++fault){
  rrNativeFrame={};currentFrame=42;failGet=false;
  assert(rrNativeFrame.Begin({40,3840,2160,2560,1440},true,true));assert(rrNativeFrame.FeatureResult(true,true));
  assert(rrNativeFrame.BeginLighting(40));assert(rrNativeFrame.BeginEvaluation(40,true,true));assert(rrNativeFrame.EvaluationResult(true));
  assert(rrNativeFrame.Begin({41,3840,2160,2560,1440},true,true));assert(rrNativeFrame.FeatureResult(true,true));
  assert(!rrNativeFrame.Begin({42,3840,2160,2560,1440},true,true));assert(rrNativeFrame.Recovering());assert(rrNativeFrame.FeatureResult(true,false));
  if(fault!=2)assert(rrNativeFrame.HistoryResetResult(true));
  failGet=fault==3;if(fault==4)currentFrame=44;
  nativeResult=fault==1?0xbad00007u:1u;const auto calls=nativeCalls;SetLastError(17);
  assert(RREvaluationEntry0(reinterpret_cast<ID3D12GraphicsCommandList*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),reinterpret_cast<void*>(4))==nativeResult);
  assert(nativeCalls==calls+1&&GetLastError()==18);
  assert(rrNativeFrame.Stopped()==(fault!=0));assert(!rrNativeFrame.Recovering());
  if(!fault){
   currentFrame=43;assert(rrNativeFrame.Begin({43,3840,2160,2560,1440},true,true));assert(rrNativeFrame.FeatureResult(true,true));
   assert(rrNativeFrame.BeginLighting(43));rrNativeLightingComplete=true;lastRRReset=false;SetLastError(17);
   assert(RREvaluationEntry1(reinterpret_cast<ID3D12GraphicsCommandList*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),reinterpret_cast<void*>(4))==1);
   assert(nativeCalls==calls+2&&GetLastError()==18&&lastRRReset);
   assert(rrNativeFrame.Stage()==control_rr::RRFrameStage::Complete&&control_rr::rrUserStatus.load()==control_rr::RRUserStatus::Active);
  }
 }
 checkRTStack=true;currentFrame=42;nativeResult=1;failGet=false;
 for(unsigned fault=0;fault<3;++fault){
  rrNativeFrame={};assert(rrNativeFrame.Begin({42,3840,2160,2560,1440,107},true,true));
  assert(rrNativeFrame.FeatureResult(true,true));assert(rrNativeFrame.BeginLighting(42));
  rrNativeLightingComplete=true;rrNativeRTFrame.Begin(42,107,true);currentEffects=fault==2?1:107;
  if(fault!=1)assert(rrNativeRTFrame.ObserveGI(42,true,true,true));
  const auto calls=nativeCalls;SetLastError(17);
  const auto result=RREvaluationEntry1(reinterpret_cast<ID3D12GraphicsCommandList*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
  assert(result==(fault?0xbad00001u:1u)&&nativeCalls==calls+(fault?0:1));
 }
 // Exercise the actual gateway: F binds; E clears; bad projection skips dispatch;
 // unavailable distance remains optional and cannot stop a valid RR evaluation.
 checkRTStack=false;missingGuides=setFailure=failGet=false;rrNativeLightingComplete=true;
 for(unsigned mode=0;mode<4;++mode){
  testPreset=mode==1?5:6;testProjection=mode!=2;
  testDistance=mode==3?nullptr:reinterpret_cast<ID3D12Resource*>(0x9876);
  rrNativeFrame={};assert(rrNativeFrame.Begin({42,3840,2160,2560,1440},true,true));
  assert(rrNativeFrame.FeatureResult(true,true));assert(rrNativeFrame.BeginLighting(42));
  const auto before=distanceCalls;SetLastError(17);
  assert(RREvaluationEntry1(reinterpret_cast<ID3D12GraphicsCommandList*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),reinterpret_cast<void*>(4))==1);
  assert(distanceCalls==before+((mode==0||mode==3)?1u:0u));
  assert(lastDistance==(mode==0?testDistance:nullptr));
 }
 puts("PASS D1 actual gateway F binding, E clearing, invalid projection and optional fallback");
 puts("PASS: r21y gateway forwards once; hit distance is not a guide prerequisite; getter faults, RR guards and SR recovery remain fail-closed");
}
