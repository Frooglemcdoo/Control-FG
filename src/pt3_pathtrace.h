#pragma once
#include "../build/pt3_pathtrace_compiled.h"
#include "fg_ui_device_identity.h"
#include <d3dcompiler.h>

namespace control_pt3 {

static std::atomic<unsigned int> enabled{0};
static std::atomic<unsigned int> viewMode{0};
static std::atomic<unsigned long long> stateGeneration{0};
static bool f7Down=false;

static std::atomic<unsigned long long> attempts{0},successes{0},skips{0},composites{0},compositeSkips{0};
static std::atomic<unsigned long long> statActualRays{0},statSampledRays{0},statHits{0},statMisses{0},statFrame{0};
static std::atomic<unsigned int> statMinFixed{0},statMaxFixed{0},statSumFixed{0},statChecksum{0};

static const char* ModeName(unsigned m) noexcept {
    switch(m){case 0:return "beauty";case 1:return "hit_miss";case 2:return "hit_distance";case 3:return "instance_id";default:return "beauty";}
}
static const wchar_t* ModeNameWide(unsigned m) noexcept {
    switch(m){case 0:return L"BEAUTY";case 1:return L"HIT / MISS";case 2:return L"HIT DISTANCE";case 3:return L"INSTANCE ID";default:return L"BEAUTY";}
}

using PT3D3DCompileFn = HRESULT (WINAPI*)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO*,ID3DInclude*,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob**,ID3DBlob**);
static PT3D3DCompileFn PT3ResolveD3DCompile() noexcept {
    static HMODULE compiler=nullptr;
    static PT3D3DCompileFn fn=nullptr;
    if(fn)return fn;
    if(!compiler)compiler=LoadLibraryW(L"d3dcompiler_47.dll");
    if(compiler)fn=reinterpret_cast<PT3D3DCompileFn>(GetProcAddress(compiler,"D3DCompile"));
    return fn;
}
static HRESULT PT3Compile(LPCVOID source,SIZE_T bytes,LPCSTR sourceName,LPCSTR entry,LPCSTR target,ID3DBlob** blob,ID3DBlob** errors) noexcept {
    auto fn=PT3ResolveD3DCompile();
    if(!fn){if(blob)*blob=nullptr;if(errors)*errors=nullptr;return E_NOINTERFACE;}
    return fn(source,bytes,sourceName,nullptr,nullptr,entry,target,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,blob,errors);
}

static bool IsEnabled() noexcept { return enabled.load(std::memory_order_acquire)!=0; }
static unsigned ViewMode() noexcept { unsigned m=viewMode.load(std::memory_order_acquire); return m<=3?m:0; }

static void ResetStats() noexcept {
    statActualRays.store(0);statSampledRays.store(0);statHits.store(0);statMisses.store(0);statFrame.store(0);
    statMinFixed.store(0);statMaxFixed.store(0);statSumFixed.store(0);statChecksum.store(0);
}

static void SetEnabled(bool value,const char* source) noexcept {
    const unsigned next=value?1u:0u;
    const unsigned previous=enabled.exchange(next,std::memory_order_acq_rel);
    if(previous==next)return;
    const auto generation=++stateGeneration;
    ResetStats();
    Log("PT3_TOGGLE previous=%u state=%s enabled=%u source=%s generation=%llu dispatch=%s composite=%s history_reset=1",
        previous,value?"on":"off",next,source?source:"unknown",generation,value?"enabled":"disabled",value?"enabled":"disabled");
}
static void SetViewMode(unsigned value,const char* source) noexcept {
    if(value>3)value=0;
    const unsigned previous=viewMode.exchange(value,std::memory_order_acq_rel);
    if(previous==value)return;
    const auto generation=++stateGeneration;
    ResetStats();
    Log("PT3_MODE previous=%u mode=%u name=%s source=%s generation=%llu history_reset=1",
        previous,value,ModeName(value),source?source:"unknown",generation);
}

struct Constants {
    float clipToWorld[16];
    float cameraPosTMin[4];
    float viewToWorldBasisX[4];
    float viewToWorldBasisY[4];
    float viewToWorldBasisZ[4];
    UINT outputSize[2];
    UINT mode;
    UINT counterStride;
    UINT frameIndex;
    UINT hdrActive;
    UINT materialCount;
    UINT spp;
};
static_assert(sizeof(Constants)==160,"PT3 root constant ABI");

struct CompositeSlot {
    ID3D12CommandAllocator* allocator=nullptr;
    ID3D12GraphicsCommandList* list=nullptr;
    ID3D12Resource* readback=nullptr;
    ID3D12Resource* nativeCopy=nullptr;
    UINT64 nativeWidth=0;
    UINT nativeHeight=0;
    DXGI_FORMAT nativeFormat=DXGI_FORMAT_UNKNOWN;
    UINT64 fenceValue=0;
    unsigned long long sourceFrame=0;
    unsigned long long actualRays=0;
    unsigned mode=0;
};

struct CompositePso {
    DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
    ID3D12PipelineState* pso=nullptr;
};

struct Owner {
    ID3D12Device* device=nullptr;

    ID3D12RootSignature* rayRoot=nullptr;
    ID3D12PipelineState* rayPipeline=nullptr;
    ID3D12DescriptorHeap* rayHeap=nullptr;
    UINT rayIncrement=0;
    ID3D12Resource* output=nullptr;
    ID3D12Resource* counters=nullptr;
    ID3D12Resource* zeroUpload=nullptr;
    ID3D12Resource* fallbackMaterial=nullptr;
    UINT outputWidth=0,outputHeight=0;
    std::vector<ID3D12Resource*> retiredOutputs;

    ID3D12RootSignature* compositeRoot=nullptr;
    ID3D12DescriptorHeap* compositeSrvHeap=nullptr;
    ID3D12DescriptorHeap* compositeRtvHeap=nullptr;
    UINT compositeSrvIncrement=0,compositeRtvIncrement=0;
    CompositePso compositePsos[4]{};
    CompositeSlot compositeSlots[3]{};
    ID3D12Fence* compositeFence=nullptr;
    UINT64 nextFenceValue=0;

