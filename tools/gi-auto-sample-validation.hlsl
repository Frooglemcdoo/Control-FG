#include "../src/shaders/gi_auto_sample.hlsl"
#include "../src/gi_reuse/pilot_guide_resolve.h"
RWStructuredBuffer<float4> output:register(u0);
static const uint count=32768;
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint scenario=id.x/count,index=id.x%count;
 if(scenario>=6)return;
 GIAutoTargets targets=(GIAutoTargets)0;
 targets.schema=0x41544731;targets.version=7;targets.frame=12;targets.validMask=3;
 targets.bounds[0].center.z=3;targets.bounds[0].radius=.5;
 targets.bounds[1].center.x=1;targets.bounds[1].center.z=2;targets.bounds[1].radius=.3;
 targets.counts[0]=targets.counts[1]=42;targets.scores[0]=targets.scores[1]=123;
 GIAutoSelection selection;selection.version=7;selection.sourceFrame=12;selection.rayCount=10;selection.fraction=.1;
 if(scenario==1){targets.bounds[0].center.z=-3;targets.bounds[1].center.z=-2;}
 if(scenario==2)targets.validMask=0;
 if(scenario==3)selection.sourceFrame=13;
 if(scenario==4)targets.validMask=2;
 if(scenario==5)targets.bounds[1].radius=asfloat(0x7fc00000);
 float alpha=GIAutoAlpha(targets,selection);
 GIPilotResolve resolve=(GIPilotResolve)0;float3 sum=0;uint valid=1;
 [loop]for(uint i=0;i<10;++i){
  GIAutoDraw draw=GIAutoSample(uint2(index%256,index/256),897,0,i,0,float3(0,0,1),targets,selection);
  // Closed-form cosine integrals: constant 4, E[z^2]=1/2, P[z>.8]=.36.
  GIPoint rgb;rgb.x=4;rgb.y=draw.direction.z*draw.direction.z;rgb.z=draw.direction.z>.8?1:0;
  if(alpha==0)sum+=float3(rgb.x,rgb.y,rgb.z);
  else if(i==0)resolve=GIPilotBegin(rgb,10,alpha,7);
  else resolve=GIPilotAdd(resolve,rgb,draw.weight,7);
  if(!all(isfinite(draw.direction))||abs(dot(draw.direction,draw.direction)-1)>.00001||
     draw.weight<0||draw.weight>1.111114||draw.range<0||draw.range>=1||
     (!draw.traceRay&&draw.weight!=0))valid=0;
  if(index==0){output[6*count+scenario*20+i*2]=float4(draw.direction,draw.weight);
   output[6*count+scenario*20+i*2+1]=float4(draw.range,draw.traceRay,alpha,0);}
 }
 float3 result=sum/10;
 if(alpha>0){GIPilotResult resolved=GIPilotFinish(resolve);result=float3(resolved.rgb.x,resolved.rgb.y,resolved.rgb.z);valid&=resolved.valid;}
 output[id.x]=float4(result,valid);
}
