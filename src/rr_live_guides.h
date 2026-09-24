#pragma once
#include "rr_frame_slots.h"
#include "rr_live_frame_input.h"
#include "rr_reflectance_projection.h"
#include "../build/rr_live_compiled.h"
// Owned per-frame candidate textures. No readback/upload loop, no RR tagging.
// Included after rr_albedo_capture.h so exact native Part1 readers are available.
#include "rr_live_capture.h"
struct RRLiveSlot {
    ID3D12DescriptorHeap* heap=nullptr;
    ID3D12Resource* normal=nullptr;ID3D12Resource* specular=nullptr;
    ID3D12Resource* diffuse=nullptr;
    bool diffuseValid=false;
    CameraSnapshot camera{};
    ID3D12Resource* sources[4]{};
    unsigned long long present=0;
    control_rr::LiveFrameInput frameInput{};
};
struct RRLiveOwner {
    ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;
    ID3D12Fence* fence=nullptr;ID3D12RootSignature* root=nullptr;ID3D12PipelineState* pipeline=nullptr;
    RRLiveCapture* capture=nullptr;
    RRLiveSlot slots[3]{};control_rr::FrameSlots policy;
    UINT width=0,height=0,requestedWidth=0,requestedHeight=0,increment=0;
    unsigned long long epoch=0,signal=0,recorded=0,retired=0,skipped=0,guideFrames=0;
    bool preparing=false,prepared=false,stopped=false;
    control_rr::FenceKey Key() const noexcept {return {reinterpret_cast<std::uintptr_t>(device),reinterpret_cast<std::uintptr_t>(queue),reinterpret_cast<std::uintptr_t>(fence)};}
};
// r21t retains the r21s production owned-guide readback recorder.
#include "rr_live_capture_record.h"
static SRWLOCK rrLiveLock=SRWLOCK_INIT;
static unsigned long long rrLiveContextSkips=0;
static RRLiveOwner* rrLive=nullptr; // bounded owner retained through process exit
// r21y production: the r21s one-shot 4K guide readback was diagnostic-only
// (~199 MB at 4K). Keep the recorder source for offline/debug builds, but do
// not allocate or record readbacks in the production runtime.
static constexpr bool rrLiveGuideStatsEnabled=false;
static bool rrLiveCaptureRRFrame=false;
static void RRLiveSkip(RRLiveOwner* c,const char* reason) noexcept {
    const auto n=++c->skipped;
    if(n<=4 || (n&(n-1))==0) Log("RR_G12_LIVE_SKIP count=%llu reason=%s stage=guide_generation",n,reason);
}
static bool RRLiveAllFree(RRLiveOwner* c) noexcept {
    for(const auto& s:c->policy.Inspect()) if(s.state!=control_rr::SlotState::Free) return false;
    return true;
}
static void RRLiveReleaseSources(RRLiveSlot& s) noexcept {for(auto*& p:s.sources) RRGuideRelease(p);}
static HRESULT RRLiveMakeTexture(RRLiveOwner* c,ID3D12Resource** out,bool=false,bool=false) noexcept {
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=c->requestedWidth;d.Height=c->requestedHeight;
    d.DepthOrArraySize=d.MipLevels=1;d.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;d.SampleDesc.Count=1;
    d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    return c->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(out));
}

