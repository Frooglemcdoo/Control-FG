#ifndef CONTROL_GI_FINITE_RANGE_PROPOSAL_H
#define CONTROL_GI_FINITE_RANGE_PROPOSAL_H
#include "initial_sample.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_RANGE_INLINE inline
#else
#define GI_RANGE_INLINE
#endif
// Continuous proposal model for the owned cosine sampler with an independent
// uniform range variate. NOT evidence that the original correlated native
// sequence has this distribution. Caller must establish this producer contract.
struct GIRangeProposal {
    float directionalPdf;
    float reachProbability;
    float surfacePdf;
    uint valid;
};
GI_RANGE_INLINE GIRangeProposal GIMakeRangeProposal(float cosine, float distance,
    float baseRange, uint producerContractVerified) {
#ifdef __cplusplus
    GIRangeProposal r{};
#else
    GIRangeProposal r=(GIRangeProposal)0;
#endif
    if(producerContractVerified!=1||!GIFinite(cosine)||cosine<=0||cosine>1||
       !GIFinite(distance)||distance<0||!GIFinite(baseRange)||baseRange<=0) return r;
    // Same float coefficients as the audited producer's 0.8 + 0.4*w law.
    float relative=distance/baseRange;
    if(!GIFinite(relative)) return r;
    float q=(0.8f+0.4f-relative)/0.4f;
    q=q>1?1:q<0?0:q;
    r.directionalPdf=cosine*0.3183098861837907f;
    r.reachProbability=q;r.surfacePdf=r.directionalPdf*q;
    if(!GIFinite(r.surfacePdf)||r.surfacePdf<=0)return r;
    r.valid=1;return r;
}
// Surface PDF includes the probability that the randomized finite ray reaches
// the hit. The receiving response includes ITS reach probability. Together they
// preserve native finite-radius hit energy for a source-only sample; misses
// retain their existing fallback estimator and all-ray normalization.
// Inputs require true hit distance, NOT the native biased position texture.
// This does not authorize spatial reuse or supply connection visibility.
GI_RANGE_INLINE GIInitialInput GIApplyRangeProposal(GIInitialInput input,
    GIRangeProposal source, GIRangeProposal receiver) {
    // Keep existing independent verification flags; never manufacture them.
    if(!source.valid||!receiver.valid||input.material>=65534) {
        input.verified=0;input.sourcePdf=0;return input;
    }
    input.sourcePdf=source.surfacePdf;
    input.primaryBrdfCos.x*=receiver.reachProbability;
    input.primaryBrdfCos.y*=receiver.reachProbability;
    input.primaryBrdfCos.z*=receiver.reachProbability;
    return input;
}
#ifdef __cplusplus
}
#endif
#undef GI_RANGE_INLINE
#endif
