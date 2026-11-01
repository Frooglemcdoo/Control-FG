#pragma once
// Source provider audited at renderer+0x1553d: g_DLF_rwtColorSource.
// Called AFTER the native temporal dispatch has committed SRV states, BEFORE
// the enclosing filter copies/composes its output. R262 only reads this source.
static void RRInputCaptureAtGI(unsigned long long frame,ID3D12GraphicsCommandList* list) noexcept {
 if(!control_rr_gi::boundaryRequested.load()||!rrInputCaptureSettledFrame||frame!=rrInputCaptureSettledFrame+1)return;
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 RRInputCaptureJob* active=nullptr;
 __try {
  using P=control_rr::InputCapturePolicy;
  const unsigned slot=control_rr::SelectInputCapturePair(rrInputCaptures[0].policy.phase,rrInputCaptures[1].policy.phase,rrInputCaptures[0].frame,frame);
  if(slot>=2)__leave;
  auto* c=&rrInputCaptures[slot];active=c;
  if(c->policy.phase!=P::Ready||c->preGICaptured||c->preGIIndex>=c->count)__leave;
  if(c->giHits.enabled&&!c->giHits.ledger.Seal(frame)){
   c->policy.phase=P::Failed;Log("GI_HITS_R271_FAILED reason=incomplete_batches frame=%llu recorded=%u expected=%u",frame,c->giHits.ledger.recorded,c->giHits.ledger.expected);__leave;
  }
  RRGuideInputTexture input{};RRGuideInputSnapshot native{};const char* reason=nullptr;
  if(!RRInputCaptureReadShaderTexture(0x128f3c8,&input)||
     !RRGuideReadRendererContext(&native,&reason)||native.engineFrame!=frame||native.commandList!=list||native.queue!=c->queue){
   control_rr_capture_ui::detail.store(L"Waiting for verified pre-composition GI read state.");
   static unsigned rejects=0;if(++rejects<=4||rejects%240==0)Log("RR_INPUT_CAPTURE_R263_REJECT reason=early_gi_state_or_context frame=%llu",frame);
   __leave;
  }
  const unsigned index=c->preGIIndex;const auto& d=c->descriptions[index];
  if(input.desc.Dimension!=d.Dimension||input.desc.Format!=d.Format||input.desc.Width!=d.Width||input.desc.Height!=d.Height||input.desc.SampleDesc.Count!=1||input.desc.DepthOrArraySize!=1||input.desc.MipLevels<1)__leave;
  ID3D12Device* device=nullptr;const HRESULT hr=input.resource->GetDevice(IID_PPV_ARGS(&device));
  const bool same=SUCCEEDED(hr)&&device==c->device;if(device)device->Release();if(!same)__leave;
  RRInputCaptureJob copy{};copy.count=1;copy.sources[0]=input.resource;copy.readback[0]=c->readback[index];copy.footprint[0]=c->footprint[index];copy.states[0]=static_cast<D3D12_RESOURCE_STATES>(input.trackedState);
  c->sources[index]=input.resource;input.resource->AddRef();c->states[index]=copy.states[0];
  DWORD fault=0;bool started=false;
  c->preGICaptured=RRInputCaptureRecordGuarded(&copy,&native,&input,&fault,&started,1);
  if(!c->preGICaptured){
   if(started){c->policy.phase=P::Failed;Log("RR_INPUT_CAPTURE_R263_FAILED reason=early_gi_record resources_retained=1");}
   else RRGuideRelease(c->sources[index]);
   __leave;
  }
  c->preGIFrame=frame;c->preGIPresent=native.presentToken;c->preGITick=GetTickCount64();
  Log("RR_INPUT_CAPTURE_R263_EARLY_GI frame=%llu pair=%u slot=%u source=%p state=%u",frame,c->policy.requests,slot,input.resource,input.trackedState);
 }__except(EXCEPTION_EXECUTE_HANDLER){if(active)active->policy.phase=control_rr::InputCapturePolicy::Failed;}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}
