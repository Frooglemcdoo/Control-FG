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
; g_tEnvBRDF                        texture     f32          2d      T5             t5     1
; g_tStaticBlueNoiseRGBA_0          texture     f32          2d      T6             t6     1
; g_rtScene                         texture     i32         ras      T7             t7     1
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
%struct.RaytracingAccelerationStructure = type { i32 }
%"class.RWTexture2DArray<unsigned int>" = type { i32 }
%sys_constants = type { <2 x float>, <2 x float>, <2 x float>, <2 x float>, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, <4 x float>, float, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, float, float, float, float, float, i32, i32, i32, float, float }
%class.matrix.float.4.4 = type { [4 x <4 x float>] }
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
%"class.StructuredBuffer<DeferredLightSun>" = type { %struct.DeferredLightSun }
%struct.DeferredLightSun = type { <3 x float>, i32, <3 x float>, i32, %class.matrix.float.4.4, float, float, i32, float, i32, [6 x %class.matrix.float.4.4], [6 x <2 x float>], [6 x float], [1 x i32] }
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
  %8 = load %"class.StructuredBuffer<MaterialDataPart1>", %"class.StructuredBuffer<MaterialDataPart1>"* @"\01?g_sbMaterialDataPart1@@3V?$StructuredBuffer@UMaterialDataPart1@@@@A", align 4
  %9 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %10 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %11 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer2@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %12 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %13 = load %"class.RWTexture2DArray<unsigned int>", %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", align 4
  %14 = load %rtreflection, %rtreflection* @rtreflection, align 4
  %15 = load %sys_constants, %sys_constants* @sys_constants, align 4
  %rtreflection = call %dx.types.Handle @dx.op.createHandleForLib.rtreflection(i32 160, %rtreflection %14)  ; CreateHandleForLib(Resource)
  %sys_constants = call %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32 160, %sys_constants %15)  ; CreateHandleForLib(Resource)
  %16 = alloca %struct.HitData, align 8
  %ray = alloca %struct.RayDesc, align 4
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex162 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %17 = uitofp i32 %DispatchRaysIndex to float
  %18 = uitofp i32 %DispatchRaysIndex162 to float
  %.i0 = fadd fast float %17, 5.000000e-01
  %.i1 = fadd fast float %18, 5.000000e-01
  %19 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %20 = extractvalue %dx.types.CBufRet.f32 %19, 2
  %21 = extractvalue %dx.types.CBufRet.f32 %19, 3
  %.i0168 = fmul fast float %20, %.i0
  %.i1169 = fmul fast float %.i1, %21
  %22 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %9)  ; CreateHandleForLib(Resource)
  %TextureLoad = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %22, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex162, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %23 = extractvalue %dx.types.ResRet.f32 %TextureLoad, 0
  %24 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %10)  ; CreateHandleForLib(Resource)
  %TextureLoad155 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %24, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex162, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %25 = extractvalue %dx.types.ResRet.f32 %TextureLoad155, 0
  %.i0170 = fmul fast float %.i0168, 2.000000e+00
  %.i1171 = fmul fast float %.i1169, 2.000000e+00
  %.i0172 = fadd fast float %.i0170, -1.000000e+00
  %.i1173487 = fsub fast float 1.000000e+00, %.i1171
  %26 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 14)  ; CBufferLoadLegacy(handle,regIndex)
  %27 = extractvalue %dx.types.CBufRet.f32 %26, 0
  %28 = extractvalue %dx.types.CBufRet.f32 %26, 1
  %29 = extractvalue %dx.types.CBufRet.f32 %26, 2
  %30 = extractvalue %dx.types.CBufRet.f32 %26, 3
  %31 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %32 = extractvalue %dx.types.CBufRet.f32 %31, 0
  %33 = extractvalue %dx.types.CBufRet.f32 %31, 1
  %34 = extractvalue %dx.types.CBufRet.f32 %31, 2
  %35 = extractvalue %dx.types.CBufRet.f32 %31, 3
  %36 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %37 = extractvalue %dx.types.CBufRet.f32 %36, 0
  %38 = extractvalue %dx.types.CBufRet.f32 %36, 1
  %39 = extractvalue %dx.types.CBufRet.f32 %36, 2
  %40 = extractvalue %dx.types.CBufRet.f32 %36, 3
  %41 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %42 = extractvalue %dx.types.CBufRet.f32 %41, 0
  %43 = extractvalue %dx.types.CBufRet.f32 %41, 1
  %44 = extractvalue %dx.types.CBufRet.f32 %41, 2
  %45 = extractvalue %dx.types.CBufRet.f32 %41, 3
  %46 = fmul fast float %27, %.i0172
  %FMad124 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %32, float %46)  ; FMad(a,b,c)
  %FMad123 = call float @dx.op.tertiary.f32(i32 46, float %23, float %37, float %FMad124)  ; FMad(a,b,c)
  %47 = fadd fast float %FMad123, %42
  %48 = fmul fast float %28, %.i0172
  %FMad121 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %33, float %48)  ; FMad(a,b,c)
  %FMad120 = call float @dx.op.tertiary.f32(i32 46, float %23, float %38, float %FMad121)  ; FMad(a,b,c)
  %49 = fadd fast float %FMad120, %43
  %50 = fmul fast float %29, %.i0172
  %FMad118 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %34, float %50)  ; FMad(a,b,c)
  %FMad117 = call float @dx.op.tertiary.f32(i32 46, float %23, float %39, float %FMad118)  ; FMad(a,b,c)
  %51 = fadd fast float %FMad117, %44
  %52 = fmul fast float %30, %.i0172
  %FMad115 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %35, float %52)  ; FMad(a,b,c)
  %FMad114 = call float @dx.op.tertiary.f32(i32 46, float %23, float %40, float %FMad115)  ; FMad(a,b,c)
  %53 = fadd fast float %FMad114, %45
  %54 = fdiv fast float 1.000000e+00, %53
  %.i0174 = fmul fast float %54, %47
  %.i1175 = fmul fast float %54, %49
  %.i2 = fmul fast float %54, %51
  %55 = fmul fast float %.i0174, %.i0174
  %56 = fmul fast float %.i1175, %.i1175
  %57 = fadd fast float %55, %56
  %58 = fmul fast float %.i2, %.i2
  %59 = fadd fast float %57, %58
  %Sqrt72 = call float @dx.op.unary.f32(i32 24, float %59)  ; Sqrt(value)
  %.i0176 = fdiv fast float %.i0174, %Sqrt72
  %.i1177 = fdiv fast float %.i1175, %Sqrt72
  %.i2178 = fdiv fast float %.i2, %Sqrt72
  %60 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 22)  ; CBufferLoadLegacy(handle,regIndex)
  %61 = extractvalue %dx.types.CBufRet.f32 %60, 0
  %62 = extractvalue %dx.types.CBufRet.f32 %60, 1
  %63 = extractvalue %dx.types.CBufRet.f32 %60, 2
  %64 = extractvalue %dx.types.CBufRet.f32 %60, 3
  %65 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 23)  ; CBufferLoadLegacy(handle,regIndex)
  %66 = extractvalue %dx.types.CBufRet.f32 %65, 0
  %67 = extractvalue %dx.types.CBufRet.f32 %65, 1
  %68 = extractvalue %dx.types.CBufRet.f32 %65, 2
  %69 = extractvalue %dx.types.CBufRet.f32 %65, 3
  %70 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 24)  ; CBufferLoadLegacy(handle,regIndex)
  %71 = extractvalue %dx.types.CBufRet.f32 %70, 0
  %72 = extractvalue %dx.types.CBufRet.f32 %70, 1
  %73 = extractvalue %dx.types.CBufRet.f32 %70, 2
  %74 = extractvalue %dx.types.CBufRet.f32 %70, 3
  %75 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 25)  ; CBufferLoadLegacy(handle,regIndex)
  %76 = extractvalue %dx.types.CBufRet.f32 %75, 0
  %77 = extractvalue %dx.types.CBufRet.f32 %75, 1
  %78 = extractvalue %dx.types.CBufRet.f32 %75, 2
  %79 = extractvalue %dx.types.CBufRet.f32 %75, 3
  %80 = fmul fast float %61, %.i0172
  %FMad94 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %66, float %80)  ; FMad(a,b,c)
  %FMad93 = call float @dx.op.tertiary.f32(i32 46, float %23, float %71, float %FMad94)  ; FMad(a,b,c)
  %81 = fadd fast float %FMad93, %76
  %82 = fmul fast float %62, %.i0172
  %FMad91 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %67, float %82)  ; FMad(a,b,c)
  %FMad90 = call float @dx.op.tertiary.f32(i32 46, float %23, float %72, float %FMad91)  ; FMad(a,b,c)
  %83 = fadd fast float %FMad90, %77
  %84 = fmul fast float %63, %.i0172
  %FMad88 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %68, float %84)  ; FMad(a,b,c)
  %FMad87 = call float @dx.op.tertiary.f32(i32 46, float %23, float %73, float %FMad88)  ; FMad(a,b,c)
  %85 = fadd fast float %FMad87, %78
  %86 = fmul fast float %64, %.i0172
  %FMad85 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %69, float %86)  ; FMad(a,b,c)
  %FMad84 = call float @dx.op.tertiary.f32(i32 46, float %23, float %74, float %FMad85)  ; FMad(a,b,c)
  %87 = fadd fast float %FMad84, %79
  %88 = fdiv fast float 1.000000e+00, %87
  %.i0183 = fmul fast float %88, %81
  %.i1184 = fmul fast float %88, %83
  %.i2185 = fmul fast float %88, %85
  %89 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 54)  ; CBufferLoadLegacy(handle,regIndex)
  %90 = extractvalue %dx.types.CBufRet.f32 %89, 0
  %91 = extractvalue %dx.types.CBufRet.f32 %89, 1
  %92 = extractvalue %dx.types.CBufRet.f32 %89, 2
  %.i0186 = fsub fast float %.i0183, %90
  %.i1187 = fsub fast float %.i1184, %91
  %.i2188 = fsub fast float %.i2185, %92
  %93 = fmul fast float %.i0186, %.i0186
  %94 = fmul fast float %.i1187, %.i1187
  %95 = fadd fast float %93, %94
  %96 = fmul fast float %.i2188, %.i2188
  %97 = fadd fast float %95, %96
  %Sqrt73 = call float @dx.op.unary.f32(i32 24, float %97)  ; Sqrt(value)
  %.i0189 = fdiv fast float %.i0186, %Sqrt73
  %.i1190 = fdiv fast float %.i1187, %Sqrt73
  %.i2191 = fdiv fast float %.i2188, %Sqrt73
  %98 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %12)  ; CreateHandleForLib(Resource)
  %TextureLoad156 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %98, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex162, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %99 = extractvalue %dx.types.ResRet.f32 %TextureLoad156, 0
  %100 = extractvalue %dx.types.ResRet.f32 %TextureLoad156, 1
  %101 = extractvalue %dx.types.ResRet.f32 %TextureLoad156, 3
  %102 = fmul fast float %101, 2.550000e+02
  %103 = fadd fast float %102, 5.000000e-01
  %104 = fptoui float %103 to i32
  %105 = and i32 %104, 254
  %106 = uitofp i32 %105 to float
  %.i0192 = fmul fast float %99, 2.550000e+02
  %.i1193 = fmul fast float %100, 2.550000e+02
  %.i0195 = fadd fast float %.i0192, 5.000000e-01
  %.i1196 = fadd fast float %.i1193, 5.000000e-01
  %.i2197 = fadd fast float %106, 5.000000e-01
  %Round_ni66 = call float @dx.op.unary.f32(i32 27, float %.i0195)  ; Round_ni(value)
  %Round_ni67 = call float @dx.op.unary.f32(i32 27, float %.i1196)  ; Round_ni(value)
  %Round_ni68 = call float @dx.op.unary.f32(i32 27, float %.i2197)  ; Round_ni(value)
  %.i0198 = fptosi float %Round_ni66 to i32
  %.i1199 = fptosi float %Round_ni67 to i32
  %.i2200 = fptosi float %Round_ni68 to i32
  %.i0201.485 = lshr i32 %.i2200, 1
  %.i1202.486 = lshr i32 %.i2200, 4
  %.i0203 = and i32 %.i0201.485, 7
  %.i1204 = and i32 %.i1202.486, 15
  %.i0205 = shl i32 %.i0198, 3
  %.i1206 = shl i32 %.i1199, 4
  %.i0207 = or i32 %.i0203, %.i0205
  %.i1208 = or i32 %.i1204, %.i1206
  %107 = sitofp i32 %.i0207 to float
  %108 = sitofp i32 %.i1208 to float
  %109 = fmul fast float %107, 0x3F54CF66A0000000
  %.i0215 = fadd fast float %109, 0xBFF4CCCCC0000000
  %110 = fmul fast float %108, 0x3F44CE19C0000000
  %.i1216 = fadd fast float %110, 0xBFF4CCCCC0000000
  %111 = fmul fast float %.i0215, %.i0215
  %112 = fmul fast float %.i1216, %.i1216
  %113 = fadd fast float %111, 1.000000e+00
  %114 = fadd fast float %113, %112
  %115 = fmul fast float %107, 0x3F64CF66A0000000
  %.i0217 = fadd fast float %115, 0xC004CCCCC0000000
  %116 = fmul fast float %108, 0x3F54CE19C0000000
  %.i1218 = fadd fast float %116, 0xC004CCCCC0000000
  %117 = fadd fast float %112, -1.000000e+00
  %118 = fadd fast float %117, %111
  %.i0219 = fdiv fast float %.i0217, %114
  %.i1220 = fdiv fast float %.i1218, %114
  %.i2221 = fdiv fast float %118, %114
  %119 = fmul fast float %.i0219, %.i0219
  %120 = fmul fast float %.i1220, %.i1220
  %121 = fadd fast float %120, %119
  %122 = fmul fast float %.i2221, %.i2221
  %123 = fadd fast float %121, %122
  %Sqrt74 = call float @dx.op.unary.f32(i32 24, float %123)  ; Sqrt(value)
  %.i0222 = fdiv fast float %.i0219, %Sqrt74
  %.i1223 = fdiv fast float %.i1220, %Sqrt74
  %.i2224 = fdiv fast float %.i2221, %Sqrt74
  %124 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %125 = extractvalue %dx.types.CBufRet.f32 %124, 0
  %126 = extractvalue %dx.types.CBufRet.f32 %124, 1
  %127 = extractvalue %dx.types.CBufRet.f32 %124, 2
  %128 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %129 = extractvalue %dx.types.CBufRet.f32 %128, 0
  %130 = extractvalue %dx.types.CBufRet.f32 %128, 1
  %131 = extractvalue %dx.types.CBufRet.f32 %128, 2
  %132 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %133 = extractvalue %dx.types.CBufRet.f32 %132, 0
  %134 = extractvalue %dx.types.CBufRet.f32 %132, 1
  %135 = extractvalue %dx.types.CBufRet.f32 %132, 2
  %136 = fmul fast float %125, %.i0222
  %FMad154 = call float @dx.op.tertiary.f32(i32 46, float %.i1223, float %129, float %136)  ; FMad(a,b,c)
  %FMad153 = call float @dx.op.tertiary.f32(i32 46, float %.i2224, float %133, float %FMad154)  ; FMad(a,b,c)
  %137 = fmul fast float %126, %.i0222
  %FMad152 = call float @dx.op.tertiary.f32(i32 46, float %.i1223, float %130, float %137)  ; FMad(a,b,c)
  %FMad151 = call float @dx.op.tertiary.f32(i32 46, float %.i2224, float %134, float %FMad152)  ; FMad(a,b,c)
  %138 = fmul fast float %127, %.i0222
  %FMad150 = call float @dx.op.tertiary.f32(i32 46, float %.i1223, float %131, float %138)  ; FMad(a,b,c)
  %FMad149 = call float @dx.op.tertiary.f32(i32 46, float %.i2224, float %135, float %FMad150)  ; FMad(a,b,c)
  %.i0225 = fmul fast float %25, 0x3F50624DE0000000
  %.i0228 = fmul fast float %.i0225, %.i0189
  %.i1229 = fmul fast float %.i0225, %.i1190
  %.i2230 = fmul fast float %.i0225, %.i2191
  %.i0231 = fsub fast float %.i0183, %.i0228
  %.i1232 = fsub fast float %.i1184, %.i1229
  %.i2233 = fsub fast float %.i2185, %.i2230
  %.i0237 = fmul fast float %.i0225, %FMad153
  %.i1238 = fmul fast float %.i0225, %FMad151
  %.i2239 = fmul fast float %.i0225, %FMad149
  %.i0240 = fadd fast float %.i0231, %.i0237
  %.i1241 = fadd fast float %.i1232, %.i1238
  %.i2242 = fadd fast float %.i2233, %.i2239
  %.upto0425 = insertelement <3 x float> undef, float %.i0240, i32 0
  %.upto1426 = insertelement <3 x float> %.upto0425, float %.i1241, i32 1
  %139 = insertelement <3 x float> %.upto1426, float %.i2242, i32 2
  %140 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 0
  store <3 x float> %139, <3 x float>* %140, align 4, !tbaa !522
  %141 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 1
  store float 0.000000e+00, float* %141, align 4, !tbaa !525
  %142 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %11)  ; CreateHandleForLib(Resource)
  %TextureLoad157 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %142, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex162, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %143 = extractvalue %dx.types.ResRet.f32 %TextureLoad157, 1
  %144 = extractvalue %dx.types.ResRet.f32 %TextureLoad157, 2
  %145 = extractvalue %dx.types.ResRet.f32 %TextureLoad157, 3
  %146 = fmul fast float %144, 2.550000e+02
  %147 = fptoui float %146 to i32
  %148 = shl i32 %147, 8
  %149 = fmul fast float %145, 2.550000e+02
  %150 = fptoui float %149 to i32
  %151 = or i32 %148, %150
  %152 = call %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<MaterialDataPart1>"(i32 160, %"class.StructuredBuffer<MaterialDataPart1>" %8)  ; CreateHandleForLib(Resource)
  %RawBufferLoad159 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %152, i32 %151, i32 0, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %153 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad159, 0
  %154 = lshr i32 %153, 24
  %155 = uitofp i32 %154 to float
  %156 = lshr i32 %153, 16
  %157 = and i32 %156, 255
  %158 = uitofp i32 %157 to float
  %159 = lshr i32 %153, 8
  %160 = and i32 %159, 255
  %161 = uitofp i32 %160 to float
  %.i0243 = fmul fast float %155, 0x3F70101020000000
  %.i1244 = fmul fast float %158, 0x3F70101020000000
  %.i2245 = fmul fast float %161, 0x3F70101020000000
  %162 = and i32 %153, 255
  %163 = icmp eq i32 %162, 0
  br i1 %163, label %164, label %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"

