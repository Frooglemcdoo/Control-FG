#include "../src/native_loader_trace_metrics.h"
#include <cassert>
#include <cstdio>
int main(){
    loader_trace::Tracker t;
    t.Enqueue(1,100,1024);assert(t.Begin(1,150));assert(!t.Begin(1,200));
    assert(t.matched.load()==1&&t.bytes.load()==1024&&t.waitTicks.load()==50&&t.waitMax.load()==50);
    t.Enqueue(2,200,10);t.Enqueue(4098,250,20);assert(!t.Begin(2,300));assert(t.Begin(4098,325));
    assert(t.replaced.load()==1&&t.waitTicks.load()==125&&t.bytes.load()==1044);
    t.Enqueue(3,500,9);assert(!t.Begin(3,400));
    t.lock.test_and_set();t.Enqueue(4,0,1);assert(!t.Begin(4,1));t.lock.clear();assert(t.dropped.load()==2);
    t.Enqueue(0,0,1);assert(!t.Begin(0,1));assert(t.dropped.load()==4);
    t.Enqueue(7,100,77,11);assert(!t.Begin(7,150,12));assert(t.Begin(7,150,11));
    puts("PASS R235 loader tracker: exact owner/IDs, consume once, collision rejection, bounded replacement, reversed time and nonblocking contention");
}
