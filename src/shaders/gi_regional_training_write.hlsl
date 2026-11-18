void GIRegionalStore(uint index,GIWorldGuideKey key,GIAutoTraining best,uint version){
  GIAutoTraining a=(GIAutoTraining)0,b=(GIAutoTraining)0;
  a.position.x=asfloat(key.x);a.position.y=asfloat(key.y);a.position.z=asfloat(key.z);a.energy=asfloat(key.normal);
  float3 hit=float3(best.position.x,best.position.y,best.position.z);
  float3 center=(floor(hit*2.0f)+.5f)*.5f;
  a.valid=key.valid;a.version=asuint(center.x);a.reserved0=asuint(center.y);a.reserved1=asuint(center.z);
  // Half the diagonal of a 0.5m cell, with sub-millimeter rounding margin.
  b.position.x=.4331f;b.position.y=best.energy;b.position.z=asfloat(version);
  autoTraining[2*index]=a;autoTraining[2*index+1]=b;
}
