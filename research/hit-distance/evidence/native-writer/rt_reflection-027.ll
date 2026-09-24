;
; Note: shader requires additional functionality:
;       UAVs at every shader stage
;
;
; Buffer Definitions:
;
; cbuffer sys_constants
; {
;
;   struct sys_constants
;   {
;
;       float2 g_vScreenRes;                          ; Offset:    0
;       float2 g_vInvScreenRes;                       ; Offset:    8
;       float2 g_vOutputRes;                          ; Offset:   16
;       float2 g_vInvOutputRes;                       ; Offset:   24
;       column_major float4x4 g_mWorldToView;         ; Offset:   32
;       column_major float4x4 g_mViewToWorld;         ; Offset:   96
;       column_major float4x4 g_mViewToClip;          ; Offset:  160
;       column_major float4x4 g_mClipToView;          ; Offset:  224
;       column_major float4x4 g_mWorldToClip;         ; Offset:  288
;       column_major float4x4 g_mClipToWorld;         ; Offset:  352
;       column_major float4x4 g_mClipToPreviousClip;  ; Offset:  416
;       column_major float4x4 g_mViewToPreviousClip;  ; Offset:  480
;       column_major float4x4 g_mPreviousViewToView;  ; Offset:  544
;       column_major float4x4 g_mPreviousWorldToClip; ; Offset:  608
;       column_major float4x4 g_mPreviousViewToClip;  ; Offset:  672
;       column_major float4x4 g_mClipToPreviousClipNoJitter;; Offset:  736
;       column_major float4x4 g_mPreviousClipToClipNoJitter;; Offset:  800
;       float4 g_vViewPoint;                          ; Offset:  864
;       float g_fInvNear;                             ; Offset:  880
;       column_major float4x4 g_mViewToCameraView;    ; Offset:  896
;       column_major float4x4 g_mCameraViewToCameraClip;; Offset:  960
;       column_major float4x4 g_mCameraClipToView;    ; Offset: 1024
;       float g_fWorldTime;                           ; Offset: 1088
;       float g_fWorldTimeDelta;                      ; Offset: 1092
;       float g_fRealTime;                            ; Offset: 1096
;       float g_fRealTimeDelta;                       ; Offset: 1100
;       float g_fLastValidWorldTimeDelta;             ; Offset: 1104
;       uint g_uTemporalFrame;                        ; Offset: 1108
;       uint g_uCurrentFrame;                         ; Offset: 1112
;       bool g_bHDR;                                  ; Offset: 1116
;       float g_fHDRSDRSceneBrightnessMultiplier;     ; Offset: 1120
;       float g_fHDRSDRUIBrightnessMultiplier;        ; Offset: 1124
;   
;   } sys_constants;                                  ; Offset:    0 Size:  1128
;
; }
;
; cbuffer rtreflection
; {
;
;   struct rtreflection
;   {
;
;       float g_fClampReflectionIntensity;            ; Offset:    0
;       float g_fRTReflectionAddRays;                 ; Offset:    4
;       uint g_uRTReflectionRayCount;                 ; Offset:    8
;       float g_fRTZeroRayReflectance;                ; Offset:   12
;       float g_fRTMaxRayReflectance;                 ; Offset:   16
;       float g_fRTMaxRayRoughness;                   ; Offset:   20
;   
;   } rtreflection;                                   ; Offset:    0 Size:    24
;
; }
;
; Resource bind info for g_sbMaterialDataPart1
; {
;
;   struct struct.MaterialDataPart1
;   {
;
;       uint vSpecularColor_uBRDF;                    ; Offset:    0
;       uint uScatterIndex_vTranslucencyDepthRange;   ; Offset:    4
;   
;   } $Element;                                       ; Offset:    0 Size:     8
;
; }
;
; Resource bind info for g_sbDeferredLightSunData
; {
;
;   struct struct.DeferredLightSun
;   {
;
;       float3 vDirectionInView;                      ; Offset:    0
;       int pad1;                                     ; Offset:   12
;       float3 vColor;                                ; Offset:   16
;       int pad2;                                     ; Offset:   28
;       column_major float4x4 mViewToProjectionClip;  ; Offset:   32
;       float fProjectionMapTransparency;             ; Offset:   96
;       float fRadius;                                ; Offset:  100
;       uint uTechniqueProperties;                    ; Offset:  104
;       float fScatterIntensityMul;                   ; Offset:  108
;       uint uCascadeCount;                           ; Offset:  112
;       column_major float4x4 mCascadeViewToShadowClip;; Offset:  116
;       float2 vCascadeRange[6];                      ; Offset:  500
;       float fCascadePCFWidth[6];                    ; Offset:  548
;       int pad3[1];                                  ; Offset:  572
;   
;   } $Element;                                       ; Offset:    0 Size:   576
;
; }
;
;
; Resource Bindings:
;
; Name                                 Type  Format         Dim      ID      HLSL Bind  Count
; ------------------------------ ---------- ------- ----------- ------- -------------- ------
; sys_constants                     cbuffer      NA          NA     CB0            cb0     1
; rtreflection                      cbuffer      NA          NA     CB1            cb1     1
; g_sLinearClamp                    sampler      NA          NA      S0      s6,space1     1
; g_tGBuffer1                       texture     f32          2d      T0             t0     1
; g_tGBuffer2                       texture     f32          2d      T1             t1     1
; g_tLinearDepth                    texture     f32          2d      T2             t2     1
; g_tClipDepth                      texture     f32          2d      T3             t3     1
; g_sbMaterialDataPart1             texture  struct         r/o      T4             t4     1
; g_sbDeferredLightSunData          texture  struct         r/o      T5             t5     1
; g_tEnvBRDF                        texture     f32          2d      T6             t6     1
; g_tStaticBlueNoiseRGBA_0          texture     f32          2d      T7             t7     1
; g_rtScene                         texture     i32         ras      T8             t8     1
; g_rwtMaterialId                       UAV     u32     2darray      U0             u0     1
; g_rwtShadow                           UAV     u32     2darray      U1             u1     1
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%"class.Texture2D<vector<float, 4> >" = type { <4 x float>, %"class.Texture2D<vector<float, 4> >::mips_type" }
%"class.Texture2D<vector<float, 4> >::mips_type" = type { i32 }
%struct.SamplerState = type { i32 }
%"class.StructuredBuffer<MaterialDataPart1>" = type { %struct.MaterialDataPart1 }
%struct.MaterialDataPart1 = type { i32, i32 }
%"class.StructuredBuffer<DeferredLightSun>" = type { %struct.DeferredLightSun }
%struct.DeferredLightSun = type { <3 x float>, i32, <3 x float>, i32, %class.matrix.float.4.4, float, float, i32, float, i32, [6 x %class.matrix.float.4.4], [6 x <2 x float>], [6 x float], [1 x i32] }
%class.matrix.float.4.4 = type { [4 x <4 x float>] }
%struct.RaytracingAccelerationStructure = type { i32 }
%"class.RWTexture2DArray<unsigned int>" = type { i32 }
%sys_constants = type { <2 x float>, <2 x float>, <2 x float>, <2 x float>, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, <4 x float>, float, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, float, float, float, float, float, i32, i32, i32, float, float }
%rtreflection = type { float, float, i32, float, float, float }
%struct.HitData = type { i32 }
%struct.IntersectionAttributes = type { <2 x float> }
%dx.types.Handle = type { i8* }
%struct.RayDesc = type { <3 x float>, float, <3 x float>, float }
%dx.types.CBufRet.f32 = type { float, float, float, float }
%dx.types.ResRet.f32 = type { float, float, float, float, i32 }
%dx.types.ResRet.i32 = type { i32, i32, i32, i32, i32 }
%dx.types.CBufRet.i32 = type { i32, i32, i32, i32 }
%struct.GBufferFormat = type { <4 x float>, <4 x float> }
%"class.Texture2D<float>" = type { float, %"class.Texture2D<float>::mips_type" }
%"class.Texture2D<float>::mips_type" = type { i32 }
%"class.Texture2D<unsigned int>" = type { i32, %"class.Texture2D<unsigned int>::mips_type" }
%"class.Texture2D<unsigned int>::mips_type" = type { i32 }
%"class.Texture2DMS<float, 0>" = type { float, %"class.Texture2DMS<float, 0>::sample_type" }
%"class.Texture2DMS<float, 0>::sample_type" = type { i32 }
%"class.StructuredBuffer<vector<unsigned int, 4> >" = type { <4 x i32> }
%"class.StructuredBuffer<MaterialDataPart3>" = type { %struct.MaterialDataPart3 }
%struct.MaterialDataPart3 = type { float, float, <3 x float>, float, <3 x float>, float, float, float, float, <4 x float>, <2 x float>, <2 x float>, <2 x float>, float, float, <3 x float>, <4 x float>, <2 x float>, <2 x float>, <2 x float>, float, float, float, float, float, float, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, i32, float, float, float, <3 x float>, float, float, float, i32, i32 }
%"class.StructuredBuffer<MaterialBindingData>" = type { %struct.MaterialBindingData }
%struct.MaterialBindingData = type { i32 }
%struct.order2_sh = type { <3 x float>, <3 x float>, <3 x float>, <3 x float> }
%"class.RWTexture3D<vector<float, 4> >" = type { <4 x float> }
%"class.Texture3D<vector<float, 4> >" = type { <4 x float>, %"class.Texture3D<vector<float, 4> >::mips_type" }
%"class.Texture3D<vector<float, 4> >::mips_type" = type { i32 }
%struct.LightVolumeData = type { <3 x float>, <4 x float> }
%"class.TextureCube<vector<float, 4> >" = type { <4 x float> }
%"class.StructuredBuffer<DeferredLightPoint>" = type { %struct.DeferredLightPoint }
%struct.DeferredLightPoint = type { <3 x float>, float, <3 x float>, float, <4 x float>, <3 x float>, float, %class.matrix.float.4.4, <4 x float>, float, float, float, i32 }
%"class.StructuredBuffer<DeferredLightPointClipping>" = type { %struct.DeferredLightPointClipping }
%struct.DeferredLightPointClipping = type { <3 x float>, float }
%"class.StructuredBuffer<DeferredLightPointBVH>" = type { %struct.DeferredLightPointBVH }
%struct.DeferredLightPointBVH = type { <3 x float>, <3 x float>, i32, i32, i32, i32 }
%"class.RWTexture2D<unsigned int>" = type { i32 }
%"class.StructuredBuffer<DeferredLightSpot>" = type { %struct.DeferredLightSpot }
%struct.DeferredLightSpot = type { <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <2 x float>, float, float, %class.matrix.float.4.4, <3 x float>, float, %class.matrix.float.3.3, float, float, float, [3 x <4 x float>], <4 x float>, <4 x float>, <4 x float>, i32, <3 x i32> }
%class.matrix.float.3.3 = type { [3 x <3 x float>] }
%"class.StructuredBuffer<DeferredLightSpotClipping>" = type { %struct.DeferredLightSpotClipping }
%struct.DeferredLightSpotClipping = type { <3 x float>, float, <3 x float>, float, float, i32, <2 x i32> }
%"class.StructuredBuffer<DeferredLightSpotBVH>" = type { %struct.DeferredLightSpotBVH }
%struct.DeferredLightSpotBVH = type { <3 x float>, <3 x float>, i32, i32, i32, i32 }
%"class.StructuredBuffer<VolumeSamplingInfo>" = type { %struct.VolumeSamplingInfo }
%struct.VolumeSamplingInfo = type { %class.matrix.float.3.3, <3 x float>, i32, i32, i32, float, float, i32, <2 x float> }
%"class.StructuredBuffer<unsigned int>" = type { i32 }
%"class.StructuredBuffer<InternalNode>" = type { %struct.InternalNode }
%struct.InternalNode = type { i32, i32, i32, i32 }
%"class.Texture3D<vector<float, 3> >" = type { <3 x float>, %"class.Texture3D<vector<float, 3> >::mips_type" }
%"class.Texture3D<vector<float, 3> >::mips_type" = type { i32 }
%struct.VolumeCullingInCB = type { <3 x float>, float, <3 x float>, float }
%struct.VolumeTopLevelInfo = type { [3 x <4 x float>], <3 x i32>, float }
%struct.DeferredLightSpecularBRDF = type { <3 x float>, <3 x float> }
%struct.DeferredLightGenericSurface = type { <3 x float>, <3 x float>, float, float }
%struct.DeferredLightTransparentBRDF = type { <3 x float>, <3 x float>, <3 x float> }
%struct.DeferredLightIntensity = type { <3 x float>, <3 x float> }
%struct.DeferredLightVolumeResult = type { %struct.LightVolumeData }
%struct.DeferredLightPointSurface = type { <3 x float> }
%struct.DeferredLightGBufferData = type { <4 x float>, <4 x float>, <3 x float>, <3 x float>, float, float }
%struct.CellInfo = type { <3 x float>, float, <3 x float>, float, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float, [16 x i32] }
%"class.StructuredBuffer<EnvironmentMapDynamicInfo>" = type { %struct.EnvironmentMapDynamicInfo }
%struct.EnvironmentMapDynamicInfo = type { float }
%"class.StructuredBuffer<EnvironmentMapReference>" = type { %struct.EnvironmentMapReference }
%struct.EnvironmentMapReference = type { i32 }
%"class.StructuredBuffer<CellInfo>" = type { %struct.CellInfo }
%"class.StructuredBuffer<Node>" = type { %struct.Node }
%struct.Node = type { %struct.ChildMask, %struct.NodePointer, i32 }
%struct.ChildMask = type { [2 x i32] }
%struct.NodePointer = type { i32 }
%"class.StructuredBuffer<IrradianceProbe>" = type { %struct.IrradianceProbe }
%struct.IrradianceProbe = type { i32, i32, i32, i32, i32, i32, i32, i32, i32 }
%"class.StructuredBuffer<TransportProbe>" = type { %struct.TransportProbe }
%struct.TransportProbe = type { [8 x i32] }
%"class.StructuredBuffer<vector<float, 4> >" = type { <4 x float> }
%"class.StructuredBuffer<ProbeRef>" = type { %struct.ProbeRef }
%struct.ProbeRef = type { i32, i32 }
%"class.StructuredBuffer<EnvironmentMapInfo>" = type { %struct.EnvironmentMapInfo }
%struct.EnvironmentMapInfo = type { <3 x float>, float, <3 x float>, [6 x <3 x float>], [6 x <3 x float>] }
%struct.VoxelInfo = type { <3 x float>, float, i32, i32, <3 x float> }
%struct.LeafInfo = type { i32, <3 x float>, <3 x float>, float, i32, i32, i32 }
%"class.StructuredBuffer<PerInstanceLBData>" = type { %struct.PerInstanceLBData }
%struct.PerInstanceLBData = type { <2 x float>, i32, i32 }
%"class.StructuredBuffer<RenderInstanceData>" = type { %struct.RenderInstanceData }
%struct.RenderInstanceData = type { [8 x float], <2 x i32>, i32, i32, float, float, <2 x i32> }
%struct.ByteAddressBuffer = type { i32 }
%struct.RaytracingHitRootConstants = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, i32 }
%"class.RWTexture2DArray<vector<float, 4> >" = type { <4 x float> }
%"class.RWStructuredBuffer<unsigned int>" = type { i32 }
%debug_general = type { float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }
%shadow_general = type { <2 x float>, <2 x float>, <2 x float>, <2 x i32> }
%mid_translucency = type { float }
%mid_general = type { i32 }
%deferredlight_constants = type { [5 x <4 x float>], [5 x <4 x float>], <2 x float>, i32, <2 x i32>, <3 x i32>, <2 x float>, <2 x float>, i32, i32, i32, float, float, float, float, float, i32, i32, i32, i32, float, float, float, float, <4 x i32>, <4 x float>, <4 x float>, <3 x float>, <2 x float>, <3 x float>, float, <3 x float>, float, float, i32 }
%env_general = type { float, float }
%atmosphere_general = type { <4 x float>, <4 x float>, <2 x float>, <2 x float>, float, <2 x float>, <4 x float>, <3 x float>, <3 x float>, float, float, float, float, float, <2 x float>, <3 x float>, float, float, float, <3 x float> }
%illuminationvolume = type { <3 x float>, i32, <3 x float>, i32, float, float }
%ManualUpdateCB_ActiveVolumeTopLevelInfoArray = type { [384 x %struct.VolumeTopLevelInfo] }
%ManualUpdateCB_DebugVolumeTopLevelInfoArray = type { [384 x %struct.VolumeTopLevelInfo] }
%WorldGridInfo = type { float, float, i32 }
%EnvironmentMapAtlas = type { float, float, float, float, <2 x float> }
%vertex_binding = type { <4 x i32>, <4 x i32>, <4 x i32> }
%g_cbRaytracingHit = type { %struct.RaytracingHitRootConstants }

@"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tGBuffer2@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_sLinearClamp@@3USamplerState@@A" = external constant %struct.SamplerState, align 4
@"\01?g_sbMaterialDataPart1@@3V?$StructuredBuffer@UMaterialDataPart1@@@@A" = external constant %"class.StructuredBuffer<MaterialDataPart1>", align 4
@"\01?g_sbDeferredLightSunData@@3V?$StructuredBuffer@UDeferredLightSun@@@@A" = external constant %"class.StructuredBuffer<DeferredLightSun>", align 4
@"\01?g_tEnvBRDF@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_rtScene@@3URaytracingAccelerationStructure@@A" = external constant %struct.RaytracingAccelerationStructure, align 4
@"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A" = external constant %"class.RWTexture2DArray<unsigned int>", align 4
@"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A" = external constant %"class.RWTexture2DArray<unsigned int>", align 4
@sys_constants = external constant %sys_constants
@rtreflection = external constant %rtreflection

