#pragma once
#include "rr_specular_mv_policy.h"
#include "rr_projection_validation.h"
#include "../build/rr_specular_mv_compiled.h"

// GI27: explicit reflected-image motion vectors for DLSS-RR Model F.
// Reflection hit arrays come from the existing frame-locked reflection capture;
// GBuffer1 comes from the retained live-guide source; depth/MVs are the exact
// resources on the current NGX evaluation. Output lifetime follows the same
// reflection slot/fence that protects the hit-array copies.
struct RRSpecMvConstants {
 float clipToView[16];
 float viewToClip[16];
 float clipToPrevClip[16];
 UINT width,height;
 float mvOutScaleX,mvOutScaleY;
 UINT layers;
};
static_assert(sizeof(RRSpecMvConstants)==53*4,"GI27 spec-MV constant ABI");

struct RRSpecMvSlot {
 ID3D12DescriptorHeap* heap=nullptr;
 ID3D12Resource* output=nullptr;
 ID3D12Resource* debug=nullptr;
};
struct RRSpecMvOwner {
 ID3D12Device* device=nullptr;
 ID3D12RootSignature* root=nullptr;
 ID3D12PipelineState* pipeline=nullptr;
 RRSpecMvSlot slots[3]{};
 UINT width=0,height=0,increment=0;
 bool preparing=false,prepared=false,stopped=false;
 CameraSnapshot previousCamera{};
 bool previousValid=false;
 unsigned long long recorded=0;
};
static std::vector<RRSpecMvOwner*> rrSpecMvOwners;
static unsigned long long rrSpecMvRejected=0;

