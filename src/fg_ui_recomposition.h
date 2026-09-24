#pragma once
#include "fg_ui_work.h"
#include "fg_ui_device_identity.h"
#include "../build/fg_ui_alpha_compiled.h"

// Frame Generation UI recomposition owns a private pre-UI snapshot plus an R8
// UI-alpha mask. In SDR, the snapshot is already in the same RGB10/SDR color
// domain as the intercepted backbuffer. While the HDR10 bridge is active,
// Control renders into an FP16/scRGB shadow backbuffer, but DLSS-G does not
// support FP16/scRGB. For HDR, keep the FP16 pre-UI snapshot for alpha
// extraction and convert a second private copy to RGB10/PQ/BT.2020 with the
// exact same shader math used by the final HDR10 bridge before tagging it as
// kBufferTypeHUDLessColor.

struct FGUIRecomposeSlot {
    FGUIWork work{};
    ID3D12Resource* postHud=nullptr;
    D3D12_RESOURCE_STATES postHudState=D3D12_RESOURCE_STATE_COPY_DEST;
    ID3D12Resource* hudless=nullptr;       // Source-domain pre-UI snapshot.
    ID3D12Resource* uiAlpha=nullptr;       // R8 UI alpha generated post-UI.
    ID3D12Resource* hdrHudless=nullptr;    // RGB10/PQ pre-UI scene for HDR FG.
    UINT width=0,height=0;
    DXGI_FORMAT colorFormat=DXGI_FORMAT_UNKNOWN;
    bool hdr10=false;
    D3D12_RESOURCE_STATES hudlessState=D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_RESOURCE_STATES uiAlphaState=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    D3D12_RESOURCE_STATES hdrHudlessState=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    unsigned long long targetPresent=0;
};

struct FGUIRecomposeOwner {
    ID3D12Device* device=nullptr;

    // Existing UI-alpha compute path.
    ID3D12RootSignature* root=nullptr;
    ID3D12PipelineState* pipeline=nullptr;
    ID3D12DescriptorHeap* heap=nullptr;
    UINT descriptorSize=0;

    // HDR HUDless conversion path. Built lazily only after HDR activates.
    ID3D12RootSignature* hdrRoot=nullptr;
    ID3D12PipelineState* hdrPipeline=nullptr;
    ID3D12DescriptorHeap* hdrHeap=nullptr;
    UINT hdrDescriptorSize=0;
    bool hdrPipelineAttempted=false;
    bool hdrPipelineReady=false;

    // Keep SDR and HDR slots separate so repeated live HDR/SDR transitions reuse
    // the previous mode's resources instead of continuously allocating new 4K
    // surfaces. Resize behavior remains the same as the proven SDR path.
    FGUIRecomposeSlot slots[3]{};
    FGUIRecomposeSlot hdrSlots[3]{};
    bool stopped=false;
    bool privateFailure=false;
};

static SRWLOCK fgUIRecomposeLock=SRWLOCK_INIT;
static std::vector<FGUIRecomposeOwner*> fgUIRecomposeOwners;
static std::atomic<unsigned long long> fgUIRecomposeCopies{0};
static std::atomic<unsigned long long> fgUIRecomposeDispatches{0};
static std::atomic<unsigned long long> fgUIRecomposeFailures{0};
static std::atomic<unsigned long long> fgUIHdrConversions{0};
static std::atomic<unsigned long long> fgUIHdrConversionFailures{0};

static bool FGUIRecomposeSupportedColorFormat(DXGI_FORMAT f) noexcept {
    return f==DXGI_FORMAT_R16G16B16A16_FLOAT || f==DXGI_FORMAT_R10G10B10A2_UNORM ||
           f==DXGI_FORMAT_R8G8B8A8_UNORM || f==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
           f==DXGI_FORMAT_B8G8R8A8_UNORM || f==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
}

static constexpr char kFGHdrHudlessComputeShader[] = R"HLSL(
Texture2D<float4> Src : register(t0);
RWTexture2D<float4> Dst : register(u0);

float3 Linear709ToLinear2020(float3 c)
{
    return float3(
        0.6274040 * c.r + 0.3292820 * c.g + 0.0433136 * c.b,
        0.0690970 * c.r + 0.9195400 * c.g + 0.0113612 * c.b,
        0.0163916 * c.r + 0.0880132 * c.g + 0.8955950 * c.b);
}

float3 LinearNitsToPQ(float3 nits)
{
    const float m1 = 2610.0 / 16384.0;
    const float m2 = 2523.0 / 32.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;
    float3 L = saturate(max(nits, 0.0) / 10000.0);
    float3 Lm = pow(L, m1);
    return pow((c1 + c2 * Lm) / (1.0 + c3 * Lm), m2);
}

