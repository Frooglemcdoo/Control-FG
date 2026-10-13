#include "../src/shaders/pt_native_primary_lights.hlsli"
struct Input {float3 positionView;float choose;};
StructuredBuffer<Input> Inputs:register(t0,space4);
RWStructuredBuffer<PTPrimaryLightSample> Results:register(u0,space4);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){uint count,stride;Inputs.GetDimensions(count,stride);if(id.x<count){Input value=Inputs[id.x];Results[id.x]=PTSamplePrimaryLight(value.positionView,value.choose);}}
