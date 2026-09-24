#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#define RR_SEH_TRY try
#define RR_SEH_EXCEPT(x) catch(...)
using UINT=unsigned;using DWORD=unsigned long;
constexpr UINT D3D12_RESOURCE_BARRIER_TYPE_TRANSITION=0,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES=~0u,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE=64,D3D12_RESOURCE_STATE_COPY_SOURCE=2048,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX=0,D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT=1;
struct ID3D12Resource{};struct Footprint{unsigned marker;};
struct D3D12_TEXTURE_COPY_LOCATION{ID3D12Resource* pResource;UINT Type;Footprint PlacedFootprint;};
struct D3D12_RESOURCE_BARRIER{UINT Type;struct{ID3D12Resource*pResource;UINT Subresource,StateBefore,StateAfter;}Transition;};
static ID3D12Resource sources[5],outputs[5];static unsigned seen=0,failAt=0,total=0;
static unsigned long long* counter=nullptr;
static DWORD GetExceptionCode(){return 123;}
static void Step(){++seen;assert(*counter==seen);if(failAt==seen)throw std::runtime_error("command fault");}
struct List{
 void ResourceBarrier(UINT n,D3D12_RESOURCE_BARRIER*b){Step();assert(n==total);for(UINT i=0;i<n;++i){assert(b[i].Transition.pResource==&sources[i]);assert(b[i].Transition.Subresource==~0u);assert(b[i].Transition.StateBefore==(seen==1?64u:2048u));assert(b[i].Transition.StateAfter==(seen==1?2048u:64u));}}
 void CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION*d,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION*s,void*box){Step();assert(!x&&!y&&!z&&!box);assert(s->Type==0&&d->Type==1);assert(s->pResource==&sources[seen-2]);assert(d->pResource==&outputs[seen-2]);assert(d->PlacedFootprint.marker==seen-2);}
};
struct RRInputCaptureJob{unsigned count=0;ID3D12Resource*sources[5]{},*readback[5]{};Footprint footprint[5]{};};
struct RRGuideInputSnapshot{List*commandList;void*commandContext;};
#include "../../src/rr_input_capture_record.h"
int main(){List list;RRInputCaptureJob job;struct{unsigned long long unused,count;}context{};RRGuideInputSnapshot native{&list,&context};counter=&context.count;
 for(unsigned n:{3u,5u}){total=job.count=n;for(unsigned i=0;i<n;++i){job.sources[i]=&sources[i];job.readback[i]=&outputs[i];job.footprint[i].marker=i;}
 for(unsigned fault=0;fault<=n+2;++fault){seen=0;failAt=fault;context.count=0;DWORD code=0;const bool okay=RRInputCaptureRecord(&job,&native,&code);assert(okay==(fault==0));assert(seen==(fault?fault:n+2));assert(code==(fault?DWORD{123}:DWORD{0}));}}
 puts("PASS actual C1 recorder: E/F buffer copies, state restoration, native command count, every recording fault");}
