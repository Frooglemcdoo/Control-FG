#include "gi_weight_transport.hlsl"
float GIReadWeight(uint2 pixel,uint ray){
 uint address=GIWeightAddress(pixel,ray,trainingWidth,trainingHeight,trainingCapacityPacked,selection.rayCount);
 GIAutoTraining v=autoTraining[address/32u];uint lane=(address/4u)&7u;
 if(lane==0)return v.position.x;if(lane==1)return v.position.y;if(lane==2)return v.position.z;if(lane==3)return v.energy;
 if(lane==4)return asfloat(v.valid);if(lane==5)return asfloat(v.version);if(lane==6)return asfloat(v.reserved0);return asfloat(v.reserved1);
}
