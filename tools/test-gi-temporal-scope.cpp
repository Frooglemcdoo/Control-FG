#include "../src/settings_check_state.h"
#include <windows.h>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <atomic>
#include <d3d12.h>
struct RRAlbedoCallPatch{};struct RRIndirectCallPatch{};
namespace control_rr {enum class RRFrameStage {Lighting,Ready};}
struct Frame {bool selected=true,failed=false;control_rr::RRFrameStage stage=control_rr::RRFrameStage::Lighting;
 struct KeyType{unsigned long long frame=123;};KeyType Key(){return {};}
 bool Selected(){return selected;}auto Stage(){return stage;}void Fail(const char*){failed=true;selected=false;}} rrNativeFrame;
static bool rrNativeFrameEnabled=true,rrNativeGIKeepThisFrame=true;
static void* verifiedRenderer=reinterpret_cast<void*>(0x10000000);
static unsigned expectedPasses=6;static bool expectedHistoryDisabled=false;static unsigned expectedView=0;
static unsigned fault=0,binds=0,dispatches=0,spatials=0,invokes=0,filters=0;
static void* record=reinterpret_cast<void*>(0x2000);static void* bound=nullptr;
namespace control_rr_reflection {struct Context{unsigned long long frame=123;std::uintptr_t list=0x3000,view=0x4000;};}
static bool RRReflectionCurrent(void*,control_rr_reflection::Context& c){if(fault==10)c.frame=124;return fault!=9;}
struct Copy {bool StateMatches(std::uintptr_t){return fault!=8;}} rrNativeCopy;
namespace control_rr_gi {static std::atomic<unsigned> ready{1};}
namespace control_rr_clamp {
static bool InvokeGI(void(*fn)(void*,const void*),void* s,const void* g,ID3D12GraphicsCommandList*){++invokes;fn(s,g);return fault!=3;}
}
template<class... T>void Log(const char*,T...){SetLastError(99);}
namespace native_lighting {template<class... T>void Trace(const char*,T...){SetLastError(99);}}
static void Bind(void*);static void Dispatch(void*,const void*);static void Filter(void*,void*,unsigned);
static auto rrNativeTemporalBindOriginal=&Bind;static auto rrNativeDispatchOriginal=&Dispatch;static auto rrNativeDiffuseFilterOriginal=&Filter;
#include "../src/rr_gi_temporal_native.h"
static void Bind(void* p){++binds;bound=p;assert(p==record);if(fault==1)RaiseException(0xE0420001,0,0,nullptr);SetLastError(18);}
static void Dispatch(void*,const void*){++dispatches;if(fault==2||(fault==11&&dispatches==2))RaiseException(0xE0420002,0,0,nullptr);SetLastError(19);}
static void Filter(void*,void*,unsigned passes){++filters;assert(passes==expectedPasses);assert(control_rr_gi::historyDisabled==expectedHistoryDisabled);assert(unsigned(control_rr_gi::activeView)==expectedView);assert(control_rr_gi::outlierEnabled==((expectedHistoryDisabled&&expectedPasses==0&&!control_rr_gi::rawBypass&&control_rr_gi::outlierRequested.load())||expectedView==1));RRNativeHookGITemporalBind(record);RRNativeHookGITemporalDispatch(nullptr,nullptr);for(unsigned i=0;i<passes;++i)RRNativeHookGISpatialDispatch(nullptr,nullptr);++spatials;SetLastError(20);}
int main(){auto* history=reinterpret_cast<void*>(std::uintptr_t(verifiedRenderer)+0x90eac8);
 for(unsigned view=0;view<3;++view)for(unsigned cleanup=0;cleanup<2;++cleanup)for(unsigned mode=0;mode<8;++mode)for(fault=0;fault<12;++fault){
  control_rr_gi::viewRequested.store(static_cast<control_rr_gi::DiagnosticView>(view));
  control_rr_gi::outlierRequested.store(cleanup!=0);
  auto stage=static_cast<control_rr_gi::StageMode>(mode);control_rr_gi::stageMode.store(stage);
  const bool eligible=fault<4||fault>=8;
  expectedPasses=eligible?control_rr_gi::SpatialPasses(stage,6):6;
  expectedHistoryDisabled=eligible&&!control_rr_gi::Temporal(stage);
  expectedView=eligible&&mode==3?view:0;
  binds=dispatches=spatials=invokes=filters=0;bound=nullptr;rrNativeFrame={};rrNativeGIKeepThisFrame=true;rrNativeGIHistoryZeroThisFrame=true;
  bool caught=false;
  if(fault==4)rrNativeGIHistoryZeroThisFrame=false;
  if(fault==5)rrNativeFrame.selected=false;
  if(fault==6)rrNativeFrame.stage=control_rr::RRFrameStage::Ready;
  if(fault==7)rrNativeGIKeepThisFrame=false;
  __try {RRNativeRunGIFilter(verifiedRenderer,history,6);}
  __except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
  assert(caught==(fault==1||fault==2||(fault==11&&expectedPasses>0)));assert(!rrNativeGIHistoryScope&&!rrNativeGIMotionBound&&filters==1&&!control_rr_gi::historyDisabled&&!control_rr_gi::outlierEnabled&&control_rr_gi::activeView==control_rr_gi::DiagnosticView::Normal);
  assert(!rrNativeFrame.failed&&rrNativeFrame.selected==(fault!=5));assert(bound==record);
  if(fault==0)assert(rrNativeGIHistoryDispatches==1&&invokes==(mode>=4&&mode<=6?1u+expectedPasses:1u));
  if(fault==0)assert(rrNativeGISpatialObserved==expectedPasses);
  if(fault==0)assert(rrNativeGISpatialApplied==(mode>=4&&mode<=6?expectedPasses:0u));
  assert(!control_rr_gi::rawBypass&&!control_rr_gi::spatialSteady&&!control_rr_gi::spatialHistoryBounded&&!control_rr_gi::spatialDispatch&&!rrNativeGISingleHistory);
  if(fault==3)assert(rrNativeGIHistoryDispatches==0&&invokes==1);
  if(fault>=4&&fault<=10)assert(invokes==0);
  if(!caught)assert(spatials==1&&GetLastError()==20);
 }
 fault=0;rrNativeFrame={};rrNativeGIKeepThisFrame=true;rrNativeGIHistoryZeroThisFrame=true;
 control_rr_gi::stageMode.store(control_rr_gi::StageMode::RawBypass);
 control_rr_gi::viewRequested.store(control_rr_gi::DiagnosticView::Normal);
 for(unsigned phase=1;phase<8;++phase){
  rt_surface_source::request.store(phase);const auto before=rt_surface_source::Read(phase);
  expectedPasses=phase==7?1:(phase==3||phase==5)?2:0;
  expectedHistoryDisabled=phase!=3&&phase!=5&&phase!=7;expectedView=phase==2?2:0;
  RRNativeRunGIFilter(verifiedRenderer,history,6);
  const auto after=rt_surface_source::Read(phase);
  assert(after[unsigned(rt_surface_source::Event::GIEligible)]==before[unsigned(rt_surface_source::Event::GIEligible)]+1);
  assert(after[unsigned(rt_surface_source::Event::GITemporal)]==before[unsigned(rt_surface_source::Event::GITemporal)]+1);
  assert(!control_rr_gi::ownedDenoise);
  assert(rt_surface_source::giScope==0 && control_rr_gi::stageMode.load()==control_rr_gi::StageMode::RawBypass);
 }
 rt_surface_source::request.store(0);
 puts("PASS R215 all GI stages, cleanup and diagnostic views, scoped history flag restoration, original-record forwarding, scoped pipeline invocation, context/frame gates, native fallback and exception cleanup");
}
