#include "../src/gi_reuse/regional_target_lookup.h"
struct Query {GIWorldGuideKey receiver;uint frame,version,maximumAge;};
StructuredBuffer<GIRegionalTarget> entries:register(t0);
StructuredBuffer<Query> queries:register(t1);
RWStructuredBuffer<uint> results:register(u0);
cbuffer Limits:register(b0){uint capacity;}
[numthreads(1,1,1)]void main(uint3 id:SV_DispatchThreadID){
 Query q=queries[id.x];uint chosen=0xffffffffu;
 for(uint probe=0;probe<4&&probe<capacity;++probe){
  uint slot=GIRegionalSlot(q.receiver,capacity,probe);
  if(GIRegionalAccept(entries[slot],q.receiver,q.frame,q.version,q.maximumAge)){chosen=slot;break;}
 }
 results[id.x]=chosen;
}
