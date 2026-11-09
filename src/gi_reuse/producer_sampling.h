#ifndef CONTROL_GI_PRODUCER_SAMPLING_H
#define CONTROL_GI_PRODUCER_SAMPLING_H
#include "connection.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_SAMPLER_INLINE inline
#else
#define GI_SAMPLER_INLINE
#endif
struct GIProducerCoordinates {float u,v,range,selection;uint valid;};
GI_SAMPLER_INLINE uint GIProducerHash(uint v){v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;v^=v>>16;return v;}
GI_SAMPLER_INLINE float GIProducerUnit(uint seed,uint salt){return float(GIProducerHash(seed^salt)>>8)*0.000000059604644775390625f;}
// Matches R307's pixel/frame/batch/ray hash and complete equal-area strata.
// The fourth dimension is independent reservoir selection, not the range draw.
GI_SAMPLER_INLINE GIProducerCoordinates GIProducerSample(uint x,uint y,uint frame,uint batch,uint ray,uint count){
#ifdef __cplusplus
    GIProducerCoordinates r{};
#else
    GIProducerCoordinates r=(GIProducerCoordinates)0;
#endif
    if(!count||count>64||ray>=count)return r;
    uint seed=(x*73856093u)^(y*19349663u)^(frame*83492791u)^(batch*2654435761u)^(ray*1597334677u);
    uint columns=(count&1u)?1u:2u,rows=count/columns;
    r.u=(float(ray%columns)+GIProducerUnit(seed,0x68bc21ebu))/float(columns);
    r.v=(float(ray/columns)+GIProducerUnit(seed,0x02e5be93u))/float(rows);
    // Division/addition may round the final stratum to one.
    if(r.u>0.9999999403953552f)r.u=0.9999999403953552f;
    if(r.v>0.9999999403953552f)r.v=0.9999999403953552f;
    r.range=GIProducerUnit(seed,0x967a889bu);r.selection=GIProducerUnit(seed,0x368cc8b7u);r.valid=1;return r;
}
#ifdef __cplusplus
}
#endif
#undef GI_SAMPLER_INLINE
#endif
