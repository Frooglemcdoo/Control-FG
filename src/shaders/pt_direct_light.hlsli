#ifndef CONTROL_PT_DIRECT_LIGHT
#define CONTROL_PT_DIRECT_LIGHT
#include "pt_surface_transport.hlsli"
#ifdef __cplusplus
namespace control_pt {
#define PT_LIGHT_INLINE inline
#else
#define PT_LIGHT_INLINE
#endif
struct PTDirectSample {
    PTVec3 wi; PTVec3 incidentRadiance;
    // Non-delta: complete solid-angle PDF INCLUDING light-selection probability.
    // Delta point/directional: discrete selection probability (no angular PDF).
    float pdf; uint delta;
};
struct PTDirectResult { PTVec3 radiance; uint valid; };
PT_LIGHT_INLINE float PTPowerWeight(float a,float b) {
    float scale=max(a,b);
    if(scale<=0) return 0;
    a/=scale;b/=scale;return (a*a)/(a*a+b*b);
}
PT_LIGHT_INLINE PTDirectResult PTEvaluateDirect(PTSurfaceMaterial material,PTVec3 wo,
                                               PTDirectSample light,uint visibility) {
    PTDirectResult r;r.radiance=PTV(0,0,0);r.valid=0;
    if(visibility>1u||!PTFinite(light.pdf)||light.pdf<=0||!PTValid(light.incidentRadiance)||
       !PTValid(light.wi)||min(light.incidentRadiance.x,min(light.incidentRadiance.y,light.incidentRadiance.z))<0) return r;
    r.valid=1;
    if(visibility==0u||light.wi.z<=0||wo.z<=0) return r;
    PTSurfaceEvaluation b=PTEvaluateSurface(material,wo,light.wi);
    float mis=light.delta!=0u?1.0f:PTPowerWeight(light.pdf,b.pdf);
    r.radiance=PTScale(PTMul(b.f,light.incidentRadiance),light.wi.z*mis/light.pdf);
    if(!PTValid(r.radiance)||min(r.radiance.x,min(r.radiance.y,r.radiance.z))<0) {r.radiance=PTV(0,0,0);r.valid=0;}
    return r;
}
#ifdef __cplusplus
}
#endif
#undef PT_LIGHT_INLINE
#endif
