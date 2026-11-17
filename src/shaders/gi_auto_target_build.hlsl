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
}
groupshared uint bestSlots[128];groupshared uint bestScores[128];
groupshared uint firstSlot,fallbackSlot;
bool Better(uint score,uint slot,uint oldScore,uint oldSlot){return score>oldScore||(score==oldScore&&slot<oldSlot);}
bool Candidate(GIAutoCell c){return c.state==2&&c.count>=minimumCount&&c.score>0;}
[numthreads(64,1,1)]void Freeze(uint3 id:SV_GroupThreadID){
 uint slots[2]={0xffffffffu,0xffffffffu},scores[2]={0,0};
 if(ControlsValid())for(uint i=id.x;i<cellCount;i+=64){
  GIAutoCell c=cells[i];if(!Candidate(c))continue;
  uint peak=c.reserved0>>16;
  if(Better(peak,i,scores[0],slots[0])){scores[1]=scores[0];slots[1]=slots[0];scores[0]=peak;slots[0]=i;}
  else if(Better(peak,i,scores[1],slots[1])){scores[1]=peak;slots[1]=i;}
 }
 bestSlots[id.x*2]=slots[0];bestSlots[id.x*2+1]=slots[1];bestScores[id.x*2]=scores[0];bestScores[id.x*2+1]=scores[1];GroupMemoryBarrierWithGroupSync();
 if(id.x==0){
  firstSlot=0xffffffffu;fallbackSlot=0xffffffffu;uint firstScore=0,secondScore=0;
  for(uint j=0;j<128;++j)if(bestScores[j]>0){
   if(Better(bestScores[j],bestSlots[j],firstScore,firstSlot)){
    secondScore=firstScore;fallbackSlot=firstSlot;firstScore=bestScores[j];firstSlot=bestSlots[j];
   }else if(Better(bestScores[j],bestSlots[j],secondScore,fallbackSlot)){secondScore=bestScores[j];fallbackSlot=bestSlots[j];}
  }
 }
 GroupMemoryBarrierWithGroupSync();
 // Search the entire table again: a distinct region may rank below both local
 // winners. Prefer it over spending both slots on adjacent grid cells.
 uint separate=0xffffffffu,separateScore=0;
 if(ControlsValid()&&firstSlot!=0xffffffffu){
  GIAutoCell a=cells[firstSlot];int3 anchor=int3(a.x,a.y,a.z);
  for(uint k=id.x;k<cellCount;k+=64){
   GIAutoCell c=cells[k];if(!Candidate(c)||k==firstSlot)continue;
   if(all(abs(int3(c.x,c.y,c.z)-anchor)<=1))continue;
   uint peak=c.reserved0>>16;
   if(Better(peak,k,separateScore,separate)){separateScore=peak;separate=k;}
  }
 }
 bestSlots[id.x]=separate;bestScores[id.x]=separateScore;GroupMemoryBarrierWithGroupSync();
 if(id.x==0){
  uint second=fallbackSlot,score=0,chosen=0xffffffffu;
  for(uint q=0;q<64;++q)if(bestScores[q]>0&&Better(bestScores[q],bestSlots[q],score,chosen)){score=bestScores[q];chosen=bestSlots[q];}
  if(chosen!=0xffffffffu)second=chosen;
  GIAutoTargets t=(GIAutoTargets)0;t.schema=0x41544731u;t.version=version;t.frame=frame;
  [unroll]for(uint rank=0;rank<2;++rank){
   uint slot=rank==0?firstSlot:second;
   if(slot!=0xffffffffu){
    GIAutoCell c=cells[slot];t.bounds[rank].center.x=(float(c.x)+.5)*cellSize;t.bounds[rank].center.y=(float(c.y)+.5)*cellSize;t.bounds[rank].center.z=(float(c.z)+.5)*cellSize;
    t.bounds[rank].radius=cellSize*.866025404;t.counts[rank]=c.count;t.scores[rank]=c.score;t.validMask|=1u<<rank;
   }
  }targets[0]=t;
 }
}