; <label>:164                                     ; preds = %0
  %165 = call float @dx.op.dot3.f32(i32 55, float %.i0243, float %.i1244, float %.i2245, float 0x3FCB367A00000000, float 0x3FE6E2EB20000000, float 0x3FB27BB300000000)  ; Dot3(ax,ay,az,bx,by,bz)
  %166 = fmul fast float %165, 1.000000e+04
  %Saturate42 = call float @dx.op.unary.f32(i32 7, float %166)  ; Saturate(value)
  %167 = fadd fast float %165, 0x3EB0C6F7A0000000
  %168 = fdiv fast float 1.000000e+00, %167
  %.i0246 = fmul fast float %168, %.i0243
  %.i1247 = fmul fast float %168, %.i1244
  %.i2248 = fmul fast float %168, %.i2245
  %.i0249 = fadd fast float %.i0246, -1.000000e+00
  %.i1250 = fadd fast float %.i1247, -1.000000e+00
  %.i2251 = fadd fast float %.i2248, -1.000000e+00
  %.i0252 = fmul fast float %.i0249, %Saturate42
  %.i1253 = fmul fast float %.i1250, %Saturate42
  %.i2254 = fmul fast float %.i2251, %Saturate42
  %.i0255 = fadd fast float %.i0252, 1.000000e+00
  %.i1256 = fadd fast float %.i1253, 1.000000e+00
  %.i2257 = fadd fast float %.i2254, 1.000000e+00
  %.i0258 = fmul fast float %.i0255, %143
  %.i1259 = fmul fast float %.i1256, %143
  %.i2260 = fmul fast float %.i2257, %143
  br label %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"

"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit": ; preds = %164, %0
  %.025.i0 = phi float [ %.i0258, %164 ], [ %.i0243, %0 ]
  %.025.i1 = phi float [ %.i1259, %164 ], [ %.i1244, %0 ]
  %.025.i2 = phi float [ %.i2260, %164 ], [ %.i2245, %0 ]
  %.i0261 = fsub fast float -0.000000e+00, %.i0189
  %.i1262 = fsub fast float -0.000000e+00, %.i1190
  %.i2263 = fsub fast float -0.000000e+00, %.i2191
  %169 = call float @dx.op.dot3.f32(i32 55, float %FMad153, float %FMad151, float %FMad149, float %.i0261, float %.i1262, float %.i2263)  ; Dot3(ax,ay,az,bx,by,bz)
  %170 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %7)  ; CreateHandleForLib(Resource)
  %171 = call %dx.types.Handle @dx.op.createHandleForLib.struct.SamplerState(i32 160, %struct.SamplerState %4)  ; CreateHandleForLib(Resource)
  %172 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %170, %dx.types.Handle %171, float %169, float 1.000000e+00, float undef, float undef, i32 undef, i32 undef, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %173 = extractvalue %dx.types.ResRet.f32 %172, 0
  %174 = extractvalue %dx.types.ResRet.f32 %172, 1
  %.i0264 = fmul fast float %173, %.025.i0
  %.i1265 = fmul fast float %173, %.025.i1
  %.i2266 = fmul fast float %173, %.025.i2
  %.i0267 = fadd fast float %.i0264, %174
  %.i1268 = fadd fast float %.i1265, %174
  %.i2269 = fadd fast float %.i2266, %174
  %.i0270 = fmul fast float %.i0267, 0x3FD45F3060000000
  %.i1271 = fmul fast float %.i1268, 0x3FD45F3060000000
  %.i2272 = fmul fast float %.i2269, 0x3FD45F3060000000
  %FMax82 = call float @dx.op.binary.f32(i32 35, float %.i0270, float %.i1271)  ; FMax(a,b)
  %FMax81 = call float @dx.op.binary.f32(i32 35, float %FMax82, float %.i2272)  ; FMax(a,b)
  %175 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %rtreflection, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %176 = extractvalue %dx.types.CBufRet.f32 %175, 3
  %177 = fcmp fast olt float %FMax81, %176
  br i1 %177, label %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit", label %178

; <label>:178                                     ; preds = %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"
  %179 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %rtreflection, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %180 = extractvalue %dx.types.CBufRet.f32 %179, 0
  %FMax80 = call float @dx.op.binary.f32(i32 35, float 0x3F847AE140000000, float %180)  ; FMax(a,b)
  %181 = fdiv fast float 1.000000e+00, %FMax80
  %182 = fmul fast float %181, %FMax81
  %Saturate39 = call float @dx.op.unary.f32(i32 7, float %182)  ; Saturate(value)
  %183 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %rtreflection, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %184 = extractvalue %dx.types.CBufRet.i32 %183, 2
  %185 = uitofp i32 %184 to float
  %186 = fadd fast float %185, -1.000000e+00
  %187 = fmul fast float %186, %Saturate39
  %188 = fadd fast float %187, 1.000000e+00
  %Round_ne = call float @dx.op.unary.f32(i32 26, float %188)  ; Round_ne(value)
  %phitmp = fptosi float %Round_ne to i32
  br label %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit"