; Function Attrs: nounwind readnone
define void @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture %ia) #0 {
  ret void
}

; Function Attrs: nounwind readnone
define void @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture %ia) #0 {
  ret void
}

; Function Attrs: nounwind readnone
define void @"\01?reflectionMiss@@YAXUHitData@@@Z"(%struct.HitData* noalias nocapture %payload) #0 {
  ret void
}

; Function Attrs: nounwind readnone
define void @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture %ia) #0 {
  ret void
}

; Function Attrs: nounwind readnone
define void @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture %ia) #0 {
  ret void
}

; Function Attrs: nounwind
define void @"\01?shadowMiss@@YAXUHitData@@@Z"(%struct.HitData* noalias nocapture %payload) #1 {
  %1 = load %"class.RWTexture2DArray<unsigned int>", %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", align 4
  %2 = getelementptr inbounds %struct.HitData, %struct.HitData* %payload, i32 0, i32 0
  %3 = load i32, i32* %2, align 4
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex1 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %4 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32 160, %"class.RWTexture2DArray<unsigned int>" %1)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %4, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex1, i32 %3, i32 1, i32 1, i32 1, i32 1, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  ret void
}

; Function Attrs: nounwind
define void @"\01?reflectionRayGeneration@@YAXXZ"() #1 {
  %1 = alloca [3 x float], align 4
  %2 = alloca [3 x i32], align 4
  %3 = alloca [3 x float], align 4
  %4 = load %struct.SamplerState, %struct.SamplerState* @"\01?g_sLinearClamp@@3USamplerState@@A", align 4
  %5 = load %struct.RaytracingAccelerationStructure, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", align 4
  %6 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %7 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tEnvBRDF@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %8 = load %"class.StructuredBuffer<DeferredLightSun>", %"class.StructuredBuffer<DeferredLightSun>"* @"\01?g_sbDeferredLightSunData@@3V?$StructuredBuffer@UDeferredLightSun@@@@A", align 4
  %9 = load %"class.StructuredBuffer<MaterialDataPart1>", %"class.StructuredBuffer<MaterialDataPart1>"* @"\01?g_sbMaterialDataPart1@@3V?$StructuredBuffer@UMaterialDataPart1@@@@A", align 4
  %10 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %11 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %12 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer2@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %13 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %14 = load %"class.RWTexture2DArray<unsigned int>", %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", align 4
  %15 = load %rtreflection, %rtreflection* @rtreflection, align 4
  %16 = load %sys_constants, %sys_constants* @sys_constants, align 4
  %rtreflection174 = call %dx.types.Handle @dx.op.createHandleForLib.rtreflection(i32 160, %rtreflection %15)  ; CreateHandleForLib(Resource)
  %sys_constants = call %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32 160, %sys_constants %16)  ; CreateHandleForLib(Resource)
  %17 = alloca %struct.HitData, align 8
  %18 = alloca %struct.HitData, align 8
  %ray = alloca %struct.RayDesc, align 4
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex169 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %19 = uitofp i32 %DispatchRaysIndex to float
  %20 = uitofp i32 %DispatchRaysIndex169 to float
  %.i0 = fadd fast float %19, 5.000000e-01
  %.i1 = fadd fast float %20, 5.000000e-01
  %21 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %22 = extractvalue %dx.types.CBufRet.f32 %21, 2
  %23 = extractvalue %dx.types.CBufRet.f32 %21, 3
  %.i0175 = fmul fast float %22, %.i0
  %.i1176 = fmul fast float %.i1, %23
  %24 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %10)  ; CreateHandleForLib(Resource)
  %TextureLoad = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %24, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex169, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %25 = extractvalue %dx.types.ResRet.f32 %TextureLoad, 0
  %26 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %11)  ; CreateHandleForLib(Resource)
  %TextureLoad161 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %26, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex169, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %27 = extractvalue %dx.types.ResRet.f32 %TextureLoad161, 0
  %.i0177 = fmul fast float %.i0175, 2.000000e+00
  %.i1178 = fmul fast float %.i1176, 2.000000e+00
  %.i0179 = fadd fast float %.i0177, -1.000000e+00
  %.i1180508 = fsub fast float 1.000000e+00, %.i1178
  %28 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 14)  ; CBufferLoadLegacy(handle,regIndex)
  %29 = extractvalue %dx.types.CBufRet.f32 %28, 0
  %30 = extractvalue %dx.types.CBufRet.f32 %28, 1
  %31 = extractvalue %dx.types.CBufRet.f32 %28, 2
  %32 = extractvalue %dx.types.CBufRet.f32 %28, 3
  %33 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %34 = extractvalue %dx.types.CBufRet.f32 %33, 0
  %35 = extractvalue %dx.types.CBufRet.f32 %33, 1
  %36 = extractvalue %dx.types.CBufRet.f32 %33, 2
  %37 = extractvalue %dx.types.CBufRet.f32 %33, 3
  %38 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %39 = extractvalue %dx.types.CBufRet.f32 %38, 0
  %40 = extractvalue %dx.types.CBufRet.f32 %38, 1
  %41 = extractvalue %dx.types.CBufRet.f32 %38, 2
  %42 = extractvalue %dx.types.CBufRet.f32 %38, 3
  %43 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %44 = extractvalue %dx.types.CBufRet.f32 %43, 0
  %45 = extractvalue %dx.types.CBufRet.f32 %43, 1
  %46 = extractvalue %dx.types.CBufRet.f32 %43, 2
  %47 = extractvalue %dx.types.CBufRet.f32 %43, 3
  %48 = fmul fast float %29, %.i0179
  %FMad118 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %34, float %48)  ; FMad(a,b,c)
  %FMad117 = call float @dx.op.tertiary.f32(i32 46, float %25, float %39, float %FMad118)  ; FMad(a,b,c)
  %49 = fadd fast float %FMad117, %44
  %50 = fmul fast float %30, %.i0179
  %FMad115 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %35, float %50)  ; FMad(a,b,c)
  %FMad114 = call float @dx.op.tertiary.f32(i32 46, float %25, float %40, float %FMad115)  ; FMad(a,b,c)
  %51 = fadd fast float %FMad114, %45
  %52 = fmul fast float %31, %.i0179
  %FMad112 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %36, float %52)  ; FMad(a,b,c)
  %FMad111 = call float @dx.op.tertiary.f32(i32 46, float %25, float %41, float %FMad112)  ; FMad(a,b,c)
  %53 = fadd fast float %FMad111, %46
  %54 = fmul fast float %32, %.i0179
  %FMad109 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %37, float %54)  ; FMad(a,b,c)
  %FMad108 = call float @dx.op.tertiary.f32(i32 46, float %25, float %42, float %FMad109)  ; FMad(a,b,c)
  %55 = fadd fast float %FMad108, %47
  %56 = fdiv fast float 1.000000e+00, %55
  %.i0181 = fmul fast float %56, %49
  %.i1182 = fmul fast float %56, %51
  %.i2 = fmul fast float %56, %53
  %57 = fmul fast float %.i0181, %.i0181
  %58 = fmul fast float %.i1182, %.i1182
  %59 = fadd fast float %57, %58
  %60 = fmul fast float %.i2, %.i2
  %61 = fadd fast float %59, %60
  %Sqrt72 = call float @dx.op.unary.f32(i32 24, float %61)  ; Sqrt(value)
  %.i0183 = fdiv fast float %.i0181, %Sqrt72
  %.i1184 = fdiv fast float %.i1182, %Sqrt72
  %.i2185 = fdiv fast float %.i2, %Sqrt72
  %62 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 22)  ; CBufferLoadLegacy(handle,regIndex)
  %63 = extractvalue %dx.types.CBufRet.f32 %62, 0
  %64 = extractvalue %dx.types.CBufRet.f32 %62, 1
  %65 = extractvalue %dx.types.CBufRet.f32 %62, 2
  %66 = extractvalue %dx.types.CBufRet.f32 %62, 3
  %67 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 23)  ; CBufferLoadLegacy(handle,regIndex)
  %68 = extractvalue %dx.types.CBufRet.f32 %67, 0
  %69 = extractvalue %dx.types.CBufRet.f32 %67, 1
  %70 = extractvalue %dx.types.CBufRet.f32 %67, 2
  %71 = extractvalue %dx.types.CBufRet.f32 %67, 3
  %72 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 24)  ; CBufferLoadLegacy(handle,regIndex)
  %73 = extractvalue %dx.types.CBufRet.f32 %72, 0
  %74 = extractvalue %dx.types.CBufRet.f32 %72, 1
  %75 = extractvalue %dx.types.CBufRet.f32 %72, 2
  %76 = extractvalue %dx.types.CBufRet.f32 %72, 3
  %77 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 25)  ; CBufferLoadLegacy(handle,regIndex)
  %78 = extractvalue %dx.types.CBufRet.f32 %77, 0
  %79 = extractvalue %dx.types.CBufRet.f32 %77, 1
  %80 = extractvalue %dx.types.CBufRet.f32 %77, 2
  %81 = extractvalue %dx.types.CBufRet.f32 %77, 3
  %82 = fmul fast float %63, %.i0179
  %FMad160 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %68, float %82)  ; FMad(a,b,c)
  %FMad159 = call float @dx.op.tertiary.f32(i32 46, float %25, float %73, float %FMad160)  ; FMad(a,b,c)
  %83 = fadd fast float %FMad159, %78
  %84 = fmul fast float %64, %.i0179
  %FMad157 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %69, float %84)  ; FMad(a,b,c)
  %FMad156 = call float @dx.op.tertiary.f32(i32 46, float %25, float %74, float %FMad157)  ; FMad(a,b,c)
  %85 = fadd fast float %FMad156, %79
  %86 = fmul fast float %65, %.i0179
  %FMad154 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %70, float %86)  ; FMad(a,b,c)
  %FMad153 = call float @dx.op.tertiary.f32(i32 46, float %25, float %75, float %FMad154)  ; FMad(a,b,c)
  %87 = fadd fast float %FMad153, %80
  %88 = fmul fast float %66, %.i0179
  %FMad151 = call float @dx.op.tertiary.f32(i32 46, float %.i1180508, float %71, float %88)  ; FMad(a,b,c)
  %FMad150 = call float @dx.op.tertiary.f32(i32 46, float %25, float %76, float %FMad151)  ; FMad(a,b,c)
  %89 = fadd fast float %FMad150, %81
  %90 = fdiv fast float 1.000000e+00, %89
  %.i0190 = fmul fast float %90, %83
  %.i1191 = fmul fast float %90, %85
  %.i2192 = fmul fast float %90, %87
  %91 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 54)  ; CBufferLoadLegacy(handle,regIndex)
  %92 = extractvalue %dx.types.CBufRet.f32 %91, 0
  %93 = extractvalue %dx.types.CBufRet.f32 %91, 1
  %94 = extractvalue %dx.types.CBufRet.f32 %91, 2
  %.i0193 = fsub fast float %.i0190, %92
  %.i1194 = fsub fast float %.i1191, %93
  %.i2195 = fsub fast float %.i2192, %94
  %95 = fmul fast float %.i0193, %.i0193
  %96 = fmul fast float %.i1194, %.i1194
  %97 = fadd fast float %95, %96
  %98 = fmul fast float %.i2195, %.i2195
  %99 = fadd fast float %97, %98
  %Sqrt73 = call float @dx.op.unary.f32(i32 24, float %99)  ; Sqrt(value)
  %.i0196 = fdiv fast float %.i0193, %Sqrt73
  %.i1197 = fdiv fast float %.i1194, %Sqrt73
  %.i2198 = fdiv fast float %.i2195, %Sqrt73
  %100 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %13)  ; CreateHandleForLib(Resource)
  %TextureLoad162 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %100, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex169, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %101 = extractvalue %dx.types.ResRet.f32 %TextureLoad162, 0
  %102 = extractvalue %dx.types.ResRet.f32 %TextureLoad162, 1
  %103 = extractvalue %dx.types.ResRet.f32 %TextureLoad162, 2
  %104 = extractvalue %dx.types.ResRet.f32 %TextureLoad162, 3
  %105 = fmul fast float %104, 2.550000e+02
  %106 = fadd fast float %105, 5.000000e-01
  %107 = fptoui float %106 to i32
  %108 = and i32 %107, 254
  %109 = uitofp i32 %108 to float
  %.i0199 = fmul fast float %101, 2.550000e+02
  %.i1200 = fmul fast float %102, 2.550000e+02
  %.i0202 = fadd fast float %.i0199, 5.000000e-01
  %.i1203 = fadd fast float %.i1200, 5.000000e-01
  %.i2204 = fadd fast float %109, 5.000000e-01
  %Round_ni66 = call float @dx.op.unary.f32(i32 27, float %.i0202)  ; Round_ni(value)
  %Round_ni67 = call float @dx.op.unary.f32(i32 27, float %.i1203)  ; Round_ni(value)
  %Round_ni68 = call float @dx.op.unary.f32(i32 27, float %.i2204)  ; Round_ni(value)
  %.i0205 = fptosi float %Round_ni66 to i32
  %.i1206 = fptosi float %Round_ni67 to i32
  %.i2207 = fptosi float %Round_ni68 to i32
  %.i0208.506 = lshr i32 %.i2207, 1
  %.i1209.507 = lshr i32 %.i2207, 4
  %.i0210 = and i32 %.i0208.506, 7
  %.i1211 = and i32 %.i1209.507, 15
  %.i0212 = shl i32 %.i0205, 3
  %.i1213 = shl i32 %.i1206, 4
  %.i0214 = or i32 %.i0210, %.i0212
  %.i1215 = or i32 %.i1211, %.i1213
  %110 = sitofp i32 %.i0214 to float
  %111 = sitofp i32 %.i1215 to float
  %112 = fmul fast float %110, 0x3F54CF66A0000000
  %.i0222 = fadd fast float %112, 0xBFF4CCCCC0000000
  %113 = fmul fast float %111, 0x3F44CE19C0000000
  %.i1223 = fadd fast float %113, 0xBFF4CCCCC0000000
  %114 = fmul fast float %.i0222, %.i0222
  %115 = fmul fast float %.i1223, %.i1223
  %116 = fadd fast float %114, 1.000000e+00
  %117 = fadd fast float %116, %115
  %118 = fmul fast float %110, 0x3F64CF66A0000000
  %.i0224 = fadd fast float %118, 0xC004CCCCC0000000
  %119 = fmul fast float %111, 0x3F54CE19C0000000
  %.i1225 = fadd fast float %119, 0xC004CCCCC0000000
  %120 = fadd fast float %115, -1.000000e+00
  %121 = fadd fast float %120, %114
  %.i0226 = fdiv fast float %.i0224, %117
  %.i1227 = fdiv fast float %.i1225, %117
  %.i2228 = fdiv fast float %121, %117
  %122 = fmul fast float %.i0226, %.i0226
  %123 = fmul fast float %.i1227, %.i1227
  %124 = fadd fast float %123, %122
  %125 = fmul fast float %.i2228, %.i2228
  %126 = fadd fast float %124, %125
  %Sqrt74 = call float @dx.op.unary.f32(i32 24, float %126)  ; Sqrt(value)
  %.i0229 = fdiv fast float %.i0226, %Sqrt74
  %.i1230 = fdiv fast float %.i1227, %Sqrt74
  %.i2231 = fdiv fast float %.i2228, %Sqrt74
  %127 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %128 = extractvalue %dx.types.CBufRet.f32 %127, 0
  %129 = extractvalue %dx.types.CBufRet.f32 %127, 1
  %130 = extractvalue %dx.types.CBufRet.f32 %127, 2
  %131 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %132 = extractvalue %dx.types.CBufRet.f32 %131, 0
  %133 = extractvalue %dx.types.CBufRet.f32 %131, 1
  %134 = extractvalue %dx.types.CBufRet.f32 %131, 2
  %135 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %136 = extractvalue %dx.types.CBufRet.f32 %135, 0
  %137 = extractvalue %dx.types.CBufRet.f32 %135, 1
  %138 = extractvalue %dx.types.CBufRet.f32 %135, 2
  %139 = fmul fast float %128, %.i0229
  %FMad124 = call float @dx.op.tertiary.f32(i32 46, float %.i1230, float %132, float %139)  ; FMad(a,b,c)
  %FMad123 = call float @dx.op.tertiary.f32(i32 46, float %.i2231, float %136, float %FMad124)  ; FMad(a,b,c)
  %140 = fmul fast float %129, %.i0229
  %FMad122 = call float @dx.op.tertiary.f32(i32 46, float %.i1230, float %133, float %140)  ; FMad(a,b,c)
  %FMad121 = call float @dx.op.tertiary.f32(i32 46, float %.i2231, float %137, float %FMad122)  ; FMad(a,b,c)
  %141 = fmul fast float %130, %.i0229
  %FMad120 = call float @dx.op.tertiary.f32(i32 46, float %.i1230, float %134, float %141)  ; FMad(a,b,c)
  %FMad119 = call float @dx.op.tertiary.f32(i32 46, float %.i2231, float %138, float %FMad120)  ; FMad(a,b,c)
  %142 = fsub fast float 1.000000e+00, %103
  %.i0232 = fmul fast float %27, 0x3F50624DE0000000
  %.i0235 = fmul fast float %.i0232, %.i0196
  %.i1236 = fmul fast float %.i0232, %.i1197
  %.i2237 = fmul fast float %.i0232, %.i2198
  %.i0238 = fsub fast float %.i0190, %.i0235
  %.i1239 = fsub fast float %.i1191, %.i1236
  %.i2240 = fsub fast float %.i2192, %.i2237
  %.i0244 = fmul fast float %.i0232, %FMad123
  %.i1245 = fmul fast float %.i0232, %FMad121
  %.i2246 = fmul fast float %.i0232, %FMad119
  %.i0247 = fadd fast float %.i0238, %.i0244
  %.i1248 = fadd fast float %.i1239, %.i1245
  %.i2249 = fadd fast float %.i2240, %.i2246
  %.upto0444 = insertelement <3 x float> undef, float %.i0247, i32 0
  %.upto1445 = insertelement <3 x float> %.upto0444, float %.i1248, i32 1
  %143 = insertelement <3 x float> %.upto1445, float %.i2249, i32 2
  %144 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 0
  store <3 x float> %143, <3 x float>* %144, align 4, !tbaa !524
  %145 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 1
  store float 0.000000e+00, float* %145, align 4, !tbaa !527
  %146 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %12)  ; CreateHandleForLib(Resource)
  %TextureLoad163 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %146, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex169, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %147 = extractvalue %dx.types.ResRet.f32 %TextureLoad163, 1
  %148 = extractvalue %dx.types.ResRet.f32 %TextureLoad163, 2
  %149 = extractvalue %dx.types.ResRet.f32 %TextureLoad163, 3
  %150 = fmul fast float %148, 2.550000e+02
  %151 = fptoui float %150 to i32
  %152 = shl i32 %151, 8
  %153 = fmul fast float %149, 2.550000e+02
  %154 = fptoui float %153 to i32
  %155 = or i32 %152, %154
  %156 = call %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<MaterialDataPart1>"(i32 160, %"class.StructuredBuffer<MaterialDataPart1>" %9)  ; CreateHandleForLib(Resource)
  %RawBufferLoad165 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %156, i32 %155, i32 0, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %157 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad165, 0
  %158 = lshr i32 %157, 24
  %159 = uitofp i32 %158 to float
  %160 = lshr i32 %157, 16
  %161 = and i32 %160, 255
  %162 = uitofp i32 %161 to float
  %163 = lshr i32 %157, 8
  %164 = and i32 %163, 255
  %165 = uitofp i32 %164 to float
  %.i0250 = fmul fast float %159, 0x3F70101020000000
  %.i1251 = fmul fast float %162, 0x3F70101020000000
  %.i2252 = fmul fast float %165, 0x3F70101020000000
  %166 = and i32 %157, 255
  %167 = icmp eq i32 %166, 0
  br i1 %167, label %168, label %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"

