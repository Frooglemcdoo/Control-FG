#include "../src/lighting_capture_plan.h"
#include <cassert>
#include <set>
int main(){
 assert(native_lighting::specularSampleChoice.load()==0);
 assert(native_lighting::SetSpecularSampleChoice(4));
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
 assert(buttons.count(Mode::RoomNativeNoRR)&&BypassesRR(Mode::RoomNativeNoRR));
 assert(!UsesDirectSurfaceNormals(Mode::RoomAuthoredClip));
 assert(UsesAuthoredClip(Mode::RoomCorrectedNoRR)&&PreservesGiFiltering(Mode::RoomCorrectedNoRR));
 assert(BypassesRR(Mode::RoomCorrectedNoRR)&&!IsSignalDiagnostic(Mode::RoomCorrectedNoRR));
 assert(!Applied(Mode::RoomCorrectedNoRR,kAllBits,true));
 assert(!Applied(Mode::RoomCorrectedNoRR,kDirectBit,false));
 assert(Applied(Mode::RoomCorrectedNoRR,kAllBits,false));
 // The automatic default must await both normal PSOs, authored rays and samples.
 mapProbeReady=true;spotAuthoredClipReady=true;spotProjectionReady=true;specularSamplesReady=true;
 assert(!ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 bentNormalLightReady=true;
 assert(!ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 bentNormalSelectorReady=true;
 assert(ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 assert(ModeAvailable(Mode::RoomCorrectedNoRR,kRtSpotReadyBit));
 assert(!ModeAvailable(kDefaultMode,0));
 const auto open=Mode::RoomCorrectedOpen;
 assert(buttons.count(open)&&SupportsFocusedLightingCapture(open));
 assert(UsesDirectSurfaceNormals(open)&&UsesAuthoredClip(open)&&PreservesGiFiltering(open));
 assert(BypassesSpotHistory(open)&&BypassesRR(open)&&!UsesRawSpotVisibility(open));
 assert(NeedsIsolationProducer(open)&&IsRtSpotMode(open)&&!IsSignalDiagnostic(open));
 assert(!ModeAvailable(open,kRtSpotReadyBit));
 correctedOpenLightReady=true;
 assert(ModeAvailable(open,kRtSpotReadyBit));
 correctedOpenLightReady=false;
 assert(ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 assert(!SupportsFocusedLightingCapture(kDefaultMode));
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
 for(auto m:{Mode::RoomCorrectedVisibility,Mode::RoomCorrectedRawVisibility,Mode::RoomCorrectedLightIds}) {
  assert(modes.count(m));
  assert(UsesDirectSurfaceNormals(m)&&UsesAuthoredClip(m)&&BypassesSpotHistory(m));
  assert(PreservesGiFiltering(m)&&UsesWorldAlbedo(m)&&SpecularSampleRequest(m)==4);
  assert(NeedsIsolationProducer(m)&&IsSignalDiagnostic(m)&&BypassesRR(m));
  assert(UsesRawSpotVisibility(m)==(m==Mode::RoomCorrectedRawVisibility));
  assert(ShowsSpotVisibility(m)==(m!=Mode::RoomCorrectedLightIds));
  assert(ShowsSpotLightIds(m)==(m==Mode::RoomCorrectedLightIds));
  assert(!Applied(m,kAllBits,true)&&!Applied(m,kDirectBit,false)&&Applied(m,kAllBits,false));
  assert(ModeAvailable(m,kRtSpotReadyBit));
  spotAuthoredClipReady=false;assert(!ModeAvailable(m,kRtSpotReadyBit));spotAuthoredClipReady=true;
  bentNormalSelectorReady=false;assert(!ModeAvailable(m,kRtSpotReadyBit));bentNormalSelectorReady=true;
  LiveSelection live;assert(LiveSelection::GetMode(live.Select(m))==m);
 }
 for(auto m:{Mode::RoomCorrectedNoBakedGi,Mode::RoomCorrectedNoBakedGiNoRR}) {
  assert(modes.count(m));
  assert(!buttons.count(m));
  assert(UsesDirectSurfaceNormals(m)&&UsesAuthoredClip(m)&&BypassesSpotHistory(m));
  assert(PreservesGiFiltering(m)&&UsesWorldAlbedo(m)&&SpecularSampleRequest(m)==4);
  assert(!UsesRawSpotVisibility(m)&&!IsSignalDiagnostic(m));
  assert(BypassesRR(m)==(m==Mode::RoomCorrectedNoBakedGiNoRR));
  assert(Disables(m,kGiBit)&&!Disables(m,kDirectBit));
  assert(!ModeAvailable(m,kRtSpotReadyBit));
  rtBakedGiReadyMask=0;assert(!ModeAvailable(m,kRtSpotReadyBit|kGiBit));
  rtBakedGiReadyMask=1;assert(!ModeAvailable(m,kRtSpotReadyBit|kGiBit));
  rtBakedGiReadyMask=2;assert(!ModeAvailable(m,kRtSpotReadyBit|kGiBit));
  rtBakedGiReadyMask=3;assert(ModeAvailable(m,kRtSpotReadyBit|kGiBit));
  assert(!Applied(m,kAllBits,false));
  assert(!Applied(m,kBakedGiOverrideBit,false));
  assert(Applied(m,kBindingBits,false));
  assert(Applied(m,kBindingBits,true)==!BypassesRR(m));
  LiveSelection live;auto old=live.Select(m);
  assert(live.Observe(old,kAllBits));assert(live.Observe(old,kBakedGiOverrideBit));
  assert(LiveSelection::Observed(live.Read())==kBindingBits);
  assert(LiveSelection::GetMode(live.Read())==m);
  auto next=live.Select(kDefaultMode);
  assert(LiveSelection::Sequence(next)==LiveSelection::Sequence(old)+1);
  assert(LiveSelection::Observed(next)==0&&!live.Observe(old,kBakedGiOverrideBit));
 }
 assert(UsesDirectSurfaceNormals(Mode::RoomNativeBlend)&&BypassesSpotHistory(Mode::RoomNativeBlend)&&!BypassesRR(Mode::RoomNativeBlend));
 assert(!ModeAvailable(Mode::RoomNativeBlend,kRtSpotReadyBit));nativeBlendReady=true;
 assert(ModeAvailable(Mode::RoomNativeBlend,kRtSpotReadyBit));
 assert(Applied(Mode::RoomNativeBlend,kAllBits,true));
 assert(Applied(Mode::RoomNativeBlend,kAllBits,false));
 assert(!Applied(Mode::RoomNativeBlend,kDirectBit,true));
 assert(SupportsLightContextCapture(Mode::RoomNativeBlend));
 assert(BlockerMode(Mode::RoomNativeBlend)==Mode::RoomCorrectedNoRR);
 assert(capture_plan::BlockerMode(Mode::RoomMapProbe)==Mode::RoomMapProbe);
 assert(UsesDirectSurfaceNormals(Mode::RoomMapProbe)&&BypassesSpotHistory(Mode::RoomMapProbe)&&BypassesRR(Mode::RoomMapProbe));
 assert(ModeAvailable(Mode::RoomMapProbe,kRtSpotReadyBit));
 mapProbeReady=false;assert(!ModeAvailable(Mode::RoomMapProbe,kRtSpotReadyBit));mapProbeReady=true;
 assert(UsesDirectSurfaceNormals(Mode::RoomProjectionOrigin)&&BypassesSpotHistory(Mode::RoomProjectionOrigin)&&BypassesRR(Mode::RoomProjectionOrigin));
 assert(ModeAvailable(Mode::RoomProjectionOrigin,kRtSpotReadyBit));
 spotProjectionReady=false;assert(!ModeAvailable(Mode::RoomProjectionOrigin,kRtSpotReadyBit));spotProjectionReady=true;
 const auto priority=Mode::RoomCorrectedMapPriority;
 assert(!buttons.count(priority)&&SupportsFocusedLightingCapture(priority)&&SupportsLightContextCapture(priority));
 assert(BlockerMode(priority)==Mode::RoomCorrectedNoRR&&BlockerMode(kDefaultMode)==Mode::RoomCorrectedNoRR);
 assert(UsesDirectSurfaceNormals(priority)&&UsesAuthoredClip(priority)&&PreservesGiFiltering(priority));
 assert(BypassesRR(priority)&&BypassesSpotHistory(priority)&&!UsesRawSpotVisibility(priority));
 assert(!ModeAvailable(priority,kRtSpotReadyBit));mapPrioritySelectorReady=true;
 assert(ModeAvailable(priority,kRtSpotReadyBit));mapPrioritySelectorReady=false;
 assert(ModeAvailable(kDefaultMode,kRtSpotReadyBit));
 assert(blockerStep.mode==Mode::RoomCorrectedNoRR && blockerStep.gpu);
 assert(SupportsLightContextCapture(blockerStep.mode));
 assert(!BlockerSettled(4999,100,true));
 assert(!BlockerSettled(5000,89,true));
 assert(!BlockerSettled(5000,100,false));
 assert(BlockerSettled(5000,90,true));
 assert(!Applied(blockerStep.mode,kAllBits,true));
 assert(Applied(blockerStep.mode,kAllBits,false));
 assert(SupportsLightContextCapture(kDefaultMode)&&SupportsLightContextCapture(Mode::RoomBoth));
 assert(!SupportsLightContextCapture(Mode::Native)&&!SupportsLightContextCapture(Mode::RoomCorrectedVisibility));
 assert(transitionSamples==6 && transitionStep.mode==kDefaultMode && transitionStep.gpu);
 assert(!TransitionSettled(999,60,true,4000,true));
 assert(!TransitionSettled(1000,29,true,4000,true));
 assert(!TransitionSettled(1000,60,false,4000,true));
 assert(TransitionSettled(1000,60,true,0,true));
 assert(!TransitionSettled(1000,60,true,3999,false));
 assert(TransitionSettled(1000,60,true,4000,false));
 assert(count==30 && imagesPerStep*count==90);
 assert(gpu==1);assert(modes.count(Mode::Native)&&modes.count(Mode::RoomAuthoredClip));
 assert(modes.count(Mode::RoomAuthoredSpatial)&&modes.count(Mode::RoomAuthoredRaw));
 assert(!Settled(2499,100,true));assert(!Settled(4000,89,true));assert(!Settled(4000,100,false));assert(Settled(2500,90,true));
 assert(timeoutMs>settleMs+imagesPerStep*burstIntervalMs);
 // Switching lighting must not silently alter specular quality.
 for(unsigned samples:kSpecularSampleChoices) {
  assert(SetSpecularSampleChoice(samples));
  for(auto mode:{Mode::Native,Mode::RoomNative,Mode::RoomBoth,kDefaultMode,Mode::RoomNativeBlend})
   assert(SpecularSampleRequest(mode)==samples);
  assert(!IsSpecularSampleMode(Mode::Native));
  assert(!UsesWorldAlbedo(Mode::Native));
  assert(!PreservesGiFiltering(Mode::Native));
  assert(UsesWorldAlbedo(Mode::RoomNativeBlend));
  assert(PreservesGiFiltering(Mode::RoomNativeBlend));
  assert(SpecularSampleRequest(Mode::RtSpotSpec16)==16);
  assert(SpecularSampleRequest(Mode::RtSpotTemporal2)==2);
 }
 assert(!SetSpecularSampleChoice(3));assert(specularSampleChoice.load()==16);
 assert(NextSpecularSampleChoice(16)==0);assert(NextSpecularSampleChoice(0)==1);
 assert(SetSpecularSampleChoice(4));
}

