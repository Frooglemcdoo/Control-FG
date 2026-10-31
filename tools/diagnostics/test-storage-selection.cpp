#include "../../src/storage_selection_trace.h"
#include <cassert>
#include <cstdio>
int main(){
 static storage_selection_trace::Trace t;
 assert(!t.Matches("other.tex"));assert(t.Matches("data_pc\\textures\\decals\\signs\\hazard.tex"));assert(t.Matches("data_pc\\textures\\props\\toilet_sign_d.tex"));assert(t.Matches("data_pc\\textures\\props\\signs\\wayfinding_text_luck_probability_a.tex"));
 for(unsigned i=0;i<t.TargetCapacity;++i)t.Register(i+1,"data_pc\\textures\\props\\signs\\wayfinding_text_fixture.tex");
 assert(t.targetsCount==t.TargetCapacity);t.Register(999,"data_pc\\textures\\props\\signs\\wayfinding_text_fixture.tex");assert(t.targetOverflow==1);
 t.Register(1,"other.tex");assert(!t.Contains(1)&&t.targetsCount==t.TargetCapacity-1);
 t.Register(999,"data_pc\\textures\\props\\signs\\wayfinding_text_fixture.tex");assert(t.Contains(999));
 storage_selection_trace::Trace::Row r;r.texture=999;
 for(unsigned i=0;i<t.Capacity+1;++i)t.Add(r);
 assert(t.count==t.Capacity&&t.overflow==1);t.Reset();assert(!t.count&&!t.overflow&&t.Contains(999));
 puts("PASS selection metadata: target/row bounds, overflow, reset and identity replacement");
}
