// Cosine-weighted samples estimate irradiance/pi directly. Keep native material
// and Fresnel factors at dword 6728; replace only part of the indirect term.
void PTGlassApplyDiffuse(uint2 pixel,inout float3 illumination) {
    float3 fallback=illumination;
    if(any(pixel>=ptGlassExtent))return;
    float3 centerP=g_tPosition_TexcoordY.Load(int3(pixel,0)).xyz;
    float3 centerN=g_tNormal_TexcoordX.Load(int3(pixel,0)).xyz;
    if(!all(isfinite(centerP))||!all(isfinite(centerN))||dot(centerN,centerN)<0.5f)return;
    centerN=normalize(centerN);
    uint material=g_tMaterialId.Load(int3(pixel,0)).x;
    // Broader support is confined to this low-frequency indirect term.
    // Track the effective sample count and variance so a few bright paths
    // cannot receive the same confidence as well-supported soft illumination.
    float3 sum=0;float3 squares=0;float weights=0;float squaredWeights=0;
    [loop] for(int y=-5;y<=5;++y) [loop] for(int x=-5;x<=5;++x) {
        int2 q=int2(pixel)+int2(x,y);
        if(any(q<0)||any(q>=int2(ptGlassExtent)))continue;
        if(g_tMaterialId.Load(int3(q,0)).x!=material)continue;
        float3 n=g_tNormal_TexcoordX.Load(int3(q,0)).xyz;
        float3 p=g_tPosition_TexcoordY.Load(int3(q,0)).xyz;
        if(!all(isfinite(p))||!all(isfinite(n))||dot(n,n)<0.5f||dot(normalize(n),centerN)<0.95f)continue;
        float3 delta=p-centerP;
        float d2=dot(delta,delta);if(d2>0.5625f)continue;
        float w=exp2(-float(x*x+y*y)/24.0f-d2*8.0f);
        float4 sample=ptGlassRadiance.Load(int3(q+int2(0,ptGlassExtent.y),0));
        // A miss or rejected hit retains native GI; do not average only hits,
        // which would bias the estimate toward visible bright geometry.
        float3 value=(sample.w>0.5f&&all(isfinite(sample.xyz)))?max(sample.xyz,0.0f):fallback;
        sum+=value*w;squares+=value*value*w;weights+=w;squaredWeights+=w*w;
    }
    if(weights>0.0f) {
        float3 mean=sum/max(weights,1e-6f);
        float effectiveSamples=weights*weights/max(squaredWeights,1e-6f);
        float3 variance=max(squares/max(weights,1e-6f)-mean*mean,0.0f);
        float3 errorVariance=variance/max(effectiveSamples-1.0f,1.0f);
        // Ten percent relative error budget, per color channel. A dark native
        // estimate can still gain stable light; unreliable paths fall back.
        float3 tolerance=0.1f*max(max(fallback,mean),1e-4f);
        float3 confidence=tolerance*tolerance/(tolerance*tolerance+errorVariance);
        float stable=min(confidence.x,min(confidence.y,confidence.z));
        float support=saturate((effectiveSamples-4.0f)/12.0f);
        if(all(isfinite(mean))&&isfinite(stable))
            illumination=lerp(fallback,mean,0.5f*stable*support);
    }
}
