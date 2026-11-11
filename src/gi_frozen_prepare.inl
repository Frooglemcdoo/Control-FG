// Included inside native_lighting, after the native root-byte private-data key.
namespace gi_frozen_prepare {
inline const GUID kBundle={0x426f0011,0x7519,0x4451,{0xa1,0x55,0x71,0x60,0x42,0x60,0x00,0x01}};
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native)noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 char enabled[4]{};
 if(GetEnvironmentVariableA("CONTROLTOH_GI_FROZEN_PREPARE",enabled,4)!=1||enabled[0]!='1'||
    !device||!desc||!desc->pRootSignature||!native)return;
 gi_compute_hooks::EnableForDenoiser();
 // Called only after gi_evaluator's exact size+SHA validation of native GI6.
 try {
  unsigned char bytes[16384]{};UINT size=sizeof(bytes);
  HRESULT hr=desc->pRootSignature->GetPrivateData(rt_spot_capture_audit::kRootBytes,&size,bytes);
  if(FAILED(hr)||!size||size>sizeof(bytes)){
   Log("GI_FROZEN_R426 compute_prepared=0 stage=root_bytes hr=0x%08lX deferred_dispatch_enabled=0",static_cast<unsigned long>(hr));return;}
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
  Log("GI_FROZEN_R426 compute_prepared=%u hr=0x%08lX native_root_bytes=%u stages=6 deferred_dispatch_enabled=0",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr),size);
 }catch(...){Log("GI_FROZEN_R426 compute_prepared=0 stage=exception deferred_dispatch_enabled=0");}
#else
 (void)device;(void)desc;(void)native;
#endif
}
inline void Share(IUnknown* native,ID3D12PipelineState* alternate)noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 Microsoft::WRL::ComPtr<ID3D12PipelineState> source;
 if(!native||!alternate||FAILED(native->QueryInterface(IID_PPV_ARGS(&source))))return;
 Microsoft::WRL::ComPtr<IUnknown> bundle;UINT size=sizeof(IUnknown*);
 if(SUCCEEDED(source->GetPrivateData(kBundle,&size,bundle.GetAddressOf()))&&bundle)
  alternate->SetPrivateDataInterface(kBundle,bundle.Get());
#else
 (void)native;(void)alternate;
#endif
}
inline void CheckBinding(ID3D12GraphicsCommandList* list)noexcept {
#ifdef CONTROL_GI_LONG_SEQUENCE
 static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_FROZEN_PREPARE",v,4)==1&&v[0]=='1';}();
 static std::atomic<unsigned> attempts{0};static std::atomic<bool> done{false};
 if(!enabled||done.load()||attempts.load()>=64||!list)return;
 UINT mode=0,size=sizeof(mode);
 if(FAILED(list->GetPrivateData(gi_evaluator::kMode,&size,&mode))||!mode)return;
 const auto attempt=++attempts;
 gi_reuse::ComputeBindingSnapshot bindings;UINT64 generation=0;
 const bool captured=gi_compute_hooks::Capture(list,bindings,generation);
 Microsoft::WRL::ComPtr<IUnknown> owner;size=sizeof(IUnknown*);
 const bool attached=captured&&bindings.pipeline&&SUCCEEDED(bindings.pipeline->GetPrivateData(kBundle,&size,owner.GetAddressOf()))&&owner;
 const bool accepted=attached&&static_cast<gi_reuse::D3D12FrozenPipelineBundle*>(owner.Get())->evaluator.Accept(bindings);
 if(accepted)done=true;
 if(accepted||attempt==1||attempt==64)Log("GI_FROZEN_R426 binding_ready=%u captured=%u attached=%u attempt=%u generation=%llu deferred_dispatch_enabled=0",
  unsigned(accepted),unsigned(captured),unsigned(attached),attempt,generation);
#else
 (void)list;
#endif
}

}
