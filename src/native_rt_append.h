#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace native_rt_append {
// Verified renderer array<64-byte record> layout. Capacity's high two bits
// are native ownership flags; never rewrite them or take over allocations.
struct Array { unsigned char* data;std::uint32_t size,capacityFlags; };
static_assert(sizeof(Array)==16 && offsetof(Array,size)==8 && offsetof(Array,capacityFlags)==12,"native array ABI");
inline bool Overlap(std::uintptr_t a,std::size_t an,std::uintptr_t b,std::size_t bn) noexcept {
    return a<=b ? b-a<an : a-b<bn;
}
inline unsigned char* AppendIfRoom(Array* array,const void* source) noexcept {
    if(!array||!source||!array->data)return nullptr;
    const auto size=array->size;
    if(size>=(array->capacityFlags&0x3fffffffu))return nullptr;
    auto* destination=array->data+std::size_t(size)*64;
    const auto src=reinterpret_cast<std::uintptr_t>(source),dst=reinterpret_cast<std::uintptr_t>(destination);
    const auto header=reinterpret_cast<std::uintptr_t>(array);
    // Native append performs interleaved 16-byte loads/stores and increments
    // size before reading the source. Delegate all alias cases to that exact
    // behavior rather than using memcpy on overlapping storage.
    if(Overlap(src,64,dst,64)||Overlap(src,64,header,sizeof(Array))||Overlap(dst,64,header,sizeof(Array)))return nullptr;
    array->size=size+1;
    std::memcpy(destination,source,64);
    return destination;
}
// Separate eight-byte material-reference append used by the same RT builder.
// Keep the accepted 64-byte implementation above unchanged for an isolated A/B.
inline unsigned char* AppendPointerIfRoom(Array* array,const void* source) noexcept {
    if(!array||!source||!array->data)return nullptr;
    const auto size=array->size;
    if(size>=(array->capacityFlags&0x3fffffffu))return nullptr;
    auto* destination=array->data+std::size_t(size)*8;
    const auto dst=reinterpret_cast<std::uintptr_t>(destination),header=reinterpret_cast<std::uintptr_t>(array);
    if(Overlap(dst,8,header,sizeof(Array)))return nullptr;
    // Native order: increment count, load one whole pointer, store it. Unlike
    // the 64-byte interleaved copy, one temporary preserves source overlap
    // and source/header aliasing exactly. Never dereference the copied pointer.
    array->size=size+1;
    std::uint64_t value;
    std::memcpy(&value,source,8);
    std::memcpy(destination,&value,8);
    return destination;
}
}
