// Standalone proposal-model validation kernel; not bound by the game.
#include "../gi_reuse/finite_range_proposal.h"
StructuredBuffer<float4> inputs : register(t0); // cosine, true distance, base range, verified
RWStructuredBuffer<GIRangeProposal> outputs : register(u0);
[numthreads(64,1,1)]
void main(uint3 id : SV_DispatchThreadID) {
 uint count,stride,nout,sout;inputs.GetDimensions(count,stride);outputs.GetDimensions(nout,sout);
 if(id.x>=count||id.x>=nout)return;
 float4 v=inputs[id.x];outputs[id.x]=GIMakeRangeProposal(v.x,v.y,v.z,v.w==1?1u:0u);
}
