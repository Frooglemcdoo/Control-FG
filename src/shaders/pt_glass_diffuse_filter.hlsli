// Cosine-weighted samples estimate irradiance/pi directly. Keep native material
// and Fresnel factors at dword 6728; replace only part of the indirect term.
float3 PTGlassFilteredDiffuse(uint2 pixel,float3 fallback) {
    if(any(pixel>=ptGlassExtent))return fallback;
    float3 centerP=g_tPosition_TexcoordY.Load(int3(pixel,0)).xyz;
    float3 centerN=g_tNormal_TexcoordX.Load(int3(pixel,0)).xyz;
    if(!all(isfinite(centerP))||dot(centerN,centerN)<0.5f)return fallback;
    centerN=normalize(centerN);
    uint material=g_tMaterialId.Load(int3(pixel,0)).x;
    float3 sum=0;float weights=0;
    [loop] for(int y=-3;y<=3;++y) [loop] for(int x=-3;x<=3;++x) {
        int2 q=int2(pixel)+int2(x,y);
        if(any(q<0)||any(q>=int2(ptGlassExtent)))continue;
        if(g_tMaterialId.Load(int3(q,0)).x!=material)continue;
        float3 n=g_tNormal_TexcoordX.Load(int3(q,0)).xyz;
        float3 p=g_tPosition_TexcoordY.Load(int3(q,0)).xyz;
        if(!all(isfinite(p))||dot(n,n)<0.5f||dot(normalize(n),centerN)<0.95f)continue;
        float3 delta=p-centerP;
        float d2=dot(delta,delta);if(d2>0.5625f)continue;
        float w=exp2(-float(x*x+y*y)/8.0f-d2*8.0f);
        float4 sample=ptGlassRadiance.Load(int3(q+int2(0,ptGlassExtent.y),0));
        // A miss or rejected hit retains native GI; do not average only hits,
        // which would bias the estimate toward visible bright geometry.
        float3 value=(sample.w>0.5f&&all(isfinite(sample.xyz)))?max(sample.xyz,0.0f):fallback;
        sum+=value*w;weights+=w;
    }
    return weights>0?lerp(fallback,sum/weights,0.5f):fallback;
}