[numthreads(8,8,1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    Dst.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) return;
    float4 src = Src.Load(int3(id.xy, 0));
    float3 linear2020 = max(Linear709ToLinear2020(src.rgb), 0.0);
    float3 pq = LinearNitsToPQ(linear2020 * 80.0);
    Dst[id.xy] = float4(pq, 1.0);
}
)HLSL";

static void FGUIRecomposeReleaseHdrPipeline(FGUIRecomposeOwner* o) noexcept {
    if(!o)return;
    RRGuideRelease(o->hdrHeap);
    RRGuideRelease(o->hdrPipeline);
    RRGuideRelease(o->hdrRoot);
    o->hdrDescriptorSize=0;
    o->hdrPipelineReady=false;
}

static bool FGUIRecomposeBuildHdrPipeline(FGUIRecomposeOwner* o) noexcept {
    if(!o||!o->device)return false;
    if(o->hdrPipelineReady)return true;
    if(o->hdrPipelineAttempted)return false;
    o->hdrPipelineAttempted=true;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT fs{};fs.Format=DXGI_FORMAT_R10G10B10A2_UNORM;
    const HRESULT supportHr=o->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&fs,sizeof(fs));
    if(FAILED(supportHr)||!(fs.Support1&D3D12_FORMAT_SUPPORT1_TEXTURE2D)||!(fs.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)){
        Log("FG_HDR_HUDLESS_PIPELINE_FAIL stage=r10_uav_support hr=0x%08lX support1=0x%X support2=0x%llX action=disable_hdr_ui_recomposition",static_cast<unsigned long>(supportHr),unsigned(fs.Support1),static_cast<unsigned long long>(fs.Support2));
        return false;
    }

    HMODULE compiler=LoadLibraryW(L"d3dcompiler_47.dll");
    HMODULE d3d12=GetModuleHandleW(L"d3d12.dll");
    bool releaseD3D12=false;
    if(!d3d12){d3d12=LoadLibraryW(L"d3d12.dll");releaseD3D12=d3d12!=nullptr;}
    auto compile=compiler?reinterpret_cast<D3DCompileDynamicFn>(GetProcAddress(compiler,"D3DCompile")):nullptr;
    auto serialize=d3d12?reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(d3d12,"D3D12SerializeRootSignature")):nullptr;
    if(!compile||!serialize){
        Log("FG_HDR_HUDLESS_PIPELINE_FAIL stage=dynamic_functions compiler=%p d3d12=%p compile=%p serialize=%p",compiler,d3d12,compile,serialize);
        if(compiler)FreeLibrary(compiler);if(releaseD3D12&&d3d12)FreeLibrary(d3d12);return false;
    }

    ID3DBlob* cs=nullptr;ID3DBlob* errors=nullptr;ID3DBlob* rootBlob=nullptr;
    HRESULT hr=compile(kFGHdrHudlessComputeShader,sizeof(kFGHdrHudlessComputeShader)-1,"ControlFG_HDR10_HUDLess_Compute",
        nullptr,nullptr,"CSMain","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&cs,&errors);
    if(FAILED(hr)||!cs){
        Log("FG_HDR_HUDLESS_PIPELINE_FAIL stage=compile_cs hr=0x%08lX error=%s",static_cast<unsigned long>(hr),errors?static_cast<const char*>(errors->GetBufferPointer()):"none");
        RRGuideRelease(errors);RRGuideRelease(cs);if(compiler)FreeLibrary(compiler);if(releaseD3D12&&d3d12)FreeLibrary(d3d12);return false;
    }
    RRGuideRelease(errors);

    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=1;ranges[0].BaseShaderRegister=0;ranges[0].RegisterSpace=0;ranges[0].OffsetInDescriptorsFromTableStart=0;
    ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=1;ranges[1].BaseShaderRegister=0;ranges[1].RegisterSpace=0;ranges[1].OffsetInDescriptorsFromTableStart=1;
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable.NumDescriptorRanges=2;parameter.DescriptorTable.pDescriptorRanges=ranges;parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};rootDesc.NumParameters=1;rootDesc.pParameters=&parameter;
    hr=serialize(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&rootBlob,&errors);
    if(FAILED(hr)||!rootBlob){
        Log("FG_HDR_HUDLESS_PIPELINE_FAIL stage=serialize_root hr=0x%08lX error=%s",static_cast<unsigned long>(hr),errors?static_cast<const char*>(errors->GetBufferPointer()):"none");
        RRGuideRelease(errors);RRGuideRelease(rootBlob);RRGuideRelease(cs);if(compiler)FreeLibrary(compiler);if(releaseD3D12&&d3d12)FreeLibrary(d3d12);return false;
    }
    RRGuideRelease(errors);
    hr=o->device->CreateRootSignature(0,rootBlob->GetBufferPointer(),rootBlob->GetBufferSize(),IID_PPV_ARGS(&o->hdrRoot));
    RRGuideRelease(rootBlob);
    if(SUCCEEDED(hr)&&o->hdrRoot){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=o->hdrRoot;pd.CS={cs->GetBufferPointer(),cs->GetBufferSize()};hr=o->device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&o->hdrPipeline));}
    RRGuideRelease(cs);if(compiler)FreeLibrary(compiler);if(releaseD3D12&&d3d12)FreeLibrary(d3d12);
    if(FAILED(hr)||!o->hdrRoot||!o->hdrPipeline){Log("FG_HDR_HUDLESS_PIPELINE_FAIL stage=create_compute_pso hr=0x%08lX",static_cast<unsigned long>(hr));FGUIRecomposeReleaseHdrPipeline(o);return false;}

    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=6;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr=o->device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&o->hdrHeap));
    if(FAILED(hr)||!o->hdrHeap){Log("FG_HDR_HUDLESS_PIPELINE_FAIL stage=compute_heap hr=0x%08lX",static_cast<unsigned long>(hr));FGUIRecomposeReleaseHdrPipeline(o);return false;}
    o->hdrDescriptorSize=o->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    o->hdrPipelineReady=true;
    Log("FG_HDR_HUDLESS_PIPELINE_READY owner=%p slots=3 source_format=%u target_format=%u source_space=scrgb_linear target_space=bt2020_pq conversion=compute_uav no_graphics_state=1 private_command_list=1",
        o,unsigned(DXGI_FORMAT_R16G16B16A16_FLOAT),unsigned(DXGI_FORMAT_R10G10B10A2_UNORM));
    return true;
}

