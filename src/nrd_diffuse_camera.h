#pragma once
#include <array>
#include <cmath>
#include <limits>

namespace control_nrd {
// Control uses row-vector matrices. Its row-major storage is the same byte
// order as the transposed, column-vector matrices required by NRD/HLSL.
// Keep double precision until the native inverse/composition checks finish.
using Matrix = std::array<double,16>;
inline Matrix ExpandAffine(const double* native) {
 Matrix m{};
 for(unsigned c=0;c<4;++c)for(unsigned r=0;r<3;++r)m[c*4+r]=native[c*3+r];
 m[15]=1;return m;
}
inline Matrix Multiply(const Matrix&a,const Matrix&b) {
 Matrix m{};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)
  for(unsigned k=0;k<4;++k)m[c*4+r]+=a[k*4+r]*b[c*4+k];
 return m;
}
inline bool Near(const Matrix&a,const Matrix&b) {
 for(unsigned i=0;i<16;++i)if(!std::isfinite(a[i])||!std::isfinite(b[i])||
  std::abs(a[i]-b[i])>1e-5*(1+std::abs(b[i])))return false;
 return true;
}
struct CameraMatrices {
 std::array<float,16> worldToView{},viewToWorld{},viewToClip{},clipToView{};
};
// Audited native projection translation: renderer instance +0x480/+0x488.
// NGX raster offsets are (width*dx/2, -height*dy/2). NRD asks for sample
// offsets instead: the unjittered ray through a raster texel subtracts NGX.
// See validation/jitter-native/evidence.json and rr_jitter_ray.hlsli.
inline bool NativeClipShiftToNrdJitter(double dx,double dy,unsigned width,unsigned height,
                                     std::array<float,2>& output) {
 if(!width||!height||!std::isfinite(dx)||!std::isfinite(dy))return false;
 const double x=-.5*width*dx,y=.5*height*dy;
 if(std::abs(x)>.500001||std::abs(y)>.500001)return false;
 // Clamp only the accepted floating-point endpoint tolerance. Session admission
 // requires the actual submitted values to stay within [-0.5, 0.5].
 output={float(std::fmax(-.5,std::fmin(.5,x))),float(std::fmax(-.5,std::fmin(.5,y)))};return true;
}
template<class Camera> bool ConvertCamera(const Camera& c,CameraMatrices& output) {
 const auto wv=ExpandAffine(c.worldToView),vw=ExpandAffine(c.viewToWorld);
 Matrix vc{},cv{},wc{},cw{},identity{};
 for(unsigned i=0;i<16;++i){vc[i]=c.viewToClip[i];cv[i]=c.clipToView[i];
  wc[i]=c.worldToClip[i];cw[i]=c.clipToWorld[i];identity[i]=i%5==0?1:0;}
 if(!Near(Multiply(wv,vw),identity)||!Near(Multiply(vc,cv),identity)||
    !Near(Multiply(vc,wv),wc)||!Near(Multiply(vw,cv),cw))return false;
 CameraMatrices next;
 for(unsigned i=0;i<16;++i){
  const double values[]={wv[i],vw[i],vc[i],cv[i]};
  for(double v:values)if(!std::isfinite(v)||std::abs(v)>(std::numeric_limits<float>::max)())return false;
  next.worldToView[i]=float(wv[i]);next.viewToWorld[i]=float(vw[i]);
  next.viewToClip[i]=float(vc[i]);next.clipToView[i]=float(cv[i]);
 }
 output=next;return true;
}
}
