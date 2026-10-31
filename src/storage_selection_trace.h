#pragma once
#include <cstdint>
#include <cstring>
namespace storage_selection_trace {
using U64=std::uint64_t;
struct Trace {
 static constexpr unsigned TargetCapacity=512,Capacity=32768;
 struct Target {U64 texture=0;char name[256]{};};
 struct State {
  U64 scan=0,cycle=0,pendingBytes=0;
  int base=0,resident=0,wanted=0,retain=0,delta=0,wantedState=-1,priorityMin=0,priorityMax=0;
  unsigned deferred=0,enabled=0,special=0,bypass=0,policy=0,direction=0;
  double spentMs=0;float budgetMs=0,distanceMetric=0;
 };
 struct Row {U64 texture=0,begin=0,end=0;unsigned kind=0;int level=-1,result=-1;State before,after;};
 Target targets[TargetCapacity]{};Row rows[Capacity]{};
 unsigned targetsCount=0,count=0,overflow=0,targetOverflow=0;
 static bool Matches(const char* name){return name&&(std::strstr(name,"sign")||std::strstr(name,"wayfinding"));}
 void Register(U64 texture,const char* name){
  // Update or invalidate reused numeric identities whenever a request supplies a name.
  const bool match=Matches(name);
  for(auto& t:targets)if(t.texture==texture&&texture){if(!match){t={};--targetsCount;}else Copy(t,texture,name);return;}
  if(!match||!texture)return;
  for(auto& t:targets)if(!t.texture){Copy(t,texture,name);++targetsCount;return;}
  ++targetOverflow;
 }
 static void Copy(Target& t,U64 texture,const char* name){t={};t.texture=texture;std::strncpy(t.name,name,sizeof(t.name)-1);}
 bool Contains(U64 texture)const{for(const auto& t:targets)if(t.texture==texture&&texture)return true;return false;}
 void Reset(){count=overflow=0;}
 void Add(const Row& row){if(count==Capacity){++overflow;return;}rows[count++]=row;}
};
}