static FGUIRecomposeOwner* FGUIRecomposeFindOrCreate(ID3D12Device* device) noexcept {
    if(!device)return nullptr;
    for(auto* o:fgUIRecomposeOwners)if(o&&o->privateFailure)return nullptr;
    for(auto* o:fgUIRecomposeOwners)if(o&&!o->stopped&&o->device==device)return o;
    auto* o=new(std::nothrow) FGUIRecomposeOwner;if(!o)return nullptr;o->device=device;device->AddRef();HRESULT hr=S_OK;
    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=2;ranges[0].BaseShaderRegister=0;ranges[0].OffsetInDescriptorsFromTableStart=0;
    ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=1;ranges[1].BaseShaderRegister=0;ranges[1].OffsetInDescriptorsFromTableStart=2;
    D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;param.DescriptorTable.NumDescriptorRanges=2;param.DescriptorTable.pDescriptorRanges=ranges;
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&param;
    auto serialize=reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
    ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;if(!serialize)hr=E_NOINTERFACE;else hr=serialize(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);
    if(SUCCEEDED(hr)&&blob)hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&o->root));else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
    RRGuideRelease(blob);RRGuideRelease(errors);
    if(SUCCEEDED(hr)){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=o->root;pd.CS={kFGUIAlphaShader,sizeof(kFGUIAlphaShader)};hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&o->pipeline));}
    if(SUCCEEDED(hr)){D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=18;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&o->heap));}
    if(FAILED(hr)||!o->root||!o->pipeline||!o->heap){o->stopped=true;Log("FG_UI_RECOMPOSE_CREATE_FAILED hr=0x%08lX",static_cast<unsigned long>(hr));fgUIRecomposeOwners.push_back(o);return nullptr;}
    o->descriptorSize=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);fgUIRecomposeOwners.push_back(o);
    Log("FG_UI_RECOMPOSE_READY owner=%p slots=3 ui_alpha_format=%u hdr10_hudless_conversion=lazy",o,unsigned(DXGI_FORMAT_R8_UNORM));return o;
}

