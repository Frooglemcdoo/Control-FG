#pragma once
// CPU evidence replay in observed queue submission order. Does not submit GPU
// work, wait on a fence, or claim that CPU samples equal completed GPU contents.
namespace rt_spot_table_audit {
namespace audit=rt_spot_capture_audit;
inline constexpr UINT64 kCapacity=8ull*1024*1024,kCopyBudget=64ull*1024*1024;
inline constexpr unsigned kMaximumEvents=1024;
inline const GUID kGeneration={0xf27cd9f1,0xf963,0x467a,{0x91,0x40,0x21,0x00,0x00,0x00,0x00,0x01}};
inline const GUID kQueueIdentity={0xf27cd9f1,0xf963,0x467a,{0x91,0x40,0x22,0x00,0x00,0x00,0x00,0x01}};
inline const GUID kResourceIdentity={0xf27cd9f1,0xf963,0x467a,{0x91,0x40,0x23,0x00,0x00,0x00,0x00,0x01}};
inline std::atomic<UINT64> nextIdentity{1},request{0};
inline std::atomic<bool> armed{false},done{false},resetTracking{true},overlapReported{false};
inline std::atomic<unsigned> inFlightSubmissions{0};
inline SRWLOCK lock=SRWLOCK_INIT;
inline UINT64 activeRequest=0,copiedBytes=0,deadline=0,queueIdentity=0,base=0,resourceIdentity=0,extent=0;
inline unsigned eventCount=0,dispatchReports=0;
inline unsigned char archive[kCopyBudget],mirror[kCapacity];
inline rt_spot_capture::TableCoverage coverage;
enum class Kind { Copy,Dispatch };
struct Event {
    Kind kind=Kind::Copy;UINT64 generation=0,resource=0,base=0,extent=0,offset=0,size=0,archiveOffset=0;
    unsigned char identifiers[2][32]{};bool submitted=false;
};
inline Event events[kMaximumEvents];
inline UINT64 Identity(ID3D12Object* object,REFGUID guid,bool reset=false) noexcept {
    UINT64 value=0;UINT size=sizeof(value);
    if(reset || FAILED(object->GetPrivateData(guid,&size,&value)) || size!=sizeof(value) || !value) {
        value=nextIdentity.fetch_add(1);
        if(FAILED(object->SetPrivateData(guid,sizeof(value),&value)))return 0;
    }
    return value;
}
inline UINT64 Token(ID3D12GraphicsCommandList* list,bool reset=false) noexcept {return Identity(list,kGeneration,reset);}
inline void Request() noexcept { request.fetch_add(1,std::memory_order_release); }
inline void Stop(const char* reason) noexcept {
    if(!done.exchange(true))Trace("rt_hit_table_stop reason=%s",reason);
}
inline bool Enabled() noexcept {
    if(!armed.load(std::memory_order_acquire) || done.load(std::memory_order_relaxed))return false;
    return true;
}
// Called from the native dispatch after a user request. A lock protects all
// journal storage; hooks only try it and abandon evidence on contention.
inline void BeginIfRequested() noexcept {
    const auto wanted=request.load(std::memory_order_acquire);
    if(!wanted)return;
    if(!TryAcquireSRWLockExclusive(&lock))return;
    if(activeRequest!=wanted) {
        armed.store(false);activeRequest=wanted;eventCount=dispatchReports=0;copiedBytes=0;
        queueIdentity=base=resourceIdentity=extent=0;coverage.Clear();
        deadline=GetTickCount64()+60000;done.store(false);overlapReported.store(false);
        if(!resetTracking.load()){done.store(false);Stop("reset_hook_unavailable");ReleaseSRWLockExclusive(&lock);return;}
        armed.store(true,std::memory_order_release);
        Trace("rt_hit_table_armed request=%llu trigger=both copy_budget=%llu",wanted,kCopyBudget);
    }
    ReleaseSRWLockExclusive(&lock);
}
inline bool LockRecording() noexcept {
    if(!Enabled())return false;
    if(!TryAcquireSRWLockExclusive(&lock)){Stop("journal_contention");return false;}
    if(!Enabled() || GetTickCount64()>deadline) {
        if(Enabled())Stop("timeout");
        ReleaseSRWLockExclusive(&lock);return false;
    }
    if(eventCount==kMaximumEvents){Stop("event_budget");ReleaseSRWLockExclusive(&lock);return false;}
    return true;
}
inline void Observe(ID3D12GraphicsCommandList* list,ID3D12Resource* destination,UINT64 destinationOffset,
    ID3D12Resource* source,UINT64 sourceOffset,UINT64 count) noexcept {
    if(!list || !destination || !source || !Enabled())return;
    UINT64 target=0;
    if(!TryAcquireSRWLockShared(&audit::ownerLock)){Stop("target_contention");return;}
    target=audit::targetAddress;ReleaseSRWLockShared(&audit::ownerLock);
    const auto desc=destination->GetDesc();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER || desc.Width>kCapacity)return;
    const auto address=destination->GetGPUVirtualAddress();
    if(!rt_spot_capture::ContainsGpuRange(address,desc.Width,target,1))return;
    const auto sourceDesc=source->GetDesc();D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
    if(!count || destinationOffset>desc.Width || count>desc.Width-destinationOffset ||
        sourceDesc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER || sourceOffset>sourceDesc.Width || count>sourceDesc.Width-sourceOffset ||
        FAILED(source->GetHeapProperties(&heap,&flags)) || heap.Type!=D3D12_HEAP_TYPE_UPLOAD){Stop("unsupported_target_write");return;}
    const auto token=Token(list),resource=Identity(destination,kResourceIdentity);
    if(!token || !resource){Stop("object_identity");return;}
    D3D12_RANGE range{static_cast<SIZE_T>(sourceOffset),static_cast<SIZE_T>(sourceOffset+count)},written{0,0};void* mapped=nullptr;
    if(FAILED(source->Map(0,&range,&mapped)) || !mapped){Stop("upload_map");return;}
    if(!LockRecording()){source->Unmap(0,&written);return;}
    if(count>kCopyBudget-copiedBytes)Stop("cpu_copy_budget");
    else {
        const auto* input=static_cast<const unsigned char*>(mapped)+sourceOffset;
        std::memcpy(archive+copiedBytes,input,static_cast<size_t>(count));MemoryBarrier();
        if(std::memcmp(archive+copiedBytes,input,static_cast<size_t>(count)))Stop("unstable_upload");
        else {
            auto& e=events[eventCount++];e={};e.kind=Kind::Copy;e.generation=token;e.resource=resource;
            e.base=address;e.extent=desc.Width;e.offset=destinationOffset;e.size=count;e.archiveOffset=copiedBytes;
            copiedBytes+=count;
        }
    }
    ReleaseSRWLockExclusive(&lock);source->Unmap(0,&written);
}
inline void RecordDispatch(ID3D12GraphicsCommandList* list,UINT64 address,UINT64 size,const unsigned char identifiers[2][32]) noexcept {
    const auto token=Token(list);if(!token){Stop("dispatch_identity");return;}
    if(!LockRecording())return;
    auto& e=events[eventCount++];e={};e.kind=Kind::Dispatch;e.generation=token;e.base=address;e.size=size;
    std::memcpy(e.identifiers,identifiers,sizeof(e.identifiers));ReleaseSRWLockExclusive(&lock);
}
inline void Inspect(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state,const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if(!list || !state || !desc)return;
    BeginIfRequested();if(!Enabled())return;
    const auto& table=desc->HitGroupTable;
    if(!rt_spot_capture::NativeHitTableLayout(table.StartAddress,table.SizeInBytes,table.StrideInBytes)){Stop("table_layout");return;}
    if(!TryAcquireSRWLockExclusive(&audit::ownerLock)){Stop("target_contention");return;}
    audit::targetAddress=table.StartAddress;audit::targetBytes=table.SizeInBytes;
    audit::targetActive.store(true,std::memory_order_release);ReleaseSRWLockExclusive(&audit::ownerLock);
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
    if(FAILED(state->QueryInterface(IID_PPV_ARGS(&properties)))){Stop("state_properties");return;}
    unsigned char identifiers[2][32]{};const wchar_t* names[]={L"HitGroup_0_0",L"HitGroup_1_0"};
    for(unsigned i=0;i<2;++i){const auto* id=properties->GetShaderIdentifier(names[i]);if(!id){Stop("missing_identifier");return;}std::memcpy(identifiers[i],id,32);}
    RecordDispatch(list,table.StartAddress,table.SizeInBytes,identifiers);
}
// Replay one recorded generation. Caller holds lock. Never use CPU recording
// timestamps as execution order; only the actual ExecuteCommandLists array.
inline void Replay(UINT64 token) noexcept {
    for(unsigned i=0;i<eventCount && !done.load();++i) {
        auto& e=events[i];if(e.generation!=token)continue;
        if(e.submitted){Stop("command_list_replay");return;}e.submitted=true;
        if(e.kind==Kind::Copy) {
            if(resourceIdentity!=e.resource || base!=e.base || extent!=e.extent || e.offset==0)coverage.Clear();
            resourceIdentity=e.resource;base=e.base;extent=e.extent;
            std::memcpy(mirror+e.offset,archive+e.archiveOffset,static_cast<size_t>(e.size));
            if(!coverage.Add(e.offset,e.size,kCapacity)){Stop("coverage_capacity");return;}
        } else {
            const bool range=rt_spot_capture::ContainsGpuRange(base,extent,e.base,e.size);
            const UINT64 offset=range?e.base-base:0;
            const bool complete=range && coverage.Covers(offset,e.size);
            rt_spot_capture::TableIdentifiers counts;
            if(complete)counts=rt_spot_capture::CountTableIdentifiers(mirror+offset,e.size,e.identifiers[0],e.identifiers[1]);
            const bool valid=complete && !counts.unknown && (counts.group0+counts.group1)>0;
            if(dispatchReports++<8 || valid)Trace("rt_hit_table_submission request=%llu queue=%llu generation=%llu coverage_complete=%u records=%llu group0=%u group1=%u empty=%u unknown=%u identifiers_validated=%u copy_bytes=%llu gpu_completion_confirmed=0 gpu_hit_capture=0",activeRequest,queueIdentity,token,unsigned(complete),e.size/96,counts.group0,counts.group1,counts.empty,counts.unknown,unsigned(valid),copiedBytes);
            if(valid) {
                for(unsigned n=0;n<2;++n){char hex[65]{};audit::Hex(e.identifiers[n],32,hex);Trace("rt_hit_table_identifier group=%u bytes=%s",n,hex);}
                const UINT64 records=e.size/96,indices[]={0ull,records>1?1ull:0ull,records/2,records-1};
                for(unsigned n=0;n<4;++n){char hex[193]{};audit::Hex(mirror+offset+indices[n]*96,96,hex);Trace("rt_hit_table_record generation=%llu index=%llu bytes=%s",token,indices[n],hex);}
                done.store(true);
            }
        }
    }
}
inline void Submitted(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    if(!queue || !lists || !Enabled())return;
    if(count>1024){Stop("submission_count");return;}
    if(!LockRecording())return;
    for(UINT n=0;n<count && !done.load();++n) {
        UINT64 token=0;UINT size=sizeof(token);
        if(!lists[n] || FAILED(lists[n]->GetPrivateData(kGeneration,&size,&token)))continue;
        bool relevant=false;for(unsigned i=0;i<eventCount;++i)if(events[i].generation==token){relevant=true;break;}
        if(!relevant)continue;
        const auto identity=Identity(queue,kQueueIdentity);
        if(!identity || (queueIdentity && queueIdentity!=identity)){Stop("cross_queue_order_unverified");break;}
        queueIdentity=identity;Replay(token);
    }
    ReleaseSRWLockExclusive(&lock);
}
using Execute=void (STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*);
struct QueueHook{void** table=nullptr;Execute execute=nullptr;};
inline QueueHook queueHooks[16];inline SRWLOCK queueHookLock=SRWLOCK_INIT;
inline thread_local unsigned submitDepth=0;
inline void STDMETHODCALLTYPE ExecuteHook(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) {
    Execute original=nullptr;auto** table=*reinterpret_cast<void***>(queue);
    AcquireSRWLockShared(&queueHookLock);for(const auto& entry:queueHooks)if(entry.table==table){original=entry.execute;break;}ReleaseSRWLockShared(&queueHookLock);
    const bool outer=submitDepth++==0;
    const bool track=outer && (Enabled() || inFlightSubmissions.load()!=0);
    if(track && inFlightSubmissions.fetch_add(1)!=0) {
        done.store(true);
        if(!overlapReported.exchange(true))Trace("rt_hit_table_invalidate reason=overlapping_submissions prior_evidence_unusable=1");
    }
    if(outer)Submitted(queue,count,lists);
    original(queue,count,lists);
    if(track)--inFlightSubmissions;
    --submitDepth;
}
inline void InstallQueue(ID3D12CommandQueue* queue) noexcept {
    auto** table=*reinterpret_cast<void***>(queue);AcquireSRWLockExclusive(&queueHookLock);
    QueueHook* slot=nullptr;
    for(auto& e:queueHooks){if(e.table==table){ReleaseSRWLockExclusive(&queueHookLock);return;}if(!e.table&&!slot)slot=&e;}
    if(slot){*slot={table,reinterpret_cast<Execute>(table[10])};const bool ready=PatchSlot(&table[10],reinterpret_cast<void*>(slot->execute),reinterpret_cast<void*>(&ExecuteHook));if(!ready)*slot={};Trace("rt_hit_table_queue_hook ready=%u type=%u",unsigned(ready),unsigned(queue->GetDesc().Type));}
    ReleaseSRWLockExclusive(&queueHookLock);
}
inline void InstallQueues(ID3D12Device* device) noexcept {
    for(auto type:{D3D12_COMMAND_LIST_TYPE_DIRECT,D3D12_COMMAND_LIST_TYPE_COMPUTE,D3D12_COMMAND_LIST_TYPE_COPY}) {
        D3D12_COMMAND_QUEUE_DESC desc{};desc.Type=type;Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        if(SUCCEEDED(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue))))InstallQueue(queue.Get());
        else Trace("rt_hit_table_queue_hook probe_failed=1 type=%u",unsigned(type));
    }
}
} // namespace rt_spot_table_audit
