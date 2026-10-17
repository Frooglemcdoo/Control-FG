#ifndef CONTROL_PT_TERMINAL_GI
#define CONTROL_PT_TERMINAL_GI
// Diffuse GI-volume subset of native 4fc0a0e9... DWORD1377..2735.
// Evaluated once at a terminal hit or bounded miss, never at every bounce.
// No native direct lights, shadow atlases, reflection targets or RT GI outputs.
#include "pt_gi_volume_coverage.hlsli"
StructuredBuffer<uint> PTGITopNodes : register(t22);
StructuredBuffer<uint4> PTGIInternalNodes : register(t23);
Texture3D<float4> PTGIBricks[6] : register(t24);
#ifdef PT_GI_TEST
SamplerState PTGIClamp : register(s6);
#else
SamplerState PTGIClamp : register(s6,space1);
#endif
// First 48 bytes: native illuminationvolume b3. Last16: scale, admission.
cbuffer PTGIParameters : register(b5) { uint4 PTGIParams[4]; };
bool PTGIBrickPosition(PTGIVolume volume,uint3 cell,uint3 size,float3 fraction,out float3 texel){
    texel=0;
    uint topCount,topStride,nodeCount,nodeStride;
    PTGITopNodes.GetDimensions(topCount,topStride);PTGIInternalNodes.GetDimensions(nodeCount,nodeStride);
    if(topStride!=4||nodeStride!=16||any(size==0)||any(cell>=size))return false;
    // Check products before computing the native linear index.
    if(size.y>0xffffffffu/size.x||size.z>0xffffffffu/(size.x*size.y))return false;
    uint index=(cell.z*size.y+cell.y)*size.x+cell.x;
    if(volume.words[12]>=topCount||index>=topCount-volume.words[12])return false;
    uint node=PTGITopNodes[volume.words[12]+index];
    float extent=asfloat(volume.words[15]);
    if(!isfinite(extent)||extent<=0)return false;
    [loop] for(uint level=0;level<32;++level){
        uint brick=node&0x7fffffffu;
        if((node&0x80000000u)!=0||extent<=1e-5){
            if(brick>0xffffffffu-volume.words[14])return false;
            brick+=volume.words[14];
            texel=float3(brick&127u,(brick>>7)&127u,brick>>14)*4+fraction*3+.5;
            return all(isfinite(texel));
        }
        if(volume.words[13]>=nodeCount||node>=nodeCount-volume.words[13])return false;
        uint4 children=PTGIInternalNodes[volume.words[13]+node];
        uint3 subcell=(uint3)floor(fraction*3);
        if(any(subcell>2))return false;
        uint child=(subcell.z*3+subcell.y)*3+subcell.x,bit=1u<<child;
        if((children.x&bit)==0){
            if(node>0xffffffffu-volume.words[14])return false;
            brick=node+volume.words[14];
            texel=float3(brick&127u,(brick>>7)&127u,brick>>14)*4+fraction*3+.5;return true;
        }
        fraction=fraction*3-subcell;
        uint prefix=bit-1u;
        bool leaf=(children.y&bit)!=0;
        uint offset=countbits((leaf?children.y:(~children.y&children.x))&prefix);
        uint base=leaf?children.w:children.z;
        if(base>0x7fffffffu-offset)return false;
        node=(base+offset)|(leaf?0x80000000u:0u);extent*=1.0/3.0;
    }
    return false; // Malformed/cyclic tree must not hang the GPU.
}
bool PTLookupTerminalGI(float3 positionWorld,float3 normalWorld,out float3 radiance){
    radiance=0;
    if(PTGIParams[3].y!=1)return false;
    uint count,stride;PTGIVolumes.GetDimensions(count,stride);
    uint volumes=PTGIParams[0].w;
    float scale=asfloat(PTGIParams[3].x);
    float3 inverseSize=asfloat(PTGIParams[0].xyz),bias=asfloat(PTGIParams[1].xyz);
    if(stride!=80||volumes>384||volumes>count||!isfinite(scale)||scale<0||
       !all(isfinite(inverseSize))||any(inverseSize<=0)||!all(isfinite(bias)))return false;
    float3 sum=0;float weightSum=0;
    [loop] for(uint i=0;i<volumes;++i){
        float3 local;uint3 size;
        float weight=PTGIVolumeWeight(positionWorld,i,local,size);
        if(weight<=0)continue;
        PTGIVolume v=PTGIVolumes[i];
        float3 texel;
        if(!PTGIBrickPosition(v,(uint3)floor(local),size,frac(local),texel))continue;
        float3 uv=texel*inverseSize;
        if(any(uv<0)||any(uv>1)||!all(isfinite(uv)))continue;
        // Native volume basis is multiplied by the stored world-space hit normal.
        float3 direction=normalWorld.x*asfloat(uint3(v.words[0],v.words[1],v.words[2]))+
                         normalWorld.y*asfloat(uint3(v.words[3],v.words[4],v.words[5]))+
                         normalWorld.z*asfloat(uint3(v.words[6],v.words[7],v.words[8]));
        if(!all(isfinite(direction)))continue;
        float3 positive=saturate(direction),negative=saturate(-direction);
        positive*=positive;negative*=negative;
        float3 irradiance=PTGIBricks[0].SampleLevel(PTGIClamp,uv,0).rgb*positive.x+
                          PTGIBricks[1].SampleLevel(PTGIClamp,uv,0).rgb*negative.x+
                          PTGIBricks[2].SampleLevel(PTGIClamp,uv,0).rgb*positive.y+
                          PTGIBricks[3].SampleLevel(PTGIClamp,uv,0).rgb*negative.y+
                          PTGIBricks[4].SampleLevel(PTGIClamp,uv,0).rgb*positive.z+
                          PTGIBricks[5].SampleLevel(PTGIClamp,uv,0).rgb*negative.z;
        if(!all(isfinite(irradiance))||any(irradiance<0))continue;
        sum+=irradiance*weight;weightSum+=weight;
    }
    if(weightSum<=0)return false; // No invented ambient light outside authored volumes.
    radiance=(sum/weightSum+bias)*(scale*0.3183098861837907);
    if(!all(isfinite(radiance)))return false;
    radiance=max(radiance,0);return true;
}
float3 PTTerminalGI(float3 positionWorld,float3 normalWorld){
    float3 radiance;return PTLookupTerminalGI(positionWorld,normalWorld,radiance)?radiance:0;
}
#endif