static bool FGUIRecomposeEnsureSlot(FGUIRecomposeOwner* o,FGUIRecomposeSlot& s,const D3D12_RESOURCE_DESC& colorDesc,UINT slotIndex,bool hdr10) noexcept {
    if(!o||!o->device)return false;
    if(!FGUIRecomposeSupportedColorFormat(colorDesc.Format)){
        static std::atomic<unsigned long long> unsupportedFormats{0};
        const auto count=++unsupportedFormats;
        if(count<=8||(count%240)==0)Log("FG_UI_RECOMPOSE_SKIP count=%llu reason=unsupported_color_format format=%u width=%llu height=%u",count,unsigned(colorDesc.Format),static_cast<unsigned long long>(colorDesc.Width),colorDesc.Height);
        return false;
    }
    if(colorDesc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||colorDesc.SampleDesc.Count!=1||!colorDesc.Width||!colorDesc.Height){
        Log("FG_UI_RECOMPOSE_SKIP reason=unsupported_shape dimension=%u samples=%u width=%llu height=%u",unsigned(colorDesc.Dimension),colorDesc.SampleDesc.Count,static_cast<unsigned long long>(colorDesc.Width),colorDesc.Height);return false;
    }
    if(hdr10&&colorDesc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT){
        Log("FG_UI_RECOMPOSE_SKIP reason=hdr10_source_format_mismatch source_format=%u expected=%u",unsigned(colorDesc.Format),unsigned(DXGI_FORMAT_R16G16B16A16_FLOAT));return false;
    }
    if(s.postHud&&s.hudless&&s.uiAlpha&&s.width==colorDesc.Width&&s.height==colorDesc.Height&&s.colorFormat==colorDesc.Format&&s.hdr10==hdr10&&(!hdr10||s.hdrHudless))return true;

    if(slotIndex>=3)return false;
    if(hdr10&&!o->hdrPipelineReady){
        AcquireSRWLockExclusive(&fgUIRecomposeLock);const bool ready=FGUIRecomposeBuildHdrPipeline(o);ReleaseSRWLockExclusive(&fgUIRecomposeLock);
        if(!ready){++fgUIHdrConversionFailures;return false;}
    }

    // Never destroy an old in-flight slot during a resize/mode transition. As in
    // the signed-off SDR path, old resource references are intentionally retained
    // for the diagnostic session instead of being released under DLSS-G.
    D3D12_FEATURE_DATA_FORMAT_SUPPORT fs{};fs.Format=DXGI_FORMAT_R8_UNORM;
    if(FAILED(o->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&fs,sizeof(fs))) || !(fs.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)){
        Log("FG_UI_RECOMPOSE_SLOT_FAILED reason=r8_uav_typed_store_unsupported support2=0x%llX",static_cast<unsigned long long>(fs.Support2));return false;
    }
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC hd=colorDesc;hd.Flags=D3D12_RESOURCE_FLAG_NONE;hd.MipLevels=1;hd.DepthOrArraySize=1;
    ID3D12Resource* hud=nullptr;HRESULT hr=o->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&hd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&hud));
    ID3D12Resource* postHud=nullptr;if(SUCCEEDED(hr))hr=o->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&hd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&postHud));
    D3D12_RESOURCE_DESC ad{};ad.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;ad.Width=colorDesc.Width;ad.Height=colorDesc.Height;ad.DepthOrArraySize=1;ad.MipLevels=1;ad.Format=DXGI_FORMAT_R8_UNORM;ad.SampleDesc.Count=1;ad.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;ad.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ID3D12Resource* alpha=nullptr;if(SUCCEEDED(hr))hr=o->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&ad,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&alpha));
    ID3D12Resource* hdrHudless=nullptr;
    if(SUCCEEDED(hr)&&hdr10){
        D3D12_RESOURCE_DESC rd=hd;rd.Format=DXGI_FORMAT_R10G10B10A2_UNORM;rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        hr=o->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&hdrHudless));
    }
    if(FAILED(hr)||!postHud||!hud||!alpha||(hdr10&&!hdrHudless)){
        RRGuideRelease(postHud);RRGuideRelease(hud);RRGuideRelease(alpha);RRGuideRelease(hdrHudless);
        Log("FG_UI_RECOMPOSE_SLOT_FAILED hr=0x%08lX width=%llu height=%u source_format=%u hdr10=%u",static_cast<unsigned long>(hr),static_cast<unsigned long long>(colorDesc.Width),colorDesc.Height,unsigned(colorDesc.Format),unsigned(hdr10));return false;
    }
    if(hdr10){
        wchar_t name[96]{};swprintf_s(name,L"ControlFG HDR10 FG HUDless %u",slotIndex);hdrHudless->SetName(name);
        D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=colorDesc.Format;sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.Texture2D.MipLevels=1;
        auto cpu=o->hdrHeap->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(slotIndex*2)*SIZE_T(o->hdrDescriptorSize);o->device->CreateShaderResourceView(hud,&sd,cpu);cpu.ptr+=SIZE_T(o->hdrDescriptorSize);
        D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R10G10B10A2_UNORM;ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;o->device->CreateUnorderedAccessView(hdrHudless,nullptr,&ud,cpu);
    }
    s.postHud=postHud;s.postHudState=D3D12_RESOURCE_STATE_COPY_DEST;
    s.hudless=hud;s.uiAlpha=alpha;s.hdrHudless=hdrHudless;s.width=static_cast<UINT>(colorDesc.Width);s.height=colorDesc.Height;s.colorFormat=colorDesc.Format;s.hdr10=hdr10;
    s.hudlessState=D3D12_RESOURCE_STATE_COPY_DEST;s.uiAlphaState=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;s.hdrHudlessState=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    Log("FG_UI_RECOMPOSE_SLOT_READY slot=%u width=%u height=%u source_format=%u hdr10=%u tagged_hudless_format=%u",slotIndex,s.width,s.height,unsigned(s.colorFormat),unsigned(s.hdr10),unsigned(s.hdr10?DXGI_FORMAT_R10G10B10A2_UNORM:s.colorFormat));
    return true;
}

