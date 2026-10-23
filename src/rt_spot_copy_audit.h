#pragma once
// Included inside native_lighting. Observe existing copies only: the sampled
// upload bytes are copy-time evidence, never a claim of GPU completion or a
// coherent DispatchRays snapshot.
namespace rt_spot_copy_audit {
namespace audit = rt_spot_capture_audit;
using CopyBuffer = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12Resource*,UINT64,ID3D12Resource*,UINT64,UINT64);
using CopyResource = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12Resource*,ID3D12Resource*);
using Reset = HRESULT (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
struct Hooks { void** table = nullptr; CopyBuffer buffer = nullptr; CopyResource resource = nullptr; Reset reset = nullptr; };
inline Hooks hooks[16]{};
inline SRWLOCK hookLock = SRWLOCK_INIT;
inline std::atomic<unsigned> calls{0}, matches{0}, samples{0};
inline constexpr unsigned kMaximumReports = 4;
inline Hooks Lookup(ID3D12GraphicsCommandList* list) noexcept {
    Hooks found{};
    auto** table = *reinterpret_cast<void***>(list);
    AcquireSRWLockShared(&hookLock);
    for (const auto& entry : hooks) if (entry.table == table) { found = entry; break; }
    ReleaseSRWLockShared(&hookLock);
    return found;
}
inline void Observe(ID3D12Resource* destination, UINT64 destinationOffset,
    ID3D12Resource* source, UINT64 sourceOffset, UINT64 bytes) noexcept {
    if (!destination || !source || !audit::targetActive.load(std::memory_order_acquire) ||
        audit::captured.load(std::memory_order_relaxed) ||
        audit::attempts.load(std::memory_order_relaxed) >= audit::kMaximumAttempts ||
        matches.load(std::memory_order_relaxed) >= kMaximumReports) return;
    const unsigned call = ++calls;
    if (call == 1 || call % 65536 == 0) Trace("rt_hit_copy_progress calls=%u matches=%u samples=%u",call,matches.load(),samples.load());
    UINT64 target = 0, targetBytes = 0;
    if (!TryAcquireSRWLockShared(&audit::ownerLock)) return;
    target = audit::targetAddress; targetBytes = audit::targetBytes;
    ReleaseSRWLockShared(&audit::ownerLock);
    const auto destinationDesc = destination->GetDesc();
    if (destinationDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) return;
    const auto base = destination->GetGPUVirtualAddress();
    if (!rt_spot_capture::ContainsGpuRange(base,destinationDesc.Width,target,1)) return;
    const auto sourceDesc = source->GetDesc();
    if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || !bytes ||
        destinationOffset > destinationDesc.Width || bytes > destinationDesc.Width-destinationOffset ||
        sourceOffset > sourceDesc.Width || bytes > sourceDesc.Width-sourceOffset) return;
    const UINT64 tableOffset = target-base;
    if (tableOffset < destinationOffset || tableOffset-destinationOffset >= bytes) return;
    const UINT64 delta = tableOffset-destinationOffset;
    const UINT64 available = bytes-delta;
    const UINT64 sampleBytes = (available < targetBytes ? available : targetBytes)/96*96;
    const unsigned report = matches.fetch_add(1);
    if (report >= kMaximumReports) return;
    D3D12_HEAP_PROPERTIES destinationHeap{}, sourceHeap{}; D3D12_HEAP_FLAGS flags{};
    const HRESULT destinationHr = destination->GetHeapProperties(&destinationHeap,&flags);
    const HRESULT sourceHr = source->GetHeapProperties(&sourceHeap,&flags);
    Trace("rt_hit_copy report=%u target=%llu destination_base=%llu destination_bytes=%llu destination_heap=%u destination_heap_hr=0x%08lX source_bytes=%llu source_heap=%u source_heap_hr=0x%08lX copy_bytes=%llu source_offset=%llu destination_offset=%llu table_bytes=%llu covered_bytes=%llu gpu_completion_confirmed=0",
        report,target,base,destinationDesc.Width,unsigned(destinationHeap.Type),static_cast<unsigned long>(destinationHr),sourceDesc.Width,unsigned(sourceHeap.Type),static_cast<unsigned long>(sourceHr),bytes,sourceOffset,destinationOffset,targetBytes,sampleBytes);
    if (!sampleBytes || FAILED(sourceHr) || sourceHeap.Type != D3D12_HEAP_TYPE_UPLOAD) return;
    const UINT64 readOffset = sourceOffset+delta;
    D3D12_RANGE range{static_cast<SIZE_T>(readOffset),static_cast<SIZE_T>(readOffset+sampleBytes)};
    void* mapped = nullptr;
    const HRESULT mapHr = source->Map(0,&range,&mapped);
    if (FAILED(mapHr) || !mapped) { Trace("rt_hit_copy_map report=%u hr=0x%08lX",report,static_cast<unsigned long>(mapHr)); return; }
    const UINT64 count = sampleBytes/96;
    const UINT64 indices[] = {0ull,count > 1 ? 1ull : 0ull,count/2,count-1};
    unsigned char first[4][96]{}, second[4][96]{};
    const auto* data = static_cast<const unsigned char*>(mapped)+readOffset;
    for (UINT i=0;i<4;++i) std::memcpy(first[i],data+indices[i]*96,96);
    MemoryBarrier();
    for (UINT i=0;i<4;++i) std::memcpy(second[i],data+indices[i]*96,96);
    D3D12_RANGE written{0,0}; source->Unmap(0,&written);
    if (std::memcmp(first,second,sizeof(first))) { Trace("rt_hit_copy_sample report=%u stable=0",report); return; }
    ++samples;
    Trace("rt_hit_copy_sample report=%u stable=1 records=%llu full_previous_table=%u dispatch_snapshot=0 gpu_hit_capture=0",report,count,unsigned(sampleBytes==targetBytes));
    for (UINT i=0;i<4;++i) { char hex[193]{}; audit::Hex(first[i],96,hex); Trace("rt_hit_copy_record report=%u index=%llu bytes=%s",report,indices[i],hex); }
}
inline void STDMETHODCALLTYPE BufferHook(ID3D12GraphicsCommandList* list,ID3D12Resource* destination,UINT64 destinationOffset,ID3D12Resource* source,UINT64 sourceOffset,UINT64 bytes) {
    const auto original = Lookup(list).buffer;
    rt_spot_capture_runtime::ObserveTableOwner(destination);
    rt_spot_table_audit::Observe(list,destination,destinationOffset,source,sourceOffset,bytes);
    Observe(destination,destinationOffset,source,sourceOffset,bytes);
    original(list,destination,destinationOffset,source,sourceOffset,bytes);
}
inline void STDMETHODCALLTYPE ResourceHook(ID3D12GraphicsCommandList* list,ID3D12Resource* destination,ID3D12Resource* source) {
    const auto original = Lookup(list).resource;
    rt_spot_capture_runtime::ObserveTableOwner(destination);
    if (destination && source && rt_spot_table_audit::Enabled())
        rt_spot_table_audit::Observe(list,destination,0,source,0,destination->GetDesc().Width);
    // CopyResource permits textures; buffer inspection rejects them before use.
    if (destination && source && audit::targetActive.load(std::memory_order_acquire) &&
        matches.load(std::memory_order_relaxed) < kMaximumReports)
        Observe(destination,0,source,0,destination->GetDesc().Width);
    original(list,destination,source);
}
inline HRESULT STDMETHODCALLTYPE ResetHook(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pipeline) {
    const auto hr=Lookup(list).reset(list,allocator,pipeline);
    if(SUCCEEDED(hr)){rt_spot_capture_runtime::ResetObserved(list);rt_spot_table_audit::Token(list,true);}
    return hr;
}
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
    if (!list) return;
    auto** table = *reinterpret_cast<void***>(list);
    AcquireSRWLockExclusive(&hookLock);
    Hooks* slot = nullptr;
    for (auto& entry : hooks) {
        if (entry.table == table) { ReleaseSRWLockExclusive(&hookLock); return; }
        if (!entry.table && !slot) slot = &entry;
    }
    if (slot) {
        *slot = {table,reinterpret_cast<CopyBuffer>(table[15]),reinterpret_cast<CopyResource>(table[17]),reinterpret_cast<Reset>(table[10])};
        const bool buffer = PatchSlot(&table[15],reinterpret_cast<void*>(slot->buffer),reinterpret_cast<void*>(&BufferHook));
        const bool resource = PatchSlot(&table[17],reinterpret_cast<void*>(slot->resource),reinterpret_cast<void*>(&ResourceHook));
        const bool reset = PatchSlot(&table[10],reinterpret_cast<void*>(slot->reset),reinterpret_cast<void*>(&ResetHook));
        Trace("rt_hit_copy_hooks buffer=%u resource=%u reset=%u",unsigned(buffer),unsigned(resource),unsigned(reset));
        if (!reset) rt_spot_table_audit::resetTracking.store(false); // No reuse tracking means no safe correlation.
        if (!buffer && !resource && !reset) *slot = {};
    } else Trace("rt_hit_copy_hooks capacity_exhausted=1");
    ReleaseSRWLockExclusive(&hookLock);
}
} // namespace rt_spot_copy_audit
