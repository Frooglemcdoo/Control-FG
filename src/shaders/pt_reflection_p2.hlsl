// Experimental Control PT reflections P2 shader library.
// Compile-only until runtime admission and replacement transport are proven.
// Native register contract is recovered from rt_reflection-000.dxbc / lib_6_3.

#include "ControlPrimaryGuideDecode.hlsli"
#include "rr_hit_origin_shared.h"
#include "pt_secondary_encoding.hlsli"

static const float PT_PI = 3.14159265358979323846f;

struct HitData {
    uint value; // input: output array layer. hit: RayTCurrent*10000. miss: 0.
};

// Only fields through vColorMultiplier are consumed by alpha testing.
// Tail padding preserves Control's native 336-byte StructuredBuffer stride.
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
    float tail[65];
};

cbuffer sys_constants : register(b0)
{
    float2 g_vScreenRes;
    float2 g_vInvScreenRes;
    float2 g_vOutputRes;
    float2 g_vInvOutputRes;

    float4 g_mWorldToView0;
    float4 g_mWorldToView1;
    float4 g_mWorldToView2;
    float4 g_mWorldToView3;

    float4 g_mViewToWorld0;
    float4 g_mViewToWorld1;
    float4 g_mViewToWorld2;
    float4 g_mViewToWorld3;

    float4 g_mViewToClip0;
    float4 g_mViewToClip1;
    float4 g_mViewToClip2;
    float4 g_mViewToClip3;

    float4 g_mClipToView0;
    float4 g_mClipToView1;
    float4 g_mClipToView2;
    float4 g_mClipToView3;
};

cbuffer g_cbRaytracingHit : register(b0, space3)
{
    uint uIndexOffset;
    uint uIndexStride;
    uint uVertexStride1;
    uint uMaterialID;
    uint uTangentOffset;
    uint uNormalOffset;
    uint uTexcoordOffset;
    uint uColorOffset;
    uint uIndexBufferSize;
    uint uVertexBuffer1Size;
};

cbuffer rtreflection : register(b1)
{
    float g_fClampReflectionIntensity;
    float g_fRTReflectionAddRays;
    uint  g_uRTReflectionRayCount;
    float g_fRTZeroRayReflectance;
    float g_fRTMaxRayReflectance;
    float g_fRTMaxRayRoughness;
};

Texture2D<float4> g_tGBuffer1 : register(t0);
Texture2D<float4> g_tLinearDepth : register(t1);
Texture2D<float4> g_tClipDepth : register(t2);
StructuredBuffer<PTMaterialDataPart3> g_sbMaterialDataPart3 : register(t3);
Texture2D<float4> g_tStaticBlueNoiseRGBA_0 : register(t4);
RaytracingAccelerationStructure g_rtScene : register(t5);
Texture2D<float4> g_sMaterialTextureArray[] : register(t0, space1);
SamplerState g_sLinearWrap : register(s5, space1);
ByteAddressBuffer g_bRaytracingIndexBuffer : register(t0, space3);
ByteAddressBuffer g_bRaytracingVertexBuffer1 : register(t1, space3);

globallycoherent RWTexture2DArray<uint>   g_rwtMaterialId : register(u0);
globallycoherent RWTexture2DArray<float4> g_rwtNormal_TexcoordX : register(u1);
globallycoherent RWTexture2DArray<float4> g_rwtPosition_TexcoordY : register(u2);
RWTexture2DArray<uint>   g_rwtShadow : register(u3);

float3 PTTransformPointColumns(float3 p, float4 c0, float4 c1, float4 c2, float4 c3)
{
    return float3(
        p.x*c0.x + p.y*c1.x + p.z*c2.x + c3.x,
        p.x*c0.y + p.y*c1.y + p.z*c2.y + c3.y,
        p.x*c0.z + p.y*c1.z + p.z*c2.z + c3.z);
}

float3 PTTransformVectorColumns(float3 v, float4 c0, float4 c1, float4 c2)
{
    return normalize(float3(
        v.x*c0.x + v.y*c1.x + v.z*c2.x,
        v.x*c0.y + v.y*c1.y + v.z*c2.y,
        v.x*c0.z + v.y*c1.z + v.z*c2.z));
}

