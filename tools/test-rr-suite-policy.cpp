#define CONTROL_RR_F9_SUITE
#include "../src/rr_capture_ui.h"
#include "../src/rr_clamp_strength_policy.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 using namespace rr_suite;using P=control_rr::InputCapturePolicy;namespace ui=control_rr_capture_ui;
 assert(!control_rr::InputCapturePairMayExport(P::Submitted,P::Ready,false));
 assert(!control_rr::InputCapturePairMayExport(P::Ready,P::Submitted,false));
 assert(control_rr::InputCapturePairMayExport(P::Submitted,P::Submitted,false));
 assert(control_rr::InputCapturePairMayExport(P::Exporting,P::Recorded,false));
 assert(control_rr::InputCapturePairMayExport(P::Submitted,P::Ready,true));
 P a,b;
 for(unsigned n=1;n<=16;++n){
  const unsigned s=(n-1)%count+1;requested=s;assert(Latch(n));
  assert(Current()->samples==(s==8?16:4));assert(control_rr_clamp::RuntimeRequested()==(s==2?0:60));
  clampFrame=n;clampPercent=s==2?0:60;dispatchFrame=n;dispatchStep=s;sampleFrame=n;effectiveSamples=Current()->samples;assert(Verified(n));
  effectiveSamples=1;assert(!Verified(n));effectiveSamples=Current()->samples;
  dispatchFrame=n-1;assert(!Verified(n));dispatchFrame=n;
  assert(ui::Request());assert(!ui::Request());ui::requested=false;
  for(auto* p:{&a,&b}){assert(p->Request());assert(p->BeginPrepare());p->Prepared(true);assert(p->Record(n));assert(!p->CanSubmit(n));assert(p->CanSubmit(n+1));p->Submit(true);assert(!p->Retire(0));assert(p->Retire(1));p->Exported();}
  ui::Publish(a.phase,b.phase,n,true);assert(ui::completedPairs==n);
 }
 assert(!ui::Request());requested=0;assert(Latch(17));assert(!Current());assert(control_rr_clamp::RuntimeRequested()==60);
 puts("PASS: sixteen capture pairs, state changes, frame/sample guards, clamp restore and fence gating");
}
