// GENERATED native GI6 reference: 1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de
// Complete evaluator port. Not a new estimator or sample reuse.
SamplerState g_sLinearWrap : register(s5, space1);
SamplerState g_sLinearClamp : register(s6, space1);
SamplerState g_sLinearMipLinearClamp : register(s11, space1);
SamplerState g_sMaterialArrayGlobal : register(s19, space1);
Texture2D<float4> g_tGBuffer1 : register(t0, space0);
Texture2D<float4> g_tGBuffer2 : register(t1, space0);
Texture2D<float4> g_tLinearDepth : register(t2, space0);
Texture2D<float4> g_tClipDepth : register(t3, space0);
struct GINativeRaw4 { uint d[2]; };
StructuredBuffer<GINativeRaw4> g_sbMaterialDataPart1 : register(t4, space0);
struct GINativeRaw5 { uint d[84]; };
StructuredBuffer<GINativeRaw5> g_sbMaterialDataPart3 : register(t5, space0);
Texture2D<float4> g_tPointLightShadowAtlas : register(t6, space0);
struct GINativeRaw7 { uint d[40]; };
StructuredBuffer<GINativeRaw7> g_sbDeferredLightPointData : register(t7, space0);
Texture2D<float4> g_tSpotLightPCFShadowAtlas : register(t8, space0);
Texture2D<float4> g_tSpotLightVSMShadowAtlas : register(t9, space0);
Texture2D<float4> g_tSpotLightProjectionAtlas : register(t10, space0);
struct GINativeRaw11 { uint d[80]; };
StructuredBuffer<GINativeRaw11> g_sbDeferredLightSpotData : register(t11, space0);
Texture2D<float4> g_tEnvBRDF : register(t12, space0);
struct GINativeRaw13 { uint d[20]; };
StructuredBuffer<GINativeRaw13> g_sbActiveVolumeSamplingInfoArray : register(t13, space0);
struct GINativeRaw14 { uint d[1]; };
StructuredBuffer<GINativeRaw14> g_sbTopLevelNodeIndicesBuffer : register(t14, space0);
struct GINativeRaw15 { uint d[4]; };
StructuredBuffer<GINativeRaw15> g_sbInternalNodesBuffer : register(t15, space0);
Texture3D<float4> g_tGIBrickTextures[6] : register(t16, space0);
Texture2D<uint4> g_tRaytracingLightCellPointLights : register(t22, space0);
Texture2D<uint4> g_tRaytracingLightCellSpotLights : register(t23, space0);
Texture2DArray<uint4> g_tMaterialId : register(t24, space0);
Texture2DArray<float4> g_tNormal_TexcoordX : register(t25, space0);
Texture2DArray<float4> g_tPosition_TexcoordY : register(t26, space0);
Texture2D<float4> g_sMaterialTextureArray[] : register(t0, space1);
RWTexture2D<float4> g_rwtDiffuseGIColorTarget : register(u0, space0);
RWTexture2D<float4> g_rwtHitInfo : register(u1, space0);
cbuffer sys_constants : register(b0) { uint4 giCB0[71]; };
cbuffer deferredlight_constants : register(b1) { uint4 giCB1[26]; };
cbuffer illuminationvolume : register(b2) { uint4 giCB2[3]; };
cbuffer ManualUpdateCB_ActiveVolumeTopLevelInfoArray : register(b3) { uint4 giCB3[1536]; };
cbuffer DiffuseGIConstants : register(b4) { uint4 giCB4[1]; };

uint4 GIBitExtract(uint4 width,uint4 offset,uint4 value) {
    width &= 31u; offset &= 31u;
    return (value>>offset)&((1u<<width)-1u);
}
uint4 GIBitInsert(uint4 width,uint4 offset,uint4 value,uint4 target) {
    width &= 31u; offset &= 31u;
    uint4 mask=((1u<<width)-1u)<<offset;
    return (target&~mask)|((value<<offset)&mask);
}

