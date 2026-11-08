#include "../src/rt_surface_source_capture.h"
#include <cassert>
#include <iostream>
#include <cwchar>
int main(){
 assert(control_rr_gi::SpatialPasses(control_rr_gi::StageMode::BothSteadySpatial,6)==2);
 assert(control_rr_gi::SpatialPasses(control_rr_gi::StageMode::BothSteadySpatial,1)==1);
 using namespace rt_surface_source;using namespace control_rr_gi;
 assert(request.load()==0 && giScope==0);
 manualGI.store(1);assert(EffectiveRequest()==2);
 manualGI.store(2);assert(EffectiveRequest()==3);
 manualContact.store(2);assert(ManualContactUnfiltered());
 request.store(1);assert(EffectiveRequest()==1 && !ManualContactUnfiltered());
 manualContact.store(1);assert(!ManualContactPreserved());
 request.store(0);assert(EffectiveRequest()==3 && ManualContactPreserved());
 manualGI.store(3);assert(EffectiveRequest()==7 && Stage(7,StageMode::RawBypass)==StageMode::TemporalOnly && SpatialPasses(Stage(7,StageMode::RawBypass),6)==0);
 SelectSavedStage(StageMode::RawBypass);assert(EffectiveRequest()==0);
 assert(std::wcscmp(FilterLabel(0,StageMode::RawBypass),L"GI: RAW BYPASS")==0);
 assert(std::wcscmp(FilterLabel(7,StageMode::RawBypass),L"GI: OWNED FILTER")==0);
 assert(rt_surface_source::SpatialPasses(0,StageMode::RawBypass,6)==0);
 manualGI=3;request=3;SelectSavedStage(StageMode::RawBypass);assert(EffectiveRequest()==3);request=0;
 ResetManual();assert(EffectiveRequest()==0 && !ManualContactPreserved() && !ManualContactUnfiltered());
 for(unsigned phase=0;phase<7;++phase)for(unsigned mode=0;mode<8;++mode)for(unsigned v=0;v<3;++v){
  auto saved=StageMode(mode);auto view=DiagnosticView(v);auto result=Stage(phase,saved);
  if(phase==2){assert(result==StageMode::ClampOnly && View(phase,result,view)==DiagnosticView::Zero);}
  else if(phase==3||phase==5){assert(result==StageMode::BothSteadySpatial && View(phase,result,view)==DiagnosticView::Normal);}
  else {assert(result==saved && View(phase,result,view)==EffectiveView(saved,view));}
  assert(PreserveContact(phase)==(phase==4||phase==5));
 }
 Snapshot empty{};for(unsigned p:{2u,3u,4u,5u})assert(!Covered(p,empty));
 for(unsigned p:{0u,8u,100u})assert(!Covered(p,empty));
 Snapshot gi{};gi[unsigned(Event::GIEligible)]=gi[unsigned(Event::GITemporal)]=gi[unsigned(Event::GISteady)]=gi[unsigned(Event::GIStableTemporal)]=1;
 assert(Covered(3,gi) && !Covered(5,gi));
 gi[unsigned(Event::ContactEntry)]=gi[unsigned(Event::ContactNative)]=1;assert(!Covered(5,gi));
 gi[unsigned(Event::ContactTemporal)]=1;assert(Covered(5,gi));
 auto partial=gi;partial[unsigned(Event::GISteady)]=0;assert(!Covered(5,partial));
 Snapshot raw{};raw[unsigned(Event::GIRaw)]=raw[unsigned(Event::GIEligible)]=1;
 assert(Covered(8,raw));raw[unsigned(Event::GISpatial)]=1;assert(!Covered(8,raw));
 auto fallback=gi;fallback[unsigned(Event::GIStableTemporal)]=0;assert(!Covered(3,fallback));
 Snapshot zero{};zero[unsigned(Event::GITemporal)]=1;assert(!Covered(2,zero));
 zero[unsigned(Event::GIZero)]=1;assert(!Covered(2,zero));
 zero[unsigned(Event::GIEligible)]=1;assert(Covered(2,zero));
 Record(0,Event::GIZero);assert(Read(0)[unsigned(Event::GIZero)]==0);
 Record(2,Event::GIZero);assert(Read(2)[unsigned(Event::GIZero)]==1 && Read(3)[unsigned(Event::GIZero)]==0);
 std::cout<<"PASS transient source override truth table, missing and partial execution coverage, phase counter attribution and unchanged gameplay\n";
}
