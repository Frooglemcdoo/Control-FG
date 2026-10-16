#ifndef CONTROL_PT_LIGHT_RESERVOIR
#define CONTROL_PT_LIGHT_RESERVOIR
#include "pt_surface_transport.hlsli"
#ifdef __cplusplus
namespace control_pt {
#define PT_RIS_INLINE inline
#else
#define PT_RIS_INLINE
#endif
struct PTLightReservoir {float sum,target;uint count,selected,valid;};
PT_RIS_INLINE PTLightReservoir PTEmptyLightReservoir(){
    PTLightReservoir r;r.sum=0;r.target=0;r.count=0;r.selected=0;r.valid=1;return r;
}
// Independent candidates; no spatial/temporal reservoir reuse. Zero-target
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
