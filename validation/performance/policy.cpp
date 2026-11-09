#include "../../src/rr_performance_policy.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace control_rr_perf;
 Pool<3> pool;Ticket a,b,c,d;
 assert(pool.Begin(10,20,a)&&pool.Begin(10,20,b)&&pool.Begin(10,20,c)&&!pool.Begin(10,20,d));
 assert(!pool.Submit(a.index,21,1)&&!pool.Completed(a.index,100));
 assert(pool.End(a,true)&&!pool.End(a,true));
 assert(!pool.Submit(a.index,20,1)&&!pool.Submit(a.index,22,1)&&pool.Submit(a.index,21,7));
 assert(!pool.Completed(a.index,6)&&!pool.Completed(a.index,UINT64_MAX)&&pool.Retire(a.index,7));
 assert(!pool.End(a,true));
 assert(pool.End(b,false)&&pool.Submit(b.index,21,8)&&pool.Retire(b.index,8));
 assert(pool.End(c,true)&&pool.Submit(c.index,21,8)&&pool.Retire(c.index,8));
 for(unsigned i=0;i<5000;++i){assert(pool.Begin(i,i,d));assert(pool.End(d,true));assert(pool.Submit(d.index,i+1,i+1));assert(!pool.Retire(d.index,i));assert(pool.Retire(d.index,i+1));}
 double ms=0;assert(Milliseconds(100,250,100000,ms)&&ms==1.5);
 assert(!Milliseconds(250,100,100000,ms)&&!Milliseconds(100,250,0,ms));
 Stability stable;Key key{2560,1440,3840,2160,0,0,1,false,false,true};
 for(unsigned i=0;i<240;++i){bool eligible=stable.Observe(key,i,1000+i*20,1000,false,true,ms);assert(eligible==(i>=150));}
 const auto segment=stable.segment;key.rr=true;key.requested=true;
 assert(!stable.Observe(key,240,5800,1000,false,true,ms)&&stable.segment==segment+1);
 assert(!stable.Observe(key,241,5820,1000,true,true,ms));
 assert(!stable.Observe(key,260,6200,1000,false,true,ms)); // skipped rendering
 key.width=3840;assert(!stable.Observe(key,261,6220,1000,false,true,ms));
 key.fg=3;assert(!stable.Observe(key,262,6240,1000,false,true,ms));
 key.ready=false;assert(!stable.Observe(key,263,6260,1000,false,true,ms));
 key.ready=true;assert(!stable.Observe(key,264,6280,1000,false,false,ms));
 assert(!stable.Observe(key,265,6780,1000,false,true,ms)); // stall excludes interval
 puts("PASS query leases: exact Present, fence reuse, abandonment, stale serials, exhaustion, device loss, 5000 cycles; timing conversion and comparison warmup/mode/resize/FG/reset/gap gates");
}
