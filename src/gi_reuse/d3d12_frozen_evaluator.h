#pragma once
#include "d3d12_compute_bindings.h"
#include <d3dcompiler.h>
#include <vector>
namespace gi_reuse {
// Immutable pipeline shared by all in-flight frames. Preparation is outside
// recording. This owns shaders/root only, not native descriptors/resources.
class D3D12FrozenEvaluator {
 Microsoft::WRL::ComPtr<ID3D12RootSignature> native_,root_;
 Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
 UINT first_=0;
 std::vector<D3D12_ROOT_PARAMETER_TYPE> kinds_;
 std::vector<UINT> constants_;
public:
 HRESULT Initialize(ID3D12Device* device,ID3D12RootSignature* native,
                    const void* bytes,SIZE_T size,D3D12_SHADER_BYTECODE shader) {
  if(root_||!device||!native||!bytes||!size||!shader.pShaderBytecode||!shader.BytecodeLength)return E_INVALIDARG;
  Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> decoded;
  auto hr=D3D12CreateVersionedRootSignatureDeserializer(bytes,size,IID_PPV_ARGS(&decoded));if(FAILED(hr))return hr;
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc=nullptr;
  hr=decoded->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0,&desc);if(FAILED(hr))return hr;
  if(!desc||!desc->Desc_1_0.NumParameters||desc->Desc_1_0.NumParameters>60)return E_INVALIDARG;
  auto rd=desc->Desc_1_0;
  if(rd.Flags&D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)return E_INVALIDARG;
  // Reject collisions with the appended private register space, including
  // descriptor ranges and static samplers. No silent register remapping.
  UINT cost=14;
  std::vector<D3D12_ROOT_PARAMETER> parameters(rd.pParameters,rd.pParameters+rd.NumParameters);
  std::vector<D3D12_ROOT_PARAMETER_TYPE> kinds;std::vector<UINT> constants;
  for(const auto& p:parameters){
   kinds.push_back(p.ParameterType);constants.push_back(0);
   if(p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE){
    ++cost;for(UINT i=0;i<p.DescriptorTable.NumDescriptorRanges;++i)
     if(p.DescriptorTable.pDescriptorRanges[i].RegisterSpace==18)return E_INVALIDARG;
   }else if(p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS){
    if(p.Constants.RegisterSpace==18||p.Constants.Num32BitValues>64)return E_INVALIDARG;
    cost+=p.Constants.Num32BitValues;constants.back()=p.Constants.Num32BitValues;
   }else{
    if(p.Descriptor.RegisterSpace==18)return E_INVALIDARG;cost+=2;
   }
  }
  if(cost>64)return E_INVALIDARG;
  for(UINT i=0;i<rd.NumStaticSamplers;++i)if(rd.pStaticSamplers[i].RegisterSpace==18)return E_INVALIDARG;
  const UINT first=rd.NumParameters;
  parameters.resize(first+4);
  parameters[first].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[first].Constants={0,18,8};
  parameters[first+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[first+1].Descriptor={1,18};
  parameters[first+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[first+2].Descriptor={0,18};
  parameters[first+3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;parameters[first+3].Descriptor={0,18};
  rd.NumParameters=first+4;rd.pParameters=parameters.data();
  Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
  hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);if(FAILED(hr))return hr;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> root;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
  hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root));if(FAILED(hr))return hr;
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS=shader;
  hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline));if(FAILED(hr))return hr;
  native_=native;root_=std::move(root);pipeline_=std::move(pipeline);first_=first;
  kinds_=std::move(kinds);constants_=std::move(constants);return S_OK;
 }
 // Require all non-constant native parameters, plus every declared constant.
 // A generally healthy tracker is not evidence that every required slot exists.
 bool Accept(const ComputeBindingSnapshot& bindings)const noexcept {
  if(!pipeline_||!bindings.Ready()||bindings.root.Get()!=native_.Get())return false;
  struct Verify {
   const D3D12FrozenEvaluator& self;UINT64 found=0;std::array<UINT64,64> masks{};bool good=true;
   void Constants(unsigned index,unsigned offset,unsigned count,const std::uint32_t*)noexcept{
    if(index>=self.first_||self.kinds_[index]!=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS||
       offset>self.constants_[index]||count>self.constants_[index]-offset){good=false;return;}
    for(UINT i=0;i<count;++i)masks[index]|=UINT64(1)<<(offset+i);found|=UINT64(1)<<index;
   }
   void Descriptor(unsigned index,RootArgumentKind kind,std::uint64_t address)noexcept{
    if(index>=self.first_||!address){good=false;return;}
    auto expected=RootArgumentKind::None;
    switch(self.kinds_[index]){
     case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:expected=RootArgumentKind::Table;break;
     case D3D12_ROOT_PARAMETER_TYPE_CBV:expected=RootArgumentKind::CBV;break;
     case D3D12_ROOT_PARAMETER_TYPE_SRV:expected=RootArgumentKind::SRV;break;
     case D3D12_ROOT_PARAMETER_TYPE_UAV:expected=RootArgumentKind::UAV;break;
     default:break;
    }
    if(kind!=expected||(kind==RootArgumentKind::CBV&&address%256))good=false;
    found|=UINT64(1)<<index;
   }
  } verify{*this};
  bindings.arguments.Replay(verify);
  if(!verify.good||verify.found!=((UINT64(1)<<first_)-1))return false;
  for(UINT i=0;i<first_;++i)if(constants_[i]&&verify.masks[i]!=(constants_[i]==64?~UINT64(0):(UINT64(1)<<constants_[i])-1))return false;
  return true;
 }
 // Caller holds native heap/resource leases, sets their correct states, and
 // mutes hook observers. No allocations, heap switch, readback or wait here.
 bool Record(ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& bindings,
             UINT width,UINT height,UINT frame,UINT epoch,UINT parity,UINT phase,
             D3D12_GPU_VIRTUAL_ADDRESS rayConstants,D3D12_GPU_VIRTUAL_ADDRESS training,
             D3D12_GPU_VIRTUAL_ADDRESS weights,bool weighted)const noexcept {
  if(!list||!Accept(bindings)||width<2||(width&1)||!height||!frame||!epoch||parity>1||phase>1||
     width>16384||height>16384||!rayConstants||rayConstants%256||!training||training%4||!weights||weights%4||
     (list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE))return false;
  struct Replay {
   ID3D12GraphicsCommandList4* list;
   void Constants(unsigned index,unsigned offset,unsigned count,const std::uint32_t* values)noexcept{list->SetComputeRoot32BitConstants(index,count,values,offset);}
   void Descriptor(unsigned index,RootArgumentKind kind,std::uint64_t address)noexcept{
    switch(kind){
     case RootArgumentKind::Table:list->SetComputeRootDescriptorTable(index,{address});break;
     case RootArgumentKind::CBV:list->SetComputeRootConstantBufferView(index,address);break;
     case RootArgumentKind::SRV:list->SetComputeRootShaderResourceView(index,address);break;
     case RootArgumentKind::UAV:list->SetComputeRootUnorderedAccessView(index,address);break;
     default:break;
    }
   }
  } replay{list};
  list->SetComputeRootSignature(root_.Get());bindings.arguments.Replay(replay);list->SetPipelineState(pipeline_.Get());
  const UINT control[]={width,height,frame,epoch,parity,phase,weighted?1u:0u,0};
  list->SetComputeRoot32BitConstants(first_,8,control,0);
  list->SetComputeRootConstantBufferView(first_+1,rayConstants);
  list->SetComputeRootUnorderedAccessView(first_+2,training);
  list->SetComputeRootShaderResourceView(first_+3,weights);
  list->Dispatch((width/2+7)/8,(height+15)/16,1);
  return bindings.Restore(list);
 }
};
}
