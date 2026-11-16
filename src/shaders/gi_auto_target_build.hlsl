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
[numthreads(64,1,1)]void Claim(uint3 id:SV_DispatchThreadID){
 if(id.x>=recordCount)return;GIAutoTraining r=training[id.x];if(!Eligible(r))return;
 int3 k=Key(r);uint slot=GIAutoHash(k.x,k.y,k.z)%cellCount,old;
 InterlockedCompareExchange(cells[slot].state,0,1,old);
 if(old==0){cells[slot].x=k.x;cells[slot].y=k.y;cells[slot].z=k.z;DeviceMemoryBarrier();InterlockedExchange(cells[slot].state,2,old);}
}
[numthreads(64,1,1)]void Accumulate(uint3 id:SV_DispatchThreadID){
 if(id.x>=recordCount)return;GIAutoTraining r=training[id.x];if(!Eligible(r))return;
 int3 k=Key(r);uint slot=GIAutoHash(k.x,k.y,k.z)%cellCount;
 if(cells[slot].state!=2||any(int3(cells[slot].x,cells[slot].y,cells[slot].z)!=k))return;
 uint score=uint(clamp(r.energy*scoreScale,1.,65535.)),old;
 InterlockedAdd(cells[slot].score,score,old);InterlockedAdd(cells[slot].count,1,old);
}
groupshared uint bestSlots[128];groupshared uint bestScores[128];
bool Better(uint score,uint slot,uint oldScore,uint oldSlot){return score>oldScore||(score==oldScore&&slot<oldSlot);}
[numthreads(64,1,1)]void Freeze(uint3 id:SV_GroupThreadID){
 uint slots[2]={0xffffffffu,0xffffffffu},scores[2]={0,0};
 if(ControlsValid())for(uint i=id.x;i<cellCount;i+=64){
  GIAutoCell c=cells[i];if(c.state!=2||c.count<minimumCount||c.score==0)continue;
  if(Better(c.score,i,scores[0],slots[0])){scores[1]=scores[0];slots[1]=slots[0];scores[0]=c.score;slots[0]=i;}
  else if(Better(c.score,i,scores[1],slots[1])){scores[1]=c.score;slots[1]=i;}
 }
 bestSlots[id.x*2]=slots[0];bestSlots[id.x*2+1]=slots[1];bestScores[id.x*2]=scores[0];bestScores[id.x*2+1]=scores[1];GroupMemoryBarrierWithGroupSync();
 if(id.x==0){
  GIAutoTargets t=(GIAutoTargets)0;t.schema=0x41544731u;t.version=version;t.frame=frame;
  [unroll]for(uint rank=0;rank<2;++rank){
   uint chosen=0xffffffffu,score=0,entry=0;
   for(uint j=0;j<128;++j)if(bestScores[j]>0&&Better(bestScores[j],bestSlots[j],score,chosen)){chosen=bestSlots[j];score=bestScores[j];entry=j;}
   if(chosen!=0xffffffffu){
    GIAutoCell c=cells[chosen];t.bounds[rank].center.x=(float(c.x)+.5)*cellSize;t.bounds[rank].center.y=(float(c.y)+.5)*cellSize;t.bounds[rank].center.z=(float(c.z)+.5)*cellSize;
    t.bounds[rank].radius=cellSize*.866025404;t.counts[rank]=c.count;t.scores[rank]=score;t.validMask|=1u<<rank;bestScores[entry]=0;
   }
  }targets[0]=t;
 }
}
