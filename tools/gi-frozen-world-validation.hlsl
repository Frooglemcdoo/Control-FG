#include "../src/gi_reuse/frozen_world_guide.h"
RWStructuredBuffer<GIFrozenRay> Output : register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
 uint i=id.x;if(i>=4096)return;
 uint scenario=i%4;
 float energy[64];for(uint j=0;j<64;++j)energy[j]=0;
 energy[63]=1e30;energy[2]=3e28;
 if(scenario==3)energy[0]=asfloat(0x7f800000);
 GIFrozenWorldGuide g=GIFreezeWorldGuide(energy,scenario==2?255:512,100,7);
 GIPoint n;n.x=.6;n.y=0;n.z=(i&4)?-.8:.8;
 float u=(float(i)+.5)/4096,v=(float((i*73)%4096)+.5)/4096;
 Output[i]=GISampleFrozenWorld(g,n,scenario==1?101:100,7,.125,u,v);
}
