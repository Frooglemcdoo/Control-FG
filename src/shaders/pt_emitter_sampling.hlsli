#ifndef CONTROL_PT_EMITTER_SAMPLING
#define CONTROL_PT_EMITTER_SAMPLING
#include "pt_direct_light.hlsli"
#ifdef __cplusplus
namespace control_pt {
#define PT_EMITTER_INLINE inline
#else
#define PT_EMITTER_INLINE
#endif
struct PTEmitterPoint { PTVec3 position,normal;float area,pdfArea;uint valid; };
// Uniform triangle selection followed by uniform area sampling. Count includes
// every triangle in the immutable emitter list, including currently black texels.
PT_EMITTER_INLINE PTEmitterPoint PTSampleEmitterTriangle(PTVec3 a,PTVec3 b,PTVec3 c,
                                                        uint count,float u,float v){
    PTEmitterPoint r;r.position=PTV(0,0,0);r.normal=PTV(0,0,0);r.area=0;r.pdfArea=0;r.valid=0;
    if(count==0||!PTValid(a)||!PTValid(b)||!PTValid(c)||!PTFinite(u)||!PTFinite(v)||u<0||u>=1||v<0||v>=1)return r;
    PTVec3 ab=PTAdd(b,PTScale(a,-1)),ac=PTAdd(c,PTScale(a,-1));
    PTVec3 cross=PTCross(ab,ac);float len2=PTDot(cross,cross);
    if(!PTFinite(len2)||len2<=1e-20f)return r;
    float len=sqrt(len2),s=sqrt(u);
    r.position=PTAdd(PTScale(a,1-s),PTAdd(PTScale(b,s*(1-v)),PTScale(c,s*v)));
    r.normal=PTScale(cross,1/len);r.area=.5f*len;r.pdfArea=1/(float(count)*r.area);
    r.valid=PTValid(r.position)&&PTFinite(r.pdfArea)&&r.pdfArea>0?1u:0u;return r;
}
PT_EMITTER_INLINE float PTEmitterSolidAnglePdf(float area,uint count,float distanceSquared,float emitterCosine){
    if(count==0||!PTFinite(area)||area<=0||!PTFinite(distanceSquared)||distanceSquared<=0||
       !PTFinite(emitterCosine)||emitterCosine<=0||emitterCosine>1)return 0;
    float pdf=distanceSquared/(float(count)*area*emitterCosine);
    return PTFinite(pdf)&&pdf>0?pdf:0;
}
// Same PDF conventions on both sides: include emitter selection probability,
// and use the actual BSDF proposal (including balanced primary lobe selection).
PT_EMITTER_INLINE float PTEmitterHitWeight(float bsdfPdf,float lightPdf){
    if(!PTFinite(bsdfPdf)||bsdfPdf<=0||!PTFinite(lightPdf)||lightPdf<0)return 0;
    return PTPowerWeight(bsdfPdf,lightPdf);
}
struct PTEmitterDirectResult {PTVec3 radiance,diffuse;uint valid;};
PT_EMITTER_INLINE PTEmitterDirectResult PTEvaluateEmitterDirect(PTSurfaceMaterial material,PTVec3 wo,PTVec3 wi,
    PTVec3 emission,float lightPdf,float specularProbability,uint visibility,uint continuationAvailable){
    PTEmitterDirectResult r;r.radiance=PTV(0,0,0);r.diffuse=r.radiance;r.valid=0;
    if(!PTValid(wo)||!PTValid(wi)||!PTValid(emission)||PTMax(PTScale(emission,-1))>0||
       !PTFinite(lightPdf)||lightPdf<=0||!PTFinite(specularProbability)||specularProbability<0||specularProbability>1||
       visibility>1||continuationAvailable>1)return r;
    r.valid=1;if(visibility==0||wi.z<=0||wo.z<=0)return r;
    PTSurfaceEvaluation b=PTEvaluateSurfaceProposal(material,wo,wi,specularProbability);
    float weight=continuationAvailable!=0?PTPowerWeight(lightPdf,b.pdf):1;
    float scale=wi.z*weight/lightPdf;
    r.radiance=PTScale(PTMul(b.f,emission),scale);
    PTSurfaceMaterial diffuse=material;diffuse.lobes&=1u;
    r.diffuse=PTScale(PTMul(PTEvaluateSurfaceProposal(diffuse,wo,wi,specularProbability).f,emission),scale);
    if(!PTValid(r.radiance)||!PTValid(r.diffuse)){r.radiance=PTV(0,0,0);r.diffuse=r.radiance;r.valid=0;}
    return r;
}
#ifdef __cplusplus
}
#endif
#undef PT_EMITTER_INLINE
#endif
