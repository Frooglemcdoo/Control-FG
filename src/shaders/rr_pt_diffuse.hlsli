#include "pt_guide_normal.hlsli"
// Raw PT is 64 bytes/pixel, validated surface output is 64 bytes/pixel.
// Never use radiance as albedo; black diffuse reflectance is a valid material.
ByteAddressBuffer PTGuideRaw : register(t4);
ByteAddressBuffer PTGuideSurface : register(t5);
ByteAddressBuffer PTGuideAdmission : register(t6);
float4 PTMaterialGuide(uint index,uint material,float3 nativeGuide,float nativeViewCosine,inout float3 normalView,out bool ptOwned) {
    ptOwned=false;
    float4 value=float4(nativeGuide,nativeViewCosine);
    [branch] if(PTGuideAdmission.Load(0)==1){
        float4 covered=asfloat(PTGuideSurface.Load4(index*64));
        uint surfaceMaterial=PTGuideSurface.Load(index*64+20);
        float4 raw=asfloat(PTGuideRaw.Load4(index*64));
        float3 kd=asfloat(PTGuideRaw.Load3(index*64+32));
        float viewCosine=asfloat(PTGuideRaw.Load(index*64+60));
        if(covered.w==1&&raw.w==1&&surfaceMaterial==material&&
           all(isfinite(kd))&&all(kd>=0)&&all(kd<=1)&&
           isfinite(viewCosine)&&viewCosine>0&&viewCosine<=1){
            value=float4(kd,viewCosine);ptOwned=true;
            uint packedNormal=PTGuideRaw.Load(index*64+44);
            if((packedNormal&0x80000000u)!=0)normalView=PTUnpackGuideNormal(packedNormal);
        }
    }
    return value;
}
