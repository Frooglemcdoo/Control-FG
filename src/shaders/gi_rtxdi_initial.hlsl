#include "Rtxdi/GI/ReSTIRGIParameters.h"
#define RTXDI_GI_RESERVOIR_BUFFER giReservoirStorage
#define RTXDI_ENABLE_STORE_RESERVOIR 0
StructuredBuffer<RTXDI_PackedGIReservoir> giReservoirStorage : register(t2);
#include "../gi_reuse/rtxdi_initial.hlsli"
struct GIProducerSample {
    GIInitialInput shading;
    float3 secondaryGeometricNormal; uint geometryVerified;
};
StructuredBuffer<GIProducerSample> inputs : register(t0);
RWStructuredBuffer<GIInitialResult> outputs : register(u0);
cbuffer Parameters : register(b0) { uint frameTag; }
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint count,stride,outCount,outStride;inputs.GetDimensions(count,stride);outputs.GetDimensions(outCount,outStride);
 if(id.x>=outCount) return;
 outputs[id.x]=(GIInitialResult)0;
 if(id.x>=count||stride!=96||outStride!=48) return;
 GIProducerSample sample=inputs[id.x];RTXDI_GIReservoir reservoir;GIInitialResult estimate;
 GICreateRTXDIInitial(sample.shading,sample.secondaryGeometricNormal,sample.geometryVerified,frameTag,reservoir,estimate);
 outputs[id.x]=estimate;
}
