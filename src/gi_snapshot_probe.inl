#pragma once
// Opt-in integration and bounded index-byte capture. Replacement GI stays off.
namespace gi_snapshot_probe {
using Microsoft::WRL::ComPtr;
struct Owner {UINT64 generation=0;std::unique_ptr<gi_reuse::IndexSample> sample,instances;bool completeData=false;};
inline bool CaptureEnabled() noexcept {static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_INDEX_SNAPSHOTS",v,4)==1&&v[0]=='1';}();return enabled;}
inline bool InstancesEnabled() noexcept {static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_INSTANCE_SNAPSHOTS",v,4)==1&&v[0]=='1';}();return enabled;}
inline std::atomic<UINT64> instanceAttempts{0},instanceRecorded{0},instanceRead{0},instanceFailed{0},instanceSkipped{0},instanceBudget{0},instanceLastHash{0};
inline bool TrianglesEnabled() noexcept {static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_TRIANGLE_SNAPSHOTS",v,4)==1&&v[0]=='1';}();return enabled;}
inline gi_reuse::SampleDiversity<> diversity;
inline SRWLOCK diversityLock=SRWLOCK_INIT;
inline std::atomic<UINT64> diversitySkipped{0};
inline std::atomic<UINT64> lastVertexHash{0};
inline std::atomic<UINT64> triangleRead{0},triangleFailed{0},triangleCount{0},triangleNonfinite{0},triangleDegenerate{0},triangleBadIndices{0},triangleSkipped{0};
inline std::atomic<bool> captureReady{false};
inline gi_reuse::IndexSnapshotPipeline indexPipeline;
inline ComPtr<ID3D12Device> captureDevice;
inline std::atomic<UINT> lastCaptureMaximum{0};
inline std::atomic<UINT64> captureAttempts[3]{},captureRecorded{0},captureRead{0},captureFailed{0},captureBudget{0};
inline std::atomic<UINT64> captureBadIndices{0},captureBadPadding{0},lastCaptureHash{0};
inline std::atomic<bool(*)() noexcept> queueCoverage{nullptr};
inline bool Coverage() noexcept {const auto check=queueCoverage.load();return check&&check()&&!gi_reuse::runtime::fault.load()&&!gi_compute_hooks::failed.load();}

using Runtime=gi_reuse::D3D12SnapshotRetirement<Owner>;
inline Runtime* runtime=nullptr;
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::array<UINT64,256> records{};
inline std::atomic<UINT64> builds{0},bindingReady{0},bindingMissing{0},rayBindings{0},ordinaryBindings{0};
inline std::atomic<UINT64> admitted{0},completed{0},refused{0},signalsFailed{0},batchOverflow{0};
inline thread_local Runtime::Batch batch;
inline const GUID key={0x62507d7e,0x3d21,0x46df,{0x95,0x42,0xb7,0x69,0x3e,0x2a,0x01,0x31}};
inline bool Enabled() noexcept {return gi_compute_hooks::Enabled();}
inline void Configure(ID3D12Device* device,const void* shader,SIZE_T bytes) noexcept {
    if(!CaptureEnabled()||!Enabled())return;
    AcquireSRWLockExclusive(&lock);
    HRESULT hr=E_FAIL;
    if(!captureDevice){captureDevice=device;hr=indexPipeline.Initialize(device,shader,bytes);captureReady=SUCCEEDED(hr);}
    else if(captureDevice.Get()==device)hr=captureReady?S_OK:E_FAIL;
    else captureReady=false;
    ReleaseSRWLockExclusive(&lock);
    Trace("GI_INDEX_INIT_R296 ready=%u hr=0x%08X max_samples=48 max_sample_bytes=262144 reserved_budget=134217728 source_committed_only=1",unsigned(SUCCEEDED(hr)),unsigned(hr));
}
inline bool Reserve(std::atomic<UINT64>& value,UINT64 amount,UINT64 limit) noexcept {
    auto old=value.load();while(old<=limit&&amount<=limit-old){if(value.compare_exchange_weak(old,old+amount))return true;}return false;
}
inline void CaptureInstances(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc,
 Owner& owner,const gi_reuse::ComputeBindingSnapshot& bindings) noexcept {
 if(!InstancesEnabled()||owner.instances||instanceAttempts.load()>=4)return;
 const auto& input=desc.Inputs;const UINT64 bytes=UINT64(input.NumDescs)*64;
 if(input.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY||!bytes||bytes>4*1024*1024||!input.InstanceDescs||input.InstanceDescs%16){++instanceSkipped;return;}
 ComPtr<ID3D12Resource> source;UINT64 offset=0;
 if(!gi_reuse::runtime::owners.Resolve(input.InstanceDescs,bytes,source,offset)){++instanceSkipped;return;}
 const auto journal=gi_reuse::runtime::Read(list);if(!journal||!journal->states.Complete()){++instanceSkipped;return;}
 const auto state=journal->states.Find(gi_reuse::runtime::owners.Identity(source.Get()));
 if(state.kind==gi_reuse::LocalStateKind::Pending||state.kind==gi_reuse::LocalStateKind::Invalid||
    (state.kind==gi_reuse::LocalStateKind::Explicit&&!(state.after&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))){++instanceSkipped;return;}
 const auto charge=gi_reuse::IndexSample::Charge(captureDevice.Get(),source.Get(),UINT(bytes));
 if(!charge||!Reserve(instanceAttempts,1,4)||!Reserve(instanceBudget,charge,32ull*1024*1024)){++instanceSkipped;return;}
 auto sample=std::unique_ptr<gi_reuse::IndexSample>(new(std::nothrow) gi_reuse::IndexSample);
 if(!sample||FAILED(sample->Prepare(captureDevice.Get(),source.Get(),offset,bytes,bindings,4,4*1024*1024,true))){++instanceFailed;return;}
 sample->buildDestination=desc.DestAccelerationStructureData;sample->buildFlags=UINT(input.Flags);sample->indexAddress=input.InstanceDescs;
 owner.instances=std::move(sample);gi_compute_hooks::Mute mute;
 if(owner.instances->Record(list,indexPipeline))++instanceRecorded;else ++instanceFailed;
}
inline void Capture(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc,
                    Owner& owner,const gi_reuse::ComputeBindingSnapshot& bindings) noexcept {
    if(!CaptureEnabled()||!captureReady.load()||!bindings.Ready()||!Coverage()||!desc)return;
    if(owner.sample&&desc->Inputs.Type!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)return;
    if(desc->Inputs.Type!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL&&captureAttempts[0].load()>=16&&captureAttempts[1].load()>=16&&captureAttempts[2].load()>=16)return;
    ComPtr<ID3D12Device> recordingDevice;if(FAILED(list->GetDevice(IID_PPV_ARGS(&recordingDevice)))||recordingDevice.Get()!=captureDevice.Get())return;
    if(desc->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){CaptureInstances(list,*desc,owner,bindings);return;}
    const auto& input=desc->Inputs;
    if(input.Type!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL||!input.NumDescs||input.NumDescs>gi_reuse::GeometryMetadata::MaxGeometries)return;
    const bool array=input.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY;
    if((!array&&input.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS)||(array?!input.pGeometryDescs:!input.ppGeometryDescs))return;
    const auto journal=gi_reuse::runtime::Read(list);if(!journal||!journal->states.Complete())return;
    for(UINT i=0;i<input.NumDescs;++i){
        const auto* geometry=array?input.pGeometryDescs+i:input.ppGeometryDescs[i];
        if(!geometry||geometry->Type!=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)continue;
        const auto& t=geometry->Triangles;if((t.IndexFormat!=DXGI_FORMAT_R16_UINT&&t.IndexFormat!=DXGI_FORMAT_R32_UINT)||!t.IndexCount||!t.VertexCount)continue;
        const UINT width=t.IndexFormat==DXGI_FORMAT_R32_UINT?4u:2u;
        if(t.IndexBuffer%width)continue;
        const UINT64 full=UINT64(t.IndexCount)*width;ComPtr<ID3D12Resource> source;UINT64 offset=0;
        if(!gi_reuse::runtime::owners.Resolve(t.IndexBuffer,full,source,offset))continue;
        const auto state=journal->states.Find(gi_reuse::runtime::owners.Identity(source.Get()));
        if(state.kind==gi_reuse::LocalStateKind::Pending||state.kind==gi_reuse::LocalStateKind::Invalid||
           (state.kind==gi_reuse::LocalStateKind::Explicit&&!(state.after&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)))continue;
        // Unknown local state is admitted under the native AS input API contract,
        // not guessed COMMON. Committed storage cannot alias another placed object.
        gi_reuse::BufferStorageLease lease;if(!gi_reuse::AcquireStorageLease(source.Get(),lease)||lease.kind!=gi_reuse::BufferStorageKind::Committed)continue;
        gi_reuse::RawSnapshotPlan raw;gi_reuse::RawSnapshotReject reason;
        const bool rawOK=gi_reuse::RawSnapshotPlan::Make(source->GetDesc().Width,offset,full,raw,&reason);
        const unsigned category=width==4?2u:(rawOK?0u:1u);if(captureAttempts[category].load()>=16)continue;
        const bool paired=TrianglesEnabled();
        const UINT64 cap=paired?(gi_reuse::IndexSample::MaxSampleBytes/(3*width))*(3*width):gi_reuse::IndexSample::MaxSampleBytes;
        const UINT64 bytes=full<cap?full:cap;
        ComPtr<ID3D12Resource> vertexSource;UINT64 vertexOffset=0,vertexBytes=0,vertexCharge=0;
        if(paired){
            const gi_reuse::TriangleMetadata metadata{t.VertexBuffer.StartAddress,t.IndexBuffer,t.Transform3x4,t.VertexBuffer.StrideInBytes,t.VertexCount,t.IndexCount,UINT(t.VertexFormat),UINT(t.IndexFormat),UINT(geometry->Flags)};
            if(gi_reuse::GeometryMetadata::Validate(metadata)!=gi_reuse::GeometryReject::None){++triangleSkipped;continue;}
            vertexBytes=gi_reuse::GeometryMetadata::VertexBytes(metadata);
            if(vertexBytes>16*1024*1024||!gi_reuse::runtime::owners.Resolve(t.VertexBuffer.StartAddress,vertexBytes,vertexSource,vertexOffset)){++triangleSkipped;continue;}
            const auto vs=journal->states.Find(gi_reuse::runtime::owners.Identity(vertexSource.Get()));
            gi_reuse::BufferStorageLease vl;
            if(vs.kind==gi_reuse::LocalStateKind::Pending||vs.kind==gi_reuse::LocalStateKind::Invalid||
               (vs.kind==gi_reuse::LocalStateKind::Explicit&&!(vs.after&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))||
               !gi_reuse::AcquireStorageLease(vertexSource.Get(),vl)||vl.kind!=gi_reuse::BufferStorageKind::Committed){++triangleSkipped;continue;}
            vertexCharge=gi_reuse::IndexSample::Charge(captureDevice.Get(),vertexSource.Get(),UINT(vertexBytes));
            if(!vertexCharge){++triangleSkipped;continue;}
            const gi_reuse::SampleKey sampleKey{gi_reuse::runtime::owners.Identity(vertexSource.Get()),gi_reuse::runtime::owners.Identity(source.Get()),vertexOffset,offset,t.VertexBuffer.StrideInBytes,t.VertexCount,t.IndexCount,width,i};
            AcquireSRWLockExclusive(&diversityLock);const bool selected=diversity.Reserve(sampleKey);ReleaseSRWLockExclusive(&diversityLock);
            if(!selected){++diversitySkipped;continue;}
        }
        const UINT64 tailOffset=offset+(full-bytes);gi_reuse::IndexSnapshotPlan plan;
        if(!gi_reuse::IndexSnapshotPlan::Make(source->GetDesc().Width,tailOffset,bytes,plan))continue;
        const auto charge=gi_reuse::IndexSample::Charge(captureDevice.Get(),source.Get(),plan.copyBytes);
        if(!charge||!Reserve(captureAttempts[category],1,16)||vertexCharge>UINT64_MAX-charge||!Reserve(captureBudget,charge+vertexCharge,128ull*1024*1024))continue;
        auto sample=std::unique_ptr<gi_reuse::IndexSample>(new(std::nothrow) gi_reuse::IndexSample);
        if(!sample||FAILED(sample->Prepare(captureDevice.Get(),source.Get(),tailOffset,bytes,bindings,width))){++captureFailed;return;}
        if(paired){
            sample->vertices=std::unique_ptr<gi_reuse::IndexSample>(new(std::nothrow) gi_reuse::IndexSample);
            if(!sample->vertices||FAILED(sample->vertices->Prepare(captureDevice.Get(),vertexSource.Get(),vertexOffset,vertexBytes,bindings,4,16*1024*1024))){++captureFailed;return;}
            sample->buildDestination=desc->DestAccelerationStructureData;sample->buildFlags=UINT(input.Flags);sample->geometryCount=input.NumDescs;sample->vertexAddress=t.VertexBuffer.StartAddress;sample->indexAddress=t.IndexBuffer;
            sample->vertexStride=t.VertexBuffer.StrideInBytes;sample->geometryIndex=i;sample->hasTransform=t.Transform3x4!=0;
        }
        sample->fullBytes=full;sample->vertexCount=t.VertexCount;sample->rawReject=unsigned(reason);
        // Ownership and session allocation budget are established before commands.
        owner.sample=std::move(sample);
        gi_compute_hooks::Mute mute;
        if(owner.sample->Record(list,indexPipeline)&&(!paired||owner.sample->vertices->Record(list,indexPipeline)))++captureRecorded;else ++captureFailed;
        return;
    }
}

class State final:public IUnknown {
    std::atomic<ULONG> refs{1};
public:
    Runtime::Recording recording;
    Owner* owner=nullptr;
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
inline void Build(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc) noexcept {
    if(!Enabled()||gi_reuse::runtime::fault.load())return;++builds;
    gi_reuse::ComputeBindingSnapshot bindings;UINT64 generation=0;
    if(gi_compute_hooks::Capture(list,bindings,generation)){
        ++bindingReady;if(bindings.rays)++rayBindings;else ++ordinaryBindings;
    }else ++bindingMissing;
    if(auto current=Read(list)){if(current->owner)Capture(list,desc,*current->owner,bindings);return;}
    const auto journal=gi_reuse::runtime::Read(list);
    if(!journal){++refused;return;}
    auto owner=std::unique_ptr<Owner>(new(std::nothrow) Owner{journal->data.generation});
    auto* state=new(std::nothrow) State;
    if(!state||!owner){delete state;++refused;return;}
    state->owner=owner.get();
    AcquireSRWLockExclusive(&lock);
    if(!runtime)runtime=new(std::nothrow) Runtime;
    if(runtime)for(auto& id:records)if(!id){
        id=runtime->Admit(owner->generation,sizeof(Owner),owner);
        state->recording={id,journal->data.generation};break;
    }
    ReleaseSRWLockExclusive(&lock);
    if(!state->recording.id){++refused;state->Release();return;}
    if(FAILED(list->SetPrivateDataInterface(key,state)))++refused;else {++admitted;Capture(list,desc,*state->owner,bindings);}
    state->Release();
}
inline void Poll() noexcept {
    std::array<std::unique_ptr<Owner>,256> retired;
    AcquireSRWLockExclusive(&lock);
    if(runtime){
      if(CaptureEnabled()&&!Coverage())runtime->QuarantineAll();
      runtime->Poll();unsigned gpuReads=0;
      for(unsigned i=0;i<records.size();++i)if(records[i]){
        const auto* view=runtime->View(records[i]);const bool published=view!=nullptr;
        if(view&&(view->sample||view->instances)&&gpuReads++>=2)continue;
        if(runtime->Take(records[i],retired[i])){retired[i]->completeData=published;if(published)++completed;records[i]=0;}
    }}
    ReleaseSRWLockExclusive(&lock);
    // Readback and COM destruction happen outside the host observation lock.
    for(const auto& owner:retired)if(owner&&owner->completeData&&owner->instances){
        const auto& sample=*owner->instances;gi_reuse::InstanceSampleSummary result;const auto hr=sample.ReadInstances(result);
        if(SUCCEEDED(hr)){++instanceRead;instanceLastHash=result.hash;}else ++instanceFailed;
        Trace("GI_INSTANCE_COPY_R302 hr=0x%08X destination=%llu build_flags=%u source_va=%llu bytes=%llu hash=%llu count=%u masked=%u null_blas=%u bad_alignment=%u nonfinite=%u unknown_flags=%u sealed=1 gpu_completed=1 material_mapping=0",unsigned(hr),sample.buildDestination,sample.buildFlags,sample.indexAddress,sample.bytes,result.hash,result.count,result.masked,result.nullBLAS,result.badAlignment,result.nonfinite,result.unknownFlags);
        if(SUCCEEDED(hr))for(unsigned i=0;i<result.count&&i<result.first.size();++i){const auto& v=result.first[i];
            Trace("GI_INSTANCE_RECORD_R302 array_index=%u instance_id=%u mask=%u hit_group_offset=%u flags=%u blas=%llu transform=%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",i,v.id,v.mask,v.hitGroup,v.flags,v.blas,double(v.transform[0]),double(v.transform[1]),double(v.transform[2]),double(v.transform[3]),double(v.transform[4]),double(v.transform[5]),double(v.transform[6]),double(v.transform[7]),double(v.transform[8]),double(v.transform[9]),double(v.transform[10]),double(v.transform[11]));
        }
    }
    for(const auto& owner:retired)if(owner&&owner->completeData&&owner->sample&&owner->sample->recorded){
        const auto& sample=*owner->sample;gi_reuse::IndexSample::Summary result;
        if(sample.vertices){gi_reuse::TriangleSampleSummary tri;const auto th=sample.ReadTriangles(tri);
            if(SUCCEEDED(th)){++triangleRead;lastVertexHash=tri.vertexHash;triangleCount+=tri.triangles;triangleNonfinite+=tri.nonfinite;triangleDegenerate+=tri.degenerate;triangleBadIndices+=tri.outOfRange;}else ++triangleFailed;
            Trace("GI_TRIANGLE_COPY_R300 hr=0x%08X geometry=%u vertex_bytes=%llu vertex_hash=%llu stride=%llu vertex_count=%u index_bytes=%u first_primitive=%llu triangles=%u out_of_range=%u nonfinite=%u degenerate=%u has_geometry_transform=%u object_space=1 sealed=1 gpu_completed=1",unsigned(th),sample.geometryIndex,sample.vertices->bytes,tri.vertexHash,sample.vertexStride,sample.vertexCount,sample.indexSize,(sample.fullBytes-sample.bytes)/(3*sample.indexSize),tri.triangles,tri.outOfRange,tri.nonfinite,tri.degenerate,unsigned(sample.hasTransform));
            Trace("GI_TRIANGLE_DETAIL_R301 destination=%llu build_flags=%u geometry_count=%u geometry=%u vertex_va=%llu index_va=%llu repeated_indices=%u coincident_positions=%u collinear=%u all_zero_positions=%u inactive_x_nan=%u",sample.buildDestination,sample.buildFlags,sample.geometryCount,sample.geometryIndex,sample.vertexAddress,sample.indexAddress,tri.repeatedIndices,tri.coincidentPositions,tri.collinear,tri.zeroPositions,tri.inactive);
            const gi_reuse::TriangleWitness* witnesses[]={&tri.first,&tri.last,&tri.firstDegenerate,&tri.firstNonfinite};
            for(unsigned w=0;w<4;++w)if(witnesses[w]->present){const auto& v=*witnesses[w];
                Trace("GI_TRIANGLE_WITNESS_R301 kind=%u primitive=%llu indices=%u,%u,%u p0_bits=%08X,%08X,%08X p1_bits=%08X,%08X,%08X p2_bits=%08X,%08X,%08X",w,(sample.fullBytes-sample.bytes)/(3*sample.indexSize)+v.primitive,v.indices[0],v.indices[1],v.indices[2],v.bits[0][0],v.bits[0][1],v.bits[0][2],v.bits[1][0],v.bits[1][1],v.bits[1][2],v.bits[2][0],v.bits[2][1],v.bits[2][2]);
            }
        }
        const auto hr=sample.Read(result);if(SUCCEEDED(hr)){++captureRead;captureBadIndices+=result.outOfRange;if(!result.paddingZero)++captureBadPadding;lastCaptureHash=result.hash;lastCaptureMaximum=result.maximum;}else ++captureFailed;
        Trace("GI_INDEX_COPY_R296 index_bytes=%u hr=0x%08X bytes=%llu full_bytes=%llu tail_window=%u source_offset=%llu raw_reject=%u hash=%llu max_index=%u vertex_count=%u out_of_range=%u padding_zero=%u sealed=1 gpu_completed=1",sample.indexSize,unsigned(hr),sample.bytes,sample.fullBytes,unsigned(sample.bytes!=sample.fullBytes),sample.offset,sample.rawReject,result.hash,result.maximum,sample.vertexCount,result.outOfRange,unsigned(result.paddingZero));
    }
}
inline UINT64 Before(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
    if(!Enabled())return 0;Poll();
    if(!queue||!lists||queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_COPY)return 0;
    std::array<Runtime::Recording,256> submitted{};unsigned n=0;
    for(UINT i=0;i<count;++i){auto state=Read(lists[i]);if(!state)continue;
        if(n==submitted.size()){++batchOverflow;AcquireSRWLockExclusive(&lock);if(runtime)runtime->QuarantineAll();ReleaseSRWLockExclusive(&lock);return 0;}submitted[n++]=state->recording;
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
    Trace("GI_LIFETIME_R295 active=%u quarantined=%u cpu_metadata_owners_only=%u publication_requires_reset_and_gpu_completion=1",active,quarantined,unsigned(!CaptureEnabled()));
    Trace("GI_INDEX_STATUS_R296 enabled=%u ready=%u recorded=%llu readback=%llu failures=%llu reserved_bytes=%llu ordinary_attempts=%llu rejected_raw_attempts=%llu uint32_attempts=%llu native_lighting_changed=0",unsigned(CaptureEnabled()),unsigned(captureReady.load()),captureRecorded.load(),captureRead.load(),captureFailed.load(),captureBudget.load(),captureAttempts[0].load(),captureAttempts[1].load(),captureAttempts[2].load());
    Trace("GI_INSTANCE_STATUS_R302 enabled=%u attempts=%llu recorded=%llu readback=%llu failures=%llu skipped=%llu reserved_bytes=%llu",unsigned(InstancesEnabled()),instanceAttempts.load(),instanceRecorded.load(),instanceRead.load(),instanceFailed.load(),instanceSkipped.load(),instanceBudget.load());
    Trace("GI_SAMPLE_DIVERSITY_R301 repeated_or_untracked_skipped=%llu max_per_descriptor=2",diversitySkipped.load());
    Trace("GI_TRIANGLE_STATUS_R300 enabled=%u readback=%llu failures=%llu triangles=%llu out_of_range=%llu nonfinite=%llu degenerate=%llu skipped=%llu",unsigned(TrianglesEnabled()),triangleRead.load(),triangleFailed.load(),triangleCount.load(),triangleBadIndices.load(),triangleNonfinite.load(),triangleDegenerate.load(),triangleSkipped.load());
    Trace("GI_INDEX_VALIDATION_R296 out_of_range_indices=%llu padding_failures=%llu",captureBadIndices.load(),captureBadPadding.load());
    const auto& reasons=gi_reuse::runtime::indexRawReasons;
    Trace("GI_INDEX_PLAN_R296 raw_pass=%llu invalid_range=%llu address_overflow=%llu padding_outside=%llu dispatch_limit=%llu exact16_eligible=%llu",reasons[0].load(),reasons[1].load(),reasons[2].load(),reasons[3].load(),reasons[4].load(),gi_reuse::runtime::exactIndexEligible.load());
    for(unsigned kind=0;kind<4;++kind){const auto& c=gi_reuse::runtime::inputAudit[kind];
        Trace("GI_INPUT_R295 kind=%u committed=%llu placed=%llu storage_unknown=%llu deny_srv=%llu range_rejected=%llu local_preflight_pass=%llu alias_verified=0 gpu_copies=0",kind,c[0].load(),c[1].load(),c[2].load(),c[3].load(),c[4].load(),c[5].load());
    }
    Trace("GI_INTEGRATION_R295 builds=%llu binding_ready=%llu binding_missing=%llu ordinary_pso=%llu ray_pso=%llu hook_failed=%u live_trackers=%u admitted_recordings=%llu completed_sealed_recordings=%llu admission_refused=%llu signal_failures=%llu batch_overflow=%llu gpu_copies=%llu replacement_gi=0",builds.load(),bindingReady.load(),bindingMissing.load(),ordinaryBindings.load(),rayBindings.load(),unsigned(gi_compute_hooks::failed.load()),gi_compute_hooks::liveStates.load(),admitted.load(),completed.load(),refused.load(),signalsFailed.load(),batchOverflow.load(),captureRecorded.load());
}
}
