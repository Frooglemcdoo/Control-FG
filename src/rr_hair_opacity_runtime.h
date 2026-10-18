#pragma once
// Only the audited hair albedo record is cloned. Native records are read-only.
struct RRHairOpacityRecord {
 std::uintptr_t original=0,selected=0;
 alignas(16) unsigned char record[0xA8]{};
 alignas(16) unsigned char descriptor[0x100]{};
};
static RRHairOpacityRecord* rrHairOpacityRecords[32]{};
static unsigned rrHairOpacityRecordCount=0; // native primary preparation thread only
static std::uintptr_t RRHairOpacityReplacement(std::uintptr_t original,std::uintptr_t selected,
 const RRAlbedoShader::CapturedPixelShader& albedo) noexcept {
 if(!rrHairOpacityStorageReady.load(std::memory_order_acquire)||!original||!selected||
    albedo.bytes.size()!=2932||albedo.hashFNV1a!=UINT64_C(0x2f95f8a616bc4e4f))return selected;
 for(unsigned i=0;i<rrHairOpacityRecordCount;++i){const auto* c=rrHairOpacityRecords[i];
  if(c->original==original&&c->selected==selected)return reinterpret_cast<std::uintptr_t>(c->record);}
 if(rrHairOpacityRecordCount>=32)return selected;
 try {
  char hash[65]{};
  if(!PTReflectionSha256(albedo.bytes.data(),albedo.bytes.size(),hash)||
     strcmp(hash,"184f8baa3cdbf53b1f25cfce1a45ad1b54dc44cd4045902b4064ee789cde01e0"))return selected;
  RRAlbedoShader::NativeDescriptor desc{},after{};
  if(!RRAlbedoShader::ReadDescriptor(original,0x50,&desc)||desc.stage!=4||desc.size!=32528)return selected;
  std::vector<uint8_t> bytes(static_cast<size_t>(desc.size));
  if(!RRAlbedoShader::CopyResident(&desc,bytes.data(),bytes.size())||
     !RRAlbedoShader::ReadDescriptor(original,0x50,&after)||desc.identifier!=after.identifier||desc.shaderBase!=after.shaderBase||desc.size!=after.size||
     !PTReflectionSha256(bytes.data(),bytes.size(),hash)||
     strcmp(hash,"33fde7037525c2b9aa6cca58bd45d3151ecc8f1f91bfa4ff6529909179dc41ca"))return selected;
  auto* c=new(std::nothrow) RRHairOpacityRecord;if(!c)return selected;
  if(!PTReflectionCloneShaderRecord(reinterpret_cast<void*>(selected),0x50,4,kRRHairOpacityStorageId,
       sizeof(kRRHairOpacityShader),c->record,c->descriptor)){delete c;return selected;}
  c->original=original;c->selected=selected;rrHairOpacityRecords[rrHairOpacityRecordCount++]=c;
  Log("RR_HAIR_OPACITY_READY original=%p selected=%p replacement=%p native_hash_match=1 opacity_texture_slot=26 channel=w reject=zero_only partial_opacity=weighted hair_alpha_tag=minus2_minus_opacity normal_guide=hair_only_5x5_majority_outlier skin_unchanged=1",reinterpret_cast<void*>(original),reinterpret_cast<void*>(selected),c->record);
  return reinterpret_cast<std::uintptr_t>(c->record);
 }catch(...){return selected;}
}
