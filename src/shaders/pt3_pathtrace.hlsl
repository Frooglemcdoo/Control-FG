#include "ControlPrimaryGuideDecode.hlsli"

RaytracingAccelerationStructure Scene : register(t0);
Texture2D<float4> GBuffer1 : register(t1);
Texture2D<float4> GBuffer2 : register(t2);
StructuredBuffer<uint2> MaterialDataPart1 : register(t3);
Texture2D<float4> HistoryLighting : register(t4);
Texture2D<float4> HistoryMeta : register(t5);
Texture2D<float4> PreviousScene : register(t6);
Texture2D<float4> HistorySurface : register(t7);
StructuredBuffer<float4> MaterialDataPart3Raw : register(t8);
StructuredBuffer<float4> PointLightsRaw : register(t9);
StructuredBuffer<float4> SpotLightsRaw : register(t10);
Texture2D<float4> NativeLightDiffuse : register(t11);

RWTexture2D<float4> Output : register(u0);
RWStructuredBuffer<uint> Counters : register(u1);
RWTexture2D<float4> HistoryLightingOut : register(u2);
RWTexture2D<float4> HistoryMetaOut : register(u3);
RWTexture2D<float4> HistorySurfaceOut : register(u4);
RWStructuredBuffer<uint> EmissiveCandidates : register(u5);
RWTexture2D<float4> EmissiveOutput : register(u6);

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
    row_major float4x4 PrevWorldToClip;
    float4 PrevCameraPos;
    uint HistoryValid;
    uint MaxHistory;
};

uint PTViewMode() { return Mode & 0xFFu; }
bool PTStandalone() { return (Mode & 0x100u) != 0u; }
uint PTPathSamples() { uint samples = Spp & 0xFFu; return max(samples, 1u); }
uint PTBounceCount() { uint b = (Spp >> 8u) & 0xFFu; return clamp(b, 1u, 4u); }
bool PTEmissiveDiscoveryPass() { return (Spp & 0x40000000u) != 0u; }
float PTAugStrength() { return saturate(((Mode >> 9u) & 0x7Fu) / 100.0); }
float PTContactRadiusMeters() { return clamp(((Mode >> 16u) & 0xFFu) / 100.0,0.05,0.75); }
float PTMaxShadowDistanceMeters() { return clamp(((Mode >> 24u) & 0xFFu) * 0.05,0.25,3.00); }
float PTPreExposure() { return clamp(PrevCameraPos.w, 0.125, 16.0); }

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
    // PT9: neutral temporary environment. The old blue-biased synthetic sky
    // was being treated as real incoming radiance and produced a purple/cool
    // cast indoors. Keep directional energy while removing artificial chroma.
    float y=saturate(d.y*0.5+0.5);
    float ground=0.075;
    float horizon=0.18;
    float zenith=0.34;
    float energy=d.y>=0.0?lerp(horizon,zenith,pow(y,0.65)):lerp(ground,horizon,y);
    return energy.xxx;
}

float Luminance(float3 c)
{
    return dot(c,float3(0.2126,0.7152,0.0722));
}

float3 DisplayToSceneLinear(float3 c)
{
    // PreviousScene is the authored display-domain Control frame in SDR. It is
    // useful as a secondary-hit radiance cache only after decoding it back to
    // a linear-light approximation. HDR bridge content is already treated as
    // linear/high-range by this experimental path.
    return HdrActive!=0u?max(c,0.0):pow(saturate(c),2.2);
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
    // Standalone needs authored material chroma because the native low-frequency
    // beauty lighting is deliberately removed at composite. Keep the hybrid
    // weighting conservative, but trust the authenticated packed reflectance
    // substantially more for standalone primary/material history.
    float colorWeight=PTStandalone()?0.72:(0.30+0.55*metalHint);
    float3 materialColor=saturate(lerp(neutral,max(packedColor,0.03),colorWeight));
    if(PTStandalone()){
        uint rawCount=0,rawStride=0;MaterialDataPart3Raw.GetDimensions(rawCount,rawStride);
        uint part3Count=rawCount/21u; // current Control PT22 capture: 336 bytes / 16-byte float4 blocks
        if(materialID<part3Count){
            uint base=materialID*21u;
            float4 cloth=MaterialDataPart3Raw[base+1u];
            float4 spec2=MaterialDataPart3Raw[base+2u];
            float4 colorMul=MaterialDataPart3Raw[base+4u];
            float3 mul=clamp(colorMul.xyz,0.35,2.25);
            float clothAmount=saturate(MaterialDataPart3Raw[base+0u].x);
            float hairSmooth=saturate(cloth.w);
            materialColor*=lerp(1.0.xxx,mul,0.28);
            materialColor=lerp(materialColor,clamp(cloth.xyz,0.02,1.5),clothAmount*0.12);
            f0=lerp(f0,clamp(spec2.xyz,0.01,1.0),hairSmooth*0.18);
        }
    }
    return saturate(materialColor);
}

// PT27: Control MaterialDataPart3 contains the real uniform emission controls.
// The reflected layout is 336 bytes (21 float4 blocks): fEmissionIntensity is
// byte 48 = block 3.x; vEmissionMultiplier is byte 100 = block 6.yzw.
float3 MaterialEmissionFromID(uint materialID)
{
    uint rawCount=0,rawStride=0;
    MaterialDataPart3Raw.GetDimensions(rawCount,rawStride);
    uint part3Count=rawCount/21u;
    if(materialID>=part3Count)return 0.0.xxx;
    uint base=materialID*21u;
    float intensity=max(MaterialDataPart3Raw[base+3u].x,0.0);
    float3 multiplier=max(MaterialDataPart3Raw[base+6u].yzw,0.0);
    if(intensity<=0.0005)return 0.0.xxx;
    return clamp(multiplier*intensity,0.0,64.0);
}

float3 MaterialEmissionAtPixel(uint2 pixel,out uint materialID)
{
    materialID=ControlDecodeMaterialID(GBuffer2.Load(int3(pixel,0)));
    return MaterialEmissionFromID(materialID);
}

static const uint PTEmissiveCandidateCapacity=4096u;

uint PTEmissiveTileSize()
{
    uint tiles64=((OutputSize.x+63u)/64u)*((OutputSize.y+63u)/64u);
    return tiles64<=PTEmissiveCandidateCapacity?64u:128u;
}

uint PTEmissiveTileCount(out uint tilesX,out uint tilesY)
{
    uint tileSize=PTEmissiveTileSize();
    tilesX=(OutputSize.x+tileSize-1u)/tileSize;
    tilesY=(OutputSize.y+tileSize-1u)/tileSize;
    return min(tilesX*tilesY,PTEmissiveCandidateCapacity);
}