; <label>:168                                     ; preds = %0
  %169 = call float @dx.op.dot3.f32(i32 55, float %.i0250, float %.i1251, float %.i2252, float 0x3FCB367A00000000, float 0x3FE6E2EB20000000, float 0x3FB27BB300000000)  ; Dot3(ax,ay,az,bx,by,bz)
  %170 = fmul fast float %169, 1.000000e+04
  %Saturate47 = call float @dx.op.unary.f32(i32 7, float %170)  ; Saturate(value)
  %171 = fadd fast float %169, 0x3EB0C6F7A0000000
  %172 = fdiv fast float 1.000000e+00, %171
  %.i0253 = fmul fast float %172, %.i0250
  %.i1254 = fmul fast float %172, %.i1251
  %.i2255 = fmul fast float %172, %.i2252
  %.i0256 = fadd fast float %.i0253, -1.000000e+00
  %.i1257 = fadd fast float %.i1254, -1.000000e+00
  %.i2258 = fadd fast float %.i2255, -1.000000e+00
  %.i0259 = fmul fast float %.i0256, %Saturate47
  %.i1260 = fmul fast float %.i1257, %Saturate47
  %.i2261 = fmul fast float %.i2258, %Saturate47
  %.i0262 = fadd fast float %.i0259, 1.000000e+00
  %.i1263 = fadd fast float %.i1260, 1.000000e+00
  %.i2264 = fadd fast float %.i2261, 1.000000e+00
  %.i0265 = fmul fast float %.i0262, %147
  %.i1266 = fmul fast float %.i1263, %147
  %.i2267 = fmul fast float %.i2264, %147
  br label %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"

"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit": ; preds = %168, %0
  %.0.i0 = phi float [ %.i0265, %168 ], [ %.i0250, %0 ]
  %.0.i1 = phi float [ %.i1266, %168 ], [ %.i1251, %0 ]
  %.0.i2 = phi float [ %.i2267, %168 ], [ %.i2252, %0 ]
  %.i0268 = fsub fast float -0.000000e+00, %.i0196
  %.i1269 = fsub fast float -0.000000e+00, %.i1197
  %.i2270 = fsub fast float -0.000000e+00, %.i2198
  %173 = call float @dx.op.dot3.f32(i32 55, float %FMad123, float %FMad121, float %FMad119, float %.i0268, float %.i1269, float %.i2270)  ; Dot3(ax,ay,az,bx,by,bz)
  %174 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %7)  ; CreateHandleForLib(Resource)
  %175 = call %dx.types.Handle @dx.op.createHandleForLib.struct.SamplerState(i32 160, %struct.SamplerState %4)  ; CreateHandleForLib(Resource)
  %176 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %174, %dx.types.Handle %175, float %173, float %103, float undef, float undef, i32 undef, i32 undef, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %177 = extractvalue %dx.types.ResRet.f32 %176, 0
  %178 = extractvalue %dx.types.ResRet.f32 %176, 1
  %.i0271 = fmul fast float %177, %.0.i0
  %.i1272 = fmul fast float %177, %.0.i1
  %.i2273 = fmul fast float %177, %.0.i2
  %.i0274 = fadd fast float %.i0271, %178
  %.i1275 = fadd fast float %.i1272, %178
  %.i2276 = fadd fast float %.i2273, %178
  %.i0277 = fmul fast float %.i0274, 0x3FD45F3060000000
  %.i1278 = fmul fast float %.i1275, 0x3FD45F3060000000
  %.i2279 = fmul fast float %.i2276, 0x3FD45F3060000000
  %179 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %rtreflection174, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %180 = extractvalue %dx.types.CBufRet.f32 %179, 1
  %FMax83 = call float @dx.op.binary.f32(i32 35, float 0x3F847AE140000000, float %180)  ; FMax(a,b)
  %181 = fdiv fast float 1.000000e+00, %FMax83
  %182 = fmul fast float %181, %142
  %Saturate46 = call float @dx.op.unary.f32(i32 7, float %182)  ; Saturate(value)
  %183 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %rtreflection174, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %184 = extractvalue %dx.types.CBufRet.i32 %183, 2
  %185 = uitofp i32 %184 to float
  %186 = fadd fast float %185, -1.000000e+00
  %187 = fmul fast float %186, %Saturate46
  %188 = fadd fast float %187, 1.000000e+00
  %Round_ne45 = call float @dx.op.unary.f32(i32 26, float %188)  ; Round_ne(value)
  %189 = fptosi float %Round_ne45 to i32
  %FMax82 = call float @dx.op.binary.f32(i32 35, float %.i0277, float %.i1278)  ; FMax(a,b)
  %FMax81 = call float @dx.op.binary.f32(i32 35, float %FMax82, float %.i2279)  ; FMax(a,b)
  %190 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %rtreflection174, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %191 = extractvalue %dx.types.CBufRet.f32 %190, 3
  %192 = fcmp fast olt float %FMax81, %191
  br i1 %192, label %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit", label %193

; <label>:193                                     ; preds = %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"
  %194 = extractvalue %dx.types.CBufRet.f32 %179, 0
  %FMax80 = call float @dx.op.binary.f32(i32 35, float 0x3F847AE140000000, float %194)  ; FMax(a,b)
  %195 = fdiv fast float 1.000000e+00, %FMax80
  %196 = fmul fast float %195, %FMax81
  %Saturate44 = call float @dx.op.unary.f32(i32 7, float %196)  ; Saturate(value)
  %197 = fmul fast float %186, %Saturate44
  %198 = fadd fast float %197, 1.000000e+00
  %Round_ne = call float @dx.op.unary.f32(i32 26, float %198)  ; Round_ne(value)
  %phitmp = fptosi float %Round_ne to i32
  br label %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit"

"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit": ; preds = %193, %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"
  %199 = phi i32 [ %phitmp, %193 ], [ 0, %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit" ]
  %IMin = call i32 @dx.op.binary.i32(i32 38, i32 %199, i32 %189)  ; IMin(a,b)
  %200 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32 160, %"class.RWTexture2DArray<unsigned int>" %14)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %200, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex169, i32 %IMin, i32 65534, i32 65534, i32 65534, i32 65534, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %201 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %202 = extractvalue %dx.types.CBufRet.f32 %201, 1
  %203 = extractvalue %dx.types.CBufRet.f32 %201, 3
  %204 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %205 = extractvalue %dx.types.CBufRet.f32 %204, 1
  %206 = extractvalue %dx.types.CBufRet.f32 %204, 3
  %207 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %208 = extractvalue %dx.types.CBufRet.f32 %207, 1
  %209 = extractvalue %dx.types.CBufRet.f32 %207, 3
  %210 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 18)  ; CBufferLoadLegacy(handle,regIndex)
  %211 = extractvalue %dx.types.CBufRet.f32 %210, 0
  %212 = extractvalue %dx.types.CBufRet.f32 %210, 1
  %213 = extractvalue %dx.types.CBufRet.f32 %210, 3
  %214 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 19)  ; CBufferLoadLegacy(handle,regIndex)
  %215 = extractvalue %dx.types.CBufRet.f32 %214, 0
  %216 = extractvalue %dx.types.CBufRet.f32 %214, 1
  %217 = extractvalue %dx.types.CBufRet.f32 %214, 3
  %218 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 20)  ; CBufferLoadLegacy(handle,regIndex)
  %219 = extractvalue %dx.types.CBufRet.f32 %218, 0
  %220 = extractvalue %dx.types.CBufRet.f32 %218, 1
  %221 = extractvalue %dx.types.CBufRet.f32 %218, 3
  %222 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 21)  ; CBufferLoadLegacy(handle,regIndex)
  %223 = extractvalue %dx.types.CBufRet.f32 %222, 0
  %224 = extractvalue %dx.types.CBufRet.f32 %222, 1
  %225 = extractvalue %dx.types.CBufRet.f32 %222, 3
  %226 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %227 = extractvalue %dx.types.CBufRet.f32 %226, 0
  %228 = extractvalue %dx.types.CBufRet.f32 %226, 1
  %229 = getelementptr inbounds [3 x float], [3 x float]* %1, i32 0, i32 0
  store float %.i0190, float* %229, align 4
  %230 = getelementptr inbounds [3 x float], [3 x float]* %1, i32 0, i32 1
  store float %.i1191, float* %230, align 4
  %231 = getelementptr inbounds [3 x float], [3 x float]* %1, i32 0, i32 2
  store float %.i2192, float* %231, align 4
  %.i0284 = fmul fast float %FMad123, 4.000000e+00
  %.i1285 = fmul fast float %FMad121, 4.000000e+00
  %.i2286 = fmul fast float %FMad119, 4.000000e+00
  %Round_ne54 = call float @dx.op.unary.f32(i32 26, float %.i0284)  ; Round_ne(value)
  %Round_ne55 = call float @dx.op.unary.f32(i32 26, float %.i1285)  ; Round_ne(value)
  %Round_ne56 = call float @dx.op.unary.f32(i32 26, float %.i2286)  ; Round_ne(value)
  %232 = fmul fast float %Round_ne54, %Round_ne54
  %233 = fmul fast float %Round_ne55, %Round_ne55
  %234 = fadd fast float %233, %232
  %235 = fmul fast float %Round_ne56, %Round_ne56
  %236 = fadd fast float %234, %235
  %Sqrt53 = call float @dx.op.unary.f32(i32 24, float %236)  ; Sqrt(value)
  %.i0287 = fdiv fast float %Round_ne54, %Sqrt53
  %.i1288 = fdiv fast float %Round_ne55, %Sqrt53
  %.i2289 = fdiv fast float %Round_ne56, %Sqrt53
  %FAbs63 = call float @dx.op.unary.f32(i32 6, float %.i0287)  ; FAbs(value)
  %FAbs64 = call float @dx.op.unary.f32(i32 6, float %.i1288)  ; FAbs(value)
  %FAbs65 = call float @dx.op.unary.f32(i32 6, float %.i2289)  ; FAbs(value)
  %FMax79 = call float @dx.op.binary.f32(i32 35, float %FAbs63, float %FAbs64)  ; FMax(a,b)
  %FMax78 = call float @dx.op.binary.f32(i32 35, float %FMax79, float %FAbs65)  ; FMax(a,b)
  %237 = fcmp fast oeq float %FMax78, %FAbs64
  %iMajorAxis.i.0 = zext i1 %237 to i32
  %238 = fcmp fast oeq float %FMax78, %FAbs65
  %iMajorAxis.i.1 = select i1 %238, i32 2, i32 %iMajorAxis.i.0
  %FMad138 = call float @dx.op.tertiary.f32(i32 46, float %25, float %205, float 0.000000e+00)  ; FMad(a,b,c)
  %239 = fadd fast float %FMad138, %208
  %FMad132 = call float @dx.op.tertiary.f32(i32 46, float %25, float %206, float 0.000000e+00)  ; FMad(a,b,c)
  %240 = fadd fast float %FMad132, %209
  %241 = fdiv fast float 1.000000e+00, %240
  %.i1291 = fmul fast float %241, %239
  %242 = fdiv fast float 2.000000e+00, %228
  %FMad91 = call float @dx.op.tertiary.f32(i32 46, float %242, float %202, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad90 = call float @dx.op.tertiary.f32(i32 46, float %25, float %205, float %FMad91)  ; FMad(a,b,c)
  %243 = fadd fast float %FMad90, %208
  %FMad85 = call float @dx.op.tertiary.f32(i32 46, float %242, float %203, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad84 = call float @dx.op.tertiary.f32(i32 46, float %25, float %206, float %FMad85)  ; FMad(a,b,c)
  %244 = fadd fast float %FMad84, %209
  %245 = fdiv fast float 1.000000e+00, %244
  %.i1294 = fmul fast float %245, %243
  %246 = fsub fast float %.i1294, %.i1291
  %FAbs = call float @dx.op.unary.f32(i32 6, float %246)  ; FAbs(value)
  %247 = fmul fast float %FAbs, 6.400000e+01
  %Log = call float @dx.op.unary.f32(i32 23, float %247)  ; Log(value)
  %Round_pi = call float @dx.op.unary.f32(i32 28, float %Log)  ; Round_pi(value)
  %Exp = call float @dx.op.unary.f32(i32 21, float %Round_pi)  ; Exp(value)
  %.i0296 = fdiv fast float %.i0190, %Exp
  %.i1297 = fdiv fast float %.i1191, %Exp
  %.i2298 = fdiv fast float %.i2192, %Exp
  %Round_ni57 = call float @dx.op.unary.f32(i32 27, float %.i0296)  ; Round_ni(value)
  %Round_ni58 = call float @dx.op.unary.f32(i32 27, float %.i1297)  ; Round_ni(value)
  %Round_ni59 = call float @dx.op.unary.f32(i32 27, float %.i2298)  ; Round_ni(value)
  %.i0299 = fptosi float %Round_ni57 to i32
  %.i1300 = fptosi float %Round_ni58 to i32
  %.i2301 = fptosi float %Round_ni59 to i32
  %.i0302 = sitofp i32 %.i0299 to float
  %.i1303 = sitofp i32 %.i1300 to float
  %.i2304 = sitofp i32 %.i2301 to float
  %.i0305 = fmul fast float %.i0302, %Exp
  %.i1306 = fmul fast float %.i1303, %Exp
  %.i2307 = fmul fast float %.i2304, %Exp
  %248 = getelementptr inbounds [3 x i32], [3 x i32]* %2, i32 0, i32 0
  store i32 %.i0299, i32* %248, align 4
  %249 = getelementptr inbounds [3 x i32], [3 x i32]* %2, i32 0, i32 1
  store i32 %.i1300, i32* %249, align 4
  %250 = getelementptr inbounds [3 x i32], [3 x i32]* %2, i32 0, i32 2
  store i32 %.i2301, i32* %250, align 4
  %251 = getelementptr [3 x i32], [3 x i32]* %2, i32 0, i32 %iMajorAxis.i.1
  store i32 0, i32* %251, align 4, !tbaa !529
  %252 = load i32, i32* %248, align 4
  %253 = load i32, i32* %249, align 4
  %254 = load i32, i32* %250, align 4
  %255 = shl i32 %253, 5
  %256 = xor i32 %255, %253
  %257 = shl i32 %254, 13
  %258 = xor i32 %257, %254
  %259 = add i32 %256, %252
  %260 = add i32 %259, %258
  %261 = xor i32 %260, 61
  %262 = lshr i32 %260, 16
  %263 = xor i32 %261, %262
  %264 = mul i32 %263, 9
  %265 = lshr i32 %264, 4
  %266 = xor i32 %265, %264
  %267 = mul i32 %266, 668265261
  %268 = lshr i32 %267, 15
  %269 = xor i32 %268, %267
  %270 = uitofp i32 %269 to float
  %271 = fmul fast float %270, 0x3DF0000000000000
  %272 = fdiv fast float %247, %Exp
  %273 = fmul fast float %272, 4.000000e+00
  %274 = fadd fast float %273, -2.000000e+00
  %275 = fsub fast float %274, %271
  %Saturate41 = call float @dx.op.unary.f32(i32 7, float %275)  ; Saturate(value)
  %276 = fmul fast float %Saturate41, 5.000000e-01
  %277 = fadd fast float %276, 5.000000e-01
  %278 = fmul fast float %277, %Exp
  %279 = load float, float* %229, align 4, !tbaa !527
  %280 = fsub fast float %279, %.i0305
  %281 = fcmp fast ogt float %280, %278
  %282 = fadd fast float %.i0302, 5.000000e-01
  %283 = fmul fast float %Exp, %282
  %284 = select i1 %281, float %283, float %.i0305
  %285 = load float, float* %230, align 4, !tbaa !527
  %286 = fsub fast float %285, %.i1306
  %287 = fcmp fast ogt float %286, %278
  %288 = fadd fast float %.i1303, 5.000000e-01
  %289 = fmul fast float %Exp, %288
  %290 = select i1 %287, float %289, float %.i1306
  %291 = load float, float* %231, align 4, !tbaa !527
  %292 = fsub fast float %291, %.i2307
  %293 = fcmp fast ogt float %292, %278
  %294 = fadd fast float %.i2304, 5.000000e-01
  %295 = fmul fast float %Exp, %294
  %296 = select i1 %293, float %295, float %.i2307
  %297 = getelementptr inbounds [3 x float], [3 x float]* %3, i32 0, i32 0
  store float %284, float* %297, align 4
  %298 = getelementptr inbounds [3 x float], [3 x float]* %3, i32 0, i32 1
  store float %290, float* %298, align 4
  %299 = getelementptr inbounds [3 x float], [3 x float]* %3, i32 0, i32 2
  store float %296, float* %299, align 4
  %300 = getelementptr [3 x float], [3 x float]* %3, i32 0, i32 %iMajorAxis.i.1
  store float 0.000000e+00, float* %300, align 4, !tbaa !527
  %301 = load float, float* %297, align 4
  %302 = load float, float* %298, align 4
  %303 = load float, float* %299, align 4
  %.i0392 = bitcast float %301 to i32
  %.i1393 = bitcast float %302 to i32
  %.i2394 = bitcast float %303 to i32
  %304 = shl i32 %.i1393, 5
  %305 = xor i32 %304, %.i1393
  %306 = shl i32 %.i2394, 13
  %307 = xor i32 %306, %.i2394
  %308 = add i32 %305, %.i0392
  %309 = add i32 %308, %307
  %310 = xor i32 %309, 61
  %311 = lshr i32 %309, 16
  %312 = xor i32 %310, %311
  %313 = mul i32 %312, 9
  %314 = lshr i32 %313, 4
  %315 = xor i32 %314, %313
  %316 = mul i32 %315, 668265261
  %317 = lshr i32 %316, 15
  %318 = xor i32 %317, %316
  %319 = lshr i32 %318, 16
  %.i0395 = fsub fast float %279, %284
  %.i1396 = fsub fast float %285, %290
  %.i2397 = fsub fast float %291, %296
  %320 = call float @dx.op.dot3.f32(i32 55, float %.i0395, float %.i1396, float %.i2397, float %.i0287, float %.i1288, float %.i2289)  ; Dot3(ax,ay,az,bx,by,bz)
  %.i0398 = fmul fast float %320, %.i0287
  %.i1399 = fmul fast float %320, %.i1288
  %.i2400 = fmul fast float %320, %.i2289
  %.i0401 = fadd fast float %.i0398, %284
  %.i1402 = fadd fast float %.i1399, %290
  %.i2403 = fadd fast float %.i2400, %296
  %321 = fmul fast float %.i0401, %211
  %FMad106 = call float @dx.op.tertiary.f32(i32 46, float %.i1402, float %215, float %321)  ; FMad(a,b,c)
  %FMad105 = call float @dx.op.tertiary.f32(i32 46, float %.i2403, float %219, float %FMad106)  ; FMad(a,b,c)
  %322 = fadd fast float %FMad105, %223
  %323 = fmul fast float %.i0401, %212
  %FMad103 = call float @dx.op.tertiary.f32(i32 46, float %.i1402, float %216, float %323)  ; FMad(a,b,c)
  %FMad102 = call float @dx.op.tertiary.f32(i32 46, float %.i2403, float %220, float %FMad103)  ; FMad(a,b,c)
  %324 = fadd fast float %FMad102, %224
  %325 = fmul fast float %.i0401, %213
  %FMad97 = call float @dx.op.tertiary.f32(i32 46, float %.i1402, float %217, float %325)  ; FMad(a,b,c)
  %FMad96 = call float @dx.op.tertiary.f32(i32 46, float %.i2403, float %221, float %FMad97)  ; FMad(a,b,c)
  %326 = fadd fast float %FMad96, %225
  %327 = fdiv fast float 1.000000e+00, %326
  %.i0404 = fmul fast float %322, 5.000000e-01
  %.i0407 = fmul fast float %.i0404, %327
  %.i1405 = fmul fast float %324, 5.000000e-01
  %.i1408 = fmul fast float %.i1405, %327
  %.i0409 = fadd fast float %.i0407, 5.000000e-01
  %.i1410510 = fsub fast float 5.000000e-01, %.i1408
  %.i0411 = fmul fast float %.i0409, %227
  %.i1412 = fmul fast float %.i1410510, %228
  %.i0282.neg = fsub fast float -5.000000e-01, %19
  %.i0413 = fadd fast float %.i0282.neg, %.i0411
  %.i1283.neg = fsub fast float -5.000000e-01, %20
  %.i1414 = fadd fast float %.i1283.neg, %.i1412
  %Round_ni = call float @dx.op.unary.f32(i32 27, float %.i0413)  ; Round_ni(value)
  %Round_ni51 = call float @dx.op.unary.f32(i32 27, float %.i1414)  ; Round_ni(value)
  %.i0415 = fptosi float %Round_ni to i32
  %.i1416 = fptosi float %Round_ni51 to i32
  %.i0417 = add i32 %.i0415, %319
  %.i1418 = add i32 %.i1416, %318
  %.i0419 = and i32 %.i0417, 255
  %.i1420 = and i32 %.i1418, 255
  %328 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %6)  ; CreateHandleForLib(Resource)
  %TextureLoad164 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %328, i32 0, i32 %.i0419, i32 %.i1420, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %.i0311 = extractvalue %dx.types.ResRet.f32 %TextureLoad164, 0
  %.i1313 = extractvalue %dx.types.ResRet.f32 %TextureLoad164, 1
  %.i2315 = extractvalue %dx.types.ResRet.f32 %TextureLoad164, 2
  %329 = icmp sgt i32 %IMin, 0
  br i1 %329, label %.lr.ph31.preheader, label %._crit_edge.32

.lr.ph31:                                         ; preds = %.lr.ph31.preheader, %435
  %reflectionRayIndex.030 = phi i32 [ %438, %435 ], [ 0, %.lr.ph31.preheader ]
  %330 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)  ; CBufferLoadLegacy(handle,regIndex)
  %331 = extractvalue %dx.types.CBufRet.i32 %330, 1
  %332 = mul i32 %331, %IMin
  %333 = add i32 %332, %reflectionRayIndex.030
  %334 = uitofp i32 %333 to float
  %.i0308 = fmul fast float %334, 0x3FE827F540000000
  %.i1309 = fmul fast float %334, 0x3FE23C21C0000000
  %.i2310 = fmul fast float %334, 0x3FF9E377A0000000
  %.i0312 = fadd fast float %.i0308, %.i0311
  %.i1314 = fadd fast float %.i1309, %.i1313
  %.i2316 = fadd fast float %.i2310, %.i2315
  %Frc69 = call float @dx.op.unary.f32(i32 22, float %.i0312)  ; Frc(value)
  %Frc70 = call float @dx.op.unary.f32(i32 22, float %.i1314)  ; Frc(value)
  %Frc71 = call float @dx.op.unary.f32(i32 22, float %.i2316)  ; Frc(value)
  %.i1318 = fmul fast float %Frc70, 0x3FD99999A0000000
  %335 = fmul fast float %Frc69, 0x401921FB60000000
  %336 = fsub fast float 1.000000e+00, %.i1318
  %337 = fmul fast float %.i1318, %443
  %338 = fadd fast float %337, 1.000000e+00
  %339 = fdiv fast float %336, %338
  %Saturate40 = call float @dx.op.unary.f32(i32 7, float %339)  ; Saturate(value)
  %Sqrt39 = call float @dx.op.unary.f32(i32 24, float %Saturate40)  ; Sqrt(value)
  %340 = fmul fast float %Sqrt39, %Sqrt39
  %341 = fsub fast float 1.000000e+00, %340
  %Sqrt38 = call float @dx.op.unary.f32(i32 24, float %341)  ; Sqrt(value)
  %Sin37 = call float @dx.op.unary.f32(i32 13, float %335)  ; Sin(value)
  %Cos36 = call float @dx.op.unary.f32(i32 12, float %335)  ; Cos(value)
  %342 = fmul fast float %Cos36, %Sqrt38
  %343 = fmul fast float %Sin37, %Sqrt38
  br i1 %444, label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit", label %344

