#pragma once
#include "directstorage_packed_textures.h"
#include "directstorage_runtime_loader.h"
#include <string>
#include <new>
namespace native_gpu_storage {
using U64=std::uint64_t;
template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
using Request=native_texture_contract::Request;
using QueueResolver=HRESULT(*)(ID3D12CommandQueue*,ID3D12CommandQueue**);inline QueueResolver resolveQueue=nullptr;
using AdjustPending=void(*)(void*,int);
inline void AdjustNativePending(void* owner,int delta){InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(static_cast<unsigned char*>(owner)+0x818),delta);}
inline AdjustPending adjustPending=&AdjustNativePending;
using Ref=void(*)(void*);using Finish=void(*)(void*,int,bool);using Lookup=void(*)(void*,void*,U64,int);
inline Request original=nullptr;inline Ref addRef=nullptr,removeRef=nullptr;inline Finish finish=nullptr;inline Lookup lookup=nullptr;
inline unsigned char *runtime=nullptr,*renderer=nullptr,*d3d=nullptr;
using Allocate=bool(*)(void*,unsigned);inline Allocate allocate=nullptr;
inline NativeCpuCallPatch allocationPatch{};
struct Allocation {void* texture=nullptr;unsigned level=0;U64 container=0;};
inline thread_local Allocation allocation{};
inline RRIndirectCallPatch patch;inline HMODULE storageModule=nullptr;inline Ptr<IDStorageFactory> factory;
inline SRWLOCK lock=SRWLOCK_INIT,fileLock=SRWLOCK_INIT;inline HANDLE wake=nullptr,worker=nullptr;
inline std::atomic<bool> available{false};
inline std::atomic<U64> ineligible{0},layoutRejected{0},sourceRejected{0},capacityRejected{0};
inline std::atomic<U64> offered{0},accepted{0},completed{0},fallback{0},bytes{0},failed{0},batches{0},pending{0};
inline constexpr unsigned QueueCapacity=2048;
struct Job {
    bool used=false;U64 serial=0,offset=0,sourceOffset=0,gpuBytes=0;UINT size=0;unsigned level=0,slot=QueueCapacity;int thread=0,priority=0;
    void* owner=nullptr;void* texture=nullptr;std::string name;
    Ptr<IDStorageFile> file;Ptr<ID3D12Resource> resource;Ptr<ID3D12CommandQueue> queue;
};
inline Job jobs[QueueCapacity];inline ID3D12Resource* active[QueueCapacity]{};inline U64 serial=0,reservedBytes=0;inline unsigned cursor=0;
struct Source {std::wstring path;HANDLE guard=INVALID_HANDLE_VALUE;Ptr<IDStorageFile> file;};
// Archives remain open read-only, just as the native mounted packs do. The guard
// denies replacement/write so resolved ranges cannot silently refer to new data.
inline Source sources[32];
inline U64 Field(const void* p,unsigned offset){return native_loader_stages::Field(p,offset);}
inline unsigned Dword(const void* p,unsigned offset){return native_texture_contract::Dword(p,offset);}
// Admit only a newly allocated, unpublished container. Existing mip resources
// can already be referenced by recorded rendering work and remain native.
inline bool AllocateHook(void* texture,unsigned level){
    const DWORD incoming=GetLastError();auto* levels=reinterpret_cast<unsigned char*>(Field(texture,0xe8));
    const U64 before=levels?Field(levels+U64(level)*0x28,0x10):0;
    SetLastError(incoming);const bool result=allocate(texture,level);const DWORD error=GetLastError();
    levels=reinterpret_cast<unsigned char*>(Field(texture,0xe8));
    const U64 after=levels?Field(levels+U64(level)*0x28,0x10):0;
    allocation=result&&!before&&after?Allocation{texture,level,after}:Allocation{};
    SetLastError(error);return result;
}
inline bool SourceFile(const std::wstring& path,Ptr<IDStorageFile>& file){
    AcquireSRWLockExclusive(&fileLock);struct Unlock{~Unlock(){ReleaseSRWLockExclusive(&fileLock);}} unlock;
    Source* empty=nullptr;for(auto& s:sources){if(s.file&&s.path==path){file=s.file;return true;}if(!s.file&&!empty)empty=&s;}
    if(!empty)return false;
    HANDLE guard=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(guard==INVALID_HANDLE_VALUE)return false;
    Ptr<IDStorageFile> opened;BY_HANDLE_FILE_INFORMATION a{},b{};
    const bool ok=GetFileInformationByHandle(guard,&a)&&SUCCEEDED(factory->OpenFile(path.c_str(),IID_PPV_ARGS(&opened)))&&SUCCEEDED(opened->GetFileInformation(&b))&&
        a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow&&a.nFileSizeHigh==b.nFileSizeHigh&&a.nFileSizeLow==b.nFileSizeLow;
    if(!ok){CloseHandle(guard);return false;}empty->path=path;empty->guard=guard;empty->file=opened;file=opened;return true;
}
inline void Retire(Job& job,bool success,bool retry){
    if(success){finish(job.texture,static_cast<int>(job.level),true);++completed;}
    else if(retry){original(job.owner,job.name.c_str(),job.texture,job.offset,job.size,nullptr,job.thread,job.priority,false,reinterpret_cast<void*>(U64(job.level)));++fallback;}
    else finish(job.texture,static_cast<int>(job.level),false);
    removeRef(job.texture);
    // hasLoadsRemaining = queued files (528) + pending completions (818).
    // Reserve only a global pending completion, never a phantom disk-queue
    // item: the loader thread must not spin selecting nonexistent requests.
    // Release after publication/retry is queued. Per-thread native completion
    // arrays remain untouched and cannot consume this reservation.
    adjustPending(job.owner,-1);
    AcquireSRWLockExclusive(&lock);reservedBytes-=job.size;active[job.slot]=nullptr;ReleaseSRWLockExclusive(&lock);--pending;job={};
}
inline DWORD WINAPI Work(void*){
    struct Lane {control_storage::PackedTextureBatch transfer;Job jobs[control_storage::PackedTextureBatch::Capacity];unsigned count=0;bool initialized=false;};
    Lane lanes[2];
    for(;;){
        for(auto& lane:lanes){
            if(lane.count){
                HRESULT result=E_PENDING;
                if(!lane.transfer.Poll(result))continue;
                const bool success=SUCCEEDED(result);const bool lost=result==DXGI_ERROR_DEVICE_REMOVED||result==DXGI_ERROR_DEVICE_RESET;
                if(!success){++failed;Log("DIRECTSTORAGE_R241 batch_failed=1 hr=0x%08lX count=%u",static_cast<unsigned long>(result),lane.count);}
                if(lost)available=false;
                for(unsigned i=0;i<lane.count;++i)Retire(lane.jobs[i],success,!lost);lane.count=0;
            }
            U64 laneBytes=0;
            AcquireSRWLockExclusive(&lock);
            for(;lane.count<control_storage::PackedTextureBatch::Capacity;++lane.count){
                Job* best=nullptr;for(auto& j:jobs)if(j.used&&(!best||j.priority>best->priority||(j.priority==best->priority&&j.serial<best->serial)))best=&j;
                if(!best||laneBytes+best->gpuBytes>128ull*1024*1024)break;laneBytes+=best->gpuBytes;lane.jobs[lane.count]=std::move(*best);*best={};
            }
            ReleaseSRWLockExclusive(&lock);if(!lane.count)continue;
            HRESULT hr=available?S_OK:E_ABORT;
            if(SUCCEEDED(hr)&&!lane.initialized){hr=lane.transfer.Initialize(factory.Get(),lane.jobs[0].queue.Get());lane.initialized=SUCCEEDED(hr);if(!lane.initialized){available=false;lane.transfer.Close();}}
            control_storage::PackedTextureBatch::Input inputs[control_storage::PackedTextureBatch::Capacity]{};
            for(unsigned i=0;i<lane.count;++i){const auto& j=lane.jobs[i];inputs[i]={j.file.Get(),j.resource.Get(),j.sourceOffset,j.size,D3D12_RESOURCE_STATES(0xc0)};}
            if(SUCCEEDED(hr))hr=lane.transfer.Submit(inputs,lane.count);
            if(FAILED(hr)){++failed;Log("DIRECTSTORAGE_R241 batch_rejected=1 hr=0x%08lX count=%u",static_cast<unsigned long>(hr),lane.count);
                for(unsigned i=0;i<lane.count;++i)Retire(lane.jobs[i],false,true);lane.count=0;
            }else ++batches;
        }
        HANDLE events[3]={wake};DWORD count=1;
        for(auto& lane:lanes)if(lane.count)events[count++]=lane.transfer.Event();
        // New requests can fill the other lane while I/O or GPU work is pending.
        // Timeout is only for device-loss / event-registration recovery.
        WaitForMultipleObjects(count,events,FALSE,count>1?1000:INFINITE);
    }
}
inline bool Describe(void* texture,unsigned level,U64 size,Job& job){
    if(!texture||level>=Dword(texture,0xf0)||level>=32||static_cast<int>(Dword(texture,0x24))<=0)return false;
    auto* levels=reinterpret_cast<unsigned char*>(Field(texture,0xe8));if(!levels)return false;
    auto* e=levels+U64(level)*0x28;if(Dword(e,0x20)!=1||Field(e,0x18))return false;
    auto* container=reinterpret_cast<void*>(Field(e,0x10));if(!container)return false;
    auto* native=reinterpret_cast<unsigned char*>(Field(container,8));if(!native||native[0x80]!=0||Dword(native,0x38)!=0)return false;
    auto* state=reinterpret_cast<void*>(Field(native,0x50));if(Dword(state?state:native+0x40,0x20)!=0xc0)return false;
    auto* metadataOwner=reinterpret_cast<void*>(Field(texture,0x30));if(!metadataOwner)return false;
    auto* metadata=reinterpret_cast<void*>(Field(metadataOwner,0x20));if(!metadata||Dword(metadata,0x84)!=0)return false;
    using Type=int(*)(void*);using Expected=int(*)();
    const auto type=reinterpret_cast<Type>((*reinterpret_cast<void***>(metadata))[0x98/8]);
    if(type(metadata)!=reinterpret_cast<Expected>(renderer+0x155bc0)())return false;
    auto* resource=reinterpret_cast<ID3D12Resource*>(Field(native,0x88));if(!resource)return false;
    Ptr<ID3D12Device> device;if(FAILED(resource->GetDevice(IID_PPV_ARGS(&device))))return false;
    native_texture_layout::Layout layout{};if(!native_texture_layout::Build(device.Get(),resource->GetDesc(),layout)||layout.compactBytes!=size)return false;
    for(unsigned m=0;m<layout.count;++m)if(layout.mips[m].rowBytes%4)return false;
    auto* engineQueue=reinterpret_cast<void*>(Field(d3d,0x111c38));if(!engineQueue)return false;
    auto* queue=reinterpret_cast<ID3D12CommandQueue*>(Field(engineQueue,0x10));if(!queue)return false;
    // An unregistered worker (thread zero) must take locked deferred publication.
    if(!Dword(renderer,0x7ec134))return false;
    job.gpuBytes=layout.compactBytes+layout.footprintBytes+U64(layout.count)*512;
    if(job.gpuBytes>128ull*1024*1024)return false;
    Ptr<ID3D12CommandQueue> nativeQueue;
    const HRESULT resolved=resolveQueue?resolveQueue(queue,&nativeQueue):queue->QueryInterface(IID_PPV_ARGS(&nativeQueue));
    if(FAILED(resolved)||!nativeQueue||nativeQueue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
    Ptr<ID3D12Device> queueDevice;if(FAILED(nativeQueue->GetDevice(IID_PPV_ARGS(&queueDevice)))||queueDevice.Get()!=device.Get())return false;
    job.resource=resource;job.queue=nativeQueue;return true;
}
inline bool Resolve(const char* name,U64 offset,U64 size,Job& job){
    if(!name||strnlen_s(name,512)>=512)return false;
    auto* fs=reinterpret_cast<void*>(Field(runtime,0x3a3178));if(!fs)return false;
    alignas(16) unsigned char record[0x188]{};const auto nameAddress=reinterpret_cast<U64>(name);memcpy(record+8,&nameAddress,8);
    lookup(fs,record,1,0x188);auto* pack=reinterpret_cast<void*>(Field(record,0x120));
    const U64 base=Field(record,0x128),length=Field(record,0x130);
    loader_candidate::Range range{};
    if(!pack||!loader_candidate::Resolve(base,Field(pack,0x288),length,offset,size,range))return false;
    auto* host=reinterpret_cast<const char*>(Field(pack,0x170));if(!host||strnlen_s(host,1024)>=1024)return false;
    wchar_t wide[1024]{};if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,host,-1,wide,1024))return false;
    const DWORD required=GetFullPathNameW(wide,0,nullptr,nullptr);if(!required||required>32768)return false;
    std::wstring full(required,L'\0');const DWORD n=GetFullPathNameW(wide,required,full.data(),nullptr);
    if(!n||n>=required)return false;full.resize(n);if(!SourceFile(full,job.file))return false;
    job.sourceOffset=range.offset;return true;
}
using DescribeFn=bool(*)(void*,unsigned,U64,Job&);using ResolveFn=bool(*)(const char*,U64,U64,Job&);
inline DescribeFn describe=&Describe;inline ResolveFn resolve=&Resolve;
inline std::int64_t RequestHook(void* owner,const char* name,void* texture,U64 offset,U64 size,char* buffer,int thread,int priority,bool flag,void* user){
    const DWORD incoming=GetLastError();++offered;
    const auto fresh=allocation;allocation={};
    if(pending.load(std::memory_order_relaxed)>=QueueCapacity){++capacityRejected;++fallback;SetLastError(incoming);return original(owner,name,texture,offset,size,buffer,thread,priority,flag,user);}
    if(available&&fresh.texture==texture&&fresh.level==reinterpret_cast<U64>(user)&&fresh.container&&!buffer&&!flag&&size&&size<=32u*1024u*1024u&&(thread==3||thread==14)&&reinterpret_cast<U64>(user)<32){
        Job j{};j.owner=owner;j.texture=texture;j.offset=offset;j.size=static_cast<UINT>(size);j.level=static_cast<unsigned>(reinterpret_cast<U64>(user));j.thread=thread;j.priority=priority;
        if(!describe(texture,j.level,size,j))++layoutRejected;
        else if(!resolve(name,offset,size,j))++sourceRejected;
        else {
            j.name=name;U64 ticket=0;
            AcquireSRWLockExclusive(&lock);bool duplicate=false;for(auto* p:active)if(p&&p==j.resource.Get())duplicate=true;
            if(!duplicate&&available&&pending.load()<QueueCapacity){
                for(unsigned scan=0;scan<QueueCapacity;++scan){const unsigned i=cursor++%QueueCapacity;if(!active[i]&&!jobs[i].used){addRef(texture);adjustPending(owner,1);j.used=true;j.serial=++serial;j.slot=i;ticket=j.serial;active[i]=j.resource.Get();jobs[i]=std::move(j);reservedBytes+=size;++pending;++accepted;bytes+=size;break;}}
            }
            ReleaseSRWLockExclusive(&lock);
            if(ticket){SetEvent(wake);SetLastError(incoming);return static_cast<std::int64_t>(ticket|0x8000000000000000ull);}
            ++capacityRejected;
        }
    }else ++ineligible;
    ++fallback;SetLastError(incoming);return original(owner,name,texture,offset,size,buffer,thread,priority,flag,user);
}
inline bool Install(){
    char value[8]{};if(GetEnvironmentVariableA("CONTROLFG_NATIVE_GPU_STORAGE",value,8)!=1||value[0]!='1')return false;
    const char* conflicts[]={"CONTROLFG_TEXTURE_CONTRACT","CONTROLFG_EARLY_STORAGE","CONTROLFG_DIRECTSTORAGE"};
    for(const auto* key:conflicts)if(GetEnvironmentVariableA(key,value,8)==1&&value[0]=='1'){Log("DIRECTSTORAGE_R241 disabled=conflicting_mode key=%s",key);return false;}
    auto r=GetModuleHandleW(L"renderer_rmdwin10_f.dll"),n=GetModuleHandleW(L"rl_rmdwin10_f.dll"),d=GetModuleHandleW(L"d3d_rmdwin10_f.dll");
    if(!r||!n||!d||!HashMatches(r,"EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433")||!HashMatches(n,"A79234E4B34C2488DEAAC3459E8054BE206B5AF78A37723D834C1CBD826CE484")||!HashMatches(d,"CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5")){Log("DIRECTSTORAGE_R241 disabled=module_hash");return false;}
    renderer=reinterpret_cast<unsigned char*>(r);runtime=reinterpret_cast<unsigned char*>(n);d3d=reinterpret_cast<unsigned char*>(d);
    original=reinterpret_cast<Request>(runtime+0xfa2b0);addRef=reinterpret_cast<Ref>(runtime+0x12e160);removeRef=reinterpret_cast<Ref>(runtime+0x12e190);
    allocate=reinterpret_cast<Allocate>(renderer+0x159960);
    finish=reinterpret_cast<Finish>(renderer+0x158390);lookup=reinterpret_cast<Lookup>(runtime+0x18f760);
    control_storage::RuntimeLoadResult result;if(!control_storage::LoadRuntimeFactory(ModulePath(selfModule),storageModule,factory,result)){Log("DIRECTSTORAGE_R241 disabled=%s hr=0x%08lX",result.stage,static_cast<unsigned long>(result.hr));return false;}
    if(!patch.Prepare(renderer+0x158247,reinterpret_cast<void**>(renderer+0x5e0140),reinterpret_cast<void*>(original),reinterpret_cast<void*>(&RequestHook)))return false;
    if(!NativeCpuPrepareCallPatch(renderer+0x1580d7,reinterpret_cast<void*>(allocate),reinterpret_cast<void*>(&AllocateHook),&allocationPatch))return false;
    wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!wake)return false;worker=CreateThread(nullptr,0,&Work,nullptr,0,nullptr);if(!worker)return false;
    if(!NativeCpuExchangeCall(&allocationPatch,true))return false;
    if(!allocationPatch.writeHealthy){NativeCpuExchangeCall(&allocationPatch,false);return false;}
    if(!patch.Exchange(true)){NativeCpuExchangeCall(&allocationPatch,false);return false;}
    if(!patch.healthy){patch.Exchange(false);NativeCpuExchangeCall(&allocationPatch,false);return false;}
    available=true;Log("DIRECTSTORAGE_R241 installed=1 native_gpu_redirect=1 source=archive destination=gpu_buffer repack=gpu texture_copy=gpu native_cpu_payload_read=0 compression=none unsupported=native");return true;
}
inline void Poll(U64 present){
    static ULONGLONG next=0;if(!worker||present%60)return;const auto now=GetTickCount64();if(now<next)return;next=now+5000;
    Log("DIRECTSTORAGE_R241_STATUS present=%llu available=%u offered=%llu accepted=%llu completed=%llu fallback=%llu failed=%llu pending=%llu batches=%llu accepted_bytes=%llu ineligible=%llu layout_rejected=%llu source_rejected=%llu capacity_rejected=%llu cpu_payload_bytes=0",present,unsigned(available.load()),offered.load(),accepted.load(),completed.load(),fallback.load(),failed.load(),pending.load(),batches.load(),bytes.load(),ineligible.load(),layoutRejected.load(),sourceRejected.load(),capacityRejected.load());
}
}
