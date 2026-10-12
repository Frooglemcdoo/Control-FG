// Compile fixture only; resource layout is not a proposed Control binding ABI.
#include "../src/shaders/pt_surface_transport.hlsli"
StructuredBuffer<float4> inputs : register(t0);
RWStructuredBuffer<float4> outputs : register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    float4 a=inputs[id.x];
    PTSurfaceMaterial m;m.kd=float3(.4,.3,.2);m.f0=float3(.04,.04,.04);m.roughness=a.w;m.lobes=3;
    PTSurfaceSample s=PTSampleSurface(m,float3(0,0,1),a.x,a.y,a.z);
    PTTransportState p=PTContinue(PTBeginTransport(),s,a.z,3);
    p=PTAccumulate(p,float3(1,1,1));
    outputs[id.x]=float4(p.radiance,(float)p.alive);
}
