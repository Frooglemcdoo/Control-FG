#include "../src/gi_reuse/auto_capture_region.h"
#include "../src/gi_reuse/auto_temporal_audit.h"
#include <cassert>
#include <iostream>
int main(){using namespace gi_reuse;
 auto r=GIAuditRegion(2560,1440,200,500);assert(r.x==256&&r.y==464&&r.w==512&&r.h==512);
 r=GIAuditRegion(2560,1440,0,0);assert(r.x==0&&r.y==0);
 r=GIAuditRegion(2560,1440,1000,1000);assert(r.x==2048&&r.y==928);
 r=GIAuditRegion(19,17,1000,1000);assert(r.x==0&&r.y==0&&r.w==19&&r.h==17);
 for(unsigned w:{512u,1024u,2560u,16384u})for(unsigned h:{512u,768u,1440u,16384u})for(unsigned c:{0u,200u,500u,1000u}){
  r=GIAuditRegion(w,h,c,1000-c);assert(r.x+r.w<=w&&r.y+r.h<=h);
  const auto step=GIAuditStepControl(256,r.x),capacity=GIAuditCapacityControl(65535,r.y,true);
  assert((step&511)==256&&((step>>9)&16383)==r.x&&(capacity&65535)==65535&&(capacity&65536)!=0&&(capacity>>17)==r.y);
 }
 GIAuditPairCache cache;GIAutoPixelAudit pixel{};pixel.flags=7;GIAuditTemporalSummary s;unsigned first=0,second=0;
 assert(!cache.Add(600,10,1,1,1,&pixel,s,first,second,1,1));
 assert(!cache.Add(601,11,1,1,1,&pixel,s,first,second,2,1));
 std::cout<<"PASS movable ROI edges, scaled-input bounds, packed-root round trips, changed-origin temporal rejection.\n";
}
