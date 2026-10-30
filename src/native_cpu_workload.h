#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
namespace native_workload {
using U64=unsigned long long;
constexpr unsigned Fields=4;
struct Snapshot {U64 samples=0,sum[Fields]{},last[Fields]{};};
struct Cell {
    std::atomic<U64> version{0},samples{0},sum[Fields]{},last[Fields]{};
    void Add(const U64 (&values)[Fields]) noexcept {
        version.fetch_add(1);
        samples.store(samples.load()+1);
        for(unsigned i=0;i<Fields;++i){sum[i].store(sum[i].load()+values[i]);last[i].store(values[i]);}
        version.fetch_add(1);
    }
    bool Read(Snapshot& out) const noexcept {
        for(unsigned attempt=0;attempt<3;++attempt){
            const auto v=version.load();if(v&1)continue;
            Snapshot s;s.samples=samples.load();
            for(unsigned i=0;i<Fields;++i){s.sum[i]=sum[i].load();s.last[i]=last[i].load();}
            if(v==version.load()){out=s;return true;}
        }return false;
    }
};
template<class T> inline T Load(const void* base,unsigned offset) noexcept {
    T value{};std::memcpy(&value,static_cast<const unsigned char*>(base)+offset,sizeof(value));return value;
}
// Read only scalar fields while native ownership is still held. Never traverse arrays.
inline void MaterialValues(const void* manager,std::uint32_t rebuildCounter,U64 (&out)[Fields]) noexcept {
    out[0]=Load<std::uint32_t>(manager,0x80);
    out[1]=Load<std::uint32_t>(manager,0xd0);
    out[2]=Load<std::uint64_t>(manager,0x100);
    out[3]=rebuildCounter;
}
inline void PreparationValues(const void* job,U64 (&out)[Fields]) noexcept {
    const auto* context=Load<const void*>(job,0x18);
    out[0]=Load<std::uint32_t>(context,0x18);
    out[1]=Load<std::uint32_t>(context,0x78);
    out[2]=Load<std::uint32_t>(context,0x88);
    out[3]=Load<std::uint32_t>(context,0x98);
}
}
