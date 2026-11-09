#include "gi_dispatch_gate.hlsl"
// A bounded tile of complete ray batches. The host supplies baseRange from the
// matched native ray dispatch, not an assumed field in GI6's evaluator constants.
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_evaluator_secondary.generated.hlsl"
#include "../gi_reuse/producer_ray.h"
#include "../gi_reuse/producer_sampling.h"
struct GIProposalBatch {uint first;uint count;uint frameTag;float depthScale;};
RWStructuredBuffer<GIProducerRay> giProposals : register(u2);
RWStructuredBuffer<GIProposalBatch> giProposalBatches : register(u3);
cbuffer GIProposalConstants : register(b5){
    uint giProposalFrame;uint giProposalScene;uint giProposalWidth;uint giProposalHeight;
    uint giProposalFirstPixel;uint giProposalPixels;uint giProposalRayCount;float giProposalBaseRange;
}
GIPoint GIProposalPoint(float3 p){GIPoint v;v.x=p.x;v.y=p.y;v.z=p.z;return v;}
float GIProposalDepthScale(float depth){
    float4 c=asfloat(giCB1[17]);
    float nearStart=c.x-c.y,farScale=c.w-c.z;
    float width=(c.y+c.x)-nearStart;
    float t=saturate((1.0f/width)*(depth-nearStart));
    return (t*t)*(3.0f-2.0f*t)*farScale+c.z;
}
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    if(any(id.yz)||!giProposalScene||!GIOwnedGateOpen())return;
    uint rays,rayStride,batches,batchStride;
    giProposals.GetDimensions(rays,rayStride);giProposalBatches.GetDimensions(batches,batchStride);
    if(id.x>=giProposalPixels||id.x>=batches||rayStride!=96||batchStride!=16)return;
    giProposalBatches[id.x]=(GIProposalBatch)0;
    uint count=giProposalRayCount;
    if(!count||count>64||id.x>0xffffffffu/count)return;
    uint first=id.x*count;
    if(first>rays||count>rays-first)return;
    for(uint i=0;i<count;++i)giProposals[first+i]=(GIProducerRay)0;
    uint width,height;g_tLinearDepth.GetDimensions(width,height);
    if(!width||!height||width!=giProposalWidth||height!=giProposalHeight||
       width>0xffffffffu/height||giProposalFirstPixel>=width*height||
       giProposalPixels>width*height-giProposalFirstPixel||!giProposalScene||
       count!=giCB4[0].z||!isfinite(giProposalBaseRange)||giProposalBaseRange<=0)return;
    uint pixel=giProposalFirstPixel+id.x;uint2 xy=uint2(pixel%width,pixel/width);
    GIShadingRegisters receiver;
    if(!GIInitializeReceiver(xy,receiver))return;
    float3 worldPosition=asfloat(receiver.r5.xyz),nativeNormal=asfloat(receiver.r7.xyz);
    float3 view=worldPosition-asfloat(giCB0[54].xyz);
    float depth=asfloat(receiver.r1.w),n2=dot(nativeNormal,nativeNormal),v2=dot(view,view);
    float3 throughput=asfloat(receiver.r3.xyz);
    float scale=GIProposalDepthScale(depth);
    if(!all(isfinite(worldPosition))||!all(isfinite(nativeNormal))||!isfinite(n2)||abs(n2-1)>0.01f||
       !isfinite(v2)||v2<=0||!isfinite(depth)||depth<0||!isfinite(scale)||scale<0||
       !all(isfinite(throughput))||any(throughput<0))return;
    // Native ray producer uses .002*depth normal offset; the evaluator's
    // receiver offset is .001*depth. Reusing r1.xyz as ray origin is incorrect.
    float3 origin=worldPosition-depth*0.001f*(view*rsqrt(v2))+depth*0.002f*nativeNormal;
    float3 normal=nativeNormal*rsqrt(n2);
    float3 tangent,bitangent;
    // Stable orthonormal basis for the owned cosine proposal (including poles).
    float signZ=normal.z>=0?1.0f:-1.0f;
    float a=-1.0f/(signZ+normal.z),b=normal.x*normal.y*a;
    tangent=float3(1+signZ*normal.x*normal.x*a,signZ*b,-signZ*normal.x);
    bitangent=float3(b,signZ+normal.y*normal.y*a,-normal.y);
    if(!all(isfinite(origin)))return;
    GIProposalBatch batch;batch.first=first;batch.count=count;batch.frameTag=giProposalFrame;batch.depthScale=scale;
    for(uint ray=0;ray<count;++ray){
        GIProducerCoordinates random=GIProducerSample(xy.x,xy.y,giProposalFrame,giCB4[0].w,ray,count);
        float cosine=sqrt(random.v),radius=sqrt(1-random.v),angle=6.283185307179586f*random.u;
        float3 direction=radius*cos(angle)*tangent+radius*sin(angle)*bitangent+cosine*normal;
        GIProducerRay proposal=(GIProducerRay)0;
        proposal.origin=GIProposalPoint(origin);proposal.direction=GIProposalPoint(direction);
        proposal.baseRange=giProposalBaseRange;proposal.cosine=cosine;
        proposal.primaryBrdfCos=GIProposalPoint(throughput*(cosine*0.3183098861837907f));
        proposal.pixelX=xy.x;proposal.pixelY=xy.y;proposal.frameTag=giProposalFrame;proposal.sceneRevision=giProposalScene;
        proposal.rayIndex=ray;proposal.configuredCount=count;proposal.depthScale=scale;
        proposal.selectionRandom=random.selection;proposal.rangeRandom=random.range;proposal.proposalVerified=random.valid;
        giProposals[first+ray]=proposal;
    }
    giProposalBatches[id.x]=batch;
}
