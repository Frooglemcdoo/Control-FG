#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
namespace native_material_diff {
constexpr unsigned Words=0x150/4,Examples=8,Capacity=1024;
struct Event {
    std::uintptr_t manager=0;std::uint32_t entry=0,oldHash=0,newHash=0,sourceDerived=0,committedDerived=0;
    std::uint64_t oldKey=0,newKey=0,mask[2]{};
    unsigned changes=0,offset[Examples]{};std::uint32_t before[Examples]{},after[Examples]{};
};
inline Event Compare(std::uintptr_t manager,std::uint32_t entry,std::uint32_t oldHash,std::uint32_t newHash,
    std::uint64_t oldKey,std::uint64_t newKey,std::uint32_t sourceDerived,const std::uint32_t* before,const std::uint32_t* after) noexcept {
    Event e;e.manager=manager;e.entry=entry;e.oldHash=oldHash;e.newHash=newHash;e.oldKey=oldKey;e.newKey=newKey;e.sourceDerived=sourceDerived;e.committedDerived=after[0x148/4];
    for(unsigned i=0;i<Words;++i)if(before[i]!=after[i]){
        e.mask[i/64]|=std::uint64_t(1)<<(i%64);
        if(e.changes<Examples){e.offset[e.changes]=i*4;e.before[e.changes]=before[i];e.after[e.changes]=after[i];}
        ++e.changes;
    }return e;
}
struct Queue {
    std::atomic_flag busy=ATOMIC_FLAG_INIT;
    std::atomic<unsigned long long> dropped{0};
    Event events[Capacity]{};unsigned read=0,write=0,used=0;
    bool Push(const Event& e) noexcept {
        if(busy.test_and_set(std::memory_order_acquire)){dropped.fetch_add(1,std::memory_order_relaxed);return false;}
        bool ok=used<Capacity;
        if(ok){events[write]=e;write=(write+1)%Capacity;++used;}else dropped.fetch_add(1,std::memory_order_relaxed);
        busy.clear(std::memory_order_release);return ok;
    }
    bool Pop(Event& e) noexcept {
        if(busy.test_and_set(std::memory_order_acquire))return false;
        const bool ok=used!=0;if(ok){e=events[read];read=(read+1)%Capacity;--used;}
        busy.clear(std::memory_order_release);return ok;
    }
};
}
