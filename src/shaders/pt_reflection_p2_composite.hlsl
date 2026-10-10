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
Texture2D<float4> g_sMaterialTextureArray[] : register(t0, space1);
Texture2DArray<uint> g_tMaterialId : register(t26);
Texture2DArray<float4> g_tNormal_TexcoordX : register(t27);
Texture2DArray<float4> g_tPosition_TexcoordY : register(t28);
SamplerState g_sLinearWrap : register(s5, space1);
RWTexture2D<float4> g_rwtReflectionTarget : register(u0);

// Runtime execution is proven. P2 now uses normal energy and mixed
// specular/diffuse secondary shading.
#define PT_REFLECTION_P2_VISUAL_PROOF 0
#define PT_REFLECTION_P2_VALIDATION_GAIN 1.0f
// Separate cs_5_1 variant with the same native root/descriptor ABI.
// This is a deliberately destructive diagnostic shader, never production.
#ifndef PT_REFLECTION_VERIFY_VARIANT
#define PT_REFLECTION_VERIFY_VARIANT 0
#endif

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


void PTBasis(float3 n,out float3 t,out float3 b)
{
    float signZ=n.z>=0.0f?1.0f:-1.0f;
    float a=-1.0f/(signZ+n.z);
    float c=n.x*n.y*a;
    t=normalize(float3(1.0f+signZ*n.x*n.x*a,signZ*c,-signZ*n.x));
    b=normalize(float3(c,signZ+n.y*n.y*a,-n.y));
}

float2 PTUnpackUV(float packed)
{
    uint value=asuint(packed);
    return float2(value&0xffffu,value>>16u)*(1.0f/65535.0f);
}

float3 PTMaterialBaseColor(uint materialId,float2 uv)
{
    PTMaterialDataPart3 m=g_sbMaterialDataPart3[materialId];
    uint textureIndex=NonUniformResourceIndex(materialId*35u);
    float3 tex=max(g_sMaterialTextureArray[textureIndex].SampleLevel(g_sLinearWrap,uv,0.0f).rgb,0.0f);
    return max(tex*max(m.vColorMultiplier.rgb,0.0f),0.0f);
}

float3 PTDiffuseEnvironment(float3 n)
{
    float3 t,b;
    PTBasis(n,t,b);
    float mip=max(g_fEnvReflectionMipCount-1.0f,0.0f);

    float3 sum=max(g_sEnvironmentCube.SampleLevel(g_sLinearWrap,n,mip).rgb,0.0f)*0.40f;
    sum+=max(g_sEnvironmentCube.SampleLevel(g_sLinearWrap,normalize(n+t*0.75f),mip).rgb,0.0f)*0.15f;
    sum+=max(g_sEnvironmentCube.SampleLevel(g_sLinearWrap,normalize(n-t*0.75f),mip).rgb,0.0f)*0.15f;
    sum+=max(g_sEnvironmentCube.SampleLevel(g_sLinearWrap,normalize(n+b*0.75f),mip).rgb,0.0f)*0.15f;
    sum+=max(g_sEnvironmentCube.SampleLevel(g_sLinearWrap,normalize(n-b*0.75f),mip).rgb,0.0f)*0.15f;
    return sum;
}

