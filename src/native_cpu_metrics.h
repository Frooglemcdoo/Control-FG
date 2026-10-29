#pragma once
#include <atomic>
#include <cstdint>

namespace native_cpu {
using U64 = unsigned long long;
constexpr unsigned kStages=4, kThreads=64, kInterval=64;
enum Stage : unsigned { Material, Preparation, Recording, Join };
inline const char* Name(unsigned s) noexcept {
    constexpr const char* names[]={"material_update","rt_prepare","rt_record","rt_prepare_join"};
    return names[s];
}
struct Snapshot { U64 samples=0,ticks=0,maximum=0,cycles=0,cycleSamples=0; };
// Single writer per cell. Atomic members also make bounded concurrent reads
// well-defined; a busy cell is skipped instead of blocking a game thread.
struct Cell {
    std::atomic<U64> version{0},calls{0},samples{0},ticks{0},maximum{0},cycles{0},cycleSamples{0};
    void Add(U64 elapsed,U64 cycleDelta,bool validCycles) noexcept {
        version.fetch_add(1); // seq_cst ordering brackets every field
        samples.store(samples.load()+1); ticks.store(ticks.load()+elapsed);
        if(elapsed>maximum.load())maximum.store(elapsed);
        if(validCycles){cycles.store(cycles.load()+cycleDelta);cycleSamples.store(cycleSamples.load()+1);}
        version.fetch_add(1);
    }
    bool Read(Snapshot& out) const noexcept {
        for(unsigned retry=0;retry<3;++retry){
            const auto v=version.load();if(v&1)continue;
            Snapshot s{samples.load(),ticks.load(),maximum.load(),cycles.load(),cycleSamples.load()};
            if(version.load()==v){out=s;return true;}
        }
        return false;
    }
};
struct alignas(64) Thread {
    std::atomic<unsigned long> tid{0};Cell cells[kStages];
};
inline Thread threads[kThreads];
inline std::atomic<unsigned> nextThread{0};
inline std::atomic<U64> overflowThreads{0};
inline unsigned Reserve(unsigned long tid) noexcept {
    const unsigned slot=nextThread.fetch_add(1,std::memory_order_relaxed);
    if(slot>=kThreads){overflowThreads.fetch_add(1,std::memory_order_relaxed);return kThreads;}
    threads[slot].tid.store(tid,std::memory_order_release);return slot;
}
inline bool Sample(U64 ordinal) noexcept {return (ordinal-1)%kInterval==0;}
}
