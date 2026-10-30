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
#include <thread>
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
    // Eight-byte copies at both read and write guard-page boundaries.
    bounded={pages+pageSize*3-8,0,1};src=pages+pageSize-8;
    assert(AppendPointerIfRoom(&bounded,src)==bounded.data&&memcmp(bounded.data,src,8)==0);
    native_rt_pointer::original=&AppendFallback;native_rt_pointer::enabled=true;
    a.size=0;SetLastError(123);assert(native_rt_pointer::Hook(&a,source)==data&&a.size==1&&GetLastError()==123);
    native_rt_pointer::enabled=false;SetLastError(123);assert(native_rt_pointer::Hook(&a,source)==reinterpret_cast<void*>(0x1234)&&GetLastError()==456);
    native_rt_pointer::enabled=true;a.size=2;SetLastError(123);assert(native_rt_pointer::Hook(&a,source)==reinterpret_cast<void*>(0x1234)&&a.size==2);
    native_rt_pointer::enabled=false;
    VirtualFree(pages,0,MEM_RELEASE);enabled=false;
}
static void TestDirtyWriters(){
    for(const auto& source:native_dirty::sites){
        native_dirty::Site probe=source;
        auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(page);
        unsigned n=0;
        const bool r14=probe.bytes[0]==0x41&&probe.bytes[2]==0x86;
        const bool r15=probe.bytes[0]==0x41&&probe.bytes[2]==0x87;
        const bool rsi=probe.bytes[0]==0xc6&&probe.bytes[1]==0x86;
        if(r14||r15){page[n++]=0x41;page[n++]=r14?0x56:0x57;}else if(rsi)page[n++]=0x56;
        page[n++]=0x31;page[n++]=0xc0;page[n++]=0xf9; // xor eax,eax; stc
        if(r14||r15){page[n++]=0x49;page[n++]=0x89;page[n++]=r14?0xce:0xcf;}
        else if(rsi){page[n++]=0x48;page[n++]=0x89;page[n++]=0xce;}
        else if(probe.bytes[1]==0x80){page[n++]=0x48;page[n++]=0x89;page[n++]=0xc8;}
        auto* site=page+n;memcpy(site,probe.bytes,probe.size);n+=probe.size;
        page[n++]=0x9c;page[n++]=0x58; // capture flags in return register
        if(r14||r15){page[n++]=0x41;page[n++]=r14?0x5e:0x5f;}else if(rsi)page[n++]=0x5e;
        page[n++]=0xc3;FlushInstructionCache(GetCurrentProcess(),page,n);
        using Fn=unsigned long long(*)(void*);auto fn=reinterpret_cast<Fn>(page);
        unsigned char manager[0x119]{};
        assert((fn(manager)&0x8c5)==0x45&&manager[0x118]==1);
        assert(!native_dirty::Prepare(probe,site+1));
        assert(native_dirty::Prepare(probe,site));
        assert(native_dirty::Exchange(probe,true)&&probe.healthy);
        manager[0x118]=0;SetLastError(123);
        assert((fn(manager)&0x8c5)==0x45&&manager[0x118]==1&&GetLastError()==123);
        assert(InterlockedCompareExchange64(probe.count,0,0)==1);
        // Validate unwind at entry, after pushfq, at counter, at popfq and ret.
        for(unsigned offset:{0u,1u,1u+probe.size,9u+probe.size,10u+probe.size}){
            alignas(16) DWORD64 stack[4]{0x246,0x12345678,0,0};
            CONTEXT ctx{};ctx.ContextFlags=CONTEXT_FULL;
            ctx.Rip=reinterpret_cast<DWORD64>(probe.code+offset);
            ctx.Rsp=reinterpret_cast<DWORD64>(stack+((offset==0||offset==10+probe.size)?1:0));
            DWORD64 imageBase=0;auto* entry=RtlLookupFunctionEntry(ctx.Rip,&imageBase,nullptr);assert(entry);
            void* handler=nullptr;DWORD64 frame=0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER,imageBase,ctx.Rip,entry,&ctx,&handler,&frame,nullptr);
            assert(ctx.Rip==0x12345678&&ctx.Rsp==reinterpret_cast<DWORD64>(stack+2));
        }
        std::thread workers[4];
        for(auto& worker:workers)worker=std::thread([&]{unsigned char own[0x119]{};for(unsigned j=0;j<1000;++j){assert((fn(own)&0x8c5)==0x45&&own[0x118]==1);}});
        for(auto& worker:workers)worker.join();
        assert(InterlockedCompareExchange64(probe.count,0,0)==4001);
        assert(native_dirty::Exchange(probe,false)&&probe.healthy);
        manager[0x118]=0;assert((fn(manager)&0x8c5)==0x45&&manager[0x118]==1);
        assert(InterlockedCompareExchange64(probe.count,0,0)==4001);
        assert(RtlDeleteFunctionTable(&probe.unwind));
        VirtualFree(probe.code,0,MEM_RELEASE);VirtualFree(const_cast<LONG64*>(probe.count),0,MEM_RELEASE);VirtualFree(page,0,MEM_RELEASE);
    }
    std::puts("PASS all eight dirty-store probes: original writes, flags, LastError, exact counters, unwind at instruction boundaries and rollback");
}
static unsigned diffCalls=0;
static void* diffExpected[4]{};
static bool diffMutate=true;
static void DiffOriginal(void* manager,void* handle,void* source,void* textures){
    assert(GetLastError()==123);assert(manager==diffExpected[0]&&handle==diffExpected[1]&&source==diffExpected[2]&&textures==diffExpected[3]);++diffCalls;
    if(diffMutate){
        using native_workload::Load;
        auto* hash=Load<std::uint32_t*>(manager,0x98);auto* record=Load<std::uint32_t*>(manager,0x88);
        ++*hash;record[3]=99;record[0x148/4]=77;
    }
    SetLastError(456);
}
static void TestMaterialDiff(){
    using namespace native_material_diff;
    unsigned char manager[0x100]{};std::uint32_t handle=0,count=1,hash=10,record[Words]{},incoming[Words]{};
    std::uint64_t key=5;const void* pair[2]{&key,incoming};
    void* hashes=&hash;void* records=record;void* keys=&key;
    memcpy(manager+0x80,&count,4);memcpy(manager+0x98,&hashes,8);memcpy(manager+0x88,&records,8);memcpy(manager+0x78,&keys,8);
    diffExpected[0]=manager;diffExpected[1]=&handle;diffExpected[2]=pair;diffExpected[3]=reinterpret_cast<void*>(0x1234);
    original=&DiffOriginal;ordinal=0;
    SetLastError(123);Hook(manager,&handle,pair,diffExpected[3]);assert(GetLastError()==456&&diffCalls==1);
    Event event;assert(queue.Pop(event)&&event.entry==0&&event.oldHash==10&&event.newHash==11&&event.changes==2&&event.offset[0]==12&&event.committedDerived==77);
    SetLastError(123);Hook(manager,&handle,pair,diffExpected[3]);assert(GetLastError()==456&&!queue.Pop(event));
    diffMutate=false;SetLastError(123);Hook(manager,&handle,pair,diffExpected[3]);assert(changed==2&&!queue.Pop(event));
    handle=2;SetLastError(123);Hook(manager,&handle,pair,diffExpected[3]);assert(invalid==1&&GetLastError()==456);
    diffExpected[1]=nullptr;SetLastError(123);Hook(manager,nullptr,pair,diffExpected[3]);assert(GetLastError()==456&&diffCalls==5);
    std::puts("PASS material diff runtime: four-argument forwarding, committed changes, sampling, unchanged/invalid/null paths and LastError");
}
int main(){
    TestDirtyWriters();
    TestMaterialDiff();
    // No opt-in (or malformed opt-in) must never inspect renderer memory.
    for(const char* value:{static_cast<const char*>(nullptr),"0","10","true","123456789"}){
        SetEnvironmentVariableA("CONTROLFG_CPU_APPEND_HOOKS",value);
        assert(!native_cpu::AppendHooksRequested());
        native_cpu::InstallAppendHooks(reinterpret_cast<HMODULE>(1));
        assert(!native_rt_append::installed&&!native_rt_pointer::installed);
    }
    SetEnvironmentVariableA("CONTROLFG_CPU_APPEND_HOOKS","1");
    assert(native_cpu::AppendHooksRequested());
    SetEnvironmentVariableA("CONTROLFG_CPU_APPEND_HOOKS",nullptr);
    TestAppend();
    using namespace native_cpu;
    material=preparation=recording=&Original;wait=&OriginalWait;
    int object=0,handle=0;
    for(auto fn:{&MaterialHook,&PreparationHook,&RecordingHook}){
        SetLastError(123);fn(&object);assert(observed==&object&&GetLastError()==456);
    }
    SetLastError(123);JoinHook(&object,&handle);assert(observedHandle==&handle&&calls==4&&GetLastError()==456);
    assert(native_rt_append::completedFast.load()==0&&native_rt_append::completedNative.load()==0);
    assert(native_rt_pointer::completedFast.load()==0&&native_rt_pointer::completedNative.load()==0);
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
    // Scalar workload capture forwards the native tail and preserves LastError.
    unsigned char manager[0x108]{},context[0x9c]{},job[0x20]{};
    std::uint32_t rebuilds=42,entries=12;std::uint64_t unique=9;
    memcpy(manager+0x80,&entries,4);memcpy(manager+0xd0,&entries,4);memcpy(manager+0x100,&unique,8);
    const void* contextPtr=context;memcpy(job+0x18,&contextPtr,sizeof(contextPtr));
    memcpy(context+0x18,&entries,4);
    materialTail=&Original;rebuildCounter=&rebuilds;workloadEnabled=true;
    local.ordinal[Material]=65;local.ordinal[Preparation]=65;
    SetLastError(123);MaterialTailHook(manager);assert(observed==manager&&GetLastError()==456);
    CapturePreparation(job);assert(GetLastError()==456);
    native_workload::Snapshot workload;
    assert(materialWorkload[local.slot].Read(workload)&&workload.samples==1&&workload.last[2]==9&&workload.last[3]==42);
    assert(preparationWorkload[local.slot].Read(workload)&&workload.samples==1&&workload.last[0]==12);
    local.ordinal[Preparation]=66;CapturePreparation(reinterpret_cast<void*>(1));
    workloadEnabled=false;CapturePreparation(reinterpret_cast<void*>(1));
    std::puts("PASS RT append forwarding and guard pages; native CPU runtime: forwarding, last-error, exceptions, exact call validation, executable relays and rollback");
}
