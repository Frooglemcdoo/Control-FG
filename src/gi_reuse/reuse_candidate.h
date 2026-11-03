#ifndef CONTROL_GI_REUSE_CANDIDATE_H
#define CONTROL_GI_REUSE_CANDIDATE_H
#include "finite_range_proposal.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_REUSE_INLINE inline
#else
#define GI_REUSE_INLINE
#endif
// Result of a FRESH connection trace using the current scene and native alpha
// policy. This struct does not perform a trace or establish those facts itself.
struct GIConnectionVisibility {
    uint frameTag; uint connectionId; uint sceneRevision; uint visible;
    uint completed; uint nativeAlphaPolicy; uint reserved0; uint reserved1;
};
// Build one receiver contribution from already admitted geometry and shading.
// No reservoir combination/history is performed. secondaryRadiance must have
// been evaluated for the receiver direction, or under an explicitly verified
// direction-independent model. Native source-dependent GI6 RGB is insufficient.
GI_REUSE_INLINE GIInitialResult GIShadeReuseCandidate(GIConnection connection,
    GIInitialInput receiver, float sourceBaseRange, float receiverBaseRange,
    float sourceCosine, float receiverCosine, uint producerContractVerified,
    uint radianceTransportVerified, uint sceneRevision,
    GIConnectionVisibility visibility, uint frameTag) {
#ifdef __cplusplus
    GIInitialResult rejected{};
#else
    GIInitialResult rejected=(GIInitialResult)0;
#endif
    if(!sceneRevision||radianceTransportVerified!=1||visibility.frameTag!=frameTag||
       visibility.connectionId!=connection.connectionId||visibility.sceneRevision!=sceneRevision||
       visibility.visible!=1||visibility.completed!=1||visibility.nativeAlphaPolicy!=1||
       visibility.reserved0||visibility.reserved1||receiver.material!=connection.material||
       receiver.material>=65534||receiver.frameTag!=frameTag) return rejected;
    // Replace per-draw TMax gates with the marginal model's support envelope.
    // A second receiver randomized-TMax gate would apply reach twice.
    float sourceDistance=GISqrt(GIDot(GIDelta(connection.secondary,connection.source),
                                     GIDelta(connection.secondary,connection.source)));
    float receiverDistance=GISqrt(GIDot(GIDelta(connection.secondary,connection.target),
                                       GIDelta(connection.secondary,connection.target)));
    GIRangeProposal source=GIMakeRangeProposal(sourceCosine,sourceDistance,sourceBaseRange,producerContractVerified);
    GIRangeProposal target=GIMakeRangeProposal(receiverCosine,receiverDistance,receiverBaseRange,producerContractVerified);
    if(!source.valid||!target.valid) return rejected;
    connection.sourceMaxDistance=sourceBaseRange*(0.8f+0.4f);
    connection.targetMaxDistance=receiverBaseRange*(0.8f+0.4f);
    connection.sourceSolidAnglePdf=source.surfacePdf;
    GIConnectionResult geometry=GIEvaluateConnection(connection,frameTag);
    if(!geometry.valid) return rejected;
    receiver=GIApplyRangeProposal(receiver,source,target);
    receiver.sourcePdf=geometry.mappedSolidAnglePdf;
    receiver.secondaryPosition=connection.secondary;
    return GIMakeInitialSample(receiver,frameTag);
}
#ifdef __cplusplus
static_assert(sizeof(GIConnectionVisibility)==32);
}
#endif
#undef GI_REUSE_INLINE
#endif
