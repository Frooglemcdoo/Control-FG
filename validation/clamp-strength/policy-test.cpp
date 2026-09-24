#include "../../src/rr_clamp_strength_policy.h"
#include "../../src/rr_no_clamp_test.h"
#include <cassert>
#include <cstdio>
using namespace control_rr_clamp;
int main(){
 for(unsigned bits=0;bits<16;++bits)for(unsigned count=0;count<4;++count)
  assert(Admit((bits&1)!=0,(bits&2)!=0,(bits&4)!=0,(bits&8)!=0,count)==(bits==15&&count==1));
 assert(!Valid(1)&&!Valid(101)&&!Valid(201));
 for(unsigned v=25;v<=75;++v){assert(Valid(v));assert((Variant(v)>=0)==(v!=0&&v!=100));assert(control_rr::UseNativeSignalClamp(false,true,v)==(v!=0));assert(!control_rr::UseNativeSignalClamp(true,true,v));assert(!control_rr::UseNativeSignalClamp(false,false,v));}
 for(unsigned v=25;v<=75;++v){assert(Variant(v)==static_cast<int>(v-25));assert(FromX(ToX(v))==v);}
 assert(FromX(-32768)==25&&FromX(32767)==75);assert(Normalize(0)==60&&Normalize(100)==60);
 for(int x=31;x<=710;++x)assert(FromX(x)>=FromX(x-1));
 History history;assert(history.Update(100));assert(!history.Update(100));assert(history.Update(50));assert(!history.Update(50));assert(history.Update(100));assert(history.Update(0));
 puts("PASS CS3 scoped/list/PSO/readiness/one-dispatch admission; selection, contamination, raw/native modes and effective-history transitions");
}
