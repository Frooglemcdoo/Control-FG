cbuffer PTGlassDispatch : register(b6) {
    uint ptGlassEpoch; uint unused; uint2 ptGlassExtent;
    uint ptGlassMode; uint ptGlassHistorySerial; uint testCase; float inputLight;
};
#include "../src/shaders/pt_glass_temporal.hlsli"
static const uint ptGlassCapture=1u;
#include "../src/shaders/pt_glass_diagnostics.hlsli"
RWStructuredBuffer<float4> results : register(u0);
[numthreads(1,1,1)]
void main() {
    float3 p=testCase==1?float3(1,0,0):float3(0,0,0);
    float3 n=testCase==2?float3(0,0,-1):float3(0,0,1);
    uint material=testCase==3?7u:1u;
    float3 value=inputLight.xxx;
    float count=PTGlassAccumulate(uint2(0,0),p,n,material,float3(1,1,1),value);
    PTGlassDiagnostic diagnostic=(PTGlassDiagnostic)0;
    diagnostic.identity=uint4(ptGlassEpoch,1u,ptGlassMode,0u);
    diagnostic.history=float4(count,count>1?1.0f:0.0f,saturate((count-1)/7),1.0f);
    diagnostic.levels=float4(1,inputLight,inputLight,PTGlassLuma(value));
    diagnostic.pointStats=float4(2,1,1.25f,3);
    diagnostic.spot=float4(4,2,2.5f,3);
    diagnostic.output=float4(2,-0.125f,0.25f,0);
    PTGlassDiagnosticStore(uint2(0,0),ptGlassEpoch-1,diagnostic);
    results[ptGlassEpoch-1]=float4(value,ptGlassHistory[0].illumination_count.w);
}
