#include "opaque_reflection_layout.hlsli"
cbuffer Sys : register(b0) { float4 sys[71]; }
cbuffer Reflection : register(b1) { float clampReflection; float extraRays; uint rayCount; float zeroReflectance; float maxReflectance; float maxRoughness; }
cbuffer OpaqueDispatch : register(b7) { uint epoch; uint capacity; uint2 extent; }
Texture2D<float4> clipDepth : register(t2);
struct Material {float d[84];};
StructuredBuffer<Material> materials : register(t3);
RaytracingAccelerationStructure scene : register(t5);
Texture2D<float4> materialTextures[] : register(t0,space1);
SamplerState linearWrap : register(s5,space1);
globallycoherent RWTexture2DArray<uint> nativeMaterial : register(u0);
globallycoherent RWTexture2DArray<float4> nativeNormal : register(u1);
globallycoherent RWTexture2DArray<float4> nativePosition : register(u2);
globallycoherent RWStructuredBuffer<OpaqueHit> extraHits : register(u4);
struct Payload {uint value;};
float3 World(float3 p){return p.x*sys[6].xyz+p.y*sys[7].xyz+p.z*sys[8].xyz+sys[9].xyz;}
[shader("raygeneration")]
void opaqueContinuationRayGeneration(){
    uint2 pixel=DispatchRaysIndex().xy;if(any(pixel>=extent))return;
    uint w,h,layers;nativeMaterial.GetDimensions(w,h,layers);
    uint nw,nh,nl;nativeNormal.GetDimensions(nw,nh,nl);
    uint pw,ph,pl;nativePosition.GetDimensions(pw,ph,pl);
    uint count,stride;materials.GetDimensions(count,stride);
    uint base=(pixel.y*extent.x+pixel.x)*capacity;
    OpaqueHit empty=(OpaqueHit)0;empty.material=65535u;
    for(uint layer=0;layer<capacity;++layer)extraHits[base+layer]=empty;
    if(w<extent.x||h<extent.y||nw!=w||nh!=h||pw!=w||ph!=h||nl!=layers||pl!=layers||capacity!=layers||stride!=336||rayCount>capacity)return;
    float2 xy=(float2(pixel)+0.5f)*sys[1].zw*float2(2,-2)+float2(-1,1);
    float z=clipDepth.Load(int3(pixel,0)).x;
    float4 primary=xy.x*sys[14]+xy.y*sys[15]+z*sys[16]+sys[17];
    float3 primaryWorld=World(primary.xyz/primary.w);
    for(uint layer=0;layer<rayCount;++layer){
        uint3 at=uint3(pixel,layer);uint first=nativeMaterial[at];
        if(first==65534u)break; // native end-of-samples sentinel
        if(first>=min(count,65534u))continue;
        float4 n=nativeNormal[at],p=nativePosition[at];
        if(!all(isfinite(n))||!all(isfinite(p))||dot(n.xyz,n.xyz)<0.5f)continue;
        float smooth=saturate(lerp(materials[first].d[17],materials[first].d[18],materialTextures[NonUniformResourceIndex(first*35u+2u)].SampleLevel(linearWrap,float2(n.w,p.w),0).x));
        if(smooth<0.8f)continue; // Same glossy continuation as glass; no diffuse ray.
        float3 hitWorld=World(p.xyz),delta=hitWorld-primaryWorld;
        if(dot(delta,delta)<1.e-6f)continue;
        float3 incoming=normalize(delta),normal=normalize(n.xyz);
        if(dot(incoming,normal)>0)normal=-normal;
        float3 direction=normalize(reflect(incoming,normal));
        // Native positions are FP16. Bias scales with their quantization.
        float bias=max(0.01f,max(abs(p.x),max(abs(p.y),abs(p.z)))*0.001f);
        RayDesc ray;ray.Origin=hitWorld+normal*bias;ray.Direction=direction;ray.TMin=bias;ray.TMax=60;
        Payload payload;payload.value=layer;DeviceMemoryBarrier();
        TraceRay(scene,RAY_FLAG_NONE,1,0,2,0,ray,payload);DeviceMemoryBarrier();
        uint second=nativeMaterial[at];float4 sn=nativeNormal[at],sp=nativePosition[at];
        // Restore every native texel before any validation/exit.
        nativeMaterial[at]=first;nativeNormal[at]=n;nativePosition[at]=p;DeviceMemoryBarrier();
        if(payload.value==0||second>=min(count,65534u)||!all(isfinite(sn))||!all(isfinite(sp))||dot(sn.xyz,sn.xyz)<0.5f)continue;
        OpaqueHit hit;hit.position=sp.xyz;hit.material=second;hit.normal=OpaquePackNormal(normalize(sn.xyz));hit.uv=float2(sn.w,sp.w);hit.valid=epoch;
        extraHits[base+layer]=hit;
    }
}
