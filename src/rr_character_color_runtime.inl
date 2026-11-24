#pragma once
#include "rr_character_root.h"
#include "../build/gi_character_blend.h"
#include "../build/gi_character_measured.h"

struct RRCharacterEvalData : RREvaluationInputs {float scaleX=0,scaleY=0;bool motionKnown=false;};
static thread_local const RRCharacterEvalData* rrCharacterEvaluationInputs=nullptr;

static void RRCharacterPrepareEvaluation(unsigned branch,ID3D12GraphicsCommandList* list,
    const control_rr::LiveFrameInput& frame,const RREvaluationInputs& base,void* parameters) noexcept {
    RRCharacterEvalData in{base};
    const auto* previous=rrCharacterEvaluationInputs;
    rrCharacterEvaluationInputs=branch==1 && rrCharacterBlendRequested.load() && !in.fault && in.valid==127 ? &in : nullptr;
    if(rrCharacterEvaluationInputs && ngxGetInt && ngxGetFloat){
        __try {
        int createFlags=0;
        const bool flagsKnown=ngxGetInt(parameters,"DLSS.Feature.Create.Flags",&createFlags)==1;
        in.motionKnown=(!flagsKnown || (createFlags&4)==0) &&
            ngxGetFloat(parameters,"MV.Scale.X",&in.scaleX)==1 && ngxGetFloat(parameters,"MV.Scale.Y",&in.scaleY)==1 &&
            std::isfinite(in.scaleX) && std::isfinite(in.scaleY) && in.scaleX!=0 && in.scaleY!=0 &&
            std::isfinite(in.jitterX) && std::isfinite(in.jitterY);
        } __except(EXCEPTION_EXECUTE_HANDLER) {in.motionKnown=false;}
    }
    __try {RRLiveBeforeEvaluation(list,frame);}
    __finally {rrCharacterEvaluationInputs=previous;}
}

static bool RRCharacterPrepareSlot(RRLiveOwner* owner,RRCharacterColorSlot& slot,DXGI_FORMAT format) noexcept {
    auto* device=owner->device;
    if(!slot.normal){
        Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
        if(FAILED(RRCharacterSerializeRoot(&blob,&error)) ||
           FAILED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root))))return false;
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={kGICharacterBlend,sizeof(kGICharacterBlend)};
        if(FAILED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&slot.normal))))return false;
        pd.CS={kGICharacterMeasured,sizeof(kGICharacterMeasured)};
        device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&slot.measured));
        slot.root=std::move(root);
    }
    if(!slot.heap){
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hd.NumDescriptors=10;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&slot.heap))))return false;
    }
    if(slot.output){
        auto d=slot.output->GetDesc();
        if(d.Width==owner->width && d.Height==owner->height && d.Format==format)return true;
        slot.output.Reset();
    }
    D3D12_FEATURE_DATA_FORMAT_SUPPORT support{};support.Format=format;
    if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support))) ||
       !(support.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))return false;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=owner->width;d.Height=owner->height;
    d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Format=format;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    return SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        nullptr,IID_PPV_ARGS(&slot.output)));
}

