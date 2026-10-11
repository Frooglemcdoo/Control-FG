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
// Native Control rt_deferredReflection RDEF:
// t10 has exactly 160-byte structured point-light elements,
// t5 is a UINT4 tile/cell index texture, and b1 is 416 bytes.
// These are bound by Control's original composite root signature.
// Diagnostic only until shadow visibility and cell-index packing are proven.
#if PT_REFLECTION_VERIFY_VARIANT == 2
// Small native-GPU-readable hexadecimal font (3x5 pixels per digit).
// Rows are packed low to high, with bit (row*3+column). Displays the
// *unmodified raw* 32-bit native constants and SRV dimensions on-screen.
uint PTLightProbeHexFont(uint nibble)
{
    static const uint glyph[16] = {
        0x7b6fu,0x749au,0x73e7u,0x79e7u,
        0x49edu,0x79cfu,0x7bcfu,0x4927u,
        0x7befu,0x79efu,0x5befu,0x3aebu,
        0x724fu,0x3b6bu,0x73cfu,0x13cfu
    };
    return glyph[nibble & 15u];
}
struct PTNativePointLight {
    float3 positionView;
    float shadowMapShrink;
    float3 color;
    float clipRange;
    float4 falloff;
    float3 directionView;
    float invShadowMapRange;
    column_major float4x4 viewToShadowClip;
    float4 shadowAtlasOffsetScale;
    float radius;
    float boundsRadius;
    float scatterIntensity;
    uint techniqueProperties;
};
StructuredBuffer<PTNativePointLight> ptNativePointLights : register(t10);
Texture2D<uint4> ptNativePointCellIds : register(t5);
// Native spotlight records are 320 bytes, with fixed RDEF field offsets.
// float4[20] avoids depending on HLSL matrix array packing conventions.
struct PTNativeSpotLightRaw { float4 words[20]; };
StructuredBuffer<PTNativeSpotLightRaw> ptNativeSpotLights : register(t14);
Texture2D<uint4> ptNativeSpotCellIds : register(t6);
cbuffer deferredlight_constants : register(b1)
{
    float4 ptNativeDeferredPrefix[16]; // byte 0-255; native pre-count area
    uint ptNativePointLightTotalCount; // byte 256; native UINT, not float bits
    uint ptNativeSpotLightTotalCount;  // byte 260
    uint ptNativeSunLightTotalCount;   // byte 264
    uint ptNativeAmbientLightEnabled; // byte 268
    float4 ptNativeDeferredTail[9];   // byte 272-415; total b1 = 416
}
uint PTNativePointCount()
{
    return ptNativePointLightTotalCount;
}
uint PTNativeSpotCount()
{
    return ptNativeSpotLightTotalCount;
}
float3 PTWorldNormalToView(float3 n)
{
    return normalize(mul((float3x3)g_mWorldToView,n));
}
float3 PTLocalUnshadowedPotential(float3 hitPosView,float3 hitNormalView,
                                  uint numLights)
{
    float3 total=0.0f;
    // Only used in the debug quadrant. This is NOT final PT lighting:
    // shadow visibility and tiled-cell decoding are not yet validated.
    [loop]
    for(uint i=0u;i<numLights;++i){
        PTNativePointLight l=ptNativePointLights[i];
        if(!all(isfinite(l.positionView)) || !all(isfinite(l.color)) ||
            !isfinite(l.boundsRadius) || l.boundsRadius<=0.01f ||
            l.boundsRadius>5000.0f ||
            any(l.color<0.0f) || any(l.color>10000.0f))continue;
        float3 toLight=l.positionView-hitPosView;
        float d2=dot(toLight,toLight);
        if(d2<1.0e-3f)continue;
        float distance=sqrt(d2);
        float ndotl=saturate(dot(hitNormalView,toLight/distance));
        // Range and distance weighting; source scale remains intentionally
        // uncalibrated. Nonzero diagnostic proves the *actual bound light
        // records* can illuminate a secondary hit, not shadow correctness.
        float radius=max(l.boundsRadius,0.01f);
        float range=saturate((radius-distance)/max(radius,0.01f));
        float attenuation=(range*range)/(1.0f+0.0025f*d2);
        total+=max(l.color,0.0f)*ndotl*attenuation;
    }
    return total;
}


