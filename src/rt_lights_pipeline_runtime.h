#pragma once
// Included inside native_lighting, after the native state-object observer.
namespace rt_lights_runtime {
using Microsoft::WRL::ComPtr;
inline const GUID kStage = {0x67cad101,0xc118,0x42e4,{0x91,0x30,0x04,0x20,0x26,0x10,0x22,0x01}};
inline const GUID kRayRecord = {0x67cad101,0xc118,0x42e4,{0x91,0x30,0x04,0x20,0x26,0x10,0x22,0x02}};
inline constexpr char kProducerSha[] = "a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f";
enum Asset : unsigned { Batch, Arguments, Pack, Compose, Commit, Visibility, Base, Point, Extract, AssetCount };
static_assert(AssetCount == sizeof(rt_lights::RuntimeAssets) / sizeof(rt_lights::RuntimeAssets[0]));
inline SRWLOCK pipelineLock = SRWLOCK_INIT;
inline std::array<std::vector<unsigned char>, AssetCount> assets;
inline std::array<ComPtr<ID3D12PipelineState>, AssetCount> compute;
inline ComPtr<ID3D12CommandSignature> indirectCompute, indirectRays;
inline ComPtr<ID3D12Device> device;
inline ComPtr<ID3D12RootSignature> root;
inline rt_lights::FramePool pool;
inline bool assetAttempted = false, assetsReady = false;
inline std::atomic<bool> pipelinesReady{false}, rayReady{false}, runtimeFailed{false};
inline std::atomic<UINT64> requestedExtent{0};
inline rt_lights::Pipelines Pipelines() noexcept {
    return {compute[Base].Get(),compute[Point].Get(),compute[Batch].Get(),compute[Arguments].Get(),
        compute[Extract].Get(),compute[Pack].Get(),compute[Compose].Get(),compute[Commit].Get(),indirectCompute.Get(),indirectRays.Get()};
}
inline void RefreshReady() noexcept {
    if (pipelinesReady.load() && rayReady.load() && !runtimeFailed.load()) readyMask.fetch_or(kRtAllSpotReadyBit);
}
inline void LoadAssets() noexcept {
    AcquireSRWLockExclusive(&pipelineLock);
    if (!assetAttempted) {
        assetAttempted = true; assetsReady = true;
        for (unsigned i = 0; i < AssetCount; ++i) {
            char sha[65]{}; bool loaded = false;
            const auto& asset = rt_lights::RuntimeAssets[i];
            if (!PTReflectionLoadSidecar(asset.file,"RT_ALL_SPOT",assets[i],sha,loaded) || strcmp(sha,asset.sha)) {
                assetsReady = false;
                Trace("rt_all_assets_failed file=%ls",asset.file);
                break;
            }
        }
    }
    ReleaseSRWLockExclusive(&pipelineLock);
}
inline void ObserveCompute(ID3D12Device* owner, const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,
                           IUnknown* native, CreateCompute original) noexcept {
    if (runtimeFailed.load() || !desc || !native || (desc->CS.BytecodeLength != 71144 && desc->CS.BytecodeLength != 50752)) return;
    unsigned stage = rt_spot_runtime::Matches(desc->CS,kProducerSha) ? 1u : rt_spot_runtime::Matches(desc->CS,rt_spot_runtime::kLightSha) ? 2u : 0u;
    if (!stage) return;
    LoadAssets();
    AcquireSRWLockExclusive(&pipelineLock);
    HRESULT hr = assetsReady ? S_OK : E_FAIL;
    if (SUCCEEDED(hr) && !pipelinesReady.load()) {
        auto custom = *desc; custom.CachedPSO = {};
        for (unsigned i = 0; i < AssetCount && SUCCEEDED(hr); ++i) {
            if (i == Visibility) continue;
            custom.CS = {assets[i].data(),assets[i].size()};
            hr = original(owner,&custom,IID_PPV_ARGS(&compute[i]));
        }
        D3D12_INDIRECT_ARGUMENT_DESC argument{}; argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
        D3D12_COMMAND_SIGNATURE_DESC signature{12,1,&argument,0};
        if (SUCCEEDED(hr)) hr = owner->CreateCommandSignature(&signature,nullptr,IID_PPV_ARGS(&indirectCompute));
        argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
        signature.ByteStride = sizeof(D3D12_DISPATCH_RAYS_DESC);
        if (SUCCEEDED(hr)) hr = owner->CreateCommandSignature(&signature,nullptr,IID_PPV_ARGS(&indirectRays));
        if (SUCCEEDED(hr)) hr = pool.Initialize(owner);
        if (SUCCEEDED(hr)) { device = owner; root = desc->pRootSignature; pipelinesReady.store(true); }
        else runtimeFailed.store(true);
        Trace("rt_all_pipelines ready=%u hr=0x%08lX",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr));
    }
    if (SUCCEEDED(hr) && !runtimeFailed.load()) {
        ComPtr<ID3D12PipelineState> pipeline;
        if (SUCCEEDED(native->QueryInterface(IID_PPV_ARGS(&pipeline)))) pipeline->SetPrivateData(kStage,sizeof(stage),&stage);
    }
    ReleaseSRWLockExclusive(&pipelineLock);
    RefreshReady();
}

