#pragma once
// Only the audited character albedo record is cloned. Native records are read-only.
struct RRCharacterBaseAlbedoRecord {
 std::uintptr_t original=0,selected=0;
 alignas(16) unsigned char record[0xA8]{};
 alignas(16) unsigned char descriptor[0x100]{};
};
static RRCharacterBaseAlbedoRecord* rrCharacterBaseAlbedoRecords[32]{};
static unsigned rrCharacterBaseAlbedoRecordCount=0; // native primary preparation thread only
static std::uintptr_t RRCharacterBaseAlbedoReplacement(std::uintptr_t original,std::uintptr_t selected,
 const RRAlbedoShader::CapturedPixelShader& albedo) noexcept {
 if(!rrCharacterBaseAlbedoStorageReady.load(std::memory_order_acquire)||!original||!selected||
    albedo.bytes.size()!=3196||albedo.hashFNV1a!=UINT64_C(0x725bfe878e18cfac))return selected;
 for(unsigned i=0;i<rrCharacterBaseAlbedoRecordCount;++i){const auto* c=rrCharacterBaseAlbedoRecords[i];
  if(c->original==original&&c->selected==selected)return reinterpret_cast<std::uintptr_t>(c->record);}
 if(rrCharacterBaseAlbedoRecordCount>=32)return selected;
 try {
  char hash[65]{};
  if(!PTReflectionSha256(albedo.bytes.data(),albedo.bytes.size(),hash)||
     strcmp(hash,"dc9fe7a05347aa5b570595e75e4b19b787ecb9c4ee5bd6dfcc80b1f9ed776c24"))return selected;
  RRAlbedoShader::NativeDescriptor desc{},after{};
  if(!RRAlbedoShader::ReadDescriptor(original,0x50,&desc)||desc.stage!=4||desc.size!=30160)return selected;
  std::vector<uint8_t> bytes(static_cast<size_t>(desc.size));
  if(!RRAlbedoShader::CopyResident(&desc,bytes.data(),bytes.size())||
     !RRAlbedoShader::ReadDescriptor(original,0x50,&after)||desc.identifier!=after.identifier||desc.shaderBase!=after.shaderBase||desc.size!=after.size||
     !PTReflectionSha256(bytes.data(),bytes.size(),hash)||
     strcmp(hash,"0cd8a4b8144f1add1eac28edb815477ad7e3b42291735aa361e2f8e77b20bb96"))return selected;
  auto* c=new(std::nothrow) RRCharacterBaseAlbedoRecord;if(!c)return selected;
  if(!PTReflectionCloneShaderRecord(reinterpret_cast<void*>(selected),0x50,4,kRRCharacterBaseAlbedoStorageId,
       sizeof(kRRCharacterBaseAlbedoShader),c->record,c->descriptor)){delete c;return selected;}
  c->original=original;c->selected=selected;rrCharacterBaseAlbedoRecords[rrCharacterBaseAlbedoRecordCount++]=c;
  Log("RR_CHARACTER_BASE_ALBEDO_R18 original=%p selected=%p replacement=%p native_hash_match=1 base_texture_slot=0 tint_offset=52 detail_multiplier_removed=1 replay_only=1",reinterpret_cast<void*>(original),reinterpret_cast<void*>(selected),c->record);
  return reinterpret_cast<std::uintptr_t>(c->record);
 }catch(...){return selected;}
}
