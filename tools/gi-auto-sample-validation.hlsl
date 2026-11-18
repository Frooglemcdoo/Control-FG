#include "../src/shaders/gi_auto_sample.hlsl"
RWStructuredBuffer<float4> output:register(u0);
static const uint count=32768;
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint testCase=id.x/count,scenario=testCase%12,index=id.x%count;
 if(testCase>=24)return;
 GIAutoTargets targets=(GIAutoTargets)0;
 targets.schema=0x41544731;targets.version=7;targets.frame=12;targets.validMask=3;
 targets.bounds[0].center.z=3;targets.bounds[0].radius=.5;
 targets.bounds[1].center.x=1;targets.bounds[1].center.z=2;targets.bounds[1].radius=.3;
 targets.counts[0]=targets.counts[1]=42;targets.scores[0]=targets.scores[1]=123;
 GIAutoSelection selection;selection.version=7;selection.sourceFrame=12;selection.rayCount=10;selection.fraction=.4;selection.sampleFrame=897;
 if(testCase>=12){selection.rayCount=4;selection.fraction=.5;}
 if(scenario==1){targets.bounds[0].center.z=-3;targets.bounds[1].center.z=-2;}
 if(scenario==2)targets.validMask=0;
 if(scenario==3)selection.sourceFrame=13;
 if(scenario==4)targets.validMask=2;
 if(scenario==5)targets.bounds[1].radius=asfloat(0x7fc00000);
 // Narrow source has cosine probability .0001. Cases 6/8 align the guide,
 // case 7 moves the guide away, and case 9 blocks the source completely.
 if(scenario>=6){
  float3 axis=scenario==8?float3(.6,0,.8):float3(0,0,1);
  if(scenario==7)axis=float3(.6,0,.8);
  targets.bounds[0].center.x=axis.x*3;targets.bounds[0].center.z=axis.z*3;targets.bounds[0].radius=.03;
  if(scenario==10)targets.bounds[0].radius=.4330127;
  if(scenario==11)targets.bounds[0].radius=.25;
  targets.bounds[1]=targets.bounds[0];
 }
 float alpha=GIAutoAlpha(targets,selection);
 float3 sum=0;uint valid=1;
 [loop]for(uint i=0;i<selection.rayCount;++i){
  GIAutoDraw draw=GIAutoSample(uint2(index%256,index/256),897,0,i,0,float3(0,0,1),targets,selection);
  GIAutoDraw other=GIAutoSample(uint2(index%256,index/256),901,0,i,0,float3(0,0,1),targets,selection);
  if(any(draw.direction!=other.direction)||draw.weight!=other.weight||draw.range!=other.range||draw.traceRay!=other.traceRay)valid=0;
  GIAutoSelection next=selection;next.sampleFrame=898;
  GIAutoDraw fresh=GIAutoSample(uint2(index%256,index/256),897,0,i,0,float3(0,0,1),targets,next);
  if(all(draw.direction==fresh.direction)&&draw.range==fresh.range)valid=0;
  // Closed-form cosine integrals: constant 4, E[z^2]=1/2, P[z>.8]=.36.
  GIPoint rgb;rgb.x=4;rgb.y=draw.direction.z*draw.direction.z;rgb.z=draw.direction.z>.8?1:0;
  if(scenario>=6){
   float3 lightAxis=scenario==8?float3(.6,0,.8):float3(0,0,1);
   rgb.y=dot(draw.direction,lightAxis)>=sqrt(1-.0001)?10000:0;
   if(scenario==9)rgb.y=0; // fresh visibility: no retained lighting
   rgb.z=draw.traceRay?1:0;
  }
  sum+=float3(rgb.x,rgb.y,rgb.z)*draw.weight;
  if(!all(isfinite(draw.direction))||abs(dot(draw.direction,draw.direction)-1)>.00001||
     draw.weight<0||draw.weight>(selection.rayCount==4?2.00001:1.66667)||draw.range<0||draw.range>=1||
     (!draw.traceRay&&draw.weight!=0))valid=0;
  if(index==0){output[24*count+testCase*20+i*2]=float4(draw.direction,draw.weight);
   output[24*count+testCase*20+i*2+1]=float4(draw.range,draw.traceRay,alpha,0);}
 }
 float3 result=sum/float(selection.rayCount);
 output[id.x]=float4(result,valid);
}
