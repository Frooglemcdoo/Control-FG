// R369. Robust current statistics and validated temporal reuse at the native GI boundary.
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
 // Gather before filtering: radiance differences are not geometry edges.
 // In R368 a firefly increased its own filter weight and history bounds.
 float3 samples[9]; float weights[9];
 float peak=0,second=0,third=0,lumaSum=0,count=0;
 [unroll] for(int y=-1;y<=1;++y) [unroll] for(int x=-1;x<=1;++x) {
  int i=(y+1)*3+x+1;int2 q=p+int2(x,y);
  samples[i]=0;weights[i]=0;
  if(any(q<0)||any(q>=int2(w,h)))continue;
  float3 c=Current.Load(int3(q,0)).rgb;
  float dz=Depth.SampleLevel(LinearClamp,(float2(q)+.5)*sys[0].zw,0).x;
  if(!Valid(c)||!isfinite(dz)||!isfinite(z)||abs(dz-z)>max(abs(z)*.005,1e-5))continue;
  samples[i]=c;weights[i]=(x==0?2.:1.)*(y==0?2.:1.);
  float lum=Luma(c);third=max(third,min(second,lum));
  second=max(second,min(peak,lum));peak=max(peak,lum);
  lumaSum+=lum;count+=1;
 }
 // Only suppress unsupported one/two-sample peaks. Broad bright regions and thin
 // lines with multiple supporting samples retain their energy; no mip cap.
 float otherMean=max(0,lumaSum-peak-second)/max(1,count-2);
 float peakThreshold=max(third*4.,otherMean*6.)+.002;
 bool isolated=count>=6 && peak>peakThreshold;
 float3 sum=0,moment=0,lo=1e30,hi=0;float total=0;
 [unroll] for(int i=0;i<9;++i) {
  if(weights[i]==0)continue;
  float3 c=samples[i];float lum=Luma(c);
  if(isolated && lum>peakThreshold)c*=otherMean/max(lum,1e-8);
  sum+=c*weights[i];moment+=c*c*weights[i];total+=weights[i];
  lo=min(lo,c);hi=max(hi,c);
 }
 float3 current=total>0?sum/total:center;
 float3 variance=total>0?max(0,moment/total-current*current):0;
 float3 deviation=sqrt(variance);
 // Clip against cleaned local statistics, never against the original firefly.
 float3 lower=max(0,max(lo,current-2.*deviation)-.00001);
 float3 upper=min(hi,current+2.*deviation)+.00001;
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
   history=clamp(history,lower,upper);
   // Reprojection/visibility tests establish confidence. Screen speed alone
   // must not throw away valid history and expose noisy new samples.
   float blend=.94*validWeight;
   result=lerp(current,history,blend);
  }
 }
 // Native output ABI duplicates red in alpha. No metadata is stored in alpha.
 Output[p]=float4(result,result.r);
}
