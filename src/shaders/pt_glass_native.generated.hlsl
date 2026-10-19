// GENERATED from captured native glass lighting SHA256 b19de5e0b8a216076b6800790b100b91caa83814abdad3ad276736ba72b5e762
#include "pt_glass_lighting_direction.hlsli"
#include "pt_glass_radiance_merge.hlsli"
// R37: retain native first-hit lighting; path viewing direction applies only
// to the added continuation. Native indirect color is the conservative baseline.
SamplerState g_sLinearWrap : register(s5, space1);
SamplerState g_sLinearClamp : register(s6, space1);
SamplerState g_sLinearMipLinearWrap : register(s10, space1);
SamplerState g_sLinearMipLinearClamp : register(s11, space1);
SamplerState g_sMaterialArrayGlobal : register(s19, space1);
Texture2D<float4> g_tClipDepth : register(t0, space0);
struct PTNativeRaw1 { uint d[2]; };
StructuredBuffer<PTNativeRaw1> g_sbMaterialDataPart1 : register(t1, space0);
struct PTNativeRaw2 { uint d[84]; };
StructuredBuffer<PTNativeRaw2> g_sbMaterialDataPart3 : register(t2, space0);
Texture2D<float4> g_tAmbientSHTerms : register(t3, space0);
TextureCube<float4> g_sEnvironmentCube : register(t4, space0);
Texture2D<float4> g_tPointLightShadowAtlas : register(t5, space0);
struct PTNativeRaw6 { uint d[40]; };
StructuredBuffer<PTNativeRaw6> g_sbDeferredLightPointData : register(t6, space0);
Texture2D<float4> g_tSpotLightPCFShadowAtlas : register(t7, space0);
Texture2D<float4> g_tSpotLightVSMShadowAtlas : register(t8, space0);
Texture2D<float4> g_tSpotLightProjectionAtlas : register(t9, space0);
struct PTNativeRaw10 { uint d[80]; };
StructuredBuffer<PTNativeRaw10> g_sbDeferredLightSpotData : register(t10, space0);
Texture2D<float4> g_tEnvBRDF : register(t11, space0);
struct PTNativeRaw12 { uint d[20]; };
StructuredBuffer<PTNativeRaw12> g_sbActiveVolumeSamplingInfoArray : register(t12, space0);
struct PTNativeRaw13 { uint d[1]; };
StructuredBuffer<PTNativeRaw13> g_sbTopLevelNodeIndicesBuffer : register(t13, space0);
struct PTNativeRaw14 { uint d[4]; };
StructuredBuffer<PTNativeRaw14> g_sbInternalNodesBuffer : register(t14, space0);
Texture3D<float4> g_tGIBrickTextures[6] : register(t15, space0);
Texture2D<uint4> g_tRaytracingLightCellPointLights : register(t21, space0);
Texture2D<uint4> g_tRaytracingLightCellSpotLights : register(t22, space0);
Texture2D<float4> g_tTemporalBlueNoiseRGBA : register(t23, space0);
Texture2D<float4> g_tLinearDepth_HitT : register(t24, space0);
Texture2D<uint4> g_tMaterialId : register(t25, space0);
Texture2D<float4> g_tNormal_TexcoordX : register(t26, space0);
Texture2D<float4> g_tPosition_TexcoordY : register(t27, space0);
Texture2D<float4> g_sMaterialTextureArray[] : register(t0, space1);
RWTexture2D<float4> g_rwtRadiance : register(u0, space0);
cbuffer sys_constants : register(b0) { uint4 ptCB0[71]; };
cbuffer deferredlight_constants : register(b1) { uint4 ptCB1[26]; };
cbuffer env_general : register(b2) { uint4 ptCB2[1]; };
cbuffer illuminationvolume : register(b3) { uint4 ptCB3[3]; };
cbuffer ManualUpdateCB_ActiveVolumeTopLevelInfoArray : register(b4) { uint4 ptCB4[1536]; };
cbuffer distancefield_CS : register(b5) { uint4 ptCB5[2]; };
#ifndef PT_GLASS_SECONDARY_EVALUATION
#define PT_GLASS_SECONDARY_EVALUATION 0
#endif
#ifndef PT_GLASS_COMPOSITE
#define PT_GLASS_COMPOSITE 0
#endif
#if PT_GLASS_SECONDARY_EVALUATION
#include "pt_glass_shadow_filter.hlsli"
#endif
#if PT_GLASS_SECONDARY_EVALUATION || PT_GLASS_COMPOSITE
#include "pt_glass_secondary_layout.hlsli"
StructuredBuffer<PTGlassSecondaryHit> ptGlassSecondaryHits : register(t28, space0);
cbuffer PTGlassDispatch : register(b6) { uint ptGlassEpoch; uint ptGlassMaterialCount; uint2 ptGlassExtent; };
#endif
#if PT_GLASS_COMPOSITE
Texture2D<float4> ptGlassRadiance : register(t29);
#endif

uint4 PTBitExtract(uint4 width,uint4 offset,uint4 value) {
    width &= 31u; offset &= 31u;
    return (value>>offset)&((1u<<width)-1u);
}
uint4 PTBitInsert(uint4 width,uint4 offset,uint4 value,uint4 target) {
    width &= 31u; offset &= 31u;
    uint4 mask=((1u<<width)-1u)<<offset;
    return (target&~mask)|((value<<offset)&mask);
}
[numthreads(8,16,1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {

    uint4 r0=0u;
    uint4 r1=0u;
    uint4 r2=0u;
    uint4 r3=0u;
    uint4 r4=0u;
    uint4 r5=0u;
    uint4 r6=0u;
    uint4 r7=0u;
    uint4 r8=0u;
    uint4 r9=0u;
    uint4 r10=0u;
    uint4 r11=0u;
    uint4 r12=0u;
    uint4 r13=0u;
    uint4 r14=0u;
    uint4 r15=0u;
    uint4 r16=0u;
    uint4 r17=0u;
    uint4 r18=0u;
    uint4 r19=0u;
    uint4 r20=0u;
    uint4 r21=0u;
    uint4 r22=0u;
    uint4 r23=0u;
    uint4 r24=0u;
    uint4 r25=0u;
    uint4 r26=0u;
    uint4 r27=0u;
    r0.xy=(asuint((asfloat(ptCB0[0u].xyxx)+asfloat(uint4(0x40e00000u,0x41700000u,0x0u,0x0u))))).xy; // native dword 260
    r0.xy=(asuint((asfloat(r0.xyxx)*asfloat(uint4(0x3e000000u,0x3d800000u,0x0u,0x0u))))).xy; // native dword 272
    r0.xy=(uint4(asfloat(r0.xyxx))).xy; // native dword 282
    r0.y=((r0.yyyy<<(uint4(0x4u,0x4u,0x4u,0x4u)&31u))).y; // native dword 287
    r0.z=((r0.xxxx>>(uint4(0x4u,0x4u,0x4u,0x4u)&31u))).z; // native dword 294
    r0.w=((r0.xxxx*groupId.yyyy+groupId.xxxx)).w; // native dword 301
    uint4 divNumerator308=r0.wwww; // native dword 308
    uint4 divDenominator308=r0.yyyy; // native dword 308
    r1.x=((divDenominator308==0u)?uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu):(divNumerator308/max(divDenominator308,1u))).x; // native dword 308
    r2.x=((divDenominator308==0u)?uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu):(divNumerator308%max(divDenominator308,1u))).x; // native dword 308
    r0.y=((r0.xxxx&uint4(0xfu,0xfu,0xfu,0xfu))).y; // native dword 317
    r0.z=((uint4(r1.xxxx<r0.zzzz)*0xffffffffu)).z; // native dword 324
    r0.y=((r0.zzzz!=0u?uint4(0x10u,0x10u,0x10u,0x10u):r0.yyyy)).y; // native dword 331
    uint4 divNumerator340=r2.xxxx; // native dword 340
    uint4 divDenominator340=r0.yyyy; // native dword 340
    r2.x=((divDenominator340==0u)?uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu):(divNumerator340/max(divDenominator340,1u))).x; // native dword 340
    r3.x=((divDenominator340==0u)?uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu):(divNumerator340%max(divDenominator340,1u))).x; // native dword 340
    r0.y=((r1.xxxx<<(uint4(0x4u,0x4u,0x4u,0x4u)&31u))).y; // native dword 349
    r0.y=((r2.xxxx*r0.xxxx+r0.yyyy)).y; // native dword 356
    r0.y=((r3.xxxx+r0.yyyy)).y; // native dword 365
    uint4 divNumerator372=r0.yyyy; // native dword 372
    uint4 divDenominator372=r0.xxxx; // native dword 372
    r0.x=((divDenominator372==0u)?uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu):(divNumerator372/max(divDenominator372,1u))).x; // native dword 372
    r1.x=((divDenominator372==0u)?uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu):(divNumerator372%max(divDenominator372,1u))).x; // native dword 372
    r1.x=(PTBitInsert(uint4(0x1du,0x1du,0x1du,0x1du),uint4(0x3u,0x3u,0x3u,0x3u),r1.xxxx,groupThreadId.xxxx)).x; // native dword 381
    r1.y=(PTBitInsert(uint4(0x1cu,0x1cu,0x1cu,0x1cu),uint4(0x4u,0x4u,0x4u,0x4u),r0.xxxx,groupThreadId.yyyy)).y; // native dword 391
    r0.xy=(asuint(float4(r1.xyxx))).xy; // native dword 401
    r0.zw=((uint4(asfloat(r0.xxxy)>=asfloat(ptCB0[0u].xxxy))*0xffffffffu)).zw; // native dword 406
    r0.z=((r0.wwww|r0.zzzz)).z; // native dword 415
    if((r0.zzzz).x != 0u) { // native dword 422
    return; // native dword 425
    } // native dword 426
    r1.zw=(uint4(0x0u,0x0u,0x0u,0x0u)).zw; // native dword 427
#if PT_GLASS_SECONDARY_EVALUATION
    if (any(r1.xy >= ptGlassExtent)) return;
    g_rwtRadiance[r1.xy]=0.0f;
    PTGlassSecondaryHit ptHit=ptGlassSecondaryHits[r1.y*ptGlassExtent.x+r1.x];
    if (!PTGlassValidHit(ptHit,ptGlassEpoch,ptGlassMaterialCount)) return;
    r0=uint4(asuint(ptHit.incomingWorld_hitDistance.xyz),ptHit.metadata.x);
    r1.z=asuint(ptHit.incomingWorld_hitDistance.w*ptHit.incomingWorld_hitDistance.w);
    r2=asuint(ptHit.positionView_texcoordY);
    r3.xyz=asuint(asfloat(ptCB0[6]).xyz*ptHit.positionView_texcoordY.x+
        asfloat(ptCB0[7]).xyz*ptHit.positionView_texcoordY.y+
        asfloat(ptCB0[8]).xyz*ptHit.positionView_texcoordY.z+asfloat(ptCB0[9]).xyz);
    r4=asuint(ptHit.normalWorld_texcoordX);
