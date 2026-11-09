#ifndef CONTROL_RR_HIT_DISTANCE_SHARED_H
#define CONTROL_RR_HIT_DISTANCE_SHARED_H
#ifdef __cplusplus
#include <cmath>
using uint = unsigned int;
static bool RRHDIsFinite(float x) { return std::isfinite(x); }
static float RRHDSqrt(float x) { return std::sqrt(x); }
static float RRHDAbs(float x) { return std::fabs(x); }
#else
bool RRHDIsFinite(float x) { return isfinite(x); }
float RRHDSqrt(float x) { return sqrt(x); }
float RRHDAbs(float x) { return abs(x); }
#endif
// Status is required. A zero stored distance for a miss/no-ray is NOT an RR miss policy.
static const uint RRHDInvalid = 0;
static const uint RRHDHit = 1;
static const uint RRHDMiss = 2;
static const uint RRHDNoRay = 3;
static const uint RRHDEndMarker = 65534;
static const uint RRHDMissMarker = 65535;
struct RRHDVector { float x; float y; float z; };
// Explicit rows of the linear view-to-world transform; translation cancels.
struct RRHDBasis { RRHDVector row0; RRHDVector row1; RRHDVector row2; };
struct RRHDResult { float distance; uint status; };
uint RRHDClassify(uint materialId) {
    if(materialId == RRHDEndMarker) return RRHDNoRay;
    if(materialId == RRHDMissMarker) return RRHDMiss;
    return materialId < RRHDEndMarker ? RRHDHit : RRHDInvalid;
}
float RRHDDot(RRHDVector a, RRHDVector b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
RRHDResult RRHDFromView(uint materialId, RRHDVector primary, RRHDVector hit, RRHDBasis basis) {
    RRHDResult result; result.distance=0; result.status=RRHDClassify(materialId);
    if(result.status != RRHDHit) return result;
    result.status=RRHDInvalid;
    if(!RRHDIsFinite(primary.x)||!RRHDIsFinite(primary.y)||!RRHDIsFinite(primary.z)||
       !RRHDIsFinite(hit.x)||!RRHDIsFinite(hit.y)||!RRHDIsFinite(hit.z)) return result;
    RRHDVector delta;delta.x=hit.x-primary.x;delta.y=hit.y-primary.y;delta.z=hit.z-primary.z;
    float x=RRHDDot(basis.row0,delta),y=RRHDDot(basis.row1,delta),z=RRHDDot(basis.row2,delta);
    if(!RRHDIsFinite(x)||!RRHDIsFinite(y)||!RRHDIsFinite(z)) return result;
    float scale=RRHDAbs(x);if(RRHDAbs(y)>scale)scale=RRHDAbs(y);if(RRHDAbs(z)>scale)scale=RRHDAbs(z);
    float distance=0;
    if(scale>0) {x/=scale;y/=scale;z/=scale;distance=scale*RRHDSqrt(x*x+y*y+z*z);}
    if(!RRHDIsFinite(distance))return result;
    result.distance=distance;result.status=RRHDHit;return result;
}
#endif
