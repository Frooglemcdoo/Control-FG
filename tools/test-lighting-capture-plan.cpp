#include "../src/lighting_capture_plan.h"
#include <cassert>
#include <set>
int main(){
 using namespace native_lighting;using namespace capture_plan;
 std::set<Mode> modes;unsigned gpu=0;
 for(auto step:steps){assert(modes.insert(step.mode).second);if(step.gpu){++gpu;assert(step.mode==Mode::RoomBoth);}}
 assert(modes.count(Mode::RoomNativeNoRR)&&modes.count(Mode::RoomClipNoRR)&&modes.count(Mode::RoomSelectedOff));
 assert(Applied(Mode::RoomGi,0,true));
 assert(!Applied(Mode::RoomClipNoRR,kDirectBit,true));
 assert(!Applied(Mode::RoomClipNoRR,0,false));
 assert(Applied(Mode::RoomClipNoRR,kDirectBit,false));
 assert(!Applied(Mode::RoomNativeNoRR,0,true));
 assert(Applied(Mode::RoomNativeNoRR,0,false));
 assert(!IsRtSpotMode(Mode::RoomNativeNoRR));
 assert(!Disables(Mode::RoomNativeNoRR,kDirectBit));
 assert(!Disables(Mode::RoomNativeNoRR,kGiBit));
 assert(!IsSignalDiagnostic(Mode::RoomNativeNoRR)&&!IsSignalDiagnostic(Mode::RoomClipNoRR));
 assert(UsesAuthoredClip(Mode::RoomClipNoRR)&&UsesAuthoredClip(Mode::RoomSelectedOff));
 for(auto m:{Mode::RoomBentNormalOff,Mode::RoomLocalMapsOff,Mode::RoomContactOff,Mode::RoomScreenOcclusionOff}) {
  assert(modes.count(m)&&UsesAuthoredClip(m)&&PreservesGiFiltering(m)&&IsRtSpotMode(m));
  assert(!Disables(m,kDirectBit)&&!BypassesRR(m)&&!IsSignalDiagnostic(m));
  assert(!ModeAvailable(m,kRtSpotReadyBit));
  LiveSelection selection;const auto selected=selection.Select(m);
  assert(LiveSelection::GetMode(selected)==m);
  assert(selection.Observe(selected,kDirectBit));
  assert(LiveSelection::GetMode(selection.Read())==m);
 }
 for(auto m:{Mode::RoomBentNormalOff,Mode::RoomLocalMapsOff}) {
  assert(!Applied(m,kDirectBit,true)&&!Applied(m,kGiBit,true)&&Applied(m,kAllBits,true));
  LiveSelection selection;
  const auto old=selection.Select(m);const auto next=selection.Select(Mode::Native);
  assert(!selection.Observe(old,kAllBits));assert(LiveSelection::Observed(selection.Read())==0);
  assert(LiveSelection::Sequence(next)==LiveSelection::Sequence(old)+1);
 }
 assert(!Applied(Mode::RoomScreenOcclusionOff,kDirectBit,true));
 assert(!Applied(Mode::RoomScreenOcclusionOff,kGiBit,true));
 assert(Applied(Mode::RoomScreenOcclusionOff,kAllBits,true));
 assert(modes.count(Mode::RoomCorrectedNoRR));
 assert(UsesDirectSurfaceNormals(kDefaultMode)&&UsesDirectSurfaceNormals(Mode::RoomCorrectedNoRR));
 assert(BypassesSpotHistory(kDefaultMode)&&BypassesSpotHistory(Mode::RoomCorrectedNoRR));
 assert(!UsesRawSpotVisibility(kDefaultMode)&&!UsesRawSpotVisibility(Mode::RoomCorrectedNoRR));
 assert(UsesDirectSurfaceNormals(Mode::RoomBentNormalOff)&&!BypassesSpotHistory(Mode::RoomBentNormalOff));
 std::set<Mode> buttons;for(auto choice:kModeChoices)assert(buttons.insert(choice.mode).second);
 assert(!UsesDirectSurfaceNormals(Mode::RoomAuthoredClip));
 assert(UsesAuthoredClip(Mode::RoomCorrectedNoRR)&&PreservesGiFiltering(Mode::RoomCorrectedNoRR));
 assert(BypassesRR(Mode::RoomCorrectedNoRR)&&!IsSignalDiagnostic(Mode::RoomCorrectedNoRR));
 assert(!Applied(Mode::RoomCorrectedNoRR,kAllBits,true));
 assert(!Applied(Mode::RoomCorrectedNoRR,kDirectBit,false));
 assert(Applied(Mode::RoomCorrectedNoRR,kAllBits,false));
 // The automatic default must await both normal PSOs, authored rays and samples.
 spotAuthoredClipReady=true;specularSamplesReady=true;
 assert(!ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 bentNormalLightReady=true;
 assert(!ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 bentNormalSelectorReady=true;
 assert(ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 assert(ModeAvailable(Mode::RoomCorrectedNoRR,kRtSpotReadyBit));
 assert(!ModeAvailable(kDefaultMode,0));
 for(auto m:{Mode::RoomCorrectedNoAO,Mode::RoomCorrectedNoHistory,Mode::RoomCorrectedRaw}) {
  assert(modes.count(m)&&UsesDirectSurfaceNormals(m)&&UsesAuthoredClip(m));
  assert(PreservesGiFiltering(m)&&IsRtSpotMode(m)&&NeedsIsolationProducer(m));
  assert(!BypassesRR(m)&&!IsSignalDiagnostic(m));
  assert(BypassesSpotHistory(m));
  assert(UsesRawSpotVisibility(m)==(m==Mode::RoomCorrectedRaw));
  assert(!Applied(m,kDirectBit,true)&&!Applied(m,kGiBit,true)&&Applied(m,kAllBits,true));
 }
 assert(!ModeAvailable(Mode::RoomCorrectedNoAO,kRtSpotReadyBit));
 correctedNoAOLightReady=true;
 assert(!ModeAvailable(Mode::RoomCorrectedNoAO,kRtSpotReadyBit));
 correctedNoAOSelectorReady=true;
 assert(ModeAvailable(Mode::RoomCorrectedNoAO,kRtSpotReadyBit));
 assert(ModeAvailable(Mode::RoomCorrectedNoHistory,kRtSpotReadyBit));
 assert(ModeAvailable(Mode::RoomCorrectedRaw,kRtSpotReadyBit));
 correctedNoAOLightReady=false;
 assert(ModeAvailable(kDefaultMode,kRtSpotReadyBit)); // Optional failure cannot disable baseline.
 assert(modes.count(Mode::RoomCorrectedGuarded));
 assert(UsesGuardedSpotHistory(Mode::RoomCorrectedGuarded));
 assert(UsesDirectSurfaceNormals(Mode::RoomCorrectedGuarded)&&UsesAuthoredClip(Mode::RoomCorrectedGuarded));
 assert(!BypassesSpotHistory(Mode::RoomCorrectedGuarded)&&!UsesRawSpotVisibility(Mode::RoomCorrectedGuarded));
 assert(!BypassesRR(Mode::RoomCorrectedGuarded)&&PreservesGiFiltering(Mode::RoomCorrectedGuarded));
 assert(!ModeAvailable(Mode::RoomCorrectedGuarded,kRtSpotReadyBit));
 guardedHistoryReady=true;
 assert(ModeAvailable(Mode::RoomCorrectedGuarded,kRtSpotReadyBit));
 assert(!Applied(Mode::RoomCorrectedGuarded,kDirectBit,true));
 assert(Applied(Mode::RoomCorrectedGuarded,kAllBits,true));
 guardedHistoryReady=false;
 assert(ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 assert(gpu==1);assert(modes.count(Mode::Native)&&modes.count(Mode::RoomAuthoredClip));
 assert(modes.count(Mode::RoomAuthoredSpatial)&&modes.count(Mode::RoomAuthoredRaw));
 assert(!Settled(2499,100,true));assert(!Settled(4000,89,true));assert(!Settled(4000,100,false));assert(Settled(2500,90,true));
 assert(timeoutMs>settleMs+imagesPerStep*burstIntervalMs);
}
