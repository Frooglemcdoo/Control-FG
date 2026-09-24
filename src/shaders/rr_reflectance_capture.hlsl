// Actual GPU image pass over immutable copies of a single captured frame.
// Outputs are unmasked candidates. No RR tags/evaluation use these buffers.
#include "ControlPrimaryGuideDecode.hlsli"
#include "rr_envbrdf_generated.hlsli"
ByteAddressBuffer Captured : register(t0);
RWByteAddressBuffer Result : register(u0);
cbuffer Layout : register(b0) {
    uint Width; uint Height; uint Records; uint Reserved;
    float ProjectionX; float ProjectionY;
};
float4 UnpackRGBA(uint packed) {
    return float4(packed&255u,(packed>>8)&255u,(packed>>16)&255u,packed>>24)/255.0f;
}
[numthreads(8,8,1)]
void GenerateReflectanceCapture(uint3 tid:SV_DispatchThreadID) {
    if(tid.x>=Width || tid.y>=Height) return;
    uint n=Width*Height;uint pixel=tid.y*Width+tid.x;
    uint2 albedo=Captured.Load2(n*8+pixel*8);
    Result.Store2(pixel*8,uint2(albedo.x,(albedo.y&65535u)|0x3c000000u));
    uint g2bytes=Captured.Load(n*4+pixel*4);
    uint id=(((g2bytes>>16)&255u)<<8)|(g2bytes>>24);
    uint outOffset=n*8+pixel*16;
    if(id>=Records) {Result.Store4(outOffset,uint4(0,0,0,0));return;}
    float4 g1=UnpackRGBA(Captured.Load(pixel*4));
    float3 f0=ControlDecodeMaterialF0(Captured.Load(n*16+id*8),float((g2bytes>>8)&255u)/255.0f);
    float3 ray=float3((2*(float(tid.x)+0.5f)/float(Width)-1)/ProjectionX,
        (1-2*(float(tid.y)+0.5f)/float(Height))/ProjectionY,1);
    float cosine=clamp(dot(ControlDecodeNormalView(g1),-normalize(ray)),-1.0f,1.0f);
    float roughness=ControlDecodeMaterialRoughness(g1);
    float3 specular=EnvBRDFApprox2(f0,roughness*roughness,cosine);
    Result.Store4(outOffset,asuint(float4(specular,1)));
}
