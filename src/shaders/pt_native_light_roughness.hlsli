#ifndef CONTROL_PT_NATIVE_LIGHT_ROUGHNESS
#define CONTROL_PT_NATIVE_LIGHT_ROUGHNESS
#include "pt_surface_transport.hlsli"
#ifdef __cplusplus
namespace control_pt {
#define PT_RADIUS_INLINE inline
#else
#define PT_RADIUS_INLINE
#endif
// Captured a9e5b544... point DWORD12090..12533, spot DWORD7644..7983.
// Native finite-radius direct-light lobe broadening only. This is not a change
// to material roughness, path sampling, RR guides, or ray visibility.
PT_RADIUS_INLINE float PTNativeLightRoughness(float roughness,float radius,float distance,float nDotL){
    if(!PTFinite(roughness)||roughness<0||roughness>1||!PTFinite(radius)||radius<0||
       !PTFinite(distance)||distance<0||!PTFinite(nDotL))return -1;
    float result=roughness;
    if(radius>0&&distance>0){
        float h=2*PTSaturate(.5f-.5f*distance*nDotL/radius)-1;
        float a=h<0?-h:h;
        float ac=(((-.0187292993f*a+.0742610022f)*a-.212114394f)*a+1.57072878f)*sqrt(max(1-a,0.0f));
        if(h<0)ac=PTTransportPi-ac;
        float fraction=PTSaturate((ac-h*sqrt(max(1-h*h,0.0f)))/PTTransportPi);
        float ratio=min(radius/distance,1.0e19f);
        float area=min(fraction*.5f*ratio*ratio,1.0f);
        if(area>.000333333359f){
            float b=1/(1+3*area);
            float variance=(1-b*b)/(2*(3*b-b*b*b));
            result=sqrt(roughness*roughness+variance);
        }
    }
    return min(max(result,.05f),1.0f);
}
#ifdef __cplusplus
}
#endif
#undef PT_RADIUS_INLINE
#endif
