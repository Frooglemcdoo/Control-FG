#ifndef CONTROL_PT_NATIVE_PRIMARY_LIGHTS
#define CONTROL_PT_NATIVE_PRIMARY_LIGHTS
// Primary compute shader a9e5b544...: these differ from deferred-reflection
// bindings. The host must snapshot the primary table and b2, not reuse b1.
struct PTPrimaryPoint { uint words[40]; };
struct PTPrimarySpot { uint words[80]; };
struct PTPrimarySun { uint words[144]; };
StructuredBuffer<PTPrimaryPoint> PTPrimaryPoints : register(t10);
StructuredBuffer<PTPrimarySpot> PTPrimarySpots : register(t15);
StructuredBuffer<PTPrimarySun> PTPrimarySuns : register(t18);
Texture2D<float4> PTPrimarySpotProjection : register(t14);
Texture2D<float4> PTPrimarySunProjection : register(t17);
SamplerState PTPrimaryProjectionClamp : register(s11,space1);
SamplerState PTPrimaryProjectionWrap : register(s8,space1);
cbuffer PTPrimaryLightConstants : register(b2) { float4 PTPrimaryLightWords[26]; };
struct PTPrimaryLightSample {
    float3 directionView;
    float3 radiance;
    float3 targetView;
    float selectionPdf;
    uint valid;
    uint active;
    uint directional;
    float radius;
    float distance;
    uint castsShadow;
};
float PTPrimaryRangeFade(float range,float distance) {
    float t=saturate((range-distance)/(0.2*range));return t*t*(3-2*t);
}
// Uniform sampling includes out-of-range lights as zero-contribution samples.
// Do not resample them without also changing the selection probability.
// Point/spot/sun are treated as delta sources here, with native attenuation and
// projection. castsShadow retains the authored flag as metadata; the full PT
// adapter traces visibility for every sampled source, including native fill.
void PTSamplePrimaryLight(float3 positionView,float choose,out PTPrimaryLightSample result) {
    result.directionView=0;result.radiance=0;result.targetView=0;result.selectionPdf=0;
    result.valid=0;result.active=0;result.directional=0;result.radius=0;result.distance=0;result.castsShadow=1;
    if(!all(isfinite(positionView))||!isfinite(choose)||choose<0||choose>=1)return;
    uint3 counts=asuint(PTPrimaryLightWords[16].xyz);
    uint pointCount,pointStride,spotCount,spotStride,sunCount,sunStride;
    PTPrimaryPoints.GetDimensions(pointCount,pointStride);PTPrimarySpots.GetDimensions(spotCount,spotStride);PTPrimarySuns.GetDimensions(sunCount,sunStride);
    // The reviewed native shader consumes only sun element zero. More than one
    // declared sun is not silently combined under an unreviewed interpretation.
    if(counts.x>65535||counts.y>65535||counts.z>1||counts.x>pointCount||counts.y>spotCount||counts.z>sunCount||
       (counts.x!=0&&pointStride!=160)||(counts.y!=0&&spotStride!=320)||(counts.z!=0&&sunStride!=576))return;
    uint total=counts.x+counts.y+counts.z;result.valid=1;
    if(total==0)return;
    result.selectionPdf=1.0/total;uint selected=min((uint)(choose*total),total-1);
    float3 color=0,target=0;float attenuation=0;
    if(selected<counts.x){
        PTPrimaryPoint p=PTPrimaryPoints[selected];
        result.castsShadow=(p.words[39]&2u)!=0?1u:0u;
        target=asfloat(uint3(p.words[0],p.words[1],p.words[2]));
        color=asfloat(uint3(p.words[4],p.words[5],p.words[6]));
        float range=asfloat(p.words[7]),radius=asfloat(p.words[36]);
        float3 delta=target-positionView;float d2=dot(delta,delta),distance=sqrt(d2);
        float cutoff=asfloat(p.words[9]);
        if(!all(isfinite(target))||!isfinite(range)||range<=0||!isfinite(radius)||radius<0||!isfinite(cutoff)||!isfinite(d2))result.valid=0;
        if(result.valid==0||d2<=1e-12)return;
        result.directionView=delta/distance;result.radius=radius;result.distance=distance;
        attenuation=PTPrimaryRangeFade(range,distance)*max(1/(3.14159265359*max(radius*radius+d2,0.0001))-cutoff,0);
        if((p.words[39]&4u)!=0){
            float3 direction=asfloat(uint3(p.words[12],p.words[13],p.words[14]));
            float scale=asfloat(p.words[10]),offset=asfloat(p.words[11]);
            if(!all(isfinite(direction))||!isfinite(scale)||!isfinite(offset)){result.valid=0;return;}
            float cone=saturate(dot(-result.directionView,direction)*scale+offset);attenuation*=cone*cone;
        }
    }else if(selected<counts.x+counts.y){
        PTPrimarySpot p=PTPrimarySpots[selected-counts.x];
        uint shadowMode=p.words[76]&0x300u;
        result.castsShadow=((p.words[76]&2u)!=0||shadowMode==0x100u||shadowMode==0x200u)?1u:0u;
        target=asfloat(uint3(p.words[0],p.words[1],p.words[2]));
        color=asfloat(uint3(p.words[12],p.words[13],p.words[14]));
        float4 clip=positionView.x*asfloat(uint4(p.words[20],p.words[21],p.words[22],p.words[23]))+
                    positionView.y*asfloat(uint4(p.words[24],p.words[25],p.words[26],p.words[27]))+
                    positionView.z*asfloat(uint4(p.words[28],p.words[29],p.words[30],p.words[31]))+
                                   asfloat(uint4(p.words[32],p.words[33],p.words[34],p.words[35]));
        if(!all(isfinite(clip))||!all(isfinite(target))){result.valid=0;return;}
        if(clip.w<=0)return;
        float3 projected=clip.xyz/clip.w;
        if(any(abs(projected.xy)>1)||projected.z<0||projected.z>1)return;
        float3 wedge=asfloat(uint3(p.words[4],p.words[5],p.words[6]))-positionView;
        float range=asfloat(p.words[15]),cutoff=asfloat(p.words[17]),radius=asfloat(p.words[18]),sinSquared=asfloat(p.words[19]);
        float3 delta=target-positionView;float d2=dot(delta,delta);
        if(!all(isfinite(wedge))||!isfinite(range)||range<=0||!isfinite(cutoff)||!isfinite(radius)||radius<0||
           !isfinite(sinSquared)||sinSquared<0||!isfinite(d2)){result.valid=0;return;}
        if(d2<=1e-12)return;
        result.directionView=delta*rsqrt(d2);result.radius=radius;result.distance=sqrt(d2);
        attenuation=PTPrimaryRangeFade(range,clip.z)*max(1/(3.14159265359*max(sinSquared*dot(wedge,wedge)+radius*radius,0.25))-cutoff,0);
        if((p.words[76]&1u)!=0){
            float2 uv=projected.xy;
            if((p.words[76]&1024u)!=0)uv=2*uv/(dot(uv,uv)+1);
            uv=uv*float2(.5,-.5)+.5;
            float4 atlas=asfloat(uint4(p.words[64],p.words[65],p.words[66],p.words[67]));
            if(!all(isfinite(atlas))){result.valid=0;return;}
            color*=PTPrimarySpotProjection.SampleLevel(PTPrimaryProjectionClamp,uv*atlas.zw+atlas.xy,0).rgb;
        }
    }else{
        PTPrimarySun p=PTPrimarySuns[0];
        result.directionView=asfloat(uint3(p.words[0],p.words[1],p.words[2]));
        float lengthSquared=dot(result.directionView,result.directionView);
        if(!isfinite(lengthSquared)||abs(lengthSquared-1)>0.001){result.valid=0;return;}
        result.directionView*=rsqrt(lengthSquared);result.directional=1;
        color=asfloat(uint3(p.words[4],p.words[5],p.words[6]));attenuation=1;
        if((p.words[26]&1u)!=0){
            float2 uv=positionView.x*asfloat(uint2(p.words[8],p.words[9]))+
                      positionView.y*asfloat(uint2(p.words[12],p.words[13]))+
                      positionView.z*asfloat(uint2(p.words[16],p.words[17]))+asfloat(uint2(p.words[20],p.words[21]));
            float transparency=asfloat(p.words[24]);
            if(!all(isfinite(uv))||!isfinite(transparency)){result.valid=0;return;}
            color*=1+transparency*(PTPrimarySunProjection.SampleLevel(PTPrimaryProjectionWrap,uv,0).x-1);
        }
    }
    result.targetView=target;result.radiance=color*attenuation;
    if(!all(isfinite(result.radiance))||any(result.radiance<0)||!all(isfinite(result.directionView)))result.valid=0;
    result.active=result.valid!=0&&any(result.radiance>0)?1u:0u;return;
}
#endif
