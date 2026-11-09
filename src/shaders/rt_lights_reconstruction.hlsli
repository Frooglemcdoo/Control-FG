// Included after declaring tasks and receivers; shared by visibility and reconstruction.
#include "../rt_lights/tile_layout.h"
bool FindShadowTask(ShadowTile tile, uint2 pixel, uint lightId, out uint index) {
    index=0;
    if(any(pixel<uint2(tile.originX,tile.originY))) return false;
    uint2 local=pixel-uint2(tile.originX,tile.originY);
    if(local.x>=tile.width || local.y>=tile.height) return false;
    uint rangeCount,rangeStride,taskCount,taskStride;
    receivers.GetDimensions(rangeCount,rangeStride);tasks.GetDimensions(taskCount,taskStride);
    uint receiverIndex=local.y*tile.width+local.x;
    if(rangeStride!=16 || taskStride!=96 || receiverIndex>=rangeCount) return false;
    ReceiverRange range=receivers[receiverIndex];
    if(range.frameTag!=tile.frameTag || range.first>taskCount || range.count>taskCount-range.first || range.count>1024) return false;
    uint id=pixel.y*tile.frameWidth+pixel.x;
    for(uint i=0;i<range.count;++i) {
        ShadowTask task=tasks[range.first+i];
        if(task.lightId==lightId && task.receiverId==id && task.frameTag==tile.frameTag) {
            index=range.first+i;return true;
        }
    }
    return false;
}
bool SameShadowSurface(ShadowTask a, ShadowTask b) {
    float3 delta=b.receiver-a.receiver;
    float distance=length(delta);
    if(!isfinite(distance) || !all(isfinite(a.normal)) || !all(isfinite(b.normal)) || dot(a.normal,b.normal)<0.95) return false;
    float tolerance=max(0.002,distance*0.03);
    return abs(dot(delta,a.normal))<=tolerance && abs(dot(delta,b.normal))<=tolerance;
}
bool ValidShadowContribution(Contribution c, ShadowTask task, uint index) {
    return c.visibilityIndex==index && c.receiverId==task.receiverId && c.lightId==task.lightId &&
        c.frameTag==task.frameTag && all(isfinite(c.diffuse)) && all(isfinite(c.specular));
}
bool ZeroShadowContribution(Contribution c) { return all(c.diffuse==0) && all(c.specular==0); }
