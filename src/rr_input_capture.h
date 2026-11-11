#pragma once
#include <cmath>
#include "rr_input_capture_policy.h"
#include "rr_input_capture_limits.h"
#include "rr_input_capture_settle.h"
#include "rr_capture_ui.h"
#include "rr_input_capture_native.h"
#include "rr_input_capture_gi.h"
#include "gi_batch_capture.h"
// R10 diagnostics: two prepared slots capture successive evaluated frames.
// Inputs and post-RR output are copied on the original list, with exact restoration.
struct RRInputCaptureJob {
 unsigned exportStride=1,pathCondition=0;wchar_t exportRoot[260]{};
 control_rr::InputCapturePolicy policy{};
 GIBatchCapture giHits{};
 unsigned copySubresource[kRRInputCaptureMaxResources]{};
 D3D12_BOX copyBox[kRRInputCaptureMaxResources]{};
 bool copyHasBox=false;
 ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;ID3D12Fence* fence=nullptr;
 ID3D12Resource* sources[kRRInputCaptureMaxResources]{},*readback[kRRInputCaptureMaxResources]{};
 D3D12_RESOURCE_DESC descriptions[kRRInputCaptureMaxResources]{};
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[kRRInputCaptureMaxResources]{};UINT64 bytes[kRRInputCaptureMaxResources]{};
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
 control_rr_gi::Boundary giBoundary{};
#endif
 const char* lightingMode="unavailable";
 unsigned suiteSession=0,suiteStep=0,suiteSamples=0;bool suiteRaw=false;
#ifdef CONTROL_RR_GI_SEQUENCE
 unsigned preGIIndex=kRRInputCaptureMaxResources;
 unsigned long long preGIFrame=0,preGIPresent=0,preGITick=0,evaluationTick=0;
 bool preGICaptured=false;
#endif
 unsigned count=0,preset=0;bool nativeLighting=false;bool distanceBound=false;unsigned rawIndex=kRRInputCaptureMaxResources;
 unsigned nativeIndex=kRRInputCaptureMaxResources;D3D12_RESOURCE_STATES states[kRRInputCaptureMaxResources]{};
 unsigned pixelBytes[kRRInputCaptureMaxResources]{};const wchar_t* names[kRRInputCaptureMaxResources]{};
 float jitterX=0,jitterY=0,mvScaleX=0,mvScaleY=0;bool motionScaleValid=false;
 unsigned pairIndex=0,outputIndex=kRRInputCaptureMaxResources;bool outputCaptured=false;unsigned long long previousFrame=0;
 float preExposure=0,exposureScale=0;bool preExposureValid=false,exposureScaleValid=false;
 int reset=0;unsigned evaluationResult=0;UINT width=0,height=0;unsigned long long frame=0;
 float projection[16]{};bool projectionValid=false;
};
static SRWLOCK rrInputCaptureLock=SRWLOCK_INIT;
// Serialize large disk exports without blocking render-side capture progress.
static SRWLOCK rrInputCaptureExportLock=SRWLOCK_INIT;
static RRInputCaptureJob rrInputCaptures[2];
static unsigned rrInputCaptureRecording=2;
static control_rr::InputCaptureSettle rrInputCaptureSettle;
#ifdef CONTROL_RR_GI_SEQUENCE
static unsigned long long rrInputCaptureSettledFrame=0,rrInputCaptureRequestTick=0;
#endif
static void RRInputCaptureRelease(RRInputCaptureJob* c) noexcept {
 for(unsigned i=0;i<kRRInputCaptureMaxResources;++i){RRGuideRelease(c->sources[i]);RRGuideRelease(c->readback[i]);}
 GIBatchCaptureRelease(c->giHits);
 RRGuideRelease(c->fence);RRGuideRelease(c->queue);RRGuideRelease(c->device);
}
static DWORD WINAPI RRInputCapturePrepare(void* context) noexcept {
 auto* c=static_cast<RRInputCaptureJob*>(context);
 AcquireSRWLockExclusive(&rrInputCaptureLock);
 if(c->policy.phase!=control_rr::InputCapturePolicy::Preparing){ReleaseSRWLockExclusive(&rrInputCaptureLock);return 0;}
 HRESULT hr=c->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c->fence));
 for(unsigned i=0;i<c->count&&SUCCEEDED(hr);++i){
  c->device->GetCopyableFootprints(&c->descriptions[i],0,1,0,&c->footprint[i],nullptr,nullptr,&c->bytes[i]);
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=c->bytes[i];desc.Height=1;
  desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  hr=c->bytes[i]?c->device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&c->readback[i])):E_INVALIDARG;
 }
 if(SUCCEEDED(hr)&&!c->pathCondition)hr=GIBatchCapturePrepare(c->giHits,c->device);
 c->policy.Prepared(SUCCEEDED(hr));
 if(FAILED(hr))RRInputCaptureRelease(c); // No commands recorded during allocation.
 Log("RR_INPUT_CAPTURE_C1_PREPARED success=%u width=%u height=%u buffers=%u",unsigned(SUCCEEDED(hr)),c->width,c->height,c->count);
 ReleaseSRWLockExclusive(&rrInputCaptureLock);return 0;
}
static DWORD WINAPI RRInputCaptureExport(void* context) noexcept {
 auto* c=static_cast<RRInputCaptureJob*>(context);
 AcquireSRWLockExclusive(&rrInputCaptureExportLock);
 AcquireSRWLockExclusive(&rrInputCaptureLock);
 if(c->policy.phase!=control_rr::InputCapturePolicy::Exporting){ReleaseSRWLockExclusive(&rrInputCaptureLock);ReleaseSRWLockExclusive(&rrInputCaptureExportLock);return 0;}
 // Exporting reserves this slot. Rendering cannot record/reprepare it, and
 // timeout only changes policy, never ownership. Keep disk I/O outside the
 // shared lock; reacquire before releasing resources or publishing completion.
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
 const auto exportStarted=GetTickCount64();
 Log("RR_INPUT_CAPTURE_EXPORT_BEGIN frame=%llu buffers=%u",c->frame,c->count);
 bool okay=true;void* mapped[kRRInputCaptureMaxResources]{};
 try {
  wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
  if(!length||length>=32768)okay=false;
  std::wstring directory;
  if(okay){directory=(c->pathCondition?std::wstring(c->exportRoot):std::wstring(env,length)+L"\\ControlFGProbe")+L"\\rr-input-c1-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(c->frame);
   okay=CreateDirectoryW(directory.c_str(),nullptr)!=FALSE;}
  for(unsigned i=0;i<c->count&&okay;++i){
   if(i==c->outputIndex&&!c->outputCaptured)continue;
   D3D12_RANGE range{0,static_cast<SIZE_T>(c->bytes[i])};
   const auto mapStarted=GetTickCount64();
   Log("RR_INPUT_CAPTURE_EXPORT_MAP_BEGIN frame=%llu index=%u bytes=%llu",c->frame,i,c->bytes[i]);
   okay=SUCCEEDED(c->readback[i]->Map(0,&range,&mapped[i]))&&mapped[i];
   const auto writeStarted=GetTickCount64();
   Log("RR_INPUT_CAPTURE_EXPORT_MAP_END frame=%llu index=%u success=%u elapsed_ms=%llu",c->frame,i,unsigned(okay),writeStarted-mapStarted);
   if(okay){
    const auto* pixels=static_cast<unsigned char*>(mapped[i])+c->footprint[i].Offset;
    if(c->exportStride==1)okay=RRGuideExportDetail::WriteRaw(directory+L"\\"+c->names[i],pixels,c->footprint[i].Footprint.RowPitch,size_t(c->descriptions[i].Width)*c->pixelBytes[i],c->descriptions[i].Height);
    else {
     // Full-screen point samples, never a manually aimed crop. Native capture
     // remains full resolution; only worker-side disk payload is reduced.
     RRGuideExportDetail::File file(directory+L"\\"+c->names[i]);
     const unsigned w=static_cast<unsigned>(c->descriptions[i].Width),h=c->descriptions[i].Height,b=c->pixelBytes[i],s=c->exportStride;
     std::vector<unsigned char> row(size_t(gi_path_capture::Extent(w,s))*b);
     for(unsigned y=0;y<h&&okay;y+=s){
      for(unsigned x=0;x<w;x+=s)memcpy(row.data()+size_t(x/s)*b,pixels+size_t(y)*c->footprint[i].Footprint.RowPitch+size_t(x)*b,b);
      okay=file.Write(row.data(),row.size());
     }
     okay=okay&&file.Finish();
    }
   }
   if(mapped[i]){D3D12_RANGE written{0,0};c->readback[i]->Unmap(0,&written);mapped[i]=nullptr;}
   Log("RR_INPUT_CAPTURE_EXPORT_WRITE_END frame=%llu index=%u success=%u elapsed_ms=%llu",c->frame,i,unsigned(okay),GetTickCount64()-writeStarted);
  }
  unsigned long long giCompared=0,giDifferent=0;
#ifdef CONTROL_RR_GI_SEQUENCE
  if(okay&&c->pathCondition&&c->preGICaptured){
   const unsigned a=c->preGIIndex,b=c->nativeIndex+5;
   if(c->descriptions[a].Width==c->descriptions[b].Width&&c->descriptions[a].Height==c->descriptions[b].Height){
    D3D12_RANGE ra{0,static_cast<SIZE_T>(c->bytes[a])},rb{0,static_cast<SIZE_T>(c->bytes[b])};
    okay=SUCCEEDED(c->readback[a]->Map(0,&ra,&mapped[a]))&&mapped[a]&&SUCCEEDED(c->readback[b]->Map(0,&rb,&mapped[b]))&&mapped[b];
    if(okay)for(unsigned y=0;y<c->descriptions[a].Height;++y){
     auto* pa=static_cast<unsigned char*>(mapped[a])+c->footprint[a].Offset+size_t(y)*c->footprint[a].Footprint.RowPitch;
     auto* pb=static_cast<unsigned char*>(mapped[b])+c->footprint[b].Offset+size_t(y)*c->footprint[b].Footprint.RowPitch;
     for(size_t x=0;x<c->descriptions[a].Width;++x){++giCompared;if(memcmp(pa+x*8,pb+x*8,6)!=0)++giDifferent;}
    }
    for(unsigned i:{a,b})if(mapped[i]){D3D12_RANGE wr{0,0};c->readback[i]->Unmap(0,&wr);mapped[i]=nullptr;}
   }
  }
#endif
#if defined(CONTROL_RR_GI_SEQUENCE) && defined(CONTROL_GI_BOUNDARY_CAPTURE)
  if(c->pathCondition){
   const auto& b=c->giBoundary;
   const bool verified=okay&&c->preGICaptured&&c->preGIFrame==c->frame&&c->outputCaptured&&c->evaluationResult==1&&
    gi_path_capture::BoundaryMatches(c->pathCondition,b.valid,b.applied,b.frame,c->frame,b.stage,b.spatialPasses,b.spatialObserved)&&
    giCompared&&((c->pathCondition!=1&&c->pathCondition!=3)||giDifferent==0)&&
    gi_path_capture::PairAligned(c->pathCondition,c->pairIndex,c->previousFrame,c->frame);
   (verified?gi_path_capture::validFrames[c->pathCondition]:gi_path_capture::invalidFrames[c->pathCondition]).fetch_add(1);
  }
#endif
  if(okay)okay=GIBatchCaptureExport(c->giHits,directory);
  if(okay){
   std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(9);
   out<<"{\"full_resolution_gi_rgb_pixels_compared\":"<<giCompared<<",\"full_resolution_gi_rgb_pixels_different\":"<<giDifferent<<",";
   out<<"\"path_capture_condition\":"<<c->pathCondition<<",\"export_stride\":"<<c->exportStride<<",\"point_samples_not_filtered\":true,";
   out<<"\"gi_hit_capture_requested\":"<<(c->giHits.enabled?"true":"false")<<",";
#ifdef CONTROL_RR_GI_SEQUENCE
#ifdef CONTROL_GI_LONG_SEQUENCE
   out<<"\"gi_sampling_experiment\":\"R264_current_frame_256_creation\",";
#endif
   out<<"\"gi_sequence_build\":\"R263\",\"sequence_pairs\":"<<(c->pathCondition==3?1:c->pathCondition?2:4)<<",\"gi_signal_boundary\":\"after_filter_at_rr_input\","
      <<"\"pre_gi_frame\":"<<c->preGIFrame<<",\"pre_gi_captured\":"<<(c->preGICaptured?"true":"false")
      <<",\"pre_gi_tick_ms\":"<<c->preGITick<<",\"evaluation_tick_ms\":"<<c->evaluationTick<<",";
#endif
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
   const auto& b=c->giBoundary;
   out<<"\"gi_boundary\":{\"filter_build\":\""
#ifdef CONTROL_RR_GI_SEQUENCE
      <<"R263"
#else
      <<"R217"
#endif
      <<"\",\"valid\":"<<(b.valid?"true":"false")<<",\"frame\":"<<b.frame
      <<",\"stage\":\""<<control_rr_gi::StageName(b.stage)<<"\",\"view\":\""<<control_rr_gi::ViewName(b.view)
      <<"\",\"cleanup\":"<<(b.cleanup?"true":"false")<<",\"applied\":"<<(b.applied?"true":"false")
      <<",\"spatial_observed\":"<<b.spatialObserved<<",\"spatial_passes\":"<<b.spatialPasses<<",\"filter_color_identity\":"<<b.color<<"},";
#endif
   out<<"\"suite_session\":"<<c->suiteSession<<",\"suite_step\":"<<c->suiteStep<<",\"suite_samples\":"<<c->suiteSamples<<",\"suite_raw_rr_clamp\":"<<(c->suiteRaw?"true":"false")<<",\"lighting_mode\":\""<<c->lightingMode<<"\",\"schema\":\"control-rr-gi-r28\",\"evaluation_path\":\""<<(c->nativeLighting?"native_sr":"rr")<<"\",\"rr_active\":"<<(c->nativeLighting?"false":"true")<<",\"pre_exposure_valid\":"<<(c->preExposureValid?"true":"false")<<",\"pre_exposure\":"<<c->preExposure<<",\"exposure_scale_valid\":"<<(c->exposureScaleValid?"true":"false")<<",\"exposure_scale\":"<<c->exposureScale<<",\"pid\":"<<GetCurrentProcessId()<<",\"frame\":"<<c->frame<<",\"present\":"<<c->policy.present<<",\"preset\":"<<c->preset<<",\"width\":"<<c->width<<",\"height\":"<<c->height<<",\"buffers\":"<<c->count<<",\"hit_distance_bound\":"<<(c->distanceBound?"true":"false")<<",\"raw_albedo_bound\":"<<(c->rawIndex<c->count?"true":"false")<<",\"capture_ordinal\":"<<c->policy.requests<<",\"projection_valid\":"<<(c->projectionValid?"true":"false")<<",\"row_padding_removed\":true,\"little_endian\":true,\"view_to_clip\":[";
   for(unsigned i=0;i<16;++i){if(i)out<<',';out<<c->projection[i];}out<<"],\"jitter\":["<<c->jitterX<<','<<c->jitterY<<"],\"motion_scale\":["<<c->mvScaleX<<','<<c->mvScaleY<<"],\"motion_scale_valid\":"<<(c->motionScaleValid?"true":"false")<<",\"native_inputs_bound\":true,\"lighting_buffers_captured\":true,\"resources\":[";
   for(unsigned i=0;i<c->count;++i){if(i)out<<',';std::string name;for(const wchar_t* p=c->names[i];*p;++p)name+=static_cast<char>(*p);
    out<<"{";
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
    out<<"\"source_width\":"<<c->descriptions[i].Width<<",\"source_height\":"<<c->descriptions[i].Height<<",";
    out<<"\"resource_identity\":"<<reinterpret_cast<std::uintptr_t>(c->sources[i])<<",";
#endif
    out<<"\"file\":\""<<name<<"\",\"dxgi_format\":"<<unsigned(c->descriptions[i].Format)<<",\"pixel_bytes\":"<<c->pixelBytes[i]<<",\"state_before_and_after\":"<<unsigned(c->states[i])<<",\"width\":"<<gi_path_capture::Extent(static_cast<unsigned>(c->descriptions[i].Width),c->exportStride)<<",\"height\":"<<gi_path_capture::Extent(c->descriptions[i].Height,c->exportStride)<<",\"copied\":"<<((i!=c->outputIndex||c->outputCaptured)?"true":"false")<<'}';}
   out<<"],\"pair_index\":"<<c->pairIndex<<",\"previous_frame\":"<<c->previousFrame<<",\"consecutive\":"<<((c->pairIndex==1&&c->frame==c->previousFrame+1)?"true":"false")<<",\"reset\":"<<c->reset<<",\"rr_result\":"<<c->evaluationResult<<",\"output_captured\":"<<(c->outputCaptured?"true":"false")<<"}\n";
   const auto bytes=out.str();RRGuideExportDetail::File file(directory+L"\\metadata.pending");
   okay=file.Write(bytes.data(),bytes.size())&&file.Finish();
   if(okay)okay=MoveFileExW((directory+L"\\metadata.pending").c_str(),(directory+L"\\metadata.json").c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
  }
 }catch(...){okay=false;}
 for(unsigned i=0;i<c->count;++i)if(mapped[i]){D3D12_RANGE written{0,0};c->readback[i]->Unmap(0,&written);}
 Log("RR_INPUT_CAPTURE_C1_EXPORTED success=%u frame=%llu preset=%u buffers=%u",unsigned(okay),c->frame,c->preset,c->count);
 Log("RR_INPUT_CAPTURE_EXPORT_END frame=%llu elapsed_ms=%llu",c->frame,GetTickCount64()-exportStarted);
 AcquireSRWLockExclusive(&rrInputCaptureLock);
 RRInputCaptureRelease(c);
 if(okay)c->policy.Exported();else {c->policy.phase=control_rr::InputCapturePolicy::Failed;control_rr_capture_ui::detail.store(L"Image export failed. PACKAGE LIGHT will collect diagnostics.");}

 ReleaseSRWLockExclusive(&rrInputCaptureLock);
 ReleaseSRWLockExclusive(&rrInputCaptureExportLock);return 0;
}
#include "rr_input_capture_record.h"
#include "rr_input_capture_guard.h"
#ifdef CONTROL_RR_GI_SEQUENCE
#include "gi_batch_capture_record.h"
#include "rr_input_capture_early_gi.h"
#endif
static void RRInputCaptureOneBeforeEvaluation(RRInputCaptureJob* c,ID3D12GraphicsCommandList* list,unsigned long long frame,unsigned preset,
 ID3D12Resource* normal,ID3D12Resource* specular,ID3D12Resource* diffuse,ID3D12Resource* distance,ID3D12Resource* status,const float* projection,bool projectionValid,ID3D12Resource* rawAlbedo,ID3D12Resource* color,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* output,ID3D12Resource* responsivity,void* parameters,bool nativeLighting=false,const char* lightingMode="rr") noexcept {
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 __try {
  using Policy=control_rr::InputCapturePolicy;
  if(c->policy.phase!=Policy::Requested&&c->policy.phase!=Policy::Ready)__leave;
  // Capture the guides actually bound to RR. Hit distance is optional for F
  // too; waiting for it prevents capture in the production no-distance path.
  ID3D12Resource* resources[kRRInputCaptureMaxResources]{normal,specular,diffuse,distance,status,nullptr};const bool distanceBound=!nativeLighting&&distance&&status;const unsigned baseCount=nativeLighting?0u:(distanceBound?5u:3u);
  const unsigned rawIndex=rawAlbedo?baseCount:kRRInputCaptureMaxResources;const unsigned nativeIndex=baseCount+(rawAlbedo?1u:0u);const unsigned maskIndex=nativeIndex+kRRInputCaptureNativeCount;const unsigned outputIndex=maskIndex+(responsivity?1u:0u);const unsigned count=outputIndex+1u
#ifdef CONTROL_RR_GI_SEQUENCE
   +1u
#endif
  ;
  if(rawAlbedo)resources[rawIndex]=rawAlbedo;
  resources[nativeIndex]=color;resources[nativeIndex+1]=depth;resources[nativeIndex+2]=motion;
  RRGuideInputTexture nativeInputs[kRRInputCaptureNativeCount]{};
  if(!RRInputCaptureFindNative(frame,resources+nativeIndex,nativeInputs)){
   control_rr_capture_ui::detail.store(L"Waiting: native scene input state is not verified.");
   static unsigned rejects=0;if(++rejects<=4||(rejects%240)==0)Log("RR_INPUT_CAPTURE_C1_REJECT reason=native_input_state_unproven frame=%llu",frame);__leave;
  }
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  if(!RRGuideReadRendererContext(&native,&reason)||native.commandList!=list||native.engineFrame!=frame)__leave;
  RRGuideInputSnapshot gb{};
  if(!RRGuideReadInputs(&gb,&reason)||gb.engineFrame!=frame||gb.commandList!=list){
   static unsigned rejects=0;if(++rejects<=4||(rejects%240)==0)Log("RR_INPUT_CAPTURE_C1_REJECT reason=gbuffer_snapshot detail=%s frame=%llu",reason?reason:"context_mismatch",frame);__leave;
  }
  resources[nativeIndex+3]=gb.gbuffer1.resource;resources[nativeIndex+4]=gb.gbuffer2.resource;
  nativeInputs[3]=gb.gbuffer1;nativeInputs[4]=gb.gbuffer2;
  if(!RRInputCaptureReadGI(&nativeInputs[5])){
   control_rr_capture_ui::detail.store(L"Waiting: raw GI read state is not verified.");
   static unsigned rejects=0;if(++rejects<=4||(rejects%240)==0)Log("RR_INPUT_CAPTURE_R28_REJECT reason=raw_gi_read_state_unproven frame=%llu",frame);__leave;
  }
  resources[nativeIndex+5]=nativeInputs[5].resource;
  // Read the accumulated light buffers at the same pre-RR boundary as color.
  // These are not albedo guides and are not isolated direct-light contributions.
  if(!RRInputCaptureReadLighting(&nativeInputs[6],&nativeInputs[7])){
   control_rr_capture_ui::detail.store(L"Lighting buffers unavailable in a verified read state. Click PACKAGE LIGHT.");
   Log("RR_INPUT_CAPTURE_C1_REJECT reason=lighting_read_state_unproven frame=%llu",frame);
   c->policy.phase=Policy::Failed;__leave;
  }
  resources[nativeIndex+6]=nativeInputs[6].resource;
  resources[nativeIndex+7]=nativeInputs[7].resource;
  if(responsivity)resources[maskIndex]=responsivity;
  resources[outputIndex]=output;
#ifdef CONTROL_RR_GI_SEQUENCE
  // Allocation shape only; this slot is copied earlier at GI dispatch, never here.
  resources[outputIndex+1]=resources[nativeIndex+5];
#endif
  D3D12_RESOURCE_DESC descriptions[kRRInputCaptureMaxResources]{};
  for(unsigned i=0;i<count;++i){if(!resources[i])__leave;descriptions[i]=resources[i]->GetDesc();}
  if(!resources[0]||!resources[1]||!resources[2])__leave;
  const auto width=descriptions[0].Width;const auto height=descriptions[0].Height;
  if(!width||!height||width>4096||height>2160){Log("RR_INPUT_CAPTURE_C1_REJECT reason=extent_limit");c->policy.phase=Policy::Failed;__leave;}
  bool valid=true;
  for(unsigned i=0;i<nativeIndex;++i){const auto& d=descriptions[i];valid=valid&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.Format==((i<3||i==rawIndex)?DXGI_FORMAT_R16G16B16A16_FLOAT:i==3?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R32_UINT);}
  for(unsigned i=nativeIndex;i<nativeIndex+3;++i){const auto& d=descriptions[i];valid=valid&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.SampleDesc.Quality==0;
   const auto f=d.Format;valid=valid&&(i==nativeIndex?(f==DXGI_FORMAT_R11G11B10_FLOAT||f==DXGI_FORMAT_R16G16B16A16_FLOAT):i==nativeIndex+1?(f==DXGI_FORMAT_R32_TYPELESS||f==DXGI_FORMAT_R32_FLOAT):(f==DXGI_FORMAT_R16G16_FLOAT||f==DXGI_FORMAT_R32G32_FLOAT));
  }
  for(unsigned i=nativeIndex+3;i<outputIndex;++i){if(i>=nativeIndex+5&&i<maskIndex)continue;const auto& d=descriptions[i];
   valid=valid&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.Format==(i<maskIndex?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R16_FLOAT);
  }
  const auto& gd=descriptions[nativeIndex+5];
  valid=valid&&gd.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&gd.Width==width&&gd.Height==height&&gd.DepthOrArraySize==1&&gd.MipLevels>=1&&gd.MipLevels<=14&&gd.SampleDesc.Count==1&&gd.SampleDesc.Quality==0&&gd.Format==DXGI_FORMAT_R16G16B16A16_FLOAT;
  for(unsigned i=nativeIndex+6;i<maskIndex;++i){const auto& d=descriptions[i];
   valid=valid&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.SampleDesc.Quality==0&&(d.Format==DXGI_FORMAT_R16G16B16A16_FLOAT||d.Format==DXGI_FORMAT_R11G11B10_FLOAT);
  }
  const auto& od=descriptions[outputIndex];
  valid=valid&&od.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&od.Width>0&&od.Width<=4096&&od.Height>0&&od.Height<=2160&&od.DepthOrArraySize==1&&od.MipLevels==1&&od.SampleDesc.Count==1&&(od.Format==DXGI_FORMAT_R11G11B10_FLOAT||od.Format==DXGI_FORMAT_R16G16B16A16_FLOAT);
  for(unsigned i=0;i<=outputIndex;++i)for(unsigned j=0;j<i;++j)if(resources[i]==resources[j])valid=false;
  if(!valid){Log("RR_INPUT_CAPTURE_C1_REJECT reason=native_input_format_or_extent frame=%llu",frame);c->policy.phase=Policy::Failed;__leave;}
  if(c->policy.phase==Policy::Requested){
   if(FAILED(list->GetDevice(IID_PPV_ARGS(&c->device)))||!c->device){c->policy.phase=Policy::Failed;__leave;}
   c->queue=native.queue;c->queue->AddRef();c->count=count;c->distanceBound=distanceBound;c->rawIndex=rawIndex;c->nativeIndex=nativeIndex;c->outputIndex=outputIndex;c->width=static_cast<UINT>(width);c->height=height;c->preset=preset;c->nativeLighting=nativeLighting;
   const wchar_t* guideNames[]{L"normal-roughness.rgba16f",L"specular.rgba16f",L"diffuse.rgba16f",L"distance.r32f",L"distance-status.r32u"};
   for(unsigned i=0;i<count;++i){c->descriptions[i]=descriptions[i];c->pixelBytes[i]=(i<3||i==rawIndex)?8u:4u;
    c->names[i]=i==rawIndex?L"raw-albedo.rgba16f":i<nativeIndex?guideNames[i]:L"";
   }
   c->names[nativeIndex]=descriptions[nativeIndex].Format==DXGI_FORMAT_R11G11B10_FLOAT?L"input-color.r11g11b10f":L"input-color.rgba16f";
   c->pixelBytes[nativeIndex]=descriptions[nativeIndex].Format==DXGI_FORMAT_R11G11B10_FLOAT?4u:8u;
   c->names[nativeIndex+1]=L"input-depth.r32f";c->pixelBytes[nativeIndex+1]=4;
   c->names[nativeIndex+2]=descriptions[nativeIndex+2].Format==DXGI_FORMAT_R16G16_FLOAT?L"input-motion.rg16f":L"input-motion.rg32f";
   c->pixelBytes[nativeIndex+2]=descriptions[nativeIndex+2].Format==DXGI_FORMAT_R16G16_FLOAT?4u:8u;
   c->names[nativeIndex+3]=L"native-gbuffer1.rgba8";c->names[nativeIndex+4]=L"native-gbuffer2.rgba8";
#ifdef CONTROL_RR_GI_SEQUENCE
   c->names[nativeIndex+5]=L"gi-at-rr-input.rgba16f";
#else
   c->names[nativeIndex+5]=L"raw-gi.rgba16f";
#endif
   c->pixelBytes[nativeIndex+5]=8;
   c->names[nativeIndex+6]=descriptions[nativeIndex+6].Format==DXGI_FORMAT_R11G11B10_FLOAT?L"lighting-diffuse.r11g11b10f":L"lighting-diffuse.rgba16f";
   c->names[nativeIndex+7]=descriptions[nativeIndex+7].Format==DXGI_FORMAT_R11G11B10_FLOAT?L"lighting-specular.r11g11b10f":L"lighting-specular.rgba16f";
   for(unsigned i=nativeIndex+6;i<maskIndex;++i)c->pixelBytes[i]=descriptions[i].Format==DXGI_FORMAT_R11G11B10_FLOAT?4u:8u;
   if(responsivity){c->names[maskIndex]=L"hair-responsivity.r16f";c->pixelBytes[maskIndex]=2;}
   c->names[outputIndex]=nativeLighting?
    (od.Format==DXGI_FORMAT_R11G11B10_FLOAT?L"native-sr-output.r11g11b10f":L"native-sr-output.rgba16f"):
    (od.Format==DXGI_FORMAT_R11G11B10_FLOAT?L"rr-output.r11g11b10f":L"rr-output.rgba16f");
   c->pixelBytes[outputIndex]=od.Format==DXGI_FORMAT_R11G11B10_FLOAT?4u:8u;
#ifdef CONTROL_RR_GI_SEQUENCE
   c->preGIIndex=outputIndex+1;c->names[c->preGIIndex]=L"pre-filter-gi.rgba16f";c->pixelBytes[c->preGIIndex]=8;
#endif
   c->exportStride=gi_path_capture::enabled.load()?gi_path_capture::ExportStride(gi_path_capture::condition.load()):1;
   c->pathCondition=gi_path_capture::enabled.load()?gi_path_capture::condition.load():0;
   c->exportRoot[0]=0;if(c->pathCondition)wcscpy_s(c->exportRoot,gi_path_capture::directory);
   c->policy.BeginPrepare();
   if(!QueueUserWorkItem(&RRInputCapturePrepare,c,WT_EXECUTEDEFAULT)){RRInputCaptureRelease(c);c->policy.phase=Policy::Failed;}
   __leave;
  }
  if(c->queue!=native.queue||c->count!=count||c->rawIndex!=rawIndex||c->distanceBound!=distanceBound||c->width!=width||c->height!=height||c->preset!=preset||c->nativeLighting!=nativeLighting){
#ifdef CONTROL_RR_GI_SEQUENCE
   if(c->preGICaptured||c->giHits.commandsStarted){c->policy.phase=Policy::Failed;__leave;}
#endif
   RRInputCaptureRelease(c);c->policy.phase=Policy::Requested;__leave; // No commands yet; reprepare safely.
  }
  bool formatsMatch=true;
  for(unsigned i=0;i<count;++i)if(c->descriptions[i].Format!=descriptions[i].Format||c->descriptions[i].Width!=descriptions[i].Width||c->descriptions[i].Height!=descriptions[i].Height)formatsMatch=false;
  if(!formatsMatch){
#ifdef CONTROL_RR_GI_SEQUENCE
   if(c->preGICaptured||c->giHits.commandsStarted){c->policy.phase=Policy::Failed;__leave;}
#endif
   RRInputCaptureRelease(c);c->policy.phase=Policy::Requested;__leave;}
#ifdef CONTROL_RR_GI_SEQUENCE
  if(!c->preGICaptured||c->preGIFrame!=frame)__leave;
#endif
  for(unsigned i=0;i<count;++i){ID3D12Device* device=nullptr;const HRESULT hr=resources[i]->GetDevice(IID_PPV_ARGS(&device));const bool same=SUCCEEDED(hr)&&device==c->device;if(device)device->Release();if(!same)__leave;}
  for(unsigned i=0;i<=outputIndex;++i){c->sources[i]=resources[i];resources[i]->AddRef();c->states[i]=i>=nativeIndex&&i<nativeIndex+kRRInputCaptureNativeCount?static_cast<D3D12_RESOURCE_STATES>(nativeInputs[i-nativeIndex].trackedState):i==outputIndex?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
  c->jitterX=c->jitterY=c->mvScaleX=c->mvScaleY=0;
  if(ngxGetFloat){ngxGetFloat(parameters,"Jitter.Offset.X",&c->jitterX);ngxGetFloat(parameters,"Jitter.Offset.Y",&c->jitterY);}
  c->motionScaleValid=ngxGetFloat&&ngxGetFloat(parameters,"MV.Scale.X",&c->mvScaleX)==1&&ngxGetFloat(parameters,"MV.Scale.Y",&c->mvScaleY)==1;
  c->preExposure=c->exposureScale=0;
  c->preExposureValid=ngxGetFloat&&ngxGetFloat(parameters,"DLSS.Pre.Exposure",&c->preExposure)==1&&std::isfinite(c->preExposure);
  c->exposureScaleValid=ngxGetFloat&&ngxGetFloat(parameters,"DLSS.Exposure.Scale",&c->exposureScale)==1&&std::isfinite(c->exposureScale);
  if(!c->preExposureValid)c->preExposure=0;
  if(!c->exposureScaleValid)c->exposureScale=0;
  c->lightingMode=lightingMode;
#ifdef CONTROL_RR_F9_SUITE
  if(rr_suite::requested.load()&&!rr_suite::Verified(frame)){
#ifdef CONTROL_RR_GI_SEQUENCE
   if(c->preGICaptured||c->giHits.commandsStarted){c->policy.phase=Policy::Failed;__leave;}
#endif
   for(unsigned i=0;i<count;++i)RRGuideRelease(c->sources[i]);
   control_rr_capture_ui::detail.store(L"Waiting for verified diagnostic shader dispatch.");__leave;
  }
  c->suiteSession=rr_suite::session.load();c->suiteStep=rr_suite::latched.load();
  c->suiteSamples=rr_suite::Current()?rr_suite::Current()->samples:0;
  c->suiteRaw=rr_suite::Current()&&rr_suite::Current()->raw;
#endif
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
  c->giBoundary=control_rr_gi::ReadBoundary(frame);
#ifdef CONTROL_RR_GI_SEQUENCE
  if(!gi_path_capture::BoundaryMatches(c->pathCondition,c->giBoundary.valid,c->giBoundary.applied,
     c->giBoundary.frame,frame,c->giBoundary.stage,c->giBoundary.spatialPasses,c->giBoundary.spatialObserved)){
   Log("GI_PATH_R378_REJECT condition=%u frame=%llu boundary_frame=%llu valid=%u applied=%u stage=%s spatial_requested=%u spatial_observed=%u",
    c->pathCondition,frame,c->giBoundary.frame,unsigned(c->giBoundary.valid),unsigned(c->giBoundary.applied),
    control_rr_gi::StageName(c->giBoundary.stage),c->giBoundary.spatialPasses,c->giBoundary.spatialObserved);
   c->policy.phase=Policy::Failed;__leave;
  }
#endif
#endif
#ifdef CONTROL_RR_GI_SEQUENCE
  c->evaluationTick=GetTickCount64();
#endif
  c->frame=frame;c->projectionValid=projectionValid;memcpy(c->projection,projection,sizeof(c->projection));
  c->outputCaptured=false;c->evaluationResult=0;c->reset=0;
  if(ngxGetInt)ngxGetInt(parameters,"Reset",&c->reset);
  c->previousFrame=c->pairIndex?rrInputCaptures[0].frame:0;
  DWORD fault=0;bool commandsStarted=false;
  const unsigned allocatedCount=c->count;c->count=outputIndex;
  const bool recorded=RRInputCaptureRecordGuarded(c,&native,nativeInputs,&fault,&commandsStarted,kRRInputCaptureNativeCount);
  c->count=allocatedCount;
  if(!recorded){
   if(commandsStarted){c->policy.phase=Policy::Failed;Log("RR_INPUT_CAPTURE_C1_FAILED reason=record fault=0x%08lX resources_retained=1",fault);}
   else {
#ifdef CONTROL_RR_GI_SEQUENCE
    if(c->preGICaptured||c->giHits.commandsStarted){c->policy.phase=Policy::Failed;__leave;}
#endif
    for(unsigned i=0;i<count;++i)RRGuideRelease(c->sources[i]);
    static unsigned rejects=0;if(++rejects<=4||(rejects%240)==0)Log("RR_INPUT_CAPTURE_C1_REJECT reason=guard_revalidation_or_lock frame=%llu fault=0x%08lX",frame,fault);
   }
   __leave;
  }
  c->policy.Record(native.presentToken);rrInputCaptureRecording=c->pairIndex;
  control_rr_capture_ui::detail.store(L"Frame recorded; waiting for native output and GPU completion.");
  Log("RR_INPUT_CAPTURE_C1_RECORDED frame=%llu preset=%u buffers=%u",frame,preset,count);
 } __except(EXCEPTION_EXECUTE_HANDLER){c->policy.phase=control_rr::InputCapturePolicy::Failed;Log("RR_INPUT_CAPTURE_C1_FAILED reason=exception resources_retained=1");}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}

static void RRInputCaptureBeforeEvaluation(ID3D12GraphicsCommandList* list,unsigned long long frame,unsigned preset,
 ID3D12Resource* normal,ID3D12Resource* specular,ID3D12Resource* diffuse,ID3D12Resource* distance,ID3D12Resource* status,const float* projection,bool projectionValid,ID3D12Resource* rawAlbedo,ID3D12Resource* color,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* output,ID3D12Resource* responsivity,void* parameters,bool nativeLighting=false,const char* lightingMode="rr") noexcept {
#ifdef CONTROL_RR_F9_SUITE
 if(rr_suite::cancelled.load())return;
#endif
 // Observe every evaluated frame, including while a worker owns the capture lock.
 // Allocation gaps are not gameplay gaps and must not restart the countdown.
 int reset=1;
 const bool resetKnown=ngxGetInt&&ngxGetInt(parameters,"Reset",&reset)==1;
 const bool settled=rrInputCaptureSettle.Ready(frame,!resetKnown||reset!=0,GetTickCount64());
#ifdef CONTROL_RR_GI_SEQUENCE
 rrInputCaptureSettledFrame=settled?frame:0;
#endif
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 unsigned select=control_rr::SelectInputCapturePair(rrInputCaptures[0].policy.phase,
  rrInputCaptures[1].policy.phase,rrInputCaptures[0].frame,frame);
 // Prepare both slots immediately; only actual GPU recording waits for settling.
 if(select<2&&rrInputCaptures[select].policy.phase==control_rr::InputCapturePolicy::Ready&&!settled){
  control_rr_capture_ui::detail.store(gi_path_capture::enabled.load()&&gi_path_capture::condition.load()==3?L"Arming motion capture. Keep reproducing the issue while moving.":L"Waiting for 5 seconds of gameplay. Hold still.");
  select=2;
 }
#ifdef CONTROL_RR_GI_SEQUENCE
 if(select<2&&rrInputCaptures[select].policy.phase==control_rr::InputCapturePolicy::Ready&&
    (!rrInputCaptures[select].preGICaptured||rrInputCaptures[select].preGIFrame!=frame))select=2;
#endif
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
 if(select<2)RRInputCaptureOneBeforeEvaluation(&rrInputCaptures[select],list,frame,preset,normal,specular,diffuse,distance,status,projection,projectionValid,rawAlbedo,color,depth,motion,output,responsivity,parameters,nativeLighting,lightingMode);
}
static void RRInputCaptureAfterEvaluation(ID3D12GraphicsCommandList* list,unsigned long long frame,ID3D12Resource* output,unsigned result) noexcept {
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 if(result!=1)rrInputCaptureSettle.Reset();
 RRInputCaptureJob* active=nullptr;
 __try {
  if(rrInputCaptureRecording>=2)__leave;
  auto* c=&rrInputCaptures[rrInputCaptureRecording];active=c;rrInputCaptureRecording=2;
  if(c->frame!=frame||c->sources[c->outputIndex]!=output)__leave;
  c->evaluationResult=result;
  RRGuideInputTexture input{};RRGuideInputSnapshot native{};const char* reason=nullptr;
  if((result&0xFFF00000u)==0xBAD00000u||!RRInputCaptureFindOutput(frame,output,&input)||
     !RRGuideReadRendererContext(&native,&reason)||native.engineFrame!=frame||native.commandList!=list){
   Log("RR_INPUT_CAPTURE_C1_OUTPUT_SKIPPED frame=%llu reason=unproven_output_uav_or_failed_evaluation result=0x%08X",frame,result);__leave;
  }
  RRInputCaptureJob copy{};copy.count=1;copy.sources[0]=output;copy.readback[0]=c->readback[c->outputIndex];copy.footprint[0]=c->footprint[c->outputIndex];copy.states[0]=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  DWORD fault=0;bool started=false;
  c->outputCaptured=RRInputCaptureRecordGuarded(&copy,&native,&input,&fault,&started,1,true);
  if(!c->outputCaptured&&started)c->policy.phase=control_rr::InputCapturePolicy::Failed;
  Log("RR_INPUT_CAPTURE_C1_OUTPUT frame=%llu captured=%u pair_index=%u previous_frame=%llu consecutive=%u fault=0x%08lX",frame,unsigned(c->outputCaptured),c->pairIndex,c->previousFrame,unsigned(c->pairIndex==1&&frame==c->previousFrame+1),fault);
 } __except(EXCEPTION_EXECUTE_HANDLER){if(active)active->policy.phase=control_rr::InputCapturePolicy::Failed;}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}
static void RRInputCaptureAfterPresent(unsigned long long present) noexcept {
#if defined(CONTROL_CAPTURE_DIAGNOSTIC)
 const bool pressed=false; // Diagnostic captures are requested by the overlay button.
#else
 static bool held=false;const bool down=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
 const bool pressed=down&&!held;held=down;
#endif
 if(!TryAcquireSRWLockExclusive(&rrInputCaptureLock))return;
 __try {
  using P=control_rr::InputCapturePolicy;
  const bool buttonRequested=control_rr_capture_ui::requested.exchange(false,std::memory_order_acq_rel);
  if(pressed || buttonRequested){rrInputCaptureSettle.Reset();
#ifdef CONTROL_RR_GI_SEQUENCE
   rrInputCaptureSettledFrame=0;rrInputCaptureRequestTick=GetTickCount64();
#endif
DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
   if(buttonRequested || foregroundPid==GetCurrentProcessId()){
    bool accepted=rrInputCaptures[0].policy.phase==P::Idle&&rrInputCaptures[1].policy.phase==P::Idle&&rrInputCaptures[0].policy.requests<kRRCapturePairs;
    if(accepted){for(unsigned i=0;i<2;++i){rrInputCaptures[i].pairIndex=i;rrInputCaptures[i].frame=0;
#ifdef CONTROL_RR_GI_SEQUENCE
     rrInputCaptures[i].preGICaptured=false;rrInputCaptures[i].preGIFrame=0;
#endif
     rrInputCaptures[i].policy.Request();}
#if !defined(CONTROL_CAPTURE_DIAGNOSTIC)
    rrHairShaderAuditRequested.store(true,std::memory_order_release);
#endif
    }
    else if(buttonRequested){control_rr_capture_ui::detail.store(L"Capture busy or session limit reached. PACKAGE LIGHT collects status.");control_rr_capture_ui::state.store(control_rr_capture_ui::State::Failed);}
    Log("RR_INPUT_CAPTURE_C1_REQUEST accepted=%u mode=overlay_pair slots=2 maximum_pairs=%u",unsigned(accepted),kRRCapturePairs);
   }
  }
#ifdef CONTROL_RR_GI_SEQUENCE
  if(control_rr_capture_ui::Busy()&&rrInputCaptureRequestTick&&GetTickCount64()-rrInputCaptureRequestTick>120000){
   for(auto& job:rrInputCaptures)job.policy.phase=P::Failed;
   control_rr_capture_ui::detail.store(L"Sequence timed out. PACKAGE RR will collect diagnostics.");
  }
#endif
  for(auto& job:rrInputCaptures){auto* c=&job;
#ifdef CONTROL_RR_GI_SEQUENCE
   // An early copy without matching RR must never be overwritten or freed in flight.
   if(c->policy.phase==P::Ready&&((c->preGICaptured&&present>c->preGIPresent)||(c->giHits.commandsStarted&&present>c->giHits.present))){
    c->policy.phase=P::Failed;
    Log("RR_INPUT_CAPTURE_R263_FAILED reason=early_gi_without_matching_rr frame=%llu resources_retained=1",c->preGIFrame);
   }
#endif
   const bool mayExport=
#ifdef CONTROL_RR_F9_SUITE
    control_rr::InputCapturePairMayExport(rrInputCaptures[0].policy.phase,rrInputCaptures[1].policy.phase,rr_suite::cancelled.load());
#else
    true;
#endif
   if(mayExport&&c->policy.Retire(c->fence?c->fence->GetCompletedValue():0)){
    if(!QueueUserWorkItem(&RRInputCaptureExport,c,WT_EXECUTEDEFAULT))c->policy.phase=P::Failed;
   }
   if(c->policy.CanSubmit(present)){const HRESULT hr=c->queue->Signal(c->fence,1);c->policy.Submit(SUCCEEDED(hr));
    if(FAILED(hr))Log("RR_INPUT_CAPTURE_C1_FAILED reason=signal resources_retained=1");}
  }
  control_rr_capture_ui::Publish(rrInputCaptures[0].policy.phase,rrInputCaptures[1].policy.phase,
      rrInputCaptures[0].policy.requests,rrInputCaptures[0].outputCaptured && rrInputCaptures[1].outputCaptured &&
      rrInputCaptures[0].evaluationResult==1 && rrInputCaptures[1].evaluationResult==1 &&
      rrInputCaptures[1].frame==rrInputCaptures[0].frame+1 && rrInputCaptures[1].previousFrame==rrInputCaptures[0].frame);
 } __except(EXCEPTION_EXECUTE_HANDLER){for(auto& c:rrInputCaptures)c.policy.phase=control_rr::InputCapturePolicy::Failed;}
 ReleaseSRWLockExclusive(&rrInputCaptureLock);
}

#ifdef CONTROL_RR_GI_SEQUENCE
static std::uint64_t RRStackSourceReady(void* commandList) noexcept {
 if(!gi_stack_capture::enabled.load()||!TryAcquireSRWLockShared(&rrInputCaptureLock))return 0;
 std::uint64_t frame=0;
 __try {
  using P=control_rr::InputCapturePolicy;
  if(rrInputCaptures[0].policy.phase!=P::Ready||rrInputCaptures[1].policy.phase!=P::Ready)__leave;
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  if(RRGuideReadRendererContext(&native,&reason)&&native.commandList==commandList&&rrInputCaptureSettledFrame&&native.engineFrame==rrInputCaptureSettledFrame+1)frame=native.engineFrame;
 }__except(EXCEPTION_EXECUTE_HANDLER){}
 ReleaseSRWLockShared(&rrInputCaptureLock);return frame;
}
static const bool rrStackRegistered=[] {gi_stack_capture::readySource.store(&RRStackSourceReady);return true;}();
#endif
