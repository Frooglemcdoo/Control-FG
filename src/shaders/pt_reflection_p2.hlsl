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

cbuffer PTExtraSysConstants : register(b0)
{
    float2 ptExtra_vScreenRes;
    float2 ptExtra_vInvScreenRes;
    float2 ptExtra_vOutputRes;
    float2 ptExtra_vInvOutputRes;

    float4 ptExtra_mWorldToView0;
    float4 ptExtra_mWorldToView1;
    float4 ptExtra_mWorldToView2;
    float4 ptExtra_mWorldToView3;

    float4 ptExtra_mViewToWorld0;
    float4 ptExtra_mViewToWorld1;
    float4 ptExtra_mViewToWorld2;
    float4 ptExtra_mViewToWorld3;

    float4 ptExtra_mViewToClip0;
    float4 ptExtra_mViewToClip1;
    float4 ptExtra_mViewToClip2;
    float4 ptExtra_mViewToClip3;

    float4 ptExtra_mClipToView0;
    float4 ptExtra_mClipToView1;
    float4 ptExtra_mClipToView2;
    float4 ptExtra_mClipToView3;
};

cbuffer ptExtra_cbRaytracingHit : register(b0, space3)
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

cbuffer PTExtraReflectionConstants : register(b1)
{
    float ptExtra_fClampReflectionIntensity;
    float ptExtra_fRTReflectionAddRays;
    uint  ptExtra_uRTReflectionRayCount;
    float ptExtra_fRTZeroRayReflectance;
    float ptExtra_fRTMaxRayReflectance;
    float ptExtra_fRTMaxRayRoughness;
};

Texture2D<float4> ptExtra_tGBuffer1 : register(t0);
Texture2D<float4> ptExtra_tLinearDepth : register(t1);
Texture2D<float4> ptExtra_tClipDepth : register(t2);
StructuredBuffer<PTMaterialDataPart3> ptExtra_sbMaterialDataPart3 : register(t3);
Texture2D<float4> ptExtra_tStaticBlueNoiseRGBA_0 : register(t4);
RaytracingAccelerationStructure ptExtra_rtScene : register(t5);
Texture2D<float4> ptExtra_sMaterialTextureArray[] : register(t0, space1);
SamplerState ptExtra_sLinearWrap : register(s5, space1);
ByteAddressBuffer ptExtra_bRaytracingIndexBuffer : register(t0, space3);
ByteAddressBuffer ptExtra_bRaytracingVertexBuffer1 : register(t1, space3);

globallycoherent RWTexture2DArray<uint>   ptExtra_rwtMaterialId : register(u0);
globallycoherent RWTexture2DArray<float4> ptExtra_rwtNormal_TexcoordX : register(u1);
globallycoherent RWTexture2DArray<float4> ptExtra_rwtPosition_TexcoordY : register(u2);
RWTexture2DArray<uint>   ptExtra_rwtShadow : register(u3);

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
    ptExtra_tStaticBlueNoiseRGBA_0.GetDimensions(w,h);
    uint2 p = uint2((pixel.x + path*73u) % max(w,1u),
                    (pixel.y + path*151u) % max(h,1u));
    float4 n = ptExtra_tStaticBlueNoiseRGBA_0.Load(int3(p,0));
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
    float2 range=ptExtra_sbMaterialDataPart3[materialId].vSmoothnessRange;
    float tex=ptExtra_sMaterialTextureArray[NonUniformResourceIndex(materialId*35u+2u)].SampleLevel(ptExtra_sLinearWrap,uv,0.0f).x;
    return saturate(lerp(range.x,range.y,tex));
}

bool PTPrimary(uint2 pixel, out float3 positionView, out float3 positionWorld,
               out float3 normalWorld, out float roughness)
{
    float4 g1 = ptExtra_tGBuffer1.Load(int3(pixel,0));
    float clipDepth = ptExtra_tClipDepth.Load(int3(pixel,0)).x;

    RRHDClipToView m;
    m.column0.x=ptExtra_mClipToView0.x; m.column0.y=ptExtra_mClipToView0.y; m.column0.z=ptExtra_mClipToView0.z; m.column0.w=ptExtra_mClipToView0.w;
    m.column1.x=ptExtra_mClipToView1.x; m.column1.y=ptExtra_mClipToView1.y; m.column1.z=ptExtra_mClipToView1.z; m.column1.w=ptExtra_mClipToView1.w;
    m.column2.x=ptExtra_mClipToView2.x; m.column2.y=ptExtra_mClipToView2.y; m.column2.z=ptExtra_mClipToView2.z; m.column2.w=ptExtra_mClipToView2.w;
    m.column3.x=ptExtra_mClipToView3.x; m.column3.y=ptExtra_mClipToView3.y; m.column3.z=ptExtra_mClipToView3.z; m.column3.w=ptExtra_mClipToView3.w;
    RRHDPrimary p = RRHDPrimaryFromClip(pixel.x,pixel.y,
        ptExtra_vInvOutputRes.x,ptExtra_vInvOutputRes.y,clipDepth,m);
    if(p.valid==0u) {
        positionView=0;positionWorld=0;normalWorld=float3(0,0,1);roughness=1;
        return false;
    }

    positionView=float3(p.position.x,p.position.y,p.position.z);
    positionWorld=PTTransformPointColumns(positionView,
        ptExtra_mViewToWorld0,ptExtra_mViewToWorld1,ptExtra_mViewToWorld2,ptExtra_mViewToWorld3);
    float3 normalView=ControlDecodeNormalView(g1);
    normalWorld=ControlNormalViewToWorld(normalView,
        ptExtra_mViewToWorld0.xyz,ptExtra_mViewToWorld1.xyz,ptExtra_mViewToWorld2.xyz);
    roughness=saturate(ControlDecodeMaterialRoughness(g1));
    return true;
}

