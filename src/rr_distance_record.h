#pragma once
static bool RRDistanceRecordLeaf(RRDistanceOwner* owner,RRDistanceSlot* slot,const RRGuideInputSnapshot* native,
 const RRDistanceConstants* constants,DWORD* fault) noexcept {
 __try {
  auto* list=native->commandList;auto* count=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(native->commandContext)+8);
  ID3D12Resource* resources[2]{slot->output,slot->status};D3D12_RESOURCE_BARRIER barriers[2]{};
  for(UINT i=0;i<2;++i){auto& barrier=barriers[i];barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=resources[i];barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}
  ++*count;list->ResourceBarrier(2,barriers);++*count;list->SetDescriptorHeaps(1,&slot->heap);
  ++*count;list->SetComputeRootSignature(owner->root);++*count;list->SetPipelineState(owner->pipeline);
  ++*count;list->SetComputeRootDescriptorTable(0,slot->heap->GetGPUDescriptorHandleForHeapStart());
  ++*count;list->SetComputeRoot32BitConstants(1,36,constants,0);
  ++*count;list->Dispatch((owner->width+7)/8,(owner->height+7)/8,1);
  for(auto& barrier:barriers){barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
  ++*count;list->ResourceBarrier(2,barriers);return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();return false;}
}
static bool RRDistanceCaptureLeaf(RRDistanceOwner* owner,RRDistanceSlot* slot,const RRGuideInputSnapshot* native,DWORD* fault) noexcept {
 __try {
  auto* list=native->commandList;auto* count=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(native->commandContext)+8);
  ID3D12Resource* resources[2]{slot->output,slot->status};D3D12_RESOURCE_BARRIER barriers[2]{};
  for(UINT i=0;i<2;++i){auto& barrier=barriers[i];barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=resources[i];barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;}
  ++*count;list->ResourceBarrier(2,barriers);
  for(UINT i=0;i<2;++i){D3D12_TEXTURE_COPY_LOCATION source{},destination{};source.pResource=resources[i];source.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   destination.pResource=owner->readback[i];destination.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;destination.PlacedFootprint=owner->footprint[i];
   ++*count;list->CopyTextureRegion(&destination,0,0,0,&source,nullptr);
  }
  for(auto& barrier:barriers){barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
  ++*count;list->ResourceBarrier(2,barriers);return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();return false;}
}
