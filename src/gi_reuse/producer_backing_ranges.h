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
public:
    bool Add(std::uint64_t begin,std::uint64_t bytes) {
        if(ready_||!begin||!bytes||bytes>UINT64_MAX-begin)return false;
        entries_.push_back({begin,begin+bytes,0,0});return true;
    }
    void Seal() noexcept {
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
        auto it=std::upper_bound(entries_.begin(),entries_.end(),begin,
            [](std::uint64_t address,const Entry& entry){return address<entry.begin;});
        if(it==entries_.begin())return false;
        --it;const auto end=begin+bytes;
        return it->first>=end&&it->second<end;
    }
};
}
