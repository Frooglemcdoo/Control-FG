#include "../gi_reuse/auto_target_contract.h"
StructuredBuffer<GIAutoTraining> training:register(t0);
globallycoherent RWStructuredBuffer<GIAutoCell> cells:register(u0);
RWStructuredBuffer<GIAutoTargets> targets:register(u1);
cbuffer Controls:register(b0){uint recordCount,cellCount,version,frame;float cellSize,scoreScale;uint minimumCount,reserved;};
// At most 65535 records * score <=65535: no uint accumulation overflow.
bool ControlsValid(){return recordCount<=65535&&cellCount>=64&&cellCount<=4096&&version>0&&frame>0&&minimumCount>0&&minimumCount<=65535&&isfinite(cellSize)&&cellSize>=.01&&cellSize<=100&&isfinite(scoreScale)&&scoreScale>0&&scoreScale<=65535;}
bool Eligible(GIAutoTraining r){return ControlsValid()&&r.valid==1&&r.version==version&&all(isfinite(float3(r.position.x,r.position.y,r.position.z)))&&max(abs(r.position.x),max(abs(r.position.y),abs(r.position.z)))<=1000000&&isfinite(r.energy)&&r.energy>0;}
int3 Key(GIAutoTraining r){return int3(floor(float3(r.position.x,r.position.y,r.position.z)/cellSize));}
[numthreads(64,1,1)]void Clear(uint3 id:SV_DispatchThreadID){
 if(id.x<cellCount)cells[id.x]=(GIAutoCell)0;
 if(id.x==0){GIAutoTargets t=(GIAutoTargets)0;t.schema=0x41544731u;t.version=version;t.frame=frame;targets[0]=t;}
}
uint QuantizedScore(GIAutoTraining r){return uint(clamp(r.energy*scoreScale,1.,65535.));}
[numthreads(64,1,1)]void Claim(uint3 id:SV_DispatchThreadID){
 if(id.x>=recordCount)return;GIAutoTraining r=training[id.x];if(!Eligible(r))return;
 int3 k=Key(r);uint slot=GIAutoHash(k.x,k.y,k.z)%cellCount,old;
 // A single atomic chooses the brightest record and its index together.
 // Ties prefer the lower index. No key can be torn by competing writers.
 uint priority=(QuantizedScore(r)<<16)|(65535u-id.x);
 InterlockedMax(cells[slot].reserved0,priority,old);
}
[numthreads(64,1,1)]void Accumulate(uint3 id:SV_DispatchThreadID){
 if(id.x>=recordCount)return;GIAutoTraining r=training[id.x];if(!Eligible(r))return;
 int3 k=Key(r);uint slot=GIAutoHash(k.x,k.y,k.z)%cellCount;
 uint priority=cells[slot].reserved0;if(priority==0)return;
 uint winner=65535u-(priority&65535u);if(winner>=recordCount)return;
 int3 chosen=Key(training[winner]);if(any(chosen!=k))return;
 if(id.x==winner){cells[slot].x=k.x;cells[slot].y=k.y;cells[slot].z=k.z;cells[slot].state=2;}
 uint old;InterlockedAdd(cells[slot].score,QuantizedScore(r),old);InterlockedAdd(cells[slot].count,1,old);
 // Fit only the bright part of this cell; dim background must not widen a
 // narrow source. Positive float bits preserve ordering for atomic max.
 GIAutoTraining anchor=training[winner];
 if(r.energy>=anchor.energy*.25){
  float3 delta=float3(r.position.x-anchor.position.x,r.position.y-anchor.position.y,r.position.z-anchor.position.z);
  float maximum=cellSize*.866025404;
  float extent2=min(dot(delta,delta),maximum*maximum);
  InterlockedMax(cells[slot].reserved1,asuint(extent2),old);
 }
}
groupshared uint bestSlots[128];groupshared uint bestScores[128];
groupshared uint selectedSlots[2];groupshared uint seekAlternative;
groupshared float neighborExtent[128];
groupshared float firstRadius;
float3 Anchor(uint slot){GIPoint p=training[65535u-(cells[slot].reserved0&65535u)].position;return float3(p.x,p.y,p.z);}
bool Better(uint score,uint slot,uint oldScore,uint oldSlot){return score>oldScore||(score==oldScore&&slot<oldSlot);}
float LaneExtent(uint chosen,uint lane){
  float extent=0;
  if(chosen!=0xffffffffu&&cells[chosen].count>=8){
   GIAutoCell primary=cells[chosen];uint winner=65535u-(primary.reserved0&65535u);
   float3 anchor=Anchor(chosen);float energy=training[winner].energy;
   float reach=cellSize*1.5;
   for(uint i=lane;i<cellCount;i+=64){
    GIAutoCell c=cells[i];if(c.state!=2||c.count==0||c.score==0||i==chosen)continue;
    uint other=65535u-(c.reserved0&65535u);
    if(other>=recordCount||training[other].energy<energy*.25)continue;
    float3 delta=Anchor(i)-anchor;float d2=dot(delta,delta);
    if(d2>reach*reach)continue;
    extent=max(extent,sqrt(d2)+sqrt(asfloat(c.reserved1))+cellSize*.1);
   }
  }
 return extent;
}
float Radius(uint chosen,uint offset){
 if(chosen==0xffffffffu)return 0;
 GIAutoCell c=cells[chosen];float maximum=cellSize*.866025404;
 float fitted=clamp(sqrt(asfloat(c.reserved1))+cellSize*.1,cellSize*.5,maximum);
 float neighbor=0;
 [unroll]for(uint lane=0;lane<64;++lane)neighbor=max(neighbor,neighborExtent[offset+lane]);
 return c.count>=8?max(fitted,min(neighbor,cellSize*1.6)):maximum;
}
[numthreads(64,1,1)]void Freeze(uint3 id:SV_GroupThreadID){
 uint slots[2]={0xffffffffu,0xffffffffu},scores[2]={0,0};
 if(ControlsValid())for(uint i=id.x;i<cellCount;i+=64){
  GIAutoCell c=cells[i];if(c.state!=2||c.count<minimumCount||c.score==0)continue;
  uint peak=c.reserved0>>16;
  if(Better(peak,i,scores[0],slots[0])){scores[1]=scores[0];slots[1]=slots[0];scores[0]=peak;slots[0]=i;}
  else if(Better(peak,i,scores[1],slots[1])){scores[1]=peak;slots[1]=i;}
 }
 bestSlots[id.x*2]=slots[0];bestSlots[id.x*2+1]=slots[1];bestScores[id.x*2]=scores[0];bestScores[id.x*2+1]=scores[1];GroupMemoryBarrierWithGroupSync();
 if(id.x==0){
  [unroll]for(uint rank=0;rank<2;++rank){
   uint chosen=0xffffffffu,score=0,entry=0;
   for(uint j=0;j<128;++j)if(bestScores[j]>0&&Better(bestScores[j],bestSlots[j],score,chosen)){chosen=bestSlots[j];score=bestScores[j];entry=j;}
   selectedSlots[rank]=chosen;if(chosen!=0xffffffffu)bestScores[entry]=0;
  }
 }
 GroupMemoryBarrierWithGroupSync();
 // Fit the primary before choosing the second guide. Dense, observed coverage
 // defines redundancy; sparse discovery keeps the conservative R459 distance.
 neighborExtent[id.x]=LaneExtent(selectedSlots[0],id.x);
 GroupMemoryBarrierWithGroupSync();
 if(id.x==0){
  firstRadius=Radius(selectedSlots[0],0);
  seekAlternative=0;
  if(selectedSlots[0]!=0xffffffffu&&selectedSlots[1]!=0xffffffffu){
   float3 delta=Anchor(selectedSlots[1])-Anchor(selectedSlots[0]);
   float nearRadius=cells[selectedSlots[0]].count>=8?firstRadius:cellSize*.433012702;
   seekAlternative=dot(delta,delta)<nearRadius*nearRadius?1:0;
  }
 }
 GroupMemoryBarrierWithGroupSync();
 // An already-covered anchor need not consume the second guide when another
 // bright patch is available. Never exclude a candidate by grid key alone.
 // Scan every cell: a lane's third candidate can be the best nonduplicate.
 if(seekAlternative!=0){
  uint chosen=0xffffffffu,score=0;
  float3 first=Anchor(selectedSlots[0]);float nearRadius=cells[selectedSlots[0]].count>=8?firstRadius:cellSize*.433012702;
  for(uint i=id.x;i<cellCount;i+=64){
   GIAutoCell c=cells[i];if(c.state!=2||c.count<minimumCount||c.score==0)continue;
   float3 delta=Anchor(i)-first;if(dot(delta,delta)<nearRadius*nearRadius)continue;
   uint peak=c.reserved0>>16;if(Better(peak,i,score,chosen)){chosen=i;score=peak;}
  }
  bestSlots[id.x]=chosen;bestScores[id.x]=score;
 }
 GroupMemoryBarrierWithGroupSync();
 if(id.x==0){
  if(seekAlternative!=0){
   uint chosen=0xffffffffu,score=0;
   for(uint j=0;j<64;++j)if(bestScores[j]>0&&Better(bestScores[j],bestSlots[j],score,chosen)){chosen=bestSlots[j];score=bestScores[j];}
   // Keep both R458 guides when training contains only the one bright patch.
   if(chosen!=0xffffffffu)selectedSlots[1]=chosen;
  }
 }
 GroupMemoryBarrierWithGroupSync();
 // Only the final secondary needs fitting; retain the primary reduction.
 neighborExtent[64+id.x]=LaneExtent(selectedSlots[1],id.x);
 GroupMemoryBarrierWithGroupSync();
 if(id.x==0){
  GIAutoTargets t=(GIAutoTargets)0;t.schema=0x41544731u;t.version=version;t.frame=frame;
  [unroll]for(uint rank=0;rank<2;++rank){
   uint chosen=selectedSlots[rank];if(chosen==0xffffffffu)continue;
   GIAutoCell c=cells[chosen];uint winner=65535u-(c.reserved0&65535u);
   t.bounds[rank].center=training[winner].position;
   t.bounds[rank].radius=rank==0?firstRadius:Radius(chosen,64);
   t.counts[rank]=c.count;t.scores[rank]=c.score;t.validMask|=1u<<rank;
  }
  targets[0]=t;
 }
}
