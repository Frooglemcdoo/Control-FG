#pragma once
// Query/readback ownership follows the existing image job. Failed jobs retain
// resources; successful jobs release only after their same-queue GPU fence.
struct RRStageTimingTicket { unsigned slot=2,index=~0u;void* context=nullptr;UINT64 frame=0,present=0;DWORD thread=0; };
static RRStageTimingTicket RRStageTimingBegin(ID3D12GraphicsCommandList* list,gi_stage_timing::Stage stage) noexcept {
 RRStageTimingTicket ticket{};
#if defined(CONTROL_RR_GI_SEQUENCE) && defined(CONTROL_GI_BOUNDARY_CAPTURE)
 if(!list||!gi_path_capture::timedSequence.load()||!control_rr_gi::boundaryRequested.load()||!rrInputCaptureSettledFrame)return ticket;
 const DWORD saved=GetLastError();
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return ticket;
 __try {
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  if(!RRGuideReadRendererContext(&native,&reason)||native.commandList!=list||
     list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)__leave;
  const auto frame=native.engineFrame;
  const unsigned slot=stage==gi_stage_timing::Stage::Reconstruction?rrInputCaptureRecording:
   control_rr::SelectInputCapturePair(rrInputCaptures[0].policy.phase,rrInputCaptures[1].policy.phase,rrInputCaptures[0].frame,frame);
  if(slot>=2)__leave;
  auto& c=rrInputCaptures[slot];
  const bool reconstruction=stage==gi_stage_timing::Stage::Reconstruction;
  if(!c.timedSequence||!c.timingQueries||!c.timingReadback||c.queue!=native.queue||
     c.policy.phase!=(reconstruction?control_rr::InputCapturePolicy::Recorded:control_rr::InputCapturePolicy::Ready)||
     (reconstruction?c.frame!=frame:frame!=rrInputCaptureSettledFrame+1))__leave;
  if(c.timing.frame&&c.timing.frame!=frame){c.policy.phase=control_rr::InputCapturePolicy::Failed;__leave;}
  const unsigned index=c.timing.Begin(frame,native.presentToken,stage);
  if(index==~0u)__leave;
  c.timingStages[index]=stage;
  list->EndQuery(c.timingQueries,D3D12_QUERY_TYPE_TIMESTAMP,index*2);
  ++*reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(native.commandContext)+8);
  ticket={slot,index,native.commandContext,frame,native.presentToken,GetCurrentThreadId()};
 }__except(EXCEPTION_EXECUTE_HANDLER){
  for(auto& c:rrInputCaptures)if(c.timing.count)c.policy.phase=control_rr::InputCapturePolicy::Failed;
 }
 ReleaseSRWLockExclusive(&rrInputCaptureLock);SetLastError(saved);
#else
 (void)list;(void)stage;
#endif
 return ticket;
}
static void RRStageTimingEnd(RRStageTimingTicket ticket,ID3D12GraphicsCommandList* list) noexcept {
 if(ticket.slot>=2||ticket.index>=gi_stage_timing::capacity||!list)return;
 const DWORD saved=GetLastError();
 AcquireSRWLockExclusive(&rrInputCaptureLock);
 auto& c=rrInputCaptures[ticket.slot];
 __try {
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  if(!RRGuideReadRendererContext(&native,&reason)||native.commandList!=list||native.commandContext!=ticket.context||
     native.engineFrame!=ticket.frame||native.presentToken!=ticket.present||native.queue!=c.queue||GetCurrentThreadId()!=ticket.thread){c.policy.phase=control_rr::InputCapturePolicy::Failed;__leave;}
  if(c.timingEnded[ticket.index]){c.policy.phase=control_rr::InputCapturePolicy::Failed;__leave;}
  list->EndQuery(c.timingQueries,D3D12_QUERY_TYPE_TIMESTAMP,ticket.index*2+1);
  list->ResolveQueryData(c.timingQueries,D3D12_QUERY_TYPE_TIMESTAMP,ticket.index*2,2,c.timingReadback,UINT64(ticket.index)*16);
  *reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(ticket.context)+8)+=2;
  c.timingEnded[ticket.index]=true;++c.timing.ended;
 }__except(EXCEPTION_EXECUTE_HANDLER){c.policy.phase=control_rr::InputCapturePolicy::Failed;}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);SetLastError(saved);
}

static void RRStageTimingBatch(RRStageTimingTicket ticket,unsigned batch,unsigned total,bool valid) noexcept {
 if(ticket.slot>=2)return;
 AcquireSRWLockExclusive(&rrInputCaptureLock);
 auto& c=rrInputCaptures[ticket.slot];
 c.timing.Batch(batch,total,valid&&c.timing.frame==ticket.frame);
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}
