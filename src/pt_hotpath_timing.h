#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
// Sample once per 64 entries per thread/category. Next() keeps the SAME sample
// across stages. Totals are inclusive wall time across threads, NOT frame CPU
// time. Nested categories overlap; use ETW call stacks to find the critical path.
namespace PTHotpathTiming {
enum class Kind : unsigned {bindings,descriptors,descriptor_wait,descriptor_native,descriptor_observer,
    snapshot_wait,snapshot_work,build_wait,build_inputs,geometry_snapshot,build_record,
    reset_lookup,reset_wait,reset_native,reset_observer,primary_queue_wait,primary_guard_wait,
    primary_setup,primary_lease,primary_sbt,primary_camera,primary_metrics,primary_record,glass_capture,descriptor_untracked,descriptor_parallel_wait,descriptor_parallel_native,descriptor_parallel_mirror,count};
inline const char* Name(unsigned kind){
    static const char* names[]={"bindings","descriptors","descriptor_wait","descriptor_native","descriptor_observer",
        "snapshot_wait","snapshot_work","build_wait","build_inputs","geometry_snapshot","build_record",
        "reset_lookup","reset_wait","reset_native","reset_observer","primary_queue_wait","primary_guard_wait",
        "primary_setup","primary_lease","primary_sbt","primary_camera","primary_metrics","primary_record","glass_capture","descriptor_untracked","descriptor_parallel_wait","descriptor_parallel_native","descriptor_parallel_mirror"};
    static_assert(sizeof(names)/sizeof(names[0])==static_cast<unsigned>(Kind::count),"timing names");
    return names[kind];
}
struct Counter {std::atomic<std::uint64_t> samples{0},ticks{0},maximum{0};};
inline Counter counters[static_cast<unsigned>(Kind::count)];
class Sample {
    Counter* counter=nullptr;
    LARGE_INTEGER start{};
    void Store(LARGE_INTEGER end){
        const auto elapsed=static_cast<std::uint64_t>(end.QuadPart-start.QuadPart);
        counter->ticks.fetch_add(elapsed,std::memory_order_relaxed);
        auto maximum=counter->maximum.load(std::memory_order_relaxed);
        while(elapsed>maximum&&!counter->maximum.compare_exchange_weak(maximum,elapsed,std::memory_order_relaxed)){}
        counter->samples.fetch_add(1,std::memory_order_relaxed);
    }
public:
    explicit Sample(Kind kind,bool enabled=true){
        if(!enabled)return;
        static thread_local unsigned sequence[static_cast<unsigned>(Kind::count)]{};
        const auto index=static_cast<unsigned>(kind);
        if((sequence[index]++&63u)!=0)return;
        counter=&counters[index];QueryPerformanceCounter(&start);
    }
    void Next(Kind kind){
        if(!counter)return;
        LARGE_INTEGER end{};QueryPerformanceCounter(&end);Store(end);
        counter=&counters[static_cast<unsigned>(kind)];start=end;
    }
    ~Sample(){if(counter){LARGE_INTEGER end{};QueryPerformanceCounter(&end);Store(end);}}
    Sample(const Sample&)=delete;
    Sample& operator=(const Sample&)=delete;
};
}
