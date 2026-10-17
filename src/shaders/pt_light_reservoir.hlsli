#ifndef CONTROL_PT_LIGHT_RESERVOIR
#define CONTROL_PT_LIGHT_RESERVOIR
#include "pt_direct_light.hlsli"
#ifdef __cplusplus
namespace control_pt {
#define PT_RIS_INLINE inline
#else
#define PT_RIS_INLINE
#endif
// Enumerate bounded primary light lists to remove candidate-omission noise.
// Larger lists and secondary vertices retain the bounded four-proposal cost.
PT_RIS_INLINE uint PTLightCandidateCount(uint total,uint primary){
    return primary!=0&&total>0&&total<=64?total:4u;
}
PT_RIS_INLINE float PTLightCandidateChoose(uint total,uint primary,uint index,float random){
    return primary!=0&&total>0&&total<=64?(float(index)+0.5f)/float(total):random;
}
// Unshadowed reflected radiance, with unit PDF: the same material response as
// final direct shading. Visibility is deliberately excluded until selection.
PT_RIS_INLINE float PTLightTarget(PTSurfaceMaterial material,PTVec3 wo,PTVec3 wi,PTVec3 incident){
    PTDirectSample light;light.wi=wi;light.incidentRadiance=incident;light.pdf=1;light.delta=1;
#ifdef PT_DIRECT_LIGHT_ROUGHNESS
    light.roughness=material.roughness;
#endif
    PTDirectResult direct=PTEvaluateDirect(material,wo,light,1);
    return direct.valid!=0?PTDot(direct.radiance,PTV(.2126f,.7152f,.0722f)):-1.0f;
}
struct PTLightReservoir {float sum,target;uint count,selected,valid;};
PT_RIS_INLINE PTLightReservoir PTEmptyLightReservoir(){
    PTLightReservoir r;r.sum=0;r.target=0;r.count=0;r.selected=0;r.valid=1;return r;
}
// Uniform random proposals OR one deterministic proposal per light with q=1/N.
// For enumeration, count=N cancels the proposal factor N exactly. Zero-target
// proposals still count. Dropping them would bias normalization brighter.
PT_RIS_INLINE PTLightReservoir PTAddLightCandidate(PTLightReservoir r,float target,float proposalPdf,float random,uint index){
    r.count++;
    if(!PTFinite(target)||target<0||!PTFinite(proposalPdf)||proposalPdf<=0||!PTFinite(random)||random<0||random>=1){r.valid=0;return r;}
    float weight=target/proposalPdf;float sum=r.sum+weight;
    if(!PTFinite(sum)){r.valid=0;return r;}
    if(weight>0&&random*sum<weight){r.selected=index;r.target=target;}
    r.sum=sum;return r;
}
// Effective denominator of the RIS estimator, not a reusable angular PDF.
// Valid only for our delta-light direct estimator (MIS weight fixed at one).
PT_RIS_INLINE float PTLightDenominator(PTLightReservoir r){
    return r.valid!=0&&r.sum>0&&r.target>0?float(r.count)*r.target/r.sum:0;
}
#ifdef __cplusplus
}
#endif
#undef PT_RIS_INLINE
#endif