"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit": ; preds = %178, %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit"
  %189 = phi i32 [ %phitmp, %178 ], [ 0, %"\01?internal.getMaterialSpecular@@YA?AV?$vector@M$02@@IM@Z.exit" ]
  %IMin = call i32 @dx.op.binary.i32(i32 38, i32 %189, i32 1)  ; IMin(a,b)
  %190 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32 160, %"class.RWTexture2DArray<unsigned int>" %13)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %190, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex162, i32 %IMin, i32 65534, i32 65534, i32 65534, i32 65534, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %191 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %192 = extractvalue %dx.types.CBufRet.f32 %191, 1
  %193 = extractvalue %dx.types.CBufRet.f32 %191, 3
  %194 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %195 = extractvalue %dx.types.CBufRet.f32 %194, 1
  %196 = extractvalue %dx.types.CBufRet.f32 %194, 3
  %197 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %198 = extractvalue %dx.types.CBufRet.f32 %197, 1
  %199 = extractvalue %dx.types.CBufRet.f32 %197, 3
  %200 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 18)  ; CBufferLoadLegacy(handle,regIndex)
  %201 = extractvalue %dx.types.CBufRet.f32 %200, 0
  %202 = extractvalue %dx.types.CBufRet.f32 %200, 1
  %203 = extractvalue %dx.types.CBufRet.f32 %200, 3
  %204 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 19)  ; CBufferLoadLegacy(handle,regIndex)
  %205 = extractvalue %dx.types.CBufRet.f32 %204, 0
  %206 = extractvalue %dx.types.CBufRet.f32 %204, 1
  %207 = extractvalue %dx.types.CBufRet.f32 %204, 3
  %208 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 20)  ; CBufferLoadLegacy(handle,regIndex)
  %209 = extractvalue %dx.types.CBufRet.f32 %208, 0
  %210 = extractvalue %dx.types.CBufRet.f32 %208, 1
  %211 = extractvalue %dx.types.CBufRet.f32 %208, 3
  %212 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 21)  ; CBufferLoadLegacy(handle,regIndex)
  %213 = extractvalue %dx.types.CBufRet.f32 %212, 0
  %214 = extractvalue %dx.types.CBufRet.f32 %212, 1
  %215 = extractvalue %dx.types.CBufRet.f32 %212, 3
  %216 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %217 = extractvalue %dx.types.CBufRet.f32 %216, 0
  %218 = extractvalue %dx.types.CBufRet.f32 %216, 1
  %219 = getelementptr inbounds [3 x float], [3 x float]* %1, i32 0, i32 0
  store float %.i0183, float* %219, align 4
  %220 = getelementptr inbounds [3 x float], [3 x float]* %1, i32 0, i32 1
  store float %.i1184, float* %220, align 4
  %221 = getelementptr inbounds [3 x float], [3 x float]* %1, i32 0, i32 2
  store float %.i2185, float* %221, align 4
  %.i0277 = fmul fast float %FMad153, 4.000000e+00
  %.i1278 = fmul fast float %FMad151, 4.000000e+00
  %.i2279 = fmul fast float %FMad149, 4.000000e+00
  %Round_ne54 = call float @dx.op.unary.f32(i32 26, float %.i0277)  ; Round_ne(value)
  %Round_ne55 = call float @dx.op.unary.f32(i32 26, float %.i1278)  ; Round_ne(value)
  %Round_ne56 = call float @dx.op.unary.f32(i32 26, float %.i2279)  ; Round_ne(value)
  %222 = fmul fast float %Round_ne54, %Round_ne54
  %223 = fmul fast float %Round_ne55, %Round_ne55
  %224 = fadd fast float %223, %222
  %225 = fmul fast float %Round_ne56, %Round_ne56
  %226 = fadd fast float %224, %225
  %Sqrt53 = call float @dx.op.unary.f32(i32 24, float %226)  ; Sqrt(value)
  %.i0280 = fdiv fast float %Round_ne54, %Sqrt53
  %.i1281 = fdiv fast float %Round_ne55, %Sqrt53
  %.i2282 = fdiv fast float %Round_ne56, %Sqrt53
  %FAbs63 = call float @dx.op.unary.f32(i32 6, float %.i0280)  ; FAbs(value)
  %FAbs64 = call float @dx.op.unary.f32(i32 6, float %.i1281)  ; FAbs(value)
  %FAbs65 = call float @dx.op.unary.f32(i32 6, float %.i2282)  ; FAbs(value)
  %FMax79 = call float @dx.op.binary.f32(i32 35, float %FAbs63, float %FAbs64)  ; FMax(a,b)
  %FMax78 = call float @dx.op.binary.f32(i32 35, float %FMax79, float %FAbs65)  ; FMax(a,b)
  %227 = fcmp fast oeq float %FMax78, %FAbs64
  %iMajorAxis.i.0 = zext i1 %227 to i32
  %228 = fcmp fast oeq float %FMax78, %FAbs65
  %iMajorAxis.i.1 = select i1 %228, i32 2, i32 %iMajorAxis.i.0
  %FMad108 = call float @dx.op.tertiary.f32(i32 46, float %23, float %195, float 0.000000e+00)  ; FMad(a,b,c)
  %229 = fadd fast float %FMad108, %198
  %FMad102 = call float @dx.op.tertiary.f32(i32 46, float %23, float %196, float 0.000000e+00)  ; FMad(a,b,c)
  %230 = fadd fast float %FMad102, %199
  %231 = fdiv fast float 1.000000e+00, %230
  %.i1284 = fmul fast float %231, %229
  %232 = fdiv fast float 2.000000e+00, %218
  %FMad145 = call float @dx.op.tertiary.f32(i32 46, float %232, float %192, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad144 = call float @dx.op.tertiary.f32(i32 46, float %23, float %195, float %FMad145)  ; FMad(a,b,c)
  %233 = fadd fast float %FMad144, %198
  %FMad139 = call float @dx.op.tertiary.f32(i32 46, float %232, float %193, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad138 = call float @dx.op.tertiary.f32(i32 46, float %23, float %196, float %FMad139)  ; FMad(a,b,c)
  %234 = fadd fast float %FMad138, %199
  %235 = fdiv fast float 1.000000e+00, %234
  %.i1287 = fmul fast float %235, %233
  %236 = fsub fast float %.i1287, %.i1284
  %FAbs = call float @dx.op.unary.f32(i32 6, float %236)  ; FAbs(value)
  %237 = fmul fast float %FAbs, 6.400000e+01
  %Log = call float @dx.op.unary.f32(i32 23, float %237)  ; Log(value)
  %Round_pi = call float @dx.op.unary.f32(i32 28, float %Log)  ; Round_pi(value)
  %Exp = call float @dx.op.unary.f32(i32 21, float %Round_pi)  ; Exp(value)
  %.i0289 = fdiv fast float %.i0183, %Exp
  %.i1290 = fdiv fast float %.i1184, %Exp
  %.i2291 = fdiv fast float %.i2185, %Exp
  %Round_ni57 = call float @dx.op.unary.f32(i32 27, float %.i0289)  ; Round_ni(value)
  %Round_ni58 = call float @dx.op.unary.f32(i32 27, float %.i1290)  ; Round_ni(value)
  %Round_ni59 = call float @dx.op.unary.f32(i32 27, float %.i2291)  ; Round_ni(value)
  %.i0292 = fptosi float %Round_ni57 to i32
  %.i1293 = fptosi float %Round_ni58 to i32
  %.i2294 = fptosi float %Round_ni59 to i32
  %.i0295 = sitofp i32 %.i0292 to float
  %.i1296 = sitofp i32 %.i1293 to float
  %.i2297 = sitofp i32 %.i2294 to float
  %.i0298 = fmul fast float %.i0295, %Exp
  %.i1299 = fmul fast float %.i1296, %Exp
  %.i2300 = fmul fast float %.i2297, %Exp
  %238 = getelementptr inbounds [3 x i32], [3 x i32]* %2, i32 0, i32 0
  store i32 %.i0292, i32* %238, align 4
  %239 = getelementptr inbounds [3 x i32], [3 x i32]* %2, i32 0, i32 1
  store i32 %.i1293, i32* %239, align 4
  %240 = getelementptr inbounds [3 x i32], [3 x i32]* %2, i32 0, i32 2
  store i32 %.i2294, i32* %240, align 4
  %241 = getelementptr [3 x i32], [3 x i32]* %2, i32 0, i32 %iMajorAxis.i.1
  store i32 0, i32* %241, align 4, !tbaa !527
  %242 = load i32, i32* %238, align 4
  %243 = load i32, i32* %239, align 4
  %244 = load i32, i32* %240, align 4
  %245 = shl i32 %243, 5
  %246 = xor i32 %245, %243
  %247 = shl i32 %244, 13
  %248 = xor i32 %247, %244
  %249 = add i32 %246, %242
  %250 = add i32 %249, %248
  %251 = xor i32 %250, 61
  %252 = lshr i32 %250, 16
  %253 = xor i32 %251, %252
  %254 = mul i32 %253, 9
  %255 = lshr i32 %254, 4
  %256 = xor i32 %255, %254
  %257 = mul i32 %256, 668265261
  %258 = lshr i32 %257, 15
  %259 = xor i32 %258, %257
  %260 = uitofp i32 %259 to float
  %261 = fmul fast float %260, 0x3DF0000000000000
  %262 = fdiv fast float %237, %Exp
  %263 = fmul fast float %262, 4.000000e+00
  %264 = fadd fast float %263, -2.000000e+00
  %265 = fsub fast float %264, %261
  %Saturate36 = call float @dx.op.unary.f32(i32 7, float %265)  ; Saturate(value)
  %266 = fmul fast float %Saturate36, 5.000000e-01
  %267 = fadd fast float %266, 5.000000e-01
  %268 = fmul fast float %267, %Exp
  %269 = load float, float* %219, align 4, !tbaa !525
  %270 = fsub fast float %269, %.i0298
  %271 = fcmp fast ogt float %270, %268
  %272 = fadd fast float %.i0295, 5.000000e-01
  %273 = fmul fast float %Exp, %272
  %274 = select i1 %271, float %273, float %.i0298
  %275 = load float, float* %220, align 4, !tbaa !525
  %276 = fsub fast float %275, %.i1299
  %277 = fcmp fast ogt float %276, %268
  %278 = fadd fast float %.i1296, 5.000000e-01
  %279 = fmul fast float %Exp, %278
  %280 = select i1 %277, float %279, float %.i1299
  %281 = load float, float* %221, align 4, !tbaa !525
  %282 = fsub fast float %281, %.i2300
  %283 = fcmp fast ogt float %282, %268
  %284 = fadd fast float %.i2297, 5.000000e-01
  %285 = fmul fast float %Exp, %284
  %286 = select i1 %283, float %285, float %.i2300
  %287 = getelementptr inbounds [3 x float], [3 x float]* %3, i32 0, i32 0
  store float %274, float* %287, align 4
  %288 = getelementptr inbounds [3 x float], [3 x float]* %3, i32 0, i32 1
  store float %280, float* %288, align 4
  %289 = getelementptr inbounds [3 x float], [3 x float]* %3, i32 0, i32 2
  store float %286, float* %289, align 4
  %290 = getelementptr [3 x float], [3 x float]* %3, i32 0, i32 %iMajorAxis.i.1
  store float 0.000000e+00, float* %290, align 4, !tbaa !525
  %291 = load float, float* %287, align 4
  %292 = load float, float* %288, align 4
  %293 = load float, float* %289, align 4
  %.i0373 = bitcast float %291 to i32
  %.i1374 = bitcast float %292 to i32
  %.i2375 = bitcast float %293 to i32
  %294 = shl i32 %.i1374, 5
  %295 = xor i32 %294, %.i1374
  %296 = shl i32 %.i2375, 13
  %297 = xor i32 %296, %.i2375
  %298 = add i32 %295, %.i0373
  %299 = add i32 %298, %297
  %300 = xor i32 %299, 61
  %301 = lshr i32 %299, 16
  %302 = xor i32 %300, %301
  %303 = mul i32 %302, 9
  %304 = lshr i32 %303, 4
  %305 = xor i32 %304, %303
  %306 = mul i32 %305, 668265261
  %307 = lshr i32 %306, 15
  %308 = xor i32 %307, %306
  %309 = lshr i32 %308, 16
  %.i0376 = fsub fast float %269, %274
  %.i1377 = fsub fast float %275, %280
  %.i2378 = fsub fast float %281, %286
  %310 = call float @dx.op.dot3.f32(i32 55, float %.i0376, float %.i1377, float %.i2378, float %.i0280, float %.i1281, float %.i2282)  ; Dot3(ax,ay,az,bx,by,bz)
  %.i0379 = fmul fast float %310, %.i0280
  %.i1380 = fmul fast float %310, %.i1281
  %.i2381 = fmul fast float %310, %.i2282
  %.i0382 = fadd fast float %.i0379, %274
  %.i1383 = fadd fast float %.i1380, %280
  %.i2384 = fadd fast float %.i2381, %286
  %311 = fmul fast float %.i0382, %201
  %FMad136 = call float @dx.op.tertiary.f32(i32 46, float %.i1383, float %205, float %311)  ; FMad(a,b,c)
  %FMad135 = call float @dx.op.tertiary.f32(i32 46, float %.i2384, float %209, float %FMad136)  ; FMad(a,b,c)
  %312 = fadd fast float %FMad135, %213
  %313 = fmul fast float %.i0382, %202
  %FMad133 = call float @dx.op.tertiary.f32(i32 46, float %.i1383, float %206, float %313)  ; FMad(a,b,c)
  %FMad132 = call float @dx.op.tertiary.f32(i32 46, float %.i2384, float %210, float %FMad133)  ; FMad(a,b,c)
  %314 = fadd fast float %FMad132, %214
  %315 = fmul fast float %.i0382, %203
  %FMad127 = call float @dx.op.tertiary.f32(i32 46, float %.i1383, float %207, float %315)  ; FMad(a,b,c)
  %FMad126 = call float @dx.op.tertiary.f32(i32 46, float %.i2384, float %211, float %FMad127)  ; FMad(a,b,c)
  %316 = fadd fast float %FMad126, %215
  %317 = fdiv fast float 1.000000e+00, %316
  %.i0385 = fmul fast float %312, 5.000000e-01
  %.i0388 = fmul fast float %.i0385, %317
  %.i1386 = fmul fast float %314, 5.000000e-01
  %.i1389 = fmul fast float %.i1386, %317
  %.i0390 = fadd fast float %.i0388, 5.000000e-01
  %.i1391489 = fsub fast float 5.000000e-01, %.i1389
  %.i0392 = fmul fast float %.i0390, %217
  %.i1393 = fmul fast float %.i1391489, %218
  %.i0275.neg = fsub fast float -5.000000e-01, %17
  %.i0394 = fadd fast float %.i0275.neg, %.i0392
  %.i1276.neg = fsub fast float -5.000000e-01, %18
  %.i1395 = fadd fast float %.i1276.neg, %.i1393
  %Round_ni = call float @dx.op.unary.f32(i32 27, float %.i0394)  ; Round_ni(value)
  %Round_ni51 = call float @dx.op.unary.f32(i32 27, float %.i1395)  ; Round_ni(value)
  %.i0396 = fptosi float %Round_ni to i32
  %.i1397 = fptosi float %Round_ni51 to i32
  %.i0398 = add i32 %.i0396, %309
  %.i1399 = add i32 %.i1397, %308
  %.i0400 = and i32 %.i0398, 255
  %.i1401 = and i32 %.i1399, 255
  %318 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %6)  ; CreateHandleForLib(Resource)
  %TextureLoad158 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %318, i32 0, i32 %.i0400, i32 %.i1401, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %.i2308 = extractvalue %dx.types.ResRet.f32 %TextureLoad158, 2
  %319 = icmp sgt i32 %IMin, 0
  br i1 %319, label %.lr.ph31.preheader, label %._crit_edge.32

.lr.ph31:                                         ; preds = %.lr.ph31.preheader, %._crit_edge
  %reflectionRayIndex.030 = phi i32 [ %344, %._crit_edge ], [ 0, %.lr.ph31.preheader ]
  %320 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)  ; CBufferLoadLegacy(handle,regIndex)
  %321 = extractvalue %dx.types.CBufRet.i32 %320, 1
  %322 = mul i32 %321, %IMin
  %323 = add i32 %322, %reflectionRayIndex.030
  %324 = uitofp i32 %323 to float
  %.i2303 = fmul fast float %324, 0x3FF9E377A0000000
  %.i2309 = fadd fast float %.i2303, %.i2308
  %Frc71 = call float @dx.op.unary.f32(i32 22, float %.i2309)  ; Frc(value)
  br i1 %364, label %.lr.ph.preheader, label %._crit_edge

