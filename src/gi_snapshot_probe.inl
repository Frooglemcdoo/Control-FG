#pragma once
// Opt-in integration rehearsal. Owners contain CPU counters only; no native GPU
// command is inserted, no rendering buffer is copied and no GI output is changed.
namespace gi_snapshot_probe {
using Microsoft::WRL::ComPtr;
struct Owner {UINT64 generation=0;};
using Runtime=gi_reuse::D3D12SnapshotRetirement<Owner>;
inline Runtime* runtime=nullptr;
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::array<UINT64,256> records{};
inline std::atomic<UINT64> builds{0},bindingReady{0},bindingMissing{0},rayBindings{0},ordinaryBindings{0};
inline std::atomic<UINT64> admitted{0},completed{0},refused{0},signalsFailed{0},batchOverflow{0};
inline thread_local Runtime::Batch batch;
inline const GUID key={0x62507d7e,0x3d21,0x46df,{0x95,0x42,0xb7,0x69,0x3e,0x2a,0x01,0x31}};
inline bool Enabled() noexcept {return gi_compute_hooks::Enabled();}
class State final:public IUnknown {
    std::atomic<ULONG> refs{1};
public:
    Runtime::Recording recording;
    ~State(){AcquireSRWLockExclusive(&lock);if(runtime)runtime->Seal(recording);ReleaseSRWLockExclusive(&lock);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;
        *out=static_cast<IUnknown*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override {const auto n=--refs;if(!n)delete this;return n;}
};
inline ComPtr<State> Read(ID3D12Object* list) noexcept {
    ComPtr<State> result;if(!list)return result;IUnknown* object=nullptr;UINT bytes=sizeof(object);
    if(SUCCEEDED(list->GetPrivateData(key,&bytes,&object))&&object){
        if(bytes==sizeof(object))result.Attach(static_cast<State*>(object));else object->Release();
    }return result;
}
inline void Reset(ID3D12GraphicsCommandList* list) noexcept {
    if(Enabled()&&FAILED(list->SetPrivateDataInterface(key,nullptr)))++refused;
}
inline void Build(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*) noexcept {
    if(!Enabled())return;++builds;
    gi_reuse::ComputeBindingSnapshot bindings;UINT64 generation=0;
    if(gi_compute_hooks::Capture(list,bindings,generation)){
        ++bindingReady;if(bindings.rays)++rayBindings;else ++ordinaryBindings;
    }else ++bindingMissing;
    if(Read(list))return;
    const auto journal=gi_reuse::runtime::Read(list);
    if(!journal){++refused;return;}
    auto owner=std::unique_ptr<Owner>(new(std::nothrow) Owner{journal->data.generation});
    auto* state=new(std::nothrow) State;
    if(!state||!owner){delete state;++refused;return;}
    AcquireSRWLockExclusive(&lock);
    if(!runtime)runtime=new(std::nothrow) Runtime;
    if(runtime)for(auto& id:records)if(!id){
        id=runtime->Admit(owner->generation,sizeof(Owner),owner);
        state->recording={id,journal->data.generation};break;
    }
    ReleaseSRWLockExclusive(&lock);
    if(!state->recording.id){++refused;state->Release();return;}
    if(FAILED(list->SetPrivateDataInterface(key,state)))++refused;else ++admitted;
    state->Release();
}
inline void Poll() noexcept {
    std::array<std::unique_ptr<Owner>,256> retired;
    AcquireSRWLockExclusive(&lock);
    if(runtime){runtime->Poll();for(unsigned i=0;i<records.size();++i)if(records[i]){
        const bool published=runtime->View(records[i])!=nullptr;
        if(runtime->Take(records[i],retired[i])){if(published)++completed;records[i]=0;}
    }}
    ReleaseSRWLockExclusive(&lock);
    // Any eventual COM-backed owners must be destroyed here, outside host locks.
}
inline UINT64 Before(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    if(!Enabled())return 0;Poll();
    if(!queue||!lists||queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_COPY)return 0;
    std::array<Runtime::Recording,256> submitted{};unsigned n=0;
    for(UINT i=0;i<count;++i){auto state=Read(lists[i]);if(!state)continue;
        if(n==submitted.size()){++batchOverflow;return 0;}submitted[n++]=state->recording;
    }
    if(!n)return 0;
    AcquireSRWLockExclusive(&lock);if(runtime)batch=runtime->Before(queue,submitted.data(),n);ReleaseSRWLockExclusive(&lock);
    return batch.count?1:0;
}
inline void After(ID3D12CommandQueue*,UINT64 token) noexcept {
    if(!Enabled()||!token)return;
    AcquireSRWLockExclusive(&lock);if(runtime&&!runtime->After(batch))++signalsFailed;ReleaseSRWLockExclusive(&lock);
}
inline void Report() noexcept {
    if(!Enabled())return;
    unsigned active=0,quarantined=0;
    AcquireSRWLockExclusive(&lock);
    if(runtime)for(const auto id:records)if(id){++active;if(runtime->Quarantined(id))++quarantined;}
    ReleaseSRWLockExclusive(&lock);
    Trace("GI_LIFETIME_R295 active=%u quarantined=%u cpu_metadata_owners_only=1 publication_requires_reset_and_gpu_completion=1",active,quarantined);
    for(unsigned kind=0;kind<4;++kind){const auto& c=gi_reuse::runtime::inputAudit[kind];
        Trace("GI_INPUT_R295 kind=%u committed=%llu placed=%llu storage_unknown=%llu deny_srv=%llu range_rejected=%llu local_preflight_pass=%llu alias_verified=0 gpu_copies=0",kind,c[0].load(),c[1].load(),c[2].load(),c[3].load(),c[4].load(),c[5].load());
    }
    Trace("GI_INTEGRATION_R295 builds=%llu binding_ready=%llu binding_missing=%llu ordinary_pso=%llu ray_pso=%llu hook_failed=%u live_trackers=%u admitted_recordings=%llu completed_sealed_recordings=%llu admission_refused=%llu signal_failures=%llu batch_overflow=%llu gpu_copies=0 replacement_gi=0",builds.load(),bindingReady.load(),bindingMissing.load(),ordinaryBindings.load(),rayBindings.load(),unsigned(gi_compute_hooks::failed.load()),gi_compute_hooks::liveStates.load(),admitted.load(),completed.load(),refused.load(),signalsFailed.load(),batchOverflow.load());
}
}