    bool ready=false,failed=false,compositeReady=false;
    D3D12_GPU_VIRTUAL_ADDRESS lastTlas=0;
    unsigned long long lastRayFrame=0;
    unsigned long long lastActualRays=0;
    unsigned lastMode=0;
};
static Owner owner{};

static bool SameDevice(ID3D12Device* a,ID3D12Device* b) noexcept {
    const auto resolveNative=[](IUnknown* object,IUnknown** native) noexcept {
        void* raw=nullptr;
        const bool ok=slGetNativeInterfaceApi&&slGetNativeInterfaceApi(object,&raw)==sl::Result::eOk;
        *native=reinterpret_cast<IUnknown*>(raw);
        return ok;
    };
    return FGUISameDevice(a,b,resolveNative);
}

static D3D12_RESOURCE_DESC BufferDesc(UINT64 bytes,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE) noexcept {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;
    d.Format=DXGI_FORMAT_UNKNOWN;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;return d;
}

static bool CreateRayResources(ID3D12Device* device,UINT width,UINT height) noexcept {
    if(!device||!width||!height)return false;
    if(owner.output&&owner.outputWidth==width&&owner.outputHeight==height&&owner.counters&&owner.zeroUpload)return true;

    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;td.DepthOrArraySize=1;td.MipLevels=1;
    td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.SampleDesc.Count=1;td.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ID3D12Resource* output=nullptr;
    HRESULT hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&output));
    if(FAILED(hr)||!output){Log("PT3_RESOURCE_FAIL stage=output hr=0x%08lX width=%u height=%u",static_cast<unsigned long>(hr),width,height);return false;}

    if(!owner.counters){
        auto bd=BufferDesc(48,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&owner.counters));
        if(FAILED(hr)||!owner.counters){output->Release();Log("PT3_RESOURCE_FAIL stage=counters hr=0x%08lX",static_cast<unsigned long>(hr));return false;}

        D3D12_HEAP_PROPERTIES up{};up.Type=D3D12_HEAP_TYPE_UPLOAD;
        auto zd=BufferDesc(48);
        hr=device->CreateCommittedResource(&up,D3D12_HEAP_FLAG_NONE,&zd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&owner.zeroUpload));
        if(FAILED(hr)||!owner.zeroUpload){output->Release();Log("PT3_RESOURCE_FAIL stage=zero_upload hr=0x%08lX",static_cast<unsigned long>(hr));return false;}
        void* mapped=nullptr;D3D12_RANGE readRange{0,0};
        if(SUCCEEDED(owner.zeroUpload->Map(0,&readRange,&mapped))&&mapped){
            const UINT init[12]={0,0,0,0xFFFFFFFFu,0,0,0,0,0,0,0,0};
            std::memcpy(mapped,init,sizeof(init));owner.zeroUpload->Unmap(0,nullptr);
        } else {output->Release();Log("PT3_RESOURCE_FAIL stage=zero_map");return false;}

        D3D12_HEAP_PROPERTIES mup{};mup.Type=D3D12_HEAP_TYPE_UPLOAD;
        auto md=BufferDesc(8);
        hr=device->CreateCommittedResource(&mup,D3D12_HEAP_FLAG_NONE,&md,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&owner.fallbackMaterial));
        if(FAILED(hr)||!owner.fallbackMaterial){output->Release();Log("PT3_RESOURCE_FAIL stage=fallback_material hr=0x%08lX",static_cast<unsigned long>(hr));return false;}
        void* mm=nullptr;
        if(SUCCEEDED(owner.fallbackMaterial->Map(0,&readRange,&mm))&&mm){
            const UINT packed[2]={0x80808000u,0u};std::memcpy(mm,packed,sizeof(packed));owner.fallbackMaterial->Unmap(0,nullptr);
        } else {output->Release();Log("PT3_RESOURCE_FAIL stage=fallback_material_map");return false;}
    }

    if(owner.output)owner.retiredOutputs.push_back(owner.output);
    owner.output=output;owner.outputWidth=width;owner.outputHeight=height;
    Log("PT3_RESOURCE_READY output=%p counters=%p size=%ux%u format=%u counter_bytes=48 counter_stride_pixels=16",
        owner.output,owner.counters,width,height,unsigned(DXGI_FORMAT_R16G16B16A16_FLOAT));
    return true;
}

static bool InitializeRay(ID3D12Device* device) noexcept {
    if(owner.failed)return false;
    if(owner.ready){
        if(SameDevice(owner.device,device))return true;
        Log("PT3_FAIL stage=device_changed");owner.failed=true;return false;
    }
    if(!device||!kPT3PathTraceShaderSize){owner.failed=true;return false;}

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 opt5{};
    HRESULT hr=device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&opt5,sizeof(opt5));
    if(FAILED(hr)||opt5.RaytracingTier<D3D12_RAYTRACING_TIER_1_1){
        Log("PT3_FAIL stage=raytracing_tier hr=0x%08lX tier=%u",static_cast<unsigned long>(hr),unsigned(opt5.RaytracingTier));owner.failed=true;return false;
    }

    D3D12_DESCRIPTOR_RANGE srv{};srv.RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;srv.NumDescriptors=4;srv.BaseShaderRegister=0;srv.OffsetInDescriptorsFromTableStart=0;
    D3D12_DESCRIPTOR_RANGE uavs{};uavs.RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;uavs.NumDescriptors=2;uavs.BaseShaderRegister=0;uavs.OffsetInDescriptorsFromTableStart=0;
    D3D12_ROOT_PARAMETER params[3]{};
    params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable.NumDescriptorRanges=1;params[0].DescriptorTable.pDescriptorRanges=&srv;
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[1].DescriptorTable.NumDescriptorRanges=1;params[1].DescriptorTable.pDescriptorRanges=&uavs;
    params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants.ShaderRegister=0;params[2].Constants.Num32BitValues=40;
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=3;rd.pParameters=params;
    ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
    hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);
    if(FAILED(hr)||!blob){Log("PT3_FAIL stage=serialize_ray_root hr=0x%08lX error=%s",static_cast<unsigned long>(hr),errors?static_cast<const char*>(errors->GetBufferPointer()):"none");if(errors)errors->Release();if(blob)blob->Release();owner.failed=true;return false;}
    hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&owner.rayRoot));blob->Release();if(errors)errors->Release();
    if(FAILED(hr)||!owner.rayRoot){Log("PT3_FAIL stage=create_ray_root hr=0x%08lX",static_cast<unsigned long>(hr));owner.failed=true;return false;}

    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=owner.rayRoot;pd.CS.pShaderBytecode=kPT3PathTraceShader;pd.CS.BytecodeLength=kPT3PathTraceShaderSize;
    hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&owner.rayPipeline));
    if(FAILED(hr)||!owner.rayPipeline){Log("PT3_FAIL stage=create_ray_pipeline hr=0x%08lX",static_cast<unsigned long>(hr));owner.failed=true;return false;}

    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=6;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&owner.rayHeap));
    if(FAILED(hr)||!owner.rayHeap){Log("PT3_FAIL stage=create_ray_heap hr=0x%08lX",static_cast<unsigned long>(hr));owner.failed=true;return false;}
    owner.rayIncrement=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    owner.device=device;device->AddRef();owner.ready=true;
    Log("PT3_READY ready=1 tier=%u dxil_bytes=%zu toggle_default=off view_default=beauty output=private_fp16 final_composite=late_present counters=sampled_stride16 beauty=full_scene_native_surface_plus_path_traced_lighting_multiplier",
        unsigned(opt5.RaytracingTier),kPT3PathTraceShaderSize);
    return true;
}

