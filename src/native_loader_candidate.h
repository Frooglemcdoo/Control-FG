#pragma once
#include "native_loader_trace_metrics.h"
namespace loader_candidate {
using loader_trace::U64;
struct Range { U64 offset=0,bytes=0; };
// Native sort location contains an archive bias; it is not an I/O offset.
inline bool Resolve(U64 location,U64 bias,U64 fileBytes,U64 partial,U64 bytes,Range& out){
    out={};if(location<bias||partial==UINT64_MAX||!bytes||partial>fileBytes||bytes>fileBytes-partial)return false;
    const U64 base=location-bias;if(base>UINT64_MAX-partial)return false;
    const U64 offset=base+partial;if(offset>UINT64_MAX-bytes)return false;
    out={offset,bytes};return true;
}
struct Candidate {U64 id=0,owner=0,pack=0,ticks=0;Range range{};};
struct Table {
    static constexpr unsigned Capacity=4096;
    Candidate slots[Capacity]{};
    std::atomic_flag lock=ATOMIC_FLAG_INIT;
    std::atomic<U64> stored{0},dropped{0},replaced{0},matched{0},missing{0};
    void Put(const Candidate& value){
        if(!value.id||lock.test_and_set(std::memory_order_acquire)){++dropped;return;}
        auto& s=slots[value.id%Capacity];if(s.id)++replaced;s=value;
        lock.clear(std::memory_order_release);++stored;
    }
    bool Take(U64 id,U64 owner,U64 ticks,Candidate& out){
        out={};if(!id||lock.test_and_set(std::memory_order_acquire)){++dropped;return false;}
        auto& s=slots[id%Capacity];const bool match=s.id==id&&s.owner==owner&&ticks>=s.ticks;
        if(match)out=s;
        if(s.id==id&&s.owner==owner)s={};
        lock.clear(std::memory_order_release);
        if(match)++matched;else ++missing;return match;
    }
};
}
