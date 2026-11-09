#pragma once
#include "connection.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_GEOMETRY_INLINE inline
#else
#define GI_GEOMETRY_INLINE
#endif
struct GITrueHit {
    GIPoint position; uint valid;
    GIPoint geometricNormal; uint reserved;
};
GI_GEOMETRY_INLINE GIPoint GICross(GIPoint a,GIPoint b) {
    GIPoint r;r.x=a.y*b.z-a.z*b.y;r.y=a.z*b.x-a.x*b.z;r.z=a.x*b.y-a.y*b.x;return r;
}
// Caller must supply the current, accepted triangle's WORLD positions, including
// geometry and instance transforms. Never substitute interpolated vertex normals.
// Do not face-forward here: preserve winding for the material/two-sided policy.
GI_GEOMETRY_INLINE GITrueHit GIRecoverTrueHit(GIPoint origin,GIPoint direction,float rayT,
                                            GIPoint v0,GIPoint v1,GIPoint v2) {
#ifdef __cplusplus
    GITrueHit r{};
#else
    GITrueHit r=(GITrueHit)0;
#endif
    if(!GIFinitePoint(origin)||!GIFinitePoint(direction)||!GIFinite(rayT)||rayT<0||
       !GIFinitePoint(v0)||!GIFinitePoint(v1)||!GIFinitePoint(v2))return r;
    float d2=GIDot(direction,direction);
    if(!GIFinite(d2)||GIAbs(d2-1)>0.001f)return r;
    GIPoint e1=GIDelta(v1,v0),e2=GIDelta(v2,v0);
    // Scale edges first to avoid overflow for large triangles or underflow for
    // small triangles. This changes neither the normal direction nor winding.
    float s=GIAbs(e1.x);
    if(GIAbs(e1.y)>s)s=GIAbs(e1.y);
    if(GIAbs(e1.z)>s)s=GIAbs(e1.z);
    if(GIAbs(e2.x)>s)s=GIAbs(e2.x);
    if(GIAbs(e2.y)>s)s=GIAbs(e2.y);
    if(GIAbs(e2.z)>s)s=GIAbs(e2.z);
    if(!GIFinite(s)||s<=0)return r;
    e1.x/=s;e1.y/=s;e1.z/=s;e2.x/=s;e2.y/=s;e2.z/=s;
    GIPoint n=GICross(e1,e2);float n2=GIDot(n,n);
    if(!GIFinite(n2)||n2<=0)return r;
    float length=GISqrt(n2);n.x/=length;n.y/=length;n.z/=length;
    GIPoint p;p.x=origin.x+direction.x*rayT;p.y=origin.y+direction.y*rayT;p.z=origin.z+direction.z*rayT;
    if(!GIFinitePoint(p)||!GIFinitePoint(n))return r;
    r.position=p;r.geometricNormal=n;r.valid=1;return r;
}
#ifdef __cplusplus
static_assert(sizeof(GITrueHit)==32);
} // namespace gi_reuse
#endif
#undef GI_GEOMETRY_INLINE
