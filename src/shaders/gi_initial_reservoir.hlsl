#include "gi_dispatch_gate.hlsl"
#include "../gi_reuse/initial_reservoir.h"
#include "../gi_reuse/producer_shading.h"
struct Batch {uint first;uint count;uint frameTag;float depthScale;};
#ifdef GI_NATIVE_INITIAL_BINDINGS
cbuffer GIInitialControls : register(b5){uint giInitialFrame;uint giInitialScene;uint giInitialCount;uint giInitialReserved;}
StructuredBuffer<GIShadedProducerSample> samples : register(t27);
StructuredBuffer<Batch> batches : register(t28);
RWStructuredBuffer<GIInitialReservoir> reservoirs : register(u2);
RWStructuredBuffer<GIInitialResult> outputs : register(u3);
#else
StructuredBuffer<GIShadedProducerSample> samples : register(t0);
StructuredBuffer<Batch> batches : register(t1);
RWStructuredBuffer<GIInitialReservoir> reservoirs : register(u0);
RWStructuredBuffer<GIInitialResult> outputs : register(u1);
#endif
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz))return;
#ifdef GI_NATIVE_INITIAL_BINDINGS
    if(!giInitialScene||!GIOwnedGateOpen()||giInitialReserved)return;
#endif
    uint n,s,nb,sb,nr,sr,no,so;
    batches.GetDimensions(nb,sb);samples.GetDimensions(n,s);
    reservoirs.GetDimensions(nr,sr);outputs.GetDimensions(no,so);
    if(id.x>=nr||id.x>=no||sr!=144||so!=48)return;
    reservoirs[id.x]=(GIInitialReservoir)0;outputs[id.x]=(GIInitialResult)0;
    if(id.x>=nb||sb!=16||s!=112)return;
    Batch batch=batches[id.x];
    if(batch.first>n||batch.count>n-batch.first)return;
    GIInitialReservoir reservoir=GIBeginInitialReservoir(batch.frameTag,batch.count,batch.depthScale);
    if(!reservoir.valid)return;
    for(uint i=0;i<batch.count;++i) {
        GIShadedProducerSample v=samples[batch.first+i];
        if(!isfinite(v.hitDistance)||v.hitDistance<0||v.reserved1||v.reserved2){reservoir.valid=0;break;}
        GIPoint normal;normal.x=v.geometricNormal.x;normal.y=v.geometricNormal.y;normal.z=v.geometricNormal.z;
        if(!GIStreamInitialReservoir(reservoir,v.shading,normal,v.geometryVerified,v.random))break;
    }
    reservoirs[id.x]=reservoir;outputs[id.x]=GIResolveInitialReservoir(reservoir);
}
