cbuffer PTGlassDispatch : register(b6) {
    uint ptGlassEpoch; uint unused; uint2 ptGlassExtent;
    uint ptGlassMode; uint ptGlassHistorySerial; uint testCase; float inputLight;
};
#include "../src/shaders/pt_glass_temporal.hlsli"
RWStructuredBuffer<float4> results : register(u0);
[numthreads(1,1,1)]
void main() {
    float3 p=testCase==1?float3(1,0,0):float3(0,0,0);
    float3 n=testCase==2?float3(0,0,-1):float3(0,0,1);
    uint material=testCase==3?7u:1u;
    float3 value=inputLight.xxx;
    PTGlassAccumulate(uint2(0,0),p,n,material,float3(1,1,1),value);
    results[ptGlassEpoch-1]=float4(value,ptGlassHistory[0].illumination_count.w);
}
