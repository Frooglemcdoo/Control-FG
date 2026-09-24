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
