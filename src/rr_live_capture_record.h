#pragma once
// Owned output textures only; native GBuffer/depth/material states untouched.
static bool RRLiveCaptureRecord(RRLiveCapture* capture,RRLiveSlot* slot,const RRGuideInputSnapshot* in,DWORD* fault) noexcept {
 __try {
  auto* list=in->commandList;
  auto* count=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(in->commandContext)+8);
  ID3D12Resource* sources[3]{slot->normal,slot->specular,slot->diffuse};
  D3D12_RESOURCE_BARRIER barriers[3]{};
  for(UINT i=0;i<3;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=sources[i];b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;}
  ++*count;list->ResourceBarrier(3,barriers);
  for(UINT i=0;i<3;++i){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=sources[i];from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.pResource=capture->readback[i];to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=capture->footprint[i];++*count;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
  for(auto& b:barriers){b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
  ++*count;list->ResourceBarrier(3,barriers);return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();return false;}
}
