#define CONTROL_ON_DEMAND_DIAGNOSTICS
#include "../src/rt_surface_source_capture.h"
#include "../src/rr_lighting_diagnostic.h"
#include <cassert>
int main(){
 using namespace rt_surface_source;
 assert(!control_diagnostics::Active());assert(EffectiveRequest()==7);
 Record(7,Event::GIEntry);assert(Read(7)[unsigned(Event::GIEntry)]==0);
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::RRAttempt);
 assert(control_rr_diagnostic::counters.Snapshot()[unsigned(control_rr_diagnostic::Event::RRAttempt)]==0);
 control_diagnostics::captureActive=true;request=2;assert(EffectiveRequest()==2);
 Record(2,Event::GIEntry);assert(Read(2)[unsigned(Event::GIEntry)]==1);
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::RRAttempt);
 assert(control_rr_diagnostic::counters.Snapshot()[unsigned(control_rr_diagnostic::Event::RRAttempt)]==1);
 control_diagnostics::captureActive=false;request=0;assert(EffectiveRequest()==7);
 Record(2,Event::GIEntry);assert(Read(2)[unsigned(Event::GIEntry)]==1);
}