; <label>:344                                     ; preds = %.lr.ph31
  br label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit"

"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit": ; preds = %344, %.lr.ph31
  %.026.i0 = phi float [ %454, %344 ], [ -1.000000e+00, %.lr.ph31 ]
  %.026.i1 = phi float [ %461, %344 ], [ 0.000000e+00, %.lr.ph31 ]
  %.026.i2 = phi float [ %462, %344 ], [ 0.000000e+00, %.lr.ph31 ]
  %.025.i0 = phi float [ %457, %344 ], [ 0.000000e+00, %.lr.ph31 ]
  %.025.i2 = phi float [ %458, %344 ], [ 0.000000e+00, %.lr.ph31 ]
  %.i0323 = fmul fast float %.025.i0, %342
  %.i1324 = fmul fast float %.026.i0, %342
  %.i2325 = fmul fast float %.025.i2, %342
  %.i0326 = fmul fast float %.026.i0, %343
  %.i1327 = fmul fast float %.026.i1, %343
  %.i2328 = fmul fast float %.026.i2, %343
  %.i0332 = fmul fast float %Sqrt39, %.i0229
  %.i1333 = fmul fast float %Sqrt39, %.i1230
  %.i2334 = fmul fast float %Sqrt39, %.i2231
  %.i0329 = fadd fast float %.i0326, %.i0332
  %.i0335 = fadd fast float %.i0329, %.i0323
  %.i1330 = fadd fast float %.i1324, %.i1333
  %.i1336 = fadd fast float %.i1330, %.i1327
  %.i2331 = fadd fast float %.i2328, %.i2334
  %.i2337 = fadd fast float %.i2331, %.i2325
  %345 = call float @dx.op.dot3.f32(i32 55, float %.i0320, float %.i1321, float %.i2322, float %.i0335, float %.i1336, float %.i2337)  ; Dot3(ax,ay,az,bx,by,bz)
  %346 = fmul fast float %345, 2.000000e+00
  %.i0338 = fmul fast float %346, %.i0335
  %.i1339 = fmul fast float %346, %.i1336
  %.i2340 = fmul fast float %.i2337, %346
  %.i0341 = fadd fast float %.i0338, %.i0183
  %.i1342 = fadd fast float %.i1339, %.i1184
  %.i2343 = fadd fast float %.i2340, %.i2185
  %347 = fmul fast float %.i0341, %.i0341
  %348 = fmul fast float %.i1342, %.i1342
  %349 = fadd fast float %347, %348
  %350 = fmul fast float %.i2343, %.i2343
  %351 = fadd fast float %349, %350
  %Sqrt75 = call float @dx.op.unary.f32(i32 24, float %351)  ; Sqrt(value)
  %.i0344 = fdiv fast float %.i0341, %Sqrt75
  %.i1345 = fdiv fast float %.i1342, %Sqrt75
  %.i2346 = fdiv fast float %.i2343, %Sqrt75
  %352 = call float @dx.op.dot3.f32(i32 55, float %.i0344, float %.i1345, float %.i2346, float %.i0229, float %.i1230, float %.i2231)  ; Dot3(ax,ay,az,bx,by,bz)
  %353 = fcmp fast olt float %352, 0x3F747AE140000000
  br i1 %353, label %.lr.ph.preheader, label %._crit_edge

.lr.ph.preheader:                                 ; preds = %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit"
  %354 = uitofp i32 %331 to float
  br label %.lr.ph

.lr.ph:                                           ; preds = %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13", %.lr.ph.preheader
  %i.029 = phi i32 [ %355, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ], [ 0, %.lr.ph.preheader ]
  %355 = add nuw nsw i32 %i.029, 1
  %356 = uitofp i32 %355 to float
  %357 = fmul fast float %356, 1.000000e+01
  %358 = fadd fast float %357, %354
  %.i0347 = fmul fast float %358, 0x3FE827F540000000
  %.i1348 = fmul fast float %358, 0x3FE23C21C0000000
  %.i0349 = fadd fast float %.i0347, %.i0311
  %.i1350 = fadd fast float %.i1348, %.i1313
  %Frc = call float @dx.op.unary.f32(i32 22, float %.i0349)  ; Frc(value)
  %Frc52 = call float @dx.op.unary.f32(i32 22, float %.i1350)  ; Frc(value)
  %.i1352 = fmul fast float %Frc52, 0x3FD99999A0000000
  %359 = fmul fast float %Frc, 0x401921FB60000000
  %360 = fsub fast float 1.000000e+00, %.i1352
  %361 = fmul fast float %.i1352, %443
  %362 = fadd fast float %361, 1.000000e+00
  %363 = fdiv fast float %360, %362
  %Saturate = call float @dx.op.unary.f32(i32 7, float %363)  ; Saturate(value)
  %Sqrt35 = call float @dx.op.unary.f32(i32 24, float %Saturate)  ; Sqrt(value)
  %364 = fmul fast float %Sqrt35, %Sqrt35
  %365 = fsub fast float 1.000000e+00, %364
  %Sqrt = call float @dx.op.unary.f32(i32 24, float %365)  ; Sqrt(value)
  %Sin = call float @dx.op.unary.f32(i32 13, float %359)  ; Sin(value)
  %Cos = call float @dx.op.unary.f32(i32 12, float %359)  ; Cos(value)
  %366 = fmul fast float %Cos, %Sqrt
  %367 = fmul fast float %Sin, %Sqrt
  br i1 %444, label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13", label %368

; <label>:368                                     ; preds = %.lr.ph
  br label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13"

"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13": ; preds = %368, %.lr.ph
  %.028.i0 = phi float [ %454, %368 ], [ -1.000000e+00, %.lr.ph ]
  %.028.i1 = phi float [ %461, %368 ], [ 0.000000e+00, %.lr.ph ]
  %.028.i2 = phi float [ %462, %368 ], [ 0.000000e+00, %.lr.ph ]
  %.027.i0 = phi float [ %457, %368 ], [ 0.000000e+00, %.lr.ph ]
  %.027.i2 = phi float [ %458, %368 ], [ 0.000000e+00, %.lr.ph ]
  %.i0356 = fmul fast float %.027.i0, %366
  %.i1357 = fmul fast float %.028.i0, %366
  %.i2358 = fmul fast float %.027.i2, %366
  %.i0359 = fmul fast float %.028.i0, %367
  %.i1360 = fmul fast float %.028.i1, %367
  %.i2361 = fmul fast float %.028.i2, %367
  %.i0365 = fmul fast float %Sqrt35, %.i0229
  %.i1366 = fmul fast float %Sqrt35, %.i1230
  %.i2367 = fmul fast float %Sqrt35, %.i2231
  %.i0362 = fadd fast float %.i0359, %.i0365
  %.i0368 = fadd fast float %.i0362, %.i0356
  %.i1363 = fadd fast float %.i1357, %.i1366
  %.i1369 = fadd fast float %.i1363, %.i1360
  %.i2364 = fadd fast float %.i2361, %.i2367
  %.i2370 = fadd fast float %.i2364, %.i2358
  %369 = call float @dx.op.dot3.f32(i32 55, float %.i0320, float %.i1321, float %.i2322, float %.i0368, float %.i1369, float %.i2370)  ; Dot3(ax,ay,az,bx,by,bz)
  %370 = fmul fast float %369, 2.000000e+00
  %.i0371 = fmul fast float %370, %.i0368
  %.i1372 = fmul fast float %370, %.i1369
  %.i2373 = fmul fast float %.i2370, %370
  %.i0374 = fadd fast float %.i0371, %.i0183
  %.i1375 = fadd fast float %.i1372, %.i1184
  %.i2376 = fadd fast float %.i2373, %.i2185
  %371 = fmul fast float %.i0374, %.i0374
  %372 = fmul fast float %.i1375, %.i1375
  %373 = fadd fast float %371, %372
  %374 = fmul fast float %.i2376, %.i2376
  %375 = fadd fast float %373, %374
  %Sqrt76 = call float @dx.op.unary.f32(i32 24, float %375)  ; Sqrt(value)
  %.i0377 = fdiv fast float %.i0374, %Sqrt76
  %.i1378 = fdiv fast float %.i1375, %Sqrt76
  %.i2379 = fdiv fast float %.i2376, %Sqrt76
  %376 = call float @dx.op.dot3.f32(i32 55, float %.i0377, float %.i1378, float %.i2379, float %.i0229, float %.i1230, float %.i2231)  ; Dot3(ax,ay,az,bx,by,bz)
  %377 = fcmp fast olt float %376, 0x3F747AE140000000
  %378 = icmp ult i32 %355, 100
  %379 = and i1 %378, %377
  br i1 %379, label %.lr.ph, label %._crit_edge.loopexit