float3 PTSecondaryRadiance(uint materialId,float2 uv,float3 hitNormalWorld,float3 incomingWorld)
{
    PTMaterialDataPart3 m=g_sbMaterialDataPart3[materialId];
    float3 n=normalize(hitNormalWorld);
    if(dot(n,incomingWorld)>0.0f)n=-n;

    float smoothness=PTSmoothness(materialId);
    float roughness=1.0f-smoothness;

    // Glossy/specular lobe.
    float3 reflected=reflect(incomingWorld,n);
    float mip=roughness*max(g_fEnvReflectionMipCount-1.0f,0.0f);
    float3 envSpec=max(g_sEnvironmentCube.SampleLevel(g_sLinearWrap,reflected,mip).rgb,0.0f);
    float specWeight=lerp(0.04f,1.0f,smoothness*smoothness);
    float3 specular=envSpec*specWeight;

    // Rough surfaces such as concrete/plaster contribute diffuse indirect
    // radiance instead of disappearing from the path. This is intentionally
    // broad/low-frequency so they do not become mirror-like.
    float3 baseColor=PTMaterialBaseColor(materialId,uv);
    float3 diffuseEnv=PTDiffuseEnvironment(n);
    float diffuseWeight=roughness*roughness*0.70f;
    float3 diffuse=diffuseEnv*baseColor*diffuseWeight;

    float3 emission=max(m.vEmissionMultiplier,0.0f)*max(m.fEmissionIntensity,0.0f);
    return specular+diffuse+emission;
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
        float firstRoughness=1.0f-firstSmoothness;
        // Preserve useful throughput through both polished and rough materials.
        // Rough diffuse paths should not collapse to the old 8% specular floor.
        float firstThroughput=0.08f
            +0.34f*firstSmoothness*firstSmoothness
            +0.38f*firstRoughness*firstRoughness;
        float distanceAttenuation=rsqrt(1.0f+0.0025f*segmentLength2);
        float weight=firstThroughput*distanceAttenuation;
        float2 secondUv=PTUnpackUV(secondPosPacked.w);

        bounce2+=PTSecondaryRadiance(secondMaterial,secondUv,secondNormalPacked.xyz,incomingWorld)*weight;
        totalWeight+=weight;
    }

#if PT_REFLECTION_VERIFY_VARIANT == 1
    // Four-screen-quadrant output; *actual* render target contents are changed,
    // so any absence in the final image is a downstream consumption issue.
    // TL: native Control reflection (no P2 write).
    // TR: green=valid secondary hit, magenta=missing secondary hit.
    // BL: heatmap of computed bounce-2 radiance, no arbitrary fake light.
    // BR: native reflection plus deliberately boosted P2 radiance.
    const uint halfW=((uint)g_vScreenRes.x)/2u;
    const uint halfH=((uint)g_vScreenRes.y)/2u;
    const bool right=pixel.x>=halfW;
    const bool bottom=pixel.y>=halfH;
    if(!right&&!bottom)return;
    if(right&&!bottom){
        g_rwtReflectionTarget[pixel]=
            totalWeight>0.0f?float4(0.02f,6.0f,0.02f,1.0f):
                             float4(6.0f,0.02f,5.0f,1.0f);
        return;
    }
    const float3 unboundedRadiance=bounce2/2.0f;
    if(!right&&bottom){
        const float lum=max(dot(max(unboundedRadiance,0.0f),
                        float3(0.2126f,0.7152f,0.0722f)),0.0f);
        const float mapped=saturate(lum/(lum+0.02f));
        // blue=weak, yellow/red=bright; invalid paths are black.
        float3 heat=(totalWeight>0.0f)?
            float3(4.0f*mapped*mapped,4.0f*mapped*(1.0f-mapped),
                   2.0f*(1.0f-mapped)):float3(0.0f,0.0f,0.0f);
        g_rwtReflectionTarget[pixel]=float4(heat,1.0f);
        return;
    }
    if(totalWeight<=0.0f)return;
    float4 amplified=g_rwtReflectionTarget[pixel];
    amplified.rgb+=max(unboundedRadiance,0.0f)*16.0f;
    g_rwtReflectionTarget[pixel]=amplified;
    return;
#endif
    if(totalWeight<=0.0f)return;

    float3 secondary=(bounce2/2.0f)*PT_REFLECTION_P2_VALIDATION_GAIN;
    float luminance=max(dot(secondary,float3(0.2126f,0.7152f,0.0722f)),1.0e-5f);
    float clampValue=max(g_fClampReflectionIntensity,0.0f);
    if(clampValue>0.0f&&luminance>clampValue)
        secondary*=clampValue/luminance;

    float4 base=g_rwtReflectionTarget[pixel];
    base.rgb=max(base.rgb+secondary,0.0f);
    g_rwtReflectionTarget[pixel]=base;
}