static DXGI_FORMAT RRSpecMvSrvFormat(DXGI_FORMAT f) noexcept {
 switch(f){
 case DXGI_FORMAT_R8G8B8A8_TYPELESS:return DXGI_FORMAT_R8G8B8A8_UNORM;
 case DXGI_FORMAT_R16_TYPELESS:return DXGI_FORMAT_R16_FLOAT;
 case DXGI_FORMAT_R32_TYPELESS:return DXGI_FORMAT_R32_FLOAT;
 case DXGI_FORMAT_D32_FLOAT:return DXGI_FORMAT_R32_FLOAT;
 case DXGI_FORMAT_R16G16_TYPELESS:return DXGI_FORMAT_R16G16_FLOAT;
 case DXGI_FORMAT_R32G32_TYPELESS:return DXGI_FORMAT_R32G32_FLOAT;
 case DXGI_FORMAT_R16G16B16A16_TYPELESS:return DXGI_FORMAT_R16G16B16A16_FLOAT;
 default:return f;
 }
}
static HRESULT RRSpecMvTexture(RRSpecMvOwner* owner,DXGI_FORMAT format,ID3D12Resource** out) noexcept {
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=owner->width;d.Height=owner->height;
 d.DepthOrArraySize=1;d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;
 d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 return owner->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(out));
}
static DWORD WINAPI RRSpecMvPrepare(void* argument) noexcept {
 auto* owner=static_cast<RRSpecMvOwner*>(argument);AcquireSRWLockExclusive(&rrReflectionLock);
 HRESULT hr=S_OK;
 if(!owner->root){
  D3D12_DESCRIPTOR_RANGE ranges[2]{};
  ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=5;ranges[0].BaseShaderRegister=0;ranges[0].OffsetInDescriptorsFromTableStart=0;
  ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=2;ranges[1].BaseShaderRegister=0;ranges[1].OffsetInDescriptorsFromTableStart=5;
  D3D12_ROOT_PARAMETER params[2]{};
  params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[0].Constants.ShaderRegister=0;params[0].Constants.Num32BitValues=53;
  params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[1].DescriptorTable={2,ranges};
  D3D12_ROOT_SIGNATURE_DESC d{};d.NumParameters=2;d.pParameters=params;
  auto serialize=reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
  ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
  hr=serialize?serialize(&d,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors):E_NOINTERFACE;
  if(SUCCEEDED(hr))hr=blob?owner->device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&owner->root)):E_UNEXPECTED;
  RRGuideRelease(blob);RRGuideRelease(errors);
 }
 D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};pso.pRootSignature=owner->root;pso.CS={kRRSpecularMvShader,sizeof(kRRSpecularMvShader)};
 if(SUCCEEDED(hr)&&!owner->pipeline)hr=owner->device->CreateComputePipelineState(&pso,IID_PPV_ARGS(&owner->pipeline));
 owner->increment=owner->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 for(auto& slot:owner->slots){
  if(FAILED(hr))break;
  hr=RRSpecMvTexture(owner,DXGI_FORMAT_R16G16_FLOAT,&slot.output);
  if(SUCCEEDED(hr))hr=RRSpecMvTexture(owner,DXGI_FORMAT_R16G16B16A16_FLOAT,&slot.debug);
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=7;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if(SUCCEEDED(hr))hr=owner->device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&slot.heap));
 }
 owner->preparing=false;owner->prepared=SUCCEEDED(hr);owner->stopped=FAILED(hr);
 Log("RR_GI27_SPECMV_READY success=%u hr=0x%08lX width=%u height=%u slots=3 format=R16G16_FLOAT debug=RGBA16F reference_formula=1",
  unsigned(owner->prepared),static_cast<unsigned long>(hr),owner->width,owner->height);
 ReleaseSRWLockExclusive(&rrReflectionLock);return 0;
}
static RRSpecMvOwner* RRSpecMvFindOwner(ID3D12Device* device,UINT width,UINT height) noexcept {
 for(auto* o:rrSpecMvOwners)if(o&&o->device==device&&o->width==width&&o->height==height&&!o->stopped)return o;
 auto* o=new(std::nothrow) RRSpecMvOwner;if(!o)return nullptr;
 o->device=device;o->device->AddRef();o->width=width;o->height=height;o->preparing=true;rrSpecMvOwners.push_back(o);
 if(!QueueUserWorkItem(&RRSpecMvPrepare,o,WT_EXECUTEDEFAULT)){o->preparing=false;o->stopped=true;return nullptr;}
 return o;
}
static void RRSpecMvIdentity(float* m) noexcept {
 for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)m[r*4+c]=r==c?1.0f:0.0f;
}
static bool RRSpecMvMultiply(const double* a,const double* b,float* out) noexcept {
 if(!a||!b||!out)return false;
 for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){
  double v=0.0;for(unsigned k=0;k<4;++k)v+=a[r*4+k]*b[k*4+c];
  if(!std::isfinite(v)||std::fabs(v)>std::numeric_limits<float>::max())return false;
  out[r*4+c]=static_cast<float>(v);
 }
 return true;
}
static bool RRSpecMvMatrices(RRSpecMvOwner* owner,const CameraSnapshot& camera,bool reset,
 RRSpecMvConstants& constants,bool& history) noexcept {
 history=false;
 if(!control_rr::CopyValidatedProjection(camera.clipToView,camera.viewToClip,constants.clipToView)||
    !control_rr::CopyValidatedProjection(camera.viewToClip,camera.clipToView,constants.viewToClip))return false;
 RRSpecMvIdentity(constants.clipToPrevClip);
 if(!reset&&owner->previousValid&&owner->previousCamera.engineFrame+1==camera.engineFrame){
  float checked[16]{};
  const bool currentWorld=control_rr::CopyValidatedProjection(camera.clipToWorld,camera.worldToClip,checked);
  const bool previousWorld=control_rr::CopyValidatedProjection(owner->previousCamera.worldToClip,owner->previousCamera.clipToWorld,checked);
  if(currentWorld&&previousWorld&&RRSpecMvMultiply(camera.clipToWorld,owner->previousCamera.worldToClip,constants.clipToPrevClip))history=true;
 }
 return true;
}
static bool RRSpecMvSameDevice(ID3D12Device* expected,ID3D12Resource* resource) noexcept {
 if(!expected||!resource)return false;ID3D12Device* actual=nullptr;bool ok=false;
 __try {ok=SUCCEEDED(resource->GetDevice(IID_PPV_ARGS(&actual)))&&actual==expected;}
 __except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
 if(actual)actual->Release();return ok;
}
static ID3D12Resource* RRSpecMvBeforeEvaluation(ID3D12GraphicsCommandList* list,ID3D12Resource* depth,
 ID3D12Resource* motion,ID3D12Resource* gbuffer1,const CameraSnapshot& camera,
 unsigned long long frame,bool reset,float mvScaleX,float mvScaleY,bool* historyUsed) noexcept {
 if(historyUsed)*historyUsed=false;
 const char* rejection="reflection_unavailable";ID3D12Resource* result=nullptr;
 if(!list||!depth||!motion||!gbuffer1||!std::isfinite(mvScaleX)||!std::isfinite(mvScaleY)||
    std::fabs(mvScaleX)<1.0e-8f||std::fabs(mvScaleY)<1.0e-8f)return nullptr;
 if(!TryAcquireSRWLockExclusive(&rrReflectionLock))return nullptr;
 __try {
  auto* reflection=rrReflection;if(!reflection||!reflection->prepared||reflection->preparing||reflection->stopped)__leave;
  RRGuideInputSnapshot native{};const char* reason=nullptr;control_rr_reflection::Context context{};
  rejection="native_context";
  if(!RRGuideReadRendererContext(&native,&reason)||native.commandList!=list||native.engineFrame!=frame||!RRReflectionCurrent(nullptr,context))__leave;
  control_rr::Lease lease{};
  for(std::size_t i=0;i<3;++i){const auto& state=reflection->capture.Inspect()[i];if(state.state==control_rr::SlotState::Ready&&state.key.frame==frame){lease={i,state.serial};break;}}
  rejection="current_reflection_lease_missing";if(lease.index>=3)__leave;
  const auto input=reflection->distanceInputs[lease.index];
  rejection="producer_constants_unavailable";if(!input.valid)__leave;
  rejection="producer_thread_or_present_differs";
  if(reflection->recordingThread!=GetCurrentThreadId()||!reflection->presents.Matches(lease,native.presentToken))__leave;
  if(const auto handoff=reflection->capture.PrimaryHandoffReason(lease,context,reflection->epoch)){rejection=handoff;__leave;}
  rejection="native_clip_depth_differs_from_ngx_depth";if(input.depthResource!=reinterpret_cast<std::uintptr_t>(depth))__leave;
  const auto gd=gbuffer1->GetDesc(),dd=depth->GetDesc(),md=motion->GetDesc();
  rejection="input_extent_or_format";
  if(gd.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||gd.Width!=input.constants.width||gd.Height!=input.constants.height||
     gd.DepthOrArraySize!=1||gd.MipLevels!=1||RRSpecMvSrvFormat(gd.Format)!=DXGI_FORMAT_R8G8B8A8_UNORM||
     dd.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||dd.Width!=input.constants.width||dd.Height!=input.constants.height||
     dd.DepthOrArraySize!=1||dd.MipLevels!=1||
     md.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||md.Width!=input.constants.width||md.Height!=input.constants.height||
     md.DepthOrArraySize!=1||md.MipLevels!=1)__leave;
  const auto motionFormat=RRSpecMvSrvFormat(md.Format);
  if(motionFormat!=DXGI_FORMAT_R16G16_FLOAT&&motionFormat!=DXGI_FORMAT_R32G32_FLOAT)__leave;
  rejection="input_device_mismatch";
  if(!RRSpecMvSameDevice(reflection->device,gbuffer1)||!RRSpecMvSameDevice(reflection->device,depth)||!RRSpecMvSameDevice(reflection->device,motion))__leave;
  auto* owner=RRSpecMvFindOwner(reflection->device,input.constants.width,input.constants.height);
  rejection="pipeline_not_ready";if(!owner||!owner->prepared||owner->preparing||owner->stopped)__leave;
  const auto& destinations=reflection->backend.Destinations();
  auto* matid=reinterpret_cast<ID3D12Resource*>(destinations[lease.index][0]);
  auto* hitpos=reinterpret_cast<ID3D12Resource*>(destinations[lease.index][1]);
  rejection="reflection_copy_missing";if(!matid||!hitpos)__leave;
  const auto matDesc=matid->GetDesc(),hitDesc=hitpos->GetDesc();
  if(matDesc.Format!=DXGI_FORMAT_R16_UINT||hitDesc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||
     matDesc.Width!=input.constants.width||hitDesc.Width!=input.constants.width||
     matDesc.Height!=input.constants.height||hitDesc.Height!=input.constants.height||
     matDesc.DepthOrArraySize!=input.constants.layers||hitDesc.DepthOrArraySize!=input.constants.layers)__leave;
  RRSpecMvConstants constants{};bool history=false;
  rejection="camera_matrices";if(!RRSpecMvMatrices(owner,camera,reset,constants,history))__leave;
  constants.width=input.constants.width;constants.height=input.constants.height;constants.layers=input.constants.layers;
  constants.mvOutScaleX=0.5f*float(constants.width)/mvScaleX;
  constants.mvOutScaleY=-0.5f*float(constants.height)/mvScaleY;
  auto& slot=owner->slots[lease.index];rejection="slot_unavailable";if(!slot.heap||!slot.output||!slot.debug)__leave;

  auto cpu=slot.heap->GetCPUDescriptorHandleForHeapStart();
  auto srv=[&](ID3D12Resource* resource,DXGI_FORMAT format,D3D12_SRV_DIMENSION dimension,UINT arraySize){
   D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=format;v.ViewDimension=dimension;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   if(dimension==D3D12_SRV_DIMENSION_TEXTURE2D){v.Texture2D.MipLevels=1;}
   else {v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=arraySize;}
   owner->device->CreateShaderResourceView(resource,&v,cpu);cpu.ptr+=owner->increment;
  };
  srv(gbuffer1,RRSpecMvSrvFormat(gd.Format),D3D12_SRV_DIMENSION_TEXTURE2D,1);
  srv(matid,DXGI_FORMAT_R16_UINT,D3D12_SRV_DIMENSION_TEXTURE2DARRAY,input.constants.layers);
  srv(depth,RRSpecMvSrvFormat(dd.Format),D3D12_SRV_DIMENSION_TEXTURE2D,1);
  srv(motion,motionFormat,D3D12_SRV_DIMENSION_TEXTURE2D,1);
  srv(hitpos,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D12_SRV_DIMENSION_TEXTURE2DARRAY,input.constants.layers);
  D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;uv.Format=DXGI_FORMAT_R16G16_FLOAT;
  owner->device->CreateUnorderedAccessView(slot.output,nullptr,&uv,cpu);cpu.ptr+=owner->increment;uv.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
  owner->device->CreateUnorderedAccessView(slot.debug,nullptr,&uv,cpu);

  rejection="reflection_already_consumed";if(!reflection->capture.TakeOnPrimaryQueue(lease,context,reflection->epoch))__leave;
  auto* count=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(native.commandContext)+8);
  D3D12_RESOURCE_BARRIER before[2]{};
  before[0].Type=before[1].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  before[0].Transition.pResource=slot.output;before[1].Transition.pResource=slot.debug;
  for(auto& b:before){b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;}
  ++*count;list->ResourceBarrier(2,before);
  ++*count;list->SetDescriptorHeaps(1,&slot.heap);
  ++*count;list->SetComputeRootSignature(owner->root);
  ++*count;list->SetPipelineState(owner->pipeline);
  ++*count;list->SetComputeRoot32BitConstants(0,53,&constants,0);
  ++*count;list->SetComputeRootDescriptorTable(1,slot.heap->GetGPUDescriptorHandleForHeapStart());
  ++*count;list->Dispatch((constants.width+15)/16,(constants.height+15)/16,1);
  D3D12_RESOURCE_BARRIER after[4]{};
  after[0].Type=after[1].Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;after[0].UAV.pResource=slot.output;after[1].UAV.pResource=slot.debug;
  after[2]=before[0];after[3]=before[1];
  after[2].Transition.StateBefore=after[3].Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  after[2].Transition.StateAfter=after[3].Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  ++*count;list->ResourceBarrier(4,after);
  owner->previousCamera=camera;owner->previousValid=true;
  result=slot.output;if(historyUsed)*historyUsed=history;
  const auto n=++owner->recorded;
  if(n<=4||(n%240)==0)
   Log("RR_GI27_SPECMV frame=%llu count=%llu slot=%u serial=%llu output=%p debug=%p layers=%u history=%u mv_scale=%.6g,%.6g correction_scale=%.6g,%.6g source=reference_formula",
    frame,n,unsigned(lease.index),static_cast<unsigned long long>(lease.serial),slot.output,slot.debug,constants.layers,unsigned(history),
    double(mvScaleX),double(mvScaleY),double(constants.mvOutScaleX),double(constants.mvOutScaleY));
 } __except(EXCEPTION_EXECUTE_HANDLER){
  RRReflectionStop("specmv_recording_fault");result=nullptr;rejection="exception";
 }
 if(!result){const auto n=++rrSpecMvRejected;if(n<=4||(n&(n-1))==0)
  Log("RR_GI27_SPECMV_SKIP frame=%llu count=%llu reason=%s",frame,n,rejection);}
 ReleaseSRWLockExclusive(&rrReflectionLock);return result;
}
