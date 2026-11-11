// Included inside native_lighting, after the native root-byte private-data key.
namespace gi_frozen_prepare {
inline const GUID kBundle={0x426f0011,0x7519,0x4451,{0xa1,0x55,0x71,0x60,0x42,0x60,0x00,0x01}};
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native)noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 char enabled[4]{};
 if(GetEnvironmentVariableA("CONTROLTOH_GI_FROZEN_PREPARE",enabled,4)!=1||enabled[0]!='1'||
    !device||!desc||!desc->pRootSignature||!native)return;
 // Called only after gi_evaluator's exact size+SHA validation of native GI6.
 try {
  unsigned char bytes[16384]{};UINT size=sizeof(bytes);
  HRESULT hr=desc->pRootSignature->GetPrivateData(rt_spot_capture_audit::kRootBytes,&size,bytes);
  if(FAILED(hr)||!size||size>sizeof(bytes)){
   Trace("GI_FROZEN_R426 compute_prepared=0 stage=root_bytes hr=0x%08lX deferred_dispatch_enabled=0",static_cast<unsigned long>(hr));return;}
  Microsoft::WRL::ComPtr<gi_reuse::D3D12FrozenPipelineBundle> bundle;
  bundle.Attach(new(std::nothrow) gi_reuse::D3D12FrozenPipelineBundle);
  if(!bundle)return;
  const std::array<D3D12_SHADER_BYTECODE,4> stages={{{gi_frozen_shader::kClear,sizeof(gi_frozen_shader::kClear)},
   {gi_frozen_shader::kClaim,sizeof(gi_frozen_shader::kClaim)},{gi_frozen_shader::kAccumulate,sizeof(gi_frozen_shader::kAccumulate)},
   {gi_frozen_shader::kFreeze,sizeof(gi_frozen_shader::kFreeze)}}};
  hr=bundle->Initialize(device,desc->pRootSignature,bytes,size,
   {gi_frozen_shader::kEvaluator,sizeof(gi_frozen_shader::kEvaluator)},
   {gi_frozen_shader::kConstants,sizeof(gi_frozen_shader::kConstants)},stages);
  Microsoft::WRL::ComPtr<ID3D12PipelineState> owner;
  if(SUCCEEDED(hr))hr=native->QueryInterface(IID_PPV_ARGS(&owner));
  if(SUCCEEDED(hr))hr=owner->SetPrivateDataInterface(kBundle,bundle.Get());
  Trace("GI_FROZEN_R426 compute_prepared=%u hr=0x%08lX native_root_bytes=%u stages=6 deferred_dispatch_enabled=0",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr),size);
 }catch(...){Trace("GI_FROZEN_R426 compute_prepared=0 stage=exception deferred_dispatch_enabled=0");}
#else
 (void)device;(void)desc;(void)native;
#endif
}
}
