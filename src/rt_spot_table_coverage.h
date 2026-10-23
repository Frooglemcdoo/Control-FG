#pragma once
#include <cstdint>
#include <cstring>
namespace rt_spot_capture {
// Small bounded union of byte intervals. Overflow or too many fragments fails
// closed; a missing tail must never inherit coverage from an earlier table.
struct TableCoverage {
    struct Range { std::uint64_t begin=0,end=0; };
    Range ranges[128]{};
    unsigned count=0;
    void Clear() noexcept { count=0; }
    bool Add(std::uint64_t offset,std::uint64_t bytes,std::uint64_t capacity) noexcept {
        if (!bytes || offset>capacity || bytes>capacity-offset) return false;
        Range next{offset,offset+bytes};
        for(unsigned i=0;i<count;) {
            if(next.end<ranges[i].begin || next.begin>ranges[i].end) {++i;continue;}
            if(ranges[i].begin<next.begin)next.begin=ranges[i].begin;
            if(ranges[i].end>next.end)next.end=ranges[i].end;
            ranges[i]=ranges[--count];i=0;
        }
        if(count==128){Clear();return false;}
        ranges[count++]=next;return true;
    }
    bool Covers(std::uint64_t offset,std::uint64_t bytes) const noexcept {
        if(!bytes || bytes>UINT64_MAX-offset)return false;
        for(unsigned i=0;i<count;++i)if(ranges[i].begin<=offset && ranges[i].end>=offset+bytes)return true;
        return false;
    }
};
struct TableIdentifiers {
    unsigned group0=0,group1=0,empty=0,unknown=0;
};
inline TableIdentifiers CountTableIdentifiers(const unsigned char* data,std::uint64_t size,
    const unsigned char* group0,const unsigned char* group1) noexcept {
    TableIdentifiers result;
    if(!data || !group0 || !group1 || !size || size%96 || size>8ull*1024*1024) {result.unknown=1;return result;}
    const unsigned char zero[32]{};
    for(std::uint64_t at=0;at<size;at+=96) {
        if(!std::memcmp(data+at,group0,32))++result.group0;
        else if(!std::memcmp(data+at,group1,32))++result.group1;
        else if(!std::memcmp(data+at,zero,32))++result.empty;
        else ++result.unknown;
    }
    return result;
}
}
