#pragma once
// Included inside native_lighting after rt_lights_pipeline_runtime.h.
namespace rt_lights_runtime {
struct Context {
    void* manager = nullptr;
    ID3D12GraphicsCommandList* list = nullptr;
    ID3D12GraphicsCommandList4* rays = nullptr;
    ID3D12CommandQueue* queue = nullptr;
    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
    D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
};
inline bool ReadContext(Context& out) noexcept {
    out = {};
    if (!verifiedD3d || !device) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(verifiedD3d);
        const DWORD index = *reinterpret_cast<DWORD*>(base + 0x1115fc);
        auto** vector = reinterpret_cast<unsigned char**>(__readgsqword(0x58));
        auto* tls = vector[index]; if (!tls) return false;
        auto* manager = *reinterpret_cast<unsigned char**>(tls + 0x20);
        if (!manager) manager = *reinterpret_cast<unsigned char**>(base + 0x111c10);
        auto* context = *reinterpret_cast<unsigned char**>(tls + 8);
        if (!context) context = *reinterpret_cast<unsigned char**>(base + 0x111c18);
        auto* queue = *reinterpret_cast<unsigned char**>(tls + 0x18);
        if (!queue) queue = *reinterpret_cast<unsigned char**>(base + 0x111c38);
        if (!manager || !context || !queue) return false;
        auto* provider = *reinterpret_cast<unsigned char**>(manager + 0x50);
        if (!provider) return false;
        out.list = *reinterpret_cast<ID3D12GraphicsCommandList**>(context + 0x18);
        out.rays = *reinterpret_cast<ID3D12GraphicsCommandList4**>(context + 0x20);
        out.queue = *reinterpret_cast<ID3D12CommandQueue**>(queue + 0x10);
        out.heap = *reinterpret_cast<ID3D12DescriptorHeap**>(provider);
        out.gpu = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(manager + 0x30);
        const UINT64 gpuBase = *reinterpret_cast<UINT64*>(provider + 0x10);
        const UINT step = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        if (!out.list || !out.rays || !out.queue || !out.heap || out.gpu.ptr < gpuBase || !step || (out.gpu.ptr - gpuBase) % step) return false;
        const UINT64 at = (out.gpu.ptr - gpuBase) / step;
        const auto desc = out.heap->GetDesc();
        if (at > desc.NumDescriptors || 49 > desc.NumDescriptors - at) return false;
        out.cpu = out.heap->GetCPUDescriptorHandleForHeapStart(); out.cpu.ptr += SIZE_T(at) * step;
        out.manager = manager;
        return out.list->GetType() == D3D12_COMMAND_LIST_TYPE_DIRECT;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool AllocateTable(void* pointer, ID3D12DescriptorHeap* expected, rt_lights::DescriptorTable& out) noexcept {
    out = {}; const auto* context = static_cast<const Context*>(pointer);
    __try {
        unsigned char* provider = nullptr;
        using Allocate = void (*)(void*,UINT64,D3D12_GPU_DESCRIPTOR_HANDLE*,D3D12_CPU_DESCRIPTOR_HANDLE*,unsigned char**);
        auto allocate = reinterpret_cast<Allocate>(reinterpret_cast<unsigned char*>(verifiedD3d) + 0x1aa00);
        allocate(context->manager,49,&out.gpu,&out.cpu,&provider);
        if (!provider || *reinterpret_cast<ID3D12DescriptorHeap**>(provider) != expected) return false;
        const UINT64 base = *reinterpret_cast<UINT64*>(provider + 0x10);
        const UINT step = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const auto desc = expected->GetDesc();
        if (out.gpu.ptr < base || (out.gpu.ptr - base) % step) return false;
        const UINT64 index = (out.gpu.ptr - base) / step;
        return index >= 0x3b920 && index <= desc.NumDescriptors && 49 <= desc.NumDescriptors - index;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline rt_lights::CommandBindings* State(ID3D12GraphicsCommandList* list) noexcept {
    return rt_lights::CommandBindings::Get(list);
}
struct Pass {
    UINT64 selection = 0, descriptorEpoch = 0, observed = 0;
    bool requested = false, base = false, rays = false, committed = false;
    Context context;
    ID3D12GraphicsCommandList* mirrorList = nullptr;
    bool handoff = false;
    rt_lights::FramePool::Slot* slot = nullptr;
    rt_lights::NativeBindings producer, ray, selected;
    const char* reason = "awaiting_producer";
    UINT producerDispatches = 0, selectedDispatches = 0;
    ComPtr<ID3D12StateObject> currentState, state;
    ComPtr<ID3D12Resource> record;
    D3D12_DISPATCH_RAYS_DESC dispatch{};
};
inline thread_local Pass pass;
inline thread_local unsigned passDepth = 0;
inline thread_local bool customRecording = false;
inline std::atomic<UINT> passReports{0}, selectedOutsidePass{0}, selectedOtherList{0};
using Execute = void (STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList*const*);
inline Execute originalExecute = nullptr;
inline ComPtr<ID3D12CommandQueue> submittedQueue, nativeQueue;
using QueueResolver = HRESULT (*)(ID3D12CommandQueue*, ID3D12CommandQueue**) noexcept;
inline QueueResolver resolveQueue = nullptr;
inline SRWLOCK queueLock = SRWLOCK_INIT;
inline void STDMETHODCALLTYPE ExecuteHook(ID3D12CommandQueue* queue, UINT count, ID3D12CommandList*const* lists) {
    originalExecute(queue,count,lists);
    if (queue == submittedQueue.Get() && !pool.Submitted(nativeQueue.Get(),count,lists)) {
        runtimeFailed.store(true); readyMask.fetch_and(~kRtAllSpotReadyBit);
        Trace("rt_all_submission_failed resources_quarantined=1");
    }
}
inline bool PrepareQueue(ID3D12CommandQueue* queue) noexcept {
    AcquireSRWLockExclusive(&queueLock);
    if (!submittedQueue) {
        if (resolveQueue) resolveQueue(queue,nativeQueue.GetAddressOf());
        ComPtr<ID3D12Device> queueDevice;
        if (nativeQueue && (FAILED(nativeQueue->GetDevice(IID_PPV_ARGS(&queueDevice))) || queueDevice.Get() != device.Get()))
            nativeQueue.Reset();
        if (nativeQueue) {
            auto** table = *reinterpret_cast<void***>(queue);
            originalExecute = reinterpret_cast<Execute>(table[10]);
            if (PatchSlot(&table[10],reinterpret_cast<void*>(originalExecute),reinterpret_cast<void*>(&ExecuteHook))) submittedQueue = queue;
        }
    }
    const bool ready = submittedQueue.Get() == queue;
    ReleaseSRWLockExclusive(&queueLock);
    return ready;
}
inline void BeginPass() noexcept {
    if (++passDepth != 1) return;
    pass = Pass{};
    pass.selection = selection.Read();
    pass.requested = LiveSelection::GetMode(pass.selection) == Mode::RtAllSpot && pipelinesReady.load() && rayReady.load() && !runtimeFailed.load();
    if (!pass.requested || !ReadContext(pass.context) || !PrepareQueue(pass.context.queue)) return;
    const auto extent = requestedExtent.load();
    pass.slot = pool.Acquire(pass.context.list,static_cast<UINT>(extent >> 32),static_cast<UINT>(extent));
}
inline void EndPass() noexcept {
    if (!passDepth || --passDepth) return;
    if (pass.committed) selection.Observe(pass.selection,kDirectBit);
    if (pass.slot) pool.ReleaseRecording(pass.slot);
    if (pass.requested && (passReports.fetch_add(1) < 12 || !pass.committed && passReports.load() % 300 == 0))
        Trace("rt_all_pass slot=%u base=%u rays=%u committed=%u descriptor_mask=%llx gpu_confirmed=0",
            unsigned(pass.slot != nullptr),unsigned(pass.base),unsigned(pass.rays),unsigned(pass.committed),pass.observed);
    if (pass.requested && (passReports.load() <= 12 || (!pass.committed && passReports.load() % 300 == 0)))
        Trace("rt_all_result reason=%s producer_dispatches=%u selected_dispatches=%u producer_mask=%llx ray_mask=%llx selected_mask=%llx ray_materials=%u selected_cb1=%u selected_outside_pass=%u selected_other_list=%u handoff=%u",
            pass.reason,pass.producerDispatches,pass.selectedDispatches,pass.producer.observed,pass.ray.observed,pass.selected.observed,
            unsigned(pass.ray.table[7].ptr != 0),unsigned(pass.selected.constants[1] != 0),selectedOutsidePass.load(),selectedOtherList.load(),unsigned(pass.handoff));
    pass = Pass{};
}
inline void ObservePso(ID3D12GraphicsCommandList* list, ID3D12PipelineState* pipeline) noexcept {
    if (customRecording) return;
    if (auto* state = State(list)) state->bindings.pipeline = pipeline;
}
inline void ResetSucceeded(ID3D12GraphicsCommandList* list) noexcept {
    pool.ResetSucceeded(list);
    if (passDepth && pass.mirrorList == list) { pass.mirrorList = nullptr; pass.descriptorEpoch = pass.observed = 0; }
    if (auto* state = State(list)) state->bindings = {};
}
inline bool Snapshot(const Context& context, UINT index, rt_lights::NativeBindings& out) noexcept {
    auto* state = State(context.list);
    if (!pass.slot || !state || context.queue != pass.context.queue || context.heap != pass.context.heap ||
        (context.list != pass.context.list && !(index == 3 && pass.base && pass.rays)) ||
        pass.mirrorList != context.list ||
        state->bindings.table[0].ptr != context.gpu.ptr || pass.descriptorEpoch != context.gpu.ptr) return false;
    out = state->bindings; out.heap = context.heap; out.observed = pass.observed;
    out.snapshot = pass.slot->resources->Snapshot(index);
    device->CopyDescriptorsSimple(49,out.snapshot,pass.slot->resources->Snapshot(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return true;
}
inline bool MirrorDestination(D3D12_CPU_DESCRIPTOR_HANDLE destination, UINT count, UINT& index, UINT& skip, UINT& copied) noexcept {
    if (customRecording || !passDepth || !pass.slot || !count) return false;
    Context context;
    if (!ReadContext(context) || context.queue != pass.context.queue || context.heap != pass.context.heap ||
        (context.list != pass.context.list && !(pass.base && pass.rays))) return false;
    const UINT step = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const auto overlap = rt_lights::DescriptorOverlap(context.cpu.ptr,49,destination.ptr,count,step);
    if (!overlap.count) return false;
    index = overlap.tableIndex; skip = overlap.sourceSkip; copied = overlap.count;
    if (pass.mirrorList != context.list || pass.descriptorEpoch != context.gpu.ptr) {
        pass.mirrorList = context.list; pass.descriptorEpoch = context.gpu.ptr; pass.observed = 0;
    }
    return true;
}
using CopySimple = void (STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_DESCRIPTOR_HEAP_TYPE);
using CreateCbv = void (STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
inline CopySimple originalCopy = nullptr;
inline CreateCbv originalCbv = nullptr;
inline void STDMETHODCALLTYPE CopyHook(ID3D12Device* owner, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE to, D3D12_CPU_DESCRIPTOR_HANDLE from, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    originalCopy(owner,count,to,from,type);
    UINT index = 0, skip = 0, copied = 0;
    if (type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || !MirrorDestination(to,count,index,skip,copied)) return;
    const UINT step = owner->GetDescriptorHandleIncrementSize(type);
    auto destination = pass.slot->resources->Snapshot(0);
    destination.ptr += SIZE_T(index) * step; from.ptr += SIZE_T(skip) * step;
    originalCopy(owner,copied,destination,from,type);
    pass.observed |= ((1ull << copied) - 1) << index;
}
using CopyRanges = void (STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,D3D12_DESCRIPTOR_HEAP_TYPE);
using CreateSrv = void (STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateUav = void (STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
inline CopyRanges originalRanges = nullptr;
inline CreateSrv originalSrv = nullptr;
inline CreateUav originalUav = nullptr;
inline void STDMETHODCALLTYPE RangesHook(ID3D12Device* owner,UINT nd,const D3D12_CPU_DESCRIPTOR_HANDLE* dst,const UINT* dc,
    UINT ns,const D3D12_CPU_DESCRIPTOR_HANDLE* src,const UINT* sc,D3D12_DESCRIPTOR_HEAP_TYPE type) {
    originalRanges(owner,nd,dst,dc,ns,src,sc,type);
    if (customRecording || !passDepth || !pass.slot || type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) return;
    const UINT step = owner->GetDescriptorHandleIncrementSize(type);
    UINT si = 0, so = 0;
    for (UINT di = 0; di < nd; ++di) {
        UINT offset = 0, remaining = dc ? dc[di] : 1;
        while (remaining && si < ns) {
            const UINT sourceCount = sc ? sc[si] : 1;
            if (so == sourceCount) { ++si; so = 0; continue; }
            const UINT chunk = (std::min)(remaining,sourceCount-so);
            auto to = dst[di]; to.ptr += SIZE_T(offset)*step;
            auto from = src[si]; from.ptr += SIZE_T(so)*step;
            UINT index = 0, skip = 0, copied = 0;
            if (MirrorDestination(to,chunk,index,skip,copied)) {
                auto shadow = pass.slot->resources->Snapshot(0); shadow.ptr += SIZE_T(index)*step;
                from.ptr += SIZE_T(skip)*step;
                originalCopy(owner,copied,shadow,from,type);
                pass.observed |= ((1ull << copied)-1) << index;
            }
            offset += chunk; remaining -= chunk; so += chunk;
        }
    }
}
inline bool ShadowView(ID3D12Device* owner,D3D12_CPU_DESCRIPTOR_HANDLE& to,UINT& index) noexcept {
    UINT skip = 0, copied = 0;
    if (!MirrorDestination(to,1,index,skip,copied)) return false;
    to = pass.slot->resources->Snapshot(0);
    to.ptr += SIZE_T(index)*owner->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return true;
}
inline void STDMETHODCALLTYPE SrvHook(ID3D12Device* owner,ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE to) {
    originalSrv(owner,resource,desc,to);
    UINT index = 0;
    if (ShadowView(owner,to,index)) { originalSrv(owner,resource,desc,to); pass.observed |= 1ull << index; }
}
inline void STDMETHODCALLTYPE UavHook(ID3D12Device* owner,ID3D12Resource* resource,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE to) {
    originalUav(owner,resource,counter,desc,to);
    UINT index = 0;
    if (ShadowView(owner,to,index)) { originalUav(owner,resource,counter,desc,to); pass.observed |= 1ull << index; }
}
inline void STDMETHODCALLTYPE CbvHook(ID3D12Device* owner, const D3D12_CONSTANT_BUFFER_VIEW_DESC* cb, D3D12_CPU_DESCRIPTOR_HANDLE to) {
    originalCbv(owner,cb,to);
    UINT index = 0;
    if (cb && ShadowView(owner,to,index)) { originalCbv(owner,cb,to); pass.observed |= 1ull << index; }
}
using DispatchCompute = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
using SetRoot = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
using SetTable = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE);
using SetCbv = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_VIRTUAL_ADDRESS);
struct Hooks { void** table = nullptr; DispatchCompute dispatch = nullptr; SetRoot root = nullptr; SetTable tableBinding = nullptr; SetCbv cbv = nullptr; };
inline Hooks hooks[16];
inline SRWLOCK listLock = SRWLOCK_INIT, deviceLock = SRWLOCK_INIT;
inline Hooks Lookup(ID3D12GraphicsCommandList* list) noexcept {
    Hooks result; auto** table = *reinterpret_cast<void***>(list);
    AcquireSRWLockShared(&listLock);
    for (const auto& item : hooks) if (item.table == table) { result = item; break; }
    ReleaseSRWLockShared(&listLock);
    return result;
}
inline void STDMETHODCALLTYPE RootHook(ID3D12GraphicsCommandList* list, ID3D12RootSignature* value) {
    Lookup(list).root(list,value);
    if (customRecording) return;
    if (auto* state = State(list)) {
        if (state->bindings.root != value) {
            auto* pipeline = state->bindings.pipeline; state->bindings = {}; state->bindings.pipeline = pipeline;
        }
        state->bindings.root = value;
    }
}
inline void STDMETHODCALLTYPE TableHook(ID3D12GraphicsCommandList* list, UINT index, D3D12_GPU_DESCRIPTOR_HANDLE value) {
    Lookup(list).tableBinding(list,index,value);
    if (!customRecording && index < 8) if (auto* state = State(list)) state->bindings.table[index] = value;
}
inline void STDMETHODCALLTYPE CbvBindingHook(ID3D12GraphicsCommandList* list, UINT index, D3D12_GPU_VIRTUAL_ADDRESS value) {
    Lookup(list).cbv(list,index,value);
    if (!customRecording && index >= 2 && index < 6) if (auto* state = State(list)) state->bindings.constants[index - 2] = value;
}
inline void STDMETHODCALLTYPE DispatchHook(ID3D12GraphicsCommandList* list, UINT x, UINT y, UINT z) {
    const auto original = Lookup(list).dispatch;
    original(list,x,y,z);
    if (customRecording) return;
    auto* tracked = State(list); if (!tracked || !tracked->bindings.pipeline) return;
    UINT stage = 0, size = sizeof(stage);
    if (FAILED(tracked->bindings.pipeline->GetPrivateData(kStage,&size,&stage))) return;
    if (!passDepth) { if (stage == 2) selectedOutsidePass.fetch_add(1); return; }
    if (!pass.slot) return;
    if (list != pass.context.list && stage != 2) return;
    if (list != pass.context.list) selectedOtherList.fetch_add(1);
    Context context;
    if (!ReadContext(context) || context.list != list) { pass.reason = "dispatch_context"; return; }
    if (context.queue != pass.context.queue) { pass.reason = "handoff_queue_changed"; return; }
    customRecording = true;
    const rt_lights::TableAllocator allocator{&context,&AllocateTable,device.Get(),device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)};
    if (stage == 1) {
        ++pass.producerDispatches; pass.reason = "producer_snapshot";
        if (!Snapshot(context,1,pass.producer)) { customRecording = false; return; }
        pass.base = rt_lights::FrameRecorder::ReplayBase(list,*pass.slot->resources,pass.producer,Pipelines(),allocator,x,y,z);
        pass.reason = pass.base ? "awaiting_rays" : "producer_bindings_or_extent";
    } else if (stage == 2 && !pass.committed) {
        ++pass.selectedDispatches;
        pass.reason = !pass.base ? "missing_base" : !pass.rays ? "missing_rays" : z != 1 ? "selected_depth" : "selected_snapshot";
        auto& selected = pass.selected;
        if (pass.base && pass.rays && z == 1 && Snapshot(context,3,selected)) {
            if (!pool.AttachConsumer(pass.slot,list)) { pass.reason = "handoff_lease"; customRecording = false; return; }
            pass.handoff = pass.slot->consumer != nullptr;
            pass.committed = rt_lights::FrameRecorder::Replace(context.rays,*pass.slot->resources,pass.producer,pass.ray,selected,Pipelines(),allocator,
                pass.state.Get(),pass.dispatch,pass.record->GetGPUVirtualAddress(),static_cast<UINT>(pass.slot->lifetime.Generation()),
                pass.slot->tables.data(),static_cast<UINT>(pass.slot->tables.size()),&pass.reason);
        }
    }
    customRecording = false;
}
inline void OnState(ID3D12GraphicsCommandList4*, ID3D12StateObject* state) noexcept {
    if (passDepth && !customRecording) pass.currentState = state;
}
inline bool OnRays(ID3D12GraphicsCommandList4* list, const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
    if (!passDepth || customRecording || !desc || !pass.currentState) return false;
    ComPtr<ID3D12Resource> record; UINT bytes = sizeof(ID3D12Resource*);
    if (FAILED(pass.currentState->GetPrivateData(kRayRecord,&bytes,record.GetAddressOf())) || !record) return false;
    if (desc->Depth == 1 && desc->Width && desc->Height) requestedExtent.store((UINT64(desc->Width) << 32) | desc->Height);
    if (!pass.slot || !pass.base) return false;
    Context context;
    if (!ReadContext(context) || context.rays != list || context.list != pass.context.list) return false;
    customRecording = true;
    pass.rays = Snapshot(context,2,pass.ray);
    if (pass.rays) { pass.reason = "awaiting_selected"; pass.dispatch = *desc; pass.record = record; pass.state = pass.currentState; pass.slot->rayOwner = pass.state; }
    customRecording = false;
    return false; // Always run the untouched native rays for complete fallback.
}
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
    if (!rt_lights::CommandBindings::Attach(list)) {
        runtimeFailed.store(true); readyMask.fetch_and(~kRtAllSpotReadyBit); return;
    }
    auto** table = *reinterpret_cast<void***>(list);
    AcquireSRWLockExclusive(&listLock);
    Hooks* slot = nullptr;
    for (auto& item : hooks) {
        if (item.table == table) { ReleaseSRWLockExclusive(&listLock); return; }
        if (!item.table && !slot) slot = &item;
    }
    bool good = false;
    if (slot) {
        *slot = {table,reinterpret_cast<DispatchCompute>(table[14]),reinterpret_cast<SetRoot>(table[29]),reinterpret_cast<SetTable>(table[31]),reinterpret_cast<SetCbv>(table[37])};
        good = PatchSlot(&table[14],reinterpret_cast<void*>(slot->dispatch),reinterpret_cast<void*>(&DispatchHook)) &&
            PatchSlot(&table[29],reinterpret_cast<void*>(slot->root),reinterpret_cast<void*>(&RootHook)) &&
            PatchSlot(&table[31],reinterpret_cast<void*>(slot->tableBinding),reinterpret_cast<void*>(&TableHook)) &&
            PatchSlot(&table[37],reinterpret_cast<void*>(slot->cbv),reinterpret_cast<void*>(&CbvBindingHook));
    }
    ReleaseSRWLockExclusive(&listLock);
    if (!good) { runtimeFailed.store(true); readyMask.fetch_and(~kRtAllSpotReadyBit); }
}
inline void InstallDevice(ID3D12Device* owner) noexcept {
    AcquireSRWLockExclusive(&deviceLock);
    if (!originalCopy) {
        auto** table = *reinterpret_cast<void***>(owner);
        originalRanges = reinterpret_cast<CopyRanges>(table[23]);
        originalSrv = reinterpret_cast<CreateSrv>(table[18]); originalUav = reinterpret_cast<CreateUav>(table[19]);
        originalCopy = reinterpret_cast<CopySimple>(table[24]); originalCbv = reinterpret_cast<CreateCbv>(table[17]);
        const bool good = PatchSlot(&table[23],reinterpret_cast<void*>(originalRanges),reinterpret_cast<void*>(&RangesHook)) &&
            PatchSlot(&table[18],reinterpret_cast<void*>(originalSrv),reinterpret_cast<void*>(&SrvHook)) &&
            PatchSlot(&table[19],reinterpret_cast<void*>(originalUav),reinterpret_cast<void*>(&UavHook)) &&
            PatchSlot(&table[24],reinterpret_cast<void*>(originalCopy),reinterpret_cast<void*>(&CopyHook)) &&
            PatchSlot(&table[17],reinterpret_cast<void*>(originalCbv),reinterpret_cast<void*>(&CbvHook));
        if (!good) runtimeFailed.store(true);
    }
    ReleaseSRWLockExclusive(&deviceLock);
}
}
