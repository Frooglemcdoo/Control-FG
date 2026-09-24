#include <cassert>
#include <cstdint>
#include <cstdio>
#define __try try
#define __except(x) catch(...)
using UINT=unsigned;using DWORD=unsigned long;
static DWORD GetExceptionCode(){return 123;}
struct ID3D12Resource{unsigned id;};
constexpr unsigned D3D12_RESOURCE_BARRIER_TYPE_TRANSITION=0,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES=~0u,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE=64,D3D12_RESOURCE_STATE_COPY_SOURCE=2048;
constexpr unsigned D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX=0,D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT=1;
struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT {unsigned Offset=0;};
struct D3D12_TEXTURE_COPY_LOCATION {ID3D12Resource* pResource=nullptr;unsigned Type=0;D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint{};};
struct D3D12_RESOURCE_BARRIER {unsigned Type=0;struct{ID3D12Resource* pResource=nullptr;unsigned Subresource=0,StateBefore=0,StateAfter=0;}Transition;};
static unsigned seen=0,failAt=0;static unsigned long long* counter;
static void step(){++seen;assert(*counter==seen);if(seen==failAt)throw 1;}
struct List {
 void ResourceBarrier(unsigned n,D3D12_RESOURCE_BARRIER* b){step();assert(n==3);for(unsigned i=0;i<3;++i){assert(b[i].Transition.pResource->id==i);assert(b[i].Transition.StateBefore==(seen==1?64u:2048u));assert(b[i].Transition.StateAfter==(seen==1?2048u:64u));}}
 void CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION* to,unsigned x,unsigned y,unsigned z,D3D12_TEXTURE_COPY_LOCATION* from,void* box){step();assert(!x&&!y&&!z&&!box);assert(to->pResource->id==from->pResource->id+3);assert(from->Type==0&&to->Type==1);assert(to->PlacedFootprint.Offset==from->pResource->id*512);}
};
struct RRLiveCapture {ID3D12Resource* readback[3];D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[3];};
struct RRLiveSlot {ID3D12Resource* normal;ID3D12Resource* specular;ID3D12Resource* diffuse;};
struct RRGuideInputSnapshot {List* commandList;void* commandContext;};
#include "../../src/rr_live_capture_record.h"
#include "../../src/rr_live_capture_policy.h"
int main(){
 ID3D12Resource resources[6]{{0},{1},{2},{3},{4},{5}};RRLiveSlot slot{resources,resources+1,resources+2};RRLiveCapture cap{{resources+3,resources+4,resources+5},{{0},{512},{1024}}};List list;struct{unsigned long long reserved=0,count=0;}context;RRGuideInputSnapshot in{&list,&context};counter=&context.count;
 for(unsigned fail=0;fail<=5;++fail){seen=0;failAt=fail;context.count=0;DWORD fault=0;assert(RRLiveCaptureRecord(&cap,&slot,&in,&fault)==(fail==0));assert(seen==(fail?fail:5));}
 control_rr::LiveCapturePolicy policy;assert(!policy.Retire(100));assert(policy.Begin(1,42));policy.Recorded(true);policy.Submit(2,42,7);assert(!policy.Retire(100));policy.Submit(1,43,7);assert(!policy.Retire(100));policy.Submit(1,42,7);assert(!policy.Retire(6));assert(policy.Retire(7));assert(!policy.Retire(8));assert(!policy.Begin(1,44));
 control_rr::LiveCapturePolicy lost;assert(lost.Begin(0,1));lost.Recorded(true);lost.Submit(0,1,1);assert(!lost.Retire(UINT64_MAX));
 control_rr::LiveCapturePolicy failed;assert(failed.Begin(0,1));failed.Recorded(false);failed.Submit(0,1,1);assert(!failed.Retire(1));
 puts("PASS: actual five-command owned-output copy sequence and fault points; capture waits for matching slot serial and fence, rejects device loss, exports at most once");
}
