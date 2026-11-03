#include "../gi_reuse/initial_reservoir.h"
struct ProducerSample {
    GIInitialInput shading;
    float3 geometricNormal; uint geometryVerified;
    float random; uint reserved0; uint reserved1; uint reserved2;
};
struct Batch {uint first;uint count;uint frameTag;float depthScale;};
StructuredBuffer<ProducerSample> samples : register(t0);
StructuredBuffer<Batch> batches : register(t1);
RWStructuredBuffer<GIInitialReservoir> reservoirs : register(u0);
RWStructuredBuffer<GIInitialResult> outputs : register(u1);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz))return;
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
        ProducerSample v=samples[batch.first+i];
        if(v.reserved0||v.reserved1||v.reserved2){reservoir.valid=0;break;}
        GIPoint normal;normal.x=v.geometricNormal.x;normal.y=v.geometricNormal.y;normal.z=v.geometricNormal.z;
        if(!GIStreamInitialReservoir(reservoir,v.shading,normal,v.geometryVerified,v.random))break;
    }
    reservoirs[id.x]=reservoir;outputs[id.x]=GIResolveInitialReservoir(reservoir);
}