void DiscoverEmissiveCandidate(uint2 pixel)
{
    // Half-rate discovery keeps the pass cheap while retaining thin fixtures.
    if((pixel.x&1u)!=0u||(pixel.y&1u)!=0u)return;
    uint materialID=0u;
    float3 emission=MaterialEmissionAtPixel(pixel,materialID);
    float emissionL=Luminance(emission);
    if(emissionL<=0.0025)return;

    uint tilesX=0u,tilesY=0u;
    uint tileCount=PTEmissiveTileCount(tilesX,tilesY);
    uint tileSize=PTEmissiveTileSize();
    uint2 tile=pixel/tileSize;
    uint tileIndex=tile.y*tilesX+tile.x;
    if(tileIndex>=tileCount)return;

    uint score=max(1u,min(255u,(uint)(saturate(log2(1.0+emissionL)/6.0)*255.0+0.5)));
    uint px=min(pixel.x,4095u),py=min(pixel.y,4095u);
    uint packed=(score<<24u)|((py&0xFFFu)<<12u)|(px&0xFFFu);
    uint previous=0u;
    InterlockedMax(EmissiveCandidates[tileIndex],packed,previous);
    InterlockedAdd(Counters[12],1u);
    if(packed>previous)InterlockedAdd(Counters[13],1u);
}

uint2 DecodeEmissivePixel(uint packed)
{
    return uint2(packed&0xFFFu,(packed>>12u)&0xFFFu);
}

float EmissiveCandidateMerit(uint packed,uint2 receiverPixel)
{
    if(packed==0u)return 0.0;
    uint score=(packed>>24u)&0xFFu;
    uint2 p=DecodeEmissivePixel(packed);
    float2 delta=(float2(p)-float2(receiverPixel))/max(float2(OutputSize),1.0);
    float screenDistance=length(delta);
    return (score/255.0)*rcp(0.20+screenDistance*5.0);
}

bool FindEmissiveCandidate(uint2 receiverPixel,uint pixelIndex,out uint2 candidatePixel,out float candidateMerit)
{
    candidatePixel=uint2(0u,0u);candidateMerit=0.0;
    uint tilesX=0u,tilesY=0u;
    uint tileCount=PTEmissiveTileCount(tilesX,tilesY);
    if(tileCount==0u)return false;
    uint tileSize=PTEmissiveTileSize();
    int2 center=int2(receiverPixel/tileSize);

    [loop] for(int oy=-2;oy<=2;++oy){
        [loop] for(int ox=-2;ox<=2;++ox){
            int2 t=center+int2(ox,oy);
            if(t.x<0||t.y<0||t.x>=(int)tilesX||t.y>=(int)tilesY)continue;
            uint index=(uint)t.y*tilesX+(uint)t.x;
            if(index>=tileCount)continue;
            uint packed=EmissiveCandidates[index];
            float merit=EmissiveCandidateMerit(packed,receiverPixel);
            if(merit>candidateMerit){candidateMerit=merit;candidatePixel=DecodeEmissivePixel(packed);}
        }
    }

    // Four deterministic long-range probes keep distant signs/panels eligible without
    // turning candidate lookup into a large per-pixel buffer walk.
    [unroll] for(uint i=0u;i<4u;++i){
        uint index=Hash32(pixelIndex^(0x9E3779B9u*(i+1u)))%tileCount;
        uint packed=EmissiveCandidates[index];
        float merit=EmissiveCandidateMerit(packed,receiverPixel)*0.72;
        if(merit>candidateMerit){candidateMerit=merit;candidatePixel=DecodeEmissivePixel(packed);}
    }
    return candidateMerit>0.0&&candidatePixel.x<OutputSize.x&&candidatePixel.y<OutputSize.y;
}

float3 WorldRayForPixel(uint2 pixel)
{
    float2 uv=(float2(pixel)+0.5)/float2(OutputSize);
    float2 ndc=float2(uv.x*2.0-1.0,1.0-uv.y*2.0);
    float3 p0=ClipPointToWorld(ndc,0.0);
    float3 p1=ClipPointToWorld(ndc,1.0);
    float3 camera=CameraPosTMin.xyz;
    float3 farPoint=dot(p1-camera,p1-camera)>dot(p0-camera,p0-camera)?p1:p0;
    return normalize(farPoint-camera);
}

float TraceVisibility(float3 origin,float3 normal,float3 dir,float epsilon,float maxDistance);

float3 SampleEmissiveBounce(float3 hitPos,float3 normal,float3 receiverColor,uint2 receiverPixel,uint pixelIndex)
{
    uint2 candidatePixel;float merit;
    if(!FindEmissiveCandidate(receiverPixel,pixelIndex,candidatePixel,merit))return 0.0.xxx;

    uint materialID=0u;
    float3 emission=MaterialEmissionAtPixel(candidatePixel,materialID);
    if(Luminance(emission)<=0.0025)return 0.0.xxx;

    float3 camera=CameraPosTMin.xyz;
    float3 emitterRay=WorldRayForPixel(candidatePixel);
    float emitterT;uint emitterInstance;
    if(!Trace(camera,emitterRay,max(CameraPosTMin.w,0.001),100000.0,emitterT,emitterInstance))return 0.0.xxx;
    float3 emitterPos=camera+emitterRay*emitterT;
    float3 toEmitter=emitterPos-hitPos;
    float distanceToEmitter=length(toEmitter);
    if(distanceToEmitter<0.08||distanceToEmitter>30.0)return 0.0.xxx;
    float3 L=toEmitter/distanceToEmitter;
    float ndotl=saturate(dot(normal,L));
    if(ndotl<=0.0005)return 0.0.xxx;

    float4 emitterG1=GBuffer1.Load(int3(candidatePixel,0));
    float3 emitterN=ControlNormalViewToWorld(ControlDecodeNormalView(emitterG1),ViewToWorldBasisX.xyz,ViewToWorldBasisY.xyz,ViewToWorldBasisZ.xyz);
    float emitterFacing=max(0.22,abs(dot(emitterN,-L)));
    float visibility=TraceVisibility(hitPos,normal,L,0.008,max(distanceToEmitter-0.035,0.01));
    if(visibility<=0.0)return 0.0.xxx;

    // One discovered pixel represents a finite patch of visible emitter area.
    // Scale its footprint with camera distance, then use inverse-square falloff.
    float patchArea=clamp(0.20+emitterT*emitterT*0.015,0.20,2.0);
    float attenuation=patchArea/max(distanceToEmitter*distanceToEmitter,0.30);
    float3 receiverReflectance=lerp(0.38.xxx,clamp(receiverColor,0.05,1.0),0.62);
    float3 bounce=emission*receiverReflectance*(ndotl*emitterFacing*visibility*attenuation*0.75);
    return clamp(bounce,0.0,HdrActive!=0u?8.0:4.0);
}


