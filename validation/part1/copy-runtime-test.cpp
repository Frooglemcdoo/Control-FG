// Compile the exact command emitter with fake D3D objects; no driver/ABI claim.
#include "../../src/rr_part1_copy_policy.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
using DWORD=unsigned int;
using D3D12_RESOURCE_STATES=unsigned;
constexpr unsigned D3D12_RESOURCE_STATE_COPY_SOURCE=0x800;
constexpr unsigned D3D12_RESOURCE_BARRIER_TYPE_TRANSITION=0;
constexpr unsigned D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES=~0u;
struct ID3D12Resource { unsigned state=0xC0;unsigned char data[16]{}; };
struct D3D12_RESOURCE_BARRIER {
 unsigned Type=0;
 struct {ID3D12Resource* pResource=nullptr;unsigned Subresource=0,StateBefore=0,StateAfter=0;} Transition;
};
static DWORD GetExceptionCode(){return 123;}
struct List {
 unsigned calls=0,fail=0;bool failAfter=false;
 void before(){++calls;if(calls==fail && !failAfter)throw 1;}
 void after(){if(calls==fail && failAfter)throw 1;}
 void ResourceBarrier(unsigned n,const D3D12_RESOURCE_BARRIER* b){
  before();assert(n==1 && b->Type==0 && b->Transition.Subresource==~0u);
  assert(b->Transition.pResource->state==b->Transition.StateBefore);
  assert(b->Transition.StateAfter==0x8C0 || b->Transition.StateAfter==0xC0);
  b->Transition.pResource->state=b->Transition.StateAfter;after();
 }
 void CopyBufferRegion(ID3D12Resource* dst,unsigned long long d,ID3D12Resource* src,unsigned long long s,unsigned long long bytes){
  before();assert(d==0 && s==0 && bytes==16 && src->state==0x8C0);
  std::memcpy(dst->data,src->data,static_cast<std::size_t>(bytes));after();
 }
};
struct RRGuideJob {ID3D12Resource* part1Readback=nullptr;ID3D12Resource* part1Source=nullptr;unsigned long long part1Bytes=16;};
struct RRGuideInputSnapshot {void* commandContext=nullptr;List* commandList=nullptr;};
#define __try try
#define __except(x) catch(...)
#define EXCEPTION_EXECUTE_HANDLER 1
#define CONTROL_RR_PART1_EMIT_ONLY
#include "../../src/rr_part1_readback.h"
int main(){
 for(unsigned fail=0;fail<=3;++fail)for(bool after:{false,true}){
  ID3D12Resource src{},dst{};for(unsigned i=0;i<16;++i)src.data[i]=static_cast<unsigned char>(i*11);
  RRGuideJob job{&dst,&src,16};List list{};list.fail=fail;list.failAfter=after;
  unsigned long long context[2]{};RRGuideInputSnapshot snapshot{context,&list};
  control_rr_part1::CopyProgress p{};DWORD fault=0;
  const bool result=RRPart1RecordCommands(&job,&snapshot,&p,&fault);
  assert(result==(fail==0) && context[1]==list.calls);
  assert(fault==(fail?123u:0u));assert(list.calls==(fail==1?1u:3u));
  if(fail==0 || fail==2)assert(src.state==0xC0 && p.restored);
  if(result)assert(std::memcmp(src.data,dst.data,16)==0);
  if(fail==1)assert(!p.restoreAttempted);
  assert(!control_rr_part1::CanMap(result,0));
 }
 std::cout<<"PASS: exact emitter, byte copy, C0/8C0 transitions, faults before/after each command, native command count\n";
}
