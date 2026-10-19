#pragma once
// R22: F9-only CPU evidence. Never changes replay admission, shaders or GPU work.
static void RRWorldExportShaderAudit(unsigned long long frame,
 const control_rr_albedo::PreparedReplay& replay) noexcept {
 const DWORD saved=GetLastError();
 try {
  wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
  if(!length||length>=32768){SetLastError(saved);return;}
  const auto directory=std::wstring(env,length)+L"\\ControlFGProbe\\rr-world-shaders-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(frame);
  if(!CreateDirectoryW(directory.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS){SetLastError(saved);return;}
  std::uintptr_t source=0;unsigned count=0;
  memcpy(&source,replay.manager.data()+0x30,sizeof(source));
  memcpy(&count,replay.manager.data()+0x38,sizeof(count));
  if(!source||count>32768u){SetLastError(saved);return;}
  struct Blob {std::uintptr_t shader;std::string file;bool okay;};
  std::vector<Blob> blobs;size_t copied=0;unsigned failed=0,capped=0;
  const auto exportPixel=[&](std::uintptr_t shader)->std::string {
   if(!shader)return "unavailable";
   for(const auto& b:blobs)if(b.shader==shader)return b.okay?b.file:"failed";
   if(blobs.size()>=256u){++capped;return "cap";}
   const auto name=std::string("pixel-")+std::to_string(blobs.size())+".dxbc";
   blobs.push_back({shader,name,false});auto& entry=blobs.back();
   RRAlbedoShader::NativeDescriptor before{},after{};
   if(!RRAlbedoShader::ReadDescriptor(shader,0x50,&before)||before.stage!=4||before.identifier<0||
      before.size<32||before.size>RRAlbedoShader::kMaximumBlobBytes){++failed;return "failed";}
   const size_t size=static_cast<size_t>(before.size);
   if(size>64u*1024u*1024u-copied){++capped;return "cap";}
   std::vector<uint8_t> bytes(size);copied+=size;
   if(!RRAlbedoShader::CopyResident(&before,bytes.data(),size)||
      !RRAlbedoShader::ReadDescriptor(shader,0x50,&after)||before.stage!=after.stage||
      before.shaderBase!=after.shaderBase||before.identifier!=after.identifier||before.size!=after.size){++failed;return "failed";}
   const auto path=directory+L"\\"+std::wstring(name.begin(),name.end());
   RRGuideExportDetail::File output(path);
   entry.okay=output.Write(bytes.data(),bytes.size())&&output.Finish();
   if(!entry.okay)++failed;
   return entry.okay?name:"failed";
  };
  std::ostringstream csv;
  csv<<"source_batch,material_pointer,original_shader,selected_shader,prepared_family,original_pixel,selected_pixel,instance_count\n";
  unsigned readFailed=0;size_t selectedIndex=0;
  for(unsigned i=0;i<count;++i){
   control_rr_albedo::OpaqueBatch batch{};
   if(!RRAlbedoRead(source+size_t(i)*sizeof(batch),&batch,sizeof(batch))){++readFailed;continue;}
   while(selectedIndex<replay.sourceIndices.size()&&replay.sourceIndices[selectedIndex]<i)++selectedIndex;
   const bool selected=selectedIndex<replay.sourceIndices.size()&&replay.sourceIndices[selectedIndex]==i;
   const auto shader=selected?replay.batches[selectedIndex].shader:std::uintptr_t(0);
   const auto nativeFile=exportPixel(batch.shader),albedoFile=exportPixel(shader);
   csv<<i<<",0x"<<std::hex<<batch.material<<",0x"<<batch.shader<<",0x"<<shader<<std::dec
      <<','<<(selected?unsigned(replay.familyKinds[selectedIndex]):0u)<<','<<nativeFile<<','<<albedoFile<<','<<batch.instanceCount<<'\n';
  }
  const auto contents=csv.str();RRGuideExportDetail::File inventory(directory+L"\\batches.csv");
  const bool okay=inventory.Write(contents.data(),contents.size())&&inventory.Finish();
  std::ostringstream report;report<<"schema=control-rr-world-shaders-r22\nframe="<<frame
    <<"\nsource_batches="<<count<<"\nunique_shader_records="<<blobs.size()<<"\ncopied_bytes="<<copied
    <<"\nread_failures="<<readFailed<<"\nshader_failures="<<failed<<"\ncap_events="<<capped
    <<"\ninventory_written="<<unsigned(okay)<<"\nrendering=R21_unchanged\n";
  const auto summary=report.str();RRGuideExportDetail::File summaryFile(directory+L"\\summary.txt");
  const bool summaryOkay=summaryFile.Write(summary.data(),summary.size())&&summaryFile.Finish();
  Log("RR_WORLD_SHADER_AUDIT_R22 frame=%llu batches=%u records=%u read_failed=%u shader_failed=%u capped=%u inventory=%u summary=%u rendering_unchanged=1",
      frame,count,unsigned(blobs.size()),readFailed,failed,capped,unsigned(okay),unsigned(summaryOkay));
 }catch(...){Log("RR_WORLD_SHADER_AUDIT_R22 frame=%llu failed=exception rendering_unchanged=1",frame);}
 SetLastError(saved);
}
