// Capture tail: existing pixel summaries, producer packets (rounded to 32B),
// then six training-sized records per pixel for its strongest red contribution.
uint GIRedIndex(uint auditIndex,uint rays){
 uint pixels=min(trainingWidth,512u)*min(trainingHeight,512u);
 return 2*trainingCapacity+2*pixels+(pixels*(rays+1)+1)/2+3*(auditIndex-2*trainingCapacity);
}
void GIRedWrite(uint index,GIAutoTraining a,GIAutoTraining b,float scale,uint rays){
 if(index==0xffffffff)return;b.reserved1=asuint(scale);
 uint base=GIRedIndex(index,rays);autoTraining[base]=a;autoTraining[base+1]=b;
}

#include "gi_secondary_terms.hlsl"
GIAutoTraining GIRedPair(float4 a,float4 b){
 GIAutoTraining v;v.position.x=a.x;v.position.y=a.y;v.position.z=a.z;v.energy=a.w;
 v.valid=asuint(b.x);v.version=asuint(b.y);v.reserved0=asuint(b.z);v.reserved1=asuint(b.w);return v;
}
void GIRedTermsWrite(uint index,GISecondaryTerms t,uint rays){
 if(index==0xffffffff)return;uint base=GIRedIndex(index,rays)+2;
 autoTraining[base]=GIRedPair(t.texture,t.tint);
 autoTraining[base+1]=GIRedPair(t.volume,t.surface);
 autoTraining[base+2]=GIRedPair(t.point,t.spot);
 autoTraining[base+3]=GIRedPair(t.fallback,t.brdf);
}
