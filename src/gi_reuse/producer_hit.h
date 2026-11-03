#ifndef CONTROL_GI_PRODUCER_HIT_H
#define CONTROL_GI_PRODUCER_HIT_H
#include "true_geometry.h"
#ifdef __cplusplus
namespace gi_reuse {
#endif
// The first DWORD is reserved for the native GI alpha handler's payload prefix.
// This layout alone does not prove compatibility with an arbitrary any-hit export.
struct GIProducerHit {
    uint nativeValue; uint material; uint geometry; uint primitive;
    GIPoint position; float rayT;
    GIPoint geometricNormal; uint valid;
    uint instanceIndex; uint instanceId; uint hitKind; uint reserved;
    // Interpolated inverse-transpose shading normal, kept distinct from the face.
    GIPoint shadingNormal; float texcoordU;
    float texcoordV; uint reserved1; uint reserved2; uint reserved3;
};
#ifdef __cplusplus
static_assert(sizeof(GIProducerHit)==96);
}
#endif
#endif
