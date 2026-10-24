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
 assert(gpu==1);assert(modes.count(Mode::Native)&&modes.count(Mode::RoomAuthoredClip));
 assert(modes.count(Mode::RoomAuthoredSpatial)&&modes.count(Mode::RoomAuthoredRaw));
 assert(!Settled(2499,100,true));assert(!Settled(4000,89,true));assert(!Settled(4000,100,false));assert(Settled(2500,90,true));
 assert(timeoutMs>settleMs+imagesPerStep*burstIntervalMs);
}
