#include "../src/native_loader_candidate.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace loader_candidate;Range r;
 assert(Resolve(100100,100000,4096,32,512,r)&&r.offset==132&&r.bytes==512);
 assert(!Resolve(99,100,4096,0,1,r)&&r.bytes==0);
 assert(!Resolve(100,0,4096,UINT64_MAX,1,r));
 assert(!Resolve(100,0,4096,4097,1,r));
 assert(!Resolve(100,0,4096,4000,97,r));
 assert(Resolve(100,0,4096,4000,96,r)&&r.offset==4100);
 assert(!Resolve(UINT64_MAX-4,0,4096,8,1,r));
 assert(!Resolve(UINT64_MAX-4,0,4096,0,8,r));
 assert(!Resolve(0,0,4096,0,0,r));
 Table t;Candidate c;
 t.Put({1,10,20,30,{40,50}});
 assert(!t.Take(1,11,31,c));assert(t.Take(1,10,31,c)&&c.pack==20&&c.range.offset==40);
 assert(!t.Take(1,10,32,c));
 t.Put({1,10,20,40,{40,50}});t.Put({1+Table::Capacity,10,20,41,{60,70}});
 assert(t.replaced==1&&!t.Take(1,10,42,c));assert(t.Take(1+Table::Capacity,10,42,c)&&c.range.bytes==70);
 t.Put({2,10,20,100,{1,2}});assert(!t.Take(2,10,99,c));assert(!t.Take(2,10,101,c));
 t.lock.test_and_set();t.Put({3,10,20,100,{1,2}});assert(!t.Take(3,10,101,c));t.lock.clear();assert(t.dropped==2);
 puts("PASS R236 candidates: sort bias, EOF and overflow checks, exact owner/ID, consume once, collisions and contention");
}
