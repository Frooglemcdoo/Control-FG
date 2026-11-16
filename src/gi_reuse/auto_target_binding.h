#ifndef CONTROL_GI_AUTO_TARGET_BINDING_H
#define CONTROL_GI_AUTO_TARGET_BINDING_H
#include "auto_target_contract.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_BIND_INLINE inline
#else
#define GI_BIND_INLINE
#endif
// One immutable selection shared by producer and evaluator. Source frame is the
// frame that BUILT the targets, not necessarily the frame tracing these rays.
struct GIAutoSelection {uint version,sourceFrame,rayCount;float fraction;};
GI_BIND_INLINE bool GIAutoSelectionValid(GIAutoSelection s) {
 return s.version!=0&&s.sourceFrame!=0&&s.rayCount>=2&&s.rayCount<=64&&
  GIFinite(s.fraction)&&s.fraction>=0&&s.fraction<=1.0f/float(s.rayCount);
}
GI_BIND_INLINE bool GIAutoUsable(GIAutoTargets t,GIAutoSelection s) {
 if(!GIAutoSelectionValid(s)||!GIAutoMatches(t,s.version,s.sourceFrame))return false;
 for(uint i=0;i<2;++i)if((t.validMask&(1u<<i))!=0) {
  if(!GIFinitePoint(t.bounds[i].center)||!GIFinite(t.bounds[i].radius)||
     t.bounds[i].radius<=0||t.counts[i]==0||t.scores[i]==0)return false;
 }
 return true;
}
#ifdef __cplusplus
static_assert(sizeof(GIAutoSelection)==16,"GPU selection ABI");
}
#endif
#undef GI_BIND_INLINE
#endif
