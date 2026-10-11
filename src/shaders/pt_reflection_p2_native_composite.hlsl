// Secondary-hit material and lighting use the captured native evaluator.
// Same root signature/descriptors, hit layout, and ON/OFF A/B boundary.
#include "pt_native_secondary.generated.hlsli"
#include "ControlPrimaryGuideDecode.hlsli"
#include "pt_private_hits.hlsli"
float3 PTWorldToView(float3 p) {
    return asfloat(ptCB0[2]).xyz*p.x+asfloat(ptCB0[3]).xyz*p.y+asfloat(ptCB0[4]).xyz*p.z+asfloat(ptCB0[5]).xyz;
}
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
bool PTFinite3(float3 v) { return !any((asuint(v)&0x7f800000u)==0x7f800000u); }
bool PTFinite1(float v) { return (asuint(v)&0x7f800000u)!=0x7f800000u; }
float PTG1(float nd,float alpha2) {
    return 2.0f*nd/max(nd+sqrt(alpha2+(1.0f-alpha2)*nd*nd),1.0e-7f);
}
float3 PTFirstHitThroughput(uint id,float2 uv,float3 n,float3 wo,float3 wi) {
    float nv=saturate(dot(n,wo)),nl=saturate(dot(n,wi));
    uint packed=g_sbMaterialDataPart1[id].d[0];
    float mask=g_sMaterialTextureArray[NonUniformResourceIndex(id*35u+4u)].SampleLevel(g_sMaterialArrayGlobal,uv,0).x;
    float3 f0=saturate(float3((packed>>24u)&255u,(packed>>16u)&255u,(packed>>8u)&255u)*(mask/255.0f));
    float smoothTex=g_sMaterialTextureArray[NonUniformResourceIndex(id*35u+2u)].SampleLevel(g_sLinearWrap,uv,0).x;
    float smoothness=saturate(lerp(asfloat(g_sbMaterialDataPart3[id].d[17]),asfloat(g_sbMaterialDataPart3[id].d[18]),smoothTex));
    float roughness=1.0f-smoothness;
    float alpha=max(roughness*roughness,0.02f),a2=alpha*alpha;
    float3 halfVector=wo+wi;
    float3 h=halfVector*rsqrt(max(dot(halfVector,halfVector),1.0e-12f));
    float vh=saturate(dot(wo,h));
    float3 F=f0+(1.0f-f0)*pow(1.0f-vh,5.0f);
    // For the GGX visible-normal sampler, f_spec*cos/pdf = F*G1(wi).
    // Diffuse has no contribution here: native diffuse GI remains in place.
    float3 throughput=F*PTG1(nl,a2);
    bool finiteWeight=!any((asuint(throughput)&0x7f800000u)==0x7f800000u);
    return (nv>0.0f&&nl>0.0f&&finiteWeight)?max(throughput,0.0f):0.0f;
}
float3 PTNativePrimaryOrigin(uint2 pixel,out float lodNormalFactor) {
    float2 xy=(float2(pixel)+0.5f)*asfloat(ptCB0[1]).zw*float2(2,-2)+float2(-1,1);
    float z=g_tClipDepth.Load(int3(pixel,0)).x;
    float4 w=xy.x*asfloat(ptCB0[22])+xy.y*asfloat(ptCB0[23])+z*asfloat(ptCB0[24])+asfloat(ptCB0[25]);
    float3 world=w.xyz/w.w;
    float3 n=ControlNormalViewToWorld(ControlDecodeNormalView(g_tGBuffer1.Load(int3(pixel,0))),
        asfloat(ptCB0[6]).xyz,asfloat(ptCB0[7]).xyz,asfloat(ptCB0[8]).xyz);
    lodNormalFactor=pow(saturate(n.y*0.5f+0.5f),asfloat(ptCB5[1].y)*10.0f);
    float depth=g_tLinearDepth.Load(int3(pixel,0)).x;
    return world+(n-normalize(world-asfloat(ptCB0[54]).xyz))*(0.001f*depth);
}
float3 PTNativeOutput(float3 radiance,uint2 pixel) {
    float luminance=max(dot(radiance,float3(0.3333f,0.3333f,0.3333f)),1.0e-5f);
    float clampValue=asfloat(ptCB6[0].x);
    if(clampValue>=0.0f&&luminance>clampValue)radiance*=clampValue/luminance;
    float4 fade=asfloat(ptCB1[17]);
    float linearDepth=g_tLinearDepth.Load(int3(pixel,0)).x;
    float blend=saturate((linearDepth-(fade.x-fade.y))/max(2.0f*fade.y,1.0e-6f));
    float scale=fade.z+(fade.w-fade.z)*blend*blend*(3.0f-2.0f*blend);
    return max(radiance,0.0f)*max(scale,0.0f);
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
    uint privatePixel=pixel.y*size.x+pixel.x;
    if(privatePixel>=PT_PRIVATE_PIXEL_CAPACITY)return;
    PTPrivateHit hit2=g_ptPrivateHits[privatePixel*2u];
    PTPrivateHit hit3=g_ptPrivateHits[privatePixel*2u+1u];
    float4 secondPos=float4(PTWorldToView(hit2.worldPosition),0.0f);
    float4 normal=float4(PTUnpackNormal(hit2.packedNormal),(float)hit2.material);
    uint rejection=1u;
    float3 bounce=0.0f;
    float3 replacement=0.0f;
    float3 retained=0.0f;
    float3 reference=0.0f;
    float4 base=g_rwtReflectionTarget[pixel];
    if(first<65534u){
        rejection=5u;
        if(first<count){
            rejection=2u;
            if(hit2.material<65534u){
                uint second=hit2.material;
                rejection=5u;
                if(second<count){
                    rejection=3u;
                    float n2=dot(normal.xyz,normal.xyz);
                    if(PTFinite3(normal.xyz)&&n2>1.0e-6f){
                        rejection=4u;
                        float3 delta=secondPos.xyz-firstPos.xyz;
                        float d2=dot(delta,delta);
                        if(PTFinite3(firstPos.xyz)&&PTFinite3(secondPos.xyz)&&
                           PTFinite1(d2)&&d2>1.0e-6f){
                            rejection=0u;
                            float3 hitWorld=hit2.worldPosition;
                            float3 worldDelta=hitWorld-PTNativeViewToWorld(firstPos.xyz);
                            float3 incoming=normalize(worldDelta);
                            float3 n=normal.xyz*rsqrt(n2);
                            if(dot(n,incoming)>0.0f)n=-n;
                            float2 uv=hit2.uv;
                            float3 secondRetained;
                            float3 radiance=PTNativeSecondHit(second,uv,secondPos.xyz,hitWorld,
                                n,incoming,delta*rsqrt(d2),firstPos.xyz,pixel,false,1.0f,secondRetained);
                            // At vertex two, preserve diffuse GI/direct/emission, and
                            // replace its indirect specular lobe only when hit three is valid.
                            if(hit3.material<count&&hit3.material<65534u){
                                float3 thirdDelta=hit3.worldPosition-hitWorld;
                                float thirdD2=dot(thirdDelta,thirdDelta);
                                if(PTFinite3(hit3.worldPosition)&&thirdD2>1.0e-6f){
                                    float3 incoming3=thirdDelta*rsqrt(thirdD2);
                                    float3 normal3=PTUnpackNormal(hit3.packedNormal);
                                    if(dot(normal3,incoming3)>0.0f)normal3=-normal3;
                                    float3 view3=PTWorldToView(hit3.worldPosition);
                                    float3 ignoredRetained;
                                    float3 radiance3=PTNativeSecondHit(hit3.material,hit3.uv,view3,hit3.worldPosition,
                                        normal3,incoming3,normalize(view3-secondPos.xyz),secondPos.xyz,pixel,false,1.0f,ignoredRetained);
                                    float3 weight2=PTFirstHitThroughput(second,uv,n,-incoming,incoming3);
                                    float3 candidate=secondRetained+weight2*radiance3;
                                    if(PTFinite3(candidate))radiance=max(candidate,0.0f);
                                }
                            }
                            float4 firstNormal=g_tNormal_TexcoordX.Load(int4(pixel,0,0));
                            float3 primaryView=PTPrimaryView(pixel);
                            float3 firstIncomingView=normalize(firstPos.xyz-primaryView);
                            float3 firstIncoming=PTWorldDirection(firstIncomingView);
                            float3 firstN=normalize(firstNormal.xyz);
                            if(dot(firstN,firstIncoming)>0.0f)firstN=-firstN;
                            float2 firstUv=float2(firstNormal.w,firstPos.w);
                            float3 throughput=PTFirstHitThroughput(first,firstUv,firstN,-firstIncoming,incoming);
                            bounce=radiance*throughput;
                            float nativeLod;
                            float3 nativeOrigin=PTNativePrimaryOrigin(pixel,nativeLod);
                            float3 firstWorld=PTNativeViewToWorld(firstPos.xyz);
                            // Match the ORIGINAL first-hit evaluation for the reference:
                            // stored normal, native offset origin and camera-space light BRDF.
                            float3 nativeFull=PTNativeSecondHit(first,firstUv,firstPos.xyz,firstWorld,
                                firstNormal.xyz,normalize(firstWorld-nativeOrigin),normalize(firstPos.xyz),
                                primaryView,pixel,false,nativeLod,retained);
                            reference=PTNativeOutput(nativeFull,pixel);
                            replacement=PTNativeOutput(retained+bounce,pixel);
                            if(!PTFinite3(replacement)||!PTFinite3(reference))rejection=4u;
                            // A port mismatch must never erase native illumination.
                            // This is a parity admission check, not an intensity adjustment.
                            float3 tolerance=0.001f+0.05f*max(abs(base.rgb),abs(reference));
                            if(any(abs(reference-base.rgb)>tolerance))rejection=6u;


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
            rejection==4u?float3(0.02f,6.0f,6.0f):
            rejection==6u?float3(6.0f,1.5f,0.02f):float3(6.0f,6.0f,6.0f);
        g_rwtReflectionTarget[pixel]=float4(color,1.0f);return;
    }
    if(!right&&bottom){
        // Show the actual shaded secondary radiance after specular transport.
        g_rwtReflectionTarget[pixel]=float4(PTNativeOutput(bounce,pixel),base.a);return;
    }
    g_rwtReflectionTarget[pixel]=float4(PTNativeOutput(retained,pixel),base.a);return;
#else
    if(rejection!=0u)return;
    // Apply only the specular substitution delta to the actual native output.
    // Native diffuse, direct lighting and emission cancel from this difference.
    base.rgb=max(base.rgb+(replacement-reference),0.0f);
    g_rwtReflectionTarget[pixel]=base;

#endif
}
