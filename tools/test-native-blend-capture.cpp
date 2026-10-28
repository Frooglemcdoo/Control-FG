#include "../src/rr_native_blend_capture_policy.h"
#include "../src/rr_compact_distance_policy.h"
#include <cassert>
#include <cstdio>
int main(){using namespace native_blend_capture;
 const Admission valid{false,true,true,true,false,true,true,true,false,false};
 assert(Check(valid)==Failure::None);
 auto a=valid;a.busy=true;assert(Check(a)==Failure::Busy);
 a=valid;a.nativeBlend=false;assert(Check(a)==Failure::Mode);
 a=valid;a.rr=false;assert(Check(a)==Failure::RR);
 a=valid;a.contactShadows=false;assert(Check(a)==Failure::ContactShadows);
 a=valid;a.contactShadows=false;a.nativeBlend=false;assert(Check(a)==Failure::ContactShadows);
 a=valid;a.hitDistance=true;assert(Check(a)==Failure::HitDistance);
 a=valid;a.rayCapture=false;assert(Check(a)==Failure::RayCapture);
 a=valid;a.composition=false;assert(Check(a)==Failure::Composition);
 a=valid;a.pipeline=false;assert(Check(a)==Failure::Pipeline);
 a=valid;a.previousFailed=true;assert(Check(a)==Failure::PreviousFailed);
 a=valid;a.exportPending=true;assert(Check(a)==Failure::ExportPending);
 assert(SettingsMatch(true,true,0,0,0x6b,0x6b,false,false));
 assert(!SettingsMatch(false,true,0,0,0x6b,0x6b,false,false));
 assert(!SettingsMatch(true,true,4,0,0x6b,0x6b,false,false));
 assert(!SettingsMatch(true,true,0,0,0x21,0x6b,false,false));
 assert(!SettingsMatch(true,true,0,0,0x6b,0x6b,true,false));
 assert(!rr_compact_distance::Parse(nullptr)&&!rr_compact_distance::Parse("")&&rr_compact_distance::Parse("on"));
 puts("PASS Native Blend admission, precise failure reasons, settings continuity and hit-distance default OFF");
}
