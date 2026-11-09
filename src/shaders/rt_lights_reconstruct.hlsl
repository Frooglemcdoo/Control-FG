#include "../rt_lights/layout.h"
StructuredBuffer<Visibility> visibility : register(t0);
StructuredBuffer<Contribution> contributions : register(t1);
StructuredBuffer<ReceiverRange> receivers : register(t2);
StructuredBuffer<ShadowTask> tasks : register(t3);
RWStructuredBuffer<Lighting> outputLighting : register(u0);
#include "rt_lights_reconstruction.hlsli"
cbuffer Reconstruction : register(b0) { ShadowTile tile; }
bool ReadShadow(uint index, ShadowTask task, out Visibility shadow) {
    uint count,stride;visibility.GetDimensions(count,stride);shadow=(Visibility)0;
    if(index>=count || stride!=32) return false;
    shadow=visibility[index];
    return shadow.receiverId==task.receiverId && shadow.lightId==task.lightId && shadow.frameTag==task.frameTag &&
        (shadow.valid==1 || shadow.valid==2) && isfinite(shadow.value) && shadow.value>=0 && shadow.value<=1;
}
bool Reconstruct(uint index, ShadowTask task, uint2 pixel, out float value) {
    value=0;Visibility center;
    if(!ReadShadow(index,task,center)) return false;
    // Preserve full-resolution results at geometry boundaries and tight contact.
    float reach=length(task.lightCenter-task.receiver);
    if(center.valid==1 && center.reserved1!=1 &&
       (center.reserved1==2 || center.nearestBlocker<max(0.01,reach*0.005))) { value=center.value;return true; }
    float sum=0,weightSum=0;
    if(center.valid==1 && center.reserved1!=1) {sum=center.value;weightSum=1;}
    int2 anchor=int2(pixel&~1u);
    [loop] for(int y=-8;y<=8;y+=4) [loop] for(int x=-8;x<=8;x+=4) {
        int2 candidate=anchor+int2(x,y);
        if(any(candidate<0) || candidate.x>=int(tile.frameWidth) || candidate.y>=int(tile.frameHeight)) continue;
        uint other;
        if(!FindShadowTask(tile,uint2(candidate),task.lightId,other) || other==index) continue;
        ShadowTask neighbour=tasks[other];
        if(!SameShadowSurface(task,neighbour)) continue;
        Visibility sample;
        if(!ReadShadow(other,neighbour,sample) || sample.valid!=1 || sample.reserved1==1) continue;
        float2 offset=float2(candidate)-float2(pixel);
        float weight=exp2(-dot(offset,offset)/32.0);
        // Geometry tests above keep the shadow signal on the receiving surface.
        sum+=sample.value*weight;weightSum+=weight;
    }
    if(weightSum<=0 || !isfinite(sum)) return false;
    value=saturate(sum/weightSum);return true;
}
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz) || id.x>=tile.receiverCount || tile.width==0 || tile.frameWidth==0) return;
    uint outputCount,outputStride;outputLighting.GetDimensions(outputCount,outputStride);
    if(id.x>=outputCount || outputStride!=32) return;
    Lighting result=(Lighting)0;outputLighting[id.x]=result;
    uint2 pixel=uint2(tile.originX,tile.originY)+uint2(id.x%tile.width,id.x/tile.width);
    if(pixel.x<tile.coreX || pixel.y<tile.coreY || pixel.x>=tile.coreX+tile.coreWidth || pixel.y>=tile.coreY+tile.coreHeight) return;
    uint rangeCount,rangeStride,taskCount,taskStride,contributionCount,contributionStride;
    receivers.GetDimensions(rangeCount,rangeStride);tasks.GetDimensions(taskCount,taskStride);contributions.GetDimensions(contributionCount,contributionStride);
    if(id.x>=rangeCount || rangeStride!=16 || taskStride!=96 || contributionStride!=48) return;
    ReceiverRange range=receivers[id.x];
    uint limit=min(taskCount,contributionCount);
    if(range.frameTag!=tile.frameTag || range.first>limit || range.count>limit-range.first || range.count>1024) return;
    for(uint i=0;i<range.count;++i) {
        uint index=range.first+i;ShadowTask task=tasks[index];Contribution light=contributions[index];
        if(task.receiverId!=pixel.y*tile.frameWidth+pixel.x || task.frameTag!=tile.frameTag || !ValidShadowContribution(light,task,index)) return;
        if(ZeroShadowContribution(light)) continue;
        float shadow;if(!Reconstruct(index,task,pixel,shadow)) return;
        result.diffuse+=light.diffuse*shadow;result.specular+=light.specular*shadow;
    }
    if(!all(isfinite(result.diffuse)) || !all(isfinite(result.specular))) return;
    result.valid=1;result.lightCount=range.count;outputLighting[id.x]=result;
}
