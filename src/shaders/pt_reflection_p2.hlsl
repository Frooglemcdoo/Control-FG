// Experimental Control PT reflections P2 shader library.
// Compile-only until runtime admission and replacement transport are proven.
// Native register contract is recovered from rt_reflection-000.dxbc / lib_6_3.

#include "ControlPrimaryGuideDecode.hlsli"
#include "rr_hit_origin_shared.h"
#include "pt_private_hits.hlsli"

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
    ray.TMin=0.01f;
    ray.Direction=normalize(direction);
    ray.TMax=10000.0f;
    payload.value=layer;
    // Publish raygen clears before a callee overwrites the same texels.
    DeviceMemoryBarrier();
    TraceRay(g_rtScene,RAY_FLAG_NONE,0x1,0,2,0,ray,payload);
}

// Use the native hit/miss shaders with a temporary layer-0 scratch write.
// Save/restore every field inside this helper, including on misses. Layer 1
// is never touched. Only the private records survive the additional dispatch.
bool PTTracePreserved(uint2 pixel,float3 origin,float3 direction,out PTPrivateHit hit)
{
    const uint3 slot=uint3(pixel,0u);
    const uint savedMaterial=g_rwtMaterialId[slot];
    const float4 savedNormal=g_rwtNormal_TexcoordX[slot];
    const float4 savedPosition=g_rwtPosition_TexcoordY[slot];
    HitData payload;
    PTTrace(0u,origin,direction,payload);
    DeviceMemoryBarrier();
    const uint material=g_rwtMaterialId[slot];
    const float4 normal=g_rwtNormal_TexcoordX[slot];
    const float4 position=g_rwtPosition_TexcoordY[slot];
    // Restore unconditionally BEFORE testing the hit or returning.
    g_rwtMaterialId[slot]=savedMaterial;
    g_rwtNormal_TexcoordX[slot]=savedNormal;
    g_rwtPosition_TexcoordY[slot]=savedPosition;
    DeviceMemoryBarrier();
    hit.worldPosition=origin+normalize(direction)*((float)payload.value*0.0001f);
    hit.material=material;hit.uv=float2(normal.w,position.w);
    hit.packedNormal=PTPackNormal(normal.xyz);hit.reserved=0u;
    return payload.value!=0u&&material<65534u&&all(isfinite(hit.worldPosition))&&
        all(isfinite(normal.xyz))&&dot(normal.xyz,normal.xyz)>1.0e-6f;
}

[shader("raygeneration")]
void reflectionRayGeneration()
{
    uint2 pixel=DispatchRaysIndex().xy;
    uint privatePixel=pixel.y*DispatchRaysDimensions().x+pixel.x;
    if(privatePixel>=PT_PRIVATE_PIXEL_CAPACITY)return;
    PTClearPrivate(privatePixel*2u);PTClearPrivate(privatePixel*2u+1u);
    uint width,height,layers;g_rwtMaterialId.GetDimensions(width,height,layers);
    if(any(pixel>=uint2(width,height))||layers!=2u||g_uRTReflectionRayCount==0u)return;
    uint count,stride;g_sbMaterialDataPart3.GetDimensions(count,stride);
    uint firstMaterial=g_rwtMaterialId[uint3(pixel,0u)];
    if(stride!=336u||firstMaterial>=min(count,65534u))return;
    float4 firstPosition=g_rwtPosition_TexcoordY[uint3(pixel,0u)];
    if(!all(isfinite(firstPosition.xyz)))return;
    float3 primaryView,primaryWorld,normalWorld;float roughness;
    if(!PTPrimary(pixel,primaryView,primaryWorld,normalWorld,roughness))return;
    float3 nativeFirstWorld=PTTransformPointColumns(firstPosition.xyz,
        g_mViewToWorld0,g_mViewToWorld1,g_mViewToWorld2,g_mViewToWorld3);
    // Re-intersect the first surface to recover a sub-millimetre endpoint.
    // The native half-float position is a direction guide, NEVER a new origin.
    float depth=g_tLinearDepth.Load(int3(pixel,0)).x;
    const float3 firstOrigin=primaryWorld+(normalWorld-normalize(primaryWorld-PTCameraWorld()))*(0.001f*depth);
    float3 firstDirection=normalize(nativeFirstWorld-firstOrigin);
    PTPrivateHit first;
    if(!PTTracePreserved(pixel,firstOrigin,firstDirection,first))return;
    if(first.material!=firstMaterial)return;
    float tolerance=0.003f+0.002f*max(max(abs(firstPosition.x),abs(firstPosition.y)),abs(firstPosition.z));
    if(distance(first.worldPosition,nativeFirstWorld)>tolerance)return;
    float3 firstNormal=PTUnpackNormal(first.packedNormal);
    if(dot(firstNormal,firstDirection)>0.0f)firstNormal=-firstNormal;
    float firstRoughness=1.0f-PTMaterialSmoothness(first.material,first.uv);
    float3 h2=PTSampleGGXVNDF(firstNormal,-firstDirection,firstRoughness,PTRandom2(pixel,11u));
    float3 secondDirection=normalize(reflect(firstDirection,h2));
    if(dot(secondDirection,firstNormal)<=0.0f)return;
    PTPrivateHit second;
    if(!PTTracePreserved(pixel,first.worldPosition+firstNormal*0.01f,secondDirection,second))return;
    if(second.material>=count)return;
    g_ptPrivateHits[privatePixel*2u]=second;
    float3 secondNormal=PTUnpackNormal(second.packedNormal);
    if(dot(secondNormal,secondDirection)>0.0f)secondNormal=-secondNormal;
    float secondRoughness=1.0f-PTMaterialSmoothness(second.material,second.uv);
    float3 h3=PTSampleGGXVNDF(secondNormal,-secondDirection,secondRoughness,PTRandom2(pixel,29u));
    float3 thirdDirection=normalize(reflect(secondDirection,h3));
    if(dot(thirdDirection,secondNormal)<=0.0f)return;
    PTPrivateHit third;
    if(PTTracePreserved(pixel,second.worldPosition+secondNormal*0.01f,thirdDirection,third)&&third.material<count)
        g_ptPrivateHits[privatePixel*2u+1u]=third;
}

// Exactly TWO native layers are preserved by the extra raygen. The following
// sidecar exports ONLY reflectionRayGeneration into the combined state.
// All actual hit/miss/shadow exports are the game's original DXIL.
#define PT_REFLECTION_P2_NATIVE_LAYER_COUNT 2
#define PT_REFLECTION_P2_POST_COMPOSITE_READY 1
