#pragma once
#include "../build/rr_distance_compiled.h"
// Shares the reflection capture's lease and fence. Outputs cannot be reused
// until all commands, including RR consumption, are covered by that fence.
struct RRDistanceSlot {ID3D12DescriptorHeap* heap=nullptr;ID3D12Resource* output=nullptr;ID3D12Resource* status=nullptr;};
struct RRDistanceOwner {
 ID3D12Device* device=nullptr;ID3D12RootSignature* root=nullptr;ID3D12PipelineState* pipeline=nullptr;
 RRDistanceSlot slots[3]{};UINT width=0,height=0,increment=0;
 bool preparing=false,prepared=false,stopped=false,exporting=false;
 unsigned long long recorded=0,epoch=0,lastUseEpoch=0;
 ID3D12Resource* readback[2]{};D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[2]{};UINT64 readbackBytes[2]{};
 control_rr::LiveCapturePolicy capturePolicy{};RRDistanceInput captured{};
};
#include "rr_distance_resize.h"
static RRDistanceOwner* rrDistance=nullptr;
static unsigned long long rrDistanceRejected=0;
static ID3D12Resource* rrDistanceLastStatus=nullptr; // render-thread-only, current successful evaluation
__declspec(noinline) static RRDistanceOwner* RRDistanceCreateOwner() noexcept {
 try {return new(std::nothrow) RRDistanceOwner;}catch(...){return nullptr;}
}
static void RRDistanceObserveSubmission(control_rr::Lease lease,std::uint64_t fence) noexcept {
 if(rrDistance)rrDistance->capturePolicy.Submit(static_cast<unsigned>(lease.index),lease.serial,fence);
}
static HRESULT RRDistanceTexture(RRDistanceOwner* owner,DXGI_FORMAT format,ID3D12Resource** resource) noexcept {
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=owner->width;desc.Height=owner->height;
 desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=format;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 return owner->device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(resource));
}
static DWORD WINAPI RRDistancePrepare(void* argument) noexcept {
 auto* owner=static_cast<RRDistanceOwner*>(argument);AcquireSRWLockExclusive(&rrReflectionLock);
 HRESULT hr=S_OK;
 if(!owner->root){
 D3D12_DESCRIPTOR_RANGE ranges[2]{};
 ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=3;
 ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=2;ranges[1].OffsetInDescriptorsFromTableStart=3;
 D3D12_ROOT_PARAMETER parameters[2]{};parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={2,ranges};
 parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[1].Constants.Num32BitValues=36;
 D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=2;desc.pParameters=parameters;
 auto serialize=reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
 ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
 hr=serialize?serialize(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors):E_NOINTERFACE;
 if(SUCCEEDED(hr))hr=blob?owner->device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&owner->root)):E_UNEXPECTED;
 RRGuideRelease(blob);RRGuideRelease(errors);
 }
 D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=owner->root;pipeline.CS={kRRDistanceShader,sizeof(kRRDistanceShader)};
 if(SUCCEEDED(hr)&&!owner->pipeline)hr=owner->device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&owner->pipeline));
 owner->increment=owner->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 for(auto& slot:owner->slots){
  if(FAILED(hr))break;
  hr=RRDistanceTexture(owner,DXGI_FORMAT_R32_FLOAT,&slot.output);
  if(SUCCEEDED(hr))hr=RRDistanceTexture(owner,DXGI_FORMAT_R32_UINT,&slot.status);
  D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=5;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if(SUCCEEDED(hr))hr=owner->device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&slot.heap));
  if(FAILED(hr))break;
  auto handle=slot.heap->GetCPUDescriptorHandleForHeapStart();handle.ptr+=SIZE_T(owner->increment)*3;
  D3D12_UNORDERED_ACCESS_VIEW_DESC view{};view.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;view.Format=DXGI_FORMAT_R32_FLOAT;
  owner->device->CreateUnorderedAccessView(slot.output,nullptr,&view,handle);handle.ptr+=owner->increment;view.Format=DXGI_FORMAT_R32_UINT;
  owner->device->CreateUnorderedAccessView(slot.status,nullptr,&view,handle);
 }
 owner->preparing=false;owner->prepared=SUCCEEDED(hr);owner->stopped=FAILED(hr);
 Log("RR_DISTANCE_PREPARED success=%u hr=0x%08lX width=%u height=%u slots=3 diagnostic_readback=0",unsigned(owner->prepared),static_cast<unsigned long>(hr),owner->width,owner->height);
 ReleaseSRWLockExclusive(&rrReflectionLock);return 0;
}
#include "rr_distance_record.h"
static ID3D12Resource* RRDistanceBeforeEvaluation(ID3D12GraphicsCommandList* list,ID3D12Resource* depth,unsigned long long frame,bool rrLighting) noexcept {
 rrDistanceLastStatus=nullptr;
 const char* rejection="reflection_unavailable";std::uintptr_t expectedDepth=0;
 ID3D12Resource* result=nullptr;void* depthTracker=nullptr;void* recording=nullptr;bool depthLocked=false,recordingLocked=false;
 if(!TryAcquireSRWLockExclusive(&rrReflectionLock))return nullptr;
 __try {
  auto* reflection=rrReflection;if(!reflection||!reflection->prepared||reflection->preparing||reflection->stopped||!depth)__leave;
  RRGuideInputSnapshot native{};const char* reason=nullptr;
  control_rr_reflection::Context context{};
  rejection="native_context";
  if(!RRGuideReadRendererContext(&native,&reason)||native.commandList!=list||native.engineFrame!=frame||!RRReflectionCurrent(nullptr,context))__leave;
  control_rr::Lease lease{};
  for(std::size_t i=0;i<3;++i){const auto& state=reflection->capture.Inspect()[i];if(state.state==control_rr::SlotState::Ready&&state.key.frame==frame){lease={i,state.serial};break;}}
  rejection="current_reflection_lease_missing";
  if(lease.index>=3)__leave;
  const auto input=reflection->distanceInputs[lease.index];
  expectedDepth=input.depthResource;rejection="producer_constants_unavailable";if(!input.valid)__leave;
  rejection="producer_thread_or_present_differs";
  if(reflection->recordingThread!=GetCurrentThreadId()||!reflection->presents.Matches(lease,native.presentToken))__leave;
  if(const auto handoff=reflection->capture.PrimaryHandoffReason(lease,context,reflection->epoch)){rejection=handoff;__leave;}
  const auto consumerContext=context;
  rejection="native_clip_depth_differs_from_ngx_depth";if(input.depthResource!=reinterpret_cast<std::uintptr_t>(depth))__leave;
  rejection="pipeline_prepare_requested";
  if(!rrDistance){
   auto* owner=RRDistanceCreateOwner();if(!owner)__leave;rrDistance=owner;
   owner->device=reflection->device;owner->device->AddRef();owner->width=input.constants.width;owner->height=input.constants.height;owner->epoch=reflection->epoch;owner->preparing=true;
   if(!QueueUserWorkItem(&RRDistancePrepare,owner,WT_EXECUTEDEFAULT)){owner->preparing=false;owner->stopped=true;}
   __leave;
  }
  auto* owner=rrDistance;
  rejection="pipeline_device_changed";
  if(owner->device!=reflection->device)__leave;
  if(owner->width!=input.constants.width||owner->height!=input.constants.height){
   rejection="pipeline_resize_retirement_pending";
   if(!RRDistanceCanResize(owner,reflection->epoch))__leave;
   RRDistanceReleaseOutputs(owner);
   owner->width=input.constants.width;owner->height=input.constants.height;owner->epoch=reflection->epoch;owner->preparing=true;
   Log("RR_DISTANCE_RESIZE width=%u height=%u epoch=%llu old_use_epoch=%llu",owner->width,owner->height,owner->epoch,owner->lastUseEpoch);
   if(!QueueUserWorkItem(&RRDistancePrepare,owner,WT_EXECUTEDEFAULT)){owner->preparing=false;owner->stopped=true;}
   __leave;
  }
  if(owner->epoch!=reflection->epoch){owner->epoch=reflection->epoch;owner->recorded=0;}
  rejection="pipeline_not_ready_or_extent_changed";
  if(!owner->prepared||owner->preparing||owner->stopped||owner->device!=reflection->device||owner->width!=input.constants.width||owner->height!=input.constants.height)__leave;
  rejection="depth_format_or_extent";
  const auto depthDesc=depth->GetDesc();
  if(depthDesc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||depthDesc.Width!=owner->width||depthDesc.Height!=owner->height||depthDesc.DepthOrArraySize!=1||depthDesc.MipLevels!=1||depthDesc.SampleDesc.Count!=1||
   (depthDesc.Format!=DXGI_FORMAT_R32_TYPELESS&&depthDesc.Format!=DXGI_FORMAT_R32_FLOAT)||(depthDesc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))__leave;
  auto access=rrReflectionAccess.Callbacks();std::uintptr_t tracker=0,resource=0;unsigned state=0,flags=0;unsigned char manual=0;
  rejection="depth_tracker_fields";
  if(!control_rr_reflection::Read(access,input.depthNative,0x50,tracker)||!control_rr_reflection::Read(access,input.depthNative,0x38,flags)||
   !control_rr_reflection::Read(access,input.depthNative,0x68,manual)||(flags&0x30)||manual)__leave;
  if(!tracker)tracker=input.depthNative+0x40;depthTracker=reinterpret_cast<void*>(tracker);recording=native.recordingLock;
  rejection="depth_tracker_busy";
  depthLocked=RRGuideTryAcquireNativeLock(depthTracker);if(!depthLocked)__leave;
  rejection="recording_lock_busy";
  recordingLocked=RRGuideTryAcquireNativeLock(recording);if(!recordingLocked)__leave;
  rejection="depth_state_or_context_changed";
  if(!control_rr_reflection::Read(access,tracker,0x20,state)||!(state&0x40)||(state&~0xe0u)||
   !control_rr_reflection::Read(access,input.depthNative,0x88,resource)||resource!=input.depthResource||
   !RRReflectionCurrent(nullptr,context)||!control_rr_reflection::Same(context,consumerContext)||
   !reflection->presents.Matches(lease,native.presentToken)||reflection->capture.PrimaryHandoffReason(lease,context,reflection->epoch))__leave;
  auto& slot=owner->slots[lease.index];auto handle=slot.heap->GetCPUDescriptorHandleForHeapStart();
  D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  view.Texture2DArray.MipLevels=1;view.Texture2DArray.ArraySize=input.constants.layers;
  const auto& destinations=reflection->backend.Destinations();
  view.Format=DXGI_FORMAT_R16_UINT;owner->device->CreateShaderResourceView(reinterpret_cast<ID3D12Resource*>(destinations[lease.index][0]),&view,handle);handle.ptr+=owner->increment;
  view.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;owner->device->CreateShaderResourceView(reinterpret_cast<ID3D12Resource*>(destinations[lease.index][1]),&view,handle);handle.ptr+=owner->increment;
  view={};view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Texture2D.MipLevels=1;view.Format=DXGI_FORMAT_R32_FLOAT;
  owner->device->CreateShaderResourceView(depth,&view,handle);
  rejection="reflection_already_consumed";
  if(!reflection->capture.TakeOnPrimaryQueue(lease,context,reflection->epoch))__leave;
  DWORD fault=0;
  owner->lastUseEpoch=reflection->epoch; // mark before commands can be emitted
  const auto perf=RRPerfBegin(native,RRPerfStage::Distance);
  if(!RRDistanceRecordLeaf(owner,&slot,&native,&input.constants,&fault)){
   owner->stopped=true;RRReflectionStop("distance_recording_fault");Log("RR_DISTANCE_STOP fault=0x%08lX",fault);__leave;
  }
  RRPerfEnd(perf);
  if(false&&rrLighting&&owner->capturePolicy.Begin(static_cast<unsigned>(lease.index),lease.serial)){
   const bool captured=RRDistanceCaptureLeaf(owner,&slot,&native,&fault);owner->capturePolicy.Recorded(captured);
   if(!captured){owner->stopped=true;RRReflectionStop("distance_capture_fault");__leave;}
   owner->captured=input;
   rrReflectionSubmissionObserver=&RRDistanceObserveSubmission;
  }
  result=slot.output;rrDistanceLastStatus=slot.status;const auto count=++owner->recorded;
  if(count<=4||(count%240)==0)Log("RR_DISTANCE_RECORDED count=%llu frame=%llu slot=%u serial=%llu representative_ray=0",count,frame,unsigned(lease.index),static_cast<unsigned long long>(lease.serial));
 } __except(EXCEPTION_EXECUTE_HANDLER){if(rrDistance)rrDistance->stopped=true;RRReflectionStop("distance_snapshot_fault");}
 __try {if(recordingLocked)RRGuideReleaseNativeLock(recording);if(depthLocked)RRGuideReleaseNativeLock(depthTracker);}
 __except(EXCEPTION_EXECUTE_HANDLER){result=nullptr;if(rrDistance)rrDistance->stopped=true;RRReflectionStop("distance_unlock_fault");}
 control_rr::RRUserDistanceDispatch(result!=nullptr,rejection);
 if(!result){const auto count=++rrDistanceRejected;if(count<=4||(count%240)==0)Log("RR_DISTANCE_SKIP count=%llu frame=%llu reason=%s expected_depth=%p ngx_depth=%p",count,frame,rejection,reinterpret_cast<void*>(expectedDepth),depth);}
 ReleaseSRWLockExclusive(&rrReflectionLock);return result;
}
static DWORD WINAPI RRDistanceExport(void* argument) noexcept {
 auto* owner=static_cast<RRDistanceOwner*>(argument);void* mapped[2]{};bool okay=true;unsigned long long counts[5]{};
 try {
  for(UINT i=0;i<2;++i){D3D12_RANGE range{0,static_cast<SIZE_T>(owner->readbackBytes[i])};if(FAILED(owner->readback[i]->Map(0,&range,&mapped[i]))||!mapped[i]){okay=false;break;}}
  if(okay){
   wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
   if(!length||length>=32768)okay=false;
   else {
    std::wstring directory(env,length);directory+=L"\\ControlFGProbe\\rr-distance-r20-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(owner->captured.context.frame);
    okay=CreateDirectoryW(directory.c_str(),nullptr)!=FALSE;
    const wchar_t* names[2]{L"distance.r32f",L"status.r32u"};
    for(UINT i=0;i<2&&okay;++i)okay=RRGuideExportDetail::WriteRaw(directory+L"\\"+names[i],static_cast<unsigned char*>(mapped[i])+owner->footprint[i].Offset,owner->footprint[i].Footprint.RowPitch,size_t(owner->width)*4,owner->height);
    if(okay){
     for(UINT y=0;y<owner->height;++y){auto* row=static_cast<unsigned char*>(mapped[1])+owner->footprint[1].Offset+size_t(y)*owner->footprint[1].Footprint.RowPitch;
      for(UINT x=0;x<owner->width;++x){unsigned status=0;memcpy(&status,row+size_t(x)*4,4);++counts[status<5?status:0];}}
     std::ostringstream out;out.imbue(std::locale::classic());out<<"{\"schema\":\"control-rr-distance-r20\",\"pid\":"<<GetCurrentProcessId()<<",\"frame\":"<<owner->captured.context.frame<<",\"width\":"<<owner->width<<",\"height\":"<<owner->height<<",\"representative_ray\":0,\"rr_lighting\":true,\"invalid\":"<<counts[0]<<",\"hit\":"<<counts[1]<<",\"miss\":"<<counts[2]<<",\"no_ray\":"<<counts[3]<<",\"sky\":"<<counts[4]<<"}\n";
     const auto json=out.str();RRGuideExportDetail::File file(directory+L"\\metadata.pending");okay=file.Write(json.data(),json.size())&&file.Finish();
     if(okay)okay=MoveFileExW((directory+L"\\metadata.pending").c_str(),(directory+L"\\metadata.json").c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
    }
   }
  }
 }catch(...){okay=false;}
 for(UINT i=0;i<2;++i)if(mapped[i]){D3D12_RANGE written{0,0};owner->readback[i]->Unmap(0,&written);}
 AcquireSRWLockExclusive(&rrReflectionLock);
 if(okay&&counts[0])owner->stopped=true;
 Log("RR_DISTANCE_EXPORTED success=%u frame=%llu invalid=%llu hit=%llu miss=%llu no_ray=%llu sky=%llu",unsigned(okay),static_cast<unsigned long long>(owner->captured.context.frame),counts[0],counts[1],counts[2],counts[3],counts[4]);
 owner->exporting=false;ReleaseSRWLockExclusive(&rrReflectionLock);return 0;
}
static void RRDistanceAfterPresent() noexcept {
 if(!TryAcquireSRWLockExclusive(&rrReflectionLock))return;
 __try {
  auto* owner=rrDistance;auto* reflection=rrReflection;
  if(owner&&reflection&&!owner->stopped&&owner->capturePolicy.state==control_rr::CaptureState::Submitted){
   const auto completed=reflection->fence->GetCompletedValue();
   if(owner->capturePolicy.Retire(completed)){
    owner->exporting=true;
    if(!QueueUserWorkItem(&RRDistanceExport,owner,WT_EXECUTEDEFAULT)){owner->exporting=false;Log("RR_DISTANCE_EXPORT_FAILED reason=worker_queue");}
   }
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){if(rrDistance)rrDistance->stopped=true;}
 ReleaseSRWLockExclusive(&rrReflectionLock);
}
