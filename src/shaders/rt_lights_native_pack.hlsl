#include "../rt_lights/layout.h"
// The atlas and tasks must come from the same immutable frame batch.
Texture2D<uint4> work : register(t0);
Texture2D<float4> diffuseAtlas : register(t1);
Texture2D<float4> specularAtlas : register(t2);
StructuredBuffer<ShadowTask> tasks : register(t3);
RWStructuredBuffer<Contribution> contributions : register(u0);
cbuffer Batch : register(b0) {
    uint receiverWidth; uint receiverHeight; uint frameTag; uint workCount;
}
[numthreads(64,1,1)]
void main(uint3 id : SV_DispatchThreadID) {
    if(any(id.yz)) return;
    uint index=id.x, outputCount, outputStride, taskCount, taskStride;
    contributions.GetDimensions(outputCount,outputStride);
    if(index>=outputCount) return;
    Contribution result=(Contribution)0;
    result.visibilityIndex=0xffffffff;
    contributions[index]=result;
    tasks.GetDimensions(taskCount,taskStride);
    if(index>=workCount || index>=taskCount || taskStride!=96 || outputStride!=48 ||
       receiverWidth==0 || receiverHeight==0 || receiverHeight>0xffffffff/receiverWidth) return;
    uint width,height,dw,dh,sw,sh;
    work.GetDimensions(width,height);
    diffuseAtlas.GetDimensions(dw,dh); specularAtlas.GetDimensions(sw,sh);
    if(width==0 || height==0 || width!=dw || width!=sw || height!=dh || height!=sh) return;
    uint2 pixel=uint2(index%width,index/width);
    if(pixel.y>=height) return;
    uint4 job=work.Load(int3(pixel,0));
    if(job.w!=1 || job.y>=receiverWidth || job.z>=receiverHeight) return;
    ShadowTask task=tasks[index];
    if(task.lightId!=job.x || task.receiverId!=job.z*receiverWidth+job.y || task.frameTag!=frameTag) return;
    float4 diffuse=diffuseAtlas.Load(int3(pixel,0));
    float4 specular=specularAtlas.Load(int3(pixel,0));
    if(diffuse.w!=1 || specular.w!=1 || !all(isfinite(diffuse)) || !all(isfinite(specular))) return;
    result.visibilityIndex=index;result.receiverId=task.receiverId;result.lightId=task.lightId;result.frameTag=frameTag;
    result.diffuse=diffuse.xyz;result.specular=specular.xyz;
    contributions[index]=result;
}
