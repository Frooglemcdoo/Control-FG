#pragma once
namespace native_blend_capture {
enum class Failure { None, Busy, Mode, RR, ContactShadows, HitDistance, RayCapture, Composition, Pipeline, PreviousFailed, ExportPending, Files };
struct Admission {bool busy,nativeBlend,rr,contactShadows,hitDistance,rayCapture,composition,pipeline,previousFailed,exportPending;};
inline constexpr Failure Check(Admission a) noexcept {
 return a.busy?Failure::Busy:!a.contactShadows?Failure::ContactShadows:
  !a.nativeBlend?Failure::Mode:!a.rr?Failure::RR:a.hitDistance?Failure::HitDistance:
  !a.rayCapture?Failure::RayCapture:!a.composition?Failure::Composition:!a.pipeline?Failure::Pipeline:
  a.previousFailed?Failure::PreviousFailed:a.exportPending?Failure::ExportPending:Failure::None;
}
inline const wchar_t* Message(Failure f) noexcept {
 switch(f){
 case Failure::None:return L"Ready";
 case Failure::Busy:return L"Another lighting capture is active. Wait for it to finish.";
 case Failure::Mode:return L"Select NATIVE BLEND before TRACE SHADOW. The capture will keep that mode.";
 case Failure::RR:return L"Enable Ray Reconstruction before capturing Native Blend.";
 case Failure::ContactShadows:return L"Enable Ray Traced Contact Shadows in the game settings, then return to gameplay. The ray capture needs that pipeline.";
 case Failure::HitDistance:return L"Restart with hit distance OFF for the accepted Native Blend baseline. It is now OFF by default.";
 case Failure::RayCapture:return L"Ray capture is not ready. Return to gameplay; if this persists, SAVE LOG.";
 case Failure::Composition:return L"Lighting composition capture is not ready. Return to gameplay; if this persists, SAVE LOG.";
 case Failure::Pipeline:return L"Native Blend shader pipelines are not ready. Return to gameplay; if this persists, SAVE LOG.";
 case Failure::PreviousFailed:return L"The previous GPU capture failed. SAVE LOG, then restart before capturing again.";
 case Failure::ExportPending:return L"The previous GPU capture is still exporting. Wait before starting another.";
 default:return L"Could not create or open the capture files. SAVE LOG for diagnosis.";
 }
}
inline constexpr bool SettingsMatch(bool rr,bool savedRR,unsigned samples,unsigned savedSamples,unsigned effects,unsigned savedEffects,bool distance,bool savedDistance) noexcept {
 return rr==savedRR&&samples==savedSamples&&effects==savedEffects&&distance==savedDistance;
}
}
