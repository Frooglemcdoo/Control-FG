#pragma once
#include "d3d12_auto_evaluator.h"
#include "d3d12_auto_target_build.h"
#include <atomic>
namespace gi_reuse {
class D3D12AutoPipelineBundle final:public IUnknown {
 std::atomic<ULONG> refs_{1};
public:
 D3D12AutoEvaluator evaluator;
 D3D12AutoTargetBuild builder;
 Microsoft::WRL::ComPtr<ID3D12RootSignature> nativeRoot;
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override {
  if(!out)return E_POINTER;*out=nullptr;if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;
  *out=static_cast<IUnknown*>(this);AddRef();return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs_;}
 ULONG STDMETHODCALLTYPE Release()override{auto n=--refs_;if(!n)delete this;return n;}
};
}
