#ifndef CONTROL_GI_PRODUCER_SHADING_H
#define CONTROL_GI_PRODUCER_SHADING_H
#include "producer_hit.h"
#include "initial_sample.h"
#ifdef __cplusplus
namespace gi_reuse {
#endif
// Produced in the same dispatch epoch as hit; never constructed from filtered RGB.
// The host must establish native alpha acceptance and matching scene bindings.
struct GIProducerShadingRequest {
    GIProducerHit hit;
    uint pixelX; uint pixelY; uint frameTag; uint sceneRevision;
    GIPoint primaryBrdfCos; float sourcePdf;
    uint rayIndex; uint configuredCount; float depthScale; float random;
    // Miss endpoint O+D*TMax supplies the fallback direction, not a surface.
    GIPoint missEndpoint; uint producerVerified;
};
struct GIShadedProducerSample {
    GIInitialInput shading;
    GIPoint geometricNormal; uint geometryVerified;
    float random; float hitDistance; uint reserved1; uint reserved2;
};
#ifdef __cplusplus
static_assert(sizeof(GIProducerShadingRequest)==160);
static_assert(sizeof(GIShadedProducerSample)==112);
}
#endif
#endif
