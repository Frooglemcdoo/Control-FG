#pragma once
#include "d3d12_auto_target_binding.h"
#include <d3dcompiler.h>
#include <vector>
namespace gi_reuse {
// Prepared alongside the native state object. Caller copies native shader-config
// associations to this export and audits private space 19 against all native
// roots. Never borrow hit/miss identifiers from another state object.
class D3D12AutoRaygenAugment {
 Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
 D3D12_DXIL_LIBRARY_DESC library_{};
 D3D12_EXPORT_DESC export_{};
 D3D12_LOCAL_ROOT_SIGNATURE local_{};
 D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association_{};
 LPCWSTR name_=Export;
public:
 static constexpr LPCWSTR Export=L"controlAutoGIRaygen";
 HRESULT Initialize(ID3D12Device* device,D3D12_SHADER_BYTECODE code)noexcept {
  if(root_||!device||!code.pShaderBytecode||!code.BytecodeLength)return E_INVALIDARG;
  const auto p=D3D12AutoTargetBinding::RaygenParameters();
  D3D12_ROOT_SIGNATURE_DESC rd{UINT(p.size()),p.data(),0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE};
  Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
  auto hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);if(FAILED(hr))return hr;
  hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_));if(FAILED(hr))return hr;
  export_={Export,L"giAutoRaygen",D3D12_EXPORT_FLAG_NONE};library_={code,1,&export_};local_={root_.Get()};return S_OK;
 }
 // Final vector storage must not move after association pointers are written.
 bool Append(std::vector<D3D12_STATE_SUBOBJECT>& objects)noexcept {
  if(!root_||objects.capacity()-objects.size()<3)return false;
  const auto first=objects.size();
  objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library_});
  objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&local_});
  association_={objects.data()+first+1,1,&name_};
  objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&association_});return true;
 }
};
class D3D12AutoRaygenRecord {
 Microsoft::WRL::ComPtr<ID3D12Resource> data_;
 Microsoft::WRL::ComPtr<ID3D12StateObject> state_;
 HRESULT Write(ID3D12Device* device,ID3D12StateObject* state,const D3D12AutoTargetBinding& binding,bool reuse,UINT64 captureAddress,const UINT* controls)noexcept {
  if((data_&&!reuse)||!device||!state||!binding.Resource())return E_INVALIDARG;
  Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
  auto hr=state->QueryInterface(IID_PPV_ARGS(&properties));if(FAILED(hr))return hr;
  const void* identifier=properties->GetShaderIdentifier(D3D12AutoRaygenAugment::Export);
  std::array<unsigned char,D3D12AutoTargetBinding::RecordBytes> bytes;
  if(!binding.EncodeRaygen(identifier,bytes,captureAddress,controls))return E_INVALIDARG;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes.size();rd.Height=1;
  rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  Microsoft::WRL::ComPtr<ID3D12Resource> data=data_;
  if(!data)hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&data));if(FAILED(hr))return hr;
  void* mapped=nullptr;D3D12_RANGE empty{};hr=data->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
  if(!mapped){data->Unmap(0,&empty);return E_UNEXPECTED;}
  std::memcpy(mapped,bytes.data(),bytes.size());const D3D12_RANGE written{0,bytes.size()};data->Unmap(0,&written);
  data_=std::move(data);state_=state;return S_OK;
 }
public:
 HRESULT Initialize(ID3D12Device* device,ID3D12StateObject* state,const D3D12AutoTargetBinding& binding,UINT64 captureAddress=0,const UINT* controls=nullptr)noexcept {return Write(device,state,binding,false,captureAddress,controls);}
 // Only storage returned by completed AND sealed invocation retirement is admitted.
 HRESULT RewriteRetired(ID3D12Device* device,ID3D12StateObject* state,const D3D12AutoTargetBinding& binding,UINT64 captureAddress=0,const UINT* controls=nullptr)noexcept {return Write(device,state,binding,true,captureAddress,controls);}
 D3D12_GPU_VIRTUAL_ADDRESS_RANGE At(ID3D12StateObject* state)const noexcept {
  if(!data_||state!=state_.Get())return {};
  return {data_->GetGPUVirtualAddress(),D3D12AutoTargetBinding::RecordBytes};
 }
 // Lifetime owner must retain this immutable record AND its target binding.
 UINT64 RetainedBytes()const noexcept{return data_?D3D12AutoTargetBinding::RecordBytes:0;}
};
}
