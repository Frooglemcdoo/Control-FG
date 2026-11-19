// Included in native_lighting after compute observers and native root metadata.
namespace gi_auto_prepare {
#ifdef CONTROL_GI_AUTO_TARGETS
inline const GUID key={0x448a0011,0x7519,0x4451,{0xa1,0x55,0x71,0x60,0x44,0x80,0x00,0x01}};
inline std::mutex mutex;
inline Microsoft::WRL::ComPtr<gi_reuse::D3D12AutoPipelineBundle> latest;
inline Microsoft::WRL::ComPtr<gi_reuse::D3D12AutoPipelineBundle> ForRoot(ID3D12RootSignature* root)noexcept {
 std::lock_guard<std::mutex> lock(mutex);return latest&&latest->nativeRoot.Get()==root?latest:Microsoft::WRL::ComPtr<gi_reuse::D3D12AutoPipelineBundle>{};
}
inline bool RootReady(ID3D12RootSignature* root)noexcept{return bool(ForRoot(root));}
inline Microsoft::WRL::ComPtr<gi_reuse::D3D12AutoPipelineBundle> ForPSO(ID3D12PipelineState* pso)noexcept {
 Microsoft::WRL::ComPtr<IUnknown> raw;UINT n=sizeof(IUnknown*);
 if(!pso||FAILED(pso->GetPrivateData(key,&n,raw.GetAddressOf()))||n!=sizeof(IUnknown*)||!raw)return {};
 Microsoft::WRL::ComPtr<gi_reuse::D3D12AutoPipelineBundle> result;result.Attach(static_cast<gi_reuse::D3D12AutoPipelineBundle*>(raw.Detach()));return result;
}
#endif
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native)noexcept {
#ifdef CONTROL_GI_AUTO_TARGETS
 if(!gi_auto_dispatch::Enabled()||!device||!desc||!desc->pRootSignature||!native)return;
 try {
  unsigned char bytes[16384]{};UINT n=sizeof(bytes);
  auto hr=desc->pRootSignature->GetPrivateData(rt_spot_capture_audit::kRootBytes,&n,bytes);if(FAILED(hr)||n>sizeof(bytes)||!n){static std::atomic<bool> reported{false};if(!reported.exchange(true))Log("GI_AUTO_R476 pipeline_ready=0 reason=root_metadata hr=0x%08lX",static_cast<unsigned long>(hr));return;}
  Microsoft::WRL::ComPtr<gi_reuse::D3D12AutoPipelineBundle> bundle;bundle.Attach(new(std::nothrow) gi_reuse::D3D12AutoPipelineBundle);if(!bundle)return;
  hr=bundle->evaluator.Initialize(device,desc->pRootSignature,bytes,n,{gi_auto_shader::kLighting,sizeof(gi_auto_shader::kLighting)});
#ifdef CONTROL_GI_REGIONAL
  const std::array<D3D12_SHADER_BYTECODE,5> code={{{gi_auto_shader::kClear,sizeof(gi_auto_shader::kClear)},{gi_auto_shader::kClaim,sizeof(gi_auto_shader::kClaim)},{gi_auto_shader::kFirst,sizeof(gi_auto_shader::kFirst)},{gi_auto_shader::kSecond,sizeof(gi_auto_shader::kSecond)},{gi_auto_shader::kFreeze,sizeof(gi_auto_shader::kFreeze)}}};
#else
  const std::array<D3D12_SHADER_BYTECODE,4> code={{{gi_auto_shader::kClear,sizeof(gi_auto_shader::kClear)},{gi_auto_shader::kClaim,sizeof(gi_auto_shader::kClaim)},{gi_auto_shader::kAccumulate,sizeof(gi_auto_shader::kAccumulate)},{gi_auto_shader::kFreeze,sizeof(gi_auto_shader::kFreeze)}}};
#endif
  if(SUCCEEDED(hr))hr=bundle->builder.Initialize(device,code);
  if(SUCCEEDED(hr)){bundle->nativeRoot=desc->pRootSignature;Microsoft::WRL::ComPtr<ID3D12Object> object;hr=native->QueryInterface(IID_PPV_ARGS(&object));
   if(SUCCEEDED(hr))hr=object->SetPrivateDataInterface(key,bundle.Get());}
  if(SUCCEEDED(hr)){std::lock_guard<std::mutex> lock(mutex);latest=bundle;gi_auto_dispatch::rootReady=&RootReady;}
  Log("GI_AUTO_R476 pipeline_ready=%u hr=0x%08lX",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr));
 }catch(...){Log("GI_AUTO_R476 pipeline_ready=0 reason=exception");}
#else
 (void)device;(void)desc;(void)native;
#endif
}
inline void Share(IUnknown* native,ID3D12PipelineState* alternate)noexcept {
#ifdef CONTROL_GI_AUTO_TARGETS
 Microsoft::WRL::ComPtr<ID3D12PipelineState> source;
 if(!native||!alternate||FAILED(native->QueryInterface(IID_PPV_ARGS(&source))))return;
 const auto bundle=ForPSO(source.Get());if(bundle)alternate->SetPrivateDataInterface(key,bundle.Get());
#else
 (void)native;(void)alternate;
#endif
}
}
