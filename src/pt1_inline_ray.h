#pragma once
#include "../build/pt1_inline_ray_compiled.h"
#include <d3dcompiler.h>

namespace control_pt1 {

static std::atomic<unsigned int> mode{0};
static bool f7Down=false;
static std::atomic<unsigned long long> attempts{0},successes{0},skips{0};

struct Owner {
 ID3D12Device* device=nullptr;
 ID3D12RootSignature* root=nullptr;
 ID3D12PipelineState* pipeline=nullptr;
 ID3D12DescriptorHeap* heap=nullptr;
 UINT increment=0;
 bool ready=false,failed=false;
 DXGI_FORMAT lastFormat=DXGI_FORMAT_UNKNOWN;
 D3D12_GPU_VIRTUAL_ADDRESS lastTlas=0;
 ID3D12Resource* lastTarget=nullptr;
};
static Owner owner{};

struct Constants {
 float clipToWorld[16];
 float cameraPosTMin[4];
 UINT outputSize[2];
 UINT mode;
 UINT pad;
};
static_assert(sizeof(Constants)==96,"PT1 root constant ABI");

static const char* ModeName(unsigned m) noexcept {
 switch(m){case 1:return "hit_miss";case 2:return "hit_distance";case 3:return "instance_id";default:return "off";}
}

static bool SameDevice(ID3D12Device* a,ID3D12Device* b) noexcept {
 if(!a||!b)return false;IUnknown* ua=nullptr;IUnknown* ub=nullptr;
 const HRESULT ha=a->QueryInterface(IID_PPV_ARGS(&ua));const HRESULT hb=b->QueryInterface(IID_PPV_ARGS(&ub));
 const bool same=SUCCEEDED(ha)&&SUCCEEDED(hb)&&ua==ub;if(ua)ua->Release();if(ub)ub->Release();return same;
}

static bool Initialize(ID3D12Device* device) noexcept {
 if(owner.failed)return false;
 if(owner.ready){
  if(SameDevice(owner.device,device))return true;
  Log("PT1_INLINE_RAY_FAIL stage=device_changed");owner.failed=true;return false;
 }
 if(!device||!kPT1InlineRayShaderSize){owner.failed=true;return false;}
 D3D12_FEATURE_DATA_D3D12_OPTIONS5 opt5{};
 HRESULT hr=device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&opt5,sizeof(opt5));
 if(FAILED(hr)||opt5.RaytracingTier<D3D12_RAYTRACING_TIER_1_1){
  Log("PT1_INLINE_RAY_FAIL stage=raytracing_tier hr=0x%08lX tier=%u",static_cast<unsigned long>(hr),unsigned(opt5.RaytracingTier));owner.failed=true;return false;
 }

 D3D12_DESCRIPTOR_RANGE ranges[2]{};
 ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=1;ranges[0].BaseShaderRegister=0;ranges[0].RegisterSpace=0;ranges[0].OffsetInDescriptorsFromTableStart=0;
 ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=1;ranges[1].BaseShaderRegister=0;ranges[1].RegisterSpace=0;ranges[1].OffsetInDescriptorsFromTableStart=0;
 D3D12_ROOT_PARAMETER params[3]{};
 params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable.NumDescriptorRanges=1;params[0].DescriptorTable.pDescriptorRanges=&ranges[0];params[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
 params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[1].DescriptorTable.NumDescriptorRanges=1;params[1].DescriptorTable.pDescriptorRanges=&ranges[1];params[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
 params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants.ShaderRegister=0;params[2].Constants.RegisterSpace=0;params[2].Constants.Num32BitValues=24;params[2].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=3;rd.pParameters=params;rd.Flags=D3D12_ROOT_SIGNATURE_FLAG_NONE;
 ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
 hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);
 if(FAILED(hr)||!blob){
  Log("PT1_INLINE_RAY_FAIL stage=serialize_root hr=0x%08lX errors=%s",static_cast<unsigned long>(hr),errors?static_cast<const char*>(errors->GetBufferPointer()):"<none>");
  if(errors)errors->Release();if(blob)blob->Release();owner.failed=true;return false;
 }
 hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&owner.root));blob->Release();if(errors)errors->Release();
 if(FAILED(hr)||!owner.root){Log("PT1_INLINE_RAY_FAIL stage=create_root hr=0x%08lX",static_cast<unsigned long>(hr));owner.failed=true;return false;}
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=owner.root;pd.CS.pShaderBytecode=kPT1InlineRayShader;pd.CS.BytecodeLength=kPT1InlineRayShaderSize;
 hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&owner.pipeline));
 if(FAILED(hr)||!owner.pipeline){Log("PT1_INLINE_RAY_FAIL stage=create_pipeline hr=0x%08lX dxil_bytes=%zu",static_cast<unsigned long>(hr),kPT1InlineRayShaderSize);owner.failed=true;return false;}
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
 hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&owner.heap));
 if(FAILED(hr)||!owner.heap){Log("PT1_INLINE_RAY_FAIL stage=create_heap hr=0x%08lX",static_cast<unsigned long>(hr));owner.failed=true;return false;}
 owner.increment=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 owner.device=device;device->AddRef();owner.ready=true;
 Log("PT1_INLINE_RAY_READY ready=1 tier=%u dxil_bytes=%zu root_constants=24 descriptor_count=2 mode_default=off hotkey=F7 target=reflectionRayGeneration",
  unsigned(opt5.RaytracingTier),kPT1InlineRayShaderSize);
 return true;
}

