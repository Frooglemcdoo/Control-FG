#pragma once
#include "d3d12_frozen_evaluator.h"
#include "d3d12_frozen_guide_build.h"
#include "d3d12_ray_constant_snapshot.h"
#include <atomic>
namespace gi_reuse {
// Attached to the native GI6 PSO. Owns only immutable pipelines/root signatures,
// so there is no cycle back to that PSO and no per-frame PSO creation.
class D3D12FrozenPipelineBundle final:public IUnknown {
 std::atomic<ULONG> references_{1};
public:
 D3D12FrozenEvaluator evaluator;
 D3D12FrozenGuideBuild guide;
 D3D12GIRayConstantPipeline constants;
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** output)override {
  if(!output)return E_POINTER;*output=nullptr;
  if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;
  *output=static_cast<IUnknown*>(this);AddRef();return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef()override{return ++references_;}
 ULONG STDMETHODCALLTYPE Release()override{const auto n=--references_;if(!n)delete this;return n;}
 HRESULT Initialize(ID3D12Device* device,ID3D12RootSignature* root,const void* bytes,SIZE_T size,
                    D3D12_SHADER_BYTECODE evaluation,D3D12_SHADER_BYTECODE copy,
                    const std::array<D3D12_SHADER_BYTECODE,4>& build) {
  auto hr=evaluator.Initialize(device,root,bytes,size,evaluation);if(FAILED(hr))return hr;
  hr=constants.Initialize(device,root,bytes,size,copy);if(FAILED(hr))return hr;
  return guide.Initialize(device,build);
 }
};
}
