#ifndef CONTROL_GI_FROZEN_TRAINING_H
#define CONTROL_GI_FROZEN_TRAINING_H
#include "frozen_world_guide.h"
#ifdef __cplusplus
namespace gi_reuse {
#endif
struct GITrainingRecord {GIWorldGuideKey key;GIPoint direction;float energy;};
struct GITrainingCell {int x,y,z;uint normal;uint state,count,poison;uint energy[64];};
struct GIKeyedGuide {GIWorldGuideKey key;GIFrozenWorldGuide guide;};
#ifdef __cplusplus
inline
#endif
uint GITrainingHash(GIWorldGuideKey k) {
#ifdef __cplusplus
 uint h=uint(k.x)*0x8da6b343u^uint(k.y)*0xd8163841u^uint(k.z)*0xcb1ab31fu^k.normal*0x165667b1u;
#else
 uint h=asuint(k.x)*0x8da6b343u^asuint(k.y)*0xd8163841u^asuint(k.z)*0xcb1ab31fu^k.normal*0x165667b1u;
#endif
 h^=h>>16;h*=0x7feb352du;h^=h>>15;return h;
}
#ifdef __cplusplus
static_assert(sizeof(GITrainingRecord)==36,"Training ABI");
static_assert(sizeof(GITrainingCell)==284,"Atomic cell ABI");
static_assert(sizeof(GIKeyedGuide)==296,"Frozen keyed guide ABI");
}
#endif
#endif
