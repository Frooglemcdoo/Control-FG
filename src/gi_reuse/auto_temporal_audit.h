#pragma once
#include "auto_pixel_audit.h"
namespace gi_reuse {
inline bool GIAuditPairDue(std::uint64_t ordinal) noexcept {
 return ordinal>=600&&ordinal<=3001&&ordinal%600<=1;
}
struct GIAuditDelta {uint index=0;float absolute=0,relative=0;GIAutoPixelAudit before{},after{};};
struct GIAuditTemporalSummary {
 uint compared=0,invalid=0,changed25=0,changed50=0,changed100=0;
 double rgbAbsSum=0;float maxRgbDelta=0;
 std::array<GIAuditDelta,8> examples{};
};
inline bool GIAuditPixelValid(const GIAutoPixelAudit& v){
 if((v.flags&5)!=5)return false;
 for(float c:v.weighted)if(!std::isfinite(c)||c<0)return false;
 return true;
}
inline GIAuditTemporalSummary GIAuditTemporal(const GIAutoPixelAudit* a,const GIAutoPixelAudit* b,size_t count){
 GIAuditTemporalSummary s;
 for(size_t i=0;i<count;++i){
  if(!GIAuditPixelValid(a[i])||!GIAuditPixelValid(b[i])){++s.invalid;continue;}
  ++s.compared;float delta=0,relative=0;
  for(uint k=0;k<3;++k){const float d=std::abs(b[i].weighted[k]-a[i].weighted[k]);
   s.rgbAbsSum+=d;delta=std::max(delta,d);
   relative=std::max(relative,d/std::max(.5f*(b[i].weighted[k]+a[i].weighted[k]),.00001f));}
  s.maxRgbDelta=std::max(s.maxRgbDelta,delta);
  if(delta<=.00001f||relative<=.25f)continue;
  ++s.changed25;if(relative>.5f)++s.changed50;if(relative>1)++s.changed100;
  GIAuditDelta e{uint(i),delta,relative,a[i],b[i]};for(auto& top:s.examples)if(e.absolute>top.absolute)std::swap(e,top);
 }
 return s;
}
// CPU-only copies after GPU completion. Never used as rendering history.
// Matching ordinals alone are insufficient: require exact adjacent engine
// frames, matching version and dimensions; accept either retirement order.
class GIAuditPairCache {
 std::uint64_t pair_=0;uint version_=0,w_=0,h_=0,frames_[2]{};
 std::vector<GIAutoPixelAudit> pixels_[2];bool present_[2]{};
public:
 bool Add(std::uint64_t ordinal,uint frame,uint version,uint w,uint h,const GIAutoPixelAudit* pixels,
          GIAuditTemporalSummary& summary,uint& first,uint& second){
  if(!GIAuditPairDue(ordinal)||!frame||!version||!w||!h||w>512||h>512||!pixels)return false;
  const auto pair=ordinal/600;const auto side=uint(ordinal%600);
  if(pair<pair_)return false;
  if(pair!=pair_||version!=version_||w!=w_||h!=h_){
   pair_=pair;version_=version;w_=w;h_=h;present_[0]=present_[1]=false;
  }
  pixels_[side].assign(pixels,pixels+size_t(w)*h);frames_[side]=frame;present_[side]=true;
  if(!present_[0]||!present_[1])return false;
  present_[0]=present_[1]=false;
  if(frames_[1]!=frames_[0]+1)return false;
  first=frames_[0];second=frames_[1];summary=GIAuditTemporal(pixels_[0].data(),pixels_[1].data(),size_t(w)*h);return true;
 }
};
}
