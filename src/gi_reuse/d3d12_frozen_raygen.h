#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <vector>
#include <cstring>
#include <cmath>
namespace gi_reuse {
// Attach to the SAME native state object as the compact-aware hit/miss exports.
// Existing shader-config associations must include Export; never copy hit/miss
// identifiers from a different state object. No native local-root substitution.
class D3D12FrozenRaygenAugment {
 Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
 D3D12_DXIL_LIBRARY_DESC library_{};
 D3D12_EXPORT_DESC export_{};
 D3D12_LOCAL_ROOT_SIGNATURE local_{};
 D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association_{};
 LPCWSTR name_=Export;
public:
 static constexpr LPCWSTR Export=L"controlFrozenGIRaygen";
 HRESULT Initialize(ID3D12Device* device,D3D12_SHADER_BYTECODE code)noexcept {
  if(root_||!device||!code.pShaderBytecode||!code.BytecodeLength)return E_INVALIDARG;
  D3D12_ROOT_PARAMETER p[3]{};
  p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;p[0].Descriptor={0,18};
  p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[1].Descriptor={0,18};
  p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[2].Constants={0,18,8};
  D3D12_ROOT_SIGNATURE_DESC rd{3,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE};
  Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
  auto hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);if(FAILED(hr))return hr;
  hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_));if(FAILED(hr))return hr;
  export_={Export,L"giFrozenRaygen",D3D12_EXPORT_FLAG_NONE};library_={code,1,&export_};local_={root_.Get()};return S_OK;
 }
 // Caller reserves the FINAL vector capacity before rebasing any associations.
 // Object remains alive until CreateStateObject returns; no reallocations here.
 bool Append(std::vector<D3D12_STATE_SUBOBJECT>& objects)noexcept {
  if(!root_||objects.capacity()-objects.size()<3)return false;
  const auto first=objects.size();
  objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library_});
  objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&local_});
  association_={objects.data()+first+1,1,&name_};
  objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&association_});return true;
 }
};
// Immutable per-invocation records: 32-byte identifier, two 64-bit root GPU
// addresses, eight constants. 80 used bytes, size padded to 96, starts 128 apart.
// Both phases use this raygen and the same receiver/sampling implementation
// as the training shader. Owned compact also uses Duff; unmodified native uses
// the older basis. Do not mix unmodified native draws with Duff reconstruction.
class D3D12FrozenRaygenRecords {
 Microsoft::WRL::ComPtr<ID3D12Resource> records_;
 std::array<UINT,6> identity_{};
 D3D12_GPU_VIRTUAL_ADDRESS guides_=0,weights_=0;
 std::array<unsigned char,32> identifier_{};
 bool ready_=false;
public:
 static constexpr UINT RecordSize=96,Stride=128,Bytes=384;
 D3D12FrozenRaygenRecords()=default;
 D3D12FrozenRaygenRecords(const D3D12FrozenRaygenRecords&)=delete;
 D3D12FrozenRaygenRecords& operator=(const D3D12FrozenRaygenRecords&)=delete;
 HRESULT Initialize(ID3D12Device* device,const void* identifier,
                    D3D12_GPU_VIRTUAL_ADDRESS guides,D3D12_GPU_VIRTUAL_ADDRESS weights,
                    UINT width,UINT height,UINT frame,UINT epoch,UINT guideSlots,
                    UINT trainingParity,float fraction)noexcept {
  if(ready_||!device||!identifier||!guides||guides%4||!weights||weights%4||
     width<2||(width&1)||!height||width>16384||height>16384||!frame||!epoch||
     !guideSlots||guideSlots>65536||trainingParity>1||!std::isfinite(fraction)||fraction<0||fraction>.5f)return E_INVALIDARG;
  std::array<unsigned char,Bytes> bytes{};
  for(UINT phase=0;phase<3;++phase){
   auto* target=bytes.data()+phase*Stride;std::memcpy(target,identifier,32);
   std::memcpy(target+32,&guides,8);std::memcpy(target+40,&weights,8);
   const UINT control[]={width,height,frame,epoch,guideSlots,trainingParity^(phase?1u:0u),phase==1?1u:0u,0};
   std::memcpy(target+48,control,sizeof(control));std::memcpy(target+76,&fraction,4);
  }
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=Bytes;rd.Height=1;
  rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  Microsoft::WRL::ComPtr<ID3D12Resource> data=records_;HRESULT hr=S_OK;
  if(data){Microsoft::WRL::ComPtr<ID3D12Device> owner;hr=data->GetDevice(IID_PPV_ARGS(&owner));
   if(FAILED(hr)||owner.Get()!=device)return E_INVALIDARG;
  }else{hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&data));if(FAILED(hr))return hr;}
  void* mapped=nullptr;const D3D12_RANGE empty{};hr=data->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
  if(!mapped){data->Unmap(0,&empty);return E_UNEXPECTED;}
  std::memcpy(mapped,bytes.data(),Bytes);const D3D12_RANGE written{0,Bytes};data->Unmap(0,&written);records_=std::move(data);identity_={width,height,frame,epoch,guideSlots,trainingParity};
  guides_=guides;weights_=weights;std::memcpy(identifier_.data(),identifier,identifier_.size());ready_=true;return S_OK;
 }
 bool MatchesStateObject(ID3D12StateObject* state)const noexcept {
  if(!ready_||!state)return false;
  Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
  if(FAILED(state->QueryInterface(IID_PPV_ARGS(&properties))))return false;
  const auto* identifier=properties->GetShaderIdentifier(D3D12FrozenRaygenAugment::Export);
  return identifier&&std::memcmp(identifier_.data(),identifier,identifier_.size())==0;
 }
 bool Matches(UINT width,UINT height,UINT frame,UINT epoch,UINT guideSlots,UINT parity,
              D3D12_GPU_VIRTUAL_ADDRESS guides,D3D12_GPU_VIRTUAL_ADDRESS weights)const noexcept {
  return ready_&&identity_==std::array<UINT,6>{width,height,frame,epoch,guideSlots,parity}&&guides_==guides&&weights_==weights;
 }
 D3D12_GPU_VIRTUAL_ADDRESS_RANGE At(UINT phase)const noexcept {
  return ready_&&phase<2?D3D12_GPU_VIRTUAL_ADDRESS_RANGE{records_->GetGPUVirtualAddress()+phase*Stride,RecordSize}:D3D12_GPU_VIRTUAL_ADDRESS_RANGE{};
 }
 // Third immutable record traces only the complement with guide use disabled.
 // It cannot accidentally read uninitialized guide contents on recovery.
 D3D12_GPU_VIRTUAL_ADDRESS_RANGE NativeComplement()const noexcept {
  return ready_?D3D12_GPU_VIRTUAL_ADDRESS_RANGE{records_->GetGPUVirtualAddress()+2*Stride,RecordSize}:D3D12_GPU_VIRTUAL_ADDRESS_RANGE{};
 }
 ID3D12Resource* Resource()const noexcept{return ready_?records_.Get():nullptr;}
 // UPLOAD records may be rewritten only after the whole invocation retires.
 // The host must obtain this owner through Take after both resets/final fence.
 bool RearmAfterRetirement()noexcept {
  if(!ready_)return false;
  ready_=false;identity_={};guides_=weights_=0;identifier_={};return true;
 }
};
}
