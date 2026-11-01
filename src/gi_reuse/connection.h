#pragma once
// Geometry/PDF conversion only. This is not a reservoir or a final lighting weight.
#ifdef __cplusplus
#include <cmath>
#include <cstdint>
namespace gi_reuse {
using uint = std::uint32_t;
#define GI_INLINE inline
inline bool GIFinite(float v) { return std::isfinite(v); }
inline float GISqrt(float v) { return std::sqrt(v); }
inline float GIAbs(float v) { return std::fabs(v); }
#else
#define GI_INLINE
bool GIFinite(float v) { return isfinite(v); }
float GISqrt(float v) { return sqrt(v); }
float GIAbs(float v) { return abs(v); }
#endif
struct GIPoint { float x, y, z; };
struct GIConnection {
    GIPoint source; float sourceMaxDistance;
    GIPoint target; float targetMaxDistance;
    GIPoint secondary; float sourceSolidAnglePdf;
    GIPoint secondaryGeometricNormal; uint material;
    uint frameTag; uint connectionId; uint verified; uint twoSided;
};
struct GIConnectionResult {
    GIPoint direction; float distance;
    float jacobian; float mappedSolidAnglePdf; uint valid; uint reserved;
};
GI_INLINE bool GIFinitePoint(GIPoint p) { return GIFinite(p.x)&&GIFinite(p.y)&&GIFinite(p.z); }
GI_INLINE GIPoint GIDelta(GIPoint a,GIPoint b) {
    GIPoint d; d.x=a.x-b.x; d.y=a.y-b.y; d.z=a.z-b.z; return d;
}
GI_INLINE float GIDot(GIPoint a,GIPoint b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
GI_INLINE GIConnectionResult GIEvaluateConnection(GIConnection c,uint frameTag) {
#ifdef __cplusplus
    GIConnectionResult r{};
#else
    GIConnectionResult r=(GIConnectionResult)0;
#endif
    // Bit 0: actual geometric normal; bit 1: verified source proposal PDF.
    // Bit 2: randomized finite-ray support accounted for in the estimator.
    // R278 consumer normals/weighted colors alone do NOT satisfy these flags.
    if((c.verified&7)!=7 || c.frameTag!=frameTag || c.material>=65534 || c.twoSided>1 ||
       !GIFinitePoint(c.source)||!GIFinitePoint(c.target)||!GIFinitePoint(c.secondary)||
       !GIFinitePoint(c.secondaryGeometricNormal)||!GIFinite(c.sourceSolidAnglePdf)||
       c.sourceSolidAnglePdf<=0 || !GIFinite(c.sourceMaxDistance)||c.sourceMaxDistance<=0 ||
       !GIFinite(c.targetMaxDistance)||c.targetMaxDistance<=0) return r;
    float n2=GIDot(c.secondaryGeometricNormal,c.secondaryGeometricNormal);
    if(!GIFinite(n2)||GIAbs(n2-1)>0.001f) return r;
    GIPoint sourceDelta=GIDelta(c.secondary,c.source), targetDelta=GIDelta(c.secondary,c.target);
    float sourceD2=GIDot(sourceDelta,sourceDelta), targetD2=GIDot(targetDelta,targetDelta);
    if(!GIFinite(sourceD2)||!GIFinite(targetD2)||sourceD2<=0||targetD2<=0) return r;
    float sourceDistance=GISqrt(sourceD2), targetDistance=GISqrt(targetD2);
    if(sourceDistance>c.sourceMaxDistance||targetDistance>c.targetMaxDistance) return r;
    float sourceCos=-GIDot(c.secondaryGeometricNormal,sourceDelta)/sourceDistance;
    float targetCos=-GIDot(c.secondaryGeometricNormal,targetDelta)/targetDistance;
    if(c.twoSided!=0){sourceCos=GIAbs(sourceCos);targetCos=GIAbs(targetCos);}
    // No grazing-angle clamp: it would change the probability transformation.
    if(sourceCos<=0||targetCos<=0) return r;
    float j=(targetCos/sourceCos)*(sourceD2/targetD2);
    float pdf=c.sourceSolidAnglePdf/j;
    if(!GIFinite(j)||j<=0||!GIFinite(pdf)||pdf<=0) return r;
    r.direction.x=targetDelta.x/targetDistance;r.direction.y=targetDelta.y/targetDistance;r.direction.z=targetDelta.z/targetDistance;
    r.distance=targetDistance;r.jacobian=j;r.mappedSolidAnglePdf=pdf;r.valid=1;
    return r;
}
#ifdef __cplusplus
static_assert(sizeof(GIConnection)==80);
static_assert(sizeof(GIConnectionResult)==32);
} // namespace gi_reuse
#endif
#undef GI_INLINE
