// Private DXR scene-query library. Requires a NEW state object and rebuilt SBT.
// Never substitute into native 4-byte payload shaders or native output UAVs.
#include "pt_geometry_bounds.hlsli"
struct PTSceneHit { float distance; float2 uv; float3 shadingNormal; uint material; uint status; };
struct PTVisibility { uint status; }; // 0=occluded, 1=visible, 2=invalid
// Hit status: 0=miss, 1=valid attributes, 2=invalid record/geometry/material.
cbuffer PTHitRecord : register(b0,space3) {
    uint indexOffset,indexStride,vertexStride,materialID;
    uint tangentOffset,normalOffset,texcoordOffset,colorOffset;
    uint indexBytes,vertexBytes;
};
ByteAddressBuffer PTIndices : register(t0,space3);
ByteAddressBuffer PTVertices : register(t1,space3);
struct PTNativeMaterial { uint words[84]; };
StructuredBuffer<PTNativeMaterial> PTMaterials : register(t3);
Texture2D<float4> PTTextures[] : register(t0,space1);
SamplerState PTLinearWrap : register(s5,space1);

bool PTReadIndices(uint primitive,out uint3 indices) {
    // Root SRVs contain GPU addresses, not buffer-view lengths. The host must
    // validate these record byte limits against each backing allocation.
    indices=0;uint physical=indexBytes;
    PTByteAddress tri=PTCheckedTriangle(primitive,indexOffset,indexStride,min(indexBytes,physical));
    if(tri.valid==0) return false;
    [unroll] for(uint i=0;i<3;++i) {
        uint a=tri.address+i*indexStride,word=a&~3u;
        if(!PTCanLoadWord(word,physical)) return false;
        uint packed=PTIndices.Load(word);
        indices[i]=indexStride==2u?((packed>>((a&2u)*8u))&65535u):packed;
    }
    return true;
}
bool PTReadAttributes(BuiltInTriangleIntersectionAttributes attr,out float2 uv,out float3 normalObject) {
    uv=0;normalObject=0;uint3 indices;
    if(!PTReadIndices(PrimitiveIndex(),indices)) return false;
    float3 w=float3(1-attr.barycentrics.x-attr.barycentrics.y,attr.barycentrics);
    uint limit=vertexBytes;
    [unroll] for(uint i=0;i<3;++i) {
        PTByteAddress t=PTCheckedField(indices[i],vertexStride,texcoordOffset,4,limit);
        PTByteAddress n=PTCheckedField(indices[i],vertexStride,normalOffset,8,limit);
        if(t.valid==0||n.valid==0) return false;
        uint a=PTVertices.Load(t.address);uint2 b=PTVertices.Load2(n.address);
        float2 ti=float2((int)(a<<16)>>16,(int)a>>16)/4095.0f;
        float3 ni=float3((int)(b.x<<16)>>16,(int)b.x>>16,(int)(b.y<<16)>>16)/32767.0f;
        uv+=ti*w[i];normalObject+=ni*w[i];
    }
    return all(isfinite(uv))&&all(isfinite(normalObject))&&dot(normalObject,normalObject)>1e-12;
}
bool PTMaterialRecordValid() {
    uint count,stride;PTMaterials.GetDimensions(count,stride);
    return stride==336u&&materialID<count&&materialID<(244000u/35u);
}
// Preserves the previously recovered native alpha rule for alpha-hit groups.
// Opaque hit groups MUST NOT bind this any-hit shader.
uint PTAlphaDecision(BuiltInTriangleIntersectionAttributes attr) {
    float2 uv;float3 n;
    if(!PTMaterialRecordValid()||!PTReadAttributes(attr,uv,n)) return 2u;
    float a=PTTextures[NonUniformResourceIndex(materialID*35u)].SampleLevel(PTLinearWrap,uv,0).a;
    float scale=asfloat(PTMaterials[materialID].words[16]);
    if(!isfinite(a)||!isfinite(scale)) return 2u;
    return a*scale>=0.5f?1u:0u;
}
[shader("closesthit")]
void PTSceneClosestHit(inout PTSceneHit p,in BuiltInTriangleIntersectionAttributes attr) {
    if(p.status==2u) return;
    float2 uv;float3 n;
    if(!PTMaterialRecordValid()||!PTReadAttributes(attr,uv,n)) {p.status=2;return;}
    float3x4 m=WorldToObject3x4();
    float3 world=float3(dot(n,float3(m[0][0],m[1][0],m[2][0])),
                        dot(n,float3(m[0][1],m[1][1],m[2][1])),
                        dot(n,float3(m[0][2],m[1][2],m[2][2])));
    float len2=dot(world,world);
    if(!all(isfinite(world))||len2<1e-12) {p.status=2;return;}
    p.distance=RayTCurrent();p.uv=uv;p.shadingNormal=world*rsqrt(len2);p.material=materialID;p.status=1;
}
[shader("anyhit")]
void PTSceneAlphaHit(inout PTSceneHit p,in BuiltInTriangleIntersectionAttributes attr) {
    uint a=PTAlphaDecision(attr);
    if(a==0u) IgnoreHit();
    else if(a==2u) {p.status=2u;AcceptHitAndEndSearch();}
}
[shader("miss")]
void PTSceneMiss(inout PTSceneHit p) {p.status=0u;p.distance=0;p.uv=0;p.shadingNormal=0;p.material=0xffffffffu;}
[shader("closesthit")]
void PTVisibilityClosestHit(inout PTVisibility p,in BuiltInTriangleIntersectionAttributes attr) {if(p.status!=2u)p.status=0u;}
[shader("anyhit")]
void PTVisibilityAlphaHit(inout PTVisibility p,in BuiltInTriangleIntersectionAttributes attr) {
    uint a=PTAlphaDecision(attr);
    if(a==0u) IgnoreHit();
    else {p.status=a==2u?2u:0u;AcceptHitAndEndSearch();}
}
[shader("miss")]
void PTVisibilityMiss(inout PTVisibility p) {p.status=1u;}
