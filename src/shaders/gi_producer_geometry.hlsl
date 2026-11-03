// Owned accepted-hit producer for the recovered PT private SBT ABI. It requires
// a host-admitted index/position mapping and retained current backing buffers.
// Do not bind to the native 96-byte SBT: the private records add a position SRV
// and position-stride/bounds constants. Native alpha linkage remains a host task.
#include "../gi_reuse/producer_hit.h"
ByteAddressBuffer giIndices : register(t0,space3);
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
[shader("closesthit")]
void giAcceptedGeometry(inout GIProducerHit hit,BuiltInTriangleIntersectionAttributes attr) {
    hit.valid=0;hit.reserved=0;hit.material=material;
    hit.instanceIndex=InstanceIndex();hit.instanceId=InstanceID();
    hit.geometry=GeometryIndex();hit.primitive=PrimitiveIndex();hit.hitKind=HitKind();
    if(material>=65534||hit.primitive>0xffffffffu/3u)return;
    uint i0,i1,i2;uint first=hit.primitive*3;
    if(first>0xffffffffu-2||!GIReadIndex(first,i0)||!GIReadIndex(first+1,i1)||!GIReadIndex(first+2,i2))return;
    GIPoint v0,v1,v2;
    if(!GIReadPosition(i0,v0)||!GIReadPosition(i1,v1)||!GIReadPosition(i2,v2))return;
    float3 origin=WorldRayOrigin(),direction=WorldRayDirection();
    GIPoint o,d;o.x=origin.x;o.y=origin.y;o.z=origin.z;
    d.x=direction.x;d.y=direction.y;d.z=direction.z;
    GITrueHit geometry=GIRecoverTrueHit(o,d,RayTCurrent(),v0,v1,v2);
    if(!geometry.valid)return;
    hit.position=geometry.position;hit.geometricNormal=geometry.geometricNormal;
    hit.rayT=RayTCurrent();hit.valid=1;
}
[shader("miss")]
void giGeometryMiss(inout GIProducerHit hit) {
    hit.valid=0;hit.material=65535;hit.rayT=RayTCurrent();hit.reserved=0;
}
