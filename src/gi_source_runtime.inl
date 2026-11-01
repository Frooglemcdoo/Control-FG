// Included within native_lighting after Prepare and CreateCompute declarations.
namespace gi_source_probe {
inline const GUID variantGuid={0x93768267,0x2187,0x4db3,{0x81,0xf0,0xe1,0xb6,0xc2,0xa3,0x61,0x17}};
inline std::atomic<unsigned long long> bound{0};
inline unsigned Requested() noexcept {
 static const unsigned mode=[] {
  char value[32]{};
  const DWORD length=GetEnvironmentVariableA("CONTROLTOH_GI_SOURCE",value,sizeof(value));
  const unsigned result=!length?4:(length<sizeof(value)?gi_source::Parse(value):0);
  Trace("GI_SOURCE_R269 request=%s restart_scoped=1 diagnostic_only=1",gi_source::Name(result));
  return result;
 }();
 return mode;
}
inline void Observe(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,
 IUnknown* native,CreateCompute create) noexcept {
 const unsigned mode=Requested();
 if(!mode||!device||!desc||!native||!create)return;
 for(unsigned variant=0;variant<8;++variant){
  const auto& patch=*kGISourcePatches[variant][mode-1];
  std::vector<unsigned char> bytes;
  if(!Prepare(patch,desc->CS,bytes))continue;
  auto replacement=*desc;replacement.CS={bytes.data(),bytes.size()};replacement.CachedPSO={};
  ID3D12PipelineState* alternate=nullptr;
  HRESULT hr=create(device,&replacement,IID_PPV_ARGS(&alternate));
  ID3D12PipelineState* owner=nullptr;
  if(SUCCEEDED(hr)&&alternate){
   hr=native->QueryInterface(IID_PPV_ARGS(&owner));
   if(SUCCEEDED(hr)&&owner)hr=owner->SetPrivateDataInterface(variantGuid,alternate);
  }else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
  if(owner)owner->Release();
  if(alternate)alternate->Release();
  Trace("GI_SOURCE_R269 prepare=%s variant=%u ready=%u hr=0x%08lX native_sha=%s patched_sha=%s",
   gi_source::Name(mode),variant,unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr),patch.nativeSha,patch.patchedSha);
  return;
 }
}
// Returned interface is AddRef'ed by GetPrivateData and released by the caller.
inline ID3D12PipelineState* Select(ID3D12PipelineState* native) noexcept {
 if(!Requested()||!native)return nullptr;
 ID3D12PipelineState* result=nullptr;UINT size=sizeof(result);
 if(FAILED(native->GetPrivateData(variantGuid,&size,&result)))return nullptr;
 if(size!=sizeof(result)){if(result)result->Release();return nullptr;}
 if(result){
  const auto count=++bound;
  if(count<=4||(count%240)==0)Trace("GI_SOURCE_R269 bound=%s count=%llu cpu_recording_only=1",gi_source::Name(Requested()),count);
 }
 return result;
}
}
