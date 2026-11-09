#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../src/native_lighting_log_queue.h"
#include <cassert>
#include <thread>
#include <vector>
#include <iostream>
int main() {
    native_lighting::DiagnosticLogQueue queue;
    std::vector<char> buffer(queue.Capacity());
    assert(queue.Push("first\n",6)); assert(queue.Push("second\n",7));
    assert(queue.Drain(buffer.data(),buffer.size())==13);
    assert(memcmp(buffer.data(),"first\nsecond\n",13)==0);
    // A stopped/slow consumer cannot block a full queue's producers.
    assert(queue.Push(buffer.data(),buffer.size()));
    assert(!queue.Push("x\n",2)); assert(queue.TakeDropped()==1);
    assert(queue.Drain(buffer.data(),buffer.size())==buffer.size());
    std::atomic<unsigned> accepted{0};
    std::vector<std::thread> producers;
    for(unsigned i=0;i<4;++i) producers.emplace_back([&] {
        for(unsigned n=0;n<10000;++n) if(queue.Push("line\n",5))++accepted;
    });
    for(auto& thread:producers)thread.join();
    const auto bytes=queue.Drain(buffer.data(),buffer.size());
    assert(bytes==size_t(accepted.load())*5);
    assert(accepted.load()+queue.TakeDropped()==40000);
    for(size_t i=0;i<bytes;i+=5)assert(memcmp(buffer.data()+i,"line\n",5)==0);
    std::cout<<"PASS diagnostic queue ordering, full-queue drop and concurrent complete-line accounting\n";
}
