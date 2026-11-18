#include "../src/gi_reuse/regional_target_lookup.h"
#include <array>
#include <cassert>
#include <iostream>
int main(){
 using namespace gi_reuse;
 std::array<GIRegionalTarget,16> table{};
 const auto key=[](float x){return GIMakeWorldGuideKey({x,.1f,.1f},{0,0,1});};
 const auto target=[](float x){GIAutoTargets t{};t.schema=0x41544731;t.frame=20;t.version=7;t.validMask=3;
  for(unsigned i=0;i<2;++i){t.bounds[i].center={x+float(i),0,3};t.bounds[i].radius=.3f;t.counts[i]=16;t.scores[i]=100;}return t;};
 const auto a=key(.1f);auto b=key(2.1f);
 // Force a real collision, then place the other region in the next probe.
 for(float x=2.1f;GIRegionalSlot(a,16,0)!=GIRegionalSlot(b,16,0);x+=.25f)b=key(x);
 assert(!GIWorldGuideKeysMatch(a,b));
 table[GIRegionalSlot(a,16,0)]={a,target(1)};
 table[GIRegionalSlot(b,16,1)]={b,target(30)};
 GIAutoTargets selected{};
 assert(GIRegionalLookup(table.data(),16,a,22,7,4,selected)&&selected.bounds[0].center.x==1);
 assert(GIRegionalLookup(table.data(),16,b,22,7,4,selected)&&selected.bounds[0].center.x==30);
 assert(!GIRegionalLookup(table.data(),16,a,25,7,4,selected)&&selected.validMask==0);
 assert(!GIRegionalLookup(table.data(),16,a,19,7,4,selected));
 assert(!GIRegionalLookup(table.data(),16,a,22,8,4,selected));
 auto back=a;back.normal^=4;assert(!GIRegionalLookup(table.data(),16,back,22,7,4,selected));
 auto invalid=a;invalid.valid=0;assert(!GIRegionalLookup(table.data(),16,invalid,22,7,4,selected));
 table[GIRegionalSlot(a,16,0)].targets.bounds[0].radius=0;
 assert(!GIRegionalLookup(table.data(),16,a,22,7,4,selected));
 // A same-key entry beyond the bounded probe budget must not trigger unbounded work.
 table={};table[GIRegionalSlot(a,16,4)]={a,target(1)};
 assert(!GIRegionalLookup(table.data(),16,a,22,7,4,selected));
 assert(!GIRegionalLookup(nullptr,16,a,22,7,4,selected));
 std::cout<<"PASS receiver separation under hash collision, stale/future/version/normal rejection, invalid bound fallback and fixed probe limit. Prototype lookup only; no GPU or live integration claim.\n";
}
