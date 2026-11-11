// Current-frame training only. No temporal history or endpoint reuse.
#include "../gi_reuse/frozen_guide_training.h"
StructuredBuffer<GITrainingRecord> training : register(t0);
globallycoherent RWStructuredBuffer<GITrainingCell> cells : register(u0);
RWStructuredBuffer<GIKeyedGuide> frozen : register(u1);
cbuffer Controls : register(b0) {uint recordCount,cellCount,frame,epoch;};
[numthreads(64,1,1)]
void Clear(uint3 id:SV_DispatchThreadID) {
 if(id.x>=cellCount)return;
 cells[id.x]=(GITrainingCell)0;frozen[id.x]=(GIKeyedGuide)0;
}
[numthreads(64,1,1)]
void Claim(uint3 id:SV_DispatchThreadID) {
 id.x+=id.y*(65535u*64u);
 if(id.x>=recordCount||!cellCount)return;
 GITrainingRecord r=training[id.x];
 if(r.key.valid!=1||r.key.normal>7)return;
 uint slot=GITrainingHash(r.key)%cellCount,old;
 InterlockedCompareExchange(cells[slot].state,0,1,old);
 if(old==0){
  cells[slot].x=r.key.x;cells[slot].y=r.key.y;cells[slot].z=r.key.z;cells[slot].normal=r.key.normal;
  DeviceMemoryBarrier();InterlockedExchange(cells[slot].state,2,old);
 }

}
[numthreads(64,1,1)]
void Accumulate(uint3 id:SV_DispatchThreadID) {
 id.x+=id.y*(65535u*64u);
 if(id.x>=recordCount||!cellCount)return;
 GITrainingRecord r=training[id.x];if(r.key.valid!=1||r.key.normal>7)return;
 uint slot=GITrainingHash(r.key)%cellCount,old;
 if(cells[slot].state!=2)return; DeviceMemoryBarrier();
 if(cells[slot].x!=r.key.x||cells[slot].y!=r.key.y||cells[slot].z!=r.key.z||cells[slot].normal!=r.key.normal)return;
 if(!GIFinitePoint(r.direction)||abs(GIDot(r.direction,r.direction)-1)>0.0001f||!isfinite(r.energy)||r.energy<0){
  InterlockedExchange(cells[slot].poison,1,old);return;
 }
 uint bin=GIFrozenCell(r.direction);
 uint expected=cells[slot].energy[bin];bool stored=false;
 [loop]for(uint attempt=0;attempt<32;++attempt){
  float sum=asfloat(expected)+r.energy;if(!isfinite(sum))break;
  uint observed;InterlockedCompareExchange(cells[slot].energy[bin],expected,asuint(sum),observed);
  if(observed==expected){stored=true;break;}expected=observed;
 }
 if(stored)InterlockedAdd(cells[slot].count,1,old);
 else InterlockedExchange(cells[slot].poison,1,old);
}
[numthreads(64,1,1)]
void Freeze(uint3 id:SV_DispatchThreadID) {
 if(id.x>=cellCount)return;
 GIKeyedGuide result=(GIKeyedGuide)0;
 if(cells[id.x].state==2&&!cells[id.x].poison){
  result.key.x=cells[id.x].x;result.key.y=cells[id.x].y;result.key.z=cells[id.x].z;
  result.key.normal=cells[id.x].normal;result.key.valid=1;
  float energy[64];for(uint i=0;i<64;++i)energy[i]=asfloat(cells[id.x].energy[i]);
  result.guide=GIFreezeWorldGuide(energy,cells[id.x].count,frame,epoch);
 }
 frozen[id.x]=result;
}