static const char* RRCharacterRecordColor(RRLiveOwner* owner,RRLiveSlot* live,const RRGuideInputSnapshot& in,
                                         const RRCharacterEvalData& eval) noexcept {
    if(rrCharacterRawFrame.load()!=eval.frame)return "raw_gi_not_seen";
    if(eval.frame!=in.engineFrame || rrInputCaptureNativeFrame!=eval.frame)return "aa_frame";
    if(rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Joined)return "replay_not_joined";
    if(!rrDiffuse || rrDiffuse->frame!=eval.frame)return "replay_frame";
    // The primary-view replay rejects draws from other views at draw and join.
    // Use the renderer/queue provenance of the live RR guide and the actual NGX depth.
    if(rrDiffuse->renderer!=reinterpret_cast<UINT64>(in.camera.renderer) || rrDiffuse->queue!=in.queue)return "replay_owner";
    auto* mask=RRDiffuseCharacterCurrent(eval.frame,eval.width,eval.height);
    if(!mask)return "no_complete_mask";
    RRGuideInputTexture native[3]{},gi{};
    ID3D12Resource* aa[]={eval.color,eval.depth,eval.motion};
    if(!RRInputCaptureFindNative(eval.frame,aa,native))return "aa_resource_state";
    if(!RRInputCaptureReadGI(&gi))return "raw_gi_at_rr";
    const auto cf=native[0].desc.Format,df=native[1].desc.Format;
    if((cf!=DXGI_FORMAT_R11G11B10_FLOAT && cf!=DXGI_FORMAT_R16G16B16A16_FLOAT) ||
       (df!=DXGI_FORMAT_R32_TYPELESS && df!=DXGI_FORMAT_R32_FLOAT) ||
       gi.desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT)return "input_format";
    ID3D12Resource* inputs[]={gi.resource,mask,eval.depth,in.gbuffer1.resource,in.gbuffer2.resource,eval.color};
    const DXGI_FORMAT formats[]={gi.desc.Format,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R32_FLOAT,
        in.gbuffer1.desc.Format,in.gbuffer2.desc.Format,cf};
    const UINT states[]={gi.trackedState,D3D12_RESOURCE_STATE_RENDER_TARGET,native[1].trackedState,
        in.gbuffer1.trackedState,in.gbuffer2.trackedState,native[0].trackedState};
    for(UINT i=0;i<6;++i){
        const auto d=inputs[i]->GetDesc();
        if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || d.Width!=eval.width || d.Height!=eval.height ||
           d.DepthOrArraySize!=1 || d.SampleDesc.Count!=1 || !d.MipLevels || (d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) ||
           (i==5 && d.MipLevels!=1))return "input_extent_or_flags";
        for(UINT j=0;j<i;++j)if(inputs[i]==inputs[j])return "input_alias";
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        if(FAILED(inputs[i]->GetDevice(IID_PPV_ARGS(&device))) || device.Get()!=owner->device)return "input_device";
    }
    auto& slot=live->characterColor;
    slot.Retire(); // The guide lease just admitted this free slot, including its GPU fence.
    if(!RRCharacterPrepareSlot(owner,slot,cf))return "slot_allocation";
    auto cpu=slot.heap->GetCPUDescriptorHandleForHeapStart();
    for(UINT i=0;i<6;++i){
        D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;d.Format=formats[i];
        d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d.Texture2D.MipLevels=1;
        owner->device->CreateShaderResourceView(inputs[i],&d,cpu);cpu.ptr+=owner->increment;
    }
    D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=cf;u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
    owner->device->CreateUnorderedAccessView(slot.output.Get(),nullptr,&u,cpu);
    const auto& mv=native[2];
    auto& history=owner->characterHistory;
    RRCharacterHistoryParams hp{};
    const bool motionOK=eval.motionKnown && mv.desc.Width==eval.width && mv.desc.Height==eval.height &&
        mv.desc.MipLevels==1 && mv.desc.SampleDesc.Count==1 && mv.desc.DepthOrArraySize==1 &&
        !(mv.desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) &&
        (mv.desc.Format==DXGI_FORMAT_R16G16_FLOAT || mv.desc.Format==DXGI_FORMAT_R32G32_FLOAT);
    if(motionOK && history.Prepare(owner->device,eval.width,eval.height,owner->epoch)) {
        hp=RRCharacterHistoryForFrame(history,eval.frame,rrCharacterEvaluatedFrame.load(),
            in.camera.view,eval.reset,eval.jitterX,eval.jitterY,eval.scaleX,eval.scaleY);
    }
    // Prepare may have reset the history index on resize.
    const UINT writeIndex=1-history.previous;
    D3D12_SHADER_RESOURCE_VIEW_DESC hs{};hs.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    hs.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;hs.Texture2D.MipLevels=1;
    hs.Format=motionOK?mv.desc.Format:DXGI_FORMAT_R16G16_FLOAT;cpu.ptr+=owner->increment;
    owner->device->CreateShaderResourceView(hp.mode?eval.motion:nullptr,&hs,cpu);
    hs.Format=DXGI_FORMAT_R32G32B32A32_UINT;cpu.ptr+=owner->increment;
    owner->device->CreateShaderResourceView(hp.mode==2?history.image[history.previous].Get():nullptr,&hs,cpu);
    u.Format=hs.Format;cpu.ptr+=owner->increment;
    owner->device->CreateUnorderedAccessView(hp.mode?history.image[writeIndex].Get():nullptr,nullptr,&u,cpu);
    if(hp.mode){
        slot.held[4]=mv.resource;slot.held[5]=history.image[history.previous];slot.held[6]=history.image[writeIndex];
    }
    slot.held[0]=gi.resource;slot.held[1]=mask;slot.held[2]=eval.depth;slot.held[3]=eval.color;
    static UINT64 calls=0;
    const bool sample=(calls++%240)==0;
    const bool measuring=sample && slot.measured && slot.sample.Prepare(owner->device,eval.frame);
    if(sample && !measuring)Log("GI_CHARACTER_BLEND_R502 frame=%llu sample=unavailable",eval.frame);
    auto* list=in.commandList;
    auto& commands=*reinterpret_cast<UINT64*>(static_cast<unsigned char*>(in.commandContext)+8);
    D3D12_RESOURCE_BARRIER barriers[9]{};UINT count=0;
    auto transition=[&](ID3D12Resource* res,UINT from,UINT to){
        auto& b=barriers[count++];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={res,0,D3D12_RESOURCE_STATES(from),D3D12_RESOURCE_STATES(to)};
    };
    // Use the exact source bits outside the correction, including alpha.
    transition(eval.color,states[5],D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition(slot.output.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
    ++commands;list->ResourceBarrier(count,barriers);count=0;
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=eval.color;dst.pResource=slot.output.Get();
    src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    ++commands;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    transition(slot.output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    transition(eval.color,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    for(UINT i=0;i<5;++i)if(!(states[i]&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))
        transition(inputs[i],states[i],D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    const bool moveMotion=hp.mode && !(mv.trackedState&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if(moveMotion)transition(mv.resource,mv.trackedState,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if(hp.mode)transition(history.image[writeIndex].Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ++commands;list->ResourceBarrier(count,barriers);count=0;
    ID3D12DescriptorHeap* heap=slot.heap.Get();++commands;list->SetDescriptorHeaps(1,&heap);
    ++commands;list->SetComputeRootSignature(slot.root.Get());++commands;list->SetPipelineState(measuring?slot.measured.Get():slot.normal.Get());
    if(measuring){commands+=3;slot.sample.Begin(list);}
    ++commands;list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());
    ++commands;list->SetComputeRoot32BitConstants(2,5,&hp,0);
    ++commands;list->Dispatch((eval.width+7)/8,(eval.height+7)/8,1);
    if(measuring){commands+=2;slot.sample.End(list);}
    for(UINT i=0;i<5;++i)if(!(states[i]&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))
        transition(inputs[i],D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,states[i]);
    if(moveMotion)transition(mv.resource,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,mv.trackedState);
    if(hp.mode)transition(history.image[writeIndex].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if(states[5]!=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        transition(eval.color,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,states[5]);
    transition(slot.output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    ++commands;list->ResourceBarrier(count,barriers);
    if(hp.mode){
        history.previous=writeIndex;history.frame=eval.frame;history.view=in.camera.view;
        history.jitterX=eval.jitterX;history.jitterY=eval.jitterY;
        history.scaleX=eval.scaleX;history.scaleY=eval.scaleY;
    } else history.frame=0;
    slot.writtenFrame=eval.frame;
    return hp.mode==2?"recorded_history":hp.mode==1?"recorded_seed":"recorded_spatial";

}

static void RRCharacterBeforeGuideDispatch(RRLiveOwner* owner,RRLiveSlot* slot,const RRGuideInputSnapshot* in) noexcept {
    if(!rrCharacterEvaluationInputs)return;
    const auto* reason=RRCharacterRecordColor(owner,slot,*in,*rrCharacterEvaluationInputs);
    rrCharacterBlendReason.store(reason);
    rrCharacterBlendAttemptFrame.store(in->engineFrame);
}
