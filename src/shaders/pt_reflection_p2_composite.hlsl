// Experimental P2 secondary-bounce add pass for Control.
// Runs immediately after Control's native rt_deferredReflection composite.
// Native first-bounce shading remains untouched; this adds paired bounce-2 radiance.

struct PTMaterialDataPart3 {
    float fClothDiffScatterAmount;
    float fClothFuzzWrap;
    float3 vClothScatterColor;
    float fHairSmoothness;
    float3 vSpecularColor2;
    float fHairSpecularShift;
    float fHairDiffuseWrap;
    float fPAD1;
    float fEmissionIntensity;
    float4 vColorMultiplier;
    float2 vSmoothnessRange;
    float2 vShadowSoftnessRange;
    float2 vScatterWidthRange;
    float fHairDepthOffsetAmount;
    float fHairNormalAmount;
    float3 vEmissionMultiplier;
    float4 vColorMultiplier2;
    float2 vBaseDetailUVScale;
    float2 vSmoothnessRange2;
    float2 vBlendDetailUVScale;
    float fBaseDetailAmount;
    float fBlendUVMultiplier;
    float fBlendDetailAmount;
    float fFuzzUVMultiplier;
    float fPupilDilation;
    float fPupilOuterRadius;
    float4 vIntensity;
    float4 vWrinkleWeights0;
    float4 vWrinkleWeights1;
    float4 vWrinkleWeights2;
    float4 vWrinkleWeights3;
    float4 vWrinkleWeights4;
    float4 vWrinkleWeights5;
    uint uDisableBackfaceFlip;
    float fEdgeSharpness;
    float fTrunkBendFactor;
    float fTrunkPivotOffset;
    float3 vColorRgbMultiplier;
    float fWindFactor;
    float fDecalMaterialBlendFactor;
    float fDecalAlbedoBlendFactor;
    uint uStableHash;
    uint pad;
};

cbuffer sys_constants : register(b0)
{
    float2 g_vScreenRes;
    float2 g_vInvScreenRes;
    float2 g_vOutputRes;
    float2 g_vInvOutputRes;

    column_major float4x4 g_mWorldToView;
    column_major float4x4 g_mViewToWorld;
    column_major float4x4 g_mViewToClip;
    column_major float4x4 g_mClipToView;
};

cbuffer env_general : register(b2)
{
    float g_fEnvReflectionEdgeLength;
    float g_fEnvReflectionMipCount;
    float2 g_envPad;
};

cbuffer rtreflection : register(b6)
{
    float g_fClampReflectionIntensity;
    float g_fRTReflectionAddRays;
    uint  g_uRTReflectionRayCount;
    uint  g_uMaterialIDCount;
    float g_fMissSamplingPoint;
    float3 g_rtreflectionPad;
};

StructuredBuffer<PTMaterialDataPart3> g_sbMaterialDataPart3 : register(t4);
TextureCube<float4> g_sEnvironmentCube : register(t8);
Texture2DArray<uint> g_tMaterialId : register(t26);
Texture2DArray<float4> g_tNormal_TexcoordX : register(t27);
Texture2DArray<float4> g_tPosition_TexcoordY : register(t28);
SamplerState g_sLinearWrap : register(s5, space1);
RWTexture2D<float4> g_rwtReflectionTarget : register(u0);

float3 PTViewToWorldPoint(float3 p)
{
    float4 w=mul(g_mViewToWorld,float4(p,1.0f));
    return w.xyz/max(abs(w.w),1.0e-6f);
}

float PTSmoothness(uint materialId)
{
    PTMaterialDataPart3 m=g_sbMaterialDataPart3[materialId];
    return saturate(0.5f*(m.vSmoothnessRange.x+m.vSmoothnessRange.y));
}

float3 PTSecondaryRadiance(uint materialId,float3 hitNormalWorld,float3 incomingWorld)
{
    PTMaterialDataPart3 m=g_sbMaterialDataPart3[materialId];
    float3 n=normalize(hitNormalWorld);
    if(dot(n,incomingWorld)>0.0f)n=-n;

    float smoothness=PTSmoothness(materialId);
    float roughness=1.0f-smoothness;
    float3 reflected=reflect(incomingWorld,n);
    float mip=roughness*max(g_fEnvReflectionMipCount-1.0f,0.0f);
    float3 env=max(g_sEnvironmentCube.SampleLevel(g_sLinearWrap,reflected,mip).rgb,0.0f);

    float3 tint=max(m.vColorMultiplier.rgb,0.0f);
    tint=lerp(1.0f.xxx,tint,saturate(dot(tint,float3(0.3333333f,0.3333333f,0.3333333f))));
    float3 emission=max(m.vEmissionMultiplier,0.0f)*max(m.fEmissionIntensity,0.0f);

    return env*tint+emission;
}

[numthreads(8,16,1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixel=dispatchThreadId.xy;
    if(pixel.x>=(uint)g_vScreenRes.x||pixel.y>=(uint)g_vScreenRes.y)return;

    float3 bounce2=0.0f;
    float totalWeight=0.0f;

    [unroll]
    for(uint path=0;path<2u;++path)
    {
        uint firstLayer=path;
        uint secondLayer=path+2u;

        uint firstMaterial=g_tMaterialId.Load(int4(pixel,firstLayer,0));
        if(firstMaterial>=65534u||firstMaterial>=g_uMaterialIDCount)continue;

        float4 firstPosPacked=g_tPosition_TexcoordY.Load(int4(pixel,firstLayer,0));
        float4 secondPosPacked=g_tPosition_TexcoordY.Load(int4(pixel,secondLayer,0));
        float4 secondNormalPacked=g_tNormal_TexcoordX.Load(int4(pixel,secondLayer,0));
        uint secondMaterial=asuint(secondNormalPacked.w);
        if(secondMaterial>=65534u||secondMaterial>=g_uMaterialIDCount)continue;

        float3 firstWorld=PTViewToWorldPoint(firstPosPacked.xyz);
        float3 secondWorld=PTViewToWorldPoint(secondPosPacked.xyz);
        float3 segment=secondWorld-firstWorld;
        float segmentLength2=dot(segment,segment);
        if(segmentLength2<1.0e-6f)continue;
        float3 incomingWorld=segment*rsqrt(segmentLength2);

        float firstSmoothness=PTSmoothness(firstMaterial);
        float firstThroughput=lerp(0.08f,0.42f,firstSmoothness*firstSmoothness);
        float distanceAttenuation=rsqrt(1.0f+0.0025f*segmentLength2);
        float weight=firstThroughput*distanceAttenuation;

        bounce2+=PTSecondaryRadiance(secondMaterial,secondNormalPacked.xyz,incomingWorld)*weight;
        totalWeight+=weight;
    }

    if(totalWeight<=0.0f)return;

    float3 secondary=bounce2/2.0f;
    float luminance=max(dot(secondary,float3(0.2126f,0.7152f,0.0722f)),1.0e-5f);
    float clampValue=max(g_fClampReflectionIntensity,0.0f);
    if(clampValue>0.0f&&luminance>clampValue)
        secondary*=clampValue/luminance;

    float4 base=g_rwtReflectionTarget[pixel];
    base.rgb=max(base.rgb+secondary,0.0f);
    g_rwtReflectionTarget[pixel]=base;
}
