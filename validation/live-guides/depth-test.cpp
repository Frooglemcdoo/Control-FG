#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../../src/rr_albedo_depth.h"
int main(){
 alignas(8) unsigned char texture[0xB0]{},state[0xA00]{},tracker[0x30]{};
 auto put32=[](unsigned char* p,unsigned v){std::memcpy(p,&v,4);};
 auto putPtr=[](unsigned char* p,std::uintptr_t v){std::memcpy(p,&v,sizeof(v));};
 putPtr(texture+0x50,reinterpret_cast<std::uintptr_t>(tracker));
 putPtr(texture+0xA0,1234);putPtr(state+0x40,1234);putPtr(state+0x9E0,reinterpret_cast<std::uintptr_t>(texture));
 put32(tracker+0x20,0xC0);
 assert(RRAlbedoNativeDepthBinding(texture,state,true));assert(!RRAlbedoNativeDepthBinding(texture,state,false));
 // Worker policy must not read the primary recording tracker at all.
 putPtr(texture+0x50,1);assert(RRAlbedoNativeDepthBinding(texture,state,true));
 putPtr(texture+0x50,reinterpret_cast<std::uintptr_t>(tracker));
 put32(tracker+0x20,0x10);assert(RRAlbedoNativeDepthBinding(texture,state,false));
 putPtr(state+0x40,4321);assert(!RRAlbedoNativeDepthBinding(texture,state,true));
 putPtr(state+0x40,1234);putPtr(state+0x9E0,1);assert(!RRAlbedoNativeDepthBinding(texture,state,true));
 putPtr(state+0x9E0,reinterpret_cast<std::uintptr_t>(texture));
 putPtr(texture+0xA0,0);putPtr(state+0x40,0);assert(!RRAlbedoNativeDepthBinding(texture,state,true));
 putPtr(texture+0xA0,1234);putPtr(state+0x40,1234);
 for(unsigned flags:{0x10u,0x20u,0x30u}){put32(texture+0x38,flags);assert(!RRAlbedoNativeDepthBinding(texture,state,true));assert(!RRAlbedoNativeDepthBinding(texture,state,false));}
 put32(texture+0x38,0);texture[0x68]=1;assert(!RRAlbedoNativeDepthBinding(texture,state,true));assert(!RRAlbedoNativeDepthBinding(texture,state,false));texture[0x68]=0;
 assert(!RRAlbedoNativeDepthBinding(nullptr,state,true));assert(!RRAlbedoNativeDepthBinding(texture,nullptr,true));
 putPtr(texture+0x50,0);put32(texture+0x60,0x10);assert(RRAlbedoNativeDepthBinding(texture,state,false));
 puts("PASS: actual depth policy preserves inherited worker DSV without reading tracker; rejects changed/null descriptors, wrong texture and unsupported flags; primary exact depth-write check retained");
}
