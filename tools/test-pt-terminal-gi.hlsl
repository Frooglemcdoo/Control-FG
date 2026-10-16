#define PT_GI_TEST 1
#include "../src/shaders/pt_terminal_gi.hlsli"
RWStructuredBuffer<float4> output : register(u0);
[numthreads(8,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    float3 n=0;uint axis=id.x/2;
    if(id.x<6)n[axis]=(id.x&1)?-1:1;
    else n=normalize(float3(1,1,0));
    output[id.x]=float4(PTTerminalGI(id.x==7?float3(3,3,3):float3(.5,.5,.5),n),1);
}
