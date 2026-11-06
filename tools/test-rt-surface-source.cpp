#include "../src/rt_surface_source_capture.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace rt_surface_source;using namespace control_rr_gi;
 assert(request.load()==0 && giScope==0);
 for(unsigned phase=0;phase<7;++phase)for(unsigned mode=0;mode<8;++mode)for(unsigned v=0;v<3;++v){
  auto saved=StageMode(mode);auto view=DiagnosticView(v);auto result=Stage(phase,saved);
  if(phase==2){assert(result==StageMode::ClampOnly && View(phase,result,view)==DiagnosticView::Zero);}
  else if(phase==3||phase==5){assert(result==StageMode::BothSteadySpatial && View(phase,result,view)==DiagnosticView::Normal);}
  else {assert(result==saved && View(phase,result,view)==EffectiveView(saved,view));}
  assert(PreserveContact(phase)==(phase==4||phase==5));
 }
 Snapshot empty{};for(unsigned p:{2u,3u,4u,5u})assert(!Covered(p,empty));
 for(unsigned p:{0u,7u,100u})assert(!Covered(p,empty));
 Snapshot gi{};gi[unsigned(Event::GIEligible)]=gi[unsigned(Event::GITemporal)]=gi[unsigned(Event::GISteady)]=1;
 assert(Covered(3,gi) && !Covered(5,gi));
 gi[unsigned(Event::ContactEntry)]=gi[unsigned(Event::ContactNative)]=1;assert(!Covered(5,gi));
 gi[unsigned(Event::ContactTemporal)]=1;assert(Covered(5,gi));
 auto partial=gi;partial[unsigned(Event::GISteady)]=0;assert(!Covered(5,partial));
 Snapshot zero{};zero[unsigned(Event::GITemporal)]=1;assert(!Covered(2,zero));
 zero[unsigned(Event::GIZero)]=1;assert(!Covered(2,zero));
 zero[unsigned(Event::GIEligible)]=1;assert(Covered(2,zero));
 Record(0,Event::GIZero);assert(Read(0)[unsigned(Event::GIZero)]==0);
 Record(2,Event::GIZero);assert(Read(2)[unsigned(Event::GIZero)]==1 && Read(3)[unsigned(Event::GIZero)]==0);
 std::cout<<"PASS transient source override truth table, missing and partial execution coverage, phase counter attribution and unchanged gameplay\n";
}
