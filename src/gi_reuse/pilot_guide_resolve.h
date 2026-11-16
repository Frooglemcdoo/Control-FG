#ifndef CONTROL_GI_PILOT_GUIDE_RESOLVE_H
#define CONTROL_GI_PILOT_GUIDE_RESOLVE_H
// Research-only; not wired into the game. Frozen proposal must precede all draws.
// One independent native cosine pilot, n-1 fresh q draws. Complete RGB includes
// miss/volume. Weight = p/q; alpha <= 1/n. Native range law stays independent.
#include "connection.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_PILOT_INLINE inline
#else
#define GI_PILOT_INLINE
#endif
struct GIPilotResolve {
 GIPoint pilot; GIPoint weighted;
 float sumWeight,maxWeight;
 uint expected,added,version,valid;
};
struct GIPilotResult {GIPoint rgb;uint valid;};
GI_PILOT_INLINE bool GIPilotColorValid(GIPoint c) {
 return GIFinitePoint(c)&&c.x>=0&&c.y>=0&&c.z>=0;
}
GI_PILOT_INLINE GIPilotResolve GIPilotBegin(GIPoint pilot,uint rays,float alpha,uint version) {
#ifdef __cplusplus
 GIPilotResolve s{};
#else
 GIPilotResolve s=(GIPilotResolve)0;
#endif
 if(rays<2||rays>64||!GIPilotColorValid(pilot)||!GIFinite(alpha)||alpha<0||alpha>1.0f/float(rays))return s;
 s.pilot=pilot;s.expected=rays-1;s.version=version;s.maxWeight=1/(1-alpha);s.valid=1;return s;
}
// Invalid/mismatched input poisons the result. Never undo a guided draw's weight
// based on observed brightness; a live caller must handle an invalid result explicitly.
GI_PILOT_INLINE GIPilotResolve GIPilotAdd(GIPilotResolve s,GIPoint rgb,float weight,uint version) {
 if(!s.valid)return s;
 if(version!=s.version||s.added>=s.expected||!GIPilotColorValid(rgb)||!GIFinite(weight)||weight<0||weight>s.maxWeight+0.000002f){s.valid=0;return s;}
 s.weighted.x+=rgb.x*weight;s.weighted.y+=rgb.y*weight;s.weighted.z+=rgb.z*weight;s.sumWeight+=weight;++s.added;
 if(!GIFinitePoint(s.weighted)||!GIFinite(s.sumWeight))s.valid=0;
 return s;
}
GI_PILOT_INLINE GIPilotResult GIPilotFinish(GIPilotResolve s) {
#ifdef __cplusplus
 GIPilotResult r{};
#else
 GIPilotResult r=(GIPilotResult)0;
#endif
 if(!s.valid||s.added!=s.expected)return r;
 float count=float(s.expected+1),inverse=1/count;
 if(s.sumWeight>count+0.0001f)return r;
 // Correct only an accumulated floating-point overshoot of the theoretical bound.
 if(s.sumWeight>count)inverse=1/s.sumWeight;
 float pilotCoefficient=1-s.sumWeight*inverse;
 if(pilotCoefficient<0)pilotCoefficient=0;
 r.rgb.x=pilotCoefficient*s.pilot.x+inverse*s.weighted.x;
 r.rgb.y=pilotCoefficient*s.pilot.y+inverse*s.weighted.y;
 r.rgb.z=pilotCoefficient*s.pilot.z+inverse*s.weighted.z;
 r.valid=GIPilotColorValid(r.rgb)?1:0;return r;
}
#ifdef __cplusplus
}
#endif
#undef GI_PILOT_INLINE
#endif
