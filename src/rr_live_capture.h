#pragma once
#include "rr_live_capture_policy.h"
struct RRLiveCapture {
 control_rr::LiveCapturePolicy policy;
 ID3D12Resource* readback[3]{};D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[3]{};UINT64 bytes[3]{};
 UINT width=0,height=0;unsigned long long present=0;
 control_rr::LiveFrameInput frameInput{};CameraSnapshot camera{};
 std::uintptr_t gbuffer1=0,gbuffer2=0,material=0,envbrdf=0;
 UINT64 materialBytes=0;DXGI_FORMAT envFormat=DXGI_FORMAT_UNKNOWN;UINT envWidth=0,envHeight=0;
};
struct RRLiveGuideSummary {
 unsigned long long samples=0,finite=0,normalFinite=0,ratioSamples=0;
 unsigned long long roughLt10=0,roughLt20=0,roughGt60=0,roughGt80=0,specGtDiffuse=0,specGt50=0,specGt100=0;
 double roughMin=1.0e30,roughMax=-1.0e30,roughSum=0.0;
 double diffuseMin=1.0e30,diffuseMax=-1.0e30,diffuseSum=0.0;
 double specMin=1.0e30,specMax=-1.0e30,specSum=0.0;
 double normalLengthMin=1.0e30,normalLengthMax=-1.0e30,normalLengthSum=0.0;
 double ratioSum=0.0;
 unsigned long long roughBins[20]{},diffuseBins[20]{},specBins[20]{};
};
static unsigned RRLiveBin01(double v) noexcept {
 if(!std::isfinite(v)||v<=0.0)return 0;if(v>=1.0)return 19;
 unsigned b=static_cast<unsigned>(v*20.0);return b>19?19:b;
}
static unsigned RRLiveBin02(double v) noexcept {
 if(!std::isfinite(v)||v<=0.0)return 0;if(v>=2.0)return 19;
 unsigned b=static_cast<unsigned>(v*10.0);return b>19?19:b;
}
static double RRLiveLuma(const float p[4]) noexcept {
 return double(p[0])*0.2126+double(p[1])*0.7152+double(p[2])*0.0722;
}
static RRLiveGuideSummary RRLiveAnalyze(const RRGuideExportImageView& nr,const RRGuideExportImageView& spec,const RRGuideExportImageView& diffuse) noexcept {
 RRLiveGuideSummary s{};
 // 8-pixel sampling gives ~130k representative samples at 4K while keeping the
 // post-fence worker cheap. Raw guide textures remain untouched and are not written to disk.
 for(UINT y=0;y<nr.height;y+=8) for(UINT x=0;x<nr.width;x+=8) {
  ++s.samples;float n[4]{},sp[4]{},df[4]{};
  RRGuideExportDetail::ReadHalfPixel(nr,x,y,n);RRGuideExportDetail::ReadHalfPixel(spec,x,y,sp);RRGuideExportDetail::ReadHalfPixel(diffuse,x,y,df);
  const bool nf=std::isfinite(n[0])&&std::isfinite(n[1])&&std::isfinite(n[2])&&std::isfinite(n[3]);
  const bool sf=std::isfinite(sp[0])&&std::isfinite(sp[1])&&std::isfinite(sp[2]);
  const bool dfok=std::isfinite(df[0])&&std::isfinite(df[1])&&std::isfinite(df[2]);
  if(!(nf&&sf&&dfok))continue;++s.finite;
  const double rough=n[3],dl=RRLiveLuma(df),sl=RRLiveLuma(sp);
  if(rough<s.roughMin)s.roughMin=rough;if(rough>s.roughMax)s.roughMax=rough;s.roughSum+=rough;++s.roughBins[RRLiveBin01(rough)];
  if(rough<0.1)++s.roughLt10;if(rough<0.2)++s.roughLt20;if(rough>0.6)++s.roughGt60;if(rough>0.8)++s.roughGt80;
  if(dl<s.diffuseMin)s.diffuseMin=dl;if(dl>s.diffuseMax)s.diffuseMax=dl;s.diffuseSum+=dl;++s.diffuseBins[RRLiveBin02(dl)];
  if(sl<s.specMin)s.specMin=sl;if(sl>s.specMax)s.specMax=sl;s.specSum+=sl;++s.specBins[RRLiveBin02(sl)];
  if(sl>dl)++s.specGtDiffuse;if(sl>0.5)++s.specGt50;if(sl>1.0)++s.specGt100;
  if(dl>1.0e-4){s.ratioSum+=sl/dl;++s.ratioSamples;}
  const double len=std::sqrt(double(n[0])*n[0]+double(n[1])*n[1]+double(n[2])*n[2]);
  if(std::isfinite(len)){++s.normalFinite;if(len<s.normalLengthMin)s.normalLengthMin=len;if(len>s.normalLengthMax)s.normalLengthMax=len;s.normalLengthSum+=len;}
 }
 return s;
}
static void RRLiveJsonBins(std::ostringstream& out,const unsigned long long (&bins)[20]) {
 out<<'[';for(unsigned i=0;i<20;++i){if(i)out<<',';out<<bins[i];}out<<']';
}
static void RRLiveJsonSample(std::ostringstream& out,const RRGuideExportImageView& nr,const RRGuideExportImageView& spec,const RRGuideExportImageView& diffuse,UINT x,UINT y) {
 float n[4]{},sp[4]{},df[4]{};RRGuideExportDetail::ReadHalfPixel(nr,x,y,n);RRGuideExportDetail::ReadHalfPixel(spec,x,y,sp);RRGuideExportDetail::ReadHalfPixel(diffuse,x,y,df);
 out<<"{\"x\":"<<x<<",\"y\":"<<y<<",\"roughness\":"<<n[3]<<",\"normal\":["<<n[0]<<','<<n[1]<<','<<n[2]<<"],\"diffuse\":["<<df[0]<<','<<df[1]<<','<<df[2]<<"],\"specular\":["<<sp[0]<<','<<sp[1]<<','<<sp[2]<<"]}";
}
static DWORD WINAPI RRLiveCaptureExport(void* context) noexcept {
 auto* c=static_cast<RRLiveCapture*>(context);void* mapped[3]{};bool okay=true;
 try {
  for(UINT i=0;i<3;++i){D3D12_RANGE range{0,static_cast<SIZE_T>(c->bytes[i])};if(FAILED(c->readback[i]->Map(0,&range,&mapped[i]))||!mapped[i]){okay=false;break;}}
  if(okay){
   RRGuideExportImageView nr{RRGuideImageSemantic::NativeAlbedoTarget0,RRGuideImageFormat::Rgba16Float,c->width,c->height,static_cast<unsigned char*>(mapped[0])+c->footprint[0].Offset,c->footprint[0].Footprint.RowPitch,c->frameInput.frame};
   RRGuideExportImageView spec{RRGuideImageSemantic::NativeAlbedoTarget0,RRGuideImageFormat::Rgba16Float,c->width,c->height,static_cast<unsigned char*>(mapped[1])+c->footprint[1].Offset,c->footprint[1].Footprint.RowPitch,c->frameInput.frame};
   RRGuideExportImageView diffuse{RRGuideImageSemantic::NativeAlbedoTarget0,RRGuideImageFormat::Rgba16Float,c->width,c->height,static_cast<unsigned char*>(mapped[2])+c->footprint[2].Offset,c->footprint[2].Footprint.RowPitch,c->frameInput.frame};
   const auto summary=RRLiveAnalyze(nr,spec,diffuse);
   wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
   if(!length||length>=32768)okay=false;
   else {
    std::wstring directory(env,length);directory+=L"\\ControlFGProbe\\rr-live-r21t-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(c->frameInput.frame);
    okay=CreateDirectoryW(directory.c_str(),nullptr)!=FALSE;
    if(okay){
     std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(9);
     const double denom=summary.finite?double(summary.finite):1.0;
     out<<"{\"schema\":\"control-rr-live-r21t-stats\",\"pid\":"<<GetCurrentProcessId()<<",\"frame\":"<<c->frameInput.frame<<",\"present\":"<<c->present<<",\"width\":"<<c->width<<",\"height\":"<<c->height<<",\"fence\":"<<c->policy.fence<<",\"jitter_x\":"<<c->frameInput.jitterX<<",\"jitter_y\":"<<c->frameInput.jitterY<<",\"reset\":"<<c->frameInput.reset
        <<",\"source\":\"owned_rr_guides_same_dispatch\",\"raw_files_written\":false,\"sample_step\":8,\"samples\":"<<summary.samples<<",\"finite_samples\":"<<summary.finite
        <<",\"resources\":{\"gbuffer1\":\"0x"<<std::hex<<c->gbuffer1<<"\",\"gbuffer2\":\"0x"<<c->gbuffer2<<"\",\"material\":\"0x"<<c->material<<"\",\"envbrdf\":\"0x"<<c->envbrdf<<std::dec<<"\",\"material_bytes\":"<<c->materialBytes<<",\"env_format\":"<<static_cast<unsigned>(c->envFormat)<<",\"env_width\":"<<c->envWidth<<",\"env_height\":"<<c->envHeight<<"}"
        <<",\"roughness\":{\"min\":"<<summary.roughMin<<",\"max\":"<<summary.roughMax<<",\"mean\":"<<(summary.roughSum/denom)<<",\"lt_0_1\":"<<summary.roughLt10<<",\"lt_0_2\":"<<summary.roughLt20<<",\"gt_0_6\":"<<summary.roughGt60<<",\"gt_0_8\":"<<summary.roughGt80<<",\"hist_0_to_1\":";RRLiveJsonBins(out,summary.roughBins);out<<'}'
        <<",\"diffuse_luma\":{\"min\":"<<summary.diffuseMin<<",\"max\":"<<summary.diffuseMax<<",\"mean\":"<<(summary.diffuseSum/denom)<<",\"hist_0_to_2\":";RRLiveJsonBins(out,summary.diffuseBins);out<<'}'
        <<",\"specular_luma\":{\"min\":"<<summary.specMin<<",\"max\":"<<summary.specMax<<",\"mean\":"<<(summary.specSum/denom)<<",\"gt_diffuse\":"<<summary.specGtDiffuse<<",\"gt_0_5\":"<<summary.specGt50<<",\"gt_1_0\":"<<summary.specGt100<<",\"hist_0_to_2\":";RRLiveJsonBins(out,summary.specBins);out<<'}'
        <<",\"specular_to_diffuse_mean\":"<<(summary.ratioSamples?summary.ratioSum/double(summary.ratioSamples):0.0)
        <<",\"normal_length\":{\"min\":"<<summary.normalLengthMin<<",\"max\":"<<summary.normalLengthMax<<",\"mean\":"<<(summary.normalFinite?summary.normalLengthSum/double(summary.normalFinite):0.0)<<"},\"grid\":[";
     bool first=true;for(unsigned gy=1;gy<=3;++gy)for(unsigned gx=1;gx<=7;++gx){UINT x=UINT((std::uint64_t(c->width-1)*gx)/8),y=UINT((std::uint64_t(c->height-1)*gy)/4);if(!first)out<<',';first=false;RRLiveJsonSample(out,nr,spec,diffuse,x,y);}out<<"]}\n";
     const auto json=out.str();RRGuideExportDetail::File file(directory+L"\\metadata.pending");okay=file.Write(json.data(),json.size())&&file.Finish();
     if(okay)okay=MoveFileExW((directory+L"\\metadata.pending").c_str(),(directory+L"\\metadata.json").c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
     Log("RR_G12_GUIDE_STATS frame=%llu samples=%llu finite=%llu rough_min=%.6f rough_mean=%.6f rough_max=%.6f rough_lt_0_2=%llu rough_gt_0_8=%llu diffuse_mean=%.6f specular_mean=%.6f spec_gt_diffuse=%llu spec_diff_ratio=%.6f normal_mean=%.6f metadata=%u",
         c->frameInput.frame,summary.samples,summary.finite,summary.roughMin,summary.roughSum/denom,summary.roughMax,summary.roughLt20,summary.roughGt80,summary.diffuseSum/denom,summary.specSum/denom,summary.specGtDiffuse,summary.ratioSamples?summary.ratioSum/double(summary.ratioSamples):0.0,summary.normalFinite?summary.normalLengthSum/double(summary.normalFinite):0.0,unsigned(okay));
    }
   }
  }
 }catch(...){okay=false;}
 for(UINT i=0;i<3;++i)if(mapped[i]){D3D12_RANGE written{0,0};c->readback[i]->Unmap(0,&written);}
 Log("RR_G12_LIVE_CAPTURE_EXPORTED success=%u frame=%llu width=%u height=%u fence=%llu source=live_dispatch stats_only=1 raw_files=0",unsigned(okay),c->frameInput.frame,c->width,c->height,c->policy.fence);
 return 0;
}
