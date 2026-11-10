#include "../../src/rt_heap_refresh.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <random>
using namespace rt_heap_refresh;
int main(){
 Tracker tracker;constexpr U32 Count=4096;
 Key key{1,2,3};auto plan=tracker.Prepare(4,key,0,0,Count,0);assert(plan.full&&plan.copied==Count);tracker.Commit(key,0,Count);
 plan=tracker.Prepare(4,key,0,0,Count,Count);assert(!plan.full&&!plan.copied);
 tracker.Publish(4,0,270);plan=tracker.Prepare(4,key,1,0,Count,Count);
 assert(!plan.full&&plan.copied==256&&plan.ranges[0].start==256);tracker.Commit(key,1,Count);
 tracker.Publish(4,1,270);plan=tracker.Prepare(4,key,2,1,Count,Count);assert(plan.copied==256); // same handle is still dirty
 assert(tracker.Prepare(4,key,2,1,Count-1,Count).full);
 assert(tracker.Prepare(4,key,2,1,Count+1,Count).full);
 assert(tracker.Prepare(4,{1,20,3},2,1,Count,Count).full);
 assert(tracker.Prepare(4,key,3,1,Count,Count).full); // unobserved publication
 assert(tracker.Prepare(5,key,3,1,Count,Count).full); // manager replacement
 Tracker replay;std::vector<unsigned> source(Count),gpu[4];U64 versions[4]{},generation=0;
 for(auto& heap:gpu)heap.resize(Count);
 std::mt19937 random(410);
 for(unsigned iteration=0;iteration<2000;++iteration){
  const unsigned writes=random()%65;
  for(unsigned j=0;j<writes;++j){const U32 index=random()%Count;source[index]=random();replay.Publish(9,generation++,index);}
  const unsigned h=random()%4;Key identity{h+1,h+10,h+20};
  auto p=replay.Prepare(9,identity,generation,versions[h],Count,iteration<4?0u:Count);
  for(U32 j=0;j<p.rangeCount;++j){const auto r=p.ranges[j];assert(r.start+r.count<=Count);for(U32 k=r.start;k<r.start+r.count;++k)gpu[h][k]=source[k];}
  assert(gpu[h]==source);replay.Commit(identity,generation,Count);versions[h]=generation;
 }
 // A wrapped generation or table outside the audited prefix always loses history.
 replay.Publish(9,UINT64_MAX,0);assert(replay.Prepare(9,{1,10,20},0,0,Count,Count).full);
 assert(replay.Prepare(9,key,0,0,Capacity+1,0).full);
 std::puts("heap refresh: randomized full-copy oracle, publication, resizing and invalidation passed");
}
