// Compute-only scene emitter extraction: no native lighting or GI values.
#include "pt_geometry_bounds.hlsli"
struct PTEmitterMaterial {uint words[84];};
StructuredBuffer<PTEmitterMaterial> Materials:register(t8);
#ifdef PT_EMITTER_BUILD_TEST
#define PT_EMITTER_REGISTER(x) register(x)
#else
#define PT_EMITTER_REGISTER(x) register(x,space4)
#endif
ByteAddressBuffer Indices:PT_EMITTER_REGISTER(t0);
ByteAddressBuffer Attributes:PT_EMITTER_REGISTER(t1);
ByteAddressBuffer Positions:PT_EMITTER_REGISTER(t2);
cbuffer GeometryConstants:PT_EMITTER_REGISTER(b0){
    uint indexOffset,indexStride,vertexStride,material;
    uint tangentOffset,normalOffset,texcoordOffset,colorOffset;
    uint indexBytes,vertexBytes,positionStride,positionBytes;
    float4 transform0,transform1,transform2;
    uint triangleCount,alphaTest,pad0,pad1;
};
// Header: attempted records, invalid geometry, overflow, reserved. Records are
// 80 bytes: three world vertices, three UVs, material ID, alpha-test flag.
RWByteAddressBuffer Emitters:register(u4);
void InvalidEmitter(){uint old;Emitters.InterlockedOr(4,1,old);}
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    if(id.x>=triangleCount)return;
    uint bytes;Emitters.GetDimensions(bytes);if(bytes<16)return;
    uint count,stride;Materials.GetDimensions(count,stride);
    if(stride!=336||material>=count||material>=244000/35){InvalidEmitter();return;}
    PTEmitterMaterial m=Materials[material];
    float3 emission=asfloat(m.words[12])*asfloat(uint3(m.words[25],m.words[26],m.words[27]));
    if(!all(isfinite(emission))||any(emission<0)){InvalidEmitter();return;}
    if(!any(emission>0))return;
    PTByteAddress tri=PTCheckedTriangle(id.x,indexOffset,indexStride,indexBytes);
    if(tri.valid==0){InvalidEmitter();return;}
    float3 p[3];float2 uv[3];
    [unroll]for(uint i=0;i<3;++i){
        uint a=tri.address+i*indexStride,word=a&~3u;
        if(!PTCanLoadWord(word,indexBytes)){InvalidEmitter();return;}
        uint packed=Indices.Load(word),index=indexStride==2?((packed>>((a&2u)*8))&65535):packed;
        PTByteAddress pos=PTCheckedField(index,positionStride,0,12,positionBytes);
        PTByteAddress tex=PTCheckedField(index,vertexStride,texcoordOffset,4,vertexBytes);
        if(pos.valid==0||tex.valid==0){InvalidEmitter();return;}
        float4 v=float4(asfloat(Positions.Load3(pos.address)),1);
        p[i]=float3(dot(transform0,v),dot(transform1,v),dot(transform2,v));
        uint t=Attributes.Load(tex.address);uv[i]=float2((int)(t<<16)>>16,(int)t>>16)/4095.0;
        if(!all(isfinite(p[i]))){InvalidEmitter();return;}
    }
    float area2=length(cross(p[1]-p[0],p[2]-p[0]));
    if(!isfinite(area2)){InvalidEmitter();return;}if(area2<=1e-10)return;
    uint slot;Emitters.InterlockedAdd(0,1,slot);
    if(slot>=(bytes-16)/80){uint old;Emitters.InterlockedOr(8,1,old);return;}
    uint at=16+slot*80;
    Emitters.Store4(at,asuint(float4(p[0],uv[0].x)));
    Emitters.Store4(at+16,asuint(float4(p[1],uv[1].x)));
    Emitters.Store4(at+32,asuint(float4(p[2],uv[2].x)));
    Emitters.Store4(at+48,uint4(asuint(float3(uv[0].y,uv[1].y,uv[2].y)),material));
    Emitters.Store4(at+64,uint4(alphaTest,0,0,0));
}
