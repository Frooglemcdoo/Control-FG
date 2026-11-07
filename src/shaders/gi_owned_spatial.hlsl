// R366: one normalized surface-guided spatial pass at the native GI boundary.
#include "ControlPrimaryGuideDecode.hlsli"
Texture2D<float4> G1 : register(t0);
Texture2D<float4> G2 : register(t1);
Texture2D<float4> Depth : register(t2);
Texture2D<float4> Current : register(t5);
RWTexture2D<float4> Output : register(u0);
uint Material(float4 g){uint2 b=uint2(floor(g.ba*255.+.5));return b.x|(b.y<<8);}
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID){
 uint w,h;Output.GetDimensions(w,h);if(any(tid.xy>=uint2(w,h)))return;
 int2 p=int2(tid.xy);float4 g=G1.Load(int3(p,0));float3 n=ControlDecodeNormalView(g);
 uint material=Material(G2.Load(int3(p,0)));uint mask=uint(floor(g.a*255.+.5))&1;
 float z=Depth.Load(int3(p,0)).x;float3 center=Current.Load(int3(p,0)).rgb;
 if(!all(isfinite(center)))center=0;
 float3 sum=0;float total=0;
 [unroll]for(int y=-2;y<=2;++y)[unroll]for(int x=-2;x<=2;++x){
  int2 q=p+int2(x,y);if(any(q<0)||any(q>=int2(w,h)))continue;
  float4 tapG=G1.Load(int3(q,0));
  if((uint(floor(tapG.a*255.+.5))&1)!=mask || Material(G2.Load(int3(q,0)))!=material)continue;
  float nz=Depth.Load(int3(q,0)).x;
  if(!isfinite(nz)||abs(nz-z)>max(abs(z)*.005,1e-5))continue;
  float cosine=dot(n,ControlDecodeNormalView(tapG));
  if(cosine<.98)continue;
  float3 c=Current.Load(int3(q,0)).rgb;if(!all(isfinite(c)))continue;
  float wx=x==0?6.:abs(x)==1?4.:1.;float wy=y==0?6.:abs(y)==1?4.:1.;
  float normalWeight=saturate((cosine-.98)*50.);normalWeight*=normalWeight;
  float weight=wx*wy*normalWeight;
  sum+=c*weight;total+=weight;
 }
 float3 result=total>0?sum/total:center;
 Output[p]=float4(result,result.r);
}
