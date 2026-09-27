// GI29 direct shader replacement, derived from RenoDX Control RR revision 120663347.
// Diffuse temporal pre-accumulator: retain current-frame firefly clamp, no history.
#ifndef FIREFLY_CLAMP
#define FIREFLY_CLAMP 1.0
#endif
#ifndef TEMPORAL_WEIGHT
#define TEMPORAL_WEIGHT 0.0
#endif
cbuffer sys_constants : register(b0) {
  float2 g_vInvScreenRes : packoffset(c0.z);
  float2 g_vInvOutputRes : packoffset(c1.z);
  float4 g_vPrevViewToViewCol3 : packoffset(c37);
};
cbuffer deferredlight_filtering : register(b1) { uint g_uCameraCut : packoffset(c1.z); };
SamplerState linear_clamp : register(s6, space1);
SamplerState linear_mip_clamp : register(s11, space1);
Texture2D<float4> linear_depth : register(t0);
Texture2D<float4> prev_linear_depth : register(t1);
Texture2D<float4> velocity_tex : register(t2);
Texture2D<float3> source : register(t3);
Texture2D<float3> history_tex : register(t4);
RWTexture2D<float3> target : register(u0);
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 int2 p=int2(id.xy);float2 pix=(float2)p+0.5;float2 uv=pix*g_vInvScreenRes;
 const float3 kLum=float3(0.2126,0.7152,0.0722);
 float3 c=source[p];float3 c_blur=source.SampleLevel(linear_mip_clamp,uv,1.2);
 float lum_b=dot(c_blur,kLum),lum_c=dot(c,kLum);
 float inv_lum=(lum_c!=0.0)?1.0/lum_c:0.0;
 float3 c_clamped=min(lum_b,lum_c)*(c*inv_lum);
 {float cap=min(lum_b,lum_c);float ratio=(lum_c>0.0)?min(cap/lum_c,1.0):1.0;c_clamped=c*pow(ratio,(float)FIREFLY_CLAMP);}
 float3 result=c_clamped;
 static const float kTemporalWeight=TEMPORAL_WEIGHT;
 if(kTemporalWeight>0.0){
  float conf=(1.0-(float)g_uCameraCut)*kTemporalWeight;float2 vel_uv=pix*g_vInvOutputRes;
  float2 prev_uv=pix*g_vInvOutputRes+velocity_tex.SampleLevel(linear_clamp,vel_uv,0).xy;
  float cur_z=linear_depth.SampleLevel(linear_clamp,uv,0).x;
  float prev_z=prev_linear_depth.SampleLevel(linear_clamp,prev_uv,0).x+g_vPrevViewToViewCol3.z;
  conf*=(abs(prev_z-cur_z)/abs(cur_z)<=0.03)?1.0:0.0;conf*=max(1.0-40.0*length(pix*g_vInvScreenRes-prev_uv),0.0);
  float3 hist=history_tex.SampleLevel(linear_clamp,prev_uv,0);result=lerp(c_clamped,hist,conf);
 }
 target[p]=result;
}
