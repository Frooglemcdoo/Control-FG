#pragma once
#include "rr_input_capture_policy.h"
#include "rr_input_capture_native.h"
// R5 diagnostics: one outstanding job; exact-state paired copies at evaluation.
struct RRInputCaptureJob {
 control_rr::InputCapturePolicy policy{};
 ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;ID3D12Fence* fence=nullptr;
 ID3D12Resource* sources[9]{},*readback[9]{};
 D3D12_RESOURCE_DESC descriptions[9]{};
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[9]{};UINT64 bytes[9]{};
 unsigned count=0,preset=0;bool distanceBound=false;unsigned rawIndex=9;
 unsigned nativeIndex=9;D3D12_RESOURCE_STATES states[9]{};
 unsigned pixelBytes[9]{};const wchar_t* names[9]{};
 float jitterX=0,jitterY=0,mvScaleX=0,mvScaleY=0;bool motionScaleValid=false;
 unsigned remaining=0;unsigned long long nextPresent=0;UINT width=0,height=0;unsigned long long frame=0;
 float projection[16]{};bool projectionValid=false;
};
static SRWLOCK rrInputCaptureLock=SRWLOCK_INIT;
static RRInputCaptureJob rrInputCapture;
static void RRInputCaptureRelease(RRInputCaptureJob* c) noexcept {
 for(unsigned i=0;i<9;++i){RRGuideRelease(c->sources[i]);RRGuideRelease(c->readback[i]);}
 RRGuideRelease(c->fence);RRGuideRelease(c->queue);RRGuideRelease(c->device);
}
static DWORD WINAPI RRInputCapturePrepare(void* context) noexcept {
 auto* c=static_cast<RRInputCaptureJob*>(context);
 AcquireSRWLockExclusive(&rrInputCaptureLock);
 HRESULT hr=c->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c->fence));
 for(unsigned i=0;i<c->count&&SUCCEEDED(hr);++i){
  c->device->GetCopyableFootprints(&c->descriptions[i],0,1,0,&c->footprint[i],nullptr,nullptr,&c->bytes[i]);
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=c->bytes[i];desc.Height=1;
  desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  hr=c->bytes[i]?c->device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&c->readback[i])):E_INVALIDARG;
 }
 c->policy.Prepared(SUCCEEDED(hr));
 if(FAILED(hr))RRInputCaptureRelease(c); // No commands recorded during allocation.
 Log("RR_INPUT_CAPTURE_C1_PREPARED success=%u width=%u height=%u buffers=%u",unsigned(SUCCEEDED(hr)),c->width,c->height,c->count);
 ReleaseSRWLockExclusive(&rrInputCaptureLock);return 0;
}
static DWORD WINAPI RRInputCaptureExport(void* context) noexcept {
 auto* c=static_cast<RRInputCaptureJob*>(context);
 AcquireSRWLockExclusive(&rrInputCaptureLock);
 bool okay=true;void* mapped[9]{};
 try {
  wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
  if(!length||length>=32768)okay=false;
  std::wstring directory;
  if(okay){directory=std::wstring(env,length)+L"\\ControlFGProbe\\rr-input-c1-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(c->frame);
   okay=CreateDirectoryW(directory.c_str(),nullptr)!=FALSE;}
  for(unsigned i=0;i<c->count&&okay;++i){
   D3D12_RANGE range{0,static_cast<SIZE_T>(c->bytes[i])};
   okay=SUCCEEDED(c->readback[i]->Map(0,&range,&mapped[i]))&&mapped[i];
   if(okay)okay=RRGuideExportDetail::WriteRaw(directory+L"\\"+c->names[i],static_cast<unsigned char*>(mapped[i])+c->footprint[i].Offset,c->footprint[i].Footprint.RowPitch,size_t(c->width)*c->pixelBytes[i],c->height);
  }
  if(okay){
   std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(9);
   out<<"{\"schema\":\"control-rr-input-c1\",\"pid\":"<<GetCurrentProcessId()<<",\"frame\":"<<c->frame<<",\"present\":"<<c->policy.present<<",\"preset\":"<<c->preset<<",\"width\":"<<c->width<<",\"height\":"<<c->height<<",\"buffers\":"<<c->count<<",\"hit_distance_bound\":"<<(c->distanceBound?"true":"false")<<",\"raw_albedo_bound\":"<<(c->rawIndex<c->count?"true":"false")<<",\"capture_ordinal\":"<<c->policy.requests<<",\"projection_valid\":"<<(c->projectionValid?"true":"false")<<",\"row_padding_removed\":true,\"little_endian\":true,\"view_to_clip\":[";
   for(unsigned i=0;i<16;++i){if(i)out<<',';out<<c->projection[i];}out<<"],\"jitter\":["<<c->jitterX<<','<<c->jitterY<<"],\"motion_scale\":["<<c->mvScaleX<<','<<c->mvScaleY<<"],\"motion_scale_valid\":"<<(c->motionScaleValid?"true":"false")<<",\"native_inputs_bound\":true,\"resources\":[";
   for(unsigned i=0;i<c->count;++i){if(i)out<<',';std::string name;for(const wchar_t* p=c->names[i];*p;++p)name+=static_cast<char>(*p);
    out<<"{\"file\":\""<<name<<"\",\"dxgi_format\":"<<unsigned(c->descriptions[i].Format)<<",\"pixel_bytes\":"<<c->pixelBytes[i]<<",\"state_before_and_after\":"<<unsigned(c->states[i])<<'}';}
   out<<"]}\n";
   const auto bytes=out.str();RRGuideExportDetail::File file(directory+L"\\metadata.pending");
   okay=file.Write(bytes.data(),bytes.size())&&file.Finish();
   if(okay)okay=MoveFileExW((directory+L"\\metadata.pending").c_str(),(directory+L"\\metadata.json").c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
  }
 }catch(...){okay=false;}
 for(unsigned i=0;i<c->count;++i)if(mapped[i]){D3D12_RANGE written{0,0};c->readback[i]->Unmap(0,&written);}
 Log("RR_INPUT_CAPTURE_C1_EXPORTED success=%u frame=%llu preset=%u buffers=%u",unsigned(okay),c->frame,c->preset,c->count);
 RRInputCaptureRelease(c);c->policy.Exported();
 if(!okay)c->remaining=0;
 ReleaseSRWLockExclusive(&rrInputCaptureLock);return 0;
}
#include "rr_input_capture_record.h"
#include "rr_input_capture_guard.h"
static void RRInputCaptureBeforeEvaluation(ID3D12GraphicsCommandList* list,unsigned long long frame,unsigned preset,
 ID3D12Resource* normal,ID3D12Resource* specular,ID3D12Resource* diffuse,ID3D12Resource* distance,ID3D12Resource* status,const float* projection,bool projectionValid,ID3D12Resource* rawAlbedo,ID3D12Resource* color,ID3D12Resource* depth,ID3D12Resource* motion,void* parameters) noexcept {
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 __try {
  auto* c=&rrInputCapture;using Policy=control_rr::InputCapturePolicy;
  if(c->policy.phase!=Policy::Requested&&c->policy.phase!=Policy::Ready)__leave;
  // Capture the guides actually bound to RR. Hit distance is optional for F
  // too; waiting for it prevents capture in the production no-distance path.
  ID3D12Resource* resources[9]{normal,specular,diffuse,distance,status,nullptr};const bool distanceBound=distance&&status;const unsigned baseCount=distanceBound?5u:3u;
  const unsigned rawIndex=rawAlbedo?baseCount:9u;const unsigned nativeIndex=baseCount+(rawAlbedo?1u:0u);const unsigned count=nativeIndex+3u;
  if(rawAlbedo)resources[rawIndex]=rawAlbedo;
  resources[nativeIndex]=color;resources[nativeIndex+1]=depth;resources[nativeIndex+2]=motion;
  RRGuideInputTexture nativeInputs[3]{};
  if(!RRInputCaptureFindNative(frame,resources+nativeIndex,nativeInputs)){
   static unsigned rejects=0;if(++rejects<=4||(rejects%240)==0)Log("RR_INPUT_CAPTURE_C1_REJECT reason=native_input_state_unproven frame=%llu",frame);__leave;
  }
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  if(!RRGuideReadRendererContext(&native,&reason)||native.commandList!=list||native.engineFrame!=frame)__leave;
  D3D12_RESOURCE_DESC descriptions[9]{};
  for(unsigned i=0;i<count;++i){if(!resources[i])__leave;descriptions[i]=resources[i]->GetDesc();}
  if(!resources[0]||!resources[1]||!resources[2])__leave;
  const auto width=descriptions[0].Width;const auto height=descriptions[0].Height;
  if(!width||!height||width>4096||height>2160){Log("RR_INPUT_CAPTURE_C1_REJECT reason=extent_limit");c->policy.phase=Policy::Failed;__leave;}
  bool valid=true;
  for(unsigned i=0;i<nativeIndex;++i){const auto& d=descriptions[i];valid=valid&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.Format==((i<3||i==rawIndex)?DXGI_FORMAT_R16G16B16A16_FLOAT:i==3?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R32_UINT);}
  for(unsigned i=nativeIndex;i<count;++i){const auto& d=descriptions[i];valid=valid&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.SampleDesc.Quality==0;
   const auto f=d.Format;valid=valid&&(i==nativeIndex?(f==DXGI_FORMAT_R11G11B10_FLOAT||f==DXGI_FORMAT_R16G16B16A16_FLOAT):i==nativeIndex+1?(f==DXGI_FORMAT_R32_TYPELESS||f==DXGI_FORMAT_R32_FLOAT):(f==DXGI_FORMAT_R16G16_FLOAT||f==DXGI_FORMAT_R32G32_FLOAT));
  }
  for(unsigned i=0;i<count;++i)for(unsigned j=0;j<i;++j)if(resources[i]==resources[j])valid=false;
  if(!valid){Log("RR_INPUT_CAPTURE_C1_REJECT reason=native_input_format_or_extent frame=%llu",frame);c->policy.phase=Policy::Failed;__leave;}
  if(c->policy.phase==Policy::Requested){
   if(FAILED(list->GetDevice(IID_PPV_ARGS(&c->device)))||!c->device){c->policy.phase=Policy::Failed;__leave;}
   c->queue=native.queue;c->queue->AddRef();c->count=count;c->distanceBound=distanceBound;c->rawIndex=rawIndex;c->nativeIndex=nativeIndex;c->width=static_cast<UINT>(width);c->height=height;c->preset=preset;
   const wchar_t* guideNames[]{L"normal-roughness.rgba16f",L"specular.rgba16f",L"diffuse.rgba16f",L"distance.r32f",L"distance-status.r32u"};
   for(unsigned i=0;i<count;++i){c->descriptions[i]=descriptions[i];c->pixelBytes[i]=(i<3||i==rawIndex)?8u:4u;
    c->names[i]=i==rawIndex?L"raw-albedo.rgba16f":i<nativeIndex?guideNames[i]:L"";
   }
   c->names[nativeIndex]=descriptions[nativeIndex].Format==DXGI_FORMAT_R11G11B10_FLOAT?L"input-color.r11g11b10f":L"input-color.rgba16f";
   c->pixelBytes[nativeIndex]=descriptions[nativeIndex].Format==DXGI_FORMAT_R11G11B10_FLOAT?4u:8u;
   c->names[nativeIndex+1]=L"input-depth.r32f";
   c->names[nativeIndex+2]=descriptions[nativeIndex+2].Format==DXGI_FORMAT_R16G16_FLOAT?L"input-motion.rg16f":L"input-motion.rg32f";
   c->pixelBytes[nativeIndex+2]=descriptions[nativeIndex+2].Format==DXGI_FORMAT_R16G16_FLOAT?4u:8u;
   c->policy.BeginPrepare();
   if(!QueueUserWorkItem(&RRInputCapturePrepare,c,WT_EXECUTEDEFAULT)){RRInputCaptureRelease(c);c->policy.phase=Policy::Failed;}
   __leave;
  }
  if(c->queue!=native.queue||c->count!=count||c->rawIndex!=rawIndex||c->distanceBound!=distanceBound||c->width!=width||c->height!=height||c->preset!=preset){
   RRInputCaptureRelease(c);c->policy.phase=Policy::Requested;__leave; // No commands yet; reprepare safely.
  }
  bool formatsMatch=true;
  for(unsigned i=0;i<count;++i)if(c->descriptions[i].Format!=descriptions[i].Format)formatsMatch=false;
  if(!formatsMatch){RRInputCaptureRelease(c);c->policy.phase=Policy::Requested;__leave;}
  for(unsigned i=0;i<count;++i){ID3D12Device* device=nullptr;const HRESULT hr=resources[i]->GetDevice(IID_PPV_ARGS(&device));const bool same=SUCCEEDED(hr)&&device==c->device;if(device)device->Release();if(!same)__leave;}
  for(unsigned i=0;i<count;++i){c->sources[i]=resources[i];resources[i]->AddRef();c->states[i]=i<nativeIndex?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:static_cast<D3D12_RESOURCE_STATES>(nativeInputs[i-nativeIndex].trackedState);}
  c->jitterX=c->jitterY=c->mvScaleX=c->mvScaleY=0;
  if(ngxGetFloat){ngxGetFloat(parameters,"Jitter.Offset.X",&c->jitterX);ngxGetFloat(parameters,"Jitter.Offset.Y",&c->jitterY);}
  c->motionScaleValid=ngxGetFloat&&ngxGetFloat(parameters,"MV.Scale.X",&c->mvScaleX)==1&&ngxGetFloat(parameters,"MV.Scale.Y",&c->mvScaleY)==1;
  c->frame=frame;c->projectionValid=projectionValid;memcpy(c->projection,projection,sizeof(c->projection));
  c->nextPresent=native.presentToken+30;
  DWORD fault=0;bool commandsStarted=false;
  if(!RRInputCaptureRecordGuarded(c,&native,nativeInputs,&fault,&commandsStarted)){
   if(commandsStarted){c->policy.phase=Policy::Failed;Log("RR_INPUT_CAPTURE_C1_FAILED reason=record fault=0x%08lX resources_retained=1",fault);}
   else {for(unsigned i=0;i<count;++i)RRGuideRelease(c->sources[i]);}
   __leave;
  }
  c->policy.Record(native.presentToken);
  Log("RR_INPUT_CAPTURE_C1_RECORDED frame=%llu preset=%u buffers=%u",frame,preset,count);
 } __except(EXCEPTION_EXECUTE_HANDLER){rrInputCapture.policy.phase=control_rr::InputCapturePolicy::Failed;Log("RR_INPUT_CAPTURE_C1_FAILED reason=exception resources_retained=1");}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}
static void RRInputCaptureAfterPresent(unsigned long long present) noexcept {
 static bool held=false;const bool down=(GetAsyncKeyState(VK_F9)&0x8000)!=0;const bool pressed=down&&!held;held=down;
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 __try {
  auto* c=&rrInputCapture;
  if(pressed){DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
   if(foregroundPid==GetCurrentProcessId()){const bool accepted=c->policy.Request();if(accepted){c->remaining=4-c->policy.requests;rrHairShaderAuditRequested.store(true,std::memory_order_release);}Log("RR_INPUT_CAPTURE_C1_REQUEST accepted=%u ordinal=%u",unsigned(accepted),c->policy.requests);}}
  if(c->remaining&&c->policy.phase==control_rr::InputCapturePolicy::Idle&&present>=c->nextPresent){
   if(c->policy.Request())--c->remaining;else c->remaining=0;
  }
  if(c->policy.Retire(c->fence?c->fence->GetCompletedValue():0)){
   if(!QueueUserWorkItem(&RRInputCaptureExport,c,WT_EXECUTEDEFAULT))c->policy.phase=control_rr::InputCapturePolicy::Failed;
  }
  if(c->policy.CanSubmit(present)){const HRESULT hr=c->queue->Signal(c->fence,1);c->policy.Submit(SUCCEEDED(hr));
   if(FAILED(hr))Log("RR_INPUT_CAPTURE_C1_FAILED reason=signal resources_retained=1");}
 } __except(EXCEPTION_EXECUTE_HANDLER){rrInputCapture.policy.phase=control_rr::InputCapturePolicy::Failed;}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}
