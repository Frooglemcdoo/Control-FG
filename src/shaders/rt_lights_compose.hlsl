#include "../rt_lights/layout.h"
StructuredBuffer<Visibility> visibility : register(t0);
StructuredBuffer<Contribution> contributions : register(t1);
StructuredBuffer<ReceiverRange> receivers : register(t2);
RWStructuredBuffer<Lighting> outputLighting : register(u0);
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    uint index=id.x,n,stride,outCount,outStride;
    receivers.GetDimensions(n,stride); outputLighting.GetDimensions(outCount,outStride);
    if(index>=outCount) return;
    Lighting result=(Lighting)0;
    outputLighting[index]=result;
    if(index>=n || stride!=16 || outStride!=32) return;
    ReceiverRange range=receivers[index];
    uint count,contributionStride,visibilityCount,visibilityStride;
    contributions.GetDimensions(count,contributionStride);
    visibility.GetDimensions(visibilityCount,visibilityStride);
    if(contributionStride!=48 || visibilityStride!=32 || range.first>count || range.count>count-range.first) return;
    for(uint i=0;i<range.count;++i) {
        Contribution light=contributions[range.first+i];
        if(light.receiverId!=index || light.frameTag!=range.frameTag || light.visibilityIndex>=visibilityCount) return;
        Visibility shadow=visibility[light.visibilityIndex];
        if(shadow.valid!=1 || shadow.receiverId!=index || shadow.lightId!=light.lightId ||
            shadow.frameTag!=range.frameTag || !isfinite(shadow.value) || shadow.value<0 || shadow.value>1 ||
            !all(isfinite(light.diffuse)) || !all(isfinite(light.specular))) return;
        result.diffuse+=light.diffuse*shadow.value;
        result.specular+=light.specular*shadow.value;
    }
    result.valid=1; result.lightCount=range.count;
    outputLighting[index]=result;
}
