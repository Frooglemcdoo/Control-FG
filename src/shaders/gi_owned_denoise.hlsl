// R370. Preserve current lighting; reject stale history before blending.
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
 // Preserve all finite current radiance. Spatial support alone cannot distinguish
 // a small real light from an isolated Monte Carlo sample: never delete either.
 float3 sum=0,moment=0,lo=1e30,hi=0;float total=0;
 float2 centerMotion=Velocity.SampleLevel(LinearClamp,(float2(p)+.5)*sys[1].zw,0).xy;
 float motionDiscontinuity=0;float depthDiscontinuity=0;
 [unroll] for(int y=-1;y<=1;++y) [unroll] for(int x=-1;x<=1;++x) {
  int2 q=p+int2(x,y);if(any(q<0)||any(q>=int2(w,h)))continue;
  float3 c=Current.Load(int3(q,0)).rgb;
  float dz=Depth.SampleLevel(LinearClamp,(float2(q)+.5)*sys[0].zw,0).x;
  float2 mv=Velocity.SampleLevel(LinearClamp,(float2(q)+.5)*sys[1].zw,0).xy;
  if(all(isfinite(mv)) && all(isfinite(centerMotion)))
   motionDiscontinuity=max(motionDiscontinuity,length((mv-centerMotion)*float2(w,h)));
  if(isfinite(dz)&&isfinite(z))depthDiscontinuity=max(depthDiscontinuity,abs(dz-z)/max(abs(z),1e-5));
  if(!Valid(c)||!isfinite(dz)||!isfinite(z)||abs(dz-z)>max(abs(z)*.005,1e-5))continue;
  float weight=(x==0?2.:1.)*(y==0?2.:1.);
  sum+=c*weight;moment+=c*c*weight;total+=weight;lo=min(lo,c);hi=max(hi,c);
 }
 float3 current=total>0?sum/total:center;
 float3 variance=total>0?max(0,moment/total-current*current):0;
 float3 deviation=sqrt(variance);
 // These bounds constrain HISTORY only. Current light is never capped.
 float3 lower=max(0,max(lo,current-deviation)-.00001);
 float3 upper=min(hi,current+deviation)+.00001;
 if(total==0){lower=max(0,center-.00001);upper=center+.00001;}
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
   // Reject mismatched shading before clipping can disguise the disagreement.
   float disagreement=max(max(abs(history-current).r/(deviation.r+.001),
                              abs(history-current).g/(deviation.g+.001)),
                              abs(history-current).b/(deviation.b+.001));
   // Variance alone is permissive at bright compact lights. Symmetric relative
   // luminance change rejects both old dark history and old bright trails.
   float a=Luma(current),b=Luma(history);
   float illuminationChange=abs(a-b)/max(max(a,b),.001);
   float reactive=max(saturate(disagreement-1.),saturate((illuminationChange-.05)/.15));
   float pixels=length(motion*float2(w,h));
   float edgeConfidence=1.-saturate(max(motionDiscontinuity*2.,depthDiscontinuity*200.));
   float blend=.9*validWeight*(1.-reactive)*edgeConfidence/(1.+pixels*.5);
   history=clamp(history,lower,upper);
   result=lerp(current,history,blend);
  }
 }
 // Native output ABI duplicates red in alpha. No metadata is stored in alpha.
 Output[p]=float4(result,result.r);
}
