#pragma once

// Routes P2 radiance for rough primary surfaces into Control's raw diffuse-GI
// color target. Called at the reviewed DGI boundary (renderer+0x12CD55), after
// the native DGI producer and before the later filter/history branch.

struct PTDiffuseGIBridgeOwner {
    ID3D12Device* device=nullptr;
    ID3D12RootSignature* root=nullptr;
    ID3D12PipelineState* pipeline=nullptr;
    ID3D12DescriptorHeap* heap=nullptr;
    UINT descriptorSize=0;
    bool stopped=false;
};
static SRWLOCK ptDiffuseGIBridgeLock=SRWLOCK_INIT;
static std::vector<PTDiffuseGIBridgeOwner*> ptDiffuseGIBridgeOwners;
static std::atomic<unsigned long long> ptDiffuseGIBridgeCalls{0};
static std::atomic<unsigned long long> ptDiffuseGIBridgeDispatches{0};
static std::atomic<unsigned long long> ptDiffuseGIBridgeRejects{0};

struct PTDiffuseGIBorrowed {
    void* native=nullptr;
    void* tracker=nullptr;
    ID3D12Resource* resource=nullptr;
    unsigned state=0;
    D3D12_RESOURCE_DESC desc{};
};

static bool PTDiffuseGIReadShaderTexture(size_t rva,PTDiffuseGIBorrowed* out) noexcept {
    if(out)*out={};
    if(!out||!verifiedD3d||!verifiedRenderer)return false;
    using GetNativeTextureFn=void* (*)(void*);
    auto getter=reinterpret_cast<GetNativeTextureFn>(
        GetProcAddress(verifiedD3d,kRRGetNativeTextureSymbol));
    if(!getter)return false;
    DWORD fault=0;
    __try {
        out->native=getter(reinterpret_cast<unsigned char*>(verifiedRenderer)+rva);
        if(!out->native)__leave;
        auto* n=reinterpret_cast<unsigned char*>(out->native);
        out->tracker=*reinterpret_cast<void**>(n+0x50);
        if(!out->tracker)out->tracker=n+0x40;
        out->resource=*reinterpret_cast<ID3D12Resource**>(n+0x88);
        if(!out->resource)__leave;
        out->state=*reinterpret_cast<unsigned*>(
            reinterpret_cast<unsigned char*>(out->tracker)+0x20);
        out->desc=out->resource->GetDesc();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
    }
    return fault==0&&out->native&&out->tracker&&out->resource;
}