.lr.ph.preheader:                                 ; preds = %.lr.ph31
  br label %.lr.ph

.lr.ph:                                           ; preds = %.lr.ph.preheader
  br label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph, %.lr.ph31
  %325 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %326 = extractvalue %dx.types.CBufRet.f32 %325, 0
  %327 = extractvalue %dx.types.CBufRet.f32 %325, 1
  %328 = extractvalue %dx.types.CBufRet.f32 %325, 2
  %329 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %330 = extractvalue %dx.types.CBufRet.f32 %329, 0
  %331 = extractvalue %dx.types.CBufRet.f32 %329, 1
  %332 = extractvalue %dx.types.CBufRet.f32 %329, 2
  %333 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %334 = extractvalue %dx.types.CBufRet.f32 %333, 0
  %335 = extractvalue %dx.types.CBufRet.f32 %333, 1
  %336 = extractvalue %dx.types.CBufRet.f32 %333, 2
  %337 = fmul fast float %326, %.i0337
  %FMad100 = call float @dx.op.tertiary.f32(i32 46, float %.i1338, float %330, float %337)  ; FMad(a,b,c)
  %FMad99 = call float @dx.op.tertiary.f32(i32 46, float %.i2339, float %334, float %FMad100)  ; FMad(a,b,c)
  %338 = fmul fast float %327, %.i0337
  %FMad98 = call float @dx.op.tertiary.f32(i32 46, float %.i1338, float %331, float %338)  ; FMad(a,b,c)
  %FMad97 = call float @dx.op.tertiary.f32(i32 46, float %.i2339, float %335, float %FMad98)  ; FMad(a,b,c)
  %339 = fmul fast float %328, %.i0337
  %FMad96 = call float @dx.op.tertiary.f32(i32 46, float %.i1338, float %332, float %339)  ; FMad(a,b,c)
  %FMad95 = call float @dx.op.tertiary.f32(i32 46, float %.i2339, float %336, float %FMad96)  ; FMad(a,b,c)
  %340 = fadd fast float %Frc71, 5.000000e-01
  %341 = fmul fast float %340, 0x4066666660000000
  %342 = fadd fast float %341, 0x4066666660000000
  store i32 %reflectionRayIndex.030, i32* %350, align 8
  %343 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %5)  ; CreateHandleForLib(Resource)
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %343, i32 0, i32 1, i32 0, i32 2, i32 0, float %352, float %353, float %354, float %355, float %FMad99, float %FMad97, float %FMad95, float %342, %struct.HitData* nonnull %16)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  %344 = add nuw nsw i32 %reflectionRayIndex.030, 1
  %exitcond = icmp eq i32 %344, %IMin
  br i1 %exitcond, label %._crit_edge.32.loopexit, label %.lr.ph31

._crit_edge.32.loopexit:                          ; preds = %._crit_edge
  %.lcssa = phi float [ %342, %._crit_edge ]
  %FMad95.lcssa = phi float [ %FMad95, %._crit_edge ]
  %FMad97.lcssa = phi float [ %FMad97, %._crit_edge ]
  %FMad99.lcssa = phi float [ %FMad99, %._crit_edge ]
  %345 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 3
  %346 = insertelement <3 x float> undef, float %FMad99.lcssa, i64 0
  %347 = insertelement <3 x float> %346, float %FMad97.lcssa, i64 1
  %348 = insertelement <3 x float> %347, float %FMad95.lcssa, i64 2
  store <3 x float> %348, <3 x float>* %349, align 4
  store float %.lcssa, float* %345, align 4
  br label %._crit_edge.32

._crit_edge.32:                                   ; preds = %._crit_edge.32.loopexit, %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit"
  ret void

.lr.ph31.preheader:                               ; preds = %"\01?internal.getReflectionRayCount@@YAHV?$vector@M$02@@M@Z.exit"
  %.i0313 = fsub fast float -0.000000e+00, %.i0176
  %.i1314 = fsub fast float -0.000000e+00, %.i1177
  %.i2315 = fsub fast float -0.000000e+00, %.i2178
  %349 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 2
  %350 = getelementptr inbounds %struct.HitData, %struct.HitData* %16, i32 0, i32 0
  %351 = load <3 x float>, <3 x float>* %140, align 4
  %352 = extractelement <3 x float> %351, i64 0
  %353 = extractelement <3 x float> %351, i64 1
  %354 = extractelement <3 x float> %351, i64 2
  %355 = load float, float* %141, align 4
  %356 = call float @dx.op.dot3.f32(i32 55, float %.i0313, float %.i1314, float %.i2315, float %.i0222, float %.i1223, float %.i2224)  ; Dot3(ax,ay,az,bx,by,bz)
  %357 = fmul fast float %356, 2.000000e+00
  %.i0331 = fmul fast float %357, %.i0222
  %.i1332 = fmul fast float %357, %.i1223
  %.i2333 = fmul fast float %.i2224, %357
  %.i0334 = fadd fast float %.i0331, %.i0176
  %.i1335 = fadd fast float %.i1332, %.i1177
  %.i2336 = fadd fast float %.i2333, %.i2178
  %358 = fmul fast float %.i0334, %.i0334
  %359 = fmul fast float %.i1335, %.i1335
  %360 = fadd fast float %358, %359
  %361 = fmul fast float %.i2336, %.i2336
  %362 = fadd fast float %360, %361
  %Sqrt75 = call float @dx.op.unary.f32(i32 24, float %362)  ; Sqrt(value)
  %.i0337 = fdiv fast float %.i0334, %Sqrt75
  %.i1338 = fdiv fast float %.i1335, %Sqrt75
  %.i2339 = fdiv fast float %.i2336, %Sqrt75
  %363 = call float @dx.op.dot3.f32(i32 55, float %.i0337, float %.i1338, float %.i2339, float %.i0222, float %.i1223, float %.i2224)  ; Dot3(ax,ay,az,bx,by,bz)
  %364 = fcmp fast olt float %363, 0x3F747AE140000000
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
!dx.typeAnnotations = !{!25, !498}
!dx.entryPoints = !{!508, !511, !513, !515, !517, !519, !520, !521}