float2 PTRandom2(uint2 pixel, uint path)
{
    uint w=0,h=0;
    g_tStaticBlueNoiseRGBA_0.GetDimensions(w,h);
    uint2 p = uint2((pixel.x + path*73u) % max(w,1u),
                    (pixel.y + path*151u) % max(h,1u));
    float4 n = g_tStaticBlueNoiseRGBA_0.Load(int3(p,0));
    // Decorrelate the two paths and temporal frame without changing resources.
    float frame = frac((float)(path*17u + pixel.x*3u + pixel.y*5u) * 0.61803398875f);
    return frac(n.xy + float2(frame, frame*0.754877666f));
}

void PTBasis(float3 n, out float3 t, out float3 b)
{
    float signZ = n.z >= 0.0f ? 1.0f : -1.0f;
    float a = -1.0f / (signZ + n.z);
    float c = n.x*n.y*a;
    t = normalize(float3(1.0f + signZ*n.x*n.x*a, signZ*c, -signZ*n.x));
    b = normalize(float3(c, signZ + n.y*n.y*a, -n.y));
}

// Isotropic GGX visible-normal sampling (Heitz-style VNDF).
float3 PTSampleGGXVNDF(float3 n, float3 viewToCamera, float roughness, float2 u)
{
    float3 t,b;
    PTBasis(n,t,b);
    float3 v = float3(dot(viewToCamera,t),dot(viewToCamera,b),dot(viewToCamera,n));
    v.z = max(v.z,1.0e-4f);

    float alpha = max(roughness*roughness,0.02f);
    float3 vh = normalize(float3(alpha*v.x,alpha*v.y,v.z));
    float lensq = vh.x*vh.x + vh.y*vh.y;
    float3 t1 = lensq > 1.0e-7f ? float3(-vh.y,vh.x,0.0f)/sqrt(lensq) : float3(1,0,0);
    float3 t2 = cross(vh,t1);

    float r = sqrt(saturate(u.x));
    float phi = 2.0f*PT_PI*u.y;
    float p1 = r*cos(phi);
    float p2 = r*sin(phi);
    float s = 0.5f*(1.0f + vh.z);
    p2 = lerp(sqrt(saturate(1.0f-p1*p1)),p2,s);

    float z = sqrt(saturate(1.0f-p1*p1-p2*p2));
    float3 nh = p1*t1 + p2*t2 + z*vh;
    float3 hLocal = normalize(float3(alpha*nh.x,alpha*nh.y,max(0.0f,nh.z)));
    return normalize(hLocal.x*t + hLocal.y*b + hLocal.z*n);
}

// Control's first-reflection sampler is a truncated GGX NDF, not the full
// VNDF used for the extra PT bounce. Its native deferred shading/filtering
// expects this narrower primary lobe (native Frc64 * 0.4, roughness^4).
float3 PTNativePrimaryDirection(float3 n,float3 incoming,float roughness,float2 u)
{
    float3 t,b;
    if(n.z < -0.9999999f) {
        t=float3(0,-1,0); b=float3(-1,0,0);
    } else {
        float a=1.0f/(1.0f+n.z),xy=-n.x*n.y*a;
        t=float3(1.0f-n.x*n.x*a,xy,-n.x);
        b=float3(xy,1.0f-n.y*n.y*a,-n.y);
    }
    float alpha=roughness*roughness;
    float v=frac(u.y)*0.4f;
    float cosTheta=sqrt(saturate((1.0f-v)/(1.0f+v*(alpha*alpha-1.0f))));
    float sinTheta=sqrt(saturate(1.0f-cosTheta*cosTheta));
    float phi=2.0f*PT_PI*frac(u.x);
    float3 h=t*(cos(phi)*sinTheta)+b*(sin(phi)*sinTheta)+n*cosTheta;
    return normalize(reflect(incoming,h));
}


float3 PTSampleCosineHemisphere(float3 n, float2 u)
{
    float3 t,b;
    PTBasis(n,t,b);
    float r=sqrt(saturate(u.x));
    float phi=2.0f*PT_PI*u.y;
    float x=r*cos(phi);
    float y=r*sin(phi);
    float z=sqrt(saturate(1.0f-u.x));
    return normalize(x*t+y*b+z*n);
}

