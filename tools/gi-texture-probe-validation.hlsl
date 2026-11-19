#include "../src/shaders/gi_texture_probe.hlsl"
Texture2D<float4> tex:register(t0);SamplerState samp:register(s0);
RWByteAddressBuffer result:register(u0);cbuffer Case:register(b0){uint testCase;}
[numthreads(1,1,1)]void main(){
 float2 uv=testCase<4?float2(.375,.625):(testCase<8?float2(0,1):float2(-.125,1.125));
 GITextureProbeRecord r=GIReadTextureProbe(tex,samp,uv,123,0,0,.5);
 if(any(GISampleSecondaryDiagnostic(tex,samp,uv,0)!=asfloat(r.words[3]))||any(GISampleSecondaryDiagnostic(tex,samp,uv,1)!=asfloat(r.words[11])))r.words[0].x=0;
 [unroll]for(uint i=0;i<12;++i)result.Store4(testCase*192+i*16,r.words[i]);
}
