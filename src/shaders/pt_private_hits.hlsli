#ifndef PT_PRIVATE_HITS
#define PT_PRIVATE_HITS
// Three 32-byte records per pixel (two hits + native backup). Fixed 768 MiB allocation covers up to 4K.
// This avoids replacing/freeing a resource while submitted frames still use it.
static const uint PT_PRIVATE_PIXEL_CAPACITY=8388608u;
struct PTPrivateHit {
    float3 worldPosition;
    uint material;
    float2 uv;
    uint packedNormal;
    uint reserved;
};
globallycoherent RWStructuredBuffer<PTPrivateHit> g_ptPrivateHits : register(u0,space99);
float2 PTSignNotZero(float2 v) { return float2(v.x>=0?1:-1,v.y>=0?1:-1); }
uint PTPackNormal(float3 n) {
    n/=max(abs(n.x)+abs(n.y)+abs(n.z),1.0e-12f);
    float2 p=n.z>=0?n.xy:(1.0f-abs(n.yx))*PTSignNotZero(n.xy);
    uint2 q=(uint2)round(saturate(p*0.5f+0.5f)*65535.0f);
    return q.x|(q.y<<16u);
}
float3 PTUnpackNormal(uint packed) {
    float2 p=float2(packed&65535u,packed>>16u)*(2.0f/65535.0f)-1.0f;
    float3 n=float3(p,1.0f-abs(p.x)-abs(p.y));
    if(n.z<0)n.xy=(1.0f-abs(n.yx))*PTSignNotZero(n.xy);
    return normalize(n);
}
void PTClearPrivate(uint index) {
    PTPrivateHit h=(PTPrivateHit)0;
    h.material=65535u;
    g_ptPrivateHits[index]=h;
}
#endif