inline HRESULT CreateAugmented(ID3D12Device5* owner, const D3D12_STATE_OBJECT_DESC* desc, REFIID iid, void** out) {
    const auto original = rt_shadow_capture::originalState;
    if (!desc || desc->Type != D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) return original(owner,desc,iid,out);
    bool target = false;
    for (UINT i = 0; i < desc->NumSubobjects; ++i) {
        const auto& object = desc->pSubobjects[i];
        if (object.Type != D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY || !object.pDesc) continue;
        const auto& shader = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(object.pDesc)->DXILLibrary;
        target |= rt_spot_runtime::Matches(shader,rt_spot_runtime::kRaySha) || rt_spot_runtime::Matches(shader,rt_spot_runtime::kMultiRaySha);
    }
    if (!target) return original(owner,desc,iid,out);
    LoadAssets();
    if (!assetsReady) return original(owner,desc,iid,out);
    try {
        std::vector<D3D12_STATE_SUBOBJECT> objects(desc->pSubobjects,desc->pSubobjects + desc->NumSubobjects);
        objects.reserve(desc->NumSubobjects + 1);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
        std::vector<std::vector<LPCWSTR>> names(desc->NumSubobjects);
        for (UINT i = 0; i < desc->NumSubobjects; ++i) {
            if (objects[i].Type != D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) continue;
            associations[i] = *static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[i].pDesc);
            UINT index = desc->NumSubobjects;
            for (UINT j = 0; j < desc->NumSubobjects; ++j) if (associations[i].pSubobjectToAssociate == desc->pSubobjects + j) index = j;
            if (index == desc->NumSubobjects) return original(owner,desc,iid,out);
            bool applies = false;
            for (UINT j = 0; j < associations[i].NumExports; ++j) applies |= wcscmp(associations[i].pExports[j],L"spotRayGeneration") == 0;
            const auto type = objects[index].Type;
            if (applies && (type == D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE || type == D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG)) {
                names[i].assign(associations[i].pExports,associations[i].pExports + associations[i].NumExports);
                names[i].push_back(L"controlPerLightVisibility");
                associations[i].NumExports = static_cast<UINT>(names[i].size()); associations[i].pExports = names[i].data();
            }
            associations[i].pSubobjectToAssociate = &objects[index]; objects[i].pDesc = &associations[i];
        }
        D3D12_EXPORT_DESC exportDesc{L"controlPerLightVisibility",L"perLightVisibility",D3D12_EXPORT_FLAG_NONE};
        D3D12_DXIL_LIBRARY_DESC library{{assets[Visibility].data(),assets[Visibility].size()},1,&exportDesc};
        objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library});
        auto custom = *desc; custom.NumSubobjects = static_cast<UINT>(objects.size()); custom.pSubobjects = objects.data();
        HRESULT hr = original(owner,&custom,iid,out);
        if (FAILED(hr)) {
            Trace("rt_all_state_failed hr=0x%08lX native_fallback=1",static_cast<unsigned long>(hr));
            return original(owner,desc,iid,out);
        }
        ComPtr<ID3D12StateObject> state; ComPtr<ID3D12StateObjectProperties> properties;
        if (!out || !*out || FAILED(static_cast<IUnknown*>(*out)->QueryInterface(IID_PPV_ARGS(&state))) || FAILED(state.As(&properties))) return hr;
        const void* identifier = properties->GetShaderIdentifier(L"controlPerLightVisibility");
        if (!identifier) return hr;
        D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD; heap.CreationNodeMask = heap.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC buffer{}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; buffer.Width = 64; buffer.Height = 1;
        buffer.DepthOrArraySize = buffer.MipLevels = 1; buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> record;
        HRESULT prepared = owner->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&record));
        void* mapped = nullptr; D3D12_RANGE empty{0,0};
        if (SUCCEEDED(prepared)) prepared = record->Map(0,&empty,&mapped);
        if (SUCCEEDED(prepared)) {
            memset(mapped,0,64); memcpy(mapped,identifier,32); record->Unmap(0,nullptr);
            prepared = state->SetPrivateDataInterface(kRayRecord,record.Get());
        }
        if (SUCCEEDED(prepared)) rayReady.store(true);
        RefreshReady();
        Trace("rt_all_state ready=%u native_exports_retained=1 gpu_confirmed=0",unsigned(SUCCEEDED(prepared)));
        return hr;
    } catch (...) { return original(owner,desc,iid,out); }
}
inline void WorkerTick() noexcept {
    if (!pipelinesReady.load() || runtimeFailed.load()) return;
    const bool enabled = LiveSelection::GetMode(selection.Read()) == Mode::RtAllSpot;
    pool.Poll(!enabled, [](const rt_lights::FrameResources::EvidenceSummary& e) {
        Trace("rt_all_gpu first=%u receivers=%u tasks=%u ranges=%u lit_ranges=%u lighting=%u sampled=%u visibility=%u contributions=%u sampled_rays=%u zero_contributions=%u rejected=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
            e.first,e.receivers,e.tasks,e.validRanges,e.litRanges,e.validLighting,e.sampledTasks,e.validVisibility,e.validContributions,e.tracedRays,e.zeroContributions,
            e.rejected[0],e.rejected[1],e.rejected[2],e.rejected[3],e.rejected[4],e.rejected[5],e.rejected[6],e.rejected[7],e.rejected[8],e.rejected[9]);
    });
    const UINT64 extent = requestedExtent.load();
    if (!enabled || !extent) return;
    try {
        const HRESULT hr = pool.Prepare(static_cast<UINT>(extent >> 32),static_cast<UINT>(extent),1536ull * 1024 * 1024);
        if (FAILED(hr)) {
            runtimeFailed.store(true); readyMask.fetch_and(~kRtAllSpotReadyBit);
            Trace("rt_all_allocation_failed hr=0x%08lX",static_cast<unsigned long>(hr));
        }
    } catch (...) { runtimeFailed.store(true); readyMask.fetch_and(~kRtAllSpotReadyBit); }
}
}
