// Owned accepted-hit producer for the recovered PT private SBT ABI. It requires
// a host-admitted index/position mapping and retained current backing buffers.
// Do not bind to the native 96-byte SBT: the private records add a position SRV
// and position-stride/bounds constants. Native alpha linkage remains a host task.
#include "../gi_reuse/producer_hit.h"
ByteAddressBuffer giIndices : register(t0,space3);
ByteAddressBuffer giAttributes : register(t1,space3);
ByteAddressBuffer giPositions : register(t2,space3);
cbuffer GIPrivateHit : register(b0,space3) {
    uint indexOffset;uint indexStride;uint attributeStride;uint material;
    uint tangentOffset;uint normalOffset;uint uvOffset;uint colorOffset;
    uint indexBufferBytes;uint attributeBufferBytes;uint positionStride;uint positionBytes;
}
bool GIReadIndex(uint element,out uint index) {
    index=0;
    if((indexStride!=2&&indexStride!=4)||indexOffset%indexStride||
       indexOffset>indexBufferBytes||element>=(indexBufferBytes-indexOffset)/indexStride)return false;
    uint address=indexOffset+element*indexStride;
    uint wordAddress=address&~3u;
    if(indexBufferBytes<4||wordAddress>indexBufferBytes-4)return false;
    uint word=giIndices.Load(wordAddress);
    index=indexStride==4?word:(word>>((address&2u)*8u))&65535u;return true;
}
bool GIReadPosition(uint index,out GIPoint vertex) {
    vertex=(GIPoint)0;
    if(positionStride<12||(positionStride&3)||positionBytes<12||index>(positionBytes-12)/positionStride)return false;
    float3 p=asfloat(giPositions.Load3(index*positionStride));
    float3 world=mul(ObjectToWorld3x4(),float4(p,1));
    vertex.x=world.x;vertex.y=world.y;vertex.z=world.z;
    return all(isfinite(p))&&GIFinitePoint(vertex);
}
// Decode the same signed packed attributes used by PTNativeHitAttributes.
// Reject invalid addresses rather than clamping to another vertex's attributes.
bool GIAttributeAddress(uint vertex,uint offset,uint bytes,out uint address) {
    address=0;
    if(!attributeStride||(attributeStride&3)||(offset&3)||offset>attributeStride||
       bytes>attributeStride-offset||offset>attributeBufferBytes||
       bytes>attributeBufferBytes-offset)return false;
    if(vertex>(attributeBufferBytes-offset-bytes)/attributeStride)return false;
    address=vertex*attributeStride+offset;return true;
}
bool GIReadAttributes(uint vertex,out float3 normal,out float2 uv) {
    normal=0;uv=0;uint n,t;
    if(!GIAttributeAddress(vertex,normalOffset,8,n)||!GIAttributeAddress(vertex,uvOffset,4,t))return false;
    uint2 packed=giAttributes.Load2(n);
    int3 signedNormal=int3((int)(packed.x<<16u)>>16,(int)packed.x>>16,(int)(packed.y<<16u)>>16);
    normal=float3(signedNormal)*(1.0f/32767.0f);
    uint tex=giAttributes.Load(t);
    uv=float2(int2((int)(tex<<16u)>>16,(int)tex>>16))*(1.0f/4095.0f);
    return true;
}
bool GIInterpolateAttributes(uint3 indices,float2 bary,out float3 worldNormal,out float2 uv) {
    worldNormal=0;uv=0;
    if(!all(isfinite(bary))||any(bary<0)||bary.x+bary.y>1.000001f)return false;
    float3 n0,n1,n2;float2 t0,t1,t2;
    if(!GIReadAttributes(indices.x,n0,t0)||!GIReadAttributes(indices.y,n1,t1)||
       !GIReadAttributes(indices.z,n2,t2))return false;
    float3 weights=float3(1-bary.x-bary.y,bary);
    float3 n=n0*weights.x+n1*weights.y+n2*weights.z;
    uv=t0*weights.x+t1*weights.y+t2*weights.z;
    float3x4 inverse=WorldToObject3x4();
    worldNormal=float3(
        dot(n,float3(inverse[0][0],inverse[1][0],inverse[2][0])),
        dot(n,float3(inverse[0][1],inverse[1][1],inverse[2][1])),
        dot(n,float3(inverse[0][2],inverse[1][2],inverse[2][2])));
    // Preserve PTNativeHitAttributes interpolation magnitude for the evaluator ABI.
    float lengthSquared=dot(worldNormal,worldNormal);
    return all(isfinite(worldNormal))&&all(isfinite(uv))&&isfinite(lengthSquared)&&lengthSquared>0;
}
[shader("closesthit")]
void giAcceptedGeometry(inout GIProducerHit hit,BuiltInTriangleIntersectionAttributes attr) {
    hit.valid=0;hit.reserved=0;hit.material=material;
    hit.reserved1=0;hit.reserved2=0;hit.reserved3=0;
    hit.instanceIndex=InstanceIndex();hit.instanceId=InstanceID();
    hit.geometry=GeometryIndex();hit.primitive=PrimitiveIndex();hit.hitKind=HitKind();
    if(hit.nativeValue==0xffffffffu||material>=65534||hit.primitive>0xffffffffu/3u)return;
    uint i0,i1,i2;uint first=hit.primitive*3;
    if(first>0xffffffffu-2||!GIReadIndex(first,i0)||!GIReadIndex(first+1,i1)||!GIReadIndex(first+2,i2))return;
    GIPoint v0,v1,v2;
    if(!GIReadPosition(i0,v0)||!GIReadPosition(i1,v1)||!GIReadPosition(i2,v2))return;
    float3 origin=WorldRayOrigin(),direction=WorldRayDirection();
    GIPoint o,d;o.x=origin.x;o.y=origin.y;o.z=origin.z;
    d.x=direction.x;d.y=direction.y;d.z=direction.z;
    GITrueHit geometry=GIRecoverTrueHit(o,d,RayTCurrent(),v0,v1,v2);
    if(!geometry.valid)return;
    float3 shading;float2 uv;
    if(!GIInterpolateAttributes(uint3(i0,i1,i2),attr.barycentrics,shading,uv))return;
    hit.shadingNormal.x=shading.x;hit.shadingNormal.y=shading.y;hit.shadingNormal.z=shading.z;
    hit.texcoordU=uv.x;hit.texcoordV=uv.y;
    hit.position=geometry.position;hit.geometricNormal=geometry.geometricNormal;
    hit.rayT=RayTCurrent();hit.valid=1;
}
[shader("miss")]
void giGeometryMiss(inout GIProducerHit hit) {
    hit=(GIProducerHit)0;hit.material=65535;hit.rayT=RayTCurrent();hit.reserved=0;
}
