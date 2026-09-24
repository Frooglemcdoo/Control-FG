#pragma once
static bool RRInputCaptureRecord(RRInputCaptureJob* c,const RRGuideInputSnapshot* native,DWORD* fault) noexcept {
 RR_SEH_TRY {
  auto* list=native->commandList;auto* commands=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(native->commandContext)+8);
  D3D12_RESOURCE_BARRIER barriers[5]{};
  for(unsigned i=0;i<c->count;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=c->sources[i];b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;}
  ++*commands;list->ResourceBarrier(c->count,barriers);
  for(unsigned i=0;i<c->count;++i){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=c->sources[i];from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   to.pResource=c->readback[i];to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=c->footprint[i];++*commands;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
  for(unsigned i=0;i<c->count;++i){barriers[i].Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;barriers[i].Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
  ++*commands;list->ResourceBarrier(c->count,barriers);return true;
 } RR_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();return false;}
}
