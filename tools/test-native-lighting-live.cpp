#include "../src/native_lighting_live_state.h"
#include "../src/rr_broad_diffuse_policy.h"
#include "../src/rr_lighting_diagnostic.h"
#include <thread>
#include <cassert>
#include <iostream>
int main(){
    using namespace native_lighting;
    for(auto mode:{Mode::RoomAuthoredSpatial,Mode::RoomAuthoredRaw}) {
        assert(UsesAuthoredClip(mode) && BypassesSpotHistory(mode));
        assert(IsRtSpotMode(mode) && PreservesGiFiltering(mode));
        assert(!IsSignalDiagnostic(mode) && !ShowsSpotVisibility(mode));
        assert(UsesRawSpotVisibility(mode)==(mode==Mode::RoomAuthoredRaw));
    }
    assert(!BypassesSpotHistory(Mode::RoomAuthoredClip));
    assert(kModeChoices[0].mode==Mode::Native);
    assert(kModeChoices[1].mode==Mode::RoomBoth);
    assert(kModeChoices[2].mode==kDefaultMode);
    assert(kModeChoices[4].mode==Mode::RoomAuthoredClip);
    assert(UsesDirectSurfaceNormals(kDefaultMode)&&UsesAuthoredClip(kDefaultMode));
    LiveSelection live;
    assert(LiveSelection::GetMode(live.Read())==Mode::Native);
    const auto first=live.Select(Mode::DisableBakedGi);
    assert(Disables(LiveSelection::GetMode(first),kGiBit));
    assert(!Disables(LiveSelection::GetMode(first),kDirectBit));
    assert(live.Observe(first,kGiBit));
    const auto second=live.Select(Mode::DisableDirect);
    assert(!live.Observe(first,kDirectBit)); // Old worker cannot confirm a new selection.
    assert(LiveSelection::Observed(live.Read())==0);
    assert(live.Observe(second,kDirectBit));assert(live.Observe(second,kGiBit));
    assert(LiveSelection::Observed(live.Read())==kAllBits);
    const auto native=live.Select(Mode::Native);
    assert(!Disables(LiveSelection::GetMode(native),kGiBit));
    assert(!Disables(LiveSelection::GetMode(native),kDirectBit));
    assert(LiveSelection::Observed(live.Read())==0);
    for (auto mode : {Mode::DisableDirectDiffuse, Mode::DisableDirectSpecular, Mode::BypassShadows, Mode::DisableLightPattern, Mode::BypassScreenShadows, Mode::BypassSpotShadows, Mode::BypassPointShadows, Mode::SpotVarianceLow, Mode::SpotVarianceHigh, Mode::SpotPcfCenter, Mode::SpotPcf32, Mode::SpotStableSoft, Mode::SpotStableTight, Mode::SpotBilinear}) {
        const auto old = live.Read(); const auto next = live.Select(mode);
        assert(LiveSelection::GetMode(next) == mode);
        assert(Disables(mode, kDirectBit) && !Disables(mode, kGiBit));
        assert(!live.Observe(old, kDirectBit));
        assert(live.Observe(next, kDirectBit));
        assert(LiveSelection::Observed(live.Read()) == kDirectBit);
        assert(RequiredVariant(mode) != kDirectBit);
    }
    assert(!CanReportVisual(Mode::SpotStableSoft, 0));
    assert(!CanReportVisual(Mode::SpotStableSoft, kGiBit));
    assert(CanReportVisual(Mode::SpotStableSoft, kDirectBit));
    // Exercise both the unmodified policy and the diagnostic across every effect mask.
    for (unsigned effects=0; effects<64; ++effects) {
        for (bool full : {false,true}) for (bool partial : {false,true}) {
            const bool expected=full && !partial && (effects & (4u|16u));
            assert(control_rr::BroadDiffusePolicy(full,partial,effects).bypass==expected);
            assert(!control_rr::BroadDiffusePolicy(full,partial,effects,true).bypass);
        }
    }
    const auto filtered=Mode::RtSpotDiffuseFiltered;
    assert(PreservesBroadDiffuseFiltering(filtered));
    assert(!PreservesBroadDiffuseFiltering(Mode::RtSpot));
    assert(IsRtSpotMode(filtered) && UsesUnshadowedSpotSelection(filtered));
    assert(!UsesRawSpotVisibility(filtered) && !ShowsSpotVisibility(filtered));
    assert(!Disables(filtered,kGiBit) && !Disables(filtered,kDirectBit));
    assert(RequiredVariant(filtered)==kRtSpotReadyBit);
    assert(LiveSelection::GetMode(live.Select(filtered))==filtered);
    assert(PreservesGiFiltering(Mode::RtSpotGiFiltered));
    assert(!PreservesBroadDiffuseFiltering(Mode::RtSpotGiFiltered));
    assert(!PreservesGiFiltering(Mode::RtSpot));
    assert(IsRtSpotMode(Mode::RtSpotGiFiltered) && UsesUnshadowedSpotSelection(Mode::RtSpotGiFiltered));
    assert(!UsesRawSpotVisibility(Mode::RtSpotGiFiltered) && !Disables(Mode::RtSpotGiFiltered,kGiBit));
    assert(RequiredVariant(Mode::RtSpotGiFiltered)==kRtSpotReadyBit);
    const auto world=Mode::RtSpotWorldAlbedo;
    assert(UsesWorldAlbedo(world) && !UsesWorldAlbedo(Mode::RtSpotGiFiltered));
    assert(PreservesGiFiltering(world) && IsRtSpotMode(world));
    assert(!UsesRawSpotVisibility(world) && !PreservesBroadDiffuseFiltering(world));
    assert(RequiredVariant(world)==kRtSpotReadyBit && !Disables(world,kGiBit));
    assert(LiveSelection::GetMode(live.Select(world))==world);
    const auto specular=Mode::RtSpotSpecularFiltered;
    assert(PreservesSpecularFiltering(specular) && !PreservesSpecularFiltering(world));
    assert(UsesWorldAlbedo(specular) && PreservesGiFiltering(specular));
    assert(IsRtSpotMode(specular) && RequiredVariant(specular)==kRtSpotReadyBit);
    assert(!UsesRawSpotVisibility(specular) && !PreservesBroadDiffuseFiltering(specular));
    assert(!Disables(specular,kDirectBit) && !Disables(specular,kGiBit));
    assert(LiveSelection::GetMode(live.Select(specular))==specular);
    const auto angle=Mode::RtSpotViewAngle;
    assert(UsesPerspectiveGuide(angle) && !UsesPerspectiveGuide(world));
    assert(UsesWorldAlbedo(angle) && PreservesGiFiltering(angle));
    assert(!PreservesSpecularFiltering(angle) && !PreservesBroadDiffuseFiltering(angle));
    assert(IsRtSpotMode(angle) && !UsesRawSpotVisibility(angle));
    assert(RequiredVariant(angle)==kRtSpotReadyBit && !Disables(angle,kGiBit));
    assert(LiveSelection::GetMode(live.Select(angle))==angle);
    assert(kModeCount==9);
    for(auto mode:{Mode::RtSpotGi2,Mode::RtSpotGi4,Mode::RtSpotGi8,Mode::RtSpotGiMenu,Mode::RtSpotGi12,Mode::RtSpotGi16}){
        const auto old=live.Read();const auto next=live.Select(mode);
        assert(LiveSelection::GetMode(next)==mode);
        assert(!live.Observe(old,kDirectBit));assert(live.Observe(next,kDirectBit));
        assert(LiveSelection::GetMode(live.Read())==mode);
        assert(IsRtSpotMode(mode)&&UsesWorldAlbedo(mode)&&PreservesGiFiltering(mode));
        assert(!UsesPerspectiveGuide(mode)&&!PreservesSpecularFiltering(mode)&&!UsesRawSpotVisibility(mode));
        assert(RequiredVariant(mode)==kRtSpotReadyBit&&!Disables(mode,kDirectBit));
    }
    assert(GiSampleRequest(Mode::RtSpotGi2,8)==2&&GiSampleRequest(Mode::RtSpotGi4,8)==4);
    assert(GiSampleRequest(Mode::RtSpotGi8,2)==8&&GiSampleRequest(Mode::RtSpotGiMenu,8)==0);
    assert(GiSampleRequest(Mode::RtSpotWorldAlbedo,6)==6);
    assert(GiSampleRequest(Mode::RtSpotGi12,4)==12&&GiSampleRequest(Mode::RtSpotGi16,4)==16);
    for(auto mode:{Mode::RoomNative,Mode::RoomGi,Mode::RoomSpot,Mode::RoomBoth}){
        assert(SpecularSampleRequest(mode)==4&&GiSampleRequest(mode,6)==6&&UsesWorldAlbedo(mode));
        assert(!Disables(mode,kGiBit)&&!Disables(mode,kDirectBit));
        assert(IsRtSpotMode(mode)==(mode==Mode::RoomSpot||mode==Mode::RoomBoth));
        assert(PreservesGiFiltering(mode)==(mode==Mode::RoomGi||mode==Mode::RoomBoth));
        assert(!UsesSpecularTemporalOnly(mode));
        assert(LiveSelection::GetMode(live.Select(mode))==mode);
    }
    specularSamplesReady.store(true);
    for(auto mode:{Mode::RtSpotTemporal2,Mode::RtSpotTemporal4}){
        control_rr_temporal::ready.store(false);assert(!ModeAvailable(mode,kRtSpotReadyBit));
        control_rr_temporal::ready.store(true);assert(ModeAvailable(mode,kRtSpotReadyBit));
        assert(UsesSpecularTemporalOnly(mode)&&!PreservesSpecularFiltering(mode)&&PreservesGiFiltering(mode));
        assert(UsesWorldAlbedo(mode)&&IsRtSpotMode(mode));
    }
    for(auto mode:{Mode::RoomNativeSelection,Mode::RoomOpen,Mode::RoomSelectedOff}){
        assert(SpecularSampleRequest(mode)==4&&GiSampleRequest(mode,6)==6);
        assert(IsRtSpotMode(mode)&&PreservesGiFiltering(mode)&&UsesWorldAlbedo(mode));
        assert(!Disables(mode,kDirectBit)&&!UsesSpecularTemporalOnly(mode));
        assert(UsesUnshadowedSpotSelection(mode)==(mode!=Mode::RoomNativeSelection));
        assert(LiveSelection::GetMode(live.Select(mode))==mode);
    }
    spotOpenReady.store(false);
    assert(!ModeAvailable(Mode::RoomOpen,kRtSpotReadyBit));
    assert(ModeAvailable(Mode::RoomBoth,kRtSpotReadyBit));
    spotOpenReady.store(true);
    assert(ModeAvailable(Mode::RoomOpen,kRtSpotReadyBit));
    for(auto mode:{Mode::RoomRaw,Mode::RoomBlocker}){
        assert(IsRoomMode(mode)&&SpecularSampleRequest(mode)==4&&GiSampleRequest(mode,6)==6);
        assert(IsRtSpotMode(mode)&&PreservesGiFiltering(mode)&&UsesWorldAlbedo(mode));
        assert(UsesRawSpotVisibility(mode));
        assert(ShowsSpotVisibility(mode)==(mode==Mode::RoomBlocker));
        assert(!UsesSpecularTemporalOnly(mode)&&!Disables(mode,kDirectBit));
        assert(LiveSelection::GetMode(live.Select(mode))==mode);
    }
    assert(IsRtSpotMode(Mode::RoomAuthoredClip)&&PreservesGiFiltering(Mode::RoomAuthoredClip));
    assert(!UsesRawSpotVisibility(Mode::RoomAuthoredClip)&&!ShowsSpotVisibility(Mode::RoomAuthoredClip)&&!IsSignalDiagnostic(Mode::RoomAuthoredClip));
    assert(SpecularSampleRequest(Mode::RoomAuthoredClip)==SpecularSampleRequest(Mode::RoomBoth));
    assert(!ModeAvailable(Mode::RoomAuthoredClip,kRtSpotReadyBit));
    spotAuthoredClipReady.store(true);
    assert(ModeAvailable(Mode::RoomAuthoredClip,kRtSpotReadyBit));
    assert(LiveSelection::GetMode(live.Select(Mode::RoomAuthoredClip))==Mode::RoomAuthoredClip);
    spotBlockerReady.store(false);
    assert(!ModeAvailable(Mode::RoomBlocker,kRtSpotReadyBit));
    assert(ModeAvailable(Mode::RoomBoth,kRtSpotReadyBit));
    spotBlockerReady.store(true);
    assert(!ModeAvailable(Mode::RoomBlocker,kRtSpotReadyBit));
    spotBandColorsReady.store(true);
    assert(ModeAvailable(Mode::RoomBlocker,kRtSpotReadyBit));
    for(auto mode:{Mode::RoomHitX,Mode::RoomHitY,Mode::RoomHitZ}){
        assert(IsSignalDiagnostic(mode)&&UsesRawSpotVisibility(mode)&&ShowsSpotVisibility(mode));
        assert(IsRtSpotMode(mode)&&SpecularSampleRequest(mode)==4);
        spotHitPositionReady.store(false);assert(!ModeAvailable(mode,kRtSpotReadyBit));
        spotHitPositionReady.store(true);assert(ModeAvailable(mode,kRtSpotReadyBit));
    }
    // UI snapshots must not reset or lose concurrent render-side observations.
    control_rr_diagnostic::Counters counters;
    using Event=control_rr_diagnostic::Event;
    std::thread worker([&]{for(unsigned i=0;i<10000;++i)counters.Record(Event::RRSuccess);});
    std::uint64_t previous=0;
    for(unsigned i=0;i<10000;++i){
        counters.Record(Event::BroadA);
        const auto snapshot=counters.Snapshot();
        const auto current=snapshot[static_cast<std::size_t>(Event::RRSuccess)];
        assert(current>=previous);previous=current;
    }
    worker.join();
    const auto totals=counters.Snapshot();
    assert(totals[static_cast<std::size_t>(Event::RRSuccess)]==10000);
    assert(totals[static_cast<std::size_t>(Event::BroadA)]==10000);
    assert(totals[static_cast<std::size_t>(Event::BroadB)]==0);
    std::cout<<"PASS: live modes, return to native, stale-recording rejection\n";
}