static const char* PTDiffuseGIResourceShapeReason(
    const D3D12_RESOURCE_DESC& d,bool uav) noexcept {
    if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D)return "not_texture2d";
    if(!d.Width||!d.Height||d.Width>8192||d.Height>8192)
        return "invalid_extent";
    if(d.DepthOrArraySize!=1)return "array_or_depth_not_one";
    if(d.MipLevels!=1)return "mip_count_not_one";
    if(d.SampleDesc.Count!=1)return "multisampled";
    if(d.Format==DXGI_FORMAT_UNKNOWN)return "unknown_format";
    if(uav&&!(d.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        return "missing_uav_flag";
    if(!uav&&(d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        return "srv_denied";
    return "ready";
}
static void PTDiffuseGILogResourceShape(
    unsigned long long call,unsigned long long frame,unsigned mode,
    const char* label,const PTDiffuseGIBorrowed& resource,
    const char* verdict,bool uav) noexcept {
    const auto& d=resource.desc;
    Log("PT_REFLECTION_P2_DGI_RESOURCE_DESC call=%llu frame=%llu verify_mode=%u name=%s result=%s expected_uav=%u native=%p resource=%p tracker=%p tracked_state=0x%X dim=%u width=%llu height=%u array=%u mips=%u sample_count=%u sample_quality=%u format=%u flags=0x%X layout=%u alignment=%llu",
        call,frame,mode,label,verdict,unsigned(uav),
        resource.native,resource.resource,resource.tracker,resource.state,
        unsigned(d.Dimension),static_cast<unsigned long long>(d.Width),d.Height,
        unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),
        d.SampleDesc.Count,d.SampleDesc.Quality,unsigned(d.Format),unsigned(d.Flags),
        unsigned(d.Layout),static_cast<unsigned long long>(d.Alignment));
}

static bool PTDiffuseGIResourceShape(const D3D12_RESOURCE_DESC& d,bool uav) noexcept {
    if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d.Width||!d.Height||
       d.Width>8192||d.Height>8192||d.DepthOrArraySize!=1||d.MipLevels!=1||
       d.SampleDesc.Count!=1||d.Format==DXGI_FORMAT_UNKNOWN)return false;
    if(uav&&!(d.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))return false;
    if(!uav&&(d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))return false;
    return true;
}

static PTDiffuseGIBridgeOwner* PTDiffuseGIFindOwner(ID3D12Device* device) noexcept {
    for(auto* c:ptDiffuseGIBridgeOwners)
        if(c&&!c->stopped&&c->device==device)return c;
    return nullptr;
}

static bool PTDiffuseGICreateOwner(ID3D12Device* device,PTDiffuseGIBridgeOwner** out) noexcept {
    if(out)*out=nullptr;
    if(!device||!out||!ptReflectionP2DiffuseBridgeLoaded||
       ptReflectionP2DiffuseBridgeBytes.empty())return false;
    auto* c=new(std::nothrow) PTDiffuseGIBridgeOwner;
    if(!c)return false;
    c->device=device;c->device->AddRef();
    HRESULT hr=S_OK;

    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors=2;
    ranges[0].BaseShaderRegister=0;
    ranges[0].RegisterSpace=0;
    ranges[0].OffsetInDescriptorsFromTableStart=0;
    ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors=1;
    ranges[1].BaseShaderRegister=0;
    ranges[1].RegisterSpace=0;
    ranges[1].OffsetInDescriptorsFromTableStart=2;

    D3D12_ROOT_PARAMETER params[2]{};
    params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[0].DescriptorTable.NumDescriptorRanges=2;
    params[0].DescriptorTable.pDescriptorRanges=ranges;
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[1].Constants.ShaderRegister=0;
    params[1].Constants.RegisterSpace=0;
    params[1].Constants.Num32BitValues=8;

    D3D12_ROOT_SIGNATURE_DESC rd{};
    rd.NumParameters=2;rd.pParameters=params;
    auto serialize=reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(
        GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
    ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
    hr=serialize?serialize(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors):E_NOINTERFACE;
    if(SUCCEEDED(hr))
        hr=blob?device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),
            IID_PPV_ARGS(&c->root)):E_UNEXPECTED;
    if(blob)blob->Release();if(errors)errors->Release();

    if(SUCCEEDED(hr)){
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};
        pd.pRootSignature=c->root;
        pd.CS={ptReflectionP2DiffuseBridgeBytes.data(),ptReflectionP2DiffuseBridgeBytes.size()};
        hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&c->pipeline));
    }
    if(SUCCEEDED(hr)){
        D3D12_DESCRIPTOR_HEAP_DESC hd{};
        hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hd.NumDescriptors=3;
        hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&c->heap));
    }
    if(FAILED(hr)||!c->root||!c->pipeline||!c->heap){
        c->stopped=true;ptDiffuseGIBridgeOwners.push_back(c);
        Log("PT_REFLECTION_P2_DGI_BRIDGE_CREATE ready=0 hr=0x%08lX device=%p retained_partial=1",
            static_cast<unsigned long>(hr),device);
        return false;
    }
    c->descriptorSize=device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ptDiffuseGIBridgeOwners.push_back(c);
    *out=c;
    Log("PT_REFLECTION_P2_DGI_BRIDGE_CREATE ready=1 device=%p owner=%p shader_bytes=%llu sha256=%s rough_route=diffuse_gi strength=0.45",
        device,c,static_cast<unsigned long long>(ptReflectionP2DiffuseBridgeBytes.size()),
        ptReflectionP2DiffuseBridgeSha256);
    return true;
}

static PTDiffuseGIBridgeOwner* PTDiffuseGIGetOwner(ID3D12Device* device) noexcept {
    AcquireSRWLockExclusive(&ptDiffuseGIBridgeLock);
    auto* c=PTDiffuseGIFindOwner(device);
    if(!c)PTDiffuseGICreateOwner(device,&c);
    ReleaseSRWLockExclusive(&ptDiffuseGIBridgeLock);
    return c;
}

