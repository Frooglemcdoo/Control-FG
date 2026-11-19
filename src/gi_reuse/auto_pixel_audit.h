#pragma once
#include "auto_target_contract.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace gi_reuse {
// Two training-sized records, in a disjoint tail used only on audit frames.
struct GIAutoPixelAudit {
 float raw[3],rawPeak,weighted[3],weightedPeak,position[3],weight;
 uint material,flags,ray,covered;
};
static_assert(sizeof(GIAutoPixelAudit)==64,"pixel audit GPU ABI");
struct GIAutoRedAudit {
 float raw[3],weight,secondary[3];uint material;
 float position[3];uint ray;float throughput[3],scale;
 float terms[8][4]; // texture, tint, volume, surface, point, spot, fallback, BRDF
};
static_assert(sizeof(GIAutoRedAudit)==192,"red audit GPU ABI");
inline uint GIAuditWidth(uint w){return std::min(w,512u);}
inline uint GIAuditHeight(uint h){return std::min(h,512u);}
inline std::uint64_t GIAuditProducerBytes(uint w,uint h,uint rays){return ((std::uint64_t(GIAuditWidth(w))*GIAuditHeight(h)*(rays+1)+1)/2)*32;}
inline std::uint64_t GIAuditRedBytes(uint w,uint h){return std::uint64_t(GIAuditWidth(w))*GIAuditHeight(h)*sizeof(GIAutoRedAudit);}
inline uint GIAuditX(uint w){const auto n=GIAuditWidth(w);return std::min(w-n,w/5>n/2?w/5-n/2:0u);}
inline uint GIAuditY(uint h){return (h-GIAuditHeight(h))/2;}
inline float GIAuditLuma(const float* c){return c[0]*.2126f+c[1]*.7152f+c[2]*.0722f;}
struct GIAuditExample {uint index=0;float luminance=0,ratio=0;};
struct GIAuditSummary {
 uint valid=0,guided=0,invalid=0,spikes=0,downweightedSpikes=0;
 uint small2=0,small4=0,small10=0,peakDirectionMismatch=0,peakInsideUncovered=0;
 uint producerStampMismatch=0,producerWeightMismatch=0,producerDirectionMismatch=0,producerHitMismatch=0;
 uint redOutliers=0;std::array<GIAuditExample,12> redExamples{};
 double rawSum=0,weightedSum=0;
 std::array<GIAuditExample,12> smallExamples{};
 std::array<GIAuditExample,12> examples{};
};
inline GIAuditSummary GIAuditSummarize(const GIAutoPixelAudit* p,uint w,uint h){
 GIAuditSummary s;std::vector<float> l(size_t(w)*h);
 for(size_t i=0;i<l.size();++i){const auto& v=p[i];if(!(v.flags&1))continue;
  const float a=GIAuditLuma(v.raw),b=GIAuditLuma(v.weighted);
  if(!std::isfinite(a)||!std::isfinite(b)||a<0||b<0||!(v.flags&4)){++s.invalid;continue;}
  if(v.flags&8)++s.peakDirectionMismatch;
  if(v.flags&16)++s.peakInsideUncovered;
  if(v.flags&32)++s.producerStampMismatch;if(v.flags&64)++s.producerWeightMismatch;
  if(v.flags&128)++s.producerDirectionMismatch;if(v.flags&256)++s.producerHitMismatch;
  ++s.valid;if(v.flags&2)++s.guided;s.rawSum+=a;s.weightedSum+=b;l[i]=b;
 }
 for(uint y=2;y+2<h;++y)for(uint x=2;x+2<w;++x){const uint i=y*w+x;if(l[i]<=.00001f)continue;
  std::array<float,25> window{};uint n=0;
  for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)window[n++]=l[size_t(int(y)+dy)*w+size_t(int(x)+dx)];
  std::nth_element(window.begin(),window.begin()+12,window.end());const float ratio=l[i]/std::max(window[12],1e-10f);
  if(ratio>2){++s.small2;if(ratio>4)++s.small4;if(ratio>10)++s.small10;
   GIAuditExample e{i,l[i],ratio};for(auto& top:s.smallExamples)if(e.luminance>top.luminance)std::swap(e,top);
  }
  const auto& rgb=p[i].weighted;
  if((p[i].flags&5)==5&&rgb[0]>1.25f*std::max(rgb[1],rgb[2])){
   std::array<float,25> reds{};uint rn=0;
   for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx){const auto& q=p[size_t(int(y)+dy)*w+size_t(int(x)+dx)];reds[rn++]=(q.flags&5)==5&&std::isfinite(q.weighted[0])?q.weighted[0]:0;}
   std::nth_element(reds.begin(),reds.begin()+12,reds.end());
   const float redRatio=rgb[0]/std::max(reds[12],1e-10f);
   if(redRatio>2){++s.redOutliers;GIAuditExample e{i,rgb[0]-std::max(rgb[1],rgb[2]),redRatio};for(auto& top:s.redExamples)if(e.luminance>top.luminance)std::swap(e,top);}
  }
  if(l[i]<=.001f||ratio<=10)continue;
  ++s.spikes;if(p[i].weight<1)++s.downweightedSpikes;
  GIAuditExample e{i,l[i],ratio};for(auto& top:s.examples)if(e.luminance>top.luminance)std::swap(e,top);
 }
 return s;
}
}
