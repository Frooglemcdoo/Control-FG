#include "ControlPrimaryGuideDecode.hlsli"

RaytracingAccelerationStructure Scene : register(t0);
Texture2D<float4> GBuffer1 : register(t1);
Texture2D<float4> GBuffer2 : register(t2);
StructuredBuffer<uint2> MaterialDataPart1 : register(t3);

RWTexture2D<float4> Output : register(u0);
RWStructuredBuffer<uint> Counters : register(u1);

cbuffer PT3Constants : register(b0)
{
    row_major float4x4 ClipToWorld;
    float4 CameraPosTMin;
    float4 ViewToWorldBasisX;
    float4 ViewToWorldBasisY;
    float4 ViewToWorldBasisZ;
    uint2 OutputSize;
    uint Mode;           // 0 beauty, 1 hit/miss, 2 distance, 3 instance id
    uint CounterStride;
    uint FrameIndex;
    uint HdrActive;
    uint MaterialCount;
    uint Spp;
};

float3 ClipPointToWorld(float2 ndc, float z)
{
    float4 p = mul(float4(ndc, z, 1.0), ClipToWorld);
    float invW = abs(p.w) > 1e-7 ? rcp(p.w) : 1.0;
    return p.xyz * invW;
}

uint Hash32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

float Rand01(inout uint state)
{
    state = Hash32(state + 0x9e3779b9u);
    return (state & 0x00ffffffu) / 16777216.0;
}

void BasisFromNormal(float3 n, out float3 t, out float3 b)
{
    float3 up = abs(n.z) < 0.999 ? float3(0,0,1) : float3(0,1,0);
    t = normalize(cross(up,n));
    b = cross(n,t);
}

float3 CosineHemisphere(float3 n, inout uint rng)
{
    float u1 = Rand01(rng);
    float u2 = Rand01(rng);
    float r = sqrt(u1);
    float phi = 6.28318530718 * u2;
    float3 t,b;
    BasisFromNormal(n,t,b);
    return normalize(t * (r*cos(phi)) + b * (r*sin(phi)) + n * sqrt(max(0.0,1.0-u1)));
}

float3 RoughReflection(float3 incident, float3 n, float roughness, inout uint rng)
{
    float3 r = normalize(reflect(incident,n));
    float3 jitter = CosineHemisphere(r,rng);
    float a = saturate(roughness*roughness);
    float3 d = normalize(lerp(r,jitter,a));
    if(dot(d,n)<=0.001) d = normalize(r + n*0.05);
    return d;
}

float3 SkyRadiance(float3 d)
{
    float y = saturate(d.y*0.5+0.5);
    float3 horizon = float3(0.12,0.14,0.18);
    float3 zenith = float3(0.65,0.78,1.05);
    float3 ground = float3(0.055,0.045,0.038);
    return d.y >= 0.0 ? lerp(horizon,zenith,pow(y,0.65)) : lerp(ground,horizon,y);
}

bool Trace(float3 origin,float3 direction,float tmin,float tmax,out float t,out uint instanceId)
{
    RayDesc ray;
    ray.Origin=origin;
    ray.Direction=normalize(direction);
    ray.TMin=tmin;
    ray.TMax=tmax;
    RayQuery<RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(Scene,RAY_FLAG_FORCE_OPAQUE,0xFF,ray);
    while(q.Proceed()){}
    const bool hit=q.CommittedStatus()==COMMITTED_TRIANGLE_HIT;
    t=hit?q.CommittedRayT():tmax;
    instanceId=hit?q.CommittedInstanceID():0u;
    return hit;
}

float3 IdColor(uint id)
{
    uint x = Hash32(id*3u+1u);
    uint y = Hash32(id*3u+2u);
    uint z = Hash32(id*3u+3u);
    return float3((x&255u)/255.0,(y&255u)/255.0,(z&255u)/255.0);
}

float3 MaterialColor(float4 g2,out float3 f0,out uint brdfMode)
{
    uint materialID=ControlDecodeMaterialID(g2);
    f0=float3(0.04,0.04,0.04);
    brdfMode=0u;
    float3 packedColor=float3(0.45,0.47,0.50);
    if(materialID<MaterialCount){
        uint packed=MaterialDataPart1[materialID].x;
        brdfMode=packed&255u;
        packedColor=float3((packed>>24u)&255u,(packed>>16u)&255u,(packed>>8u)&255u)/255.0;
        f0=ControlDecodeMaterialF0(packed,g2.g);
    }
    // The available Part1 value is an authenticated material reflectance term,
    // not full secondary-hit BSDF data. Turn it into a stable primary-surface
    // tint for the first hybrid PT beauty pass.
    float brightness=0.42+0.58*saturate(g2.r);
    float3 neutral=float3(0.52,0.50,0.47)*brightness;
    float metalHint=saturate(max(f0.r,max(f0.g,f0.b))*1.4);
    return saturate(lerp(neutral,max(packedColor,0.03),0.30+0.55*metalHint));
}

float3 TonemapSDR(float3 c)
{
    c=max(c,0.0);
    c=(c*(2.51*c+0.03))/(c*(2.43*c+0.59)+0.14);
    return pow(saturate(c),1.0/2.2);
}

