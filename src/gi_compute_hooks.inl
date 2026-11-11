#pragma once
// Included inside native_lighting (or a test fixture) after PatchSlot.
namespace gi_compute_hooks {
using Microsoft::WRL::ComPtr;
// Rendering-state preservation for NRD must not require the legacy scene,
// descriptor inventory or shadow diagnostics. Latch for the process lifetime:
// toggling observation off mid-recording would leave an incomplete snapshot.
inline std::atomic<bool> denoiserBindingsRequired{false};
inline void EnableForDenoiser() noexcept {denoiserBindingsRequired.store(true,std::memory_order_release);}
inline bool Enabled() noexcept {
    if(denoiserBindingsRequired.load(std::memory_order_acquire))return true;
    static const bool enabled=[] {char value[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_BINDINGS",value,4)==1 && value[0]=='1';}();
    return enabled && gi_reuse::runtime::Enabled();
}
inline std::atomic<bool> failed{false};
inline std::atomic<unsigned> liveStates{0};
inline thread_local unsigned muted=0;
using RecordingResetObserver=void (*)(ID3D12CommandList*,UINT64)noexcept;
inline std::atomic<RecordingResetObserver> onRecordingReset{nullptr};
struct Mute {Mute() noexcept {++muted;} ~Mute(){--muted;} Mute(const Mute&)=delete; Mute& operator=(const Mute&)=delete;};
inline const GUID stateKey={0xa574c93e,0x3d21,0x46df,{0x95,0x42,0xb7,0x69,0x3e,0x2a,0x01,0x21}};
class State final : public IUnknown {
    std::atomic<ULONG> refs{1};
public:
    gi_reuse::ComputeBindingTracker tracker;
    gi_reuse::GraphicsTableState graphics;
    UINT64 generation=0,recordingGeneration=0;
    State() noexcept {tracker.Invalidate();}
    ~State(){--liveStates;}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;
        *out=static_cast<IUnknown*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override {const auto n=--refs;if(!n)delete this;return n;}
};
inline ComPtr<State> Read(ID3D12Object* list) noexcept {
    ComPtr<State> result;if(!list)return result;
    IUnknown* object=nullptr;UINT bytes=sizeof(object);
    if(SUCCEEDED(list->GetPrivateData(stateKey,&bytes,&object)) && object) {
        if(bytes==sizeof(object))result.Attach(static_cast<State*>(object));else object->Release();
    }
    return result;
}
inline bool Attach(ID3D12GraphicsCommandList* list) noexcept {
    if(Read(list))return true;
    if(liveStates.fetch_add(1)>=4096){--liveStates;failed=true;return false;}
    auto* state=new(std::nothrow) State;
    if(!state){--liveStates;failed=true;return false;}
    const auto hr=list->SetPrivateDataInterface(stateKey,state);state->Release();
    if(FAILED(hr)){failed=true;return false;}return true;
}
template<class F> inline void Change(ID3D12GraphicsCommandList* list,F&& fn) noexcept {
    if(muted || failed.load())return;
    if(auto state=Read(list))fn(*state.Get());
}
// Binding generation also advances on ClearState. Recording identity advances
// ONLY at creation / successful Reset: ClearState does not erase GPU commands.
inline void Begin(ID3D12GraphicsCommandList* list,ID3D12PipelineState* initial,bool newRecording=true) noexcept {
    if(!Enabled())return;
    Change(list,[&](State& s){
        if(s.generation==UINT64_MAX||(newRecording&&s.recordingGeneration==UINT64_MAX)){
            failed=true;s.tracker.Invalidate();return;
        }
        if(newRecording){
            if(s.recordingGeneration)if(const auto observer=onRecordingReset.load(std::memory_order_acquire))observer(list,s.recordingGeneration);
            ++s.recordingGeneration;
        }
        ++s.generation;s.tracker.Reset(initial);s.graphics.Reset();
    });
}
inline UINT64 RecordingGeneration(ID3D12CommandList* list)noexcept {
    if(!Enabled()||failed.load())return 0;
    auto state=Read(list);return state?state->recordingGeneration:0;
}
inline bool Capture(ID3D12GraphicsCommandList* list,gi_reuse::ComputeBindingSnapshot& out,UINT64& generation) noexcept {
    out={};generation=0;if(!Enabled() || failed.load())return false;
    auto state=Read(list);if(!state || !state->generation)return false;
    if(!state->tracker.Capture(out))return false;generation=state->generation;return true;
}
inline bool CaptureGraphics(ID3D12GraphicsCommandList* list,gi_reuse::GraphicsTableState& out) noexcept {
    out={};if(!Enabled()||failed.load())return false;
    auto state=Read(list);if(!state||!state->generation||!state->graphics.known)return false;
    out=state->graphics;return true;
}
using ResetFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso);
using ClearFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState* pso);
using PipelineFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState* pso);
using BundleFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12GraphicsCommandList* bundle);
using HeapsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT count,ID3D12DescriptorHeap* const* heaps);
using RootFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature* root);
using TableFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE handle);
using ConstantFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT index,UINT value,UINT offset);
using ConstantsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT index,UINT count,const void* values,UINT offset);
using CbvFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT index,D3D12_GPU_VIRTUAL_ADDRESS address);
using SrvFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT index,D3D12_GPU_VIRTUAL_ADDRESS address);
using UavFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT index,D3D12_GPU_VIRTUAL_ADDRESS address);
using IndirectFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandSignature* signature,UINT count,ID3D12Resource* arguments,UINT64 offset,ID3D12Resource* counts,UINT64 countOffset);
struct Hooks {void** table=nullptr;
    ResetFn reset=nullptr;
    ClearFn clear=nullptr;
    PipelineFn pipeline=nullptr;
    BundleFn bundle=nullptr;
    HeapsFn heaps=nullptr;
    RootFn root=nullptr,graphicsRoot=nullptr;
    TableFn tableBinding=nullptr,graphicsTable=nullptr;
    ConstantFn constant=nullptr;
    ConstantsFn constants=nullptr;
    CbvFn cbv=nullptr;
    SrvFn srv=nullptr;
    UavFn uav=nullptr;
    IndirectFn indirect=nullptr;
};
inline std::array<Hooks,8> hooks{};inline SRWLOCK lock=SRWLOCK_INIT;
inline Hooks Lookup(ID3D12GraphicsCommandList* list) noexcept {
    const auto table=*reinterpret_cast<void***>(list);Hooks result{};AcquireSRWLockShared(&lock);
    for(const auto& h:hooks)if(h.table==table){result=h;break;}ReleaseSRWLockShared(&lock);return result;
}
inline HRESULT STDMETHODCALLTYPE ResetHook(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso) {
    const auto result=Lookup(list).reset(list,allocator,pso);
    if(SUCCEEDED(result))Begin(list,pso);
    return result;
}
inline void STDMETHODCALLTYPE ClearHook(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) {
    Lookup(list).clear(list,pso);
    Begin(list,pso,false);
}
inline void STDMETHODCALLTYPE PipelineHook(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) {
    Lookup(list).pipeline(list,pso);
    Change(list,[&](State& s){s.tracker.Pipeline(pso);});
}
inline void STDMETHODCALLTYPE BundleHook(ID3D12GraphicsCommandList* list,ID3D12GraphicsCommandList* bundle) {
    Lookup(list).bundle(list,bundle);
    Change(list,[](State& s){s.tracker.Invalidate();s.graphics.Invalidate();});
}
inline void STDMETHODCALLTYPE HeapsHook(ID3D12GraphicsCommandList* list,UINT count,ID3D12DescriptorHeap* const* heaps) {
    Lookup(list).heaps(list,count,heaps);
    Change(list,[&](State& s){if(s.tracker.Heaps(count,heaps))s.graphics.HeapChange();});
}
inline void STDMETHODCALLTYPE RootHook(ID3D12GraphicsCommandList* list,ID3D12RootSignature* root) {
    Lookup(list).root(list,root);
    Change(list,[&](State& s){s.tracker.Root(root);});
}
inline void STDMETHODCALLTYPE TableHook(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE handle) {
    Lookup(list).tableBinding(list,index,handle);
    Change(list,[&](State& s){s.tracker.Descriptor(index,gi_reuse::RootArgumentKind::Table,handle.ptr);});
}
inline void STDMETHODCALLTYPE GraphicsRootHook(ID3D12GraphicsCommandList* list,ID3D12RootSignature* root) {
    Lookup(list).graphicsRoot(list,root);
    Change(list,[&](State& s){s.graphics.Root(reinterpret_cast<UINT64>(root));});
}
inline void STDMETHODCALLTYPE GraphicsTableHook(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE handle) {
    Lookup(list).graphicsTable(list,index,handle);
    Change(list,[&](State& s){s.graphics.Table(index,handle.ptr);});
}
inline void STDMETHODCALLTYPE ConstantHook(ID3D12GraphicsCommandList* list,UINT index,UINT value,UINT offset) {
    Lookup(list).constant(list,index,value,offset);
    Change(list,[&](State& s){s.tracker.Constants(index,offset,1,&value);});
}
inline void STDMETHODCALLTYPE ConstantsHook(ID3D12GraphicsCommandList* list,UINT index,UINT count,const void* values,UINT offset) {
    Lookup(list).constants(list,index,count,values,offset);
    Change(list,[&](State& s){s.tracker.Constants(index,offset,count,static_cast<const UINT*>(values));});
}
inline void STDMETHODCALLTYPE CbvHook(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_VIRTUAL_ADDRESS address) {
    Lookup(list).cbv(list,index,address);
    Change(list,[&](State& s){s.tracker.Descriptor(index,gi_reuse::RootArgumentKind::CBV,address);});
}
inline void STDMETHODCALLTYPE SrvHook(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_VIRTUAL_ADDRESS address) {
    Lookup(list).srv(list,index,address);
    Change(list,[&](State& s){s.tracker.Descriptor(index,gi_reuse::RootArgumentKind::SRV,address);});
}
inline void STDMETHODCALLTYPE UavHook(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_VIRTUAL_ADDRESS address) {
    Lookup(list).uav(list,index,address);
    Change(list,[&](State& s){s.tracker.Descriptor(index,gi_reuse::RootArgumentKind::UAV,address);});
}
inline void STDMETHODCALLTYPE IndirectHook(ID3D12GraphicsCommandList* list,ID3D12CommandSignature* signature,UINT count,ID3D12Resource* arguments,UINT64 offset,ID3D12Resource* counts,UINT64 countOffset) {
    Lookup(list).indirect(list,signature,count,arguments,offset,counts,countOffset);
    Change(list,[](State& s){s.tracker.Invalidate();s.graphics.Invalidate();});
}
using RaysFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,ID3D12StateObject*);
struct RayHooks {void** table=nullptr;RaysFn rays=nullptr;};
inline std::array<RayHooks,8> rayHooks{};
inline void STDMETHODCALLTYPE RaysHook(ID3D12GraphicsCommandList4* list,ID3D12StateObject* rays) {
    const auto table=*reinterpret_cast<void***>(list);RaysFn original=nullptr;
    AcquireSRWLockShared(&lock);for(const auto& h:rayHooks)if(h.table==table){original=h.rays;break;}ReleaseSRWLockShared(&lock);
    original(list,rays);Change(list,[&](State& s){s.tracker.Rays(rays);});
}
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
    if(!Enabled() || !list || failed.load())return;
    if(!Attach(list))return;
    ComPtr<ID3D12GraphicsCommandList4> ray;
    if(FAILED(list->QueryInterface(IID_PPV_ARGS(&ray)))){failed=true;return;}
    const auto baseTable=*reinterpret_cast<void***>(list);
    const auto rayTable=*reinterpret_cast<void***>(ray.Get());
    AcquireSRWLockExclusive(&lock);
    void** tables[]={baseTable,rayTable};
    for(auto table:tables) {
    Hooks* available=nullptr;bool exists=false;
    for(auto& h:hooks){if(h.table==table)exists=true;if(!h.table && !available)available=&h;}
    if(!exists && !available)failed=true;
    if(!exists && available) {
        available->table=table;
        available->reset=reinterpret_cast<ResetFn>(table[10]);
        available->clear=reinterpret_cast<ClearFn>(table[11]);
        available->pipeline=reinterpret_cast<PipelineFn>(table[25]);
        available->bundle=reinterpret_cast<BundleFn>(table[27]);
        available->heaps=reinterpret_cast<HeapsFn>(table[28]);
        available->root=reinterpret_cast<RootFn>(table[29]);
        available->graphicsRoot=reinterpret_cast<RootFn>(table[30]);
        available->graphicsTable=reinterpret_cast<TableFn>(table[32]);
        available->tableBinding=reinterpret_cast<TableFn>(table[31]);
        available->constant=reinterpret_cast<ConstantFn>(table[33]);
        available->constants=reinterpret_cast<ConstantsFn>(table[35]);
        available->cbv=reinterpret_cast<CbvFn>(table[37]);
        available->srv=reinterpret_cast<SrvFn>(table[39]);
        available->uav=reinterpret_cast<UavFn>(table[41]);
        available->indirect=reinterpret_cast<IndirectFn>(table[59]);
        if(!PatchSlot(&table[10],reinterpret_cast<void*>(available->reset),reinterpret_cast<void*>(&ResetHook)))failed=true;
        if(!PatchSlot(&table[11],reinterpret_cast<void*>(available->clear),reinterpret_cast<void*>(&ClearHook)))failed=true;
        if(!PatchSlot(&table[25],reinterpret_cast<void*>(available->pipeline),reinterpret_cast<void*>(&PipelineHook)))failed=true;
        if(!PatchSlot(&table[27],reinterpret_cast<void*>(available->bundle),reinterpret_cast<void*>(&BundleHook)))failed=true;
        if(!PatchSlot(&table[28],reinterpret_cast<void*>(available->heaps),reinterpret_cast<void*>(&HeapsHook)))failed=true;
        if(!PatchSlot(&table[30],reinterpret_cast<void*>(available->graphicsRoot),reinterpret_cast<void*>(&GraphicsRootHook)))failed=true;
        if(!PatchSlot(&table[32],reinterpret_cast<void*>(available->graphicsTable),reinterpret_cast<void*>(&GraphicsTableHook)))failed=true;
        if(!PatchSlot(&table[29],reinterpret_cast<void*>(available->root),reinterpret_cast<void*>(&RootHook)))failed=true;
        if(!PatchSlot(&table[31],reinterpret_cast<void*>(available->tableBinding),reinterpret_cast<void*>(&TableHook)))failed=true;
        if(!PatchSlot(&table[33],reinterpret_cast<void*>(available->constant),reinterpret_cast<void*>(&ConstantHook)))failed=true;
        if(!PatchSlot(&table[35],reinterpret_cast<void*>(available->constants),reinterpret_cast<void*>(&ConstantsHook)))failed=true;
        if(!PatchSlot(&table[37],reinterpret_cast<void*>(available->cbv),reinterpret_cast<void*>(&CbvHook)))failed=true;
        if(!PatchSlot(&table[39],reinterpret_cast<void*>(available->srv),reinterpret_cast<void*>(&SrvHook)))failed=true;
        if(!PatchSlot(&table[41],reinterpret_cast<void*>(available->uav),reinterpret_cast<void*>(&UavHook)))failed=true;
        if(!PatchSlot(&table[59],reinterpret_cast<void*>(available->indirect),reinterpret_cast<void*>(&IndirectHook)))failed=true;
    }
    }
    RayHooks* availableRay=nullptr;bool exists=false;
    for(auto& h:rayHooks){if(h.table==rayTable)exists=true;if(!h.table && !availableRay)availableRay=&h;}
    if(!exists && !availableRay)failed=true;
    if(!exists && availableRay){*availableRay={rayTable,reinterpret_cast<RaysFn>(rayTable[75])};
        if(!PatchSlot(&rayTable[75],reinterpret_cast<void*>(availableRay->rays),reinterpret_cast<void*>(&RaysHook)))failed=true;}
    ReleaseSRWLockExclusive(&lock);
}
} // namespace gi_compute_hooks
