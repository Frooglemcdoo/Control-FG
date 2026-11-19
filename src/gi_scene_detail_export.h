#pragma once
namespace gi_scene_capture {
struct DetailExport {
 Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
 UINT64 bytes=0;unsigned frame=0,width=0,height=0,rays=0;
};
inline DWORD WINAPI ExportDetail(void* context) noexcept {
 std::unique_ptr<DetailExport> job(static_cast<DetailExport*>(context));void* data=nullptr;bool okay=false;
 try {
  wchar_t env[32768]{};const auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
  if(n&&n<32768){
   const std::wstring directory=std::wstring(env,n)+L"\\ControlFGProbe\\gi-scene-detail-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(job->frame);
   if(CreateDirectoryW(directory.c_str(),nullptr)){
    D3D12_RANGE read{0,SIZE_T(job->bytes)};
    if(SUCCEEDED(job->buffer->Map(0,&read,&data))&&data){
     RRGuideExportDetail::File file(directory+L"\\details.bin");okay=file.Write(data,size_t(job->bytes))&&file.Finish();
    }
    if(data){D3D12_RANGE empty{};job->buffer->Unmap(0,&empty);data=nullptr;}
    if(okay){
     const unsigned w=(std::min)(job->width,512u),h=(std::min)(job->height,512u);
     const UINT64 pixels=UINT64(w)*h,producer=((pixels*(job->rays+1)+1)/2)*32;
     std::ostringstream out;out<<"{\"schema\":\"gi-scene-details-r476\",\"frame\":"<<job->frame<<",\"width\":"<<job->width<<",\"height\":"<<job->height<<",\"columns\":"<<w<<",\"rows\":"<<h<<",\"rays\":"<<job->rays<<",\"file\":\"details.bin\",\"bytes\":"<<job->bytes<<",\"summary_offset\":0,\"summary_stride\":64,\"producer_offset\":"<<pixels*64<<",\"producer_stride\":16,\"red_offset\":"<<pixels*64+producer<<",\"red_stride\":192,\"mapping\":\"pixel_x=floor(column*width/columns); pixel_y=floor(row*height/rows)\",\"gpu_completed\":true}\n";
     const auto text=out.str();RRGuideExportDetail::File meta(directory+L"\\metadata.pending");okay=meta.Write(text.data(),text.size())&&meta.Finish();
     if(okay)okay=MoveFileExW((directory+L"\\metadata.pending").c_str(),(directory+L"\\metadata.json").c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
    }
   }
  }
 }catch(...){okay=false;}
 if(data){D3D12_RANGE empty{};job->buffer->Unmap(0,&empty);}
 Log("GI_SCENE_R476 detail_export frame=%u success=%u bytes=%llu",job->frame,unsigned(okay),job->bytes);return 0;
}
inline void QueueDetail(ID3D12Resource* buffer,UINT64 bytes,unsigned frame,unsigned width,unsigned height,unsigned rays) noexcept {
 auto* job=new(std::nothrow) DetailExport;if(!job){Log("GI_SCENE_R476 detail_export frame=%u success=0 allocation=0",frame);return;}
 job->buffer=buffer;job->bytes=bytes;job->frame=frame;job->width=width;job->height=height;job->rays=rays;
 if(!QueueUserWorkItem(&ExportDetail,job,WT_EXECUTEDEFAULT)){delete job;Log("GI_SCENE_R476 detail_export frame=%u success=0 queue=0",frame);}
}
}
