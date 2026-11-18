#ifndef CONTROL_GI_AUTO_TARGET_CONTRACT_H
#define CONTROL_GI_AUTO_TARGET_CONTRACT_H
// Research target builder ABI. Proposal scores never replace shaded radiance.
#include "connection.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_TARGET_INLINE inline
using GIGridInt=std::int32_t;
#else
#define GI_TARGET_INLINE
#define GIGridInt int
#endif
struct GIAutoTraining {GIPoint position;float energy;uint valid,version,reserved0,reserved1;};
struct GIAutoCell {GIGridInt x,y,z;uint state,score,count,reserved0,reserved1;};
struct GIAutoBound {GIPoint center;float radius;};
struct GIAutoTargets {uint schema,version,frame,validMask;GIAutoBound bounds[2];uint counts[2];uint scores[2];};
GI_TARGET_INLINE uint GIAutoHash(GIGridInt x,GIGridInt y,GIGridInt z) {
 uint h=uint(x)*73856093u^uint(y)*19349663u^uint(z)*83492791u;h^=h>>16;h*=0x7feb352du;h^=h>>15;return h;
}
GI_TARGET_INLINE bool GIAutoMatches(GIAutoTargets t,uint version,uint sourceFrame) {
 return t.schema==0x41544731u&&t.version==version&&t.frame==sourceFrame&&t.validMask>0&&t.validMask<=3;
}
#ifdef __cplusplus
#ifdef CONTROL_GI_REGIONAL
inline constexpr unsigned GIAutoTrainingStride=64,GIAutoCellStride=16,GIAutoPublicationBytes=4096*84;
#else
inline constexpr unsigned GIAutoTrainingStride=32,GIAutoCellStride=32,GIAutoPublicationBytes=64;
#endif
static_assert(sizeof(GIAutoTraining)==32&&sizeof(GIAutoCell)==32&&sizeof(GIAutoTargets)==64,"GPU ABI");
}
#else
#undef GIGridInt
#endif
#undef GI_TARGET_INLINE
#endif
