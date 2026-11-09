// GENERATED from native rt_deferredReflection SHA256 4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9
// Native point/spot shadows, material BRDF, SH and GI-volume sampling.
#ifndef PT_REFLECTION_VERIFY_VARIANT
#define PT_REFLECTION_VERIFY_VARIANT 0
#endif
SamplerState g_sLinearWrap : register(s5, space1);
SamplerState g_sLinearClamp : register(s6, space1);
SamplerState g_sLinearMipLinearWrap : register(s10, space1);
SamplerState g_sLinearMipLinearClamp : register(s11, space1);
SamplerState g_sMaterialArrayGlobal : register(s19, space1);
Texture2D<float4> g_tGBuffer1 : register(t0, space0);
Texture2D<float4> g_tLinearDepth : register(t1, space0);
Texture2D<float4> g_tClipDepth : register(t2, space0);
struct PTNativeRaw3 { uint d[2]; };
StructuredBuffer<PTNativeRaw3> g_sbMaterialDataPart1 : register(t3, space0);
struct PTNativeRaw4 { uint d[84]; };
StructuredBuffer<PTNativeRaw4> g_sbMaterialDataPart3 : register(t4, space0);
Texture2D<uint4> g_tRaytracingLightCellPointLights : register(t5, space0);
Texture2D<uint4> g_tRaytracingLightCellSpotLights : register(t6, space0);
Texture2D<float4> g_tAmbientSHTerms : register(t7, space0);
TextureCube<float4> g_sEnvironmentCube : register(t8, space0);
Texture2D<float4> g_tPointLightShadowAtlas : register(t9, space0);
struct PTNativeRaw10 { uint d[40]; };
StructuredBuffer<PTNativeRaw10> g_sbDeferredLightPointData : register(t10, space0);
Texture2D<float4> g_tSpotLightPCFShadowAtlas : register(t11, space0);
Texture2D<float4> g_tSpotLightVSMShadowAtlas : register(t12, space0);
Texture2D<float4> g_tSpotLightProjectionAtlas : register(t13, space0);
struct PTNativeRaw14 { uint d[80]; };
StructuredBuffer<PTNativeRaw14> g_sbDeferredLightSpotData : register(t14, space0);
Texture2D<float4> g_tEnvBRDF : register(t15, space0);
struct PTNativeRaw16 { uint d[20]; };
StructuredBuffer<PTNativeRaw16> g_sbActiveVolumeSamplingInfoArray : register(t16, space0);
struct PTNativeRaw17 { uint d[1]; };
StructuredBuffer<PTNativeRaw17> g_sbTopLevelNodeIndicesBuffer : register(t17, space0);
struct PTNativeRaw18 { uint d[4]; };
StructuredBuffer<PTNativeRaw18> g_sbInternalNodesBuffer : register(t18, space0);
Texture3D<float4> g_tGIBrickTextures[6] : register(t19, space0);
Texture2D<float4> g_tTemporalBlueNoiseRGBA : register(t25, space0);
Texture2DArray<uint4> g_tMaterialId : register(t26, space0);
Texture2DArray<float4> g_tNormal_TexcoordX : register(t27, space0);
Texture2DArray<float4> g_tPosition_TexcoordY : register(t28, space0);
Texture2D<float4> g_sMaterialTextureArray[] : register(t0, space1);
RWTexture2D<float4> g_rwtReflectionTarget : register(u0, space0);
cbuffer sys_constants : register(b0) { uint4 ptCB0[71]; };
cbuffer deferredlight_constants : register(b1) { uint4 ptCB1[26]; };
cbuffer env_general : register(b2) { uint4 ptCB2[1]; };
cbuffer illuminationvolume : register(b3) { uint4 ptCB3[3]; };
cbuffer ManualUpdateCB_ActiveVolumeTopLevelInfoArray : register(b4) { uint4 ptCB4[1536]; };
cbuffer distancefield_CS : register(b5) { uint4 ptCB5[2]; };
cbuffer rtreflection : register(b6) { uint4 ptCB6[2]; };

