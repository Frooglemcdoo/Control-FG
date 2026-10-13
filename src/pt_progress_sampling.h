#pragma once
#include <cstdint>
// Cleanup fences are conservative completion samples, never dispatch admission
// or resource-retirement fences. An omitted sample proves no new completion.
namespace PTSceneProgress {
struct Sampling {
    static constexpr unsigned Interval=64;
    unsigned operations=0;
    std::uint64_t signalledStamp=0;
    bool Due(std::uint64_t stamp){
        if(operations<Interval)++operations;
        return stamp&&stamp!=signalledStamp&&(!signalledStamp||operations>=Interval);
    }
    void Signalled(std::uint64_t stamp){signalledStamp=stamp;operations=0;}
};
}
