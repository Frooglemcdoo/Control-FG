#ifndef CONTROL_GI_REGIONAL_TARGET_LOOKUP_H
#define CONTROL_GI_REGIONAL_TARGET_LOOKUP_H
// Overhaul prototype. Not connected to game dispatches. Immutable receiver-keyed
// lookup shared by trace and evaluation; collisions never authorize another region.
#include "frozen_guide_training.h"
#include "auto_target_binding.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_REGIONAL_INLINE inline
#else
#define GI_REGIONAL_INLINE
#endif
struct GIRegionalTarget {GIWorldGuideKey receiver;GIAutoTargets targets;};
GI_REGIONAL_INLINE uint GIRegionalSlot(GIWorldGuideKey key,uint capacity,uint probe){
 return capacity?((GITrainingHash(key)+probe)%capacity):0;
}
GI_REGIONAL_INLINE bool GIRegionalAccept(GIRegionalTarget entry,GIWorldGuideKey receiver,
 uint frame,uint version,uint maximumAge){
 if(!GIWorldGuideKeysMatch(entry.receiver,receiver)||!frame||!version||
    entry.targets.version!=version||entry.targets.frame>frame||
    frame-entry.targets.frame>maximumAge)return false;
 GIAutoSelection s;s.version=version;s.sourceFrame=entry.targets.frame;
 s.sampleFrame=frame;s.rayCount=4;s.fraction=.5f;
 return GIAutoUsable(entry.targets,s);
}
#ifdef __cplusplus
// Four probes bound the CPU fixture and the intended GPU lookup. Missing,
// stale or collided entries fall back before tracing, never after observing light.
inline bool GIRegionalLookup(const GIRegionalTarget* table,uint capacity,
 GIWorldGuideKey receiver,uint frame,uint version,uint maximumAge,GIAutoTargets& out){
 out={};if(!table||!capacity||receiver.valid!=1)return false;
 for(uint probe=0;probe<4&&probe<capacity;++probe){
  const auto& entry=table[GIRegionalSlot(receiver,capacity,probe)];
  if(GIRegionalAccept(entry,receiver,frame,version,maximumAge)){out=entry.targets;return true;}
 }
 return false;
}
static_assert(sizeof(GIRegionalTarget)==84,"regional target prototype ABI");
}
#endif
#undef GI_REGIONAL_INLINE
#endif