float2 LowDiscrepancy2D(uint pixelIndex,uint frameIndex,uint sampleIndex,uint lobe)
{
    uint scramble=Hash32(pixelIndex ^ (lobe*0x9e3779b9u));
    float rotX=(scramble&0xffffu)/65536.0;
    float rotY=((scramble>>16u)&0xffffu)/65536.0;
    float k=(float)(frameIndex*4u+sampleIndex)+0.5;
    return frac(float2(k*0.7548776662466927+rotX,k*0.5698402909980532+rotY));
}

float3 CosineHemisphereUV(float3 n,float2 xi)
{
    float r=sqrt(saturate(xi.x));
    float phi=6.28318530718*xi.y;
    float3 t,b;BasisFromNormal(n,t,b);
    return normalize(t*(r*cos(phi))+b*(r*sin(phi))+n*sqrt(max(0.0,1.0-xi.x)));
}

float3 RoughReflectionUV(float3 incident,float3 n,float roughness,float2 xi)
{
    float3 r=normalize(reflect(incident,n));
    float3 jitter=CosineHemisphereUV(r,xi);
    float a=saturate(roughness*roughness);
    float3 d=normalize(lerp(r,jitter,a));
    if(dot(d,n)<=0.001)d=normalize(r+n*0.05);
    return d;
}

float3 ViewVectorToWorld(float3 v)
{
    return normalize(ViewToWorldBasisX.xyz*v.x + ViewToWorldBasisY.xyz*v.y + ViewToWorldBasisZ.xyz*v.z);
}

float3 ViewPointToWorld(float3 v)
{
    return CameraPosTMin.xyz + ViewToWorldBasisX.xyz*v.x + ViewToWorldBasisY.xyz*v.y + ViewToWorldBasisZ.xyz*v.z;
}

float3 JitterLightPosition(float3 center,float radius,float2 xi,float3 toCenter)
{
    float3 n=normalize(toCenter);float3 t,b;BasisFromNormal(n,t,b);
    float r=sqrt(saturate(xi.x))*radius;float a=6.28318530718*xi.y;
    return center+t*(cos(a)*r)+b*(sin(a)*r);
}

float TraceVisibility(float3 origin,float3 normal,float3 dir,float epsilon,float maxDistance)
{
    float t; uint instanceId;
    bool blocked=Trace(origin+normal*epsilon,dir,epsilon,maxDistance,t,instanceId);
    return blocked?0.0:1.0;
}

float NativeDistanceAttenuation(float distanceToLight,float clipRange,float4 falloff)
{
    float range=max(abs(clipRange),0.25);
    float normalized=saturate(1.0-distanceToLight/range);
    float rangeFade=normalized*normalized;
    float denom=max(abs(falloff.x)+abs(falloff.y)*distanceToLight+abs(falloff.z)*distanceToLight*distanceToLight,0.20);
    float polynomial=rcp(denom);
    return clamp(rangeFade*polynomial,0.0,4.0);
}

float3 NativeDiffuseAnchor(uint2 pixel)
{
    uint w=0,h=0;NativeLightDiffuse.GetDimensions(w,h);
    if(w==0u||h==0u)return 0.0.xxx;
    int2 p=int2(min(pixel,uint2(w-1u,h-1u)));
    int2 dx=int2(4,0),dy=int2(0,4);
    int2 maxP=int2(w-1u,h-1u);
    float3 c=max(NativeLightDiffuse.Load(int3(p,0)).rgb,0.0)*0.36;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p+dx,int2(0,0),maxP),0)).rgb,0.0)*0.10;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p-dx,int2(0,0),maxP),0)).rgb,0.0)*0.10;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p+dy,int2(0,0),maxP),0)).rgb,0.0)*0.10;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p-dy,int2(0,0),maxP),0)).rgb,0.0)*0.10;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p+dx+dy,int2(0,0),maxP),0)).rgb,0.0)*0.06;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p+dx-dy,int2(0,0),maxP),0)).rgb,0.0)*0.06;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p-dx+dy,int2(0,0),maxP),0)).rgb,0.0)*0.06;
    c+=max(NativeLightDiffuse.Load(int3(clamp(p-dx-dy,int2(0,0),maxP),0)).rgb,0.0)*0.06;
    // Control's deferred light buffer is in the pre-exposed lighting domain.
    // Bring only its low-frequency energy back to scene-linear for the PT reference path.
    return clamp(c*PTPreExposure(),0.0,16.0);
}

float3 PreviousSceneLowFrequencyAnchor(uint2 pixel)
{
    uint w=0,h=0;PreviousScene.GetDimensions(w,h);
    if(w==0u||h==0u)return 0.18.xxx;
    float2 uv=(float2(pixel)+0.5)/float2(OutputSize);
    int2 center=int2(min((uint2)(uv*float2(w,h)),uint2(w-1u,h-1u)));
    int radius=max(12,(int)(max(w,h)*0.018));
    int2 maxP=int2(w-1u,h-1u);
    int2 ox=int2(radius,0), oy=int2(0,radius);
    int2 ox2=int2(radius*2,0), oy2=int2(0,radius*2);
    float3 c=0.0.xxx;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(center,0)).rgb,0.0))*0.20;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center+ox,int2(0,0),maxP),0)).rgb,0.0))*0.10;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center-ox,int2(0,0),maxP),0)).rgb,0.0))*0.10;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center+oy,int2(0,0),maxP),0)).rgb,0.0))*0.10;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center-oy,int2(0,0),maxP),0)).rgb,0.0))*0.10;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center+ox+oy,int2(0,0),maxP),0)).rgb,0.0))*0.075;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center+ox-oy,int2(0,0),maxP),0)).rgb,0.0))*0.075;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center-ox+oy,int2(0,0),maxP),0)).rgb,0.0))*0.075;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center-ox-oy,int2(0,0),maxP),0)).rgb,0.0))*0.075;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center+ox2,int2(0,0),maxP),0)).rgb,0.0))*0.025;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center-ox2,int2(0,0),maxP),0)).rgb,0.0))*0.025;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center+oy2,int2(0,0),maxP),0)).rgb,0.0))*0.025;
    c+=DisplayToSceneLinear(max(PreviousScene.Load(int3(clamp(center-oy2,int2(0,0),maxP),0)).rgb,0.0))*0.025;
    // Compress contrast so native RT shadows/noise do not survive as the lighting solution.
    float l=max(Luminance(c),1.0e-4);
    float3 chroma=c/l;
    chroma=clamp(chroma,0.55,1.85);
    chroma/=max(Luminance(chroma),0.10);
    float stabilized=sqrt(saturate(l))*0.78+0.06;
    return clamp(chroma*stabilized,0.025,2.25);
}

