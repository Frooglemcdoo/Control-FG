#define CONTROL_PERFORMANCE_BUILD
#include "../src/rt_surface_source_capture.h"
#include "../src/rr_lighting_diagnostic.h"
#include <cassert>
#include "../src/native_lighting_live_state.h"
int main(){
 using namespace rt_surface_source;
 native_lighting::LiveSelection selection;
 assert(!selection.EnsureDefault(0));
 native_lighting::specularSamplesReady=true;
 native_lighting::spotProjectionReady=true;
 native_lighting::nativeBlendLegacyReady=true;
 native_lighting::spotAuthoredClipReady=true;
 native_lighting::bentNormalLightReady=true;
 native_lighting::bentNormalSelectorReady=true;
 native_lighting::guardedHistoryReady=true;
 control_rr_temporal::ready=true;
 assert(selection.EnsureDefault(~0u));
 assert(native_lighting::LiveSelection::GetMode(selection.Read())==native_lighting::kNativeBlendDefaultMode);
 assert(!selection.EnsureDefault(~0u));
 selection.Select(native_lighting::Mode::Native);
 assert(!selection.EnsureDefault(~0u));
 assert(native_lighting::LiveSelection::GetMode(selection.Read())==native_lighting::Mode::Native);
 ownedFilterEnabled=false;assert(EffectiveRequest()==0);ownedFilterEnabled=true;
 request.store(2);manualGI.store(1);assert(EffectiveRequest()==7);
 Record(7,Event::GIEntry);assert(Read(7)[unsigned(Event::GIEntry)]==0);
 assert(SpatialPasses(EffectiveRequest(),Stage(EffectiveRequest(),control_rr_gi::StageMode::RawBypass),6)==1);
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::RRAttempt);
 assert(control_rr_diagnostic::counters.Snapshot()[unsigned(control_rr_diagnostic::Event::RRAttempt)]==0);
}
