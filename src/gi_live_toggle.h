#pragma once
#include "gi_reuse/native_gi_identity.h"
#include "gi_reuse/owned_request.h"
// Both raygens share one state object: native hit/miss identifiers remain valid.
namespace gi_live_toggle {
inline const GUID kRecord={0x30730801,0x6611,0x4788,{0x81,0x22,0x33,0x44,0x55,0x66,0x77,0x01}};
inline const GUID kState={0x30730801,0x6611,0x4788,{0x81,0x22,0x33,0x44,0x55,0x66,0x77,0x02}};
inline std::atomic<bool> requested{gi_sequence::LiveRequested()},ready{false};
inline std::atomic<unsigned long long> revision{0},applied{~0ull};
inline constexpr LPCWSTR kExport=L"controlNewGIRayGen";
inline HRESULT Create(gi_sequence::Create next,ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
 if(!device||!desc||!out||!desc->pSubobjects||desc->Type!=D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)return next(device,desc,iid,out);
 UINT match=desc->NumSubobjects;unsigned variant=0;
 for(UINT i=0;i<desc->NumSubobjects;++i){
  const auto& sub=desc->pSubobjects[i];if(sub.Type!=D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY||!sub.pDesc)continue;
  const auto& code=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(sub.pDesc)->DXILLibrary;
  if(!code.pShaderBytecode||(code.BytecodeLength!=24472&&code.BytecodeLength!=27904))continue;
  char sha[65]{};const unsigned v=code.BytecodeLength==24472?0u:1u;
  const char* expected=v==0?"6297319e1fec315c06ee75993e797b51def658e7d41f1588d7df3feb41a1ea2f":"1ff8a93460d94f58fc0e70afff994a334f1c45be292e3e858a63f51070e5734a";
  if(PTReflectionSha256(static_cast<const unsigned char*>(code.pShaderBytecode),code.BytecodeLength,sha)&&!std::strcmp(sha,expected)){
   if(match!=desc->NumSubobjects)return next(device,desc,iid,out);match=i;variant=v;
  }
 }
 if(match==desc->NumSubobjects)return next(device,desc,iid,out);
 try {
  std::vector<D3D12_STATE_SUBOBJECT> objects(desc->pSubobjects,desc->pSubobjects+desc->NumSubobjects);objects.reserve(desc->NumSubobjects+1);
  std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc->NumSubobjects);
  std::vector<std::vector<LPCWSTR>> names(desc->NumSubobjects);
  for(UINT i=0;i<desc->NumSubobjects;++i){
   if(objects[i].Type!=D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION)continue;
   if(!objects[i].pDesc)return next(device,desc,iid,out);
   associations[i]=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[i].pDesc);
   UINT target=desc->NumSubobjects;for(UINT j=0;j<desc->NumSubobjects;++j)if(associations[i].pSubobjectToAssociate==desc->pSubobjects+j)target=j;
   if(target==desc->NumSubobjects||(!associations[i].pExports&&associations[i].NumExports))return next(device,desc,iid,out);
   for(UINT j=0;j<associations[i].NumExports;++j)if(!wcscmp(associations[i].pExports[j],L"rayGenMain")){
    // No replacement record may silently discard native local-root arguments.
    if(objects[target].Type==D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE)return next(device,desc,iid,out);
    names[i].assign(associations[i].pExports,associations[i].pExports+associations[i].NumExports);names[i].push_back(kExport);
    associations[i].NumExports=static_cast<UINT>(names[i].size());associations[i].pExports=names[i].data();break;
   }
   associations[i].pSubobjectToAssociate=objects.data()+target;objects[i].pDesc=&associations[i];
  }
  D3D12_EXPORT_DESC alias{kExport,L"rayGenMain",D3D12_EXPORT_FLAG_NONE};
  D3D12_DXIL_LIBRARY_DESC lib{variant==0?D3D12_SHADER_BYTECODE{gi_sequence::kGILiveShader0,sizeof(gi_sequence::kGILiveShader0)}:D3D12_SHADER_BYTECODE{gi_sequence::kGILiveShader1,sizeof(gi_sequence::kGILiveShader1)},1,&alias};
  auto shared=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(objects[match].pDesc);
  if(gi_reuse::SingleProducerEnabled()){
   shared.DXILLibrary=variant==0?D3D12_SHADER_BYTECODE{gi_sequence::kGISharedHit0,sizeof(gi_sequence::kGISharedHit0)}:D3D12_SHADER_BYTECODE{gi_sequence::kGISharedHit1,sizeof(gi_sequence::kGISharedHit1)};
   objects[match].pDesc=&shared;
   lib.DXILLibrary=variant==0?D3D12_SHADER_BYTECODE{gi_sequence::kGIOwnedRay0,sizeof(gi_sequence::kGIOwnedRay0)}:D3D12_SHADER_BYTECODE{gi_sequence::kGIOwnedRay1,sizeof(gi_sequence::kGIOwnedRay1)};
  }
  objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&lib});auto augmented=*desc;augmented.NumSubobjects=static_cast<UINT>(objects.size());augmented.pSubobjects=objects.data();
  void* candidate=nullptr;const HRESULT hr=next(device,&augmented,iid,&candidate);
  if(FAILED(hr)||!candidate){if(candidate)static_cast<IUnknown*>(candidate)->Release();Log("GI_TOGGLE_R308 create_failed=1 hr=0x%08lX native_fallback=1",static_cast<unsigned long>(hr));return next(device,desc,iid,out);}
  Microsoft::WRL::ComPtr<ID3D12StateObject> state;Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> props;
  HRESULT prepared=static_cast<IUnknown*>(candidate)->QueryInterface(IID_PPV_ARGS(&state));if(SUCCEEDED(prepared))prepared=state.As(&props);
  const void* identifier=SUCCEEDED(prepared)?props->GetShaderIdentifier(kExport):nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> record;
  if(identifier){
   D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
   D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=64;buffer.Height=1;buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   prepared=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&record));
   void* mapped=nullptr;D3D12_RANGE empty{0,0};if(SUCCEEDED(prepared))prepared=record->Map(0,&empty,&mapped);
   if(SUCCEEDED(prepared)){memset(mapped,0,64);memcpy(mapped,identifier,32);record->Unmap(0,nullptr);prepared=state->SetPrivateDataInterface(kRecord,record.Get());}
  }else prepared=E_FAIL;
  if(FAILED(prepared)){static_cast<IUnknown*>(candidate)->Release();Log("GI_TOGGLE_R308 record_failed=1 native_fallback=1");return next(device,desc,iid,out);}
  const UINT variantTag=variant+1;state->SetPrivateData(gi_reuse::nativeGIVariantKey,sizeof(variantTag),&variantTag);
  *out=candidate;ready.store(true);Log("GI_TOGGLE_R308 ready=1 variant=%u native_original=1 new_sampler=R307 same_state=1",variant);return hr;
 }catch(...){Log("GI_TOGGLE_R308 exception=1 native_fallback=1");return next(device,desc,iid,out);}
}
inline void OnState(ID3D12GraphicsCommandList4* list,ID3D12StateObject* state) noexcept {list->SetPrivateDataInterface(kState,state);rt_spot_runtime::OnState(list,state);}
inline bool Dispatch(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc) noexcept {
 if(desc&&desc->Depth==1&&desc->RayGenerationShaderRecord.SizeInBytes==32){
  Microsoft::WRL::ComPtr<ID3D12StateObject> state;UINT size=sizeof(ID3D12StateObject*);
  if(SUCCEEDED(list->GetPrivateData(kState,&size,state.GetAddressOf()))&&state){
   Microsoft::WRL::ComPtr<ID3D12Resource> record;size=sizeof(ID3D12Resource*);
   if(SUCCEEDED(state->GetPrivateData(kRecord,&size,record.GetAddressOf()))&&record){
    const auto version=revision.load();const bool single=gi_reuse::SingleProducerEnabled();
    const bool useNew=single?(gi_reuse::ownedRequested.load()&&gi_reuse::singleEvaluatorReady.load()):requested.load();
    if(single){gi_reuse::singleRayApplied.store(useNew);if(useNew)++gi_reuse::singleRayDispatches;}
    if(applied.exchange(version)!=version)Log("GI_TOGGLE_R308 dispatch_recorded=1 mode=%s revision=%llu gpu_completion=0",useNew?"new":"native",version);
    settings_check::Record(useNew?settings_check::OwnedRays:settings_check::NativeRays);
    if(useNew){auto custom=*desc;custom.RayGenerationShaderRecord={record->GetGPUVirtualAddress(),32};rt_shadow_capture::Lookup(list).dispatch(list,&custom);
     return true;}
    return false;
   }
  }
 }
 return rt_spot_runtime::Dispatch(list,desc);
}
inline void Toggle() noexcept {requested.store(!requested.load());revision.fetch_add(1);Log("GI_TOGGLE_R308 requested=%s filters_unchanged=1",requested.load()?"new":"native");}
inline const wchar_t* Label() noexcept {return !ready.load()?L"GI SAMPLER: WAIT":requested.load()?L"GI SAMPLER: NEW":L"GI SAMPLER: NATIVE";}
}
