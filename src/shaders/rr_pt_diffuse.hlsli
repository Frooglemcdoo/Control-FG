// Raw PT is 64 bytes/pixel, validated surface output is 32 bytes/pixel.
// Never use radiance as albedo; black diffuse reflectance is a valid material.
ByteAddressBuffer PTGuideRaw : register(t4);
ByteAddressBuffer PTGuideSurface : register(t5);
ByteAddressBuffer PTGuideAdmission : register(t6);
float3 PTDiffuseGuide(uint index,uint material,float3 nativeGuide) {
    float3 value=nativeGuide;
    [branch] if(PTGuideAdmission.Load(0)==1){
        float4 covered=asfloat(PTGuideSurface.Load4(index*32));
        uint surfaceMaterial=PTGuideSurface.Load(index*32+20);
        float4 raw=asfloat(PTGuideRaw.Load4(index*64));
        float3 kd=asfloat(PTGuideRaw.Load3(index*64+32));
        if(covered.w==1&&raw.w==1&&surfaceMaterial==material&&
           all(isfinite(kd))&&all(kd>=0)&&all(kd<=1))value=kd;
    }
    return value;
}
