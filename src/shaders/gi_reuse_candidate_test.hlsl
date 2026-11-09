// Contract validation only; the application must supply real trace evidence.
#include "../gi_reuse/reuse_candidate.h"
struct Input {
 GIConnection connection;
 GIInitialInput receiver;
 GIConnectionVisibility visibility;
 float sourceRange; float receiverRange; float sourceCosine; float receiverCosine;
 uint producerVerified; uint radianceVerified; uint sceneRevision; uint frameTag;
};
StructuredBuffer<Input> inputs : register(t0);
RWStructuredBuffer<GIInitialResult> outputs : register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint n,s,no,so;inputs.GetDimensions(n,s);outputs.GetDimensions(no,so);
 if(id.x>=n||id.x>=no)return;
 Input v=inputs[id.x];
 outputs[id.x]=GIShadeReuseCandidate(v.connection,v.receiver,v.sourceRange,v.receiverRange,
 v.sourceCosine,v.receiverCosine,v.producerVerified,v.radianceVerified,v.sceneRevision,v.visibility,v.frameTag);
}