!0 = !{!"dxc 1.2"}
!1 = !{i32 1, i32 3}
!2 = !{!"lib", i32 6, i32 3}
!3 = !{!4, !16, !20, !23}
!4 = !{!5, !7, !8, !9, !10, !12, !13, !14}
!5 = !{i32 0, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer1", i32 0, i32 0, i32 1, i32 2, i32 0, !6}
!6 = !{i32 0, i32 9}
!7 = !{i32 1, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer2@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer2", i32 0, i32 1, i32 1, i32 2, i32 0, !6}
!8 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tLinearDepth", i32 0, i32 2, i32 1, i32 2, i32 0, !6}
!9 = !{i32 3, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tClipDepth", i32 0, i32 3, i32 1, i32 2, i32 0, !6}
!10 = !{i32 4, %"class.StructuredBuffer<MaterialDataPart1>"* @"\01?g_sbMaterialDataPart1@@3V?$StructuredBuffer@UMaterialDataPart1@@@@A", !"g_sbMaterialDataPart1", i32 0, i32 4, i32 1, i32 12, i32 0, !11}
!11 = !{i32 1, i32 8}
!12 = !{i32 5, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tEnvBRDF@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tEnvBRDF", i32 0, i32 5, i32 1, i32 2, i32 0, !6}
!13 = !{i32 6, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tStaticBlueNoiseRGBA_0", i32 0, i32 6, i32 1, i32 2, i32 0, !6}
!14 = !{i32 7, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", !"g_rtScene", i32 0, i32 7, i32 1, i32 16, i32 0, !15}
!15 = !{i32 0, i32 4}
!16 = !{!17, !19}
!17 = !{i32 0, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", !"g_rwtMaterialId", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !18}
!18 = !{i32 0, i32 5}
!19 = !{i32 1, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", !"g_rwtShadow", i32 0, i32 1, i32 1, i32 7, i1 false, i1 false, i1 false, !18}
!20 = !{!21, !22}
!21 = !{i32 0, %sys_constants* @sys_constants, !"sys_constants", i32 0, i32 0, i32 1, i32 1128, null}
!22 = !{i32 1, %rtreflection* @rtreflection, !"rtreflection", i32 0, i32 1, i32 1, i32 24, null}
!23 = !{!24}
!24 = !{i32 0, %struct.SamplerState* @"\01?g_sLinearClamp@@3USamplerState@@A", !"g_sLinearClamp", i32 1, i32 6, i32 1, i32 0, null}
!25 = !{i32 0, %"class.Texture2D<vector<float, 4> >" undef, !26, %"class.Texture2D<vector<float, 4> >::mips_type" undef, !29, %struct.GBufferFormat undef, !31, %struct.SamplerState undef, !34, %"class.Texture2D<float>" undef, !36, %"class.Texture2D<float>::mips_type" undef, !29, %"class.Texture2D<unsigned int>" undef, !38, %"class.Texture2D<unsigned int>::mips_type" undef, !29, %"class.Texture2DMS<float, 0>" undef, !40, %"class.Texture2DMS<float, 0>::sample_type" undef, !29, %"class.StructuredBuffer<vector<unsigned int, 4> >" undef, !42, %"class.StructuredBuffer<MaterialDataPart1>" undef, !43, %struct.MaterialDataPart1 undef, !45, %"class.StructuredBuffer<MaterialDataPart3>" undef, !48, %struct.MaterialDataPart3 undef, !49, %"class.StructuredBuffer<MaterialBindingData>" undef, !93, %struct.MaterialBindingData undef, !94, %struct.order2_sh undef, !96, %"class.RWTexture3D<vector<float, 4> >" undef, !101, %"class.Texture3D<vector<float, 4> >" undef, !26, %"class.Texture3D<vector<float, 4> >::mips_type" undef, !29, %struct.LightVolumeData undef, !102, %"class.TextureCube<vector<float, 4> >" undef, !101, %"class.StructuredBuffer<DeferredLightPoint>" undef, !105, %struct.DeferredLightPoint undef, !106, %"class.StructuredBuffer<DeferredLightPointClipping>" undef, !121, %struct.DeferredLightPointClipping undef, !122, %"class.StructuredBuffer<DeferredLightPointBVH>" undef, !124, %struct.DeferredLightPointBVH undef, !125, %"class.RWTexture2D<unsigned int>" undef, !132, %"class.StructuredBuffer<DeferredLightSpot>" undef, !133, %struct.DeferredLightSpot undef, !134, %"class.StructuredBuffer<DeferredLightSpotClipping>" undef, !159, %struct.DeferredLightSpotClipping undef, !160, %"class.StructuredBuffer<DeferredLightSpotBVH>" undef, !124, %struct.DeferredLightSpotBVH undef, !125, %"class.StructuredBuffer<DeferredLightSun>" undef, !165, %struct.DeferredLightSun undef, !166, %"class.StructuredBuffer<VolumeSamplingInfo>" undef, !180, %struct.VolumeSamplingInfo undef, !181, %"class.StructuredBuffer<unsigned int>" undef, !132, %"class.StructuredBuffer<InternalNode>" undef, !121, %struct.InternalNode undef, !191, %"class.Texture3D<vector<float, 3> >" undef, !196, %"class.Texture3D<vector<float, 3> >::mips_type" undef, !29, %struct.VolumeCullingInCB undef, !198, %struct.VolumeTopLevelInfo undef, !203, %struct.DeferredLightSpecularBRDF undef, !207, %struct.DeferredLightGenericSurface undef, !210, %struct.DeferredLightTransparentBRDF undef, !214, %struct.DeferredLightIntensity undef, !217, %struct.DeferredLightVolumeResult undef, !220, %struct.DeferredLightPointSurface undef, !222, %struct.DeferredLightGBufferData undef, !223, %struct.CellInfo undef, !230, %"class.StructuredBuffer<EnvironmentMapDynamicInfo>" undef, !93, %struct.EnvironmentMapDynamicInfo undef, !256, %"class.StructuredBuffer<EnvironmentMapReference>" undef, !93, %struct.EnvironmentMapReference undef, !258, %"class.StructuredBuffer<CellInfo>" undef, !260, %"class.StructuredBuffer<Node>" undef, !261, %struct.Node undef, !262, %struct.ChildMask undef, !266, %struct.NodePointer undef, !268, %"class.StructuredBuffer<IrradianceProbe>" undef, !270, %struct.IrradianceProbe undef, !271, %"class.StructuredBuffer<TransportProbe>" undef, !281, %struct.TransportProbe undef, !282, %"class.StructuredBuffer<vector<float, 4> >" undef, !101, %"class.StructuredBuffer<ProbeRef>" undef, !43, %struct.ProbeRef undef, !284, %"class.StructuredBuffer<EnvironmentMapInfo>" undef, !287, %struct.EnvironmentMapInfo undef, !288, %struct.VoxelInfo undef, !294, %struct.LeafInfo undef, !300, %"class.StructuredBuffer<PerInstanceLBData>" undef, !121, %struct.PerInstanceLBData undef, !308, %"class.StructuredBuffer<RenderInstanceData>" undef, !312, %struct.RenderInstanceData undef, !313, %struct.ByteAddressBuffer undef, !34, %struct.RaytracingHitRootConstants undef, !321, %struct.RaytracingAccelerationStructure undef, !34, %struct.IntersectionAttributes undef, !332, %struct.HitData undef, !334, %"class.RWTexture2DArray<unsigned int>" undef, !132, %"class.RWTexture2DArray<vector<float, 4> >" undef, !101, %struct.RayDesc undef, !336, %debug_general undef, !341, %sys_constants undef, !364, %shadow_general undef, !397, %mid_translucency undef, !402, %mid_general undef, !94, %deferredlight_constants undef, !404, %env_general undef, !440, %atmosphere_general undef, !443, %illuminationvolume undef, !464, %ManualUpdateCB_ActiveVolumeTopLevelInfoArray undef, !471, %ManualUpdateCB_DebugVolumeTopLevelInfoArray undef, !473, %WorldGridInfo undef, !475, %EnvironmentMapAtlas undef, !479, %vertex_binding undef, !485, %g_cbRaytracingHit undef, !489, %rtreflection undef, !491}
!26 = !{i32 20, !27, !28}
!27 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 9}
!28 = !{i32 6, !"mips", i32 3, i32 16}
!29 = !{i32 4, !30}
!30 = !{i32 6, !"handle", i32 3, i32 0, i32 7, i32 5}
!31 = !{i32 32, !32, !33}
!32 = !{i32 6, !"vTarget1", i32 3, i32 0, i32 4, !"SV_Target0", i32 7, i32 9}
!33 = !{i32 6, !"vTarget2", i32 3, i32 16, i32 4, !"SV_Target1", i32 7, i32 9}
!34 = !{i32 4, !35}
!35 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 4}
!36 = !{i32 8, !27, !37}
!37 = !{i32 6, !"mips", i32 3, i32 4}
!38 = !{i32 8, !39, !37}
!39 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 5}
!40 = !{i32 8, !27, !41}
!41 = !{i32 6, !"sample", i32 3, i32 4}
!42 = !{i32 16, !39}
!43 = !{i32 8, !44}
!44 = !{i32 6, !"h", i32 3, i32 0}
!45 = !{i32 8, !46, !47}
!46 = !{i32 6, !"vSpecularColor_uBRDF", i32 3, i32 0, i32 7, i32 5}
!47 = !{i32 6, !"uScatterIndex_vTranslucencyDepthRange", i32 3, i32 4, i32 7, i32 5}
!48 = !{i32 352, !44}
!49 = !{i32 352, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !87, !88, !89, !90, !91, !92}
!50 = !{i32 6, !"fClothDiffScatterAmount", i32 3, i32 0, i32 7, i32 9}
!51 = !{i32 6, !"fClothFuzzWrap", i32 3, i32 4, i32 7, i32 9}
!52 = !{i32 6, !"vClothScatterColor", i32 3, i32 16, i32 7, i32 9}
!53 = !{i32 6, !"fHairSmoothness", i32 3, i32 28, i32 7, i32 9}
!54 = !{i32 6, !"vSpecularColor2", i32 3, i32 32, i32 7, i32 9}
!55 = !{i32 6, !"fHairSpecularShift", i32 3, i32 44, i32 7, i32 9}
!56 = !{i32 6, !"fHairDiffuseWrap", i32 3, i32 48, i32 7, i32 9}
!57 = !{i32 6, !"fPAD1", i32 3, i32 52, i32 7, i32 9}
!58 = !{i32 6, !"fEmissionIntensity", i32 3, i32 56, i32 7, i32 9}
!59 = !{i32 6, !"vColorMultiplier", i32 3, i32 64, i32 7, i32 9}
!60 = !{i32 6, !"vSmoothnessRange", i32 3, i32 80, i32 7, i32 9}
!61 = !{i32 6, !"vShadowSoftnessRange", i32 3, i32 88, i32 7, i32 9}
!62 = !{i32 6, !"vScatterWidthRange", i32 3, i32 96, i32 7, i32 9}
!63 = !{i32 6, !"fHairDepthOffsetAmount", i32 3, i32 104, i32 7, i32 9}
!64 = !{i32 6, !"fHairNormalAmount", i32 3, i32 108, i32 7, i32 9}
!65 = !{i32 6, !"vEmissionMultiplier", i32 3, i32 112, i32 7, i32 9}
!66 = !{i32 6, !"vColorMultiplier2", i32 3, i32 128, i32 7, i32 9}
!67 = !{i32 6, !"vBaseDetailUVScale", i32 3, i32 144, i32 7, i32 9}
!68 = !{i32 6, !"vSmoothnessRange2", i32 3, i32 152, i32 7, i32 9}
!69 = !{i32 6, !"vBlendDetailUVScale", i32 3, i32 160, i32 7, i32 9}
!70 = !{i32 6, !"fBaseDetailAmount", i32 3, i32 168, i32 7, i32 9}
!71 = !{i32 6, !"fBlendUVMultiplier", i32 3, i32 172, i32 7, i32 9}
!72 = !{i32 6, !"fBlendDetailAmount", i32 3, i32 176, i32 7, i32 9}
!73 = !{i32 6, !"fFuzzUVMultiplier", i32 3, i32 180, i32 7, i32 9}
!74 = !{i32 6, !"fPupilDilation", i32 3, i32 184, i32 7, i32 9}
!75 = !{i32 6, !"fPupilOuterRadius", i32 3, i32 188, i32 7, i32 9}
!76 = !{i32 6, !"vIntensity", i32 3, i32 192, i32 7, i32 9}
!77 = !{i32 6, !"vWrinkleWeights0", i32 3, i32 208, i32 7, i32 9}
!78 = !{i32 6, !"vWrinkleWeights1", i32 3, i32 224, i32 7, i32 9}
!79 = !{i32 6, !"vWrinkleWeights2", i32 3, i32 240, i32 7, i32 9}
!80 = !{i32 6, !"vWrinkleWeights3", i32 3, i32 256, i32 7, i32 9}
!81 = !{i32 6, !"vWrinkleWeights4", i32 3, i32 272, i32 7, i32 9}
!82 = !{i32 6, !"vWrinkleWeights5", i32 3, i32 288, i32 7, i32 9}
!83 = !{i32 6, !"uDisableBackfaceFlip", i32 3, i32 304, i32 7, i32 5}
!84 = !{i32 6, !"fEdgeSharpness", i32 3, i32 308, i32 7, i32 9}
!85 = !{i32 6, !"fTrunkBendFactor", i32 3, i32 312, i32 7, i32 9}
!86 = !{i32 6, !"fTrunkPivotOffset", i32 3, i32 316, i32 7, i32 9}
!87 = !{i32 6, !"vColorRgbMultiplier", i32 3, i32 320, i32 7, i32 9}
!88 = !{i32 6, !"fWindFactor", i32 3, i32 332, i32 7, i32 9}
!89 = !{i32 6, !"fDecalMaterialBlendFactor", i32 3, i32 336, i32 7, i32 9}
!90 = !{i32 6, !"fDecalAlbedoBlendFactor", i32 3, i32 340, i32 7, i32 9}
!91 = !{i32 6, !"uStableHash", i32 3, i32 344, i32 7, i32 5}
!92 = !{i32 6, !"pad", i32 3, i32 348, i32 7, i32 5}
!93 = !{i32 4, !44}
!94 = !{i32 4, !95}
!95 = !{i32 6, !"g_uMaterialID", i32 3, i32 0, i32 7, i32 5}
!96 = !{i32 60, !97, !98, !99, !100}
!97 = !{i32 6, !"c0", i32 3, i32 0, i32 7, i32 9}
!98 = !{i32 6, !"c11", i32 3, i32 16, i32 7, i32 9}
!99 = !{i32 6, !"c10", i32 3, i32 32, i32 7, i32 9}
!100 = !{i32 6, !"c1m1", i32 3, i32 48, i32 7, i32 9}
!101 = !{i32 16, !27}
!102 = !{i32 32, !103, !104}
!103 = !{i32 6, !"value0", i32 3, i32 0, i32 7, i32 9}
!104 = !{i32 6, !"value1", i32 3, i32 16, i32 7, i32 9}
!105 = !{i32 160, !44}
!106 = !{i32 160, !107, !108, !109, !110, !111, !112, !113, !114, !116, !117, !118, !119, !120}
!107 = !{i32 6, !"vPositionInView", i32 3, i32 0, i32 7, i32 9}
!108 = !{i32 6, !"fShadowMapShrink", i32 3, i32 12, i32 7, i32 9}
!109 = !{i32 6, !"vColor", i32 3, i32 16, i32 7, i32 9}
!110 = !{i32 6, !"fClipRange", i32 3, i32 28, i32 7, i32 9}
!111 = !{i32 6, !"vFalloff", i32 3, i32 32, i32 7, i32 9}
!112 = !{i32 6, !"vDirectionInView", i32 3, i32 48, i32 7, i32 9}
!113 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 60, i32 7, i32 9}
!114 = !{i32 6, !"mViewToShadowClip", i32 2, !115, i32 3, i32 64, i32 7, i32 9}
!115 = !{i32 4, i32 4, i32 2}
!116 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 128, i32 7, i32 9}
!117 = !{i32 6, !"fRadius", i32 3, i32 144, i32 7, i32 9}
!118 = !{i32 6, !"fBoundsRadius", i32 3, i32 148, i32 7, i32 9}
!119 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 152, i32 7, i32 9}
!120 = !{i32 6, !"uTechniqueProperties", i32 3, i32 156, i32 7, i32 5}
!121 = !{i32 16, !44}
!122 = !{i32 16, !107, !123}
!123 = !{i32 6, !"fBoundsRadius", i32 3, i32 12, i32 7, i32 9}
!124 = !{i32 44, !44}
!125 = !{i32 44, !126, !127, !128, !129, !130, !131}
!126 = !{i32 6, !"vMinInView", i32 3, i32 0, i32 7, i32 9}
!127 = !{i32 6, !"vMaxInView", i32 3, i32 16, i32 7, i32 9}
!128 = !{i32 6, !"uChildLHS", i32 3, i32 28, i32 7, i32 5}
!129 = !{i32 6, !"uChildRHS", i32 3, i32 32, i32 7, i32 5}
!130 = !{i32 6, !"uLightIndex", i32 3, i32 36, i32 7, i32 5}
!131 = !{i32 6, !"uCount", i32 3, i32 40, i32 7, i32 5}
!132 = !{i32 4, !39}
!133 = !{i32 336, !44}
!134 = !{i32 336, !107, !135, !136, !137, !138, !139, !140, !141, !142, !143, !144, !145, !146, !147, !148, !150, !151, !152, !153, !154, !155, !156, !157, !158}
!135 = !{i32 6, !"fNearDistance", i32 3, i32 12, i32 7, i32 9}
!136 = !{i32 6, !"vWedPositionInView", i32 3, i32 16, i32 7, i32 9}
!137 = !{i32 6, !"fFarDistance", i32 3, i32 28, i32 7, i32 9}
!138 = !{i32 6, !"vDirectionInView", i32 3, i32 32, i32 7, i32 9}
!139 = !{i32 6, !"fFarWidth", i32 3, i32 44, i32 7, i32 9}
!140 = !{i32 6, !"vColor", i32 3, i32 48, i32 7, i32 9}
!141 = !{i32 6, !"fClipRange", i32 3, i32 60, i32 7, i32 9}
!142 = !{i32 6, !"vFalloff", i32 3, i32 64, i32 7, i32 9}
!143 = !{i32 6, !"fRadius", i32 3, i32 72, i32 7, i32 9}
!144 = !{i32 6, !"fSinConeAnglePerTwo_sq", i32 3, i32 76, i32 7, i32 9}
!145 = !{i32 6, !"mViewToProjectionClip", i32 2, !115, i32 3, i32 80, i32 7, i32 9}
!146 = !{i32 6, !"mCameraPositionInCone", i32 3, i32 144, i32 7, i32 9}
!147 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 156, i32 7, i32 9}
!148 = !{i32 6, !"mViewToCone", i32 2, !149, i32 3, i32 160, i32 7, i32 9}
!149 = !{i32 3, i32 3, i32 2}
!150 = !{i32 6, !"fShadowPCFSize", i32 3, i32 204, i32 7, i32 9}
!151 = !{i32 6, !"fShadowLOD", i32 3, i32 208, i32 7, i32 9}
!152 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 212, i32 7, i32 9}
!153 = !{i32 6, !"mConeToView", i32 3, i32 224, i32 7, i32 9}
!154 = !{i32 6, !"vProjectionAtlasOffsetScale", i32 3, i32 272, i32 7, i32 9}
!155 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 288, i32 7, i32 9}
!156 = !{i32 6, !"vDynamicShadowAtlasOffsetScale", i32 3, i32 304, i32 7, i32 9}
!157 = !{i32 6, !"uTechniqueProperties", i32 3, i32 320, i32 7, i32 5}
!158 = !{i32 6, !"padTo16bytes", i32 3, i32 324, i32 7, i32 5}
!159 = !{i32 48, !44}
!160 = !{i32 48, !107, !135, !161, !137, !162, !163, !164}
!161 = !{i32 6, !"vDirectionInView", i32 3, i32 16, i32 7, i32 9}
!162 = !{i32 6, !"fFarWidth", i32 3, i32 32, i32 7, i32 9}
!163 = !{i32 6, !"uTechniqueProperties", i32 3, i32 36, i32 7, i32 5}
!164 = !{i32 6, !"padTo16bytes", i32 3, i32 40, i32 7, i32 5}
!165 = !{i32 708, !44}
!166 = !{i32 708, !167, !168, !109, !169, !170, !171, !172, !173, !174, !175, !176, !177, !178, !179}
!167 = !{i32 6, !"vDirectionInView", i32 3, i32 0, i32 7, i32 9}
!168 = !{i32 6, !"pad1", i32 3, i32 12, i32 7, i32 4}
!169 = !{i32 6, !"pad2", i32 3, i32 28, i32 7, i32 4}
!170 = !{i32 6, !"mViewToProjectionClip", i32 2, !115, i32 3, i32 32, i32 7, i32 9}
!171 = !{i32 6, !"fProjectionMapTransparency", i32 3, i32 96, i32 7, i32 9}
!172 = !{i32 6, !"fRadius", i32 3, i32 100, i32 7, i32 9}
!173 = !{i32 6, !"uTechniqueProperties", i32 3, i32 104, i32 7, i32 5}
!174 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 108, i32 7, i32 9}
!175 = !{i32 6, !"uCascadeCount", i32 3, i32 112, i32 7, i32 5}
!176 = !{i32 6, !"mCascadeViewToShadowClip", i32 2, !115, i32 3, i32 128, i32 7, i32 9}
!177 = !{i32 6, !"vCascadeRange", i32 3, i32 512, i32 7, i32 9}
!178 = !{i32 6, !"fCascadePCFWidth", i32 3, i32 608, i32 7, i32 9}
!179 = !{i32 6, !"pad3", i32 3, i32 704, i32 7, i32 4}
!180 = !{i32 92, !44}
!181 = !{i32 92, !182, !183, !184, !185, !186, !187, !188, !189, !190}
!182 = !{i32 6, !"mWorldToLocalRotationMatrix", i32 2, !149, i32 3, i32 0, i32 7, i32 9}
!183 = !{i32 6, !"vOneOverTopLevelFadeLength", i32 3, i32 48, i32 7, i32 9}
!184 = !{i32 6, !"uTopLevelNodeIndicesOffset", i32 3, i32 60, i32 7, i32 5}
!185 = !{i32 6, !"uInternalNodeIndexOffset", i32 3, i32 64, i32 7, i32 5}
!186 = !{i32 6, !"uBrickIndexOffset", i32 3, i32 68, i32 7, i32 5}
!187 = !{i32 6, !"fTopLevelTexelSizeInWorld", i32 3, i32 72, i32 7, i32 9}
!188 = !{i32 6, !"fWeightMultiplier", i32 3, i32 76, i32 7, i32 9}
!189 = !{i32 6, !"uAdditive", i32 3, i32 80, i32 7, i32 5}
!190 = !{i32 6, !"fpadTo16Bytes", i32 3, i32 84, i32 7, i32 9}
!191 = !{i32 16, !192, !193, !194, !195}
!192 = !{i32 6, !"uChildMask", i32 3, i32 0, i32 7, i32 5}
!193 = !{i32 6, !"uLeafChildMask", i32 3, i32 4, i32 7, i32 5}
!194 = !{i32 6, !"uChildBlockOffset", i32 3, i32 8, i32 7, i32 5}
!195 = !{i32 6, !"uLeafChildBlockOffset", i32 3, i32 12, i32 7, i32 5}
!196 = !{i32 16, !27, !197}
!197 = !{i32 6, !"mips", i32 3, i32 12}
!198 = !{i32 32, !199, !200, !201, !202}
!199 = !{i32 6, !"vAABBMinInView", i32 3, i32 0, i32 7, i32 9}
!200 = !{i32 6, !"fPad0", i32 3, i32 12, i32 7, i32 9}
!201 = !{i32 6, !"vAABBMaxInView", i32 3, i32 16, i32 7, i32 9}
!202 = !{i32 6, !"fPad1", i32 3, i32 28, i32 7, i32 9}
!203 = !{i32 64, !204, !205, !206}
!204 = !{i32 6, !"mWorldToTopLevelMatrixDecomp", i32 3, i32 0, i32 7, i32 9}
!205 = !{i32 6, !"vTopLevelSize", i32 3, i32 48, i32 7, i32 5}
!206 = !{i32 6, !"fPad0", i32 3, i32 60, i32 7, i32 9}
!207 = !{i32 28, !208, !209}
!208 = !{i32 6, !"vResultDiffuse", i32 3, i32 0, i32 7, i32 9}
!209 = !{i32 6, !"vResultSpecular", i32 3, i32 16, i32 7, i32 9}
!210 = !{i32 36, !107, !211, !212, !213}
!211 = !{i32 6, !"vNormalInView", i32 3, i32 16, i32 7, i32 9}
!212 = !{i32 6, !"fSmoothness", i32 3, i32 28, i32 7, i32 9}
!213 = !{i32 6, !"fSpecularAlbedoIntensity", i32 3, i32 32, i32 7, i32 9}
!214 = !{i32 44, !208, !215, !216}
!215 = !{i32 6, !"vResultDiffuseTransmission", i32 3, i32 16, i32 7, i32 9}
!216 = !{i32 6, !"vResultSpecular", i32 3, i32 32, i32 7, i32 9}
!217 = !{i32 28, !218, !219}
!218 = !{i32 6, !"vIntensityDiffuse", i32 3, i32 0, i32 7, i32 9}
!219 = !{i32 6, !"vIntensitySpecular", i32 3, i32 16, i32 7, i32 9}
!220 = !{i32 32, !221}
!221 = !{i32 6, !"d", i32 3, i32 0}
!222 = !{i32 12, !107}
!223 = !{i32 68, !224, !225, !226, !227, !228, !229}
!224 = !{i32 6, !"vGBufferTarget1", i32 3, i32 0, i32 7, i32 9}
!225 = !{i32 6, !"vGBufferTarget2", i32 3, i32 16, i32 7, i32 9}
!226 = !{i32 6, !"vPositionInView", i32 3, i32 32, i32 7, i32 9}
!227 = !{i32 6, !"vBentNormalInView", i32 3, i32 48, i32 7, i32 9}
!228 = !{i32 6, !"fDiffuseOcclusion", i32 3, i32 60, i32 7, i32 9}
!229 = !{i32 6, !"fSpecularOcclusion", i32 3, i32 64, i32 7, i32 9}
!230 = !{i32 356, !231, !232, !233, !234, !235, !236, !237, !238, !239, !240, !241, !242, !243, !244, !245, !246, !247, !248, !249, !250, !251, !252, !253, !254, !255}
!231 = !{i32 6, !"m_vMin", i32 3, i32 0, i32 7, i32 9}
!232 = !{i32 6, !"m_fBaseVoxelSize", i32 3, i32 12, i32 7, i32 9}
!233 = !{i32 6, !"m_vMax", i32 3, i32 16, i32 7, i32 9}
!234 = !{i32 6, !"m_fInvBaseVoxelSize", i32 3, i32 28, i32 7, i32 9}
!235 = !{i32 6, !"m_uVoxelResolution", i32 3, i32 32, i32 7, i32 5}
!236 = !{i32 6, !"m_uLog2VoxelResolution", i32 3, i32 36, i32 7, i32 5}
!237 = !{i32 6, !"m_uBaseVoxelResolutionX", i32 3, i32 40, i32 7, i32 5}
!238 = !{i32 6, !"m_uBaseVoxelResolutionXY", i32 3, i32 44, i32 7, i32 5}
!239 = !{i32 6, !"m_iPrimaryTreeOffset", i32 3, i32 48, i32 7, i32 4}
!240 = !{i32 6, !"m_iDualTreeOffset", i32 3, i32 52, i32 7, i32 4}
!241 = !{i32 6, !"m_iIrradianceProbeOffset", i32 3, i32 56, i32 7, i32 4}
!242 = !{i32 6, !"m_iDirectTransportProbeOffset", i32 3, i32 60, i32 7, i32 4}
!243 = !{i32 6, !"m_iIndirectTransportProbeOffset", i32 3, i32 64, i32 7, i32 4}
!244 = !{i32 6, !"m_iLightGatherOffset", i32 3, i32 68, i32 7, i32 4}
!245 = !{i32 6, !"m_iLightProbeRefOffset", i32 3, i32 72, i32 7, i32 4}
!246 = !{i32 6, !"m_iDualLightProbeRefOffset", i32 3, i32 76, i32 7, i32 4}
!247 = !{i32 6, !"m_iEnvironmentMapRefOffset", i32 3, i32 80, i32 7, i32 4}
!248 = !{i32 6, !"m_iEnvironmentMapDualRefOffset", i32 3, i32 84, i32 7, i32 4}
!249 = !{i32 6, !"m_iEnvironmentMapInfoOffset", i32 3, i32 88, i32 7, i32 4}
!250 = !{i32 6, !"m_fWorldToGrid", i32 3, i32 92, i32 7, i32 9}
!251 = !{i32 6, !"m_fGridToWorld", i32 3, i32 96, i32 7, i32 9}
!252 = !{i32 6, !"pad0", i32 3, i32 100, i32 7, i32 9}
!253 = !{i32 6, !"pad1", i32 3, i32 104, i32 7, i32 9}
!254 = !{i32 6, !"pad2", i32 3, i32 108, i32 7, i32 9}
!255 = !{i32 6, !"m_iRowTranslationTable", i32 3, i32 112, i32 7, i32 4}
!256 = !{i32 4, !257}
!257 = !{i32 6, !"m_fDiffuseC0", i32 3, i32 0, i32 7, i32 9}
!258 = !{i32 4, !259}
!259 = !{i32 6, !"m_data", i32 3, i32 0, i32 7, i32 5}
!260 = !{i32 356, !44}
!261 = !{i32 28, !44}
!262 = !{i32 28, !263, !264, !265}
!263 = !{i32 6, !"m_childMask", i32 3, i32 0}
!264 = !{i32 6, !"m_pointer", i32 3, i32 20}
!265 = !{i32 6, !"m_payload", i32 3, i32 24, i32 7, i32 5}
!266 = !{i32 20, !267}
!267 = !{i32 6, !"m_uMask", i32 3, i32 0, i32 7, i32 5}
!268 = !{i32 4, !269}
!269 = !{i32 6, !"m_flagAndPointer", i32 3, i32 0, i32 7, i32 5}
!270 = !{i32 36, !44}
!271 = !{i32 36, !272, !273, !274, !275, !276, !277, !278, !279, !280}
!272 = !{i32 6, !"m_vR00", i32 3, i32 0, i32 7, i32 5}
!273 = !{i32 6, !"m_vR01", i32 3, i32 4, i32 7, i32 5}
!274 = !{i32 6, !"m_vR02", i32 3, i32 8, i32 7, i32 5}
!275 = !{i32 6, !"m_vR10", i32 3, i32 12, i32 7, i32 5}
!276 = !{i32 6, !"m_vR11", i32 3, i32 16, i32 7, i32 5}
!277 = !{i32 6, !"m_vR12", i32 3, i32 20, i32 7, i32 5}
!278 = !{i32 6, !"m_vR20", i32 3, i32 24, i32 7, i32 5}
!279 = !{i32 6, !"m_vR21", i32 3, i32 28, i32 7, i32 5}
!280 = !{i32 6, !"m_vR22", i32 3, i32 32, i32 7, i32 5}
!281 = !{i32 116, !44}
!282 = !{i32 116, !283}
!283 = !{i32 6, !"m_mTransport", i32 3, i32 0, i32 7, i32 5}
!284 = !{i32 8, !285, !286}
!285 = !{i32 6, !"m_index1", i32 3, i32 0, i32 7, i32 5}
!286 = !{i32 6, !"m_index2", i32 3, i32 4, i32 7, i32 5}
!287 = !{i32 220, !44}
!288 = !{i32 220, !289, !290, !291, !292, !293}
!289 = !{i32 6, !"m_vDebugColor", i32 3, i32 0, i32 7, i32 9}
!290 = !{i32 6, !"m_fRadius", i32 3, i32 12, i32 7, i32 9}
!291 = !{i32 6, !"m_vCenter", i32 3, i32 16, i32 7, i32 9}
!292 = !{i32 6, !"m_vMin", i32 3, i32 32, i32 7, i32 9}
!293 = !{i32 6, !"m_vMax", i32 3, i32 128, i32 7, i32 9}
!294 = !{i32 44, !295, !296, !297, !298, !299}
!295 = !{i32 6, !"m_vVoxelMin", i32 3, i32 0, i32 7, i32 9}
!296 = !{i32 6, !"m_fVoxelSize", i32 3, i32 12, i32 7, i32 9}
!297 = !{i32 6, !"m_uPayload", i32 3, i32 16, i32 7, i32 5}
!298 = !{i32 6, !"m_uLinearBaseVoxelIndex", i32 3, i32 20, i32 7, i32 5}
!299 = !{i32 6, !"m_vBaseVoxelMin", i32 3, i32 32, i32 7, i32 9}
!300 = !{i32 44, !301, !302, !303, !304, !305, !306, !307}
!301 = !{i32 6, !"m_iParentNode", i32 3, i32 0, i32 7, i32 4}
!302 = !{i32 6, !"m_vParentVoxelMin", i32 3, i32 4, i32 7, i32 9}
!303 = !{i32 6, !"m_vLeafVoxelMin", i32 3, i32 16, i32 7, i32 9}
!304 = !{i32 6, !"m_fLeafVoxelSize", i32 3, i32 28, i32 7, i32 9}
!305 = !{i32 6, !"m_bIsTerminal", i32 3, i32 32, i32 7, i32 1}
!306 = !{i32 6, !"m_bIsSolid", i32 3, i32 36, i32 7, i32 1}
!307 = !{i32 6, !"m_iPayload", i32 3, i32 40, i32 7, i32 4}
!308 = !{i32 16, !309, !310, !311}
!309 = !{i32 6, !"m_vUVOffset", i32 3, i32 0, i32 7, i32 9}
!310 = !{i32 6, !"m_uEnvMapIndex", i32 3, i32 8, i32 7, i32 5}
!311 = !{i32 6, !"m_uEnvInfoIndex", i32 3, i32 12, i32 7, i32 5}
!312 = !{i32 152, !44}
!313 = !{i32 152, !314, !315, !316, !317, !318, !319, !320}
!314 = !{i32 6, !"g_fPerLODDissolve", i32 3, i32 0, i32 7, i32 9}
!315 = !{i32 6, !"g_vWindIndex_Base_Trunk", i32 3, i32 116, i32 7, i32 5}
!316 = !{i32 6, !"g_uMask", i32 3, i32 124, i32 7, i32 5}
!317 = !{i32 6, !"g_iCharacterLightRigIndex", i32 3, i32 128, i32 7, i32 4}
!318 = !{i32 6, !"g_fDepthMultiply", i32 3, i32 132, i32 7, i32 9}
!319 = !{i32 6, !"g_fExtraDepthMultiply", i32 3, i32 136, i32 7, i32 9}
!320 = !{i32 6, !"padTo16Bytes", i32 3, i32 144, i32 7, i32 4}
!321 = !{i32 40, !322, !323, !324, !325, !326, !327, !328, !329, !330, !331}
!322 = !{i32 6, !"uIndexOffset", i32 3, i32 0, i32 7, i32 5}
!323 = !{i32 6, !"uIndexStride", i32 3, i32 4, i32 7, i32 5}
!324 = !{i32 6, !"uVertexStride1", i32 3, i32 8, i32 7, i32 5}
!325 = !{i32 6, !"uMaterialID", i32 3, i32 12, i32 7, i32 5}
!326 = !{i32 6, !"uTangentOffset", i32 3, i32 16, i32 7, i32 5}
!327 = !{i32 6, !"uNormalOffset", i32 3, i32 20, i32 7, i32 5}
!328 = !{i32 6, !"uTexcoordOffset", i32 3, i32 24, i32 7, i32 5}
!329 = !{i32 6, !"uColorOffset", i32 3, i32 28, i32 7, i32 5}
!330 = !{i32 6, !"uIndexBufferSize", i32 3, i32 32, i32 7, i32 5}
!331 = !{i32 6, !"uVertexBuffer1Size", i32 3, i32 36, i32 7, i32 5}
!332 = !{i32 8, !333}
!333 = !{i32 6, !"vIntersection", i32 3, i32 0, i32 4, !"INTERSECTION", i32 7, i32 9}
!334 = !{i32 4, !335}
!335 = !{i32 6, !"uRayIndex", i32 3, i32 0, i32 4, !"RAY_INDEX", i32 7, i32 5}
!336 = !{i32 32, !337, !338, !339, !340}
!337 = !{i32 6, !"Origin", i32 3, i32 0, i32 7, i32 9}
!338 = !{i32 6, !"TMin", i32 3, i32 12, i32 7, i32 9}
!339 = !{i32 6, !"Direction", i32 3, i32 16, i32 7, i32 9}
!340 = !{i32 6, !"TMax", i32 3, i32 28, i32 7, i32 9}
!341 = !{i32 88, !342, !343, !344, !345, !346, !347, !348, !349, !350, !351, !352, !353, !354, !355, !356, !357, !358, !359, !360, !361, !362, !363}
!342 = !{i32 6, !"g_fDebug_h_Slider0", i32 3, i32 0, i32 7, i32 9}
!343 = !{i32 6, !"g_fDebug_h_Slider1", i32 3, i32 4, i32 7, i32 9}
!344 = !{i32 6, !"g_fDebug_h_Slider2", i32 3, i32 8, i32 7, i32 9}
!345 = !{i32 6, !"g_fDebug_h_Slider3", i32 3, i32 12, i32 7, i32 9}
!346 = !{i32 6, !"g_fDebug_h_Slider4", i32 3, i32 16, i32 7, i32 9}
!347 = !{i32 6, !"g_fDebug_h_Slider5", i32 3, i32 20, i32 7, i32 9}
!348 = !{i32 6, !"g_fDebug_h_Slider6", i32 3, i32 24, i32 7, i32 9}
!349 = !{i32 6, !"g_fDebug_h_Slider7", i32 3, i32 28, i32 7, i32 9}
!350 = !{i32 6, !"g_fDebug_h_Knob0", i32 3, i32 32, i32 7, i32 9}
!351 = !{i32 6, !"g_fDebug_h_Knob1", i32 3, i32 36, i32 7, i32 9}
!352 = !{i32 6, !"g_fDebug_h_Knob2", i32 3, i32 40, i32 7, i32 9}
!353 = !{i32 6, !"g_fDebug_h_Knob3", i32 3, i32 44, i32 7, i32 9}
!354 = !{i32 6, !"g_fDebug_h_Knob4", i32 3, i32 48, i32 7, i32 9}
!355 = !{i32 6, !"g_fDebug_h_Knob5", i32 3, i32 52, i32 7, i32 9}
!356 = !{i32 6, !"g_fDebug_h_Knob6", i32 3, i32 56, i32 7, i32 9}
!357 = !{i32 6, !"g_fDebug_h_Knob7", i32 3, i32 60, i32 7, i32 9}
!358 = !{i32 6, !"g_fDebug_h_Toggle0", i32 3, i32 64, i32 7, i32 9}
!359 = !{i32 6, !"g_fDebug_h_Toggle1", i32 3, i32 68, i32 7, i32 9}
!360 = !{i32 6, !"g_fDebug_h_Toggle2", i32 3, i32 72, i32 7, i32 9}
!361 = !{i32 6, !"g_fDebug_h_Toggle3", i32 3, i32 76, i32 7, i32 9}
!362 = !{i32 6, !"g_fDebug_h_Toggle4", i32 3, i32 80, i32 7, i32 9}
!363 = !{i32 6, !"g_fDebug_h_Toggle5", i32 3, i32 84, i32 7, i32 9}
!364 = !{i32 1128, !365, !366, !367, !368, !369, !370, !371, !372, !373, !374, !375, !376, !377, !378, !379, !380, !381, !382, !383, !384, !385, !386, !387, !388, !389, !390, !391, !392, !393, !394, !395, !396}
!365 = !{i32 6, !"g_vScreenRes", i32 3, i32 0, i32 7, i32 9}
!366 = !{i32 6, !"g_vInvScreenRes", i32 3, i32 8, i32 7, i32 9}
!367 = !{i32 6, !"g_vOutputRes", i32 3, i32 16, i32 7, i32 9}
!368 = !{i32 6, !"g_vInvOutputRes", i32 3, i32 24, i32 7, i32 9}
!369 = !{i32 6, !"g_mWorldToView", i32 2, !115, i32 3, i32 32, i32 7, i32 9}
!370 = !{i32 6, !"g_mViewToWorld", i32 2, !115, i32 3, i32 96, i32 7, i32 9}
!371 = !{i32 6, !"g_mViewToClip", i32 2, !115, i32 3, i32 160, i32 7, i32 9}
!372 = !{i32 6, !"g_mClipToView", i32 2, !115, i32 3, i32 224, i32 7, i32 9}
!373 = !{i32 6, !"g_mWorldToClip", i32 2, !115, i32 3, i32 288, i32 7, i32 9}
!374 = !{i32 6, !"g_mClipToWorld", i32 2, !115, i32 3, i32 352, i32 7, i32 9}
!375 = !{i32 6, !"g_mClipToPreviousClip", i32 2, !115, i32 3, i32 416, i32 7, i32 9}
!376 = !{i32 6, !"g_mViewToPreviousClip", i32 2, !115, i32 3, i32 480, i32 7, i32 9}
!377 = !{i32 6, !"g_mPreviousViewToView", i32 2, !115, i32 3, i32 544, i32 7, i32 9}
!378 = !{i32 6, !"g_mPreviousWorldToClip", i32 2, !115, i32 3, i32 608, i32 7, i32 9}
!379 = !{i32 6, !"g_mPreviousViewToClip", i32 2, !115, i32 3, i32 672, i32 7, i32 9}
!380 = !{i32 6, !"g_mClipToPreviousClipNoJitter", i32 2, !115, i32 3, i32 736, i32 7, i32 9}
!381 = !{i32 6, !"g_mPreviousClipToClipNoJitter", i32 2, !115, i32 3, i32 800, i32 7, i32 9}
!382 = !{i32 6, !"g_vViewPoint", i32 3, i32 864, i32 7, i32 9}
!383 = !{i32 6, !"g_fInvNear", i32 3, i32 880, i32 7, i32 9}
!384 = !{i32 6, !"g_mViewToCameraView", i32 2, !115, i32 3, i32 896, i32 7, i32 9}
!385 = !{i32 6, !"g_mCameraViewToCameraClip", i32 2, !115, i32 3, i32 960, i32 7, i32 9}
!386 = !{i32 6, !"g_mCameraClipToView", i32 2, !115, i32 3, i32 1024, i32 7, i32 9}
!387 = !{i32 6, !"g_fWorldTime", i32 3, i32 1088, i32 7, i32 9}
!388 = !{i32 6, !"g_fWorldTimeDelta", i32 3, i32 1092, i32 7, i32 9}
!389 = !{i32 6, !"g_fRealTime", i32 3, i32 1096, i32 7, i32 9}
!390 = !{i32 6, !"g_fRealTimeDelta", i32 3, i32 1100, i32 7, i32 9}
!391 = !{i32 6, !"g_fLastValidWorldTimeDelta", i32 3, i32 1104, i32 7, i32 9}
!392 = !{i32 6, !"g_uTemporalFrame", i32 3, i32 1108, i32 7, i32 5}
!393 = !{i32 6, !"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5}
!394 = !{i32 6, !"g_bHDR", i32 3, i32 1116, i32 7, i32 1}
!395 = !{i32 6, !"g_fHDRSDRSceneBrightnessMultiplier", i32 3, i32 1120, i32 7, i32 9}
!396 = !{i32 6, !"g_fHDRSDRUIBrightnessMultiplier", i32 3, i32 1124, i32 7, i32 9}
!397 = !{i32 32, !398, !399, !400, !401}
!398 = !{i32 6, !"g_vShadowMapRes", i32 3, i32 0, i32 7, i32 9}
!399 = !{i32 6, !"g_vShadowMapVSMRes", i32 3, i32 8, i32 7, i32 9}
!400 = !{i32 6, !"g_vJitterOffset", i32 3, i32 16, i32 7, i32 9}
!401 = !{i32 6, !"g_vSnapOffset", i32 3, i32 24, i32 7, i32 4}
!402 = !{i32 4, !403}
!403 = !{i32 6, !"g_fOnePerTranslucencyKernelCount", i32 3, i32 0, i32 7, i32 9}
!404 = !{i32 408, !405, !406, !407, !408, !409, !410, !411, !412, !413, !414, !415, !416, !417, !418, !419, !420, !421, !422, !423, !424, !425, !426, !427, !428, !429, !430, !431, !432, !433, !434, !435, !436, !437, !438, !439}
!405 = !{i32 6, !"g_fTileDepthClipRanges", i32 3, i32 0, i32 7, i32 9}
!406 = !{i32 6, !"g_fTileDepthRanges", i32 3, i32 80, i32 7, i32 9}
!407 = !{i32 6, !"g_vDepthTileResolve", i32 3, i32 160, i32 7, i32 9}
!408 = !{i32 6, !"g_uDepthTileCount", i32 3, i32 168, i32 7, i32 5}
!409 = !{i32 6, !"g_vTileResolution", i32 3, i32 176, i32 7, i32 5}
!410 = !{i32 6, !"g_vTileWidthHeightDepth", i32 3, i32 192, i32 7, i32 5}
!411 = !{i32 6, !"g_vTileResolutionPerScreenResolution", i32 3, i32 208, i32 7, i32 9}
!412 = !{i32 6, !"g_vTileDepthNearFar", i32 3, i32 216, i32 7, i32 9}
!413 = !{i32 6, !"g_uMaxPointLightsPerTile", i32 3, i32 224, i32 7, i32 5}
!414 = !{i32 6, !"g_uMaxSpotLightsPerTile", i32 3, i32 228, i32 7, i32 5}
!415 = !{i32 6, !"g_uAmbientLightTotalCount", i32 3, i32 232, i32 7, i32 5}
!416 = !{i32 6, !"g_fAmbientEnvDiffuseIntensity", i32 3, i32 236, i32 7, i32 9}
!417 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnTransparent", i32 3, i32 240, i32 7, i32 9}
!418 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnOpaque", i32 3, i32 244, i32 7, i32 9}
!419 = !{i32 6, !"g_fAmbientSkyIntensity", i32 3, i32 248, i32 7, i32 9}
!420 = !{i32 6, !"g_fAmbientLocalIntensity", i32 3, i32 252, i32 7, i32 9}
!421 = !{i32 6, !"g_uPointLightTotalCount", i32 3, i32 256, i32 7, i32 5}
!422 = !{i32 6, !"g_uSpotLightTotalCount", i32 3, i32 260, i32 7, i32 5}
!423 = !{i32 6, !"g_uSunLightTotalCount", i32 3, i32 264, i32 7, i32 5}
!424 = !{i32 6, !"g_uAmbientLightEnabled", i32 3, i32 268, i32 7, i32 5}
!425 = !{i32 6, !"g_fAmbientDepthRampDistance", i32 3, i32 272, i32 7, i32 9}
!426 = !{i32 6, !"g_fAmbientDepthRampFeather", i32 3, i32 276, i32 7, i32 9}
!427 = !{i32 6, !"g_fAmbientDepthRampIntensityNear", i32 3, i32 280, i32 7, i32 9}
!428 = !{i32 6, !"g_fAmbientDepthRampIntensityFar", i32 3, i32 284, i32 7, i32 9}
!429 = !{i32 6, !"g_vVolumeLightDimensions", i32 3, i32 288, i32 7, i32 5}
!430 = !{i32 6, !"g_vVolumeLightProjectionConstants", i32 3, i32 304, i32 7, i32 9}
!431 = !{i32 6, !"g_vHalfResVolumeLightProjectionConstants", i32 3, i32 320, i32 7, i32 9}
!432 = !{i32 6, !"g_vOnePerVolumeLightDimensions", i32 3, i32 336, i32 7, i32 9}
!433 = !{i32 6, !"g_vVolumeLightXYToTileXY", i32 3, i32 352, i32 7, i32 9}
!434 = !{i32 6, !"g_vVolumeLightDepthResolve", i32 3, i32 368, i32 7, i32 9}
!435 = !{i32 6, !"g_fVolumeLightOnePerDepthMinusOne", i32 3, i32 380, i32 7, i32 9}
!436 = !{i32 6, !"g_vVolumeLightNearSplit0Far", i32 3, i32 384, i32 7, i32 9}
!437 = !{i32 6, !"g_fVolumeLightKernelWidth", i32 3, i32 396, i32 7, i32 9}
!438 = !{i32 6, !"g_fVolumeLightSamplingBias", i32 3, i32 400, i32 7, i32 9}
!439 = !{i32 6, !"g_uRTContactShadowRayCount", i32 3, i32 404, i32 7, i32 5}
!440 = !{i32 8, !441, !442}
!441 = !{i32 6, !"g_fEnvReflectionEdgeLength", i32 3, i32 0, i32 7, i32 9}
!442 = !{i32 6, !"g_fEnvReflectionMipCount", i32 3, i32 4, i32 7, i32 9}
!443 = !{i32 188, !444, !445, !446, !447, !448, !449, !450, !451, !452, !453, !454, !455, !456, !457, !458, !459, !460, !461, !462, !463}
!444 = !{i32 6, !"g_atmosphere_vSunDir", i32 3, i32 0, i32 7, i32 9}
!445 = !{i32 6, !"g_atmosphere_vSunE", i32 3, i32 16, i32 7, i32 9}
!446 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants1", i32 3, i32 32, i32 7, i32 9}
!447 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants2", i32 3, i32 40, i32 7, i32 9}
!448 = !{i32 6, !"g_atmosphere_vPhaseBlend", i32 3, i32 48, i32 7, i32 9}
!449 = !{i32 6, !"g_atmosphere_vPhaseG", i32 3, i32 52, i32 7, i32 9}
!450 = !{i32 6, !"g_atmosphere_vFog", i32 3, i32 64, i32 7, i32 9}
!451 = !{i32 6, !"g_vFogColor", i32 3, i32 80, i32 7, i32 9}
!452 = !{i32 6, !"g_vFogColorOpposite", i32 3, i32 96, i32 7, i32 9}
!453 = !{i32 6, !"g_fFogExp", i32 3, i32 108, i32 7, i32 9}
!454 = !{i32 6, !"g_fFogGroundDensityAtViewer", i32 3, i32 112, i32 7, i32 9}
!455 = !{i32 6, !"g_fFogGroundHeight", i32 3, i32 116, i32 7, i32 9}
!456 = !{i32 6, !"g_fFogGroundFalloff", i32 3, i32 120, i32 7, i32 9}
!457 = !{i32 6, !"g_fFogGroundDensity", i32 3, i32 124, i32 7, i32 9}
!458 = !{i32 6, !"g_vFogGroundDensityMapRange", i32 3, i32 128, i32 7, i32 9}
!459 = !{i32 6, !"g_vFogGroundSimulationVelocityAndScale", i32 3, i32 144, i32 7, i32 9}
!460 = !{i32 6, !"g_fFogDepthRampDistance", i32 3, i32 156, i32 7, i32 9}
!461 = !{i32 6, !"g_fFogDepthRampFeather", i32 3, i32 160, i32 7, i32 9}
!462 = !{i32 6, !"g_fFogDepthRampIntensityNear", i32 3, i32 164, i32 7, i32 9}
!463 = !{i32 6, !"g_vFogAlbedo", i32 3, i32 176, i32 7, i32 9}
!464 = !{i32 40, !465, !466, !467, !468, !469, !470}
!465 = !{i32 6, !"g_vOnePerVolumeAtlasSize", i32 3, i32 0, i32 7, i32 9}
!466 = !{i32 6, !"g_uActiveVolumeCount", i32 3, i32 12, i32 7, i32 5}
!467 = !{i32 6, !"g_vAdditiveAmbient", i32 3, i32 16, i32 7, i32 9}
!468 = !{i32 6, !"g_uActiveDebugVolumeCount", i32 3, i32 28, i32 7, i32 5}
!469 = !{i32 6, !"g_fTransparentBalanceEnergy", i32 3, i32 32, i32 7, i32 9}
!470 = !{i32 6, !"g_fTransparentBalanceEnergyStrength", i32 3, i32 36, i32 7, i32 9}
!471 = !{i32 24576, !472}
!472 = !{i32 6, !"g_cbActiveVolumeTopLevelInfoArray", i32 3, i32 0}
!473 = !{i32 24576, !474}
!474 = !{i32 6, !"g_cbDebugVolumeTopLevelInfoArray", i32 3, i32 0}
!475 = !{i32 12, !476, !477, !478}
!476 = !{i32 6, !"g_fWorldSize", i32 3, i32 0, i32 7, i32 9}
!477 = !{i32 6, !"g_fCellSize", i32 3, i32 4, i32 7, i32 9}
!478 = !{i32 6, !"g_iCellCount", i32 3, i32 8, i32 7, i32 4}
!479 = !{i32 24, !480, !481, !482, !483, !484}
!480 = !{i32 6, !"g_fInvEnvironmentMapsPerRow", i32 3, i32 0, i32 7, i32 9}
!481 = !{i32 6, !"g_fEnvironmentMapsPerRow", i32 3, i32 4, i32 7, i32 9}
!482 = !{i32 6, !"g_fEnvironmentMapColSize", i32 3, i32 8, i32 7, i32 9}
!483 = !{i32 6, !"g_fEnvironmentMapRowSize", i32 3, i32 12, i32 7, i32 9}
!484 = !{i32 6, !"g_fInvEnvironmentMapAtlasSize", i32 3, i32 16, i32 7, i32 9}
!485 = !{i32 48, !486, !487, !488}
!486 = !{i32 6, !"g_uStride0_uStride1_uOffset0_uOffset1", i32 3, i32 0, i32 7, i32 5}
!487 = !{i32 6, !"g_Tangent_Normal_Texcoord_Color", i32 3, i32 16, i32 7, i32 5}
!488 = !{i32 6, !"g_BIndices_BWeights", i32 3, i32 32, i32 7, i32 5}
!489 = !{i32 40, !490}
!490 = !{i32 6, !"g_cbRaytracingHit", i32 3, i32 0}
!491 = !{i32 24, !492, !493, !494, !495, !496, !497}
!492 = !{i32 6, !"g_fClampReflectionIntensity", i32 3, i32 0, i32 7, i32 9}
!493 = !{i32 6, !"g_fRTReflectionAddRays", i32 3, i32 4, i32 7, i32 9}
!494 = !{i32 6, !"g_uRTReflectionRayCount", i32 3, i32 8, i32 7, i32 5}
!495 = !{i32 6, !"g_fRTZeroRayReflectance", i32 3, i32 12, i32 7, i32 9}
!496 = !{i32 6, !"g_fRTMaxRayReflectance", i32 3, i32 16, i32 7, i32 9}
!497 = !{i32 6, !"g_fRTMaxRayRoughness", i32 3, i32 20, i32 7, i32 9}
!498 = !{i32 1, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !499, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !499, void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !506, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !499, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !499, void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !506, void ()* @"\01?reflectionRayGeneration@@YAXXZ", !507}
!499 = !{!500, !502, !504}
!500 = !{i32 1, !501, !501}
!501 = !{}
!502 = !{i32 2, !503, !501}
!503 = !{i32 4, !"SV_RayPayload"}
!504 = !{i32 0, !505, !501}
!505 = !{i32 4, !"SV_IntersectionAttributes"}
!506 = !{!500, !502}
!507 = !{!500}
!508 = !{null, !"", null, !3, !509}
!509 = !{i32 0, i64 65808, i32 5, !510}
!510 = !{i32 0}
!511 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !512}
!512 = !{i32 8, i32 9, i32 6, i32 4, i32 7, i32 8, i32 5, !510}
!513 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !514}
!514 = !{i32 8, i32 10, i32 6, i32 4, i32 7, i32 8, i32 5, !510}
!515 = !{void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !"\01?reflectionMiss@@YAXUHitData@@@Z", null, null, !516}
!516 = !{i32 8, i32 11, i32 6, i32 4, i32 5, !510}
!517 = !{void ()* @"\01?reflectionRayGeneration@@YAXXZ", !"\01?reflectionRayGeneration@@YAXXZ", null, null, !518}
!518 = !{i32 8, i32 7, i32 5, !510}
!519 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !512}
!520 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !514}
!521 = !{void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !"\01?shadowMiss@@YAXUHitData@@@Z", null, null, !516}
!522 = !{!523, !523, i64 0}
!523 = !{!"omnipotent char", !524, i64 0}
!524 = !{!"Simple C/C++ TBAA"}
!525 = !{!526, !526, i64 0}
!526 = !{!"float", !523, i64 0}
!527 = !{!528, !528, i64 0}
!528 = !{!"int", !523, i64 0}

