#pragma once
#include <cmath>
#include <limits>
namespace control_rr {
inline bool CopyValidatedProjection(const double* projection, const double* inverse, float* output) noexcept {
 if (!projection || !inverse || !output) return false;
 for (unsigned i=0;i<16;++i) {
  if (!std::isfinite(projection[i]) || !std::isfinite(inverse[i]) ||
      std::fabs(projection[i]) > std::numeric_limits<float>::max()) return false;
 }
 // Both native matrices must form an inverse pair; no guessed projection.
 for (unsigned row=0;row<4;++row) for (unsigned col=0;col<4;++col) {
  double forward=0,backward=0;
  for (unsigned k=0;k<4;++k) {
   forward+=projection[row*4+k]*inverse[k*4+col];
   backward+=inverse[row*4+k]*projection[k*4+col];
  }
  const double expected=row==col?1.0:0.0;
  if (!std::isfinite(forward) || !std::isfinite(backward) ||
      std::fabs(forward-expected)>0.001 || std::fabs(backward-expected)>0.001) return false;
 }
 for (unsigned i=0;i<16;++i) output[i]=static_cast<float>(projection[i]);
 return true;
}
}

inline bool CopyValidatedAffine43(const double* matrix,const double* inverse,float* output) noexcept {
 if(!matrix||!inverse||!output)return false;
 double a[16]{},b[16]{};
 // Remedy stores affine transforms as row-major 4x3: three basis rows followed
 // by translation. Expand to the row-vector 4x4 form validated by camera_capture.
 for(unsigned row=0;row<4;++row){
  for(unsigned col=0;col<3;++col){
   const double av=matrix[row*3+col],bv=inverse[row*3+col];
   if(!std::isfinite(av)||!std::isfinite(bv)||
      std::fabs(av)>std::numeric_limits<float>::max()||
      std::fabs(bv)>std::numeric_limits<float>::max())return false;
   a[row*4+col]=av;b[row*4+col]=bv;
  }
  a[row*4+3]=(row==3)?1.0:0.0;
  b[row*4+3]=(row==3)?1.0:0.0;
 }
 for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){
  double forward=0.0,backward=0.0;
  for(unsigned k=0;k<4;++k){
   forward+=a[row*4+k]*b[k*4+col];
   backward+=b[row*4+k]*a[k*4+col];
  }
  const double expected=row==col?1.0:0.0;
  if(!std::isfinite(forward)||!std::isfinite(backward)||
     std::fabs(forward-expected)>0.001||std::fabs(backward-expected)>0.001)return false;
 }
 for(unsigned i=0;i<16;++i)output[i]=static_cast<float>(a[i]);
 return true;
}
