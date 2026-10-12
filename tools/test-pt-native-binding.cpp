#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_native_binding.h"
#include "../src/pt_native_buffer_observer.h"
#include <cassert>
#include <cstdio>
static int calls=0,expectedMode=-1;
static PTNativeBinding::Binding rows[2]{};
static void Mock(int mode,void* buffer,void* state,int bytes,int rays,int count,const void* bindings) {
    ++calls;assert(mode==expectedMode&&buffer==rows&&state==rows+1&&bytes==1728&&rays==2&&count==2&&bindings==rows);
}
static unsigned char nativeBuffer[0x50]{};
static int createCalls=0;
static void General(std::uint64_t a,std::uint64_t b,unsigned c,unsigned d,std::uint64_t e,std::uint64_t f,void* g){
    assert(a==0x100000010ull&&b==20&&c==3&&d==6&&e==11&&f==12&&g==nativeBuffer);++createCalls;
}
static void Ray(int a,int b,void* c,int d){assert(a==4096&&b==2&&c==nativeBuffer&&d==1);++createCalls;}
int main() {
    const std::uint64_t resource=0x123456780ull,bytes=0x100000010ull,gpu=0x234000000ull;
    std::memcpy(nativeBuffer,&resource,8);std::memcpy(nativeBuffer+0x30,&bytes,8);std::memcpy(nativeBuffer+0x48,&gpu,8);
    PTNativeBufferObserver::Snapshot snap{};PTNativeBufferObserver::Decode(nativeBuffer,snap);
    assert(snap.resource==resource&&snap.bytes==bytes&&snap.address==gpu);
    PTNativeBufferObserver::ForwardGeneral(&General,bytes,20,3,6,11,12,nativeBuffer);
    PTNativeBufferObserver::ForwardRay(&Ray,4096,2,nativeBuffer,1);assert(createCalls==2);
#ifdef _WIN32
    assert(PTNativeBufferObserver::Read(nativeBuffer,snap));
    assert(!PTNativeBufferObserver::Read(nullptr,snap));
    assert(!PTNativeBufferObserver::Read(reinterpret_cast<void*>(1),snap));
#endif
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
    std::puts("PASS M8 native buffer metadata and forwarding; M7 native binding ABI, exact forwarding, offset/stride/overflow checks and guarded reads.");
}
