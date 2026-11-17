#ifndef CONTROL_GI_AUTO_SAMPLE_H
#define CONTROL_GI_AUTO_SAMPLE_H
#include "../gi_reuse/auto_target_binding.h"
#include "../gi_reuse/producer_sampling.h"
#include "gi_guided_target.hlsl"
// First live candidate is deliberately restricted to the captured ten-ray ABI.
// The caller must freeze one target resource for producer AND evaluator.
float GIAutoAlpha(GIAutoTargets targets,GIAutoSelection selection) {
 return selection.rayCount==10&&GIAutoUsable(targets,selection)?selection.fraction:0;
}
uint GIAutoSeed(uint2 pixel,uint frame,uint batch,uint ray) {
 return pixel.x*73856093u^pixel.y*19349663u^frame*83492791u^
        batch*2654435761u^ray*1597334677u;
}
// Exact R307 + owned strata permutation; four-ray progressive mode is excluded
// by ten-ray admission. Range remains the original third hash dimension.
float2 GIAutoOwned10(uint2 pixel,uint frame,uint batch,uint ray,uint seed) {
 uint column=ray%2,row=ray/2;
 uint key=(pixel.x*73856093u^pixel.y*19349663u^batch*2654435761u)^((frame>>2)*2654435761u);
 uint hash=(key^(key>>16))*2146121005u;
 uint xbin=((hash>>12)+column*73u+row*67u)%5u;
 uint ybin=((hash>>23)^row^column)%2u;
 float u=(float(column)+(float(xbin)+GIProducerUnit(seed,0x68bc21ebu))/5.0f)/2.0f;
 float v=(float(row)+(float(ybin)+GIProducerUnit(seed,0x02e5be93u))/2.0f)/5.0f;
 return min(float2(u,v),.9999999403953552f);
}
struct GIAutoDraw {float3 direction;float weight,range;uint traceRay;};
GIAutoDraw GIAutoSample(uint2 pixel,uint frame,uint batch,uint ray,float3 origin,
                      float3 normal,GIAutoTargets targets,GIAutoSelection selection) {
 GIAutoDraw result=(GIAutoDraw)0;
 uint seed=GIAutoSeed(pixel,frame,batch,ray);
 result.range=GIProducerUnit(seed,0x967a889bu);
 float alpha=GIAutoAlpha(targets,selection);
 if(alpha==0) {
  float2 uv=GIAutoOwned10(pixel,frame,batch,ray,seed);
  result.direction=GTLocal(normal,sqrt(uv.y),2*GT_PI*uv.x);
  result.weight=1;result.traceRay=1;return result;
 }
 // Stratify the mixture selector over all ten rays. At alpha=.4 this gives
 // two samples per target cone and six cosine samples, with no noisy pilot.
 // Each stratum has a fresh independent range draw and within-stratum jitter.
 float u=min((float(ray)+GIProducerUnit(seed,0x68bc21ebu))/10.0f,.9999999403953552f);
 float v=GIProducerUnit(seed,0x02e5be93u);
 // Latin stratification within each fixed component preserves the component's
 // full radial marginal while avoiding six independent cosine elevations.
 // The shared random rotation is independent of per-ray angular/range jitter.
 if(alpha==.4f){
  uint component=ray<2?0:(ray<4?1:2);
  uint size=component==2?6:2;
  uint local=component==2?ray-4:ray-component*2;
  uint rotation=GIProducerHash(pixel.x*0x8da6b343u^pixel.y*0xd8163841u^
                    frame*0xcb1ab31fu^batch*0x165667b1u^component*0x9e3779b9u)%size;
  v=(float((local+rotation)%size)+v)/float(size);
 }

 uint first=(targets.validMask&1)?0:1;
 uint second=targets.validMask==3?1:first;
 GTBound a,b;
 a.center=float3(targets.bounds[first].center.x,targets.bounds[first].center.y,targets.bounds[first].center.z);a.radius=targets.bounds[first].radius;
 b.center=float3(targets.bounds[second].center.x,targets.bounds[second].center.y,targets.bounds[second].center.z);b.radius=targets.bounds[second].radius;
 GTDraw draw=GTSampleMixture(origin,normal,a,b,alpha,true,u,v);
 result.direction=draw.direction;result.weight=draw.radianceWeight;result.traceRay=draw.traceRay;return result;
}
#endif