static FGUIRecomposeSlot* PrepareFGUIRecompositionBeforeHUD(unsigned long long currentPresent,ID3D12Resource* finalColor,const D3D12_RESOURCE_DESC& desc,unsigned trackedState,bool stateKnown,ID3D12GraphicsCommandList* list) noexcept {
    if(!finalColor||!list||!stateKnown||trackedState==UINT_MAX)return nullptr;
    ID3D12Device* device=nullptr;if(FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||!device)return nullptr;
    AcquireSRWLockExclusive(&fgUIRecomposeLock);auto* o=FGUIRecomposeFindOrCreate(device);ReleaseSRWLockExclusive(&fgUIRecomposeLock);device->Release();if(!o)return nullptr;
    const unsigned long long target=currentPresent+1;const bool hdr10=IsHdr10BridgeActive();const UINT slotIndex=static_cast<UINT>(target%3);auto& s=hdr10?o->hdrSlots[slotIndex]:o->slots[slotIndex];if(FAILED(s.work.Begin(o->device,target))){o->privateFailure=true;o->stopped=true;Log("FG_UI_PRIVATE_FAIL stage=reuse target=%llu",target);return nullptr;}if(!FGUIRecomposeEnsureSlot(o,s,desc,slotIndex,hdr10)){if(FAILED(s.work.list->Close())){o->privateFailure=true;o->stopped=true;}s.work.recording=false;return nullptr;}
    if(s.hudlessState!=D3D12_RESOURCE_STATE_COPY_DEST){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=s.hudless;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=s.hudlessState;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;list->ResourceBarrier(1,&b);s.hudlessState=D3D12_RESOURCE_STATE_COPY_DEST;}
    D3D12_RESOURCE_BARRIER cb{};bool transitioned=trackedState!=D3D12_RESOURCE_STATE_COPY_SOURCE;if(transitioned){cb.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;cb.Transition.pResource=finalColor;cb.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;cb.Transition.StateBefore=static_cast<D3D12_RESOURCE_STATES>(trackedState);cb.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;list->ResourceBarrier(1,&cb);}
    list->CopyResource(s.hudless,finalColor);
    if(transitioned){std::swap(cb.Transition.StateBefore,cb.Transition.StateAfter);list->ResourceBarrier(1,&cb);}
    D3D12_RESOURCE_BARRIER hb{};hb.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;hb.Transition.pResource=s.hudless;hb.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;hb.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;hb.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&hb);s.hudlessState=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;s.targetPresent=target;
    if(FGAlignActive(target)) Log("ALIGN_HUD_PRE target=%llu color=%p snapshot=%p slot=%u hdr=%u command=%p",target,finalColor,s.hudless,slotIndex,unsigned(hdr10),list);
    const auto n=++fgUIRecomposeCopies;if(n<=8||(n%240)==0)Log("FG_UI_HUDLESS_SNAPSHOT present=%llu target_present=%llu count=%llu resource=%p snapshot=%p width=%u height=%u source_format=%u hdr10=%u",currentPresent,target,n,finalColor,s.hudless,s.width,s.height,unsigned(s.colorFormat),unsigned(s.hdr10));return &s;
}

