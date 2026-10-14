#pragma once
#include <cstdint>
#include <map>
#include <vector>
#include <mutex>
#include <algorithm>
#include "pt_descriptor_interest.h"
// Sparse identities for AS descriptors only. Unknown/null/CBV/UAV writes erase
// previous identities. Copies preserve the allocation generation, never resolve
// an address against whichever allocation happens to exist later.
namespace PTSceneDescriptors {
using U64=std::uint64_t;
struct Acceleration {U64 address=0,token=0,bytes=0;};
struct Range {U64 start=0,count=0;};
// Native copy range counts are not descriptor counts. In particular, Control
// can copy its large bindless table from thousands of one-descriptor ranges.
// Preserve all ranges; do not impose a small pass-table limit on this API.
template<class Handle>
bool CopyRanges(unsigned count,const Handle* handles,const unsigned* sizes,std::vector<Range>& output){
    output.clear();if(count&&!handles)return false;
    output.reserve(count);
    for(unsigned n=0;n<count;++n){const auto size=sizes?sizes[n]:1u;
        if(size)output.push_back({static_cast<U64>(handles[n].ptr),size});}
    return true;
}
class Index {
    std::mutex mutex;
    std::map<U64,Acceleration> entries;
    U64 step;
    bool valid=true;
    const char* failure="none";
    U64 writes=0,copies=0,asWrites=0;
    static constexpr std::size_t Maximum=65536;
    bool Span(Range r) const {return step&&r.start&&r.count&&r.count<=(UINT64_MAX-r.start)/step;}
    void Erase(Range r){entries.erase(entries.lower_bound(r.start),entries.lower_bound(r.start+r.count*step));}
    bool Fail(const char* reason){entries.clear();valid=false;failure=reason;return false;}
public:
    explicit Index(U64 increment):step(increment){}
    bool Write(U64 handle,Acceleration as={}){
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        if(!Span({handle,1}))return Fail("descriptor_write_span");
        ++writes;
        entries.erase(handle);
        if(!as.address||!as.token||!as.bytes)return true;
        if(entries.size()>=Maximum)return Fail("descriptor_write_capacity");
        ++asWrites;
        PTDescriptorInterest::filter.Mark(handle);
        entries.emplace(handle,as);return true;
    }
    bool InvalidateAll(const char* reason){std::lock_guard<std::mutex> guard(mutex);return Fail(reason);}
    bool Invalidate(Range range){
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        if(!Span(range))return Fail("descriptor_invalidate_span");
        Erase(range);return true;
    }
    // The overwhelmingly common one-slot copy needs no temporary vectors.
    bool CopySimple(U64 destination,U64 source,U64 count){
        if(count!=1)return Copy({{destination,count}},{{source,count}});
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        if(!Span({destination,1}))return Fail("descriptor_copy_destination_span");
        if(!Span({source,1}))return Fail("descriptor_copy_source_span");
        ++copies;const auto found=entries.find(source);
        const auto value=found==entries.end()?Acceleration{}:found->second;
        entries.erase(destination);
        if(value.token){if(entries.size()>=Maximum)return Fail("descriptor_copy_capacity");PTDescriptorInterest::filter.Mark(destination);entries.emplace(destination,value);}
        return true;
    }
    bool Copy(const std::vector<Range>& destinations,const std::vector<Range>& sources){
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        U64 totalDst=0,totalSrc=0;
        for(const auto& r:destinations){if(!Span(r)||r.count>UINT64_MAX-totalDst)return Fail("descriptor_copy_destination_span");totalDst+=r.count;}
        for(const auto& r:sources){if(!Span(r)||r.count>UINT64_MAX-totalSrc)return Fail("descriptor_copy_source_span");totalSrc+=r.count;}
        if(!totalDst||totalDst!=totalSrc)return Fail("descriptor_copy_count");
        ++copies;
        // Snapshot before erasing destinations. Native API disallows overlap,
        // but the metadata transaction also handles aliases without stale reads.
        std::vector<std::pair<U64,Acceleration>> snapshot;U64 offset=0;
        for(const auto& r:sources){
            for(auto it=entries.lower_bound(r.start);it!=entries.end()&&it->first<r.start+r.count*step;++it){
                if((it->first-r.start)%step)return Fail("descriptor_copy_alignment");
                if(snapshot.size()>=Maximum)return Fail("descriptor_copy_snapshot_capacity");
                snapshot.push_back({offset+(it->first-r.start)/step,it->second});
            }
            offset+=r.count;
        }
        for(const auto& r:destinations)Erase(r);
        std::size_t range=0;offset=0;
        for(const auto& s:snapshot){
            while(s.first-offset>=destinations[range].count){offset+=destinations[range].count;++range;}
            const auto handle=destinations[range].start+(s.first-offset)*step;
            PTDescriptorInterest::filter.Mark(handle);entries[handle]=s.second;
            if(entries.size()>Maximum)return Fail("descriptor_copy_capacity");
        }
        return true;
    }
    struct Stats {bool healthy;const char* failure;U64 entries,writes,copies,asWrites;};
    Stats Statistics(){std::lock_guard<std::mutex> guard(mutex);return {valid,failure,static_cast<U64>(entries.size()),writes,copies,asWrites};}
    Acceleration Read(U64 handle,const char** reason=nullptr){
        std::lock_guard<std::mutex> guard(mutex);auto it=entries.find(handle);
        if(reason)*reason=!valid?failure:it==entries.end()?"descriptor_missing":"none";
        return valid&&it!=entries.end()?it->second:Acceleration{};
    }
};
}
