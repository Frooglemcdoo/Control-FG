#pragma once
#include "rr_input_capture_limits.h"
static bool RRInputCaptureRecord(RRInputCaptureJob* c,const RRGuideInputSnapshot* native,DWORD* fault,bool* started) noexcept {
 bool initialDone=false,restoreAttempted=false,okay=false;
 D3D12_RESOURCE_BARRIER barriers[kRRInputCaptureMaxResources]{},restore[kRRInputCaptureMaxResources]{};
 auto* list=native->commandList;auto* commands=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(native->commandContext)+8);
 RR_SEH_TRY {
  for(unsigned i=0;i<c->count;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=c->sources[i];b.Transition.Subresource=0;b.Transition.StateBefore=c->states[i];b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
   restore[i]=b;restore[i].Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;restore[i].Transition.StateAfter=c->states[i];}
  *started=true;++*commands;list->ResourceBarrier(c->count,barriers);initialDone=true;
  for(unsigned i=0;i<c->count;++i){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=c->sources[i];from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   to.pResource=c->readback[i];to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=c->footprint[i];++*commands;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
  restoreAttempted=true;++*commands;list->ResourceBarrier(c->count,restore);okay=true;
 } RR_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();}
 // Restore after a copy fault; never retry an ambiguous barrier call.
 if(initialDone&&!restoreAttempted){
  RR_SEH_TRY {++*commands;list->ResourceBarrier(c->count,restore);}
  RR_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();}
 }
 return okay;
}
