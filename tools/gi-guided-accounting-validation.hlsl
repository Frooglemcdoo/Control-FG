// Captured accounting boundary only. Does not replay material/light evaluation.
#include "../src/gi_reuse/guiding_contribution.h"
struct Ray {
 GIPoint throughput;uint material;
 GIPoint secondary;uint index;
 float nativePdf,guidedPdf;uint frame,generation;
};
struct Pixel {uint count;float scale;uint frame,generation;Ray rays[64];};
struct Output {float3 total;uint valid;float3 surface;uint hits;float3 miss;uint misses;float training;uint count;uint2 reserved;};
StructuredBuffer<Pixel> inputs:register(t0);
RWStructuredBuffer<Output> outputs:register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
 uint count,stride;inputs.GetDimensions(count,stride);if(id.x>=count)return;
 Pixel p=inputs[id.x];Output r=(Output)0;
 if(!p.count||p.count>64||!isfinite(p.scale)||p.scale<0){outputs[id.x]=r;return;}
 for(uint i=0;i<p.count;++i){
  Ray v=p.rays[i];
  GIGuidedContribution c=GIApplyGuideCorrection(v.throughput,v.secondary,v.nativePdf,v.guidedPdf);
  if(!c.valid||v.index!=i||v.frame!=p.frame||v.generation!=p.generation||
     (v.material>=65534&&v.material!=65535)){outputs[id.x]=(Output)0;return;}
  float3 rgb=float3(c.rgb.x,c.rgb.y,c.rgb.z);r.total+=rgb;r.training+=c.training;++r.count;
  if(v.material==65535){r.miss+=rgb;++r.misses;}else{r.surface+=rgb;++r.hits;}
 }
 r.total=(p.scale*r.total)/p.count;r.surface=(p.scale*r.surface)/p.count;r.miss=(p.scale*r.miss)/p.count;
 r.training/=p.count;
 if(!all(isfinite(r.total))||!all(isfinite(r.surface))||!all(isfinite(r.miss))||!isfinite(r.training)){outputs[id.x]=(Output)0;return;}
 r.valid=1;outputs[id.x]=r;
}
