#pragma once
// Included inside native_lighting. Bounded CPU mirror of copies recorded in the
// same command-list generation as the native spotlight dispatch. No GPU work.
namespace rt_spot_table_audit {
namespace audit=rt_spot_capture_audit;
inline constexpr UINT64 kCapacity=8ull*1024*1024, kCopyBudget=64ull*1024*1024;
inline const GUID kGeneration={0xf27cd9f1,0xf963,0x467a,{0x91,0x40,0x21,0x00,0x00,0x00,0x00,0x01}};
inline std::atomic<UINT64> nextGeneration{1}, copiedBytes{0}, lossEpoch{0};
inline std::atomic<unsigned> reports{0};
inline std::atomic<bool> done{false};
inline SRWLOCK lock=SRWLOCK_INIT;
inline unsigned char bytes[kCapacity];
inline rt_spot_capture::TableCoverage coverage;
inline UINT64 generation=0, destinationBase=0, destinationBytes=0, observedLoss=0;
inline UINT64 Token(ID3D12GraphicsCommandList* list,bool reset=false) noexcept {
    UINT64 value=0;UINT size=sizeof(value);
    if(reset || FAILED(list->GetPrivateData(kGeneration,&size,&value)) || size!=sizeof(value) || !value) {
        value=nextGeneration.fetch_add(1);
        if(FAILED(list->SetPrivateData(kGeneration,sizeof(value),&value)))return 0;
    }
    return value;
}
inline bool Enabled() noexcept {
    return !done.load(std::memory_order_relaxed) && audit::targetActive.load(std::memory_order_acquire) &&
        audit::attempts.load(std::memory_order_relaxed)<audit::kMaximumAttempts;
}
inline void Observe(ID3D12GraphicsCommandList* list,ID3D12Resource* destination,UINT64 destinationOffset,
    ID3D12Resource* source,UINT64 sourceOffset,UINT64 count) noexcept {
    if(!list || !destination || !source || !Enabled())return;
    UINT64 target=0;
    if(!TryAcquireSRWLockShared(&audit::ownerLock)) {++lossEpoch;return;}
    target=audit::targetAddress;ReleaseSRWLockShared(&audit::ownerLock);
    const auto desc=destination->GetDesc();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER || desc.Width>kCapacity)return;
    const auto base=destination->GetGPUVirtualAddress();
    if(!rt_spot_capture::ContainsGpuRange(base,desc.Width,target,1))return;
    const auto token=Token(list);
    const auto sourceDesc=source->GetDesc();
    D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
    if(!token || !count || destinationOffset>desc.Width || count>desc.Width-destinationOffset ||
        sourceDesc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER || sourceOffset>sourceDesc.Width || count>sourceDesc.Width-sourceOffset ||
        FAILED(source->GetHeapProperties(&heap,&flags)) || heap.Type!=D3D12_HEAP_TYPE_UPLOAD) {++lossEpoch;return;}
    const auto spent=copiedBytes.fetch_add(count);
    if(spent>kCopyBudget || count>kCopyBudget-spent) {
        if(!done.exchange(true))Trace("rt_hit_table_stop reason=cpu_copy_budget bytes=%llu",spent);
        return;
    }
    D3D12_RANGE range{static_cast<SIZE_T>(sourceOffset),static_cast<SIZE_T>(sourceOffset+count)};
    void* mapped=nullptr;
    if(FAILED(source->Map(0,&range,&mapped)) || !mapped) {++lossEpoch;return;}
    D3D12_RANGE written{0,0};
    if(!TryAcquireSRWLockExclusive(&lock)) {++lossEpoch;source->Unmap(0,&written);return;}
    const auto epoch=lossEpoch.load();
    // Offset zero starts a fresh upload of the reused buffer, even within one
    // command list. Never combine its new prefix with a preceding pass's tail.
    if(generation!=token || destinationBase!=base || destinationBytes!=desc.Width || observedLoss!=epoch || destinationOffset==0)coverage.Clear();
    generation=token;destinationBase=base;destinationBytes=desc.Width;observedLoss=epoch;
    const auto* input=static_cast<const unsigned char*>(mapped)+sourceOffset;
    std::memcpy(bytes+destinationOffset,input,static_cast<size_t>(count));
    MemoryBarrier();
    if(std::memcmp(bytes+destinationOffset,input,static_cast<size_t>(count)) || !coverage.Add(destinationOffset,count,kCapacity)) {
        coverage.Clear();++lossEpoch;
    }
    ReleaseSRWLockExclusive(&lock);source->Unmap(0,&written);
}
// Caller holds lock. CPU recording correlation only; no submission assertion.
inline bool Matches(UINT64 token,UINT64 address,UINT64 size) noexcept {
    return token && generation==token && observedLoss==lossEpoch.load() &&
        rt_spot_capture::ContainsGpuRange(destinationBase,destinationBytes,address,size) &&
        coverage.Covers(address-destinationBase,size);
}
inline void Inspect(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state,const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if(!list || !state || !desc || !Enabled())return;
    const auto& table=desc->HitGroupTable;
    if(!rt_spot_capture::NativeHitTableLayout(table.StartAddress,table.SizeInBytes,table.StrideInBytes))return;
    const auto token=Token(list);
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
    if(FAILED(state->QueryInterface(IID_PPV_ARGS(&properties)))) {
        if(!done.exchange(true))Trace("rt_hit_table_stop reason=state_properties_unavailable");
        return;
    }
    unsigned char identifiers[2][32]{};
    const wchar_t* names[]={L"HitGroup_0_0",L"HitGroup_1_0"};
    for(unsigned i=0;i<2;++i) {
        const auto* id=properties->GetShaderIdentifier(names[i]);
        if(!id) {
            if(!done.exchange(true))Trace("rt_hit_table_stop reason=missing_identifier name=%ls",names[i]);
            return;
        }
        std::memcpy(identifiers[i],id,32);
    }
    if(!TryAcquireSRWLockExclusive(&lock))return;
    const bool sameList=token && generation==token;
    const bool range=rt_spot_capture::ContainsGpuRange(destinationBase,destinationBytes,table.StartAddress,table.SizeInBytes);
    const UINT64 offset=range ? table.StartAddress-destinationBase : 0;
    const bool complete=Matches(token,table.StartAddress,table.SizeInBytes);
    const UINT64 records=table.SizeInBytes/96;
    rt_spot_capture::TableIdentifiers counts;
    if(complete)counts=rt_spot_capture::CountTableIdentifiers(bytes+offset,table.SizeInBytes,identifiers[0],identifiers[1]);
    const unsigned known[]={counts.group0,counts.group1};
    const unsigned empty=counts.empty,unknown=counts.unknown;
    const bool validated=complete && !unknown && (known[0]+known[1])>0;
    const unsigned report=reports.fetch_add(1);
    if(report<8 || validated) {
        Trace("rt_hit_table_dispatch generation=%llu mirror_generation=%llu same_list=%u coverage_complete=%u records=%llu group0=%u group1=%u empty=%u unknown=%u identifiers_validated=%u copy_bytes=%llu gpu_completion_confirmed=0 gpu_hit_capture=0",token,generation,unsigned(sameList),unsigned(complete),records,known[0],known[1],empty,unknown,unsigned(validated),copiedBytes.load());
        for(unsigned i=0;i<2;++i) {char hex[65]{};audit::Hex(identifiers[i],32,hex);Trace("rt_hit_table_identifier name=%ls bytes=%s",names[i],hex);}
    }
    if(validated) {
        const UINT64 indices[]={0ull,records>1?1ull:0ull,records/2,records-1};
        for(unsigned i=0;i<4;++i) {char hex[193]{};audit::Hex(bytes+offset+indices[i]*96,96,hex);Trace("rt_hit_table_record generation=%llu index=%llu bytes=%s",token,indices[i],hex);}
        done.store(true);
    }
    ReleaseSRWLockExclusive(&lock);
}
} // namespace rt_spot_table_audit
