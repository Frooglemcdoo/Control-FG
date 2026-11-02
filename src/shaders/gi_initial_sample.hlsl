#include "../gi_reuse/initial_sample.h"
StructuredBuffer<GIInitialInput> inputs : register(t0);
RWStructuredBuffer<GIInitialResult> outputs : register(u0);
cbuffer Parameters : register(b0) { uint frameTag; }
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint count,stride,outCount,outStride;inputs.GetDimensions(count,stride);outputs.GetDimensions(outCount,outStride);
 if(id.x>=outCount) return;
 outputs[id.x]=(GIInitialResult)0;
 if(id.x>=count||stride!=80||outStride!=48) return;
 outputs[id.x]=GIMakeInitialSample(inputs[id.x],frameTag);
}