// The broad RR guide reader requires GBuffer2 and a matching camera frame.
// This DGI bridge consumes neither: only the active native command context
// and GBuffer1 are required. Capture exactly those inputs at the GI boundary,
// and repeat that same read after taking the engine locks.
static bool PTDiffuseGIReadBridgeInputs(RRGuideInputSnapshot* out,
                                       const char** reason) noexcept {
    if(!out||!reason)return false;
    *out={};
    *reason="renderer_context";
    if(!RRGuideReadRendererContext(out,reason))return false;
    *reason="gbuffer1";
    if(!RRGuideReadTexture(kRRGuideGBuffer1ShaderRva,
                           &out->gbuffer1,reason,&out->fault))return false;
    *reason="ready";
    return true;
}

static bool PTReflectionBridgeDiffuseGI() noexcept {
    const auto call=ptDiffuseGIBridgeCalls.fetch_add(1,std::memory_order_relaxed)+1;
    const unsigned verifyMode=ptReflectionVerifyMode.load(std::memory_order_acquire);
    if(!ptReflectionP2InjectionEnabled||!ptReflectionP2DiffuseBridgeLoaded||
       !(rrNativeRTFrame.effects&control_rr::RTDiffuseGI))return false;
    // While displaying raw P2 hit/energy quadrants, do not feed the deliberately
    // colored reflection buffer into diffuse GI and confuse the stage test.
    if(verifyMode==1u){
        if(call<=3u||(call%240u)==0u)
            Log("PT_REFLECTION_VERIFY_GI_SKIP call=%llu mode=1 reason=isolate_composite_output",
                call);
        return false;
    }

    const char* reason="unknown";
    RRGuideInputSnapshot input{},again{};
    PTDiffuseGIBorrowed reflection{},dgi{};
    ID3D12Device* device=nullptr;
    PTDiffuseGIBridgeOwner* owner=nullptr;
    void* locks[4]{};
    unsigned lockCount=0,held=0;
    bool recordingHeld=false;
    bool transitionsIssued=false;
    bool restored=false;
    DWORD fault=0;
    bool dispatched=false;
    D3D12_RESOURCE_BARRIER enter[3]{},leave[3]{};
    UINT barrierCount=0;

    __try {
        reason="renderer_inputs";
        if(!PTDiffuseGIReadBridgeInputs(&input,&reason))__leave;
        if(!input.commandList||!input.commandContext||!input.recordingLock||
           !input.gbuffer1.resource||!input.gbuffer1.stateTracker){
            reason="incomplete_renderer_inputs";
            __leave;
        }

        reason="reflection_texture";
        if(!PTDiffuseGIReadShaderTexture(kRRShaderReflectionTargetRva,&reflection))__leave;
        reason="dgi_texture";
        if(!PTDiffuseGIReadShaderTexture(kRRShaderDiffuseGIColorRva,&dgi))__leave;
        reason="resource_shape";
        const char* reflectionShape=PTDiffuseGIResourceShapeReason(reflection.desc,false);
        const char* gbufferShape=PTDiffuseGIResourceShapeReason(input.gbuffer1.desc,false);
        const char* dgiShape=PTDiffuseGIResourceShapeReason(dgi.desc,true);
        if(call<=4u||(call%240u)==0u||
           strcmp(reflectionShape,"ready")!=0||
           strcmp(gbufferShape,"ready")!=0||
           strcmp(dgiShape,"ready")!=0){
            // Only log the first six failures and the periodic sample, not
            // thousands of copies of the same descriptor every frame.
            static std::atomic<unsigned> shapeLogs{0};
            const auto n=shapeLogs.fetch_add(1u,std::memory_order_relaxed);
            if(n<6u||(n%512u)==0u||
                (verifyMode==2u&&((n%128u)==0u))){
                PTDiffuseGILogResourceShape(call,input.engineFrame,verifyMode,
                    "reflection",reflection,reflectionShape,false);
                PTDiffuseGIBorrowed gb{};
                gb.resource=input.gbuffer1.resource;
                gb.native=input.gbuffer1.nativeTexture;
                gb.tracker=input.gbuffer1.stateTracker;
                gb.state=input.gbuffer1.trackedState;
                gb.desc=input.gbuffer1.desc;
                PTDiffuseGILogResourceShape(call,input.engineFrame,verifyMode,
                    "gbuffer1",gb,gbufferShape,false);
                PTDiffuseGILogResourceShape(call,input.engineFrame,verifyMode,
                    "diffuse_gi",dgi,dgiShape,true);
            }
        }
        if(strcmp(reflectionShape,"ready")!=0){
            reason="reflection_resource_shape";__leave;
        }
        if(strcmp(gbufferShape,"ready")!=0){
            reason="gbuffer_resource_shape";__leave;
        }
        if(strcmp(dgiShape,"ready")!=0){
            reason="dgi_resource_shape";__leave;
        }
        reason="device";
        if(FAILED(input.commandList->GetDevice(IID_PPV_ARGS(&device)))||!device)__leave;
        owner=PTDiffuseGIGetOwner(device);
        if(!owner){reason="owner";__leave;}

        void* candidates[3]{reflection.tracker,input.gbuffer1.stateTracker,dgi.tracker};
        for(void* candidate:candidates){
            if(!candidate)continue;
            bool duplicate=false;
            for(unsigned i=0;i<lockCount;++i)if(locks[i]==candidate){duplicate=true;break;}
            if(!duplicate)locks[lockCount++]=candidate;
        }
        for(unsigned i=0;i<lockCount;++i)
            for(unsigned j=i+1;j<lockCount;++j)
                if(reinterpret_cast<std::uintptr_t>(locks[j])<reinterpret_cast<std::uintptr_t>(locks[i])){
                    void* t=locks[i];locks[i]=locks[j];locks[j]=t;
                }
        reason="tracker_lock";
        for(;held<lockCount;++held)if(!RRGuideTryAcquireNativeLock(locks[held]))__leave;
        bool recordingAlreadyHeld=false;
        for(unsigned i=0;i<lockCount;++i)if(locks[i]==input.recordingLock)recordingAlreadyHeld=true;
        if(!recordingAlreadyHeld){
            reason="recording_lock";
            recordingHeld=RRGuideTryAcquireNativeLock(input.recordingLock);
            if(!recordingHeld)__leave;
        }

        reason="context_changed";
        if(!PTDiffuseGIReadBridgeInputs(&again,&reason))__leave;
        if(again.commandList!=input.commandList||
           again.commandContext!=input.commandContext||
           again.engineFrame!=input.engineFrame||
           again.gbuffer1.resource!=input.gbuffer1.resource||
           again.gbuffer1.stateTracker!=input.gbuffer1.stateTracker){
            reason="renderer_inputs_changed";
            __leave;
        }
        PTDiffuseGIBorrowed reflection2{},dgi2{};
        if(!PTDiffuseGIReadShaderTexture(kRRShaderReflectionTargetRva,&reflection2)||
           !PTDiffuseGIReadShaderTexture(kRRShaderDiffuseGIColorRva,&dgi2)||
           reflection2.resource!=reflection.resource||reflection2.tracker!=reflection.tracker||
           dgi2.resource!=dgi.resource||dgi2.tracker!=dgi.tracker)__leave;
        reflection.state=reflection2.state;dgi.state=dgi2.state;
        const unsigned gbufferState=*reinterpret_cast<unsigned*>(
            reinterpret_cast<unsigned char*>(input.gbuffer1.stateTracker)+0x20);

        reason="descriptor_setup";
        auto cpu=owner->heap->GetCPUDescriptorHandleForHeapStart();
        D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels=1;
        sd.Format=reflection.desc.Format;
        owner->device->CreateShaderResourceView(reflection.resource,&sd,cpu);
        cpu.ptr+=owner->descriptorSize;
        sd.Format=input.gbuffer1.desc.Format;
        owner->device->CreateShaderResourceView(input.gbuffer1.resource,&sd,cpu);
        cpu.ptr+=owner->descriptorSize;
        D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        ud.Format=dgi.desc.Format;
        owner->device->CreateUnorderedAccessView(dgi.resource,nullptr,&ud,cpu);

        ID3D12Resource* resources[3]{reflection.resource,input.gbuffer1.resource,dgi.resource};
        unsigned before[3]{reflection.state,gbufferState,dgi.state};
        D3D12_RESOURCE_STATES after[3]{
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS};
        for(UINT i=0;i<3;++i){
            if(before[i]==static_cast<unsigned>(after[i]))continue;
            auto& b=enter[barrierCount];
            b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource=resources[i];
            b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            b.Transition.StateBefore=static_cast<D3D12_RESOURCE_STATES>(before[i]);
            b.Transition.StateAfter=after[i];
            leave[barrierCount]=b;
            leave[barrierCount].Transition.StateBefore=after[i];
            leave[barrierCount].Transition.StateAfter=static_cast<D3D12_RESOURCE_STATES>(before[i]);
            ++barrierCount;
        }

        reason="record";
        if(barrierCount)input.commandList->ResourceBarrier(barrierCount,enter);
        transitionsIssued=true;
        ID3D12DescriptorHeap* heaps[]{owner->heap};
        input.commandList->SetDescriptorHeaps(1,heaps);
        input.commandList->SetComputeRootSignature(owner->root);
        input.commandList->SetPipelineState(owner->pipeline);
        input.commandList->SetComputeRootDescriptorTable(0,owner->heap->GetGPUDescriptorHandleForHeapStart());
        unsigned constants[8]{
            static_cast<unsigned>(dgi.desc.Width),dgi.desc.Height,
            static_cast<unsigned>(reflection.desc.Width),reflection.desc.Height,
            static_cast<unsigned>(input.gbuffer1.desc.Width),input.gbuffer1.desc.Height,0,0};
        const float strength=0.45f;
        const float diagnostic=verifyMode==2u?2.0f:0.0f;
        memcpy(&constants[6],&strength,sizeof(strength));
        memcpy(&constants[7],&diagnostic,sizeof(diagnostic));
        input.commandList->SetComputeRoot32BitConstants(1,8,constants,0);
        input.commandList->Dispatch((constants[0]+7u)/8u,(constants[1]+7u)/8u,1);
        D3D12_RESOURCE_BARRIER u{};
        u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;u.UAV.pResource=dgi.resource;
        input.commandList->ResourceBarrier(1,&u);
        if(barrierCount)input.commandList->ResourceBarrier(barrierCount,leave);
        restored=true;dispatched=true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
        reason="exception";
    }

    // If recording faulted after the initial transitions, attempt the exact
    // reverse barriers before releasing native tracker locks.
    if(transitionsIssued&&!restored&&input.commandList&&barrierCount){
        DWORD restoreFault=0;
        __try {
            input.commandList->ResourceBarrier(barrierCount,leave);
            restored=true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            restoreFault=GetExceptionCode();
        }
        Log("PT_REFLECTION_P2_DGI_BRIDGE_EMERGENCY_RESTORE call=%llu attempted=1 success=%u exception=0x%08lX",
            call,unsigned(restored),restoreFault);
    }
    if(recordingHeld)RRGuideReleaseNativeLock(input.recordingLock);
    while(held)RRGuideReleaseNativeLock(locks[--held]);
    if(device)device->Release();

    if(dispatched){
        const auto n=ptDiffuseGIBridgeDispatches.fetch_add(1,std::memory_order_relaxed)+1;
        if(n<=16||(n%240)==0)
            Log("PT_REFLECTION_P2_DGI_BRIDGE call=%llu dispatch=%llu frame=%llu output=%llux%u reflection=%llux%u gbuffer=%llux%u strength=0.45 roughness_gate=0.42_0.88 destination=raw_diffuse_gi_color resource_states_restored=1 verify_mode=%u marker=%u",
                call,n,input.engineFrame,
                static_cast<unsigned long long>(dgi.desc.Width),dgi.desc.Height,
                static_cast<unsigned long long>(reflection.desc.Width),reflection.desc.Height,
                static_cast<unsigned long long>(input.gbuffer1.desc.Width),input.gbuffer1.desc.Height,
                verifyMode,unsigned(verifyMode==2u));
        return true;
    }
    const auto reject=ptDiffuseGIBridgeRejects.fetch_add(1,std::memory_order_relaxed)+1;
    if(reject<=16||(reject&(reject-1))==0)
        Log("PT_REFLECTION_P2_DGI_BRIDGE_SKIP call=%llu reject=%llu frame=%llu reason=%s exception=0x%08lX verify_mode=%u",
            call,reject,input.engineFrame,reason?reason:"unknown",fault,verifyMode);
    return false;
}
