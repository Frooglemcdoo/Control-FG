// Evaluate the single-history Both filter on private GI targets while the engine's
// temporal bindings are live. Only the later visible-character pass can consume them.
#include "rr_character_gi_dispatch.h"
namespace RRCharacterSource {
using Microsoft::WRL::ComPtr;
using Image=RRCharacterGITarget;
using Texture=control_nrd_native::Texture;

static std::vector<std::shared_ptr<Image>> pool;
static std::weak_ptr<Image> latest;

static std::shared_ptr<Image> Acquire(ID3D12Device* device,UINT w,UINT h,const std::shared_ptr<Image>& previous){
    for(auto& v:pool)if(v.use_count()==1&&v!=previous&&v->width==w&&v->height==h&&!v->invalid)return v;
    pool.erase(std::remove_if(pool.begin(),pool.end(),[&](const auto& v){
        return v.use_count()==1&&(v->invalid||v->width!=w||v->height!=h);
    }),pool.end());
    if(pool.size()>=8)return {};
    auto v=std::make_shared<Image>();v->width=w;v->height=h;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=w;d.Height=h;
    d.MipLevels=d.DepthOrArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
    d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    for(auto& tex:v->image)if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&tex))))return {};
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;hd.NumDescriptors=27;
    if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&v->descriptors))))return {};
    pool.push_back(v);return v;
}
static bool Ordered(const std::shared_ptr<Image>& image,ID3D12GraphicsCommandList* list){
    if(!image||image->invalid)return false;
    if(image->submitted)return true;
    for(const auto& r:ControlNrdRuntime::recordings)
        if(!r->reset && r->characterSource==image && r->characterList.Get()==list)return true;
    return false;
}
static const char* Record(UINT64 frame,UINT64 view,ID3D12CommandQueue* gameQueue,ID3D12GraphicsCommandList* list,UINT passes){
    using namespace ControlNrdRuntime;
    if(!passes||passes>MaxPasses)return "unsupported_spatial_count";
    Poll();if(fence&&fence->GetCompletedValue()==UINT64_MAX)return "device_removed";
    gi_reuse::ComputeBindingSnapshot b;UINT64 generation=0;
    gi_reuse::GraphicsTableState graphics;
    const auto reset=gi_dual_bridge::resetReady.load();
    if(!list||!reset||!reset(list)||!hooks::Capture(list,b,generation)||!b.Ready()||!b.heaps[0])return "gi_compute_bindings";
    if(!hooks::CaptureGraphics(list,graphics))return "gi_graphics_tables";
    ComPtr<ID3D12Device> device;ComPtr<ID3D12GraphicsCommandList4> nativeList;
    if(FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||FAILED(list->QueryInterface(IID_PPV_ARGS(&nativeList)))||
       !Initialize(gameQueue,device.Get()))return "gi_queue";
    auto* bundle=control_rr_gi::Find(b.pipeline.Get());
    if(!bundle||bundle->spatial)return "gi_temporal_pipeline";
    struct Extract{
        UINT64 cb[2]{};
        void Constants(unsigned,unsigned,unsigned,const std::uint32_t*){}
        void Descriptor(unsigned i,gi_reuse::RootArgumentKind kind,UINT64 address){
            if(kind==gi_reuse::RootArgumentKind::CBV&&i>=2&&i<=3)cb[i-2]=address;
        }
    } native;
    b.arguments.Replay(native);
    if(!native.cb[0]||!native.cb[1])return "gi_native_constants";
    static const char* names[]={"g_tGBuffer1","g_tGBuffer2","g_tLinearDepth","g_tPreviousLinearDepth",
                                 "g_tGeometryVelocityTexture","g_DLF_rwtColorSource"};
    Texture t[7]{};
    for(UINT i=0;i<_countof(names);++i){
        if(i<2){
            RRGuideInputTexture nativeTexture{};DWORD fault=0;const char* why=nullptr;
            if(!RRGuideReadTexture(i?kRRGuideGBuffer2ShaderRva:kRRGuideGBuffer1ShaderRva,&nativeTexture,&why,&fault))return names[i];
            t[i].resource=nativeTexture.resource;t[i].desc=nativeTexture.desc;
            t[i].state=D3D12_RESOURCE_STATES(nativeTexture.trackedState);
        }else if(!NamedTexture(names[i],t[i],i==2||i==3))return names[i];
        if(!(t[i].state&(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))||
           (t[i].desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)||ReadFormat(t[i].desc.Format)==DXGI_FORMAT_UNKNOWN)return names[i];
    }
    const UINT w=UINT(t[5].desc.Width),h=t[5].desc.Height;
    if(t[5].desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT)return "gi_source_format";
    for(UINT i=0;i<_countof(names);++i)if(t[i].desc.Width!=w||t[i].desc.Height!=h)return "gi_source_extents";
    // Hit distance first becomes useful in the private spatial pass. RAW bypass
    // can reach this temporal callback without a readable spatial hit provider.
    const bool useHitDistance=HitDistanceInput(w,h,t[6],[](Texture& hit){
        return ControlNrdRuntime::NamedTexture("g_tHitInfo",hit);
    });
    const UINT inputCount=useHitDistance?7:6;
    if(!Prepare(device.Get()))return "gi_filter_pipeline";
    auto prior=latest.lock();
    const bool eligible=Ordered(prior,list)&&prior->frame+1==frame&&prior->view==view&&prior->width==w&&prior->height==h&&
        !rrNativeFrame.Reset()&&rrCharacterEvaluatedFrame.load()==prior->frame;
    if(!eligible)prior.reset();
    auto image=Acquire(device.Get(),w,h,prior);
    if(!image)return "gi_source_pool";
    auto* r=GetRecording(nativeList.Get(),generation);if(!r||r->characterSource)return "gi_source_recording";
    const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto cpu=image->descriptors->GetCPUDescriptorHandleForHeapStart();
    auto gpu=image->descriptors->GetGPUDescriptorHandleForHeapStart();
    auto srv=[&](ID3D12Resource* texture,DXGI_FORMAT fmt,UINT mips){
        D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.Format=fmt;d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d.Texture2D.MipLevels=mips;
        device->CreateShaderResourceView(texture,&d,cpu);cpu.ptr+=step;
    };
    auto input=[&](UINT i){srv(t[i].resource,ReadFormat(t[i].desc.Format),i==5?t[i].desc.MipLevels:1);};
    auto history=[&](){if(prior)srv(prior->image[prior->output].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,1);else input(5);};
    auto uav=[&](UINT i){D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(image->image[i].Get(),nullptr,&u,cpu);cpu.ptr+=step;};
    input(2);input(3);input(4);input(5);history();
    for(UINT i=5;i<8;++i)srv(nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT,1);
    uav(0);
    for(UINT i=0;i<2;++i){
        for(UINT j=0;j<5;++j)input(j);
        srv(image->image[i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,1);history();
        HitView(device.Get(),cpu,useHitDistance?t[6].resource:nullptr,ReadFormat(t[6].desc.Format));
        cpu.ptr+=step;
        uav(1-i);
    }
    r->characterSource=image;r->characterPrevious=prior;r->characterList=list;
    for(UINT i=0;i<inputCount;++i)r->characterInputs[i]=t[i].resource;
    image->frame=frame;image->view=view;image->submitted=false;image->invalid=false;image->passes=passes;image->output=passes&1;
    constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    bool transitioned[7]{};
    for(UINT i=0;i<inputCount;++i)if(!(t[i].state&read)){
        bool alias=false;for(UINT j=0;j<i;++j)if(t[j].resource==t[i].resource)alias=true;
        if(!alias){Barrier(list,t[i].resource,t[i].state,read);transitioned[i]=true;}
    }
    ID3D12Resource* images[]={image->image[0].Get(),image->image[1].Get()};
    ID3D12DescriptorHeap* privateHeap=image->descriptors.Get();
    list->SetDescriptorHeaps(1,&privateHeap);
    DispatchPasses(list,native.cb[0],native.cb[1],gpu,step,images,w,h,passes,bool(prior),useHitDistance);
    for(UINT i=0;i<inputCount;++i)if(transitioned[i])Barrier(list,t[i].resource,read,t[i].state);
    ID3D12DescriptorHeap* restore[]={b.heaps[0].Get(),b.heaps[1].Get()};
    list->SetDescriptorHeaps(b.heaps[1]?2:1,restore);
    graphics.Replay([&](UINT index,UINT64 address){list->SetGraphicsRootDescriptorTable(index,{address});});
    b.Restore(nativeList.Get());latest=image;
    if(useHitDistance)return prior?"both_history_hit":"both_seed_hit";
    return prior?"both_history_geometry":"both_seed_geometry";
}
}
static bool RRCharacterUsesSingleSample() noexcept {
    int id=-1;std::uint32_t count=0;
    return control_nrd_native::Resolve(rrNativeClampApi,"g_uNumSamples",id)&&rrNativeClampApi.ReadCurrent(id,count)&&count<=1;
}
static const char* RRCharacterRecordGISource(UINT64 frame,UINT64 view,ID3D12CommandQueue* gameQueue,ID3D12GraphicsCommandList* list,unsigned passes) noexcept {
    std::lock_guard<std::recursive_mutex> lock(ControlNrdRuntime::mutex);
    ControlNrdRuntime::Mute mute;
    try{return RRCharacterSource::Record(frame,view,gameQueue,list,passes);}catch(...){return "gi_allocation";}
}
static std::shared_ptr<RRCharacterGITarget> RRCharacterGIForColor(UINT64 frame,UINT64 view,
    ID3D12CommandQueue* queue,ID3D12GraphicsCommandList* list,UINT w,UINT h) noexcept {
    std::lock_guard<std::recursive_mutex> lock(ControlNrdRuntime::mutex);
    auto v=RRCharacterSource::latest.lock();
    if(queue!=ControlNrdRuntime::queue.Get()||!RRCharacterSource::Ordered(v,list)||v->frame!=frame||
       v->view!=view||v->width!=w||v->height!=h)return {};
    return v;
}
