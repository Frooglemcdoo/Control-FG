#include "../gi_reuse/regional_target_lookup.h"
StructuredBuffer<GIRegionalTarget> regionalTargets:register(t0,space19);
GIAutoTargets GISelectRegional(float3 origin,float3 normal,GIAutoSelection selection){
 GIAutoTargets result=(GIAutoTargets)0;
 GIWorldGuideKey key=GIMakeWorldGuideKey(GIFrozenPoint(origin*.125f),GIFrozenPoint(normal));
 [unroll]for(uint probe=0;probe<4;++probe){
  GIRegionalTarget entry=regionalTargets[GIRegionalSlot(key,4096,probe)];
  if(GIRegionalAccept(entry,key,selection.sampleFrame,selection.version,8)&&entry.targets.frame==selection.sourceFrame){result=entry.targets;break;}
 }
 return result;
}
