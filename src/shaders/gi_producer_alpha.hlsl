// Owned counterpart of the two hash-pinned GI anyHitAlphaTest exports.
// Uses the evaluator's material register t5 (native ray library uses t3).
// Host must bind the matching material array/sampler and private geometry root.
#include "gi_producer_geometry.hlsl"
struct GIAlphaMaterial {uint d[84];};
StructuredBuffer<GIAlphaMaterial> giAlphaMaterials : register(t5);
Texture2D<float4> giAlphaTextures[] : register(t0,space1);
SamplerState giAlphaWrap : register(s5,space1);
bool GIReadAlphaUV(uint vertex,out float2 uv) {
    uv=0;uint address;
    if(!GIAttributeAddress(vertex,uvOffset,4,address))return false;
    uint packed=giAttributes.Load(address);
    uv=float2(int2((int)(packed<<16u)>>16,(int)packed>>16));
    return true;
}
[shader("anyhit")]
void giAcceptedAlpha(inout GIProducerHit hit,BuiltInTriangleIntersectionAttributes attr) {
    uint count,stride;giAlphaMaterials.GetDimensions(count,stride);
    if(stride!=336||material>=count||material>=244000u/35u||PrimitiveIndex()>0xffffffffu/3u){hit.nativeValue=0xffffffffu;return;}
    uint first=PrimitiveIndex()*3,i0,i1,i2;
    if(first>0xffffffffu-2||!GIReadIndex(first,i0)||!GIReadIndex(first+1,i1)||!GIReadIndex(first+2,i2)){hit.nativeValue=0xffffffffu;return;}
    float2 uv0,uv1,uv2;
    if(!GIReadAlphaUV(i0,uv0)||!GIReadAlphaUV(i1,uv1)||!GIReadAlphaUV(i2,uv2)){hit.nativeValue=0xffffffffu;return;}
    float3 weights=float3(1-attr.barycentrics.x-attr.barycentrics.y,attr.barycentrics);
    float2 uv=((uv1*weights.y+uv2*weights.z)+uv0*weights.x)*(1.0f/4095.0f);
    float multiplier=asfloat(giAlphaMaterials[material].d[16]); // byte 64: vColorMultiplier.w
    float alpha=giAlphaTextures[NonUniformResourceIndex(material*35u)].SampleLevel(giAlphaWrap,uv,0).a;
    // Native policy: no stochastic alpha, mip override, threshold adjustment,
    // valid-path payload mutation or forced opaque tracing. Equality at 0.5 accepts the hit.
    if(alpha*multiplier<0.5f)IgnoreHit();
}