// Progressive local-light stage audit, diagnostic ONLY:
//   x: raw native light color luminance (no geometry gating)
//   y: light within distance/range, normal/cone ignored
//   z: range + second-hit NdotL
//   w: range + NdotL + spotlight cone (points retain z)
// These values never enter the normal P2 shader variant.
float4 PTPointGateAudit(float3 hitPosView,float3 normalView,uint count)
{
    float4 stages=0.0f;
    [loop]
    for(uint i=0u;i<count;++i){
        PTNativePointLight l=ptNativePointLights[i];
        float3 c=l.color;
        if(!all(isfinite(c))||any(c<0.0f)||any(c>10000.0f)||
           !all(isfinite(l.positionView))||
           !isfinite(l.boundsRadius)||l.boundsRadius<=0.01f||
           l.boundsRadius>5000.0f)continue;
        const float power=max(dot(c,float3(0.2126f,0.7152f,0.0722f)),0.0f);
        stages.x+=power;
        const float3 toLight=l.positionView-hitPosView;
        const float d2=dot(toLight,toLight);
        if(!isfinite(d2)||d2<1.0e-6f)continue;
        const float dist=sqrt(d2);
        const float w=saturate((l.boundsRadius-dist)/max(l.boundsRadius,0.01f));
        const float atten=w*w/(1.0f+0.0025f*d2);
        const float atRange=power*atten;
        stages.y+=atRange;
        const float facing=saturate(dot(normalView,toLight/dist));
        stages.z+=atRange*facing;
        stages.w+=atRange*facing;
    }
    return stages;
}

float4 PTSpotGateAudit(float3 hitPosView,float3 normalView,uint count)
{
    float4 stages=0.0f;
    [loop]
    for(uint i=0u;i<count;++i){
        PTNativeSpotLightRaw l=ptNativeSpotLights[i];
        float3 color=l.words[3].xyz;
        float3 lightPos=l.words[0].xyz;
        float3 direction=l.words[2].xyz;
        float farDist=l.words[1].w;
        if(!all(isfinite(color))||any(color<0.0f)||any(color>10000.0f)||
           !all(isfinite(lightPos))||!all(isfinite(direction))||
           !isfinite(farDist)||farDist<=0.01f||farDist>5000.0f||
           dot(direction,direction)<1.0e-8f)continue;
        const float power=max(dot(color,float3(0.2126f,0.7152f,0.0722f)),0.0f);
        stages.x+=power;
        const float3 toLight=lightPos-hitPosView;
        const float d2=dot(toLight,toLight);
        if(!isfinite(d2)||d2<1.0e-6f)continue;
        const float dist=sqrt(d2);
        const float range=saturate((farDist-dist)/max(farDist,0.01f));
        const float atten=range*range/(1.0f+0.0025f*d2);
        const float atRange=power*atten;
        stages.y+=atRange;
        const float facing=saturate(dot(normalView,toLight/dist));
        const float atFace=atRange*facing;
        stages.z+=atFace;
        const float cosine=dot(normalize(direction),-toLight/dist);
        const float sinHalf2=l.words[4].w;
        const float cosHalf=sqrt(saturate(1.0f-saturate(sinHalf2)));
        const float cone=saturate((cosine-cosHalf)/max(1.0f-cosHalf,0.001f));
        stages.w+=atFace*cone;
    }
    return stages;
}