static bool UpdateRayDescriptors(D3D12_GPU_VIRTUAL_ADDRESS tlas,
    ID3D12Resource* gbuffer1,ID3D12Resource* gbuffer2,ID3D12Resource* material,UINT materialCount) noexcept {
    if(!owner.ready||!owner.device||!owner.rayHeap||!owner.output||!owner.counters||!tlas||!gbuffer1||!gbuffer2)return false;
    if(!material||!materialCount){material=owner.fallbackMaterial;materialCount=1;}
    auto cpu=owner.rayHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC as{};as.Format=DXGI_FORMAT_UNKNOWN;as.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    as.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;as.RaytracingAccelerationStructure.Location=tlas;
    owner.device->CreateShaderResourceView(nullptr,&as,cpu);cpu.ptr+=owner.rayIncrement;

    D3D12_SHADER_RESOURCE_VIEW_DESC gs{};gs.Format=DXGI_FORMAT_R8G8B8A8_UNORM;gs.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    gs.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;gs.Texture2D.MipLevels=1;
    owner.device->CreateShaderResourceView(gbuffer1,&gs,cpu);cpu.ptr+=owner.rayIncrement;
    owner.device->CreateShaderResourceView(gbuffer2,&gs,cpu);cpu.ptr+=owner.rayIncrement;

    D3D12_SHADER_RESOURCE_VIEW_DESC ms{};ms.Format=DXGI_FORMAT_UNKNOWN;ms.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
    ms.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;ms.Buffer.NumElements=materialCount;ms.Buffer.StructureByteStride=8;
    owner.device->CreateShaderResourceView(material,&ms,cpu);cpu.ptr+=owner.rayIncrement;

    D3D12_UNORDERED_ACCESS_VIEW_DESC out{};out.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;out.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
    owner.device->CreateUnorderedAccessView(owner.output,nullptr,&out,cpu);cpu.ptr+=owner.rayIncrement;
    D3D12_UNORDERED_ACCESS_VIEW_DESC bd{};bd.Format=DXGI_FORMAT_UNKNOWN;bd.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;bd.Buffer.NumElements=12;bd.Buffer.StructureByteStride=4;
    owner.device->CreateUnorderedAccessView(owner.counters,nullptr,&bd,cpu);
    owner.lastTlas=tlas;return true;
}

static void RestoreNative(ID3D12GraphicsCommandList4* list,const control_pt0::CommandState& saved,const control_pt0::CommandHooks& h) noexcept {
    auto* base=reinterpret_cast<ID3D12GraphicsCommandList*>(list);
    if(saved.heapCount&&h.setHeaps)h.setHeaps(base,saved.heapCount,saved.heaps);
    if(saved.root&&h.setRoot)h.setRoot(base,saved.root);
    if(saved.pipeline&&h.setPipeline)h.setPipeline(base,saved.pipeline);
    if(saved.stateObject&&h.setState)h.setState(list,saved.stateObject);
    for(UINT i=0;i<control_pt0::MaxCommandTables;++i){
        if(saved.tables[i].ptr&&h.setTable)h.setTable(base,i,saved.tables[i]);
        else if(saved.cbv[i]&&h.setCBV)h.setCBV(base,i,saved.cbv[i]);
        else if(saved.srv[i]&&h.setSRV)h.setSRV(base,i,saved.srv[i]);
        else if(saved.uav[i]&&h.setUAV)h.setUAV(base,i,saved.uav[i]);
        else if(saved.constantsCount[i]&&h.setConstants)h.setConstants(base,i,saved.constantsCount[i],saved.constants[i],0);
    }
}