static bool GetReflectionTarget(ID3D12Resource** target,unsigned* trackedState,D3D12_RESOURCE_DESC* desc) noexcept {
 if(target)*target=nullptr;if(trackedState)*trackedState=0;if(desc)*desc={};
 if(!target||!trackedState||!desc||!verifiedRenderer||!rrGuideGetNativeTexture)return false;
 __try {
  auto* native=rrGuideGetNativeTexture(reinterpret_cast<unsigned char*>(verifiedRenderer)+kRRShaderReflectionTargetRva);
  NativeTextureStateSnapshot snap{};
  if(!native||!ReadNativeTextureState(native,&snap)||!snap.resource||!snap.stateKnown)return false;
  *target=static_cast<ID3D12Resource*>(snap.resource);*trackedState=snap.trackedState;*desc=snap.desc;return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}

static bool UpdateDescriptors(ID3D12Resource* target,const D3D12_RESOURCE_DESC& desc,D3D12_GPU_VIRTUAL_ADDRESS tlas) noexcept {
 if(!owner.ready||!owner.device||!owner.heap||!target||!tlas)return false;
 if(!(desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.DepthOrArraySize!=1||desc.MipLevels<1)return false;
 D3D12_FEATURE_DATA_FORMAT_SUPPORT fs{};fs.Format=desc.Format;
 if(FAILED(owner.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&fs,sizeof(fs)))||!(fs.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)){
  Log("PT1_INLINE_RAY_SKIP reason=target_format_no_typed_uav_store format=%u support2=0x%X",unsigned(desc.Format),unsigned(fs.Support2));return false;
 }
 auto cpu=owner.heap->GetCPUDescriptorHandleForHeapStart();
 D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=DXGI_FORMAT_UNKNOWN;sd.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.RaytracingAccelerationStructure.Location=tlas;
 owner.device->CreateShaderResourceView(nullptr,&sd,cpu);
 cpu.ptr+=owner.increment;
 D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=desc.Format;ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;ud.Texture2D.MipSlice=0;ud.Texture2D.PlaneSlice=0;
 owner.device->CreateUnorderedAccessView(target,nullptr,&ud,cpu);
 owner.lastFormat=desc.Format;owner.lastTlas=tlas;owner.lastTarget=target;return true;
}

static void RestoreNative(ID3D12GraphicsCommandList4* list,const control_pt0::CommandState& saved,const control_pt0::CommandHooks& h) noexcept {
 auto* base=reinterpret_cast<ID3D12GraphicsCommandList*>(list);
 if(saved.heapCount&&h.setHeaps)h.setHeaps(base,saved.heapCount,saved.heaps);
 if(saved.root&&h.setRoot)h.setRoot(base,saved.root);
 if(saved.pipeline&&h.setPipeline)h.setPipeline(base,saved.pipeline);
 if(saved.stateObject&&h.setState)h.setState(list,saved.stateObject);
 for(UINT i=0;i<control_pt0::MaxCommandTables;++i){
  if(saved.tables[i].ptr&&h.setTable)h.setTable(base,i,saved.tables[i]);
  else if(saved.cbv[i]&&h.setCBV)h.setCBV(base,i,saved.cbv[i]);
  else if(saved.srv[i]&&h.setSRV)h.setSRV(base,i,saved.srv[i]);
  else if(saved.uav[i]&&h.setUAV)h.setUAV(base,i,saved.uav[i]);
  else if(saved.constantsCount[i]&&h.setConstants)h.setConstants(base,i,saved.constantsCount[i],saved.constants[i],0);
 }
}

static bool Run(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* nativeDesc,unsigned long long frame,std::uint64_t signature) noexcept {
 const unsigned m=mode.load(std::memory_order_acquire);if(!m)return false;
 const auto attempt=++attempts;
 const D3D12_GPU_VIRTUAL_ADDRESS tlas=PT0LatestTlas();
 if(!tlas){if(attempt<=8||(attempt%240)==0)Log("PT1_INLINE_RAY_SKIP frame=%llu reason=tlas_unavailable mode=%s sig=%016llX",frame,ModeName(m),signature);++skips;return false;}
 ID3D12Resource* target=nullptr;unsigned state=0;D3D12_RESOURCE_DESC desc{};
 if(!GetReflectionTarget(&target,&state,&desc)){if(attempt<=8||(attempt%240)==0)Log("PT1_INLINE_RAY_SKIP frame=%llu reason=reflection_target_unavailable mode=%s",frame,ModeName(m));++skips;return false;}
 if(state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS){if(attempt<=8||(attempt%240)==0)Log("PT1_INLINE_RAY_SKIP frame=%llu reason=reflection_target_state state=0x%X mode=%s",frame,state,ModeName(m));++skips;return false;}
 ID3D12Device* device=nullptr;if(FAILED(target->GetDevice(IID_PPV_ARGS(&device)))||!device){++skips;return false;}
 const bool initialized=Initialize(device);device->Release();if(!initialized){++skips;return false;}
 if(!UpdateDescriptors(target,desc,tlas)){++skips;return false;}
 CameraSnapshot camera{};DWORD fault=0;const char* reason=nullptr;
 if(!ReadCamera(&camera,&fault,&reason)){if(attempt<=8||(attempt%240)==0)Log("PT1_INLINE_RAY_SKIP frame=%llu reason=camera_%s exception=0x%08lX",frame,reason?reason:"unknown",fault);++skips;return false;}
 Constants constants{};for(UINT i=0;i<16;++i)constants.clipToWorld[i]=static_cast<float>(camera.clipToWorld[i]);
 constants.cameraPosTMin[0]=static_cast<float>(camera.viewToWorld[9]);constants.cameraPosTMin[1]=static_cast<float>(camera.viewToWorld[10]);constants.cameraPosTMin[2]=static_cast<float>(camera.viewToWorld[11]);constants.cameraPosTMin[3]=0.01f;
 constants.outputSize[0]=static_cast<UINT>(desc.Width);constants.outputSize[1]=desc.Height;constants.mode=m;
 if(!nativeDesc||nativeDesc->Width!=constants.outputSize[0]||nativeDesc->Height!=constants.outputSize[1]){
  if(attempt<=8||(attempt%240)==0)Log("PT1_INLINE_RAY_SKIP frame=%llu reason=extent_mismatch native=%ux%u target=%ux%u",frame,nativeDesc?nativeDesc->Width:0,nativeDesc?nativeDesc->Height:0,constants.outputSize[0],constants.outputSize[1]);++skips;return false;
 }
 const auto h=control_pt0::HooksFor(list);if(!h.dispatchCompute||!h.setPipeline||!h.setHeaps||!h.setRoot||!h.setTable||!h.setConstants){++skips;return false;}
 const auto saved=control_pt0::command;
 auto* base=reinterpret_cast<ID3D12GraphicsCommandList*>(list);
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barrier.UAV.pResource=target;base->ResourceBarrier(1,&barrier);
 ID3D12DescriptorHeap* customHeap=owner.heap;h.setHeaps(base,1,&customHeap);h.setRoot(base,owner.root);h.setPipeline(base,owner.pipeline);
 auto gpu=owner.heap->GetGPUDescriptorHandleForHeapStart();h.setTable(base,0,gpu);gpu.ptr+=owner.increment;h.setTable(base,1,gpu);h.setConstants(base,2,24,&constants,0);
 h.dispatchCompute(base,(constants.outputSize[0]+7)/8,(constants.outputSize[1]+7)/8,1);
 base->ResourceBarrier(1,&barrier);
 RestoreNative(list,saved,h);
 const auto ok=++successes;if(ok<=8||(ok%240)==0)Log("PT1_CUSTOM_DISPATCH_OK frame=%llu mode=%s sig=%016llX tlas=0x%llX target=%p format=%u size=%ux%u groups=%ux%u restored_root=%p restored_state=%p restored_pipeline=%p successes=%llu",
  frame,ModeName(m),signature,tlas,target,unsigned(desc.Format),constants.outputSize[0],constants.outputSize[1],(constants.outputSize[0]+7)/8,(constants.outputSize[1]+7)/8,saved.root,saved.stateObject,saved.pipeline,ok);
 return true;
}

static void Poll(unsigned long long present) noexcept {
 const bool down=(GetAsyncKeyState(VK_F7)&0x8000)!=0;
 if(down&&!f7Down){unsigned next=(mode.load(std::memory_order_acquire)+1u)%4u;mode.store(next,std::memory_order_release);Log("PT1_MODE present=%llu mode=%u name=%s hotkey=F7 sequence=off_hitmiss_distance_instance",present,next,ModeName(next));}
 f7Down=down;
}

} // namespace control_pt1

static void PT1AfterNativeReflection(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* nativeDesc,unsigned long long frame,std::uint64_t signature) noexcept {
 control_pt1::Run(list,nativeDesc,frame,signature);
}
static void PT1Poll(unsigned long long present) noexcept {control_pt1::Poll(present);}
