#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
namespace gi_reuse {
// Immutable batch index. Two largest prefix ends suffice to distinguish zero,
// one and multiple containing allocations, including nested/aliased buffers.
class ProducerBackingRanges {
    struct Entry { std::uint64_t begin,end,first=0,second=0; };
    std::vector<Entry> entries_;
    bool ready_=false;
    // Exact-range memoization within this immutable batch only. Aliases still
    // pass through the prefix index; collisions merely replace an entry.
    struct Query {std::uint64_t begin=0,bytes=0;bool unique=false;};
    mutable std::vector<Query> queries_;
    static std::size_t Slot(std::uint64_t begin,std::uint64_t bytes){
        auto h=begin^(bytes+0x9e3779b97f4a7c15ull+(begin<<6)+(begin>>2));
        h=(h^(h>>30))*0xbf58476d1ce4e5b9ull;h=(h^(h>>27))*0x94d049bb133111ebull;
        return static_cast<std::size_t>(h^(h>>31))&4095;
    }
public:
    bool Add(std::uint64_t begin,std::uint64_t bytes) {
        if(ready_||!begin||!bytes||bytes>UINT64_MAX-begin)return false;
        entries_.push_back({begin,begin+bytes,0,0});return true;
    }
    void Seal() {
        queries_.assign(4096,{});
        std::sort(entries_.begin(),entries_.end(),[](const Entry& a,const Entry& b){return a.begin<b.begin;});
        std::uint64_t first=0,second=0;
        for(auto& entry:entries_){
            if(entry.end>=first){second=first;first=entry.end;}
            else if(entry.end>second)second=entry.end;
            entry.first=first;entry.second=second;
        }
        ready_=true;
    }
    bool Unique(std::uint64_t begin,std::uint64_t bytes) const noexcept {
        if(!ready_||!begin||!bytes||bytes>UINT64_MAX-begin)return false;
        auto& cached=queries_[Slot(begin,bytes)];
        if(cached.begin==begin&&cached.bytes==bytes)return cached.unique;
        auto it=std::upper_bound(entries_.begin(),entries_.end(),begin,
            [](std::uint64_t address,const Entry& entry){return address<entry.begin;});
        if(it==entries_.begin()){cached={begin,bytes,false};return false;}
        --it;const auto end=begin+bytes;
        const bool unique=it->first>=end&&it->second<end;
        cached={begin,bytes,unique};return unique;
    }
};
}
