// R383 Stable temporal core. Sidecars are owned by the exact-dispatch adapter.
// Native RGB/red-alpha output continues through two native spatial passes.
cbuffer Sys : register(b0) { float4 sys[38]; }
cbuffer Filtering : register(b1) { float4 filtering[2]; }
Texture2D<float4> Depth : register(t0);
Texture2D<float4> PreviousDepth : register(t1);
Texture2D<float4> Velocity : register(t2);
Texture2D<float4> Current : register(t3);
Texture2D<float4> History : register(t4);
Texture2D<float4> FastHistory : register(t5); // RGB + accumulated age
Texture2D<float4> MomentsHistory : register(t6); // luminance mean/second moment
SamplerState LinearClamp : register(s6,space1);
RWTexture2D<float4> Output : register(u0); // native RGB/red-alpha contract
RWTexture2D<float4> FastOutput : register(u1);
RWTexture2D<float4> MomentsOutput : register(u2);
float Luma(float3 c) {return dot(c,float3(.2126,.7152,.0722));}
bool Valid(float3 c) {return all(isfinite(c))&&all(c>=0);}
float Limited(float left,float right) {return left*right>=0?(abs(left)<abs(right)?left:right):0;}
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID) {
 uint w,h;Output.GetDimensions(w,h);if(any(tid.xy>=uint2(w,h)))return;
 int2 p=int2(tid.xy);float2 uv=(float2(p)+.5)*sys[0].zw;
 float3 c=Current.Load(int3(p,0)).rgb;if(!Valid(c))c=0;
 float z=Depth.SampleLevel(LinearClamp,uv,0).x;
 uint dw,dh;Depth.GetDimensions(dw,dh);float2 dsize=float2(dw,dh),inv=1/dsize;
 float2 slope=float2(Limited(z-Depth.SampleLevel(LinearClamp,uv-float2(inv.x,0),0).x,Depth.SampleLevel(LinearClamp,uv+float2(inv.x,0),0).x-z),
                     Limited(z-Depth.SampleLevel(LinearClamp,uv-float2(0,inv.y),0).x,Depth.SampleLevel(LinearClamp,uv+float2(0,inv.y),0).x-z))*dsize;
 float3 sum=c*4;float total=4;
 [unroll]for(int y=-1;y<=1;++y)[unroll]for(int x=-1;x<=1;++x){
  if(x==0&&y==0)continue;int2 q=p+int2(x,y);if(any(q<0)||any(q>=int2(w,h)))continue;
  float2 quv=(float2(q)+.5)*sys[0].zw;float qz=Depth.SampleLevel(LinearClamp,quv,0).x;
  float3 qc=Current.Load(int3(q,0)).rgb;
  if(!Valid(qc)||!isfinite(qz)||abs(qz-z-dot(slope,quv-uv))>max(abs(z)*.005,1e-6))continue;
  float wt=(x==0?2.:1.)*(y==0?2.:1.);sum+=wt*qc;total+=wt;
 }
 c=sum/total;
 float2 vuv=(float2(p)+.5)*sys[1].zw,prevUV=vuv+Velocity.SampleLevel(LinearClamp,vuv,0).xy;
 float3 slow=0;float4 fast=0;float3 moments=0;float weight=0;
 if(asuint(filtering[1].z)==0&&isfinite(z)&&z>0&&all(isfinite(prevUV))&&all(prevUV>=0)&&all(prevUV<=1)){
  uint hw,hh,fw,fh;History.GetDimensions(hw,hh);FastHistory.GetDimensions(fw,fh);float2 hp=prevUV*float2(hw,hh)-.5;int2 base=int2(floor(hp));float2 f=frac(hp);
  [unroll]for(int y=0;y<2;++y)[unroll]for(int x=0;x<2;++x){
   int2 q=base+int2(x,y);if(fw!=hw||fh!=hh||any(q<0)||any(q>=int2(hw,hh)))continue;
   float2 huv=(float2(q)+.5)/float2(hw,hh);float pz=PreviousDepth.SampleLevel(LinearClamp,huv,0).x+sys[37].z;
   float3 old=History.Load(int3(q,0)).rgb;float4 sh=FastHistory.Load(int3(q,0));float3 m=MomentsHistory.Load(int3(q,0)).xyz;
   if(!Valid(old)||!Valid(sh.rgb)||!all(isfinite(m))||!isfinite(sh.a)||sh.a<1||!isfinite(pz)||abs(pz-z-dot(slope,huv-prevUV))>max(abs(z)*.01,1e-6))continue;
   float wt=(x?f.x:1-f.x)*(y?f.y:1-f.y);slow+=old*wt;fast+=sh*wt;moments+=m*wt;weight+=wt;
  }
 }
 float l=Luma(c),age=1,sampleAge=1;float3 result=c,newFast=c;float2 newMoments=float2(l,l*l);
 if(weight>1e-6){
  slow/=weight;fast/=weight;moments/=weight;
  float variance=max(0,moments.y-moments.x*moments.x),sigma=sqrt(variance);
  float oldL=Luma(fast.rgb);float delta=abs(l-oldL);
  // Confidence uses temporal noise, not a fresh single-frame min/max box.
  float reactive=saturate((delta-3*sigma-.02*max(oldL,l))/(max(oldL,l)*.25+.001));
  // A nearly extinguished or newly dominant light must not leave a long trail.
  if(l<oldL*.1||oldL<l*.1)reactive=1;
  if(fast.a<4)reactive=0;
  sampleAge=min(fast.a+1,32.);
  age=lerp(min(moments.z+1,32.),1.,reactive);
  float alpha=1/age,fastAlpha=1/min(sampleAge,4.);
  newFast=lerp(fast.rgb,c,fastAlpha);
  newMoments=lerp(moments.xy,float2(l,l*l),1/min(sampleAge,8.));
  // Only long history is constrained. Never cap the current light sample.
  float radius=2*sqrt(max(0,newMoments.y-newMoments.x*newMoments.x))+.02*max(Luma(newFast),.001);
  float3 bounded=clamp(slow,max(0,newFast-radius),newFast+radius);
  result=lerp(bounded,c,alpha);
 }
 Output[p]=float4(result,result.r);FastOutput[p]=float4(newFast,sampleAge);MomentsOutput[p]=float4(newMoments,age,0);
}
