#define CONTROL_PERFORMANCE_BUILD
#include "../src/rt_surface_source_capture.h"
#include "../src/rr_lighting_diagnostic.h"
#include <cassert>
int main(){
 using namespace rt_surface_source;
 request.store(2);manualGI.store(1);assert(EffectiveRequest()==7);
 Record(7,Event::GIEntry);assert(Read(7)[unsigned(Event::GIEntry)]==0);
 assert(SpatialPasses(EffectiveRequest(),Stage(EffectiveRequest(),control_rr_gi::StageMode::RawBypass),6)==1);
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::RRAttempt);
 assert(control_rr_diagnostic::counters.Snapshot()[unsigned(control_rr_diagnostic::Event::RRAttempt)]==0);
}
