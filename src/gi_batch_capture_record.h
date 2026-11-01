#pragma once
// Called synchronously immediately after the audited native GI shading dispatch.
static void RRInputCaptureGIBatch(unsigned long long frame,ID3D12GraphicsCommandList* list,
 unsigned batch,unsigned batches,unsigned rays,bool constantsValid) noexcept {
 if(!GIBatchCaptureEnabled()||!control_rr_gi::boundaryRequested.load()||!rrInputCaptureSettledFrame||frame!=rrInputCaptureSettledFrame+1)return;
 // Losing a batch to lock contention is detected by the contiguous batch ledger.
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 RRInputCaptureJob* active=nullptr;
 __try {
  using P=control_rr::InputCapturePolicy;
  const unsigned slot=control_rr::SelectInputCapturePair(rrInputCaptures[0].policy.phase,rrInputCaptures[1].policy.phase,rrInputCaptures[0].frame,frame);
  if(slot>=2)__leave;
  auto* c=&rrInputCaptures[slot];active=c;auto& capture=c->giHits;
  if(c->policy.phase!=P::Ready||!capture.enabled||!capture.readback)__leave;
  if(!constantsValid||!capture.ledger.Begin(frame,batch,batches,rays)){
   capture.ledger.failed=true;c->policy.phase=P::Failed;Log("GI_HITS_R271_FAILED reason=batch_sequence_or_constants frame=%llu batch=%u expected=%u rays=%u",frame,batch,batches,rays);__leave;
  }
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  RRGuideInputTexture input[3]{};
  const size_t providers[3]{0x1291888,0x12918b0,0x12918d8};
  bool valid=RRGuideReadRendererContext(&native,&reason)&&native.engineFrame==frame&&native.commandList==list&&native.queue==c->queue;
  for(unsigned i=0;i<3&&valid;++i)valid=RRInputCaptureReadShaderTexture(providers[i],&input[i]);
  for(unsigned i=0;i<3&&valid;++i){const auto& d=input[i].desc;
   valid=d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width>=128&&d.Width<=4096&&d.Height>=128&&d.Height<=2160&&
    d.DepthOrArraySize>=rays&&d.DepthOrArraySize<=gi_batch_capture::MaxSlices&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.SampleDesc.Quality==0&&GIBatchCapturePixelBytes(d.Format)!=0;
   valid=valid&&(i==0?(d.Format==DXGI_FORMAT_R16_UINT||d.Format==DXGI_FORMAT_R32_UINT||d.Format==DXGI_FORMAT_R32G32_UINT||d.Format==DXGI_FORMAT_R32G32B32A32_UINT):
                         (d.Format==DXGI_FORMAT_R16G16B16A16_FLOAT||d.Format==DXGI_FORMAT_R32G32B32A32_FLOAT));
   if(i)valid=valid&&d.Width==input[0].desc.Width&&d.Height==input[0].desc.Height&&d.DepthOrArraySize==input[0].desc.DepthOrArraySize;
   for(unsigned j=0;j<i;++j)if(input[i].resource==input[j].resource)valid=false;
   ID3D12Device* device=nullptr;
   if(valid){const HRESULT hr=input[i].resource->GetDevice(IID_PPV_ARGS(&device));valid=SUCCEEDED(hr)&&device==c->device;if(device)device->Release();}
  }
  if(!valid){for(unsigned i=0;i<3;++i)Log("GI_HITS_R271_INPUT index=%u width=%llu height=%u slices=%u mips=%u format=%u state=%u",i,input[i].desc.Width,input[i].desc.Height,input[i].desc.DepthOrArraySize,input[i].desc.MipLevels,unsigned(input[i].desc.Format),input[i].trackedState);capture.ledger.failed=true;c->policy.phase=P::Failed;Log("GI_HITS_R271_FAILED reason=input_state_format_or_context frame=%llu batch=%u",frame,batch);__leave;}
  capture.present=native.presentToken;
  auto& b=capture.batches[batch];UINT64 end=capture.used;
  b.x[0]=input[0].desc.Width<256?0:static_cast<unsigned>(input[0].desc.Width/4)-64;b.x[1]=static_cast<unsigned>(input[0].desc.Width/2)-64;
  b.y=input[0].desc.Height/2-64;b.commandList=reinterpret_cast<std::uintptr_t>(list);
  for(unsigned i=0;i<3;++i){b.descriptions[i]=input[i].desc;b.states[i]=static_cast<D3D12_RESOURCE_STATES>(input[i].trackedState);
   b.pixelBytes[i]=GIBatchCapturePixelBytes(input[i].desc.Format);b.rowPitch[i]=(128*b.pixelBytes[i]+255)&~255u;
   b.sliceBytes[i]=UINT64(b.rowPitch[i])*128;b.offset[i]=end;
   end+=b.sliceBytes[i]*gi_batch_capture::Regions*input[i].desc.DepthOrArraySize;
  }
  if(end>gi_batch_capture::Capacity){capture.ledger.failed=true;c->policy.phase=P::Failed;Log("GI_HITS_R271_FAILED reason=capacity bytes=%llu",end);__leave;}
  for(unsigned i=0;i<3;++i){b.sources[i]=input[i].resource;b.sources[i]->AddRef();}
  bool recorded=true;
  for(unsigned slice=0;slice<input[0].desc.DepthOrArraySize&&recorded;++slice)for(unsigned roi=0;roi<2&&recorded;++roi){
   RRInputCaptureJob copy{};copy.count=3;copy.copyHasBox=true;
   for(unsigned i=0;i<3;++i){copy.sources[i]=b.sources[i];copy.readback[i]=capture.readback;copy.states[i]=b.states[i];copy.copySubresource[i]=slice;
    copy.copyBox[i]={b.x[roi],b.y,0,b.x[roi]+128,b.y+128,1};
    auto& f=copy.footprint[i];f.Offset=b.offset[i]+(UINT64(slice)*2+roi)*b.sliceBytes[i];
    f.Footprint.Format=b.descriptions[i].Format;f.Footprint.Width=f.Footprint.Height=128;f.Footprint.Depth=1;f.Footprint.RowPitch=b.rowPitch[i];
   }
   DWORD fault=0;bool started=false;
   recorded=RRInputCaptureRecordGuarded(&copy,&native,input,&fault,&started,3);
   capture.commandsStarted=capture.commandsStarted||started;
   if(!recorded)Log("GI_HITS_R271_FAILED reason=copy frame=%llu batch=%u slice=%u roi=%u fault=%lu resources_retained=1",frame,batch,slice,roi,fault);
  }
  if(!recorded){capture.ledger.failed=true;c->policy.phase=P::Failed;__leave;}
  capture.used=end;capture.ledger.Recorded();
  Log("GI_HITS_R271_RECORDED frame=%llu batch=%u batches=%u rays=%u slices=%u bytes=%llu gpu_complete=0",frame,batch,batches,rays,input[0].desc.DepthOrArraySize,end);
 }__except(EXCEPTION_EXECUTE_HANDLER){if(active){active->giHits.ledger.failed=true;active->policy.phase=control_rr::InputCapturePolicy::Failed;}Log("GI_HITS_R271_FAILED reason=exception resources_retained=1");}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}