static bool GetReflectionExtent(ID3D12Device** deviceOut,UINT* width,UINT* height) noexcept {
    if(deviceOut)*deviceOut=nullptr;if(width)*width=0;if(height)*height=0;
    if(!deviceOut||!width||!height||!verifiedRenderer||!rrGuideGetNativeTexture)return false;
    __try {
        auto* native=rrGuideGetNativeTexture(reinterpret_cast<unsigned char*>(verifiedRenderer)+kRRShaderReflectionTargetRva);
        NativeTextureStateSnapshot snap{};
        if(!native||!ReadNativeTextureState(native,&snap)||!snap.resource)return false;
        auto* resource=static_cast<ID3D12Resource*>(snap.resource);ID3D12Device* d=nullptr;
        if(FAILED(resource->GetDevice(IID_PPV_ARGS(&d)))||!d)return false;
        *deviceOut=d;*width=static_cast<UINT>(snap.desc.Width);*height=snap.desc.Height;return *width&&*height;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}

static bool RunRayPass(ID3D12GraphicsCommandList4* list,unsigned long long frame,std::uint64_t signature) noexcept {
    if(!IsEnabled())return false;
    const auto attempt=++attempts;
    const auto tlas=PT0LatestTlas();
    if(!tlas){if(attempt<=8||(attempt%240)==0)Log("PT3_DISPATCH_SKIP frame=%llu reason=tlas_unavailable",frame);++skips;return false;}

    RRGuideInputSnapshot in{};const char* reason="unavailable";
    if(!RRGuideReadInputs(&in,&reason)||in.commandList!=reinterpret_cast<ID3D12GraphicsCommandList*>(list)||in.engineFrame!=frame){
        if(attempt<=8||(attempt%240)==0)Log("PT3_DISPATCH_SKIP frame=%llu reason=gbuffer_%s context_frame=%llu context_list=%p expected_list=%p",frame,reason?reason:"unavailable",in.engineFrame,in.commandList,list);
        ++skips;return false;
    }
    const UINT width=static_cast<UINT>(in.gbuffer1.desc.Width),height=in.gbuffer1.desc.Height;
    ID3D12Device* device=nullptr;if(FAILED(in.gbuffer1.resource->GetDevice(IID_PPV_ARGS(&device)))||!device){++skips;return false;}
    const bool initialized=InitializeRay(device);
    const bool resources=initialized&&CreateRayResources(device,width,height);
    device->Release();if(!resources){++skips;return false;}

    ID3D12Resource* material=owner.fallbackMaterial;UINT materialCount=1;bool materialNative=false;
    const auto sample=control_rr_part1::Observe(&RRAlbedoRead,reinterpret_cast<std::uintptr_t>(in.camera.renderer));
    const auto part=RRPart1ReadResource(sample);
    if(part.read&&part.descriptorMatched&&!part.fault&&part.heapType==1&&part.nativeState==0xC0&&sample.metadata.bytes>=8&&sample.metadata.bytes<=524288&&(sample.metadata.bytes%8)==0){
        auto* candidate=reinterpret_cast<ID3D12Resource*>(part.resource);
        ID3D12Device* md=nullptr;
        if(candidate&&SUCCEEDED(candidate->GetDevice(IID_PPV_ARGS(&md)))&&md){
            materialNative=SameDevice(md,owner.device);md->Release();
            if(materialNative){material=candidate;materialCount=static_cast<UINT>(sample.metadata.bytes/8);}
        }
    }
    if(!UpdateRayDescriptors(tlas,in.gbuffer1.resource,in.gbuffer2.resource,material,materialCount)){++skips;return false;}

    Constants constants{};
    for(UINT i=0;i<16;++i)constants.clipToWorld[i]=static_cast<float>(in.camera.clipToWorld[i]);
    constants.cameraPosTMin[0]=static_cast<float>(in.camera.viewToWorld[9]);constants.cameraPosTMin[1]=static_cast<float>(in.camera.viewToWorld[10]);constants.cameraPosTMin[2]=static_cast<float>(in.camera.viewToWorld[11]);constants.cameraPosTMin[3]=0.01f;
    for(UINT i=0;i<3;++i){constants.viewToWorldBasisX[i]=static_cast<float>(in.camera.viewToWorld[i]);constants.viewToWorldBasisY[i]=static_cast<float>(in.camera.viewToWorld[3+i]);constants.viewToWorldBasisZ[i]=static_cast<float>(in.camera.viewToWorld[6+i]);}
    constants.outputSize[0]=width;constants.outputSize[1]=height;constants.mode=ViewMode();constants.counterStride=16;constants.frameIndex=static_cast<UINT>(frame);
    constants.hdrActive=IsHdr10BridgeActive()?1u:0u;constants.materialCount=materialCount;constants.spp=1;

    const auto h=control_pt0::HooksFor(list);
    if(!h.dispatchCompute||!h.setPipeline||!h.setHeaps||!h.setRoot||!h.setTable||!h.setConstants){++skips;return false;}
    const auto saved=control_pt0::command;auto* base=reinterpret_cast<ID3D12GraphicsCommandList*>(list);

    D3D12_RESOURCE_BARRIER cb{};cb.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;cb.Transition.pResource=owner.counters;cb.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cb.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;cb.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;base->ResourceBarrier(1,&cb);
    base->CopyBufferRegion(owner.counters,0,owner.zeroUpload,0,48);std::swap(cb.Transition.StateBefore,cb.Transition.StateAfter);base->ResourceBarrier(1,&cb);
    D3D12_RESOURCE_BARRIER ub{};ub.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;ub.UAV.pResource=owner.output;base->ResourceBarrier(1,&ub);

    ID3D12DescriptorHeap* heap=owner.rayHeap;h.setHeaps(base,1,&heap);h.setRoot(base,owner.rayRoot);h.setPipeline(base,owner.rayPipeline);
    auto gpu=owner.rayHeap->GetGPUDescriptorHandleForHeapStart();h.setTable(base,0,gpu);gpu.ptr+=UINT64(owner.rayIncrement)*4ull;h.setTable(base,1,gpu);h.setConstants(base,2,40,&constants,0);
    h.dispatchCompute(base,(width+7)/8,(height+7)/8,1);
    D3D12_RESOURCE_BARRIER post[2]{};post[0].Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;post[0].UAV.pResource=owner.output;post[1].Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;post[1].UAV.pResource=owner.counters;base->ResourceBarrier(2,post);
    RestoreNative(list,saved,h);

    const unsigned long long pixels=static_cast<unsigned long long>(width)*height;
    owner.lastRayFrame=frame;owner.lastActualRays=pixels*(constants.mode==0?3ull:1ull);owner.lastMode=constants.mode;
    const auto ok=++successes;
    if(ok<=8||(ok%240)==0)Log("PT3_DISPATCH_OK frame=%llu mode=%s sig=%016llX tlas=0x%llX output=%p size=%ux%u actual_rays=%llu primary_pixels=%llu beauty_secondary_rays_per_pixel=%u material=%s material_count=%u gbuffer1=%p gbuffer2=%p hdr=%u successes=%llu",
        frame,ModeName(constants.mode),signature,tlas,owner.output,width,height,owner.lastActualRays,pixels,constants.mode==0?2u:0u,materialNative?"native_part1":"fallback",materialCount,in.gbuffer1.resource,in.gbuffer2.resource,constants.hdrActive,ok);
    return true;
}

static constexpr char kPT3CompositeShader[] = R"HLSL(
Texture2D<float4> PathLighting : register(t0);
Texture2D<float4> NativeScene : register(t1);
SamplerState LinearClamp : register(s0);
cbuffer C : register(b0) { uint2 SrcSize; uint2 DstSize; uint Mode; uint HdrActive; };
struct V { float4 p:SV_Position; };
V VSMain(uint id:SV_VertexID) {
    V o; float2 p=(id==0)?float2(-1,-1):(id==1)?float2(-1,3):float2(3,-1); o.p=float4(p,0,1); return o;
}
float4 PSMain(V i):SV_Target {
    float2 uv=(i.p.xy+0.5)/max(float2(DstSize),1.0);
    float3 traced=PathLighting.SampleLevel(LinearClamp,saturate(uv),0).rgb;
    if(Mode!=0u) return float4(traced,1.0);

    float3 nativeColor=NativeScene.SampleLevel(LinearClamp,saturate(uv),0).rgb;
    float3 multiplier=clamp(traced,0.45,1.55);

    // Preserve bright HUD/UI/post elements while allowing the path-traced
    // lighting multiplier to drive the authored scene underneath.
    float peak=max(nativeColor.r,max(nativeColor.g,nativeColor.b));
    float protect=HdrActive!=0u ? smoothstep(2.0,6.0,peak) : smoothstep(0.82,1.0,peak);
    float influence=lerp(0.95,0.20,protect);
    float3 result=nativeColor*lerp(1.0.xxx,multiplier,influence);
    result=HdrActive!=0u ? clamp(result,0.0,16.0) : saturate(result);
    return float4(result,1.0);
}
)HLSL";

