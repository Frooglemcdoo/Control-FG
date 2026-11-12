#pragma once
#include "d3d12_compute_bindings.h"
#include "frozen_ray_constants.h"
#include <d3dcompiler.h>
namespace gi_reuse {
// Record at the ORIGINAL native ray site, while its CBV lifetimes/states apply.
// No CPU copy, source resource lookup, descriptor copy, source transition or wait.
// Caller mutes injected binding observers and retains this object until the LAST
// consumer fence. This does not own descriptor/geometry/AS contents for replay.
class D3D12GIRayConstantPipeline {
 Microsoft::WRL::ComPtr<ID3D12RootSignature> nativeRoot_,copyRoot_;
 Microsoft::WRL::ComPtr<ID3D12PipelineState> copyPipeline_;
public:
 HRESULT Initialize(ID3D12Device* device,ID3D12RootSignature* nativeRoot,
                    const void* nativeRootBytes,SIZE_T nativeRootSize,
                    D3D12_SHADER_BYTECODE shader) noexcept {
  if(copyPipeline_||!device||!nativeRoot||!nativeRootBytes||!nativeRootSize||
     !shader.pShaderBytecode||!shader.BytecodeLength)return E_INVALIDARG;
  Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> decoded;
  auto hr=D3D12CreateVersionedRootSignatureDeserializer(nativeRootBytes,nativeRootSize,IID_PPV_ARGS(&decoded));
  if(FAILED(hr))return hr;
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* description=nullptr;
  hr=decoded->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0,&description);if(FAILED(hr))return hr;
  if(!description||description->Desc_1_0.NumParameters<4)return E_INVALIDARG;
  for(UINT i=0;i<2;++i){const auto& p=description->Desc_1_0.pParameters[i+2];
   if(p.ParameterType!=D3D12_ROOT_PARAMETER_TYPE_CBV||p.Descriptor.ShaderRegister!=i||
      p.Descriptor.RegisterSpace!=0||p.ShaderVisibility!=D3D12_SHADER_VISIBILITY_ALL)return E_INVALIDARG;}
  D3D12_ROOT_PARAMETER parameters[3]{};
  for(UINT i=0;i<2;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i].Descriptor={i,0};}
  parameters[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
  D3D12_ROOT_SIGNATURE_DESC root{3,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
  Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
  hr=D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);if(FAILED(hr))return hr;
  hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&copyRoot_));if(FAILED(hr))return hr;
  D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};pso.pRootSignature=copyRoot_.Get();pso.CS=shader;
  hr=device->CreateComputePipelineState(&pso,IID_PPV_ARGS(&copyPipeline_));if(FAILED(hr))return hr;
  nativeRoot_=nativeRoot;return S_OK;
 }
 ID3D12RootSignature* NativeRoot()const noexcept{return nativeRoot_.Get();}
 ID3D12RootSignature* CopyRoot()const noexcept{return copyRoot_.Get();}
 ID3D12PipelineState* Pipeline()const noexcept{return copyPipeline_.Get();}
};
// One immutable per-invocation buffer. Initialize with a shared pipeline so
// rendering does not create a root signature and PSO for every frame.
class D3D12GIRayConstantSnapshot {
 Microsoft::WRL::ComPtr<ID3D12RootSignature> nativeRoot_,copyRoot_;
 Microsoft::WRL::ComPtr<ID3D12PipelineState> copyPipeline_;
 Microsoft::WRL::ComPtr<ID3D12Resource> data_;
 bool recorded_=false;
 bool dataCommon_=false;
public:
 static constexpr UINT Bytes=FrozenRayConstants::Bytes,GIConstantOffset=FrozenRayConstants::GIConstantOffset;
 D3D12GIRayConstantSnapshot()=default;
 D3D12GIRayConstantSnapshot(const D3D12GIRayConstantSnapshot&)=delete;
 D3D12GIRayConstantSnapshot& operator=(const D3D12GIRayConstantSnapshot&)=delete;
 HRESULT Initialize(ID3D12Device* device,const D3D12GIRayConstantPipeline& pipeline)noexcept {
  if(data_||!device||!pipeline.NativeRoot()||!pipeline.CopyRoot()||!pipeline.Pipeline())return E_INVALIDARG;
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=Bytes;
  buffer.Height=1;buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;
  buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;buffer.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  const auto hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&data_));
  if(SUCCEEDED(hr)){nativeRoot_=pipeline.NativeRoot();copyRoot_=pipeline.CopyRoot();copyPipeline_=pipeline.Pipeline();}return hr;
 }
 // Convenience for isolated tests; live owners use the shared-pipeline overload.
 HRESULT Initialize(ID3D12Device* device,ID3D12RootSignature* nativeRoot,
                    const void* nativeRootBytes,SIZE_T nativeRootSize,
                    D3D12_SHADER_BYTECODE shader)noexcept {
  if(data_)return E_INVALIDARG;D3D12GIRayConstantPipeline pipeline;
  const auto hr=pipeline.Initialize(device,nativeRoot,nativeRootBytes,nativeRootSize,shader);
  return FAILED(hr)?hr:Initialize(device,pipeline);
 }
 bool CanRecord(ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& native)const noexcept {
  if(!list||!data_||recorded_||!native.Ready()||native.root.Get()!=nativeRoot_.Get()||
     (list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE))return false;
  struct Extract {
   UINT64 source[2]{};
   void Constants(unsigned,unsigned,unsigned,const std::uint32_t*)noexcept{}
   void Descriptor(unsigned index,RootArgumentKind kind,std::uint64_t address)noexcept{
    if(index>=2&&index<=3&&kind==RootArgumentKind::CBV)source[index-2]=address;
   }
  } input;native.arguments.Replay(input);
  return input.source[0]&&input.source[1]&&input.source[0]%256==0&&input.source[1]%256==0;
 }
 bool RecordAndRestore(ID3D12GraphicsCommandList4* list,const ComputeBindingSnapshot& native) noexcept {
  if(!CanRecord(list,native))return false;
  struct Extract {
   UINT64 source[2]{};
   void Constants(unsigned,unsigned,unsigned,const std::uint32_t*)noexcept{}
   void Descriptor(unsigned index,RootArgumentKind kind,std::uint64_t address)noexcept{
    if(index>=2&&index<=3&&kind==RootArgumentKind::CBV)source[index-2]=address;
   }
  } input;native.arguments.Replay(input);
  if(dataCommon_){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   b.Transition={data_.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS};
   list->ResourceBarrier(1,&b);dataCommon_=false;}
  list->SetComputeRootSignature(copyRoot_.Get());list->SetPipelineState(copyPipeline_.Get());
  list->SetComputeRootConstantBufferView(0,input.source[0]);list->SetComputeRootConstantBufferView(1,input.source[1]);
  list->SetComputeRootUnorderedAccessView(2,data_->GetGPUVirtualAddress());list->Dispatch(6,1,1);
  D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition={data_.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER};
  list->ResourceBarrier(1,&barrier);recorded_=true;
  return native.Restore(list);
 }
 bool ApplyTo(ComputeBindingSnapshot& ray)const noexcept {
  if(!recorded_||!ray.Ready()||ray.root.Get()!=nativeRoot_.Get())return false;
  auto updated=ray;
  updated.arguments.Descriptor(2,RootArgumentKind::CBV,data_->GetGPUVirtualAddress());
  updated.arguments.Descriptor(3,RootArgumentKind::CBV,data_->GetGPUVirtualAddress()+GIConstantOffset);
  if(!updated.Ready())return false;ray=std::move(updated);return true;
 }
 ID3D12Resource* Resource()const noexcept{return recorded_?data_.Get():nullptr;}
 // Host calls only after whole-invocation Take, never on producer Reset alone.
 bool RearmAfterRetirement()noexcept {
  if(!recorded_)return false;
  recorded_=false;dataCommon_=true;return true;
 }
};
}
