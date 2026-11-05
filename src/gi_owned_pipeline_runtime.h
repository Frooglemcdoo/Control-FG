#pragma once
#include "gi_reuse/d3d12_initial_gi_pipeline.h"
#include <atomic>
#include "gi_reuse/owned_request.h"
#include <new>
#include "gi_reuse/attempt_throttle.h"
#ifdef CONTROL_GI_LONG_SEQUENCE
#include "../build/gi_owned.generated.h"
#endif
// The hash-matched GI6 PSO owns this immutable pipeline. No global PSO/device
// retention and no native-PSO back-reference: releasing the last PSO releases it.
namespace GIOwnedPipeline {
inline const GUID key={0x310e0004,0x1739,0x4401,{0x91,0x33,0x41,0x51,0x61,0x71,0x81,0x91}};
inline bool Enabled() noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 static const bool enabled=[] {char value[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_OWNED",value,4)==1&&value[0]=='1';}();return enabled;
#else
 return false;
#endif
}
using DispatchObserver=void(*)(ID3D12GraphicsCommandList4*,const gi_reuse::ComputeBindingSnapshot&,UINT,UINT,UINT) noexcept;
inline std::atomic<DispatchObserver> dispatchObserver{nullptr};
inline std::atomic<UINT64> recorded{0},admitted{0},rejected{0};
inline auto& requested=gi_reuse::ownedRequested;
inline std::atomic<bool> active{false};
inline gi_reuse::AttemptThrottle attempts;
inline thread_local bool recording=false;
class Owner final:public IUnknown {
 std::atomic<ULONG> refs{1};
public:
 gi_reuse::D3D12InitialGIPipeline pipeline;
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override {if(!out)return E_POINTER;*out=nullptr;
  if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;*out=static_cast<IUnknown*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n)delete this;return n;}
};
inline Microsoft::WRL::ComPtr<Owner> Read(ID3D12Object* object) noexcept {
 Microsoft::WRL::ComPtr<Owner> result;if(!object)return result;
 IUnknown* value=nullptr;UINT bytes=sizeof(value);
 if(SUCCEEDED(object->GetPrivateData(key,&bytes,&value))&&value){if(bytes==sizeof(value))result.Attach(static_cast<Owner*>(value));else value->Release();}return result;
}
inline HRESULT Attach(ID3D12Device* device,ID3D12RootSignature* root,IUnknown* native) noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 if(!Enabled())return S_FALSE;if(!device||!root||!native)return E_INVALIDARG;
 Microsoft::WRL::ComPtr<ID3D12Device5> dxr;Microsoft::WRL::ComPtr<ID3D12PipelineState> target;
 auto hr=device->QueryInterface(IID_PPV_ARGS(&dxr));if(FAILED(hr))return hr;
 hr=native->QueryInterface(IID_PPV_ARGS(&target));if(FAILED(hr))return hr;
 if(Read(target.Get()))return S_FALSE;
 Microsoft::WRL::ComPtr<Owner> owner;owner.Attach(new(std::nothrow) Owner);if(!owner)return E_OUTOFMEMORY;
 hr=owner->pipeline.Initialize(dxr.Get(),root,{kGIOwnedProposals,sizeof(kGIOwnedProposals)},
  {kGIOwnedTrace,sizeof(kGIOwnedTrace)},{kGIOwnedShading,sizeof(kGIOwnedShading)},
  {kGIOwnedResolve,sizeof(kGIOwnedResolve)},true);
 if(SUCCEEDED(hr))hr=target->SetPrivateDataInterface(key,owner.Get());return hr;
#else
 (void)device;(void)root;(void)native;return S_FALSE;
#endif
}
inline HRESULT Share(IUnknown* native,ID3D12PipelineState* alternate) noexcept {
 Microsoft::WRL::ComPtr<ID3D12PipelineState> source;
 if(!native||!alternate||FAILED(native->QueryInterface(IID_PPV_ARGS(&source))))return E_INVALIDARG;
 auto owner=Read(source.Get());return owner?alternate->SetPrivateDataInterface(key,owner.Get()):S_FALSE;
}
}
