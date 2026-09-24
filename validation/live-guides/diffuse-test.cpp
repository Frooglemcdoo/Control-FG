#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <vector>
#define __except(x) catch(...)
using UINT=unsigned;using DWORD=unsigned long;
constexpr UINT D3D12_RESOURCE_BARRIER_TYPE_TRANSITION=0,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES=~0u,
 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE=64,D3D12_RESOURCE_STATE_RENDER_TARGET=4,
 D3D12_RESOURCE_STATE_COPY_SOURCE=2048,D3D12_RESOURCE_STATE_COPY_DEST=1024,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX=0;
struct uint4 {unsigned x,y,z,w;};
#include "../../src/shaders/rr_diffuse_bits.hlsli"
#include "../../src/rr_part1_copy_policy.h"
struct ID3D12Resource {std::vector<std::uint16_t> pixels;};
struct D3D12_RESOURCE_BARRIER {UINT Type=0;struct {ID3D12Resource* pResource=nullptr;UINT Subresource=0,StateBefore=0,StateAfter=0;} Transition;};
struct D3D12_TEXTURE_COPY_LOCATION {ID3D12Resource* pResource=nullptr;UINT Type=0;};
static unsigned mask=0,attempts=0;static unsigned long long* counter=nullptr;static DWORD GetExceptionCode(){return 123;}
struct List {
 void CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION* to,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION* from,void* box){
  ++attempts;assert(*counter==attempts);assert(!x&&!y&&!z&&!box);if(mask&2)throw std::runtime_error("copy");to->pResource->pixels=from->pResource->pixels;
 }
 void ResourceBarrier(UINT n,D3D12_RESOURCE_BARRIER* b){
  ++attempts;assert(*counter==attempts);assert(n==2);const bool initial=b[0].Transition.StateBefore==4;
  assert(b[0].Transition.StateAfter==(initial?2048u:4u));assert(b[1].Transition.StateBefore==(initial?64u:1024u));assert(b[1].Transition.StateAfter==(initial?1024u:64u));
  if(mask&(initial?1u:4u))throw std::runtime_error("barrier");
 }
};
struct RRGuideInputSnapshot {List* commandList;void* commandContext;};
#include "../../src/rr_diffuse_copy.h"
int main(){
 ID3D12Resource from,to;from.pixels.resize(65536*4);for(unsigned i=0;i<65536;++i){from.pixels[i*4]=std::uint16_t(i);from.pixels[i*4+1]=std::uint16_t(65535-i);from.pixels[i*4+2]=std::uint16_t(i^0x5555);from.pixels[i*4+3]=std::uint16_t(i);}
 List list;struct Context{unsigned long long reserved=0,count=0;} context;RRGuideInputSnapshot in{&list,&context};counter=&context.count;
 for(mask=0;mask<8;++mask){context.count=attempts=0;DWORD fault=0;const bool okay=RRDiffuseCopy(&in,&from,&to,&fault);assert(okay==(mask==0));assert(attempts==((mask&1)?1u:3u));}
 for(unsigned i=0;i<65536;++i){const auto* a=&to.pixels[i*4];const auto b=ControlDiffuseAlphaOne({a[0],a[1],a[2],a[3]});assert(b.x==a[0]&&b.y==a[1]&&b.z==a[2]&&b.w==0x3c00);}
 puts("PASS: actual diffuse copy emitter, eight failure combinations, restoration after copy failure, no ambiguous barrier retry; shared shader alpha operation preserves all 65536 half-bit patterns");
}