float3 NativeDirectLighting(float3 hitPos,float3 normal,uint pixelIndex,out float visibilityRatio)
{
    float3 shadowed=0.0.xxx;
    float3 unshadowed=0.0.xxx;
    uint pointElems=0,pointStride=0;PointLightsRaw.GetDimensions(pointElems,pointStride);
    uint pointCount=min(pointElems/10u,16u);
    // PT25: expand native point-light coverage so more local lights participate in the shadow/visibility solve.
    [loop] for(uint i=0u;i<pointCount;++i){
        uint base=i*10u;
        float4 p0=PointLightsRaw[base+0u];
        float4 p1=PointLightsRaw[base+1u];
        float4 p2=PointLightsRaw[base+2u];
        float4 p9=PointLightsRaw[base+9u];
        float3 center=ViewPointToWorld(p0.xyz);
        float3 toCenter=center-hitPos;float dist=max(length(toCenter),0.001);
        float3 L=toCenter/dist;float ndotl=saturate(dot(normal,L));
        if(ndotl<=0.0001)continue;
        float atten=NativeDistanceAttenuation(dist,p1.w,p2);
        if(atten<=0.0001)continue;
        float radius=clamp(abs(p9.x),0.012,2.0);
        float2 xi=LowDiscrepancy2D(pixelIndex,FrameIndex,i,91u+i);
        float3 samplePos=JitterLightPosition(center,radius,xi,toCenter);
        float3 toSample=samplePos-hitPos;float sampleDist=max(length(toSample),0.001);
        float localTMax=min(max(sampleDist-0.012,0.01),PTMaxShadowDistanceMeters());
        float vis=TraceVisibility(hitPos,normal,toSample/sampleDist,0.008,localTMax);
        float3 color=max(p1.xyz,0.0);
        float3 contribution=color*(ndotl*atten);
        unshadowed+=contribution;
        shadowed+=contribution*vis;
    }

    uint spotElems=0,spotStride=0;SpotLightsRaw.GetDimensions(spotElems,spotStride);
    uint spotCount=min(spotElems/20u,32u);
    // PT25: expand spot-light coverage for broader scene-light participation.
    [loop] for(uint i=0u;i<spotCount;++i){
        uint base=i*20u;
        float4 s0=SpotLightsRaw[base+0u];
        float4 s1=SpotLightsRaw[base+1u];
        float4 s2=SpotLightsRaw[base+2u];
        float4 s3=SpotLightsRaw[base+3u];
        float4 s4=SpotLightsRaw[base+4u];
        float3 center=ViewPointToWorld(s0.xyz);
        float3 dir=ViewVectorToWorld(s2.xyz);
        float3 delta=hitPos-center;float axis=dot(delta,dir);
        float nearDistance=max(s0.w,0.0);
        float farDistance=max(abs(s1.w),nearDistance+0.05);
        if(axis<nearDistance||axis>farDistance)continue;
        float3 deltaN=normalize(delta);
        float cosAxis=dot(deltaN,dir);
        float farWidth=max(abs(s2.w),0.01);
        float tanHalf=farWidth/max(farDistance,0.05);
        float cosEdge=rsqrt(1.0+tanHalf*tanHalf);
        float cone=smoothstep(cosEdge,min(1.0,cosEdge+0.06),cosAxis);
        if(cone<=0.0005)continue;
        float3 toCenter=center-hitPos;float dist=max(length(toCenter),0.001);
        float3 L=toCenter/dist;float ndotl=saturate(dot(normal,L));
        if(ndotl<=0.0001)continue;
        float4 spotFalloff=float4(s4.x,s4.y,0.0,0.0);
        float atten=NativeDistanceAttenuation(dist,s3.w,spotFalloff);
        if(atten<=0.0001)continue;
        float radius=clamp(abs(s4.z),0.008,1.5);
        float2 xi=LowDiscrepancy2D(pixelIndex,FrameIndex,i,127u+i);
        float3 samplePos=JitterLightPosition(center,radius,xi,toCenter);
        float3 toSample=samplePos-hitPos;float sampleDist=max(length(toSample),0.001);
        float localTMax=min(max(sampleDist-0.012,0.01),PTMaxShadowDistanceMeters());
        float vis=TraceVisibility(hitPos,normal,toSample/sampleDist,0.008,localTMax);
        float3 color=max(s3.xyz,0.0);
        float3 contribution=color*(ndotl*atten*cone);
        unshadowed+=contribution;
        shadowed+=contribution*vis;
    }
    float rawUnshadowedL=Luminance(unshadowed);
    if(rawUnshadowedL<1.0e-4){visibilityRatio=1.0;return 0.0.xxx;}
    float unshadowedL=max(rawUnshadowedL,1.0e-4);
    visibilityRatio=clamp(Luminance(shadowed)/unshadowedL,0.18,1.0);
    return clamp(shadowed,0.0,16.0);
}

float3 BuildShadowDir(float3 baseDir,float2 xi,float cone)
{
    float angle=6.2831853*xi.x;
    float radius=cone*sqrt(xi.y);
    float3 tangent=normalize(abs(baseDir.y)<0.99?cross(float3(0,1,0),baseDir):cross(float3(1,0,0),baseDir));
    float3 bitangent=normalize(cross(baseDir,tangent));
    float3 jitter=cos(angle)*tangent*radius + sin(angle)*bitangent*radius;
    return normalize(baseDir + jitter);
}

float SoftShadowVisibility(float3 hitPos,float3 normal,float3 baseDir,float cone,float maxDistance,uint pixelIndex,uint sampleSeed)
{
    float visibility=0.0;
    const uint shadowSamples=20u;
    // PT25: slightly higher soft-shadow sample count for better stability at the source.
    [unroll] for(uint si=0u; si<shadowSamples; ++si){
        float2 xi=LowDiscrepancy2D(pixelIndex,FrameIndex,sampleSeed+si,23u+si);
        float3 dir=BuildShadowDir(baseDir,xi,cone);
        visibility+=TraceVisibility(hitPos,normal,dir,0.012,maxDistance);
    }
    return visibility/20.0;
}

