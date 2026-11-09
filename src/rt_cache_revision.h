#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
namespace rt_cache_revision {
inline constexpr std::size_t ProviderSlots=4096,MaterialSlots=65536;
inline std::size_t ProviderSlot(std::uintptr_t pointer)noexcept{return ((pointer>>4)^(pointer>>19))&(ProviderSlots-1);}
struct Revisions {
 std::atomic<std::uint64_t> providers[ProviderSlots]{},materials[MaterialSlots]{};
 std::atomic<std::uint64_t> creates{0},destroys{0},notifications{0};
 void ProviderCreated(const void* p)noexcept{providers[ProviderSlot(reinterpret_cast<std::uintptr_t>(p))].fetch_add(1,std::memory_order_relaxed);creates.fetch_add(1,std::memory_order_relaxed);}
 void ProviderDestroyed(const void* p)noexcept{providers[ProviderSlot(reinterpret_cast<std::uintptr_t>(p))].fetch_add(1,std::memory_order_relaxed);destroys.fetch_add(1,std::memory_order_relaxed);}
 void MaterialChanged(std::uint16_t id)noexcept{materials[id].fetch_add(1,std::memory_order_relaxed);notifications.fetch_add(1,std::memory_order_relaxed);}
};
// Neither table retains or dereferences a cached native pointer. Bucket collisions
// conservatively invalidate providers. Recycled material IDs receive a new revision
// through the constructor/copy/load notification, even while native dirty is set.
inline std::uint64_t Mix(std::uint64_t h,std::uint64_t v)noexcept{h^=v;h*=1099511628211ull;return h;}
struct Observation {std::uintptr_t provider=0,owner=0;std::int32_t variant=0;std::uint32_t options=0;std::uint64_t guard=0,payload=0;};
struct Entry {Observation value{};bool valid=false;};
enum class Outcome {Cold,Unchanged,Invalidated,Untracked};
inline Outcome Observe(Entry& entry,const Observation& value)noexcept{
 const auto& old=entry.value;
 const bool same=entry.valid&&old.provider==value.provider&&old.owner==value.owner&&old.variant==value.variant&&old.options==value.options;
 const auto result=!same?Outcome::Cold:old.guard!=value.guard?Outcome::Invalidated:old.payload!=value.payload?Outcome::Untracked:Outcome::Unchanged;
 entry.value=value;entry.valid=true;return result;
}
// These hashes are diagnostic only. They are NEVER used to authorize reuse.
}
