#ifndef CONTROL_GI_PRODUCER_RAY_H
#define CONTROL_GI_PRODUCER_RAY_H
#include "producer_shading.h"
#include "finite_range_proposal.h"
#ifdef __cplusplus
namespace gi_reuse {
#endif
// Input from the owned cosine proposal generator, before finite-range weighting.
// cosine and BRDF*cos refer to the same receiver and direction. rangeRandom is
// independent of that direction; selectionRandom is independent of both.
struct GIProducerRay {
    GIPoint origin; float tMin;
    GIPoint direction; float baseRange;
    GIPoint primaryBrdfCos; float cosine;
    uint pixelX; uint pixelY; uint frameTag; uint sceneRevision;
    uint rayIndex; uint configuredCount; float depthScale; float selectionRandom;
    float rangeRandom; uint proposalVerified; uint reserved0; uint reserved1;
};
#ifdef __cplusplus
static_assert(sizeof(GIProducerRay)==96);
}
#endif
#endif
