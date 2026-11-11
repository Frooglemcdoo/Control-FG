// Captured accounting boundary only. Does not replay material/light evaluation.
#include "../src/gi_reuse/guiding_contribution.h"
struct Ray {
 GIPoint throughput;uint material;
 GIPoint secondary;uint index;
 float nativePdf,guidedPdf;uint frame,generation;
};
struct Output {float3 total;uint valid;float3 surface;uint hits;float3 miss;uint misses;float training;uint count;uint2 reserved;};
// Scalar word storage avoids a >2048-byte structured-buffer element at 64 rays.
StructuredBuffer<uint> inputs:register(t0);
RWStructuredBuffer<Output> outputs:register(u0);
Ray ReadRay(uint a){
 Ray r;
 r.throughput.x=asfloat(inputs[a]);r.throughput.y=asfloat(inputs[a+1]);r.throughput.z=asfloat(inputs[a+2]);r.material=inputs[a+3];
 r.secondary.x=asfloat(inputs[a+4]);r.secondary.y=asfloat(inputs[a+5]);r.secondary.z=asfloat(inputs[a+6]);r.index=inputs[a+7];
 r.nativePdf=asfloat(inputs[a+8]);r.guidedPdf=asfloat(inputs[a+9]);r.frame=inputs[a+10];r.generation=inputs[a+11];return r;
}
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
 uint words,stride;inputs.GetDimensions(words,stride);if(id.x>=words/772)return;
 uint start=id.x*772,count=inputs[start],frame=inputs[start+2],generation=inputs[start+3];float scale=asfloat(inputs[start+1]);Output r=(Output)0;
 if(!count||count>64||!isfinite(scale)||scale<0){outputs[id.x]=r;return;}
 for(uint i=0;i<count;++i){
  Ray v=ReadRay(start+4+i*12);
  GIGuidedContribution c=GIApplyGuideCorrection(v.throughput,v.secondary,v.nativePdf,v.guidedPdf);
  if(!c.valid||v.index!=i||v.frame!=frame||v.generation!=generation||
     (v.material>=65534&&v.material!=65535)){outputs[id.x]=(Output)0;return;}
  float3 rgb=float3(c.rgb.x,c.rgb.y,c.rgb.z);r.total+=rgb;r.training+=c.training;++r.count;
  if(v.material==65535){r.miss+=rgb;++r.misses;}else{r.surface+=rgb;++r.hits;}
 }
 r.total=(scale*r.total)/count;r.surface=(scale*r.surface)/count;r.miss=(scale*r.miss)/count;
 r.training/=count;
 if(!all(isfinite(r.total))||!all(isfinite(r.surface))||!all(isfinite(r.miss))||!isfinite(r.training)){outputs[id.x]=(Output)0;return;}
 r.valid=1;outputs[id.x]=r;
}
