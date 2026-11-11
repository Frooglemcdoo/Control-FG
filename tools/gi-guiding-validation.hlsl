// Standalone WARP fixture. Not a production learning pipeline.
#include "../src/gi_reuse/world_directional_guide.h"
#include "../src/gi_reuse/guiding_contribution.h"
struct Frozen { GIDirectionalGuide guide;float energy[64];uint generation;uint frame; };
struct Input { GIPoint normal;float fraction;float u,v;uint guideIndex,generation;uint frame;uint3 reserved; };
struct Result { GIWorldProposal proposal;uint cell;float3 contribution;uint training; };
struct Learned { GIDirectionalGuide guide;float energy[64];uint parentGeneration,nextFrame,valid,count; };
struct Prepared {float rowCDF[8];float columnCDF[64];uint generation,frame;};
StructuredBuffer<Prepared> prepared : register(t2);
RWStructuredBuffer<Prepared> published : register(u2);
StructuredBuffer<Frozen> frozen : register(t0);
StructuredBuffer<Input> inputs : register(t1);
RWStructuredBuffer<Result> results : register(u0);
RWStructuredBuffer<Learned> learned : register(u1);
uint WorldCell(GIPoint d) {
 float u=GIWorldAtan2(d.y,d.x)*0.1591549430918953f;if(u<0)u+=1;
 return min(uint(u*8),7u)+8*min(uint((d.z+1)*4),7u);
}
// Read only the selected CDF entries. No 64-element local copy per ray.
float CellMass(uint guideIndex,uint cell){
 uint row=cell/8,col=cell%8;
 float rowLo=row?prepared[guideIndex].rowCDF[row-1]:0;
 float colLo=col?prepared[guideIndex].columnCDF[cell-1]:0;
 return (prepared[guideIndex].rowCDF[row]-rowLo)*(prepared[guideIndex].columnCDF[cell]-colLo);
}
GIWorldProposal FastSample(uint guideIndex,GIPoint n,float fraction,float u,float v){
 GIWorldProposal r=(GIWorldProposal)0;
 if(!GIFinitePoint(n)||GIAbs(GIDot(n,n)-1)>.0001f||!isfinite(fraction)||fraction<0||fraction>.5f||!isfinite(u)||!isfinite(v)||u<0||u>=1||v<0||v>=1)return r;
 float guideDensity=0;
 if(fraction>0&&u<fraction){
  float pick=u/fraction,rowLo=0;uint row=7;
  for(uint ri=0;ri<7;++ri){float hi=prepared[guideIndex].rowCDF[ri];if(pick<hi){row=ri;break;}rowLo=hi;}
  float rowMass=prepared[guideIndex].rowCDF[row]-rowLo;
  pick=(pick-rowLo)/rowMass;float colLo=0;uint col=7;
  for(uint ci=0;ci<7;++ci){float hi=prepared[guideIndex].columnCDF[row*8+ci];if(pick<hi){col=ci;break;}colLo=hi;}
  float colMass=prepared[guideIndex].columnCDF[row*8+col]-colLo;
  float x=min((float(col)+clamp((pick-colLo)/colMass,0,0.9999999403953552f))/8,0.9999999403953552f);
  float y=min((float(row)+v)/8,0.9999999403953552f);
  float z=clamp(2*y-1,-.9999999403953552f,.9999999403953552f),rad=sqrt(max(0,1-z*z)),angle=6.283185307179586f*x;
  r.direction.x=rad*cos(angle);r.direction.y=rad*sin(angle);r.direction.z=z;

 }else{
  float cosineV=v>0?v:0.0000000298023223876953125f;
  float angle=6.283185307179586f*((u-fraction)/(1-fraction)),rad=sqrt(1-cosineV),z=sqrt(cosineV);
  float x=rad*cos(angle),y=rad*sin(angle),sign=n.z>=0?1.0f:-1.0f,a=-1/(sign+n.z),b=n.x*n.y*a;
  float3 t=float3(1+sign*n.x*n.x*a,sign*b,-sign*n.x),bt=float3(b,sign+n.y*n.y*a,-n.y);
  float3 d=x*t+y*bt+z*float3(n.x,n.y,n.z);r.direction.x=d.x;r.direction.y=d.y;r.direction.z=d.z;
 }
 if(fraction>0)guideDensity=CellMass(guideIndex,WorldCell(r.direction))*5.092958178940651f;
 float cosine=max(0,GIDot(n,r.direction)),base=cosine*.3183098861837907f;
 r.pdf=(1-fraction)*base+fraction*guideDensity;
 if(!isfinite(r.pdf)||r.pdf<=0)return r;
 r.throughputWeight=base/r.pdf;r.trace=cosine>0?1u:0u;r.valid=1;return r;
}
[numthreads(64,1,1)]
void Sample(uint3 id:SV_DispatchThreadID){
 uint count,stride,guideCount;inputs.GetDimensions(count,stride);frozen.GetDimensions(guideCount,stride);
 if(id.x>=count)return;
 Result r=(Result)0;Input v=inputs[id.x];
 if(v.guideIndex>=guideCount){results[id.x]=r;return;}
 uint oldGeneration=prepared[v.guideIndex].generation,oldFrame=prepared[v.guideIndex].frame;
 if(oldGeneration!=v.generation||oldFrame!=v.frame){results[id.x]=r;return;}
 r.proposal=FastSample(v.guideIndex,v.normal,v.fraction,v.u,v.v);
 if(r.proposal.valid){
  r.cell=WorldCell(r.proposal.direction);
  // Analytic stand-in for a newly evaluated ray. Includes a nonzero fallback.
  float3 fresh=r.cell==63?float3(100,20,1):float3(.2,.3,.4);
  GIPoint throughput;throughput.x=throughput.y=throughput.z=1;
  GIPoint secondary;secondary.x=fresh.x;secondary.y=fresh.y;secondary.z=fresh.z;
  float nativePdf=max(0,GIDot(v.normal,r.proposal.direction))*.3183098861837907f;
  GIGuidedContribution evaluated=GIApplyGuideCorrection(throughput,secondary,nativePdf,r.proposal.pdf);
  r.contribution=float3(evaluated.rgb.x,evaluated.rgb.y,evaluated.rgb.z);
  r.training=evaluated.valid; // Includes legitimate zero terms below the hemisphere.
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
#ifdef GI_GUIDE_CURRENT_FRAME
 // Pilot rays are native. Their fresh contributions form THIS frame's table.
 for(uint writeCell=0;writeCell<64;++writeCell)next.energy[writeCell]=next.count?accumulated[writeCell]/next.count:0;
#else
 for(uint writeCell=0;writeCell<64;++writeCell)next.energy[writeCell]=next.count?old.energy[writeCell]*.8f+.2f*accumulated[writeCell]/next.count:0;
#endif
 next.guide=GIBuildDirectionalGuide(next.energy,.5f);
 next.parentGeneration=old.generation;next.nextFrame=old.frame+1;next.valid=next.count?1u:0u;
#ifdef GI_GUIDE_CURRENT_FRAME
 next.nextFrame=old.frame;
#endif
 learned[id.x]=next;
}
Prepared PrepareTable(GIDirectionalGuide guide,uint generation,uint frame){
 Prepared p=(Prepared)0;p.generation=generation;p.frame=frame;
 float total=0;for(uint sumCell=0;sumCell<64;++sumCell)total+=guide.mass[sumCell];
 float prefix=0;
 for(uint row=0;row<8;++row){
  float rowTotal=0;for(uint col=0;col<8;++col)rowTotal+=guide.mass[row*8+col];
  float colPrefix=0;for(uint writeCol=0;writeCol<8;++writeCol){colPrefix+=guide.mass[row*8+writeCol];p.columnCDF[row*8+writeCol]=colPrefix/rowTotal;}
  p.columnCDF[row*8+7]=1;prefix+=rowTotal;p.rowCDF[row]=prefix/total;
 }
 p.rowCDF[7]=1;return p;
}
[numthreads(1,1,1)]
void Prepare(uint3 id:SV_DispatchThreadID){
 uint count,stride;frozen.GetDimensions(count,stride);if(id.x>=count)return;
 Frozen old=frozen[id.x];published[id.x]=PrepareTable(old.guide,old.generation,old.frame);
}
[numthreads(1,1,1)]
void Publish(uint3 id:SV_DispatchThreadID){
 uint count,stride;frozen.GetDimensions(count,stride);if(id.x>=count)return;
 Frozen old=frozen[id.x];Learned n=learned[id.x];
 published[id.x]=PrepareTable(n.guide,old.generation+1,n.nextFrame);
}
