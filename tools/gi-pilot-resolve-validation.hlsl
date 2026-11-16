#include "../src/gi_reuse/pilot_guide_resolve.h"
// Each fixture has controls, pilot, then up to 63 RGB/weight records.
StructuredBuffer<float4> input : register(t0);
RWStructuredBuffer<float4> output : register(u0);
[numthreads(1,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint base=id.x*65;float4 control=input[base];uint n=uint(control.x),version=asuint(control.z);
 GIPoint pilot;pilot.x=input[base+1].x;pilot.y=input[base+1].y;pilot.z=input[base+1].z;
 GIPilotResolve s=GIPilotBegin(pilot,n,control.y,version);
 [loop]for(uint i=1;i<n&&i<64;++i){
  float4 sample=input[base+1+i];GIPoint rgb;rgb.x=sample.x;rgb.y=sample.y;rgb.z=sample.z;
  s=GIPilotAdd(s,rgb,sample.w,version);
 }
 GIPilotResult r=GIPilotFinish(s);output[id.x]=float4(r.rgb.x,r.rgb.y,r.rgb.z,float(r.valid));
}
