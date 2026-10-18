#pragma once
// F9-only semantic evidence: shaders are copied, never patched or substituted.
static std::atomic<bool> rrHairShaderAuditRequested{false};
static void RRHairExportShaderPair(unsigned long long frame,unsigned family,unsigned source,
 std::uintptr_t original,std::uintptr_t selected,const RRAlbedoShader::CapturedPixelShader& pixel) noexcept {
 try {
  wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
  if(!length||length>=32768)return;
  const auto directory=std::wstring(env,length)+L"\\ControlFGProbe\\rr-hair-shaders-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(frame);
  if(!CreateDirectoryW(directory.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return;
  const auto prefix=directory+L"\\family-"+std::to_wstring(family)+L"-batch-"+std::to_wstring(source);
  RRGuideExportDetail::File albedo(prefix+L"-albedo.dxbc");
  bool okay=albedo.Write(pixel.bytes.data(),pixel.bytes.size())&&albedo.Finish();
  RRAlbedoShader::NativeDescriptor desc{},after{};
  if(RRAlbedoShader::ReadDescriptor(original,0x50,&desc)&&desc.stage==4&&desc.size>=32&&desc.size<=RRAlbedoShader::kMaximumBlobBytes){
   std::vector<uint8_t> bytes(static_cast<size_t>(desc.size));
   if(RRAlbedoShader::CopyResident(&desc,bytes.data(),bytes.size())&&RRAlbedoShader::ReadDescriptor(original,0x50,&after)&&desc.shaderBase==after.shaderBase&&desc.identifier==after.identifier&&desc.size==after.size){
    RRGuideExportDetail::File native(prefix+L"-original.dxbc");okay=native.Write(bytes.data(),bytes.size())&&native.Finish()&&okay;
   }else okay=false;
  }else okay=false;
  Log("RR_HAIR_SHADER_AUDIT frame=%llu family=%u source_batch=%u original=%p selected=%p albedo_hash=0x%016llX exported=%u",frame,family,source,reinterpret_cast<void*>(original),reinterpret_cast<void*>(selected),static_cast<unsigned long long>(pixel.hashFNV1a),unsigned(okay));
 }catch(...){Log("RR_HAIR_SHADER_AUDIT_FAILED frame=%llu reason=export_exception",frame);}
}
