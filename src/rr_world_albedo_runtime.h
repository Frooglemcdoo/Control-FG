#pragma once
// No shader replacement: admit the existing native base-texture albedo variant
// only when both captured pixel shaders match the audited R22 bytecode.
static bool RRWorldAlbedoPairMatches(std::uintptr_t original,
 const RRAlbedoShader::CapturedPixelShader& albedo) noexcept {
 try {
  if(!original||albedo.bytes.size()!=2908)return false;
  char hash[65]{};
  if(!PTReflectionSha256(albedo.bytes.data(),albedo.bytes.size(),hash)||
     strcmp(hash,"6e8fdc47e77f203157c23e9f77e19b1a66f874a66e3955e29aff3181309536dd"))return false;
  RRAlbedoShader::NativeDescriptor before{},after{};
  if(!RRAlbedoShader::ReadDescriptor(original,0x50,&before)||before.stage!=4||before.size!=15072)return false;
  std::vector<uint8_t> bytes(static_cast<size_t>(before.size));
  if(!RRAlbedoShader::CopyResident(&before,bytes.data(),bytes.size())||
     !RRAlbedoShader::ReadDescriptor(original,0x50,&after)||before.stage!=after.stage||
     before.identifier!=after.identifier||before.shaderBase!=after.shaderBase||before.size!=after.size||
     !PTReflectionSha256(bytes.data(),bytes.size(),hash)||
     strcmp(hash,"7518cf8fa5becc5bef03f7e3b0a87f910c1c93ce7e178e81413f6402da96b939"))return false;
  return true;
 }catch(...){return false;}
}

// Called only after RRWorldAlbedoAdmission proved the exact original/selected pair.
// Bounded immutable clones; failure retains native albedo without the private tag.
struct RRWorldAlbedoRecord {
 std::uintptr_t original=0,selected=0;
 alignas(16) unsigned char record[0xA8]{};
 alignas(16) unsigned char descriptor[0x100]{};
};
static RRWorldAlbedoRecord* rrWorldAlbedoRecords[128]{};
static unsigned rrWorldAlbedoRecordCount=0;
static std::uintptr_t RRWorldAlbedoReplacement(std::uintptr_t original,std::uintptr_t selected) {
 if(!rrWorldAlbedoStorageReady.load(std::memory_order_acquire))return selected;
 for(unsigned i=0;i<rrWorldAlbedoRecordCount;++i){const auto* c=rrWorldAlbedoRecords[i];
  if(c->original==original&&c->selected==selected)return reinterpret_cast<std::uintptr_t>(c->record);}
 if(rrWorldAlbedoRecordCount>=128)return selected;
 auto* c=new(std::nothrow) RRWorldAlbedoRecord;if(!c)return selected;
 if(!PTReflectionCloneShaderRecord(reinterpret_cast<void*>(selected),0x50,4,kRRWorldAlbedoStorageId,
    sizeof(kRRWorldAlbedoShader),c->record,c->descriptor)){delete c;return selected;}
 c->original=original;c->selected=selected;rrWorldAlbedoRecords[rrWorldAlbedoRecordCount++]=c;
 Log("RR_WORLD_TAG_R25 original=%p selected=%p alpha=-4 rgb=audited_base_texture_times_tint",reinterpret_cast<void*>(original),reinterpret_cast<void*>(selected));
 return reinterpret_cast<std::uintptr_t>(c->record);
}
