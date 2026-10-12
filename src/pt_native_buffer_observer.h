#pragma once
#include <cstdint>
#include <cstring>
// Verified Steam NativeBuffer layout. Raw CPU metadata only; no COM calls,
// AddRef, lifetime guarantee or GPU completion inference.
namespace PTNativeBufferObserver {
struct Snapshot {std::uint64_t resource=0,bytes=0,address=0;std::uint32_t state=0;};
using CreateGeneral=void(*)(std::uint64_t,std::uint64_t,unsigned,unsigned,std::uint64_t,std::uint64_t,void*);
using CreateRay=void(*)(int,int,void*,int);
using Release=void(*)(void*);
inline void Decode(const unsigned char* bytes,Snapshot& out) noexcept {
    std::memcpy(&out.resource,bytes,8);
    std::memcpy(&out.state,bytes+0x20,4);
    std::memcpy(&out.bytes,bytes+0x30,8);
    std::memcpy(&out.address,bytes+0x48,8);
}
inline void ForwardGeneral(CreateGeneral fn,std::uint64_t a,std::uint64_t b,unsigned c,unsigned d,std::uint64_t e,std::uint64_t f,void* g){fn(a,b,c,d,e,f,g);}
inline void ForwardRay(CreateRay fn,int a,int b,void* c,int d){fn(a,b,c,d);}
#ifdef _WIN32
inline bool Read(const void* buffer,Snapshot& out) noexcept {
    __try {if(!buffer)return false;Decode(static_cast<const unsigned char*>(buffer),out);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
#endif
}
