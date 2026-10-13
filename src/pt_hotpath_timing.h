#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
// Sample on each thread, once per 64 operations. No shared atomic update on
// unsampled calls. Durations are sampled wall time, not GPU time or frame time.
namespace PTHotpathTiming {
enum class Kind : unsigned {bindings,descriptors,count};
struct Counter {std::atomic<std::uint64_t> samples{0},ticks{0},maximum{0};};
inline Counter counters[static_cast<unsigned>(Kind::count)];
class Sample {
    Counter* counter=nullptr;
    LARGE_INTEGER start{};
public:
    explicit Sample(Kind kind,bool enabled=true){
        if(!enabled)return;
        static thread_local unsigned sequence[static_cast<unsigned>(Kind::count)]{};
        const auto index=static_cast<unsigned>(kind);
        if((sequence[index]++&63u)!=0)return;
        counter=&counters[index];QueryPerformanceCounter(&start);
    }
    ~Sample(){
        if(!counter)return;LARGE_INTEGER end{};QueryPerformanceCounter(&end);
        const auto elapsed=static_cast<std::uint64_t>(end.QuadPart-start.QuadPart);
        counter->ticks.fetch_add(elapsed,std::memory_order_relaxed);
        auto maximum=counter->maximum.load(std::memory_order_relaxed);
        while(elapsed>maximum&&!counter->maximum.compare_exchange_weak(maximum,elapsed,std::memory_order_relaxed)){}
        counter->samples.fetch_add(1,std::memory_order_relaxed);
    }
    Sample(const Sample&)=delete;
    Sample& operator=(const Sample&)=delete;
};
}
