Texture2D<float4> Denoised : register(t0);
Texture2D<float4> RemodulationFactor : register(t1);
Texture2D<float4> NativeGI : register(t2);
RWTexture2D<float4> NativeOutput : register(u0);
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID){
 uint w,h;NativeOutput.GetDimensions(w,h);if(any(tid.xy>=uint2(w,h)))return;
 int3 p=int3(tid.xy,0);float4 factor=RemodulationFactor.Load(p);
 float3 rgb=Denoised.Load(p).rgb*factor.rgb;
 // Rejected guides preserve native energy. Never manufacture black holes.
 if(factor.a==0 || !all(isfinite(rgb)) || any(rgb<0))rgb=NativeGI.Load(p).rgb;
 NativeOutput[tid.xy]=float4(rgb,rgb.r);
}