float ContactOcclusion(float3 hitPos,float3 normal,uint pixelIndex,uint sampleSeed)
{
    float occlusion=0.0;
    const uint contactSamples=16u;
    // PT25: raise contact sample count to calm fine shadow boiling on characters and props.
    [unroll] for(uint si=0u; si<contactSamples; ++si){
        float2 xi=LowDiscrepancy2D(pixelIndex,FrameIndex,sampleSeed+si,51u+si);
        float3 dir=CosineHemisphereUV(normal,xi);
        float maxDistance=lerp(0.55,1.85,xi.x);
        float t; uint instanceId;
        bool hit=Trace(hitPos+normal*0.008,dir,0.008,maxDistance,t,instanceId);
        if(hit){
            float nearWeight=1.0-saturate(t/maxDistance);
            occlusion+=nearWeight*nearWeight;
        }
    }
    return saturate(occlusion/16.0);
}

float ContactOcclusionRadius(float3 hitPos,float3 normal,uint pixelIndex,float radiusMeters)
{
    float occlusion=0.0;
    const uint contactSamples=16u;
    // PT25: raise contact sample count to calm fine shadow boiling on characters and props.
    [unroll] for(uint si=0u;si<contactSamples;++si){
        float2 xi=LowDiscrepancy2D(pixelIndex,FrameIndex,97u+si,151u+si);
        float3 dir=CosineHemisphereUV(normal,xi);
        float maxDistance=max(0.03,radiusMeters*lerp(0.35,1.0,xi.x));
        float t;uint instanceId;
        bool hit=Trace(hitPos+normal*0.006,dir,0.006,maxDistance,t,instanceId);
        if(hit){float nearWeight=1.0-saturate(t/maxDistance);occlusion+=nearWeight*nearWeight;}
    }
    return saturate(occlusion/16.0);
}

float DirectShadowTerm(float3 hitPos,float3 normal,float roughness,uint pixelIndex)
{
    // PT19: replace directional "contact" rays with true short-range
    // hemisphere occlusion. The broad source remains a large area sample, so
    // standalone gets visibly softer penumbrae plus much stronger local
    // contact around characters, feet, props and wall/floor junctions.
    float3 keyDir=normalize(float3(0.16,0.982,0.085));
    float nKey=saturate(dot(normal,keyDir));
    float cone=lerp(0.070,0.135,saturate(roughness*0.55 + (1.0-normal.y)*0.45));
    float broad=SoftShadowVisibility(hitPos,normal,keyDir,cone,18.0,pixelIndex,31u);
    float contact=ContactOcclusion(hitPos,normal,pixelIndex,79u);
    float contactVisibility=1.0-contact*0.82;
    float facing=lerp(0.78,1.0,nKey);
    float shaded=broad*contactVisibility*facing;
    float ambientFloor=lerp(0.16,0.24,saturate(roughness));
    return clamp(lerp(ambientFloor,1.0,shaded),ambientFloor,1.0);
}

// PreviousSceneRadiance compatibility marker: PT14 supersedes it with validated PreviousSceneSurface reprojection.
bool PreviousSceneSurface(float3 hitPos,
                          out float3 radiance,
                          out float3 surfaceNormal,
                          out float3 surfaceColor,
                          out float surfaceRoughness)
{
    radiance=1.0.xxx;
    surfaceNormal=0.0.xxx;
    surfaceColor=0.5.xxx;
    surfaceRoughness=0.5;
    if((Spp&0x80000000u)==0u||HistoryValid==0u)return false;

    float4 prevClip=mul(float4(hitPos,1.0),PrevWorldToClip);
    if(prevClip.w<=1.0e-5)return false;
    float2 ndc=prevClip.xy/prevClip.w;
    float2 uv=float2(ndc.x*0.5+0.5,0.5-ndc.y*0.5);
    if(any(uv<0.0)||any(uv>=1.0))return false;

    uint2 hp=min((uint2)(uv*float2(OutputSize)),OutputSize-1u);
    float4 history=HistoryLighting.Load(int3(hp,0));
    float4 meta=HistoryMeta.Load(int3(hp,0));
    float4 surface=HistorySurface.Load(int3(hp,0));
    float expected=length(hitPos-PrevCameraPos.xyz);
    float tolerance=max(0.20,expected*0.06);
    if(history.a<=0.0||meta.w<0.5||abs(history.a-expected)>tolerance)return false;

    float n2=dot(meta.xyz,meta.xyz);
    if(n2<0.25)return false;
    surfaceNormal=normalize(meta.xyz);
    surfaceColor=clamp(surface.rgb,0.025,1.0);
    surfaceRoughness=saturate(surface.a);

    uint sw,sh;PreviousScene.GetDimensions(sw,sh);
    if(sw==0u||sh==0u)return false;
    uint2 sp=min((uint2)(uv*float2(sw,sh)),uint2(sw-1u,sh-1u));
    float3 encoded=max(PreviousScene.Load(int3(sp,0)).rgb,0.0);
    float3 sceneLinear=DisplayToSceneLinear(encoded);
    float l=max(Luminance(sceneLinear),1.0e-4);

    if(PTStandalone()){
        // PT15: use only the authored frame's scalar light energy at the
        // reprojected hit. Do not recycle its RGB lighting and do not recurse
        // the previous PT result back into itself. Material chroma comes from
        // the validated PT surface cache, which avoids the dark feedback loop
        // that made standalone collapse toward gray/black over time.
        float3 materialChroma=surfaceColor/max(Luminance(surfaceColor),0.06);
        materialChroma=clamp(materialChroma,0.42,2.35);
        materialChroma/=max(Luminance(materialChroma),0.12);
        float energy=HdrActive!=0u ? 0.14+1.16*saturate(log2(1.0+l)*0.62)
                                  : 0.16+1.28*sqrt(saturate(l));
        radiance=clamp(materialChroma*energy,0.035,HdrActive!=0u?6.0:2.5);
        return true;
    }

    // Hybrid keeps the native frame only as a bounded secondary-radiance cache.
    float3 chroma=sceneLinear/l;
    chroma/=max(Luminance(chroma),1.0e-4);
    chroma=clamp(chroma,0.72,1.32);
    chroma=lerp(1.0.xxx,chroma,0.18);
    float energy=HdrActive!=0u ? 0.86+0.26*saturate(log2(1.0+l)*0.55)
                              : 0.86+0.26*saturate(sqrt(l));
    radiance=clamp(chroma*energy,0.62,1.42);
    return true;
}