static bool FGUIRecomposeRecordHdrConversion(FGUIRecomposeOwner* o,FGUIRecomposeSlot& s,UINT slotIndex,ID3D12GraphicsCommandList* list) noexcept {
    if(!s.hdr10)return true;
    if(!o||!list||!o->hdrPipelineReady||!o->hdrPipeline||!o->hdrRoot||!o->hdrHeap||!s.hudless||!s.hdrHudless){++fgUIHdrConversionFailures;return false;}
    if(slotIndex>=3){++fgUIHdrConversionFailures;return false;}

    // R11: this dispatch is recorded only on the slot-owned command list.
    if(s.hudlessState!=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE){++fgUIHdrConversionFailures;Log("FG_HDR_HUDLESS_CONVERT_FAIL target_present=%llu reason=source_state state=0x%X",s.targetPresent,unsigned(s.hudlessState));return false;}
    if(s.hdrHudlessState!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=s.hdrHudless;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=s.hdrHudlessState;b.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;list->ResourceBarrier(1,&b);s.hdrHudlessState=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}

    ID3D12DescriptorHeap* heaps[]{o->hdrHeap};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(o->hdrRoot);list->SetPipelineState(o->hdrPipeline);
    auto gpu=o->hdrHeap->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(slotIndex*2)*UINT64(o->hdrDescriptorSize);list->SetComputeRootDescriptorTable(0,gpu);
    list->Dispatch((s.width+7)/8,(s.height+7)/8,1);
    D3D12_RESOURCE_BARRIER barriers[2]{};barriers[0].Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barriers[0].UAV.pResource=s.hdrHudless;barriers[1].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barriers[1].Transition.pResource=s.hdrHudless;barriers[1].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barriers[1].Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;barriers[1].Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(2,barriers);s.hdrHudlessState=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    const auto n=++fgUIHdrConversions;if(n<=8||(n%240)==0)Log("FG_HDR_HUDLESS_CONVERT target_present=%llu count=%llu slot=%u source=%p target=%p width=%u height=%u source_format=%u target_format=%u source_space=scrgb_linear target_space=bt2020_pq mode=compute no_graphics_state=1 private_command_list=1",s.targetPresent,n,slotIndex,s.hudless,s.hdrHudless,s.width,s.height,unsigned(s.colorFormat),unsigned(DXGI_FORMAT_R10G10B10A2_UNORM));
    return true;
}

