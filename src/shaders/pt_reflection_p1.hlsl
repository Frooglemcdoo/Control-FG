// Experimental Control PT reflections P1 shader library.
// Four independent stochastic first-bounce reflection samples.
// This preserves Control's stock four-layer deferred reflection composite contract.
// Runtime activation remains gated by ShaderCodeStorage transport validation.

#include "ControlPrimaryGuideDecode.hlsli"
#include "rr_hit_origin_shared.h"

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
    float tail[67];
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

RWTexture2DArray<uint>   g_rwtMaterialId : register(u0);
RWTexture2DArray<float4> g_rwtNormal_TexcoordX : register(u1);
RWTexture2DArray<float4> g_rwtPosition_TexcoordY : register(u2);
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
    ray.TMin=0.01f;
    ray.Direction=normalize(direction);
    ray.TMax=10000.0f;
    payload.value=layer;
    TraceRay(g_rtScene,RAY_FLAG_NONE,0x1,0,2,0,ray,payload);
}

[shader("raygeneration")]
void reflectionRayGeneration()
{
    uint2 pixel=DispatchRaysIndex().xy;

    [unroll]
    for(uint layer=0;layer<4;++layer)
        g_rwtMaterialId[uint3(pixel,layer)]=65534u;

    float3 primaryView,primaryWorld,normalWorld;
    float roughness;
    if(!PTPrimary(pixel,primaryView,primaryWorld,normalWorld,roughness)) return;
    if(roughness>g_fRTMaxRayRoughness) return;

    const float3 viewToCamera=normalize(PTCameraWorld()-primaryWorld);
    const uint rayCount=min(g_uRTReflectionRayCount,4u);

    // Stock-compatible layout: every populated layer is an independent
    // first-bounce sample. Control's native deferred reflection pass can
    // therefore shade/combine these layers exactly as it does today.
    [loop]
    for(uint layer=0;layer<rayCount;++layer) {
        float2 random=PTRandom2(pixel,layer);
        float3 h=PTSampleGGXVNDF(normalWorld,viewToCamera,roughness,random);
        float3 direction=reflect(-viewToCamera,h);
        if(dot(direction,normalWorld)<=0.0f) continue;

        HitData payload;
        PTTrace(layer,primaryWorld+normalWorld*0.01f,direction,payload);
    }
}

// P1 is stock-composite compatible: no layer is repurposed as a second bounce.
#define PT_REFLECTION_P1_STOCK_COMPOSITE_COMPATIBLE 1

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

    g_rwtMaterialId[uint3(pixel,layer)]=uMaterialID;
    g_rwtNormal_TexcoordX[uint3(pixel,layer)]=float4(normalWorld,texcoord.x);
    g_rwtPosition_TexcoordY[uint3(pixel,layer)]=float4(viewHit,texcoord.y);
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
    g_rwtMaterialId[uint3(pixel,layer)]=65535u;
    g_rwtPosition_TexcoordY[uint3(pixel,layer)]=float4(PTWorldToViewPoint(worldEnd),0.0f);
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
