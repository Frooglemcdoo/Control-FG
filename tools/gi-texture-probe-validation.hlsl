#include "../src/shaders/gi_texture_probe.hlsl"
Texture2D<float4> tex:register(t0);SamplerState samp:register(s0);
RWByteAddressBuffer result:register(u0);cbuffer Case:register(b0){uint testCase;}
[numthreads(1,1,1)]void main(){
 GITextureProbeRecord r=GIReadTextureProbe(tex,samp,float2(.375,.625),123,0,0,.5);
 [unroll]for(uint i=0;i<8;++i)result.Store4(testCase*128+i*16,r.words[i]);
}
