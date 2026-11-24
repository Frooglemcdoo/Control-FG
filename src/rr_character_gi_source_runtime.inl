// Evaluate the single-history Both filter on private GI targets while the engine's
// temporal bindings are live, then select GI for the native material path.
#include "rr_character_gi_dispatch.h"
#include "rr_character_material_dispatch.h"
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
    hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;hd.NumDescriptors=31;
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
static bool PrepareReference(ID3D12Device* device,Image& image){
    if(image.selected)return true;
    const auto d=image.image[0]->GetDesc();
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
    ComPtr<ID3D12Resource> textures[3];
    for(auto& t:textures)if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&t))))return false;
    image.raw=std::move(textures[0]);image.temporal=std::move(textures[1]);image.selected=std::move(textures[2]);return true;
}
static void SaveMip(ID3D12GraphicsCommandList* list,ID3D12Resource* src,D3D12_RESOURCE_STATES state,ID3D12Resource* dst){
    constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={src,0,state,D3D12_RESOURCE_STATE_COPY_SOURCE};
    list->ResourceBarrier(1,&barrier);Barrier(list,dst,read,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION a{},b{};
    a.pResource=src;b.pResource=dst;a.Type=b.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    list->CopyTextureRegion(&b,0,0,0,&a,nullptr);
    Barrier(list,dst,D3D12_RESOURCE_STATE_COPY_DEST,read);
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);
}
// The producer binds this ShaderTexture immediately before the GI filtering call.
static bool ReadProducerHit(Texture& hit) noexcept {
    if(!verifiedRenderer||!rrGuideGetNativeTexture)return false;
    __try {
        auto* shader=reinterpret_cast<unsigned char*>(verifiedRenderer)+0x12974a0;
        auto* native=rrGuideGetNativeTexture(shader);
        return control_nrd_native::ReadTexture({reinterpret_cast<uintptr_t>(shader),reinterpret_cast<uintptr_t>(native)},hit);
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// The early worker captured the actual bound depth attachment. At the GI
// boundary its native tracker can still be in a complete depth-only state.
static bool CurrentDepth(const RRCharacterEarly::Stamp& stamp,RRGuideInputTexture& out) noexcept {
    if(RRInputCaptureReadNative(stamp.nativeDepth,stamp.depth,&out))return true;
    __try {
        if(!stamp.depth||!stamp.nativeDepth || *reinterpret_cast<ID3D12Resource**>(
                static_cast<unsigned char*>(stamp.nativeDepth)+0x88)!=stamp.depth)return false;
        const UINT states[]={D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ};
        for(auto state:states)if(RRAlbedoNativeTextureState(stamp.nativeDepth,state)){
            out={};out.resource=stamp.depth;out.desc=stamp.depth->GetDesc();out.trackedState=state;return true;
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){}
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
    const auto session=rr_pair::Armed();
    const bool hit=(ReadProducerHit(t[6])||NamedTexture("g_tHitInfo",t[6])) && t[6].resource &&
        t[6].desc.Width==w&&t[6].desc.Height==h&&t[6].desc.MipLevels&&
        !(t[6].desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)&&ReadFormat(t[6].desc.Format)!=DXGI_FORMAT_UNKNOWN;
    if(!hit)return "producer_hit_unavailable";
    uint32_t cut=0;int cutId=-1;
    if(!control_nrd_native::Resolve(rrNativeClampApi,"g_uCameraCut",cutId)||!rrNativeClampApi.ReadCurrent(cutId,cut)||cut>1)return "camera_cut_unavailable";
    Texture target{};
    if(!NamedTexture("g_DLF_rwtColorTarget",target)||target.state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS||
       target.desc.Width!=w||target.desc.Height!=h||target.desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||
       !(target.desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))return "native_raw_destination";
    for(auto& input:t)if(input.resource==target.resource)return "native_target_alias";
    if(!Prepare(device.Get())||!RRCharacterMaterial::Prepare(device.Get()))return "gi_filter_pipeline";
    auto prior=latest.lock();
    const bool eligible=Ordered(prior,list)&&prior->frame+1==frame&&prior->view==view&&prior->width==w&&prior->height==h&&
        !cut&&!rrNativeFrame.Reset()&&rrCharacterEvaluatedFrame.load()==prior->frame;
    if(!eligible)prior.reset();
    auto image=Acquire(device.Get(),w,h,prior);if(!image)return "gi_source_pool";
    auto* r=GetRecording(nativeList.Get(),generation);if(!r||r->characterSource)return "gi_source_recording";
    const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const bool capture=session&&PrepareReference(device.Get(),*image);
    if(session&&!capture)rr_pair::Issue(session,L"Capture snapshot allocation failed");
    image->materialDispatched=false;image->session=capture?session:0;
    image->historyFrames=prior?prior->historyFrames+1:1;
    auto cpu=image->descriptors->GetCPUDescriptorHandleForHeapStart();
    auto gpu=image->descriptors->GetGPUDescriptorHandleForHeapStart();
    auto srv=[&](ID3D12Resource* texture,DXGI_FORMAT fmt,UINT mips){
        D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.Format=fmt;d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d.Texture2D.MipLevels=mips;
        device->CreateShaderResourceView(texture,&d,cpu);cpu.ptr+=step;
    };
    auto input=[&](UINT i){srv(t[i].resource,ReadFormat(t[i].desc.Format),i==5?t[i].desc.MipLevels:1);};
    auto history=[&](){if(prior)srv(prior->image[prior->output].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,1);else input(5);};
    auto uav=[&](ID3D12Resource* output){D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(output,nullptr,&u,cpu);cpu.ptr+=step;};
    input(2);input(3);input(4);input(5);history();
    for(UINT i=5;i<8;++i)srv(nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT,1);uav(image->image[0].Get());
    for(UINT i=0;i<2;++i){
        for(UINT j=0;j<5;++j)input(j);
        srv(image->image[i].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,1);history();input(6);uav(image->image[1-i].Get());
    }
    const auto stamp=RRCharacterEarly::Snapshot();
    RRGuideInputTexture depth{};
    const char* outcome="mask_not_recorded";
    bool apply=false;
    if(stamp.frame==frame && stamp.view==view && stamp.queue==gameQueue && rrDiffuse && rrDiffuse->frame==frame){
        outcome="mask_submission_pending";
        if(stamp.submitted || stamp.list==list){
            outcome="gbuffer_depth_unavailable";
            if(RRCharacterUsesSingleSample()&&CurrentDepth(stamp,depth) &&
               depth.desc.Width==w&&depth.desc.Height==h&&depth.desc.Format==DXGI_FORMAT_R32_TYPELESS&&
               depth.resource!=target.resource&&std::none_of(std::begin(t),std::end(t),[&](const Texture& v){return v.resource==depth.resource;})&&
               !(depth.desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)){
                apply=true;outcome="native_character_gi";
                srv(rrDiffuse->earlyCharacterTarget.resource,DXGI_FORMAT_R16G16B16A16_FLOAT,1);
                srv(depth.resource,DXGI_FORMAT_R32_FLOAT,1);srv(image->image[passes&1].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,1);
                uav(target.resource);
            }
        }
    }
    r->characterSource=image;r->characterPrevious=prior;r->characterList=list;
    for(UINT i=0;i<7;++i)r->characterInputs[i]=t[i].resource;
    image->frame=frame;image->view=view;image->submitted=false;image->invalid=false;image->passes=passes;image->output=passes&1;
    constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    bool transitioned[7]{};
    for(UINT i=0;i<7;++i)if(!(t[i].state&read)){
        bool alias=false;for(UINT j=0;j<i;++j)if(t[j].resource==t[i].resource)alias=true;
        if(!alias){Barrier(list,t[i].resource,t[i].state,read);transitioned[i]=true;}
    }
    ID3D12Resource* images[]={image->image[0].Get(),image->image[1].Get()};
    ID3D12DescriptorHeap* privateHeap=image->descriptors.Get();
    list->SetDescriptorHeaps(1,&privateHeap);
    DispatchPasses(list,native.cb[0],native.cb[1],gpu,step,images,w,h,passes,bool(prior),true,
        capture?image->temporal.Get():nullptr,cut,true);
    if(capture)SaveMip(list,t[5].resource,transitioned[5]?read:t[5].state,image->raw.Get());
    if(apply){
        auto* mask=rrDiffuse->earlyCharacterTarget.resource;
        r->characterInputs[7]=mask;r->characterInputs[8]=depth.resource;r->characterInputs[9]=target.resource;
        Barrier(list,mask,D3D12_RESOURCE_STATE_RENDER_TARGET,read);
        const bool depthMove=!(depth.trackedState&read);
        if(depthMove)Barrier(list,depth.resource,D3D12_RESOURCE_STATES(depth.trackedState),read);
        const auto measured=rrCharacterMeasuredFrame.load();
        const bool sample=RRCharacterMaterial::measured && (!measured || frame%120==0) &&
            r->materialSample.Prepare(device.Get(),frame);
        RRCharacterMaterial::Dispatch(list,{gpu.ptr+UINT64(27)*step},target.resource,w,h,sample?&r->materialSample:nullptr);
        if(depthMove)Barrier(list,depth.resource,read,D3D12_RESOURCE_STATES(depth.trackedState));
        Barrier(list,mask,read,D3D12_RESOURCE_STATE_RENDER_TARGET);
        image->materialDispatched=true;
    }
    if(capture){r->characterInputs[9]=target.resource;SaveMip(list,target.resource,target.state,image->selected.Get());}
    for(UINT i=0;i<7;++i)if(transitioned[i])Barrier(list,t[i].resource,read,t[i].state);
    ID3D12DescriptorHeap* restore[]={b.heaps[0].Get(),b.heaps[1].Get()};
    list->SetDescriptorHeaps(b.heaps[1]?2:1,restore);
    graphics.Replay([&](UINT index,UINT64 address){list->SetGraphicsRootDescriptorTable(index,{address});});
    b.Restore(nativeList.Get());latest=image;
    return outcome;
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