[numthreads(8,8,1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    if(dispatchId.x>=OutputSize.x||dispatchId.y>=OutputSize.y)return;

    float2 uv=(float2(dispatchId.xy)+0.5)/float2(OutputSize);
    float2 ndc=float2(uv.x*2.0-1.0,1.0-uv.y*2.0);
    float3 p0=ClipPointToWorld(ndc,0.0);
    float3 p1=ClipPointToWorld(ndc,1.0);
    float3 camera=CameraPosTMin.xyz;
    float3 farPoint=dot(p1-camera,p1-camera)>dot(p0-camera,p0-camera)?p1:p0;
    float3 primaryDir=normalize(farPoint-camera);

    float primaryT;
    uint instanceId;
    bool primaryHit=Trace(camera,primaryDir,max(CameraPosTMin.w,0.001),100000.0,primaryT,instanceId);

    const bool sampled=(dispatchId.x%max(CounterStride,1u))==0u && (dispatchId.y%max(CounterStride,1u))==0u;
    if(sampled){
        InterlockedAdd(Counters[0],1u);
        if(primaryHit){
            InterlockedAdd(Counters[1],1u);
            uint fixedDistance=min((uint)(min(primaryT,1000.0)*16.0+0.5),16000u);
            InterlockedMin(Counters[3],fixedDistance);
            InterlockedMax(Counters[4],fixedDistance);
            InterlockedAdd(Counters[5],fixedDistance);
            InterlockedXor(Counters[6],(instanceId*2654435761u)^(dispatchId.y*OutputSize.x+dispatchId.x));
        }else InterlockedAdd(Counters[2],1u);
    }

    if(Mode!=0u){
        float3 value=0.0;
        if(Mode==1u)value=primaryHit?1.0.xxx:0.0.xxx;
        else if(Mode==2u){
            float v=primaryHit?(1.0-exp(-primaryT*0.02)):0.0;
            value=v.xxx;
        }else value=primaryHit?IdColor(instanceId):0.0.xxx;
        Output[dispatchId.xy]=float4(value,1.0);
        return;
    }

    if(!primaryHit){
        // Full-scene mode preserves Control's native sky/background. A primary
        // miss therefore carries a neutral lighting multiplier.
        Output[dispatchId.xy]=float4(1.0.xxx,1.0);
        return;
    }

    const float4 g1=GBuffer1.Load(int3(dispatchId.xy,0));
    const float4 g2=GBuffer2.Load(int3(dispatchId.xy,0));
    float3 nView=ControlDecodeNormalView(g1);
    float3 n=ControlNormalViewToWorld(nView,ViewToWorldBasisX.xyz,ViewToWorldBasisY.xyz,ViewToWorldBasisZ.xyz);
    if(dot(n,-primaryDir)<0.0)n=-n;
    const float roughness=saturate(ControlDecodeMaterialRoughness(g1));

    float3 f0;
    uint brdfMode;
    MaterialColor(g2,f0,brdfMode);
    const float3 hitPos=camera+primaryDir*primaryT;
    const float epsilon=max(0.003,primaryT*0.00015);

    // Stable per-pixel sampling is intentional for the first full-scene build:
    // it avoids frame-to-frame glitter until temporal accumulation is added.
    uint rng=Hash32(dispatchId.x+dispatchId.y*OutputSize.x+0x68bc21ebu);

    float diffuseT;
    uint diffuseInstance;
    float3 diffuseDir=CosineHemisphere(n,rng);
    bool diffuseHit=Trace(hitPos+n*epsilon,diffuseDir,epsilon,250.0,diffuseT,diffuseInstance);
    if(sampled)InterlockedAdd(Counters[diffuseHit?7:8],1u);

    float diffuseOpen=diffuseHit
        ? lerp(0.42,0.98,saturate(diffuseT/24.0))
        : 1.12;
    float3 diffuseTint=1.0.xxx;
    if(!diffuseHit){
        float3 sky=SkyRadiance(diffuseDir);
        float skyL=max(dot(sky,float3(0.2126,0.7152,0.0722)),0.001);
        diffuseTint=clamp(sky/skyL,0.72,1.28);
    }

    float specT;
    uint specInstance;
    float3 specDir=RoughReflection(primaryDir,n,roughness,rng);
    bool specHit=Trace(hitPos+n*epsilon,specDir,epsilon,500.0,specT,specInstance);
    if(sampled)InterlockedAdd(Counters[specHit?9:10],1u);

    float specOpen=specHit
        ? lerp(0.94,1.05,saturate(specT/64.0))
        : 1.20;
    float3 specTint=1.0.xxx;
    if(!specHit){
        float3 sky=SkyRadiance(specDir);
        float skyL=max(dot(sky,float3(0.2126,0.7152,0.0722)),0.001);
        specTint=clamp(sky/skyL,0.70,1.32);
    }

    float ndotv=saturate(dot(n,-primaryDir));
    float3 fresnel=f0+(1.0-f0)*pow(1.0-ndotv,5.0);
    float specWeight=lerp(1.0,0.30,roughness);
    float upward=saturate(n.y*0.5+0.5);

    // PT4 full-scene BEAUTY is a path-traced lighting field, not a synthetic
    // replacement material. The late composite multiplies this over Control's
    // authored color frame so textures, characters and post effects remain.
    float3 multiplier=diffuseOpen*lerp(1.0.xxx,diffuseTint,0.10);
    multiplier+=(specOpen-1.0)*lerp(1.0.xxx,specTint,0.18)*fresnel*(1.15*specWeight);
    multiplier*=lerp(0.96,1.04,upward);
    if(brdfMode==3u)multiplier=diffuseOpen.xxx;
    multiplier=lerp(1.0.xxx,multiplier,0.90);
    multiplier=clamp(multiplier,0.50,1.50);

    Output[dispatchId.xy]=float4(multiplier,1.0);
}
