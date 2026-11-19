#pragma once
namespace gi_scene_capture {
struct DetailExport {
 Microsoft::WRL::ComPtr<ID3D12Resource> buffer,paired;
 UINT64 bytes=0,pairBytes=0;unsigned frame=0,width=0,height=0,rays=0;
};
inline bool WriteCompletedBuffer(ID3D12Resource* buffer,UINT64 bytes,const std::wstring& path){
 if(!buffer||!bytes||buffer->GetDesc().Width<bytes)return false;
 void* data=nullptr;D3D12_RANGE read{0,SIZE_T(bytes)};
 if(FAILED(buffer->Map(0,&read,&data))||!data)return false;
 struct Unmap {ID3D12Resource* b;~Unmap(){D3D12_RANGE empty{};b->Unmap(0,&empty);}} unmap{buffer};
 RRGuideExportDetail::File file(path);return file.Write(data,size_t(bytes))&&file.Finish();
}
inline DWORD WINAPI ExportDetail(void* context) noexcept {
 std::unique_ptr<DetailExport> job(static_cast<DetailExport*>(context));bool okay=false;
 try {
  wchar_t env[32768]{};const auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
  if(n&&n<32768){
   const std::wstring directory=std::wstring(env,n)+L"\\ControlFGProbe\\gi-scene-detail-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(job->frame);
   if(CreateDirectoryW(directory.c_str(),nullptr)){
    okay=WriteCompletedBuffer(job->buffer.Get(),job->bytes,directory+L"\\details.bin")&&job->pairBytes==gi_reuse::GIAutoPairBytes(job->width,job->height)&&WriteCompletedBuffer(job->paired.Get(),job->pairBytes,directory+L"\\paired-mips.bin");
    if(okay){
     const unsigned w=(std::min)(job->width,512u),h=(std::min)(job->height,512u);
     const UINT64 pixels=UINT64(w)*h,producer=((pixels*(job->rays+1)+1)/2)*32;
     std::ostringstream out;out<<"{\"schema\":\"gi-scene-details-r481\",\"frame\":"<<job->frame<<",\"width\":"<<job->width<<",\"height\":"<<job->height<<",\"columns\":"<<w<<",\"rows\":"<<h<<",\"rays\":"<<job->rays<<",\"file\":\"details.bin\",\"bytes\":"<<job->bytes<<",\"summary_offset\":0,\"summary_stride\":64,\"producer_offset\":"<<pixels*64<<",\"producer_stride\":16,\"red_offset\":"<<pixels*64+producer<<",\"red_stride\":192,\"mapping\":\"pixel_x=floor(column*width/columns); pixel_y=floor(row*height/rows)\",\"gpu_completed\":true,";
     out<<"\"paired\":{\"file\":\"paired-mips.bin\",\"bytes\":"<<UINT64(job->width)*job->height*32<<",\"total_file_bytes\":"<<job->pairBytes<<",\"texture_probe_offset\":"<<UINT64(job->width)*job->height*32<<",\"texture_probe_stride\":192,\"texture_probe_points\":"<<pixels<<",\"texture_probe_layout\":\"uint4 material_descriptor_ray_flags; uint4 width_height_levels_pad; float4 uv_weight_pad; float4 sample0; float4 sample1; float4 load0; float4 load1; uint4 mip1_dimensions; float4 controlled0; float4 controlled1; float4 bilinear0; float4 bilinear1\",\"texture_probe_flags\":\"1=dimensions_valid 2=uv_valid 4=load1_valid; Load wraps UV then reads nearest texel; not bilinear\",\"stride\":32,\"baseline_sampling\":\"native_samplelevel0\",\"comparison_sampling\":\"sampler_independent_bilinear_mip1_clamped_to_view\",\"mip0_rgb_offset\":0,\"mip0_flags_offset\":12,\"mip1_rgb_offset\":16,\"mip1_flags_offset\":28,\"rgb_format\":\"float32_le\",\"flags_format\":\"uint32_le\",\"flags\":\"bit0=valid_receiver bit1=valid_radiance\",\"mapping\":\"row_major_full_resolution\",\"same_hits_weights_and_scene\":true,\"writes_game_outputs\":false,\"before_rr\":true,\"separate_rr_histories\":false}}\n";
     const auto text=out.str();RRGuideExportDetail::File meta(directory+L"\\metadata.pending");okay=meta.Write(text.data(),text.size())&&meta.Finish();
     if(okay)okay=MoveFileExW((directory+L"\\metadata.pending").c_str(),(directory+L"\\metadata.json").c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
    }
   }
  }
 }catch(...){okay=false;}
 Log("GI_SCENE_R481 detail_export frame=%u success=%u bytes=%llu pair_bytes=%llu",job->frame,unsigned(okay),job->bytes,job->pairBytes);return 0;
}
inline void QueueDetail(ID3D12Resource* buffer,UINT64 bytes,unsigned frame,unsigned width,unsigned height,unsigned rays,ID3D12Resource* paired,UINT64 pairBytes) noexcept {
 auto* job=new(std::nothrow) DetailExport;if(!job){Log("GI_SCENE_R481 detail_export frame=%u success=0 allocation=0",frame);return;}
 job->buffer=buffer;job->bytes=bytes;job->frame=frame;job->width=width;job->height=height;job->rays=rays;job->paired=paired;job->pairBytes=pairBytes;
 if(!QueueUserWorkItem(&ExportDetail,job,WT_EXECUTEDEFAULT)){delete job;Log("GI_SCENE_R481 detail_export frame=%u success=0 queue=0",frame);}
}
}
