// Native GI6 resource contract plus unused native table slots t27/u2/b5.
// This pass shades accepted producer hits at their target receiver. It does not
// replace the native evaluator dispatch until producer/scene ownership is bound.
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_evaluator_secondary.generated.hlsl"
#include "../gi_reuse/producer_shading.h"
StructuredBuffer<GIProducerShadingRequest> giProducerRequests : register(t27);
RWStructuredBuffer<GIShadedProducerSample> giProducerSamples : register(u2);
cbuffer GIProducerShadingConstants : register(b5) {
    uint giProducerFrame;uint giProducerScene;uint giProducerCount;uint giProducerReserved;
}
float3 GIVector(GIPoint p) {return float3(p.x,p.y,p.z);}
GIPoint GIPointFromVector(float3 v) {GIPoint p;p.x=v.x;p.y=v.y;p.z=v.z;return p;}
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz))return;
    uint inputs,inputStride,outputs,outputStride;
    giProducerRequests.GetDimensions(inputs,inputStride);
    giProducerSamples.GetDimensions(outputs,outputStride);
    if(id.x>=outputs||outputStride!=112)return;
    giProducerSamples[id.x]=(GIShadedProducerSample)0;
    if(id.x>=inputs||id.x>=giProducerCount||inputStride!=160||giProducerReserved)return;
    GIProducerShadingRequest request=giProducerRequests[id.x];
    if(request.frameTag!=giProducerFrame||request.sceneRevision!=giProducerScene||
       request.producerVerified!=1||!request.configuredCount||request.configuredCount>64||
       request.rayIndex>=request.configuredCount||!isfinite(request.depthScale)||request.depthScale<0||
       !isfinite(request.random)||request.random<0||request.random>=1)return;
    uint width,height;
    g_tLinearDepth.GetDimensions(width,height);
    if(request.pixelX>=width||request.pixelY>=height)return;
    GIProducerHit hit=request.hit;
    if(hit.reserved||hit.reserved1||hit.reserved2||hit.reserved3)return;
    const bool miss=hit.material==65535;
    float3 worldHit=miss?GIVector(request.missEndpoint):GIVector(hit.position);
    if(!all(isfinite(worldHit)))return;
    if(!miss&&(!hit.valid||hit.material>=65534||!GIFinitePoint(hit.shadingNormal)||
       !GIFinitePoint(hit.geometricNormal)||abs(GIDot(hit.geometricNormal,hit.geometricNormal)-1)>0.001f||
       !isfinite(hit.texcoordU)||!isfinite(hit.texcoordV)||
       !isfinite(request.sourcePdf)||request.sourcePdf<=0||!GINonnegative(request.primaryBrdfCos)))return;
    GIShadingRegisters state;
    if(!GIInitializeReceiver(uint2(request.pixelX,request.pixelY),state))return;
    GIEvaluateSecondaryWorld(state,hit.material,worldHit,
        miss?float3(0,0,0):GIVector(hit.shadingNormal),float2(hit.texcoordU,hit.texcoordV));
    GIShadedProducerSample result=(GIShadedProducerSample)0;
    result.shading.secondaryPosition=GIPointFromVector(worldHit);
    result.shading.material=hit.material;
    result.shading.frameTag=request.frameTag;
    result.shading.rayIndex=request.rayIndex;
    result.shading.configuredCount=request.configuredCount;
    result.shading.depthScale=request.depthScale;
    if(miss) {
        // This is already a primary-throughput-weighted fallback, not radiance.
        result.shading.fallbackContribution=GIPointFromVector(asfloat(state.r3.xyz)*asfloat(state.r15.xyz));
    } else {
        result.shading.secondaryRadiance=GIPointFromVector(asfloat(state.r15.xyz));
        result.shading.primaryBrdfCos=request.primaryBrdfCos;
        result.shading.sourcePdf=request.sourcePdf;
        result.shading.verified=7;
        result.geometricNormal=hit.geometricNormal;result.geometryVerified=1;
    }
    result.random=request.random;
    // Preserve GI6's distance convention, including its base-range value for
    // misses. RR hit distance is not the randomly selected reservoir distance.
    result.hitDistance=asfloat(state.r5.w);
    if(!isfinite(result.hitDistance)||result.hitDistance<0)return;
    if(!GIMakeInitialSample(result.shading,giProducerFrame).valid)return;
    giProducerSamples[id.x]=result;
}