static void RRLiveCaptureRelease(RRLiveOwner* c) noexcept {
    if(!c||!c->capture)return;for(auto*& r:c->capture->readback)RRGuideRelease(r);delete c->capture;c->capture=nullptr;
}
static void RRLiveCapturePrepare(RRLiveOwner* c) noexcept {
    if(c->capture){
        if(c->capture->width==c->requestedWidth&&c->capture->height==c->requestedHeight)return;
        if(c->capture->policy.state!=control_rr::CaptureState::Armed)return; // completed/active capture is retained until exit
        RRLiveCaptureRelease(c);
    }
    if(std::uint64_t(c->requestedWidth)*c->requestedHeight>control_rr::MaxDiagnosticPixels){
        Log("RR_G12_LIVE_CAPTURE_SKIPPED reason=diagnostic_size_limit width=%u height=%u rr_guides_unaffected=1",c->requestedWidth,c->requestedHeight);return;
    }
    auto* cap=new(std::nothrow) RRLiveCapture;if(!cap)return;c->capture=cap;
    cap->width=c->requestedWidth;cap->height=c->requestedHeight;
    ID3D12Resource* outputs[3]{c->slots[0].normal,c->slots[0].specular,c->slots[0].diffuse};
    for(UINT i=0;i<3;++i){
        const auto source=outputs[i]->GetDesc();c->device->GetCopyableFootprints(&source,0,1,0,&cap->footprint[i],nullptr,nullptr,&cap->bytes[i]);
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;hp.CreationNodeMask=hp.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=cap->bytes[i];desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(!cap->bytes[i]||FAILED(c->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&cap->readback[i])))){cap->policy.state=control_rr::CaptureState::Failed;Log("RR_G12_LIVE_CAPTURE_FAILED reason=allocation stage=guide_generation");return;}
    }
    Log("RR_G12_LIVE_CAPTURE_READY width=%u height=%u stats_only=1 readback_bytes=%llu",cap->width,cap->height,cap->bytes[0]+cap->bytes[1]+cap->bytes[2]);
}
static DWORD WINAPI RRLivePrepare(void* context) noexcept {
    auto* c=static_cast<RRLiveOwner*>(context);
    AcquireSRWLockExclusive(&rrLiveLock);
    D3D12_FEATURE_DATA_FORMAT_SUPPORT floatSupport{};floatSupport.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
    HRESULT hr=c->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&floatSupport,sizeof(floatSupport));
    if(SUCCEEDED(hr)&&!(floatSupport.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))hr=E_NOTIMPL;
    if(SUCCEEDED(hr)&&!c->root){
        // r21t: match the supplied RenoDX RRG root contract exactly:
        // [0] = four b0 constants, [1] = one descriptor table {t0..t3,u0..u2}.
        // In particular, MaterialDataPart1 is a bounded StructuredBuffer SRV at t2
        // instead of an unbounded root GPU VA.
        D3D12_DESCRIPTOR_RANGE ranges[2]{};
        ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=4;ranges[0].BaseShaderRegister=0;ranges[0].OffsetInDescriptorsFromTableStart=0;
        ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=3;ranges[1].BaseShaderRegister=0;ranges[1].OffsetInDescriptorsFromTableStart=4;
        D3D12_ROOT_PARAMETER params[2]{};
        params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[0].Constants.Num32BitValues=4;params[0].Constants.ShaderRegister=0;
        params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[1].DescriptorTable={2,ranges};
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MipLODBias=0.0f;sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.BorderColor=D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;sampler.MinLOD=0.0f;sampler.MaxLOD=D3D12_FLOAT32_MAX;
        sampler.ShaderRegister=0;sampler.RegisterSpace=0;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        D3D12_ROOT_SIGNATURE_DESC d{};d.NumParameters=2;d.pParameters=params;d.NumStaticSamplers=1;d.pStaticSamplers=&sampler;
        auto serialize=reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
        ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
        hr=serialize?serialize(&d,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors):E_NOINTERFACE;
        if(SUCCEEDED(hr))hr=blob?c->device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&c->root)):E_UNEXPECTED;
        RRGuideRelease(blob);RRGuideRelease(errors);
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=c->root;pd.CS={kRRLiveShader,sizeof(kRRLiveShader)};
        if(SUCCEEDED(hr))hr=c->device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&c->pipeline));
        if(SUCCEEDED(hr))hr=c->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c->fence));
        if(SUCCEEDED(hr)&&!c->policy.Bind(c->Key()))hr=E_UNEXPECTED;
        c->increment=c->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    if(SUCCEEDED(hr)&&RRLiveAllFree(c)){
        for(auto& s:c->slots){
            RRLiveReleaseSources(s);RRGuideRelease(s.normal);RRGuideRelease(s.specular);RRGuideRelease(s.diffuse);RRGuideRelease(s.heap);
            hr=RRLiveMakeTexture(c,&s.normal);if(SUCCEEDED(hr))hr=RRLiveMakeTexture(c,&s.diffuse);if(SUCCEEDED(hr))hr=RRLiveMakeTexture(c,&s.specular);
            D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=7;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if(SUCCEEDED(hr))hr=c->device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&s.heap));if(FAILED(hr))break;
            auto handle=s.heap->GetCPUDescriptorHandleForHeapStart();handle.ptr+=SIZE_T(c->increment)*4;
            D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
            c->device->CreateUnorderedAccessView(s.normal,nullptr,&ud,handle);handle.ptr+=c->increment;
            c->device->CreateUnorderedAccessView(s.diffuse,nullptr,&ud,handle);handle.ptr+=c->increment;
            c->device->CreateUnorderedAccessView(s.specular,nullptr,&ud,handle);
        }
        if(SUCCEEDED(hr)&&rrLiveGuideStatsEnabled)RRLiveCapturePrepare(c);
    }else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
    c->preparing=false;c->prepared=SUCCEEDED(hr);c->stopped=FAILED(hr);
    if(c->prepared){c->width=c->requestedWidth;c->height=c->requestedHeight;c->guideFrames=0;++c->epoch;}
    Log("RR_G12_LIVE_PREPARED success=%u hr=0x%08lX width=%u height=%u epoch=%llu slots=3 stage=guide_generation architecture=renodx_descriptor_parity formats=normal_rgba16f,diffuse_material_rgba16f,specular_envbrdf_rgba16f",
        unsigned(c->prepared),static_cast<unsigned long>(hr),c->width,c->height,c->epoch);
    ReleaseSRWLockExclusive(&rrLiveLock);return 0;
}

