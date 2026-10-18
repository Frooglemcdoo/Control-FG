// Derived from RenoDX revision 120663347a89ebe36c37eeb14653ad6b50958bc6.
// MIT license: legal/RENODX-LICENSE.txt
// Control - DeferredLightFiltering DIFFUSE temporal PRE-ACCUMULATOR.
// Role is Remedy's: this blob is deferredlight_filtering_diffuse__0.
// Replacement for 0x591FC46F: firefly clamp (luminance limited
// to a mip-blurred copy) followed by a history blend. Runs BEFORE the spatial
// passes — the main temporal stage for the diffuse channel.
// TEMPORAL_WEIGHT: 0.7 = vanilla, 0.0 = firefly clamp only, no accumulation.

// Final config: vanilla-strength clamp. This stage is part of the game's
// energy calibration (min(lum, local mip avg) biases reflection brightness
// down, and the art was tuned against that bias) — NOT just despeckling.
#ifndef FIREFLY_CLAMP
#define FIREFLY_CLAMP 1.0
#endif

#ifndef TEMPORAL_WEIGHT
#define TEMPORAL_WEIGHT 0.35
#endif

cbuffer sys_constants : register(b0) {
  float2 g_vInvScreenRes : packoffset(c0.z);
  float2 g_vInvOutputRes : packoffset(c1.z);
  float4 g_vPrevViewToViewCol3 : packoffset(c37);
};

cbuffer deferredlight_filtering : register(b1) {
  uint g_uCameraCut : packoffset(c1.z);
};

SamplerState linear_clamp : register(s6, space1);
SamplerState linear_mip_clamp : register(s11, space1);
Texture2D<float4> linear_depth : register(t0);
Texture2D<float4> prev_linear_depth : register(t1);
Texture2D<float4> velocity_tex : register(t2);
Texture2D<float3> source : register(t3);
Texture2D<float3> history_tex : register(t4);
RWTexture2D<float3> target : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id: SV_DispatchThreadID) {
  uint width,height;target.GetDimensions(width,height);
  if(any(id.xy>=uint2(width,height)))return;
  int2 p = int2(id.xy);
  float2 pix = (float2)p + 0.5;
  float2 uv = pix * g_vInvScreenRes;

  // firefly clamp: limit luminance to the mip-blurred local level
  const float3 kLum = float3(0.2126, 0.7152, 0.0722);
  float3 c = source[p];
  float3 c_blur = source.SampleLevel(linear_mip_clamp, uv, 1.2);
  float lum_b = dot(c_blur, kLum);
  float lum_c = dot(c, kLum);
  float inv_lum = (lum_c != 0.0) ? 1.0 / lum_c : 0.0;
  float3 c_clamped = min(lum_b, lum_c) * (c * inv_lum);

  // FIREFLY_CLAMP is a log-domain strength: 0 = off (raw), 1 = vanilla hard
  // cap; 0.5 removes half the excess *stops*. Linear lerp is useless here --
  // outliers are orders of magnitude, so perceptual blending must be
  // geometric: out = lum * (cap/lum)^s.
  {
    float cap = min(lum_b, lum_c);
    float ratio = (lum_c > 0.0) ? min(cap / lum_c, 1.0) : 1.0;
    c_clamped = c * pow(ratio, (float)FIREFLY_CLAMP);
  }
  float3 result = c_clamped;

  static const float kTemporalWeight = TEMPORAL_WEIGHT;
  if (kTemporalWeight > 0.0) {
    // R16: use output-pixel velocity sampling, map reprojected UV back
    // into the GI buffer domain, reject invalid/depth-disoccluded history.
    float2 vel_uv = pix * g_vInvOutputRes;
    float2 velocity = velocity_tex.SampleLevel(linear_clamp, vel_uv, 0).xy;
    float2 prev_uv = vel_uv + velocity;
    float cur_z = linear_depth.SampleLevel(linear_clamp, uv, 0).x;
    float prev_z = prev_linear_depth.SampleLevel(linear_clamp, prev_uv, 0).x
                 + g_vPrevViewToViewCol3.z;
    float motionPixels = length(velocity / max(g_vInvOutputRes, 1e-8));
    float conf = (g_uCameraCut == 0u) ? kTemporalWeight : 0.0;
    conf *= saturate(1.0 - motionPixels / 8.0);
    conf *= (abs(prev_z - cur_z) <= 0.01 * max(abs(cur_z), 1e-4)) ? 1.0 : 0.0;
    conf *= all(prev_uv >= 0.0) && all(prev_uv <= 1.0) ? 1.0 : 0.0;
    float3 hist = history_tex.SampleLevel(linear_clamp, prev_uv, 0);
    // Clamp stale illumination to a current 3x3 range, with a small
    // allowance for stochastic variation. Do not blur the current result.
    float3 lo=c_clamped,hi=c_clamped;
    [unroll] for(int dy=-1;dy<=1;++dy) [unroll] for(int dx=-1;dx<=1;++dx) {
      int2 q=clamp(p+int2(dx,dy),int2(0,0),int2(width,height)-1);
      float3 v=source[q];lo=min(lo,v);hi=max(hi,v);
    }
    float3 allowance=0.05*max(abs(c_clamped),0.01);
    hist=clamp(hist,lo-allowance,hi+allowance);
    if(!all(isfinite(hist)) || !all(isfinite(velocity)) || !isfinite(cur_z) || !isfinite(prev_z)) {
      conf=0.0;hist=c_clamped;
    }
    result = lerp(c_clamped, hist, conf);
  }

  target[p] = result;
}

