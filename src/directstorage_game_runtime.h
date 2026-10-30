#pragma once
#include "directstorage_archive_cache.h"
#include "directstorage_runtime_loader.h"
#include "rr_indirect_call_windows.h"
#include "directstorage_pack_tail.h"
namespace native_directstorage {
using Read=BOOL(WINAPI*)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
inline Read original=nullptr;
inline control_storage::ArchiveCache* cache=nullptr;
inline HMODULE storageModule=nullptr;
inline RRIndirectCallPatch patch, syncPatch;
inline std::atomic<unsigned long long> syncCalls{0},syncBytes{0},syncFailures{0},syncSamples{0},syncTicks{0},syncMaxTicks{0},syncArchiveSamples{0},syncPathFailures{0};
inline bool syncInstalled=false;
inline std::atomic<unsigned long long> calls{0},served{0},fallbacks{0};
inline BOOL WINAPI ReadHook(HANDLE file,LPVOID output,DWORD count,LPDWORD received,LPOVERLAPPED overlapped){
    const DWORD saved=GetLastError();calls.fetch_add(1,std::memory_order_relaxed);
    // This exact native callsite uses an explicit offset and a FileEvent. Do
    // not emulate other OVERLAPPED protocols, IOCP, APCs or file-position reads.
    if(cache&&received&&overlapped&&overlapped->hEvent&&
        !(reinterpret_cast<std::uintptr_t>(overlapped->hEvent)&1)){
        const std::uint64_t offset=(std::uint64_t(overlapped->OffsetHigh)<<32)|overlapped->Offset;
        if(cache->TryRead(file,offset,output,count)){
            overlapped->Internal=0;overlapped->InternalHigh=count;*received=count;
            if(SetEvent(overlapped->hEvent)){
                served.fetch_add(1,std::memory_order_relaxed);SetLastError(saved);return TRUE;
            }
        }
    }
    fallbacks.fetch_add(1,std::memory_order_relaxed);SetLastError(saved);
    return original(file,output,count,received,overlapped);
}
using NativeRead=std::uint64_t(*)(void*,void*,void*,std::uint64_t);
using ThreadIndex=int(*)();
inline NativeRead nativeRead=nullptr;
inline ThreadIndex threadIndex=nullptr;
inline control_storage::PackTailPatch packPatch;
inline thread_local HANDLE ownedArchiveHandle=nullptr;
inline std::atomic<unsigned long long> syncEligible{0},syncServed{0},syncServedBytes{0},syncFallback{0},syncPositionFailures{0};
inline std::uint64_t PackReadHook(void* fs,void* file,void* output,std::uint64_t count){
    const DWORD incoming=GetLastError();
    const auto dispatch=reinterpret_cast<NativeRead>((*reinterpret_cast<void***>(fs))[0x90/sizeof(void*)]);
    // PackFile selected its handle from the native TLS-indexed array. Slot 0
    // is shared by unregistered threads and must never use implicit-position hits.
    const int index=threadIndex?threadIndex():0;
    const HANDLE allowed=(dispatch==nativeRead&&file&&index>0)?
        *reinterpret_cast<HANDLE*>(static_cast<unsigned char*>(file)+8):nullptr;
    struct Scope {HANDLE previous;~Scope(){ownedArchiveHandle=previous;}} scope{ownedArchiveHandle};
    ownedArchiveHandle=allowed;SetLastError(incoming);
    return dispatch(fs,file,output,count);
}
// Only the audited, registered-worker PackFile scope may consume cached data.
inline BOOL SyncRead(HANDLE file,LPVOID output,DWORD count,LPDWORD received,LPOVERLAPPED overlapped){
    const DWORD incoming=GetLastError();
    if(cache&&file&&file==ownedArchiveHandle&&!overlapped&&output&&received&&count&&count<=control_storage::BlockBytes){
        syncEligible.fetch_add(1,std::memory_order_relaxed);
        LARGE_INTEGER zero{},position{};
        if(SetFilePointerEx(file,zero,&position,FILE_CURRENT)&&position.QuadPart>=0&&position.QuadPart<=LLONG_MAX-count){
            if(cache->TryRead(file,static_cast<std::uint64_t>(position.QuadPart),output,count)){
                LARGE_INTEGER next{};next.QuadPart=position.QuadPart+count;
                if(SetFilePointerEx(file,next,nullptr,FILE_BEGIN)){
                    *received=count;syncServed.fetch_add(1,std::memory_order_relaxed);
                    syncServedBytes.fetch_add(count,std::memory_order_relaxed);
                    SetLastError(incoming);return TRUE;
                }
                syncPositionFailures.fetch_add(1,std::memory_order_relaxed);
            }
        }else syncPositionFailures.fetch_add(1,std::memory_order_relaxed);
    }
    syncFallback.fetch_add(1,std::memory_order_relaxed);SetLastError(incoming);
    return original(file,output,count,received,overlapped);
}
inline BOOL WINAPI SyncReadHook(HANDLE file,LPVOID output,DWORD count,LPDWORD received,LPOVERLAPPED overlapped){
    const DWORD incoming=GetLastError();
    const bool sample=(syncCalls.fetch_add(1,std::memory_order_relaxed)&63)==0;
    LARGE_INTEGER start{},end{};
    if(sample)QueryPerformanceCounter(&start);
    SetLastError(incoming);
    const BOOL result=SyncRead(file,output,count,received,overlapped);
    const DWORD error=GetLastError();
    if(sample)QueryPerformanceCounter(&end);
    if(result&&received)syncBytes.fetch_add(*received,std::memory_order_relaxed);
    if(!result)syncFailures.fetch_add(1,std::memory_order_relaxed);
    if(sample){
        const auto ticks=static_cast<unsigned long long>(end.QuadPart-start.QuadPart);
        syncSamples.fetch_add(1,std::memory_order_relaxed);
        syncTicks.fetch_add(ticks,std::memory_order_relaxed);
        auto maximum=syncMaxTicks.load(std::memory_order_relaxed);
        while(maximum<ticks&&!syncMaxTicks.compare_exchange_weak(maximum,ticks,std::memory_order_relaxed)){}
        wchar_t path[1024]{};
        const DWORD size=GetFinalPathNameByHandleW(file,path,1024,FILE_NAME_NORMALIZED);
        if(!size||size>=1024)syncPathFailures.fetch_add(1,std::memory_order_relaxed);
        else if(control_storage::ArchivePath(path))syncArchiveSamples.fetch_add(1,std::memory_order_relaxed);
    }
    SetLastError(error);return result;
}
inline bool Install() noexcept {
    char enabled[8]{};
    if(GetEnvironmentVariableA("CONTROLFG_DIRECTSTORAGE",enabled,sizeof(enabled))!=1||enabled[0]!='1')return false;
    HMODULE runtime=GetModuleHandleW(L"rl_rmdwin10_f.dll");
    if(!runtime||!HashMatches(runtime,"A79234E4B34C2488DEAAC3459E8054BE206B5AF78A37723D834C1CBD826CE484")){
        Log("DIRECTSTORAGE_R230 disabled=runtime_hash");return false;
    }
    auto* base=reinterpret_cast<unsigned char*>(runtime);
    constexpr unsigned char expected[]={0xff,0x15,0xee,0xbb,0x07,0};
    if(memcmp(base+0x19351c,expected,6)){Log("DIRECTSTORAGE_R230 disabled=callsite");return false;}
    auto** slot=FindImport(runtime,"KERNEL32.dll","ReadFile");
    const auto read=reinterpret_cast<Read>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"ReadFile"));
    if(!slot||*slot!=reinterpret_cast<void*>(read)){Log("DIRECTSTORAGE_R230 disabled=readfile_import");return false;}
    constexpr unsigned char syncExpected[]={0xff,0x15,0x19,0xbd,0x07,0};
    if(memcmp(base+0x1933f1,syncExpected,6)){Log("DIRECTSTORAGE_R232 disabled=sync_callsite");return false;}
    control_storage::ComPtr<IDStorageFactory> factory;
    control_storage::RuntimeLoadResult load;
    if(!control_storage::LoadRuntimeFactory(ModulePath(selfModule),storageModule,factory,load)){
        Log("DIRECTSTORAGE_R231 disabled=%s hr=0x%08lX win32=%lu",load.stage,static_cast<unsigned long>(load.hr),load.win32);return false;
    }
    Log("DIRECTSTORAGE_R231 factory_ready=1 deployment=exe_directory hr=0x%08lX",static_cast<unsigned long>(load.hr));
    cache=new(std::nothrow) control_storage::ArchiveCache;
    if(!cache||!cache->Initialize(factory.Get())){
        if(cache){cache->CloseIfIdle();delete cache;cache=nullptr;}
        Log("DIRECTSTORAGE_R230 disabled=cache_init");return false;
    }
    original=read;
    if(syncPatch.Prepare(base+0x1933f1,slot,reinterpret_cast<void*>(read),reinterpret_cast<void*>(&SyncReadHook))&&
       syncPatch.Exchange(true)&&syncPatch.healthy){
        syncInstalled=true;
        Log("DIRECTSTORAGE_R233 sync_hook=1 sample_stride=64 cache_scope=registered_pack_worker");
    }else{
        if(syncPatch.changed&&!syncPatch.Exchange(false))Log("DIRECTSTORAGE_R232 sync_rollback_failed=1");
        Log("DIRECTSTORAGE_R232 sync_observer=0");return false;
    }
    original=read;
    if(!patch.Prepare(base+0x19351c,slot,reinterpret_cast<void*>(read),reinterpret_cast<void*>(&ReadHook))||
        !patch.Exchange(true)||!patch.healthy){
        if(patch.changed&&!patch.Exchange(false))Log("DIRECTSTORAGE_R230 rollback_failed=1");
        Log("DIRECTSTORAGE_R230 disabled=patch");return false;
    }
    nativeRead=reinterpret_cast<NativeRead>(base+0x193390);
    threadIndex=reinterpret_cast<ThreadIndex>(base+0x1676e0);
    if(!packPatch.Prepare(base+0x19671b,reinterpret_cast<void*>(&PackReadHook),AllocateExecutableRelayNear(base+0x19671b))||
       !packPatch.Exchange(true)||!packPatch.healthy){
        if(packPatch.changed&&!packPatch.Exchange(false))Log("DIRECTSTORAGE_R233 pack_rollback_failed=1");
        Log("DIRECTSTORAGE_R233 pack_scope=0");return false;
    }
    Log("DIRECTSTORAGE_R234 pack_scope=1 thread_slot_zero=native cache_mib=32 max_read_mib=2 window_kib=64 next_block_prefetch=0");
    Log("DIRECTSTORAGE_R230 installed=1 path=archive_prefetch source=real_rmdp destination=cpu_memory cache_mib=32 decode=native gpu_decompression=0 fallback=native phase_hooks=unchanged");
    return true;
}
inline void Poll(unsigned long long present) noexcept {
    static ULONGLONG next=0;if((!cache&&!syncInstalled)||present%60)return;
    const auto now=GetTickCount64();if(now<next)return;next=now+5000;
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    const double scale=frequency.QuadPart?1000.0/frequency.QuadPart:0;
    Log("DIRECTSTORAGE_R233_SYNC present=%llu calls=%llu completed_bytes=%llu failures=%llu samples=%llu archive_samples=%llu path_failures=%llu sampled_ms=%.3f sampled_max_ms=%.3f",
        present,syncCalls.load(),syncBytes.load(),syncFailures.load(),syncSamples.load(),syncArchiveSamples.load(),syncPathFailures.load(),syncTicks.load()*scale,syncMaxTicks.load()*scale);
    Log("DIRECTSTORAGE_R233_SYNC_CACHE present=%llu eligible=%llu served=%llu served_bytes=%llu native_fallback=%llu position_failures=%llu",
        present,syncEligible.load(),syncServed.load(),syncServedBytes.load(),syncFallback.load(),syncPositionFailures.load());
    if(!cache)return;
    auto& c=cache->counters;
    Log("DIRECTSTORAGE_R234_CACHE present=%llu submitted_requests=%llu submitted_bytes=%llu unused_evicted_bytes=%llu pending_covered=%llu capacity_misses=%llu",
        present,c.submittedRequests.load(),c.submittedBytes.load(),c.unusedEvictedBytes.load(),c.pendingCovered.load(),c.capacityMisses.load());
    Log("DIRECTSTORAGE_R230_STATUS present=%llu calls=%llu served=%llu native_fallback=%llu served_bytes=%llu prefetched_bytes=%llu io_failures=%llu busy_fallback=%llu",
        present,calls.load(),served.load(),fallbacks.load(),c.hitBytes.load(),c.submittedBytes.load(),c.failures.load(),c.busy.load());
}
}
