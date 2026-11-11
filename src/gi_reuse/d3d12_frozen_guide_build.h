#pragma once
#include "d3d12_compute_bindings.h"
#include "frozen_guide_training.h"
#include <d3dcompiler.h>
namespace gi_reuse {
// Shared immutable pipeline; no PSO creation in the per-frame recording path.
class D3D12FrozenGuideBuild {
 Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
 std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,4> stages_;
 static void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource)noexcept {
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=resource;list->ResourceBarrier(1,&b);
 }
 static bool Buffer(ID3D12Resource* resource,UINT64 bytes,bool write)noexcept {
  if(!resource||!resource->GetGPUVirtualAddress())return false;const auto d=resource->GetDesc();
  return d.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER&&d.Width>=bytes&&
   !(d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)&&(!write||(d.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
 }
public:
 HRESULT Initialize(ID3D12Device* device,const std::array<D3D12_SHADER_BYTECODE,4>& shaders)noexcept {
  if(root_||!device)return E_INVALIDARG;
  for(const auto& shader:shaders)if(!shader.pShaderBytecode||!shader.BytecodeLength)return E_INVALIDARG;
  D3D12_ROOT_PARAMETER p[4]{};
  p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;p[0].Descriptor={0,0};
  p[1].ParameterType=p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[1].Descriptor={0,0};p[2].Descriptor={1,0};
  p[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[3].Constants={0,0,4};
  D3D12_ROOT_SIGNATURE_DESC rd{4,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
  Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
  auto hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);if(FAILED(hr))return hr;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
  hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root));if(FAILED(hr))return hr;
  std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,4> stages;
  for(UINT i=0;i<4;++i){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS=shaders[i];
   hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&stages[i]));if(FAILED(hr))return hr;}
  root_=std::move(root);stages_=std::move(stages);return S_OK;
 }
 // Input is NON_PIXEL_SHADER_RESOURCE, outputs UAV, all held through completion.
 // Leaves frozen guide NON_PIXEL_SHADER_RESOURCE; cells remain UAV. Caller mutes
 // injected observers. All admission checks precede the first recorded command.
 bool Record(ID3D12GraphicsCommandList4* list,ID3D12Resource* training,UINT count,
             ID3D12Resource* cells,ID3D12Resource* frozen,UINT slots,UINT frame,UINT epoch,
             const ComputeBindingSnapshot& restore)const noexcept {
  if(!root_||!list||!restore.Ready()||!count||count>16*1024*1024||!slots||slots>65536||!frame||!epoch||
     training==cells||training==frozen||cells==frozen||
     !Buffer(training,UINT64(count)*sizeof(GITrainingRecord),false)||
     !Buffer(cells,UINT64(slots)*sizeof(GITrainingCell),true)||!Buffer(frozen,UINT64(slots)*sizeof(GIKeyedGuide),true)||
     (list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE))return false;
  list->SetComputeRootSignature(root_.Get());
  list->SetComputeRootShaderResourceView(0,training->GetGPUVirtualAddress());
  list->SetComputeRootUnorderedAccessView(1,cells->GetGPUVirtualAddress());
  list->SetComputeRootUnorderedAccessView(2,frozen->GetGPUVirtualAddress());
  const UINT control[]={count,slots,frame,epoch};list->SetComputeRoot32BitConstants(3,4,control,0);
  for(UINT i=0;i<4;++i){
   list->SetPipelineState(stages_[i].Get());
   const UINT groups=((i==0||i==3?slots:count)+63)/64;
   list->Dispatch(groups>65535?65535:groups,(groups+65534)/65535,1);
   if(i!=3)Barrier(list,cells);
   if(i==0)Barrier(list,frozen);
  }
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition={frozen,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
  list->ResourceBarrier(1,&b);return restore.Restore(list);
 }
};
}