static bool EnsureCompositeCore(ID3D12Device* device) noexcept {
    if(owner.compositeReady)return SameDevice(owner.device,device);
    if(!owner.ready||!SameDevice(owner.device,device))return false;

    ID3DBlob *vs=nullptr,*ps=nullptr,*errors=nullptr,*rootBlob=nullptr;
    HRESULT hr=PT3Compile(kPT3CompositeShader,sizeof(kPT3CompositeShader)-1,"PT3Composite","VSMain","vs_5_1",&vs,&errors);
    if(FAILED(hr)||!vs){Log("PT3_COMPOSITE_FAIL stage=compile_vs hr=0x%08lX error=%s",static_cast<unsigned long>(hr),errors?static_cast<const char*>(errors->GetBufferPointer()):"none");if(errors)errors->Release();return false;}if(errors){errors->Release();errors=nullptr;}
    hr=PT3Compile(kPT3CompositeShader,sizeof(kPT3CompositeShader)-1,"PT3Composite","PSMain","ps_5_1",&ps,&errors);
    if(FAILED(hr)||!ps){Log("PT3_COMPOSITE_FAIL stage=compile_ps hr=0x%08lX error=%s",static_cast<unsigned long>(hr),errors?static_cast<const char*>(errors->GetBufferPointer()):"none");if(errors)errors->Release();vs->Release();return false;}if(errors){errors->Release();errors=nullptr;}

    D3D12_DESCRIPTOR_RANGE range{};range.RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;range.NumDescriptors=2;range.BaseShaderRegister=0;
    D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[0].DescriptorTable.NumDescriptorRanges=1;p[0].DescriptorTable.pDescriptorRanges=&range;p[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants.ShaderRegister=0;p[1].Constants.Num32BitValues=6;p[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxAnisotropy=1;sampler.MinLOD=0;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderRegister=0;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=2;rd.pParameters=p;rd.NumStaticSamplers=1;rd.pStaticSamplers=&sampler;
    hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&rootBlob,&errors);
    if(FAILED(hr)||!rootBlob){Log("PT3_COMPOSITE_FAIL stage=serialize_root hr=0x%08lX",static_cast<unsigned long>(hr));if(errors)errors->Release();if(rootBlob)rootBlob->Release();vs->Release();ps->Release();return false;}
    hr=device->CreateRootSignature(0,rootBlob->GetBufferPointer(),rootBlob->GetBufferSize(),IID_PPV_ARGS(&owner.compositeRoot));rootBlob->Release();if(errors)errors->Release();
    if(FAILED(hr)||!owner.compositeRoot){vs->Release();ps->Release();Log("PT3_COMPOSITE_FAIL stage=create_root hr=0x%08lX",static_cast<unsigned long>(hr));return false;}

    D3D12_DESCRIPTOR_HEAP_DESC sh{};sh.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;sh.NumDescriptors=2;sh.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr=device->CreateDescriptorHeap(&sh,IID_PPV_ARGS(&owner.compositeSrvHeap));
    D3D12_DESCRIPTOR_HEAP_DESC rh{};rh.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;rh.NumDescriptors=3;
    if(SUCCEEDED(hr))hr=device->CreateDescriptorHeap(&rh,IID_PPV_ARGS(&owner.compositeRtvHeap));
    if(FAILED(hr)){vs->Release();ps->Release();Log("PT3_COMPOSITE_FAIL stage=create_heaps hr=0x%08lX",static_cast<unsigned long>(hr));return false;}
    owner.compositeSrvIncrement=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    owner.compositeRtvIncrement=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for(UINT i=0;i<3&&SUCCEEDED(hr);++i){
        hr=device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&owner.compositeSlots[i].allocator));
        if(SUCCEEDED(hr))hr=device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,owner.compositeSlots[i].allocator,nullptr,IID_PPV_ARGS(&owner.compositeSlots[i].list));
        if(SUCCEEDED(hr))hr=owner.compositeSlots[i].list->Close();
        if(SUCCEEDED(hr)){
            D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;auto bd=BufferDesc(48);
            hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&owner.compositeSlots[i].readback));
        }
    }
    if(SUCCEEDED(hr))hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&owner.compositeFence));
    if(FAILED(hr)||!owner.compositeFence){vs->Release();ps->Release();Log("PT3_COMPOSITE_FAIL stage=private_work hr=0x%08lX",static_cast<unsigned long>(hr));return false;}

    // Store shader bytecode temporarily in static blobs while creating format-specific PSOs below.
    static ID3DBlob* cachedVs=nullptr;static ID3DBlob* cachedPs=nullptr;
    if(cachedVs)cachedVs->Release();if(cachedPs)cachedPs->Release();cachedVs=vs;cachedPs=ps;
    owner.compositeReady=true;
    Log("PT3_COMPOSITE_READY ready=1 slots=3 no_cpu_wait=1 source_format=%u target_formats=r10_fp16_rgba8_bgra8 native_copy=1 beauty=path_lighting_multiplier_over_native",unsigned(DXGI_FORMAT_R16G16B16A16_FLOAT));\n    Log("PT4_FULL_SCENE_READY renderer=integrated_full_scene primary=camera_ray secondary=diffuse_plus_specular composite=native_scene_times_path_lighting ui_protection=bright_pixel");
    return true;
}

