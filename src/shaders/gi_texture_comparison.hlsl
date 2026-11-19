#ifndef GI_TEXTURE_COMPARISON
#define GI_TEXTURE_COMPARISON
#include "gi_texture_bilinear.hlsl"
SamplerState giDiagnosticLinearWrap:register(s0,space19);
float4 GISampleSecondaryDiagnostic(Texture2D<float4> tex,SamplerState nativeSampler,float2 uv,float mode){
 if(mode<.5)return tex.SampleLevel(nativeSampler,uv,0);
 return GILoadBilinear(tex,uv,1);
}
#endif
