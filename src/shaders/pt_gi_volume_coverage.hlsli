#ifndef CONTROL_PT_GI_VOLUME_COVERAGE
#define CONTROL_PT_GI_VOLUME_COVERAGE
// Shared membership/weight calculation. Raygen checks only coverage; brick
// traversal and irradiance sampling remain in the post-trace compute pass.
struct PTGIVolume { uint words[20]; };
StructuredBuffer<PTGIVolume> PTGIVolumes : register(t21);
cbuffer PTGITopLevel : register(b4) { uint4 PTGITop[1536]; };
float PTGIVolumeWeight(float3 positionWorld,uint i,out float3 local,out uint3 size){
    uint base=i*4;float4 world=float4(positionWorld,1);
    local=float3(dot(asfloat(PTGITop[base]),world),dot(asfloat(PTGITop[base+1]),world),dot(asfloat(PTGITop[base+2]),world));
    size=PTGITop[base+3].xyz;
    if(!all(isfinite(local))||any(local<0)||any(local>=float3(size)))return 0;
    PTGIVolume v=PTGIVolumes[i];
    if(v.words[14]==0xffffffffu)return 0;
    float3 fadeScale=asfloat(uint3(v.words[9],v.words[10],v.words[11]));
    float priority=asfloat(v.words[16]);
    if(!all(isfinite(fadeScale))||any(fadeScale<0)||!isfinite(priority)||priority<0)return 0;
    float3 edge=saturate(local*fadeScale)*saturate((float3(size)-local)*fadeScale);
    float fade=edge.x*edge.y*edge.z;
    float weight=fade*(1+(priority-1)*fade*fade*(3-2*fade));
    return isfinite(weight)&&weight>0?weight:0;
}
bool PTHasGIVolume(float3 position,uint volumes){
    uint count,stride;PTGIVolumes.GetDimensions(count,stride);
    if(stride!=80||volumes>384||volumes>count)return false;
    [loop] for(uint i=0;i<volumes;++i){float3 local;uint3 size;
        if(PTGIVolumeWeight(position,i,local,size)>0)return true;
    }
    return false;
}
#endif
