// Included inside native_lighting after rt_spot_capture_audit.
namespace gi_binding_audit {
inline const GUID identityGuid={0x7273bd31,0xa358,0x4c72,{0x94,0x31,0x23,0x55,0x67,0x18,0x72,0x73}};
inline std::atomic<unsigned> reported{0};
inline std::atomic<unsigned long long> dispatches[8]{};
inline thread_local ID3D12GraphicsCommandList* boundList=nullptr;
inline thread_local unsigned boundVariant=0;
inline bool Enabled() noexcept {
 static const bool enabled=[] {char value[4]{};const bool active=GetEnvironmentVariableA("CONTROLTOH_GI_BIND_AUDIT",value,sizeof(value))==1&&value[0]=='1';if(active)Trace("GI_BIND_R273_ENABLED read_only=1 image_capture=0");return active;}();
 return enabled;
}
inline void Observe(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native) noexcept {
 if(!Enabled()||!desc||!native||!desc->CS.pShaderBytecode)return;
 bool candidate=false;
 for(unsigned i=0;i<8;++i)candidate=candidate||desc->CS.BytecodeLength==kGISourcePatches[i][0]->size;
 if(!candidate)return;
 char hash[65]{};
 if(!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode),desc->CS.BytecodeLength,hash))return;
 unsigned variant=8;for(unsigned i=0;i<8;++i)if(!strcmp(hash,kGISourcePatches[i][0]->nativeSha)){variant=i;break;}
 if(variant==8)return;
 Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
 if(FAILED(native->QueryInterface(IID_PPV_ARGS(&pso))))return;
 const unsigned tag=variant+1;
 if(FAILED(pso->SetPrivateData(identityGuid,sizeof(tag),&tag)))return;
 if(reported.fetch_or(1u<<variant)&(1u<<variant))return;
 unsigned char bytes[8192]{};UINT size=sizeof(bytes);
 if(!desc->pRootSignature||FAILED(desc->pRootSignature->GetPrivateData(rt_spot_capture_audit::kRootBytes,&size,bytes))){
  Trace("GI_BIND_R273_ROOT variant=%u captured=0 shader=%s",variant,hash);return;
 }
 using Deserialize=HRESULT(WINAPI*)(LPCVOID,SIZE_T,REFIID,void**);
 const HMODULE module=GetModuleHandleW(L"d3d12.dll");
 const auto deserialize=module?reinterpret_cast<Deserialize>(GetProcAddress(module,"D3D12CreateVersionedRootSignatureDeserializer")):nullptr;
 Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> decoder;
 if(!deserialize||FAILED(deserialize(bytes,size,IID_PPV_ARGS(&decoder)))){Trace("GI_BIND_R273_ROOT variant=%u decode_failed=1",variant);return;}
 const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* version=nullptr;
 if(FAILED(decoder->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0,&version))||!version)return;
 const auto& root=version->Desc_1_0;
 char rootHash[65]{};if(!PTReflectionSha256(bytes,size,rootHash))return;
 Trace("GI_BIND_R273_ROOT variant=%u captured=1 shader=%s root_sha=%s bytes=%u parameters=%u samplers=%u flags=%u",variant,hash,rootHash,size,root.NumParameters,root.NumStaticSamplers,unsigned(root.Flags));
 if(root.NumParameters>32){Trace("GI_BIND_R273_REJECT variant=%u reason=parameter_bound",variant);return;}
 for(UINT i=0;i<root.NumParameters;++i){const auto& parameter=root.pParameters[i];
  if(parameter.ParameterType==D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE){
   if(parameter.DescriptorTable.NumDescriptorRanges>32){Trace("GI_BIND_R273_REJECT variant=%u reason=range_bound",variant);return;}
   for(UINT j=0;j<parameter.DescriptorTable.NumDescriptorRanges;++j){const auto& range=parameter.DescriptorTable.pDescriptorRanges[j];
    Trace("GI_BIND_R273_RANGE variant=%u parameter=%u range=%u visibility=%u type=%u count=%u reg=%u space=%u offset=%u",variant,i,j,unsigned(parameter.ShaderVisibility),unsigned(range.RangeType),range.NumDescriptors,range.BaseShaderRegister,range.RegisterSpace,range.OffsetInDescriptorsFromTableStart);
   }
  }else if(parameter.ParameterType==D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS){
   Trace("GI_BIND_R273_CONSTANTS variant=%u parameter=%u visibility=%u count=%u reg=%u space=%u",variant,i,unsigned(parameter.ShaderVisibility),parameter.Constants.Num32BitValues,parameter.Constants.ShaderRegister,parameter.Constants.RegisterSpace);
  }else{
   Trace("GI_BIND_R273_DESCRIPTOR variant=%u parameter=%u visibility=%u type=%u reg=%u space=%u",variant,i,unsigned(parameter.ShaderVisibility),unsigned(parameter.ParameterType),parameter.Descriptor.ShaderRegister,parameter.Descriptor.RegisterSpace);
  }
 }
 Trace("GI_BIND_R273_ROOT_COMPLETE variant=%u native_outputs_unchanged=1",variant);
}
inline void Bind(ID3D12GraphicsCommandList* list,ID3D12PipelineState* rendered) noexcept {
 if(!Enabled())return;
 unsigned tag=0;UINT size=sizeof(tag);
 boundList=nullptr;boundVariant=0;
 if(rendered&&SUCCEEDED(rendered->GetPrivateData(identityGuid,&size,&tag))&&size==sizeof(tag)&&tag>=1&&tag<=8){boundList=list;boundVariant=tag;}
}
inline void Dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z) noexcept {
 if(!Enabled()||boundList!=list||!boundVariant)return;
 const unsigned variant=boundVariant-1;const auto count=++dispatches[variant];
 if(count<=4||count%600==0)Trace("GI_BIND_R273_DISPATCH variant=%u count=%llu groups=%u,%u,%u list=%p present=%llu",variant,count,x,y,z,static_cast<void*>(list),presentCount.load());
}
}
