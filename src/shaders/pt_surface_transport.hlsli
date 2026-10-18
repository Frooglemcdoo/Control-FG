// Shared HLSL/C++ transport math for the new primary-surface integrator.
// No native descriptors or runtime hooks are enabled by this file.
#ifndef CONTROL_PT_SURFACE_TRANSPORT
#define CONTROL_PT_SURFACE_TRANSPORT
#ifdef __cplusplus
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace control_pt {
using uint = uint32_t;
struct PTVec3 { float x, y, z; };
using std::sqrt; using std::sin; using std::cos; using std::fabs;
using std::min; using std::max;
#define PT_INLINE inline
#else
typedef float3 PTVec3;
#define PT_INLINE
#endif
PT_INLINE PTVec3 PTV(float x,float y,float z) { PTVec3 r; r.x=x;r.y=y;r.z=z;return r; }
PT_INLINE PTVec3 PTAdd(PTVec3 a,PTVec3 b) { return PTV(a.x+b.x,a.y+b.y,a.z+b.z); }
PT_INLINE PTVec3 PTScale(PTVec3 a,float s) { return PTV(a.x*s,a.y*s,a.z*s); }
PT_INLINE PTVec3 PTMul(PTVec3 a,PTVec3 b) { return PTV(a.x*b.x,a.y*b.y,a.z*b.z); }
PT_INLINE float PTDot(PTVec3 a,PTVec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
PT_INLINE float PTMax(PTVec3 a) { return max(a.x,max(a.y,a.z)); }
PT_INLINE float PTSaturate(float a) { return min(1.0f,max(0.0f,a)); }
PT_INLINE bool PTFinite(float a) {
#ifdef __cplusplus
    return std::isfinite(a);
#else
    return isfinite(a);
#endif
}
PT_INLINE bool PTValid(PTVec3 a) { return PTFinite(a.x)&&PTFinite(a.y)&&PTFinite(a.z); }
PT_INLINE PTVec3 PTUnit(PTVec3 a) { return PTScale(a,1.0f/sqrt(max(PTDot(a,a),1.0e-30f))); }
PT_INLINE PTVec3 PTCross(PTVec3 a,PTVec3 b) { return PTV(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x); }
// Camera/radiance transport uses the raster shading frame. Reflection must
// still stay on the front geometric hemisphere; never resample rejected rays
// (resampling without changing the PDF would brighten grazing surfaces).
PT_INLINE bool PTValidPrimaryShadingNormal(PTVec3 ns,PTVec3 ng,PTVec3 wo){
    return PTValid(ns)&&PTValid(ng)&&PTValid(wo)&&PTDot(ns,ns)>1-1e-4f&&PTDot(ns,ns)<1+1e-4f&&
        PTDot(ns,ng)>0&&PTDot(ns,wo)>0&&PTDot(ng,wo)>0;
}
// A finite raster normal can put a geometrically visible primary point behind
// its shading hemisphere. Keep PT ownership and smoothly reduce that normal-map
// tilt instead of swapping the pixel back to native lighting. The same corrected
// frame is used by BRDF evaluation, sampling, PDFs and direct-light selection.
// Invalid inputs still reject; geometry continues to control visibility/spawn.
PT_INLINE PTVec3 PTPrimaryShadingNormal(PTVec3 ns,PTVec3 ng,PTVec3 wo){
    if(!PTValid(ns)||!PTValid(ng)||!PTValid(wo)||
       PTDot(ns,ns)<1-1e-4f||PTDot(ns,ns)>1+1e-4f||
       PTDot(ng,ng)<1-1e-4f||PTDot(ng,ng)>1+1e-4f||PTDot(wo,wo)<1-1e-4f||PTDot(wo,wo)>1+1e-4f)
        return PTV(0,0,0);
    float gv=PTDot(ng,wo),sv=PTDot(ns,wo),sg=PTDot(ns,ng);
    if(gv<=0)return PTV(0,0,0);
    float blend=0,target=0.1f*gv;
    if(sv<target)blend=(target-sv)/(gv-sv);
    if(sg<0.1f)blend=max(blend,(0.1f-sg)/(1-sg));
    return PTUnit(PTAdd(PTScale(ns,1-blend),PTScale(ng,blend)));
}
PT_INLINE bool PTPrimaryReflectionAboveGeometry(PTVec3 ng,PTVec3 wi){return PTValid(wi)&&PTDot(ng,wi)>0;}
// Secondary mesh normals are already inverse-transpose transformed by the hit
// shader. Orient them to the two-sided geometric frame. At silhouettes where
// the interpolated frame excludes the viewer, retain geometric shading instead
// of dropping the whole reflection path. Primary raster admission is unchanged.
PT_INLINE PTVec3 PTSecondaryShadingNormal(PTVec3 ns,PTVec3 ng,PTVec3 wo){
    if(!PTValid(ns)||PTDot(ns,ns)<1-1e-4f||PTDot(ns,ns)>1+1e-4f)return ng;
    if(PTDot(ns,ng)<0)ns=PTScale(ns,-1);
    return PTValidPrimaryShadingNormal(ns,ng,wo)?ns:ng;
}
static const float PTTransportPi=3.14159265358979323846f;

// Directions are normalized, point away from the surface, and are in its local
// shading frame (+Z normal). kd is a diffuse BRDF coefficient, not raw albedo
// inferred from a G-buffer. Material admission/energy partition is caller-owned.
struct PTSurfaceMaterial { PTVec3 kd; PTVec3 f0; float roughness; uint lobes; };
// Used only when the native primary specular-fill term is retained separately.
// Direct lighting still evaluates the original material; continuation samples
// only the diffuse lobe so the retained specular transport is not counted twice.
PT_INLINE PTSurfaceMaterial PTPrimaryDiffuseContinuation(PTSurfaceMaterial m) { m.lobes=1u;return m; }
struct PTSurfaceEvaluation { PTVec3 f; float pdf; };
struct PTSurfaceSample { PTVec3 wi; PTVec3 weight; float pdf; uint valid; uint fault; };
PT_INLINE float PTSpecularProbability(PTSurfaceMaterial m) {
    if ((m.lobes&1u)==0u) return 1.0f;
    if ((m.lobes&2u)==0u) return 0.0f;
    float d=max(0.0f,PTMax(m.kd)),s=max(0.0f,PTMax(m.f0));
    return min(0.95f,max(0.05f,s/max(d+s,1.0e-20f)));
}
PT_INLINE float PTAlpha(PTSurfaceMaterial m) {
    float r=PTSaturate(m.roughness);
    return max(r*r,0.001f); // Finite-width GGX; exact delta mirrors not admitted.
}
PT_INLINE float PTMasking(float nz,float alpha) {
    if (nz<=0.0f) return 0.0f;
    return 2.0f*nz/(nz+sqrt(alpha*alpha+(1.0f-alpha*alpha)*nz*nz));
}
PT_INLINE PTSurfaceEvaluation PTEvaluateSurfaceProposal(PTSurfaceMaterial m,PTVec3 wo,PTVec3 wi,float probability) {
    PTSurfaceEvaluation e; e.f=PTV(0,0,0);e.pdf=0;
    if(wo.z<=0||wi.z<=0||!PTValid(wo)||!PTValid(wi)||(m.lobes&3u)==0u) return e;
    float p=probability;
    if((m.lobes&1u)!=0u) {e.f=PTScale(m.kd,1.0f/PTTransportPi);e.pdf=(1.0f-p)*wi.z/PTTransportPi;}
    if((m.lobes&2u)!=0u) {
        PTVec3 h=PTUnit(PTAdd(wo,wi));float vh=max(0.0f,PTDot(wo,h));
        float a=PTAlpha(m),a2=a*a;
        // This form avoids subtractive cancellation at h.z=1 for narrow lobes.
        float den=(1.0f-h.z*h.z)+a2*h.z*h.z;
        float D=a2/(PTTransportPi*den*den);
        float gv=PTMasking(wo.z,a),gl=PTMasking(wi.z,a);
        float x=1.0f-PTSaturate(vh),x2=x*x,x5=x2*x2*x;
        PTVec3 F=PTAdd(m.f0,PTScale(PTAdd(PTV(1,1,1),PTScale(m.f0,-1)),x5));
        e.f=PTAdd(e.f,PTScale(F,D*gv*gl/(4.0f*wo.z*wi.z)));
        // Visible-normal half-vector PDF / reflection Jacobian 4*(wo.h).
        e.pdf+=p*D*gv/(4.0f*wo.z);
    }
    return e;
}
PT_INLINE PTSurfaceEvaluation PTEvaluateSurface(PTSurfaceMaterial m,PTVec3 wo,PTVec3 wi) {
    return PTEvaluateSurfaceProposal(m,wo,wi,PTSpecularProbability(m));
}
// The proposal may differ from the material's lobe energy. Its SAME mixture
// probability must be used both to choose a lobe and to evaluate the PDF.
PT_INLINE PTSurfaceSample PTSampleSurfaceProposal(PTSurfaceMaterial m,PTVec3 wo,float choose,float u,float v,float probability) {
    PTSurfaceSample s;s.wi=PTV(0,0,0);s.weight=PTV(0,0,0);s.pdf=0;s.valid=0;s.fault=0;
    if(!PTValid(wo)||!PTValid(m.kd)||!PTValid(m.f0)||!PTFinite(m.roughness)||
       !PTFinite(choose)||!PTFinite(u)||!PTFinite(v)||choose<0||choose>=1||!PTFinite(probability)||probability<0||probability>1||
       ((m.lobes&1u)==0u&&probability!=1)||((m.lobes&2u)==0u&&probability!=0)||
       min(m.kd.x,min(m.kd.y,m.kd.z))<0||min(m.f0.x,min(m.f0.y,m.f0.z))<0||
       PTMax(m.kd)>1||PTMax(m.f0)>1) {s.fault=1;return s;}
    if(wo.z<=0||(m.lobes&3u)==0u) return s;
    u=min(0.99999994f,max(0.0f,u));v=min(0.99999994f,max(0.0f,v));
    float radius=sqrt(u),phi=2.0f*PTTransportPi*v;
    if(choose<probability) {
        // GGX visible-normal disk warp (Heitz 2018). Below-surface reflections
        // terminate; do not resample them without correcting the conditional PDF.
        float a=PTAlpha(m);PTVec3 vh=PTUnit(PTV(a*wo.x,a*wo.y,wo.z));
        float lensq=vh.x*vh.x+vh.y*vh.y;
        PTVec3 t1=lensq>0?PTScale(PTV(-vh.y,vh.x,0),1.0f/sqrt(lensq)):PTV(1,0,0);
        PTVec3 t2=PTCross(vh,t1);
        float x=radius*cos(phi),y=radius*sin(phi),warp=0.5f*(1.0f+vh.z);
        y=(1.0f-warp)*sqrt(max(0.0f,1.0f-x*x))+warp*y;
        PTVec3 nh=PTAdd(PTAdd(PTScale(t1,x),PTScale(t2,y)),PTScale(vh,sqrt(max(0.0f,1.0f-x*x-y*y))));
        PTVec3 h=PTUnit(PTV(a*nh.x,a*nh.y,max(0.0f,nh.z)));
        s.wi=PTAdd(PTScale(h,2.0f*PTDot(wo,h)),PTScale(wo,-1));
    } else s.wi=PTV(radius*cos(phi),radius*sin(phi),sqrt(1.0f-u));
    PTSurfaceEvaluation e=PTEvaluateSurfaceProposal(m,wo,s.wi,probability);
    if(!PTFinite(e.pdf)||!PTValid(e.f)) {s.fault=1;return s;}
    if(e.pdf<=0) return s;
    s.weight=PTScale(e.f,s.wi.z/e.pdf);s.pdf=e.pdf;
    s.valid=PTValid(s.weight)&&min(s.weight.x,min(s.weight.y,s.weight.z))>=0?1u:0u;
    s.fault=s.valid==0u?1u:0u;
    return s;
}

PT_INLINE PTSurfaceSample PTSampleSurface(PTSurfaceMaterial m,PTVec3 wo,float choose,float u,float v) {
    return PTSampleSurfaceProposal(m,wo,choose,u,v,PTSpecularProbability(m));
}
PT_INLINE PTSurfaceSample PTSamplePrimarySurface(PTSurfaceMaterial m,PTVec3 wo,float choose,float u,float v) {
    // Unit-color primary diffuse is an output convention, not a reason to
    // starve glossy transport. Balance both proposals without changing f.
    return PTSampleSurfaceProposal(m,wo,choose,u,v,m.lobes==3u?.5f:PTSpecularProbability(m));
}

struct PTTransportState { PTVec3 throughput; PTVec3 radiance; uint depth; uint alive; uint valid; };
PT_INLINE PTTransportState PTBeginTransport() {
    PTTransportState p;p.throughput=PTV(1,1,1);p.radiance=PTV(0,0,0);p.depth=0;p.alive=1;p.valid=1;return p;
}
// Caller supplies emitted or direct-light radiance with the appropriate MIS
// weight. Native cached GI must not be fed here as though it were a path sample.
PT_INLINE PTTransportState PTAccumulate(PTTransportState p,PTVec3 incoming) {
    if(p.alive==0u) return p;
    if(!PTValid(incoming)||min(incoming.x,min(incoming.y,incoming.z))<0) {p.alive=0;p.valid=0;return p;}
    p.radiance=PTAdd(p.radiance,PTMul(p.throughput,incoming));
    if(!PTValid(p.radiance)) {p.alive=0;p.valid=0;}
    return p;
}
PT_INLINE PTTransportState PTContinue(PTTransportState p,PTSurfaceSample s,float roulette,uint rouletteStart) {
    if(p.alive==0u) return p;
    if(s.fault!=0u||!PTFinite(roulette)||roulette<0||roulette>=1) {p.alive=0;p.valid=0;return p;}
    if(s.valid==0u) {p.alive=0;return p;}
    p.throughput=PTMul(p.throughput,s.weight);p.depth+=1u;
    if(!PTValid(p.throughput)) {p.alive=0;p.valid=0;return p;}
    if(PTMax(p.throughput)<=0) {p.alive=0;return p;}
    if(p.depth>=rouletteStart) {
        float survival=min(0.95f,PTMax(p.throughput));
        if(roulette>=survival) {p.alive=0;return p;}
        p.throughput=PTScale(p.throughput,1.0f/survival);
    }
    return p;
}
#ifdef __cplusplus
} // namespace control_pt
#endif
#undef PT_INLINE
#endif
