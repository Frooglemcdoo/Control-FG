#include "gi_dispatch_gate.hlsl"
#include "../gi_reuse/output_resolve.h"
struct GIOutputBatch {uint first;uint count;uint frameTag;float depthScale;};
StructuredBuffer<GIInitialResult> giOutputResults : register(t27);
StructuredBuffer<GIShadedProducerSample> giOutputSamples : register(t28);
StructuredBuffer<GIOutputBatch> giOutputBatches : register(t29);
RWTexture2D<float4> giOutputColor : register(u0);
RWTexture2D<float4> giOutputHitInfo : register(u1);
cbuffer GIOutputConstants : register(b5) {
    uint giOutputFrame;uint giOutputScene;uint giOutputWidth;uint giOutputHeight;
    uint giOutputFirstPixel;uint giOutputPixels;uint giOutputRayCount;float giOutputBaseRange;
}
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz)||id.x>=giOutputPixels||!giOutputScene||!GIOwnedGateOpen()||!giOutputWidth||!giOutputHeight||
       giOutputHeight>0xffffffffu/giOutputWidth||!giOutputRayCount||giOutputRayCount>64||
       !isfinite(giOutputBaseRange)||giOutputBaseRange<=0)return;
    uint total=giOutputWidth*giOutputHeight;
    if(giOutputFirstPixel>=total||giOutputPixels>total-giOutputFirstPixel)return;
    uint cw,ch,hw,hh;giOutputColor.GetDimensions(cw,ch);giOutputHitInfo.GetDimensions(hw,hh);
    if(cw!=giOutputWidth||ch!=giOutputHeight||hw!=cw||hh!=ch)return;
    uint results,rs,samples,ss,batches,bs;
    giOutputResults.GetDimensions(results,rs);giOutputSamples.GetDimensions(samples,ss);
    giOutputBatches.GetDimensions(batches,bs);
    if(id.x>=results||id.x>=batches||rs!=48||ss!=112||bs!=16)return;
    GIOutputBatch batch=giOutputBatches[id.x];
    if(batch.frameTag!=giOutputFrame||batch.count!=giOutputRayCount||
       batch.first>samples||batch.count>samples-batch.first)return;
    GIOutputState state=GIBeginOutput(giOutputResults[id.x],giOutputFrame,batch.count,batch.depthScale);
    if(!state.valid)return;
    for(uint i=0;i<batch.count;++i)if(!GIAddOutputDistance(state,giOutputSamples[batch.first+i]))return;
    if(!GIOutputComplete(state))return;
    uint pixel=giOutputFirstPixel+id.x;uint2 at=uint2(pixel%cw,pixel/cw);
    // Invalid/disarmed batches preserve the original GI6 outputs. The host runs
    // this after native evaluation and supplies matching UAVs/barriers.
    giOutputColor[at]=float4(state.color.x,state.color.y,state.color.z,1);
    giOutputHitInfo[at]=state.distance.xxxx;
}