static bool FinalizeFGUIRecompositionAfterHUD(unsigned long long currentPresent,ID3D12Resource* finalColor,const D3D12_RESOURCE_DESC& desc,unsigned trackedState,bool stateKnown,ID3D12GraphicsCommandList* list) noexcept {
    if(!finalColor||!list||!stateKnown||trackedState==UINT_MAX)return false;
    ID3D12Device* device=nullptr;if(FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||!device)return false;
    AcquireSRWLockExclusive(&fgUIRecomposeLock);FGUIRecomposeOwner* o=nullptr;for(auto* x:fgUIRecomposeOwners)if(x&&!x->stopped&&x->device==device){o=x;break;}ReleaseSRWLockExclusive(&fgUIRecomposeLock);device->Release();if(!o)return false;
    const unsigned long long target=currentPresent+1;const bool hdr10=IsHdr10BridgeActive();const UINT slotIndex=static_cast<UINT>(target%3);auto& s=hdr10?o->hdrSlots[slotIndex]:o->slots[slotIndex];if(s.targetPresent!=target||!s.hudless||!s.uiAlpha||s.width!=desc.Width||s.height!=desc.Height||s.colorFormat!=desc.Format||s.hdr10!=hdr10)return false;
    // Only copies/barriers touch the engine list. Preserve its bindings and PSO.
    if(s.postHudState!=D3D12_RESOURCE_STATE_COPY_DEST){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=s.postHud;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=s.postHudState;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;list->ResourceBarrier(1,&b);}
    const auto nativeState=static_cast<D3D12_RESOURCE_STATES>(trackedState);
    D3D12_RESOURCE_BARRIER nativeBarrier{};nativeBarrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;nativeBarrier.Transition.pResource=finalColor;nativeBarrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;nativeBarrier.Transition.StateBefore=nativeState;nativeBarrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    if(nativeState!=D3D12_RESOURCE_STATE_COPY_SOURCE)list->ResourceBarrier(1,&nativeBarrier);
    list->CopyResource(s.postHud,finalColor);
    if(nativeState!=D3D12_RESOURCE_STATE_COPY_SOURCE){std::swap(nativeBarrier.Transition.StateBefore,nativeBarrier.Transition.StateAfter);list->ResourceBarrier(1,&nativeBarrier);}
    s.postHudState=D3D12_RESOURCE_STATE_COPY_DEST;
    finalColor=s.postHud;trackedState=unsigned(s.postHudState);list=s.work.list;
    if(s.uiAlphaState!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=s.uiAlpha;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=s.uiAlphaState;b.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;list->ResourceBarrier(1,&b);s.uiAlphaState=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}
    const auto finalState=static_cast<D3D12_RESOURCE_STATES>(trackedState);bool finalTransition=finalState!=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;D3D12_RESOURCE_BARRIER fb{};if(finalTransition){fb.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;fb.Transition.pResource=finalColor;fb.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;fb.Transition.StateBefore=finalState;fb.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&fb);}
    const UINT descriptorSlot=slotIndex+(s.hdr10?3u:0u);
    auto cpu=o->heap->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(descriptorSlot*3)*SIZE_T(o->descriptorSize);D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=desc.Format;sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.Texture2D.MipLevels=1;
    o->device->CreateShaderResourceView(finalColor,&sd,cpu);cpu.ptr+=o->descriptorSize;o->device->CreateShaderResourceView(s.hudless,&sd,cpu);cpu.ptr+=o->descriptorSize;D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R8_UNORM;ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;o->device->CreateUnorderedAccessView(s.uiAlpha,nullptr,&ud,cpu);
    ID3D12DescriptorHeap* heaps[]{o->heap};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(o->root);list->SetPipelineState(o->pipeline);auto gpu=o->heap->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(descriptorSlot*3)*UINT64(o->descriptorSize);list->SetComputeRootDescriptorTable(0,gpu);list->Dispatch((s.width+7)/8,(s.height+7)/8,1);
    D3D12_RESOURCE_BARRIER ab[2]{};ab[0].Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;ab[0].UAV.pResource=s.uiAlpha;ab[1].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;ab[1].Transition.pResource=s.uiAlpha;ab[1].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;ab[1].Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;ab[1].Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(2,ab);s.uiAlphaState=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if(finalTransition){std::swap(fb.Transition.StateBefore,fb.Transition.StateAfter);list->ResourceBarrier(1,&fb);}

    if(s.hdr10&&!FGUIRecomposeRecordHdrConversion(o,s,slotIndex,list)){
        const auto fail=++fgUIRecomposeFailures;Log("FG_HDR_HUDLESS_CONVERT_FAIL present=%llu target_present=%llu failures=%llu action=stop_private_work",currentPresent,target,fail);o->privateFailure=true;o->stopped=true;return false;
    }

    FGPixelProducer(list,finalColor,finalState,target,s.hudless,s.hudlessState,s.uiAlpha,s.uiAlphaState,s.hdr10?s.hdrHudless:nullptr,s.hdrHudlessState);
    sl::FrameToken* token=GetSLFrameToken(target,false);if(!token){if(FAILED(s.work.Finish())){o->privateFailure=true;o->stopped=true;}return false;}
    sl::Extent extent{};extent.width=s.width;extent.height=s.height;
    ID3D12Resource* taggedHudless=s.hdr10?s.hdrHudless:s.hudless;
    const D3D12_RESOURCE_STATES taggedHudlessState=s.hdr10?s.hdrHudlessState:s.hudlessState;
    const DXGI_FORMAT taggedHudlessFormat=s.hdr10?DXGI_FORMAT_R10G10B10A2_UNORM:s.colorFormat;
    sl::Resource hud(sl::ResourceType::eTex2d,taggedHudless,taggedHudlessState);sl::Resource alpha(sl::ResourceType::eTex2d,s.uiAlpha,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    sl::ResourceTag tags[]{sl::ResourceTag(&hud,sl::kBufferTypeHUDLessColor,sl::ResourceLifecycle::eValidUntilPresent,&extent),sl::ResourceTag(&alpha,sl::kBufferTypeUIAlpha,sl::ResourceLifecycle::eValidUntilPresent,&extent)};
    ++slResourceTagCalls;const sl::Result result=slSetTagForFrameApi(*token,slFgViewport,tags,_countof(tags),reinterpret_cast<sl::CommandBuffer*>(list));const bool ok=result==sl::Result::eOk;
    if(FGAlignActive(target)) Log("ALIGN_UI_TAG target=%llu token=%p color=%p hudless=%p alpha=%p slot=%u hdr=%u success=%u command=%p",target,token,finalColor,taggedHudless,s.uiAlpha,slotIndex,unsigned(hdr10),unsigned(ok),list);
    if(ok){++slResourceTagSuccesses;slFgColorWidth.store(s.width);slFgColorHeight.store(s.height);slFgColorFormat.store(unsigned(taggedHudlessFormat));slFgHudLessFormat.store(unsigned(taggedHudlessFormat));MarkSLFrameTagBits(target,kSLTagHUDLess|kSLTagUIAlpha);}else{++slResourceTagFailures;++fgUIRecomposeFailures;}
    const auto n=++fgUIRecomposeDispatches;if(n<=8||(n%240)==0||!ok)Log("FG_UI_RECOMPOSE_TAG present=%llu target_present=%llu count=%llu result=%lld success=%u hudless=%p ui_alpha=%p width=%u height=%u source_format=%u hudless_format=%u ui_format=%u hdr10=%u color_space=%s tag_mask=0x%X recomposition_candidate=%u",currentPresent,target,n,SLResultCode(result),unsigned(ok),taggedHudless,s.uiAlpha,s.width,s.height,unsigned(s.colorFormat),unsigned(taggedHudlessFormat),unsigned(DXGI_FORMAT_R8_UNORM),unsigned(s.hdr10),s.hdr10?"bt2020_pq":"backbuffer_native",GetSLFrameTagMask(target),unsigned(ok));if(FAILED(s.work.Finish())){o->privateFailure=true;o->stopped=true;return false;}return ok;
}

