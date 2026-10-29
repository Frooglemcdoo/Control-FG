#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <stdexcept>
#include <initializer_list>
// Hook lifecycle is tested separately against synthetic executable callsites.
static HMODULE verifiedRenderer=nullptr;
static void Log(const char*,...) noexcept {}
struct Patch {void** slot;void* original;void* hook;const char* label;};
static bool Exchange(Patch& p,bool install){
    DWORD old=0;if(!VirtualProtect(p.slot,sizeof(void*),PAGE_READWRITE,&old))return false;
    void* expected=install?p.original:p.hook;
    void* previous=InterlockedCompareExchangePointer(p.slot,install?p.hook:p.original,expected);
    DWORD ignored=0;VirtualProtect(p.slot,sizeof(void*),old,&ignored);return previous==expected;
}
static void* AllocateExecutableRelayNear(const void* origin) noexcept {
    SYSTEM_INFO si{};GetSystemInfo(&si);const auto gran=si.dwAllocationGranularity;
    const auto start=reinterpret_cast<std::uintptr_t>(origin)&~(std::uintptr_t(gran)-1);
    for(std::uintptr_t offset=gran;offset<0x10000000;offset+=gran){
        if(auto* p=VirtualAlloc(reinterpret_cast<void*>(start+offset),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))return p;
    }return nullptr;
}
static bool Rel32Fits(const unsigned char* from,const void* to) noexcept {
    const auto delta=reinterpret_cast<std::intptr_t>(to)-reinterpret_cast<std::intptr_t>(from);
    return delta>=INT32_MIN&&delta<=INT32_MAX;
}
static void** FindImport(HMODULE,const char*,const char*){return nullptr;}
// Reuse the actual direct-call helper without unrelated albedo installation.
#include "../src/native_cpu_call_patch.h"
#include "../src/native_cpu_runtime.h"
static void* observed=nullptr;static void* observedHandle=nullptr;
static unsigned calls=0;
static void Original(void* self){assert(GetLastError()==123);observed=self;++calls;SetLastError(456);}
static void OriginalWait(void* self,void* handle){Original(self);observedHandle=handle;}
static void Throw(void*){throw std::runtime_error("native exception");}
static unsigned appendFallbacks=0;
static void* AppendFallback(native_rt_append::Array* array,const void* source){
    assert(array&&source);++appendFallbacks;assert(GetLastError()==123);SetLastError(456);return reinterpret_cast<void*>(0x1234);
}
static void TestAppend(){
    using namespace native_rt_append;
    unsigned char data[128]{},source[64]{};for(unsigned i=0;i<64;++i)source[i]=static_cast<unsigned char>(i);
    Array a{data,0,2};original=&AppendFallback;enabled=true;
    SetLastError(123);assert(Hook(&a,source)==data&&a.size==1&&memcmp(data,source,64)==0&&GetLastError()==123&&appendFallbacks==0);
    enabled=false;SetLastError(123);assert(Hook(&a,source)==reinterpret_cast<void*>(0x1234)&&GetLastError()==456&&appendFallbacks==1);
    enabled=true;a.size=2;SetLastError(123);Hook(&a,source);assert(appendFallbacks==2&&a.size==2);
    a.size=0;SetLastError(123);Hook(&a,data+1);assert(appendFallbacks==3&&a.size==0);
    SYSTEM_INFO info{};GetSystemInfo(&info);const auto pageSize=info.dwPageSize;
    auto* pages=static_cast<unsigned char*>(VirtualAlloc(nullptr,pageSize*4,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(pages);
    DWORD old=0;assert(VirtualProtect(pages+pageSize,pageSize,PAGE_NOACCESS,&old));assert(VirtualProtect(pages+pageSize*3,pageSize,PAGE_NOACCESS,&old));
    auto* src=pages+pageSize-64;auto* dst=pages+pageSize*3-64;
    for(unsigned i=0;i<64;++i)src[i]=static_cast<unsigned char>(i+3);
    Array bounded{dst,0,1};assert(AppendIfRoom(&bounded,src)==dst&&memcmp(dst,src,64)==0);
    VirtualFree(pages,0,MEM_RELEASE);enabled=false;
}
int main(){
    TestAppend();
    using namespace native_cpu;
    material=preparation=recording=&Original;wait=&OriginalWait;
    int object=0,handle=0;
    for(auto fn:{&MaterialHook,&PreparationHook,&RecordingHook}){
        SetLastError(123);fn(&object);assert(observed==&object&&GetLastError()==456);
    }
    SetLastError(123);JoinHook(&object,&handle);assert(observedHandle==&handle&&calls==4&&GetLastError()==456);
    Snapshot s;for(unsigned i=0;i<kStages;++i)assert(threads[local.slot].cells[i].Read(s)&&s.samples==1&&threads[local.slot].cells[i].calls==1);
    material=&Throw;bool caught=false;try{MaterialHook(&object);}catch(const std::runtime_error&){caught=true;}assert(caught);
    material=&Original;
    // Both production encoders execute through a real relay and roll back.
    auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(page);
    auto* original=page+128;unsigned char thunk[]={0xff,0x25,0,0,0,0,0,0,0,0,0,0,0,0};
    void* target=reinterpret_cast<void*>(&Original);memcpy(thunk+6,&target,8);memcpy(original,thunk,14);
    const unsigned char prologue[]={0x48,0x83,0xec,0x28};memcpy(page,prologue,4);
    const unsigned char epilogue[]={0x48,0x83,0xc4,0x28,0xc3};
    page[4]=0xe8;const int delta=static_cast<int>(original-(page+9));memcpy(page+5,&delta,4);memcpy(page+9,epilogue,5);
    NativeCpuCallPatch patch{};assert(!NativeCpuPrepareCallPatch(page+4,page+129,reinterpret_cast<void*>(&MaterialHook),&patch));
    assert(NativeCpuPrepareCallPatch(page+4,original,reinterpret_cast<void*>(&MaterialHook),&patch));
    assert(NativeCpuExchangeCall(&patch,true)&&patch.writeHealthy);
    SetLastError(123);reinterpret_cast<Callback>(page)(&object);assert(GetLastError()==456);
    assert(NativeCpuExchangeCall(&patch,false));
    SetLastError(123);reinterpret_cast<Callback>(page)(&object);
    auto** slot=reinterpret_cast<void**>(page+256);*slot=target;
    page[4]=0xff;page[5]=0x15;const int indirect=static_cast<int>(reinterpret_cast<unsigned char*>(slot)-(page+10));memcpy(page+6,&indirect,4);memcpy(page+10,epilogue,5);
    RRIndirectCallPatch six;assert(six.Prepare(page+4,slot,target,reinterpret_cast<void*>(&MaterialHook)));
    assert(six.Exchange(true)&&six.healthy);SetLastError(123);reinterpret_cast<Callback>(page)(&object);assert(GetLastError()==456);
    assert(six.Exchange(false)&&!six.changed);SetLastError(123);reinterpret_cast<Callback>(page)(&object);
    VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS RT append forwarding and guard pages; native CPU runtime: forwarding, last-error, exceptions, exact call validation, executable relays and rollback");
}
