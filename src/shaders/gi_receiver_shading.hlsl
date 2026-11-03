// Receiver shading consumes actual trace output in a separate SRV, after the
// host has inserted a UAV->SRV transition. No CPU-manufactured visibility flags.
#include "../gi_reuse/receiver_input.h"
StructuredBuffer<GIReceiverInput> inputs : register(t0);
StructuredBuffer<GIConnectionVisibility> visibility : register(t1);
RWStructuredBuffer<GIInitialResult> outputs : register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz)) return;
    uint n,s,nv,sv,no,so;
    inputs.GetDimensions(n,s);visibility.GetDimensions(nv,sv);outputs.GetDimensions(no,so);
    if(id.x>=no||so!=48) return;
    outputs[id.x]=(GIInitialResult)0;
    if(id.x>=n||id.x>=nv||s!=192||sv!=32) return;
    GIReceiverInput v=inputs[id.x];
    outputs[id.x]=GIShadeReuseCandidate(v.connection,v.receiver,v.sourceRange,v.receiverRange,
        v.sourceCosine,v.receiverCosine,v.producerVerified,v.radianceVerified,v.sceneRevision,visibility[id.x],v.frameTag);
}
