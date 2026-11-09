#include "../src/native_preparation_phases.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace native_prep;Tick d[kPhases]{};
    Timeline t{10,20,40,90,100,120,true,true,true,true};
    assert(t.Split(d));assert(d[0]==10&&d[1]==20&&d[2]==50&&d[3]==10&&d[4]==20);
    Tick sum=0;for(auto x:d)sum+=x;assert(sum==t.end-t.begin);
    t.readback=t.unmapped=false;assert(t.Split(d)&&d[0]==0&&d[1]==0&&d[2]==80);
    t.estimateComplete=false;assert(!t.Split(d));t.estimateComplete=true;
    t.end=99;assert(!t.Split(d));t.end=120;t.readback=true;assert(!t.Split(d));
    t.unmapped=true;t.unmap=95;assert(!t.Split(d));t.unmap=40;t.map=9;assert(!t.Split(d));
    puts("PASS preparation partition accounting, absent readback and incomplete/out-of-order rejection");
}