float PTMaterialSmoothness(uint materialId,float2 uv)
{
    if(materialId>=65534u)return 0.0f;
    float2 range=g_sbMaterialDataPart3[materialId].vSmoothnessRange;
    float tex=g_sMaterialTextureArray[NonUniformResourceIndex(materialId*35u+2u)].SampleLevel(g_sLinearWrap,uv,0.0f).x;
    return saturate(lerp(range.x,range.y,tex));
}

bool PTPrimary(uint2 pixel, out float3 positionView, out float3 positionWorld,
               out float3 normalWorld, out float roughness)
{
    float4 g1 = g_tGBuffer1.Load(int3(pixel,0));
    float clipDepth = g_tClipDepth.Load(int3(pixel,0)).x;

    RRHDClipToView m;
    m.column0.x=g_mClipToView0.x; m.column0.y=g_mClipToView0.y; m.column0.z=g_mClipToView0.z; m.column0.w=g_mClipToView0.w;
    m.column1.x=g_mClipToView1.x; m.column1.y=g_mClipToView1.y; m.column1.z=g_mClipToView1.z; m.column1.w=g_mClipToView1.w;
    m.column2.x=g_mClipToView2.x; m.column2.y=g_mClipToView2.y; m.column2.z=g_mClipToView2.z; m.column2.w=g_mClipToView2.w;
    m.column3.x=g_mClipToView3.x; m.column3.y=g_mClipToView3.y; m.column3.z=g_mClipToView3.z; m.column3.w=g_mClipToView3.w;
    RRHDPrimary p = RRHDPrimaryFromClip(pixel.x,pixel.y,
        g_vInvOutputRes.x,g_vInvOutputRes.y,clipDepth,m);
    if(p.valid==0u) {
        positionView=0;positionWorld=0;normalWorld=float3(0,0,1);roughness=1;
        return false;
    }

    positionView=float3(p.position.x,p.position.y,p.position.z);
    positionWorld=PTTransformPointColumns(positionView,
        g_mViewToWorld0,g_mViewToWorld1,g_mViewToWorld2,g_mViewToWorld3);
    float3 normalView=ControlDecodeNormalView(g1);
    normalWorld=ControlNormalViewToWorld(normalView,
        g_mViewToWorld0.xyz,g_mViewToWorld1.xyz,g_mViewToWorld2.xyz);
    roughness=saturate(ControlDecodeMaterialRoughness(g1));
    return true;
}

float3 PTCameraWorld()
{
    return g_mViewToWorld3.xyz;
}

void PTTrace(uint layer, float3 origin, float3 direction, out HitData payload)
{
    RayDesc ray;
    ray.Origin=origin;
    // The native primary ray starts at its already biased origin with TMin=0.
    // Keep the existing extra-bounce self-intersection guard on layer 1.
    ray.TMin=layer==0u?0.0f:0.01f;
    ray.Direction=normalize(direction);
    ray.TMax=10000.0f;
    payload.value=layer;
    // Publish raygen clears before a callee overwrites the same texels.
    DeviceMemoryBarrier();
    TraceRay(g_rtScene,RAY_FLAG_NONE,0x1,0,2,0,ray,payload);
}

