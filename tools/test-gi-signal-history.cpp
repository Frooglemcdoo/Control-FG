#define CONTROL_ON_DEMAND_DIAGNOSTICS
#include "../src/rr_gi_signal_history.h"
#include "../src/gi_reuse/owned_request.h"
#include "../src/rt_surface_source_capture.h"
#include <cassert>
#include <iostream>
int main(){
 assert(!gi_reuse::ownedRequested.load());assert(rt_surface_source::EffectiveRequest()==0);
 assert(control_rr_gi::stageMode.load()==control_rr_gi::StageMode::Both);
 assert(control_rr_gi::Temporal(control_rr_gi::stageMode.load()));
 assert(control_rr_gi::SpatialPasses(control_rr_gi::stageMode.load(),6)==6);
 assert(rt_surface_source::Stage(rt_surface_source::EffectiveRequest(),control_rr_gi::stageMode.load())==control_rr_gi::StageMode::Both);
 control_rr::GISignalHistory h;
 assert(!h.Update(true,false,0,7,1));
 for(unsigned i=0;i<1000;++i)assert(!h.Update(true,false,0,7,1));
 assert(h.Update(true,true,0,7,1));assert(!h.Update(true,true,0,7,1));
 assert(h.Update(true,true,7,1,1));assert(!h.Update(true,true,7,1,1));
 assert(h.Update(true,true,0,7,1));assert(h.Update(true,true,0,0,1));
 assert(h.Update(true,true,0,0,2));assert(!h.Update(false,false,0,7,1));
 assert(!h.Update(true,false,0,7,1)); // coordinator handles re-entry reset
 std::cout<<"PASS R435 native/Both defaults and single RR reset per producer/filter/lighting transition\n";
}
