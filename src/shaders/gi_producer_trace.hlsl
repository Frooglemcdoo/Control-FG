// Owned ray generation + accepted geometry + native-policy alpha. The native
// GI6 root can bind this pass using spare t27/t28/u2/b5 slots in a PRIVATE table.
// No descriptor in Control's table is overwritten by this shader contract.
#include "gi_producer_alpha.hlsl"
#include "../gi_reuse/producer_ray.h"
StructuredBuffer<GIProducerRay> giRays : register(t27);
RaytracingAccelerationStructure giProducerScene : register(t28);
RWStructuredBuffer<GIProducerShadingRequest> giAcceptedRequests : register(u2);
cbuffer GIProducerTraceConstants : register(b5) {
    uint giTraceFrame;uint giTraceScene;uint giTraceCount;uint giTraceReserved;
}
float3 GITraceVector(GIPoint p){return float3(p.x,p.y,p.z);}
[shader("raygeneration")]
void giProduceAcceptedHits() {
    if(any(DispatchRaysIndex().yz)||!giTraceScene)return;
    uint index=DispatchRaysIndex().x,inputs,inputStride,outputs,outputStride;
    giRays.GetDimensions(inputs,inputStride);giAcceptedRequests.GetDimensions(outputs,outputStride);
    if(index>=outputs||outputStride!=160)return;
    giAcceptedRequests[index]=(GIProducerShadingRequest)0;
    if(index>=inputs||index>=giTraceCount||inputStride!=96||!giTraceScene||giTraceReserved)return;
    GIProducerRay proposal=giRays[index];
    if(proposal.frameTag!=giTraceFrame||proposal.sceneRevision!=giTraceScene||
       proposal.proposalVerified!=1||proposal.reserved0||proposal.reserved1||
       !proposal.configuredCount||proposal.configuredCount>64||proposal.rayIndex>=proposal.configuredCount||
       !GIFinitePoint(proposal.origin)||!GIFinitePoint(proposal.direction)||
       abs(GIDot(proposal.direction,proposal.direction)-1)>0.001f||
       !GINonnegative(proposal.primaryBrdfCos)||!isfinite(proposal.cosine)||proposal.cosine<=0||proposal.cosine>1||
       !isfinite(proposal.baseRange)||proposal.baseRange<=0||!isfinite(proposal.tMin)||proposal.tMin<0||
       !isfinite(proposal.depthScale)||proposal.depthScale<0||
       !isfinite(proposal.selectionRandom)||proposal.selectionRandom<0||proposal.selectionRandom>=1||
       !isfinite(proposal.rangeRandom)||proposal.rangeRandom<0||proposal.rangeRandom>=1)return;
    RayDesc ray;
    ray.Origin=GITraceVector(proposal.origin);ray.Direction=GITraceVector(proposal.direction);
    ray.TMin=proposal.tMin;ray.TMax=proposal.baseRange*(0.8f+0.4f*proposal.rangeRandom);
    if(!isfinite(ray.TMax)||ray.TMin>=ray.TMax)return;
    float3 endpoint=ray.Origin+ray.Direction*ray.TMax;
    if(!all(isfinite(endpoint)))return;
    GIProducerHit hit=(GIProducerHit)0;hit.nativeValue=proposal.rayIndex;hit.material=65534;
    // Preserve native mask, flags, even hit-table rows and alpha cutout policy.
    TraceRay(giProducerScene,RAY_FLAG_NONE,1,0,2,0,ray,hit);
    GIProducerShadingRequest result=(GIProducerShadingRequest)0;
    if(hit.material!=65535){
        if(hit.valid!=1||hit.material>=65534||!isfinite(hit.rayT)||hit.rayT<ray.TMin||hit.rayT>ray.TMax)return;
        GIRangeProposal range=GIMakeRangeProposal(proposal.cosine,hit.rayT,proposal.baseRange,proposal.proposalVerified);
        if(!range.valid)return;
        result.sourcePdf=range.surfacePdf;
        result.primaryBrdfCos.x=proposal.primaryBrdfCos.x*range.reachProbability;
        result.primaryBrdfCos.y=proposal.primaryBrdfCos.y*range.reachProbability;
        result.primaryBrdfCos.z=proposal.primaryBrdfCos.z*range.reachProbability;
    }
    result.hit=hit;result.pixelX=proposal.pixelX;result.pixelY=proposal.pixelY;
    result.frameTag=proposal.frameTag;result.sceneRevision=proposal.sceneRevision;
    result.rayIndex=proposal.rayIndex;result.configuredCount=proposal.configuredCount;
    result.depthScale=proposal.depthScale;result.random=proposal.selectionRandom;
    result.missEndpoint.x=endpoint.x;result.missEndpoint.y=endpoint.y;result.missEndpoint.z=endpoint.z;
    result.producerVerified=1;giAcceptedRequests[index]=result;
}
