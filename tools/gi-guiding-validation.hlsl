// Standalone WARP fixture. Not a production learning pipeline.
#include "../src/gi_reuse/world_directional_guide.h"
struct Frozen { GIDirectionalGuide guide;float energy[64];uint generation;uint frame; };
struct Input { GIPoint normal;float fraction;float u,v;uint guideIndex,generation;uint frame;uint3 reserved; };
struct Result { GIWorldProposal proposal;uint cell;float3 contribution;uint training; };
struct Learned { GIDirectionalGuide guide;float energy[64];uint parentGeneration,nextFrame,valid,count; };
StructuredBuffer<Frozen> frozen : register(t0);
StructuredBuffer<Input> inputs : register(t1);
RWStructuredBuffer<Result> results : register(u0);
RWStructuredBuffer<Learned> learned : register(u1);
uint WorldCell(GIPoint d) {
 float u=GIWorldAtan2(d.y,d.x)*0.1591549430918953f;if(u<0)u+=1;
 return min(uint(u*8),7u)+8*min(uint((d.z+1)*4),7u);
}
[numthreads(64,1,1)]
void Sample(uint3 id:SV_DispatchThreadID){
 uint count,stride,guideCount;inputs.GetDimensions(count,stride);frozen.GetDimensions(guideCount,stride);
 if(id.x>=count)return;
 Result r=(Result)0;Input v=inputs[id.x];
 if(v.guideIndex>=guideCount){results[id.x]=r;return;}
 Frozen old=frozen[v.guideIndex];
 if(old.generation!=v.generation||old.frame!=v.frame){results[id.x]=r;return;}
 r.proposal=GISampleWorldGuide(old.guide,v.normal,v.fraction,v.u,v.v);
 if(r.proposal.valid){
  r.cell=WorldCell(r.proposal.direction);
  // Analytic stand-in for a newly evaluated ray. Includes a nonzero fallback.
  float3 fresh=r.cell==63?float3(100,20,1):float3(.2,.3,.4);
  r.contribution=fresh*r.proposal.throughputWeight;
  r.training=1; // Includes legitimate zero terms below the hemisphere.
 }
 results[id.x]=r;
}
[numthreads(1,1,1)]
void Learn(uint3 id:SV_DispatchThreadID){
 uint guideCount,stride,count;frozen.GetDimensions(guideCount,stride);inputs.GetDimensions(count,stride);
 if(id.x>=guideCount)return;
 Learned next=(Learned)0;Frozen old=frozen[id.x];float accumulated[64];
 for(uint k=0;k<64;++k)accumulated[k]=0;
 for(uint j=0;j<count;++j){
  Input v=inputs[j];Result r=results[j];
  if(v.guideIndex!=id.x||v.frame!=old.frame||v.generation!=old.generation||!r.training)continue;
  accumulated[r.cell]+=dot(r.contribution,float3(.2126,.7152,.0722));++next.count;
 }
 for(uint writeCell=0;writeCell<64;++writeCell)next.energy[writeCell]=next.count?old.energy[writeCell]*.8f+.2f*accumulated[writeCell]/next.count:0;
 next.guide=GIBuildDirectionalGuide(next.energy,.5f);
 next.parentGeneration=old.generation;next.nextFrame=old.frame+1;next.valid=next.count?1u:0u;
 learned[id.x]=next;
}
RWStructuredBuffer<Frozen> published : register(u2);
[numthreads(1,1,1)]
void Publish(uint3 id:SV_DispatchThreadID){
 uint count,stride;frozen.GetDimensions(count,stride);if(id.x>=count)return;
 Frozen old=frozen[id.x];Learned n=learned[id.x];Frozen ready=(Frozen)0;
 // Empty learning publishes a uniform proposal, never uninitialized memory.
 ready.guide=n.guide;for(uint copyCell=0;copyCell<64;++copyCell)ready.energy[copyCell]=n.energy[copyCell];
 ready.generation=old.generation+1;ready.frame=n.nextFrame;published[id.x]=ready;
}
