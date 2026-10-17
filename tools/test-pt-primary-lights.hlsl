// Constant fixture inputs make a few finite checks redundant under FXC.
#pragma warning(disable: 3577)
// FXC reports aggregate finite-check branches for constant fixture inputs.
// All fields are initialized; numerical results are checked by GPU readback.
#pragma warning(disable: 4000)
#define PT_DIRECT_LIGHT_ROUGHNESS 1
#include "../src/shaders/pt_native_primary_lights.hlsli"
#include "../src/shaders/pt_native_light_roughness.hlsli"
#include "../src/shaders/pt_direct_light.hlsli"
struct Input {float3 positionView;float choose;};
struct TestResult {PTPrimaryLightSample light;float effectiveRoughness;float specular;};
StructuredBuffer<Input> Inputs:register(t0,space4);
RWStructuredBuffer<TestResult> Results:register(u0,space4);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint count,stride;Inputs.GetDimensions(count,stride);if(id.x>=count)return;
    Input value=Inputs[id.x];TestResult result=(TestResult)0;
    PTSamplePrimaryLight(value.positionView,value.choose,result.light);
    if(result.light.active!=0){
        result.effectiveRoughness=PTNativeLightRoughness(.05,result.light.radius,result.light.distance,1);
        PTSurfaceMaterial m=(PTSurfaceMaterial)0;m.f0=.04;m.roughness=.05;m.lobes=2;
        PTDirectSample light=(PTDirectSample)0;light.wi=normalize(float3(.4,0,1));light.incidentRadiance=1;light.pdf=1;light.delta=1;light.roughness=result.effectiveRoughness;
        result.specular=PTEvaluateDirect(m,float3(0,0,1),light,1).radiance.x;
    }
    Results[id.x]=result;
}
