// Included where the live RR input textures and the fenced character slot are available.
namespace rr_pair {
inline void Before(RRLiveOwner* owner,RRCharacterColorSlot& slot,const RRCharacterEvalData& eval,
                   const RRGuideInputSnapshot& in,const RRGuideInputTexture* native,
                   ID3D12Resource* mask,const RRGuideInputTexture& remap){
    const auto id=Armed();if(!id)return;
    const auto& ref=slot.sourceLease;
    if(!ref||ref->session!=id||ref->historyFrames<4||!eval.motionKnown||rr_pair::fourRayFrame.load()!=eval.frame)return;
    constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    RRGuideInputTexture light[2]{};
    RRInputCaptureReadShaderTexture(kRRShaderLightBufferDiffuseRva,&light[0]);
    RRInputCaptureReadShaderTexture(kRRShaderLightBufferSpecularRva,&light[1]);
    std::array<ID3D12Resource*,PlaneCount> src{ref->raw.Get(),ref->temporal.Get(),ref->reference[ref->output].Get(),
        eval.color,slot.output.Get(),mask,eval.depth,in.gbuffer1.resource,eval.motion,remap.resource,light[0].resource,light[1].resource,eval.output};
    std::array<UINT,PlaneCount> states{read,read,read,native[0].trackedState,read,D3D12_RESOURCE_STATE_RENDER_TARGET,
        native[1].trackedState,in.gbuffer1.trackedState,native[2].trackedState,remap.trackedState,light[0].trackedState,light[1].trackedState,D3D12_RESOURCE_STATE_UNORDERED_ACCESS};
    UINT64 total=0;
    std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT,PlaneCount> layouts{};
    for(unsigned i=0;i<PlaneCount;++i){
        if(i==Diffuse||i==Specular){
            const auto& t=light[i-Diffuse];
            if(!t.resource||!t.trackedState||(t.desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT&&t.desc.Format!=DXGI_FORMAT_R11G11B10_FLOAT)||
               t.desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||t.desc.Width!=eval.width||t.desc.Height!=eval.height||
               t.desc.SampleDesc.Count!=1||t.desc.DepthOrArraySize!=1){src[i]=nullptr;continue;}
        }
        if(!src[i])return;auto desc=src[i]->GetDesc();auto fmt=desc.Format;
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.SampleDesc.Count!=1||desc.DepthOrArraySize!=1)return;
        if(i<=Spatial||i==Mask){if(fmt!=DXGI_FORMAT_R16G16B16A16_FLOAT)return;}
        if(i==Normal&&fmt!=DXGI_FORMAT_R8G8B8A8_UNORM)return;
        if(i==Motion&&fmt!=DXGI_FORMAT_R16G16_FLOAT&&fmt!=DXGI_FORMAT_R32G32_FLOAT)return;
        if((i==Scene||i==RRInput||i==RROut)&&fmt!=DXGI_FORMAT_R16G16B16A16_FLOAT&&fmt!=DXGI_FORMAT_R11G11B10_FLOAT)return;
        if(i==Depth&&fmt!=DXGI_FORMAT_R32_FLOAT&&fmt!=DXGI_FORMAT_R32_TYPELESS)return;
        UINT rows=0;UINT64 rowBytes=0;
        const auto base=(total+511)&~UINT64(511);
        owner->device->GetCopyableFootprints(&desc,0,1,base,&layouts[i],&rows,&rowBytes,nullptr);
        if(!rows||!rowBytes||rowBytes>layouts[i].Footprint.RowPitch||layouts[i].Offset!=base)return;
        total=layouts[i].Offset+UINT64(rows-1)*layouts[i].Footprint.RowPitch+rowBytes;
    }
    auto f=CaptureReader().Acquire(owner->device,total);
    if(!f){Issue(id,L"Readback budget reached");return;}
    f->bytes=total;f->layouts=layouts;f->session=id;f->number=eval.frame;f->epoch=owner->epoch;
    f->view=reinterpret_cast<UINT64>(in.camera.view);f->warm=ref->historyFrames;f->passes=ref->passes;
    f->w=eval.width;f->h=eval.height;f->reset=eval.reset!=0;
    f->jitterX=eval.jitterX;f->jitterY=eval.jitterY;f->scaleX=eval.scaleX;f->scaleY=eval.scaleY;
    f->outputRecorded=false;f->recordedList=in.commandList;
    for(unsigned i=0;i<PlaneCount;++i){
        const auto& a=layouts[i].Footprint;
        unsigned bpp=a.Format==DXGI_FORMAT_R16G16B16A16_FLOAT||a.Format==DXGI_FORMAT_R32G32_FLOAT?8:4;
        f->g[i]={nullptr,a.Width,a.Height,a.RowPitch,bpp,unsigned(a.Format)};
    }
    slot.pair=f;f->sources[RROut]=eval.output;
    auto& commands=*reinterpret_cast<UINT64*>(static_cast<unsigned char*>(in.commandContext)+8);
    for(unsigned i=0;i<RROut;++i)if(src[i])Copy(in.commandList,commands,*f,i,src[i],D3D12_RESOURCE_STATES(states[i]));
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
            if(f&&f->number==eval.frame&&f->session==Armed()&&f->recordedList==list&&!f->outputRecorded&&f->sources[RROut].Get()==eval.output){
                auto& commands=*reinterpret_cast<UINT64*>(static_cast<unsigned char*>(now.commandContext)+8);
                Copy(list,commands,*f,RROut,eval.output,D3D12_RESOURCE_STATES(output.trackedState));f->outputRecorded=true;break;
            }
        }
    }__finally {ReleaseSRWLockExclusive(&rrLiveLock);}
}
}