float3 TonemapSDR(float3 c)
{
    c=max(c,0.0);
    c=(c*(2.51*c+0.03))/(c*(2.43*c+0.59)+0.14);
    return pow(saturate(c),1.0/2.2);
}

bool TemporalResolve(float3 hitPos, float3 normal, float roughness, float3 current,
                     out float3 resolved, out float nextCount)
{
    resolved=current;
    nextCount=1.0;
    if(HistoryValid==0u || MaxHistory<2u)return false;

    float4 prevClip=mul(float4(hitPos,1.0),PrevWorldToClip);
    if(prevClip.w<=1.0e-5)return false;
    float2 prevNdc=prevClip.xy/prevClip.w;
    float2 prevUv=float2(prevNdc.x*0.5+0.5,0.5-prevNdc.y*0.5);
    if(any(prevUv<0.0)||any(prevUv>=1.0))return false;

    uint2 prevPixel=min((uint2)(prevUv*float2(OutputSize)),OutputSize-1u);
    float4 history=HistoryLighting.Load(int3(prevPixel,0));
    float4 meta=HistoryMeta.Load(int3(prevPixel,0));
    if(history.a<=0.0 || meta.w<0.5)return false;

    float expectedPrevT=length(hitPos-PrevCameraPos.xyz);
    float depthTolerance=max(0.065,expectedPrevT*0.022);
    if(abs(history.a-expectedPrevT)>depthTolerance)return false;

    float3 previousNormal=normalize(meta.xyz);
    if(dot(previousNormal,normal)<0.90)return false;

    float previousCount=min(meta.w,(float)MaxHistory);
    float historyWeight=min(previousCount/(previousCount+1.0),PTStandalone()?0.982:0.975);

    // PT26 dedicated shadow denoise: stronger history for stable visibility/contact,
    // with aggressive rejection when reprojection disagrees with the current frame.
    // This signal is composited after native RR, so RR itself does not denoise it.
    // Keep history in the PT signal before final composite.
    //
    // recovered signal diverges from the current sample to avoid obvious lag/ghosting.
    float clampRadius=PTStandalone()?lerp(0.060,0.110,saturate(roughness)):lerp(0.055,0.095,saturate(roughness));
    float historyCeiling=PTStandalone()?(HdrActive!=0u?10.0:4.5):1.60;
    float3 stableHistory=PTStandalone()?clamp(history.rgb,0.0,historyCeiling):clamp(history.rgb,0.50,1.50);
    float3 historyClamped=clamp(stableHistory,current-clampRadius,current+clampRadius);
    float signalDelta=max(max(abs(historyClamped.r-current.r),abs(historyClamped.g-current.g)),abs(historyClamped.b-current.b));
    float luminanceDelta=abs(Luminance(historyClamped)-Luminance(current));
    float rejection=saturate(max(signalDelta*7.0,luminanceDelta*3.5)-0.06);
    historyWeight*=1.0-0.78*rejection;
    resolved=lerp(current,historyClamped,historyWeight);
    nextCount=min(previousCount+1.0,(float)MaxHistory);
    return true;
}