._crit_edge.loopexit:                             ; preds = %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13"
  %.i2379.lcssa = phi float [ %.i2379, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ]
  %.i1378.lcssa = phi float [ %.i1378, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ]
  %.i0377.lcssa = phi float [ %.i0377, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ]
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge.loopexit, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit"
  %vReflectionDirectionInView.0.lcssa.i0 = phi float [ %.i0344, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit" ], [ %.i0377.lcssa, %._crit_edge.loopexit ]
  %vReflectionDirectionInView.0.lcssa.i1 = phi float [ %.i1345, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit" ], [ %.i1378.lcssa, %._crit_edge.loopexit ]
  %vReflectionDirectionInView.0.lcssa.i2 = phi float [ %.i2346, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit" ], [ %.i2379.lcssa, %._crit_edge.loopexit ]
  %380 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %381 = extractvalue %dx.types.CBufRet.f32 %380, 0
  %382 = extractvalue %dx.types.CBufRet.f32 %380, 1
  %383 = extractvalue %dx.types.CBufRet.f32 %380, 2
  %384 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %385 = extractvalue %dx.types.CBufRet.f32 %384, 0
  %386 = extractvalue %dx.types.CBufRet.f32 %384, 1
  %387 = extractvalue %dx.types.CBufRet.f32 %384, 2
  %388 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %389 = extractvalue %dx.types.CBufRet.f32 %388, 0
  %390 = extractvalue %dx.types.CBufRet.f32 %388, 1
  %391 = extractvalue %dx.types.CBufRet.f32 %388, 2
  %392 = fmul fast float %381, %vReflectionDirectionInView.0.lcssa.i0
  %FMad148 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i1, float %385, float %392)  ; FMad(a,b,c)
  %FMad147 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i2, float %389, float %FMad148)  ; FMad(a,b,c)
  %393 = insertelement <3 x float> undef, float %FMad147, i64 0
  %394 = fmul fast float %382, %vReflectionDirectionInView.0.lcssa.i0
  %FMad146 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i1, float %386, float %394)  ; FMad(a,b,c)
  %FMad145 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i2, float %390, float %FMad146)  ; FMad(a,b,c)
  %395 = insertelement <3 x float> %393, float %FMad145, i64 1
  %396 = fmul fast float %383, %vReflectionDirectionInView.0.lcssa.i0
  %FMad144 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i1, float %387, float %396)  ; FMad(a,b,c)
  %FMad143 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i2, float %391, float %FMad144)  ; FMad(a,b,c)
  %397 = insertelement <3 x float> %395, float %FMad143, i64 2
  %398 = fadd fast float %Frc71, 5.000000e-01
  %399 = fmul fast float %446, %398
  %400 = fadd fast float %399, %446
  store i32 %reflectionRayIndex.030, i32* %447, align 8
  %401 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %5)  ; CreateHandleForLib(Resource)
  %402 = load <3 x float>, <3 x float>* %144, align 4
  %403 = extractelement <3 x float> %402, i64 0
  %404 = extractelement <3 x float> %402, i64 1
  %405 = extractelement <3 x float> %402, i64 2
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %401, i32 0, i32 1, i32 0, i32 2, i32 0, float %403, float %404, float %405, float %448, float %FMad147, float %FMad145, float %FMad143, float %400, %struct.HitData* nonnull %18)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  %406 = load i32, i32* %447, align 8
  %407 = icmp eq i32 %406, 0
  br i1 %407, label %435, label %408

; <label>:408                                     ; preds = %._crit_edge
  %409 = uitofp i32 %406 to float
  %410 = fmul fast float %409, 0x3F1A36E2E0000000
  %.i0381 = fmul fast float %FMad147, %410
  %.i1383 = fmul fast float %FMad145, %410
  %.i2385 = fmul fast float %FMad143, %410
  %.i0386 = extractelement <3 x float> %402, i32 0
  %.i0387 = fadd fast float %.i0386, %.i0381
  %.i1388 = extractelement <3 x float> %402, i32 1
  %.i1389 = fadd fast float %.i1388, %.i1383
  %.i2390 = extractelement <3 x float> %402, i32 2
  %.i2391 = fadd fast float %.i2390, %.i2385
  %.upto0492 = insertelement <3 x float> undef, float %.i0387, i32 0
  %.upto1493 = insertelement <3 x float> %.upto0492, float %.i1389, i32 1
  %411 = insertelement <3 x float> %.upto1493, float %.i2391, i32 2
  store <3 x float> %411, <3 x float>* %144, align 4, !tbaa !524
  %412 = call %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<DeferredLightSun>"(i32 160, %"class.StructuredBuffer<DeferredLightSun>" %8)  ; CreateHandleForLib(Resource)
  %RawBufferLoad166 = call %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32 139, %dx.types.Handle %412, i32 0, i32 0, i8 7, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %413 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad166, 0
  %414 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad166, 1
  %415 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad166, 2
  %416 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %417 = extractvalue %dx.types.CBufRet.f32 %416, 0
  %418 = extractvalue %dx.types.CBufRet.f32 %416, 1
  %419 = extractvalue %dx.types.CBufRet.f32 %416, 2
  %420 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %421 = extractvalue %dx.types.CBufRet.f32 %420, 0
  %422 = extractvalue %dx.types.CBufRet.f32 %420, 1
  %423 = extractvalue %dx.types.CBufRet.f32 %420, 2
  %424 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %425 = extractvalue %dx.types.CBufRet.f32 %424, 0
  %426 = extractvalue %dx.types.CBufRet.f32 %424, 1
  %427 = extractvalue %dx.types.CBufRet.f32 %424, 2
  %428 = fmul fast float %417, %413
  %FMad130 = call float @dx.op.tertiary.f32(i32 46, float %414, float %421, float %428)  ; FMad(a,b,c)
  %FMad129 = call float @dx.op.tertiary.f32(i32 46, float %415, float %425, float %FMad130)  ; FMad(a,b,c)
  %429 = insertelement <3 x float> undef, float %FMad129, i64 0
  %430 = fmul fast float %418, %413
  %FMad128 = call float @dx.op.tertiary.f32(i32 46, float %414, float %422, float %430)  ; FMad(a,b,c)
  %FMad127 = call float @dx.op.tertiary.f32(i32 46, float %415, float %426, float %FMad128)  ; FMad(a,b,c)
  %431 = insertelement <3 x float> %429, float %FMad127, i64 1
  %432 = fmul fast float %419, %413
  %FMad126 = call float @dx.op.tertiary.f32(i32 46, float %414, float %423, float %432)  ; FMad(a,b,c)
  %FMad125 = call float @dx.op.tertiary.f32(i32 46, float %415, float %427, float %FMad126)  ; FMad(a,b,c)
  %433 = insertelement <3 x float> %431, float %FMad125, i64 2
  store i32 %reflectionRayIndex.030, i32* %449, align 8
  %434 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %5)  ; CreateHandleForLib(Resource)
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %434, i32 4, i32 1, i32 1, i32 2, i32 1, float %.i0387, float %.i1389, float %.i2391, float %448, float %FMad129, float %FMad127, float %FMad125, float 1.000000e+03, %struct.HitData* nonnull %17)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  br label %435

; <label>:435                                     ; preds = %408, %._crit_edge
  %436 = phi float [ %400, %._crit_edge ], [ 1.000000e+03, %408 ]
  %437 = phi <3 x float> [ %397, %._crit_edge ], [ %433, %408 ]
  %438 = add nuw nsw i32 %reflectionRayIndex.030, 1
  %exitcond = icmp eq i32 %438, %IMin
  br i1 %exitcond, label %._crit_edge.32.loopexit, label %.lr.ph31

._crit_edge.32.loopexit:                          ; preds = %435
  %.lcssa516 = phi <3 x float> [ %437, %435 ]
  %.lcssa = phi float [ %436, %435 ]
  %439 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 2
  %440 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 3
  store <3 x float> %.lcssa516, <3 x float>* %439, align 4
  store float %.lcssa, float* %440, align 4
  br label %._crit_edge.32

._crit_edge.32:                                   ; preds = %._crit_edge.32.loopexit, %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit"
  ret void

.lr.ph31.preheader:                               ; preds = %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit"
  %.i0320 = fsub fast float -0.000000e+00, %.i0183
  %.i1321 = fsub fast float -0.000000e+00, %.i1184
  %.i2322 = fsub fast float -0.000000e+00, %.i2185
  %441 = fmul fast float %142, %142
  %442 = fmul fast float %441, %441
  %443 = fadd fast float %442, -1.000000e+00
  %444 = fcmp fast olt float %.i2231, 0xBFEFFFFFC0000000
  %Saturate49 = call float @dx.op.unary.f32(i32 7, float %142)  ; Saturate(value)
  %Sqrt50 = call float @dx.op.unary.f32(i32 24, float %Saturate49)  ; Sqrt(value)
  %445 = fmul fast float %Sqrt50, 6.000000e+00
  %Exp48 = call float @dx.op.unary.f32(i32 21, float %445)  ; Exp(value)
  %446 = fdiv fast float 0x4066666660000000, %Exp48
  %447 = getelementptr inbounds %struct.HitData, %struct.HitData* %18, i32 0, i32 0
  %448 = load float, float* %145, align 4
  %449 = getelementptr inbounds %struct.HitData, %struct.HitData* %17, i32 0, i32 0
  %450 = fadd fast float %.i2231, 1.000000e+00
  %451 = fdiv fast float 1.000000e+00, %450
  %452 = fmul fast float %.i0229, %.i1230
  %453 = fmul fast float %452, %451
  %454 = fsub fast float -0.000000e+00, %453
  %455 = fmul fast float %.i0229, %.i0229
  %456 = fmul fast float %455, %451
  %457 = fsub fast float 1.000000e+00, %456
  %458 = fsub fast float -0.000000e+00, %.i0229
  %459 = fmul fast float %.i1230, %.i1230
  %460 = fmul fast float %459, %451
  %461 = fsub fast float 1.000000e+00, %460
  %462 = fsub fast float -0.000000e+00, %.i1230
  br label %.lr.ph31
}

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32, %dx.types.Handle, i32) #2

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32, %dx.types.Handle, i32) #2

; Function Attrs: nounwind readnone
declare float @dx.op.unary.f32(i32, float) #3

; Function Attrs: nounwind readnone
declare float @dx.op.dot3.f32(i32, float, float, float, float, float, float) #3

; Function Attrs: nounwind readnone
declare float @dx.op.binary.f32(i32, float, float) #3

; Function Attrs: nounwind readnone
declare float @dx.op.tertiary.f32(i32, float, float, float) #3

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i32) #2

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32, %dx.types.Handle, %dx.types.Handle, float, float, float, float, i32, i32, i32, float) #2

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32, %dx.types.Handle, i32, i32, i8, i32) #2

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32, %dx.types.Handle, i32, i32, i8, i32) #2

; Function Attrs: nounwind readnone
declare i32 @dx.op.binary.i32(i32, i32, i32) #3

; Function Attrs: nounwind
declare void @dx.op.textureStore.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i32, i8) #4

; Function Attrs: nounwind readnone
declare i32 @dx.op.dispatchRaysIndex.i32(i32, i8) #3

; Function Attrs: nounwind
declare void @dx.op.traceRay.struct.HitData(i32, %dx.types.Handle, i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, %struct.HitData*) #4

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32, %sys_constants) #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.rtreflection(i32, %rtreflection) #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32, %"class.Texture2D<vector<float, 4> >") #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.struct.SamplerState(i32, %struct.SamplerState) #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<MaterialDataPart1>"(i32, %"class.StructuredBuffer<MaterialDataPart1>") #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<DeferredLightSun>"(i32, %"class.StructuredBuffer<DeferredLightSun>") #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32, %"class.RWTexture2DArray<unsigned int>") #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32, %struct.RaytracingAccelerationStructure) #2

attributes #0 = { nounwind readnone "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="0" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="0" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readonly }
attributes #3 = { nounwind readnone }
attributes #4 = { nounwind }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!1}
!dx.shaderModel = !{!2}
!dx.resources = !{!3}
!dx.typeAnnotations = !{!27, !500}
!dx.entryPoints = !{!510, !513, !515, !517, !519, !521, !522, !523}

