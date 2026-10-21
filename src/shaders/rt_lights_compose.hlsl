#include "../rt_lights/layout.h"
StructuredBuffer<Visibility> visibility : register(t0);
StructuredBuffer<Contribution> contributions : register(t1);
StructuredBuffer<ReceiverRange> receivers : register(t2);
RWStructuredBuffer<Lighting> outputLighting : register(u0);
cbuffer CompositionBatch : register(b0) {
    uint firstReceiver; uint receiverCount; uint frameTag; uint reserved;
}
void Compose(uint index,uint receiverId,bool enabled,bool checkFrame) {
    uint n,stride,outCount,outStride;
    receivers.GetDimensions(n,stride); outputLighting.GetDimensions(outCount,outStride);
    if(index>=outCount) return;
    Lighting result=(Lighting)0;
    outputLighting[index]=result;
    if(!enabled || index>=n || stride!=16 || outStride!=32) return;
    ReceiverRange range=receivers[index];
    if(checkFrame && range.frameTag!=frameTag) return;
    uint count,contributionStride,visibilityCount,visibilityStride;
    contributions.GetDimensions(count,contributionStride);
    visibility.GetDimensions(visibilityCount,visibilityStride);
    if(contributionStride!=48 || visibilityStride!=32 || range.first>count || range.count>count-range.first) return;
    for(uint i=0;i<range.count;++i) {
        Contribution light=contributions[range.first+i];
        if(light.receiverId!=receiverId || light.frameTag!=range.frameTag || light.visibilityIndex>=visibilityCount) return;
        Visibility shadow=visibility[light.visibilityIndex];
        if(shadow.valid!=1 || shadow.receiverId!=receiverId || shadow.lightId!=light.lightId ||
            shadow.frameTag!=range.frameTag || !isfinite(shadow.value) || shadow.value<0 || shadow.value>1 ||
            !all(isfinite(light.diffuse)) || !all(isfinite(light.specular))) return;
        result.diffuse+=light.diffuse*shadow.value;
        result.specular+=light.specular*shadow.value;
    }
    if(!all(isfinite(result.diffuse)) || !all(isfinite(result.specular))) return;
    result.valid=1; result.lightCount=range.count;
    outputLighting[index]=result;
}

// Original standalone entry retains its original resource contract.
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz)) return;
    Compose(id.x,id.x,true,false);
}
// Bounded batches use local output indices with global receiver identities.
[numthreads(64,1,1)]
void batchMain(uint3 id:SV_DispatchThreadID) {
    if(any(id.yz)) return;
    bool enabled=id.x<receiverCount && firstReceiver<=0xffffffff-id.x;
    Compose(id.x,enabled?firstReceiver+id.x:0,enabled,true);
}