[numthreads(8,16,1)]
void main(uint3 dispatchId : SV_DispatchThreadID) {
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
    uint4 r28=0u;
    uint4 r29=0u;
    r0.xy=(uint4(dispatchId,0u).xyxx).xy; // native dword 245
    r0.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 249
    r1.x=(g_tMaterialId.Load(asint(r0.xyww).xyzw)).xyzw.x; // native dword 254
    r1.x=((uint4(r1.xxxx!=uint4(0xfffeu,0xfffeu,0xfffeu,0xfffeu))*0xffffffffu)).x; // native dword 262
    if((r1.xxxx).x != 0u) { // native dword 269
    r1.xy=(asuint(float4(uint4(dispatchId,0u).xyxx))).xy; // native dword 272
    r1.xy=(asuint((asfloat(r1.xyxx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 276
    r1.xy=(asuint((asfloat(r1.xyxx)*asfloat(giCB0[1u].zwzz)))).xy; // native dword 286
    r1.z=(asuint(g_tClipDepth.Load(asint(r0.xyww).xyw))).yzxw.z; // native dword 295
    r1.w=(asuint(g_tLinearDepth.Load(asint(r0.xyww).xyw))).yzwx.w; // native dword 303
    r1.xy=(asuint(mad(asfloat(r1.xyxx),asfloat(uint4(0x40000000u,0xc0000000u,0x0u,0x0u)),asfloat(uint4(0xbf800000u,0x3f800000u,0x0u,0x0u))))).xy; // native dword 311
    r2.xyzw=(asuint((asfloat(r1.yyyy)*asfloat(giCB0[15u].xyzw)))).xyzw; // native dword 326
    r2.xyzw=(asuint(mad(asfloat(giCB0[14u].xyzw),asfloat(r1.xxxx),asfloat(r2.xyzw)))).xyzw; // native dword 335
    r2.xyzw=(asuint(mad(asfloat(giCB0[16u].xyzw),asfloat(r1.zzzz),asfloat(r2.xyzw)))).xyzw; // native dword 346
    r2.xyzw=(asuint((asfloat(r2.xyzw)+asfloat(giCB0[17u].xyzw)))).xyzw; // native dword 357
    r2.w=(asuint((1.0f/asfloat(r2.wwww)))).w; // native dword 366
    r3.xyz=(asuint((asfloat(r2.wwww)*asfloat(r2.xyzx)))).xyz; // native dword 371
    r3.w=(asuint(dot(asfloat(r3.xyzx).xyz,asfloat(r3.xyzx).xyz).xxxx)).w; // native dword 378
    r3.w=(asuint(rsqrt(asfloat(r3.wwww)))).w; // native dword 385
    r3.xyz=(asuint((asfloat(r3.wwww)*asfloat(r3.xyzx)))).xyz; // native dword 390
    r4.xyzw=(asuint((asfloat(r1.yyyy)*asfloat(giCB0[23u].xyzw)))).xyzw; // native dword 397
    r4.xyzw=(asuint(mad(asfloat(giCB0[22u].xyzw),asfloat(r1.xxxx),asfloat(r4.xyzw)))).xyzw; // native dword 406
    r4.xyzw=(asuint(mad(asfloat(giCB0[24u].xyzw),asfloat(r1.zzzz),asfloat(r4.xyzw)))).xyzw; // native dword 417
    r4.xyzw=(asuint((asfloat(r4.xyzw)+asfloat(giCB0[25u].xyzw)))).xyzw; // native dword 428
    r1.x=(asuint((1.0f/asfloat(r4.wwww)))).x; // native dword 437
    r5.xyz=(asuint((asfloat(r1.xxxx)*asfloat(r4.xyzx)))).xyz; // native dword 442
    r1.xyz=(asuint(mad(asfloat(r4.xyzx),asfloat(r1.xxxx),asfloat(asuint(-asfloat(giCB0[54u].xyzx)))))).xyz; // native dword 449
    r3.w=(asuint(dot(asfloat(r1.xyzx).xyz,asfloat(r1.xyzx).xyz).xxxx)).w; // native dword 461
    r3.w=(asuint(rsqrt(asfloat(r3.wwww)))).w; // native dword 468
    r1.xyz=(asuint((asfloat(r1.xyzx)*asfloat(r3.wwww)))).xyz; // native dword 473
    r4.xyzw=(asuint(g_tGBuffer1.Load(asint(r0.xyww).xyw))).xzyw.xyzw; // native dword 480
    r3.w=(asuint(mad(asfloat(r4.wwww),asfloat(uint4(0x437f0000u,0x437f0000u,0x437f0000u,0x437f0000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).w; // native dword 488
    r3.w=(uint4(asfloat(r3.wwww))).w; // native dword 497
    r3.w=((r3.wwww&uint4(0xfeu,0xfeu,0xfeu,0xfeu))).w; // native dword 502
    r6.z=(asuint(float4(r3.wwww))).z; // native dword 509
    r6.xy=(r4.xzxx).xy; // native dword 514
    r6.xyz=(asuint(mad(asfloat(r6.xyzx),asfloat(uint4(0x437f0000u,0x437f0000u,0x3f800000u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 519
    r6.xyz=(asuint(floor(asfloat(r6.xyzx)))).xyz; // native dword 534
    r6.xyz=(asuint(int4(asfloat(r6.xyzx)))).xyz; // native dword 539
    r7.xyzw=(GIBitExtract(uint4(0x3u,0x4u,0x3u,0x4u),uint4(0x1u,0x4u,0x1u,0x4u),r6.zzzz)).xyzw; // native dword 544
    r6.xyzw=(GIBitInsert(uint4(0x1du,0x1cu,0x1du,0x1cu),uint4(0x3u,0x4u,0x3u,0x4u),r6.xyxy,r7.xyzw)).xyzw; // native dword 559
    r6.xyzw=(asuint(float4(asint(r6.xyzw)))).xyzw; // native dword 576
    r6.xyzw=(asuint(mad(asfloat(r6.xyzw),asfloat(uint4(0x3a801002u,0x3a000801u,0x3a801002u,0x3a000801u)),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).xyzw; // native dword 581
    r6.xyzw=(asuint((asfloat(r6.xyzw)*asfloat(uint4(0x3fa66666u,0x3fa66666u,0x40266666u,0x40266666u))))).xyzw; // native dword 596
    r3.w=(asuint(dot(asfloat(r6.xyxx).xy,asfloat(r6.xyxx).xy).xxxx)).w; // native dword 606
    r6.xy=(asuint((asfloat(r3.wwww)+asfloat(uint4(0x3f800000u,0xbf800000u,0x0u,0x0u))))).xy; // native dword 613
    r6.xyz=(asuint((asfloat(r6.zwyz)/asfloat(r6.xxxx)))).xyz; // native dword 623
    r7.xyz=(asuint((asfloat(r6.yyyy)*asfloat(giCB0[7u].xyzx)))).xyz; // native dword 630
    r7.xyz=(asuint(mad(asfloat(giCB0[6u].xyzx),asfloat(r6.xxxx),asfloat(r7.xyzx)))).xyz; // native dword 639
    r7.xyz=(asuint(mad(asfloat(giCB0[8u].xyzx),asfloat(r6.zzzz),asfloat(r7.xyzx)))).xyz; // native dword 650
    r8.xyz=(asuint(g_tGBuffer2.Load(asint(r0.xyww).xyw))).yzwx.xyz; // native dword 661
    r4.zw=(asuint((asfloat(r8.yyyz)*asfloat(uint4(0x0u,0x0u,0x437f0000u,0x437f0000u))))).zw; // native dword 669
    r4.zw=(uint4(asfloat(r4.zzzw))).zw; // native dword 679
    r3.w=((r4.zzzz<<(uint4(0x8u,0x8u,0x8u,0x8u)&31u))).w; // native dword 684
    r3.w=((r4.wwww|r3.wwww)).w; // native dword 691
    r4.x=(asuint(dot(asfloat(r6.xyzx).xyz,asfloat(asuint(-asfloat(r3.xyzx))).xyz).xxxx)).x; // native dword 698
    r3.x=g_sbMaterialDataPart1[(r3.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 706
    r3.y=((r3.xxxx>>(uint4(0x18u,0x18u,0x18u,0x18u)&31u))).y; // native dword 716
    r3.xz=(GIBitExtract(uint4(0x8u,0x0u,0x8u,0x0u),uint4(0x10u,0x0u,0x8u,0x0u),r3.xxxx)).xz; // native dword 723
    r6.xyz=(asuint(float4(r3.yxzy))).xyz; // native dword 738
    r3.xyz=(asuint((asfloat(r6.xyzx)*asfloat(uint4(0x3b808081u,0x3b808081u,0x3b808081u,0x0u))))).xyz; // native dword 743
    r3.w=g_sbMaterialDataPart1[(r3.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 753
    r3.w=((r3.wwww&uint4(0xffu,0xffu,0xffu,0xffu))).w; // native dword 763
    r4.z=(asuint(dot(asfloat(r3.xyzx).xyz,asfloat(uint4(0x3e59b3d0u,0x3f371759u,0x3d93dd98u,0x0u)).xyz).xxxx)).z; // native dword 770
    r4.w=(asuint((asfloat(r4.zzzz)+asfloat(uint4(0x358637bdu,0x358637bdu,0x358637bdu,0x358637bdu))))).w; // native dword 780
    r4.w=(asuint((1.0f/asfloat(r4.wwww)))).w; // native dword 787
    r4.z=(asuint((asfloat(r4.zzzz)*asfloat(uint4(0x461c4000u,0x461c4000u,0x461c4000u,0x461c4000u))))).z; // native dword 792
    r4.z=(asuint(min(asfloat(r4.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 799
    r6.xyz=(asuint(mad(asfloat(r3.xyzx),asfloat(r4.wwww),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0x0u))))).xyz; // native dword 806
    r6.xyz=(asuint(mad(asfloat(r4.zzzz),asfloat(r6.xyzx),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x0u))))).xyz; // native dword 818
    r6.xyz=(asuint((asfloat(r8.xxxx)*asfloat(r6.xyzx)))).xyz; // native dword 830
    r3.xyz=((r3.wwww!=0u?r3.xyzx:r6.xyzx)).xyz; // native dword 837
    r1.xyz=(asuint((asfloat(r1.wwww)*asfloat(r1.xyzx)))).xyz; // native dword 846
    r1.xyz=(asuint(mad(asfloat(asuint(-asfloat(r1.xyzx))),asfloat(uint4(0x3a83126fu,0x3a83126fu,0x3a83126fu,0x0u)),asfloat(r5.xyzx)))).xyz; // native dword 853
    r6.xyz=(asuint((asfloat(r1.wwww)*asfloat(r7.xyzx)))).xyz; // native dword 866
    r1.xyz=(asuint(mad(asfloat(r6.xyzx),asfloat(uint4(0x3a83126fu,0x3a83126fu,0x3a83126fu,0x0u)),asfloat(r1.xyzx)))).xyz; // native dword 873
    r4.xyz=(asuint(g_tEnvBRDF.SampleLevel(g_sLinearClamp,asfloat(r4.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 885
    r3.w=(asuint((asfloat(giCB1[15u].wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).w; // native dword 898
    r3.xyz=(asuint(mad(asfloat(r3.xyzx),asfloat(r4.xxxx),asfloat(r4.yyyy)))).xyz; // native dword 907
    r3.xyz=(asuint((asfloat(asuint(-asfloat(r3.xyzx)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x0u))))).xyz; // native dword 916
    r3.xyz=(asuint(max(asfloat(r3.xyzx),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).xyz; // native dword 927
    r3.xyz=(asuint((asfloat(r4.zzzz)*asfloat(r3.xyzx)))).xyz; // native dword 937
    r4.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 944
    r6.z=(uint4(0x0u,0x0u,0x0u,0x0u)).z; // native dword 949
    r8.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 954
    r9.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 959
    r10.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 964
    r5.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 972
    r0.z=(uint4(0x0u,0x0u,0x0u,0x0u)).z; // native dword 977
    [loop] while(true) { // native dword 982
    r7.w=((uint4(asint(r0.zzzz)>=asint(giCB4[0u].zzzz))*0xffffffffu)).w; // native dword 983
    if((r7.wwww).x != 0u) break; // native dword 992
    r7.w=(g_tMaterialId.Load(asint(r0.xyzw).xyzw)).yzwx.w; // native dword 995
    r11.xyzw=(asuint(g_tPosition_TexcoordY.Load(asint(r0.xyzw).xyzw))).xyzw.xyzw; // native dword 1003
    r12.xyz=(asuint((asfloat(r11.yyyy)*asfloat(giCB0[7u].xyzx)))).xyz; // native dword 1011
    r12.xyz=(asuint(mad(asfloat(giCB0[6u].xyzx),asfloat(r11.xxxx),asfloat(r12.xyzx)))).xyz; // native dword 1020
    r12.xyz=(asuint(mad(asfloat(giCB0[8u].xyzx),asfloat(r11.zzzz),asfloat(r12.xyzx)))).xyz; // native dword 1031
    r12.xyz=(asuint((asfloat(r12.xyzx)+asfloat(giCB0[9u].xyzx)))).xyz; // native dword 1042
    r13.xyzw=(asuint(g_tNormal_TexcoordX.Load(asint(r0.xyzw).xyzw))).xyzw.xyzw; // native dword 1051
    r14.xyz=(asuint((asfloat(asuint(-asfloat(r1.xyzx)))+asfloat(r12.xyzx)))).xyz; // native dword 1059
    r10.w=(asuint(dot(asfloat(r14.xyzx).xyz,asfloat(r14.xyzx).xyz).xxxx)).w; // native dword 1067
    r10.w=(asuint(rsqrt(asfloat(r10.wwww)))).w; // native dword 1074
    r14.xyz=(asuint((asfloat(r10.wwww)*asfloat(r14.xyzx)))).xyz; // native dword 1079
    r10.w=((uint4(r7.wwww==uint4(0xffffu,0xffffu,0xffffu,0xffffu))*0xffffffffu)).w; // native dword 1086
    r15.xyz=(asuint(mad(asfloat(giCB4[0u].yyyy),asfloat(r14.xyzx),asfloat(r5.xyzx)))).xyz; // native dword 1093
    r4.xyz=((r10.wwww!=0u?r15.xyzx:r12.xyzx)).xyz; // native dword 1104
    r15.xyz=((r10.wwww!=0u?r7.xyzx:r13.xyzx)).xyz; // native dword 1113
    r16.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1122
    r12.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 1130
    r14.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 1135
    [loop] while(true) { // native dword 1140
    r15.w=((uint4(r14.wwww>=giCB2[0u].wwww)*0xffffffffu)).w; // native dword 1141
    if((r15.wwww).x != 0u) break; // native dword 1150
    r15.w=((r14.wwww<<(uint4(0x2u,0x2u,0x2u,0x2u)&31u))).w; // native dword 1153
    r17.x=(asuint(dot(asfloat(giCB3[(r15.wwww).x].xyzw).xyzw,asfloat(r4.xyzw).xyzw).xxxx)).x; // native dword 1160
    r17.y=(asuint(dot(asfloat(giCB3[(1u+(r15.wwww).x)].xyzw).xyzw,asfloat(r4.xyzw).xyzw).xxxx)).y; // native dword 1170
    r17.z=(asuint(dot(asfloat(giCB3[(2u+(r15.wwww).x)].xyzw).xyzw,asfloat(r4.xyzw).xyzw).xxxx)).z; // native dword 1181
    r18.xyz=((uint4(asfloat(r17.xyzx)>=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).xyz; // native dword 1192
    r16.w=((r18.yyyy&r18.xxxx)).w; // native dword 1202
    r16.w=((r18.zzzz&r16.wwww)).w; // native dword 1209
    r18.xyz=(asuint(float4(giCB3[(3u+(r15.wwww).x)].xyzx))).xyz; // native dword 1216
    r19.xyz=((uint4(asfloat(r17.xyzx)<asfloat(r18.xyzx))*0xffffffffu)).xyz; // native dword 1225
    r17.w=((r19.yyyy&r19.xxxx)).w; // native dword 1232
    r17.w=((r19.zzzz&r17.wwww)).w; // native dword 1239
    r16.w=((r16.wwww&r17.wwww)).w; // native dword 1246
    if((r16.wwww).x != 0u) { // native dword 1253
    r19.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u]); // native dword 1256
    r20.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+2u]); // native dword 1266
    r21.xyzw=uint4(g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+2u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+3u]); // native dword 1276
    r22.xyzw=uint4(g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+2u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+3u]); // native dword 1286
    r23.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r14.wwww).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+2u]); // native dword 1296
    r16.w=(asuint(saturate((asfloat(r17.xxxx)*asfloat(r21.wwww))))).w; // native dword 1306
    r18.xyz=(asuint((asfloat(asuint(-asfloat(r17.xyzx)))+asfloat(r18.xyzx)))).xyz; // native dword 1313
    r17.w=(asuint(saturate((asfloat(r21.wwww)*asfloat(r18.xxxx))))).w; // native dword 1321
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r17.wwww)))).w; // native dword 1328
    r18.xw=(asuint(saturate((asfloat(r17.yyyz)*asfloat(r22.xxxy))))).xw; // native dword 1335
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r18.xxxx)))).w; // native dword 1342
    r18.xy=(asuint(saturate((asfloat(r22.xyxx)*asfloat(r18.yzyy))))).xy; // native dword 1349
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r18.xxxx)))).w; // native dword 1356
    r16.w=(asuint((asfloat(r18.wwww)*asfloat(r16.wwww)))).w; // native dword 1363
    r16.w=(asuint((asfloat(r18.yyyy)*asfloat(r16.wwww)))).w; // native dword 1370
    r17.w=((uint4(r23.xxxx!=uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu))*0xffffffffu)).w; // native dword 1377
    if((r17.wwww).x != 0u) { // native dword 1384
    r18.xyz=(asuint(floor(asfloat(r17.xyzx)))).xyz; // native dword 1387
    r17.xyz=(asuint((asfloat(r17.xyzx)+asfloat(asuint(-asfloat(r18.xyzx)))))).xyz; // native dword 1392
    r18.xyz=(uint4(asfloat(r18.xyzx))).xyz; // native dword 1400
    r17.w=((r18.zzzz*giCB3[(3u+(r15.wwww).x)].yyyy+r18.yyyy)).w; // native dword 1405
    r15.w=((r17.wwww*giCB3[(3u+(r15.wwww).x)].xxxx+r18.xxxx)).w; // native dword 1418
    r15.w=((r22.zzzz+r15.wwww)).w; // native dword 1431
    r15.w=g_sbTopLevelNodeIndicesBuffer[(r15.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 1438
    r18.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1448
    r22.xyz=(r17.xyzx).xyz; // native dword 1456
    r17.w=(r15.wwww).w; // native dword 1461
    r18.w=(r23.yyyy).w; // native dword 1466
    [loop] while(true) { // native dword 1471
    r19.w=((r17.wwww&uint4(0x80000000u,0x80000000u,0x80000000u,0x80000000u))).w; // native dword 1472
    r19.w=((uint4(r19.wwww!=uint4(0x0u,0x0u,0x0u,0x0u))*0xffffffffu)).w; // native dword 1479
    r20.w=((uint4(asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))>=asfloat(r18.wwww))*0xffffffffu)).w; // native dword 1486
    r19.w=((r19.wwww|r20.wwww)).w; // native dword 1493
    if((r19.wwww).x != 0u) { // native dword 1500
    r19.w=((r17.wwww&uint4(0x7fffffffu,0x7fffffffu,0x7fffffffu,0x7fffffffu))).w; // native dword 1503
    r19.w=((r23.xxxx+r19.wwww)).w; // native dword 1510
    r24.x=((r19.wwww&uint4(0x7fu,0x7fu,0x7fu,0x7fu))).x; // native dword 1517
    r24.y=(GIBitExtract(uint4(0x7u,0x7u,0x7u,0x7u),uint4(0x7u,0x7u,0x7u,0x7u),r19.wwww)).y; // native dword 1524
    r24.z=((r19.wwww>>(uint4(0xeu,0xeu,0xeu,0xeu)&31u))).z; // native dword 1533
    r24.xyz=((r24.xyzx<<(uint4(0x2u,0x2u,0x2u,0x0u)&31u))).xyz; // native dword 1540
    r24.xyz=(asuint(float4(r24.xyzx))).xyz; // native dword 1550
    r24.xyz=(asuint(mad(asfloat(r22.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(r24.xyzx)))).xyz; // native dword 1555
    r18.xyz=(asuint((asfloat(r24.xyzx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 1567
    break; // native dword 1577
    } // native dword 1578
    r19.w=((r22.wwww+r17.wwww)).w; // native dword 1579
    r24.xyzw=uint4(g_sbInternalNodesBuffer[(r19.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbInternalNodesBuffer[(r19.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbInternalNodesBuffer[(r19.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbInternalNodesBuffer[(r19.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 1586
    r25.xyz=(asuint((asfloat(r22.xyzx)*asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u))))).xyz; // native dword 1596
    r25.xyz=(asuint(floor(asfloat(r25.xyzx)))).xyz; // native dword 1606
    r26.xyz=(uint4(asfloat(r25.xyzx))).xyz; // native dword 1611
    r26.xz=(asuint(float4(r26.zzxz))).xz; // native dword 1616
    r19.w=((r26.yyyy*uint4(0x3u,0x3u,0x3u,0x3u))).w; // native dword 1621
    r19.w=(asuint(float4(r19.wwww))).w; // native dword 1629
    r19.w=(asuint(mad(asfloat(r26.xxxx),asfloat(uint4(0x41100000u,0x41100000u,0x41100000u,0x41100000u)),asfloat(r19.wwww)))).w; // native dword 1634
    r19.w=(asuint((asfloat(r26.zzzz)+asfloat(r19.wwww)))).w; // native dword 1643
    r19.w=(uint4(asfloat(r19.wwww))).w; // native dword 1650
    r20.w=((uint4(0x1u,0x1u,0x1u,0x1u)<<(r19.wwww&31u))).w; // native dword 1655
    r21.w=((r20.wwww&r24.xxxx)).w; // native dword 1662
    if((r21.wwww).x == 0u) { // native dword 1669
    r21.w=((r23.xxxx+r17.wwww)).w; // native dword 1672
    r26.x=((r21.wwww&uint4(0x7fu,0x7fu,0x7fu,0x7fu))).x; // native dword 1679
    r26.y=(GIBitExtract(uint4(0x7u,0x7u,0x7u,0x7u),uint4(0x7u,0x7u,0x7u,0x7u),r21.wwww)).y; // native dword 1686
    r26.z=((r21.wwww>>(uint4(0xeu,0xeu,0xeu,0xeu)&31u))).z; // native dword 1695
    r26.xyz=((r26.xyzx<<(uint4(0x2u,0x2u,0x2u,0x0u)&31u))).xyz; // native dword 1702
    r26.xyz=(asuint(float4(r26.xyzx))).xyz; // native dword 1712
    r26.xyz=(asuint(mad(asfloat(r22.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(r26.xyzx)))).xyz; // native dword 1717
    r18.xyz=(asuint((asfloat(r26.xyzx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 1729
    break; // native dword 1739
    } // native dword 1740
    r22.xyz=(asuint(mad(asfloat(r22.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(asuint(-asfloat(r25.xyzx)))))).xyz; // native dword 1741
    r20.w=((r20.wwww&r24.yyyy)).w; // native dword 1754
    r21.w=(GIBitExtract(r19.wwww,uint4(0x0u,0x0u,0x0u,0x0u),r24.yyyy)).w; // native dword 1761
    r21.w=(countbits(r21.wwww)).w; // native dword 1770
    r21.w=((r21.wwww+r24.wwww)).w; // native dword 1775
    r21.w=((r21.wwww|uint4(0x80000000u,0x80000000u,0x80000000u,0x80000000u))).w; // native dword 1782
    r23.w=((~r24.yyyy)).w; // native dword 1789
    r23.w=((r23.wwww&r24.xxxx)).w; // native dword 1794
    r19.w=(GIBitExtract(r19.wwww,uint4(0x0u,0x0u,0x0u,0x0u),r23.wwww)).w; // native dword 1801
    r19.w=(countbits(r19.wwww)).w; // native dword 1810
    r19.w=((r19.wwww+r24.zzzz)).w; // native dword 1815
    r17.w=((r20.wwww!=0u?r21.wwww:r19.wwww)).w; // native dword 1822
    r18.w=(asuint((asfloat(r18.wwww)*asfloat(uint4(0x3eaaaaabu,0x3eaaaaabu,0x3eaaaaabu,0x3eaaaaabu))))).w; // native dword 1831
    r18.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1838
    } // native dword 1846
    } else { // native dword 1847
    r18.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1848
    } // native dword 1856
    r17.xyz=(asuint((asfloat(r18.xyzx)*asfloat(giCB2[0u].xyzx)))).xyz; // native dword 1857
    r22.x=(r19.xxxx).x; // native dword 1866
    r22.y=(r20.xxxx).y; // native dword 1871
    r22.z=(r21.xxxx).z; // native dword 1876
    r22.x=(asuint(dot(asfloat(r22.xyzx).xyz,asfloat(r15.xyzx).xyz).xxxx)).x; // native dword 1881
    r24.x=(r19.yyyy).x; // native dword 1888
    r24.y=(r20.yyyy).y; // native dword 1893
    r24.z=(r21.yyyy).z; // native dword 1898
    r22.y=(asuint(dot(asfloat(r24.xyzx).xyz,asfloat(r15.xyzx).xyz).xxxx)).y; // native dword 1903
    r21.x=(r19.zzzz).x; // native dword 1910
    r21.y=(r20.zzzz).y; // native dword 1915
    r22.z=(asuint(dot(asfloat(r21.xyzx).xyz,asfloat(r15.xyzx).xyz).xxxx)).z; // native dword 1920
    r19.xyz=(asuint(saturate(asfloat(r22.xyzx)))).xyz; // native dword 1927
    r19.xyz=(asuint((asfloat(r19.xyzx)*asfloat(r19.xyzx)))).xyz; // native dword 1932
    r22.xyz=(asuint(saturate(asfloat(asuint(-asfloat(r22.xyzx)))))).xyz; // native dword 1939
    r20.xyz=(asuint((asfloat(r22.xyzx)*asfloat(r22.xyzx)))).xyz; // native dword 1945
    r21.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r19.xyzx))*0xffffffffu)).xyz; // native dword 1952
    if((r21.xxxx).x != 0u) { // native dword 1962
    r22.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(16u-16u)].SampleLevel(g_sLinearClamp,asfloat(r17.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 1965
    r22.xyz=(asuint((asfloat(r19.xxxx)*asfloat(r22.xyzx)))).xyz; // native dword 1978
    } else { // native dword 1985
    r22.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1986
    } // native dword 1994
    r24.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r20.xyzx))*0xffffffffu)).xyz; // native dword 1995
    if((r24.xxxx).x != 0u) { // native dword 2005
    r25.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(17u-16u)].SampleLevel(g_sLinearClamp,asfloat(r17.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2008
    r22.xyz=(asuint(mad(asfloat(r25.xyzx),asfloat(r20.xxxx),asfloat(r22.xyzx)))).xyz; // native dword 2021
    } // native dword 2030
    if((r21.yyyy).x != 0u) { // native dword 2031
    r21.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(18u-16u)].SampleLevel(g_sLinearClamp,asfloat(r17.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 2034
    r22.xyz=(asuint(mad(asfloat(r21.xywx),asfloat(r19.yyyy),asfloat(r22.xyzx)))).xyz; // native dword 2047
    } // native dword 2056
    if((r24.yyyy).x != 0u) { // native dword 2057
    r19.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(19u-16u)].SampleLevel(g_sLinearClamp,asfloat(r17.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 2060
    r22.xyz=(asuint(mad(asfloat(r19.xywx),asfloat(r20.yyyy),asfloat(r22.xyzx)))).xyz; // native dword 2073
    } // native dword 2082
    if((r21.zzzz).x != 0u) { // native dword 2083
    r19.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(20u-16u)].SampleLevel(g_sLinearClamp,asfloat(r17.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 2086
    r22.xyz=(asuint(mad(asfloat(r19.xywx),asfloat(r19.zzzz),asfloat(r22.xyzx)))).xyz; // native dword 2099
    } // native dword 2108
    if((r24.zzzz).x != 0u) { // native dword 2109
    r17.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(21u-16u)].SampleLevel(g_sLinearClamp,asfloat(r17.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2112
    r22.xyz=(asuint(mad(asfloat(r17.xyzx),asfloat(r20.zzzz),asfloat(r22.xyzx)))).xyz; // native dword 2125
    } // native dword 2134
    r15.w=((uint4(r23.xxxx==uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu))*0xffffffffu)).w; // native dword 2135
    r17.xyz=((r15.wwww!=0u?uint4(0x3f800000u,0x3eb33333u,0x0u,0x0u):r22.xyzx)).xyz; // native dword 2142
    r15.w=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 2154
    r17.w=(asuint((asfloat(r16.wwww)*asfloat(r16.wwww)))).w; // native dword 2163
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r17.wwww)))).w; // native dword 2170
    r17.w=(asuint((asfloat(r23.zzzz)+asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 2177
    r15.w=(asuint(mad(asfloat(r15.wwww),asfloat(r17.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 2184
    r17.w=(asuint((asfloat(r15.wwww)*asfloat(r16.wwww)))).w; // native dword 2193
    r16.xyz=(asuint(mad(asfloat(r17.xyzx),asfloat(r17.wwww),asfloat(r16.xyzx)))).xyz; // native dword 2200
    r12.w=(asuint(mad(asfloat(r16.wwww),asfloat(r15.wwww),asfloat(r12.wwww)))).w; // native dword 2209
    } // native dword 2218
    r14.w=((r14.wwww+uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 2219
    } // native dword 2226
    r4.x=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r12.wwww))*0xffffffffu)).x; // native dword 2227
    r15.xyz=(asuint((asfloat(r16.xyzx)/asfloat(r12.wwww)))).xyz; // native dword 2234
    r4.xyz=((r4.xxxx!=0u?r15.xyzx:r16.xyzx)).xyz; // native dword 2241
    r4.xyz=(asuint((asfloat(r4.xyzx)+asfloat(giCB2[1u].xyzx)))).xyz; // native dword 2250
    r15.xyz=(asuint(mad(asfloat(r2.xyzx),asfloat(r2.wwww),asfloat(asuint(-asfloat(r11.xyzx)))))).xyz; // native dword 2259
    r14.w=(asuint(dot(asfloat(r15.xyzx).xyz,asfloat(r15.xyzx).xyz).xxxx)).w; // native dword 2269
    r14.w=(asuint(sqrt(asfloat(r14.wwww)))).w; // native dword 2276
    r14.w=((r10.wwww!=0u?giCB4[0u].xxxx:r14.wwww)).w; // native dword 2281
    r5.w=(asuint((asfloat(r5.wwww)+asfloat(r14.wwww)))).w; // native dword 2292
    if((r10.wwww).x != 0u) { // native dword 2299
    r15.xyz=(asuint((asfloat(r3.wwww)*asfloat(r4.xyzx)))).xyz; // native dword 2302
    } else { // native dword 2309
    r17.xyz=uint3(g_sbMaterialDataPart3[(r7.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+0u],g_sbMaterialDataPart3[(r7.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+1u],g_sbMaterialDataPart3[(r7.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+2u]); // native dword 2310
    r10.w=((r7.wwww*uint4(0x23u,0x23u,0x23u,0x23u))).w; // native dword 2320
    r19.x=(r13.wwww).x; // native dword 2328
    r19.y=(r11.wwww).y; // native dword 2333
    r20.xyz=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r10.wwww).x-0u)].SampleLevel(g_sLinearWrap,asfloat(r19.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2338
    r17.xyz=(asuint((asfloat(r17.xyzx)*asfloat(r20.xyzx)))).xyz; // native dword 2353
    r10.w=(asuint(dot(asfloat(r17.xyzx).xyz,asfloat(r17.xyzx).xyz).xxxx)).w; // native dword 2360
    r10.w=((uint4(asfloat(r10.wwww)>=asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u)))*0xffffffffu)).w; // native dword 2367
    if((r10.wwww).x != 0u) { // native dword 2374
    r10.w=(asuint(saturate((asfloat(r14.wwww)*asfloat(uint4(0x42480000u,0x42480000u,0x42480000u,0x42480000u)))))).w; // native dword 2377
    r4.xyz=(asuint((asfloat(r4.xyzx)*asfloat(r10.wwww)))).xyz; // native dword 2384
    r19.zw=uint2(g_sbMaterialDataPart3[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbMaterialDataPart3[(r7.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u]); // native dword 2391
    r20.xy=((r7.wwww*uint4(0x23u,0x23u,0x0u,0x0u)+uint4(0x2u,0x4u,0x0u,0x0u))).xy; // native dword 2401
    r10.w=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r20.xxxx).x-0u)].SampleLevel(g_sMaterialArrayGlobal,asfloat(r19.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 2416
    r11.w=(asuint((asfloat(asuint(-asfloat(r19.zzzz)))+asfloat(r19.wwww)))).w; // native dword 2431
    r21.y=(asuint(mad(asfloat(r10.wwww),asfloat(r11.wwww),asfloat(r19.zzzz)))).y; // native dword 2439
    r10.w=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r20.yyyy).x-0u)].SampleLevel(g_sMaterialArrayGlobal,asfloat(r19.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 2448
    r7.w=g_sbMaterialDataPart1[(r7.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 2463
    r11.w=((r7.wwww>>(uint4(0x18u,0x18u,0x18u,0x18u)&31u))).w; // native dword 2473
    r19.xy=(GIBitExtract(uint4(0x8u,0x8u,0x0u,0x0u),uint4(0x10u,0x8u,0x0u,0x0u),r7.wwww)).xy; // native dword 2480
    r20.x=(asuint(float4(r11.wwww))).x; // native dword 2495
    r20.yz=(asuint(float4(r19.xxyx))).yz; // native dword 2500
    r19.xyz=(asuint((asfloat(r10.wwww)*asfloat(r20.xyzx)))).xyz; // native dword 2505
    r19.xyz=(asuint((asfloat(r19.xyzx)*asfloat(uint4(0x3b808081u,0x3b808081u,0x3b808081u,0x0u))))).xyz; // native dword 2512
    r7.w=(asuint(dot(asfloat(r19.xyzx).xyz,asfloat(uint4(0x3e59b3d0u,0x3f371759u,0x3d93dd98u,0x0u)).xyz).xxxx)).w; // native dword 2522
    r21.x=(asuint(dot(asfloat(r13.xyzx).xyz,asfloat(asuint(-asfloat(r14.xyzx))).xyz).xxxx)).x; // native dword 2532
    r14.xyz=(asuint(g_tEnvBRDF.SampleLevel(g_sLinearClamp,asfloat(r21.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2540
    r14.xyw=(asuint(mad(asfloat(r19.xyxz),asfloat(r14.xxxx),asfloat(r14.yyyy)))).xyw; // native dword 2553
    r14.xyw=(asuint((asfloat(asuint(-asfloat(r14.xyxw)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x0u,0x3f800000u))))).xyw; // native dword 2562
    r14.xyw=(asuint((asfloat(r14.xyxw)*asfloat(giCB1[15u].wwww)))).xyw; // native dword 2573
    r14.xyz=(asuint((asfloat(r14.zzzz)*asfloat(r14.xywx)))).xyz; // native dword 2582
    r4.xyz=(asuint((asfloat(r4.xyzx)*asfloat(r14.xyzx)))).xyz; // native dword 2589
    r14.xyz=(asuint((asfloat(r12.yyyy)*asfloat(giCB0[3u].xyzx)))).xyz; // native dword 2596
    r14.xyz=(asuint(mad(asfloat(giCB0[2u].xyzx),asfloat(r12.xxxx),asfloat(r14.xyzx)))).xyz; // native dword 2605
    r12.xyz=(asuint(mad(asfloat(giCB0[4u].xyzx),asfloat(r12.zzzz),asfloat(r14.xyzx)))).xyz; // native dword 2616
    r12.xyz=(asuint((asfloat(r12.xyzx)+asfloat(giCB0[5u].xyzx)))).xyz; // native dword 2627
    r14.xyz=(asuint((asfloat(r13.yyyy)*asfloat(giCB0[3u].xyzx)))).xyz; // native dword 2636
    r13.xyw=(asuint(mad(asfloat(giCB0[2u].xyxz),asfloat(r13.xxxx),asfloat(r14.xyxz)))).xyw; // native dword 2645
    r13.xyz=(asuint(mad(asfloat(giCB0[4u].xyzx),asfloat(r13.zzzz),asfloat(r13.xywx)))).xyz; // native dword 2656
    r11.xyz=(asuint((asfloat(r11.xyzx)+asfloat(uint4(0x43160000u,0x42960000u,0x42960000u,0x0u))))).xyz; // native dword 2667
    r11.xyz=(asuint((asfloat(r11.xyzx)*asfloat(uint4(0x3d5a740eu,0x3d5a740eu,0x3d5a740eu,0x0u))))).xyz; // native dword 2677
    r11.xyz=(asuint(floor(asfloat(r11.xyzx)))).xyz; // native dword 2687
    r11.xyz=(asuint(max(asfloat(r11.xyzx),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).xyz; // native dword 2692
    r11.xyz=(asuint(min(asfloat(r11.xyzx),asfloat(uint4(0x41700000u,0x40e00000u,0x41300000u,0x0u))))).xyz; // native dword 2702
    r11.xyz=(asuint(int4(asfloat(r11.xyzx)))).xyz; // native dword 2712
    r6.x=((r11.xxxx*uint4(0xcu,0xcu,0xcu,0xcu)+r11.zzzz)).x; // native dword 2717
    r6.y=((r11.yyyy<<(uint4(0x8u,0x8u,0x8u,0x8u)&31u))).y; // native dword 2726
    r10.w=(g_tRaytracingLightCellPointLights.Load(asint(r6.xyzz).xyw)).yzwx.w; // native dword 2733
    r11.x=(asuint(dot(asfloat(asuint(-asfloat(r12.xyzx))).xyz,asfloat(asuint(-asfloat(r12.xyzx))).xyz).xxxx)).x; // native dword 2741
    r9.xyz=(asuint(mad(asfloat(r13.xyzx),asfloat(uint4(0x3d8f5c29u,0x3d8f5c29u,0x3d8f5c29u,0x0u)),asfloat(r12.xyzx)))).xyz; // native dword 2750
    r11.x=(asuint(rsqrt(asfloat(r11.xxxx)))).x; // native dword 2762
    r11.xyz=(asuint((asfloat(r11.xxxx)*asfloat(asuint(-asfloat(r12.xyzx)))))).xyz; // native dword 2767
    r11.w=(asuint((asfloat(asuint(-asfloat(r7.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 2775
    r14.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 2783
    r13.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 2791
    r14.w=(r10.wwww).w; // native dword 2796
    [loop] while(true) { // native dword 2801
    r15.w=((uint4(r14.wwww!=uint4(0xffffu,0xffffu,0xffffu,0xffffu))*0xffffffffu)).w; // native dword 2802
    r16.w=((uint4(asint(r13.wwww)<asint(uint4(0x100u,0x100u,0x100u,0x100u)))*0xffffffffu)).w; // native dword 2809
    r15.w=((r15.wwww&r16.wwww)).w; // native dword 2816
    if((r15.wwww).x == 0u) break; // native dword 2823
    r19.xyzw=uint4(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 2826
    r20.xyzw=uint4(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+2u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+3u]); // native dword 2836
    r21.xyzw=uint4(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+2u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+3u]); // native dword 2846
    r22.xyzw=uint4(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+2u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+3u]); // native dword 2856
    r15.w=g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x90u,0x90u,0x90u,0x90u)).x/4u+0u]; // native dword 2866
    r16.w=g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x9cu,0x9cu,0x9cu,0x9cu)).x/4u+0u]; // native dword 2876
    r19.xyz=(asuint((asfloat(asuint(-asfloat(r12.xyzx)))+asfloat(r19.xyzx)))).xyz; // native dword 2886
    r17.w=(asuint(dot(asfloat(r19.xyzx).xyz,asfloat(r19.xyzx).xyz).xxxx)).w; // native dword 2894
    r18.w=(asuint(rsqrt(asfloat(r17.wwww)))).w; // native dword 2901
    r23.xyz=(asuint((asfloat(r18.wwww)*asfloat(r19.xyzx)))).xyz; // native dword 2906
    r23.w=(asuint(sqrt(asfloat(r17.wwww)))).w; // native dword 2913
    r24.x=(asuint((asfloat(r20.wwww)+asfloat(asuint(-asfloat(r23.wwww)))))).x; // native dword 2918
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(uint4(0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu))))).w; // native dword 2926
    r20.w=(asuint(saturate((asfloat(r24.xxxx)/asfloat(r20.wwww))))).w; // native dword 2933
    r24.x=(asuint(mad(asfloat(r20.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).x; // native dword 2940
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(r20.wwww)))).w; // native dword 2949
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(r24.xxxx)))).w; // native dword 2956
    r15.w=(asuint(mad(asfloat(r15.wwww),asfloat(r15.wwww),asfloat(r17.wwww)))).w; // native dword 2963
    r15.w=(asuint(max(asfloat(r15.wwww),asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u))))).w; // native dword 2972
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 2979
    r15.w=(asuint((1.0f/asfloat(r15.wwww)))).w; // native dword 2986
    r15.w=(asuint((asfloat(asuint(-asfloat(r21.xxxx)))+asfloat(r15.wwww)))).w; // native dword 2991
    r15.w=(asuint(max(asfloat(r15.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 2999
    r15.w=(asuint((asfloat(r20.wwww)*asfloat(r15.wwww)))).w; // native dword 3006
    r20.xyz=(asuint((asfloat(r15.wwww)*asfloat(r20.xyzx)))).xyz; // native dword 3013
    r24.xy=((r16.wwww&uint4(0x4u,0x2u,0x0u,0x0u))).xy; // native dword 3020
    r25.x=(r21.wwww).x; // native dword 3030
    r25.yz=(r22.xxyx).yz; // native dword 3035
    r15.w=(asuint(dot(asfloat(asuint(-asfloat(r23.xyzx))).xyz,asfloat(r25.xyzx).xyz).xxxx)).w; // native dword 3040
    r15.w=(asuint(saturate(mad(asfloat(r15.wwww),asfloat(r21.yyyy),asfloat(r21.zzzz))))).w; // native dword 3048
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r15.wwww)))).w; // native dword 3057
    r21.xyz=(asuint((asfloat(r15.wwww)*asfloat(r20.xyzx)))).xyz; // native dword 3064
    r20.xyz=((r24.xxxx!=0u?r21.xyzx:r20.xyzx)).xyz; // native dword 3071
    if((r24.yyyy).x != 0u) { // native dword 3080
    r21.xy=uint2(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u]); // native dword 3083
    r24.xyz=uint3(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+2u]); // native dword 3093
    r25.xyz=uint3(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+2u]); // native dword 3103
    r26.xyw=uint3(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+2u]); // native dword 3113
    r27.xyzw=uint4(g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+0u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+1u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+2u],g_sbDeferredLightPointData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+3u]); // native dword 3123
    r28.x=(r22.wwww).x; // native dword 3133
    r28.y=(r24.xxxx).y; // native dword 3138
    r28.z=(r25.xxxx).z; // native dword 3143
    r28.w=(r26.xxxx).w; // native dword 3148
    r28.x=(asuint(dot(asfloat(r28.xyzw).xyzw,asfloat(r9.xyzw).xyzw).xxxx)).x; // native dword 3153
    r29.x=(r21.xxxx).x; // native dword 3160
    r29.y=(r24.yyyy).y; // native dword 3165
    r29.z=(r25.yyyy).z; // native dword 3170
    r29.w=(r26.yyyy).w; // native dword 3175
    r28.y=(asuint(dot(asfloat(r29.xyzw).xyzw,asfloat(r9.xyzw).xyzw).xxxx)).y; // native dword 3180
    r26.x=(r21.yyyy).x; // native dword 3187
    r26.y=(r24.zzzz).y; // native dword 3192
    r26.z=(r25.zzzz).z; // native dword 3197
    r28.z=(asuint(dot(asfloat(r26.xyzw).xyzw,asfloat(r9.xyzw).xyzw).xxxx)).z; // native dword 3202
    r15.w=(asuint(dot(asfloat(r28.xyzx).xyz,asfloat(r28.xyzx).xyz).xxxx)).w; // native dword 3209
    r15.w=(asuint(rsqrt(asfloat(r15.wwww)))).w; // native dword 3216
    r21.xyz=(asuint((asfloat(r15.wwww)*asfloat(r28.xyzx)))).xyz; // native dword 3221
    r24.xyzw=((uint4(asfloat(asuint(abs(asfloat(r21.yzxz))))<asfloat(asuint(abs(asfloat(r21.xxyy)))))*0xffffffffu)).xyzw; // native dword 3228
    r22.xyw=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r21.xyxz))*0xffffffffu)).xyw; // native dword 3237
    r24.xy=((r24.ywyy&r24.xzxx)).xy; // native dword 3247
    r22.xyw=((r22.xyxw!=0u?uint4(0x0u,0x2u,0x0u,0x4u):uint4(0x1u,0x3u,0x0u,0x5u))).xyw; // native dword 3254
    r15.w=((r24.yyyy!=0u?r22.yyyy:r22.wwww)).w; // native dword 3269
    r15.w=((r24.xxxx!=0u?r22.xxxx:r15.wwww)).w; // native dword 3278
    if((r15.wwww).x != 0u) { // native dword 3287
    r16.w=((uint4(r15.wwww!=uint4(0x1u,0x1u,0x1u,0x1u))*0xffffffffu)).w; // native dword 3290
    if((r16.wwww).x != 0u) { // native dword 3297
    r24.xyz=(asuint((asfloat(r21.xxyx)/asfloat(r21.yzzy)))).xyz; // native dword 3300
    r16.w=(asuint((asfloat(asuint(-asfloat(r21.zzzz)))/asfloat(r21.yyyy)))).w; // native dword 3307
    r25.y=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).y; // native dword 3315
    r21.xw=(asuint((asfloat(r21.xxxz)/asfloat(asuint(-asfloat(r21.yyyy)))))).xw; // native dword 3324
    r21.xw=(asuint(mad(asfloat(r21.xxxw),asfloat(uint4(0x3f000000u,0x0u,0x0u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x0u,0x0u,0x3f000000u))))).xw; // native dword 3332
    r26.xyz=((uint4(r15.wwww==uint4(0x2u,0x3u,0x4u,0x0u))*0xffffffffu)).xyz; // native dword 3347
    r24.xyz=(asuint(mad(asfloat(r24.xyzx),asfloat(uint4(0x3f000000u,0x3f000000u,0xbf000000u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 3357
    r16.w=(asuint((asfloat(r21.yyyy)/asfloat(asuint(-asfloat(r21.zzzz)))))).w; // native dword 3372
    r24.w=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).w; // native dword 3380
    r21.yz=((r26.zzzz!=0u?r24.yyzy:r24.yywy)).yz; // native dword 3389
    r21.xy=((r26.yyyy!=0u?r21.xwxx:r21.yzyy)).xy; // native dword 3398
    r25.x=(r24.xxxx).x; // native dword 3407
    r22.xy=((r26.xxxx!=0u?r25.xyxx:r21.xyxx)).xy; // native dword 3412
    } else { // native dword 3421
    r24.xy=(asuint((asfloat(r21.zyzz)/asfloat(asuint(-asfloat(r21.xxxx)))))).xy; // native dword 3422
    r22.xy=(asuint(mad(asfloat(r24.xyxx),asfloat(uint4(0x3f000000u,0xbf000000u,0x0u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 3430
    } // native dword 3445
    } else { // native dword 3446
    r16.w=(asuint((asfloat(asuint(-asfloat(r21.zzzz)))/asfloat(r21.xxxx)))).w; // native dword 3447
    r22.x=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).x; // native dword 3455
    r16.w=(asuint((asfloat(r21.yyyy)/asfloat(r21.xxxx)))).w; // native dword 3464
    r22.y=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).y; // native dword 3471
    } // native dword 3480
    r21.xy=(asuint((asfloat(r22.xyxx)+asfloat(uint4(0xbf000000u,0xbf000000u,0x0u,0x0u))))).xy; // native dword 3481
    r21.yz=(asuint(mad(asfloat(r21.xxyx),asfloat(r19.wwww),asfloat(uint4(0x0u,0x3f000000u,0x3f000000u,0x0u))))).yz; // native dword 3491
    r15.w=(asuint(float4(r15.wwww))).w; // native dword 3503
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(uint4(0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu))))).w; // native dword 3508
    r21.x=(asuint(mad(asfloat(r21.yyyy),asfloat(uint4(0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu)),asfloat(r15.wwww)))).x; // native dword 3515
    r21.xy=(asuint(mad(asfloat(r21.xzxx),asfloat(r27.zwzz),asfloat(r27.xyxx)))).xy; // native dword 3524
    r21.xy=(asuint(g_tPointLightShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r21.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xy; // native dword 3533
    r15.w=(asuint((asfloat(r23.wwww)+asfloat(uint4(0xbd4ccccdu,0xbd4ccccdu,0xbd4ccccdu,0xbd4ccccdu))))).w; // native dword 3546
    r16.w=(asuint((asfloat(r22.zzzz)*asfloat(r15.wwww)))).w; // native dword 3553
    r17.w=(asuint(mad(asfloat(asuint(-asfloat(r21.xxxx))),asfloat(r21.xxxx),asfloat(r21.yyyy)))).w; // native dword 3560
    r17.w=(asuint(max(asfloat(r17.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 3570
    r15.w=(asuint(mad(asfloat(asuint(-asfloat(r15.wwww))),asfloat(r22.zzzz),asfloat(r21.xxxx)))).w; // native dword 3577
    r15.w=(asuint(mad(asfloat(r15.wwww),asfloat(r15.wwww),asfloat(r17.wwww)))).w; // native dword 3587
    r15.w=(asuint((asfloat(r17.wwww)/asfloat(r15.wwww)))).w; // native dword 3596
    r15.w=(asuint(min(asfloat(r15.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 3603
    r16.w=((uint4(asfloat(r16.wwww)<asfloat(r21.xxxx))*0xffffffffu)).w; // native dword 3610
    r15.w=((r16.wwww!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r15.wwww)).w; // native dword 3617
    r20.xyz=(asuint((asfloat(r15.wwww)*asfloat(r20.xyzx)))).xyz; // native dword 3626
    } // native dword 3633
    r19.xyz=(asuint(mad(asfloat(r19.xyzx),asfloat(r18.wwww),asfloat(r11.xyzx)))).xyz; // native dword 3634
    r15.w=(asuint(dot(asfloat(r19.xyzx).xyz,asfloat(r19.xyzx).xyz).xxxx)).w; // native dword 3643
    r15.w=(asuint(rsqrt(asfloat(r15.wwww)))).w; // native dword 3650
    r19.xyz=(asuint((asfloat(r15.wwww)*asfloat(r19.xyzx)))).xyz; // native dword 3655
    r15.w=(asuint(dot(asfloat(r11.xyzx).xyz,asfloat(r19.xyzx).xyz).xxxx)).w; // native dword 3662
    r16.w=(asuint(saturate(dot(asfloat(r13.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx))).w; // native dword 3669
    r15.w=(asuint((asfloat(asuint(-abs(asfloat(r15.wwww))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 3676
    r15.w=(asuint(max(asfloat(r15.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 3684
    r17.w=(asuint((asfloat(r15.wwww)*asfloat(r15.wwww)))).w; // native dword 3691
    r17.w=(asuint((asfloat(r17.wwww)*asfloat(r17.wwww)))).w; // native dword 3698
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(r17.wwww)))).w; // native dword 3705
    r15.w=(asuint(mad(asfloat(r11.wwww),asfloat(r15.wwww),asfloat(r7.wwww)))).w; // native dword 3712
    r15.w=(asuint(saturate((asfloat(asuint(-asfloat(r15.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)))))).w; // native dword 3721
    r15.w=(asuint((asfloat(r16.wwww)*asfloat(r15.wwww)))).w; // native dword 3729
    r15.w=(asuint((asfloat(r15.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).w; // native dword 3736
    r14.xyz=(asuint(mad(asfloat(r15.wwww),asfloat(r20.xyzx),asfloat(r14.xyzx)))).xyz; // native dword 3743
    r13.w=((r13.wwww+uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 3752
    r6.w=((r6.yyyy+r13.wwww)).w; // native dword 3759
    r14.w=(g_tRaytracingLightCellPointLights.Load(asint(r6.xwzz).xyw)).yzwx.w; // native dword 3766
    } // native dword 3774
    r6.w=(g_tRaytracingLightCellSpotLights.Load(asint(r6.xyzz).xyw)).yzwx.w; // native dword 3775
    r9.xyz=(asuint((asfloat(r12.zzzz)*asfloat(r13.xyzx)))).xyz; // native dword 3783
    r8.xyz=(asuint(mad(asfloat(r9.xyzx),asfloat(uint4(0x399d4952u,0x399d4952u,0x399d4952u,0x0u)),asfloat(r12.xyzx)))).xyz; // native dword 3790
    r19.xzw=(r6.xxzz).xzw; // native dword 3802
    r9.xyz=(r14.xyzx).xyz; // native dword 3807
    r6.x=(uint4(0x0u,0x0u,0x0u,0x0u)).x; // native dword 3812
    r10.w=(r6.wwww).w; // native dword 3817
    [loop] while(true) { // native dword 3822
    r13.w=((uint4(r10.wwww!=uint4(0xffffu,0xffffu,0xffffu,0xffffu))*0xffffffffu)).w; // native dword 3823
    r14.w=((uint4(asint(r6.xxxx)<asint(uint4(0x100u,0x100u,0x100u,0x100u)))*0xffffffffu)).w; // native dword 3830
    r13.w=((r13.wwww&r14.wwww)).w; // native dword 3837
    if((r13.wwww).x == 0u) break; // native dword 3844
    r6.x=((r6.xxxx+uint4(0x1u,0x1u,0x1u,0x1u))).x; // native dword 3847
    r19.y=((r6.xxxx+r6.yyyy)).y; // native dword 3854
    r13.w=(g_tRaytracingLightCellSpotLights.Load(asint(r19.xyzw).xyw)).yzwx.w; // native dword 3861
    r20.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+3u]); // native dword 3869
    r21.xyz=uint3(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+2u]); // native dword 3879
    r22.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+3u]); // native dword 3889
    r23.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+3u]); // native dword 3899
    r24.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+3u]); // native dword 3909
    r25.x=(r20.wwww).x; // native dword 3919
    r25.y=(r22.xxxx).y; // native dword 3924
    r25.z=(r23.xxxx).z; // native dword 3929
    r25.w=(r24.xxxx).w; // native dword 3934
    r25.x=(asuint(dot(asfloat(r25.xyzw).xyzw,asfloat(r8.xyzw).xyzw).xxxx)).x; // native dword 3939
    r26.x=(r21.xxxx).x; // native dword 3946
    r26.y=(r22.yyyy).y; // native dword 3951
    r26.z=(r23.yyyy).z; // native dword 3956
    r26.w=(r24.yyyy).w; // native dword 3961
    r25.y=(asuint(dot(asfloat(r26.xyzw).xyzw,asfloat(r8.xyzw).xyzw).xxxx)).y; // native dword 3966
    r26.x=(r21.yyyy).x; // native dword 3973
    r26.y=(r22.zzzz).y; // native dword 3978
    r26.z=(r23.zzzz).z; // native dword 3983
    r26.w=(r24.zzzz).w; // native dword 3988
    r25.z=(asuint(dot(asfloat(r26.xyzw).xyzw,asfloat(r8.xyzw).xyzw).xxxx)).z; // native dword 3993
    r24.x=(r21.zzzz).x; // native dword 4000
    r24.y=(r22.wwww).y; // native dword 4005
    r24.z=(r23.wwww).z; // native dword 4010
    r14.w=(asuint(dot(asfloat(r24.xyzw).xyzw,asfloat(r8.xyzw).xyzw).xxxx)).w; // native dword 4015
    r15.w=(asuint((1.0f/asfloat(r14.wwww)))).w; // native dword 4022
    r21.xyz=(asuint((asfloat(r15.wwww)*asfloat(r25.xyzx)))).xyz; // native dword 4027
    r22.xyz=(asuint(mad(asfloat(r21.xyzx),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f800000u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xyz; // native dword 4034
    r23.xyz=(asuint(saturate(asfloat(r22.xyzx)))).xyz; // native dword 4049
    r22.xyz=((uint4(asfloat(r22.xyzx)==asfloat(r23.xyzx))*0xffffffffu)).xyz; // native dword 4054
    r15.w=((r22.yyyy&r22.xxxx)).w; // native dword 4061
    r15.w=((r22.zzzz&r15.wwww)).w; // native dword 4068
    if((r15.wwww).x != 0u) { // native dword 4075
    r22.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 4078
    r15.w=g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x9cu,0x9cu,0x9cu,0x9cu)).x/4u+0u]; // native dword 4088
    r16.w=g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0xc8u,0xc8u,0xc8u,0xc8u)).x/4u+0u]; // native dword 4098
    r23.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+3u]); // native dword 4108
    r17.w=g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x130u,0x130u,0x130u,0x130u)).x/4u+0u]; // native dword 4118
    r24.xy=(asuint(mad(asfloat(r21.xyxx),asfloat(uint4(0x3f000000u,0xbf000000u,0x0u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 4128
    r24.zw=((r17.wwww&uint4(0x0u,0x0u,0x2u,0x300u))).zw; // native dword 4143
    if((r24.zzzz).x != 0u) { // native dword 4153
    r26.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+3u]); // native dword 4156
    r25.xy=(asuint(mad(asfloat(r24.xyxx),asfloat(r26.zwzz),asfloat(r26.xyxx)))).xy; // native dword 4166
    r25.xy=(asuint(g_tSpotLightVSMShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r25.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xy; // native dword 4175
    r26.xyz=(asuint((asfloat(asuint(-asfloat(r12.xyzx)))+asfloat(r22.xyzx)))).xyz; // native dword 4188
    r18.w=(asuint(dot(asfloat(r26.xyzx).xyz,asfloat(r26.xyzx).xyz).xxxx)).w; // native dword 4196
    r18.w=(asuint(sqrt(asfloat(r18.wwww)))).w; // native dword 4203
    r18.w=(asuint((asfloat(asuint(-asfloat(r22.wwww)))+asfloat(r18.wwww)))).w; // native dword 4208
    r19.y=(asuint((asfloat(r15.wwww)*asfloat(r18.wwww)))).y; // native dword 4216
    r20.w=(asuint(mad(asfloat(asuint(-asfloat(r25.xxxx))),asfloat(r25.xxxx),asfloat(r25.yyyy)))).w; // native dword 4223
    r20.w=(asuint(max(asfloat(r20.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 4233
    r18.w=(asuint(mad(asfloat(asuint(-asfloat(r18.wwww))),asfloat(r15.wwww),asfloat(r25.xxxx)))).w; // native dword 4240
    r18.w=(asuint(mad(asfloat(r18.wwww),asfloat(r18.wwww),asfloat(r20.wwww)))).w; // native dword 4250
    r18.w=(asuint((asfloat(r20.wwww)/asfloat(r18.wwww)))).w; // native dword 4259
    r18.w=(asuint(min(asfloat(r18.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4266
    r19.y=((uint4(asfloat(r19.yyyy)<asfloat(r25.xxxx))*0xffffffffu)).y; // native dword 4273
    r18.w=((r19.yyyy!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r18.wwww)).w; // native dword 4280
    } else { // native dword 4289
    r18.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 4290
    } // native dword 4295
    r25.xy=((uint4(r24.wwww==uint4(0x100u,0x200u,0x0u,0x0u))*0xffffffffu)).xy; // native dword 4296
    if((r25.xxxx).x != 0u) { // native dword 4306
    r25.xw=(asuint(mad(asfloat(r24.xxxy),asfloat(r23.zzzw),asfloat(r23.xxxy)))).xw; // native dword 4309
    r19.y=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r25.xwxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yxzw.y; // native dword 4318
    r19.y=((uint4(asfloat(r19.yyyy)>=asfloat(r21.zzzz))*0xffffffffu)).y; // native dword 4331
    r19.y=((r19.yyyy&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).y; // native dword 4338
    } else { // native dword 4345
    r20.w=((uint4(r24.wwww==uint4(0x200u,0x200u,0x200u,0x200u))*0xffffffffu)).w; // native dword 4346
    if((r20.wwww).x != 0u) { // native dword 4353
    r21.zw=(asuint(mad(asfloat(r24.xxxy),asfloat(r23.zzzw),asfloat(r23.xxxy)))).zw; // native dword 4356
    r21.zw=(asuint(g_tSpotLightVSMShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r21.zwzz).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).zwxy.zw; // native dword 4365
    r23.xyz=(asuint((asfloat(asuint(-asfloat(r12.xyzx)))+asfloat(r22.xyzx)))).xyz; // native dword 4378
    r20.w=(asuint(dot(asfloat(r23.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).w; // native dword 4386
    r20.w=(asuint(sqrt(asfloat(r20.wwww)))).w; // native dword 4393
    r20.w=(asuint((asfloat(asuint(-asfloat(r22.wwww)))+asfloat(r20.wwww)))).w; // native dword 4398
    r22.w=(asuint((asfloat(r15.wwww)*asfloat(r20.wwww)))).w; // native dword 4406
    r21.w=(asuint(mad(asfloat(asuint(-asfloat(r21.zzzz))),asfloat(r21.zzzz),asfloat(r21.wwww)))).w; // native dword 4413
    r21.w=(asuint(max(asfloat(r21.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 4423
    r15.w=(asuint(mad(asfloat(asuint(-asfloat(r20.wwww))),asfloat(r15.wwww),asfloat(r21.zzzz)))).w; // native dword 4430
    r15.w=(asuint(mad(asfloat(r15.wwww),asfloat(r15.wwww),asfloat(r21.wwww)))).w; // native dword 4440
    r15.w=(asuint((asfloat(r21.wwww)/asfloat(r15.wwww)))).w; // native dword 4449
    r15.w=(asuint(min(asfloat(r15.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4456
    r20.w=((uint4(asfloat(r22.wwww)<asfloat(r21.zzzz))*0xffffffffu)).w; // native dword 4463
    r19.y=((r20.wwww!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r15.wwww)).y; // native dword 4470
    } else { // native dword 4479
    r19.y=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).y; // native dword 4480
    } // native dword 4485
    } // native dword 4486
    r15.w=((uint4(r24.zzzz!=uint4(0x0u,0x0u,0x0u,0x0u))*0xffffffffu)).w; // native dword 4487
    r15.w=((r15.wwww&r25.yyyy)).w; // native dword 4494
    r20.w=(asuint((asfloat(asuint(-asfloat(r18.wwww)))+asfloat(r19.yyyy)))).w; // native dword 4501
    r20.w=(asuint(mad(asfloat(r16.wwww),asfloat(r20.wwww),asfloat(r18.wwww)))).w; // native dword 4509
    r19.y=(asuint(mad(asfloat(r19.yyyy),asfloat(r16.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).y; // native dword 4518
    r16.w=(asuint((asfloat(asuint(-asfloat(r16.wwww)))+asfloat(r19.yyyy)))).w; // native dword 4527
    r16.w=(asuint(min(asfloat(r16.wwww),asfloat(r18.wwww)))).w; // native dword 4535
    r15.w=((r15.wwww!=0u?r20.wwww:r16.wwww)).w; // native dword 4542
    r16.w=((uint4(asfloat(r15.wwww)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 4551
    if((r16.wwww).x != 0u) { // native dword 4558
    r23.xyz=uint3(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+2u]); // native dword 4561
    r26.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+3u]); // native dword 4571
    r16.w=(asuint((asfloat(asuint(-asfloat(r25.zzzz)))+asfloat(r26.wwww)))).w; // native dword 4581
    r18.w=(asuint((asfloat(r26.wwww)*asfloat(uint4(0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu))))).w; // native dword 4589
    r16.w=(asuint(saturate((asfloat(r16.wwww)/asfloat(r18.wwww))))).w; // native dword 4596
    r18.w=(asuint(mad(asfloat(r16.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 4603
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r16.wwww)))).w; // native dword 4612
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r18.wwww)))).w; // native dword 4619
    r23.xyz=(asuint((asfloat(asuint(-asfloat(r12.xyzx)))+asfloat(r23.xyzx)))).xyz; // native dword 4626
    r18.w=(asuint(dot(asfloat(r23.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).w; // native dword 4634
    r19.y=(asuint((asfloat(r20.yyyy)*asfloat(r20.yyyy)))).y; // native dword 4641
    r18.w=(asuint(mad(asfloat(r20.zzzz),asfloat(r18.wwww),asfloat(r19.yyyy)))).w; // native dword 4648
    r18.w=(asuint(max(asfloat(r18.wwww),asfloat(uint4(0x3e800000u,0x3e800000u,0x3e800000u,0x3e800000u))))).w; // native dword 4657
    r18.w=(asuint((asfloat(r18.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 4664
    r18.w=(asuint((1.0f/asfloat(r18.wwww)))).w; // native dword 4671
    r18.w=(asuint((asfloat(asuint(-asfloat(r20.xxxx)))+asfloat(r18.wwww)))).w; // native dword 4676
    r18.w=(asuint(max(asfloat(r18.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 4684
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r18.wwww)))).w; // native dword 4691
    r20.xyz=(asuint((asfloat(r16.wwww)*asfloat(r26.xyzx)))).xyz; // native dword 4698
    r16.w=((r17.wwww&uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 4705
    if((r16.wwww).x != 0u) { // native dword 4712
    r23.xyzw=uint4(g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+0u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+1u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+2u],g_sbDeferredLightSpotData[(r10.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+3u]); // native dword 4715
    r16.w=((r17.wwww&uint4(0x400u,0x400u,0x400u,0x400u))).w; // native dword 4725
    r21.zw=(asuint((asfloat(r21.xxxy)+asfloat(r21.xxxy)))).zw; // native dword 4732
    r17.w=(asuint(mad(asfloat(r21.xxxx),asfloat(r21.xxxx),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4739
    r17.w=(asuint(mad(asfloat(r21.yyyy),asfloat(r21.yyyy),asfloat(r17.wwww)))).w; // native dword 4748
    r21.xy=(asuint((asfloat(r21.zwzz)/asfloat(r17.wwww)))).xy; // native dword 4757
    r21.xy=(asuint(mad(asfloat(r21.xyxx),asfloat(uint4(0x3f000000u,0xbf000000u,0x0u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 4764
    r21.xy=((r16.wwww!=0u?r21.xyxx:r24.xyxx)).xy; // native dword 4779
    r21.xy=(asuint(mad(asfloat(r21.xyxx),asfloat(r23.zwzz),asfloat(r23.xyxx)))).xy; // native dword 4788
    r21.xyz=(asuint(g_tSpotLightProjectionAtlas.SampleLevel(g_sLinearMipLinearClamp,asfloat(r21.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 4797
    r20.xyz=(asuint((asfloat(r20.xyzx)*asfloat(r21.xyzx)))).xyz; // native dword 4810
    } // native dword 4817
    r14.w=((uint4(asfloat(r14.wwww)>=asfloat(r25.zzzz))*0xffffffffu)).w; // native dword 4818
    r14.w=((r14.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 4825
    r14.w=(asuint((asfloat(r14.wwww)*asfloat(r15.wwww)))).w; // native dword 4832
    r20.xyz=(asuint((asfloat(r14.wwww)*asfloat(r20.xyzx)))).xyz; // native dword 4839
    r21.xyz=(asuint((asfloat(asuint(-asfloat(r12.xyzx)))+asfloat(r22.xyzx)))).xyz; // native dword 4846
    r14.w=(asuint(dot(asfloat(r21.xyzx).xyz,asfloat(r21.xyzx).xyz).xxxx)).w; // native dword 4854
    r14.w=(asuint(rsqrt(asfloat(r14.wwww)))).w; // native dword 4861
    r22.xyz=(asuint((asfloat(r14.wwww)*asfloat(r21.xyzx)))).xyz; // native dword 4866
    r21.xyz=(asuint(mad(asfloat(r21.xyzx),asfloat(r14.wwww),asfloat(r11.xyzx)))).xyz; // native dword 4873
    r14.w=(asuint(dot(asfloat(r21.xyzx).xyz,asfloat(r21.xyzx).xyz).xxxx)).w; // native dword 4882
    r14.w=(asuint(rsqrt(asfloat(r14.wwww)))).w; // native dword 4889
    r21.xyz=(asuint((asfloat(r14.wwww)*asfloat(r21.xyzx)))).xyz; // native dword 4894
    r14.w=(asuint(dot(asfloat(r11.xyzx).xyz,asfloat(r21.xyzx).xyz).xxxx)).w; // native dword 4901
    r15.w=(asuint(saturate(dot(asfloat(r13.xyzx).xyz,asfloat(r22.xyzx).xyz).xxxx))).w; // native dword 4908
    r14.w=(asuint((asfloat(asuint(-abs(asfloat(r14.wwww))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4915
    r14.w=(asuint(max(asfloat(r14.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 4923
    r16.w=(asuint((asfloat(r14.wwww)*asfloat(r14.wwww)))).w; // native dword 4930
    r16.w=(asuint((asfloat(r16.wwww)*asfloat(r16.wwww)))).w; // native dword 4937
    r14.w=(asuint((asfloat(r14.wwww)*asfloat(r16.wwww)))).w; // native dword 4944
    r14.w=(asuint(mad(asfloat(r11.wwww),asfloat(r14.wwww),asfloat(r7.wwww)))).w; // native dword 4951
    r14.w=(asuint(saturate((asfloat(asuint(-asfloat(r14.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)))))).w; // native dword 4960
    r14.w=(asuint((asfloat(r15.wwww)*asfloat(r14.wwww)))).w; // native dword 4968
    r14.w=(asuint((asfloat(r14.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).w; // native dword 4975
    r9.xyz=(asuint(mad(asfloat(r14.wwww),asfloat(r20.xyzx),asfloat(r9.xyzx)))).xyz; // native dword 4982
    } // native dword 4991
    } // native dword 4992
    r10.w=(r13.wwww).w; // native dword 4993
    } // native dword 4998
    r4.xyz=(asuint(mad(asfloat(r4.xyzx),asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x0u)),asfloat(r9.xyzx)))).xyz; // native dword 4999
    r15.xyz=(asuint((asfloat(r17.xyzx)*asfloat(r4.xyzx)))).xyz; // native dword 5011
    } else { // native dword 5018
    r15.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 5019
    } // native dword 5027
    } // native dword 5028
    r10.xyz=(asuint(mad(asfloat(r3.xyzx),asfloat(r15.xyzx),asfloat(r10.xyzx)))).xyz; // native dword 5029
    r0.z=((r0.zzzz+uint4(0x1u,0x1u,0x1u,0x1u))).z; // native dword 5038
    } // native dword 5045
    r0.xy=(asuint((asfloat(asuint(-asfloat(giCB1[17u].yzyy)))+asfloat(giCB1[17u].xwxx)))).xy; // native dword 5046
    r0.z=(asuint((asfloat(giCB1[17u].yyyy)+asfloat(giCB1[17u].xxxx)))).z; // native dword 5058
    r0.z=(asuint((asfloat(asuint(-asfloat(r0.xxxx)))+asfloat(r0.zzzz)))).z; // native dword 5069
    r0.x=(asuint((asfloat(asuint(-asfloat(r0.xxxx)))+asfloat(r1.wwww)))).x; // native dword 5077
    r0.z=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r0.zzzz)))).z; // native dword 5085
    r0.x=(asuint(saturate((asfloat(r0.zzzz)*asfloat(r0.xxxx))))).x; // native dword 5095
    r0.z=(asuint(mad(asfloat(r0.xxxx),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).z; // native dword 5102
    r0.x=(asuint((asfloat(r0.xxxx)*asfloat(r0.xxxx)))).x; // native dword 5111
    r0.x=(asuint((asfloat(r0.xxxx)*asfloat(r0.zzzz)))).x; // native dword 5118
    r0.x=(asuint(mad(asfloat(r0.xxxx),asfloat(r0.yyyy),asfloat(giCB1[17u].zzzz)))).x; // native dword 5125
    r0.xyz=(asuint((asfloat(r0.xxxx)*asfloat(r10.xyzx)))).xyz; // native dword 5136
    r0.w=(asuint(float4(asint(giCB4[0u].zzzz)))).w; // native dword 5143
    r1.xyz=(asuint((asfloat(r0.xyzx)/asfloat(r0.wwww)))).xyz; // native dword 5150
    r0.x=(asuint((asfloat(r5.wwww)/asfloat(r0.wwww)))).x; // native dword 5157
    r1.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 5164
    } else { // native dword 5169
    r1.xyzw=(uint4(0x0u,0x0u,0x0u,0x0u)).xyzw; // native dword 5170
    r0.x=(uint4(0x477fe000u,0x477fe000u,0x477fe000u,0x477fe000u)).x; // native dword 5178
    } // native dword 5183
    g_rwtHitInfo[(uint4(dispatchId,0u).xyyy).xy]=asfloat(r0.xxxx); // native dword 5184
    g_rwtDiffuseGIColorTarget[(uint4(dispatchId,0u).xyyy).xy]=asfloat(r1.xyzw); // native dword 5191
    return; // native dword 5198
}