[shader("raygeneration")]
void reflectionRayGeneration()
{
    uint2 pixel=DispatchRaysIndex().xy;

    // Native reflection targets have exactly TWO array slices.
    // In the previous P2 shader, layers 2/3 were written out-of-bounds,
    // silently losing second-hit position, normal, material and UV.
    // The one-path proof uses layer 0 for the native first hit and layer 1
    // for a secondary hit, hidden from native composite via material sentinel.
    [unroll]
    for(uint layer=0;layer<2u;++layer) {
        g_rwtMaterialId[uint3(pixel,layer)]=65534u;
    }
    g_rwtNormal_TexcoordX[uint3(pixel,1u)]=float4(0.0f,0.0f,0.0f,-1.0f);
    g_rwtPosition_TexcoordY[uint3(pixel,1u)]=float4(0.0f,0.0f,0.0f,0.0f);

    float3 primaryView,primaryWorld,normalWorld;
    float roughness;
    if(!PTPrimary(pixel,primaryView,primaryWorld,normalWorld,roughness)) return;
    // Admit the full decoded material roughness range, including matte surfaces.
    // Roughness still controls the native primary lobe and secondary GGX lobe;
    // it must not be used as a binary material-coverage cutoff here.
    if(!isfinite(roughness)) return;

    float3 cameraWorld=PTCameraWorld();
    float3 viewToCamera=normalize(cameraWorld-primaryWorld);

    // Native layer 0 shades ONE stock first-bounce reflection.
    // Layer 1 is a true secondary hit; its material slot remains 65534
    // so Control does not mistake it for a second first-bounce ray.
    // This is a temporary 1-path proof pending dedicated P2 private UAVs.
    [unroll]
    for(uint path=0;path<1u;++path) {
        uint firstLayer=0u;
        uint secondLayer=1u;

        float2 random=PTRandom2(pixel,path);
        float3 normalView=ControlDecodeNormalView(g_tGBuffer1.Load(int3(pixel,0)));
        float3 incomingView=normalize(primaryView);
        float3 firstViewDirection=PTNativePrimaryDirection(normalView,incomingView,roughness,random);
        // Native retries below-horizon samples instead of leaving a missing
        // first hit. Retries change only the sample, and do not trace extra rays.
        [loop]
        for(uint retry=1u;dot(firstViewDirection,normalView)<0.005f&&retry<=100u;++retry) {
            float2 retryRandom=random+(float)(retry*10u)*float2(0.754877666f,0.569840296f);
            firstViewDirection=PTNativePrimaryDirection(normalView,incomingView,roughness,retryRandom);
        }
        float3 firstDirection=PTTransformVectorColumns(firstViewDirection,
            g_mViewToWorld0,g_mViewToWorld1,g_mViewToWorld2);

        HitData firstPayload;
        const float linearDepth=g_tLinearDepth.Load(int3(pixel,0)).x;
        const float3 firstOrigin=primaryWorld+(normalWorld+viewToCamera)*(0.001f*linearDepth);
        PTTrace(firstLayer,firstOrigin,firstDirection,firstPayload);

        uint firstMaterial=g_rwtMaterialId[uint3(pixel,firstLayer)];
        if(firstPayload.value==0u || firstMaterial>=65534u) continue;

        // Do not reconstruct a new ray origin from RGBA16F view position.
        // Its quantization can exceed the 1 cm bias and place us inside geometry.
        // The native 4-byte payload retains distance to 0.1 mm (floor).
        // Normalize exactly as PTTrace does before reconstructing its endpoint.
        const float firstDistance=(float)firstPayload.value*0.0001f;
        float3 firstHitWorld=firstOrigin+normalize(firstDirection)*firstDistance;
        float4 firstNormalUV=g_rwtNormal_TexcoordX[uint3(pixel,firstLayer)];
        float3 firstHitNormal=normalize(firstNormalUV.xyz);
        if(!all(isfinite(firstHitWorld)) || !all(isfinite(firstHitNormal))) continue;

        if(dot(firstHitNormal,firstDirection)>0.0f) firstHitNormal=-firstHitNormal;

        // Native diffuse GI is retained by the composite. Trace only the
        // indirect specular lobe that this second hit will replace.
        const float firstSmoothness=PTMaterialSmoothness(firstMaterial,float2(firstNormalUV.w,g_rwtPosition_TexcoordY[uint3(pixel,firstLayer)].w));
        const float firstRoughness=1.0f-firstSmoothness;
        float2 secondRandom=PTRandom2(pixel,path+11u);
        float3 viewBack=normalize(-firstDirection);
        float3 h2=PTSampleGGXVNDF(firstHitNormal,viewBack,firstRoughness,secondRandom);
        float3 secondDirection=normalize(reflect(-viewBack,h2));
        if(dot(secondDirection,firstHitNormal)<=0.0f)continue;

        HitData secondPayload;
        PTTrace(secondLayer,firstHitWorld+firstHitNormal*0.01f,secondDirection,secondPayload);
    }
}

