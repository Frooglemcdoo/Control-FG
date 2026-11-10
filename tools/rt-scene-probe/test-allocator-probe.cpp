#include "../../src/rt_allocator_probe.h"
#include "../../src/rt_allocator_probe_patch.h"
#include <cassert>
#include <cstdio>
using namespace rt_allocator_probe;
static unsigned depth=0,enters=0,leaves=0,captures=0,clocks=0;
static U32 lastError=7;
static U64 ticks[]={10,30,80,85};
static Snapshot snapshot{reinterpret_cast<void*>(1),4,4,100,2};
static void EnterLock(void*){++depth;++enters;assert(lastError==7);lastError=11;}
static void LeaveLock(void*){assert(depth);--depth;++leaves;assert(lastError==11);lastError=13;}
static U64 Clock(){assert(clocks<4);lastError=999;return ticks[clocks++];}
static U32 Error(){return lastError;}
static void Restore(U32 value){lastError=value;}
static Snapshot Capture(void*){assert(depth);++captures;lastError=998;return snapshot;}
int main(){
 Api api{EnterLock,LeaveLock,Clock,Error,Restore,Capture};Sample sample;int lock=0;
 Enter(api,&sample,&lock);assert(lastError==11&&depth==1&&captures==1);
 snapshot.cursor+=49;Leave(api,&sample,&lock);
 assert(lastError==13&&!depth&&enters==1&&leaves==1&&captures==2);
 assert(sample.wait==20&&sample.hold==50&&sample.release==5&&sample.left&&Classify(sample)==0);
 sample.after.heap=reinterpret_cast<void*>(2);assert(Classify(sample)==1);sample.after=sample.before;
 sample.before.heapVersion=3;assert(Classify(sample)==1);sample.before.heapVersion=4;
 sample.after.cursor=99;assert(Classify(sample)==1);sample.after.cursor=150;assert(Classify(sample)==2);
 sample.before.cursor=UINT32_MAX-10;sample.after.cursor=UINT32_MAX;assert(Classify(sample)==2);
 lastError=7;Enter(api,nullptr,&lock);Leave(api,nullptr,&lock);assert(clocks==4&&captures==2);
 lastError=7;Enter(api,&sample,&lock);Leave(api,&sample,&lock);assert(clocks==4&&captures==2&&!depth);
 unsigned char code[6]{};assert(Call6(0x1000,0x2000,code));assert(code[0]==0xe8&&code[5]==0x90);
 std::int32_t displacement=0;std::memcpy(&displacement,code+1,4);assert(displacement==0xffb);
 assert(Call6(0x80000000,5,code));std::memcpy(&displacement,code+1,4);assert(displacement==INT32_MIN);
 assert(Call6(0x1000,0x80001004,code));assert(!Call6(0x1000,0x80001005,code));
 assert(!Call6(UINTPTR_MAX-3,0,code));
 std::puts("allocator timing, protected snapshots, passthrough and patch bounds passed");
}
