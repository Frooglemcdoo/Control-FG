#pragma once
#include <atomic>
#include <cstdint>
namespace loader_trace {
using U64=std::uint64_t;
struct Start {U64 id=0,ticks=0,bytes=0,owner=0;};
struct Tracker {
    static constexpr unsigned Capacity=4096;
    Start slots[Capacity]{};
    std::atomic_flag lock=ATOMIC_FLAG_INIT;
    std::atomic<U64> queued{0},matched{0},unmatched{0},dropped{0},replaced{0},bytes{0},waitTicks{0},waitMax{0};
    static void Max(std::atomic<U64>& field,U64 value){auto old=field.load(std::memory_order_relaxed);while(old<value&&!field.compare_exchange_weak(old,value,std::memory_order_relaxed)){}}
    void Enqueue(U64 id,U64 ticks,U64 size,U64 owner=1){
        queued.fetch_add(1,std::memory_order_relaxed);
        if(!id||lock.test_and_set(std::memory_order_acquire)){dropped.fetch_add(1,std::memory_order_relaxed);return;}
        auto& s=slots[id%Capacity];if(s.id)replaced.fetch_add(1,std::memory_order_relaxed);
        s={id,ticks,size,owner};lock.clear(std::memory_order_release);
    }
    bool Begin(U64 id,U64 ticks,U64 owner=1){
        if(!id||lock.test_and_set(std::memory_order_acquire)){dropped.fetch_add(1,std::memory_order_relaxed);return false;}
        auto& s=slots[id%Capacity];const bool found=s.id==id&&s.owner==owner&&ticks>=s.ticks;
        const Start value=s;if(s.id==id&&s.owner==owner)s={};lock.clear(std::memory_order_release);
        if(!found){unmatched.fetch_add(1,std::memory_order_relaxed);return false;}
        matched.fetch_add(1,std::memory_order_relaxed);bytes.fetch_add(value.bytes,std::memory_order_relaxed);
        const auto delta=ticks-value.ticks;waitTicks.fetch_add(delta,std::memory_order_relaxed);Max(waitMax,delta);return true;
    }
};
}
