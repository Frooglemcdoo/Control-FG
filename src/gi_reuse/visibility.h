#ifndef CONTROL_GI_VISIBILITY_H
#define CONTROL_GI_VISIBILITY_H
#include "connection.h"
#ifdef __cplusplus
namespace gi_reuse {
#endif
// Written only after the connection ray returns. CPU submission is not completion.
struct GIConnectionVisibility {
    uint frameTag; uint connectionId; uint sceneRevision; uint visible;
    uint completed; uint nativeAlphaPolicy; uint reserved0; uint reserved1;
};
struct GIVisibilityRequest {
    GIConnection connection;
    float originTMin; float endpointBias; uint instanceMask; uint nativeBindingsVerified;
    uint sceneRevision; uint reserved0; uint reserved1; uint reserved2;
};
struct GIVisibilityConstants {
    uint hitOffset; uint geometryStride; uint missIndex; uint frameTag;
    uint sceneRevision; uint nativeAlphaPolicy; uint reserved0; uint reserved1;
};
#ifdef __cplusplus
static_assert(sizeof(GIConnectionVisibility)==32);
static_assert(sizeof(GIVisibilityRequest)==112);
static_assert(sizeof(GIVisibilityConstants)==32);
}
#endif
#endif
