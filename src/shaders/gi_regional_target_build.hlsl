#include "../gi_reuse/regional_target_build.h"
StructuredBuffer<GIRegionalTraining> records:register(t0);
globallycoherent RWStructuredBuffer<GIRegionalCell> cells:register(u0);
RWStructuredBuffer<GIRegionalTarget> published:register(u1);
cbuffer Controls:register(b0){uint recordCount,capacity,frame,version;}
bool Valid(GIRegionalTraining r){
 return recordCount<=65535&&capacity>0&&capacity<=65536&&frame>0&&version>0&&
 r.receiver.valid==1&&r.receiver.normal<=7&&r.version==version&&
 GIFinitePoint(r.source.center)&&isfinite(r.source.radius)&&r.source.radius>0&&
 isfinite(r.score)&&r.score>0;
}
uint Priority(uint index){return (uint(clamp(records[index].score*512,1.,65535.))<<16)|(65535-index);}
uint RecordIndex(uint priority){return 65535-(priority&65535);}
uint Find(GIWorldGuideKey receiver){
 for(uint probe=0;probe<4&&probe<capacity;++probe){
  uint slot=GIRegionalSlot(receiver,capacity,probe);
  uint owner=cells[slot].owner;
  if(owner>0&&owner<=recordCount&&GIWorldGuideKeysMatch(records[owner-1].receiver,receiver))return slot;
 }
 return 0xffffffff;
}
[numthreads(64,1,1)]void Clear(uint3 id:SV_DispatchThreadID){
 if(id.x<capacity*(1+GIRegionalStripes))cells[id.x]=(GIRegionalCell)0;
 if(id.x<capacity)published[id.x]=(GIRegionalTarget)0;
}
[numthreads(64,1,1)]void Claim(uint3 id:SV_DispatchThreadID){
 if(id.x>=recordCount)return;GIRegionalTraining r=records[id.x];if(!Valid(r))return;
 for(uint probe=0;probe<4&&probe<capacity;++probe){
  uint slot=GIRegionalSlot(r.receiver,capacity,probe),old;
  // Atomically publish an index into immutable input, never a multiword key.
  // This eliminates writer spinning and partially initialized key reads.
  InterlockedCompareExchange(cells[slot].owner,0,id.x+1,old);
  if(old==0)return;
  if(old<=recordCount&&GIWorldGuideKeysMatch(records[old-1].receiver,r.receiver))return;
 }
}
[numthreads(64,1,1)]void First(uint3 id:SV_DispatchThreadID){
 if(id.x>=recordCount)return;GIRegionalTraining r=records[id.x];if(!Valid(r))return;
 uint slot=Find(r.receiver);if(slot==0xffffffff)return;uint old;
 InterlockedMax(cells[slot].first,Priority(id.x),old);InterlockedAdd(cells[slot].count,1,old);
 // Publish only an index here. The inter-dispatch UAV barrier makes the entire
 // list immutable before Second follows it; nobody follows links in First.
 uint stripe=capacity+slot*GIRegionalStripes+(id.x%GIRegionalStripes);
 InterlockedExchange(cells[stripe].owner,id.x+1,old);
 cells[capacity*(1+GIRegionalStripes)+id.x].owner=old;
}
[numthreads(64,1,1)]void Second(uint3 id:SV_DispatchThreadID){
 if(id.x>=capacity*GIRegionalStripes)return;
 uint region=id.x/GIRegionalStripes,stripe=capacity+id.x;
 GIRegionalCell c=cells[region];uint winner=0;
 if(c.first==0){cells[stripe].second=0;return;}
 uint first=RecordIndex(c.first);
 if(first>=recordCount){cells[stripe].second=0;return;}
 GIAutoBound primary=records[first].source;
 uint link=cells[stripe].owner,visited=0;
 // One writer per stripe, 32 parallel lists per receiver region. No atomic
 // operations or polling here. Each accepted record is still visited once.
 [loop]while(link!=0&&visited<recordCount){
  uint index=link-1;
  if(index>=recordCount){winner=0;break;}
  GIRegionalTraining r=records[index];
  float3 delta=float3(r.source.center.x-primary.center.x,r.source.center.y-primary.center.y,r.source.center.z-primary.center.z);
  if(index!=first&&dot(delta,delta)>primary.radius*primary.radius)winner=max(winner,Priority(index));
  link=cells[capacity*(1+GIRegionalStripes)+index].owner;++visited;
 }
 if(link!=0&&visited==recordCount)winner=0; // Corrupt/cyclic list never spins.
 cells[stripe].second=winner;
}
[numthreads(64,1,1)]void Freeze(uint3 id:SV_DispatchThreadID){
 if(id.x>=capacity)return;GIRegionalTarget outValue=(GIRegionalTarget)0;
 GIRegionalCell c=cells[id.x];
 uint secondary=0;
 [unroll]for(uint stripe=0;stripe<GIRegionalStripes;++stripe)
  secondary=max(secondary,cells[capacity+id.x*GIRegionalStripes+stripe].second);
 if(c.owner>0&&c.owner<=recordCount&&c.first>0){
  outValue.receiver=records[c.owner-1].receiver;outValue.targets.schema=0x41544731;
  outValue.targets.frame=frame;outValue.targets.version=version;
  [unroll]for(uint rank=0;rank<2;++rank){
   uint priority=rank==0?c.first:secondary;if(priority==0)continue;
   outValue.targets.bounds[rank]=records[RecordIndex(priority)].source;
   outValue.targets.counts[rank]=c.count;outValue.targets.scores[rank]=priority>>16;
   outValue.targets.validMask|=1u<<rank;
  }
 }
 published[id.x]=outValue;
}