static ID3D12PipelineState* CompositePso(ID3D12Device* device,DXGI_FORMAT format) noexcept {
    for(auto& e:owner.compositePsos)if(e.pso&&e.format==format)return e.pso;
    if(!owner.compositeRoot)return nullptr;
    ID3DBlob *vs=nullptr,*ps=nullptr,*errors=nullptr;
    HRESULT hr=PT3Compile(kPT3CompositeShader,sizeof(kPT3CompositeShader)-1,"PT3Composite","VSMain","vs_5_1",&vs,&errors);if(errors){errors->Release();errors=nullptr;}
    if(SUCCEEDED(hr))hr=PT3Compile(kPT3CompositeShader,sizeof(kPT3CompositeShader)-1,"PT3Composite","PSMain","ps_5_1",&ps,&errors);if(errors){errors->Release();errors=nullptr;}
    if(FAILED(hr)||!vs||!ps){if(vs)vs->Release();if(ps)ps->Release();return nullptr;}
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pd{};pd.pRootSignature=owner.compositeRoot;pd.VS={vs->GetBufferPointer(),vs->GetBufferSize()};pd.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    pd.BlendState.AlphaToCoverageEnable=FALSE;pd.BlendState.IndependentBlendEnable=FALSE;pd.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    pd.SampleMask=UINT_MAX;pd.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pd.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pd.RasterizerState.DepthClipEnable=TRUE;
    pd.DepthStencilState.DepthEnable=FALSE;pd.DepthStencilState.StencilEnable=FALSE;pd.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pd.NumRenderTargets=1;pd.RTVFormats[0]=format;pd.SampleDesc.Count=1;
    ID3D12PipelineState* pso=nullptr;hr=device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&pso));vs->Release();ps->Release();
    if(FAILED(hr)||!pso){Log("PT3_COMPOSITE_FAIL stage=create_pso format=%u hr=0x%08lX",unsigned(format),static_cast<unsigned long>(hr));return nullptr;}
    for(auto& e:owner.compositePsos)if(!e.pso){e.format=format;e.pso=pso;return pso;}pso->Release();return nullptr;
}

static bool EnsureNativeCopy(CompositeSlot& slot,ID3D12Device* device,const D3D12_RESOURCE_DESC& td) noexcept {
    if(!device||td.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||td.DepthOrArraySize!=1||td.MipLevels!=1||td.SampleDesc.Count!=1||!td.Width||!td.Height)return false;
    if(slot.nativeCopy&&slot.nativeWidth==td.Width&&slot.nativeHeight==td.Height&&slot.nativeFormat==td.Format)return true;
    if(slot.nativeCopy){slot.nativeCopy->Release();slot.nativeCopy=nullptr;}
    slot.nativeWidth=0;slot.nativeHeight=0;slot.nativeFormat=DXGI_FORMAT_UNKNOWN;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC copyDesc=td;copyDesc.Flags=D3D12_RESOURCE_FLAG_NONE;copyDesc.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;
    HRESULT hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&copyDesc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&slot.nativeCopy));
    if(FAILED(hr)||!slot.nativeCopy){
        Log("PT3_COMPOSITE_FAIL stage=native_copy_create hr=0x%08lX format=%u size=%llux%u",static_cast<unsigned long>(hr),unsigned(td.Format),td.Width,td.Height);
        return false;
    }
    slot.nativeWidth=td.Width;slot.nativeHeight=td.Height;slot.nativeFormat=td.Format;
    Log("PT4_NATIVE_SCENE_COPY_READY resource=%p format=%u size=%llux%u state=copy_dest",slot.nativeCopy,unsigned(td.Format),td.Width,td.Height);
    return true;
}

static void ConsumeCounters(CompositeSlot& slot,unsigned long long present) noexcept {
    if(!slot.fenceValue||!owner.compositeFence||owner.compositeFence->GetCompletedValue()<slot.fenceValue||!slot.readback)return;
    void* mapped=nullptr;D3D12_RANGE range{0,48};
    if(FAILED(slot.readback->Map(0,&range,&mapped))||!mapped)return;
    UINT values[12]{};std::memcpy(values,mapped,sizeof(values));D3D12_RANGE written{0,0};slot.readback->Unmap(0,&written);
    const UINT sampled=values[0],hits=values[1],misses=values[2],minFixed=values[3]==0xFFFFFFFFu?0:values[3],maxFixed=values[4],sumFixed=values[5],checksum=values[6];
    const UINT diffuseHits=values[7],diffuseMisses=values[8],specHits=values[9],specMisses=values[10];
    statActualRays.store(slot.actualRays);statSampledRays.store(sampled);statHits.store(hits);statMisses.store(misses);statMinFixed.store(minFixed);statMaxFixed.store(maxFixed);statSumFixed.store(sumFixed);statChecksum.store(checksum);statFrame.store(slot.sourceFrame);
    const double minT=minFixed/16.0,maxT=maxFixed/16.0,avgT=hits?double(sumFixed)/16.0/double(hits):0.0,ratio=sampled?double(hits)*100.0/double(sampled):0.0;
    static unsigned long long reports=0;const auto n=++reports;
    if(n<=8||(n%120)==0)Log("PT3_COUNTERS present=%llu source_frame=%llu mode=%s actual_rays=%llu sampled_primary=%u primary_hits=%u primary_misses=%u hit_ratio=%.2f min_t=%.3f max_t=%.3f avg_t=%.3f instance_checksum=0x%08X diffuse_hits=%u diffuse_misses=%u spec_hits=%u spec_misses=%u",
        present,slot.sourceFrame,ModeName(slot.mode),slot.actualRays,sampled,hits,misses,ratio,minT,maxT,avgT,checksum,diffuseHits,diffuseMisses,specHits,specMisses);
}

