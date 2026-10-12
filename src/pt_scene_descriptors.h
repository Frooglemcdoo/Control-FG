#pragma once
#include <cstdint>
#include <map>
#include <vector>
#include <mutex>
#include <algorithm>
// Sparse identities for AS descriptors only. Unknown/null/CBV/UAV writes erase
// previous identities. Copies preserve the allocation generation, never resolve
// an address against whichever allocation happens to exist later.
namespace PTSceneDescriptors {
using U64=std::uint64_t;
struct Acceleration {U64 address=0,token=0,bytes=0;};
struct Range {U64 start=0,count=0;};
class Index {
    std::mutex mutex;
    std::map<U64,Acceleration> entries;
    U64 step;
    bool valid=true;
    static constexpr std::size_t Maximum=65536;
    bool Span(Range r) const {return step&&r.start&&r.count&&r.count<=(UINT64_MAX-r.start)/step;}
    void Erase(Range r){entries.erase(entries.lower_bound(r.start),entries.lower_bound(r.start+r.count*step));}
    bool Fail(){entries.clear();valid=false;return false;}
public:
    explicit Index(U64 increment):step(increment){}
    bool Write(U64 handle,Acceleration as={}){
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        if(!Span({handle,1}))return Fail();
        entries.erase(handle);
        if(!as.address||!as.token||!as.bytes)return true;
        if(entries.size()>=Maximum)return Fail();
        entries.emplace(handle,as);return true;
    }
    bool Invalidate(Range range){
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        if(!Span(range))return Fail();
        Erase(range);return true;
    }
    bool Copy(const std::vector<Range>& destinations,const std::vector<Range>& sources){
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        U64 totalDst=0,totalSrc=0;
        for(const auto& r:destinations){if(!Span(r)||r.count>UINT64_MAX-totalDst)return Fail();totalDst+=r.count;}
        for(const auto& r:sources){if(!Span(r)||r.count>UINT64_MAX-totalSrc)return Fail();totalSrc+=r.count;}
        if(!totalDst||totalDst!=totalSrc)return Fail();
        // Snapshot before erasing destinations. Native API disallows overlap,
        // but the metadata transaction also handles aliases without stale reads.
        std::vector<std::pair<U64,Acceleration>> snapshot;U64 offset=0;
        for(const auto& r:sources){
            for(auto it=entries.lower_bound(r.start);it!=entries.end()&&it->first<r.start+r.count*step;++it){
                if((it->first-r.start)%step)return Fail();
                if(snapshot.size()>=Maximum)return Fail();
                snapshot.push_back({offset+(it->first-r.start)/step,it->second});
            }
            offset+=r.count;
        }
        for(const auto& r:destinations)Erase(r);
        std::size_t range=0;offset=0;
        for(const auto& s:snapshot){
            while(s.first-offset>=destinations[range].count){offset+=destinations[range].count;++range;}
            entries[destinations[range].start+(s.first-offset)*step]=s.second;
            if(entries.size()>Maximum)return Fail();
        }
        return true;
    }
    Acceleration Read(U64 handle){std::lock_guard<std::mutex> guard(mutex);auto it=entries.find(handle);return valid&&it!=entries.end()?it->second:Acceleration{};}
};
}
