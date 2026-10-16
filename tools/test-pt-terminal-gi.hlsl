#define PT_GI_TEST 1
#include "../src/shaders/pt_terminal_gi.hlsli"
RWStructuredBuffer<float4> output : register(u0);
[numthreads(8,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    float3 n=id.x<2?float3(1,0,0):(id.x<4?float3(0,1,0):float3(0,0,1));
    n*=((id.x&1)!=0)?-1:1;
    if(id.x>=6)n=normalize(float3(1,1,0));
    output[id.x]=float4(PTTerminalGI(id.x==7?float3(3,3,3):float3(.5,.5,.5),n),1);
}
