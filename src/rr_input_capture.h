#pragma once
#include "rr_input_capture_policy.h"
// C1 diagnostics: one outstanding job; only owned RR resources; no native input transitions.
struct RRInputCaptureJob {
 control_rr::InputCapturePolicy policy{};
 ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;ID3D12Fence* fence=nullptr;
 ID3D12Resource* sources[5]{},*readback[5]{};
 D3D12_RESOURCE_DESC descriptions[5]{};
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[5]{};UINT64 bytes[5]{};
 unsigned count=0,preset=0;UINT width=0,height=0;unsigned long long frame=0;
 float projection[16]{};bool projectionValid=false;
};
static SRWLOCK rrInputCaptureLock=SRWLOCK_INIT;
static RRInputCaptureJob rrInputCapture;
static void RRInputCaptureRelease(RRInputCaptureJob* c) noexcept {
 for(unsigned i=0;i<5;++i){RRGuideRelease(c->sources[i]);RRGuideRelease(c->readback[i]);}
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
 bool okay=true;void* mapped[5]{};
 try {
  wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
  if(!length||length>=32768)okay=false;
  std::wstring directory;
  if(okay){directory=std::wstring(env,length)+L"\\ControlFGProbe\\rr-input-c1-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(c->frame);
   okay=CreateDirectoryW(directory.c_str(),nullptr)!=FALSE;}
  const wchar_t* names[]{L"normal-roughness.rgba16f",L"specular.rgba16f",L"diffuse.rgba16f",L"distance.r32f",L"distance-status.r32u"};
  for(unsigned i=0;i<c->count&&okay;++i){
   D3D12_RANGE range{0,static_cast<SIZE_T>(c->bytes[i])};
   okay=SUCCEEDED(c->readback[i]->Map(0,&range,&mapped[i]))&&mapped[i];
   if(okay)okay=RRGuideExportDetail::WriteRaw(directory+L"\\"+names[i],static_cast<unsigned char*>(mapped[i])+c->footprint[i].Offset,c->footprint[i].Footprint.RowPitch,size_t(c->width)*(i<3?8:4),c->height);
  }
  if(okay){
   std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(9);
   out<<"{\"schema\":\"control-rr-input-c1\",\"pid\":"<<GetCurrentProcessId()<<",\"frame\":"<<c->frame<<",\"present\":"<<c->policy.present<<",\"preset\":"<<c->preset<<",\"width\":"<<c->width<<",\"height\":"<<c->height<<",\"buffers\":"<<c->count<<",\"hit_distance_bound\":"<<(c->count==5?"true":"false")<<",\"projection_valid\":"<<(c->projectionValid?"true":"false")<<",\"row_padding_removed\":true,\"little_endian\":true,\"view_to_clip\":[";
   for(unsigned i=0;i<16;++i){if(i)out<<',';out<<c->projection[i];}out<<"]}\n";
   const auto bytes=out.str();RRGuideExportDetail::File file(directory+L"\\metadata.pending");
   okay=file.Write(bytes.data(),bytes.size())&&file.Finish();
   if(okay)okay=MoveFileExW((directory+L"\\metadata.pending").c_str(),(directory+L"\\metadata.json").c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
  }
 }catch(...){okay=false;}
 for(unsigned i=0;i<c->count;++i)if(mapped[i]){D3D12_RANGE written{0,0};c->readback[i]->Unmap(0,&written);}
 Log("RR_INPUT_CAPTURE_C1_EXPORTED success=%u frame=%llu preset=%u buffers=%u",unsigned(okay),c->frame,c->preset,c->count);
 RRInputCaptureRelease(c);c->policy.Exported();
 ReleaseSRWLockExclusive(&rrInputCaptureLock);return 0;
}
#include "rr_input_capture_record.h"
static void RRInputCaptureBeforeEvaluation(ID3D12GraphicsCommandList* list,unsigned long long frame,unsigned preset,
 ID3D12Resource* normal,ID3D12Resource* specular,ID3D12Resource* diffuse,ID3D12Resource* distance,ID3D12Resource* status,const float* projection,bool projectionValid) noexcept {
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 __try {
  auto* c=&rrInputCapture;using Policy=control_rr::InputCapturePolicy;
  if(c->policy.phase!=Policy::Requested&&c->policy.phase!=Policy::Ready)__leave;
  // For F, wait for the real bound distance rather than capturing fallback data.
  if(preset==control_rr::RRPresetF&&(!distance||!status))__leave;
  ID3D12Resource* resources[]{normal,specular,diffuse,distance,status};const unsigned count=distance&&status?5u:3u;
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  if(!RRGuideReadRendererContext(&native,&reason)||native.commandList!=list||native.engineFrame!=frame)__leave;
  D3D12_RESOURCE_DESC descriptions[5]{};
  for(unsigned i=0;i<count;++i){if(!resources[i])__leave;descriptions[i]=resources[i]->GetDesc();}
  if(!resources[0]||!resources[1]||!resources[2])__leave;
  const auto width=descriptions[0].Width;const auto height=descriptions[0].Height;
  if(!width||!height||width>4096||height>2160){Log("RR_INPUT_CAPTURE_C1_REJECT reason=extent_limit");c->policy.phase=Policy::Failed;__leave;}
  bool valid=true;
  for(unsigned i=0;i<count;++i){const auto& d=descriptions[i];valid=valid&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.Format==(i<3?DXGI_FORMAT_R16G16B16A16_FLOAT:i==3?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R32_UINT);}
  if(!valid)__leave;
  if(c->policy.phase==Policy::Requested){
   if(FAILED(list->GetDevice(IID_PPV_ARGS(&c->device)))||!c->device){c->policy.phase=Policy::Failed;__leave;}
   c->queue=native.queue;c->queue->AddRef();c->count=count;c->width=static_cast<UINT>(width);c->height=height;c->preset=preset;
   for(unsigned i=0;i<count;++i)c->descriptions[i]=descriptions[i];
   c->policy.BeginPrepare();
   if(!QueueUserWorkItem(&RRInputCapturePrepare,c,WT_EXECUTEDEFAULT)){RRInputCaptureRelease(c);c->policy.phase=Policy::Failed;}
   __leave;
  }
  if(c->queue!=native.queue||c->count!=count||c->width!=width||c->height!=height||c->preset!=preset){
   RRInputCaptureRelease(c);c->policy.phase=Policy::Requested;__leave; // No commands yet; reprepare safely.
  }
  for(unsigned i=0;i<count;++i){c->sources[i]=resources[i];resources[i]->AddRef();}
  c->frame=frame;c->projectionValid=projectionValid;memcpy(c->projection,projection,sizeof(c->projection));
  c->policy.Record(native.presentToken);DWORD fault=0;
  if(!RRInputCaptureRecord(c,&native,&fault)){c->policy.phase=Policy::Failed;Log("RR_INPUT_CAPTURE_C1_FAILED reason=record fault=0x%08lX resources_retained=1",fault);__leave;}
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
   if(foregroundPid==GetCurrentProcessId()){const bool accepted=c->policy.Request();Log("RR_INPUT_CAPTURE_C1_REQUEST accepted=%u ordinal=%u",unsigned(accepted),c->policy.requests);}}
  if(c->policy.Retire(c->fence?c->fence->GetCompletedValue():0)){
   if(!QueueUserWorkItem(&RRInputCaptureExport,c,WT_EXECUTEDEFAULT))c->policy.phase=control_rr::InputCapturePolicy::Failed;
  }
  if(c->policy.CanSubmit(present)){const HRESULT hr=c->queue->Signal(c->fence,1);c->policy.Submit(SUCCEEDED(hr));
   if(FAILED(hr))Log("RR_INPUT_CAPTURE_C1_FAILED reason=signal resources_retained=1");}
 } __except(EXCEPTION_EXECUTE_HANDLER){rrInputCapture.policy.phase=control_rr::InputCapturePolicy::Failed;}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}