static void RRLiveQueuePrepare(RRLiveOwner* c,UINT width,UINT height) noexcept {
    if(c->preparing || !RRLiveAllFree(c)) return;
    c->requestedWidth=width;c->requestedHeight=height;c->preparing=true;c->prepared=false;
    if(!QueueUserWorkItem(&RRLivePrepare,c,WT_EXECUTEDEFAULT)) {c->preparing=false;c->stopped=true;RRLiveSkip(c,"worker_queue_failed");}
}
#include "rr_live_record.h"
static bool RRLiveSameInput(const RRGuideInputSnapshot& a,const RRGuideInputSnapshot& b) noexcept {
    return RRPart1SameContext(a,b) && a.gbuffer1.resource==b.gbuffer1.resource && a.gbuffer2.resource==b.gbuffer2.resource &&
        a.gbuffer1.stateTracker==b.gbuffer1.stateTracker && a.gbuffer2.stateTracker==b.gbuffer2.stateTracker &&
        a.gbuffer1.trackedState==b.gbuffer1.trackedState && a.gbuffer2.trackedState==b.gbuffer2.trackedState &&
        a.camera.renderer==b.camera.renderer && !memcmp(a.camera.viewToWorld,b.camera.viewToWorld,sizeof(a.camera.viewToWorld)) &&
        !memcmp(a.camera.viewToClip,b.camera.viewToClip,sizeof(a.camera.viewToClip));
}
static bool RRLiveSameTexture(const RRGuideInputTexture& a,const RRGuideInputTexture& b) noexcept {
    return a.resource==b.resource && a.stateTracker==b.stateTracker && a.trackedState==b.trackedState &&
        a.desc.Dimension==b.desc.Dimension && a.desc.Format==b.desc.Format && a.desc.Width==b.desc.Width &&
        a.desc.Height==b.desc.Height && a.desc.MipLevels==b.desc.MipLevels && a.desc.DepthOrArraySize==b.desc.DepthOrArraySize;
}
static void RRLiveBeforeImpl(ID3D12GraphicsCommandList* list,const control_rr::LiveFrameInput& frameInput) noexcept {
    RRGuideInputSnapshot before{},in{};RRGuideInputTexture envBefore{},env{};
    const char* reason="unavailable";DWORD envFault=0;
    if(!RRGuideReadInputs(&before,&reason) || before.commandList!=list || !frameInput.Matches(before.engineFrame) ||
       !RRGuideReadEnvBRDF(&envBefore,&reason,&envFault)) {
        if(before.commandList && before.commandList!=list) reason="evaluation_list_mismatch";
        else if(before.engineFrame && !frameInput.Matches(before.engineFrame)) reason="native_frame_metadata_unavailable_or_mismatched";
        const auto n=++rrLiveContextSkips;
        if(n<=4 || (n&(n-1))==0) Log("RR_G12_LIVE_CONTEXT_SKIP count=%llu reason=%s fault=0x%08lX stage=guide_generation architecture=renodx_descriptor_parity",n,reason,envFault);
        return;
    }

    // Lock the three native texture-state trackers in address order, then the
    // renderer recording lock.  The sources remain in their native compute-SRV
    // state; this pass never transitions borrowed Control resources.
    void* sourceLocks[3]{before.gbuffer1.stateTracker,before.gbuffer2.stateTracker,envBefore.stateTracker};
    for(UINT i=0;i<3;++i)for(UINT j=i+1;j<3;++j)
        if(reinterpret_cast<std::uintptr_t>(sourceLocks[j])<reinterpret_cast<std::uintptr_t>(sourceLocks[i])){
            void* t=sourceLocks[i];sourceLocks[i]=sourceLocks[j];sourceLocks[j]=t;
        }
    void* uniqueLocks[3]{};UINT uniqueCount=0;
    for(UINT i=0;i<3;++i){
        if(!sourceLocks[i])continue;
        if(!uniqueCount||sourceLocks[i]!=uniqueLocks[uniqueCount-1])uniqueLocks[uniqueCount++]=sourceLocks[i];
    }
    bool held[3]{};UINT heldCount=0;bool recordingHeld=false;
    __try {
        for(UINT i=0;i<uniqueCount;++i){
            if(!RRGuideTryAcquireNativeLock(uniqueLocks[i]))__leave;
            held[heldCount++]=true;
        }
        bool recordingAlreadyHeld=false;
        for(UINT i=0;i<uniqueCount;++i)if(uniqueLocks[i]==before.recordingLock)recordingAlreadyHeld=true;
        if(!recordingAlreadyHeld){recordingHeld=RRGuideTryAcquireNativeLock(before.recordingLock);if(!recordingHeld)__leave;}

        if(!RRGuideReadInputs(&in,&reason) || !RRLiveSameInput(before,in) || !frameInput.Matches(in.engineFrame) ||
           !RRGuideReadEnvBRDF(&env,&reason,&envFault) || !RRLiveSameTexture(envBefore,env)) __leave;
        if(!(in.gbuffer1.trackedState&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) ||
           !(in.gbuffer2.trackedState&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) ||
           !(env.trackedState&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) ||
           (in.gbuffer1.desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) ||
           (in.gbuffer2.desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) ||
           (env.desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) {
            if(rrLive)RRLiveSkip(rrLive,"sources_not_compute_readable");__leave;
        }

        if(!rrLive) {
            auto* c=new(std::nothrow) RRLiveOwner;if(!c)__leave;
            if(FAILED(list->GetDevice(IID_PPV_ARGS(&c->device)))){delete c;__leave;}
            c->queue=in.queue;c->queue->AddRef();rrLive=c;
        }
        auto* c=rrLive;if(c->stopped||c->preparing)__leave;
        if(c->queue!=in.queue || !RRGuideResourcesShareDevice(&in,c->slots[0].normal,c->slots[0].specular)){
            RRLiveSkip(c,"device_or_queue_changed");__leave;
        }
        ID3D12Device* envDevice=nullptr;
        const HRESULT envDeviceHR=env.resource->GetDevice(IID_PPV_ARGS(&envDevice));
        const bool envSameDevice=SUCCEEDED(envDeviceHR)&&envDevice==c->device;RRGuideRelease(envDevice);
        if(!envSameDevice){RRLiveSkip(c,"envbrdf_device_mismatch");__leave;}

        const UINT width=static_cast<UINT>(in.gbuffer1.desc.Width),height=in.gbuffer1.desc.Height;
        if(!frameInput.MatchesExtent(width,height)){RRLiveSkip(c,"native_color_guide_extent_mismatch");__leave;}
        if(!c->prepared||width!=c->width||height!=c->height){RRLiveQueuePrepare(c,width,height);__leave;}

        const auto sample=control_rr_part1::Observe(&RRAlbedoRead,reinterpret_cast<std::uintptr_t>(in.camera.renderer));
        const auto part=RRPart1ReadResource(sample);
        if(!part.read||!part.descriptorMatched||part.fault||part.heapType!=1||part.nativeState!=0xC0||
           (part.flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)||!sample.metadata.bytes||sample.metadata.bytes>524288||
           (sample.metadata.bytes%8)!=0){RRLiveSkip(c,"part1_unavailable");__leave;}
        auto* material=reinterpret_cast<ID3D12Resource*>(part.resource);
        ID3D12Device* materialDevice=nullptr;
        const HRESULT materialDeviceHR=material->GetDevice(IID_PPV_ARGS(&materialDevice));
        const bool materialSameDevice=SUCCEEDED(materialDeviceHR)&&materialDevice==c->device;RRGuideRelease(materialDevice);
        if(!materialSameDevice){RRLiveSkip(c,"part1_device_mismatch");__leave;}

        control_rr::Lease lease{};
        if(!c->policy.Begin({in.engineFrame,c->epoch,width,height},lease)){RRLiveSkip(c,"slots_busy_or_duplicate_frame");__leave;}
        auto& s=c->slots[lease.index];RRLiveReleaseSources(s);
        s.present=in.presentToken;s.frameInput=frameInput;s.diffuseValid=false;s.camera=in.camera;
        ID3D12Resource* inputs[4]{in.gbuffer1.resource,in.gbuffer2.resource,material,env.resource};
        for(UINT i=0;i<4;++i){inputs[i]->AddRef();s.sources[i]=inputs[i];}

        auto handle=s.heap->GetCPUDescriptorHandleForHeapStart();
        D3D12_SHADER_RESOURCE_VIEW_DESC gbufferSRV{};gbufferSRV.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        gbufferSRV.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;gbufferSRV.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        gbufferSRV.Texture2D.MipLevels=1;
        c->device->CreateShaderResourceView(in.gbuffer1.resource,&gbufferSRV,handle);handle.ptr+=c->increment;
        c->device->CreateShaderResourceView(in.gbuffer2.resource,&gbufferSRV,handle);handle.ptr+=c->increment;
        // r21t RenoDX parity: t2 is a real bounded StructuredBuffer<uint2> SRV.
        // The captured Part1 descriptor is already required to be an 8-byte-record
        // buffer, so expose exactly that many records to the shader.
        D3D12_SHADER_RESOURCE_VIEW_DESC materialSRV{};materialSRV.Format=DXGI_FORMAT_UNKNOWN;
        materialSRV.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;materialSRV.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        materialSRV.Buffer.FirstElement=0;materialSRV.Buffer.NumElements=static_cast<UINT>(sample.metadata.bytes/8);
        materialSRV.Buffer.StructureByteStride=8;materialSRV.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_NONE;
        c->device->CreateShaderResourceView(material,&materialSRV,handle);handle.ptr+=c->increment;
        D3D12_SHADER_RESOURCE_VIEW_DESC envSRV{};envSRV.Format=RRGuideEnvViewFormat(env.desc.Format);
        envSRV.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;envSRV.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        envSRV.Texture2D.MostDetailedMip=0;envSRV.Texture2D.MipLevels=env.desc.MipLevels;envSRV.Texture2D.ResourceMinLODClamp=0.0f;
        c->device->CreateShaderResourceView(env.resource,&envSRV,handle);

        RRLiveConstants constants{};constants.invertSmoothness=1;constants.guideFlags=3;constants.width=width;constants.height=height;
        if(!c->policy.CommandsStarted(lease)){c->policy.Abort(lease);RRLiveReleaseSources(s);__leave;}
        DWORD fault=0;const auto perf=RRPerfBegin(in,RRPerfStage::LiveGuides);
        if(!RRLiveRecord(c,&s,&in,&constants,&fault)){
            c->policy.Abort(lease);RRLiveReleaseSources(s);c->stopped=true;
            Log("RR_G12_LIVE_STOP reason=recording_fault fault=0x%08lX resources_retained=1 stage=guide_generation architecture=renodx_descriptor_parity",fault);__leave;
        }
        // r21t retains the one-shot, stats-only readback of the exact owned guide textures
        // consumed by RR. It records after guide generation and restores the
        // outputs to NPS-SRV before native evaluation. No borrowed game resource
        // is transitioned or copied by this diagnostic.
        if(rrLiveGuideStatsEnabled&&rrLiveCaptureRRFrame&&c->capture&&c->capture->policy.Begin(unsigned(lease.index),lease.serial)){
            c->capture->frameInput=frameInput;c->capture->camera=in.camera;c->capture->present=in.presentToken;
            c->capture->gbuffer1=reinterpret_cast<std::uintptr_t>(in.gbuffer1.resource);
            c->capture->gbuffer2=reinterpret_cast<std::uintptr_t>(in.gbuffer2.resource);
            c->capture->material=reinterpret_cast<std::uintptr_t>(material);
            c->capture->envbrdf=reinterpret_cast<std::uintptr_t>(env.resource);
            c->capture->materialBytes=sample.metadata.bytes;c->capture->envFormat=env.desc.Format;
            c->capture->envWidth=static_cast<UINT>(env.desc.Width);c->capture->envHeight=env.desc.Height;
            DWORD captureFault=0;const bool captureOkay=RRLiveCaptureRecord(c->capture,&s,&in,&captureFault);
            c->capture->policy.Recorded(captureOkay);
            Log("RR_G12_LIVE_CAPTURE_RECORDED frame=%llu slot=%u serial=%llu success=%u fault=0x%08lX stats_only=1",
                in.engineFrame,unsigned(lease.index),lease.serial,unsigned(captureOkay),captureFault);
        }
        s.diffuseValid=true;++c->guideFrames;RRPerfEnd(perf);
        c->policy.GuidesReady(lease);const auto n=++c->recorded;
        if(n<=4||(n&(n-1))==0)
            Log("RR_G12_LIVE_DISPATCH count=%llu frame=%llu slot=%u epoch=%llu width=%u height=%u source=gbuffer1_gbuffer2_part1_envbrdf guide_arch=renodx_descriptor_parity material_binding=bounded_descriptor normal_space=view diffuse_semantic=material_value specular_semantic=native_envbrdf guide_frames=%llu reset=%d stage=guide_generation",
                n,in.engineFrame,unsigned(lease.index),c->epoch,width,height,c->guideFrames,s.frameInput.reset);
    } __finally {
        if(recordingHeld)RRGuideReleaseNativeLock(before.recordingLock);
        while(heldCount){--heldCount;RRGuideReleaseNativeLock(uniqueLocks[heldCount]);}
    }
}
static void RRLiveBeforeEvaluation(ID3D12GraphicsCommandList* list,const control_rr::LiveFrameInput& frameInput) noexcept {
    if(!TryAcquireSRWLockExclusive(&rrLiveLock)) return;
    __try {RRLiveBeforeImpl(list,frameInput);}
    __except(EXCEPTION_EXECUTE_HANDLER) {if(rrLive)rrLive->stopped=true;Log("RR_G12_LIVE_STOP reason=preflight_exception resources_retained=1 stage=guide_generation");}
    ReleaseSRWLockExclusive(&rrLiveLock);
}
#include "rr_live_retire.h"
