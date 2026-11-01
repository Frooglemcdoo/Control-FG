// Included within native_lighting. Diagnostic R268, full light energy retained.
namespace gi_spot_pcf_probe {
inline const GUID variantGuid={0x93768268,0x2187,0x4db3,{0x81,0xf0,0xe1,0xb6,0xc2,0xa3,0x61,0x17}};
inline std::atomic<unsigned long long> bound{0};
inline bool Enabled() noexcept {
 static const bool enabled=[] {
  char value[32]{};const DWORD length=GetEnvironmentVariableA("CONTROLTOH_GI_SHADOW",value,sizeof(value));
  const bool active=length&&length<sizeof(value)&&!std::strcmp(value,"pcf");
  Trace("GI_SPOT_PCF_R268 requested=%u native_comparison=%u diagnostic_only=1",unsigned(active),unsigned(!active));
  return active;
 }();
 return enabled;
}
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,
 IUnknown* native,CreateCompute create) noexcept {
 if(!Enabled()||gi_source_probe::Requested()||!device||!desc||!desc->CS.pShaderBytecode||!native||!create)return;
 for(unsigned i=0;i<8;++i){
  const auto& shader=gi_spot_pcf::shaders[i];
  if(desc->CS.BytecodeLength!=shader.nativeSize)continue;
  char sha[65]{};
  if(!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode),desc->CS.BytecodeLength,sha)||std::strcmp(sha,shader.nativeSha))continue;
  auto replacement=*desc;replacement.CS={shader.bytes,shader.size};replacement.CachedPSO={};
  ID3D12PipelineState* alternate=nullptr;ID3D12PipelineState* owner=nullptr;
  HRESULT hr=create(device,&replacement,IID_PPV_ARGS(&alternate));
  if(SUCCEEDED(hr)&&alternate){
   hr=native->QueryInterface(IID_PPV_ARGS(&owner));
   if(SUCCEEDED(hr)&&owner)hr=owner->SetPrivateDataInterface(variantGuid,alternate);
  }else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
  if(owner)owner->Release();if(alternate)alternate->Release();
  Trace("GI_SPOT_PCF_R268 variant=%u ready=%u hr=0x%08lX native_sha=%s",i,unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr),shader.nativeSha);
  return;
 }
}
inline ID3D12PipelineState* Select(ID3D12PipelineState* native) noexcept {
 if(!Enabled()||gi_source_probe::Requested()||!native)return nullptr;
 ID3D12PipelineState* result=nullptr;UINT size=sizeof(result);
 if(FAILED(native->GetPrivateData(variantGuid,&size,&result)))return nullptr;
 if(size!=sizeof(result)){if(result)result->Release();return nullptr;}
 if(result){const auto count=++bound;if(count<=4||(count%240)==0)Trace("GI_SPOT_PCF_R268 bound=%llu full_spot_energy=1 cpu_recording_only=1",count);}
 return result;
}
}
