// R364. Native GI temporal ABI; replaces that dispatch, with no extra resources.
// Depth and velocity are verified native inputs. No normal/material guide is bound here.
cbuffer Sys : register(b0) { float4 sys[38]; }
cbuffer Filtering : register(b1) { float4 filtering[2]; }
Texture2D<float4> Depth : register(t0);
Texture2D<float4> PreviousDepth : register(t1);
Texture2D<float4> Velocity : register(t2);
Texture2D<float4> Current : register(t3);
Texture2D<float4> History : register(t4);
SamplerState LinearClamp : register(s6, space1);
RWTexture2D<float4> Output : register(u0);
float Luma(float3 v) { return dot(v,float3(.2126,.7152,.0722)); }
bool Valid(float3 v) { return all(isfinite(v)) && all(v>=0); }
[numthreads(8,8,1)]
void main(uint3 tid : SV_DispatchThreadID) {
 uint w,h; Output.GetDimensions(w,h); if(any(tid.xy>=uint2(w,h)))return;
 int2 p=int2(tid.xy); float2 uv=(float2(p)+.5)*sys[0].zw;
 float3 center=Current.Load(int3(p,0)).rgb;
 if(!Valid(center))center=0;
 float z=Depth.SampleLevel(LinearClamp,uv,0).x;
 float3 sum=center*4, lo=center, hi=center; float total=4;
 // A single small, normalized kernel; strong radiance edges retain center detail.
 [unroll] for(int y=-1;y<=1;++y) [unroll] for(int x=-1;x<=1;++x) {
  if(x==0 && y==0)continue;
  int2 q=p+int2(x,y); if(any(q<0)||any(q>=int2(w,h)))continue;
  float3 c=Current.Load(int3(q,0)).rgb;
  float dz=Depth.SampleLevel(LinearClamp,(float2(q)+.5)*sys[0].zw,0).x;
  if(!Valid(c)||!isfinite(dz)||!isfinite(z)||abs(dz-z)>max(abs(z)*.005,1e-5))continue;
  lo=min(lo,c);hi=max(hi,c);
  float a=Luma(center),b=Luma(c);
  float range=max(.5*max(a,b),.002);
  float weight=(x==0||y==0?2.:1.)/(1.+abs(a-b)/range);
  sum+=c*weight;total+=weight;
 }
 float3 current=sum/total;
 float2 velocityUV=(float2(p)+.5)*sys[1].zw;
 float2 motion=Velocity.SampleLevel(LinearClamp,velocityUV,0).xy;
 float2 prevUV=velocityUV+motion;
 float3 result=current;
 if(asuint(filtering[1].z)==0 && all(isfinite(prevUV)) && all(prevUV>=0) && all(prevUV<=1) && isfinite(z) && z>0) {
  uint hw,hh;History.GetDimensions(hw,hh);
  float2 hp=prevUV*float2(hw,hh)-.5;int2 base=int2(floor(hp));float2 f=frac(hp);
  float3 history=0;float validWeight=0;
  // Validate each bilinear footprint tap, avoiding blending foreground/background history.
  [unroll] for(int hy=0;hy<2;++hy) [unroll] for(int hx=0;hx<2;++hx) {
   int2 q=base+int2(hx,hy);if(any(q<0)||any(q>=int2(hw,hh)))continue;
   float2 huv=(float2(q)+.5)/float2(hw,hh);
   float oldZ=PreviousDepth.SampleLevel(LinearClamp,huv,0).x+sys[37].z;
   float3 old=History.Load(int3(q,0)).rgb;
   if(!Valid(old)||!isfinite(oldZ)||abs(oldZ-z)>max(abs(z)*.01,1e-5))continue;
   float weight=(hx?f.x:1-f.x)*(hy?f.y:1-f.y);
   history+=old*weight;validWeight+=weight;
  }
  if(validWeight>.5) {
   history/=validWeight;
   // Symmetric bounds: unlike the previous mip cap, never lower current radiance.
   float3 margin=(hi-lo)*.1+.0001;
   history=clamp(history,max(0,lo-margin),hi+margin);
   float pixels=length((prevUV-uv)*float2(w,h));
   float blend=.9*validWeight/(1.+pixels*.1);
   result=lerp(current,history,blend);
  }
 }
 // Native output ABI duplicates red in alpha. No metadata is stored in alpha.
 Output[p]=float4(result,result.r);
}
