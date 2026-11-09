// Secondary-hit material and lighting use the captured native evaluator.
// Same root signature/descriptors, hit layout, and ON/OFF A/B boundary.
#include "pt_native_secondary.generated.hlsli"
float3 PTNativeViewToWorld(float3 p) {
    return asfloat(ptCB0[6]).xyz*p.x+asfloat(ptCB0[7]).xyz*p.y+
           asfloat(ptCB0[8]).xyz*p.z+asfloat(ptCB0[9]).xyz;
}
float3 PTPrimaryView(uint2 pixel) {
    float2 xy=(float2(pixel)+0.5f)*asfloat(ptCB0[1]).zw*float2(2,-2)+float2(-1,1);
    float z=g_tClipDepth.Load(int3(pixel,0)).x;
    float4 p=xy.x*asfloat(ptCB0[14])+xy.y*asfloat(ptCB0[15])+z*asfloat(ptCB0[16])+asfloat(ptCB0[17]);
    return p.xyz/p.w;
}
float3 PTWorldDirection(float3 d) {
    return normalize(asfloat(ptCB0[6]).xyz*d.x+asfloat(ptCB0[7]).xyz*d.y+asfloat(ptCB0[8]).xyz*d.z);
}
float PTG1(float nd,float alpha2) {
    return 2.0f*nd/max(nd+sqrt(alpha2+(1.0f-alpha2)*nd*nd),1.0e-7f);
}
float3 PTFirstHitThroughput(uint id,float2 uv,float3 n,float3 wo,float3 wi) {
    float nv=saturate(dot(n,wo)),nl=saturate(dot(n,wi));
    uint ti=NonUniformResourceIndex(id*35u);
    float3 albedo=saturate(g_sMaterialTextureArray[ti].SampleLevel(g_sLinearWrap,uv,0).rgb*
        asfloat(uint3(g_sbMaterialDataPart3[id].d[13],g_sbMaterialDataPart3[id].d[14],g_sbMaterialDataPart3[id].d[15])));
    uint packed=g_sbMaterialDataPart1[id].d[0];
    float mask=g_sMaterialTextureArray[NonUniformResourceIndex(id*35u+4u)].SampleLevel(g_sLinearWrap,uv,0).x;
    float3 f0=saturate(float3((packed>>24u)&255u,(packed>>16u)&255u,(packed>>8u)&255u)*(mask/255.0f));
    float smoothTex=g_sMaterialTextureArray[NonUniformResourceIndex(id*35u+2u)].SampleLevel(g_sLinearWrap,uv,0).x;
    float smoothness=saturate(lerp(asfloat(g_sbMaterialDataPart3[id].d[17]),asfloat(g_sbMaterialDataPart3[id].d[18]),smoothTex));
    float roughness=1.0f-smoothness;
    float alpha=max(roughness*roughness,0.02f),a2=alpha*alpha;
    float3 halfVector=wo+wi;
    float3 h=halfVector*rsqrt(max(dot(halfVector,halfVector),1.0e-12f));
    float nh=saturate(dot(n,h)),vh=saturate(dot(wo,h));
    float den=nh*nh*(a2-1.0f)+1.0f;
    float D=a2/max(3.14159265359f*den*den,1.0e-10f);
    float gv=PTG1(nv,a2),gl=PTG1(nl,a2);
    float3 F=f0+(1.0f-f0)*pow(1.0f-vh,5.0f);
    float3 f=(1.0f-F)*albedo/3.14159265359f+F*(D*gv*gl/max(4.0f*nv*nl,1.0e-7f));
    float diffuseMix=clamp((roughness-0.35f)/0.45f,0.1f,0.9f);
    float pdf=diffuseMix*nl/3.14159265359f+(1.0f-diffuseMix)*D*gv/max(4.0f*nv,1.0e-7f);
    float3 throughput=f*nl/max(pdf,1.0e-10f);
    bool finiteWeight=!any((asuint(throughput)&0x7f800000u)==0x7f800000u);
    return (nv>0.0f&&nl>0.0f&&finiteWeight)?max(throughput,0.0f):0.0f;
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
    float3 replacement=0.0f;
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
                            float3 unusedDirect;
                            float3 radiance=PTNativeSecondHit(second,uv,secondPos.xyz,hitWorld,
                                n,incoming,delta*rsqrt(d2),firstPos.xyz,pixel,true,unusedDirect);
                            float4 firstNormal=g_tNormal_TexcoordX.Load(int4(pixel,0,0));
                            float3 primaryView=PTPrimaryView(pixel);
                            float3 firstIncomingView=normalize(firstPos.xyz-primaryView);
                            float3 firstIncoming=PTWorldDirection(firstIncomingView);
                            float3 firstN=normalize(firstNormal.xyz);
                            if(dot(firstN,firstIncoming)>0.0f)firstN=-firstN;
                            float2 firstUv=float2(firstNormal.w,firstPos.w);
                            float3 throughput=PTFirstHitThroughput(first,firstUv,firstN,-firstIncoming,incoming);
                            bounce=radiance*throughput;
                            float3 firstDirect;
                            PTNativeSecondHit(first,firstUv,firstPos.xyz,PTNativeViewToWorld(firstPos.xyz),
                                firstN,firstIncoming,firstIncomingView,primaryView,pixel,false,firstDirect);
                            // The explicit bounce REPLACES first-hit probe indirect light.
                            // Keep local direct lighting and emission exactly once.
                            replacement=max(firstDirect,0.0f)+bounce;
                            if(!all(isfinite(replacement)))rejection=4u;

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
    float luminance=max(dot(replacement,float3(0.3333f,0.3333f,0.3333f)),1.0e-5f);
    float clampValue=asfloat(ptCB6[0].x);
    if(clampValue>=0.0f&&luminance>clampValue)replacement*=clampValue/luminance;
    // Native reflection distance fade (native SHEX dwords 7302..7392).
    float4 fade=asfloat(ptCB1[17]);
    float linearDepth=g_tLinearDepth.Load(int3(pixel,0)).x;
    float blend=saturate((linearDepth-(fade.x-fade.y))/max(2.0f*fade.y,1.0e-6f));
    float nativeScale=fade.z+(fade.w-fade.z)*blend*blend*(3.0f-2.0f*blend);
    replacement*=max(nativeScale,0.0f);
    float4 base=g_rwtReflectionTarget[pixel];
    base.rgb=max(replacement,0.0f);g_rwtReflectionTarget[pixel]=base;
#endif
}
