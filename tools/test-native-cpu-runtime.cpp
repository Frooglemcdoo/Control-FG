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
static HMODULE verifiedRenderer=nullptr,verifiedD3d=nullptr;
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
#include "../src/native_streaming_policy_runtime.h"
static void TestStreamingWriters(){
    std::uintptr_t low=0,high=0;
    constexpr std::uintptr_t siteAddress=0x400000000ull,targetAddress=siteAddress+0x800000ull;
    assert(native_streaming_policy::RelayRange(siteAddress,targetAddress,0x10000,0x7fffffffffff,low,high));
    assert(low==targetAddress-0x7fff0000ull&&high==siteAddress+0x7fff0000ull);
    // The production allocator's old lower candidate could reach the CALL but
    // not a global 8 MiB above it. Test this layout, not only adjacent cells.
    const auto oldCandidate=siteAddress-0x7fff0000ull;
    assert(targetAddress-(oldCandidate+10)>INT32_MAX);
    assert(targetAddress-(low+10)<=INT32_MAX);
    assert(!native_streaming_policy::RelayRange(0x10000,0x400000000ull,0x10000,0x7fffffffffff,low,high));
    for(unsigned size:{7u,10u})for(int target:{8,16}){
        auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x810000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(page);
        auto* value=reinterpret_cast<int*>(page+0x800000);
        // Preserve R12, load native value 2, establish known arithmetic flags.
        const unsigned char prefix[]={0x41,0x54,0x41,0xbc,2,0,0,0,0x31,0xc0,0xf9};
        memcpy(page,prefix,sizeof(prefix));auto* site=page+sizeof(prefix);
        if(size==7){site[0]=0x44;site[1]=0x89;site[2]=0x25;}
        else{site[0]=0xc7;site[1]=0x05;const int eight=8;memcpy(site+6,&eight,4);}
        const int delta=static_cast<int>(reinterpret_cast<unsigned char*>(value)-(site+size));
        memcpy(site+(size==7?3:2),&delta,4);
        // Capture flags in RAX and R12 in a separate cell, then restore R12.
        unsigned char suffix[]={0x9c,0x58,0x4c,0x89,0x25,0,0,0,0,0x41,0x5c,0xc3};
        const int savedDelta=static_cast<int>((page+264)-(site+size+9));memcpy(suffix+5,&savedDelta,4);
        memcpy(site+size,suffix,sizeof(suffix));assert(FlushInstructionCache(GetCurrentProcess(),page,64));
        using Fn=unsigned long long(*)();auto fn=reinterpret_cast<Fn>(page);
        assert((fn()&0x8c5)==0x45&&*value==(size==7?2:8));
        native_streaming_policy::Writer bad,writer;
        assert(!bad.Prepare(site,value+1,target));assert(!bad.Prepare(site,value,2));
        assert(writer.Prepare(site,value,target));assert(writer.Exchange(true)&&writer.healthy);
        SetLastError(123);*value=0;
        assert((fn()&0x8c5)==0x45&&*value==target&&GetLastError()==123);
        assert(*reinterpret_cast<unsigned long long*>(page+264)==2);
        assert(!writer.Exchange(true)); // Reject an already modified site.
        assert(writer.Exchange(false)&&writer.healthy&&!writer.changed);
        assert((fn()&0x8c5)==0x45&&*value==(size==7?2:8));
        VirtualFree(writer.relay,0,MEM_RELEASE);VirtualFree(page,0,MEM_RELEASE);
    }
    std::puts("PASS R229 streaming relay intersection: old boundary failure, 8 MiB data gap, both encodings, 8/16 overrides, flags, R12, LastError and rollback");
}
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
#include "native-material-resolver-fixture.h"
static void TestMaterialResolver(){
    using namespace native_material_resolver;
    const unsigned char prologue[]={
        0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,
        0x48,0x81,0xec,8,2,0,0,0x48,0x89,0xe5,
        0x49,0x89,0xcc,0x49,0x89,0xd5,
        0x4c,0x89,0x85,0x60,1,0,0,
        0x4c,0x89,0x8d,0x80,1,0,0,0xbe,0x11,1,0,0
    };
    const unsigned char epilogue[]={
        0x48,0x8b,0x85,0x80,1,0,0,
        0x48,0x89,0x18,0x4c,0x89,0x68,8,0x48,0x89,0x78,0x10,0x48,0x89,0x70,0x18,
        0x48,0x81,0xc4,8,2,0,0,0x41,0x5d,0x41,0x5c,0x5f,0x5e,0x5b,0x5d,0xc3
    };
    auto* reference=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(reference);
    auto* candidate=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(candidate);
    for(auto* page:{reference,candidate}){
        memcpy(page,prologue,sizeof(prologue));memcpy(page+sizeof(prologue),kNativeMaterialLoop,sizeof(kNativeMaterialLoop));
        memcpy(page+sizeof(prologue)+sizeof(kNativeMaterialLoop),epilogue,sizeof(epilogue));
    }
    auto* block=reference+sizeof(prologue);
    const unsigned char* functions[]={kFixtureGetTexture,kFixtureConstructTexture,kFixtureNullTexture,kFixtureSetDescriptor};
    const size_t lengths[]={sizeof(kFixtureGetTexture),sizeof(kFixtureConstructTexture),sizeof(kFixtureNullTexture),sizeof(kFixtureSetDescriptor)};
    for(unsigned i=0;i<4;++i){auto* fn=reference+0xb00+i*0x100;memcpy(fn,functions[i],lengths[i]);memcpy(reference+0x800+i*8,&fn,8);}
    auto* global=reference+0x900;memcpy(reference+0x820,&global,8);
    struct Relocation {unsigned offset,size,displacement,slot;};
    const Relocation relocations[]={
        {0x56,6,2,0},{0x6a,6,2,1},{0x7f,7,3,4},{0x89,6,2,2},{0x96,7,3,4},
        {0xa0,6,2,3},{0xc7,7,3,4},{0xd1,6,2,2},{0xde,7,3,4},{0xe8,6,2,3}};
    for(const auto& r:relocations){const auto delta=static_cast<std::int32_t>((reference+0x800+r.slot*8)-(block+r.offset+r.size));memcpy(block+r.offset+r.displacement,&delta,4);}
    descriptorImport=reference+0x820;
    assert(Prepare(candidate+sizeof(prologue)));assert(Exchange(true));
    FlushInstructionCache(GetCurrentProcess(),reference,4096);FlushInstructionCache(GetCurrentProcess(),candidate,4096);
    using Run=void(*)(void*,std::uint64_t,std::uint64_t,std::uint64_t*);
    unsigned char manager[0xb0]{},slots[0x10+35*0x20]{},resources[35][0xf0]{},variants[35][0xc0]{},containers[35][2][16]{},textures[35][0x98]{};
    void* tables[3]{};Write<void*>(manager,0xa8,tables);
    unsigned char expected[0x1000],actual[0x1000];
    for(unsigned trial=0;trial<256;++trial){
        tables[2]=trial%13?slots:nullptr;
        for(unsigned i=0;i<35;++i){
            const unsigned mode=(trial+i)%9;
            const std::int32_t index=trial%17?static_cast<std::int32_t>((trial+i)%4):-1;
            auto* variantBase=variants[i]+(index<0?40:0);
            Write<void*>(slots,0x10+i*0x20,mode==0?nullptr:resources[i]);
            Write<std::int32_t>(resources[i],0xb4,static_cast<std::int32_t>(index));Write<void*>(resources[i],0xe8,variantBase);
            Write<void*>(variantBase,index*40+0x10,mode==1?nullptr:containers[i][0]);
            Write<void*>(variantBase,index*40+0x18,mode<3?nullptr:containers[i][1]);
            for(unsigned j=0;j<2;++j)Write<void*>(containers[i][j],8,mode==3?nullptr:textures[i]);
            Write<std::uint64_t>(textures[i],0x90,mode==4?0:0x1000+trial*35+i);
        }
        memset(expected,0xa5,sizeof(expected));memcpy(actual,expected,sizeof(actual));
        Write<std::uint64_t>(expected,8,UINT64_MAX-20);Write<std::uint64_t>(actual,8,UINT64_MAX-20);
        Write<std::uint64_t>(expected,0x80,0xdeadbeef);Write<std::uint64_t>(actual,0x80,0xdeadbeef);
        std::uint64_t nativeLive[4]{},fastLive[4]{};
        Write<void*>(global,0,expected);SetLastError(123);reinterpret_cast<Run>(reference)(manager,2,17,nativeLive);assert(GetLastError()==123);
        Write<void*>(global,0,actual);SetLastError(123);reinterpret_cast<Run>(candidate)(manager,2,17,fastLive);assert(GetLastError()==123);
        assert(!memcmp(expected,actual,sizeof(actual))&&!memcmp(nativeLive,fastLive,sizeof(nativeLive)));
        assert(Read<std::uint64_t>(actual,8)==14&&fastLive[0]==35&&fastLive[1]==0&&fastLive[2]==17);
    }
    // Verify stack reconstruction at entry and after allocating ABI shadow space.
    for(unsigned offset:{0u,4u,29u}){
        alignas(16) DWORD64 stack[8]{};stack[5]=0x12345678;
        CONTEXT ctx{};ctx.ContextFlags=CONTEXT_FULL;ctx.Rip=reinterpret_cast<DWORD64>(relay+offset);
        ctx.Rsp=reinterpret_cast<DWORD64>(stack+(offset==0?5:0));DWORD64 base=0;
        auto* entry=RtlLookupFunctionEntry(ctx.Rip,&base,nullptr);assert(entry);void* handler=nullptr;DWORD64 frame=0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER,base,ctx.Rip,entry,&ctx,&handler,&frame,nullptr);
        assert(ctx.Rip==0x12345678&&ctx.Rsp==reinterpret_cast<DWORD64>(stack+6));
    }
    assert(Exchange(false));assert(!memcmp(candidate+sizeof(prologue),kNativeMaterialLoop,sizeof(kNativeMaterialLoop)));
    assert(RtlDeleteFunctionTable(&function));VirtualFree(relay,0,MEM_RELEASE);relay=nullptr;site=nullptr;descriptorImport=nullptr;
    VirtualFree(reference,0,MEM_RELEASE);VirtualFree(candidate,0,MEM_RELEASE);
    std::puts("PASS material resolver against native machine-code loop: 256 mixed resource cases, all 35 descriptors, nulls, live registers, counter wrap, LastError, unwind and rollback");
}
static native_prep::AccelerationInfo EstimateFixture(int flags,int count){
    assert(flags==4&&count==123&&GetLastError()==123);
    SetLastError(456);return {{111,222,333}};
}
static void* MapFixture(void* p){assert(GetLastError()==123);SetLastError(456);return p;}
static void UnmapFixture(void*){assert(GetLastError()==123);SetLastError(456);}
static void TestPreparationPhases(){
    using namespace native_prep;
    estimateOriginal=&EstimateFixture;mapOriginal=&MapFixture;unmapOriginal=&UnmapFixture;
    installed=true;
    {
        Frame frame(true,0);int object=0;
        SetLastError(123);assert(MapHook(&object)==&object&&GetLastError()==456);
        SetLastError(123);UnmapHook(&object);assert(GetLastError()==456);
        {Frame nested(false,0);assert(current==&nested);}assert(current==&frame);
        SetLastError(123);auto result=EstimateHook(4,123);
        assert(result.bytes[0]==111&&result.bytes[1]==222&&result.bytes[2]==333&&GetLastError()==456);
        frame.Finish();assert(GetLastError()==456);
    }
    assert(current==nullptr);for(auto& c:cells[0]){native_cpu::Snapshot s;assert(c.Read(s)&&s.samples==1);}
    {Frame incomplete(true,0);incomplete.Finish();}assert(rejected.load()==1);
    installed=false;
    puts("PASS preparation hooks: hidden return ABI, forwarding, LastError, nested frame and incomplete rejection");
}
int main(){
    TestStreamingWriters();
    TestPreparationPhases();
    TestDirtyWriters();
    TestMaterialDiff();
    TestMaterialResolver();
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
