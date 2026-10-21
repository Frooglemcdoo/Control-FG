#include "../rt_lights/layout.h"
#include "rt_lights_native_geometry.hlsli"
Texture2D<float4> gbuffer : register(t0);
Texture2D<float4> clipDepth : register(t1);
Texture2D<uint4> spotCells : register(t2);
StructuredBuffer<NativeSpotLight> spotLights : register(t3);
RWTexture2D<uint4> work : register(u0);
RWStructuredBuffer<ShadowTask> tasks : register(u1);
RWStructuredBuffer<ReceiverRange> ranges : register(u2);
cbuffer NativeDeferred : register(b1) { uint4 deferredWords[26]; }
cbuffer Batch : register(b2) {
    uint firstReceiver; uint receiverCount; uint slotsPerReceiver; uint frameTag;
    uint sampleCount; uint instanceMask; float originBias; float endpointBias;
    uint workWidth; uint workHeight; float emitterScale; uint reserved;
}
static const uint Invalid=0xffffffff;
static const uint NoLight=65535;
// Fixed per-receiver slots avoid atomic allocations and GPU/CPU count readback.
// A batch is bounded in receiver count, not truncated in light count.
[numthreads(64,1,1)]
void main(uint3 dispatch : SV_DispatchThreadID) {
    uint local=dispatch.x;
    if(any(dispatch.yz) || local>=receiverCount) return;
    uint rangeCount,rangeStride; ranges.GetDimensions(rangeCount,rangeStride);
    if(local>=rangeCount || rangeStride!=16) return;
    ReceiverRange range;range.first=Invalid;range.count=1;range.frameTag=frameTag;range.reserved=0;
    ranges[local]=range; // Invalid until all candidates and tasks are complete.
    uint taskCount,taskStride,atlasWidth,atlasHeight;
    tasks.GetDimensions(taskCount,taskStride);work.GetDimensions(atlasWidth,atlasHeight);
    if(slotsPerReceiver==0 || slotsPerReceiver>1024 || taskStride!=96 ||
       workWidth==0 || workHeight==0 || workWidth!=atlasWidth || workHeight!=atlasHeight ||
       workHeight>Invalid/workWidth || local>Invalid/slotsPerReceiver) return;
    uint first=local*slotsPerReceiver, atlasCount=workWidth*workHeight;
    if(first>taskCount || slotsPerReceiver>taskCount-first ||
       first>atlasCount || slotsPerReceiver>atlasCount-first) return;
    for(uint slot=0;slot<slotsPerReceiver;++slot) {
        uint index=first+slot;
        work[uint2(index%workWidth,index/workWidth)]=0;
        tasks[index]=(ShadowTask)0;
    }
    uint width,height,depthWidth,depthHeight,lightCount,lightStride,cellWidth,cellHeight;
    gbuffer.GetDimensions(width,height);clipDepth.GetDimensions(depthWidth,depthHeight);
    spotLights.GetDimensions(lightCount,lightStride);spotCells.GetDimensions(cellWidth,cellHeight);
    uint nativeTotal=deferredWords[16].y, nativeCapacity=deferredWords[14].y;
    uint depthCount=deferredWords[10].z, depthLimit=deferredWords[12].z;
    if(width==0 || height==0 || width!=depthWidth || height!=depthHeight || height>Invalid/width ||
       firstReceiver>Invalid-local || firstReceiver+local>=width*height || lightStride!=320 ||
       nativeTotal>lightCount || nativeTotal>=NoLight || nativeCapacity==0 || nativeCapacity>slotsPerReceiver ||
       depthCount==0 || depthLimit==0 || depthLimit>depthCount || depthCount>cellHeight ||
       sampleCount==0 || sampleCount>64 || instanceMask==0 || instanceMask>255 ||
       !isfinite(originBias) || originBias<0 || !isfinite(endpointBias) || endpointBias<0 ||
       !isfinite(emitterScale) || emitterScale<0) return;
    uint receiver=firstReceiver+local;
    uint2 pixel=uint2(receiver%width,receiver/width);
    float4 packedNormal=gbuffer.Load(int3(pixel,0));float depth=clipDepth.Load(int3(pixel,0)).x;
    float2 inverseSize=systemWords[0].zw;
    if(!all(isfinite(packedNormal)) || any(packedNormal<0) || any(packedNormal>1) ||
       !isfinite(depth) || depth<0 || depth>1 || !all(isfinite(inverseSize)) ||
       any(abs(inverseSize*float2(width,height)-1)>0.00001)) return;
    float2 clip=(float2(pixel)+0.5)*inverseSize*float2(2,-2)+float2(-1,1);
    float4 viewHomogeneous=NativeClipToView(float4(clip,depth,1));
    if(!all(isfinite(viewHomogeneous)) || abs(viewHomogeneous.w)<1e-20) return;
    float3 view=viewHomogeneous.xyz/viewHomogeneous.w;
    float3 world=NativeViewPositionToWorld(view);
    float3 normal=NativeViewDirectionToWorld(NativeViewNormal(packedNormal));
    float normalSquare=dot(normal,normal);
    if(!all(isfinite(world)) || !isfinite(normalSquare) || normalSquare<0.5 || normalSquare>1.5) return;
    normal=normalize(normal);
    float nearDepth=asfloat(deferredWords[13].z);
    float2 depthResolve=asfloat(deferredWords[10].xy);
    if(!isfinite(nearDepth) || !all(isfinite(depthResolve))) return;
    uint slice=0;
    if(view.z>nearDepth) {
        if(view.z<=0) return;
        // Native SHEX: ceil((resolve.y - ln(view.z))*resolve.x), capped at Z-1.
        float resolved=min(ceil((depthResolve.y-log(view.z))*depthResolve.x),float(depthLimit-1));
        if(!isfinite(resolved) || resolved<0) return;
        slice=uint(resolved);
    }
    uint2 tile=pixel>>6;
    if(tile.x>Invalid/nativeCapacity || tile.y>(Invalid-slice)/depthCount) return;
    uint cellX=tile.x*nativeCapacity, cellY=tile.y*depthCount+slice;
    if(cellX>cellWidth || nativeCapacity>cellWidth-cellX || cellY>=cellHeight) return;
    // Inspect the complete list before emitting work. A full row without a
    // sentinel is ambiguous (possibly truncated upstream), so retain native.
    uint count=0;
    for(;count<nativeCapacity;++count) {
        uint light=spotCells.Load(int3(cellX+count,cellY,0)).x;
        if(light==NoLight) break;
        if(light>=nativeTotal) return;
    }
    if(count==nativeCapacity) return;
    for(uint candidate=0;candidate<count;++candidate) {
        uint light=spotCells.Load(int3(cellX+candidate,cellY,0)).x;
        NativeSpotLight source=spotLights[light];
        // Native contact raygen targets vWedPositionInView (+16), not the
        // virtual cone position (+0). Do not subtract fNearDistance from reach.
        float3 center=NativeViewPositionToWorld(source.words[1].xyz);
        float radius=source.words[4].z*emitterScale;
        float3 delta=center-world;float distanceSquare=dot(delta,delta);
        if(!all(isfinite(center)) || !isfinite(radius) || radius<0 ||
           !isfinite(distanceSquare) || distanceSquare<=endpointBias*endpointBias) return;
        ShadowTask task=(ShadowTask)0;
        task.receiver=world;task.normal=normal;task.originBias=originBias;task.endpointBias=endpointBias;
        task.lightCenter=center;task.lightId=light;task.receiverId=receiver;task.frameTag=frameTag;
        task.sampleCount=sampleCount;task.instanceMask=instanceMask;
        NativeEmitterBasis(delta*rsqrt(distanceSquare),task.emitterAxisU,task.emitterAxisV);
        task.emitterAxisU*=radius;task.emitterAxisV*=radius;
        uint index=first+candidate;
        tasks[index]=task;
        work[uint2(index%workWidth,index/workWidth)]=uint4(light,pixel,1);
    }
    range.first=first;range.count=count;ranges[local]=range;
}
