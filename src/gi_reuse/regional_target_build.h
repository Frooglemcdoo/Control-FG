#ifndef CONTROL_GI_REGIONAL_TARGET_BUILD_H
#define CONTROL_GI_REGIONAL_TARGET_BUILD_H
#include "regional_target_lookup.h"
#ifdef __cplusplus
namespace gi_reuse {
#endif
// Proposal data only. Scores never replace evaluated light. Source bounds must
// come from observed evidence; publication does not manufacture source radiance.
struct GIRegionalTraining {GIWorldGuideKey receiver;GIAutoBound source;float score;uint version;};
struct GIRegionalCell {uint owner,first,second,count;};
#ifdef __cplusplus
static_assert(sizeof(GIRegionalTraining)==44,"regional training ABI");
static_assert(sizeof(GIRegionalCell)==16,"regional cell ABI");
}
#endif
#endif
