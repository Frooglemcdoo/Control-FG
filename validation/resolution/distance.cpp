#include <cassert>
#include <cstdint>
#include <cstdio>
#include "rr_live_capture_policy.h"
struct Resource {unsigned releases=0;void Release(){assert(!releases);++releases;}};
template<class T>static void RRGuideRelease(T*& p){if(p)p->Release();p=nullptr;}
struct RRDistanceOwner {
 bool preparing=false,stopped=false,exporting=false,prepared=true;
 unsigned long long lastUseEpoch=5,recorded=99;
 control_rr::LiveCapturePolicy capturePolicy;
 struct{Resource* output=nullptr;Resource* status=nullptr;Resource* heap=nullptr;}slots[3];
 Resource* readback[2]{};int captured=5;
};
#include "rr_distance_resize.h"
int main(){
 RRDistanceOwner c;assert(!RRDistanceCanResize(&c,5));assert(RRDistanceCanResize(&c,6));
 c.preparing=true;assert(!RRDistanceCanResize(&c,6));c.preparing=false;
 c.stopped=true;assert(!RRDistanceCanResize(&c,6));c.stopped=false;
 assert(c.capturePolicy.Begin(0,9));assert(!RRDistanceCanResize(&c,6));
 c.capturePolicy.Recorded(true);assert(!RRDistanceCanResize(&c,6));
 c.capturePolicy.Submit(0,9,99);assert(!RRDistanceCanResize(&c,6));
 assert(!c.capturePolicy.Retire(98));assert(!RRDistanceCanResize(&c,6));
 assert(c.capturePolicy.Retire(99));c.exporting=true;assert(!RRDistanceCanResize(&c,6));
 c.exporting=false;assert(RRDistanceCanResize(&c,6));
 Resource resources[11];unsigned i=0;
 for(auto& slot:c.slots){slot.output=&resources[i++];slot.status=&resources[i++];slot.heap=&resources[i++];}
 for(auto& p:c.readback)p=&resources[i++];
 RRDistanceReleaseOutputs(&c);assert(!c.prepared&&c.recorded==0&&c.captured==0&&c.capturePolicy.state==control_rr::CaptureState::Armed);
 for(auto& p:resources)assert(p.releases==1);
 RRDistanceReleaseOutputs(&c);for(auto& p:resources)assert(p.releases==1);
 puts("PASS actual distance resize: old reflection epoch rejected, in-flight diagnostic/export held, outputs released once, counters/capture reset");
}
