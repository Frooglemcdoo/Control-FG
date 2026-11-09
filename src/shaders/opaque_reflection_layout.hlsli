#ifndef OPAQUE_REFLECTION_LAYOUT
#define OPAQUE_REFLECTION_LAYOUT
// 32 bytes per native sample. Native textures are never used as persistent scratch.
struct OpaqueHit { float3 position; uint material; uint normal; float2 uv; uint valid; };
uint OpaquePackNormal(float3 n) {
    n/=dot(abs(n),1.0f.xxx);
    float2 p=n.z>=0?n.xy:(1.0f-abs(n.yx))*float2(n.x>=0?1:-1,n.y>=0?1:-1);
    uint2 q=(uint2)round(saturate(p*0.5f+0.5f)*65535.0f);
    return q.x|(q.y<<16u);
}
float3 OpaqueUnpackNormal(uint p) {
    float2 v=float2(p&65535u,p>>16u)*(2.0f/65535.0f)-1.0f;
    float3 n=float3(v,1.0f-abs(v.x)-abs(v.y));
    float t=saturate(-n.z);n.xy+=float2(n.x>=0?-t:t,n.y>=0?-t:t);
    return normalize(n);
}
#endif
