// Included where the live RR input textures and the fenced character slot are available.
namespace rr_pair {
inline void Before(RRLiveOwner* owner,RRCharacterColorSlot& slot,const RRCharacterEvalData& eval,
                   const RRGuideInputSnapshot& in,const RRGuideInputTexture* native,const RRGuideInputTexture& gi,
                   ID3D12Resource* mask,const RRGuideInputTexture& remap){
    const auto id=Armed();if(!id)return;
    const auto& ref=slot.sourceLease;
    if(!ref||ref->session!=id||ref->historyFrames<4||!eval.motionKnown||rr_pair::fourRayFrame.load()!=eval.frame)return;
    constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    std::array<ID3D12Resource*,PlaneCount> src{gi.resource,ref->temporal.Get(),ref->reference[ref->output].Get(),
        eval.color,slot.output.Get(),mask,eval.depth,in.gbuffer1.resource,eval.motion,remap.resource,eval.output};
    std::array<UINT,PlaneCount> states{gi.trackedState,read,read,native[0].trackedState,read,D3D12_RESOURCE_STATE_RENDER_TARGET,
        native[1].trackedState,in.gbuffer1.trackedState,native[2].trackedState,remap.trackedState,D3D12_RESOURCE_STATE_UNORDERED_ACCESS};
    UINT64 total=0;
    std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT,PlaneCount> layouts{};
    for(unsigned i=0;i<PlaneCount;++i){
        if(!src[i])return;auto desc=src[i]->GetDesc();auto fmt=desc.Format;
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.SampleDesc.Count!=1||desc.DepthOrArraySize!=1)return;
        if(i<=Spatial||i==Mask){if(fmt!=DXGI_FORMAT_R16G16B16A16_FLOAT)return;}
        if(i==Normal&&fmt!=DXGI_FORMAT_R8G8B8A8_UNORM)return;
        if(i==Motion&&fmt!=DXGI_FORMAT_R16G16_FLOAT&&fmt!=DXGI_FORMAT_R32G32_FLOAT)return;
        if((i==Scene||i==RRInput||i==RROut)&&fmt!=DXGI_FORMAT_R16G16B16A16_FLOAT&&fmt!=DXGI_FORMAT_R11G11B10_FLOAT)return;
        if(i==Depth&&fmt!=DXGI_FORMAT_R32_FLOAT&&fmt!=DXGI_FORMAT_R32_TYPELESS)return;
        UINT64 bytes=0;const auto base=(total+511)&~UINT64(511);
        owner->device->GetCopyableFootprints(&desc,0,1,base,&layouts[i],nullptr,nullptr,&bytes);
        if(bytes==UINT64_MAX||bytes==0||base>UINT64_MAX-bytes)return;
        total=base+bytes;
    }
    auto f=CaptureReader().Acquire(owner->device,total);
    if(!f){Issue(id,L"Readback budget reached");return;}
    f->bytes=total;f->layouts=layouts;f->session=id;f->number=eval.frame;f->epoch=owner->epoch;
    f->view=reinterpret_cast<UINT64>(in.camera.view);f->warm=ref->historyFrames;f->passes=ref->passes;
    f->w=eval.width;f->h=eval.height;f->reset=eval.reset!=0;
    f->jitterX=eval.jitterX;f->jitterY=eval.jitterY;f->scaleX=eval.scaleX;f->scaleY=eval.scaleY;
    f->outputRecorded=false;
    for(unsigned i=0;i<PlaneCount;++i){
        const auto& a=layouts[i].Footprint;
        unsigned bpp=a.Format==DXGI_FORMAT_R16G16B16A16_FLOAT||a.Format==DXGI_FORMAT_R32G32_FLOAT?8:4;
        f->g[i]={nullptr,a.Width,a.Height,a.RowPitch,bpp,unsigned(a.Format)};
    }
    slot.pair=f;f->sources[RROut]=eval.output;
    for(unsigned i=0;i<RROut;++i)Copy(in.commandList,*f,i,src[i],D3D12_RESOURCE_STATES(states[i]));
    if(!capturingTail.load())Issue(id,L"Watching visible characters");
}
inline void After(ID3D12GraphicsCommandList* list,const RREvaluationInputs& eval,unsigned result,bool bound) noexcept {
    if(!Armed()||!bound||(result&0xfff00000u)==0xbad00000u)return;
    if(!TryAcquireSRWLockExclusive(&rrLiveLock))return;
    __try {
        if(!rrLive)__leave;
        RRGuideInputTexture output{};RRGuideInputSnapshot now{};const char* why=nullptr;
        if(!RRInputCaptureFindOutput(eval.frame,eval.output,&output)||
           !RRGuideReadRendererContext(&now,&why)||now.engineFrame!=eval.frame||now.commandList!=list||now.queue!=rrLive->queue)__leave;
        for(auto& s:rrLive->slots){
            auto* f=s.characterColor.pair.get();
            if(f&&f->number==eval.frame&&f->session==Armed()&&!f->outputRecorded&&f->sources[RROut].Get()==eval.output){
                Copy(list,*f,RROut,eval.output,D3D12_RESOURCE_STATES(output.trackedState));f->outputRecorded=true;break;
            }
        }
    }__finally {ReleaseSRWLockExclusive(&rrLiveLock);}
}
}
