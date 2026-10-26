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
inline std::atomic<bool> armed{false},done{false},validated{false},resetTracking{true};
// Readers share the lifetime lock; only rearming takes it exclusively.
// Recording never owns the replay lock, including while copying upload bytes.
inline SRWLOCK lifetimeLock=SRWLOCK_INIT,submissionLock=SRWLOCK_INIT,identityLock=SRWLOCK_INIT;
inline std::atomic<UINT64> activeRequest{0},copiedBytes{0};
inline std::atomic<unsigned> eventCount{0};
inline UINT64 deadline=0,queueIdentity=0,base=0,resourceIdentity=0,extent=0;
inline unsigned dispatchReports=0;
inline constexpr unsigned kMaximumQueues=32;
struct QueueClock {
    UINT64 value[kMaximumQueues]{};
    UINT64 dependencies[2]{}; // Provenance for the bounded 128-fence registry.
    bool UsesFence(unsigned fence)const noexcept {return (dependencies[fence/64] & (UINT64(1)<<(fence%64)))!=0;}
    void AddFence(unsigned fence) noexcept {dependencies[fence/64]|=UINT64(1)<<(fence%64);}
    void KeepLocal(unsigned queue) noexcept {
        const auto local=value[queue];*this={};value[queue]=local;
    }
    bool Includes(const QueueClock& other)const noexcept {
        for(unsigned i=0;i<kMaximumQueues;++i)if(value[i]<other.value[i])return false;
        return true;
    }
    void Merge(const QueueClock& other) noexcept {
        for(unsigned i=0;i<2;++i)dependencies[i]|=other.dependencies[i];
        for(unsigned i=0;i<kMaximumQueues;++i)if(value[i]<other.value[i])value[i]=other.value[i];
    }
};
inline QueueClock replayClock,lastWriteClock;
inline void ResetQueueOrder() noexcept;
inline unsigned char archive[kCopyBudget],mirror[kCapacity],translatedTable[kCapacity];
inline rt_spot_capture::TableCoverage coverage;
enum class Kind { Copy,Dispatch };
struct Event {
    Kind kind=Kind::Copy;UINT64 generation=0,resource=0,base=0,extent=0,offset=0,size=0,archiveOffset=0;
    unsigned char identifiers[2][32]{},captureIdentifiers[2][32]{};bool submitted=false,captureReady=false;
};
inline Event events[kMaximumEvents];
inline std::atomic<bool> published[kMaximumEvents];
inline UINT64 Identity(ID3D12Object* object,REFGUID guid,bool reset=false) noexcept {
    UINT64 value=0;UINT size=sizeof(value);
    if(!reset && SUCCEEDED(object->GetPrivateData(guid,&size,&value)) && size==sizeof(value) && value)return value;
    AcquireSRWLockExclusive(&identityLock);
    size=sizeof(value);
    if(reset || FAILED(object->GetPrivateData(guid,&size,&value)) || size!=sizeof(value) || !value) {
        value=nextIdentity.fetch_add(1);
        if(FAILED(object->SetPrivateData(guid,sizeof(value),&value)))value=0;
    }
    ReleaseSRWLockExclusive(&identityLock);
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
// Retain a shared lifetime lease while publishing or replaying. Ordinary
// producers and consumers do not exclude one another. Rearm waits for neither:
// it retries from a later dispatch if the old request still has active callbacks.
inline void BeginIfRequested() noexcept {
    const auto wanted=request.load(std::memory_order_acquire);
    if(!wanted || activeRequest.load()==wanted)return;
    if(!TryAcquireSRWLockExclusive(&lifetimeLock))return;
    if(activeRequest.load()!=wanted) {
        armed.store(false);activeRequest.store(wanted);eventCount.store(0);dispatchReports=0;copiedBytes.store(0);
        for(auto& ready:published)ready.store(false,std::memory_order_relaxed);
        queueIdentity=base=resourceIdentity=extent=0;coverage.Clear();ResetQueueOrder();
        deadline=GetTickCount64()+60000;done.store(false);validated.store(false);
        if(!resetTracking.load()){Stop("reset_hook_unavailable");ReleaseSRWLockExclusive(&lifetimeLock);return;}
        armed.store(true,std::memory_order_release);
        Trace("rt_hit_table_armed request=%llu trigger=both copy_budget=%llu",wanted,kCopyBudget);
    }
    ReleaseSRWLockExclusive(&lifetimeLock);
}
class CaptureLease {
    bool held_=false;
public:
    explicit CaptureLease(bool allowCompleted=false) noexcept {
        if(!armed.load() || (!allowCompleted && done.load()))return;
        AcquireSRWLockShared(&lifetimeLock);
        if(!armed.load() || (!allowCompleted && done.load()) || GetTickCount64()>deadline) {
            if(Enabled())Stop("timeout");
            ReleaseSRWLockShared(&lifetimeLock);return;
        }
        held_=true;
    }
    ~CaptureLease(){if(held_)ReleaseSRWLockShared(&lifetimeLock);}
    explicit operator bool()const noexcept{return held_;}
    CaptureLease(const CaptureLease&)=delete;
    CaptureLease& operator=(const CaptureLease&)=delete;
};
inline bool ReserveEvent(unsigned& index) noexcept {
    index=eventCount.load(std::memory_order_relaxed);
    while(index<kMaximumEvents) {
        if(eventCount.compare_exchange_weak(index,index+1,std::memory_order_acq_rel))return true;
    }
    Stop("event_budget");return false;
}
inline bool ReserveBytes(UINT64 size,UINT64& offset) noexcept {
    offset=copiedBytes.load(std::memory_order_relaxed);
    while(offset<=kCopyBudget && size<=kCopyBudget-offset) {
        if(copiedBytes.compare_exchange_weak(offset,offset+size,std::memory_order_relaxed))return true;
    }
    Stop("cpu_copy_budget");return false;
}
inline void Observe(ID3D12GraphicsCommandList* list,ID3D12Resource* destination,UINT64 destinationOffset,
    ID3D12Resource* source,UINT64 sourceOffset,UINT64 count) noexcept {
    if(!list || !destination || !source || !Enabled())return;
    CaptureLease lease;if(!lease)return;
    UINT64 target=0;
    AcquireSRWLockShared(&audit::ownerLock);
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
    unsigned index=0;UINT64 offset=0;
    if(ReserveEvent(index) && ReserveBytes(count,offset)) {
        const auto* input=static_cast<const unsigned char*>(mapped)+sourceOffset;
        std::memcpy(archive+offset,input,static_cast<size_t>(count));MemoryBarrier();
        if(std::memcmp(archive+offset,input,static_cast<size_t>(count)))Stop("unstable_upload");
        else {
            auto& e=events[index];e={};e.kind=Kind::Copy;e.generation=token;e.resource=resource;
            e.base=address;e.extent=desc.Width;e.offset=destinationOffset;e.size=count;e.archiveOffset=offset;
            // Release publication makes the immutable record and its bytes
            // visible together. Other threads never inspect a half-built event.
            published[index].store(true,std::memory_order_release);
        }
    }
    source->Unmap(0,&written);
}
inline void RecordDispatch(ID3D12GraphicsCommandList* list,UINT64 address,UINT64 size,const unsigned char identifiers[2][32],const unsigned char (*captureIdentifiers)[32]=nullptr) noexcept {
    CaptureLease lease;if(!lease)return;
    const auto token=Token(list);if(!token){Stop("dispatch_identity");return;}
    unsigned index=0;if(!ReserveEvent(index))return;
    auto& e=events[index];e={};e.kind=Kind::Dispatch;e.generation=token;e.base=address;e.size=size;
    std::memcpy(e.identifiers,identifiers,sizeof(e.identifiers));
    if(captureIdentifiers){std::memcpy(e.captureIdentifiers,captureIdentifiers,sizeof(e.captureIdentifiers));e.captureReady=true;}
    published[index].store(true,std::memory_order_release);
}

inline void Inspect(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state,const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if(!list || !state || !desc)return;
    BeginIfRequested();if(!Enabled())return;
    const auto& table=desc->HitGroupTable;
    if(!rt_spot_capture::NativeHitTableLayout(table.StartAddress,table.SizeInBytes,table.StrideInBytes)){Stop("table_layout");return;}
    AcquireSRWLockExclusive(&audit::ownerLock);
    audit::targetAddress=table.StartAddress;audit::targetBytes=table.SizeInBytes;
    audit::targetActive.store(true,std::memory_order_release);ReleaseSRWLockExclusive(&audit::ownerLock);
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
    if(FAILED(state->QueryInterface(IID_PPV_ARGS(&properties)))){Stop("state_properties");return;}
    unsigned char identifiers[2][32]{};const wchar_t* names[]={L"HitGroup_0_0",L"HitGroup_1_0"};
    for(unsigned i=0;i<2;++i){const auto* id=properties->GetShaderIdentifier(names[i]);if(!id){Stop("missing_identifier");return;}std::memcpy(identifiers[i],id,32);}
    unsigned char captureIdentifiers[4][32]{};UINT identifierBytes=sizeof(captureIdentifiers);
    const bool captureReady=SUCCEEDED(state->GetPrivateData(rt_spot_hit_pipeline::kIdentifiers,&identifierBytes,captureIdentifiers)) && identifierBytes==sizeof(captureIdentifiers);
    RecordDispatch(list,table.StartAddress,table.SizeInBytes,identifiers,captureReady?captureIdentifiers:nullptr);
}
// Replay one recorded generation. Caller holds a lifetime lease and replay lock. Never use CPU recording
// timestamps as execution order; only the actual ExecuteCommandLists array.
inline void Replay(UINT64 token) noexcept {
    const auto available=eventCount.load(std::memory_order_acquire);
    for(unsigned i=0;i<available && !done.load();++i) {
        if(!published[i].load(std::memory_order_acquire))continue;
        auto& e=events[i];if(e.generation!=token)continue;
        if(e.submitted){Stop("command_list_replay");return;}e.submitted=true;
        if(e.kind==Kind::Copy) {
            if(coverage.count && !replayClock.Includes(lastWriteClock)){Stop("unordered_target_writes");return;}
            lastWriteClock=replayClock;
            if(resourceIdentity!=e.resource || base!=e.base || extent!=e.extent || e.offset==0)coverage.Clear();
            resourceIdentity=e.resource;base=e.base;extent=e.extent;
            std::memcpy(mirror+e.offset,archive+e.archiveOffset,static_cast<size_t>(e.size));
            if(!coverage.Add(e.offset,e.size,kCapacity)){Stop("coverage_capacity");return;}
        } else {
            const bool range=rt_spot_capture::ContainsGpuRange(base,extent,e.base,e.size);
            const UINT64 offset=range?e.base-base:0;
            const bool ordered=replayClock.Includes(lastWriteClock);
            const bool complete=ordered && range && coverage.Covers(offset,e.size);
            rt_spot_capture::TableIdentifiers counts;
            if(complete)counts=rt_spot_capture::CountTableIdentifiers(mirror+offset,e.size,e.identifiers[0],e.identifiers[1]);
            const bool valid=complete && !counts.unknown && (counts.group0+counts.group1)>0;
            if(dispatchReports++<8 || valid)Trace("rt_hit_table_submission request=%llu queue=%llu generation=%llu coverage_complete=%u records=%llu group0=%u group1=%u empty=%u unknown=%u identifiers_validated=%u copy_bytes=%llu dependency_ordered=%u gpu_completion_confirmed=0 gpu_hit_capture=0",activeRequest.load(),queueIdentity,token,unsigned(complete),e.size/96,counts.group0,counts.group1,counts.empty,counts.unknown,unsigned(valid),copiedBytes.load(),unsigned(ordered));
            if(valid) {
                rt_spot_capture::IdentifierReplacement replacements[2]{};
                for(unsigned n=0;n<2;++n){std::memcpy(replacements[n].native.data(),e.identifiers[n],32);std::memcpy(replacements[n].capture.data(),e.captureIdentifiers[n],32);}
                const bool translated=e.captureReady && rt_spot_capture::TranslateHitTableBytes(mirror+offset,static_cast<size_t>(e.size),replacements,2,translatedTable,sizeof(translatedTable));
                Trace("rt_hit_table_translation ready=%u pipeline_ready=%u records=%llu null_records=%u local_arguments_preserved=1 gpu_upload=0 gpu_hit_capture=0",unsigned(translated),unsigned(e.captureReady),e.size/96,counts.empty);
                for(unsigned n=0;n<2;++n){char hex[65]{};audit::Hex(e.identifiers[n],32,hex);Trace("rt_hit_table_identifier group=%u bytes=%s",n,hex);}
                const UINT64 records=e.size/96,indices[]={0ull,records>1?1ull:0ull,records/2,records-1};
                for(unsigned n=0;n<4;++n){char hex[193]{};audit::Hex(mirror+offset+indices[n]*96,96,hex);Trace("rt_hit_table_record generation=%llu index=%llu bytes=%s",token,indices[n],hex);}
                validated.store(true);done.store(true);
            }
        }
    }
}
inline bool HasGeneration(UINT64 token) noexcept {
    const auto available=eventCount.load(std::memory_order_acquire);
    for(unsigned i=0;i<available;++i)
        if(published[i].load(std::memory_order_acquire) && events[i].generation==token)return true;
    return false;
}
// Bounded per-queue timelines. A wait may be recorded before its signal. Such
// a queue stays pending in the CPU model; native GPU calls are always forwarded.
inline constexpr unsigned kMaximumQueueNodes=4096,kMaximumFences=128,kNoNode=~0u;
enum class QueueOperation { Execute,Signal,Wait };
struct QueueState {
    UINT64 identity=0;unsigned head=kNoNode,tail=kNoNode;bool busy=false;
    QueueClock clock;
};
struct FenceState {
    UINT64 identity=0,baseline=0,lastValue=0;unsigned producer=kNoNode;
    bool unsupported=false;
    UINT64 firstSignalValue=0,lastBoundaryPoll=0;
    Microsoft::WRL::ComPtr<ID3D12Fence> source;
};
struct QueueNode {
    QueueOperation operation=QueueOperation::Execute;
    unsigned queue=0,fence=kNoNode,next=kNoNode;
    UINT64 value=0,token=0;bool ready=false,processed=false,pendingReported=false;
    QueueClock signalClock;
};
inline QueueState queues[kMaximumQueues];
inline FenceState fences[kMaximumFences];
inline QueueNode queueNodes[kMaximumQueueNodes];
inline unsigned queueCount=0,fenceCount=0,queueNodeCount=0;
inline unsigned waitReports=0,signalReports=0;
inline const GUID kFenceIdentity={0xf27cd9f1,0xf963,0x467a,{0x91,0x40,0x24,0x00,0x00,0x00,0x00,0x01}};
inline void ResetQueueOrder() noexcept {
    for(auto& q:queues)q={};
    for(auto& f:fences)f={};
    queueCount=fenceCount=queueNodeCount=waitReports=signalReports=0;
    replayClock={};lastWriteClock={};
}
inline unsigned FindQueue(ID3D12CommandQueue* queue) noexcept {
    const auto identity=Identity(queue,kQueueIdentity);
    if(!identity){Stop("queue_identity");return kNoNode;}
    for(unsigned i=0;i<queueCount;++i)if(queues[i].identity==identity)return i;
    if(queueCount==kMaximumQueues){Stop("queue_budget");return kNoNode;}
    const auto index=queueCount++;queues[index].identity=identity;return index;
}
inline void InstallFence(ID3D12Fence* fence) noexcept;
inline unsigned FindFence(ID3D12Fence* fence) noexcept {
    if(!fence){Stop("null_fence");return kNoNode;}
    InstallFence(fence);
    if(!resetTracking.load()){Stop("fence_hook_unavailable");return kNoNode;}
    const auto identity=Identity(fence,kFenceIdentity);
    if(!identity){Stop("fence_identity");return kNoNode;}
    for(unsigned i=0;i<fenceCount;++i)if(fences[i].identity==identity)return i;
    if(fenceCount==kMaximumFences){Stop("fence_budget");return kNoNode;}
    const auto baseline=fence->GetCompletedValue();
    if(baseline==~UINT64(0)){Stop("device_removed");return kNoNode;}
    const auto index=fenceCount++;fences[index]={identity,baseline,baseline,kNoNode};fences[index].source=fence;return index;
}
// Unsupported fence behavior removes only proofs that used that fence. Native
// calls still run. Same-queue order and independent fence proofs remain usable.
inline void IsolateFence(unsigned index,const char* reason,UINT64 queue,UINT64 value) noexcept {
    auto& fence=fences[index];if(fence.unsupported)return;
    fence.unsupported=true;
    const UINT64 previousQueue=fence.producer==kNoNode?0:queues[fence.producer].identity;
    unsigned affectedQueues=0,affectedSignals=0;
    for(unsigned i=0;i<queueCount;++i)if(queues[i].clock.UsesFence(index)) {
        queues[i].clock.KeepLocal(i);++affectedQueues;
    }
    for(unsigned i=0;i<queueNodeCount;++i) {
        auto& node=queueNodes[i];
        if(node.operation==QueueOperation::Signal && node.signalClock.UsesFence(index)) {
            node.signalClock.KeepLocal(node.queue);++affectedSignals;
        }
    }
    const bool discard=lastWriteClock.UsesFence(index);
    if(discard){coverage.Clear();lastWriteClock={};}
    Trace("rt_hit_table_fence_isolated reason=%s fence=%llu queue=%llu previous_queue=%llu value=%llu previous_value=%llu baseline=%llu affected_queues=%u affected_signals=%u coverage_discarded=%u capture_continues=1",
        reason,fence.identity,queue,previousQueue,value,fence.lastValue,fence.baseline,affectedQueues,affectedSignals,unsigned(discard));
}
inline unsigned AppendNode(unsigned queue,QueueOperation operation,UINT64 token=0,unsigned fence=kNoNode,UINT64 value=0) noexcept {
    if(queueNodeCount==kMaximumQueueNodes){Stop("queue_event_budget");return kNoNode;}
    const auto index=queueNodeCount++;auto& node=queueNodes[index];node={};
    node.queue=queue;node.operation=operation;node.token=token;node.fence=fence;node.value=value;
    auto& q=queues[queue];if(q.tail!=kNoNode)queueNodes[q.tail].next=index;else q.head=index;
    q.tail=index;return index;
}
// Arming can miss earlier queued signals that have not yet completed. Never
// bind an initial wait to a later signal behind that wait (a false model cycle).
// CompletedValue permits dropping this missing edge, not importing any clock.
inline bool ReleaseInitialWait(QueueNode& node) noexcept {
    auto& fence=fences[node.fence];
    const bool initialGap=fence.producer==kNoNode || node.value<fence.firstSignalValue;
    if(!initialGap || !fence.source)return false;
    const auto now=GetTickCount64();
    if(fence.lastBoundaryPoll && now-fence.lastBoundaryPoll<2)return false;
    fence.lastBoundaryPoll=now;
    const auto completed=fence.source->GetCompletedValue();
    if(completed==~UINT64(0)){Stop("device_removed");return false;}
    if(completed<node.value)return false;
    // Drop all partial bytes at the cut. Fresh observed copies must provide
    // complete coverage before any later dispatch can validate.
    coverage.Clear();lastWriteClock={};
    Trace("rt_hit_table_start_boundary queue=%llu fence=%llu wait=%llu completed=%llu baseline=%llu first_signal=%llu imported_dependency=0 coverage_discarded=1",
        queues[node.queue].identity,fence.identity,node.value,completed,fence.baseline,fence.firstSignalValue);
    return true;
}
inline void DrainQueueOrder() noexcept {
    bool progress=true;
    while(progress && !done.load()) {
        progress=false;
        for(unsigned qIndex=0;qIndex<queueCount && !done.load();++qIndex) {
            auto& q=queues[qIndex];if(q.head==kNoNode)continue;
            auto& node=queueNodes[q.head];if(!node.ready)continue;
            if(node.operation==QueueOperation::Wait) {
                const auto& fence=fences[node.fence];
                if(fence.unsupported) {
                    if(waitReports++<32)Trace("rt_hit_table_fence_wait queue=%llu fence=%llu value=%llu dependency_resolved=0 reason=isolated_fence capture_continues=1",q.identity,fence.identity,node.value);
                } else if(node.value>fence.baseline) {
                    const QueueNode* signal=nullptr;
                    for(unsigned i=0;i<queueNodeCount;++i) {
                        const auto& candidate=queueNodes[i];
                        if(candidate.operation==QueueOperation::Signal && candidate.fence==node.fence && candidate.value>=node.value) {
                            signal=&candidate;break;
                        }
                    }
                    if(!signal || !signal->processed) {
                        if(!node.pendingReported && waitReports++<32)Trace("rt_hit_table_fence_pending queue=%llu fence=%llu value=%llu baseline=%llu matching_signal=%u signal_processed=%u",q.identity,fence.identity,node.value,fence.baseline,unsigned(signal!=nullptr),unsigned(signal && signal->processed));
                        node.pendingReported=true;
                        if(!ReleaseInitialWait(node))continue;
                    } else {
                        q.clock.Merge(signal->signalClock);q.clock.AddFence(node.fence);
                        if(waitReports++<16)Trace("rt_hit_table_fence_wait queue=%llu fence=%llu value=%llu signal=%llu dependency_resolved=1",q.identity,fence.identity,node.value,signal->value);
                    }
                }
            }
            ++q.clock.value[qIndex];
            if(node.operation==QueueOperation::Signal)node.signalClock=q.clock;
            if(node.operation==QueueOperation::Execute) {
                replayClock=q.clock;queueIdentity=q.identity;Replay(node.token);
            }
            node.processed=true;q.head=node.next;if(q.head==kNoNode)q.tail=kNoNode;progress=true;
        }
    }
}
struct QueueCall {unsigned queue=kNoNode,first=0,last=0;};
// Caller holds a lifetime lease through the native call. The short metadata
// lock is never held across a native queue call or upload-byte publication.
inline QueueCall BeginQueueCall(ID3D12CommandQueue* queue,QueueOperation operation,UINT count=0,
    ID3D12CommandList*const* lists=nullptr,ID3D12Fence* fence=nullptr,UINT64 value=0) noexcept {
    QueueCall call;if(!queue || !Enabled())return call;
    AcquireSRWLockExclusive(&submissionLock);
    const auto index=FindQueue(queue);
    if(index!=kNoNode && !done.load()) {
        auto& q=queues[index];
        if(q.busy)Stop("overlapping_same_queue_calls");
        else {
            q.busy=true;call.queue=index;call.first=queueNodeCount;
            if(operation==QueueOperation::Execute) {
                if(count>1024 || (count && !lists))Stop("submission_count");
                else for(UINT n=0;n<count && !done.load();++n) {
                    UINT64 token=0;UINT size=sizeof(token);
                    if(lists[n] && SUCCEEDED(lists[n]->GetPrivateData(kGeneration,&size,&token)) && HasGeneration(token))
                        AppendNode(index,operation,token);
                }
            } else {
                const auto f=FindFence(fence);
                if(f!=kNoNode && !done.load()) {
                    auto& state=fences[f];
                    if(operation==QueueOperation::Signal) {
                        if(state.producer!=kNoNode && state.producer!=index)
                            IsolateFence(f,"multiple_queue_producers",q.identity,value);
                        else if(value<state.lastValue)
                            IsolateFence(f,"decreasing_value",q.identity,value);
                        // Equal values are legal: a wait must use the earliest
                        // sufficient signal, never gain the later signal's work.
                        if(signalReports++<32)Trace("rt_hit_table_fence_signal queue=%llu fence=%llu value=%llu previous_value=%llu baseline=%llu repeated=%u isolated=%u",q.identity,state.identity,value,state.lastValue,state.baseline,unsigned(value==state.lastValue),unsigned(state.unsupported));
                        if(!state.unsupported){
                            if(state.producer==kNoNode)state.firstSignalValue=value;
                            state.producer=index;state.lastValue=value;
                        }
                    }
                    if(!done.load())AppendNode(index,operation,0,f,value);
                }
            }
            call.last=queueNodeCount;
        }
    }
    ReleaseSRWLockExclusive(&submissionLock);return call;
}
inline void EndQueueCall(const QueueCall& call,HRESULT result=S_OK) noexcept {
    if(call.queue==kNoNode)return;
    AcquireSRWLockExclusive(&submissionLock);
    if(FAILED(result))Stop("native_queue_call_failed");
    for(unsigned i=call.first;i<call.last;++i)queueNodes[i].ready=true;
    queues[call.queue].busy=false;
    if(!done.load())DrainQueueOrder();
    ReleaseSRWLockExclusive(&submissionLock);
}
inline void Submitted(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    CaptureLease lease;if(!lease)return;
    EndQueueCall(BeginQueueCall(queue,QueueOperation::Execute,count,lists));
}
using Execute=void (STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*);
using FenceCall=HRESULT (STDMETHODCALLTYPE*)(ID3D12CommandQueue*,ID3D12Fence*,UINT64);
using CpuSignal=HRESULT (STDMETHODCALLTYPE*)(ID3D12Fence*,UINT64);
struct QueueHook{void** table=nullptr;Execute execute=nullptr;FenceCall signal=nullptr,wait=nullptr;};
inline QueueHook queueHooks[16];inline SRWLOCK queueHookLock=SRWLOCK_INIT;
struct FenceHook {void** table=nullptr;CpuSignal signal=nullptr;};
inline FenceHook fenceHooks[16];
inline thread_local unsigned submitDepth=0;
inline QueueHook LookupQueue(ID3D12CommandQueue* queue) noexcept {
    QueueHook result;auto** table=*reinterpret_cast<void***>(queue);
    AcquireSRWLockShared(&queueHookLock);
    for(const auto& entry:queueHooks)if(entry.table==table){result=entry;break;}
    ReleaseSRWLockShared(&queueHookLock);return result;
}
inline void STDMETHODCALLTYPE ExecuteHook(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) {
    const auto original=LookupQueue(queue).execute;
    if(submitDepth){original(queue,count,lists);return;}
    ++submitDepth;CaptureLease lease;QueueCall call;
    if(lease)call=BeginQueueCall(queue,QueueOperation::Execute,count,lists);
    rt_spot_capture_runtime::BeforeExecute(queue,count,lists);
    const auto timing=performance::BeforeExecute(queue,count,lists);
    original(queue,count,lists);
    performance::AfterExecute(queue,timing);
    rt_spot_capture_runtime::AfterExecute(queue,count,lists);
    if(lease)EndQueueCall(call);
    --submitDepth;
}
inline HRESULT ForwardFenceCall(ID3D12CommandQueue* queue,ID3D12Fence* fence,UINT64 value,QueueOperation operation) {
    const auto hook=LookupQueue(queue);const auto original=operation==QueueOperation::Signal?hook.signal:hook.wait;
    if(submitDepth)return original(queue,fence,value);
    ++submitDepth;CaptureLease lease;QueueCall call;
    if(lease)call=BeginQueueCall(queue,operation,0,nullptr,fence,value);
    const auto result=original(queue,fence,value);
    if(lease)EndQueueCall(call,result);
    --submitDepth;return result;
}
inline HRESULT STDMETHODCALLTYPE SignalHook(ID3D12CommandQueue* queue,ID3D12Fence* fence,UINT64 value) {
    return ForwardFenceCall(queue,fence,value,QueueOperation::Signal);
}
inline HRESULT STDMETHODCALLTYPE WaitHook(ID3D12CommandQueue* queue,ID3D12Fence* fence,UINT64 value) {
    return ForwardFenceCall(queue,fence,value,QueueOperation::Wait);
}
inline HRESULT STDMETHODCALLTYPE CpuSignalHook(ID3D12Fence* fence,UINT64 value) {
    // CPU signals do not carry a GPU queue dependency. Never let one silently
    // satisfy a recorded GPU producer/consumer edge.
    CaptureLease lease;
    if(lease) {
        AcquireSRWLockExclusive(&submissionLock);
        const auto index=FindFence(fence);
        if(index!=kNoNode)IsolateFence(index,"cpu_signal",0,value);
        if(!done.load())DrainQueueOrder();
        ReleaseSRWLockExclusive(&submissionLock);
    }
    CpuSignal original=nullptr;auto** table=*reinterpret_cast<void***>(fence);
    AcquireSRWLockShared(&queueHookLock);
    for(const auto& entry:fenceHooks)if(entry.table==table){original=entry.signal;break;}
    ReleaseSRWLockShared(&queueHookLock);
    return original(fence,value);
}
inline void InstallFence(ID3D12Fence* fence) noexcept {
    auto** table=*reinterpret_cast<void***>(fence);AcquireSRWLockExclusive(&queueHookLock);
    FenceHook* slot=nullptr;
    for(auto& entry:fenceHooks) {
        if(entry.table==table){ReleaseSRWLockExclusive(&queueHookLock);return;}
        if(!entry.table && !slot)slot=&entry;
    }
    if(slot) {
        *slot={table,reinterpret_cast<CpuSignal>(table[10])};
        if(!PatchSlot(&table[10],reinterpret_cast<void*>(slot->signal),reinterpret_cast<void*>(&CpuSignalHook)))resetTracking.store(false);
    } else resetTracking.store(false);
    ReleaseSRWLockExclusive(&queueHookLock);
}
inline void InstallQueue(ID3D12CommandQueue* queue) noexcept {
    auto** table=*reinterpret_cast<void***>(queue);AcquireSRWLockExclusive(&queueHookLock);
    QueueHook* slot=nullptr;
    for(auto& e:queueHooks){if(e.table==table){ReleaseSRWLockExclusive(&queueHookLock);return;}if(!e.table&&!slot)slot=&e;}
    if(slot) {
        *slot={table,reinterpret_cast<Execute>(table[10]),reinterpret_cast<FenceCall>(table[14]),reinterpret_cast<FenceCall>(table[15])};
        const bool execute=PatchSlot(&table[10],reinterpret_cast<void*>(slot->execute),reinterpret_cast<void*>(&ExecuteHook));
        const bool signal=PatchSlot(&table[14],reinterpret_cast<void*>(slot->signal),reinterpret_cast<void*>(&SignalHook));
        const bool wait=PatchSlot(&table[15],reinterpret_cast<void*>(slot->wait),reinterpret_cast<void*>(&WaitHook));
        if(!execute || !signal || !wait)resetTracking.store(false);
        Trace("rt_hit_table_queue_hook ready=%u type=%u signal=%u wait=%u",unsigned(execute&&signal&&wait),unsigned(queue->GetDesc().Type),unsigned(signal),unsigned(wait));
    } else resetTracking.store(false);
    ReleaseSRWLockExclusive(&queueHookLock);
}
inline void InstallQueues(ID3D12Device* device) noexcept {
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    if(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)))) {
        InstallFence(fence.Get());
    } else resetTracking.store(false);
    for(auto type:{D3D12_COMMAND_LIST_TYPE_DIRECT,D3D12_COMMAND_LIST_TYPE_COMPUTE,D3D12_COMMAND_LIST_TYPE_COPY}) {
        D3D12_COMMAND_QUEUE_DESC desc{};desc.Type=type;Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        if(SUCCEEDED(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue))))InstallQueue(queue.Get());
        else Trace("rt_hit_table_queue_hook probe_failed=1 type=%u",unsigned(type));
    }
}
} // namespace rt_spot_table_audit

