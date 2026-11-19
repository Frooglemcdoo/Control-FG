#ifndef CONTROL_GI_AUTO_SAMPLE_H
#define CONTROL_GI_AUTO_SAMPLE_H
#include "../gi_reuse/auto_target_binding.h"
#include "../gi_reuse/producer_sampling.h"
#include "gi_guided_target.hlsl"
// Paired ten-ray baseline and four-ray candidate, both single-batch.
// The caller must freeze one target resource for producer AND evaluator.
float GIAutoAlpha(GIAutoTargets targets,GIAutoSelection selection) {
 return ((selection.rayCount==10&&selection.fraction==.2f)||(selection.rayCount==4&&selection.fraction==.25f))&&GIAutoUsable(targets,selection)?selection.fraction:0;
}
uint GIAutoSeed(uint2 pixel,uint frame,uint batch,uint ray) {
 return pixel.x*73856093u^pixel.y*19349663u^frame*83492791u^
        batch*2654435761u^ray*1597334677u;
}
// Exact R307 + owned strata permutation; four-ray progressive mode is excluded
// from this ten-ray fallback. Four rays use their own complete strata.
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
// Receiver/guide invariants are prepared once outside the per-ray loop.
struct GIAutoPrepared {GTCone a,b;float alpha;};
GIAutoPrepared GIAutoPrepare(float3 origin,float3 normal,GIAutoTargets targets,GIAutoSelection selection) {
 GIAutoPrepared p=(GIAutoPrepared)0;p.alpha=GIAutoAlpha(targets,selection);
 if(p.alpha>0){
  uint first=(targets.validMask&1)?0:1;
  uint second=targets.validMask==3?1:first;
  GTBound a,b;
  a.center=float3(targets.bounds[first].center.x,targets.bounds[first].center.y,targets.bounds[first].center.z);a.radius=targets.bounds[first].radius;
  b.center=float3(targets.bounds[second].center.x,targets.bounds[second].center.y,targets.bounds[second].center.z);b.radius=targets.bounds[second].radius;
  p.a=GTMakeCone(origin,a);p.b=GTMakeCone(origin,b);
  // Receiver-specific proposal eligibility, decided before any ray is drawn.
  // Shared regional keys do not guarantee that targets face this exact normal.
  // Keep only front-centered bounds outside the receiver; retain full cosine
  // support for every rejected direction. Never reject or clamp a shaded hit.
  bool useA=p.a.oneMinusCos<1&&dot(normal,p.a.axis)>0;
  bool useB=p.b.oneMinusCos<1&&dot(normal,p.b.axis)>0;
  if(!useA&&!useB)p.alpha=0;
  else if(!useA)p.a=p.b;
  else if(!useB)p.b=p.a;
 }
 return p;
}
struct GIAutoDraw {float3 direction;float weight,range;uint traceRay;};
GIAutoDraw GIAutoSamplePrepared(uint2 pixel,uint frame,uint batch,uint ray,
                      float3 normal,GIAutoPrepared prepared,GIAutoSelection selection) {
 // Both stages use the invocation seed, never independently sourced native CB clocks.
 frame=selection.sampleFrame;
 GIAutoDraw result=(GIAutoDraw)0;
 uint seed=GIAutoSeed(pixel,frame,batch,ray);
 result.range=GIProducerUnit(seed,0x967a889bu);
 float alpha=prepared.alpha;
 if(alpha==0) {
  float2 uv;
  if(selection.rayCount==4){GIProducerCoordinates nativeDraw=GIProducerSample(pixel.x,pixel.y,frame,batch,ray,4);uv=float2(nativeDraw.u,nativeDraw.v);}
  else uv=GIAutoOwned10(pixel,frame,batch,ray,seed);
  result.direction=GTLocal(normal,sqrt(uv.y),2*GT_PI*uv.x);
  result.weight=1;result.traceRay=1;return result;
 }
 // Reserve most rays for full-hemisphere coverage: 1 guided + 3 cosine at
 // four rays, 2 guided + 8 cosine at ten. The single four-ray guide sample
 // selects either cone with equal probability; never always pick one cone.
 float u=min((float(ray)+GIProducerUnit(seed,0x68bc21ebu))/float(selection.rayCount),.9999999403953552f);
 float v=GIProducerUnit(seed,0x02e5be93u);
 uint guidedCount=selection.rayCount==4?1:2;
 uint component=ray<guidedCount?ray:2;
 uint size=ray<guidedCount?1:selection.rayCount-guidedCount;
 uint local=ray<guidedCount?0:ray-guidedCount;
 uint rotation=GIProducerHash(pixel.x*0x8da6b343u^pixel.y*0xd8163841u^
                   frame*0xcb1ab31fu^batch*0x165667b1u^component*0x9e3779b9u)%size;
 v=(float((local+rotation)%size)+v)/float(size);

 GTDraw draw=GTSamplePrepared(normal,prepared.a,prepared.b,alpha,u,v);
 result.direction=draw.direction;result.weight=draw.radianceWeight;result.traceRay=draw.traceRay;return result;
}
GIAutoDraw GIAutoSample(uint2 pixel,uint frame,uint batch,uint ray,float3 origin,
                      float3 normal,GIAutoTargets targets,GIAutoSelection selection) {
 return GIAutoSamplePrepared(pixel,frame,batch,ray,normal,GIAutoPrepare(origin,normal,targets,selection),selection);
}
#endif