float3 PTCameraWorld()
{
    return ptExtra_mViewToWorld3.xyz;
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
    TraceRay(ptExtra_rtScene,RAY_FLAG_NONE,0x1,0,2,0,ray,payload);
}

// Each stage is a separate DispatchRays followed by an API UAV barrier.
// Native hit shaders do not publish globallycoherent writes inside TraceRay;
// NEVER read their output in the same dispatch that invokes them.
cbuffer PTExtraStage : register(b0,space99) { uint ptExtraStage; };

PTPrivateHit PTTracePending(float3 origin,float3 direction)
{
    HitData payload;PTTrace(0u,origin,direction,payload);
    PTPrivateHit h=(PTPrivateHit)0;
    h.worldPosition=origin+normalize(direction)*((float)payload.value*0.0001f);
    h.material=65535u;h.reserved=payload.value!=0u?1u:0u;
    return h; // payload is returned by value; no hit-texture read here.
}
bool PTReadPending(uint2 pixel,PTPrivateHit pending,out PTPrivateHit hit)
{
    hit=pending;
    if(pending.reserved!=1u)return false;
    float4 n=ptExtra_rwtNormal_TexcoordX[uint3(pixel,0u)];
    float4 p=ptExtra_rwtPosition_TexcoordY[uint3(pixel,0u)];
    hit.material=ptExtra_rwtMaterialId[uint3(pixel,0u)];
    hit.uv=float2(n.w,p.w);hit.packedNormal=PTPackNormal(n.xyz);hit.reserved=0u;
    uint count,stride;ptExtra_sbMaterialDataPart3.GetDimensions(count,stride);
    return stride==336u&&hit.material<min(count,65534u)&&all(isfinite(hit.worldPosition))&&
        all(isfinite(n))&&isfinite(p.w)&&dot(n.xyz,n.xyz)>1.0e-6f;
}
void PTRestoreNative(uint2 pixel,PTPrivateHit backup)
{
    if((backup.reserved&0x10000u)==0u)return;
    float3 normal=float3(f16tof32(backup.packedNormal&65535u),
        f16tof32(backup.packedNormal>>16u),f16tof32(backup.reserved&65535u));
    ptExtra_rwtMaterialId[uint3(pixel,0u)]=backup.material;
    ptExtra_rwtNormal_TexcoordX[uint3(pixel,0u)]=float4(normal,backup.uv.x);
    ptExtra_rwtPosition_TexcoordY[uint3(pixel,0u)]=float4(backup.worldPosition,backup.uv.y);
}
[shader("raygeneration")]
void reflectionRayGeneration()
{
    uint2 pixel=DispatchRaysIndex().xy;
    uint privatePixel=pixel.y*DispatchRaysDimensions().x+pixel.x;
    if(privatePixel>=PT_PRIVATE_PIXEL_CAPACITY)return;
    uint base=privatePixel*3u;
    if(ptExtraStage==0u){
        PTClearPrivate(base);PTClearPrivate(base+1u);PTClearPrivate(base+2u);
        uint width,height,layers;ptExtra_rwtMaterialId.GetDimensions(width,height,layers);
        if(any(pixel>=uint2(width,height))||layers!=2u||ptExtra_uRTReflectionRayCount==0u)return;
        uint count,stride;ptExtra_sbMaterialDataPart3.GetDimensions(count,stride);
        uint material=ptExtra_rwtMaterialId[uint3(pixel,0u)];
        float4 n=ptExtra_rwtNormal_TexcoordX[uint3(pixel,0u)];
        float4 p=ptExtra_rwtPosition_TexcoordY[uint3(pixel,0u)];
        if(stride!=336u||material>=min(count,65534u)||!all(isfinite(n))||!all(isfinite(p)))return;
        // Lossless backup of native half-float normal components (not oct encoding).
        PTPrivateHit backup;
        backup.worldPosition=p.xyz;backup.material=material;backup.uv=float2(n.w,p.w);
        backup.packedNormal=f32tof16(n.x)|(f32tof16(n.y)<<16u);
        backup.reserved=f32tof16(n.z)|0x10000u;
        g_ptPrivateHits[base+2u]=backup;
        float3 primaryView,primaryWorld,normalWorld;float roughness;
        if(!PTPrimary(pixel,primaryView,primaryWorld,normalWorld,roughness))return;
        float3 nativeFirstWorld=PTTransformPointColumns(p.xyz,
            ptExtra_mViewToWorld0,ptExtra_mViewToWorld1,ptExtra_mViewToWorld2,ptExtra_mViewToWorld3);
        float depth=ptExtra_tLinearDepth.Load(int3(pixel,0)).x;
        const float3 firstOrigin=primaryWorld+(normalWorld-normalize(primaryWorld-PTCameraWorld()))*(0.001f*depth);
        float3 firstDirection=normalize(nativeFirstWorld-firstOrigin);
        PTPrivateHit origin=(PTPrivateHit)0;origin.material=65535u;origin.worldPosition=firstOrigin;
        g_ptPrivateHits[base+1u]=origin;
        // The native FP16 position guides a re-intersection; never use it as a new ray origin.
        g_ptPrivateHits[base]=PTTracePending(firstOrigin,firstDirection);
        return;
    }
    if(ptExtraStage==1u){
        PTPrivateHit pending=g_ptPrivateHits[base];PTClearPrivate(base);
        PTPrivateHit first;if(!PTReadPending(pixel,pending,first))return;
        PTPrivateHit backup=g_ptPrivateHits[base+2u];
        if(first.material!=backup.material)return;
        float3 nativeFirstWorld=PTTransformPointColumns(backup.worldPosition,
            ptExtra_mViewToWorld0,ptExtra_mViewToWorld1,ptExtra_mViewToWorld2,ptExtra_mViewToWorld3);
        float3 absView=abs(backup.worldPosition);
        float tolerance=0.003f+0.002f*max(max(absView.x,absView.y),absView.z);
        if(distance(first.worldPosition,nativeFirstWorld)>tolerance)return;
        float3 firstDirection=normalize(first.worldPosition-g_ptPrivateHits[base+1u].worldPosition);
        float3 firstNormal=PTUnpackNormal(first.packedNormal);
        if(dot(firstNormal,firstDirection)>0.0f)firstNormal=-firstNormal;
        float roughness=1.0f-PTMaterialSmoothness(first.material,first.uv);
        float3 h=PTSampleGGXVNDF(firstNormal,-firstDirection,roughness,PTRandom2(pixel,11u));
        float3 direction=normalize(reflect(firstDirection,h));
        if(dot(direction,firstNormal)<=0.0f)return;
        // Preserve first endpoint for the next stage's incoming direction.
        PTPrivateHit origin=(PTPrivateHit)0;origin.material=65535u;origin.worldPosition=first.worldPosition;
        g_ptPrivateHits[base+1u]=origin;
        g_ptPrivateHits[base]=PTTracePending(first.worldPosition+firstNormal*0.01f,direction);
        return;
    }
    if(ptExtraStage==2u){
        PTPrivateHit pending=g_ptPrivateHits[base];PTClearPrivate(base);
        PTPrivateHit second;if(!PTReadPending(pixel,pending,second))return;
        float3 incoming=normalize(second.worldPosition-g_ptPrivateHits[base+1u].worldPosition);
        g_ptPrivateHits[base]=second;
        float3 normal=PTUnpackNormal(second.packedNormal);
        if(dot(normal,incoming)>0.0f)normal=-normal;
        float roughness=1.0f-PTMaterialSmoothness(second.material,second.uv);
        float3 h=PTSampleGGXVNDF(normal,-incoming,roughness,PTRandom2(pixel,29u));
        float3 direction=normalize(reflect(incoming,h));
        if(dot(direction,normal)<=0.0f)return;
        g_ptPrivateHits[base+1u]=PTTracePending(second.worldPosition+normal*0.01f,direction);
        return;
    }
    if(ptExtraStage==3u){
        PTPrivateHit pending=g_ptPrivateHits[base+1u];PTClearPrivate(base+1u);
        PTPrivateHit third;if(PTReadPending(pixel,pending,third))g_ptPrivateHits[base+1u]=third;
        // Unconditional restoration even when a re-intersection or later bounce missed.
        PTRestoreNative(pixel,g_ptPrivateHits[base+2u]);
    }
}
#define PT_REFLECTION_P2_NATIVE_LAYER_COUNT 2
#define PT_REFLECTION_P2_POST_COMPOSITE_READY 1
