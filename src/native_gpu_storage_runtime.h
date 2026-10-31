#pragma once
#include "directstorage_packed_textures.h"
#include "directstorage_runtime_loader.h"
#include <string>
#include <new>
#include <algorithm>
#include "storage_latency.h"
#include "storage_publication_tracker.h"
#include "storage_asset_trace.h"
#include "storage_selection_trace.h"
#include "native_texture_upgrade_patch.h"
#include "storage_startup_policy.h"
namespace native_gpu_storage {
using U64=std::uint64_t;
inline U64 ClockUs(){static const U64 frequency=[](){LARGE_INTEGER f;QueryPerformanceFrequency(&f);return U64(f.QuadPart);}();LARGE_INTEGER t;QueryPerformanceCounter(&t);return U64(t.QuadPart)/frequency*1000000+(U64(t.QuadPart)%frequency)*1000000/frequency;}
inline bool assetTraceEnabled=false;
inline storage_asset_trace::Capture assetCapture,assetSnapshot;
inline SRWLOCK assetLock=SRWLOCK_INIT;
inline std::atomic<U64> assetDeadline{0},assetContention{0};
inline bool assetHeld=false;
inline storage_selection_trace::Trace selectionTrace,selectionSnapshot;
inline bool selectionInstalled=false;
inline std::atomic<U64> selectionDrops{0};
inline storage_publication::Tracker nativeAssetTracker;
inline std::atomic<U64> nativeAssetDropped{0};
inline U64 AssetAdmit(const char* name,U64 requested,U64 admitted,unsigned level,int priority,U64 bytes,U64 texture=0,U64 offset=0,U64 allocationUs=0){
    if(!assetTraceEnabled||!assetDeadline.load(std::memory_order_relaxed))return 0;
    if(!TryAcquireSRWLockExclusive(&assetLock)){++assetContention;return 0;}
    const auto token=assetCapture.Add(name,requested,admitted,level,priority,bytes);if(auto* row=assetCapture.Find(token)){row->texture=texture;row->offset=offset;row->allocationUs=allocationUs;}
    ReleaseSRWLockExclusive(&assetLock);return token;
}
inline void AssetEvent(U64 token,U64 now,unsigned event,int result=0){
    if(!token)return;
    if(!TryAcquireSRWLockExclusive(&assetLock)){++assetContention;return;}
    if(event==0)assetCapture.Submit(token,now);else if(event==1)assetCapture.Retire(token,now,result);else assetCapture.Publish(token,now);
    ReleaseSRWLockExclusive(&assetLock);
}
inline void AssetRoute(U64 token,U64 admitted,int path,U64 texture,unsigned level,U64 container){
    if(!token)return;
    if(!TryAcquireSRWLockExclusive(&assetLock)){++assetContention;return;}
    if(auto* row=assetCapture.Find(token)){
        row->admitted=admitted;row->path=path;
        if(path==2&&(!container||!nativeAssetTracker.Begin(texture,level,container,row->requested,token)))++nativeAssetDropped;
    }
    ReleaseSRWLockExclusive(&assetLock);
}
inline void FlushAssets(){
    const auto deadline=assetDeadline.load(std::memory_order_acquire);if(!deadline||ClockUs()<deadline)return;
    if(!TryAcquireSRWLockExclusive(&assetLock))return;
    const bool finished=assetCapture.Finish(ClockUs());
    if(finished){assetSnapshot=assetCapture;selectionSnapshot=selectionTrace;assetDeadline.store(0,std::memory_order_release);}
    ReleaseSRWLockExclusive(&assetLock);if(!finished)return;
    Log("TEXTURE_ASSET_R256_BEGIN epoch=%llu start_us=%llu end_us=%llu rows=%u overflow=%u contention_total=%llu native_tracking_drops_total=%llu capacity=16384",assetSnapshot.epoch,assetSnapshot.start,assetSnapshot.end,assetSnapshot.count,assetSnapshot.overflow,assetContention.load(),nativeAssetDropped.load());
    for(unsigned i=0;i<assetSnapshot.count;++i){const auto& r=assetSnapshot.rows[i];
        Log("TEXTURE_ASSET_R256_ROW epoch=%llu token=%llu request_us=%llu admitted_us=%llu submit_us=%llu retired_us=%llu publication_us=%llu level=%u priority=%d bytes=%llu result=%d truncated=%u path=%d texture=%llu offset=%llu allocation_us=%llu name=%s",assetSnapshot.epoch,r.token,r.requested,r.admitted,r.submitted,r.retired,r.published,r.level,r.priority,r.bytes,r.result,unsigned(r.truncated),r.path,r.texture,r.offset,r.allocationUs,r.name);
    }
    Log("TEXTURE_ASSET_R256_END epoch=%llu rows=%u",assetSnapshot.epoch,assetSnapshot.count);
    Log("TEXTURE_SELECT_R256_BEGIN epoch=%llu rows=%u overflow=%u targets=%u target_overflow_total=%u contention_total=%llu installed=%u",assetSnapshot.epoch,selectionSnapshot.count,selectionSnapshot.overflow,selectionSnapshot.targetsCount,selectionSnapshot.targetOverflow,selectionDrops.load(),unsigned(selectionInstalled));
    for(const auto& t:selectionSnapshot.targets)if(t.texture)Log("TEXTURE_SELECT_R256_TARGET epoch=%llu texture=%llu name=%s",assetSnapshot.epoch,t.texture,t.name);
    for(unsigned i=0;i<selectionSnapshot.count;++i){const auto& r=selectionSnapshot.rows[i];
        Log("TEXTURE_SELECT_R256_ROW epoch=%llu texture=%llu begin_us=%llu end_us=%llu kind=%u level=%d result=%d row=%u",assetSnapshot.epoch,r.texture,r.begin,r.end,r.kind,r.level,r.result,i);
        const storage_selection_trace::Trace::State* states[]={&r.before,&r.after};
        for(unsigned phase=0;phase<2;++phase){const auto& v=*states[phase];
            Log("TEXTURE_SELECT_R256_STATE epoch=%llu row=%u phase=%u scan=%llu cycle=%llu pending_bytes=%llu base=%d resident=%d wanted=%d retain=%d delta=%d wanted_state=%d priority_min=%d priority_max=%d deferred=%u enabled=%u special=%u bypass=%u policy=%u direction=%u spent_ms=%.6f budget_ms=%.3f distance_metric=%.6g",assetSnapshot.epoch,i,phase,v.scan,v.cycle,v.pendingBytes,v.base,v.resident,v.wanted,v.retain,v.delta,v.wantedState,v.priorityMin,v.priorityMax,v.deferred,v.enabled,v.special,v.bypass,v.policy,v.direction,v.spentMs,double(v.budgetMs),double(v.distanceMetric));
        }
    }
    Log("TEXTURE_SELECT_R256_END epoch=%llu rows=%u",assetSnapshot.epoch,selectionSnapshot.count);
}
inline bool traceLatency=false;
inline storage_latency::Timing allocationTime,admissionTime,queueTime,endToEndTime,handoffTime,storageTime,gpuObservedTime,recordTime;
inline storage_latency::Timing levelLatency[32];

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
struct Allocation {void* texture=nullptr;unsigned level=0;U64 container=0,allocationUs=0;};
inline thread_local Allocation allocation{};
inline RRIndirectCallPatch patch;inline HMODULE storageModule=nullptr;inline Ptr<IDStorageFactory> factory;
inline SRWLOCK lock=SRWLOCK_INIT,fileLock=SRWLOCK_INIT;inline HANDLE wake=nullptr,worker=nullptr;
inline std::atomic<bool> available{false};
// Present only hands off a retained queue. All expensive initialization stays
// on the existing storage worker, including when the first request wins the race.
inline Ptr<ID3D12CommandQueue> warmQueue;
inline std::atomic<bool> warmPosted{false};
inline std::atomic<unsigned> readyLanes{0};
inline bool PostWarmQueue(ID3D12CommandQueue* queue){
    if(!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
    AcquireSRWLockExclusive(&lock);
    const bool post=!warmPosted.load(std::memory_order_relaxed);
    if(post){warmQueue=queue;warmPosted.store(true,std::memory_order_release);}
    ReleaseSRWLockExclusive(&lock);
    if(post&&wake)SetEvent(wake);return post;
}
inline Ptr<ID3D12CommandQueue> TakeWarmQueue(){
    AcquireSRWLockExclusive(&lock);auto queue=std::move(warmQueue);ReleaseSRWLockExclusive(&lock);return queue;
}
inline std::atomic<U64> ineligible{0},layoutRejected{0},sourceRejected{0},capacityRejected{0},duplicateRejected{0},unavailableRejected{0},peakPending{0},stagingAllocations{0},stagingReuses{0},submitMaxUs{0},initMaxUs{0},pollMaxUs{0};
inline std::atomic<U64> offered{0},accepted{0},completed{0},fallback{0},bytes{0},failed{0},batches{0},pending{0};
inline constexpr unsigned QueueCapacity=4096;
struct Job {
    bool used=false;U64 assetToken=0,container=0,serial=0,offset=0,sourceOffset=0,gpuBytes=0,requestedUs=0,admittedUs=0;UINT size=0;unsigned level=0,slot=QueueCapacity;int thread=0,priority=0;
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
using SelectTexture=void(*)(void*,bool,bool,bool);
using RequestLevel=bool(*)(void*,int);
inline SelectTexture selectTexture=nullptr;inline RequestLevel requestLevel=nullptr;
inline NativeCpuCallPatch selectionPatches[3]{};
inline void RegisterSelection(void* texture,const char* name){
    if(!assetTraceEnabled)return;
    if(!TryAcquireSRWLockExclusive(&assetLock)){++selectionDrops;return;}
    selectionTrace.Register(reinterpret_cast<U64>(texture),name);ReleaseSRWLockExclusive(&assetLock);
}
inline U64 SelectionBegin(void* texture){
    if(!assetTraceEnabled||!assetDeadline.load(std::memory_order_relaxed))return 0;
    if(!TryAcquireSRWLockExclusive(&assetLock)){++selectionDrops;return 0;}
    const auto now=ClockUs();const auto epoch=assetCapture.active&&now<assetCapture.end&&selectionTrace.Contains(reinterpret_cast<U64>(texture))?assetCapture.epoch:0;
    ReleaseSRWLockExclusive(&assetLock);return epoch;
}
inline storage_selection_trace::Trace::State SelectionState(void* texture){
    storage_selection_trace::Trace::State v;
    v.base=int(Dword(texture,0xb4));v.resident=int(Dword(texture,0xb8));v.wanted=int(Dword(texture,0xc4));v.retain=int(Dword(texture,0xc8));v.delta=int(Dword(texture,0xc0));
    const auto* p=static_cast<const unsigned char*>(texture);v.deferred=p[0xcc];v.special=p[0xcd];v.enabled=p[0x111];memcpy(&v.distanceMetric,p+0xd4,4);
    const auto levels=Field(texture,0xe8);const auto count=Dword(texture,0xf0);
    if(levels&&v.wanted>=0&&unsigned(v.wanted)<count&&count<=32)v.wantedState=int(Dword(reinterpret_cast<void*>(levels+U64(v.wanted)*0x28),0x20));
    const auto manager=Field(renderer,0x802c48);
    if(manager){auto* m=reinterpret_cast<void*>(manager);v.scan=Field(m,0x88);v.cycle=Field(m,0x90);memcpy(&v.spentMs,reinterpret_cast<unsigned char*>(m)+0x80,8);v.spentMs*=1000;}
    v.pendingBytes=Field(renderer,0x90f0e8);v.priorityMin=int(Dword(renderer,0x802c08));v.priorityMax=int(Dword(renderer,0x802c0c));
    v.bypass=renderer[0x802c10];v.policy=renderer[0x7ec192];v.direction=renderer[0x802a85];memcpy(&v.budgetMs,renderer+0x9186a0,4);return v;
}
inline void SelectionEnd(U64 epoch,const storage_selection_trace::Trace::Row& row){
    if(!TryAcquireSRWLockExclusive(&assetLock)){++selectionDrops;return;}
    if(assetCapture.active&&assetCapture.epoch==epoch)selectionTrace.Add(row);else ++selectionDrops;
    ReleaseSRWLockExclusive(&assetLock);
}
inline void SelectTextureHook(void* texture,bool a,bool b,bool c){
    const DWORD incoming=GetLastError();const auto epoch=SelectionBegin(texture);
    storage_selection_trace::Trace::Row row; if(epoch){row.texture=reinterpret_cast<U64>(texture);row.begin=ClockUs();row.before=SelectionState(texture);}
    SetLastError(incoming);selectTexture(texture,a,b,c);const DWORD error=GetLastError();
    if(epoch){row.end=ClockUs();row.after=SelectionState(texture);SelectionEnd(epoch,row);}SetLastError(error);
}
inline bool RequestLevelHook(void* texture,int level){
    const DWORD incoming=GetLastError();const auto epoch=SelectionBegin(texture);
    storage_selection_trace::Trace::Row row;if(epoch){row.texture=reinterpret_cast<U64>(texture);row.kind=1;row.level=level;row.begin=ClockUs();row.before=SelectionState(texture);}
    SetLastError(incoming);const bool result=requestLevel(texture,level);const DWORD error=GetLastError();
    if(epoch){row.end=ClockUs();row.result=int(result);row.after=SelectionState(texture);SelectionEnd(epoch,row);}SetLastError(error);return result;
}
inline native_texture_upgrade::Patch upgradePatch;
inline bool InstallUpgrade(){
    if(!storage_startup_policy::EnvironmentFlag("CONTROLFG_TEXTURE_UPGRADE_GATE",true))return false;
    if(!upgradePatch.Prepare(renderer+0x15aa90)||!upgradePatch.Exchange(true)){
        Log("TEXTURE_UPGRADE_R256 installed=0 reason=site_or_exchange");return false;
    }
    if(!upgradePatch.healthy){const bool rolled=upgradePatch.Exchange(false);Log("TEXTURE_UPGRADE_R256 installed=0 reason=protection rollback=%u",unsigned(rolled));return false;}
    Log("TEXTURE_UPGRADE_R256 installed=1 negative_delta=eligible budget=native pending_limit=native partitions=unchanged scope=all_textures");return true;
}
inline bool InstallSelection(){
    if(!assetTraceEnabled)return false;
    selectTexture=reinterpret_cast<SelectTexture>(renderer+0x15a5e0);requestLevel=reinterpret_cast<RequestLevel>(renderer+0x15ab30);
    const unsigned sites[]={0x16923e,0x15aaab,0x15aac2};
    for(unsigned i=0;i<3;++i)if(!NativeCpuPrepareCallPatch(renderer+sites[i],i?reinterpret_cast<void*>(requestLevel):reinterpret_cast<void*>(selectTexture),i?reinterpret_cast<void*>(&RequestLevelHook):reinterpret_cast<void*>(&SelectTextureHook),&selectionPatches[i])){
        Log("TEXTURE_SELECT_R256_INSTALL installed=0 reason=prepare site=%u",i);return false;
    }
    unsigned applied=0;for(;applied<3;){auto& p=selectionPatches[applied];if(!NativeCpuExchangeCall(&p,true))break;++applied;if(!p.writeHealthy)break;}
    if(applied!=3||!selectionPatches[2].writeHealthy){while(applied){--applied;if(!NativeCpuExchangeCall(&selectionPatches[applied],false))Log("TEXTURE_SELECT_R256_INSTALL rollback_failed=%u",applied);}return false;}
    selectionInstalled=true;Log("TEXTURE_SELECT_R256_INSTALL installed=1 sites=3 scope=sign_and_wayfinding_paths policy_changed=0");return true;
}
// Admit only a newly allocated, unpublished container. Existing mip resources
// can already be referenced by recorded rendering work and remain native.
inline bool AllocateHook(void* texture,unsigned level){
    const DWORD incoming=GetLastError();auto* levels=reinterpret_cast<unsigned char*>(Field(texture,0xe8));
    const U64 before=levels?Field(levels+U64(level)*0x28,0x10):0;
    const U64 start=traceLatency?ClockUs():0;SetLastError(incoming);const bool result=allocate(texture,level);const DWORD error=GetLastError();const U64 elapsed=traceLatency?ClockUs()-start:0;
    levels=reinterpret_cast<unsigned char*>(Field(texture,0xe8));
    const U64 after=levels?Field(levels+U64(level)*0x28,0x10):0;
    allocation=result&&!before&&after?Allocation{texture,level,after,elapsed}:Allocation{};
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
inline storage_publication::Tracker publicationTracker;
inline SRWLOCK publicationLock=SRWLOCK_INIT;
inline NativeCpuCallPatch publicationPatches[3]{};
inline bool publicationInstalled=false;
inline std::atomic<U64> publicationTracked{0},publicationDropped{0},publicationCallbacks{0};
inline storage_latency::Timing publicationDelay,publicationCall;
using Consume=void(*)(void*);inline Consume consume=nullptr;
inline void PublicationBegin(const Job& job,U64 now){
    if(!publicationInstalled)return;
    if(!TryAcquireSRWLockExclusive(&publicationLock)){++publicationDropped;return;}
    const bool tracked=publicationTracker.Begin(reinterpret_cast<U64>(job.texture),job.level,job.container,now,job.assetToken);
    ReleaseSRWLockExclusive(&publicationLock);
    if(tracked)++publicationTracked;else ++publicationDropped;
}
inline void PublicationHook(void* record){
    const DWORD incoming=GetLastError();const U64 now=ClockUs();U64 start=0,assetToken=0;
    ++publicationCallbacks;
    // This is a native-valid completion record. Read identity before forwarding;
    // never inspect the texture after the consumer may notify/release owners.
    const auto texture=Field(record,0);const unsigned level=Dword(record,8);
    if(texture&&level<32&&*(static_cast<unsigned char*>(record)+12)){
        auto* owner=reinterpret_cast<void*>(texture);
        const auto levels=Field(owner,0xe8);
        if(level<Dword(owner,0xf0)&&levels){
            const auto container=Field(reinterpret_cast<void*>(levels+U64(level)*0x28),0x10);
            if(TryAcquireSRWLockExclusive(&publicationLock)){
                start=publicationTracker.Take(texture,level,container,now,&assetToken);ReleaseSRWLockExclusive(&publicationLock);
            }else ++publicationDropped;
            if(assetTraceEnabled&&assetDeadline.load(std::memory_order_relaxed)){
                if(TryAcquireSRWLockExclusive(&assetLock)){
                    U64 nativeToken=0;nativeAssetTracker.Take(texture,level,container,now,&nativeToken);
                    if(nativeToken)assetCapture.Publish(nativeToken,now);
                    ReleaseSRWLockExclusive(&assetLock);
                }else ++assetContention;
            }
        }
    }
    SetLastError(incoming);consume(record);const DWORD error=GetLastError();
    if(start){publicationDelay.Record(now-start);publicationCall.Record(ClockUs()-now);AssetEvent(assetToken,now,2);}
    SetLastError(error);
}
inline bool InstallPublication(){
    char enabled[8]{};if(GetEnvironmentVariableA("CONTROLFG_STORAGE_PUBLICATION",enabled,8)!=1||enabled[0]!='1')return false;
    consume=reinterpret_cast<Consume>(renderer+0x158410);
    // Immediate call, packet thunk, and deferred-function-object thunk.
    // The deferred thunk adjusts RCX by 8 before its five-byte tail branch.
    const unsigned sites[]={0x1603b9,0x1605d0,0x162b74};
    const unsigned char opcodes[]={0xe8,0xe9,0xe9};
    for(unsigned i=0;i<3;++i)if(!NativeCpuPrepareCallPatch(renderer+sites[i],reinterpret_cast<void*>(consume),reinterpret_cast<void*>(&PublicationHook),&publicationPatches[i],opcodes[i])){
        Log("DIRECTSTORAGE_R256_PUBLICATION installed=0 reason=prepare site=%u",i);return false;
    }
    unsigned applied=0;
    for(;applied<3;){auto& p=publicationPatches[applied];if(!NativeCpuExchangeCall(&p,true))break;++applied;if(!p.writeHealthy)break;}
    if(applied!=3||!publicationPatches[2].writeHealthy){
        while(applied){--applied;if(!NativeCpuExchangeCall(&publicationPatches[applied],false))Log("DIRECTSTORAGE_R256_PUBLICATION rollback_failed=%u",applied);}
        return false;
    }
    publicationInstalled=true;Log("DIRECTSTORAGE_R256_PUBLICATION installed=1 sites=3 visible_draw_measured=0");return true;
}
inline void Retire(Job& job,bool success,bool retry){
    if(job.assetToken)AssetEvent(job.assetToken,ClockUs(),1,success?1:retry?2:3);
    if(success){
        const U64 ready=traceLatency?ClockUs():0;
        if(publicationInstalled)PublicationBegin(job,ClockUs());
        finish(job.texture,static_cast<int>(job.level),true);
        if(traceLatency){const U64 handedOff=ClockUs();
            if(job.requestedUs){const U64 latency=ready-job.requestedUs;endToEndTime.Record(latency);levelLatency[job.level].Record(latency);}
            handoffTime.Record(handedOff-ready);}
        ++completed;
    }
    else if(retry){AssetRoute(job.assetToken,job.admittedUs,2,reinterpret_cast<U64>(job.texture),job.level,job.container);original(job.owner,job.name.c_str(),job.texture,job.offset,job.size,nullptr,job.thread,job.priority,false,reinterpret_cast<void*>(U64(job.level)));++fallback;}
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
// These are worker CPU/wall elapsed times, not GPU timestamp-query durations.
inline void MaxElapsed(std::atomic<U64>& maximum,U64 start){const U64 elapsed=ClockUs()-start;if(elapsed>maximum.load(std::memory_order_relaxed))maximum.store(elapsed,std::memory_order_relaxed);}
// Select one priority/FIFO batch in O(queue size * log(batch size)), rather
// than scanning the full queue once for each request while holding the lock.
inline unsigned Dequeue(Job* output,unsigned limit){
    if(!output||!limit)return 0;unsigned indices[QueueCapacity],n=0,count=0;U64 laneBytes=0;
    AcquireSRWLockExclusive(&lock);
    for(unsigned i=0;i<QueueCapacity;++i)if(jobs[i].used)indices[n++]=i;
    const unsigned selected=(std::min)(n,limit);
    std::partial_sort(indices,indices+selected,indices+n,[](unsigned a,unsigned b){return jobs[a].priority!=jobs[b].priority?jobs[a].priority>jobs[b].priority:jobs[a].serial<jobs[b].serial;});
    for(unsigned k=0;k<selected;++k){auto& job=jobs[indices[k]];
        // Reserve allocation alignment headroom for both reusable arenas.
        if(laneBytes+job.gpuBytes>control_storage::PackedTextureBatch::StagingBudget-128*1024)break;
        laneBytes+=job.gpuBytes;output[count++]=std::move(job);job={};
    }
    ReleaseSRWLockExclusive(&lock);return count;
}
inline DWORD WINAPI Work(void*){
    struct Lane {control_storage::PackedTextureBatch transfer;Job jobs[control_storage::PackedTextureBatch::Capacity];unsigned count=0;bool initialized=false;};
    Lane lanes[2];
    auto initialize=[&](Lane& lane,ID3D12CommandQueue* queue,const char* reason)->HRESULT {
        if(lane.initialized)return S_OK;
        if(!available)return E_ABORT;
        const control_storage::PackedTextureBatch* shared=nullptr;
        for(const auto& other:lanes)if(other.initialized){shared=&other.transfer;break;}
        const U64 start=ClockUs();const HRESULT hr=lane.transfer.Initialize(factory.Get(),queue,shared);MaxElapsed(initMaxUs,start);
        lane.transfer.SetLatencyTracking(traceLatency);lane.initialized=SUCCEEDED(hr);const auto& t=lane.transfer.InitializationTiming();
        if(lane.initialized)++readyLanes;
        Log("DIRECTSTORAGE_R256_INIT lane=%u ready=%u hr=0x%08lX reason=%s shader=build_embedded shared_pipeline=%u queues_us=%llu pipeline_us=%llu commands_us=%llu",unsigned(&lane-lanes),unsigned(lane.initialized),static_cast<unsigned long>(hr),reason,unsigned(t.sharedPipeline),t.queuesUs,t.pipelineUs,t.commandsUs);
        if(!lane.initialized){available=false;lane.transfer.Close();}
        return hr;
    };
    for(;;){
        FlushAssets();
        // A retained queue from Present allows setup while still at the title
        // screen. Never hold the admission lock while creating driver objects.
        if(auto queue=TakeWarmQueue()){
            for(auto& lane:lanes)if(FAILED(initialize(lane,queue.Get(),"prewarm")))break;
            Log("DIRECTSTORAGE_R256_PREWARM finished=1 ready_lanes=%u pending=%llu",readyLanes.load(),pending.load());
        }
        for(auto& lane:lanes){
            if(lane.count){
                HRESULT result=E_PENDING;
                const U64 pollStart=ClockUs();const bool retired=lane.transfer.Poll(result);MaxElapsed(pollMaxUs,pollStart);if(!retired)continue;
                const bool success=SUCCEEDED(result);const bool lost=result==DXGI_ERROR_DEVICE_REMOVED||result==DXGI_ERROR_DEVICE_RESET;
                if(!success){++failed;Log("DIRECTSTORAGE_R256 batch_failed=1 hr=0x%08lX count=%u",static_cast<unsigned long>(result),lane.count);}
                if(traceLatency&&success){const auto& t=lane.transfer.LastBatchTiming();
                    if(t.storageDone)storageTime.Record(t.storageObservedUs);
                    if(t.gpuDone){gpuObservedTime.Record(t.gpuObservedUs);recordTime.Record(t.recordSubmitUs);}}
                if(lost)available=false;
                for(unsigned i=0;i<lane.count;++i)Retire(lane.jobs[i],success,!lost);lane.count=0;
            }
            lane.count=Dequeue(lane.jobs,control_storage::PackedTextureBatch::Capacity);
            if(!lane.count)continue;
            HRESULT hr=available?S_OK:E_ABORT;
            if(SUCCEEDED(hr)&&!lane.initialized)hr=initialize(lane,lane.jobs[0].queue.Get(),"first_request");
            control_storage::PackedTextureBatch::Input inputs[control_storage::PackedTextureBatch::Capacity]{};
            for(unsigned i=0;i<lane.count;++i){const auto& j=lane.jobs[i];inputs[i]={j.file.Get(),j.resource.Get(),j.sourceOffset,j.size,D3D12_RESOURCE_STATES(0xc0)};}
            if(SUCCEEDED(hr)){const U64 allocations=lane.transfer.BufferAllocations(),reuses=lane.transfer.ReusedBatches(),start=ClockUs();
                if(traceLatency)for(unsigned i=0;i<lane.count;++i)if(lane.jobs[i].admittedUs)queueTime.Record(start-lane.jobs[i].admittedUs);
                for(unsigned i=0;i<lane.count;++i)AssetEvent(lane.jobs[i].assetToken,start,0);
                hr=lane.transfer.Submit(inputs,lane.count);MaxElapsed(submitMaxUs,start);stagingAllocations+=lane.transfer.BufferAllocations()-allocations;stagingReuses+=lane.transfer.ReusedBatches()-reuses;}
            if(FAILED(hr)){++failed;Log("DIRECTSTORAGE_R256 batch_rejected=1 hr=0x%08lX count=%u",static_cast<unsigned long>(hr),lane.count);
                for(unsigned i=0;i<lane.count;++i)Retire(lane.jobs[i],false,true);lane.count=0;
            }else ++batches;
        }
        HANDLE events[3]={wake};DWORD count=1;unsigned inFlight=0;
        for(auto& lane:lanes)if(lane.count){events[count++]=lane.transfer.Event();inFlight+=lane.count;}
        // A rejected batch can leave a lane idle with queued jobs remaining.
        // Drain those jobs/retries instead of waiting for a new request/event
        // that might never arrive. Pending includes queued plus active jobs.
        if(count<3&&pending.load()>inFlight)continue;
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
    if(job.gpuBytes>control_storage::PackedTextureBatch::StagingBudget-128*1024)return false;
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
    const DWORD incoming=GetLastError();const U64 requested=traceLatency?ClockUs():0;++offered;
    const auto fresh=allocation;allocation={};
    RegisterSelection(texture,name);
    const auto level=static_cast<unsigned>(reinterpret_cast<U64>(user));
    const U64 assetToken=AssetAdmit(name,requested,0,level,priority,size,reinterpret_cast<U64>(texture),offset,fresh.texture==texture?fresh.allocationUs:0);
    // Container is supplied by the already verified allocation hook; do not
    // dereference native-only objects to guess an unverified layout.
    const auto routeNative=[&](){AssetRoute(assetToken,0,2,reinterpret_cast<U64>(texture),level,fresh.texture==texture&&fresh.level==level?fresh.container:0);};
    if(pending.load(std::memory_order_relaxed)>=QueueCapacity){++capacityRejected;++fallback;routeNative();SetLastError(incoming);return original(owner,name,texture,offset,size,buffer,thread,priority,flag,user);}
    if(available&&fresh.texture==texture&&fresh.level==reinterpret_cast<U64>(user)&&fresh.container&&!buffer&&!flag&&size&&size<=32u*1024u*1024u&&(thread==3||thread==14)&&reinterpret_cast<U64>(user)<32){
        Job j{};j.owner=owner;j.texture=texture;j.offset=offset;j.size=static_cast<UINT>(size);j.level=static_cast<unsigned>(reinterpret_cast<U64>(user));j.thread=thread;j.priority=priority;
        if(!describe(texture,j.level,size,j))++layoutRejected;
        else if(!resolve(name,offset,size,j))++sourceRejected;
        else {
            j.name=name;j.container=fresh.container;j.requestedUs=requested;U64 ticket=0;
            AcquireSRWLockExclusive(&lock);bool duplicate=false;for(auto* p:active)if(p&&p==j.resource.Get())duplicate=true;
            if(!duplicate&&available&&pending.load()<QueueCapacity){
                for(unsigned scan=0;scan<QueueCapacity;++scan){const unsigned i=cursor++%QueueCapacity;if(!active[i]&&!jobs[i].used){addRef(texture);adjustPending(owner,1);if(traceLatency){j.admittedUs=ClockUs();admissionTime.Record(j.admittedUs-requested);allocationTime.Record(fresh.allocationUs);}j.assetToken=assetToken;AssetRoute(assetToken,j.admittedUs,1,0,0,0);j.used=true;j.serial=++serial;j.slot=i;ticket=j.serial;active[i]=j.resource.Get();jobs[i]=std::move(j);reservedBytes+=size;const U64 depth=++pending;if(depth>peakPending.load(std::memory_order_relaxed))peakPending.store(depth,std::memory_order_relaxed);++accepted;bytes+=size;break;}}
            }
            if(!ticket){if(duplicate)++duplicateRejected;else if(!available)++unavailableRejected;else ++capacityRejected;}
            ReleaseSRWLockExclusive(&lock);
            if(ticket){SetEvent(wake);SetLastError(incoming);return static_cast<std::int64_t>(ticket|0x8000000000000000ull);}
        }
    }else ++ineligible;
    ++fallback;routeNative();SetLastError(incoming);return original(owner,name,texture,offset,size,buffer,thread,priority,flag,user);
}
inline bool Install(){
    char value[8]{};if(!storage_startup_policy::EnvironmentFlag("CONTROLFG_NATIVE_GPU_STORAGE",GetFGDirectStorageEnabled())){Log("DIRECTSTORAGE_R256 disabled=settings_or_environment");return false;}
    char latencyValue[8]{};traceLatency=GetEnvironmentVariableA("CONTROLFG_STORAGE_LATENCY",latencyValue,8)==1&&latencyValue[0]=='1';
    char assetValue[8]{};assetTraceEnabled=GetEnvironmentVariableA("CONTROLFG_STORAGE_ASSET_TRACE",assetValue,8)==1&&assetValue[0]=='1';
    if(assetTraceEnabled)traceLatency=true;
    const char* conflicts[]={"CONTROLFG_TEXTURE_CONTRACT","CONTROLFG_EARLY_STORAGE","CONTROLFG_DIRECTSTORAGE"};
    for(const auto* key:conflicts)if(GetEnvironmentVariableA(key,value,8)==1&&value[0]=='1'){Log("DIRECTSTORAGE_R256 disabled=conflicting_mode key=%s",key);return false;}
    auto r=GetModuleHandleW(L"renderer_rmdwin10_f.dll"),n=GetModuleHandleW(L"rl_rmdwin10_f.dll"),d=GetModuleHandleW(L"d3d_rmdwin10_f.dll");
    if(!r||!n||!d||!HashMatches(r,"EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433")||!HashMatches(n,"A79234E4B34C2488DEAAC3459E8054BE206B5AF78A37723D834C1CBD826CE484")||!HashMatches(d,"CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5")){Log("DIRECTSTORAGE_R256 disabled=module_hash");return false;}
    renderer=reinterpret_cast<unsigned char*>(r);runtime=reinterpret_cast<unsigned char*>(n);d3d=reinterpret_cast<unsigned char*>(d);
    original=reinterpret_cast<Request>(runtime+0xfa2b0);addRef=reinterpret_cast<Ref>(runtime+0x12e160);removeRef=reinterpret_cast<Ref>(runtime+0x12e190);
    allocate=reinterpret_cast<Allocate>(renderer+0x159960);
    finish=reinterpret_cast<Finish>(renderer+0x158390);lookup=reinterpret_cast<Lookup>(runtime+0x18f760);
    control_storage::RuntimeLoadResult result;if(!control_storage::LoadRuntimeFactory(ModulePath(selfModule),storageModule,factory,result)){Log("DIRECTSTORAGE_R256 disabled=%s hr=0x%08lX",result.stage,static_cast<unsigned long>(result.hr));return false;}
    if(!patch.Prepare(renderer+0x158247,reinterpret_cast<void**>(renderer+0x5e0140),reinterpret_cast<void*>(original),reinterpret_cast<void*>(&RequestHook)))return false;
    if(!NativeCpuPrepareCallPatch(renderer+0x1580d7,reinterpret_cast<void*>(allocate),reinterpret_cast<void*>(&AllocateHook),&allocationPatch))return false;
    wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!wake)return false;worker=CreateThread(nullptr,0,&Work,nullptr,0,nullptr);if(!worker)return false;
    if(!NativeCpuExchangeCall(&allocationPatch,true))return false;
    if(!allocationPatch.writeHealthy){NativeCpuExchangeCall(&allocationPatch,false);return false;}
    if(!patch.Exchange(true)){NativeCpuExchangeCall(&allocationPatch,false);return false;}
    if(!patch.healthy){patch.Exchange(false);NativeCpuExchangeCall(&allocationPatch,false);return false;}
    InstallPublication();InstallSelection();InstallUpgrade();
    available=true;Log("DIRECTSTORAGE_R256 installed=1 native_gpu_redirect=1 source=archive destination=gpu_buffer repack=gpu texture_copy=gpu native_cpu_payload_read=0 compression=none unsupported=native");return true;
}
inline void ReportLatency(const char* stage,const storage_latency::Timing& t,int level=-1){
    const U64 n=t.count.load(std::memory_order_relaxed);if(!n)return;
    Log("DIRECTSTORAGE_R256_LATENCY stage=%s level=%d samples=%llu mean_us=%.1f max_us=%llu ge16ms=%llu ge100ms=%llu scope=cumulative_cpu_observed",stage,level,n,double(t.total.load(std::memory_order_relaxed))/double(n),t.maximum.load(std::memory_order_relaxed),t.over16ms.load(std::memory_order_relaxed),t.over100ms.load(std::memory_order_relaxed));
}
inline void Poll(U64 present){
    if(!worker)return;
    if(assetTraceEnabled){
        const bool down=(GetAsyncKeyState(VK_F6)&0x8000)!=0;
        if(down&&!assetHeld){
            if(TryAcquireSRWLockExclusive(&assetLock)){
                const auto mark=ClockUs();const bool started=assetCapture.Begin(mark);const auto epoch=assetCapture.epoch;
                if(started){selectionTrace.Reset();assetDeadline.store(assetCapture.flushAt,std::memory_order_release);}
                ReleaseSRWLockExclusive(&assetLock);
                Log("TEXTURE_ASSET_R256_ARM started=%u epoch=%llu present=%llu mark_us=%llu capture_seconds=30 flush_after_seconds=40",unsigned(started),epoch,present,mark);
            }else Log("TEXTURE_ASSET_R256_ARM started=0 reason=busy");
        }
        assetHeld=down;
        const auto deadline=assetDeadline.load(std::memory_order_acquire);if(deadline&&ClockUs()>=deadline)SetEvent(wake);
    }
    if(available&&!warmPosted.load(std::memory_order_acquire)){
        auto* engineQueue=d3d?reinterpret_cast<void*>(Field(d3d,0x111c38)):nullptr;
        auto* queue=engineQueue?reinterpret_cast<ID3D12CommandQueue*>(Field(engineQueue,0x10)):nullptr;
        Ptr<ID3D12CommandQueue> nativeQueue;
        if(queue&&SUCCEEDED(resolveQueue?resolveQueue(queue,&nativeQueue):queue->QueryInterface(IID_PPV_ARGS(&nativeQueue)))&&PostWarmQueue(nativeQueue.Get()))
            Log("DIRECTSTORAGE_R256_PREWARM posted=1 present=%llu pending=%llu",present,pending.load());
    }
    static ULONGLONG next=0;if(present%60)return;const auto now=GetTickCount64();if(now<next)return;next=now+5000;
    Log("DIRECTSTORAGE_R256_STATUS present=%llu available=%u offered=%llu accepted=%llu completed=%llu fallback=%llu failed=%llu pending=%llu batches=%llu accepted_bytes=%llu ineligible=%llu layout_rejected=%llu source_rejected=%llu capacity_rejected=%llu duplicate_rejected=%llu unavailable_rejected=%llu peak_pending=%llu queue_capacity=%u staging_allocations=%llu staging_reuse_batches=%llu submit_cpu_wall_max_us=%llu init_cpu_wall_max_us=%llu poll_cpu_wall_max_us=%llu cpu_payload_bytes=0",present,unsigned(available.load()),offered.load(),accepted.load(),completed.load(),fallback.load(),failed.load(),pending.load(),batches.load(),bytes.load(),ineligible.load(),layoutRejected.load(),sourceRejected.load(),capacityRejected.load(),duplicateRejected.load(),unavailableRejected.load(),peakPending.load(),QueueCapacity,stagingAllocations.load(),stagingReuses.load(),submitMaxUs.load(),initMaxUs.load(),pollMaxUs.load());
    if(publicationInstalled){
        Log("DIRECTSTORAGE_R256_PUBLICATION present=%llu tracked=%llu matched=%llu dropped=%llu callbacks=%llu",present,publicationTracked.load(),publicationDelay.count.load(),publicationDropped.load(),publicationCallbacks.load());
        ReportLatency("gpu_ready_to_renderer_publication",publicationDelay);ReportLatency("renderer_publication_call",publicationCall);
    }
    if(traceLatency){
        ReportLatency("native_allocation",allocationTime);ReportLatency("request_admission",admissionTime);
        ReportLatency("queue_to_submit_begin",queueTime);ReportLatency("storage_completion_observed_batch",storageTime);
        ReportLatency("gpu_completion_observed_batch",gpuObservedTime);ReportLatency("gpu_record_submit_cpu_batch",recordTime);
        ReportLatency("request_to_handoff_begin",endToEndTime);ReportLatency("native_handoff_call",handoffTime);
        for(unsigned level=0;level<32;++level)ReportLatency("request_to_handoff_begin_level",levelLatency[level],int(level));
    }
}
}