uint4 PTBitExtract(uint4 width,uint4 offset,uint4 value) {
    width &= 31u; offset &= 31u;
    return (value>>offset)&((1u<<width)-1u);
}
uint4 PTBitInsert(uint4 width,uint4 offset,uint4 value,uint4 target) {
    width &= 31u; offset &= 31u;
    uint4 mask=((1u<<width)-1u)<<offset;
    return (target&~mask)|((value<<offset)&mask);
}
float PTNativeMaterialMip(uint index,bool quantized) {
    if(!quantized)return 0.0f;
    uint width,height,levels;
    g_sMaterialTextureArray[NonUniformResourceIndex(index)].GetDimensions(0,width,height,levels);
    return min(log2(max((float)max(width,height)/31.0f,1.0f)),(float)(max(levels,1u)-1u));
}
float3 PTNativeSecondHit(uint materialId,float2 uv,float3 hitView,float3 hitWorld,
                        float3 normalWorld,float3 incomingWorld,float3 incomingView,
                        float3 firstView,uint2 pixel,bool quantized,out float3 directEmission) {
    directEmission=0.0f;

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
    uint4 r30=0u;
    uint4 r31=0u;
    uint4 r32=0u;
    uint4 r33=0u;
    uint4 r34=0u;
    uint4 r35=0u;
    uint4 r36=0u;
    uint4 r37=0u;
    uint4 r38=0u;
    uint4 r39=0u;
    uint4 r40=0u;
    uint4 r41=0u;
    uint4 r42=0u;
    r2=asuint(float4(firstView,1.0f));
    r3.y=asuint(1.0f); // volume LOD uses actual segment distance
    r5.w=materialId;
    r14.z=0u;
    r15=asuint(float4(hitWorld,1.0f));
    r16.w=r17.w=asuint(1.0f);
    r19=asuint(float4(hitView,uv.y));
    r20=asuint(float4(normalWorld,uv.x));
    r21.xyz=asuint(incomingWorld);
    r13=asuint(g_tTemporalBlueNoiseRGBA.Load(int3(pixel&255u,0)));

    r4.x=(asuint((asfloat(ptCB1[15u].wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).x; // native dword 894
    r4.yzw=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x0u,0x0u,0x0u,0x0u)).xyz))).wxyz.yzw; // native dword 903
    r5.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x1u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 914
    r6.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x2u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 925
    r7.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x3u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 936
    r8.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x4u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 947
    r9.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x5u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 958
    r10.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x6u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 969
    r11.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x7u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 980
    r12.xyz=(asuint(g_tAmbientSHTerms.Load(asint(uint4(0x8u,0x0u,0x0u,0x0u)).xyz))).xyzw.xyz; // native dword 991
    r12.xyz=(asuint((asfloat(r12.xyzx)*asfloat(uint4(0x3f0bd89du,0x3f0bd89du,0x3f0bd89du,0x0u))))).xyz; // native dword 1002
    r11.xyz=(asuint((asfloat(r11.xyzx)*asfloat(uint4(0x3ea17b0fu,0x3ea17b0fu,0x3ea17b0fu,0x0u))))).xyz; // native dword 1012
    r22.xy=uint2(g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u]); // native dword 1222
    r7.w=((r5.wwww*uint4(0x23u,0x23u,0x23u,0x23u)+uint4(0x2u,0x2u,0x2u,0x2u))).w; // native dword 1232
    r23.x=(r20.wwww).x; // native dword 1241
    r23.y=(r19.wwww).y; // native dword 1246
    r7.w=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r7.wwww).x-0u)].SampleLevel(g_sMaterialArrayGlobal,asfloat(r23.xyxx).xy,PTNativeMaterialMip((r7.wwww).x,quantized)))).yzwx.w; // native dword 1251
    r8.w=(asuint((asfloat(asuint(-asfloat(r22.xxxx)))+asfloat(r22.yyyy)))).w; // native dword 1266
    r22.y=(asuint(mad(asfloat(r7.wwww),asfloat(r8.wwww),asfloat(r22.xxxx)))).y; // native dword 1274
    r7.w=(asuint(dot(asfloat(r21.xyzx).xyz,asfloat(r20.xyzx).xyz).xxxx)).w; // native dword 1283
    r7.w=(asuint((asfloat(r7.wwww)+asfloat(r7.wwww)))).w; // native dword 1290
    r23.xyz=(asuint(mad(asfloat(r20.xyzx),asfloat(asuint(-asfloat(r7.wwww))),asfloat(r21.xyzx)))).xyz; // native dword 1297
    r7.w=(asuint(dot(asfloat(r23.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).w; // native dword 1307
    r7.w=(asuint(rsqrt(asfloat(r7.wwww)))).w; // native dword 1314
    r23.xyz=(asuint((asfloat(r7.wwww)*asfloat(r23.xyzx)))).xyz; // native dword 1319
    r24.xyz=(r20.xyzx).xyz; // native dword 1326
    r25.xyz=(asuint(mad(asfloat(asuint(-asfloat(r2.xyzx))),asfloat(r2.wwww),asfloat(r19.xyzx)))).xyz; // native dword 1332
    r7.w=(asuint(dot(asfloat(r25.xyzx).xyz,asfloat(r25.xyzx).xyz).xxxx)).w; // native dword 1342
    r7.w=(asuint(sqrt(asfloat(r7.wwww)))).w; // native dword 1349
    r7.w=(asuint((asfloat(r7.wwww)*asfloat(ptCB5[1u].xxxx)))).w; // native dword 1354
    r7.w=(asuint((asfloat(r3.yyyy)*asfloat(r7.wwww)))).w; // native dword 1363
    r7.w=((uint4(asfloat(uint4(0x41800000u,0x41800000u,0x41800000u,0x41800000u))>=asfloat(r7.wwww))*0xffffffffu)).w; // native dword 1370
    r25.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1377
    r26.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1385
    r8.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 1393
    r9.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 1398
    r10.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 1403
    [loop] while(true) { // native dword 1408
    r11.w=((uint4(r10.wwww>=ptCB3[0u].wwww)*0xffffffffu)).w; // native dword 1409
    if((r11.wwww).x != 0u) break; // native dword 1418
    r11.w=((r10.wwww<<(uint4(0x2u,0x2u,0x2u,0x2u)&31u))).w; // native dword 1421
    r27.x=(asuint(dot(asfloat(ptCB4[(r11.wwww).x].xyzw).xyzw,asfloat(r15.xyzw).xyzw).xxxx)).x; // native dword 1428
    r27.y=(asuint(dot(asfloat(ptCB4[(1u+(r11.wwww).x)].xyzw).xyzw,asfloat(r15.xyzw).xyzw).xxxx)).y; // native dword 1438
    r27.z=(asuint(dot(asfloat(ptCB4[(2u+(r11.wwww).x)].xyzw).xyzw,asfloat(r15.xyzw).xyzw).xxxx)).z; // native dword 1449
    r28.xyz=((uint4(asfloat(r27.xyzx)>=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).xyz; // native dword 1460
    r12.w=((r28.yyyy&r28.xxxx)).w; // native dword 1470
    r12.w=((r28.zzzz&r12.wwww)).w; // native dword 1477
    r28.xyz=(asuint(float4(ptCB4[(3u+(r11.wwww).x)].xyzx))).xyz; // native dword 1484
    r29.xyz=((uint4(asfloat(r27.xyzx)<asfloat(r28.xyzx))*0xffffffffu)).xyz; // native dword 1493
    r18.w=((r29.yyyy&r29.xxxx)).w; // native dword 1500
    r18.w=((r29.zzzz&r18.wwww)).w; // native dword 1507
    r12.w=((r12.wwww&r18.wwww)).w; // native dword 1514
    if((r12.wwww).x != 0u) { // native dword 1521
    r29.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u]); // native dword 1524
    r30.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0xcu,0xcu,0xcu,0xcu)).x/4u+2u]); // native dword 1534
    r31.xyzw=uint4(g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+2u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x18u,0x18u,0x18u,0x18u)).x/4u+3u]); // native dword 1544
    r32.xyzw=uint4(g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+2u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x28u,0x28u,0x28u,0x28u)).x/4u+3u]); // native dword 1554
    r33.xyz=uint3(g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+0u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+1u],g_sbActiveVolumeSamplingInfoArray[(r10.wwww).x].d[(uint4(0x38u,0x38u,0x38u,0x38u)).x/4u+2u]); // native dword 1564
    r12.w=(asuint(saturate((asfloat(r27.xxxx)*asfloat(r31.wwww))))).w; // native dword 1574
    r28.xyz=(asuint((asfloat(asuint(-asfloat(r27.xyzx)))+asfloat(r28.xyzx)))).xyz; // native dword 1581
    r18.w=(asuint(saturate((asfloat(r31.wwww)*asfloat(r28.xxxx))))).w; // native dword 1589
    r12.w=(asuint((asfloat(r12.wwww)*asfloat(r18.wwww)))).w; // native dword 1596
    r22.zw=(asuint(saturate((asfloat(r27.yyyz)*asfloat(r32.xxxy))))).zw; // native dword 1603
    r12.w=(asuint((asfloat(r12.wwww)*asfloat(r22.zzzz)))).w; // native dword 1610
    r28.xy=(asuint(saturate((asfloat(r32.xyxx)*asfloat(r28.yzyy))))).xy; // native dword 1617
    r12.w=(asuint((asfloat(r12.wwww)*asfloat(r28.xxxx)))).w; // native dword 1624
    r12.w=(asuint((asfloat(r22.wwww)*asfloat(r12.wwww)))).w; // native dword 1631
    r12.w=(asuint((asfloat(r28.yyyy)*asfloat(r12.wwww)))).w; // native dword 1638
    r18.w=((uint4(r33.xxxx!=uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu))*0xffffffffu)).w; // native dword 1645
    if((r18.wwww).x != 0u) { // native dword 1652
    r28.xyz=(asuint(floor(asfloat(r27.xyzx)))).xyz; // native dword 1655
    r27.xyz=(asuint((asfloat(r27.xyzx)+asfloat(asuint(-asfloat(r28.xyzx)))))).xyz; // native dword 1660
    r28.xyz=(uint4(asfloat(r28.xyzx))).xyz; // native dword 1668
    r18.w=((r28.zzzz*ptCB4[(3u+(r11.wwww).x)].yyyy+r28.yyyy)).w; // native dword 1673
    r11.w=((r18.wwww*ptCB4[(3u+(r11.wwww).x)].xxxx+r28.xxxx)).w; // native dword 1686
    r11.w=((r32.zzzz+r11.wwww)).w; // native dword 1699
    r11.w=g_sbTopLevelNodeIndicesBuffer[(r11.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 1706
    r28.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 1716
    r32.xyz=(r27.xyzx).xyz; // native dword 1724
    r18.w=(r11.wwww).w; // native dword 1729
    r21.w=(r33.yyyy).w; // native dword 1734
    [loop] while(true) { // native dword 1739
    r22.z=((r18.wwww&uint4(0x80000000u,0x80000000u,0x80000000u,0x80000000u))).z; // native dword 1740
    r22.z=((uint4(r22.zzzz!=uint4(0x0u,0x0u,0x0u,0x0u))*0xffffffffu)).z; // native dword 1747
    r22.w=((uint4(asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))>=asfloat(r21.wwww))*0xffffffffu)).w; // native dword 1754
    r22.z=((r22.wwww|r22.zzzz)).z; // native dword 1761
    if((r22.zzzz).x != 0u) { // native dword 1768
    r22.z=((r18.wwww&uint4(0x7fffffffu,0x7fffffffu,0x7fffffffu,0x7fffffffu))).z; // native dword 1771
    r22.z=((r33.xxxx+r22.zzzz)).z; // native dword 1778
    r34.x=((r22.zzzz&uint4(0x7fu,0x7fu,0x7fu,0x7fu))).x; // native dword 1785
    r34.y=(PTBitExtract(uint4(0x7u,0x7u,0x7u,0x7u),uint4(0x7u,0x7u,0x7u,0x7u),r22.zzzz)).y; // native dword 1792
    r34.z=((r22.zzzz>>(uint4(0xeu,0xeu,0xeu,0xeu)&31u))).z; // native dword 1801
    r34.xyz=((r34.xyzx<<(uint4(0x2u,0x2u,0x2u,0x0u)&31u))).xyz; // native dword 1808
    r34.xyz=(asuint(float4(r34.xyzx))).xyz; // native dword 1818
    r34.xyz=(asuint(mad(asfloat(r32.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(r34.xyzx)))).xyz; // native dword 1823
    r28.xyz=(asuint((asfloat(r34.xyzx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 1835
    break; // native dword 1845
    } // native dword 1846
    r22.z=((r32.wwww+r18.wwww)).z; // native dword 1847
    r34.xyzw=uint4(g_sbInternalNodesBuffer[(r22.zzzz).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbInternalNodesBuffer[(r22.zzzz).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbInternalNodesBuffer[(r22.zzzz).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbInternalNodesBuffer[(r22.zzzz).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 1854
    r35.xyz=(asuint((asfloat(r32.xyzx)*asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u))))).xyz; // native dword 1864
    r35.xyz=(asuint(floor(asfloat(r35.xyzx)))).xyz; // native dword 1874
    r36.xyz=(uint4(asfloat(r35.xyzx))).xyz; // native dword 1879
    r22.zw=(asuint(float4(r36.zzzx))).zw; // native dword 1884
    r23.w=((r36.yyyy*uint4(0x3u,0x3u,0x3u,0x3u))).w; // native dword 1889
    r23.w=(asuint(float4(r23.wwww))).w; // native dword 1897
    r22.z=(asuint(mad(asfloat(r22.zzzz),asfloat(uint4(0x41100000u,0x41100000u,0x41100000u,0x41100000u)),asfloat(r23.wwww)))).z; // native dword 1902
    r22.z=(asuint((asfloat(r22.wwww)+asfloat(r22.zzzz)))).z; // native dword 1911
    r22.z=(uint4(asfloat(r22.zzzz))).z; // native dword 1918
    r22.w=((uint4(0x1u,0x1u,0x1u,0x1u)<<(r22.zzzz&31u))).w; // native dword 1923
    r23.w=((r22.wwww&r34.xxxx)).w; // native dword 1930
    if((r23.wwww).x == 0u) { // native dword 1937
    r23.w=((r33.xxxx+r18.wwww)).w; // native dword 1940
    r36.x=((r23.wwww&uint4(0x7fu,0x7fu,0x7fu,0x7fu))).x; // native dword 1947
    r36.y=(PTBitExtract(uint4(0x7u,0x7u,0x7u,0x7u),uint4(0x7u,0x7u,0x7u,0x7u),r23.wwww)).y; // native dword 1954
    r36.z=((r23.wwww>>(uint4(0xeu,0xeu,0xeu,0xeu)&31u))).z; // native dword 1963
    r36.xyz=((r36.xyzx<<(uint4(0x2u,0x2u,0x2u,0x0u)&31u))).xyz; // native dword 1970
    r36.xyz=(asuint(float4(r36.xyzx))).xyz; // native dword 1980
    r36.xyz=(asuint(mad(asfloat(r32.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(r36.xyzx)))).xyz; // native dword 1985
    r28.xyz=(asuint((asfloat(r36.xyzx)+asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 1997
    break; // native dword 2007
    } // native dword 2008
    r32.xyz=(asuint(mad(asfloat(r32.xyzx),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x0u)),asfloat(asuint(-asfloat(r35.xyzx)))))).xyz; // native dword 2009
    r22.w=((r22.wwww&r34.yyyy)).w; // native dword 2022
    r23.w=(PTBitExtract(r22.zzzz,uint4(0x0u,0x0u,0x0u,0x0u),r34.yyyy)).w; // native dword 2029
    r23.w=(countbits(r23.wwww)).w; // native dword 2038
    r23.w=((r23.wwww+r34.wwww)).w; // native dword 2043
    r23.w=((r23.wwww|uint4(0x80000000u,0x80000000u,0x80000000u,0x80000000u))).w; // native dword 2050
    r24.w=((~r34.yyyy)).w; // native dword 2057
    r24.w=((r24.wwww&r34.xxxx)).w; // native dword 2062
    r22.z=(PTBitExtract(r22.zzzz,uint4(0x0u,0x0u,0x0u,0x0u),r24.wwww)).z; // native dword 2069
    r22.z=(countbits(r22.zzzz)).z; // native dword 2078
    r22.z=((r22.zzzz+r34.zzzz)).z; // native dword 2083
    r18.w=((r22.wwww!=0u?r23.wwww:r22.zzzz)).w; // native dword 2090
    r21.w=(asuint((asfloat(r21.wwww)*asfloat(uint4(0x3eaaaaabu,0x3eaaaaabu,0x3eaaaaabu,0x3eaaaaabu))))).w; // native dword 2099
    r28.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 2106
    } // native dword 2114
    } else { // native dword 2115
    r28.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 2116
    } // native dword 2124
    r27.xyz=(asuint((asfloat(r28.xyzx)*asfloat(ptCB3[0u].xyzx)))).xyz; // native dword 2125
    r32.x=(r29.xxxx).x; // native dword 2134
    r32.y=(r30.xxxx).y; // native dword 2139
    r32.z=(r31.xxxx).z; // native dword 2144
    r34.x=(asuint(dot(asfloat(r32.xyzx).xyz,asfloat(r24.xyzx).xyz).xxxx)).x; // native dword 2149
    r35.x=(r29.yyyy).x; // native dword 2156
    r35.y=(r30.yyyy).y; // native dword 2161
    r35.z=(r31.yyyy).z; // native dword 2166
    r34.y=(asuint(dot(asfloat(r35.xyzx).xyz,asfloat(r24.xyzx).xyz).xxxx)).y; // native dword 2171
    r31.x=(r29.zzzz).x; // native dword 2178
    r31.y=(r30.zzzz).y; // native dword 2183
    r34.z=(asuint(dot(asfloat(r31.xyzx).xyz,asfloat(r24.xyzx).xyz).xxxx)).z; // native dword 2188
    r29.x=(asuint(dot(asfloat(r32.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).x; // native dword 2195
    r29.y=(asuint(dot(asfloat(r35.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).y; // native dword 2202
    r29.z=(asuint(dot(asfloat(r31.xyzx).xyz,asfloat(r23.xyzx).xyz).xxxx)).z; // native dword 2209
    r30.xyz=(asuint(saturate(asfloat(r34.xyzx)))).xyz; // native dword 2216
    r30.xyz=(asuint((asfloat(r30.xyzx)*asfloat(r30.xyzx)))).xyz; // native dword 2221
    r34.xyz=(asuint(saturate(asfloat(asuint(-asfloat(r34.xyzx)))))).xyz; // native dword 2228
    r31.xyz=(asuint((asfloat(r34.xyzx)*asfloat(r34.xyzx)))).xyz; // native dword 2234
    r32.xyz=(asuint(saturate(asfloat(r29.xyzx)))).xyz; // native dword 2241
    r32.xyz=(asuint((asfloat(r32.xyzx)*asfloat(r32.xyzx)))).xyz; // native dword 2246
    r29.xyz=(asuint(saturate(asfloat(asuint(-asfloat(r29.xyzx)))))).xyz; // native dword 2253
    r29.xyz=(asuint((asfloat(r29.xyzx)*asfloat(r29.xyzx)))).xyz; // native dword 2259
    r34.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r30.xyzx))*0xffffffffu)).xyz; // native dword 2266
    r35.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r32.xyzx))*0xffffffffu)).xyz; // native dword 2276
    r34.xyz=((r34.xyzx|r35.xyzx)).xyz; // native dword 2286
    if((r34.xxxx).x != 0u) { // native dword 2293
    r35.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(19u-19u)].SampleLevel(g_sLinearClamp,asfloat(r27.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2296
    r36.xyz=(asuint((asfloat(r30.xxxx)*asfloat(r35.xyzx)))).xyz; // native dword 2309
    r35.xyz=(asuint((asfloat(r32.xxxx)*asfloat(r35.xyzx)))).xyz; // native dword 2316
    } else { // native dword 2323
    r36.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 2324
    r35.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 2332
    } // native dword 2340
    r37.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r31.xyzx))*0xffffffffu)).xyz; // native dword 2341
    r38.xyz=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r29.xyzx))*0xffffffffu)).xyz; // native dword 2351
    r37.xyz=((r37.xyzx|r38.xyzx)).xyz; // native dword 2361
    if((r37.xxxx).x != 0u) { // native dword 2368
    r38.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(20u-19u)].SampleLevel(g_sLinearClamp,asfloat(r27.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2371
    r36.xyz=(asuint(mad(asfloat(r38.xyzx),asfloat(r31.xxxx),asfloat(r36.xyzx)))).xyz; // native dword 2384
    r35.xyz=(asuint(mad(asfloat(r38.xyzx),asfloat(r29.xxxx),asfloat(r35.xyzx)))).xyz; // native dword 2393
    } // native dword 2402
    if((r34.yyyy).x != 0u) { // native dword 2403
    r34.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(21u-19u)].SampleLevel(g_sLinearClamp,asfloat(r27.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 2406
    r36.xyz=(asuint(mad(asfloat(r34.xywx),asfloat(r30.yyyy),asfloat(r36.xyzx)))).xyz; // native dword 2419
    r35.xyz=(asuint(mad(asfloat(r34.xywx),asfloat(r32.yyyy),asfloat(r35.xyzx)))).xyz; // native dword 2428
    } // native dword 2437
    if((r37.yyyy).x != 0u) { // native dword 2438
    r30.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(22u-19u)].SampleLevel(g_sLinearClamp,asfloat(r27.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 2441
    r36.xyz=(asuint(mad(asfloat(r30.xywx),asfloat(r31.yyyy),asfloat(r36.xyzx)))).xyz; // native dword 2454
    r35.xyz=(asuint(mad(asfloat(r30.xywx),asfloat(r29.yyyy),asfloat(r35.xyzx)))).xyz; // native dword 2463
    } // native dword 2472
    if((r34.zzzz).x != 0u) { // native dword 2473
    r29.xyw=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(23u-19u)].SampleLevel(g_sLinearClamp,asfloat(r27.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xywz.xyw; // native dword 2476
    r36.xyz=(asuint(mad(asfloat(r29.xywx),asfloat(r30.zzzz),asfloat(r36.xyzx)))).xyz; // native dword 2489
    r35.xyz=(asuint(mad(asfloat(r29.xywx),asfloat(r32.zzzz),asfloat(r35.xyzx)))).xyz; // native dword 2498
    } // native dword 2507
    if((r37.zzzz).x != 0u) { // native dword 2508
    r27.xyz=(asuint(g_tGIBrickTextures[NonUniformResourceIndex(24u-19u)].SampleLevel(g_sLinearClamp,asfloat(r27.xyzx).xyz,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 2511
    r36.xyz=(asuint(mad(asfloat(r27.xyzx),asfloat(r31.zzzz),asfloat(r36.xyzx)))).xyz; // native dword 2524
    r35.xyz=(asuint(mad(asfloat(r27.xyzx),asfloat(r29.zzzz),asfloat(r35.xyzx)))).xyz; // native dword 2533
    } // native dword 2542
    r11.w=((uint4(r33.xxxx==uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu))*0xffffffffu)).w; // native dword 2543
    r27.xyz=((r11.wwww!=0u?uint4(0x3f800000u,0x3eb33333u,0x0u,0x0u):r36.xyzx)).xyz; // native dword 2550
    r29.xyz=((r11.wwww!=0u?uint4(0x3f800000u,0x3eb33333u,0x0u,0x0u):r35.xyzx)).xyz; // native dword 2562
    r11.w=(asuint(mad(asfloat(r12.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 2574
    r18.w=(asuint((asfloat(r12.wwww)*asfloat(r12.wwww)))).w; // native dword 2583
    r11.w=(asuint((asfloat(r11.wwww)*asfloat(r18.wwww)))).w; // native dword 2590
    r18.w=(asuint((asfloat(r33.zzzz)+asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 2597
    r11.w=(asuint(mad(asfloat(r11.wwww),asfloat(r18.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 2604
    r18.w=(asuint((asfloat(r11.wwww)*asfloat(r12.wwww)))).w; // native dword 2613
    r25.xyz=(asuint(mad(asfloat(r27.xyzx),asfloat(r18.wwww),asfloat(r25.xyzx)))).xyz; // native dword 2620
    r26.xyz=(asuint(mad(asfloat(r29.xyzx),asfloat(r18.wwww),asfloat(r26.xyzx)))).xyz; // native dword 2629
    r8.w=(asuint(mad(asfloat(r12.wwww),asfloat(r11.wwww),asfloat(r8.wwww)))).w; // native dword 2638
    r9.w=(uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu)).w; // native dword 2647
    } // native dword 2652
    r10.w=((r10.wwww+uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 2653
    } // native dword 2660
    r7.w=((r7.wwww&r9.wwww)).w; // native dword 2661
    if((r7.wwww).x != 0u) { // native dword 2668
    r7.w=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r8.wwww))*0xffffffffu)).w; // native dword 2671
    r27.xyz=(asuint((asfloat(r25.xyzx)/asfloat(r8.wwww)))).xyz; // native dword 2678
    r29.xyz=(asuint((asfloat(r26.xyzx)/asfloat(r8.wwww)))).xyz; // native dword 2685
    r27.xyz=((r7.wwww!=0u?r27.xyzx:r25.xyzx)).xyz; // native dword 2692
    r29.xyz=((r7.wwww!=0u?r29.xyzx:r26.xyzx)).xyz; // native dword 2701
    r27.xyz=(asuint((asfloat(r27.xyzx)+asfloat(ptCB3[1u].xyzx)))).xyz; // native dword 2710
    r29.xyz=(asuint((asfloat(r29.xyzx)+asfloat(ptCB3[1u].xyzx)))).xyz; // native dword 2719
    r27.xyz=(asuint((asfloat(r4.xxxx)*asfloat(r27.xyzx)))).xyz; // native dword 2728
    r29.xyz=(asuint((asfloat(r4.xxxx)*asfloat(r29.xyzx)))).xyz; // native dword 2735
    } else { // native dword 2742
    r30.xyz=(asuint((asfloat(r24.yyyy)*asfloat(ptCB0[7u].xzyx)))).xyz; // native dword 2743
    r24.xyw=(asuint(mad(asfloat(ptCB0[6u].xzxy),asfloat(r24.xxxx),asfloat(r30.xyxz)))).xyw; // native dword 2752
    r24.xyz=(asuint(mad(asfloat(ptCB0[8u].xzyx),asfloat(r24.zzzz),asfloat(r24.xywx)))).xyz; // native dword 2763
    r30.xyz=(asuint((asfloat(r7.xyzx)*asfloat(r24.yyyy)))).xyz; // native dword 2774
    r30.xyz=(asuint(mad(asfloat(r5.xyzx),asfloat(r24.xxxx),asfloat(r30.xyzx)))).xyz; // native dword 2781
    r30.xyz=(asuint(mad(asfloat(r6.xyzx),asfloat(r24.zzzz),asfloat(r30.xyzx)))).xyz; // native dword 2790
    r30.xyz=(asuint((asfloat(r30.xyzx)*asfloat(uint4(0x3efa2a1cu,0x3efa2a1cu,0x3efa2a1cu,0x0u))))).xyz; // native dword 2799
    r30.xyz=(asuint(mad(asfloat(r4.yzwy),asfloat(uint4(0x3e906ebau,0x3e906ebau,0x3e906ebau,0x0u)),asfloat(r30.xyzx)))).xyz; // native dword 2809
    r31.xyz=(asuint((asfloat(r10.xyzx)*asfloat(r24.xxxx)))).xyz; // native dword 2821
    r32.xyz=(asuint((asfloat(r8.xyzx)*asfloat(r24.xxxx)))).xyz; // native dword 2828
    r32.xyz=(asuint((asfloat(r24.zzzz)*asfloat(r32.xyzx)))).xyz; // native dword 2835
    r31.xyz=(asuint(mad(asfloat(r31.xyzx),asfloat(r24.yyyy),asfloat(r32.xyzx)))).xyz; // native dword 2842
    r32.xyz=(asuint((asfloat(r9.xyzx)*asfloat(r24.yyyy)))).xyz; // native dword 2851
    r31.xyz=(asuint(mad(asfloat(r32.xyzx),asfloat(r24.zzzz),asfloat(r31.xyzx)))).xyz; // native dword 2858
    r30.xyz=(asuint(mad(asfloat(r31.xyzx),asfloat(uint4(0x3f8bd89du,0x3f8bd89du,0x3f8bd89du,0x0u)),asfloat(r30.xyzx)))).xyz; // native dword 2867
    r22.zw=(asuint((asfloat(r24.yyyz)*asfloat(r24.yyyz)))).zw; // native dword 2879
    r7.w=(asuint(mad(asfloat(r24.xxxx),asfloat(r24.xxxx),asfloat(asuint(-asfloat(r22.zzzz)))))).w; // native dword 2886
    r24.xyz=(asuint(mad(asfloat(r12.xyzx),asfloat(r7.wwww),asfloat(r30.xyzx)))).xyz; // native dword 2896
    r7.w=(asuint(mad(asfloat(r22.wwww),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 2905
    r24.xyz=(asuint(mad(asfloat(r11.xyzx),asfloat(r7.wwww),asfloat(r24.xyzx)))).xyz; // native dword 2914
    r27.xyz=(asuint((asfloat(r24.xyzx)*asfloat(ptCB1[15u].yyyy)))).xyz; // native dword 2923
    r7.w=(asuint((asfloat(asuint(-asfloat(r22.yyyy)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 2932
    r7.w=(asuint((asfloat(r7.wwww)*asfloat(ptCB2[0u].yyyy)))).w; // native dword 2940
    r23.xyz=(asuint(g_sEnvironmentCube.SampleLevel(g_sLinearMipLinearWrap,asfloat(r23.xyzx).xyz,asfloat(r7.wwww).x))).xyzw.xyz; // native dword 2949
    r29.xyz=(asuint((asfloat(r23.xyzx)*asfloat(ptCB1[15u].yyyy)))).xyz; // native dword 2962
    } // native dword 2971
    if((r6.wwww).x == 0u) { // native dword 2972
    r23.xyzw=uint4(g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+0u],g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+1u],g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+2u],g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+3u]); // native dword 2975
    r24.xyz=uint3(g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x64u,0x64u,0x64u,0x64u)).x/4u+0u],g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x64u,0x64u,0x64u,0x64u)).x/4u+1u],g_sbMaterialDataPart3[(r5.wwww).x].d[(uint4(0x64u,0x64u,0x64u,0x64u)).x/4u+2u]); // native dword 2985
    r6.w=((r5.wwww*uint4(0x23u,0x23u,0x23u,0x23u))).w; // native dword 2995
    r7.w=((r5.wwww*uint4(0x23u,0x23u,0x23u,0x23u)+uint4(0x4u,0x4u,0x4u,0x4u))).w; // native dword 3003
    r30.x=(r20.wwww).x; // native dword 3012
    r30.y=(r19.wwww).y; // native dword 3017
    r7.w=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r7.wwww).x-0u)].SampleLevel(g_sMaterialArrayGlobal,asfloat(r30.xyxx).xy,PTNativeMaterialMip((r7.wwww).x,quantized)))).yzwx.w; // native dword 3022
    r10.w=g_sbMaterialDataPart1[(r5.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u]; // native dword 3037
    r11.w=((r10.wwww>>(uint4(0x18u,0x18u,0x18u,0x18u)&31u))).w; // native dword 3047
    r22.zw=(PTBitExtract(uint4(0x0u,0x0u,0x8u,0x8u),uint4(0x0u,0x0u,0x10u,0x8u),r10.wwww)).zw; // native dword 3054
    r31.x=(asuint(float4(r11.wwww))).x; // native dword 3069
    r31.yz=(asuint(float4(r22.zzwz))).yz; // native dword 3074
    r31.xyz=(asuint((asfloat(r7.wwww)*asfloat(r31.xyzx)))).xyz; // native dword 3079
    r31.xyz=(asuint((asfloat(r31.xyzx)*asfloat(uint4(0x3b808081u,0x3b808081u,0x3b808081u,0x0u))))).xyz; // native dword 3086
    r7.w=(asuint(dot(asfloat(r31.xyzx).xyz,asfloat(uint4(0x3e59b3d0u,0x3f371759u,0x3d93dd98u,0x0u)).xyz).xxxx)).w; // native dword 3096
    r22.x=(asuint(dot(asfloat(r20.xyzx).xyz,asfloat(asuint(-asfloat(r21.xyzx))).xyz).xxxx)).x; // native dword 3106
    r21.xyz=(asuint(g_tEnvBRDF.SampleLevel(g_sLinearClamp,asfloat(r22.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 3114
    r21.xyw=(asuint(mad(asfloat(r31.xyxz),asfloat(r21.xxxx),asfloat(r21.yyyy)))).xyw; // native dword 3127
    r22.xzw=(asuint((asfloat(asuint(-asfloat(r21.xxyw)))+asfloat(uint4(0x3f800000u,0x0u,0x3f800000u,0x3f800000u))))).xzw; // native dword 3136
    r22.xzw=(asuint((asfloat(r21.zzzz)*asfloat(r22.xxzw)))).xzw; // native dword 3147
    r31.xyz=(asuint((asfloat(r15.yyyy)*asfloat(ptCB0[3u].xyzx)))).xyz; // native dword 3154
    r31.xyz=(asuint(mad(asfloat(ptCB0[2u].xyzx),asfloat(r15.xxxx),asfloat(r31.xyzx)))).xyz; // native dword 3163
    r15.xyz=(asuint(mad(asfloat(ptCB0[4u].xyzx),asfloat(r15.zzzz),asfloat(r31.xyzx)))).xyz; // native dword 3174
    r15.xyz=(asuint((asfloat(r15.xyzx)+asfloat(ptCB0[5u].xyzx)))).xyz; // native dword 3185
    r31.xyz=(asuint((asfloat(r20.yyyy)*asfloat(ptCB0[3u].xyzx)))).xyz; // native dword 3194
    r20.xyw=(asuint(mad(asfloat(ptCB0[2u].xyxz),asfloat(r20.xxxx),asfloat(r31.xyxz)))).xyw; // native dword 3203
    r20.xyz=(asuint(mad(asfloat(ptCB0[4u].xyzx),asfloat(r20.zzzz),asfloat(r20.xywx)))).xyz; // native dword 3214
    r19.xyz=(asuint((asfloat(r19.xyzx)+asfloat(uint4(0x43160000u,0x42960000u,0x42960000u,0x0u))))).xyz; // native dword 3225
    r19.xyz=(asuint((asfloat(r19.xyzx)*asfloat(uint4(0x3d5a740eu,0x3d5a740eu,0x3d5a740eu,0x0u))))).xyz; // native dword 3235
    r19.xyz=(asuint(floor(asfloat(r19.xyzx)))).xyz; // native dword 3245
    r19.xyz=(asuint(max(asfloat(r19.xyzx),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).xyz; // native dword 3250
    r19.xyz=(asuint(min(asfloat(r19.xyzx),asfloat(uint4(0x41700000u,0x40e00000u,0x41300000u,0x0u))))).xyz; // native dword 3260
    r19.xyz=(asuint(int4(asfloat(r19.xyzx)))).xyz; // native dword 3270
    r14.x=((r19.xxxx*uint4(0xcu,0xcu,0xcu,0xcu)+r19.zzzz)).x; // native dword 3275
    r14.y=((r19.yyyy<<(uint4(0x8u,0x8u,0x8u,0x8u)&31u))).y; // native dword 3284
    r10.w=(g_tRaytracingLightCellPointLights.Load(asint(r14.xyzz).xyz)).yzwx.w; // native dword 3291
    r17.xyz=(asuint(mad(asfloat(r20.xyzx),asfloat(uint4(0x3d8f5c29u,0x3d8f5c29u,0x3d8f5c29u,0x0u)),asfloat(r15.xyzx)))).xyz; // native dword 3299
    r11.w=(asuint((asfloat(asuint(-asfloat(r22.yyyy)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 3311
    r19.xyz=asuint(-incomingView); // native dword 3333
    r12.w=(asuint(saturate(dot(asfloat(r20.xyzx).xyz,asfloat(r19.xyzx).xyz).xxxx))).w; // native dword 3341
    r18.w=(asuint((asfloat(asuint(-asfloat(r7.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 3348
    r31.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 3356
    r32.xyz=(uint4(0x0u,0x0u,0x0u,0x0u)).xyz; // native dword 3364
    r19.w=(uint4(0x0u,0x0u,0x0u,0x0u)).w; // native dword 3372
    r20.w=(r10.wwww).w; // native dword 3377
    [loop] while(true) { // native dword 3382
    r21.z=((uint4(r20.wwww!=uint4(0xffffu,0xffffu,0xffffu,0xffffu))*0xffffffffu)).z; // native dword 3383
    r24.w=((uint4(asint(r19.wwww)<asint(uint4(0x100u,0x100u,0x100u,0x100u)))*0xffffffffu)).w; // native dword 3390
    r21.z=((r21.zzzz&r24.wwww)).z; // native dword 3397
    if((r21.zzzz).x == 0u) break; // native dword 3404
    r33.xyzw=uint4(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 3407
    r34.xyzw=uint4(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+2u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+3u]); // native dword 3417
    r35.xyzw=uint4(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+2u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x24u,0x24u,0x24u,0x24u)).x/4u+3u]); // native dword 3427
    r36.xyzw=uint4(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+2u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x34u,0x34u,0x34u,0x34u)).x/4u+3u]); // native dword 3437
    r21.z=g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x90u,0x90u,0x90u,0x90u)).x/4u+0u]; // native dword 3447
    r24.w=g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x9cu,0x9cu,0x9cu,0x9cu)).x/4u+0u]; // native dword 3457
    r33.xyz=(asuint((asfloat(asuint(-asfloat(r15.xyzx)))+asfloat(r33.xyzx)))).xyz; // native dword 3467
    r25.w=(asuint(dot(asfloat(r33.xyzx).xyz,asfloat(r33.xyzx).xyz).xxxx)).w; // native dword 3475
    r26.w=(asuint(rsqrt(asfloat(r25.wwww)))).w; // native dword 3482
    r37.xyz=(asuint((asfloat(r26.wwww)*asfloat(r33.xyzx)))).xyz; // native dword 3487
    r27.w=(asuint(sqrt(asfloat(r25.wwww)))).w; // native dword 3494
    r28.w=(asuint((asfloat(asuint(-asfloat(r27.wwww)))+asfloat(r34.wwww)))).w; // native dword 3499
    r29.w=(asuint((asfloat(r34.wwww)*asfloat(uint4(0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu))))).w; // native dword 3507
    r28.w=(asuint(saturate((asfloat(r28.wwww)/asfloat(r29.wwww))))).w; // native dword 3514
    r29.w=(asuint(mad(asfloat(r28.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 3521
    r28.w=(asuint((asfloat(r28.wwww)*asfloat(r28.wwww)))).w; // native dword 3530
    r28.w=(asuint((asfloat(r28.wwww)*asfloat(r29.wwww)))).w; // native dword 3537
    r29.w=(asuint((asfloat(r21.zzzz)*asfloat(r21.zzzz)))).w; // native dword 3544
    r30.z=(asuint(mad(asfloat(r21.zzzz),asfloat(r21.zzzz),asfloat(r25.wwww)))).z; // native dword 3551
    r30.z=(asuint(max(asfloat(r30.zzzz),asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u))))).z; // native dword 3560
    r30.z=(asuint((asfloat(r30.zzzz)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).z; // native dword 3567
    r30.z=(asuint((1.0f/asfloat(r30.zzzz)))).z; // native dword 3574
    r30.z=(asuint((asfloat(asuint(-asfloat(r35.xxxx)))+asfloat(r30.zzzz)))).z; // native dword 3579
    r30.z=(asuint(max(asfloat(r30.zzzz),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).z; // native dword 3587
    r28.w=(asuint((asfloat(r28.wwww)*asfloat(r30.zzzz)))).w; // native dword 3594
    r34.xyz=(asuint((asfloat(r28.wwww)*asfloat(r34.xyzx)))).xyz; // native dword 3601
    r30.zw=((r24.wwww&uint4(0x0u,0x0u,0x4u,0x2u))).zw; // native dword 3608
    r38.x=(r35.wwww).x; // native dword 3618
    r38.yz=(r36.xxyx).yz; // native dword 3623
    r24.w=(asuint(dot(asfloat(asuint(-asfloat(r37.xyzx))).xyz,asfloat(r38.xyzx).xyz).xxxx)).w; // native dword 3628
    r24.w=(asuint(saturate(mad(asfloat(r24.wwww),asfloat(r35.yyyy),asfloat(r35.zzzz))))).w; // native dword 3636
    r24.w=(asuint((asfloat(r24.wwww)*asfloat(r24.wwww)))).w; // native dword 3645
    r35.xyz=(asuint((asfloat(r24.wwww)*asfloat(r34.xyzx)))).xyz; // native dword 3652
    r34.xyz=((r30.zzzz!=0u?r35.xyzx:r34.xyzx)).xyz; // native dword 3659
    if((r30.wwww).x != 0u) { // native dword 3668
    r30.zw=uint2(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u]); // native dword 3671
    r35.xyz=uint3(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x50u,0x50u,0x50u,0x50u)).x/4u+2u]); // native dword 3681
    r38.xyz=uint3(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+2u]); // native dword 3691
    r39.xyw=uint3(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+2u]); // native dword 3701
    r40.xyzw=uint4(g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+0u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+1u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+2u],g_sbDeferredLightPointData[(r20.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+3u]); // native dword 3711
    r41.x=(r36.wwww).x; // native dword 3721
    r41.y=(r35.xxxx).y; // native dword 3726
    r41.z=(r38.xxxx).z; // native dword 3731
    r41.w=(r39.xxxx).w; // native dword 3736
    r41.x=(asuint(dot(asfloat(r41.xyzw).xyzw,asfloat(r17.xyzw).xyzw).xxxx)).x; // native dword 3741
    r42.x=(r30.zzzz).x; // native dword 3748
    r42.y=(r35.yyyy).y; // native dword 3753
    r42.z=(r38.yyyy).z; // native dword 3758
    r42.w=(r39.yyyy).w; // native dword 3763
    r41.y=(asuint(dot(asfloat(r42.xyzw).xyzw,asfloat(r17.xyzw).xyzw).xxxx)).y; // native dword 3768
    r39.x=(r30.wwww).x; // native dword 3775
    r39.y=(r35.zzzz).y; // native dword 3780
    r39.z=(r38.zzzz).z; // native dword 3785
    r41.z=(asuint(dot(asfloat(r39.xyzw).xyzw,asfloat(r17.xyzw).xyzw).xxxx)).z; // native dword 3790
    r24.w=(asuint(dot(asfloat(r41.xyzx).xyz,asfloat(r41.xyzx).xyz).xxxx)).w; // native dword 3797
    r24.w=(asuint(rsqrt(asfloat(r24.wwww)))).w; // native dword 3804
    r35.xyz=(asuint((asfloat(r24.wwww)*asfloat(r41.xyzx)))).xyz; // native dword 3809
    r38.xyzw=((uint4(asfloat(asuint(abs(asfloat(r35.yzxz))))<asfloat(asuint(abs(asfloat(r35.xxyy)))))*0xffffffffu)).xyzw; // native dword 3816
    r36.xyw=((uint4(asfloat(uint4(0x0u,0x0u,0x0u,0x0u))<asfloat(r35.xyxz))*0xffffffffu)).xyw; // native dword 3825
    r30.zw=((r38.yyyw&r38.xxxz)).zw; // native dword 3835
    r36.xyw=((r36.xyxw!=0u?uint4(0x0u,0x2u,0x0u,0x4u):uint4(0x1u,0x3u,0x0u,0x5u))).xyw; // native dword 3842
    r24.w=((r30.wwww!=0u?r36.yyyy:r36.wwww)).w; // native dword 3857
    r24.w=((r30.zzzz!=0u?r36.xxxx:r24.wwww)).w; // native dword 3866
    if((r24.wwww).x != 0u) { // native dword 3875
    r28.w=((uint4(r24.wwww!=uint4(0x1u,0x1u,0x1u,0x1u))*0xffffffffu)).w; // native dword 3878
    if((r28.wwww).x != 0u) { // native dword 3885
    r38.xyz=(asuint((asfloat(r35.xxyx)/asfloat(r35.yzzy)))).xyz; // native dword 3888
    r28.w=(asuint((asfloat(asuint(-asfloat(r35.zzzz)))/asfloat(r35.yyyy)))).w; // native dword 3895
    r39.y=(asuint(mad(asfloat(r28.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).y; // native dword 3903
    r30.zw=(asuint((asfloat(r35.xxxz)/asfloat(asuint(-asfloat(r35.yyyy)))))).zw; // native dword 3912
    r30.zw=(asuint(mad(asfloat(r30.zzzw),asfloat(uint4(0x0u,0x0u,0x3f000000u,0xbf000000u)),asfloat(uint4(0x0u,0x0u,0x3f000000u,0x3f000000u))))).zw; // native dword 3920
    r41.xyz=((uint4(r24.wwww==uint4(0x2u,0x3u,0x4u,0x0u))*0xffffffffu)).xyz; // native dword 3935
    r38.xyz=(asuint(mad(asfloat(r38.xyzx),asfloat(uint4(0x3f000000u,0x3f000000u,0xbf000000u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x0u))))).xyz; // native dword 3945
    r28.w=(asuint((asfloat(r35.yyyy)/asfloat(asuint(-asfloat(r35.zzzz)))))).w; // native dword 3960
    r38.w=(asuint(mad(asfloat(r28.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).w; // native dword 3968
    r35.xy=((r41.zzzz!=0u?r38.yzyy:r38.ywyy)).xy; // native dword 3977
    r30.zw=((r41.yyyy!=0u?r30.zzzw:r35.xxxy)).zw; // native dword 3986
    r39.x=(r38.xxxx).x; // native dword 3995
    r36.xy=((r41.xxxx!=0u?r39.xyxx:r30.zwzz)).xy; // native dword 4000
    } else { // native dword 4009
    r30.zw=(asuint((asfloat(r35.zzzy)/asfloat(asuint(-asfloat(r35.xxxx)))))).zw; // native dword 4010
    r36.xy=(asuint(mad(asfloat(r30.zwzz),asfloat(uint4(0x3f000000u,0xbf000000u,0x0u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 4018
    } // native dword 4033
    } else { // native dword 4034
    r28.w=(asuint((asfloat(asuint(-asfloat(r35.zzzz)))/asfloat(r35.xxxx)))).w; // native dword 4035
    r36.x=(asuint(mad(asfloat(r28.wwww),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).x; // native dword 4043
    r28.w=(asuint((asfloat(r35.yyyy)/asfloat(r35.xxxx)))).w; // native dword 4052
    r36.y=(asuint(mad(asfloat(r28.wwww),asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f000000u,0x3f000000u))))).y; // native dword 4059
    } // native dword 4068
    r30.zw=(asuint((asfloat(r36.xxxy)+asfloat(uint4(0x0u,0x0u,0xbf000000u,0xbf000000u))))).zw; // native dword 4069
    r35.yz=(asuint(mad(asfloat(r30.zzwz),asfloat(r33.wwww),asfloat(uint4(0x0u,0x3f000000u,0x3f000000u,0x0u))))).yz; // native dword 4079
    r24.w=(asuint(float4(r24.wwww))).w; // native dword 4091
    r24.w=(asuint((asfloat(r24.wwww)*asfloat(uint4(0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu))))).w; // native dword 4096
    r35.x=(asuint(mad(asfloat(r35.yyyy),asfloat(uint4(0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu,0x3e2aaaabu)),asfloat(r24.wwww)))).x; // native dword 4103
    r30.zw=(asuint(mad(asfloat(r35.xxxz),asfloat(r40.zzzw),asfloat(r40.xxxy)))).zw; // native dword 4112
    r30.zw=(asuint(g_tPointLightShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r30.zwzz).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).zwxy.zw; // native dword 4121
    r24.w=(asuint((asfloat(r27.wwww)+asfloat(uint4(0xbd4ccccdu,0xbd4ccccdu,0xbd4ccccdu,0xbd4ccccdu))))).w; // native dword 4134
    r28.w=(asuint((asfloat(r36.zzzz)*asfloat(r24.wwww)))).w; // native dword 4141
    r30.w=(asuint(mad(asfloat(asuint(-asfloat(r30.zzzz))),asfloat(r30.zzzz),asfloat(r30.wwww)))).w; // native dword 4148
    r30.w=(asuint(max(asfloat(r30.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 4158
    r24.w=(asuint(mad(asfloat(asuint(-asfloat(r24.wwww))),asfloat(r36.zzzz),asfloat(r30.zzzz)))).w; // native dword 4165
    r24.w=(asuint(mad(asfloat(r24.wwww),asfloat(r24.wwww),asfloat(r30.wwww)))).w; // native dword 4175
    r24.w=(asuint((asfloat(r30.wwww)/asfloat(r24.wwww)))).w; // native dword 4184
    r24.w=(asuint(min(asfloat(r24.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4191
    r28.w=((uint4(asfloat(r28.wwww)<asfloat(r30.zzzz))*0xffffffffu)).w; // native dword 4198
    r24.w=((r28.wwww!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r24.wwww)).w; // native dword 4205
    r34.xyz=(asuint((asfloat(r24.wwww)*asfloat(r34.xyzx)))).xyz; // native dword 4214
    } // native dword 4221
    r24.w=(asuint(dot(asfloat(r37.xyzx).xyz,asfloat(r20.xyzx).xyz).xxxx)).w; // native dword 4222
    r28.w=(asuint((asfloat(r21.zzzz)*asfloat(r27.wwww)))).w; // native dword 4229
    r28.w=((uint4(asfloat(r28.wwww)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 4236
    r29.w=(asuint((asfloat(r29.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 4243
    r25.w=(asuint((asfloat(r25.wwww)*asfloat(uint4(0x40c90fdbu,0x40c90fdbu,0x40c90fdbu,0x40c90fdbu))))).w; // native dword 4250
    r27.w=(asuint((asfloat(r27.wwww)*asfloat(r24.wwww)))).w; // native dword 4257
    r21.z=(asuint((asfloat(r27.wwww)/asfloat(r21.zzzz)))).z; // native dword 4264
    r21.z=(asuint((asfloat(r21.zzzz)+asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).z; // native dword 4271
    r21.z=(asuint(saturate((asfloat(r21.zzzz)*asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)))))).z; // native dword 4278
    r21.z=(asuint(mad(asfloat(r21.zzzz),asfloat(uint4(0x40000000u,0x40000000u,0x40000000u,0x40000000u)),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).z; // native dword 4285
    r27.w=(asuint(mad(asfloat(asuint(-asfloat(r21.zzzz))),asfloat(r21.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4294
    r27.w=(asuint(sqrt(asfloat(r27.wwww)))).w; // native dword 4304
    r30.z=(asuint((asfloat(asuint(-abs(asfloat(r21.zzzz))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4309
    r30.z=(asuint(sqrt(asfloat(r30.zzzz)))).z; // native dword 4317
    r30.w=(asuint(mad(asfloat(asuint(abs(asfloat(r21.zzzz)))),asfloat(uint4(0xbc996e30u,0xbc996e30u,0xbc996e30u,0xbc996e30u)),asfloat(uint4(0x3d981627u,0x3d981627u,0x3d981627u,0x3d981627u))))).w; // native dword 4322
    r30.w=(asuint(mad(asfloat(r30.wwww),asfloat(asuint(abs(asfloat(r21.zzzz)))),asfloat(uint4(0xbe593484u,0xbe593484u,0xbe593484u,0xbe593484u))))).w; // native dword 4332
    r30.w=(asuint(mad(asfloat(r30.wwww),asfloat(asuint(abs(asfloat(r21.zzzz)))),asfloat(uint4(0x3fc90da4u,0x3fc90da4u,0x3fc90da4u,0x3fc90da4u))))).w; // native dword 4342
    r31.w=(asuint((asfloat(r30.zzzz)*asfloat(r30.wwww)))).w; // native dword 4352
    r31.w=(asuint(mad(asfloat(r31.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 4359
    r32.w=((uint4(asfloat(r21.zzzz)<asfloat(asuint(-asfloat(r21.zzzz))))*0xffffffffu)).w; // native dword 4368
    r31.w=((r31.wwww&r32.wwww)).w; // native dword 4376
    r30.z=(asuint(mad(asfloat(r30.wwww),asfloat(r30.zzzz),asfloat(r31.wwww)))).z; // native dword 4383
    r30.z=(asuint((asfloat(asuint(-asfloat(r30.zzzz)))+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).z; // native dword 4392
    r21.z=(asuint(mad(asfloat(asuint(-asfloat(r27.wwww))),asfloat(r21.zzzz),asfloat(asuint(-asfloat(r30.zzzz)))))).z; // native dword 4400
    r21.z=(asuint((asfloat(r21.zzzz)+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).z; // native dword 4411
    r21.z=(asuint(saturate((asfloat(r21.zzzz)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u)))))).z; // native dword 4418
    r21.z=(asuint((asfloat(r21.zzzz)*asfloat(r29.wwww)))).z; // native dword 4425
    r21.z=(asuint((asfloat(r21.zzzz)/asfloat(r25.wwww)))).z; // native dword 4432
    r21.z=(asuint(min(asfloat(r21.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4439
    r25.w=((uint4(asfloat(uint4(0x39aec33fu,0x39aec33fu,0x39aec33fu,0x39aec33fu))<asfloat(r21.zzzz))*0xffffffffu)).w; // native dword 4446
    r21.z=(asuint(mad(asfloat(r21.zzzz),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4453
    r21.z=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r21.zzzz)))).z; // native dword 4462
    r27.w=(asuint((asfloat(r21.zzzz)*asfloat(r21.zzzz)))).w; // native dword 4472
    r27.w=(asuint((asfloat(r21.zzzz)*asfloat(r27.wwww)))).w; // native dword 4479
    r27.w=(asuint(mad(asfloat(r21.zzzz),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(asuint(-asfloat(r27.wwww)))))).w; // native dword 4486
    r21.z=(asuint(mad(asfloat(asuint(-asfloat(r21.zzzz))),asfloat(r21.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4496
    r21.z=(asuint((asfloat(r27.wwww)/asfloat(r21.zzzz)))).z; // native dword 4506
    r21.z=(asuint((asfloat(r21.zzzz)+asfloat(r21.zzzz)))).z; // native dword 4513
    r21.z=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r21.zzzz)))).z; // native dword 4520
    r21.z=(asuint(mad(asfloat(r11.wwww),asfloat(r11.wwww),asfloat(r21.zzzz)))).z; // native dword 4530
    r21.z=(asuint(sqrt(asfloat(r21.zzzz)))).z; // native dword 4539
    r21.z=((r25.wwww!=0u?r21.zzzz:r11.wwww)).z; // native dword 4544
    r21.z=(asuint((asfloat(asuint(-asfloat(r21.zzzz)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4553
    r21.z=((r28.wwww!=0u?r21.zzzz:r22.yyyy)).z; // native dword 4561
    r33.xyz=(asuint(mad(asfloat(r33.xyzx),asfloat(r26.wwww),asfloat(r19.xyzx)))).xyz; // native dword 4570
    r25.w=(asuint(dot(asfloat(r33.xyzx).xyz,asfloat(r33.xyzx).xyz).xxxx)).w; // native dword 4579
    r25.w=(asuint(rsqrt(asfloat(r25.wwww)))).w; // native dword 4586
    r33.xyz=(asuint((asfloat(r25.wwww)*asfloat(r33.xyzx)))).xyz; // native dword 4591
    r25.w=(asuint(dot(asfloat(r19.xyzx).xyz,asfloat(r33.xyzx).xyz).xxxx)).w; // native dword 4598
    r26.w=(asuint(dot(asfloat(r20.xyzx).xyz,asfloat(r33.xyzx).xyz).xxxx)).w; // native dword 4605
    r21.z=(asuint((asfloat(asuint(-asfloat(r21.zzzz)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4612
    r21.z=(asuint(max(asfloat(r21.zzzz),asfloat(uint4(0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu))))).z; // native dword 4620
    r21.z=(asuint(min(asfloat(r21.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4627
    r21.z=(asuint((asfloat(r21.zzzz)*asfloat(r21.zzzz)))).z; // native dword 4634
    r25.w=(asuint((asfloat(asuint(-abs(asfloat(r25.wwww))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4641
    r25.w=(asuint(max(asfloat(r25.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 4649
    r27.w=(asuint((asfloat(r25.wwww)*asfloat(r25.wwww)))).w; // native dword 4656
    r27.w=(asuint((asfloat(r27.wwww)*asfloat(r27.wwww)))).w; // native dword 4663
    r25.w=(asuint((asfloat(r25.wwww)*asfloat(r27.wwww)))).w; // native dword 4670
    r25.w=(asuint(mad(asfloat(r18.wwww),asfloat(r25.wwww),asfloat(r7.wwww)))).w; // native dword 4677
    r27.w=(asuint((asfloat(r21.zzzz)*asfloat(r21.zzzz)))).w; // native dword 4686
    r26.w=(asuint((asfloat(r26.wwww)*asfloat(r26.wwww)))).w; // native dword 4693
    r28.w=(asuint(mad(asfloat(r21.zzzz),asfloat(r21.zzzz),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 4700
    r26.w=(asuint(mad(asfloat(r26.wwww),asfloat(r28.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 4709
    r26.w=(asuint((asfloat(r26.wwww)*asfloat(r26.wwww)))).w; // native dword 4718
    r26.w=(asuint((asfloat(r26.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 4725
    r26.w=(asuint(max(asfloat(r26.wwww),asfloat(uint4(0x3a83126fu,0x3a83126fu,0x3a83126fu,0x3a83126fu))))).w; // native dword 4732
    r26.w=(asuint((asfloat(r27.wwww)/asfloat(r26.wwww)))).w; // native dword 4739
    r24.w=(asuint(saturate(asfloat(r24.wwww)))).w; // native dword 4746
    r21.z=(asuint(mad(asfloat(asuint(-asfloat(r21.zzzz))),asfloat(r21.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 4751
    r28.w=(asuint(mad(asfloat(r24.wwww),asfloat(r21.zzzz),asfloat(r27.wwww)))).w; // native dword 4761
    r21.z=(asuint(mad(asfloat(r12.wwww),asfloat(r21.zzzz),asfloat(r27.wwww)))).z; // native dword 4770
    r21.z=(asuint((asfloat(r21.zzzz)*asfloat(r28.wwww)))).z; // native dword 4779
    r21.z=(asuint(max(asfloat(r21.zzzz),asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u))))).z; // native dword 4786
    r21.z=(asuint((asfloat(uint4(0x3e800000u,0x3e800000u,0x3e800000u,0x3e800000u))/asfloat(r21.zzzz)))).z; // native dword 4793
    r27.w=(asuint(saturate((asfloat(asuint(-asfloat(r25.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)))))).w; // native dword 4800
    r27.w=(asuint((asfloat(r24.wwww)*asfloat(r27.wwww)))).w; // native dword 4808
    r27.w=(asuint((asfloat(r27.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).w; // native dword 4815
    r25.w=(asuint((asfloat(r25.wwww)*asfloat(r26.wwww)))).w; // native dword 4822
    r21.z=(asuint((asfloat(r21.zzzz)*asfloat(r25.wwww)))).z; // native dword 4829
    r21.z=(asuint((asfloat(r24.wwww)*asfloat(r21.zzzz)))).z; // native dword 4836
    r31.xyz=(asuint(mad(asfloat(r27.wwww),asfloat(r34.xyzx),asfloat(r31.xyzx)))).xyz; // native dword 4843
    r32.xyz=(asuint(mad(asfloat(r21.zzzz),asfloat(r34.xyzx),asfloat(r32.xyzx)))).xyz; // native dword 4852
    r19.w=((r19.wwww+uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 4861
    r14.w=((r14.yyyy+r19.wwww)).w; // native dword 4868
    r20.w=(g_tRaytracingLightCellPointLights.Load(asint(r14.xwzz).xyz)).yzwx.w; // native dword 4875
    } // native dword 4883
    r10.w=(g_tRaytracingLightCellSpotLights.Load(asint(r14.xyzz).xyz)).yzwx.w; // native dword 4884
    r17.xyz=(asuint((asfloat(r15.zzzz)*asfloat(r20.xyzx)))).xyz; // native dword 4892
    r16.xyz=(asuint(mad(asfloat(r17.xyzx),asfloat(uint4(0x399d4952u,0x399d4952u,0x399d4952u,0x0u)),asfloat(r15.xyzx)))).xyz; // native dword 4899
    r33.xzw=(r14.xxzz).xzw; // native dword 4911
    r17.xyz=(r31.xyzx).xyz; // native dword 4916
    r34.xyz=(r32.xyzx).xyz; // native dword 4921
    r14.x=(uint4(0x0u,0x0u,0x0u,0x0u)).x; // native dword 4926
    r14.w=(r10.wwww).w; // native dword 4931
    [loop] while(true) { // native dword 4936
    r19.w=((uint4(r14.wwww!=uint4(0xffffu,0xffffu,0xffffu,0xffffu))*0xffffffffu)).w; // native dword 4937
    r20.w=((uint4(asint(r14.xxxx)<asint(uint4(0x100u,0x100u,0x100u,0x100u)))*0xffffffffu)).w; // native dword 4944
    r19.w=((r19.wwww&r20.wwww)).w; // native dword 4951
    if((r19.wwww).x == 0u) break; // native dword 4958
    r14.x=((r14.xxxx+uint4(0x1u,0x1u,0x1u,0x1u))).x; // native dword 4961
    r33.y=((r14.xxxx+r14.yyyy)).y; // native dword 4968
    r19.w=(g_tRaytracingLightCellSpotLights.Load(asint(r33.xyzw).xyz)).yzwx.w; // native dword 4975
    r35.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x44u,0x44u,0x44u,0x44u)).x/4u+3u]); // native dword 4983
    r36.xyz=uint3(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x54u,0x54u,0x54u,0x54u)).x/4u+2u]); // native dword 4993
    r37.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x60u,0x60u,0x60u,0x60u)).x/4u+3u]); // native dword 5003
    r38.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x70u,0x70u,0x70u,0x70u)).x/4u+3u]); // native dword 5013
    r39.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x80u,0x80u,0x80u,0x80u)).x/4u+3u]); // native dword 5023
    r40.x=(r35.wwww).x; // native dword 5033
    r40.y=(r37.xxxx).y; // native dword 5038
    r40.z=(r38.xxxx).z; // native dword 5043
    r40.w=(r39.xxxx).w; // native dword 5048
    r40.x=(asuint(dot(asfloat(r40.xyzw).xyzw,asfloat(r16.xyzw).xyzw).xxxx)).x; // native dword 5053
    r41.x=(r36.xxxx).x; // native dword 5060
    r41.y=(r37.yyyy).y; // native dword 5065
    r41.z=(r38.yyyy).z; // native dword 5070
    r41.w=(r39.yyyy).w; // native dword 5075
    r40.y=(asuint(dot(asfloat(r41.xyzw).xyzw,asfloat(r16.xyzw).xyzw).xxxx)).y; // native dword 5080
    r41.x=(r36.yyyy).x; // native dword 5087
    r41.y=(r37.zzzz).y; // native dword 5092
    r41.z=(r38.zzzz).z; // native dword 5097
    r41.w=(r39.zzzz).w; // native dword 5102
    r40.z=(asuint(dot(asfloat(r41.xyzw).xyzw,asfloat(r16.xyzw).xyzw).xxxx)).z; // native dword 5107
    r39.x=(r36.zzzz).x; // native dword 5114
    r39.y=(r37.wwww).y; // native dword 5119
    r39.z=(r38.wwww).z; // native dword 5124
    r20.w=(asuint(dot(asfloat(r39.xyzw).xyzw,asfloat(r16.xyzw).xyzw).xxxx)).w; // native dword 5129
    r21.z=(asuint((1.0f/asfloat(r20.wwww)))).z; // native dword 5136
    r36.xyz=(asuint((asfloat(r21.zzzz)*asfloat(r40.xyzx)))).xyz; // native dword 5141
    r37.xyz=(asuint(mad(asfloat(r36.xyzx),asfloat(uint4(0x3f000000u,0x3f000000u,0x3f800000u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xyz; // native dword 5148
    r38.xyz=(asuint(saturate(asfloat(r37.xyzx)))).xyz; // native dword 5163
    r37.xyz=((uint4(asfloat(r37.xyzx)==asfloat(r38.xyzx))*0xffffffffu)).xyz; // native dword 5168
    r21.z=((r37.yyyy&r37.xxxx)).z; // native dword 5175
    r21.z=((r37.zzzz&r21.zzzz)).z; // native dword 5182
    if((r21.zzzz).x != 0u) { // native dword 5189
    r37.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x0u,0x0u,0x0u,0x0u)).x/4u+3u]); // native dword 5192
    r21.z=g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x9cu,0x9cu,0x9cu,0x9cu)).x/4u+0u]; // native dword 5202
    r30.zw=uint2(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0xc4u,0xc4u,0xc4u,0xc4u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0xc4u,0xc4u,0xc4u,0xc4u)).x/4u+1u]); // native dword 5212
    r38.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x120u,0x120u,0x120u,0x120u)).x/4u+3u]); // native dword 5222
    r24.w=g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x130u,0x130u,0x130u,0x130u)).x/4u+0u]; // native dword 5232
    r39.xy=(asuint(mad(asfloat(r36.xyxx),asfloat(uint4(0x3f000000u,0xbf000000u,0x0u,0x0u)),asfloat(uint4(0x3f000000u,0x3f000000u,0x0u,0x0u))))).xy; // native dword 5242
    r39.zw=((r24.wwww&uint4(0x0u,0x0u,0x2u,0x300u))).zw; // native dword 5257
    if((r39.zzzz).x != 0u) { // native dword 5267
    r41.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x110u,0x110u,0x110u,0x110u)).x/4u+3u]); // native dword 5270
    r40.xy=(asuint(mad(asfloat(r39.xyxx),asfloat(r41.zwzz),asfloat(r41.xyxx)))).xy; // native dword 5280
    r40.xy=(asuint(g_tSpotLightVSMShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r40.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xy; // native dword 5289
    r41.xyz=(asuint((asfloat(asuint(-asfloat(r15.xyzx)))+asfloat(r37.xyzx)))).xyz; // native dword 5302
    r25.w=(asuint(dot(asfloat(r41.xyzx).xyz,asfloat(r41.xyzx).xyz).xxxx)).w; // native dword 5310
    r25.w=(asuint(sqrt(asfloat(r25.wwww)))).w; // native dword 5317
    r25.w=(asuint((asfloat(asuint(-asfloat(r37.wwww)))+asfloat(r25.wwww)))).w; // native dword 5322
    r26.w=(asuint((asfloat(r21.zzzz)*asfloat(r25.wwww)))).w; // native dword 5330
    r27.w=(asuint(mad(asfloat(asuint(-asfloat(r40.xxxx))),asfloat(r40.xxxx),asfloat(r40.yyyy)))).w; // native dword 5337
    r27.w=(asuint(max(asfloat(r27.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 5347
    r25.w=(asuint(mad(asfloat(asuint(-asfloat(r25.wwww))),asfloat(r21.zzzz),asfloat(r40.xxxx)))).w; // native dword 5354
    r25.w=(asuint(mad(asfloat(r25.wwww),asfloat(r25.wwww),asfloat(r27.wwww)))).w; // native dword 5364
    r25.w=(asuint((asfloat(r27.wwww)/asfloat(r25.wwww)))).w; // native dword 5373
    r25.w=(asuint(min(asfloat(r25.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 5380
    r26.w=((uint4(asfloat(r26.wwww)<asfloat(r40.xxxx))*0xffffffffu)).w; // native dword 5387
    r25.w=((r26.wwww!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r25.wwww)).w; // native dword 5394
    } else { // native dword 5403
    r25.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 5404
    } // native dword 5409
    r40.xy=((uint4(r39.wwww==uint4(0x100u,0x200u,0x0u,0x0u))*0xffffffffu)).xy; // native dword 5410
    if((r40.xxxx).x != 0u) { // native dword 5420
    r40.xw=(asuint(mad(asfloat(r39.xxxy),asfloat(r38.zzzw),asfloat(r38.xxxy)))).xw; // native dword 5423
    r41.xyzw=(asuint((asfloat(r13.xyzw)*asfloat(r38.zwzw)))).xyzw; // native dword 5432
    r41.xyzw=(asuint((asfloat(r30.zzzz)*asfloat(r41.xyzw)))).xyzw; // native dword 5439
    r42.xy=(asuint(mad(asfloat(r41.xyxx),asfloat(uint4(0x3f78d4feu,0x3f78d4feu,0x0u,0x0u)),asfloat(r40.xwxx)))).xy; // native dword 5446
    r42.xy=(asuint(mad(asfloat(r41.zwzz),asfloat(uint4(0x3e5cac08u,0x3e5cac08u,0x0u,0x0u)),asfloat(r42.xyxx)))).xy; // native dword 5458
    r26.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r42.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5470
    r26.w=((uint4(asfloat(r26.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5483
    r26.w=((r26.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5490
    r42.xy=(asuint(mad(asfloat(r41.xyxx),asfloat(uint4(0xbe6e147bu,0xbe6e147bu,0x0u,0x0u)),asfloat(r40.xwxx)))).xy; // native dword 5497
    r42.xy=(asuint(mad(asfloat(r41.zwzz),asfloat(uint4(0x3df837b5u,0x3df837b5u,0x0u,0x0u)),asfloat(r42.xyxx)))).xy; // native dword 5509
    r27.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r42.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5521
    r27.w=((uint4(asfloat(r27.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5534
    r27.w=((r27.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5541
    r26.w=(asuint((asfloat(r26.wwww)+asfloat(r27.wwww)))).w; // native dword 5548
    r42.xy=(asuint(mad(asfloat(r41.xyxx),asfloat(uint4(0xbf09d495u,0xbf09d495u,0x0u,0x0u)),asfloat(r40.xwxx)))).xy; // native dword 5555
    r42.xy=(asuint(mad(asfloat(r41.zwzz),asfloat(uint4(0xbf355326u,0xbf355326u,0x0u,0x0u)),asfloat(r42.xyxx)))).xy; // native dword 5567
    r27.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r42.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5579
    r27.w=((uint4(asfloat(r27.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5592
    r27.w=((r27.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5599
    r26.w=(asuint((asfloat(r26.wwww)+asfloat(r27.wwww)))).w; // native dword 5606
    r42.xy=(asuint(mad(asfloat(r41.xyxx),asfloat(uint4(0x3cf69446u,0x3cf69446u,0x0u,0x0u)),asfloat(r40.xwxx)))).xy; // native dword 5613
    r42.xy=(asuint(mad(asfloat(r41.zwzz),asfloat(uint4(0xbecf837bu,0xbecf837bu,0x0u,0x0u)),asfloat(r42.xyxx)))).xy; // native dword 5625
    r27.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r42.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5637
    r27.w=((uint4(asfloat(r27.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5650
    r27.w=((r27.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5657
    r26.w=(asuint((asfloat(r26.wwww)+asfloat(r27.wwww)))).w; // native dword 5664
    r42.xy=(asuint(mad(asfloat(r41.xyxx),asfloat(uint4(0x3de66666u,0x3de66666u,0x0u,0x0u)),asfloat(r40.xwxx)))).xy; // native dword 5671
    r42.xy=(asuint(mad(asfloat(r41.zwzz),asfloat(uint4(0x3cdc5d64u,0x3cdc5d64u,0x0u,0x0u)),asfloat(r42.xyxx)))).xy; // native dword 5683
    r27.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r42.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5695
    r27.w=((uint4(asfloat(r27.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5708
    r27.w=((r27.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5715
    r26.w=(asuint((asfloat(r26.wwww)+asfloat(r27.wwww)))).w; // native dword 5722
    r42.xy=(asuint(mad(asfloat(r41.xyxx),asfloat(uint4(0x3de425afu,0x3de425afu,0x0u,0x0u)),asfloat(r40.xwxx)))).xy; // native dword 5729
    r42.xy=(asuint(mad(asfloat(r41.zwzz),asfloat(uint4(0x3f30ff97u,0x3f30ff97u,0x0u,0x0u)),asfloat(r42.xyxx)))).xy; // native dword 5741
    r27.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r42.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5753
    r27.w=((uint4(asfloat(r27.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5766
    r27.w=((r27.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5773
    r26.w=(asuint((asfloat(r26.wwww)+asfloat(r27.wwww)))).w; // native dword 5780
    r42.xy=(asuint(mad(asfloat(r41.xyxx),asfloat(uint4(0xbf0f62b7u,0xbf0f62b7u,0x0u,0x0u)),asfloat(r40.xwxx)))).xy; // native dword 5787
    r42.xy=(asuint(mad(asfloat(r41.zwzz),asfloat(uint4(0xbe92d773u,0xbe92d773u,0x0u,0x0u)),asfloat(r42.xyxx)))).xy; // native dword 5799
    r27.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r42.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5811
    r27.w=((uint4(asfloat(r27.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5824
    r27.w=((r27.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5831
    r26.w=(asuint((asfloat(r26.wwww)+asfloat(r27.wwww)))).w; // native dword 5838
    r40.xw=(asuint(mad(asfloat(r41.xxxy),asfloat(uint4(0x3ed53261u,0x0u,0x0u,0x3ed53261u)),asfloat(r40.xxxw)))).xw; // native dword 5845
    r40.xw=(asuint(mad(asfloat(r41.zzzw),asfloat(uint4(0xbe903afbu,0x0u,0x0u,0xbe903afbu)),asfloat(r40.xxxw)))).xw; // native dword 5857
    r27.w=(asuint(g_tSpotLightPCFShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r40.xwxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).yzwx.w; // native dword 5869
    r27.w=((uint4(asfloat(r27.wwww)>=asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 5882
    r27.w=((r27.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 5889
    r26.w=(asuint((asfloat(r26.wwww)+asfloat(r27.wwww)))).w; // native dword 5896
    r26.w=(asuint((asfloat(r26.wwww)*asfloat(uint4(0x3e000000u,0x3e000000u,0x3e000000u,0x3e000000u))))).w; // native dword 5903
    } else { // native dword 5910
    r27.w=((uint4(r39.wwww==uint4(0x200u,0x200u,0x200u,0x200u))*0xffffffffu)).w; // native dword 5911
    if((r27.wwww).x != 0u) { // native dword 5918
    r36.zw=(asuint(mad(asfloat(r39.xxxy),asfloat(r38.zzzw),asfloat(r38.xxxy)))).zw; // native dword 5921
    r36.zw=(asuint(g_tSpotLightVSMShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(r36.zwzz).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).zwxy.zw; // native dword 5930
    r38.xyz=(asuint((asfloat(asuint(-asfloat(r15.xyzx)))+asfloat(r37.xyzx)))).xyz; // native dword 5943
    r27.w=(asuint(dot(asfloat(r38.xyzx).xyz,asfloat(r38.xyzx).xyz).xxxx)).w; // native dword 5951
    r27.w=(asuint(sqrt(asfloat(r27.wwww)))).w; // native dword 5958
    r27.w=(asuint((asfloat(asuint(-asfloat(r37.wwww)))+asfloat(r27.wwww)))).w; // native dword 5963
    r28.w=(asuint((asfloat(r21.zzzz)*asfloat(r27.wwww)))).w; // native dword 5971
    r29.w=(asuint(mad(asfloat(asuint(-asfloat(r36.zzzz))),asfloat(r36.zzzz),asfloat(r36.wwww)))).w; // native dword 5978
    r29.w=(asuint(max(asfloat(r29.wwww),asfloat(uint4(0x3727c5acu,0x3727c5acu,0x3727c5acu,0x3727c5acu))))).w; // native dword 5988
    r21.z=(asuint(mad(asfloat(asuint(-asfloat(r27.wwww))),asfloat(r21.zzzz),asfloat(r36.zzzz)))).z; // native dword 5995
    r21.z=(asuint(mad(asfloat(r21.zzzz),asfloat(r21.zzzz),asfloat(r29.wwww)))).z; // native dword 6005
    r21.z=(asuint((asfloat(r29.wwww)/asfloat(r21.zzzz)))).z; // native dword 6014
    r21.z=(asuint(min(asfloat(r21.zzzz),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 6021
    r27.w=((uint4(asfloat(r28.wwww)<asfloat(r36.zzzz))*0xffffffffu)).w; // native dword 6028
    r26.w=((r27.wwww!=0u?uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u):r21.zzzz)).w; // native dword 6035
    } else { // native dword 6044
    r26.w=(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)).w; // native dword 6045
    } // native dword 6050
    } // native dword 6051
    r21.z=((uint4(r39.zzzz!=uint4(0x0u,0x0u,0x0u,0x0u))*0xffffffffu)).z; // native dword 6052
    r21.z=((r21.zzzz&r40.yyyy)).z; // native dword 6059
    r27.w=(asuint((asfloat(asuint(-asfloat(r25.wwww)))+asfloat(r26.wwww)))).w; // native dword 6066
    r27.w=(asuint(mad(asfloat(r30.wwww),asfloat(r27.wwww),asfloat(r25.wwww)))).w; // native dword 6074
    r26.w=(asuint(mad(asfloat(r26.wwww),asfloat(r30.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6083
    r26.w=(asuint((asfloat(asuint(-asfloat(r30.wwww)))+asfloat(r26.wwww)))).w; // native dword 6092
    r25.w=(asuint(min(asfloat(r25.wwww),asfloat(r26.wwww)))).w; // native dword 6100
    r21.z=((r21.zzzz!=0u?r27.wwww:r25.wwww)).z; // native dword 6107
    r25.w=((uint4(asfloat(r21.zzzz)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 6116
    if((r25.wwww).x != 0u) { // native dword 6123
    r38.xyz=uint3(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x10u,0x10u,0x10u,0x10u)).x/4u+2u]); // native dword 6126
    r41.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x30u,0x30u,0x30u,0x30u)).x/4u+3u]); // native dword 6136
    r25.w=(asuint((asfloat(asuint(-asfloat(r40.zzzz)))+asfloat(r41.wwww)))).w; // native dword 6146
    r26.w=(asuint((asfloat(r41.wwww)*asfloat(uint4(0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu,0x3e4ccccdu))))).w; // native dword 6154
    r25.w=(asuint(saturate((asfloat(r25.wwww)/asfloat(r26.wwww))))).w; // native dword 6161
    r26.w=(asuint(mad(asfloat(r25.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u))))).w; // native dword 6168
    r25.w=(asuint((asfloat(r25.wwww)*asfloat(r25.wwww)))).w; // native dword 6177
    r25.w=(asuint((asfloat(r25.wwww)*asfloat(r26.wwww)))).w; // native dword 6184
    r38.xyz=(asuint((asfloat(asuint(-asfloat(r15.xyzx)))+asfloat(r38.xyzx)))).xyz; // native dword 6191
    r26.w=(asuint(dot(asfloat(r38.xyzx).xyz,asfloat(r38.xyzx).xyz).xxxx)).w; // native dword 6199
    r27.w=(asuint((asfloat(r35.yyyy)*asfloat(r35.yyyy)))).w; // native dword 6206
    r26.w=(asuint(mad(asfloat(r35.zzzz),asfloat(r26.wwww),asfloat(r27.wwww)))).w; // native dword 6213
    r26.w=(asuint(max(asfloat(r26.wwww),asfloat(uint4(0x3e800000u,0x3e800000u,0x3e800000u,0x3e800000u))))).w; // native dword 6222
    r26.w=(asuint((asfloat(r26.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 6229
    r26.w=(asuint((1.0f/asfloat(r26.wwww)))).w; // native dword 6236
    r26.w=(asuint((asfloat(asuint(-asfloat(r35.xxxx)))+asfloat(r26.wwww)))).w; // native dword 6241
    r26.w=(asuint(max(asfloat(r26.wwww),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).w; // native dword 6249
    r25.w=(asuint((asfloat(r25.wwww)*asfloat(r26.wwww)))).w; // native dword 6256
    r35.xzw=(asuint((asfloat(r25.wwww)*asfloat(r41.xxyz)))).xzw; // native dword 6263
    r25.w=((r24.wwww&uint4(0x1u,0x1u,0x1u,0x1u))).w; // native dword 6270
    if((r25.wwww).x != 0u) { // native dword 6277
    r38.xyzw=uint4(g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+0u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+1u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+2u],g_sbDeferredLightSpotData[(r14.wwww).x].d[(uint4(0x100u,0x100u,0x100u,0x100u)).x/4u+3u]); // native dword 6280
    r24.w=((r24.wwww&uint4(0x400u,0x400u,0x400u,0x400u))).w; // native dword 6290
    r30.zw=(asuint((asfloat(r36.xxxy)+asfloat(r36.xxxy)))).zw; // native dword 6297
    r25.w=(asuint(mad(asfloat(r36.xxxx),asfloat(r36.xxxx),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6304
    r25.w=(asuint(mad(asfloat(r36.yyyy),asfloat(r36.yyyy),asfloat(r25.wwww)))).w; // native dword 6313
    r30.zw=(asuint((asfloat(r30.zzzw)/asfloat(r25.wwww)))).zw; // native dword 6322
    r30.zw=(asuint(mad(asfloat(r30.zzzw),asfloat(uint4(0x0u,0x0u,0x3f000000u,0xbf000000u)),asfloat(uint4(0x0u,0x0u,0x3f000000u,0x3f000000u))))).zw; // native dword 6329
    r30.zw=((r24.wwww!=0u?r30.zzzw:r39.xxxy)).zw; // native dword 6344
    r30.zw=(asuint(mad(asfloat(r30.zzzw),asfloat(r38.zzzw),asfloat(r38.xxxy)))).zw; // native dword 6353
    r36.xyz=(asuint(g_tSpotLightProjectionAtlas.SampleLevel(g_sLinearMipLinearClamp,asfloat(r30.zwzz).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x))).xyzw.xyz; // native dword 6362
    r35.xzw=(asuint((asfloat(r35.xxzw)*asfloat(r36.xxyz)))).xzw; // native dword 6375
    } // native dword 6382
    r20.w=((uint4(asfloat(r20.wwww)>=asfloat(r40.zzzz))*0xffffffffu)).w; // native dword 6383
    r20.w=((r20.wwww&uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))).w; // native dword 6390
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(r21.zzzz)))).w; // native dword 6397
    r35.xzw=(asuint((asfloat(r20.wwww)*asfloat(r35.xxzw)))).xzw; // native dword 6404
    r36.xyz=(asuint((asfloat(asuint(-asfloat(r15.xyzx)))+asfloat(r37.xyzx)))).xyz; // native dword 6411
    r20.w=(asuint(dot(asfloat(r36.xyzx).xyz,asfloat(r36.xyzx).xyz).xxxx)).w; // native dword 6419
    r21.z=(asuint(rsqrt(asfloat(r20.wwww)))).z; // native dword 6426
    r37.xyz=(asuint((asfloat(r21.zzzz)*asfloat(r36.xyzx)))).xyz; // native dword 6431
    r24.w=(asuint(sqrt(asfloat(r20.wwww)))).w; // native dword 6438
    r25.w=(asuint(dot(asfloat(r37.xyzx).xyz,asfloat(r20.xyzx).xyz).xxxx)).w; // native dword 6443
    r26.w=(asuint((asfloat(r24.wwww)*asfloat(r35.yyyy)))).w; // native dword 6450
    r26.w=((uint4(asfloat(r26.wwww)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 6457
    r27.w=(asuint((asfloat(r27.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 6464
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(uint4(0x40c90fdbu,0x40c90fdbu,0x40c90fdbu,0x40c90fdbu))))).w; // native dword 6471
    r24.w=(asuint((asfloat(r24.wwww)*asfloat(r25.wwww)))).w; // native dword 6478
    r24.w=(asuint((asfloat(r24.wwww)/asfloat(r35.yyyy)))).w; // native dword 6485
    r24.w=(asuint((asfloat(r24.wwww)+asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 6492
    r24.w=(asuint(saturate((asfloat(r24.wwww)*asfloat(uint4(0xbf000000u,0xbf000000u,0xbf000000u,0xbf000000u)))))).w; // native dword 6499
    r24.w=(asuint(mad(asfloat(r24.wwww),asfloat(uint4(0x40000000u,0x40000000u,0x40000000u,0x40000000u)),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 6506
    r28.w=(asuint(mad(asfloat(asuint(-asfloat(r24.wwww))),asfloat(r24.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6515
    r28.w=(asuint(sqrt(asfloat(r28.wwww)))).w; // native dword 6525
    r29.w=(asuint((asfloat(asuint(-abs(asfloat(r24.wwww))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6530
    r29.w=(asuint(sqrt(asfloat(r29.wwww)))).w; // native dword 6538
    r30.z=(asuint(mad(asfloat(asuint(abs(asfloat(r24.wwww)))),asfloat(uint4(0xbc996e30u,0xbc996e30u,0xbc996e30u,0xbc996e30u)),asfloat(uint4(0x3d981627u,0x3d981627u,0x3d981627u,0x3d981627u))))).z; // native dword 6543
    r30.z=(asuint(mad(asfloat(r30.zzzz),asfloat(asuint(abs(asfloat(r24.wwww)))),asfloat(uint4(0xbe593484u,0xbe593484u,0xbe593484u,0xbe593484u))))).z; // native dword 6553
    r30.z=(asuint(mad(asfloat(r30.zzzz),asfloat(asuint(abs(asfloat(r24.wwww)))),asfloat(uint4(0x3fc90da4u,0x3fc90da4u,0x3fc90da4u,0x3fc90da4u))))).z; // native dword 6563
    r30.w=(asuint((asfloat(r29.wwww)*asfloat(r30.zzzz)))).w; // native dword 6573
    r30.w=(asuint(mad(asfloat(r30.wwww),asfloat(uint4(0xc0000000u,0xc0000000u,0xc0000000u,0xc0000000u)),asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 6580
    r31.w=((uint4(asfloat(r24.wwww)<asfloat(asuint(-asfloat(r24.wwww))))*0xffffffffu)).w; // native dword 6589
    r30.w=((r30.wwww&r31.wwww)).w; // native dword 6597
    r29.w=(asuint(mad(asfloat(r30.zzzz),asfloat(r29.wwww),asfloat(r30.wwww)))).w; // native dword 6604
    r29.w=(asuint((asfloat(asuint(-asfloat(r29.wwww)))+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).w; // native dword 6613
    r24.w=(asuint(mad(asfloat(asuint(-asfloat(r28.wwww))),asfloat(r24.wwww),asfloat(asuint(-asfloat(r29.wwww)))))).w; // native dword 6621
    r24.w=(asuint((asfloat(r24.wwww)+asfloat(uint4(0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu,0x3fc90fdbu))))).w; // native dword 6632
    r24.w=(asuint(saturate((asfloat(r24.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u)))))).w; // native dword 6639
    r24.w=(asuint((asfloat(r24.wwww)*asfloat(r27.wwww)))).w; // native dword 6646
    r20.w=(asuint((asfloat(r24.wwww)/asfloat(r20.wwww)))).w; // native dword 6653
    r20.w=(asuint(min(asfloat(r20.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6660
    r24.w=((uint4(asfloat(uint4(0x39aec33fu,0x39aec33fu,0x39aec33fu,0x39aec33fu))<asfloat(r20.wwww))*0xffffffffu)).w; // native dword 6667
    r20.w=(asuint(mad(asfloat(r20.wwww),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6674
    r20.w=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r20.wwww)))).w; // native dword 6683
    r27.w=(asuint((asfloat(r20.wwww)*asfloat(r20.wwww)))).w; // native dword 6693
    r27.w=(asuint((asfloat(r20.wwww)*asfloat(r27.wwww)))).w; // native dword 6700
    r27.w=(asuint(mad(asfloat(r20.wwww),asfloat(uint4(0x40400000u,0x40400000u,0x40400000u,0x40400000u)),asfloat(asuint(-asfloat(r27.wwww)))))).w; // native dword 6707
    r20.w=(asuint(mad(asfloat(asuint(-asfloat(r20.wwww))),asfloat(r20.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6717
    r20.w=(asuint((asfloat(r27.wwww)/asfloat(r20.wwww)))).w; // native dword 6727
    r20.w=(asuint((asfloat(r20.wwww)+asfloat(r20.wwww)))).w; // native dword 6734
    r20.w=(asuint((asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))/asfloat(r20.wwww)))).w; // native dword 6741
    r20.w=(asuint(mad(asfloat(r11.wwww),asfloat(r11.wwww),asfloat(r20.wwww)))).w; // native dword 6751
    r20.w=(asuint(sqrt(asfloat(r20.wwww)))).w; // native dword 6760
    r20.w=((r24.wwww!=0u?r20.wwww:r11.wwww)).w; // native dword 6765
    r20.w=(asuint((asfloat(asuint(-asfloat(r20.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6774
    r20.w=((r26.wwww!=0u?r20.wwww:r22.yyyy)).w; // native dword 6782
    r36.xyz=(asuint(mad(asfloat(r36.xyzx),asfloat(r21.zzzz),asfloat(r19.xyzx)))).xyz; // native dword 6791
    r21.z=(asuint(dot(asfloat(r36.xyzx).xyz,asfloat(r36.xyzx).xyz).xxxx)).z; // native dword 6800
    r21.z=(asuint(rsqrt(asfloat(r21.zzzz)))).z; // native dword 6807
    r36.xyz=(asuint((asfloat(r21.zzzz)*asfloat(r36.xyzx)))).xyz; // native dword 6812
    r21.z=(asuint(dot(asfloat(r19.xyzx).xyz,asfloat(r36.xyzx).xyz).xxxx)).z; // native dword 6819
    r24.w=(asuint(dot(asfloat(r20.xyzx).xyz,asfloat(r36.xyzx).xyz).xxxx)).w; // native dword 6826
    r20.w=(asuint((asfloat(asuint(-asfloat(r20.wwww)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6833
    r20.w=(asuint(max(asfloat(r20.wwww),asfloat(uint4(0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu,0x3d4ccccdu))))).w; // native dword 6841
    r20.w=(asuint(min(asfloat(r20.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6848
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(r20.wwww)))).w; // native dword 6855
    r21.z=(asuint((asfloat(asuint(-abs(asfloat(r21.zzzz))))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).z; // native dword 6862
    r21.z=(asuint(max(asfloat(r21.zzzz),asfloat(uint4(0x0u,0x0u,0x0u,0x0u))))).z; // native dword 6870
    r26.w=(asuint((asfloat(r21.zzzz)*asfloat(r21.zzzz)))).w; // native dword 6877
    r26.w=(asuint((asfloat(r26.wwww)*asfloat(r26.wwww)))).w; // native dword 6884
    r21.z=(asuint((asfloat(r21.zzzz)*asfloat(r26.wwww)))).z; // native dword 6891
    r21.z=(asuint(mad(asfloat(r18.wwww),asfloat(r21.zzzz),asfloat(r7.wwww)))).z; // native dword 6898
    r26.w=(asuint((asfloat(r20.wwww)*asfloat(r20.wwww)))).w; // native dword 6907
    r24.w=(asuint((asfloat(r24.wwww)*asfloat(r24.wwww)))).w; // native dword 6914
    r27.w=(asuint(mad(asfloat(r20.wwww),asfloat(r20.wwww),asfloat(uint4(0xbf800000u,0xbf800000u,0xbf800000u,0xbf800000u))))).w; // native dword 6921
    r24.w=(asuint(mad(asfloat(r24.wwww),asfloat(r27.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6930
    r24.w=(asuint((asfloat(r24.wwww)*asfloat(r24.wwww)))).w; // native dword 6939
    r24.w=(asuint((asfloat(r24.wwww)*asfloat(uint4(0x40490fdbu,0x40490fdbu,0x40490fdbu,0x40490fdbu))))).w; // native dword 6946
    r24.w=(asuint(max(asfloat(r24.wwww),asfloat(uint4(0x3a83126fu,0x3a83126fu,0x3a83126fu,0x3a83126fu))))).w; // native dword 6953
    r24.w=(asuint((asfloat(r26.wwww)/asfloat(r24.wwww)))).w; // native dword 6960
    r25.w=(asuint(saturate(asfloat(r25.wwww)))).w; // native dword 6967
    r20.w=(asuint(mad(asfloat(asuint(-asfloat(r20.wwww))),asfloat(r20.wwww),asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u))))).w; // native dword 6972
    r27.w=(asuint(mad(asfloat(r25.wwww),asfloat(r20.wwww),asfloat(r26.wwww)))).w; // native dword 6982
    r20.w=(asuint(mad(asfloat(r12.wwww),asfloat(r20.wwww),asfloat(r26.wwww)))).w; // native dword 6991
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(r27.wwww)))).w; // native dword 7000
    r20.w=(asuint(max(asfloat(r20.wwww),asfloat(uint4(0x38d1b717u,0x38d1b717u,0x38d1b717u,0x38d1b717u))))).w; // native dword 7007
    r20.w=(asuint((asfloat(uint4(0x3e800000u,0x3e800000u,0x3e800000u,0x3e800000u))/asfloat(r20.wwww)))).w; // native dword 7014
    r26.w=(asuint(saturate((asfloat(asuint(-asfloat(r21.zzzz)))+asfloat(uint4(0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u)))))).w; // native dword 7021
    r26.w=(asuint((asfloat(r25.wwww)*asfloat(r26.wwww)))).w; // native dword 7029
    r26.w=(asuint((asfloat(r26.wwww)*asfloat(uint4(0x3ea2f983u,0x3ea2f983u,0x3ea2f983u,0x3ea2f983u))))).w; // native dword 7036
    r21.z=(asuint((asfloat(r21.zzzz)*asfloat(r24.wwww)))).z; // native dword 7043
    r20.w=(asuint((asfloat(r20.wwww)*asfloat(r21.zzzz)))).w; // native dword 7050
    r20.w=(asuint((asfloat(r25.wwww)*asfloat(r20.wwww)))).w; // native dword 7057
    r17.xyz=(asuint(mad(asfloat(r26.wwww),asfloat(r35.xzwx),asfloat(r17.xyzx)))).xyz; // native dword 7064
    r34.xyz=(asuint(mad(asfloat(r20.wwww),asfloat(r35.xzwx),asfloat(r34.xyzx)))).xyz; // native dword 7073
    } // native dword 7082
    } // native dword 7083
    r14.w=(r19.wwww).w; // native dword 7084
    } // native dword 7089
    r14.xyw=(asuint(mad(asfloat(r22.xzxw),asfloat(r27.xyxz),asfloat(r17.xyxz)))).xyw; // native dword 7090
    r15.xyz=(asuint(mad(asfloat(r21.xywx),asfloat(r29.xyzx),asfloat(r34.xyzx)))).xyz; // native dword 7099
    r16.xyz=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r6.wwww).x-0u)].SampleLevel(g_sLinearWrap,asfloat(r30.xyxx).xy,PTNativeMaterialMip((r6.wwww).x,quantized)))).xyzw.xyz; // native dword 7108
    r16.xyz=(asuint((asfloat(r23.yzwy)*asfloat(r16.xyzx)))).xyz; // native dword 7123
    r19.xyz=(asuint((asfloat(r23.xxxx)*asfloat(r24.xyzx)))).xyz; // native dword 7130
    r6.w=(asuint(dot(asfloat(r19.xyzx).xyz,asfloat(r19.xyzx).xyz).xxxx)).w; // native dword 7137
    r6.w=((uint4(asfloat(r6.wwww)!=asfloat(uint4(0x0u,0x0u,0x0u,0x0u)))*0xffffffffu)).w; // native dword 7144
    if((r6.wwww).x != 0u) { // native dword 7151
    r6.w=((r5.wwww*uint4(0x23u,0x23u,0x23u,0x23u)+uint4(0x5u,0x5u,0x5u,0x5u))).w; // native dword 7154
    r20.xyz=(asuint(g_sMaterialTextureArray[NonUniformResourceIndex((r6.wwww).x-0u)].SampleLevel(g_sLinearWrap,asfloat(r30.xyxx).xy,PTNativeMaterialMip((r6.wwww).x,quantized)))).xyzw.xyz; // native dword 7163
    r19.xyz=(asuint((asfloat(r19.xyzx)*asfloat(r20.xyzx)))).xyz; // native dword 7178
    } // native dword 7185
    directEmission=asfloat(r17.xyz)*asfloat(r16.xyz)+asfloat(r34.xyz)+asfloat(r19.xyz); // native dword 7186
    r14.xyw=(asuint(mad(asfloat(r14.xyxw),asfloat(r16.xyxz),asfloat(r15.xyxz)))).xyw; // native dword 7186
    r29.xyz=(asuint((asfloat(r19.xyzx)+asfloat(r14.xywx)))).xyz; // native dword 7195
    } // native dword 7202
    float3 result=asfloat(r29.xyz);
    return all(isfinite(result))?max(result,0.0f):0.0f;
}
