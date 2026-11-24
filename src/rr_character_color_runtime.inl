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

#include "rr_paired_runtime.inl"

// Native materials have already consumed the selected GI. Capture the real
// composed color and NGX input directly without substituting a late color pass.
static const char* RRCharacterRecordColor(RRLiveOwner* owner,RRLiveSlot* live,const RRGuideInputSnapshot& in,
                                         const RRCharacterEvalData& eval) noexcept {
    auto& slot=live->characterColor;
    slot.Retire();
    if(!rr_pair::Armed())return rrCharacterGISourceReason.load();
    if(eval.frame!=in.engineFrame||rrInputCaptureNativeFrame!=eval.frame||!rrDiffuse||
       rrDiffuse->frame!=eval.frame||rrDiffuse->queue!=in.queue)return "capture_frame";
    auto* mask=RRDiffuseCharacterCurrent(eval.frame,eval.width,eval.height);
    RRGuideInputTexture native[3]{},remap{};
    ID3D12Resource* aa[]={eval.color,eval.depth,eval.motion};
    if(!mask||!RRInputCaptureFindNative(eval.frame,aa,native)||
       !RRInputCaptureReadShaderTexture(kRRShaderGBufferReconstructionRva,&remap)||
       remap.desc.Format!=DXGI_FORMAT_R32_UINT||!RRCharacterUsesSingleSample())return "capture_resources";
    slot.sourceLease=RRCharacterGIForColor(eval.frame,reinterpret_cast<UINT64>(in.camera.view),in.queue,in.commandList,eval.width,eval.height);
    try{rr_pair::Before(owner,slot,eval,in,native,mask,remap);}
    catch(...){rr_pair::Issue(rr_pair::Armed(),L"Unable to allocate capture");}
    return rrCharacterGISourceReason.load();
}

static void RRCharacterBeforeGuideDispatch(RRLiveOwner* owner,RRLiveSlot* slot,const RRGuideInputSnapshot* in) noexcept {
    if(!rrCharacterEvaluationInputs)return;
    const auto* reason=RRCharacterRecordColor(owner,slot,*in,*rrCharacterEvaluationInputs);
    rrCharacterBlendReason.store(reason);
    rrCharacterBlendAttemptFrame.store(in->engineFrame);
}
