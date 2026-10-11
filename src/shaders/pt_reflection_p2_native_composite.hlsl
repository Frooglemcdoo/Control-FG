// Secondary-hit material and lighting use the captured native evaluator.
// Same root signature/descriptors, hit layout, and ON/OFF A/B boundary.
#include "pt_native_secondary.generated.hlsli"
float3 PTNativeViewToWorld(float3 p) {
    return asfloat(ptCB0[6]).xyz*p.x+asfloat(ptCB0[7]).xyz*p.y+
           asfloat(ptCB0[8]).xyz*p.z+asfloat(ptCB0[9]).xyz;
}
float PTSmoothness(uint id) {
    return saturate(0.5f*(asfloat(g_sbMaterialDataPart3[id].d[17])+
                          asfloat(g_sbMaterialDataPart3[id].d[18])));
}
[numthreads(8,16,1)]
void main(uint3 dispatchThreadId:SV_DispatchThreadID) {
    uint2 pixel=dispatchThreadId.xy;
    uint2 size=(uint2)asfloat(ptCB0[0]).xy;
    if(any(pixel>=size))return;
    uint count,stride;
    g_sbMaterialDataPart3.GetDimensions(count,stride);
    if(stride!=336u)count=0u;
    uint first=g_tMaterialId.Load(int4(pixel,0,0)).x;
    float4 firstPos=g_tPosition_TexcoordY.Load(int4(pixel,0,0));
    float4 secondPos=g_tPosition_TexcoordY.Load(int4(pixel,1,0));
    float4 normal=g_tNormal_TexcoordX.Load(int4(pixel,1,0));
    uint rejection=1u;
    float3 bounce=0.0f;
    if(first<65534u){
        rejection=5u;
        if(first<count){
            rejection=2u;
            if(normal.w>=0.0f&&normal.w<2048.0f){
                uint second=(uint)round(normal.w);
                rejection=5u;
                if(second<count){
                    rejection=3u;
                    float n2=dot(normal.xyz,normal.xyz);
                    if(all(isfinite(normal.xyz))&&n2>1.0e-6f){
                        rejection=4u;
                        float3 delta=secondPos.xyz-firstPos.xyz;
                        float d2=dot(delta,delta);
                        if(all(isfinite(firstPos.xyz))&&all(isfinite(secondPos.xyz))&&
                           isfinite(d2)&&d2>1.0e-6f&&isfinite(secondPos.w)){
                            rejection=0u;
                            float3 hitWorld=PTNativeViewToWorld(secondPos.xyz);
                            float3 worldDelta=hitWorld-PTNativeViewToWorld(firstPos.xyz);
                            float3 incoming=normalize(worldDelta);
                            float3 n=normal.xyz*rsqrt(n2);
                            if(dot(n,incoming)>0.0f)n=-n;
                            uint packed=(uint)round(clamp(secondPos.w,0.0f,1023.0f));
                            float2 uv=float2(packed&31u,(packed>>5u)&31u)/31.0f;
                            float3 radiance=PTNativeSecondHit(second,uv,secondPos.xyz,hitWorld,
                                n,incoming,delta*rsqrt(d2),firstPos.xyz,pixel);
                            // Keep the established path weight for this isolated lighting test.
                            float smoothness=PTSmoothness(first),roughness=1.0f-smoothness;
                            float weight=(0.08f+0.34f*smoothness*smoothness+
                                0.38f*roughness*roughness)*rsqrt(1.0f+0.0025f*d2);
                            bounce=radiance*weight;
                        }
                    }
                }
            }
        }
    }
#if PT_REFLECTION_VERIFY_VARIANT == 1
    bool right=pixel.x>=size.x/2u,bottom=pixel.y>=size.y/2u;
    if(!right&&!bottom)return;
    if(right&&!bottom){
        float3 color=rejection==0u?float3(0.02f,6.0f,0.02f):
            rejection==1u?float3(6.0f,0.02f,0.02f):
            rejection==2u?float3(6.0f,0.02f,5.0f):
            rejection==3u?float3(6.0f,6.0f,0.02f):
            rejection==4u?float3(0.02f,6.0f,6.0f):float3(6.0f,6.0f,6.0f);
        g_rwtReflectionTarget[pixel]=float4(color,1.0f);return;
    }
    if(!right&&bottom){
        float lum=max(dot(bounce,float3(0.2126f,0.7152f,0.0722f)),0.0f);
        float m=saturate(lum/(lum+0.02f));
        float3 heat=rejection==0u?float3(4*m*m,4*m*(1-m),2*(1-m)):0.0f;
        g_rwtReflectionTarget[pixel]=float4(heat,1.0f);return;
    }
    float4 amplified=g_rwtReflectionTarget[pixel];
    amplified.rgb+=bounce*16.0f;g_rwtReflectionTarget[pixel]=amplified;return;
#else
    if(rejection!=0u)return;
    float luminance=max(dot(bounce,float3(0.2126f,0.7152f,0.0722f)),1.0e-5f);
    float clampValue=asfloat(ptCB6[0].x);
    if(clampValue>0.0f&&luminance>clampValue)bounce*=clampValue/luminance;
    // Native reflection distance fade (native SHEX dwords 7302..7392).
    float4 fade=asfloat(ptCB1[17]);
    float linearDepth=g_tLinearDepth.Load(int3(pixel,0)).x;
    float blend=saturate((linearDepth-(fade.x-fade.y))/max(2.0f*fade.y,1.0e-6f));
    float nativeScale=fade.z+(fade.w-fade.z)*blend*blend*(3.0f-2.0f*blend);
    bounce*=max(nativeScale,0.0f);
    float4 base=g_rwtReflectionTarget[pixel];
    base.rgb=max(base.rgb+bounce,0.0f);g_rwtReflectionTarget[pixel]=base;
#endif
}
