void GIRegionalStore(uint index,GIWorldGuideKey key,GIAutoTraining best,uint version){
  GIAutoTraining a=(GIAutoTraining)0,b=(GIAutoTraining)0;
  a.position.x=asfloat(key.x);a.position.y=asfloat(key.y);a.position.z=asfloat(key.z);a.energy=asfloat(key.normal);
  a.valid=key.valid;a.version=asuint(best.position.x);a.reserved0=asuint(best.position.y);a.reserved1=asuint(best.position.z);
  b.position.x=.433012702f;b.position.y=best.energy;b.position.z=asfloat(version);
  autoTraining[2*index]=a;autoTraining[2*index+1]=b;
}
