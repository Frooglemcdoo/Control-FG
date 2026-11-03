#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
// Steam d3d 33a40 reads bindings at +8/+16 and ten dwords at +24,
// advances 64 bytes at 34378; copied fields form the 56-byte SBT local root.
namespace PTNativeBinding {
struct Binding {std::uint32_t tag,type;std::uint64_t indices,attributes;std::uint32_t constants[10];};
static_assert(sizeof(Binding)==64&&offsetof(Binding,constants)==24,"native binding ABI");
inline bool IndexAddress(const Binding& b,std::uint64_t& out) noexcept {
    out=0;if(!b.indices||(b.constants[1]!=2&&b.constants[1]!=4)||b.constants[0]>UINT64_MAX-b.indices)return false;
    out=b.indices+b.constants[0];return out%b.constants[1]==0;
}
using Fill=void(*)(int,void*,void*,int,int,int,const void*);
inline void Forward(Fill fn,int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings) {
    fn(mode,buffer,state,bytes,rays,count,bindings);
}
#ifdef _WIN32
inline bool Read(const void* source,unsigned index,Binding& out) noexcept {
    __try {if(!source||index>=131072)return false;std::memcpy(&out,static_cast<const unsigned char*>(source)+std::size_t(index)*64,64);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
#endif
}
