#include "../src/native_cpu_metrics.h"
#include <cassert>
#include <thread>
#include <vector>
#include <iostream>
int main(){
    using namespace native_cpu;
    assert(Sample(1)&&!Sample(2)&&!Sample(64)&&Sample(65));
    Cell cell;cell.Add(20,30,true);cell.Add(40,0,false);Snapshot s;
    assert(cell.Read(s)&&s.samples==2&&s.ticks==60&&s.maximum==40&&s.cycles==30&&s.cycleSamples==1);
    cell.version.store(5);assert(!cell.Read(s));cell.version.store(6);
    Cell concurrent;std::atomic<bool> done{false};
    std::thread writer([&]{for(unsigned i=0;i<100000;++i)concurrent.Add(7,11,true);done=true;});
    do {if(concurrent.Read(s)){assert(s.ticks==s.samples*7);assert(s.cycles==s.samples*11);assert(s.cycleSamples==s.samples);}}while(!done.load());
    writer.join();assert(concurrent.Read(s)&&s.samples==100000);
    std::vector<std::thread> pool;
    for(unsigned i=0;i<80;++i)pool.emplace_back([i]{const auto slot=Reserve(i+1);if(slot<kThreads)threads[slot].cells[Preparation].Add(1,2,true);});
    for(auto& t:pool)t.join();
    assert(nextThread==80&&overflowThreads==16);
    unsigned count=0;for(auto& t:threads){assert(t.tid!=0);assert(t.cells[Preparation].Read(s)&&s.samples==1);++count;}
    assert(count==64);std::cout<<"PASS native CPU metrics: sampling, coherent concurrent reads, bounded snapshots, thread overflow\n";
}