float3 PTLocalUnshadowedSpotPotential(float3 hitPosView,float3 hitNormalView,
                                     uint numLights)
{
    float3 total=0.0f;
    [loop]
    for(uint i=0u;i<numLights;++i){
        PTNativeSpotLightRaw l=ptNativeSpotLights[i];
        float3 lightPosView=l.words[0].xyz;   // RDEF offset +0
        float farDistance=l.words[1].w;       // +28
        float3 directionView=l.words[2].xyz; // +32
        float3 color=l.words[3].xyz;         // +48
        float sinConeHalfSq=l.words[4].w;    // +76
        if(!all(isfinite(lightPosView))||!all(isfinite(directionView))||
           !all(isfinite(color))||!isfinite(farDistance)||
           farDistance<=0.01f||farDistance>5000.0f||
           dot(directionView,directionView)<1.0e-8f||
           any(color<0.0f)||any(color>10000.0f))continue;
        float3 toLight=lightPosView-hitPosView;
        float d2=dot(toLight,toLight);
        if(d2<1.0e-3f||farDistance<=0.01f)continue;
        float dist=sqrt(d2);
        float3 fromLight=-toLight/dist;
        float cosine=dot(normalize(directionView),fromLight);
        float cosHalf=sqrt(saturate(1.0f-saturate(sinConeHalfSq)));
        float cone=saturate((cosine-cosHalf)/max(1.0f-cosHalf,0.001f));
        float ndotl=saturate(dot(hitNormalView,toLight/dist));
        float distanceWeight=saturate((farDistance-dist)/max(farDistance,0.01f));
        float attenuation=(distanceWeight*distanceWeight)/(1.0f+0.0025f*d2);
        total+=max(color,0.0f)*cone*ndotl*attenuation;
    }
    return total;
}

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
#if PT_REFLECTION_VERIFY_VARIANT == 2
    // Four quadrants for native local-light data validation:
    // TL = native reflection (unchanged);
    // TR = point-light count / tile-cell occupancy (not decoded);
    // BL = actual point-light *potential* at secondary hit (unshadowed);
    // BR = candidate potential x8 over native, diagnostic only.
    // Normal mode 0 and P2 radiance mode 1 are unaffected.
    const uint halfW=((uint)g_vScreenRes.x)/2u;
    const uint halfH=((uint)g_vScreenRes.y)/2u;
    const bool right=pixel.x>=halfW;
    const bool bottom=pixel.y>=halfH;
    if(!right&&!bottom)return;
    uint pointElements=0u,pointStride=0u;
    uint spotElements=0u,spotStride=0u;
    ptNativePointLights.GetDimensions(pointElements,pointStride);
    ptNativeSpotLights.GetDimensions(spotElements,spotStride);
    const uint rawCount=PTNativePointCount();
    const uint rawSpots=PTNativeSpotCount();
    // Independently validate both native light views: one bad light
    // category must not suppress the other. Do not assume a hardcoded
    // 1024-light count before checking actual per-scene buffer capacity.
    const bool pointStrideValid=pointStride==160u;
    const bool spotStrideValid=spotStride==320u;
    const bool pointCountValid=pointStrideValid&&pointElements<1048576u&&
        rawCount<=pointElements&&rawCount<=8192u;
    const bool spotCountValid=spotStrideValid&&spotElements<1048576u&&
        rawSpots<=spotElements&&rawSpots<=8192u;
    // Keep the questionable CBV counts for the HEX display, but do not
    // let them suppress independent SRV diagnostics. Sample only the
    // first 12 buffer records, guarded by the *actual SRV dimensions*.
    // This is NOT scene light-list decoding and never runs in normal P2.
    const bool pointViewSafe=pointStrideValid&&pointElements>0u&&
        pointElements<=65536u;
    const bool spotViewSafe=spotStrideValid&&spotElements>0u&&
        spotElements<=65536u;
    // V4 record-health masks still scan the first 12, while stage audit
    // checks up to 32 native records from each resource. The light counts
    // stored in b1 remain untrusted and are never used as loop bounds.
    const uint pointSampleCount=pointViewSafe?min(pointElements,32u):0u;
    const uint spotSampleCount=spotViewSafe?min(spotElements,32u):0u;
    if(right&&!bottom){
        uint tileW=0u,tileH=0u,spotW=0u,spotH=0u;
        ptNativePointCellIds.GetDimensions(tileW,tileH);
        ptNativeSpotCellIds.GetDimensions(spotW,spotH);

        // Inspect the actual first 12 SRV records independently of b1.
        // Upper 12 bits (shifted 16) = records with finite, nonzero
        // diffuse light color; lower 12 bits = color AND plausible range
        // and position. This is *resource content* auditing, not an
        // assertion about the native cell-list encoding or visibility.
        uint pointRecordHealth=0u,spotRecordHealth=0u;
        if(pointViewSafe){
            [loop]
            for(uint p=0u;p<min(pointElements,12u);++p){
                PTNativePointLight l=ptNativePointLights[p];
                const bool colorOk=all(isfinite(l.color))&&
                    all(l.color>=0.0f)&&all(l.color<10000.0f)&&
                    any(l.color>0.001f);
                const bool spatialOk=all(isfinite(l.positionView))&&
                    all(abs(l.positionView)<100000.0f)&&
                    isfinite(l.boundsRadius)&&l.boundsRadius>0.01f&&
                    l.boundsRadius<5000.0f;
                if(colorOk)pointRecordHealth|=(1u<<(16u+p));
                if(colorOk&&spatialOk)pointRecordHealth|=(1u<<p);
            }
        }
        if(spotViewSafe){
            [loop]
            for(uint p=0u;p<min(spotElements,12u);++p){
                PTNativeSpotLightRaw l=ptNativeSpotLights[p];
                const float3 pos=l.words[0].xyz;
                const float3 color=l.words[3].xyz;
                const float farDistance=l.words[1].w;
                const float3 direction=l.words[2].xyz;
                const bool colorOk=all(isfinite(color))&&
                    all(color>=0.0f)&&all(color<10000.0f)&&
                    any(color>0.001f);
                const bool spatialOk=all(isfinite(pos))&&
                    all(abs(pos)<100000.0f)&&
                    all(isfinite(direction))&&
                    dot(direction,direction)>1.0e-8f&&
                    isfinite(farDistance)&&farDistance>0.01f&&
                    farDistance<5000.0f;
                if(colorOk)spotRecordHealth|=(1u<<(16u+p));
                if(colorOk&&spatialOk)spotRecordHealth|=(1u<<p);
            }
        }

        // Eight colored HEX telemetry rows, with a left-hand green/red
        // status strip. The previous all-red screen discarded the values
        // that were needed to identify which native resource failed.
        //
        // Row 0: raw native point count at b1+256 (GREEN)
        // Row 1: raw native spot count at b1+260 (BLUE)
        // Row 2: point SRV stride (ORANGE; expect 0x000000A0)
        // Row 3: spot SRV stride (PURPLE; expect 0x00000140)
        // Row 4: point SRV element capacity (GREEN)
        // Row 5: spot SRV element capacity (BLUE)
        // Row 6: point record health (upper 12 bits color / low 12 spatial)
        // Row 7: spot record health (same encoding); 00000000 means
        //        all 12 sampled records had empty/implausible light data.
        const uint localX=pixel.x-halfW;
        const uint screenQuarterWidth=max((uint)g_vScreenRes.x-halfW,1u);
        const uint topHeight=max(halfH,1u);
        const uint band=min(7u,pixel.y*8u/topHeight);
        uint displayValue=rawCount;
        if(band==1u)displayValue=rawSpots;
        if(band==2u)displayValue=pointStride;
        if(band==3u)displayValue=spotStride;
        if(band==4u)displayValue=pointElements;
        if(band==5u)displayValue=spotElements;
        if(band==6u)displayValue=pointRecordHealth;
        if(band==7u)displayValue=spotRecordHealth;
        const uint digit=min(7u,localX*8u/screenQuarterWidth);
        const uint glyphX=((localX*8u)%screenQuarterWidth)*3u/screenQuarterWidth;
        const uint glyphY=((pixel.y*8u)%topHeight)*5u/topHeight;
        const uint glyphBits=PTLightProbeHexFont(
            (displayValue>>((7u-digit)*4u))&15u);
        const bool glyphOn=((glyphBits>>(glyphY*3u+glyphX))&1u)!=0u;
        const bool statusOkay=band==0u?pointCountValid:
            band==1u?spotCountValid:
            band==2u?pointStrideValid:
            band==3u?spotStrideValid:
            band==4u?(pointElements>0u&&pointElements<1048576u):
            band==5u?(spotElements>0u&&spotElements<1048576u):
            band==6u?(tileW>0u&&tileH>0u&&
                        (pointRecordHealth&0xFFFu)!=0u):
                       (spotW>0u&&spotH>0u&&
                        (spotRecordHealth&0xFFFu)!=0u);
        if(localX<12u){
            g_rwtReflectionTarget[pixel]=float4(
                statusOkay?float3(0.0f,6.0f,0.0f):float3(6.0f,0.0f,0.0f),1.0f);
            return;
        }
        float3 glyphColor=band==0u||band==4u?float3(0.0f,5.0f,0.3f):
            band==1u||band==5u?float3(0.3f,2.0f,5.0f):
            band==2u?float3(6.0f,2.0f,0.0f):
            band==3u?float3(4.0f,1.0f,5.0f):
            band==6u?float3(4.0f,4.0f,0.0f):
                      float3(3.0f,0.5f,4.0f);
        g_rwtReflectionTarget[pixel]=
            float4(glyphOn?glyphColor:float3(0.02f,0.025f,0.03f),1.0f);
        return;
    }

    // V6: decouple source radiance from *all* ray-hit validity checks.
    // V5 reported nearly zero even in its "raw" band, but that band was
    // actually gated by finite hit position, world normal and world->view
    // transformed normal before the helper ever ran. Thus V5 could not
    // distinguish missing source radiance from invalid hit attributes.
    float rawPointPower=0.0f;
    float rawSpotPower=0.0f;
    [loop]
    for(uint i=0u;i<min(pointSampleCount,12u);++i){
        const float3 c=ptNativePointLights[i].color;
        if(all(isfinite(c))&&all(c>=0.0f)&&
           all(c<10000.0f))
            rawPointPower+=max(dot(c,float3(0.2126f,0.7152f,0.0722f)),0.0f);
    }
    [loop]
    for(uint i=0u;i<min(spotSampleCount,12u);++i){
        const float3 c=ptNativeSpotLights[i].words[3].xyz;
        if(all(isfinite(c))&&all(c>=0.0f)&&
           all(c<10000.0f))
            rawSpotPower+=max(dot(c,float3(0.2126f,0.7152f,0.0722f)),0.0f);
    }

    float4 pointGate=0.0f;
    float4 spotGate=0.0f;
    bool firstHitValid=false,secondHitValid=false;
    bool positionFinite=false,normalRawFinite=false;
    bool normalViewFinite=false,fullyEligible=false;
    [unroll]
    for(uint probePath=0u;probePath<2u;++probePath){
        const uint firstMaterial=g_tMaterialId.Load(int4(pixel,probePath,0));
        if(firstMaterial>=65534u||firstMaterial>=g_uMaterialIDCount)continue;
        firstHitValid=true;

        const float4 n=g_tNormal_TexcoordX.Load(
            int4(pixel,probePath+2u,0));
        const uint secondMat=asuint(n.w);
        if(secondMat>=65534u||secondMat>=g_uMaterialIDCount)continue;
        secondHitValid=true;

        const float3 hitPosView=g_tPosition_TexcoordY.Load(
            int4(pixel,probePath+2u,0)).xyz;
        if(!all(isfinite(hitPosView)))continue;
        positionFinite=true;

        const float3 normalWorld=n.xyz;
        const float normalLength2=dot(normalWorld,normalWorld);
        if(!all(isfinite(normalWorld))||
           !isfinite(normalLength2)||normalLength2<1.0e-6f)continue;
        normalRawFinite=true;

        const float3 normalView=PTWorldNormalToView(
            normalWorld*rsqrt(normalLength2));
        if(!all(isfinite(normalView)))continue;
        normalViewFinite=true;
        fullyEligible=true;

        pointGate+=PTPointGateAudit(
            hitPosView,normalView,pointSampleCount)*0.5f;
        spotGate+=PTSpotGateAudit(
            hitPosView,normalView,spotSampleCount)*0.5f;
    }

    if(!right&&bottom){
        if(pixel.y<halfH+24u){
            // Resource safety + traced second-hit indicator, as in V5.
            const uint dx=(pixel.x*3u)/max(halfW,1u);
            const bool barOn=dx==0u?pointViewSafe:
                dx==1u?spotViewSafe:secondHitValid;
            const float3 channel=dx==0u?float3(4.0f,0.0f,0.0f):
                dx==1u?float3(0.0f,4.0f,0.0f):
                        float3(0.0f,0.0f,4.0f);
            g_rwtReflectionTarget[pixel]=float4(
                barOn?channel:float3(0.0f,0.0f,0.0f),1.0f);
            return;
        }

        const uint bottomHeight=max((uint)g_vScreenRes.y-halfH-24u,1u);
        const uint dy=pixel.y-halfH-24u;
        const uint stage=min(3u,dy*4u/bottomHeight);
        if(stage==0u){
            // RED: real point-buffer source light energy;
            // GREEN: real spot-buffer source energy.
            // This is fully INDEPENDENT of secondary hit geometry.
            const float pr=saturate(rawPointPower/(rawPointPower+0.002f));
            const float sg=saturate(rawSpotPower/(rawSpotPower+0.002f));
            g_rwtReflectionTarget[pixel]=float4(
                4.0f*pr,4.0f*sg,0.02f,1.0f);
            return;
        }
        if(stage==1u){
            // RED: native first-hit material valid;
            // GREEN: secondary-hit material valid;
            // BLUE: secondary hit view-space position is finite.
            g_rwtReflectionTarget[pixel]=float4(
                firstHitValid?4.0f:0.0f,
                secondHitValid?4.0f:0.0f,
                positionFinite?4.0f:0.0f,1.0f);
            return;
        }
        if(stage==2u){
            // RED: finite nonzero raw world normal;
            // GREEN: finite transformed view-space normal;
            // BLUE: every gate to the light-evaluation helper passed.
            g_rwtReflectionTarget[pixel]=float4(
                normalRawFinite?4.0f:0.0f,
                normalViewFinite?4.0f:0.0f,
                fullyEligible?4.0f:0.0f,1.0f);
            return;
        }
        // Bottom stage: actually evaluated unshadowed local radiance.
        // Red=point, green=spot, dark blue=valid helper call.
        const float pointHeat=saturate(pointGate.w/(pointGate.w+0.004f));
        const float spotHeat=saturate(spotGate.w/(spotGate.w+0.004f));
        g_rwtReflectionTarget[pixel]=float4(
            4.0f*pointHeat,4.0f*spotHeat,
            fullyEligible?0.12f:0.0f,1.0f);
        return;
    }

    // BR upper half: independent native raw-light proof (magenta),
    // without second-hit validity. BR lower half: fully gated candidate
    // (yellow). Normal P2 mode 0 remains untouched.
    float4 candidateBase=g_rwtReflectionTarget[pixel];
    if(pixel.y<halfH+((uint)g_vScreenRes.y-halfH)/2u){
        const float totalSource=rawPointPower+rawSpotPower;
        const float heat=saturate(totalSource/(totalSource+0.002f));
        candidateBase.rgb+=float3(3.0f*heat,0.3f*heat,3.0f*heat);
    }else{
        const float fullyGated=pointGate.w+spotGate.w;
        const float heat=saturate(fullyGated/(fullyGated+0.004f));
        candidateBase.rgb+=float3(3.0f*heat,3.0f*heat,0.0f);
    }
    g_rwtReflectionTarget[pixel]=candidateBase;
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
