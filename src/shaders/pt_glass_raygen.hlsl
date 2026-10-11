// Additional export linked beside Control's untouched native glass hit shaders.
// Native primary trace reconstructed from the hash-locked glass DXIL.
#include "pt_glass_secondary_layout.hlsli"
cbuffer PTGlassSys : register(b0) { float4 sys[71]; }
cbuffer PTGlassDispatch : register(b6) { uint epoch; uint materialCount; uint2 extent; }
Texture2D<float4> clipDepth : register(t0);
struct Material { float d[84]; };
StructuredBuffer<Material> materials : register(t1);
RaytracingAccelerationStructure scene : register(t2);
Texture2D<float4> materialTextures[] : register(t0,space1);
SamplerState linearWrap : register(s5,space1);
globallycoherent RWTexture2D<float2> nativeDepth : register(u0);
globallycoherent RWTexture2D<uint> nativeMaterial : register(u1);
globallycoherent RWTexture2D<float4> nativeNormal : register(u2);
globallycoherent RWTexture2D<float4> nativePosition : register(u3);
globallycoherent RWStructuredBuffer<PTGlassSecondaryHit> secondary : register(u4);
struct Payload { float value; };
float3 ViewToWorld(float3 p) {return p.x*sys[6].xyz+p.y*sys[7].xyz+p.z*sys[8].xyz+sys[9].xyz;}
[shader("raygeneration")]
void ptGlassRayGeneration() {
    uint2 pixel=DispatchRaysIndex().xy;
    float2 uv=(float2(pixel)+0.5f)/float2(DispatchRaysDimensions().xy);
    float z=saturate(clipDepth.Load(int3(pixel,0)).x);
    float2 clip=uv*float2(2,-2)+float2(-1,1);
    float4 world=mad(sys[22],clip.x,sys[23]*clip.y);
    world=mad(sys[24],z,world)+sys[25];
    float3 delta=world.xyz/world.w-sys[54].xyz;
    float distance=length(delta);
    RayDesc primary;primary.Origin=sys[54].xyz;primary.Direction=delta/distance;
    primary.TMin=1.0f/sys[55].x;primary.TMax=distance;
    float y0=(z*sys[16].y+sys[17].y)/(z*sys[16].w+sys[17].w);
    float y1=(mad(2.0f/sys[0].y,sys[15].y,z*sys[16].y)+sys[17].y)/
             (mad(2.0f/sys[0].y,sys[15].w,z*sys[16].w)+sys[17].w);
    Payload payload;payload.value=abs(y1-y0)/distance;
    TraceRay(scene,RAY_FLAG_NONE,2,0,2,0,primary,payload);
    DeviceMemoryBarrier();
    if(any(pixel>=extent))return;
    uint index=pixel.y*extent.x+pixel.x;
    secondary[index]=PTGlassInvalidHit(epoch);
    float2 firstDepth=nativeDepth[pixel];
    if(!all(isfinite(firstDepth))||firstDepth.x<=0||firstDepth.x>40000||firstDepth.y<=0||firstDepth.y>60)return;
    uint firstMaterial=nativeMaterial[pixel];
    uint actualCount,stride;materials.GetDimensions(actualCount,stride);
    uint safeCount=min(materialCount,actualCount);
    if(stride!=336||firstMaterial>=safeCount||firstMaterial>=65534)return;
    float4 firstNormal=nativeNormal[pixel],firstPosition=nativePosition[pixel];
    if(!all(isfinite(firstNormal))||!all(isfinite(firstPosition))||dot(firstNormal.xyz,firstNormal.xyz)<0.5f)return;
    // Limit the first implementation to glossy surfaces: a single ideal
    // reflection is not an estimator for rough or diffuse reflection.
    float2 range=float2(materials[firstMaterial].d[17],materials[firstMaterial].d[18]);
    float smoothness=saturate(lerp(range.x,range.y,materialTextures[NonUniformResourceIndex(firstMaterial*35+2)].SampleLevel(linearWrap,float2(firstNormal.w,firstPosition.w),0).x));
    if(smoothness<0.8f)return;
    float3 surfacePoint=ViewToWorld(firstPosition.xyz);
    float3 glass=primary.Origin+primary.Direction*firstDepth.x;
    float3 incoming=normalize(surfacePoint-glass),normal=normalize(firstNormal.xyz);
    if(dot(incoming,normal)>0)normal=-normal;
    float3 outgoing=normalize(reflect(incoming,normal));
    if(!all(isfinite(outgoing))||dot(outgoing,normal)<=0.001f)return;
    RayDesc continuation;continuation.Origin=surfacePoint+normal*0.01f;
    continuation.Direction=outgoing;continuation.TMin=0.01f;continuation.TMax=60;
    // Reflection hit/miss exports stay native, including alpha tests, tangent
    // frames, normal maps, material IDs and texcoords. Save/restore all their
    // native output texels before returning from this additional raygen.
    payload.value=firstDepth.x+firstDepth.y;
    TraceRay(scene,RAY_FLAG_NONE,1,1,2,1,continuation,payload);
    DeviceMemoryBarrier();
    float2 secondDepth=nativeDepth[pixel];
    if(secondDepth.x<40000&&secondDepth.y>0&&secondDepth.y<=60) {
        PTGlassSecondaryHit hit;
        hit.positionView_texcoordY=nativePosition[pixel];
        hit.normalWorld_texcoordX=nativeNormal[pixel];
        hit.incomingWorld_hitDistance=float4(outgoing,secondDepth.y);
        hit.metadata=uint4(nativeMaterial[pixel],1,epoch,0);
        if(PTGlassValidHit(hit,epoch,safeCount))secondary[index]=hit;
    }
    nativeDepth[pixel]=firstDepth;nativeMaterial[pixel]=firstMaterial;
    nativeNormal[pixel]=firstNormal;nativePosition[pixel]=firstPosition;
    DeviceMemoryBarrier();
}
