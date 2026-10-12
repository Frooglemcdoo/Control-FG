#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_native_binding.h"
#include <cassert>
#include <cstdio>
static int calls=0,expectedMode=-1;
static PTNativeBinding::Binding rows[2]{};
static void Mock(int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings) {
    ++calls;assert(mode==expectedMode&&buffer==rows&&state==rows+1&&bytes==1728&&rays==2&&count==2&&bindings==rows);
}
int main() {
    using namespace PTNativeBinding;
    rows[1].tag=17;rows[1].type=3;rows[1].indices=0x1000;rows[1].attributes=0x2000;
    for(unsigned i=0;i<10;++i)rows[1].constants[i]=i*4;
    Binding b=rows[1];std::uint64_t address=0;
    b.constants[0]=8;assert(IndexAddress(b,address)&&address==0x1008);
    b.constants[1]=3;assert(!IndexAddress(b,address));
    b.constants[1]=2;b.constants[0]=1;assert(!IndexAddress(b,address));
    b.constants[0]=8;b.indices=UINT64_MAX-3;assert(!IndexAddress(b,address));
    b.indices=0;assert(!IndexAddress(b,address));
    Forward(&Mock,-1,rows,rows+1,1728,2,2,rows);assert(calls==1);
    expectedMode=7;Forward(&Mock,7,rows,rows+1,1728,2,2,rows);assert(calls==2);
#ifdef _WIN32
    assert(Read(rows,1,b)&&std::memcmp(&b,&rows[1],64)==0);
    assert(!Read(nullptr,0,b)&&!Read(rows,131072,b));
    assert(!Read(reinterpret_cast<const void*>(1),0,b));
#endif
    std::puts("PASS M7 native binding ABI, exact forwarding, offset/stride/overflow checks and guarded reads.");
}