[numthreads(8,8,1)]
// PT21 probe marker: standalone reference branch evidence capture in one pass
// PT22 R5 marker: previous-scene low-frequency exposure/chroma anchor + PT visibility
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    if(dispatchId.x>=OutputSize.x||dispatchId.y>=OutputSize.y)return;

    if(PTEmissiveDiscoveryPass()){DiscoverEmissiveCandidate(dispatchId.xy);return;}
    EmissiveOutput[dispatchId.xy]=0.0.xxxx;

    const uint viewMode=PTViewMode();
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

    if(!primaryHit){
        float3 missValue=1.0.xxx;
        if(viewMode==4u){
            missValue=0.0.xxx;
        }else if(viewMode==3u){
            float3 sky=SkyRadiance(primaryDir);
            missValue=HdrActive!=0u?clamp(sky,0.0,8.0):max(sky,0.0);
        }else if(viewMode==2u && PTStandalone()){
            float3 sky=SkyRadiance(primaryDir);
            missValue=HdrActive!=0u?clamp(sky,0.0,8.0):max(sky,0.0);
        }
        Output[dispatchId.xy]=float4(missValue,1.0);
        HistoryLightingOut[dispatchId.xy]=float4(missValue,0.0);
        HistoryMetaOut[dispatchId.xy]=float4(0.0,0.0,0.0,1.0);
        HistorySurfaceOut[dispatchId.xy]=0.0.xxxx;
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
    float3 baseColor=MaterialColor(g2,f0,brdfMode);
    const float3 hitPos=camera+primaryDir*primaryT;
    const float epsilon=max(0.003,primaryT*0.00015);
    uint pixelIndex=dispatchId.y*OutputSize.x+dispatchId.x;

    if(viewMode==4u){
        uint currentMaterial=0u;
        float3 currentEmission=MaterialEmissionAtPixel(dispatchId.xy,currentMaterial);
        float analyticVisibility=1.0;
        float3 analytic=NativeDirectLighting(hitPos,n,pixelIndex,analyticVisibility);
        bool hasEmissive=Luminance(currentEmission)>0.0025;
        bool hasAnalytic=Luminance(analytic)>0.0025;
        float3 debugColor=float3(hasAnalytic?1.0:0.0,hasEmissive?1.0:0.0,0.0);
        Output[dispatchId.xy]=float4(debugColor,1.0);
        HistoryLightingOut[dispatchId.xy]=float4(debugColor,primaryT);
        HistoryMetaOut[dispatchId.xy]=float4(n,1.0);
        HistorySurfaceOut[dispatchId.xy]=float4(baseColor,roughness);
        return;
    }

    if(viewMode<=2u){
        float shadowVisibility=1.0;
        NativeDirectLighting(hitPos,n,pixelIndex,shadowVisibility);
        float contactOcclusion=ContactOcclusionRadius(hitPos,n,pixelIndex,PTContactRadiusMeters());
        float occlusionVisibility=1.0-contactOcclusion;
        float shadowDeficit=saturate(1.0-shadowVisibility);
        float contactDeficit=saturate(1.0-occlusionVisibility);
        float combinedDeficit=saturate(contactDeficit*0.78+shadowDeficit*0.22);
        float combinedVisibility=1.0-combinedDeficit;
        float3 emissiveBounce=viewMode==0u?SampleEmissiveBounce(hitPos,n,baseColor,dispatchId.xy,pixelIndex):0.0.xxx;
        float3 emissiveComposite=HdrActive!=0u?clamp(emissiveBounce,0.0,8.0):TonemapSDR(emissiveBounce);
        EmissiveOutput[dispatchId.xy]=float4(emissiveComposite,1.0);
        float3 currentSignals=float3(shadowVisibility,occlusionVisibility,combinedVisibility);
        float3 resolvedSignals=currentSignals;float historyCount=1.0;
        bool historyAccepted=TemporalResolve(hitPos,n,roughness,currentSignals,resolvedSignals,historyCount);
        if(sampled&&historyAccepted)InterlockedAdd(Counters[11],1u);
        HistoryLightingOut[dispatchId.xy]=float4(resolvedSignals,primaryT);
        HistoryMetaOut[dispatchId.xy]=float4(n,historyCount);
        HistorySurfaceOut[dispatchId.xy]=float4(baseColor,roughness);
        Output[dispatchId.xy]=float4(resolvedSignals,1.0);
        return;
    }

    if(viewMode==3u){
        Output[dispatchId.xy]=float4(1.0.xxx,1.0);
        HistoryLightingOut[dispatchId.xy]=float4(1.0.xxx,0.0);
        HistoryMetaOut[dispatchId.xy]=0.0.xxxx;
        HistorySurfaceOut[dispatchId.xy]=0.0.xxxx;
        return;
    }

    if(viewMode==1u){
        // VIS: isolate PT visibility against the captured native point/spot lights.
        float visibility=1.0;
        NativeDirectLighting(hitPos,n,pixelIndex,visibility);
        Output[dispatchId.xy]=float4(visibility.xxx,1.0);
        HistoryLightingOut[dispatchId.xy]=float4(visibility.xxx,primaryT);
        HistoryMetaOut[dispatchId.xy]=float4(n,1.0);
        HistorySurfaceOut[dispatchId.xy]=float4(baseColor,roughness);
        return;
    }

    if(viewMode==3u){
        // DIR: direct native-light decode + PT visibility with no native-frame anchor.
        float visibility=1.0;
        float3 nativeDirect=NativeDirectLighting(hitPos,n,pixelIndex,visibility);
        float ndotvDirect=saturate(dot(n,-primaryDir));
        float3 fresnelDirect=f0+(1.0-f0)*pow(1.0-ndotvDirect,5.0);
        float3 directOnly=baseColor*(nativeDirect+0.025.xxx);
        directOnly+=nativeDirect*fresnelDirect*(0.28*lerp(1.0,0.24,roughness));
        directOnly=HdrActive!=0u?clamp(directOnly,0.0,16.0):clamp(directOnly,0.0,4.0);
        Output[dispatchId.xy]=float4(directOnly,1.0);
        HistoryLightingOut[dispatchId.xy]=float4(directOnly,primaryT);
        HistoryMetaOut[dispatchId.xy]=float4(n,1.0);
        HistorySurfaceOut[dispatchId.xy]=float4(baseColor,roughness);
        return;
    }

    // LF mode continues through the current standalone path-lighting experiment.
    // PT14 keeps the current 2D+2S / 8D+8S budget per selected bounce,
    // but removes PT13's fake secondary normals and repeated radiance injection.
    // A deeper bounce continues only when the hit reprojects to a validated
    // previous-frame PT surface, which supplies the real world normal plus
    // primary material color/roughness. Radiance is consumed once, at the
    // terminal hit, instead of once per bounce.
    uint sampleCount=max(PTPathSamples(),1u);
    uint bounceCount=max(PTBounceCount(),1u);

    float3 diffuseRadiance=0.0.xxx;
    [loop] for(uint sampleIndex=0u;sampleIndex<sampleCount;++sampleIndex){
        float3 throughput=1.0.xxx;
        float3 pathAccum=0.0.xxx;
        float3 bounceOrigin=hitPos;
        float3 bounceNormal=n;
        float3 bounceDir=CosineHemisphereUV(n,LowDiscrepancy2D(pixelIndex,FrameIndex,sampleIndex,0u));
        [loop] for(uint bounceIndex=0u;bounceIndex<bounceCount;++bounceIndex){
            float diffuseT;uint diffuseInstance;
            bool diffuseHit=Trace(bounceOrigin+bounceNormal*epsilon,bounceDir,epsilon,250.0,diffuseT,diffuseInstance);
            if(sampled)InterlockedAdd(Counters[diffuseHit?7:8],1u);
            if(!diffuseHit){
                float3 sky=SkyRadiance(bounceDir);
                float3 terminal=0.0.xxx;
                if(PTStandalone())terminal=HdrActive!=0u?clamp(sky,0.0,8.0):max(sky,0.0);
                else{
                    float skyL=max(Luminance(sky),0.001);
                    terminal=clamp((sky/skyL)*1.06,0.68,1.42);
                }
                pathAccum+=throughput*terminal;
                break;
            }

            float3 bounceHit=bounceOrigin+bounceDir*diffuseT;
            float3 cachedRadiance,cachedNormal,cachedColor;
            float cachedRoughness;
            bool cached=PreviousSceneSurface(bounceHit,cachedRadiance,cachedNormal,cachedColor,cachedRoughness);
            if(!cached){
                // No trustworthy hit attributes means no trustworthy next bounce.
                // Terminate conservatively instead of inventing a surface normal.
                float3 fallback=PTStandalone()?0.035.xxx:lerp(0.84.xxx,1.02.xxx,saturate(diffuseT/42.0));
                pathAccum+=throughput*fallback;
                break;
            }

            if((bounceIndex+1u)>=bounceCount){
                pathAccum+=throughput*cachedRadiance;
                break;
            }

            float3 orientedNormal=dot(cachedNormal,-bounceDir)>=0.0?cachedNormal:-cachedNormal;
            // Conservative Lambert-like continuation. The cached surface color
            // is authored material data from the previous primary hit, not the
            // current primary material repeated at every bounce.
            float3 bounceAlbedo=lerp(0.32.xxx,cachedColor,0.62);
            throughput*=bounceAlbedo*0.66;
            if(max(throughput.r,max(throughput.g,throughput.b))<0.018)break;
            bounceOrigin=bounceHit;
            bounceNormal=orientedNormal;
            float2 xiNext=LowDiscrepancy2D(pixelIndex,FrameIndex,sampleIndex+bounceIndex+1u,2u+bounceIndex);
            bounceDir=CosineHemisphereUV(bounceNormal,xiNext);
        }
        diffuseRadiance+=pathAccum;
    }
    diffuseRadiance/=max((float)sampleCount,1.0);

    float3 specRadiance=0.0.xxx;
    [loop] for(uint sampleIndex=0u;sampleIndex<sampleCount;++sampleIndex){
        float3 throughput=1.0.xxx;
        float3 pathAccum=0.0.xxx;
        float3 bounceOrigin=hitPos;
        float3 bounceNormal=n;
        float bounceRoughness=roughness;
        float3 bounceIncident=primaryDir;
        float3 bounceDir=RoughReflectionUV(primaryDir,n,roughness,LowDiscrepancy2D(pixelIndex,FrameIndex,sampleIndex,1u));
        [loop] for(uint bounceIndex=0u;bounceIndex<bounceCount;++bounceIndex){
            float specT;uint specInstance;
            bool specHit=Trace(bounceOrigin+bounceNormal*epsilon,bounceDir,epsilon,500.0,specT,specInstance);
            if(sampled)InterlockedAdd(Counters[specHit?9:10],1u);
            if(!specHit){
                float3 sky=SkyRadiance(bounceDir);
                float3 terminal=0.0.xxx;
                if(PTStandalone())terminal=HdrActive!=0u?clamp(sky*1.10,0.0,8.0):max(sky*1.10,0.0);
                else{
                    float skyL=max(Luminance(sky),0.001);
                    terminal=clamp((sky/skyL)*1.10,0.70,1.48);
                }
                pathAccum+=throughput*terminal;
                break;
            }

            float3 bounceHit=bounceOrigin+bounceDir*specT;
            float3 cachedRadiance,cachedNormal,cachedColor;
            float cachedRoughness;
            bool cached=PreviousSceneSurface(bounceHit,cachedRadiance,cachedNormal,cachedColor,cachedRoughness);
            if(!cached){
                float3 fallback=PTStandalone()?0.025.xxx:lerp(0.94.xxx,1.04.xxx,saturate(specT/72.0));
                pathAccum+=throughput*fallback;
                break;
            }

            if((bounceIndex+1u)>=bounceCount){
                pathAccum+=throughput*cachedRadiance;
                break;
            }

            float3 orientedNormal=dot(cachedNormal,-bounceDir)>=0.0?cachedNormal:-cachedNormal;
            float smoothness=1.0-saturate(cachedRoughness);
            float reflectivity=lerp(0.22,0.68,smoothness);
            float3 tint=lerp(1.0.xxx,cachedColor,0.28*smoothness);
            throughput*=tint*reflectivity;
            if(max(throughput.r,max(throughput.g,throughput.b))<0.015)break;
            bounceOrigin=bounceHit;
            bounceNormal=orientedNormal;
            bounceIncident=bounceDir;
            bounceRoughness=clamp(cachedRoughness,0.045,0.95);
            float2 xiNext=LowDiscrepancy2D(pixelIndex,FrameIndex,sampleIndex+bounceIndex+1u,18u+bounceIndex);
            bounceDir=RoughReflectionUV(bounceIncident,bounceNormal,bounceRoughness,xiNext);
        }
        specRadiance+=pathAccum;
    }
    specRadiance/=max((float)sampleCount,1.0);
    float ndotv=saturate(dot(n,-primaryDir));
    float3 fresnel=f0+(1.0-f0)*pow(1.0-ndotv,5.0);
    float specWeight=lerp(1.0,0.24,roughness);
    float upward=saturate(n.y*0.5+0.5);
    float shadowTerm=PTStandalone()?1.0:DirectShadowTerm(hitPos,n,roughness,pixelIndex);

    float3 multiplier=1.0.xxx;
    if(PTStandalone()){
        // PT22 R5: native deferred-light buffer proved to be the wrong color/energy domain.
        // Use the already-displayed previous native frame only as a VERY low-frequency
        // exposure/chroma anchor, while PT/native-light visibility controls the shadowing.
        float nativeVisibility=1.0;
        float3 nativeDirect=NativeDirectLighting(hitPos,n,pixelIndex,nativeVisibility);
        float3 nativeAnchor=PreviousSceneLowFrequencyAnchor(dispatchId.xy);
        float anchorL=clamp(Luminance(nativeAnchor),0.025,2.25);
        float3 anchorChroma=nativeAnchor/max(anchorL,0.025);
        anchorChroma=clamp(anchorChroma,0.60,1.75);
        anchorChroma/=max(Luminance(anchorChroma),0.10);
        float visibility=lerp(0.72,1.0,saturate(nativeVisibility));
        float directEnergy=anchorL*visibility;
        float ambientEnergy=clamp(anchorL*0.24+0.035,0.035,0.62);
        float3 directColor=anchorChroma*directEnergy;
        float3 diffuseTerm=baseColor*(directColor*0.78 + ambientEnergy.xxx*0.54 + diffuseRadiance*0.20 + 0.020.xxx);
        float3 specTerm=(directColor*0.54+specRadiance*0.46)*fresnel*(0.54*specWeight);
        multiplier=diffuseTerm+specTerm;
        if(brdfMode==3u)multiplier=diffuseTerm;
        multiplier*=lerp(0.99,1.035,upward);
        multiplier=HdrActive!=0u?clamp(multiplier,0.0,10.0):clamp(multiplier,0.0,4.5);
    }else{
        float directLift=lerp(0.92,1.05,shadowTerm);
        multiplier=lerp(1.0.xxx,diffuseRadiance,0.52);
        multiplier+=(specRadiance-1.0.xxx)*fresnel*(1.10*specWeight);
        multiplier*=directLift.xxx;
        multiplier*=lerp(0.99,1.03,upward);
        if(brdfMode==3u)multiplier=lerp(1.0.xxx,diffuseRadiance,0.50)*lerp(0.92,1.04,shadowTerm).xxx;
        float ml=dot(multiplier,float3(0.2126,0.7152,0.0722));
        multiplier+=(1.0-ml).xxx*0.20;
        multiplier=clamp(multiplier,0.76,1.30);
    }

    float3 accumulated;
    float historyCount;
    bool historyAccepted=TemporalResolve(hitPos,n,roughness,multiplier,accumulated,historyCount);
    if(sampled && historyAccepted)InterlockedAdd(Counters[11],1u);

    HistoryLightingOut[dispatchId.xy]=float4(accumulated,primaryT);
    HistoryMetaOut[dispatchId.xy]=float4(n,historyCount);
    HistorySurfaceOut[dispatchId.xy]=float4(baseColor,roughness);
    Output[dispatchId.xy]=float4(accumulated,1.0);
}
