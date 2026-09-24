#pragma once
// Bit-preserving texture copy followed by integer UAV alpha replacement.
// Native target remains RT in the engine tracker; paired barriers restore it.
static bool RRDiffuseEmitCopy(control_rr_part1::CopyOp op,const RRGuideInputSnapshot* in,
    ID3D12Resource* source,ID3D12Resource* destination,DWORD* fault) noexcept {
    __try {
        auto* count=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(in->commandContext)+8);
        if(op==control_rr_part1::CopyOp::CopyBytes) {
            D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=source;to.pResource=destination;
            from.Type=to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            ++*count;in->commandList->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        } else {
            D3D12_RESOURCE_BARRIER b[2]{};
            for(auto& v:b){v.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;v.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;}
            b[0].Transition.pResource=source;b[1].Transition.pResource=destination;
            const bool initial=op==control_rr_part1::CopyOp::ToCopySource;
            b[0].Transition.StateBefore=initial?D3D12_RESOURCE_STATE_RENDER_TARGET:D3D12_RESOURCE_STATE_COPY_SOURCE;
            b[0].Transition.StateAfter=initial?D3D12_RESOURCE_STATE_COPY_SOURCE:D3D12_RESOURCE_STATE_RENDER_TARGET;
            b[1].Transition.StateBefore=initial?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_COPY_DEST;
            b[1].Transition.StateAfter=initial?D3D12_RESOURCE_STATE_COPY_DEST:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            ++*count;in->commandList->ResourceBarrier(2,b);
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();return false;}
}
static bool RRDiffuseCopy(const RRGuideInputSnapshot* in,ID3D12Resource* source,ID3D12Resource* destination,DWORD* fault) noexcept {
    control_rr_part1::CopyProgress progress{};
    return control_rr_part1::RecordCopy([&](control_rr_part1::CopyOp op) noexcept {
        return RRDiffuseEmitCopy(op,in,source,destination,fault);
    },progress);
}