!0 = !{!"dxc 1.2"}
!1 = !{i32 1, i32 3}
!2 = !{!"lib", i32 6, i32 3}
!3 = !{!4, !18, !22, !25}
!4 = !{!5, !7, !8, !9, !10, !12, !14, !15, !16}
!5 = !{i32 0, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer1", i32 0, i32 0, i32 1, i32 2, i32 0, !6}
!6 = !{i32 0, i32 9}
!7 = !{i32 1, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer2@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer2", i32 0, i32 1, i32 1, i32 2, i32 0, !6}
!8 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tLinearDepth", i32 0, i32 2, i32 1, i32 2, i32 0, !6}
!9 = !{i32 3, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tClipDepth", i32 0, i32 3, i32 1, i32 2, i32 0, !6}
!10 = !{i32 4, %"class.StructuredBuffer<MaterialDataPart1>"* @"\01?g_sbMaterialDataPart1@@3V?$StructuredBuffer@UMaterialDataPart1@@@@A", !"g_sbMaterialDataPart1", i32 0, i32 4, i32 1, i32 12, i32 0, !11}
!11 = !{i32 1, i32 8}
!12 = !{i32 5, %"class.StructuredBuffer<DeferredLightSun>"* @"\01?g_sbDeferredLightSunData@@3V?$StructuredBuffer@UDeferredLightSun@@@@A", !"g_sbDeferredLightSunData", i32 0, i32 5, i32 1, i32 12, i32 0, !13}
!13 = !{i32 1, i32 576}
!14 = !{i32 6, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tEnvBRDF@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tEnvBRDF", i32 0, i32 6, i32 1, i32 2, i32 0, !6}
!15 = !{i32 7, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tStaticBlueNoiseRGBA_0", i32 0, i32 7, i32 1, i32 2, i32 0, !6}
!16 = !{i32 8, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", !"g_rtScene", i32 0, i32 8, i32 1, i32 16, i32 0, !17}
!17 = !{i32 0, i32 4}
!18 = !{!19, !21}
!19 = !{i32 0, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", !"g_rwtMaterialId", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !20}
!20 = !{i32 0, i32 5}
!21 = !{i32 1, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", !"g_rwtShadow", i32 0, i32 1, i32 1, i32 7, i1 false, i1 false, i1 false, !20}
!22 = !{!23, !24}
!23 = !{i32 0, %sys_constants* @sys_constants, !"sys_constants", i32 0, i32 0, i32 1, i32 1128, null}
!24 = !{i32 1, %rtreflection* @rtreflection, !"rtreflection", i32 0, i32 1, i32 1, i32 24, null}
!25 = !{!26}
!26 = !{i32 0, %struct.SamplerState* @"\01?g_sLinearClamp@@3USamplerState@@A", !"g_sLinearClamp", i32 1, i32 6, i32 1, i32 0, null}
!27 = !{i32 0, %"class.Texture2D<vector<float, 4> >" undef, !28, %"class.Texture2D<vector<float, 4> >::mips_type" undef, !31, %struct.GBufferFormat undef, !33, %struct.SamplerState undef, !36, %"class.Texture2D<float>" undef, !38, %"class.Texture2D<float>::mips_type" undef, !31, %"class.Texture2D<unsigned int>" undef, !40, %"class.Texture2D<unsigned int>::mips_type" undef, !31, %"class.Texture2DMS<float, 0>" undef, !42, %"class.Texture2DMS<float, 0>::sample_type" undef, !31, %"class.StructuredBuffer<vector<unsigned int, 4> >" undef, !44, %"class.StructuredBuffer<MaterialDataPart1>" undef, !45, %struct.MaterialDataPart1 undef, !47, %"class.StructuredBuffer<MaterialDataPart3>" undef, !50, %struct.MaterialDataPart3 undef, !51, %"class.StructuredBuffer<MaterialBindingData>" undef, !95, %struct.MaterialBindingData undef, !96, %struct.order2_sh undef, !98, %"class.RWTexture3D<vector<float, 4> >" undef, !103, %"class.Texture3D<vector<float, 4> >" undef, !28, %"class.Texture3D<vector<float, 4> >::mips_type" undef, !31, %struct.LightVolumeData undef, !104, %"class.TextureCube<vector<float, 4> >" undef, !103, %"class.StructuredBuffer<DeferredLightPoint>" undef, !107, %struct.DeferredLightPoint undef, !108, %"class.StructuredBuffer<DeferredLightPointClipping>" undef, !123, %struct.DeferredLightPointClipping undef, !124, %"class.StructuredBuffer<DeferredLightPointBVH>" undef, !126, %struct.DeferredLightPointBVH undef, !127, %"class.RWTexture2D<unsigned int>" undef, !134, %"class.StructuredBuffer<DeferredLightSpot>" undef, !135, %struct.DeferredLightSpot undef, !136, %"class.StructuredBuffer<DeferredLightSpotClipping>" undef, !161, %struct.DeferredLightSpotClipping undef, !162, %"class.StructuredBuffer<DeferredLightSpotBVH>" undef, !126, %struct.DeferredLightSpotBVH undef, !127, %"class.StructuredBuffer<DeferredLightSun>" undef, !167, %struct.DeferredLightSun undef, !168, %"class.StructuredBuffer<VolumeSamplingInfo>" undef, !182, %struct.VolumeSamplingInfo undef, !183, %"class.StructuredBuffer<unsigned int>" undef, !134, %"class.StructuredBuffer<InternalNode>" undef, !123, %struct.InternalNode undef, !193, %"class.Texture3D<vector<float, 3> >" undef, !198, %"class.Texture3D<vector<float, 3> >::mips_type" undef, !31, %struct.VolumeCullingInCB undef, !200, %struct.VolumeTopLevelInfo undef, !205, %struct.DeferredLightSpecularBRDF undef, !209, %struct.DeferredLightGenericSurface undef, !212, %struct.DeferredLightTransparentBRDF undef, !216, %struct.DeferredLightIntensity undef, !219, %struct.DeferredLightVolumeResult undef, !222, %struct.DeferredLightPointSurface undef, !224, %struct.DeferredLightGBufferData undef, !225, %struct.CellInfo undef, !232, %"class.StructuredBuffer<EnvironmentMapDynamicInfo>" undef, !95, %struct.EnvironmentMapDynamicInfo undef, !258, %"class.StructuredBuffer<EnvironmentMapReference>" undef, !95, %struct.EnvironmentMapReference undef, !260, %"class.StructuredBuffer<CellInfo>" undef, !262, %"class.StructuredBuffer<Node>" undef, !263, %struct.Node undef, !264, %struct.ChildMask undef, !268, %struct.NodePointer undef, !270, %"class.StructuredBuffer<IrradianceProbe>" undef, !272, %struct.IrradianceProbe undef, !273, %"class.StructuredBuffer<TransportProbe>" undef, !283, %struct.TransportProbe undef, !284, %"class.StructuredBuffer<vector<float, 4> >" undef, !103, %"class.StructuredBuffer<ProbeRef>" undef, !45, %struct.ProbeRef undef, !286, %"class.StructuredBuffer<EnvironmentMapInfo>" undef, !289, %struct.EnvironmentMapInfo undef, !290, %struct.VoxelInfo undef, !296, %struct.LeafInfo undef, !302, %"class.StructuredBuffer<PerInstanceLBData>" undef, !123, %struct.PerInstanceLBData undef, !310, %"class.StructuredBuffer<RenderInstanceData>" undef, !314, %struct.RenderInstanceData undef, !315, %struct.ByteAddressBuffer undef, !36, %struct.RaytracingHitRootConstants undef, !323, %struct.RaytracingAccelerationStructure undef, !36, %struct.IntersectionAttributes undef, !334, %struct.HitData undef, !336, %"class.RWTexture2DArray<unsigned int>" undef, !134, %"class.RWTexture2DArray<vector<float, 4> >" undef, !103, %"class.RWStructuredBuffer<unsigned int>" undef, !134, %struct.RayDesc undef, !338, %debug_general undef, !343, %sys_constants undef, !366, %shadow_general undef, !399, %mid_translucency undef, !404, %mid_general undef, !96, %deferredlight_constants undef, !406, %env_general undef, !442, %atmosphere_general undef, !445, %illuminationvolume undef, !466, %ManualUpdateCB_ActiveVolumeTopLevelInfoArray undef, !473, %ManualUpdateCB_DebugVolumeTopLevelInfoArray undef, !475, %WorldGridInfo undef, !477, %EnvironmentMapAtlas undef, !481, %vertex_binding undef, !487, %g_cbRaytracingHit undef, !491, %rtreflection undef, !493}
!28 = !{i32 20, !29, !30}
!29 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 9}
!30 = !{i32 6, !"mips", i32 3, i32 16}
!31 = !{i32 4, !32}
!32 = !{i32 6, !"handle", i32 3, i32 0, i32 7, i32 5}
!33 = !{i32 32, !34, !35}
!34 = !{i32 6, !"vTarget1", i32 3, i32 0, i32 4, !"SV_Target0", i32 7, i32 9}
!35 = !{i32 6, !"vTarget2", i32 3, i32 16, i32 4, !"SV_Target1", i32 7, i32 9}
!36 = !{i32 4, !37}
!37 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 4}
!38 = !{i32 8, !29, !39}
!39 = !{i32 6, !"mips", i32 3, i32 4}
!40 = !{i32 8, !41, !39}
!41 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 5}
!42 = !{i32 8, !29, !43}
!43 = !{i32 6, !"sample", i32 3, i32 4}
!44 = !{i32 16, !41}
!45 = !{i32 8, !46}
!46 = !{i32 6, !"h", i32 3, i32 0}
!47 = !{i32 8, !48, !49}
!48 = !{i32 6, !"vSpecularColor_uBRDF", i32 3, i32 0, i32 7, i32 5}
!49 = !{i32 6, !"uScatterIndex_vTranslucencyDepthRange", i32 3, i32 4, i32 7, i32 5}
!50 = !{i32 352, !46}
!51 = !{i32 352, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !87, !88, !89, !90, !91, !92, !93, !94}
!52 = !{i32 6, !"fClothDiffScatterAmount", i32 3, i32 0, i32 7, i32 9}
!53 = !{i32 6, !"fClothFuzzWrap", i32 3, i32 4, i32 7, i32 9}
!54 = !{i32 6, !"vClothScatterColor", i32 3, i32 16, i32 7, i32 9}
!55 = !{i32 6, !"fHairSmoothness", i32 3, i32 28, i32 7, i32 9}
!56 = !{i32 6, !"vSpecularColor2", i32 3, i32 32, i32 7, i32 9}
!57 = !{i32 6, !"fHairSpecularShift", i32 3, i32 44, i32 7, i32 9}
!58 = !{i32 6, !"fHairDiffuseWrap", i32 3, i32 48, i32 7, i32 9}
!59 = !{i32 6, !"fPAD1", i32 3, i32 52, i32 7, i32 9}
!60 = !{i32 6, !"fEmissionIntensity", i32 3, i32 56, i32 7, i32 9}
!61 = !{i32 6, !"vColorMultiplier", i32 3, i32 64, i32 7, i32 9}
!62 = !{i32 6, !"vSmoothnessRange", i32 3, i32 80, i32 7, i32 9}
!63 = !{i32 6, !"vShadowSoftnessRange", i32 3, i32 88, i32 7, i32 9}
!64 = !{i32 6, !"vScatterWidthRange", i32 3, i32 96, i32 7, i32 9}
!65 = !{i32 6, !"fHairDepthOffsetAmount", i32 3, i32 104, i32 7, i32 9}
!66 = !{i32 6, !"fHairNormalAmount", i32 3, i32 108, i32 7, i32 9}
!67 = !{i32 6, !"vEmissionMultiplier", i32 3, i32 112, i32 7, i32 9}
!68 = !{i32 6, !"vColorMultiplier2", i32 3, i32 128, i32 7, i32 9}
!69 = !{i32 6, !"vBaseDetailUVScale", i32 3, i32 144, i32 7, i32 9}
!70 = !{i32 6, !"vSmoothnessRange2", i32 3, i32 152, i32 7, i32 9}
!71 = !{i32 6, !"vBlendDetailUVScale", i32 3, i32 160, i32 7, i32 9}
!72 = !{i32 6, !"fBaseDetailAmount", i32 3, i32 168, i32 7, i32 9}
!73 = !{i32 6, !"fBlendUVMultiplier", i32 3, i32 172, i32 7, i32 9}
!74 = !{i32 6, !"fBlendDetailAmount", i32 3, i32 176, i32 7, i32 9}
!75 = !{i32 6, !"fFuzzUVMultiplier", i32 3, i32 180, i32 7, i32 9}
!76 = !{i32 6, !"fPupilDilation", i32 3, i32 184, i32 7, i32 9}
!77 = !{i32 6, !"fPupilOuterRadius", i32 3, i32 188, i32 7, i32 9}
!78 = !{i32 6, !"vIntensity", i32 3, i32 192, i32 7, i32 9}
!79 = !{i32 6, !"vWrinkleWeights0", i32 3, i32 208, i32 7, i32 9}
!80 = !{i32 6, !"vWrinkleWeights1", i32 3, i32 224, i32 7, i32 9}
!81 = !{i32 6, !"vWrinkleWeights2", i32 3, i32 240, i32 7, i32 9}
!82 = !{i32 6, !"vWrinkleWeights3", i32 3, i32 256, i32 7, i32 9}
!83 = !{i32 6, !"vWrinkleWeights4", i32 3, i32 272, i32 7, i32 9}
!84 = !{i32 6, !"vWrinkleWeights5", i32 3, i32 288, i32 7, i32 9}
!85 = !{i32 6, !"uDisableBackfaceFlip", i32 3, i32 304, i32 7, i32 5}
!86 = !{i32 6, !"fEdgeSharpness", i32 3, i32 308, i32 7, i32 9}
!87 = !{i32 6, !"fTrunkBendFactor", i32 3, i32 312, i32 7, i32 9}
!88 = !{i32 6, !"fTrunkPivotOffset", i32 3, i32 316, i32 7, i32 9}
!89 = !{i32 6, !"vColorRgbMultiplier", i32 3, i32 320, i32 7, i32 9}
!90 = !{i32 6, !"fWindFactor", i32 3, i32 332, i32 7, i32 9}
!91 = !{i32 6, !"fDecalMaterialBlendFactor", i32 3, i32 336, i32 7, i32 9}
!92 = !{i32 6, !"fDecalAlbedoBlendFactor", i32 3, i32 340, i32 7, i32 9}
!93 = !{i32 6, !"uStableHash", i32 3, i32 344, i32 7, i32 5}
!94 = !{i32 6, !"pad", i32 3, i32 348, i32 7, i32 5}
!95 = !{i32 4, !46}
!96 = !{i32 4, !97}
!97 = !{i32 6, !"g_uMaterialID", i32 3, i32 0, i32 7, i32 5}
!98 = !{i32 60, !99, !100, !101, !102}
!99 = !{i32 6, !"c0", i32 3, i32 0, i32 7, i32 9}
!100 = !{i32 6, !"c11", i32 3, i32 16, i32 7, i32 9}
!101 = !{i32 6, !"c10", i32 3, i32 32, i32 7, i32 9}
!102 = !{i32 6, !"c1m1", i32 3, i32 48, i32 7, i32 9}
!103 = !{i32 16, !29}
!104 = !{i32 32, !105, !106}
!105 = !{i32 6, !"value0", i32 3, i32 0, i32 7, i32 9}
!106 = !{i32 6, !"value1", i32 3, i32 16, i32 7, i32 9}
!107 = !{i32 160, !46}
!108 = !{i32 160, !109, !110, !111, !112, !113, !114, !115, !116, !118, !119, !120, !121, !122}
!109 = !{i32 6, !"vPositionInView", i32 3, i32 0, i32 7, i32 9}
!110 = !{i32 6, !"fShadowMapShrink", i32 3, i32 12, i32 7, i32 9}
!111 = !{i32 6, !"vColor", i32 3, i32 16, i32 7, i32 9}
!112 = !{i32 6, !"fClipRange", i32 3, i32 28, i32 7, i32 9}
!113 = !{i32 6, !"vFalloff", i32 3, i32 32, i32 7, i32 9}
!114 = !{i32 6, !"vDirectionInView", i32 3, i32 48, i32 7, i32 9}
!115 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 60, i32 7, i32 9}
!116 = !{i32 6, !"mViewToShadowClip", i32 2, !117, i32 3, i32 64, i32 7, i32 9}
!117 = !{i32 4, i32 4, i32 2}
!118 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 128, i32 7, i32 9}
!119 = !{i32 6, !"fRadius", i32 3, i32 144, i32 7, i32 9}
!120 = !{i32 6, !"fBoundsRadius", i32 3, i32 148, i32 7, i32 9}
!121 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 152, i32 7, i32 9}
!122 = !{i32 6, !"uTechniqueProperties", i32 3, i32 156, i32 7, i32 5}
!123 = !{i32 16, !46}
!124 = !{i32 16, !109, !125}
!125 = !{i32 6, !"fBoundsRadius", i32 3, i32 12, i32 7, i32 9}
!126 = !{i32 44, !46}
!127 = !{i32 44, !128, !129, !130, !131, !132, !133}
!128 = !{i32 6, !"vMinInView", i32 3, i32 0, i32 7, i32 9}
!129 = !{i32 6, !"vMaxInView", i32 3, i32 16, i32 7, i32 9}
!130 = !{i32 6, !"uChildLHS", i32 3, i32 28, i32 7, i32 5}
!131 = !{i32 6, !"uChildRHS", i32 3, i32 32, i32 7, i32 5}
!132 = !{i32 6, !"uLightIndex", i32 3, i32 36, i32 7, i32 5}
!133 = !{i32 6, !"uCount", i32 3, i32 40, i32 7, i32 5}
!134 = !{i32 4, !41}
!135 = !{i32 336, !46}
!136 = !{i32 336, !109, !137, !138, !139, !140, !141, !142, !143, !144, !145, !146, !147, !148, !149, !150, !152, !153, !154, !155, !156, !157, !158, !159, !160}
!137 = !{i32 6, !"fNearDistance", i32 3, i32 12, i32 7, i32 9}
!138 = !{i32 6, !"vWedPositionInView", i32 3, i32 16, i32 7, i32 9}
!139 = !{i32 6, !"fFarDistance", i32 3, i32 28, i32 7, i32 9}
!140 = !{i32 6, !"vDirectionInView", i32 3, i32 32, i32 7, i32 9}
!141 = !{i32 6, !"fFarWidth", i32 3, i32 44, i32 7, i32 9}
!142 = !{i32 6, !"vColor", i32 3, i32 48, i32 7, i32 9}
!143 = !{i32 6, !"fClipRange", i32 3, i32 60, i32 7, i32 9}
!144 = !{i32 6, !"vFalloff", i32 3, i32 64, i32 7, i32 9}
!145 = !{i32 6, !"fRadius", i32 3, i32 72, i32 7, i32 9}
!146 = !{i32 6, !"fSinConeAnglePerTwo_sq", i32 3, i32 76, i32 7, i32 9}
!147 = !{i32 6, !"mViewToProjectionClip", i32 2, !117, i32 3, i32 80, i32 7, i32 9}
!148 = !{i32 6, !"mCameraPositionInCone", i32 3, i32 144, i32 7, i32 9}
!149 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 156, i32 7, i32 9}
!150 = !{i32 6, !"mViewToCone", i32 2, !151, i32 3, i32 160, i32 7, i32 9}
!151 = !{i32 3, i32 3, i32 2}
!152 = !{i32 6, !"fShadowPCFSize", i32 3, i32 204, i32 7, i32 9}
!153 = !{i32 6, !"fShadowLOD", i32 3, i32 208, i32 7, i32 9}
!154 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 212, i32 7, i32 9}
!155 = !{i32 6, !"mConeToView", i32 3, i32 224, i32 7, i32 9}
!156 = !{i32 6, !"vProjectionAtlasOffsetScale", i32 3, i32 272, i32 7, i32 9}
!157 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 288, i32 7, i32 9}
!158 = !{i32 6, !"vDynamicShadowAtlasOffsetScale", i32 3, i32 304, i32 7, i32 9}
!159 = !{i32 6, !"uTechniqueProperties", i32 3, i32 320, i32 7, i32 5}
!160 = !{i32 6, !"padTo16bytes", i32 3, i32 324, i32 7, i32 5}
!161 = !{i32 48, !46}
!162 = !{i32 48, !109, !137, !163, !139, !164, !165, !166}
!163 = !{i32 6, !"vDirectionInView", i32 3, i32 16, i32 7, i32 9}
!164 = !{i32 6, !"fFarWidth", i32 3, i32 32, i32 7, i32 9}
!165 = !{i32 6, !"uTechniqueProperties", i32 3, i32 36, i32 7, i32 5}
!166 = !{i32 6, !"padTo16bytes", i32 3, i32 40, i32 7, i32 5}
!167 = !{i32 708, !46}
!168 = !{i32 708, !169, !170, !111, !171, !172, !173, !174, !175, !176, !177, !178, !179, !180, !181}
!169 = !{i32 6, !"vDirectionInView", i32 3, i32 0, i32 7, i32 9}
!170 = !{i32 6, !"pad1", i32 3, i32 12, i32 7, i32 4}
!171 = !{i32 6, !"pad2", i32 3, i32 28, i32 7, i32 4}
!172 = !{i32 6, !"mViewToProjectionClip", i32 2, !117, i32 3, i32 32, i32 7, i32 9}
!173 = !{i32 6, !"fProjectionMapTransparency", i32 3, i32 96, i32 7, i32 9}
!174 = !{i32 6, !"fRadius", i32 3, i32 100, i32 7, i32 9}
!175 = !{i32 6, !"uTechniqueProperties", i32 3, i32 104, i32 7, i32 5}
!176 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 108, i32 7, i32 9}
!177 = !{i32 6, !"uCascadeCount", i32 3, i32 112, i32 7, i32 5}
!178 = !{i32 6, !"mCascadeViewToShadowClip", i32 2, !117, i32 3, i32 128, i32 7, i32 9}
!179 = !{i32 6, !"vCascadeRange", i32 3, i32 512, i32 7, i32 9}
!180 = !{i32 6, !"fCascadePCFWidth", i32 3, i32 608, i32 7, i32 9}
!181 = !{i32 6, !"pad3", i32 3, i32 704, i32 7, i32 4}
!182 = !{i32 92, !46}
!183 = !{i32 92, !184, !185, !186, !187, !188, !189, !190, !191, !192}
!184 = !{i32 6, !"mWorldToLocalRotationMatrix", i32 2, !151, i32 3, i32 0, i32 7, i32 9}
!185 = !{i32 6, !"vOneOverTopLevelFadeLength", i32 3, i32 48, i32 7, i32 9}
!186 = !{i32 6, !"uTopLevelNodeIndicesOffset", i32 3, i32 60, i32 7, i32 5}
!187 = !{i32 6, !"uInternalNodeIndexOffset", i32 3, i32 64, i32 7, i32 5}
!188 = !{i32 6, !"uBrickIndexOffset", i32 3, i32 68, i32 7, i32 5}
!189 = !{i32 6, !"fTopLevelTexelSizeInWorld", i32 3, i32 72, i32 7, i32 9}
!190 = !{i32 6, !"fWeightMultiplier", i32 3, i32 76, i32 7, i32 9}
!191 = !{i32 6, !"uAdditive", i32 3, i32 80, i32 7, i32 5}
!192 = !{i32 6, !"fpadTo16Bytes", i32 3, i32 84, i32 7, i32 9}
!193 = !{i32 16, !194, !195, !196, !197}
!194 = !{i32 6, !"uChildMask", i32 3, i32 0, i32 7, i32 5}
!195 = !{i32 6, !"uLeafChildMask", i32 3, i32 4, i32 7, i32 5}
!196 = !{i32 6, !"uChildBlockOffset", i32 3, i32 8, i32 7, i32 5}
!197 = !{i32 6, !"uLeafChildBlockOffset", i32 3, i32 12, i32 7, i32 5}
!198 = !{i32 16, !29, !199}
!199 = !{i32 6, !"mips", i32 3, i32 12}
!200 = !{i32 32, !201, !202, !203, !204}
!201 = !{i32 6, !"vAABBMinInView", i32 3, i32 0, i32 7, i32 9}
!202 = !{i32 6, !"fPad0", i32 3, i32 12, i32 7, i32 9}
!203 = !{i32 6, !"vAABBMaxInView", i32 3, i32 16, i32 7, i32 9}
!204 = !{i32 6, !"fPad1", i32 3, i32 28, i32 7, i32 9}
!205 = !{i32 64, !206, !207, !208}
!206 = !{i32 6, !"mWorldToTopLevelMatrixDecomp", i32 3, i32 0, i32 7, i32 9}
!207 = !{i32 6, !"vTopLevelSize", i32 3, i32 48, i32 7, i32 5}
!208 = !{i32 6, !"fPad0", i32 3, i32 60, i32 7, i32 9}
!209 = !{i32 28, !210, !211}
!210 = !{i32 6, !"vResultDiffuse", i32 3, i32 0, i32 7, i32 9}
!211 = !{i32 6, !"vResultSpecular", i32 3, i32 16, i32 7, i32 9}
!212 = !{i32 36, !109, !213, !214, !215}
!213 = !{i32 6, !"vNormalInView", i32 3, i32 16, i32 7, i32 9}
!214 = !{i32 6, !"fSmoothness", i32 3, i32 28, i32 7, i32 9}
!215 = !{i32 6, !"fSpecularAlbedoIntensity", i32 3, i32 32, i32 7, i32 9}
!216 = !{i32 44, !210, !217, !218}
!217 = !{i32 6, !"vResultDiffuseTransmission", i32 3, i32 16, i32 7, i32 9}
!218 = !{i32 6, !"vResultSpecular", i32 3, i32 32, i32 7, i32 9}
!219 = !{i32 28, !220, !221}
!220 = !{i32 6, !"vIntensityDiffuse", i32 3, i32 0, i32 7, i32 9}
!221 = !{i32 6, !"vIntensitySpecular", i32 3, i32 16, i32 7, i32 9}
!222 = !{i32 32, !223}
!223 = !{i32 6, !"d", i32 3, i32 0}
!224 = !{i32 12, !109}
!225 = !{i32 68, !226, !227, !228, !229, !230, !231}
!226 = !{i32 6, !"vGBufferTarget1", i32 3, i32 0, i32 7, i32 9}
!227 = !{i32 6, !"vGBufferTarget2", i32 3, i32 16, i32 7, i32 9}
!228 = !{i32 6, !"vPositionInView", i32 3, i32 32, i32 7, i32 9}
!229 = !{i32 6, !"vBentNormalInView", i32 3, i32 48, i32 7, i32 9}
!230 = !{i32 6, !"fDiffuseOcclusion", i32 3, i32 60, i32 7, i32 9}
!231 = !{i32 6, !"fSpecularOcclusion", i32 3, i32 64, i32 7, i32 9}
!232 = !{i32 356, !233, !234, !235, !236, !237, !238, !239, !240, !241, !242, !243, !244, !245, !246, !247, !248, !249, !250, !251, !252, !253, !254, !255, !256, !257}
!233 = !{i32 6, !"m_vMin", i32 3, i32 0, i32 7, i32 9}
!234 = !{i32 6, !"m_fBaseVoxelSize", i32 3, i32 12, i32 7, i32 9}
!235 = !{i32 6, !"m_vMax", i32 3, i32 16, i32 7, i32 9}
!236 = !{i32 6, !"m_fInvBaseVoxelSize", i32 3, i32 28, i32 7, i32 9}
!237 = !{i32 6, !"m_uVoxelResolution", i32 3, i32 32, i32 7, i32 5}
!238 = !{i32 6, !"m_uLog2VoxelResolution", i32 3, i32 36, i32 7, i32 5}
!239 = !{i32 6, !"m_uBaseVoxelResolutionX", i32 3, i32 40, i32 7, i32 5}
!240 = !{i32 6, !"m_uBaseVoxelResolutionXY", i32 3, i32 44, i32 7, i32 5}
!241 = !{i32 6, !"m_iPrimaryTreeOffset", i32 3, i32 48, i32 7, i32 4}
!242 = !{i32 6, !"m_iDualTreeOffset", i32 3, i32 52, i32 7, i32 4}
!243 = !{i32 6, !"m_iIrradianceProbeOffset", i32 3, i32 56, i32 7, i32 4}
!244 = !{i32 6, !"m_iDirectTransportProbeOffset", i32 3, i32 60, i32 7, i32 4}
!245 = !{i32 6, !"m_iIndirectTransportProbeOffset", i32 3, i32 64, i32 7, i32 4}
!246 = !{i32 6, !"m_iLightGatherOffset", i32 3, i32 68, i32 7, i32 4}
!247 = !{i32 6, !"m_iLightProbeRefOffset", i32 3, i32 72, i32 7, i32 4}
!248 = !{i32 6, !"m_iDualLightProbeRefOffset", i32 3, i32 76, i32 7, i32 4}
!249 = !{i32 6, !"m_iEnvironmentMapRefOffset", i32 3, i32 80, i32 7, i32 4}
!250 = !{i32 6, !"m_iEnvironmentMapDualRefOffset", i32 3, i32 84, i32 7, i32 4}
!251 = !{i32 6, !"m_iEnvironmentMapInfoOffset", i32 3, i32 88, i32 7, i32 4}
!252 = !{i32 6, !"m_fWorldToGrid", i32 3, i32 92, i32 7, i32 9}
!253 = !{i32 6, !"m_fGridToWorld", i32 3, i32 96, i32 7, i32 9}
!254 = !{i32 6, !"pad0", i32 3, i32 100, i32 7, i32 9}
!255 = !{i32 6, !"pad1", i32 3, i32 104, i32 7, i32 9}
!256 = !{i32 6, !"pad2", i32 3, i32 108, i32 7, i32 9}
!257 = !{i32 6, !"m_iRowTranslationTable", i32 3, i32 112, i32 7, i32 4}
!258 = !{i32 4, !259}
!259 = !{i32 6, !"m_fDiffuseC0", i32 3, i32 0, i32 7, i32 9}
!260 = !{i32 4, !261}
!261 = !{i32 6, !"m_data", i32 3, i32 0, i32 7, i32 5}
!262 = !{i32 356, !46}
!263 = !{i32 28, !46}
!264 = !{i32 28, !265, !266, !267}
!265 = !{i32 6, !"m_childMask", i32 3, i32 0}
!266 = !{i32 6, !"m_pointer", i32 3, i32 20}
!267 = !{i32 6, !"m_payload", i32 3, i32 24, i32 7, i32 5}
!268 = !{i32 20, !269}
!269 = !{i32 6, !"m_uMask", i32 3, i32 0, i32 7, i32 5}
!270 = !{i32 4, !271}
!271 = !{i32 6, !"m_flagAndPointer", i32 3, i32 0, i32 7, i32 5}
!272 = !{i32 36, !46}
!273 = !{i32 36, !274, !275, !276, !277, !278, !279, !280, !281, !282}
!274 = !{i32 6, !"m_vR00", i32 3, i32 0, i32 7, i32 5}
!275 = !{i32 6, !"m_vR01", i32 3, i32 4, i32 7, i32 5}
!276 = !{i32 6, !"m_vR02", i32 3, i32 8, i32 7, i32 5}
!277 = !{i32 6, !"m_vR10", i32 3, i32 12, i32 7, i32 5}
!278 = !{i32 6, !"m_vR11", i32 3, i32 16, i32 7, i32 5}
!279 = !{i32 6, !"m_vR12", i32 3, i32 20, i32 7, i32 5}
!280 = !{i32 6, !"m_vR20", i32 3, i32 24, i32 7, i32 5}
!281 = !{i32 6, !"m_vR21", i32 3, i32 28, i32 7, i32 5}
!282 = !{i32 6, !"m_vR22", i32 3, i32 32, i32 7, i32 5}
!283 = !{i32 116, !46}
!284 = !{i32 116, !285}
!285 = !{i32 6, !"m_mTransport", i32 3, i32 0, i32 7, i32 5}
!286 = !{i32 8, !287, !288}
!287 = !{i32 6, !"m_index1", i32 3, i32 0, i32 7, i32 5}
!288 = !{i32 6, !"m_index2", i32 3, i32 4, i32 7, i32 5}
!289 = !{i32 220, !46}
!290 = !{i32 220, !291, !292, !293, !294, !295}
!291 = !{i32 6, !"m_vDebugColor", i32 3, i32 0, i32 7, i32 9}
!292 = !{i32 6, !"m_fRadius", i32 3, i32 12, i32 7, i32 9}
!293 = !{i32 6, !"m_vCenter", i32 3, i32 16, i32 7, i32 9}
!294 = !{i32 6, !"m_vMin", i32 3, i32 32, i32 7, i32 9}
!295 = !{i32 6, !"m_vMax", i32 3, i32 128, i32 7, i32 9}
!296 = !{i32 44, !297, !298, !299, !300, !301}
!297 = !{i32 6, !"m_vVoxelMin", i32 3, i32 0, i32 7, i32 9}
!298 = !{i32 6, !"m_fVoxelSize", i32 3, i32 12, i32 7, i32 9}
!299 = !{i32 6, !"m_uPayload", i32 3, i32 16, i32 7, i32 5}
!300 = !{i32 6, !"m_uLinearBaseVoxelIndex", i32 3, i32 20, i32 7, i32 5}
!301 = !{i32 6, !"m_vBaseVoxelMin", i32 3, i32 32, i32 7, i32 9}
!302 = !{i32 44, !303, !304, !305, !306, !307, !308, !309}
!303 = !{i32 6, !"m_iParentNode", i32 3, i32 0, i32 7, i32 4}
!304 = !{i32 6, !"m_vParentVoxelMin", i32 3, i32 4, i32 7, i32 9}
!305 = !{i32 6, !"m_vLeafVoxelMin", i32 3, i32 16, i32 7, i32 9}
!306 = !{i32 6, !"m_fLeafVoxelSize", i32 3, i32 28, i32 7, i32 9}
!307 = !{i32 6, !"m_bIsTerminal", i32 3, i32 32, i32 7, i32 1}
!308 = !{i32 6, !"m_bIsSolid", i32 3, i32 36, i32 7, i32 1}
!309 = !{i32 6, !"m_iPayload", i32 3, i32 40, i32 7, i32 4}
!310 = !{i32 16, !311, !312, !313}
!311 = !{i32 6, !"m_vUVOffset", i32 3, i32 0, i32 7, i32 9}
!312 = !{i32 6, !"m_uEnvMapIndex", i32 3, i32 8, i32 7, i32 5}
!313 = !{i32 6, !"m_uEnvInfoIndex", i32 3, i32 12, i32 7, i32 5}
!314 = !{i32 152, !46}
!315 = !{i32 152, !316, !317, !318, !319, !320, !321, !322}
!316 = !{i32 6, !"g_fPerLODDissolve", i32 3, i32 0, i32 7, i32 9}
!317 = !{i32 6, !"g_vWindIndex_Base_Trunk", i32 3, i32 116, i32 7, i32 5}
!318 = !{i32 6, !"g_uMask", i32 3, i32 124, i32 7, i32 5}
!319 = !{i32 6, !"g_iCharacterLightRigIndex", i32 3, i32 128, i32 7, i32 4}
!320 = !{i32 6, !"g_fDepthMultiply", i32 3, i32 132, i32 7, i32 9}
!321 = !{i32 6, !"g_fExtraDepthMultiply", i32 3, i32 136, i32 7, i32 9}
!322 = !{i32 6, !"padTo16Bytes", i32 3, i32 144, i32 7, i32 4}
!323 = !{i32 40, !324, !325, !326, !327, !328, !329, !330, !331, !332, !333}
!324 = !{i32 6, !"uIndexOffset", i32 3, i32 0, i32 7, i32 5}
!325 = !{i32 6, !"uIndexStride", i32 3, i32 4, i32 7, i32 5}
!326 = !{i32 6, !"uVertexStride1", i32 3, i32 8, i32 7, i32 5}
!327 = !{i32 6, !"uMaterialID", i32 3, i32 12, i32 7, i32 5}
!328 = !{i32 6, !"uTangentOffset", i32 3, i32 16, i32 7, i32 5}
!329 = !{i32 6, !"uNormalOffset", i32 3, i32 20, i32 7, i32 5}
!330 = !{i32 6, !"uTexcoordOffset", i32 3, i32 24, i32 7, i32 5}
!331 = !{i32 6, !"uColorOffset", i32 3, i32 28, i32 7, i32 5}
!332 = !{i32 6, !"uIndexBufferSize", i32 3, i32 32, i32 7, i32 5}
!333 = !{i32 6, !"uVertexBuffer1Size", i32 3, i32 36, i32 7, i32 5}
!334 = !{i32 8, !335}
!335 = !{i32 6, !"vIntersection", i32 3, i32 0, i32 4, !"INTERSECTION", i32 7, i32 9}
!336 = !{i32 4, !337}
!337 = !{i32 6, !"uRayIndex", i32 3, i32 0, i32 4, !"RAY_INDEX", i32 7, i32 5}
!338 = !{i32 32, !339, !340, !341, !342}
!339 = !{i32 6, !"Origin", i32 3, i32 0, i32 7, i32 9}
!340 = !{i32 6, !"TMin", i32 3, i32 12, i32 7, i32 9}
!341 = !{i32 6, !"Direction", i32 3, i32 16, i32 7, i32 9}
!342 = !{i32 6, !"TMax", i32 3, i32 28, i32 7, i32 9}
!343 = !{i32 88, !344, !345, !346, !347, !348, !349, !350, !351, !352, !353, !354, !355, !356, !357, !358, !359, !360, !361, !362, !363, !364, !365}
!344 = !{i32 6, !"g_fDebug_h_Slider0", i32 3, i32 0, i32 7, i32 9}
!345 = !{i32 6, !"g_fDebug_h_Slider1", i32 3, i32 4, i32 7, i32 9}
!346 = !{i32 6, !"g_fDebug_h_Slider2", i32 3, i32 8, i32 7, i32 9}
!347 = !{i32 6, !"g_fDebug_h_Slider3", i32 3, i32 12, i32 7, i32 9}
!348 = !{i32 6, !"g_fDebug_h_Slider4", i32 3, i32 16, i32 7, i32 9}
!349 = !{i32 6, !"g_fDebug_h_Slider5", i32 3, i32 20, i32 7, i32 9}
!350 = !{i32 6, !"g_fDebug_h_Slider6", i32 3, i32 24, i32 7, i32 9}
!351 = !{i32 6, !"g_fDebug_h_Slider7", i32 3, i32 28, i32 7, i32 9}
!352 = !{i32 6, !"g_fDebug_h_Knob0", i32 3, i32 32, i32 7, i32 9}
!353 = !{i32 6, !"g_fDebug_h_Knob1", i32 3, i32 36, i32 7, i32 9}
!354 = !{i32 6, !"g_fDebug_h_Knob2", i32 3, i32 40, i32 7, i32 9}
!355 = !{i32 6, !"g_fDebug_h_Knob3", i32 3, i32 44, i32 7, i32 9}
!356 = !{i32 6, !"g_fDebug_h_Knob4", i32 3, i32 48, i32 7, i32 9}
!357 = !{i32 6, !"g_fDebug_h_Knob5", i32 3, i32 52, i32 7, i32 9}
!358 = !{i32 6, !"g_fDebug_h_Knob6", i32 3, i32 56, i32 7, i32 9}
!359 = !{i32 6, !"g_fDebug_h_Knob7", i32 3, i32 60, i32 7, i32 9}
!360 = !{i32 6, !"g_fDebug_h_Toggle0", i32 3, i32 64, i32 7, i32 9}
!361 = !{i32 6, !"g_fDebug_h_Toggle1", i32 3, i32 68, i32 7, i32 9}
!362 = !{i32 6, !"g_fDebug_h_Toggle2", i32 3, i32 72, i32 7, i32 9}
!363 = !{i32 6, !"g_fDebug_h_Toggle3", i32 3, i32 76, i32 7, i32 9}
!364 = !{i32 6, !"g_fDebug_h_Toggle4", i32 3, i32 80, i32 7, i32 9}
!365 = !{i32 6, !"g_fDebug_h_Toggle5", i32 3, i32 84, i32 7, i32 9}
!366 = !{i32 1128, !367, !368, !369, !370, !371, !372, !373, !374, !375, !376, !377, !378, !379, !380, !381, !382, !383, !384, !385, !386, !387, !388, !389, !390, !391, !392, !393, !394, !395, !396, !397, !398}
!367 = !{i32 6, !"g_vScreenRes", i32 3, i32 0, i32 7, i32 9}
!368 = !{i32 6, !"g_vInvScreenRes", i32 3, i32 8, i32 7, i32 9}
!369 = !{i32 6, !"g_vOutputRes", i32 3, i32 16, i32 7, i32 9}
!370 = !{i32 6, !"g_vInvOutputRes", i32 3, i32 24, i32 7, i32 9}
!371 = !{i32 6, !"g_mWorldToView", i32 2, !117, i32 3, i32 32, i32 7, i32 9}
!372 = !{i32 6, !"g_mViewToWorld", i32 2, !117, i32 3, i32 96, i32 7, i32 9}
!373 = !{i32 6, !"g_mViewToClip", i32 2, !117, i32 3, i32 160, i32 7, i32 9}
!374 = !{i32 6, !"g_mClipToView", i32 2, !117, i32 3, i32 224, i32 7, i32 9}
!375 = !{i32 6, !"g_mWorldToClip", i32 2, !117, i32 3, i32 288, i32 7, i32 9}
!376 = !{i32 6, !"g_mClipToWorld", i32 2, !117, i32 3, i32 352, i32 7, i32 9}
!377 = !{i32 6, !"g_mClipToPreviousClip", i32 2, !117, i32 3, i32 416, i32 7, i32 9}
!378 = !{i32 6, !"g_mViewToPreviousClip", i32 2, !117, i32 3, i32 480, i32 7, i32 9}
!379 = !{i32 6, !"g_mPreviousViewToView", i32 2, !117, i32 3, i32 544, i32 7, i32 9}
!380 = !{i32 6, !"g_mPreviousWorldToClip", i32 2, !117, i32 3, i32 608, i32 7, i32 9}
!381 = !{i32 6, !"g_mPreviousViewToClip", i32 2, !117, i32 3, i32 672, i32 7, i32 9}
!382 = !{i32 6, !"g_mClipToPreviousClipNoJitter", i32 2, !117, i32 3, i32 736, i32 7, i32 9}
!383 = !{i32 6, !"g_mPreviousClipToClipNoJitter", i32 2, !117, i32 3, i32 800, i32 7, i32 9}
!384 = !{i32 6, !"g_vViewPoint", i32 3, i32 864, i32 7, i32 9}
!385 = !{i32 6, !"g_fInvNear", i32 3, i32 880, i32 7, i32 9}
!386 = !{i32 6, !"g_mViewToCameraView", i32 2, !117, i32 3, i32 896, i32 7, i32 9}
!387 = !{i32 6, !"g_mCameraViewToCameraClip", i32 2, !117, i32 3, i32 960, i32 7, i32 9}
!388 = !{i32 6, !"g_mCameraClipToView", i32 2, !117, i32 3, i32 1024, i32 7, i32 9}
!389 = !{i32 6, !"g_fWorldTime", i32 3, i32 1088, i32 7, i32 9}
!390 = !{i32 6, !"g_fWorldTimeDelta", i32 3, i32 1092, i32 7, i32 9}
!391 = !{i32 6, !"g_fRealTime", i32 3, i32 1096, i32 7, i32 9}
!392 = !{i32 6, !"g_fRealTimeDelta", i32 3, i32 1100, i32 7, i32 9}
!393 = !{i32 6, !"g_fLastValidWorldTimeDelta", i32 3, i32 1104, i32 7, i32 9}
!394 = !{i32 6, !"g_uTemporalFrame", i32 3, i32 1108, i32 7, i32 5}
!395 = !{i32 6, !"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5}
!396 = !{i32 6, !"g_bHDR", i32 3, i32 1116, i32 7, i32 1}
!397 = !{i32 6, !"g_fHDRSDRSceneBrightnessMultiplier", i32 3, i32 1120, i32 7, i32 9}
!398 = !{i32 6, !"g_fHDRSDRUIBrightnessMultiplier", i32 3, i32 1124, i32 7, i32 9}
!399 = !{i32 32, !400, !401, !402, !403}
!400 = !{i32 6, !"g_vShadowMapRes", i32 3, i32 0, i32 7, i32 9}
!401 = !{i32 6, !"g_vShadowMapVSMRes", i32 3, i32 8, i32 7, i32 9}
!402 = !{i32 6, !"g_vJitterOffset", i32 3, i32 16, i32 7, i32 9}
!403 = !{i32 6, !"g_vSnapOffset", i32 3, i32 24, i32 7, i32 4}
!404 = !{i32 4, !405}
!405 = !{i32 6, !"g_fOnePerTranslucencyKernelCount", i32 3, i32 0, i32 7, i32 9}
!406 = !{i32 408, !407, !408, !409, !410, !411, !412, !413, !414, !415, !416, !417, !418, !419, !420, !421, !422, !423, !424, !425, !426, !427, !428, !429, !430, !431, !432, !433, !434, !435, !436, !437, !438, !439, !440, !441}
!407 = !{i32 6, !"g_fTileDepthClipRanges", i32 3, i32 0, i32 7, i32 9}
!408 = !{i32 6, !"g_fTileDepthRanges", i32 3, i32 80, i32 7, i32 9}
!409 = !{i32 6, !"g_vDepthTileResolve", i32 3, i32 160, i32 7, i32 9}
!410 = !{i32 6, !"g_uDepthTileCount", i32 3, i32 168, i32 7, i32 5}
!411 = !{i32 6, !"g_vTileResolution", i32 3, i32 176, i32 7, i32 5}
!412 = !{i32 6, !"g_vTileWidthHeightDepth", i32 3, i32 192, i32 7, i32 5}
!413 = !{i32 6, !"g_vTileResolutionPerScreenResolution", i32 3, i32 208, i32 7, i32 9}
!414 = !{i32 6, !"g_vTileDepthNearFar", i32 3, i32 216, i32 7, i32 9}
!415 = !{i32 6, !"g_uMaxPointLightsPerTile", i32 3, i32 224, i32 7, i32 5}
!416 = !{i32 6, !"g_uMaxSpotLightsPerTile", i32 3, i32 228, i32 7, i32 5}
!417 = !{i32 6, !"g_uAmbientLightTotalCount", i32 3, i32 232, i32 7, i32 5}
!418 = !{i32 6, !"g_fAmbientEnvDiffuseIntensity", i32 3, i32 236, i32 7, i32 9}
!419 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnTransparent", i32 3, i32 240, i32 7, i32 9}
!420 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnOpaque", i32 3, i32 244, i32 7, i32 9}
!421 = !{i32 6, !"g_fAmbientSkyIntensity", i32 3, i32 248, i32 7, i32 9}
!422 = !{i32 6, !"g_fAmbientLocalIntensity", i32 3, i32 252, i32 7, i32 9}
!423 = !{i32 6, !"g_uPointLightTotalCount", i32 3, i32 256, i32 7, i32 5}
!424 = !{i32 6, !"g_uSpotLightTotalCount", i32 3, i32 260, i32 7, i32 5}
!425 = !{i32 6, !"g_uSunLightTotalCount", i32 3, i32 264, i32 7, i32 5}
!426 = !{i32 6, !"g_uAmbientLightEnabled", i32 3, i32 268, i32 7, i32 5}
!427 = !{i32 6, !"g_fAmbientDepthRampDistance", i32 3, i32 272, i32 7, i32 9}
!428 = !{i32 6, !"g_fAmbientDepthRampFeather", i32 3, i32 276, i32 7, i32 9}
!429 = !{i32 6, !"g_fAmbientDepthRampIntensityNear", i32 3, i32 280, i32 7, i32 9}
!430 = !{i32 6, !"g_fAmbientDepthRampIntensityFar", i32 3, i32 284, i32 7, i32 9}
!431 = !{i32 6, !"g_vVolumeLightDimensions", i32 3, i32 288, i32 7, i32 5}
!432 = !{i32 6, !"g_vVolumeLightProjectionConstants", i32 3, i32 304, i32 7, i32 9}
!433 = !{i32 6, !"g_vHalfResVolumeLightProjectionConstants", i32 3, i32 320, i32 7, i32 9}
!434 = !{i32 6, !"g_vOnePerVolumeLightDimensions", i32 3, i32 336, i32 7, i32 9}
!435 = !{i32 6, !"g_vVolumeLightXYToTileXY", i32 3, i32 352, i32 7, i32 9}
!436 = !{i32 6, !"g_vVolumeLightDepthResolve", i32 3, i32 368, i32 7, i32 9}
!437 = !{i32 6, !"g_fVolumeLightOnePerDepthMinusOne", i32 3, i32 380, i32 7, i32 9}
!438 = !{i32 6, !"g_vVolumeLightNearSplit0Far", i32 3, i32 384, i32 7, i32 9}
!439 = !{i32 6, !"g_fVolumeLightKernelWidth", i32 3, i32 396, i32 7, i32 9}
!440 = !{i32 6, !"g_fVolumeLightSamplingBias", i32 3, i32 400, i32 7, i32 9}
!441 = !{i32 6, !"g_uRTContactShadowRayCount", i32 3, i32 404, i32 7, i32 5}
!442 = !{i32 8, !443, !444}
!443 = !{i32 6, !"g_fEnvReflectionEdgeLength", i32 3, i32 0, i32 7, i32 9}
!444 = !{i32 6, !"g_fEnvReflectionMipCount", i32 3, i32 4, i32 7, i32 9}
!445 = !{i32 188, !446, !447, !448, !449, !450, !451, !452, !453, !454, !455, !456, !457, !458, !459, !460, !461, !462, !463, !464, !465}
!446 = !{i32 6, !"g_atmosphere_vSunDir", i32 3, i32 0, i32 7, i32 9}
!447 = !{i32 6, !"g_atmosphere_vSunE", i32 3, i32 16, i32 7, i32 9}
!448 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants1", i32 3, i32 32, i32 7, i32 9}
!449 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants2", i32 3, i32 40, i32 7, i32 9}
!450 = !{i32 6, !"g_atmosphere_vPhaseBlend", i32 3, i32 48, i32 7, i32 9}
!451 = !{i32 6, !"g_atmosphere_vPhaseG", i32 3, i32 52, i32 7, i32 9}
!452 = !{i32 6, !"g_atmosphere_vFog", i32 3, i32 64, i32 7, i32 9}
!453 = !{i32 6, !"g_vFogColor", i32 3, i32 80, i32 7, i32 9}
!454 = !{i32 6, !"g_vFogColorOpposite", i32 3, i32 96, i32 7, i32 9}
!455 = !{i32 6, !"g_fFogExp", i32 3, i32 108, i32 7, i32 9}
!456 = !{i32 6, !"g_fFogGroundDensityAtViewer", i32 3, i32 112, i32 7, i32 9}
!457 = !{i32 6, !"g_fFogGroundHeight", i32 3, i32 116, i32 7, i32 9}
!458 = !{i32 6, !"g_fFogGroundFalloff", i32 3, i32 120, i32 7, i32 9}
!459 = !{i32 6, !"g_fFogGroundDensity", i32 3, i32 124, i32 7, i32 9}
!460 = !{i32 6, !"g_vFogGroundDensityMapRange", i32 3, i32 128, i32 7, i32 9}
!461 = !{i32 6, !"g_vFogGroundSimulationVelocityAndScale", i32 3, i32 144, i32 7, i32 9}
!462 = !{i32 6, !"g_fFogDepthRampDistance", i32 3, i32 156, i32 7, i32 9}
!463 = !{i32 6, !"g_fFogDepthRampFeather", i32 3, i32 160, i32 7, i32 9}
!464 = !{i32 6, !"g_fFogDepthRampIntensityNear", i32 3, i32 164, i32 7, i32 9}
!465 = !{i32 6, !"g_vFogAlbedo", i32 3, i32 176, i32 7, i32 9}
!466 = !{i32 40, !467, !468, !469, !470, !471, !472}
!467 = !{i32 6, !"g_vOnePerVolumeAtlasSize", i32 3, i32 0, i32 7, i32 9}
!468 = !{i32 6, !"g_uActiveVolumeCount", i32 3, i32 12, i32 7, i32 5}
!469 = !{i32 6, !"g_vAdditiveAmbient", i32 3, i32 16, i32 7, i32 9}
!470 = !{i32 6, !"g_uActiveDebugVolumeCount", i32 3, i32 28, i32 7, i32 5}
!471 = !{i32 6, !"g_fTransparentBalanceEnergy", i32 3, i32 32, i32 7, i32 9}
!472 = !{i32 6, !"g_fTransparentBalanceEnergyStrength", i32 3, i32 36, i32 7, i32 9}
!473 = !{i32 24576, !474}
!474 = !{i32 6, !"g_cbActiveVolumeTopLevelInfoArray", i32 3, i32 0}
!475 = !{i32 24576, !476}
!476 = !{i32 6, !"g_cbDebugVolumeTopLevelInfoArray", i32 3, i32 0}
!477 = !{i32 12, !478, !479, !480}
!478 = !{i32 6, !"g_fWorldSize", i32 3, i32 0, i32 7, i32 9}
!479 = !{i32 6, !"g_fCellSize", i32 3, i32 4, i32 7, i32 9}
!480 = !{i32 6, !"g_iCellCount", i32 3, i32 8, i32 7, i32 4}
!481 = !{i32 24, !482, !483, !484, !485, !486}
!482 = !{i32 6, !"g_fInvEnvironmentMapsPerRow", i32 3, i32 0, i32 7, i32 9}
!483 = !{i32 6, !"g_fEnvironmentMapsPerRow", i32 3, i32 4, i32 7, i32 9}
!484 = !{i32 6, !"g_fEnvironmentMapColSize", i32 3, i32 8, i32 7, i32 9}
!485 = !{i32 6, !"g_fEnvironmentMapRowSize", i32 3, i32 12, i32 7, i32 9}
!486 = !{i32 6, !"g_fInvEnvironmentMapAtlasSize", i32 3, i32 16, i32 7, i32 9}
!487 = !{i32 48, !488, !489, !490}
!488 = !{i32 6, !"g_uStride0_uStride1_uOffset0_uOffset1", i32 3, i32 0, i32 7, i32 5}
!489 = !{i32 6, !"g_Tangent_Normal_Texcoord_Color", i32 3, i32 16, i32 7, i32 5}
!490 = !{i32 6, !"g_BIndices_BWeights", i32 3, i32 32, i32 7, i32 5}
!491 = !{i32 40, !492}
!492 = !{i32 6, !"g_cbRaytracingHit", i32 3, i32 0}
!493 = !{i32 24, !494, !495, !496, !497, !498, !499}
!494 = !{i32 6, !"g_fClampReflectionIntensity", i32 3, i32 0, i32 7, i32 9}
!495 = !{i32 6, !"g_fRTReflectionAddRays", i32 3, i32 4, i32 7, i32 9}
!496 = !{i32 6, !"g_uRTReflectionRayCount", i32 3, i32 8, i32 7, i32 5}
!497 = !{i32 6, !"g_fRTZeroRayReflectance", i32 3, i32 12, i32 7, i32 9}
!498 = !{i32 6, !"g_fRTMaxRayReflectance", i32 3, i32 16, i32 7, i32 9}
!499 = !{i32 6, !"g_fRTMaxRayRoughness", i32 3, i32 20, i32 7, i32 9}
!500 = !{i32 1, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !501, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !501, void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !508, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !501, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !501, void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !508, void ()* @"\01?reflectionRayGeneration@@YAXXZ", !509}
!501 = !{!502, !504, !506}
!502 = !{i32 1, !503, !503}
!503 = !{}
!504 = !{i32 2, !505, !503}
!505 = !{i32 4, !"SV_RayPayload"}
!506 = !{i32 0, !507, !503}
!507 = !{i32 4, !"SV_IntersectionAttributes"}
!508 = !{!502, !504}
!509 = !{!502}
!510 = !{null, !"", null, !3, !511}
!511 = !{i32 0, i64 65808, i32 5, !512}
!512 = !{i32 0}
!513 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !514}
!514 = !{i32 8, i32 9, i32 6, i32 4, i32 7, i32 8, i32 5, !512}
!515 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !516}
!516 = !{i32 8, i32 10, i32 6, i32 4, i32 7, i32 8, i32 5, !512}
!517 = !{void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !"\01?reflectionMiss@@YAXUHitData@@@Z", null, null, !518}
!518 = !{i32 8, i32 11, i32 6, i32 4, i32 5, !512}
!519 = !{void ()* @"\01?reflectionRayGeneration@@YAXXZ", !"\01?reflectionRayGeneration@@YAXXZ", null, null, !520}
!520 = !{i32 8, i32 7, i32 5, !512}
!521 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !514}
!522 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !516}
!523 = !{void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !"\01?shadowMiss@@YAXUHitData@@@Z", null, null, !518}
!524 = !{!525, !525, i64 0}
!525 = !{!"omnipotent char", !526, i64 0}
!526 = !{!"Simple C/C++ TBAA"}
!527 = !{!528, !528, i64 0}
!528 = !{!"float", !525, i64 0}
!529 = !{!530, !530, i64 0}
!530 = !{!"int", !525, i64 0}

