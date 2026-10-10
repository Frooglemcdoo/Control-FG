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
    // Control allocates the native reflection and DGI textures with a full
    // mip pyramid (12 at 2560x1440). Our SRV/UAV descriptors select mip 0.
    if(d.MipLevels<1||d.MipLevels>14)return "invalid_mip_chain";
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
       d.Width>8192||d.Height>8192||d.DepthOrArraySize!=1||
       d.MipLevels<1||d.MipLevels>14||
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
    Log("PT_REFLECTION_P2_DGI_BRIDGE_CREATE ready=1 device=%p owner=%p shader_bytes=%llu sha256=%s rough_route=diffuse_gi strength=0.45 mip0_views=1",
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


// Forward-only DGI transport proof. This path preserves the engine's bound
// descriptor heaps, compute root signature, root arguments and pipeline.
// A small immutable UPLOAD buffer is copied into mip 0 of Control's native
// diffuse-GI target at the exact reviewed GI boundary. No custom compute PSO
// or descriptor/root changes occur.
//
// This intentionally REPLACES a 512x512 area of native GI as a diagnostic;
// it is not bounce lighting, and it only runs in explicit F11 mode 2.
struct PTDiffuseGICopyProofOwner {
    ID3D12Device* device=nullptr;
    ID3D12Resource* upload=nullptr;
    UINT width=512u;
    UINT height=512u;
    UINT rowPitch=4096u;
    bool stopped=false;
};
static SRWLOCK ptDiffuseGICopyProofLock=SRWLOCK_INIT;
static std::vector<PTDiffuseGICopyProofOwner*> ptDiffuseGICopyProofOwners;
static std::atomic<unsigned long long> ptDiffuseGICopyProofCopies{0};
static std::atomic<unsigned long long> ptDiffuseGICopyProofSkips{0};

static PTDiffuseGICopyProofOwner* PTDiffuseGIFindCopyProof(ID3D12Device* device) noexcept {
    for(auto* owner:ptDiffuseGICopyProofOwners)
        if(owner&&owner->device==device&&!owner->stopped&&owner->upload)
            return owner;
    return nullptr;
}

static PTDiffuseGICopyProofOwner* PTDiffuseGICreateCopyProof(ID3D12Device* device) noexcept {
    if(!device)return nullptr;
    auto* owner=new(std::nothrow) PTDiffuseGICopyProofOwner;
    if(!owner)return nullptr;
    owner->device=device;
    device->AddRef();
    // 512x512 RGBA16_FLOAT; each row is 4096 bytes (256-byte aligned).
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type=D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width=static_cast<UINT64>(owner->rowPitch)*owner->height;
    desc.Height=1u;
    desc.DepthOrArraySize=1u;
    desc.MipLevels=1u;
    desc.Format=DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count=1u;
    desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    HRESULT hr=device->CreateCommittedResource(
        &properties,D3D12_HEAP_FLAG_NONE,&desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,
        IID_PPV_ARGS(&owner->upload));

    if(SUCCEEDED(hr)&&owner->upload){
        D3D12_RANGE readRange{0,0};
        unsigned char* mapped=nullptr;
        hr=owner->upload->Map(0,&readRange,reinterpret_cast<void**>(&mapped));
        if(SUCCEEDED(hr)&&mapped){
            // Half-float constants: 12.0 = 0x4A00, 10.0 = 0x4900,
            // 1.0 = 0x3C00. Strong alternating magenta/cyan checker.
            constexpr unsigned short cyan[4]{0x0000,0x4A00,0x4900,0x3C00};
            constexpr unsigned short magenta[4]{0x4A00,0x0000,0x4900,0x3C00};
            for(UINT y=0;y<owner->height;++y){
                auto* dst=mapped+static_cast<size_t>(y)*owner->rowPitch;
                for(UINT x=0;x<owner->width;++x){
                    const bool chooseCyan=((x/64u+y/64u)&1u)!=0u;
                    memcpy(dst+static_cast<size_t>(x)*sizeof(cyan),
                           chooseCyan?cyan:magenta,sizeof(cyan));
                }
            }
            D3D12_RANGE writtenRange{
                0,static_cast<SIZE_T>(desc.Width)};
            owner->upload->Unmap(0,&writtenRange);
        }
    }
    if(FAILED(hr)||!owner->upload){
        owner->stopped=true;
        ptDiffuseGICopyProofOwners.push_back(owner);
        Log("PT_REFLECTION_P2_DGI_COPY_OWNER ready=0 hr=0x%08lX device=%p no_gpu_copy=1",
            static_cast<unsigned long>(hr),device);
        return nullptr;
    }
    ptDiffuseGICopyProofOwners.push_back(owner);
    Log("PT_REFLECTION_P2_DGI_COPY_OWNER ready=1 device=%p upload=%p width=%u height=%u row_pitch=%u rgba16f=1 immutable_upload=1",
        device,owner->upload,owner->width,owner->height,owner->rowPitch);
    return owner;
}

static PTDiffuseGICopyProofOwner* PTDiffuseGIGetCopyProof(ID3D12Device* device) noexcept {
    AcquireSRWLockExclusive(&ptDiffuseGICopyProofLock);
    auto* owner=PTDiffuseGIFindCopyProof(device);
    if(!owner)owner=PTDiffuseGICreateCopyProof(device);
    ReleaseSRWLockExclusive(&ptDiffuseGICopyProofLock);
    return owner;
}

static bool PTDiffuseGICopyMarkerProof(unsigned long long call) noexcept {
    const char* reason="unavailable";
    RRGuideInputSnapshot ctx{},again{};
    PTDiffuseGIBorrowed dgi{},current{};
    ID3D12Device* device=nullptr;
    PTDiffuseGICopyProofOwner* owner=nullptr;
    void* locks[2]{};
    unsigned locked=0;
    bool begun=false,restored=false,copied=false;
    DWORD fault=0,restoreFault=0;
    D3D12_RESOURCE_BARRIER before{},after{};
    UINT x=0,y=0;

    __try {
        reason="renderer_context";
        if(!RRGuideReadRendererContext(&ctx,&reason)||!ctx.commandList||
           !ctx.commandContext||!ctx.recordingLock)__leave;
        reason="gi_texture";
        if(!PTDiffuseGIReadShaderTexture(kRRShaderDiffuseGIColorRva,&dgi))__leave;
        reason="gi_format_or_state";
        if(!PTDiffuseGIResourceShape(dgi.desc,true)||
           dgi.desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||
           dgi.state!=static_cast<unsigned>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS)||
           dgi.desc.Width<512u||dgi.desc.Height<512u||!dgi.tracker)__leave;

        reason="device";
        if(FAILED(ctx.commandList->GetDevice(IID_PPV_ARGS(&device)))||!device)
            __leave;
        reason="immutable_upload";
        owner=PTDiffuseGIGetCopyProof(device);
        if(!owner||!owner->upload)__leave;

        locks[0]=ctx.recordingLock;
        locks[1]=dgi.tracker;
        if(locks[0]==locks[1])locks[1]=nullptr;
        if(locks[1]&&reinterpret_cast<std::uintptr_t>(locks[1])<
                     reinterpret_cast<std::uintptr_t>(locks[0])){
            void* tmp=locks[0];locks[0]=locks[1];locks[1]=tmp;
        }
        reason="native_locks";
        for(unsigned i=0;i<2u;++i){
            if(!locks[i])continue;
            if(!RRGuideTryAcquireNativeLock(locks[i]))__leave;
            locked=i+1u;
        }

        reason="revalidate_context";
        if(!RRGuideReadRendererContext(&again,&reason)||
           again.commandList!=ctx.commandList||
           again.commandContext!=ctx.commandContext||
           again.recordingLock!=ctx.recordingLock||
           again.engineFrame!=ctx.engineFrame)__leave;
        reason="revalidate_dgi";
        if(!PTDiffuseGIReadShaderTexture(kRRShaderDiffuseGIColorRva,&current)||
           current.resource!=dgi.resource||current.tracker!=dgi.tracker||
           current.state!=dgi.state)__leave;

        // Center the checker on screen so it is visible in the chosen scene.
        x=static_cast<UINT>((dgi.desc.Width-owner->width)/2u);
        y=(dgi.desc.Height-owner->height)/2u;
        D3D12_TEXTURE_COPY_LOCATION src{},dest{};
        src.pResource=owner->upload;
        src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Offset=0;
        src.PlacedFootprint.Footprint.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        src.PlacedFootprint.Footprint.Width=owner->width;
        src.PlacedFootprint.Footprint.Height=owner->height;
        src.PlacedFootprint.Footprint.Depth=1u;
        src.PlacedFootprint.Footprint.RowPitch=owner->rowPitch;
        dest.pResource=dgi.resource;
        dest.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dest.SubresourceIndex=0u;

        before.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        before.Transition.pResource=dgi.resource;
        before.Transition.Subresource=0u;
        before.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        before.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;
        after=before;
        after.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;
        after.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        // All three commands run on Control's *existing* renderer list.
        // No descriptor heap, root signature, root arguments or PSO changed.
        // Count them as native renderer commands for its recording bookkeeping.
        reason="record_commands";
        auto* commands=reinterpret_cast<unsigned long long*>(
            reinterpret_cast<unsigned char*>(ctx.commandContext)+8);
        ++*commands;
        ctx.commandList->ResourceBarrier(1u,&before);
        begun=true;
        ++*commands;
        ctx.commandList->CopyTextureRegion(&dest,x,y,0,&src,nullptr);
        ++*commands;
        ctx.commandList->ResourceBarrier(1u,&after);
        restored=true;
        copied=true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
        reason="exception";
    }
    if(begun&&!restored&&ctx.commandList){
        __try {
            auto* commands=reinterpret_cast<unsigned long long*>(
                reinterpret_cast<unsigned char*>(ctx.commandContext)+8);
            ++*commands;
            ctx.commandList->ResourceBarrier(1u,&after);
            restored=true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            restoreFault=GetExceptionCode();
        }
    }

    // Acquire in increasing address order; release in reverse.
    while(locked){
        --locked;
        if(locks[locked])RRGuideReleaseNativeLock(locks[locked]);
    }
    if(device)device->Release();
    if(copied&&restored){
        const auto n=ptDiffuseGICopyProofCopies.fetch_add(1u,std::memory_order_relaxed)+1u;
        if(n<=8u||(n%240u)==0u)
            Log("PT_REFLECTION_P2_DGI_COPY_PROOF call=%llu copy=%llu frame=%llu x=%u y=%u width=%u height=%u dgi_mips=%u dgi_format=%u native_state_restored=1 engine_root_heap_pso_unchanged=1 mip0_only=1 verify_mode=2",
                call,n,ctx.engineFrame,x,y,owner->width,owner->height,
                unsigned(dgi.desc.MipLevels),unsigned(dgi.desc.Format));
        return true;
    }
    const auto skip=ptDiffuseGICopyProofSkips.fetch_add(1u,std::memory_order_relaxed)+1u;
    if(skip<=8u||(skip&(skip-1ull))==0ull)
        Log("PT_REFLECTION_P2_DGI_COPY_PROOF_SKIP call=%llu skip=%llu frame=%llu reason=%s exception=0x%08lX restored=%u restore_exception=0x%08lX no_custom_compute=1 verify_mode=2",
            call,skip,ctx.engineFrame,reason?reason:"unknown",fault,unsigned(restored),restoreFault);
    return false;
}