// Exactly TWO native UAV array layers exist: 0=native first-hit,
// 1=secondary P2 hit (native material sentinel 65534). Only one
// first-bounce sample is retained in this proof; private four-layer UAV
// resources are needed for a full two-path implementation.
#define PT_REFLECTION_P2_NATIVE_LAYER_COUNT 2
#define PT_REFLECTION_P2_POST_COMPOSITE_READY 1

float3 PTWorldToViewPoint(float3 p)
{
    return PTTransformPointColumns(p,
        g_mWorldToView0,g_mWorldToView1,g_mWorldToView2,g_mWorldToView3);
}


uint3 PTLoadTriangleIndices(uint primitive)
{
    uint baseOffset=uIndexOffset + primitive*3u*uIndexStride;
    if(uIndexStride==2u) {
        uint maxOffset=uIndexBufferSize>=8u ? uIndexBufferSize-8u : 0u;
        uint aligned=min(baseOffset & ~3u,maxOffset);
        uint2 packed=g_bRaytracingIndexBuffer.Load2(aligned);
        if(aligned==baseOffset)
            return uint3(packed.x & 0xffffu,packed.x >> 16u,packed.y & 0xffffu);
        return uint3(packed.x >> 16u,packed.y & 0xffffu,packed.y >> 16u);
    }
    uint maxOffset=uIndexBufferSize>=12u ? uIndexBufferSize-12u : 0u;
    uint offset=min(baseOffset,maxOffset);
    return g_bRaytracingIndexBuffer.Load3(offset);
}

int2 PTUnpackS16x2(uint packed)
{
    return int2((int)(packed << 16u) >> 16,(int)packed >> 16);
}

int3 PTUnpackS16x3(uint2 packed)
{
    return int3((int)(packed.x << 16u) >> 16,
                (int)packed.x >> 16,
                (int)(packed.y << 16u) >> 16);
}

float2 PTLoadTexcoord(uint vertex)
{
    uint address=vertex*uVertexStride1 + uTexcoordOffset;
    uint maxOffset=uVertexBuffer1Size>=4u ? uVertexBuffer1Size-4u : 0u;
    uint packed=g_bRaytracingVertexBuffer1.Load(min(address,maxOffset));
    return float2(PTUnpackS16x2(packed))*(1.0f/4095.0f);
}

float3 PTLoadNormal(uint vertex)
{
    uint address=vertex*uVertexStride1 + uNormalOffset;
    uint maxOffset=uVertexBuffer1Size>=8u ? uVertexBuffer1Size-8u : 0u;
    uint2 packed=g_bRaytracingVertexBuffer1.Load2(min(address,maxOffset));
    return float3(PTUnpackS16x3(packed))*(1.0f/32767.0f);
}

float2 PTNativeTexcoord(in BuiltInTriangleIntersectionAttributes attribs)
{
    uint3 indices=PTLoadTriangleIndices(PrimitiveIndex());
    float3 weights=float3(1.0f-attribs.barycentrics.x-attribs.barycentrics.y,
                          attribs.barycentrics.x,
                          attribs.barycentrics.y);
    return PTLoadTexcoord(indices.x)*weights.x
          +PTLoadTexcoord(indices.y)*weights.y
          +PTLoadTexcoord(indices.z)*weights.z;
}

void PTNativeHitAttributes(in BuiltInTriangleIntersectionAttributes attribs,
                           out float2 texcoord,out float3 normalWorld)
{
    uint3 indices=PTLoadTriangleIndices(PrimitiveIndex());
    float3 weights=float3(1.0f-attribs.barycentrics.x-attribs.barycentrics.y,
                          attribs.barycentrics.x,
                          attribs.barycentrics.y);
    texcoord=PTNativeTexcoord(attribs);
    float3 normalObject=PTLoadNormal(indices.x)*weights.x
                       +PTLoadNormal(indices.y)*weights.y
                       +PTLoadNormal(indices.z)*weights.z;
    float3x4 worldToObject=WorldToObject3x4();
    normalWorld=float3(
        dot(normalObject,float3(worldToObject[0][0],worldToObject[1][0],worldToObject[2][0])),
        dot(normalObject,float3(worldToObject[0][1],worldToObject[1][1],worldToObject[2][1])),
        dot(normalObject,float3(worldToObject[0][2],worldToObject[1][2],worldToObject[2][2])));
}