// Native Present has flushed the engine commands before entering our DXGI
// wrapper. Submit private compute on that exact direct queue before HDR/SDR
// presentation work and before Streamline consumes the already-recorded tags.
static HRESULT SubmitFGUIRecompositionBeforePresent(unsigned long long present) noexcept {
    auto* directQueue=GetHdr10BridgeDirectQueue();
    AcquireSRWLockExclusive(&fgUIRecomposeLock);
    HRESULT result=S_OK;
    for(auto* o:fgUIRecomposeOwners){
        if(!o)continue;
        if(o->privateFailure){result=E_FAIL;break;}
        if(o->stopped)continue;
        for(UINT domain=0;domain<2&&SUCCEEDED(result);++domain)for(UINT i=0;i<3;++i){
            auto& s=domain?o->hdrSlots[i]:o->slots[i];
            // A HUD path can prepare a snapshot but skip finalization. Submit
            // its empty private list too, fencing the earlier engine copy.
            if(s.work.recording&&FAILED(s.work.Finish())){result=E_FAIL;o->privateFailure=true;o->stopped=true;break;}
            if(!s.work.pending)continue;
            ID3D12Device* queueDevice=nullptr;
            const HRESULT deviceHr=directQueue?directQueue->GetDevice(IID_PPV_ARGS(&queueDevice)):E_POINTER;
            const auto resolveNative=[](IUnknown* object,IUnknown** native) noexcept {
                void* raw=nullptr;
                const bool ok=slGetNativeInterfaceApi&&slGetNativeInterfaceApi(object,&raw)==sl::Result::eOk;
                *native=reinterpret_cast<IUnknown*>(raw);return ok;
            };
            const bool sameDevice=SUCCEEDED(deviceHr)&&FGUISameDevice(queueDevice,o->device,resolveNative);
            const bool direct=directQueue&&directQueue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT;
            if(!sameDevice||!direct){
                Log("FG_UI_PRIVATE_DEVICE_FAIL present=%llu queue=%p queue_device=%p owner_device=%p device_hr=0x%08lX same_native_device=%u direct=%u target=%llu",present,directQueue,queueDevice,o->device,static_cast<unsigned long>(deviceHr),unsigned(sameDevice),unsigned(direct),s.work.target);
                if(queueDevice)queueDevice->Release();result=E_UNEXPECTED;o->privateFailure=true;o->stopped=true;break;
            }
            queueDevice->Release();
            result=s.work.Submit(directQueue,present);
            if(FAILED(result)){Log("FG_UI_PRIVATE_WORK_FAIL present=%llu target=%llu pending=%u failed=%u hr=0x%08lX",present,s.work.target,unsigned(s.work.pending),unsigned(s.work.failed),static_cast<unsigned long>(result));o->privateFailure=true;o->stopped=true;break;}
            if(present<=8||(present%240)==0||FGAlignActive(present))Log("FG_UI_PRIVATE_SUBMIT present=%llu slot=%u hdr=%u engine_bindings_changed=0 descriptors_per_slot=3 cpu_wait_after_submit=0",present,i,domain);
        }
        if(FAILED(result))break;
    }
    ReleaseSRWLockExclusive(&fgUIRecomposeLock);
    if(FAILED(result)){static std::atomic<unsigned long long> failures{0};const auto n=++failures;if(n<=8||(n%240)==0)Log("FG_UI_PRIVATE_FAIL stage=submit present=%llu hr=0x%08lX",present,static_cast<unsigned long>(result));}
    return result;
}
