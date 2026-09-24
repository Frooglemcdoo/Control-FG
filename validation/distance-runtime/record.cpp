#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#define __except(x) catch(...)
using UINT=unsigned;using DWORD=unsigned long;using UINT64=std::uint64_t;
constexpr UINT D3D12_RESOURCE_BARRIER_TYPE_TRANSITION=0,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES=~0u,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE=64,D3D12_RESOURCE_STATE_UNORDERED_ACCESS=8,D3D12_RESOURCE_STATE_COPY_SOURCE=2048,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX=0,D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT=1;
struct ID3D12Resource{};struct Footprint{unsigned marker;};
struct D3D12_TEXTURE_COPY_LOCATION{ID3D12Resource* pResource;UINT Type;Footprint PlacedFootprint;};
struct D3D12_RESOURCE_BARRIER{UINT Type;struct{ID3D12Resource*pResource;UINT Subresource,StateBefore,StateAfter;}Transition;};
struct Heap{UINT64 GetGPUDescriptorHandleForHeapStart(){return 90;}};
static ID3D12Resource output,status,readback0,readback1;static Heap heap;
static unsigned seen=0,failAt=0;static bool capture=false;static unsigned long long* counter=nullptr;
static DWORD GetExceptionCode(){return 123;}
static void Step(){++seen;assert(*counter==seen);if(failAt==seen)throw std::runtime_error("command fault");}
struct List{
 void ResourceBarrier(UINT n,D3D12_RESOURCE_BARRIER*b){Step();assert(n==2);assert(b[0].Transition.pResource==&output&&b[1].Transition.pResource==&status);for(UINT i=0;i<2;++i){assert(b[i].Transition.Subresource==~0u);assert(b[i].Transition.StateBefore==(seen==1?64u:(capture?2048u:8u)));assert(b[i].Transition.StateAfter==(seen==1?(capture?2048u:8u):64u));}}
 void SetDescriptorHeaps(UINT n,Heap** p){Step();assert(n==1&&*p==&heap);}
 void SetComputeRootSignature(void*p){Step();assert(p==reinterpret_cast<void*>(1));}
 void SetPipelineState(void*p){Step();assert(p==reinterpret_cast<void*>(2));}
 void SetComputeRootDescriptorTable(UINT index,UINT64 handle){Step();assert(index==0&&handle==90);}
 void SetComputeRoot32BitConstants(UINT index,UINT n,const void*,UINT offset){Step();assert(index==1&&n==36&&offset==0);}
 void Dispatch(UINT x,UINT y,UINT z){Step();assert(x==321&&y==181&&z==1);}
 void CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION*d,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION*s,void*box){Step();assert(!x&&!y&&!z&&!box);assert(s->Type==0&&d->Type==1);assert(s->pResource==(seen==2?&output:&status));assert(d->pResource==(seen==2?&readback0:&readback1));assert(d->PlacedFootprint.marker==seen-2);}
};
struct RRDistanceConstants{float values[36];};
struct RRDistanceSlot{Heap*heap;ID3D12Resource*output;ID3D12Resource*status;};
struct RRDistanceOwner{void*root=reinterpret_cast<void*>(1);void*pipeline=reinterpret_cast<void*>(2);UINT width=2561,height=1441;ID3D12Resource*readback[2]{&readback0,&readback1};Footprint footprint[2]{{0},{1}};};
struct RRGuideInputSnapshot{List*commandList;void*commandContext;};
#include "../../src/rr_distance_record.h"
int main(){List list;RRDistanceOwner owner;RRDistanceSlot slot{&heap,&output,&status};struct{unsigned long long unused,count;}context{};RRGuideInputSnapshot native{&list,&context};RRDistanceConstants constants{};counter=&context.count;
 for(unsigned mode=0;mode<2;++mode){capture=mode!=0;const unsigned total=capture?4:8;for(unsigned fault=0;fault<=total;++fault){seen=0;failAt=fault;context.count=0;DWORD code=0;bool okay=capture?RRDistanceCaptureLeaf(&owner,&slot,&native,&code):RRDistanceRecordLeaf(&owner,&slot,&native,&constants,&code);assert(okay==(fault==0));assert(seen==(fault?fault:total));assert(code==(fault?123:0));}}
 puts("PASS distance dispatch and readback: exact descriptors/constants, output transitions, native command counter and all 12 fault points");}