static bool PTReflectionBridgeDiffuseGI() noexcept {
    const auto call=ptDiffuseGIBridgeCalls.fetch_add(1,std::memory_order_relaxed)+1;
    const unsigned verifyMode=ptReflectionVerifyMode.load(std::memory_order_acquire);
    if(!ptReflectionP2InjectionEnabled||!ptReflectionP2DiffuseBridgeLoaded||
       !(rrNativeRTFrame.effects&control_rr::RTDiffuseGI))return false;

    // Recovery R1: fail closed. The first successful native DGI bridge
    // dispatch at engine frame 2793 was immediately followed by a GPU
    // stall. This path changes the engine's native descriptor heap, compute
    // root signature and PSO but cannot restore its root arguments.
    // Do not execute it again until a state-preserving native PSO path
    // has been built and independently validated.
    //
    // CPU-only telemetry reads already-bound native objects. It creates
    // no PSO, heap or UAV and records no D3D12 commands.
    constexpr bool kPTDiffuseGIReadOnlyRecovery=true;
    if(kPTDiffuseGIReadOnlyRecovery){
        // Move FORWARD without changing Control's compute bindings.
        // Mode 2 copies a diagnostic patch into raw GI on the native list.
        // Other modes keep the previous read-only audit.
        if(verifyMode==2u)return PTDiffuseGICopyMarkerProof(call);
        RRGuideInputSnapshot ctx{};
        PTDiffuseGIBorrowed reflection{},dgi{};
        const char* reason="unknown";
        bool ctxOkay=PTDiffuseGIReadBridgeInputs(&ctx,&reason);
        const bool reflectionOkay=ctxOkay&&
            PTDiffuseGIReadShaderTexture(kRRShaderReflectionTargetRva,&reflection);
        const bool dgiOkay=reflectionOkay&&
            PTDiffuseGIReadShaderTexture(kRRShaderDiffuseGIColorRva,&dgi);
        const bool allReady=dgiOkay&&
            PTDiffuseGIResourceShape(reflection.desc,false)&&
            PTDiffuseGIResourceShape(ctx.gbuffer1.desc,false)&&
            PTDiffuseGIResourceShape(dgi.desc,true);
        if(call<=8u||(call%240u)==0u||(!allReady&&(call&(call-1ull))==0ull)){
            unsigned long long nativeCommands=0;
            DWORD countFault=0;
            if(ctx.commandContext){
                __try {
                    nativeCommands=*reinterpret_cast<const unsigned long long*>(
                        reinterpret_cast<const unsigned char*>(ctx.commandContext)+8);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    countFault=GetExceptionCode();
                }
            }
            Log("PT_REFLECTION_P2_DGI_SAFE_BYPASS call=%llu frame=%llu verify_mode=%u cause=unsafe_compute_state_mutation gpu_commands=0 shader_dispatch=0 context_ok=%u reflection_ok=%u dgi_ok=%u resource_shape_ok=%u context_reason=%s reflection_state=0x%X dgi_state=0x%X reflection_mips=%u dgi_mips=%u native_command_count=%llu count_fault=0x%08lX action=keep_p2_reflections_enabled",
                call,ctx.engineFrame,verifyMode,unsigned(ctxOkay),
                unsigned(reflectionOkay),unsigned(dgiOkay),unsigned(allReady),
                reason?reason:"unknown",reflection.state,dgi.state,
                reflection.desc.MipLevels,dgi.desc.MipLevels,
                nativeCommands,countFault);
        }
        return false;
    }
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
                Log("PT_REFLECTION_P2_DGI_MIP0_CONTRACT call=%llu reflection_mips=%u diffuse_gi_mips=%u reflection_state=0x%X dgi_state=0x%X srv_mip=0 uav_mip=0 barrier_subresource=0 verified_shape=%u",
                    call,unsigned(reflection.desc.MipLevels),unsigned(dgi.desc.MipLevels),
                    reflection.state,dgi.state,
                    unsigned(strcmp(reflectionShape,"ready")==0&&strcmp(gbufferShape,"ready")==0&&strcmp(dgiShape,"ready")==0));
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
        sd.Texture2D.MostDetailedMip=0;
        sd.Texture2D.MipLevels=1;
        sd.Texture2D.PlaneSlice=0;
        sd.Texture2D.ResourceMinLODClamp=0.0f;
        sd.Format=reflection.desc.Format;
        owner->device->CreateShaderResourceView(reflection.resource,&sd,cpu);
        cpu.ptr+=owner->descriptorSize;
        sd.Format=input.gbuffer1.desc.Format;
        owner->device->CreateShaderResourceView(input.gbuffer1.resource,&sd,cpu);
        cpu.ptr+=owner->descriptorSize;
        D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        ud.Texture2D.MipSlice=0;
        ud.Texture2D.PlaneSlice=0;
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
            // Only mip 0 is read/written. Other mips may have independent
            // native GPU states; leave them completely untouched.
            b.Transition.Subresource=0;
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
            Log("PT_REFLECTION_P2_DGI_BRIDGE call=%llu dispatch=%llu frame=%llu output=%llux%u reflection=%llux%u gbuffer=%llux%u strength=0.45 roughness_gate=0.42_0.88 destination=raw_diffuse_gi_color resource_states_restored=1 mip0_views=1 verify_mode=%u marker=%u",
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
