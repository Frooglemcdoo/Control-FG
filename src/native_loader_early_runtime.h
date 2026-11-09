#pragma once
#include <new>
#include "directstorage_early_reader.h"
#include "directstorage_runtime_loader.h"
namespace native_loader_early {
inline control_storage::EarlyReader* reader=nullptr;
inline HMODULE storageModule=nullptr;
using DestroyPack=void(*)(void*);inline DestroyPack destroyPack=nullptr;
inline NativeCpuCallPatch lifetimePatches[3]{};
inline void Destroy(void* pack){const DWORD incoming=GetLastError();if(reader)reader->Invalidate();SetLastError(incoming);destroyPack(pack);}
using ThreadIndex=int(*)();inline ThreadIndex threadIndex=nullptr;
inline void Offer(const loader_candidate::Candidate* values,unsigned count,std::uint64_t selected,std::uint64_t owner){if(reader)reader->Offer(values,count,selected,owner);}
inline bool Read(HANDLE native,const loader_candidate::Candidate& actual,void* out,DWORD count,LPDWORD received){
    // PackFile's shared slot zero is not eligible for implicit-position serving.
    const DWORD incoming=GetLastError();const bool allowed=threadIndex&&threadIndex()>0;
    SetLastError(incoming);return allowed&&reader&&reader->TryConsume(native,actual,out,count,received);
}
inline bool Install(){
    char value[8]{};
    if(GetEnvironmentVariableA("CONTROLFG_EARLY_STORAGE",value,sizeof(value))!=1||value[0]!='1')return false;
    if(GetEnvironmentVariableA("CONTROLFG_LOADER_TRACE",value,sizeof(value))!=1||value[0]!='1'){Log("DIRECTSTORAGE_R237 disabled=loader_hooks_required");return false;}
    if(GetEnvironmentVariableA("CONTROLFG_DIRECTSTORAGE",value,sizeof(value))==1&&value[0]=='1'){Log("DIRECTSTORAGE_R237 disabled=legacy_cache_conflict");return false;}
    auto runtime=GetModuleHandleW(L"rl_rmdwin10_f.dll");
    if(!runtime||!HashMatches(runtime,"A79234E4B34C2488DEAAC3459E8054BE206B5AF78A37723D834C1CBD826CE484")){Log("DIRECTSTORAGE_R237 disabled=runtime_hash");return false;}
    control_storage::ComPtr<IDStorageFactory> factory;control_storage::RuntimeLoadResult result;
    if(!control_storage::LoadRuntimeFactory(ModulePath(selfModule),storageModule,factory,result)){Log("DIRECTSTORAGE_R237 disabled=%s hr=0x%08lX win32=%lu",result.stage,static_cast<unsigned long>(result.hr),result.win32);return false;}
    reader=new(std::nothrow) control_storage::EarlyReader;
    if(!reader||!reader->Initialize(factory.Get())){
        if(reader&&reader->Stop()){delete reader;reader=nullptr;}Log("DIRECTSTORAGE_R237 disabled=initialize");return false;
    }
    auto* base=reinterpret_cast<unsigned char*>(runtime);destroyPack=reinterpret_cast<DestroyPack>(base+0x1951c0);
    const unsigned sites[]={0x18be62,0x18bf0e,0x18c2c2};
    bool prepared=true;for(unsigned i=0;i<3;++i)if(!NativeCpuPrepareCallPatch(base+sites[i],reinterpret_cast<void*>(destroyPack),reinterpret_cast<void*>(&Destroy),&lifetimePatches[i])){prepared=false;break;}
    unsigned applied=0;if(prepared)for(;applied<3;){auto i=applied;if(!NativeCpuExchangeCall(&lifetimePatches[i],true))break;++applied;if(!lifetimePatches[i].writeHealthy)break;}
    if(applied!=3||!lifetimePatches[2].writeHealthy){
        while(applied){--applied;if(!NativeCpuExchangeCall(&lifetimePatches[applied],false))Log("DIRECTSTORAGE_R237 lifetime_rollback_failed=%u",applied);}
        if(reader->Stop()){delete reader;reader=nullptr;}Log("DIRECTSTORAGE_R237 disabled=lifetime_hooks");return false;
    }
    threadIndex=reinterpret_cast<ThreadIndex>(reinterpret_cast<unsigned char*>(runtime)+0x1676e0);
    native_loader_stages::offerEarly=&Offer;native_loader_stages::readEarly=&Read;
    Log("DIRECTSTORAGE_R237 ready=1 source=resolved_queued_requests registration=native_handle worker=owned memory_mib=32 slots=16 max_read_mib=2 expiry_ms=250 demand_submit=0 cold=native archive_invalidation=1");return true;
}
inline void Poll(unsigned long long present){
    static ULONGLONG next=0;if(!reader||present%60)return;const auto now=GetTickCount64();if(now<next)return;next=now+5000;const auto& c=reader->counters;
    Log("DIRECTSTORAGE_R237_STATUS present=%llu registered=%llu files_ready=%llu registration_failures=%llu file_capacity=%llu offered=%llu admitted=%llu cold=%llu capacity=%llu busy=%llu duplicates=%llu invalidations=%llu completed=%llu submitted=%llu submitted_bytes=%llu batches=%llu served=%llu served_bytes=%llu late=%llu avoided=%llu expired=%llu unused_bytes=%llu io_failures=%llu identity_rejected=%llu position_rejected=%llu",
        present,c.registered.load(),c.filesReady.load(),c.registrationFailed.load(),c.fileCapacity.load(),c.offered.load(),c.admitted.load(),c.cold.load(),c.capacity.load(),c.busy.load(),c.duplicates.load(),c.invalidations.load(),c.completed.load(),c.submitted.load(),c.submittedBytes.load(),c.batches.load(),c.served.load(),c.servedBytes.load(),c.late.load(),c.avoided.load(),c.expired.load(),c.unusedBytes.load(),c.ioFailures.load(),c.identityRejected.load(),c.positionRejected.load());
}
}