static HRESULT CompositeBeforePresent(IDXGISwapChain3* chain,bool hdrActive,unsigned long long present) noexcept {
    if(!IsEnabled())return S_OK;
    if(!chain||!owner.ready||!owner.output||!owner.lastRayFrame){++compositeSkips;return S_OK;}
    auto* queue=GetHdr10BridgeDirectQueue();if(!queue){++compositeSkips;Log("PT3_COMPOSITE_SKIP present=%llu reason=queue_unavailable",present);return S_OK;}
    ID3D12Device* device=nullptr;HRESULT hr=queue->GetDevice(IID_PPV_ARGS(&device));if(FAILED(hr)||!device||!SameDevice(device,owner.device)){if(device)device->Release();++compositeSkips;Log("PT3_COMPOSITE_SKIP present=%llu reason=device_mismatch hr=0x%08lX queue=%p queue_device=%p ray_device=%p native_identity_compare=1",present,static_cast<unsigned long>(hr),queue,device,owner.device);return S_OK;}
    if(!EnsureCompositeCore(device)){device->Release();++compositeSkips;return S_OK;}

    const UINT index=chain->GetCurrentBackBufferIndex();ID3D12Resource* target=nullptr;hr=chain->GetBuffer(index,IID_PPV_ARGS(&target));
    if(FAILED(hr)||!target){device->Release();++compositeSkips;Log("PT3_COMPOSITE_SKIP present=%llu reason=backbuffer_unavailable index=%u hr=0x%08lX",present,index,static_cast<unsigned long>(hr));return S_OK;}
    const auto td=target->GetDesc();auto* pso=CompositePso(device,td.Format);device->Release();
    if(!pso){target->Release();++compositeSkips;return S_OK;}

    CompositeSlot& slot=owner.compositeSlots[present%3u];
    ConsumeCounters(slot,present);
    if(slot.fenceValue&&owner.compositeFence->GetCompletedValue()<slot.fenceValue){
        target->Release();++compositeSkips;const auto n=compositeSkips.load();if(n<=8||(n%240)==0)Log("PT3_COMPOSITE_SKIP present=%llu reason=slot_busy slot=%u fence=%llu completed=%llu",present,unsigned(present%3u),slot.fenceValue,owner.compositeFence->GetCompletedValue());return S_OK;
    }
    if(!EnsureNativeCopy(slot,owner.device,td)){target->Release();++compositeSkips;return S_OK;}

    hr=slot.allocator->Reset();if(SUCCEEDED(hr))hr=slot.list->Reset(slot.allocator,pso);if(FAILED(hr)){target->Release();++compositeSkips;return S_OK;}

    // Descriptor 0 is the path-traced lighting multiplier. Descriptor 1 is a
    // private copy of Control's fully authored frame so BEAUTY keeps textures,
    // direct lighting, transparencies, volumetrics, post effects and HUD.
    auto srvCpu=owner.compositeSrvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_SHADER_RESOURCE_VIEW_DESC pathSrv{};pathSrv.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;pathSrv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;pathSrv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;pathSrv.Texture2D.MipLevels=1;
    owner.device->CreateShaderResourceView(owner.output,&pathSrv,srvCpu);srvCpu.ptr+=owner.compositeSrvIncrement;
    D3D12_SHADER_RESOURCE_VIEW_DESC nativeSrv{};nativeSrv.Format=td.Format;nativeSrv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;nativeSrv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;nativeSrv.Texture2D.MipLevels=1;
    owner.device->CreateShaderResourceView(slot.nativeCopy,&nativeSrv,srvCpu);

    auto rtv=owner.compositeRtvHeap->GetCPUDescriptorHandleForHeapStart();rtv.ptr+=SIZE_T(present%3u)*owner.compositeRtvIncrement;
    owner.device->CreateRenderTargetView(target,nullptr,rtv);

    const D3D12_RESOURCE_STATES targetBefore=hdrActive?D3D12_RESOURCE_STATE_COMMON:D3D12_RESOURCE_STATE_PRESENT;
    D3D12_RESOURCE_BARRIER copyBarrier{};copyBarrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;copyBarrier.Transition.pResource=target;copyBarrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;copyBarrier.Transition.StateBefore=targetBefore;copyBarrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    slot.list->ResourceBarrier(1,&copyBarrier);
    slot.list->CopyResource(slot.nativeCopy,target);

    D3D12_RESOURCE_BARRIER toDraw[4]{};
    toDraw[0].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;toDraw[0].Transition.pResource=owner.output;toDraw[0].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;toDraw[0].Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;toDraw[0].Transition.StateAfter=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    toDraw[1].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;toDraw[1].Transition.pResource=owner.counters;toDraw[1].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;toDraw[1].Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;toDraw[1].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    toDraw[2].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;toDraw[2].Transition.pResource=slot.nativeCopy;toDraw[2].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;toDraw[2].Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;toDraw[2].Transition.StateAfter=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    toDraw[3].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;toDraw[3].Transition.pResource=target;toDraw[3].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;toDraw[3].Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;toDraw[3].Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;
    slot.list->ResourceBarrier(4,toDraw);
    slot.list->CopyBufferRegion(slot.readback,0,owner.counters,0,48);

    D3D12_VIEWPORT vp{};vp.Width=float(td.Width);vp.Height=float(td.Height);vp.MaxDepth=1.0f;D3D12_RECT sc{0,0,LONG(td.Width),LONG(td.Height)};
    slot.list->RSSetViewports(1,&vp);slot.list->RSSetScissorRects(1,&sc);slot.list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    slot.list->SetGraphicsRootSignature(owner.compositeRoot);ID3D12DescriptorHeap* heaps[]={owner.compositeSrvHeap};slot.list->SetDescriptorHeaps(1,heaps);
    slot.list->SetGraphicsRootDescriptorTable(0,owner.compositeSrvHeap->GetGPUDescriptorHandleForHeapStart());
    const UINT sizes[6]={owner.outputWidth,owner.outputHeight,UINT(td.Width),td.Height,owner.lastMode,hdrActive?1u:0u};slot.list->SetGraphicsRoot32BitConstants(1,6,sizes,0);
    slot.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);slot.list->DrawInstanced(3,1,0,0);

    D3D12_RESOURCE_BARRIER restore[4]{};
    restore[0].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;restore[0].Transition.pResource=owner.output;restore[0].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;restore[0].Transition.StateBefore=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;restore[0].Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    restore[1].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;restore[1].Transition.pResource=owner.counters;restore[1].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;restore[1].Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;restore[1].Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    restore[2].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;restore[2].Transition.pResource=slot.nativeCopy;restore[2].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;restore[2].Transition.StateBefore=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;restore[2].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;
    restore[3].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;restore[3].Transition.pResource=target;restore[3].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;restore[3].Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;restore[3].Transition.StateAfter=targetBefore;
    slot.list->ResourceBarrier(4,restore);
    hr=slot.list->Close();if(SUCCEEDED(hr)){ID3D12CommandList* lists[]={slot.list};queue->ExecuteCommandLists(1,lists);slot.fenceValue=++owner.nextFenceValue;hr=queue->Signal(owner.compositeFence,slot.fenceValue);}
    if(SUCCEEDED(hr)){
        slot.sourceFrame=owner.lastRayFrame;slot.actualRays=owner.lastActualRays;slot.mode=owner.lastMode;
        const auto n=++composites;if(n<=8||(n%240)==0)Log("PT3_COMPOSITE_OK present=%llu source_frame=%llu mode=%s target=%p target_format=%u target_size=%llux%u hdr=%u slot=%u fence=%llu composite_count=%llu",
            present,owner.lastRayFrame,ModeName(owner.lastMode),target,unsigned(td.Format),td.Width,td.Height,unsigned(hdrActive),unsigned(present%3u),slot.fenceValue,n);
    } else {++compositeSkips;Log("PT3_COMPOSITE_SKIP present=%llu reason=submit_failed hr=0x%08lX",present,static_cast<unsigned long>(hr));}
    target->Release();return S_OK;
}

