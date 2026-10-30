#pragma once
#include "directstorage_archive_cache.h"
#include "rr_indirect_call_windows.h"
namespace native_directstorage {
using Read=BOOL(WINAPI*)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
inline Read original=nullptr;
inline control_storage::ArchiveCache* cache=nullptr;
inline HMODULE storageModule=nullptr;
inline RRIndirectCallPatch patch;
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
    const auto modulePath=ModulePath(selfModule);const auto slash=modulePath.find_last_of(L"\\/");
    if(slash==std::wstring::npos)return false;
    const auto path=modulePath.substr(0,slash+1)+L"DirectStorage\\dstorage.dll";
    storageModule=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!storageModule){Log("DIRECTSTORAGE_R230 disabled=runtime_load error=%lu",GetLastError());return false;}
    using GetFactory=HRESULT(WINAPI*)(REFIID,void**);
    auto get=reinterpret_cast<GetFactory>(GetProcAddress(storageModule,"DStorageGetFactory"));
    control_storage::ComPtr<IDStorageFactory> factory;
    if(!get||FAILED(get(IID_PPV_ARGS(&factory)))){Log("DIRECTSTORAGE_R230 disabled=factory");return false;}
    cache=new(std::nothrow) control_storage::ArchiveCache;
    if(!cache||!cache->Initialize(factory.Get())){
        if(cache){cache->CloseIfIdle();delete cache;cache=nullptr;}
        Log("DIRECTSTORAGE_R230 disabled=cache_init");return false;
    }
    original=read;
    if(!patch.Prepare(base+0x19351c,slot,reinterpret_cast<void*>(read),reinterpret_cast<void*>(&ReadHook))||
        !patch.Exchange(true)||!patch.healthy){
        if(patch.changed&&!patch.Exchange(false))Log("DIRECTSTORAGE_R230 rollback_failed=1");
        Log("DIRECTSTORAGE_R230 disabled=patch");return false;
    }
    Log("DIRECTSTORAGE_R230 installed=1 path=archive_prefetch source=real_rmdp destination=cpu_memory cache_mib=32 decode=native gpu_decompression=0 fallback=native phase_hooks=unchanged");
    return true;
}
inline void Poll(unsigned long long present) noexcept {
    static ULONGLONG next=0;if(!cache||present%60)return;
    const auto now=GetTickCount64();if(now<next)return;next=now+5000;
    auto& c=cache->counters;
    Log("DIRECTSTORAGE_R230_STATUS present=%llu calls=%llu served=%llu native_fallback=%llu served_bytes=%llu prefetched_bytes=%llu io_failures=%llu busy_fallback=%llu",
        present,calls.load(),served.load(),fallbacks.load(),c.hitBytes.load(),c.submittedBytes.load(),c.failures.load(),c.busy.load());
}
}