#else
    r0.z=(asuint(g_tLinearDepth_HitT.Load(asint(r1.xyww).xyz))).yzxw.z; // native dword 435
    r0.w=((uint4(asfloat(uint4(0x471c4000u,0x471c4000u,0x471c4000u,0x471c4000u))<asfloat(r0.zzzz))*0xffffffffu)).w; // native dword 443
    if((r0.wwww).x != 0u) { // native dword 450
    return; // native dword 453
    } // native dword 454
    r0.xy=(asuint((asfloat(r0.xyxx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 455
    r0.xy=(asuint((asfloat(r0.xyxx)/asfloat(ptCB0[0u].xyxx)))).xy; // native dword 465
    r0.w=(asuint(g_tClipDepth.Load(asint(r1.xyww).xyz))).yzwx.w; // native dword 474
    r0.w=(asuint(saturate(asfloat(r0.wwww)))).w; // native dword 482
    r0.xy=(asuint(mad(asfloat(r0.xyxx),asfloat(uint4(0x40000000u,0xc0000000u,0x0u,0x0u)),asfloat(uint4(0xbf800000u,0x3f800000u,0x0u,0x0u))))).xy; // native dword 487
    r2.xyzw=(asuint((asfloat(r0.yyyy)*asfloat(ptCB0[23u].xyzw)))).xyzw; // native dword 502
    r2.xyzw=(asuint(mad(asfloat(ptCB0[22u].xyzw),asfloat(r0.xxxx),asfloat(r2.xyzw)))).xyzw; // native dword 511
    r2.xyzw=(asuint(mad(asfloat(ptCB0[24u].xyzw),asfloat(r0.wwww),asfloat(r2.xyzw)))).xyzw; // native dword 522
    r2.xyzw=(asuint((asfloat(r2.xyzw)+asfloat(ptCB0[25u].xyzw)))).xyzw; // native dword 533
    r0.x=(asuint((1.0f/asfloat(r2.wwww)))).x; // native dword 542
    r0.xyw=(asuint(mad(asfloat(r2.xyxz),asfloat(r0.xxxx),asfloat(asuint(-asfloat(ptCB0[54u].xyxz)))))).xyw; // native dword 547
    r2.x=(asuint(dot(asfloat(r0.xywx).xyz,asfloat(r0.xywx).xyz).xxxx)).x; // native dword 559
    r2.x=(asuint(rsqrt(asfloat(r2.xxxx)))).x; // native dword 566
    r0.xyw=(asuint((asfloat(r0.xyxw)*asfloat(r2.xxxx)))).xyw; // native dword 571
    r0.xyz=(asuint(mad(asfloat(r0.xywx),asfloat(r0.zzzz),asfloat(ptCB0[54u].xyzx)))).xyz; // native dword 578
    r0.w=(g_tMaterialId.Load(asint(r1.xyww).xyz)).yzwx.w; // native dword 589
    r2.xyzw=(asuint(g_tPosition_TexcoordY.Load(asint(r1.xyww).xyz))).xyzw.xyzw; // native dword 597
    r3.xyz=(asuint((asfloat(r2.yyyy)*asfloat(ptCB0[7u].xyzx)))).xyz; // native dword 605
    r3.xyz=(asuint(mad(asfloat(ptCB0[6u].xyzx),asfloat(r2.xxxx),asfloat(r3.xyzx)))).xyz; // native dword 614
    r3.xyz=(asuint(mad(asfloat(ptCB0[8u].xyzx),asfloat(r2.zzzz),asfloat(r3.xyzx)))).xyz; // native dword 625
    r3.xyz=(asuint((asfloat(r3.xyzx)+asfloat(ptCB0[9u].xyzx)))).xyz; // native dword 636
    r4.xyzw=(asuint(g_tNormal_TexcoordX.Load(asint(r1.xyzw).xyz))).xyzw.xyzw; // native dword 645
    r0.xyz=(asuint((asfloat(asuint(-asfloat(r0.xyzx)))+asfloat(r3.xyzx)))).xyz; // native dword 653
    r1.z=(asuint(dot(asfloat(r0.xyzx).xyz,asfloat(r0.xyzx).xyz).xxxx)).z; // native dword 661
    r1.w=(asuint(rsqrt(asfloat(r1.zzzz)))).w; // native dword 668
    r0.xyz=(asuint((asfloat(r0.xyzx)*asfloat(r1.wwww)))).xyz; // native dword 673
#endif
#if PT_GLASS_SECONDARY_EVALUATION
    float3 ptGlassLightView=PTGlassLightView(asfloat(r0.xyz),asfloat(ptCB0[2]).xyz,asfloat(ptCB0[3]).xyz,asfloat(ptCB0[4]).xyz);
#endif
    r5.xyzw=uint4(g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+0u],g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+1u],g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+2u],g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+3u]); // native dword 680
    r6.xy=uint2(g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u]); // native dword 690
    r7.xyz=uint3(g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x64u,0x64u,0x64u,0x64u)).x/4u+0u],g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x64u,0x64u,0x64u,0x64u)).x/4u+1u],g_sbMaterialDataPart3[(r0.wwww).x].d[(uint4(0x64u,0x64u,0x64u,0x64u)).x/4u+2u]); // native dword 700
    r1.w=((r0.wwww*uint4(0x23u,0x23u,0x23u,0x23u))).w; // native dword 710
    r6.zw=((r0.wwww*uint4(0x0u,0x0u,0x23u,0x23u)+uint4(0x0u,0x0u,0x2u,0x4u))).zw; // native dword 718
    r8.x=(r4.wwww).x; // native dword 733
    r8.y=(r2.wwww).y; // native dword 738
    r2.w=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r6.zzzz).x-0u)].SampleLevel(g_sMaterialArrayGlobal,asfloat(r8.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 743
    r4.w=(asuint((asfloat(asuint(-asfloat(r6.xxxx)))+asfloat(r6.yyyy)))).w; // native dword 758
    r6.y=(asuint(mad(asfloat(r2.wwww),asfloat(r4.wwww),asfloat(r6.xxxx)))).y; // native dword 766
    r2.w=(asuint(dot(asfloat(r0.xyzx).xyz,asfloat(r4.xyzx).xyz).xxxx)).w; // native dword 775
    r2.w=(asuint((asfloat(r2.wwww)+asfloat(r2.wwww)))).w; // native dword 782
    r9.xyz=(asuint(mad(asfloat(r4.xyzx),asfloat(asuint(-asfloat(r2.wwww))),asfloat(r0.xyzx)))).xyz; // native dword 789
    r2.w=(asuint(dot(asfloat(r9.xyzx).xyz,asfloat(r9.xyzx).xyz).xxxx)).w; // native dword 799
    r2.w=(asuint(rsqrt(asfloat(r2.wwww)))).w; // native dword 806
    r9.xyz=(asuint((asfloat(r2.wwww)*asfloat(r9.xyzx)))).xyz; // native dword 811
    r1.z=(asuint(sqrt(asfloat(r1.zzzz)))).z; // native dword 818
    r2.w=(asuint(mad(asfloat(r4.yyyy),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).w; // native dword 823
    r1.z=(asuint((asfloat(r1.zzzz)*asfloat(ptCB5[1u].xxxx)))).z; // native dword 832
    r4.w=(asuint((asfloat(ptCB5[1u].yyyy)*asfloat(uint4(0x41200000u,0x41200000u,0x41200000u,0x41200000u))))).w; // native dword 841
    r2.w=(asuint(log2(asfloat(r2.wwww)))).w; // native dword 850
    r2.w=(asuint((asfloat(r2.wwww)*asfloat(r4.wwww)))).w; // native dword 855
    r2.w=(asuint(exp2(asfloat(r2.wwww)))).w; // native dword 862
    r1.z=(asuint((asfloat(r1.zzzz)*asfloat(r2.wwww)))).z; // native dword 867
    r1.z=((uint4(asfloat(uint4(0x41800000u,0x41800000u,0x41800000u,0x41800000u))>=asfloat(r1.zzzz))*0xffffffffu)).z; // native dword 874
    r3.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 881
    r10.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 886
    r11.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 894
    r2.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 902
    r4.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 907
    r6.z=(uint4(0x0u,0x0u,0x0u,0x0u)).z; // native dword 912
    [loop] while(true) { // native dword 917
    r7.w=((uint4(r6.zzzz>=ptCB3[0u].wwww)*0xffffffffu)).w; // native dword 918
    if((r7.wwww).x != 0u) break; // native dword 927
    r7.w=((r6.zzzz<<(uint4(0x2u,0x2u,0x2u,0x2u)&31u))).w; // native dword 930
    r12.x=(asuint(dot(asfloat(ptCB4[(r7.wwww).x].xyzw).xyzw,asfloat(r3.xyzw).xyzw).xxxx)).x; // native dword 937
    r12.y=(asuint(dot(asfloat(ptCB4[(1u+(r7.wwww).x)].xyzw).xyzw,asfloat(r3.xyzw).xyzw).xxxx)).y; // native dword 947
    r12.z=(asuint(dot(asfloat(ptCB4[(2u+(r7.wwww).x)].xyzw).xyzw,asfloat(r3.xyzw).xyzw).xxxx)).z; // native dword 958
    r13.xyz=((uint4(asfloat(r12.xyzx)>=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).xyz; // native dword 969
    r8.z=((r13.yyyy&r13.xxxx)).z; // native dword 979
    r8.z=((r13.zzzz&r8.zzzz)).z; // native dword 986
    r13.xyz=(asuint(float4(ptCB4[(3u+(r7.wwww).x)].xyzx))).xyz; // native dword 993
    r14.xyz=((uint4(asfloat(r12.xyzx)<asfloat(r13.xyzx))*0xffffffffu)).xyz; // native dword 1002
    r8.w=((r14.yyyy&r14.xxxx)).w; // native dword 1009
    r8.w=((r14.zzzz&r8.wwww)).w; // native dword 1016
    r8.z=((r8.wwww&r8.zzzz)).z; // native dword 1023
    if((r8.zzzz).x != 0u) { // native dword 1030
    r14.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u]); // native dword 1033
    r15.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+2u]); // native dword 1043
    r16.xyzw=uint4(g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+2u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+3u]); // native dword 1053
    r17.xyzw=uint4(g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+2u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+3u]); // native dword 1063
    r18.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r6.zzzz).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+2u]); // native dword 1073
    r8.z=(asuint(saturate((asfloat(r12.xxxx)*asfloat(r16.wwww))))).z; // native dword 1083
    r13.xyz=(asuint((asfloat(asuint(-asfloat(r12.xyzx)))+asfloat(r13.xyzx)))).xyz; // native dword 1090
    r8.w=(asuint(saturate((asfloat(r16.wwww)*asfloat(r13.xxxx))))).w; // native dword 1098
    r8.z=(asuint((asfloat(r8.wwww)*asfloat(r8.zzzz)))).z; // native dword 1105
    r13.xw=(asuint(saturate((asfloat(r12.yyyz)*asfloat(r17.xxxy))))).xw; // native dword 1112
    r8.z=(asuint((asfloat(r8.zzzz)*asfloat(r13.xxxx)))).z; // native dword 1119
    r13.xy=(asuint(saturate((asfloat(r17.xyxx)*asfloat(r13.yzyy))))).xy; // native dword 1126
    r8.z=(asuint((asfloat(r8.zzzz)*asfloat(r13.xxxx)))).z; // native dword 1133
    r8.z=(asuint((asfloat(r13.wwww)*asfloat(r8.zzzz)))).z; // native dword 1140
    r8.z=(asuint((asfloat(r13.yyyy)*asfloat(r8.zzzz)))).z; // native dword 1147
    r8.w=((uint4(r18.xxxx!=uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu))*0xffffffffu)).w; // native dword 1154
    if((r8.wwww).x != 0u) { // native dword 1161
    r13.xyz=(asuint(floor(asfloat(r12.xyzx)))).xyz; // native dword 1164
    r12.xyz=(asuint((asfloat(r12.xyzx)+asfloat(asuint(-asfloat(r13.xyzx)))))).xyz; // native dword 1169
    r13.xyz=(uint4(asfloat(r13.xyzx))).xyz; // native dword 1177
    r8.w=((r13.zzzz*ptCB4[(3u+(r7.wwww).x)].yyyy+r13.yyyy)).w; // native dword 1182
    r7.w=((r8.wwww*ptCB4[(3u+(r7.wwww).x)].xxxx+r13.xxxx)).w; // native dword 1195
    r7.w=((r17.zzzz+r7.wwww)).w; // native dword 1208
    r7.w=g_sbTopLevelNodeIndicesBuffer[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 1215
    r13.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1225
    r17.xyz=(r12.xyzx).xyz; // native dword 1233
    r8.w=(r7.wwww).w; // native dword 1238
    r9.w=(r18.yyyy).w; // native dword 1243
    [loop] while(true) { // native dword 1248
    r10.w=((r8.wwww&uint4(0x80000000u,0x80000000u,0x80000000u,0x80000000u))).w; // native dword 1249
    r10.w=((uint4(r10.wwww!=uint4(0x0u,0x0u,0x0u,0x0u))*0xffffffffu)).w; // native dword 1256
    r11.w=((uint4(asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))>=asfloat(r9.wwww))*0xffffffffu)).w; // native dword 1263
    r10.w=((r10.wwww|r11.wwww)).w; // native dword 1270
    if((r10.wwww).x != 0u) { // native dword 1277
    r10.w=((r8.wwww&uint4(0x7fffffffu,0x7fffffffu,0x7fffffffu,0x7fffffffu))).w; // native dword 1280
    r10.w=((r18.xxxx+r10.wwww)).w; // native dword 1287
    r19.x=((r10.wwww&uint4(0x7fu,0x7fu,0x7fu,0x7fu))).x; // native dword 1294
    r19.y=(PTBitExtract(uint4(0x7u,0x7u,0x7u,0x7u),uint4(0x7u,0x7u,0x7u,0x7u),r10.wwww)).y; // native dword 1301
    r19.z=((r10.wwww>>(uint4(0xeu,0xeu,0xeu,0xeu)&31u))).z; // native dword 1310
    r19.xyz=((r19.xyzx<<(uint4(0x2u,0x2u,0x2u,0x0u)&31u))).xyz; // native dword 1317
    r19.xyz=(asuint(float4(r19.xyzx))).xyz; // native dword 1327
    r19.xyz=(asuint(mad(asfloat(r17.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(r19.xyzx)))).xyz; // native dword 1332
    r13.xyz=(asuint((asfloat(r19.xyzx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 1344
    break; // native dword 1354
    } // native dword 1355
    r10.w=((r17.wwww+r8.wwww)).w; // native dword 1356
    r19.xyzw=uint4(g_sbInternalNodesBuffer[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbInternalNodesBuffer[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbInternalNodesBuffer[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbInternalNodesBuffer[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 1363
    r20.xyz=(asuint((asfloat(r17.xyzx)*asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u))))).xyz; // native dword 1373
    r20.xyz=(asuint(floor(asfloat(r20.xyzx)))).xyz; // native dword 1383
    r21.xyz=(uint4(asfloat(r20.xyzx))).xyz; // native dword 1388
    r21.xz=(asuint(float4(r21.zzxz))).xz; // native dword 1393
    r10.w=((r21.yyyy*uint4(0x3u,0x3u,0x3u,0x3u))).w; // native dword 1398
    r10.w=(asuint(float4(r10.wwww))).w; // native dword 1406
    r10.w=(asuint(mad(asfloat(r21.xxxx),asfloat(uint4(0x41100000u,0x41100000u,0x41100000u,0x41100000u)),asfloat(r10.wwww)))).w; // native dword 1411
    r10.w=(asuint((asfloat(r21.zzzz)+asfloat(r10.wwww)))).w; // native dword 1420
    r10.w=(uint4(asfloat(r10.wwww))).w; // native dword 1427
    r11.w=((uint4(0x1u,0x1u,0x1u,0x1u)<<(r10.wwww&31u))).w; // native dword 1432
    r12.w=((r11.wwww&r19.xxxx)).w; // native dword 1439
    if((r12.wwww).x == 0u) { // native dword 1446
    r12.w=((r18.xxxx+r8.wwww)).w; // native dword 1449
    r21.x=((r12.wwww&uint4(0x7fu,0x7fu,0x7fu,0x7fu))).x; // native dword 1456
    r21.y=(PTBitExtract(uint4(0x7u,0x7u,0x7u,0x7u),uint4(0x7u,0x7u,0x7u,0x7u),r12.wwww)).y; // native dword 1463
    r21.z=((r12.wwww>>(uint4(0xeu,0xeu,0xeu,0xeu)&31u))).z; // native dword 1472
    r21.xyz=((r21.xyzx<<(uint4(0x2u,0x2u,0x2u,0x0u)&31u))).xyz; // native dword 1479
    r21.xyz=(asuint(float4(r21.xyzx))).xyz; // native dword 1489
    r21.xyz=(asuint(mad(asfloat(r17.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(r21.xyzx)))).xyz; // native dword 1494
    r13.xyz=(asuint((asfloat(r21.xyzx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 1506
    break; // native dword 1516
    } // native dword 1517
    r17.xyz=(asuint(mad(asfloat(r17.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(asuint(-asfloat(r20.xyzx)))))).xyz; // native dword 1518
    r11.w=((r11.wwww&r19.yyyy)).w; // native dword 1531
    r12.w=(PTBitExtract(r10.wwww,uint4(0x0u,0x0u,0x0u,0x0u),r19.yyyy)).w; // native dword 1538
    r12.w=(countbits(r12.wwww)).w; // native dword 1547
    r12.w=((r12.wwww+r19.wwww)).w; // native dword 1552
    r12.w=((r12.wwww|uint4(0x80000000u,0x80000000u,0x80000000u,0x80000000u))).w; // native dword 1559
    r13.w=((~r19.yyyy)).w; // native dword 1566
    r13.w=((r13.wwww&r19.xxxx)).w; // native dword 1571
    r10.w=(PTBitExtract(r10.wwww,uint4(0x0u,0x0u,0x0u,0x0u),r13.wwww)).w; // native dword 1578
    r10.w=(countbits(r10.wwww)).w; // native dword 1587
    r10.w=((r10.wwww+r19.zzzz)).w; // native dword 1592
    r8.w=((r11.wwww!=0u?r12.wwww:r10.wwww)).w; // native dword 1599
    r9.w=(asuint((asfloat(r9.wwww)*asfloat(uint4(0x3eaaaaabu,0x3eaaaaabu,0x3eaaaaabu,0x3eaaaaabu))))).w; // native dword 1608
    r13.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1615
    } // native dword 1623
    } else { // native dword 1624
    r13.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1625
    } // native dword 1633
    r12.xyz=(asuint((asfloat(r13.xyzx)*asfloat(ptCB3[0u].xyzx)))).xyz; // native dword 1634
    r17.x=(r14.xxxx).x; // native dword 1643
    r17.y=(r15.xxxx).y; // native dword 1648
    r17.z=(r16.xxxx).z; // native dword 1653
    r19.x=(asuint(dot(asfloat(r17.xyzx).xyz,asfloat(r4.xyzx).xyz).xxxx)).x; // native dword 1658
    r20.x=(r14.yyyy).x; // native dword 1665
    r20.y=(r15.yyyy).y; // native dword 1670
    r20.z=(r16.yyyy).z; // native dword 1675
    r19.y=(asuint(dot(asfloat(r20.xyzx).xyz,asfloat(r4.xyzx).xyz).xxxx)).y; // native dword 1680
    r16.x=(r14.zzzz).x; // native dword 1687
    r16.y=(r15.zzzz).y; // native dword 1692
    r19.z=(asuint(dot(asfloat(r16.xyzx).xyz,asfloat(r4.xyzx).xyz).xxxx)).z; // native dword 1697
    r14.x=(asuint(dot(asfloat(r17.xyzx).xyz,asfloat(r9.xyzx).xyz).xxxx)).x; // native dword 1704
    r14.y=(asuint(dot(asfloat(r20.xyzx).xyz,asfloat(r9.xyzx).xyz).xxxx)).y; // native dword 1711
    r14.z=(asuint(dot(asfloat(r16.xyzx).xyz,asfloat(r9.xyzx).xyz).xxxx)).z; // native dword 1718
    r15.xyz=(asuint(saturate(asfloat(r19.xyzx)))).xyz; // native dword 1725
    r15.xyz=(asuint((asfloat(r15.xyzx)*asfloat(r15.xyzx)))).xyz; // native dword 1730
    r19.xyz=(asuint(saturate(asfloat(asuint(-asfloat(r19.xyzx)))))).xyz; // native dword 1737
    r16.xyz=(asuint((asfloat(r19.xyzx)*asfloat(r19.xyzx)))).xyz; // native dword 1743
    r17.xyz=(asuint(saturate(asfloat(r14.xyzx)))).xyz; // native dword 1750
    r17.xyz=(asuint((asfloat(r17.xyzx)*asfloat(r17.xyzx)))).xyz; // native dword 1755
    r14.xyz=(asuint(saturate(asfloat(asuint(-asfloat(r14.xyzx)))))).xyz; // native dword 1762
    r14.xyz=(asuint((asfloat(r14.xyzx)*asfloat(r14.xyzx)))).xyz; // native dword 1768
    r19.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r15.xyzx))*0xffffffffu)).xyz; // native dword 1775
    r20.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r17.xyzx))*0xffffffffu)).xyz; // native dword 1785
    r19.xyz=((r19.xyzx|r20.xyzx)).xyz; // native dword 1795
    if((r19.xxxx).x != 0u) { // native dword 1802
    r20.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(15u-15u)].SampleLevel(g_sLinearClamp,asfloat(r12.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 1805
    r21.xyz=(asuint((asfloat(r15.xxxx)*asfloat(r20.xyzx)))).xyz; // native dword 1818
    r20.xyz=(asuint((asfloat(r17.xxxx)*asfloat(r20.xyzx)))).xyz; // native dword 1825
    } else { // native dword 1832
    r21.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1833
    r20.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1841
    } // native dword 1849
    r22.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r16.xyzx))*0xffffffffu)).xyz; // native dword 1850
    r23.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r14.xyzx))*0xffffffffu)).xyz; // native dword 1860
    r22.xyz=((r22.xyzx|r23.xyzx)).xyz; // native dword 1870
    if((r22.xxxx).x != 0u) { // native dword 1877
    r23.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(16u-15u)].SampleLevel(g_sLinearClamp,asfloat(r12.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 1880
    r21.xyz=(asuint(mad(asfloat(r23.xyzx),asfloat(r16.xxxx),asfloat(r21.xyzx)))).xyz; // native dword 1893
    r20.xyz=(asuint(mad(asfloat(r23.xyzx),asfloat(r14.xxxx),asfloat(r20.xyzx)))).xyz; // native dword 1902
    } // native dword 1911
    if((r19.yyyy).x != 0u) { // native dword 1912
    r19.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(17u-15u)].SampleLevel(g_sLinearClamp,asfloat(r12.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 1915
    r21.xyz=(asuint(mad(asfloat(r19.xywx),asfloat(r15.yyyy),asfloat(r21.xyzx)))).xyz; // native dword 1928
    r20.xyz=(asuint(mad(asfloat(r19.xywx),asfloat(r17.yyyy),asfloat(r20.xyzx)))).xyz; // native dword 1937
    } // native dword 1946
    if((r22.yyyy).x != 0u) { // native dword 1947
    r15.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(18u-15u)].SampleLevel(g_sLinearClamp,asfloat(r12.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 1950
    r21.xyz=(asuint(mad(asfloat(r15.xywx),asfloat(r16.yyyy),asfloat(r21.xyzx)))).xyz; // native dword 1963
    r20.xyz=(asuint(mad(asfloat(r15.xywx),asfloat(r14.yyyy),asfloat(r20.xyzx)))).xyz; // native dword 1972
    } // native dword 1981
    if((r19.zzzz).x != 0u) { // native dword 1982
    r14.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(19u-15u)].SampleLevel(g_sLinearClamp,asfloat(r12.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 1985
    r21.xyz=(asuint(mad(asfloat(r14.xywx),asfloat(r15.zzzz),asfloat(r21.xyzx)))).xyz; // native dword 1998
    r20.xyz=(asuint(mad(asfloat(r14.xywx),asfloat(r17.zzzz),asfloat(r20.xyzx)))).xyz; // native dword 2007
    } // native dword 2016
    if((r22.zzzz).x != 0u) { // native dword 2017
    r12.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(20u-15u)].SampleLevel(g_sLinearClamp,asfloat(r12.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2020
    r21.xyz=(asuint(mad(asfloat(r12.xyzx),asfloat(r16.zzzz),asfloat(r21.xyzx)))).xyz; // native dword 2033
    r20.xyz=(asuint(mad(asfloat(r12.xyzx),asfloat(r14.zzzz),asfloat(r20.xyzx)))).xyz; // native dword 2042
    } // native dword 2051
    r7.w=((uint4(r18.xxxx==uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu))*0xffffffffu)).w; // native dword 2052
    r12.xyz=((r7.wwww!=0u?uint4(0x3f800000u,0x3eb33333u,0x0u,0x0u):r21.xyzx)).xyz; // native dword 2059
    r14.xyz=((r7.wwww!=0u?uint4(0x3f800000u,0x3eb33333u,0x0u,0x0u):r20.xyzx)).xyz; // native dword 2071
    r7.w=(asuint(mad(asfloat(r8.zzzz),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 2083
    r8.w=(asuint((asfloat(r8.zzzz)*asfloat(r8.zzzz)))).w; // native dword 2092
    r7.w=(asuint((asfloat(r7.wwww)*asfloat(r8.wwww)))).w; // native dword 2099
    r8.w=(asuint((asfloat(r18.zzzz)+asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 2106
    r7.w=(asuint(mad(asfloat(r7.wwww),asfloat(r8.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 2113
    r8.w=(asuint((asfloat(r7.wwww)*asfloat(r8.zzzz)))).w; // native dword 2122
    r10.xyz=(asuint(mad(asfloat(r12.xyzx),asfloat(r8.wwww),asfloat(r10.xyzx)))).xyz; // native dword 2129
    r11.xyz=(asuint(mad(asfloat(r14.xyzx),asfloat(r8.wwww),asfloat(r11.xyzx)))).xyz; // native dword 2138
    r2.w=(asuint(mad(asfloat(r8.zzzz),asfloat(r7.wwww),asfloat(r2.wwww)))).w; // native dword 2147
    r4.w=(uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu)).w; // native dword 2156
    } // native dword 2161
    r6.z=((r6.zzzz+uint4(0x1u,0x1u,0x1u,0x1u))).z; // native dword 2162
    } // native dword 2169
    r1.z=((r1.zzzz&r4.wwww)).z; // native dword 2170
    if((r1.zzzz).x != 0u) { // native dword 2177
    r1.z=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r2.wwww))*0xffffffffu)).z; // native dword 2180
    r12.xyz=(asuint((asfloat(r10.xyzx)/asfloat(r2.wwww)))).xyz; // native dword 2187
    r13.xyz=(asuint((asfloat(r11.xyzx)/asfloat(r2.wwww)))).xyz; // native dword 2194
    r10.xyz=((r1.zzzz!=0u?r12.xyzx:r10.xyzx)).xyz; // native dword 2201
    r11.xyz=((r1.zzzz!=0u?r13.xyzx:r11.xyzx)).xyz; // native dword 2210
    r10.xyz=(asuint((asfloat(r10.xyzx)+asfloat(ptCB3[1u].xyzx)))).xyz; // native dword 2219
    r11.xyz=(asuint((asfloat(r11.xyzx)+asfloat(ptCB3[1u].xyzx)))).xyz; // native dword 2228
    r1.z=(asuint((asfloat(ptCB1[15u].wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).z; // native dword 2237
    r10.xyz=(asuint((asfloat(r10.xyzx)*asfloat(r1.zzzz)))).xyz; // native dword 2246
    r11.xyz=(asuint((asfloat(r11.xyzx)*asfloat(r1.zzzz)))).xyz; // native dword 2253
    } else { // native dword 2260
    r12.xyz=(asuint((asfloat(r4.yyyy)*asfloat(ptCB0[7u].xzyx)))).xyz; // native dword 2261
    r12.xyz=(asuint(mad(asfloat(ptCB0[6u].xzyx),asfloat(r4.xxxx),asfloat(r12.xyzx)))).xyz; // native dword 2270
    r12.xyz=(asuint(mad(asfloat(ptCB0[8u].xzyx),asfloat(r4.zzzz),asfloat(r12.xyzx)))).xyz; // native dword 2281
    r13.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x0u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2292
    r14.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x1u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2303
    r15.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x2u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2314
    r16.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x3u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2325
    r17.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x4u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2336
    r18.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x5u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2347
    r19.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x6u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2358
    r20.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x7u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2369
    r21.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x8u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 2380
    r16.xyz=(asuint((asfloat(r12.yyyy)*asfloat(r16.xyzx)))).xyz; // native dword 2391
    r14.xyz=(asuint(mad(asfloat(r14.xyzx),asfloat(r12.xxxx),asfloat(r16.xyzx)))).xyz; // native dword 2398
    r14.xyz=(asuint(mad(asfloat(r15.xyzx),asfloat(r12.zzzz),asfloat(r14.xyzx)))).xyz; // native dword 2407
    r14.xyz=(asuint((asfloat(r14.xyzx)*asfloat(uint4(0x3efa2a1cu,0x3efa2a1cu,0x3efa2a1cu,0x0u))))).xyz; // native dword 2416
    r13.xyz=(asuint(mad(asfloat(r13.xyzx),asfloat(uint4(0x3e906ebau,0x3e906ebau,0x3e906ebau,0x0u)),asfloat(r14.xyzx)))).xyz; // native dword 2426
    r14.xyz=(asuint((asfloat(r12.xxxx)*asfloat(r19.xyzx)))).xyz; // native dword 2438
    r15.xyz=(asuint((asfloat(r12.xxxx)*asfloat(r17.xyzx)))).xyz; // native dword 2445
    r15.xyz=(asuint((asfloat(r12.zzzz)*asfloat(r15.xyzx)))).xyz; // native dword 2452
    r14.xyz=(asuint(mad(asfloat(r14.xyzx),asfloat(r12.yyyy),asfloat(r15.xyzx)))).xyz; // native dword 2459
    r15.xyz=(asuint((asfloat(r12.yyyy)*asfloat(r18.xyzx)))).xyz; // native dword 2468
    r14.xyz=(asuint(mad(asfloat(r15.xyzx),asfloat(r12.zzzz),asfloat(r14.xyzx)))).xyz; // native dword 2475
    r13.xyz=(asuint(mad(asfloat(r14.xyzx),asfloat(uint4(0x3f8bd89du,0x3f8bd89du,0x3f8bd89du,0x0u)),asfloat(r13.xyzx)))).xyz; // native dword 2484
    r14.xyz=(asuint((asfloat(r21.xyzx)*asfloat(uint4(0x3f0bd89du,0x3f0bd89du,0x3f0bd89du,0x0u))))).xyz; // native dword 2496
    r8.zw=(asuint((asfloat(r12.yyyz)*asfloat(r12.yyyz)))).zw; // native dword 2506
    r1.z=(asuint(mad(asfloat(r12.xxxx),asfloat(r12.xxxx),asfloat(asuint(-asfloat(r8.zzzz)))))).z; // native dword 2513
    r12.xyz=(asuint(mad(asfloat(r14.xyzx),asfloat(r1.zzzz),asfloat(r13.xyzx)))).xyz; // native dword 2523
    r13.xyz=(asuint((asfloat(r20.xyzx)*asfloat(uint4(0x3ea17b0fu,0x3ea17b0fu,0x3ea17b0fu,0x0u))))).xyz; // native dword 2532
    r1.z=(asuint(mad(asfloat(r8.wwww),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).z; // native dword 2542
    r12.xyz=(asuint(mad(asfloat(r13.xyzx),asfloat(r1.zzzz),asfloat(r12.xyzx)))).xyz; // native dword 2551
    r10.xyz=(asuint((asfloat(r12.xyzx)*asfloat(ptCB1[15u].yyyy)))).xyz; // native dword 2560
    r1.z=(asuint((asfloat(asuint(-asfloat(r6.yyyy)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 2569
    r1.z=(asuint((asfloat(r1.zzzz)*asfloat(ptCB2[0u].yyyy)))).z; // native dword 2577
    r9.xyz=(asuint(g_sEnvironmentCube.SampleLevel(g_sLinearMipLinearWrap,asfloat(r9.xyzx).xyz,asfloat(r1.zzzz).x))).xyzw.xyz; // native dword 2586
    r11.xyz=(asuint((asfloat(r9.xyzx)*asfloat(ptCB1[15u].yyyy)))).xyz; // native dword 2599
    } // native dword 2608
    r1.z=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r6.wwww).x-0u)].SampleLevel(g_sMaterialArrayGlobal,asfloat(r8.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzxw.z; // native dword 2609
    r2.w=g_sbMaterialDataPart1[(r0.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 2624
    r3.w=((r2.wwww>>(uint4(0x18u,0x18u,0x18u,0x18u)&31u))).w; // native dword 2634
    r6.zw=(PTBitExtract(uint4(0x0u,0x0u,0x8u,0x8u),uint4(0x0u,0x0u,0x10u,0x8u),r2.wwww)).zw; // native dword 2641
    r9.xw=(asuint(float4(r3.wwww))).xw; // native dword 2656
    r9.yz=(asuint(float4(r6.zzwz))).yz; // native dword 2661
    r9.xyzw=(asuint((asfloat(r1.zzzz)*asfloat(r9.xyzw)))).xyzw; // native dword 2666
    r9.xyzw=(asuint((asfloat(r9.xyzw)*asfloat(uint4(0x3b808081u,0x3b808081u,0x3b808081u,0x3b808081u))))).xyzw; // native dword 2673
    r1.z=(asuint(dot(asfloat(r9.wyzw).xyz,asfloat(uint4(0x3e59b3d0u,0x3f371759u,0x3d93dd98u,0x0u)).xyz).xxxx)).z; // native dword 2683
    r6.x=(asuint(dot(asfloat(r4.xyzx).xyz,asfloat(asuint(-asfloat(r0.xyzx))).xyz).xxxx)).x; // native dword 2693
    r0.xyz=(asuint(g_tEnvBRDF.SampleLevel(g_sLinearClamp,asfloat(r6.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2701
    r9.xyzw=(asuint(mad(asfloat(r9.xyzw),asfloat(r0.xxxx),asfloat(r0.yyyy)))).xyzw; // native dword 2714
    r12.xyzw=(asuint((asfloat(asuint(-asfloat(r9.wyzw)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).xyzw; // native dword 2723
    r12.xyzw=(asuint((asfloat(r0.zzzz)*asfloat(r12.xyzw)))).xyzw; // native dword 2734
    r0.xyz=(asuint((asfloat(r3.yyyy)*asfloat(ptCB0[3u].xyzx)))).xyz; // native dword 2741
    r0.xyz=(asuint(mad(asfloat(ptCB0[2u].xyzx),asfloat(r3.xxxx),asfloat(r0.xyzx)))).xyz; // native dword 2750
    r0.xyz=(asuint(mad(asfloat(ptCB0[4u].xyzx),asfloat(r3.zzzz),asfloat(r0.xyzx)))).xyz; // native dword 2761
    r0.xyz=(asuint((asfloat(r0.xyzx)+asfloat(ptCB0[5u].xyzx)))).xyz; // native dword 2772
    r3.xyz=(asuint((asfloat(r4.yyyy)*asfloat(ptCB0[3u].xyzx)))).xyz; // native dword 2781
    r3.xyz=(asuint(mad(asfloat(ptCB0[2u].xyzx),asfloat(r4.xxxx),asfloat(r3.xyzx)))).xyz; // native dword 2790
    r3.xyz=(asuint(mad(asfloat(ptCB0[4u].xyzx),asfloat(r4.zzzz),asfloat(r3.xyzx)))).xyz; // native dword 2801
    r4.xy=((r1.xyxx&uint4(0xffu,0xffu,0x0u,0x0u))).xy; // native dword 2812
    r4.zw=(uint4(0x0u,0x0u,0x0u,0x0u)).zw; // native dword 2822
    r4.xyzw=(asuint(g_tTemporalBlueNoiseRGBA.Load(asint(r4.xyzw).xyz))).xyzw.xyzw; // native dword 2830
    r2.xyz=(asuint((asfloat(r2.xyzx)+asfloat(uint4(0x43160000u,0x42960000u,0x42960000u,0x0u))))).xyz; // native dword 2838
    r2.xyz=(asuint((asfloat(r2.xyzx)*asfloat(uint4(0x3d5a740eu,0x3d5a740eu,0x3d5a740eu,0x0u))))).xyz; // native dword 2848
    r2.xyz=(asuint(floor(asfloat(r2.xyzx)))).xyz; // native dword 2858
    r2.xyz=(asuint(max(asfloat(r2.xyzx),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).xyz; // native dword 2863
    r2.xyz=(asuint(min(asfloat(r2.xyzx),asfloat(uint4(0x41700000u,0x40e00000u,0x41300000u,0x0u))))).xyz; // native dword 2873
    r2.xyz=(asuint(int4(asfloat(r2.xyzx)))).xyz; // native dword 2883
    r13.x=((r2.xxxx*uint4(0xcu,0xcu,0xcu,0xcu)+r2.zzzz)).x; // native dword 2888
    r13.y=((r2.yyyy<<(uint4(0x8u,0x8u,0x8u,0x8u)&31u))).y; // native dword 2897
    r13.z=(uint4(0x0u,0x0u,0x0u,0x0u)).z; // native dword 2904
    r2.x=(g_tRaytracingLightCellPointLights.Load(asint(r13.xyzz).xyz)).xyzw.x; // native dword 2909
    r14.xyz=(asuint(mad(asfloat(r3.xyzx),asfloat(uint4(0x3d8f5c29u,0x3d8f5c29u,0x3d8f5c29u,0x0u)),asfloat(r0.xyzx)))).xyz; // native dword 2917
    r2.y=(asuint((asfloat(asuint(-asfloat(r6.yyyy)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).y; // native dword 2929
    r2.z=(asuint(dot(asfloat(asuint(-asfloat(r0.xyzx))).xyz,asfloat(asuint(-asfloat(r0.xyzx))).xyz).xxxx)).z; // native dword 2937
    r2.z=(asuint(rsqrt(asfloat(r2.zzzz)))).z; // native dword 2946
    r6.xzw=(asuint((asfloat(asuint(-asfloat(r0.xxyz)))*asfloat(r2.zzzz)))).xzw; // native dword 2951
#if PT_GLASS_SECONDARY_EVALUATION
    // Override only V, after native position/attenuation/shadow setup.
    if(all(isfinite(ptGlassLightView)))r6.xzw=asuint(ptGlassLightView);
#endif
    r2.z=(asuint(saturate(dot(asfloat(r3.xyzx).xyz,asfloat(r6.xzwx).xyz).xxxx))).z; // native dword 2959
    r2.w=(asuint((asfloat(asuint(-asfloat(r1.zzzz)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 2966
    r14.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 2974
    r15.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 2979
    r16.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 2987
    r3.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 2995
    r7.w=(r2.xxxx).w; // native dword 3000
    [loop] while(true) { // native dword 3005
    r8.z=((uint4(r7.wwww!=uint4(0xffffu,0xffffu,0xffffu,0xffffu))*0xffffffffu)).z; // native dword 3006
    r8.w=((uint4(asint(r3.wwww)<asint(uint4(0x100u,0x100u,0x100u,0x100u)))*0xffffffffu)).w; // native dword 3013
    r8.z=((r8.wwww&r8.zzzz)).z; // native dword 3020
    if((r8.zzzz).x == 0u) break; // native dword 3027
    r17.xyzw=uint4(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 3030
    r18.xyzw=uint4(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+2u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+3u]); // native dword 3040
    r19.xyzw=uint4(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+2u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+3u]); // native dword 3050
    r20.xyzw=uint4(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+2u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+3u]); // native dword 3060
    r8.z=g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x90u,0x90u,0x90u,0x90u)).x/4u+0u]; // native dword 3070
    r8.w=g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x9cu,0x9cu,0x9cu,0x9cu)).x/4u+0u]; // native dword 3080
    r17.xyz=(asuint((asfloat(asuint(-asfloat(r0.xyzx)))+asfloat(r17.xyzx)))).xyz; // native dword 3090
    r10.w=(asuint(dot(asfloat(r17.xyzx).xyz,asfloat(r17.xyzx).xyz).xxxx)).w; // native dword 3098
    r11.w=(asuint(rsqrt(asfloat(r10.wwww)))).w; // native dword 3105
    r21.xyz=(asuint((asfloat(r11.wwww)*asfloat(r17.xyzx)))).xyz; // native dword 3110
    r15.w=(asuint(sqrt(asfloat(r10.wwww)))).w; // native dword 3117
    r16.w=(asuint((asfloat(asuint(-asfloat(r15.wwww)))+asfloat(r18.wwww)))).w; // native dword 3122
    r18.w=(asuint((asfloat(r18.wwww)*asfloat(uint4(0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu))))).w; // native dword 3130
    r16.w=(asuint(saturate((asfloat(r16.wwww)/asfloat(r18.wwww))))).w; // native dword 3137
    r18.w=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 3144
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r16.wwww)))).w; // native dword 3153
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r18.wwww)))).w; // native dword 3160
    r18.w=(asuint((asfloat(r8.zzzz)*asfloat(r8.zzzz)))).w; // native dword 3167
    r21.w=(asuint(mad(asfloat(r8.zzzz),asfloat(r8.zzzz),asfloat(r10.wwww)))).w; // native dword 3174
    r21.w=(asuint(max(asfloat(r21.wwww),asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u))))).w; // native dword 3183
    r21.w=(asuint((asfloat(r21.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 3190
    r21.w=(asuint((1.0f/asfloat(r21.wwww)))).w; // native dword 3197
    r19.x=(asuint((asfloat(asuint(-asfloat(r19.xxxx)))+asfloat(r21.wwww)))).x; // native dword 3202
    r19.x=(asuint(max(asfloat(r19.xxxx),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).x; // native dword 3210
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r19.xxxx)))).w; // native dword 3217
    r18.xyz=(asuint((asfloat(r16.wwww)*asfloat(r18.xyzx)))).xyz; // native dword 3224
    r22.xy=((r8.wwww&uint4(0x4u,0x2u,0x0u,0x0u))).xy; // native dword 3231
    r23.x=(r19.wwww).x; // native dword 3241
    r23.yz=(r20.xxyx).yz; // native dword 3246
    r8.w=(asuint(dot(asfloat(asuint(-asfloat(r21.xyzx))).xyz,asfloat(r23.xyzx).xyz).xxxx)).w; // native dword 3251
    r8.w=(asuint(saturate(mad(asfloat(r8.wwww),asfloat(r19.yyyy),asfloat(r19.zzzz))))).w; // native dword 3259
    r8.w=(asuint((asfloat(r8.wwww)*asfloat(r8.wwww)))).w; // native dword 3268
    r19.xyz=(asuint((asfloat(r8.wwww)*asfloat(r18.xyzx)))).xyz; // native dword 3275
    r18.xyz=((r22.xxxx!=0u?r19.xyzx:r18.xyzx)).xyz; // native dword 3282
    if((r22.yyyy).x != 0u) { // native dword 3291
    r19.xy=uint2(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u]); // native dword 3294
    r22.xyz=uint3(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+2u]); // native dword 3304
    r23.xyz=uint3(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+2u]); // native dword 3314
    r24.xyw=uint3(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+2u]); // native dword 3324
    r25.xyzw=uint4(g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+0u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+1u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+2u],g_sbDeferredLightPointData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+3u]); // native dword 3334
    r26.x=(r20.wwww).x; // native dword 3344
    r26.y=(r22.xxxx).y; // native dword 3349
    r26.z=(r23.xxxx).z; // native dword 3354
    r26.w=(r24.xxxx).w; // native dword 3359
    r26.x=(asuint(dot(asfloat(r26.xyzw).xyzw,asfloat(r14.xyzw).xyzw).xxxx)).x; // native dword 3364
    r27.x=(r19.xxxx).x; // native dword 3371
    r27.y=(r22.yyyy).y; // native dword 3376
    r27.z=(r23.yyyy).z; // native dword 3381
    r27.w=(r24.yyyy).w; // native dword 3386
    r26.y=(asuint(dot(asfloat(r27.xyzw).xyzw,asfloat(r14.xyzw).xyzw).xxxx)).y; // native dword 3391
    r24.x=(r19.yyyy).x; // native dword 3398
    r24.y=(r22.zzzz).y; // native dword 3403
    r24.z=(r23.zzzz).z; // native dword 3408
    r26.z=(asuint(dot(asfloat(r24.xyzw).xyzw,asfloat(r14.xyzw).xyzw).xxxx)).z; // native dword 3413
    r8.w=(asuint(dot(asfloat(r26.xyzx).xyz,asfloat(r26.xyzx).xyz).xxxx)).w; // native dword 3420
    r8.w=(asuint(rsqrt(asfloat(r8.wwww)))).w; // native dword 3427
    r19.xyz=(asuint((asfloat(r8.wwww)*asfloat(r26.xyzx)))).xyz; // native dword 3432
    r22.xyzw=((uint4(asfloat(asuint(abs(asfloat(r19.yzxz))))<asfloat(asuint(abs(asfloat(r19.xxyy)))))*0xffffffffu)).xyzw; // native dword 3439
    r20.xyw=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r19.xyxz))*0xffffffffu)).xyw; // native dword 3448
    r22.xy=((r22.ywyy&r22.xzxx)).xy; // native dword 3458
    r20.xyw=((r20.xyxw!=0u?uint4(0x0u,0x2u,0x0u,0x4u):uint4(0x1u,0x3u,0x0u,0x5u))).xyw; // native dword 3465
    r8.w=((r22.yyyy!=0u?r20.yyyy:r20.wwww)).w; // native dword 3480
    r8.w=((r22.xxxx!=0u?r20.xxxx:r8.wwww)).w; // native dword 3489
    if((r8.wwww).x != 0u) { // native dword 3498
    r16.w=((uint4(r8.wwww!=uint4(0x1u,0x1u,0x1u,0x1u))*0xffffffffu)).w; // native dword 3501
    if((r16.wwww).x != 0u) { // native dword 3508
    r22.xyz=(asuint((asfloat(r19.xxyx)/asfloat(r19.yzzy)))).xyz; // native dword 3511
    r16.w=(asuint((asfloat(asuint(-asfloat(r19.zzzz)))/asfloat(r19.yyyy)))).w; // native dword 3518
    r23.y=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).y; // native dword 3526
    r19.xw=(asuint((asfloat(r19.xxxz)/asfloat(asuint(-asfloat(r19.yyyy)))))).xw; // native dword 3535
    r19.xw=(asuint(mad(asfloat(r19.xxxw),asfloat(uint4(0x3f000000u,0x0u,0x0u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x0u,0x0u,0x3f000000u))))).xw; // native dword 3543
    r24.xyz=((uint4(r8.wwww==uint4(0x2u,0x3u,0x4u,0x0u))*0xffffffffu)).xyz; // native dword 3558
    r22.xyz=(asuint(mad(asfloat(r22.xyzx),asfloat(uint4(0x3f000000u,0x3f000000u,0xbf000000u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 3568
    r16.w=(asuint((asfloat(r19.yyyy)/asfloat(asuint(-asfloat(r19.zzzz)))))).w; // native dword 3583
    r22.w=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).w; // native dword 3591
    r19.yz=((r24.zzzz!=0u?r22.yyzy:r22.yywy)).yz; // native dword 3600
    r19.xy=((r24.yyyy!=0u?r19.xwxx:r19.yzyy)).xy; // native dword 3609
    r23.x=(r22.xxxx).x; // native dword 3618
    r20.xy=((r24.xxxx!=0u?r23.xyxx:r19.xyxx)).xy; // native dword 3623
    } else { // native dword 3632
    r22.xy=(asuint((asfloat(r19.zyzz)/asfloat(asuint(-asfloat(r19.xxxx)))))).xy; // native dword 3633
    r20.xy=(asuint(mad(asfloat(r22.xyxx),asfloat(uint4(0x3f000000u,0xbf000000u,0x0u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 3641
    } // native dword 3656
    } else { // native dword 3657
    r16.w=(asuint((asfloat(asuint(-asfloat(r19.zzzz)))/asfloat(r19.xxxx)))).w; // native dword 3658
    r20.x=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).x; // native dword 3666
    r16.w=(asuint((asfloat(r19.yyyy)/asfloat(r19.xxxx)))).w; // native dword 3675
    r20.y=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).y; // native dword 3682
    } // native dword 3691
    r19.xy=(asuint((asfloat(r20.xyxx)+asfloat(uint4(0xbf000000u,0xbf000000u,0x0u,0x0u))))).xy; // native dword 3692
    r19.yz=(asuint(mad(asfloat(r19.xxyx),asfloat(r17.wwww),asfloat(uint4(0x0u,0x3f000000u,0x3f000000u,0x0u))))).yz; // native dword 3702
    r8.w=(asuint(float4(r8.wwww))).w; // native dword 3714
    r8.w=(asuint((asfloat(r8.wwww)*asfloat(uint4(0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu))))).w; // native dword 3719
    r19.x=(asuint(mad(asfloat(r19.yyyy),asfloat(uint4(0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu)),asfloat(r8.wwww)))).x; // native dword 3726
    r19.xy=(asuint(mad(asfloat(r19.xzxx),asfloat(r25.zwzz),asfloat(r25.xyxx)))).xy; // native dword 3735
    r19.xy=(asuint(g_tPointLightShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r19.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xy; // native dword 3744
    r8.w=(asuint((asfloat(r15.wwww)+asfloat(uint4(0xbd4ccccdu,0xbd4ccccdu,0xbd4ccccdu,0xbd4ccccdu))))).w; // native dword 3757
    r16.w=(asuint((asfloat(r20.zzzz)*asfloat(r8.wwww)))).w; // native dword 3764
    r17.w=(asuint(mad(asfloat(asuint(-asfloat(r19.xxxx))),asfloat(r19.xxxx),asfloat(r19.yyyy)))).w; // native dword 3771
    r17.w=(asuint(max(asfloat(r17.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 3781
    r8.w=(asuint(mad(asfloat(asuint(-asfloat(r8.wwww))),asfloat(r20.zzzz),asfloat(r19.xxxx)))).w; // native dword 3788
    r8.w=(asuint(mad(asfloat(r8.wwww),asfloat(r8.wwww),asfloat(r17.wwww)))).w; // native dword 3798
    r8.w=(asuint((asfloat(r17.wwww)/asfloat(r8.wwww)))).w; // native dword 3807
    r8.w=(asuint(min(asfloat(r8.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 3814
    r16.w=((uint4(asfloat(r16.wwww)<asfloat(r19.xxxx))*0xffffffffu)).w; // native dword 3821
    r8.w=((r16.wwww!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r8.wwww)).w; // native dword 3828
    r18.xyz=(asuint((asfloat(r8.wwww)*asfloat(r18.xyzx)))).xyz; // native dword 3837
    } // native dword 3844
    r8.w=(asuint(dot(asfloat(r21.xyzx).xyz,asfloat(r3.xyzx).xyz).xxxx)).w; // native dword 3845
    r16.w=(asuint((asfloat(r8.zzzz)*asfloat(r15.wwww)))).w; // native dword 3852
    r16.w=((uint4(asfloat(r16.wwww)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 3859
    r17.w=(asuint((asfloat(r18.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 3866
    r10.w=(asuint((asfloat(r10.wwww)*asfloat(uint4(0x40c90fdbu,0x40c90fdbu,0x40c90fdbu,0x40c90fdbu))))).w; // native dword 3873
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r8.wwww)))).w; // native dword 3880
    r8.z=(asuint((asfloat(r15.wwww)/asfloat(r8.zzzz)))).z; // native dword 3887
    r8.z=(asuint((asfloat(r8.zzzz)+asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).z; // native dword 3894
    r8.z=(asuint(saturate((asfloat(r8.zzzz)*asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)))))).z; // native dword 3901
    r8.z=(asuint(mad(asfloat(r8.zzzz),asfloat(uint4(0x40000000u,0x40000000u,0x40000000u,0x40000000u)),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).z; // native dword 3908
    r15.w=(asuint(mad(asfloat(asuint(-asfloat(r8.zzzz))),asfloat(r8.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 3917
    r15.w=(asuint(sqrt(asfloat(r15.wwww)))).w; // native dword 3927
    r18.w=(asuint((asfloat(asuint(-abs(asfloat(r8.zzzz))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 3932
    r18.w=(asuint(sqrt(asfloat(r18.wwww)))).w; // native dword 3940
    r19.x=(asuint(mad(asfloat(asuint(abs(asfloat(r8.zzzz)))),asfloat(uint4(0xbc996e30u,0xbc996e30u,0xbc996e30u,0xbc996e30u)),asfloat(uint4(0x3d981627u,0x3d981627u,0x3d981627u,0x3d981627u))))).x; // native dword 3945
    r19.x=(asuint(mad(asfloat(r19.xxxx),asfloat(asuint(abs(asfloat(r8.zzzz)))),asfloat(uint4(0xbe593484u,0xbe593484u,0xbe593484u,0xbe593484u))))).x; // native dword 3955
    r19.x=(asuint(mad(asfloat(r19.xxxx),asfloat(asuint(abs(asfloat(r8.zzzz)))),asfloat(uint4(0x3fc90da4u,0x3fc90da4u,0x3fc90da4u,0x3fc90da4u))))).x; // native dword 3965
    r19.y=(asuint((asfloat(r18.wwww)*asfloat(r19.xxxx)))).y; // native dword 3975
    r19.y=(asuint(mad(asfloat(r19.yyyy),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).y; // native dword 3982
    r19.z=((uint4(asfloat(r8.zzzz)<asfloat(asuint(-asfloat(r8.zzzz))))*0xffffffffu)).z; // native dword 3991
    r19.y=((r19.zzzz&r19.yyyy)).y; // native dword 3999
    r18.w=(asuint(mad(asfloat(r19.xxxx),asfloat(r18.wwww),asfloat(r19.yyyy)))).w; // native dword 4006
    r18.w=(asuint((asfloat(asuint(-asfloat(r18.wwww)))+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).w; // native dword 4015
    r8.z=(asuint(mad(asfloat(asuint(-asfloat(r15.wwww))),asfloat(r8.zzzz),asfloat(asuint(-asfloat(r18.wwww)))))).z; // native dword 4023
    r8.z=(asuint((asfloat(r8.zzzz)+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).z; // native dword 4034
    r8.z=(asuint(saturate((asfloat(r8.zzzz)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u)))))).z; // native dword 4041
    r8.z=(asuint((asfloat(r8.zzzz)*asfloat(r17.wwww)))).z; // native dword 4048
    r8.z=(asuint((asfloat(r8.zzzz)/asfloat(r10.wwww)))).z; // native dword 4055
    r8.z=(asuint(min(asfloat(r8.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4062
    r10.w=((uint4(asfloat(uint4(0x39aec33fu,0x39aec33fu,0x39aec33fu,0x39aec33fu))<asfloat(r8.zzzz))*0xffffffffu)).w; // native dword 4069
    r8.z=(asuint(mad(asfloat(r8.zzzz),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4076
    r8.z=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r8.zzzz)))).z; // native dword 4085
    r15.w=(asuint((asfloat(r8.zzzz)*asfloat(r8.zzzz)))).w; // native dword 4095
    r15.w=(asuint((asfloat(r8.zzzz)*asfloat(r15.wwww)))).w; // native dword 4102
    r15.w=(asuint(mad(asfloat(r8.zzzz),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(asuint(-asfloat(r15.wwww)))))).w; // native dword 4109
    r8.z=(asuint(mad(asfloat(asuint(-asfloat(r8.zzzz))),asfloat(r8.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4119
    r8.z=(asuint((asfloat(r15.wwww)/asfloat(r8.zzzz)))).z; // native dword 4129
    r8.z=(asuint((asfloat(r8.zzzz)+asfloat(r8.zzzz)))).z; // native dword 4136
    r8.z=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r8.zzzz)))).z; // native dword 4143
    r8.z=(asuint(mad(asfloat(r2.yyyy),asfloat(r2.yyyy),asfloat(r8.zzzz)))).z; // native dword 4153
    r8.z=(asuint(sqrt(asfloat(r8.zzzz)))).z; // native dword 4162
    r8.z=((r10.wwww!=0u?r8.zzzz:r2.yyyy)).z; // native dword 4167
    r8.z=(asuint((asfloat(asuint(-asfloat(r8.zzzz)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4176
    r8.z=((r16.wwww!=0u?r8.zzzz:r6.yyyy)).z; // native dword 4184
    r17.xyz=(asuint(mad(asfloat(r17.xyzx),asfloat(r11.wwww),asfloat(r6.xzwx)))).xyz; // native dword 4193
    r10.w=(asuint(dot(asfloat(r17.xyzx).xyz,asfloat(r17.xyzx).xyz).xxxx)).w; // native dword 4202
    r10.w=(asuint(rsqrt(asfloat(r10.wwww)))).w; // native dword 4209
    r17.xyz=(asuint((asfloat(r10.wwww)*asfloat(r17.xyzx)))).xyz; // native dword 4214
    r10.w=(asuint(dot(asfloat(r6.xzwx).xyz,asfloat(r17.xyzx).xyz).xxxx)).w; // native dword 4221
    r11.w=(asuint(dot(asfloat(r3.xyzx).xyz,asfloat(r17.xyzx).xyz).xxxx)).w; // native dword 4228
    r8.z=(asuint((asfloat(asuint(-asfloat(r8.zzzz)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4235
    r8.z=(asuint(max(asfloat(r8.zzzz),asfloat(uint4(0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu))))).z; // native dword 4243
    r8.z=(asuint(min(asfloat(r8.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4250
    r8.z=(asuint((asfloat(r8.zzzz)*asfloat(r8.zzzz)))).z; // native dword 4257
    r10.w=(asuint((asfloat(asuint(-abs(asfloat(r10.wwww))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4264
    r10.w=(asuint(max(asfloat(r10.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 4272
    r15.w=(asuint((asfloat(r10.wwww)*asfloat(r10.wwww)))).w; // native dword 4279
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r15.wwww)))).w; // native dword 4286
    r10.w=(asuint((asfloat(r10.wwww)*asfloat(r15.wwww)))).w; // native dword 4293
    r10.w=(asuint(mad(asfloat(r2.wwww),asfloat(r10.wwww),asfloat(r1.zzzz)))).w; // native dword 4300
    r15.w=(asuint((asfloat(r8.zzzz)*asfloat(r8.zzzz)))).w; // native dword 4309
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(r11.wwww)))).w; // native dword 4316
    r16.w=(asuint(mad(asfloat(r8.zzzz),asfloat(r8.zzzz),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 4323
    r11.w=(asuint(mad(asfloat(r11.wwww),asfloat(r16.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4332
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(r11.wwww)))).w; // native dword 4341
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 4348
    r11.w=(asuint(max(asfloat(r11.wwww),asfloat(uint4(0x3a83126fu,0x3a83126fu,0x3a83126fu,0x3a83126fu))))).w; // native dword 4355
    r11.w=(asuint((asfloat(r15.wwww)/asfloat(r11.wwww)))).w; // native dword 4362
    r8.w=(asuint(saturate(asfloat(r8.wwww)))).w; // native dword 4369
    r8.z=(asuint(mad(asfloat(asuint(-asfloat(r8.zzzz))),asfloat(r8.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4374
    r16.w=(asuint(mad(asfloat(r8.wwww),asfloat(r8.zzzz),asfloat(r15.wwww)))).w; // native dword 4384
    r8.z=(asuint(mad(asfloat(r2.zzzz),asfloat(r8.zzzz),asfloat(r15.wwww)))).z; // native dword 4393
    r8.z=(asuint((asfloat(r8.zzzz)*asfloat(r16.wwww)))).z; // native dword 4402
    r8.z=(asuint(max(asfloat(r8.zzzz),asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u))))).z; // native dword 4409
    r8.z=(asuint((asfloat(uint4(0x3e800000u,0x3e800000u,0x3e800000u,0x3e800000u))/asfloat(r8.zzzz)))).z; // native dword 4416
    r15.w=(asuint(saturate((asfloat(asuint(-asfloat(r10.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)))))).w; // native dword 4423
    r15.w=(asuint((asfloat(r8.wwww)*asfloat(r15.wwww)))).w; // native dword 4431
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).w; // native dword 4438
    r10.w=(asuint((asfloat(r10.wwww)*asfloat(r11.wwww)))).w; // native dword 4445
    r8.z=(asuint((asfloat(r8.zzzz)*asfloat(r10.wwww)))).z; // native dword 4452
    r8.z=(asuint((asfloat(r8.wwww)*asfloat(r8.zzzz)))).z; // native dword 4459
    r15.xyz=(asuint(mad(asfloat(r15.wwww),asfloat(r18.xyzx),asfloat(r15.xyzx)))).xyz; // native dword 4466
    r16.xyz=(asuint(mad(asfloat(r8.zzzz),asfloat(r18.xyzx),asfloat(r16.xyzx)))).xyz; // native dword 4475
    r3.w=((r3.wwww+uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 4484
    r13.w=((r3.wwww+r13.yyyy)).w; // native dword 4491
    r7.w=(g_tRaytracingLightCellPointLights.Load(asint(r13.xwzz).xyz)).yzwx.w; // native dword 4498
    } // native dword 4506
    r2.x=(g_tRaytracingLightCellSpotLights.Load(asint(r13.xyzz).xyz)).xyzw.x; // native dword 4507
    r14.xyz=(asuint((asfloat(r0.zzzz)*asfloat(r3.xyzx)))).xyz; // native dword 4515
    r14.xyz=(asuint(mad(asfloat(r14.xyzx),asfloat(uint4(0x399d4952u,0x399d4952u,0x399d4952u,0x0u)),asfloat(r0.xyzx)))).xyz; // native dword 4522
    r17.xzw=(r13.xxzz).xzw; // native dword 4534
    r14.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 4539
    r13.xzw=(r15.xxyz).xzw; // native dword 4544
    r18.xyz=(r16.xyzx).xyz; // native dword 4549
    r3.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 4554
    r7.w=(r2.xxxx).w; // native dword 4559
    [loop] while(true) { // native dword 4564
    r8.z=((uint4(r7.wwww!=uint4(0xffffu,0xffffu,0xffffu,0xffffu))*0xffffffffu)).z; // native dword 4565
    r8.w=((uint4(asint(r3.wwww)<asint(uint4(0x100u,0x100u,0x100u,0x100u)))*0xffffffffu)).w; // native dword 4572
    r8.z=((r8.wwww&r8.zzzz)).z; // native dword 4579
    if((r8.zzzz).x == 0u) break; // native dword 4586
    r3.w=((r3.wwww+uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 4589
    r17.y=((r3.wwww+r13.yyyy)).y; // native dword 4596
    r8.z=(g_tRaytracingLightCellSpotLights.Load(asint(r17.xyzw).xyz)).yzxw.z; // native dword 4603
    r19.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+3u]); // native dword 4611
    r20.xyz=uint3(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+2u]); // native dword 4621
    r21.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+3u]); // native dword 4631
    r22.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+3u]); // native dword 4641
    r23.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+3u]); // native dword 4651
    r24.x=(r19.wwww).x; // native dword 4661
    r24.y=(r21.xxxx).y; // native dword 4666
    r24.z=(r22.xxxx).z; // native dword 4671
    r24.w=(r23.xxxx).w; // native dword 4676
    r24.x=(asuint(dot(asfloat(r24.xyzw).xyzw,asfloat(r14.xyzw).xyzw).xxxx)).x; // native dword 4681
    r25.x=(r20.xxxx).x; // native dword 4688
    r25.y=(r21.yyyy).y; // native dword 4693
    r25.z=(r22.yyyy).z; // native dword 4698
    r25.w=(r23.yyyy).w; // native dword 4703
    r24.y=(asuint(dot(asfloat(r25.xyzw).xyzw,asfloat(r14.xyzw).xyzw).xxxx)).y; // native dword 4708
    r25.x=(r20.yyyy).x; // native dword 4715
    r25.y=(r21.zzzz).y; // native dword 4720
    r25.z=(r22.zzzz).z; // native dword 4725
    r25.w=(r23.zzzz).w; // native dword 4730
    r24.z=(asuint(dot(asfloat(r25.xyzw).xyzw,asfloat(r14.xyzw).xyzw).xxxx)).z; // native dword 4735
    r23.x=(r20.zzzz).x; // native dword 4742
    r23.y=(r21.wwww).y; // native dword 4747
    r23.z=(r22.wwww).z; // native dword 4752
    r8.w=(asuint(dot(asfloat(r23.xyzw).xyzw,asfloat(r14.xyzw).xyzw).xxxx)).w; // native dword 4757
    r10.w=(asuint((1.0f/asfloat(r8.wwww)))).w; // native dword 4764
    r20.xyz=(asuint((asfloat(r10.wwww)*asfloat(r24.xyzx)))).xyz; // native dword 4769
    r21.xyz=(asuint(mad(asfloat(r20.xyzx),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f800000u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xyz; // native dword 4776
    r22.xyz=(asuint(saturate(asfloat(r21.xyzx)))).xyz; // native dword 4791
    r21.xyz=((uint4(asfloat(r21.xyzx)==asfloat(r22.xyzx))*0xffffffffu)).xyz; // native dword 4796
    r10.w=((r21.yyyy&r21.xxxx)).w; // native dword 4803
    r10.w=((r21.zzzz&r10.wwww)).w; // native dword 4810
    if((r10.wwww).x != 0u) { // native dword 4817
    r21.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 4820
    r10.w=g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x9cu,0x9cu,0x9cu,0x9cu)).x/4u+0u]; // native dword 4830
    r22.xy=uint2(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0xc4u,0xc4u,0xc4u,0xc4u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0xc4u,0xc4u,0xc4u,0xc4u)).x/4u+1u]); // native dword 4840
    r23.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+3u]); // native dword 4850
    r11.w=g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x130u,0x130u,0x130u,0x130u)).x/4u+0u]; // native dword 4860
    r22.zw=(asuint(mad(asfloat(r20.xxxy),asfloat(uint4(0x0u,0x0u,0x3f000000u,0xbf000000u)),asfloat(uint4(0x0u,0x0u,0x3f000000u,0x3f000000u))))).zw; // native dword 4870
    r24.xy=((r11.wwww&uint4(0x2u,0x300u,0x0u,0x0u))).xy; // native dword 4885
    if((r24.xxxx).x != 0u) { // native dword 4895
    r25.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+3u]); // native dword 4898
    r25.xy=(asuint(mad(asfloat(r22.zwzz),asfloat(r25.zwzz),asfloat(r25.xyxx)))).xy; // native dword 4908
    r25.xy=(asuint(g_tSpotLightVSMShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r25.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xy; // native dword 4917
    r26.xyz=(asuint((asfloat(asuint(-asfloat(r0.xyzx)))+asfloat(r21.xyzx)))).xyz; // native dword 4930
    r15.w=(asuint(dot(asfloat(r26.xyzx).xyz,asfloat(r26.xyzx).xyz).xxxx)).w; // native dword 4938
    r15.w=(asuint(sqrt(asfloat(r15.wwww)))).w; // native dword 4945
    r15.w=(asuint((asfloat(asuint(-asfloat(r21.wwww)))+asfloat(r15.wwww)))).w; // native dword 4950
    r16.w=(asuint((asfloat(r10.wwww)*asfloat(r15.wwww)))).w; // native dword 4958
    r17.y=(asuint(mad(asfloat(asuint(-asfloat(r25.xxxx))),asfloat(r25.xxxx),asfloat(r25.yyyy)))).y; // native dword 4965
    r17.y=(asuint(max(asfloat(r17.yyyy),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).y; // native dword 4975
    r15.w=(asuint(mad(asfloat(asuint(-asfloat(r15.wwww))),asfloat(r10.wwww),asfloat(r25.xxxx)))).w; // native dword 4982
    r15.w=(asuint(mad(asfloat(r15.wwww),asfloat(r15.wwww),asfloat(r17.yyyy)))).w; // native dword 4992
    r15.w=(asuint((asfloat(r17.yyyy)/asfloat(r15.wwww)))).w; // native dword 5001
    r15.w=(asuint(min(asfloat(r15.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 5008
    r16.w=((uint4(asfloat(r16.wwww)<asfloat(r25.xxxx))*0xffffffffu)).w; // native dword 5015
    r15.w=((r16.wwww!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r15.wwww)).w; // native dword 5022
    } else { // native dword 5031
    r15.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 5032
    } // native dword 5037
    r25.xy=((uint4(r24.yyyy==uint4(0x100u,0x200u,0x0u,0x0u))*0xffffffffu)).xy; // native dword 5038
    if((r25.xxxx).x != 0u) { // native dword 5048
#if PT_GLASS_SECONDARY_EVALUATION
    // R38: stable comparison-filtered shadows on the continuation only.
    r16.w=asuint(PTGlassStableShadow(g_tSpotLightPCFShadowAtlas,
        asfloat(r22.zw)*asfloat(r23.zw)+asfloat(r23.xy),
        asfloat(r20.z),asfloat(r22.x),asfloat(r23)));
#else
    r25.xz=(asuint(mad(asfloat(r22.zzwz),asfloat(r23.zzwz),asfloat(r23.xxyx)))).xz; // native dword 5051
    r26.xyzw=(asuint((asfloat(r4.xyzw)*asfloat(r23.zwzw)))).xyzw; // native dword 5060
    r26.xyzw=(asuint((asfloat(r22.xxxx)*asfloat(r26.xyzw)))).xyzw; // native dword 5067
    r27.xy=(asuint(mad(asfloat(r26.xyxx),asfloat(uint4(0x3f78d4feu,0x3f78d4feu,0x0u,0x0u)),asfloat(r25.xzxx)))).xy; // native dword 5074
    r27.xy=(asuint(mad(asfloat(r26.zwzz),asfloat(uint4(0x3e5cac08u,0x3e5cac08u,0x0u,0x0u)),asfloat(r27.xyxx)))).xy; // native dword 5086
    r16.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r27.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5098
    r16.w=((uint4(asfloat(r16.wwww)>=asfloat(r20.zzzz))*0xffffffffu)).w; // native dword 5111
    r16.w=((r16.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5118
    r27.xy=(asuint(mad(asfloat(r26.xyxx),asfloat(uint4(0xbe6e147bu,0xbe6e147bu,0x0u,0x0u)),asfloat(r25.xzxx)))).xy; // native dword 5125
    r27.xy=(asuint(mad(asfloat(r26.zwzz),asfloat(uint4(0x3df837b5u,0x3df837b5u,0x0u,0x0u)),asfloat(r27.xyxx)))).xy; // native dword 5137
    r17.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r27.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 5149
    r17.y=((uint4(asfloat(r17.yyyy)>=asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5162
    r17.y=((r17.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 5169
    r16.w=(asuint((asfloat(r16.wwww)+asfloat(r17.yyyy)))).w; // native dword 5176
    r27.xy=(asuint(mad(asfloat(r26.xyxx),asfloat(uint4(0xbf09d495u,0xbf09d495u,0x0u,0x0u)),asfloat(r25.xzxx)))).xy; // native dword 5183
    r27.xy=(asuint(mad(asfloat(r26.zwzz),asfloat(uint4(0xbf355326u,0xbf355326u,0x0u,0x0u)),asfloat(r27.xyxx)))).xy; // native dword 5195
    r17.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r27.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 5207
    r17.y=((uint4(asfloat(r17.yyyy)>=asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5220
    r17.y=((r17.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 5227
    r16.w=(asuint((asfloat(r16.wwww)+asfloat(r17.yyyy)))).w; // native dword 5234
    r27.xy=(asuint(mad(asfloat(r26.xyxx),asfloat(uint4(0x3cf69446u,0x3cf69446u,0x0u,0x0u)),asfloat(r25.xzxx)))).xy; // native dword 5241
    r27.xy=(asuint(mad(asfloat(r26.zwzz),asfloat(uint4(0xbecf837bu,0xbecf837bu,0x0u,0x0u)),asfloat(r27.xyxx)))).xy; // native dword 5253
    r17.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r27.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 5265
    r17.y=((uint4(asfloat(r17.yyyy)>=asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5278
    r17.y=((r17.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 5285
    r16.w=(asuint((asfloat(r16.wwww)+asfloat(r17.yyyy)))).w; // native dword 5292
    r27.xy=(asuint(mad(asfloat(r26.xyxx),asfloat(uint4(0x3de66666u,0x3de66666u,0x0u,0x0u)),asfloat(r25.xzxx)))).xy; // native dword 5299
    r27.xy=(asuint(mad(asfloat(r26.zwzz),asfloat(uint4(0x3cdc5d64u,0x3cdc5d64u,0x0u,0x0u)),asfloat(r27.xyxx)))).xy; // native dword 5311
    r17.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r27.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 5323
    r17.y=((uint4(asfloat(r17.yyyy)>=asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5336
    r17.y=((r17.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 5343
    r16.w=(asuint((asfloat(r16.wwww)+asfloat(r17.yyyy)))).w; // native dword 5350
    r27.xy=(asuint(mad(asfloat(r26.xyxx),asfloat(uint4(0x3de425afu,0x3de425afu,0x0u,0x0u)),asfloat(r25.xzxx)))).xy; // native dword 5357
    r27.xy=(asuint(mad(asfloat(r26.zwzz),asfloat(uint4(0x3f30ff97u,0x3f30ff97u,0x0u,0x0u)),asfloat(r27.xyxx)))).xy; // native dword 5369
    r17.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r27.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 5381
    r17.y=((uint4(asfloat(r17.yyyy)>=asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5394
    r17.y=((r17.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 5401
    r16.w=(asuint((asfloat(r16.wwww)+asfloat(r17.yyyy)))).w; // native dword 5408
    r27.xy=(asuint(mad(asfloat(r26.xyxx),asfloat(uint4(0xbf0f62b7u,0xbf0f62b7u,0x0u,0x0u)),asfloat(r25.xzxx)))).xy; // native dword 5415
    r27.xy=(asuint(mad(asfloat(r26.zwzz),asfloat(uint4(0xbe92d773u,0xbe92d773u,0x0u,0x0u)),asfloat(r27.xyxx)))).xy; // native dword 5427
    r17.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r27.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 5439
    r17.y=((uint4(asfloat(r17.yyyy)>=asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5452
    r17.y=((r17.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 5459
    r16.w=(asuint((asfloat(r16.wwww)+asfloat(r17.yyyy)))).w; // native dword 5466
    r25.xz=(asuint(mad(asfloat(r26.xxyx),asfloat(uint4(0x3ed53261u,0x0u,0x3ed53261u,0x0u)),asfloat(r25.xxzx)))).xz; // native dword 5473
    r25.xz=(asuint(mad(asfloat(r26.zzwz),asfloat(uint4(0xbe903afbu,0x0u,0xbe903afbu,0x0u)),asfloat(r25.xxzx)))).xz; // native dword 5485
    r17.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r25.xzxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 5497
    r17.y=((uint4(asfloat(r17.yyyy)>=asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5510
    r17.y=((r17.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 5517
    r16.w=(asuint((asfloat(r16.wwww)+asfloat(r17.yyyy)))).w; // native dword 5524
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(uint4(0x3e000000u,0x3e000000u,0x3e000000u,0x3e000000u))))).w; // native dword 5531
#endif
    } else { // native dword 5538
    r17.y=((uint4(r24.yyyy==uint4(0x200u,0x200u,0x200u,0x200u))*0xffffffffu)).y; // native dword 5539
    if((r17.yyyy).x != 0u) { // native dword 5546
    r20.zw=(asuint(mad(asfloat(r22.zzzw),asfloat(r23.zzzw),asfloat(r23.xxxy)))).zw; // native dword 5549
    r20.zw=(asuint(g_tSpotLightVSMShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r20.zwzz).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).zwxy.zw; // native dword 5558
    r23.xyz=(asuint((asfloat(asuint(-asfloat(r0.xyzx)))+asfloat(r21.xyzx)))).xyz; // native dword 5571
    r17.y=(asuint(dot(asfloat(r23.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).y; // native dword 5579
    r17.y=(asuint(sqrt(asfloat(r17.yyyy)))).y; // native dword 5586
    r17.y=(asuint((asfloat(asuint(-asfloat(r21.wwww)))+asfloat(r17.yyyy)))).y; // native dword 5591
    r18.w=(asuint((asfloat(r10.wwww)*asfloat(r17.yyyy)))).w; // native dword 5599
    r19.w=(asuint(mad(asfloat(asuint(-asfloat(r20.zzzz))),asfloat(r20.zzzz),asfloat(r20.wwww)))).w; // native dword 5606
    r19.w=(asuint(max(asfloat(r19.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 5616
    r10.w=(asuint(mad(asfloat(asuint(-asfloat(r17.yyyy))),asfloat(r10.wwww),asfloat(r20.zzzz)))).w; // native dword 5623
    r10.w=(asuint(mad(asfloat(r10.wwww),asfloat(r10.wwww),asfloat(r19.wwww)))).w; // native dword 5633
    r10.w=(asuint((asfloat(r19.wwww)/asfloat(r10.wwww)))).w; // native dword 5642
    r10.w=(asuint(min(asfloat(r10.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 5649
    r17.y=((uint4(asfloat(r18.wwww)<asfloat(r20.zzzz))*0xffffffffu)).y; // native dword 5656
    r16.w=((r17.yyyy!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r10.wwww)).w; // native dword 5663
    } else { // native dword 5672
    r16.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 5673
    } // native dword 5678
    } // native dword 5679
    r10.w=((uint4(r24.xxxx!=uint4(0x0u,0x0u,0x0u,0x0u))*0xffffffffu)).w; // native dword 5680
    r10.w=((r10.wwww&r25.yyyy)).w; // native dword 5687
    r17.y=(asuint((asfloat(asuint(-asfloat(r15.wwww)))+asfloat(r16.wwww)))).y; // native dword 5694
    r17.y=(asuint(mad(asfloat(r22.yyyy),asfloat(r17.yyyy),asfloat(r15.wwww)))).y; // native dword 5702
    r16.w=(asuint(mad(asfloat(r16.wwww),asfloat(r22.yyyy),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 5711
    r16.w=(asuint((asfloat(asuint(-asfloat(r22.yyyy)))+asfloat(r16.wwww)))).w; // native dword 5720
    r15.w=(asuint(min(asfloat(r15.wwww),asfloat(r16.wwww)))).w; // native dword 5728
    r10.w=((r10.wwww!=0u?r17.yyyy:r15.wwww)).w; // native dword 5735
    r15.w=((uint4(asfloat(r10.wwww)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 5744
    if((r15.wwww).x != 0u) { // native dword 5751
    r23.xyz=uint3(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+2u]); // native dword 5754
    r25.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+3u]); // native dword 5764
    r15.w=(asuint((asfloat(asuint(-asfloat(r24.zzzz)))+asfloat(r25.wwww)))).w; // native dword 5774
    r16.w=(asuint((asfloat(r25.wwww)*asfloat(uint4(0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu))))).w; // native dword 5782
    r15.w=(asuint(saturate((asfloat(r15.wwww)/asfloat(r16.wwww))))).w; // native dword 5789
    r16.w=(asuint(mad(asfloat(r15.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 5796
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r15.wwww)))).w; // native dword 5805
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r16.wwww)))).w; // native dword 5812
    r23.xyz=(asuint((asfloat(asuint(-asfloat(r0.xyzx)))+asfloat(r23.xyzx)))).xyz; // native dword 5819
    r16.w=(asuint(dot(asfloat(r23.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).w; // native dword 5827
    r17.y=(asuint((asfloat(r19.yyyy)*asfloat(r19.yyyy)))).y; // native dword 5834
    r16.w=(asuint(mad(asfloat(r19.zzzz),asfloat(r16.wwww),asfloat(r17.yyyy)))).w; // native dword 5841
    r16.w=(asuint(max(asfloat(r16.wwww),asfloat(uint4(0x3e800000u,0x3e800000u,0x3e800000u,0x3e800000u))))).w; // native dword 5850
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 5857
    r16.w=(asuint((1.0f/asfloat(r16.wwww)))).w; // native dword 5864
    r16.w=(asuint((asfloat(asuint(-asfloat(r19.xxxx)))+asfloat(r16.wwww)))).w; // native dword 5869
    r16.w=(asuint(max(asfloat(r16.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 5877
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r16.wwww)))).w; // native dword 5884
    r19.xzw=(asuint((asfloat(r15.wwww)*asfloat(r25.xxyz)))).xzw; // native dword 5891
    r15.w=((r11.wwww&uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 5898
    if((r15.wwww).x != 0u) { // native dword 5905
    r23.xyzw=uint4(g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+0u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+1u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+2u],g_sbDeferredLightSpotData[(r7.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+3u]); // native dword 5908
    r11.w=((r11.wwww&uint4(0x400u,0x400u,0x400u,0x400u))).w; // native dword 5918
    r20.zw=(asuint((asfloat(r20.xxxy)+asfloat(r20.xxxy)))).zw; // native dword 5925
    r15.w=(asuint(mad(asfloat(r20.xxxx),asfloat(r20.xxxx),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 5932
    r15.w=(asuint(mad(asfloat(r20.yyyy),asfloat(r20.yyyy),asfloat(r15.wwww)))).w; // native dword 5941
    r20.xy=(asuint((asfloat(r20.zwzz)/asfloat(r15.wwww)))).xy; // native dword 5950
    r20.xy=(asuint(mad(asfloat(r20.xyxx),asfloat(uint4(0x3f000000u,0xbf000000u,0x0u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 5957
    r20.xy=((r11.wwww!=0u?r20.xyxx:r22.zwzz)).xy; // native dword 5972
    r20.xy=(asuint(mad(asfloat(r20.xyxx),asfloat(r23.zwzz),asfloat(r23.xyxx)))).xy; // native dword 5981
    r20.xyz=(asuint(g_tSpotLightProjectionAtlas.SampleLevel(g_sLinearMipLinearClamp,asfloat(r20.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 5990
    r19.xzw=(asuint((asfloat(r19.xxzw)*asfloat(r20.xxyz)))).xzw; // native dword 6003
    } // native dword 6010
    r8.w=((uint4(asfloat(r8.wwww)>=asfloat(r24.zzzz))*0xffffffffu)).w; // native dword 6011
    r8.w=((r8.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 6018
    r8.w=(asuint((asfloat(r8.wwww)*asfloat(r10.wwww)))).w; // native dword 6025
    r19.xzw=(asuint((asfloat(r8.wwww)*asfloat(r19.xxzw)))).xzw; // native dword 6032
    r20.xyz=(asuint((asfloat(asuint(-asfloat(r0.xyzx)))+asfloat(r21.xyzx)))).xyz; // native dword 6039
    r8.w=(asuint(dot(asfloat(r20.xyzx).xyz,asfloat(r20.xyzx).xyz).xxxx)).w; // native dword 6047
    r10.w=(asuint(rsqrt(asfloat(r8.wwww)))).w; // native dword 6054
    r21.xyz=(asuint((asfloat(r10.wwww)*asfloat(r20.xyzx)))).xyz; // native dword 6059
    r11.w=(asuint(sqrt(asfloat(r8.wwww)))).w; // native dword 6066
    r15.w=(asuint(dot(asfloat(r21.xyzx).xyz,asfloat(r3.xyzx).xyz).xxxx)).w; // native dword 6071
    r16.w=(asuint((asfloat(r11.wwww)*asfloat(r19.yyyy)))).w; // native dword 6078
    r16.w=((uint4(asfloat(r16.wwww)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 6085
    r17.y=(asuint((asfloat(r17.yyyy)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).y; // native dword 6092
    r8.w=(asuint((asfloat(r8.wwww)*asfloat(uint4(0x40c90fdbu,0x40c90fdbu,0x40c90fdbu,0x40c90fdbu))))).w; // native dword 6099
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(r15.wwww)))).w; // native dword 6106
    r11.w=(asuint((asfloat(r11.wwww)/asfloat(r19.yyyy)))).w; // native dword 6113
    r11.w=(asuint((asfloat(r11.wwww)+asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 6120
    r11.w=(asuint(saturate((asfloat(r11.wwww)*asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)))))).w; // native dword 6127
    r11.w=(asuint(mad(asfloat(r11.wwww),asfloat(uint4(0x40000000u,0x40000000u,0x40000000u,0x40000000u)),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 6134
    r18.w=(asuint(mad(asfloat(asuint(-asfloat(r11.wwww))),asfloat(r11.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6143
    r18.w=(asuint(sqrt(asfloat(r18.wwww)))).w; // native dword 6153
    r19.y=(asuint((asfloat(asuint(-abs(asfloat(r11.wwww))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).y; // native dword 6158
    r19.y=(asuint(sqrt(asfloat(r19.yyyy)))).y; // native dword 6166
    r20.w=(asuint(mad(asfloat(asuint(abs(asfloat(r11.wwww)))),asfloat(uint4(0xbc996e30u,0xbc996e30u,0xbc996e30u,0xbc996e30u)),asfloat(uint4(0x3d981627u,0x3d981627u,0x3d981627u,0x3d981627u))))).w; // native dword 6171
    r20.w=(asuint(mad(asfloat(r20.wwww),asfloat(asuint(abs(asfloat(r11.wwww)))),asfloat(uint4(0xbe593484u,0xbe593484u,0xbe593484u,0xbe593484u))))).w; // native dword 6181
    r20.w=(asuint(mad(asfloat(r20.wwww),asfloat(asuint(abs(asfloat(r11.wwww)))),asfloat(uint4(0x3fc90da4u,0x3fc90da4u,0x3fc90da4u,0x3fc90da4u))))).w; // native dword 6191
    r21.x=(asuint((asfloat(r19.yyyy)*asfloat(r20.wwww)))).x; // native dword 6201
    r21.x=(asuint(mad(asfloat(r21.xxxx),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).x; // native dword 6208
    r21.y=((uint4(asfloat(r11.wwww)<asfloat(asuint(-asfloat(r11.wwww))))*0xffffffffu)).y; // native dword 6217
    r21.x=((r21.yyyy&r21.xxxx)).x; // native dword 6225
    r19.y=(asuint(mad(asfloat(r20.wwww),asfloat(r19.yyyy),asfloat(r21.xxxx)))).y; // native dword 6232
    r19.y=(asuint((asfloat(asuint(-asfloat(r19.yyyy)))+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).y; // native dword 6241
    r11.w=(asuint(mad(asfloat(asuint(-asfloat(r18.wwww))),asfloat(r11.wwww),asfloat(asuint(-asfloat(r19.yyyy)))))).w; // native dword 6249
    r11.w=(asuint((asfloat(r11.wwww)+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).w; // native dword 6260
    r11.w=(asuint(saturate((asfloat(r11.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u)))))).w; // native dword 6267
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(r17.yyyy)))).w; // native dword 6274
    r8.w=(asuint((asfloat(r11.wwww)/asfloat(r8.wwww)))).w; // native dword 6281
    r8.w=(asuint(min(asfloat(r8.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6288
    r11.w=((uint4(asfloat(uint4(0x39aec33fu,0x39aec33fu,0x39aec33fu,0x39aec33fu))<asfloat(r8.wwww))*0xffffffffu)).w; // native dword 6295
    r8.w=(asuint(mad(asfloat(r8.wwww),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6302
    r8.w=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r8.wwww)))).w; // native dword 6311
    r17.y=(asuint((asfloat(r8.wwww)*asfloat(r8.wwww)))).y; // native dword 6321
    r17.y=(asuint((asfloat(r8.wwww)*asfloat(r17.yyyy)))).y; // native dword 6328
    r17.y=(asuint(mad(asfloat(r8.wwww),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(asuint(-asfloat(r17.yyyy)))))).y; // native dword 6335
    r8.w=(asuint(mad(asfloat(asuint(-asfloat(r8.wwww))),asfloat(r8.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6345
    r8.w=(asuint((asfloat(r17.yyyy)/asfloat(r8.wwww)))).w; // native dword 6355
    r8.w=(asuint((asfloat(r8.wwww)+asfloat(r8.wwww)))).w; // native dword 6362
    r8.w=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r8.wwww)))).w; // native dword 6369
    r8.w=(asuint(mad(asfloat(r2.yyyy),asfloat(r2.yyyy),asfloat(r8.wwww)))).w; // native dword 6379
    r8.w=(asuint(sqrt(asfloat(r8.wwww)))).w; // native dword 6388
    r8.w=((r11.wwww!=0u?r8.wwww:r2.yyyy)).w; // native dword 6393
    r8.w=(asuint((asfloat(asuint(-asfloat(r8.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6402
    r8.w=((r16.wwww!=0u?r8.wwww:r6.yyyy)).w; // native dword 6410
    r20.xyz=(asuint(mad(asfloat(r20.xyzx),asfloat(r10.wwww),asfloat(r6.xzwx)))).xyz; // native dword 6419
    r10.w=(asuint(dot(asfloat(r20.xyzx).xyz,asfloat(r20.xyzx).xyz).xxxx)).w; // native dword 6428
    r10.w=(asuint(rsqrt(asfloat(r10.wwww)))).w; // native dword 6435
    r20.xyz=(asuint((asfloat(r10.wwww)*asfloat(r20.xyzx)))).xyz; // native dword 6440
    r10.w=(asuint(dot(asfloat(r6.xzwx).xyz,asfloat(r20.xyzx).xyz).xxxx)).w; // native dword 6447
    r11.w=(asuint(dot(asfloat(r3.xyzx).xyz,asfloat(r20.xyzx).xyz).xxxx)).w; // native dword 6454
    r8.w=(asuint((asfloat(asuint(-asfloat(r8.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6461
    r8.w=(asuint(max(asfloat(r8.wwww),asfloat(uint4(0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu))))).w; // native dword 6469
    r8.w=(asuint(min(asfloat(r8.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6476
    r8.w=(asuint((asfloat(r8.wwww)*asfloat(r8.wwww)))).w; // native dword 6483
    r10.w=(asuint((asfloat(asuint(-abs(asfloat(r10.wwww))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6490
    r10.w=(asuint(max(asfloat(r10.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 6498
    r16.w=(asuint((asfloat(r10.wwww)*asfloat(r10.wwww)))).w; // native dword 6505
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r16.wwww)))).w; // native dword 6512
    r10.w=(asuint((asfloat(r10.wwww)*asfloat(r16.wwww)))).w; // native dword 6519
    r10.w=(asuint(mad(asfloat(r2.wwww),asfloat(r10.wwww),asfloat(r1.zzzz)))).w; // native dword 6526
    r16.w=(asuint((asfloat(r8.wwww)*asfloat(r8.wwww)))).w; // native dword 6535
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(r11.wwww)))).w; // native dword 6542
    r17.y=(asuint(mad(asfloat(r8.wwww),asfloat(r8.wwww),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).y; // native dword 6549
    r11.w=(asuint(mad(asfloat(r11.wwww),asfloat(r17.yyyy),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6558
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(r11.wwww)))).w; // native dword 6567
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 6574
    r11.w=(asuint(max(asfloat(r11.wwww),asfloat(uint4(0x3a83126fu,0x3a83126fu,0x3a83126fu,0x3a83126fu))))).w; // native dword 6581
    r11.w=(asuint((asfloat(r16.wwww)/asfloat(r11.wwww)))).w; // native dword 6588
    r15.w=(asuint(saturate(asfloat(r15.wwww)))).w; // native dword 6595
    r8.w=(asuint(mad(asfloat(asuint(-asfloat(r8.wwww))),asfloat(r8.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6600
    r17.y=(asuint(mad(asfloat(r15.wwww),asfloat(r8.wwww),asfloat(r16.wwww)))).y; // native dword 6610
    r8.w=(asuint(mad(asfloat(r2.zzzz),asfloat(r8.wwww),asfloat(r16.wwww)))).w; // native dword 6619
    r8.w=(asuint((asfloat(r8.wwww)*asfloat(r17.yyyy)))).w; // native dword 6628
    r8.w=(asuint(max(asfloat(r8.wwww),asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u))))).w; // native dword 6635
    r8.w=(asuint((asfloat(uint4(0x3e800000u,0x3e800000u,0x3e800000u,0x3e800000u))/asfloat(r8.wwww)))).w; // native dword 6642
    r16.w=(asuint(saturate((asfloat(asuint(-asfloat(r10.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)))))).w; // native dword 6649
    r16.w=(asuint((asfloat(r15.wwww)*asfloat(r16.wwww)))).w; // native dword 6657
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).w; // native dword 6664
    r10.w=(asuint((asfloat(r10.wwww)*asfloat(r11.wwww)))).w; // native dword 6671
    r8.w=(asuint((asfloat(r8.wwww)*asfloat(r10.wwww)))).w; // native dword 6678
    r8.w=(asuint((asfloat(r15.wwww)*asfloat(r8.wwww)))).w; // native dword 6685
    r13.xzw=(asuint(mad(asfloat(r16.wwww),asfloat(r19.xxzw),asfloat(r13.xxzw)))).xzw; // native dword 6692
    r18.xyz=(asuint(mad(asfloat(r8.wwww),asfloat(r19.xzwx),asfloat(r18.xyzx)))).xyz; // native dword 6701
    } // native dword 6710
    } // native dword 6711
    r7.w=(r8.zzzz).w; // native dword 6712
    } // native dword 6717
    r2.xyzw=(r13.xzwx).xyzw; // native dword 6718
    r3.xyzw=(r18.xyzx).xyzw; // native dword 6723
    r2.xyzw=(asuint(mad(asfloat(r12.xyzw),asfloat(r10.xyzx),asfloat(r2.xyzw)))).xyzw; // native dword 6728
#if PT_GLASS_COMPOSITE
    if (all(r1.xy < ptGlassExtent)) {
        PTGlassSecondaryHit bounce=ptGlassSecondaryHits[r1.y*ptGlassExtent.x+r1.x];
        if (PTGlassValidHit(bounce,ptGlassEpoch,ptGlassMaterialCount)) {
            float4 traced=ptGlassRadiance.Load(int3(r1.xy,0));
            if (traced.w>0.5f && all(isfinite(traced.xyz))) r11.xyz=asuint(PTGlassMergeRadiance(asfloat(r11.xyz),traced.xyz));
        }
    }
#endif
    r3.xyzw=(asuint(mad(asfloat(r9.xyzw),asfloat(r11.xyzx),asfloat(r3.xyzw)))).xyzw; // native dword 6737
    r0.xyz=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r1.wwww).x-0u)].SampleLevel(g_sLinearWrap,asfloat(r8.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 6746
    r4.xyzw=(asuint((asfloat(r5.yzwy)*asfloat(r0.xyzx)))).xyzw; // native dword 6761
    r0.xyz=(asuint((asfloat(r5.xxxx)*asfloat(r7.xyzx)))).xyz; // native dword 6768
    r1.z=(asuint(dot(asfloat(r0.xyzx).xyz,asfloat(r0.xyzx).xyz).xxxx)).z; // native dword 6775
    r1.z=((uint4(asfloat(r1.zzzz)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).z; // native dword 6782
    if((r1.zzzz).x != 0u) { // native dword 6789
    r0.w=((r0.wwww*uint4(0x23u,0x23u,0x23u,0x23u)+uint4(0x5u,0x5u,0x5u,0x5u))).w; // native dword 6792
    r5.xyz=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r0.wwww).x-0u)].SampleLevel(g_sLinearWrap,asfloat(r8.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 6801
    r0.xyz=(asuint((asfloat(r0.xyzx)*asfloat(r5.xyzx)))).xyz; // native dword 6816
    } // native dword 6823
    r2.xyzw=(asuint(mad(asfloat(r2.xyzw),asfloat(r4.xyzw),asfloat(r3.xyzw)))).xyzw; // native dword 6824
    r0.xyzw=(asuint((asfloat(r0.xyzx)+asfloat(r2.xyzw)))).xyzw; // native dword 6833
#if PT_GLASS_SECONDARY_EVALUATION
    g_rwtRadiance[r1.xy]=float4(max(asfloat(r0.xyz),0.0f),1.0f);
#else
    g_rwtRadiance[(r1.xyyy).xy]=asfloat(r0.xyzw); // native dword 6840
#endif
    return; // native dword 6848
}