static void Poll(unsigned long long present) noexcept {
    const bool down=(GetAsyncKeyState(VK_F7)&0x8000)!=0;
    if(down&&!f7Down&&IsEnabled()){
        unsigned next=ViewMode()+1u;if(next>3)next=0;SetViewMode(next,"F7");
        Log("PT3_HOTKEY present=%llu key=F7 action=cycle_view mode=%u name=%s",present,next,ModeName(next));
    }
    f7Down=down;
}

static void DeferredReflectionBoundary() noexcept {
    if(!IsEnabled())return;
    unsigned long long frame=0;DWORD frameFault=0;if(!ReadEngineFrameSafe(&frame,&frameFault))return;
    static unsigned long long lastFrame=~0ull;if(lastFrame==frame)return;
    EngineCommandContextSnapshot c{};if(!ReadEngineCommandContext(nullptr,&c)||!c.commandList)return;
    auto* base=reinterpret_cast<ID3D12GraphicsCommandList*>(c.commandList);if(!PT0EnsureCommandHooks(base))return;
    ID3D12GraphicsCommandList4* list4=nullptr;if(FAILED(base->QueryInterface(IID_PPV_ARGS(&list4)))||!list4)return;
    const auto sig=PT0LatestReflectionSig();const bool ran=RunRayPass(list4,frame,sig);if(ran)lastFrame=frame;list4->Release();
}

static void GetStats(unsigned long long* rays,unsigned long long* sampled,unsigned long long* hits,unsigned long long* misses,float* minT,float* maxT,float* avgT,unsigned* checksum) noexcept {
    if(rays)*rays=statActualRays.load();if(sampled)*sampled=statSampledRays.load();if(hits)*hits=statHits.load();if(misses)*misses=statMisses.load();
    const unsigned minF=statMinFixed.load(),maxF=statMaxFixed.load(),sumF=statSumFixed.load();const auto h=statHits.load();
    if(minT)*minT=minF/16.0f;if(maxT)*maxT=maxF/16.0f;if(avgT)*avgT=h?float(double(sumF)/16.0/double(h)):0.0f;if(checksum)*checksum=statChecksum.load();
}

} // namespace control_pt3

static bool PT3IsEnabled() noexcept {return control_pt3::IsEnabled();}
static unsigned PT3ViewMode() noexcept {return control_pt3::ViewMode();}
static const wchar_t* PT3ViewModeLabelWide() noexcept {return control_pt3::ModeNameWide(control_pt3::ViewMode());}
static void PT3SetEnabled(bool enabled,const char* source) noexcept {control_pt3::SetEnabled(enabled,source);}
static void PT3SetViewMode(unsigned mode,const char* source) noexcept {control_pt3::SetViewMode(mode,source);}
static void PT3GetOverlayStats(unsigned long long* rays,unsigned long long* sampled,unsigned long long* hits,unsigned long long* misses,float* minT,float* maxT,float* avgT,unsigned* checksum) noexcept {control_pt3::GetStats(rays,sampled,hits,misses,minT,maxT,avgT,checksum);}
static void PT3Poll(unsigned long long present) noexcept {control_pt3::Poll(present);}
static void PT3DeferredReflectionBoundary() noexcept {control_pt3::DeferredReflectionBoundary();}
static HRESULT SubmitPT3CompositeBeforePresent(IDXGISwapChain3* chain,bool hdrActive,unsigned long long present) noexcept {return control_pt3::CompositeBeforePresent(chain,hdrActive,present);}
