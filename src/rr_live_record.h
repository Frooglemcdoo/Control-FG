#pragma once
#ifndef RR_SEH_TRY
#define RR_SEH_TRY __try
#endif
#ifndef RR_SEH_EXCEPT
#define RR_SEH_EXCEPT(filter) __except(filter)
#endif
struct RRLiveConstants {UINT invertSmoothness,guideFlags,width,height;};
static_assert(sizeof(RRLiveConstants)==16,"RenoDX-style live guide constant ABI");
// POD-only recording leaf. All inputs are already compute-readable. The guide
// pass owns only its three RGBA16F UAV outputs and restores them to SRV state
// before native NGX evaluation consumes them.
static bool RRLiveRecord(RRLiveOwner* c,RRLiveSlot* s,const RRGuideInputSnapshot* in,
    const RRLiveConstants* constants,DWORD* fault) noexcept {
    RR_SEH_TRY {
        auto* list=in->commandList;
        auto* count=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(in->commandContext)+8);
        D3D12_RESOURCE_BARRIER barriers[3]{};ID3D12Resource* outputs[3]{s->normal,s->diffuse,s->specular};
        for(UINT i=0;i<3;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=outputs[i];b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}
        ++*count;list->ResourceBarrier(3,barriers);
        ++*count;list->SetDescriptorHeaps(1,&s->heap);
        ++*count;list->SetComputeRootSignature(c->root);
        ++*count;list->SetPipelineState(c->pipeline);
        ++*count;list->SetComputeRoot32BitConstants(0,4,constants,0);
        ++*count;list->SetComputeRootDescriptorTable(1,s->heap->GetGPUDescriptorHandleForHeapStart());
        ++*count;list->Dispatch((c->width+15)/16,(c->height+15)/16,1);
        for(auto& b:barriers){b.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;b.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
        ++*count;list->ResourceBarrier(3,barriers);
        return true;
    } RR_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();return false;}
}