[shader("closesthit")]
void reflectionClosestHit(inout HitData payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint layer=payload.value;
    uint2 pixel=DispatchRaysIndex().xy;

    float2 texcoord;
    float3 normalWorld;
    PTNativeHitAttributes(attribs,texcoord,normalWorld);

    float3 worldHit=WorldRayOrigin()+WorldRayDirection()*RayTCurrent();
    float3 viewHit=PTWorldToViewPoint(worldHit);

    if(layer==1u) {
        // Keep the native second material slot invalid for the stock
        // deferred composite, but encode secondary material+UV in half
        // float-compatible numeric values (NEVER asfloat(raw uint)).
        // Native normal/position textures use RGBA16_FLOAT.
        // Integers 0..2048 are exactly representable by binary16.
        g_rwtMaterialId[uint3(pixel,layer)]=65534u;
        const bool materialEncodable=uMaterialID<2048u&&dot(normalWorld,normalWorld)>1.0e-6f;
        const float encodedMaterial=materialEncodable?(float)uMaterialID:-1.0f;
        const float2 wrappedUV=frac(texcoord);
        g_rwtNormal_TexcoordX[uint3(pixel,layer)]=
            float4(PTSecondaryEncodeNormal(normalWorld),wrappedUV.x,encodedMaterial);
        g_rwtPosition_TexcoordY[uint3(pixel,layer)]=
            float4(viewHit,wrappedUV.y);
    } else {
        g_rwtMaterialId[uint3(pixel,layer)]=uMaterialID;
        g_rwtNormal_TexcoordX[uint3(pixel,layer)]=float4(normalWorld,texcoord.x);
        g_rwtPosition_TexcoordY[uint3(pixel,layer)]=float4(viewHit,texcoord.y);
    }
    // Publish hit attributes before raygen consumes them after TraceRay.
    DeviceMemoryBarrier();
    payload.value=(uint)(RayTCurrent()*10000.0f);
}

bool PTAlphaPass(in BuiltInTriangleIntersectionAttributes attribs)
{
    float2 texcoord=PTNativeTexcoord(attribs);
    uint textureIndex=NonUniformResourceIndex(uMaterialID*35u);
    float textureAlpha=g_sMaterialTextureArray[textureIndex].SampleLevel(g_sLinearWrap,texcoord,0.0f).a;
    float multiplierAlpha=g_sbMaterialDataPart3[uMaterialID].vColorMultiplier.w;
    return textureAlpha*multiplierAlpha>=0.5f;
}

[shader("anyhit")]
void reflectionAlphaTestAnyHit(inout HitData payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if(!PTAlphaPass(attribs)) IgnoreHit();
}

[shader("miss")]
void reflectionMiss(inout HitData payload)
{
    uint layer=payload.value;
    uint2 pixel=DispatchRaysIndex().xy;
    float3 worldEnd=WorldRayOrigin()+WorldRayDirection()*RayTCurrent();
    if(layer==1u) {
        g_rwtMaterialId[uint3(pixel,layer)]=65534u;
        g_rwtNormal_TexcoordX[uint3(pixel,layer)]=
            float4(0.0f,0.0f,0.0f,-1.0f);
    } else {
        g_rwtMaterialId[uint3(pixel,layer)]=65535u;
    }
    g_rwtPosition_TexcoordY[uint3(pixel,layer)]=float4(PTWorldToViewPoint(worldEnd),0.0f);
    DeviceMemoryBarrier();
    payload.value=0u;
}

[shader("closesthit")]
void shadowClosestHit(inout HitData payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint2 pixel=DispatchRaysIndex().xy;
    uint layer=payload.value;
    g_rwtShadow[uint3(pixel,layer)]=0u;
}

[shader("anyhit")]
void shadowAlphaTestAnyHit(inout HitData payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if(!PTAlphaPass(attribs)) IgnoreHit();
}

[shader("miss")]
void shadowMiss(inout HitData payload)
{
    uint2 pixel=DispatchRaysIndex().xy;
    uint layer=payload.value;
    g_rwtShadow[uint3(pixel,layer)]=1u;
}
