#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace control_rr_provider {
// This authenticates CPU candidate bytes against CURRENT THREAD provider data.
// It does not establish GPU resource lifetime, shader selection, or frame identity.
struct Entry { std::int32_t offset, reserved, size, revisionIndex; };
static_assert(sizeof(Entry)==16, "native entry stride");
struct Access {
 void* user;
 bool (*entry)(void*,std::int32_t,Entry&);
 // Native cmpProviderData returns availability separately from memcmp's result.
 bool (*compare)(void*,std::int32_t,const void*,int&);
};
template<class T> bool Authenticate(const Access& a,std::int32_t id,
                                  const T& candidate,T& output) noexcept {
 static_assert(std::is_trivially_copyable<T>::value,"byte snapshot required");
 // Copy before clearing, permitting candidate/output aliasing.
 T copy=candidate; output={};
 if(id<0||!a.entry||!a.compare)return false;
 Entry before{},after{};
 if(!a.entry(a.user,id,before)||before.offset<0||before.size!=sizeof(T))return false;
 int difference=1;
 if(!a.compare(a.user,id,&copy,difference)||difference!=0)return false;
 if(!a.entry(a.user,id,after)||std::memcmp(&before,&after,sizeof(before)))return false;
 output=copy;return true;
}
// Read the shader's actual current-thread bytes. A CPU mirror can refer to a
// different view or have advanced on another thread. Never compare an unchecked
// size, read outside the native 256 KiB provider arena, or retain a changed entry.
template<class T> bool Snapshot(const Access& a,std::int32_t id,
 bool (*read)(void*,std::int32_t,void*,std::size_t),T& output) noexcept {
 static_assert(std::is_trivially_copyable<T>::value,"byte snapshot required");
 output={};Entry before{},after{};T candidate{},authenticated{};
 constexpr std::size_t arena=0x40000;
 if(id<0||!a.entry||!read||!a.entry(a.user,id,before)||before.offset<0||
    before.size!=sizeof(T)||sizeof(T)>arena||std::size_t(before.offset)>arena-sizeof(T))return false;
 if(!read(a.user,before.offset,&candidate,sizeof(candidate))||
    !Authenticate(a,id,candidate,authenticated)||!a.entry(a.user,id,after)||
    std::memcmp(&before,&after,sizeof(before)))return false;
 output=authenticated;return true;
}

}
