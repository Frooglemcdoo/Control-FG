#pragma once
#include "rt_owned_assembly.h"
namespace rt_setup_probe {
using U64=std::uint64_t;
struct Api {void(*allocate)(void*,U64,U64*,U64*,void**);void(*samplers)(void*,U64,U64*,U64*,void**);void(*initialize)(U64);};
using Mark=void(*)(void*,unsigned);
inline void Run(const Api& api,void* state,bool clear,bool sampler,Mark mark=nullptr,void* context=nullptr){
 auto* bytes=static_cast<unsigned char*>(state);
 if(mark)mark(context,0);
 api.allocate(state,49,reinterpret_cast<U64*>(bytes+0x30),reinterpret_cast<U64*>(bytes+0x40),reinterpret_cast<void**>(bytes+0x50));
 if(mark)mark(context,1);
 api.samplers(state,sampler?4u:0u,reinterpret_cast<U64*>(bytes+0x38),reinterpret_cast<U64*>(bytes+0x48),reinterpret_cast<void**>(bytes+0x58));
 if(mark)mark(context,2);
 if(clear)api.initialize(rt_owned_assembly::Read<U64>(state,0x40));
 if(mark)mark(context,3);
}
}
