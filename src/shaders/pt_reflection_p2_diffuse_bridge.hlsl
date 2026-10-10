// P2 rough-surface diffuse GI bridge.
// Runs at Control's raw broad-diffuse boundary after the GI producer.
// Routes rough primary-surface P2 radiance into diffuse GI instead of the
// specular/reflection-only target.

cbuffer PTDiffuseBridgeConstants : register(b0)
{
    uint g_uOutputWidth;
    uint g_uOutputHeight;
    uint g_uReflectionWidth;
    uint g_uReflectionHeight;
    uint g_uGBufferWidth;
    uint g_uGBufferHeight;
    float g_fStrength;
    float g_fPad;
};

Texture2D<float4> g_tP2Reflection : register(t0);
Texture2D<float4> g_tGBuffer1 : register(t1);
RWTexture2D<float4> g_rwDiffuseGI : register(u0);

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if(id.x>=g_uOutputWidth || id.y>=g_uOutputHeight) return;

    float2 uv=(float2(id.xy)+0.5f)/float2(g_uOutputWidth,g_uOutputHeight);

    uint2 rp=min((uint2)(uv*float2(g_uReflectionWidth,g_uReflectionHeight)),
                 uint2(g_uReflectionWidth-1u,g_uReflectionHeight-1u));
    uint2 gp=min((uint2)(uv*float2(g_uGBufferWidth,g_uGBufferHeight)),
                 uint2(g_uGBufferWidth-1u,g_uGBufferHeight-1u));

    float4 g1=g_tGBuffer1.Load(int3(gp,0));
    float roughness=saturate(1.0f-g1.b);

    // Keep smooth/glossy materials in the reflection path. Blend increasingly
    // into diffuse GI for plaster/concrete/cloth and other rough materials.
    float diffuseGate=smoothstep(0.42f,0.88f,roughness);
    if(diffuseGate<=1.0e-4f)return;

    float3 incoming=max(g_tP2Reflection.Load(int3(rp,0)).rgb,0.0f);

    // Reflections can contain isolated hot samples. A soft luminance limiter
    // keeps the GI bridge broad and prevents rough walls from becoming glossy.
    float lum=max(dot(incoming,float3(0.2126f,0.7152f,0.0722f)),1.0e-5f);
    incoming*=min(1.0f,4.0f/lum);

    float4 current=g_rwDiffuseGI[id.xy];
    current.rgb=max(current.rgb+incoming*(diffuseGate*g_fStrength),0.0f);
    g_rwDiffuseGI[id.xy]=current;
}
