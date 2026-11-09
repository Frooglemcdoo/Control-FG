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
; g_tGBuffer1                       texture     f32          2d      T0             t0     1
; g_tLinearDepth                    texture     f32          2d      T1             t1     1
; g_tClipDepth                      texture     f32          2d      T2             t2     1
; g_sbDeferredLightSunData          texture  struct         r/o      T3             t3     1
; g_tStaticBlueNoiseRGBA_0          texture     f32          2d      T4             t4     1
; g_rtScene                         texture     i32         ras      T5             t5     1
; g_rwtShadow                           UAV     u32     2darray      U0             u0     1
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%"class.Texture2D<vector<float, 4> >" = type { <4 x float>, %"class.Texture2D<vector<float, 4> >::mips_type" }
%"class.Texture2D<vector<float, 4> >::mips_type" = type { i32 }
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
%dx.types.CBufRet.i32 = type { i32, i32, i32, i32 }
%struct.GBufferFormat = type { <4 x float>, <4 x float> }
%struct.SamplerState = type { i32 }
%"class.Texture2D<float>" = type { float, %"class.Texture2D<float>::mips_type" }
%"class.Texture2D<float>::mips_type" = type { i32 }
%"class.Texture2D<unsigned int>" = type { i32, %"class.Texture2D<unsigned int>::mips_type" }
%"class.Texture2D<unsigned int>::mips_type" = type { i32 }
%"class.Texture2DMS<float, 0>" = type { float, %"class.Texture2DMS<float, 0>::sample_type" }
%"class.Texture2DMS<float, 0>::sample_type" = type { i32 }
%"class.StructuredBuffer<vector<unsigned int, 4> >" = type { <4 x i32> }
%"class.StructuredBuffer<MaterialDataPart1>" = type { %struct.MaterialDataPart1 }
%struct.MaterialDataPart1 = type { i32, i32 }
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
@"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_sbDeferredLightSunData@@3V?$StructuredBuffer@UDeferredLightSun@@@@A" = external constant %"class.StructuredBuffer<DeferredLightSun>", align 4
@"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_rtScene@@3URaytracingAccelerationStructure@@A" = external constant %struct.RaytracingAccelerationStructure, align 4
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
"\01?internal.getWorldSpaceNoiseOffset@@YA?AV?$vector@I$01@@V?$vector@M$01@@V?$matrix@M$03$03@@10MV?$vector@M$02@@22M_N@Z.exit33":
  %0 = alloca [3 x float], align 4
  %1 = alloca [3 x i32], align 4
  %2 = alloca [3 x float], align 4
  %3 = load %struct.RaytracingAccelerationStructure, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", align 4
  %4 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %5 = load %"class.StructuredBuffer<DeferredLightSun>", %"class.StructuredBuffer<DeferredLightSun>"* @"\01?g_sbDeferredLightSunData@@3V?$StructuredBuffer@UDeferredLightSun@@@@A", align 4
  %6 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %7 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %8 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %9 = load %rtreflection, %rtreflection* @rtreflection, align 4
  %10 = load %sys_constants, %sys_constants* @sys_constants, align 4
  %rtreflection = call %dx.types.Handle @dx.op.createHandleForLib.rtreflection(i32 160, %rtreflection %9)  ; CreateHandleForLib(Resource)
  %sys_constants = call %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32 160, %sys_constants %10)  ; CreateHandleForLib(Resource)
  %11 = alloca %struct.HitData, align 8
  %12 = alloca %struct.HitData, align 8
  %ray = alloca %struct.RayDesc, align 4
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex156 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %13 = uitofp i32 %DispatchRaysIndex to float
  %14 = uitofp i32 %DispatchRaysIndex156 to float
  %.i0 = fadd fast float %13, 5.000000e-01
  %.i1 = fadd fast float %14, 5.000000e-01
  %15 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %16 = extractvalue %dx.types.CBufRet.f32 %15, 2
  %17 = extractvalue %dx.types.CBufRet.f32 %15, 3
  %.i0158 = fmul fast float %16, %.i0
  %.i1159 = fmul fast float %.i1, %17
  %18 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %6)  ; CreateHandleForLib(Resource)
  %TextureLoad = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %18, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex156, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %19 = extractvalue %dx.types.ResRet.f32 %TextureLoad, 0
  %20 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %7)  ; CreateHandleForLib(Resource)
  %TextureLoad151 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %20, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex156, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %21 = extractvalue %dx.types.ResRet.f32 %TextureLoad151, 0
  %.i0160 = fmul fast float %.i0158, 2.000000e+00
  %.i1161 = fmul fast float %.i1159, 2.000000e+00
  %.i0162 = fadd fast float %.i0160, -1.000000e+00
  %.i1163453 = fsub fast float 1.000000e+00, %.i1161
  %22 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 14)  ; CBufferLoadLegacy(handle,regIndex)
  %23 = extractvalue %dx.types.CBufRet.f32 %22, 0
  %24 = extractvalue %dx.types.CBufRet.f32 %22, 1
  %25 = extractvalue %dx.types.CBufRet.f32 %22, 2
  %26 = extractvalue %dx.types.CBufRet.f32 %22, 3
  %27 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %28 = extractvalue %dx.types.CBufRet.f32 %27, 0
  %29 = extractvalue %dx.types.CBufRet.f32 %27, 1
  %30 = extractvalue %dx.types.CBufRet.f32 %27, 2
  %31 = extractvalue %dx.types.CBufRet.f32 %27, 3
  %32 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %33 = extractvalue %dx.types.CBufRet.f32 %32, 0
  %34 = extractvalue %dx.types.CBufRet.f32 %32, 1
  %35 = extractvalue %dx.types.CBufRet.f32 %32, 2
  %36 = extractvalue %dx.types.CBufRet.f32 %32, 3
  %37 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %38 = extractvalue %dx.types.CBufRet.f32 %37, 0
  %39 = extractvalue %dx.types.CBufRet.f32 %37, 1
  %40 = extractvalue %dx.types.CBufRet.f32 %37, 2
  %41 = extractvalue %dx.types.CBufRet.f32 %37, 3
  %42 = fmul fast float %23, %.i0162
  %FMad84 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %28, float %42)  ; FMad(a,b,c)
  %FMad83 = call float @dx.op.tertiary.f32(i32 46, float %19, float %33, float %FMad84)  ; FMad(a,b,c)
  %43 = fadd fast float %FMad83, %38
  %44 = fmul fast float %24, %.i0162
  %FMad81 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %29, float %44)  ; FMad(a,b,c)
  %FMad80 = call float @dx.op.tertiary.f32(i32 46, float %19, float %34, float %FMad81)  ; FMad(a,b,c)
  %45 = fadd fast float %FMad80, %39
  %46 = fmul fast float %25, %.i0162
  %FMad78 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %30, float %46)  ; FMad(a,b,c)
  %FMad77 = call float @dx.op.tertiary.f32(i32 46, float %19, float %35, float %FMad78)  ; FMad(a,b,c)
  %47 = fadd fast float %FMad77, %40
  %48 = fmul fast float %26, %.i0162
  %FMad75 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %31, float %48)  ; FMad(a,b,c)
  %FMad74 = call float @dx.op.tertiary.f32(i32 46, float %19, float %36, float %FMad75)  ; FMad(a,b,c)
  %49 = fadd fast float %FMad74, %41
  %50 = fdiv fast float 1.000000e+00, %49
  %.i0164 = fmul fast float %50, %43
  %.i1165 = fmul fast float %50, %45
  %.i2 = fmul fast float %50, %47
  %51 = fmul fast float %.i0164, %.i0164
  %52 = fmul fast float %.i1165, %.i1165
  %53 = fadd fast float %51, %52
  %54 = fmul fast float %.i2, %.i2
  %55 = fadd fast float %53, %54
  %Sqrt66 = call float @dx.op.unary.f32(i32 24, float %55)  ; Sqrt(value)
  %.i0166 = fdiv fast float %.i0164, %Sqrt66
  %.i1167 = fdiv fast float %.i1165, %Sqrt66
  %.i2168 = fdiv fast float %.i2, %Sqrt66
  %56 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 22)  ; CBufferLoadLegacy(handle,regIndex)
  %57 = extractvalue %dx.types.CBufRet.f32 %56, 0
  %58 = extractvalue %dx.types.CBufRet.f32 %56, 1
  %59 = extractvalue %dx.types.CBufRet.f32 %56, 2
  %60 = extractvalue %dx.types.CBufRet.f32 %56, 3
  %61 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 23)  ; CBufferLoadLegacy(handle,regIndex)
  %62 = extractvalue %dx.types.CBufRet.f32 %61, 0
  %63 = extractvalue %dx.types.CBufRet.f32 %61, 1
  %64 = extractvalue %dx.types.CBufRet.f32 %61, 2
  %65 = extractvalue %dx.types.CBufRet.f32 %61, 3
  %66 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 24)  ; CBufferLoadLegacy(handle,regIndex)
  %67 = extractvalue %dx.types.CBufRet.f32 %66, 0
  %68 = extractvalue %dx.types.CBufRet.f32 %66, 1
  %69 = extractvalue %dx.types.CBufRet.f32 %66, 2
  %70 = extractvalue %dx.types.CBufRet.f32 %66, 3
  %71 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 25)  ; CBufferLoadLegacy(handle,regIndex)
  %72 = extractvalue %dx.types.CBufRet.f32 %71, 0
  %73 = extractvalue %dx.types.CBufRet.f32 %71, 1
  %74 = extractvalue %dx.types.CBufRet.f32 %71, 2
  %75 = extractvalue %dx.types.CBufRet.f32 %71, 3
  %76 = fmul fast float %57, %.i0162
  %FMad132 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %62, float %76)  ; FMad(a,b,c)
  %FMad131 = call float @dx.op.tertiary.f32(i32 46, float %19, float %67, float %FMad132)  ; FMad(a,b,c)
  %77 = fadd fast float %FMad131, %72
  %78 = fmul fast float %58, %.i0162
  %FMad129 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %63, float %78)  ; FMad(a,b,c)
  %FMad128 = call float @dx.op.tertiary.f32(i32 46, float %19, float %68, float %FMad129)  ; FMad(a,b,c)
  %79 = fadd fast float %FMad128, %73
  %80 = fmul fast float %59, %.i0162
  %FMad126 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %64, float %80)  ; FMad(a,b,c)
  %FMad125 = call float @dx.op.tertiary.f32(i32 46, float %19, float %69, float %FMad126)  ; FMad(a,b,c)
  %81 = fadd fast float %FMad125, %74
  %82 = fmul fast float %60, %.i0162
  %FMad123 = call float @dx.op.tertiary.f32(i32 46, float %.i1163453, float %65, float %82)  ; FMad(a,b,c)
  %FMad122 = call float @dx.op.tertiary.f32(i32 46, float %19, float %70, float %FMad123)  ; FMad(a,b,c)
  %83 = fadd fast float %FMad122, %75
  %84 = fdiv fast float 1.000000e+00, %83
  %.i0173 = fmul fast float %84, %77
  %.i1174 = fmul fast float %84, %79
  %.i2175 = fmul fast float %84, %81
  %85 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 54)  ; CBufferLoadLegacy(handle,regIndex)
  %86 = extractvalue %dx.types.CBufRet.f32 %85, 0
  %87 = extractvalue %dx.types.CBufRet.f32 %85, 1
  %88 = extractvalue %dx.types.CBufRet.f32 %85, 2
  %.i0176 = fsub fast float %.i0173, %86
  %.i1177 = fsub fast float %.i1174, %87
  %.i2178 = fsub fast float %.i2175, %88
  %89 = fmul fast float %.i0176, %.i0176
  %90 = fmul fast float %.i1177, %.i1177
  %91 = fadd fast float %89, %90
  %92 = fmul fast float %.i2178, %.i2178
  %93 = fadd fast float %91, %92
  %Sqrt67 = call float @dx.op.unary.f32(i32 24, float %93)  ; Sqrt(value)
  %.i0179 = fdiv fast float %.i0176, %Sqrt67
  %.i1180 = fdiv fast float %.i1177, %Sqrt67
  %.i2181 = fdiv fast float %.i2178, %Sqrt67
  %94 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %8)  ; CreateHandleForLib(Resource)
  %TextureLoad152 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %94, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex156, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %95 = extractvalue %dx.types.ResRet.f32 %TextureLoad152, 0
  %96 = extractvalue %dx.types.ResRet.f32 %TextureLoad152, 1
  %97 = extractvalue %dx.types.ResRet.f32 %TextureLoad152, 3
  %98 = fmul fast float %97, 2.550000e+02
  %99 = fadd fast float %98, 5.000000e-01
  %100 = fptoui float %99 to i32
  %101 = and i32 %100, 254
  %102 = uitofp i32 %101 to float
  %.i0182 = fmul fast float %95, 2.550000e+02
  %.i1183 = fmul fast float %96, 2.550000e+02
  %.i0185 = fadd fast float %.i0182, 5.000000e-01
  %.i1186 = fadd fast float %.i1183, 5.000000e-01
  %.i2187 = fadd fast float %102, 5.000000e-01
  %Round_ni60 = call float @dx.op.unary.f32(i32 27, float %.i0185)  ; Round_ni(value)
  %Round_ni61 = call float @dx.op.unary.f32(i32 27, float %.i1186)  ; Round_ni(value)
  %Round_ni62 = call float @dx.op.unary.f32(i32 27, float %.i2187)  ; Round_ni(value)
  %.i0188 = fptosi float %Round_ni60 to i32
  %.i1189 = fptosi float %Round_ni61 to i32
  %.i2190 = fptosi float %Round_ni62 to i32
  %.i0191.451 = lshr i32 %.i2190, 1
  %.i1192.452 = lshr i32 %.i2190, 4
  %.i0193 = and i32 %.i0191.451, 7
  %.i1194 = and i32 %.i1192.452, 15
  %.i0195 = shl i32 %.i0188, 3
  %.i1196 = shl i32 %.i1189, 4
  %.i0197 = or i32 %.i0193, %.i0195
  %.i1198 = or i32 %.i1194, %.i1196
  %103 = sitofp i32 %.i0197 to float
  %104 = sitofp i32 %.i1198 to float
  %105 = fmul fast float %103, 0x3F54CF66A0000000
  %.i0205 = fadd fast float %105, 0xBFF4CCCCC0000000
  %106 = fmul fast float %104, 0x3F44CE19C0000000
  %.i1206 = fadd fast float %106, 0xBFF4CCCCC0000000
  %107 = fmul fast float %.i0205, %.i0205
  %108 = fmul fast float %.i1206, %.i1206
  %109 = fadd fast float %107, 1.000000e+00
  %110 = fadd fast float %109, %108
  %111 = fmul fast float %103, 0x3F64CF66A0000000
  %.i0207 = fadd fast float %111, 0xC004CCCCC0000000
  %112 = fmul fast float %104, 0x3F54CE19C0000000
  %.i1208 = fadd fast float %112, 0xC004CCCCC0000000
  %113 = fadd fast float %108, -1.000000e+00
  %114 = fadd fast float %113, %107
  %.i0209 = fdiv fast float %.i0207, %110
  %.i1210 = fdiv fast float %.i1208, %110
  %.i2211 = fdiv fast float %114, %110
  %115 = fmul fast float %.i0209, %.i0209
  %116 = fmul fast float %.i1210, %.i1210
  %117 = fadd fast float %116, %115
  %118 = fmul fast float %.i2211, %.i2211
  %119 = fadd fast float %117, %118
  %Sqrt68 = call float @dx.op.unary.f32(i32 24, float %119)  ; Sqrt(value)
  %.i0212 = fdiv fast float %.i0209, %Sqrt68
  %.i1213 = fdiv fast float %.i1210, %Sqrt68
  %.i2214 = fdiv fast float %.i2211, %Sqrt68
  %120 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %121 = extractvalue %dx.types.CBufRet.f32 %120, 0
  %122 = extractvalue %dx.types.CBufRet.f32 %120, 1
  %123 = extractvalue %dx.types.CBufRet.f32 %120, 2
  %124 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %125 = extractvalue %dx.types.CBufRet.f32 %124, 0
  %126 = extractvalue %dx.types.CBufRet.f32 %124, 1
  %127 = extractvalue %dx.types.CBufRet.f32 %124, 2
  %128 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %129 = extractvalue %dx.types.CBufRet.f32 %128, 0
  %130 = extractvalue %dx.types.CBufRet.f32 %128, 1
  %131 = extractvalue %dx.types.CBufRet.f32 %128, 2
  %132 = fmul fast float %121, %.i0212
  %FMad120 = call float @dx.op.tertiary.f32(i32 46, float %.i1213, float %125, float %132)  ; FMad(a,b,c)
  %FMad119 = call float @dx.op.tertiary.f32(i32 46, float %.i2214, float %129, float %FMad120)  ; FMad(a,b,c)
  %133 = fmul fast float %122, %.i0212
  %FMad118 = call float @dx.op.tertiary.f32(i32 46, float %.i1213, float %126, float %133)  ; FMad(a,b,c)
  %FMad117 = call float @dx.op.tertiary.f32(i32 46, float %.i2214, float %130, float %FMad118)  ; FMad(a,b,c)
  %134 = fmul fast float %123, %.i0212
  %FMad116 = call float @dx.op.tertiary.f32(i32 46, float %.i1213, float %127, float %134)  ; FMad(a,b,c)
  %FMad115 = call float @dx.op.tertiary.f32(i32 46, float %.i2214, float %131, float %FMad116)  ; FMad(a,b,c)
  %.i0215 = fmul fast float %21, 0x3F50624DE0000000
  %.i0218 = fmul fast float %.i0215, %.i0179
  %.i1219 = fmul fast float %.i0215, %.i1180
  %.i2220 = fmul fast float %.i0215, %.i2181
  %.i0221 = fsub fast float %.i0173, %.i0218
  %.i1222 = fsub fast float %.i1174, %.i1219
  %.i2223 = fsub fast float %.i2175, %.i2220
  %.i0227 = fmul fast float %.i0215, %FMad119
  %.i1228 = fmul fast float %.i0215, %FMad117
  %.i2229 = fmul fast float %.i0215, %FMad115
  %.i0230 = fadd fast float %.i0221, %.i0227
  %.i1231 = fadd fast float %.i1222, %.i1228
  %.i2232 = fadd fast float %.i2223, %.i2229
  %.upto0395 = insertelement <3 x float> undef, float %.i0230, i32 0
  %.upto1396 = insertelement <3 x float> %.upto0395, float %.i1231, i32 1
  %135 = insertelement <3 x float> %.upto1396, float %.i2232, i32 2
  %136 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 0
  store <3 x float> %135, <3 x float>* %136, align 4, !tbaa !517
  %137 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 1
  store float 0.000000e+00, float* %137, align 4, !tbaa !520
  %138 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %rtreflection, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %139 = extractvalue %dx.types.CBufRet.i32 %138, 2
  %140 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %141 = extractvalue %dx.types.CBufRet.f32 %140, 1
  %142 = extractvalue %dx.types.CBufRet.f32 %140, 3
  %143 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %144 = extractvalue %dx.types.CBufRet.f32 %143, 1
  %145 = extractvalue %dx.types.CBufRet.f32 %143, 3
  %146 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %147 = extractvalue %dx.types.CBufRet.f32 %146, 1
  %148 = extractvalue %dx.types.CBufRet.f32 %146, 3
  %149 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 18)  ; CBufferLoadLegacy(handle,regIndex)
  %150 = extractvalue %dx.types.CBufRet.f32 %149, 0
  %151 = extractvalue %dx.types.CBufRet.f32 %149, 1
  %152 = extractvalue %dx.types.CBufRet.f32 %149, 3
  %153 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 19)  ; CBufferLoadLegacy(handle,regIndex)
  %154 = extractvalue %dx.types.CBufRet.f32 %153, 0
  %155 = extractvalue %dx.types.CBufRet.f32 %153, 1
  %156 = extractvalue %dx.types.CBufRet.f32 %153, 3
  %157 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 20)  ; CBufferLoadLegacy(handle,regIndex)
  %158 = extractvalue %dx.types.CBufRet.f32 %157, 0
  %159 = extractvalue %dx.types.CBufRet.f32 %157, 1
  %160 = extractvalue %dx.types.CBufRet.f32 %157, 3
  %161 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 21)  ; CBufferLoadLegacy(handle,regIndex)
  %162 = extractvalue %dx.types.CBufRet.f32 %161, 0
  %163 = extractvalue %dx.types.CBufRet.f32 %161, 1
  %164 = extractvalue %dx.types.CBufRet.f32 %161, 3
  %165 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %166 = extractvalue %dx.types.CBufRet.f32 %165, 0
  %167 = extractvalue %dx.types.CBufRet.f32 %165, 1
  %168 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 0
  store float %.i0173, float* %168, align 4
  %169 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 1
  store float %.i1174, float* %169, align 4
  %170 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 2
  store float %.i2175, float* %170, align 4
  %.i0237 = fmul fast float %FMad119, 4.000000e+00
  %.i1238 = fmul fast float %FMad117, 4.000000e+00
  %.i2239 = fmul fast float %FMad115, 4.000000e+00
  %Round_ne = call float @dx.op.unary.f32(i32 26, float %.i0237)  ; Round_ne(value)
  %Round_ne49 = call float @dx.op.unary.f32(i32 26, float %.i1238)  ; Round_ne(value)
  %Round_ne50 = call float @dx.op.unary.f32(i32 26, float %.i2239)  ; Round_ne(value)
  %171 = fmul fast float %Round_ne, %Round_ne
  %172 = fmul fast float %Round_ne49, %Round_ne49
  %173 = fadd fast float %172, %171
  %174 = fmul fast float %Round_ne50, %Round_ne50
  %175 = fadd fast float %173, %174
  %Sqrt48 = call float @dx.op.unary.f32(i32 24, float %175)  ; Sqrt(value)
  %.i0240 = fdiv fast float %Round_ne, %Sqrt48
  %.i1241 = fdiv fast float %Round_ne49, %Sqrt48
  %.i2242 = fdiv fast float %Round_ne50, %Sqrt48
  %FAbs57 = call float @dx.op.unary.f32(i32 6, float %.i0240)  ; FAbs(value)
  %FAbs58 = call float @dx.op.unary.f32(i32 6, float %.i1241)  ; FAbs(value)
  %FAbs59 = call float @dx.op.unary.f32(i32 6, float %.i2242)  ; FAbs(value)
  %FMax73 = call float @dx.op.binary.f32(i32 35, float %FAbs57, float %FAbs58)  ; FMax(a,b)
  %FMax72 = call float @dx.op.binary.f32(i32 35, float %FMax73, float %FAbs59)  ; FMax(a,b)
  %176 = fcmp fast oeq float %FMax72, %FAbs58
  %iMajorAxis.i.0 = zext i1 %176 to i32
  %177 = fcmp fast oeq float %FMax72, %FAbs59
  %iMajorAxis.i.1 = select i1 %177, i32 2, i32 %iMajorAxis.i.0
  %FMad146 = call float @dx.op.tertiary.f32(i32 46, float %19, float %144, float 0.000000e+00)  ; FMad(a,b,c)
  %178 = fadd fast float %FMad146, %147
  %FMad140 = call float @dx.op.tertiary.f32(i32 46, float %19, float %145, float 0.000000e+00)  ; FMad(a,b,c)
  %179 = fadd fast float %FMad140, %148
  %180 = fdiv fast float 1.000000e+00, %179
  %.i1244 = fmul fast float %180, %178
  %181 = fdiv fast float 2.000000e+00, %167
  %FMad99 = call float @dx.op.tertiary.f32(i32 46, float %181, float %141, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad98 = call float @dx.op.tertiary.f32(i32 46, float %19, float %144, float %FMad99)  ; FMad(a,b,c)
  %182 = fadd fast float %FMad98, %147
  %FMad93 = call float @dx.op.tertiary.f32(i32 46, float %181, float %142, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad92 = call float @dx.op.tertiary.f32(i32 46, float %19, float %145, float %FMad93)  ; FMad(a,b,c)
  %183 = fadd fast float %FMad92, %148
  %184 = fdiv fast float 1.000000e+00, %183
  %.i1247 = fmul fast float %184, %182
  %185 = fsub fast float %.i1247, %.i1244
  %FAbs = call float @dx.op.unary.f32(i32 6, float %185)  ; FAbs(value)
  %186 = fmul fast float %FAbs, 6.400000e+01
  %Log = call float @dx.op.unary.f32(i32 23, float %186)  ; Log(value)
  %Round_pi = call float @dx.op.unary.f32(i32 28, float %Log)  ; Round_pi(value)
  %Exp = call float @dx.op.unary.f32(i32 21, float %Round_pi)  ; Exp(value)
  %.i0249 = fdiv fast float %.i0173, %Exp
  %.i1250 = fdiv fast float %.i1174, %Exp
  %.i2251 = fdiv fast float %.i2175, %Exp
  %Round_ni51 = call float @dx.op.unary.f32(i32 27, float %.i0249)  ; Round_ni(value)
  %Round_ni52 = call float @dx.op.unary.f32(i32 27, float %.i1250)  ; Round_ni(value)
  %Round_ni53 = call float @dx.op.unary.f32(i32 27, float %.i2251)  ; Round_ni(value)
  %.i0252 = fptosi float %Round_ni51 to i32
  %.i1253 = fptosi float %Round_ni52 to i32
  %.i2254 = fptosi float %Round_ni53 to i32
  %.i0255 = sitofp i32 %.i0252 to float
  %.i1256 = sitofp i32 %.i1253 to float
  %.i2257 = sitofp i32 %.i2254 to float
  %.i0258 = fmul fast float %.i0255, %Exp
  %.i1259 = fmul fast float %.i1256, %Exp
  %.i2260 = fmul fast float %.i2257, %Exp
  %187 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 0
  store i32 %.i0252, i32* %187, align 4
  %188 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 1
  store i32 %.i1253, i32* %188, align 4
  %189 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 2
  store i32 %.i2254, i32* %189, align 4
  %190 = getelementptr [3 x i32], [3 x i32]* %1, i32 0, i32 %iMajorAxis.i.1
  store i32 0, i32* %190, align 4, !tbaa !522
  %191 = load i32, i32* %187, align 4
  %192 = load i32, i32* %188, align 4
  %193 = load i32, i32* %189, align 4
  %194 = shl i32 %192, 5
  %195 = xor i32 %194, %192
  %196 = shl i32 %193, 13
  %197 = xor i32 %196, %193
  %198 = add i32 %195, %191
  %199 = add i32 %198, %197
  %200 = xor i32 %199, 61
  %201 = lshr i32 %199, 16
  %202 = xor i32 %200, %201
  %203 = mul i32 %202, 9
  %204 = lshr i32 %203, 4
  %205 = xor i32 %204, %203
  %206 = mul i32 %205, 668265261
  %207 = lshr i32 %206, 15
  %208 = xor i32 %207, %206
  %209 = uitofp i32 %208 to float
  %210 = fmul fast float %209, 0x3DF0000000000000
  %211 = fdiv fast float %186, %Exp
  %212 = fmul fast float %211, 4.000000e+00
  %213 = fadd fast float %212, -2.000000e+00
  %214 = fsub fast float %213, %210
  %Saturate40 = call float @dx.op.unary.f32(i32 7, float %214)  ; Saturate(value)
  %215 = fmul fast float %Saturate40, 5.000000e-01
  %216 = fadd fast float %215, 5.000000e-01
  %217 = fmul fast float %216, %Exp
  %218 = load float, float* %168, align 4, !tbaa !520
  %219 = fsub fast float %218, %.i0258
  %220 = fcmp fast ogt float %219, %217
  %221 = fadd fast float %.i0255, 5.000000e-01
  %222 = fmul fast float %Exp, %221
  %223 = select i1 %220, float %222, float %.i0258
  %224 = load float, float* %169, align 4, !tbaa !520
  %225 = fsub fast float %224, %.i1259
  %226 = fcmp fast ogt float %225, %217
  %227 = fadd fast float %.i1256, 5.000000e-01
  %228 = fmul fast float %Exp, %227
  %229 = select i1 %226, float %228, float %.i1259
  %230 = load float, float* %170, align 4, !tbaa !520
  %231 = fsub fast float %230, %.i2260
  %232 = fcmp fast ogt float %231, %217
  %233 = fadd fast float %.i2257, 5.000000e-01
  %234 = fmul fast float %Exp, %233
  %235 = select i1 %232, float %234, float %.i2260
  %236 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 0
  store float %223, float* %236, align 4
  %237 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 1
  store float %229, float* %237, align 4
  %238 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 2
  store float %235, float* %238, align 4
  %239 = getelementptr [3 x float], [3 x float]* %2, i32 0, i32 %iMajorAxis.i.1
  store float 0.000000e+00, float* %239, align 4, !tbaa !520
  %240 = load float, float* %236, align 4
  %241 = load float, float* %237, align 4
  %242 = load float, float* %238, align 4
  %.i0345 = bitcast float %240 to i32
  %.i1346 = bitcast float %241 to i32
  %.i2347 = bitcast float %242 to i32
  %243 = shl i32 %.i1346, 5
  %244 = xor i32 %243, %.i1346
  %245 = shl i32 %.i2347, 13
  %246 = xor i32 %245, %.i2347
  %247 = add i32 %244, %.i0345
  %248 = add i32 %247, %246
  %249 = xor i32 %248, 61
  %250 = lshr i32 %248, 16
  %251 = xor i32 %249, %250
  %252 = mul i32 %251, 9
  %253 = lshr i32 %252, 4
  %254 = xor i32 %253, %252
  %255 = mul i32 %254, 668265261
  %256 = lshr i32 %255, 15
  %257 = xor i32 %256, %255
  %258 = lshr i32 %257, 16
  %.i0348 = fsub fast float %218, %223
  %.i1349 = fsub fast float %224, %229
  %.i2350 = fsub fast float %230, %235
  %259 = call float @dx.op.dot3.f32(i32 55, float %.i0348, float %.i1349, float %.i2350, float %.i0240, float %.i1241, float %.i2242)  ; Dot3(ax,ay,az,bx,by,bz)
  %.i0351 = fmul fast float %259, %.i0240
  %.i1352 = fmul fast float %259, %.i1241
  %.i2353 = fmul fast float %259, %.i2242
  %.i0354 = fadd fast float %.i0351, %223
  %.i1355 = fadd fast float %.i1352, %229
  %.i2356 = fadd fast float %.i2353, %235
  %260 = fmul fast float %.i0354, %150
  %FMad114 = call float @dx.op.tertiary.f32(i32 46, float %.i1355, float %154, float %260)  ; FMad(a,b,c)
  %FMad113 = call float @dx.op.tertiary.f32(i32 46, float %.i2356, float %158, float %FMad114)  ; FMad(a,b,c)
  %261 = fadd fast float %FMad113, %162
  %262 = fmul fast float %.i0354, %151
  %FMad111 = call float @dx.op.tertiary.f32(i32 46, float %.i1355, float %155, float %262)  ; FMad(a,b,c)
  %FMad110 = call float @dx.op.tertiary.f32(i32 46, float %.i2356, float %159, float %FMad111)  ; FMad(a,b,c)
  %263 = fadd fast float %FMad110, %163
  %264 = fmul fast float %.i0354, %152
  %FMad105 = call float @dx.op.tertiary.f32(i32 46, float %.i1355, float %156, float %264)  ; FMad(a,b,c)
  %FMad104 = call float @dx.op.tertiary.f32(i32 46, float %.i2356, float %160, float %FMad105)  ; FMad(a,b,c)
  %265 = fadd fast float %FMad104, %164
  %266 = fdiv fast float 1.000000e+00, %265
  %.i0357 = fmul fast float %261, 5.000000e-01
  %.i0360 = fmul fast float %.i0357, %266
  %.i1358 = fmul fast float %263, 5.000000e-01
  %.i1361 = fmul fast float %.i1358, %266
  %.i0362 = fadd fast float %.i0360, 5.000000e-01
  %.i1363455 = fsub fast float 5.000000e-01, %.i1361
  %.i0364 = fmul fast float %.i0362, %166
  %.i1365 = fmul fast float %.i1363455, %167
  %.i0235.neg = fsub fast float -5.000000e-01, %13
  %.i0366 = fadd fast float %.i0235.neg, %.i0364
  %.i1236.neg = fsub fast float -5.000000e-01, %14
  %.i1367 = fadd fast float %.i1236.neg, %.i1365
  %Round_ni = call float @dx.op.unary.f32(i32 27, float %.i0366)  ; Round_ni(value)
  %Round_ni46 = call float @dx.op.unary.f32(i32 27, float %.i1367)  ; Round_ni(value)
  %.i0368 = fptosi float %Round_ni to i32
  %.i1369 = fptosi float %Round_ni46 to i32
  %.i0370 = add i32 %.i0368, %258
  %.i1371 = add i32 %.i1369, %257
  %.i0372 = and i32 %.i0370, 255
  %.i1373 = and i32 %.i1371, 255
  %267 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %4)  ; CreateHandleForLib(Resource)
  %TextureLoad153 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %267, i32 0, i32 %.i0372, i32 %.i1373, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %.i2268 = extractvalue %dx.types.ResRet.f32 %TextureLoad153, 2
  %268 = icmp sgt i32 %139, 0
  br i1 %268, label %.lr.ph30.preheader, label %._crit_edge.31

.lr.ph30:                                         ; preds = %.lr.ph30.preheader, %329
  %reflectionRayIndex.029 = phi i32 [ %332, %329 ], [ 0, %.lr.ph30.preheader ]
  %269 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)  ; CBufferLoadLegacy(handle,regIndex)
  %270 = extractvalue %dx.types.CBufRet.i32 %269, 1
  %271 = mul i32 %270, %139
  %272 = add i32 %271, %reflectionRayIndex.029
  %273 = uitofp i32 %272 to float
  %.i2263 = fmul fast float %273, 0x3FF9E377A0000000
  %.i2269 = fadd fast float %.i2263, %.i2268
  %Frc65 = call float @dx.op.unary.f32(i32 22, float %.i2269)  ; Frc(value)
  br i1 %346, label %.lr.ph.preheader, label %._crit_edge

.lr.ph.preheader:                                 ; preds = %.lr.ph30
  br label %.lr.ph

.lr.ph:                                           ; preds = %.lr.ph.preheader
  br label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph, %.lr.ph30
  %274 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %275 = extractvalue %dx.types.CBufRet.f32 %274, 0
  %276 = extractvalue %dx.types.CBufRet.f32 %274, 1
  %277 = extractvalue %dx.types.CBufRet.f32 %274, 2
  %278 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %279 = extractvalue %dx.types.CBufRet.f32 %278, 0
  %280 = extractvalue %dx.types.CBufRet.f32 %278, 1
  %281 = extractvalue %dx.types.CBufRet.f32 %278, 2
  %282 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %283 = extractvalue %dx.types.CBufRet.f32 %282, 0
  %284 = extractvalue %dx.types.CBufRet.f32 %282, 1
  %285 = extractvalue %dx.types.CBufRet.f32 %282, 2
  %286 = fmul fast float %275, %.i0297
  %FMad138 = call float @dx.op.tertiary.f32(i32 46, float %.i1298, float %279, float %286)  ; FMad(a,b,c)
  %FMad137 = call float @dx.op.tertiary.f32(i32 46, float %.i2299, float %283, float %FMad138)  ; FMad(a,b,c)
  %287 = insertelement <3 x float> undef, float %FMad137, i64 0
  %288 = fmul fast float %276, %.i0297
  %FMad136 = call float @dx.op.tertiary.f32(i32 46, float %.i1298, float %280, float %288)  ; FMad(a,b,c)
  %FMad135 = call float @dx.op.tertiary.f32(i32 46, float %.i2299, float %284, float %FMad136)  ; FMad(a,b,c)
  %289 = insertelement <3 x float> %287, float %FMad135, i64 1
  %290 = fmul fast float %277, %.i0297
  %FMad134 = call float @dx.op.tertiary.f32(i32 46, float %.i1298, float %281, float %290)  ; FMad(a,b,c)
  %FMad133 = call float @dx.op.tertiary.f32(i32 46, float %.i2299, float %285, float %FMad134)  ; FMad(a,b,c)
  %291 = insertelement <3 x float> %289, float %FMad133, i64 2
  %292 = fadd fast float %Frc65, 5.000000e-01
  %293 = fmul fast float %292, 0x4066666660000000
  %294 = fadd fast float %293, 0x4066666660000000
  store i32 %reflectionRayIndex.029, i32* %335, align 8
  %295 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %3)  ; CreateHandleForLib(Resource)
  %296 = load <3 x float>, <3 x float>* %136, align 4
  %297 = extractelement <3 x float> %296, i64 0
  %298 = extractelement <3 x float> %296, i64 1
  %299 = extractelement <3 x float> %296, i64 2
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %295, i32 0, i32 1, i32 0, i32 2, i32 0, float %297, float %298, float %299, float %336, float %FMad137, float %FMad135, float %FMad133, float %294, %struct.HitData* nonnull %12)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  %300 = load i32, i32* %335, align 8
  %301 = icmp eq i32 %300, 0
  br i1 %301, label %329, label %302

; <label>:302                                     ; preds = %._crit_edge
  %303 = uitofp i32 %300 to float
  %304 = fmul fast float %303, 0x3F1A36E2E0000000
  %.i0334 = fmul fast float %FMad137, %304
  %.i1336 = fmul fast float %FMad135, %304
  %.i2338 = fmul fast float %FMad133, %304
  %.i0339 = extractelement <3 x float> %296, i32 0
  %.i0340 = fadd fast float %.i0339, %.i0334
  %.i1341 = extractelement <3 x float> %296, i32 1
  %.i1342 = fadd fast float %.i1341, %.i1336
  %.i2343 = extractelement <3 x float> %296, i32 2
  %.i2344 = fadd fast float %.i2343, %.i2338
  %.upto0437 = insertelement <3 x float> undef, float %.i0340, i32 0
  %.upto1438 = insertelement <3 x float> %.upto0437, float %.i1342, i32 1
  %305 = insertelement <3 x float> %.upto1438, float %.i2344, i32 2
  store <3 x float> %305, <3 x float>* %136, align 4, !tbaa !517
  %306 = call %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<DeferredLightSun>"(i32 160, %"class.StructuredBuffer<DeferredLightSun>" %5)  ; CreateHandleForLib(Resource)
  %RawBufferLoad = call %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32 139, %dx.types.Handle %306, i32 0, i32 0, i8 7, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %307 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad, 0
  %308 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad, 1
  %309 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad, 2
  %310 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %311 = extractvalue %dx.types.CBufRet.f32 %310, 0
  %312 = extractvalue %dx.types.CBufRet.f32 %310, 1
  %313 = extractvalue %dx.types.CBufRet.f32 %310, 2
  %314 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %315 = extractvalue %dx.types.CBufRet.f32 %314, 0
  %316 = extractvalue %dx.types.CBufRet.f32 %314, 1
  %317 = extractvalue %dx.types.CBufRet.f32 %314, 2
  %318 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %319 = extractvalue %dx.types.CBufRet.f32 %318, 0
  %320 = extractvalue %dx.types.CBufRet.f32 %318, 1
  %321 = extractvalue %dx.types.CBufRet.f32 %318, 2
  %322 = fmul fast float %311, %307
  %FMad90 = call float @dx.op.tertiary.f32(i32 46, float %308, float %315, float %322)  ; FMad(a,b,c)
  %FMad89 = call float @dx.op.tertiary.f32(i32 46, float %309, float %319, float %FMad90)  ; FMad(a,b,c)
  %323 = insertelement <3 x float> undef, float %FMad89, i64 0
  %324 = fmul fast float %312, %307
  %FMad88 = call float @dx.op.tertiary.f32(i32 46, float %308, float %316, float %324)  ; FMad(a,b,c)
  %FMad87 = call float @dx.op.tertiary.f32(i32 46, float %309, float %320, float %FMad88)  ; FMad(a,b,c)
  %325 = insertelement <3 x float> %323, float %FMad87, i64 1
  %326 = fmul fast float %313, %307
  %FMad86 = call float @dx.op.tertiary.f32(i32 46, float %308, float %317, float %326)  ; FMad(a,b,c)
  %FMad85 = call float @dx.op.tertiary.f32(i32 46, float %309, float %321, float %FMad86)  ; FMad(a,b,c)
  %327 = insertelement <3 x float> %325, float %FMad85, i64 2
  store i32 %reflectionRayIndex.029, i32* %337, align 8
  %328 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %3)  ; CreateHandleForLib(Resource)
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %328, i32 4, i32 1, i32 1, i32 2, i32 1, float %.i0340, float %.i1342, float %.i2344, float %336, float %FMad89, float %FMad87, float %FMad85, float 1.000000e+03, %struct.HitData* nonnull %11)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  br label %329

; <label>:329                                     ; preds = %302, %._crit_edge
  %330 = phi float [ %294, %._crit_edge ], [ 1.000000e+03, %302 ]
  %331 = phi <3 x float> [ %291, %._crit_edge ], [ %327, %302 ]
  %332 = add nuw nsw i32 %reflectionRayIndex.029, 1
  %exitcond = icmp eq i32 %332, %139
  br i1 %exitcond, label %._crit_edge.31.loopexit, label %.lr.ph30

._crit_edge.31.loopexit:                          ; preds = %329
  %.lcssa461 = phi <3 x float> [ %331, %329 ]
  %.lcssa = phi float [ %330, %329 ]
  %333 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 3
  store <3 x float> %.lcssa461, <3 x float>* %334, align 4
  store float %.lcssa, float* %333, align 4
  br label %._crit_edge.31

._crit_edge.31:                                   ; preds = %._crit_edge.31.loopexit, %"\01?internal.getWorldSpaceNoiseOffset@@YA?AV?$vector@I$01@@V?$vector@M$01@@V?$matrix@M$03$03@@10MV?$vector@M$02@@22M_N@Z.exit33"
  ret void

.lr.ph30.preheader:                               ; preds = %"\01?internal.getWorldSpaceNoiseOffset@@YA?AV?$vector@I$01@@V?$vector@M$01@@V?$matrix@M$03$03@@10MV?$vector@M$02@@22M_N@Z.exit33"
  %.i0273 = fsub fast float -0.000000e+00, %.i0166
  %.i1274 = fsub fast float -0.000000e+00, %.i1167
  %.i2275 = fsub fast float -0.000000e+00, %.i2168
  %334 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 2
  %335 = getelementptr inbounds %struct.HitData, %struct.HitData* %12, i32 0, i32 0
  %336 = load float, float* %137, align 4
  %337 = getelementptr inbounds %struct.HitData, %struct.HitData* %11, i32 0, i32 0
  %338 = call float @dx.op.dot3.f32(i32 55, float %.i0273, float %.i1274, float %.i2275, float %.i0212, float %.i1213, float %.i2214)  ; Dot3(ax,ay,az,bx,by,bz)
  %339 = fmul fast float %338, 2.000000e+00
  %.i0291 = fmul fast float %339, %.i0212
  %.i1292 = fmul fast float %339, %.i1213
  %.i2293 = fmul fast float %.i2214, %339
  %.i0294 = fadd fast float %.i0291, %.i0166
  %.i1295 = fadd fast float %.i1292, %.i1167
  %.i2296 = fadd fast float %.i2293, %.i2168
  %340 = fmul fast float %.i0294, %.i0294
  %341 = fmul fast float %.i1295, %.i1295
  %342 = fadd fast float %340, %341
  %343 = fmul fast float %.i2296, %.i2296
  %344 = fadd fast float %342, %343
  %Sqrt69 = call float @dx.op.unary.f32(i32 24, float %344)  ; Sqrt(value)
  %.i0297 = fdiv fast float %.i0294, %Sqrt69
  %.i1298 = fdiv fast float %.i1295, %Sqrt69
  %.i2299 = fdiv fast float %.i2296, %Sqrt69
  %345 = call float @dx.op.dot3.f32(i32 55, float %.i0297, float %.i1298, float %.i2299, float %.i0212, float %.i1213, float %.i2214)  ; Dot3(ax,ay,az,bx,by,bz)
  %346 = fcmp fast olt float %345, 0x3F747AE140000000
  br label %.lr.ph30
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
declare %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32, %dx.types.Handle, i32, i32, i8, i32) #2

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
!dx.typeAnnotations = !{!20, !493}
!dx.entryPoints = !{!503, !506, !508, !510, !512, !514, !515, !516}

!0 = !{!"dxc 1.2"}
!1 = !{i32 1, i32 3}
!2 = !{!"lib", i32 6, i32 3}
!3 = !{!4, !14, !17, null}
!4 = !{!5, !7, !8, !9, !11, !12}
!5 = !{i32 0, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer1", i32 0, i32 0, i32 1, i32 2, i32 0, !6}
!6 = !{i32 0, i32 9}
!7 = !{i32 1, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tLinearDepth", i32 0, i32 1, i32 1, i32 2, i32 0, !6}
!8 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tClipDepth", i32 0, i32 2, i32 1, i32 2, i32 0, !6}
!9 = !{i32 3, %"class.StructuredBuffer<DeferredLightSun>"* @"\01?g_sbDeferredLightSunData@@3V?$StructuredBuffer@UDeferredLightSun@@@@A", !"g_sbDeferredLightSunData", i32 0, i32 3, i32 1, i32 12, i32 0, !10}
!10 = !{i32 1, i32 576}
!11 = !{i32 4, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tStaticBlueNoiseRGBA_0", i32 0, i32 4, i32 1, i32 2, i32 0, !6}
!12 = !{i32 5, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", !"g_rtScene", i32 0, i32 5, i32 1, i32 16, i32 0, !13}
!13 = !{i32 0, i32 4}
!14 = !{!15}
!15 = !{i32 0, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", !"g_rwtShadow", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !16}
!16 = !{i32 0, i32 5}
!17 = !{!18, !19}
!18 = !{i32 0, %sys_constants* @sys_constants, !"sys_constants", i32 0, i32 0, i32 1, i32 1128, null}
!19 = !{i32 1, %rtreflection* @rtreflection, !"rtreflection", i32 0, i32 1, i32 1, i32 24, null}
!20 = !{i32 0, %"class.Texture2D<vector<float, 4> >" undef, !21, %"class.Texture2D<vector<float, 4> >::mips_type" undef, !24, %struct.GBufferFormat undef, !26, %struct.SamplerState undef, !29, %"class.Texture2D<float>" undef, !31, %"class.Texture2D<float>::mips_type" undef, !24, %"class.Texture2D<unsigned int>" undef, !33, %"class.Texture2D<unsigned int>::mips_type" undef, !24, %"class.Texture2DMS<float, 0>" undef, !35, %"class.Texture2DMS<float, 0>::sample_type" undef, !24, %"class.StructuredBuffer<vector<unsigned int, 4> >" undef, !37, %"class.StructuredBuffer<MaterialDataPart1>" undef, !38, %struct.MaterialDataPart1 undef, !40, %"class.StructuredBuffer<MaterialDataPart3>" undef, !43, %struct.MaterialDataPart3 undef, !44, %"class.StructuredBuffer<MaterialBindingData>" undef, !88, %struct.MaterialBindingData undef, !89, %struct.order2_sh undef, !91, %"class.RWTexture3D<vector<float, 4> >" undef, !96, %"class.Texture3D<vector<float, 4> >" undef, !21, %"class.Texture3D<vector<float, 4> >::mips_type" undef, !24, %struct.LightVolumeData undef, !97, %"class.TextureCube<vector<float, 4> >" undef, !96, %"class.StructuredBuffer<DeferredLightPoint>" undef, !100, %struct.DeferredLightPoint undef, !101, %"class.StructuredBuffer<DeferredLightPointClipping>" undef, !116, %struct.DeferredLightPointClipping undef, !117, %"class.StructuredBuffer<DeferredLightPointBVH>" undef, !119, %struct.DeferredLightPointBVH undef, !120, %"class.RWTexture2D<unsigned int>" undef, !127, %"class.StructuredBuffer<DeferredLightSpot>" undef, !128, %struct.DeferredLightSpot undef, !129, %"class.StructuredBuffer<DeferredLightSpotClipping>" undef, !154, %struct.DeferredLightSpotClipping undef, !155, %"class.StructuredBuffer<DeferredLightSpotBVH>" undef, !119, %struct.DeferredLightSpotBVH undef, !120, %"class.StructuredBuffer<DeferredLightSun>" undef, !160, %struct.DeferredLightSun undef, !161, %"class.StructuredBuffer<VolumeSamplingInfo>" undef, !175, %struct.VolumeSamplingInfo undef, !176, %"class.StructuredBuffer<unsigned int>" undef, !127, %"class.StructuredBuffer<InternalNode>" undef, !116, %struct.InternalNode undef, !186, %"class.Texture3D<vector<float, 3> >" undef, !191, %"class.Texture3D<vector<float, 3> >::mips_type" undef, !24, %struct.VolumeCullingInCB undef, !193, %struct.VolumeTopLevelInfo undef, !198, %struct.DeferredLightSpecularBRDF undef, !202, %struct.DeferredLightGenericSurface undef, !205, %struct.DeferredLightTransparentBRDF undef, !209, %struct.DeferredLightIntensity undef, !212, %struct.DeferredLightVolumeResult undef, !215, %struct.DeferredLightPointSurface undef, !217, %struct.DeferredLightGBufferData undef, !218, %struct.CellInfo undef, !225, %"class.StructuredBuffer<EnvironmentMapDynamicInfo>" undef, !88, %struct.EnvironmentMapDynamicInfo undef, !251, %"class.StructuredBuffer<EnvironmentMapReference>" undef, !88, %struct.EnvironmentMapReference undef, !253, %"class.StructuredBuffer<CellInfo>" undef, !255, %"class.StructuredBuffer<Node>" undef, !256, %struct.Node undef, !257, %struct.ChildMask undef, !261, %struct.NodePointer undef, !263, %"class.StructuredBuffer<IrradianceProbe>" undef, !265, %struct.IrradianceProbe undef, !266, %"class.StructuredBuffer<TransportProbe>" undef, !276, %struct.TransportProbe undef, !277, %"class.StructuredBuffer<vector<float, 4> >" undef, !96, %"class.StructuredBuffer<ProbeRef>" undef, !38, %struct.ProbeRef undef, !279, %"class.StructuredBuffer<EnvironmentMapInfo>" undef, !282, %struct.EnvironmentMapInfo undef, !283, %struct.VoxelInfo undef, !289, %struct.LeafInfo undef, !295, %"class.StructuredBuffer<PerInstanceLBData>" undef, !116, %struct.PerInstanceLBData undef, !303, %"class.StructuredBuffer<RenderInstanceData>" undef, !307, %struct.RenderInstanceData undef, !308, %struct.ByteAddressBuffer undef, !29, %struct.RaytracingHitRootConstants undef, !316, %struct.RaytracingAccelerationStructure undef, !29, %struct.IntersectionAttributes undef, !327, %struct.HitData undef, !329, %"class.RWTexture2DArray<unsigned int>" undef, !127, %"class.RWTexture2DArray<vector<float, 4> >" undef, !96, %"class.RWStructuredBuffer<unsigned int>" undef, !127, %struct.RayDesc undef, !331, %debug_general undef, !336, %sys_constants undef, !359, %shadow_general undef, !392, %mid_translucency undef, !397, %mid_general undef, !89, %deferredlight_constants undef, !399, %env_general undef, !435, %atmosphere_general undef, !438, %illuminationvolume undef, !459, %ManualUpdateCB_ActiveVolumeTopLevelInfoArray undef, !466, %ManualUpdateCB_DebugVolumeTopLevelInfoArray undef, !468, %WorldGridInfo undef, !470, %EnvironmentMapAtlas undef, !474, %vertex_binding undef, !480, %g_cbRaytracingHit undef, !484, %rtreflection undef, !486}
!21 = !{i32 20, !22, !23}
!22 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 9}
!23 = !{i32 6, !"mips", i32 3, i32 16}
!24 = !{i32 4, !25}
!25 = !{i32 6, !"handle", i32 3, i32 0, i32 7, i32 5}
!26 = !{i32 32, !27, !28}
!27 = !{i32 6, !"vTarget1", i32 3, i32 0, i32 4, !"SV_Target0", i32 7, i32 9}
!28 = !{i32 6, !"vTarget2", i32 3, i32 16, i32 4, !"SV_Target1", i32 7, i32 9}
!29 = !{i32 4, !30}
!30 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 4}
!31 = !{i32 8, !22, !32}
!32 = !{i32 6, !"mips", i32 3, i32 4}
!33 = !{i32 8, !34, !32}
!34 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 5}
!35 = !{i32 8, !22, !36}
!36 = !{i32 6, !"sample", i32 3, i32 4}
!37 = !{i32 16, !34}
!38 = !{i32 8, !39}
!39 = !{i32 6, !"h", i32 3, i32 0}
!40 = !{i32 8, !41, !42}
!41 = !{i32 6, !"vSpecularColor_uBRDF", i32 3, i32 0, i32 7, i32 5}
!42 = !{i32 6, !"uScatterIndex_vTranslucencyDepthRange", i32 3, i32 4, i32 7, i32 5}
!43 = !{i32 352, !39}
!44 = !{i32 352, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !87}
!45 = !{i32 6, !"fClothDiffScatterAmount", i32 3, i32 0, i32 7, i32 9}
!46 = !{i32 6, !"fClothFuzzWrap", i32 3, i32 4, i32 7, i32 9}
!47 = !{i32 6, !"vClothScatterColor", i32 3, i32 16, i32 7, i32 9}
!48 = !{i32 6, !"fHairSmoothness", i32 3, i32 28, i32 7, i32 9}
!49 = !{i32 6, !"vSpecularColor2", i32 3, i32 32, i32 7, i32 9}
!50 = !{i32 6, !"fHairSpecularShift", i32 3, i32 44, i32 7, i32 9}
!51 = !{i32 6, !"fHairDiffuseWrap", i32 3, i32 48, i32 7, i32 9}
!52 = !{i32 6, !"fPAD1", i32 3, i32 52, i32 7, i32 9}
!53 = !{i32 6, !"fEmissionIntensity", i32 3, i32 56, i32 7, i32 9}
!54 = !{i32 6, !"vColorMultiplier", i32 3, i32 64, i32 7, i32 9}
!55 = !{i32 6, !"vSmoothnessRange", i32 3, i32 80, i32 7, i32 9}
!56 = !{i32 6, !"vShadowSoftnessRange", i32 3, i32 88, i32 7, i32 9}
!57 = !{i32 6, !"vScatterWidthRange", i32 3, i32 96, i32 7, i32 9}
!58 = !{i32 6, !"fHairDepthOffsetAmount", i32 3, i32 104, i32 7, i32 9}
!59 = !{i32 6, !"fHairNormalAmount", i32 3, i32 108, i32 7, i32 9}
!60 = !{i32 6, !"vEmissionMultiplier", i32 3, i32 112, i32 7, i32 9}
!61 = !{i32 6, !"vColorMultiplier2", i32 3, i32 128, i32 7, i32 9}
!62 = !{i32 6, !"vBaseDetailUVScale", i32 3, i32 144, i32 7, i32 9}
!63 = !{i32 6, !"vSmoothnessRange2", i32 3, i32 152, i32 7, i32 9}
!64 = !{i32 6, !"vBlendDetailUVScale", i32 3, i32 160, i32 7, i32 9}
!65 = !{i32 6, !"fBaseDetailAmount", i32 3, i32 168, i32 7, i32 9}
!66 = !{i32 6, !"fBlendUVMultiplier", i32 3, i32 172, i32 7, i32 9}
!67 = !{i32 6, !"fBlendDetailAmount", i32 3, i32 176, i32 7, i32 9}
!68 = !{i32 6, !"fFuzzUVMultiplier", i32 3, i32 180, i32 7, i32 9}
!69 = !{i32 6, !"fPupilDilation", i32 3, i32 184, i32 7, i32 9}
!70 = !{i32 6, !"fPupilOuterRadius", i32 3, i32 188, i32 7, i32 9}
!71 = !{i32 6, !"vIntensity", i32 3, i32 192, i32 7, i32 9}
!72 = !{i32 6, !"vWrinkleWeights0", i32 3, i32 208, i32 7, i32 9}
!73 = !{i32 6, !"vWrinkleWeights1", i32 3, i32 224, i32 7, i32 9}
!74 = !{i32 6, !"vWrinkleWeights2", i32 3, i32 240, i32 7, i32 9}
!75 = !{i32 6, !"vWrinkleWeights3", i32 3, i32 256, i32 7, i32 9}
!76 = !{i32 6, !"vWrinkleWeights4", i32 3, i32 272, i32 7, i32 9}
!77 = !{i32 6, !"vWrinkleWeights5", i32 3, i32 288, i32 7, i32 9}
!78 = !{i32 6, !"uDisableBackfaceFlip", i32 3, i32 304, i32 7, i32 5}
!79 = !{i32 6, !"fEdgeSharpness", i32 3, i32 308, i32 7, i32 9}
!80 = !{i32 6, !"fTrunkBendFactor", i32 3, i32 312, i32 7, i32 9}
!81 = !{i32 6, !"fTrunkPivotOffset", i32 3, i32 316, i32 7, i32 9}
!82 = !{i32 6, !"vColorRgbMultiplier", i32 3, i32 320, i32 7, i32 9}
!83 = !{i32 6, !"fWindFactor", i32 3, i32 332, i32 7, i32 9}
!84 = !{i32 6, !"fDecalMaterialBlendFactor", i32 3, i32 336, i32 7, i32 9}
!85 = !{i32 6, !"fDecalAlbedoBlendFactor", i32 3, i32 340, i32 7, i32 9}
!86 = !{i32 6, !"uStableHash", i32 3, i32 344, i32 7, i32 5}
!87 = !{i32 6, !"pad", i32 3, i32 348, i32 7, i32 5}
!88 = !{i32 4, !39}
!89 = !{i32 4, !90}
!90 = !{i32 6, !"g_uMaterialID", i32 3, i32 0, i32 7, i32 5}
!91 = !{i32 60, !92, !93, !94, !95}
!92 = !{i32 6, !"c0", i32 3, i32 0, i32 7, i32 9}
!93 = !{i32 6, !"c11", i32 3, i32 16, i32 7, i32 9}
!94 = !{i32 6, !"c10", i32 3, i32 32, i32 7, i32 9}
!95 = !{i32 6, !"c1m1", i32 3, i32 48, i32 7, i32 9}
!96 = !{i32 16, !22}
!97 = !{i32 32, !98, !99}
!98 = !{i32 6, !"value0", i32 3, i32 0, i32 7, i32 9}
!99 = !{i32 6, !"value1", i32 3, i32 16, i32 7, i32 9}
!100 = !{i32 160, !39}
!101 = !{i32 160, !102, !103, !104, !105, !106, !107, !108, !109, !111, !112, !113, !114, !115}
!102 = !{i32 6, !"vPositionInView", i32 3, i32 0, i32 7, i32 9}
!103 = !{i32 6, !"fShadowMapShrink", i32 3, i32 12, i32 7, i32 9}
!104 = !{i32 6, !"vColor", i32 3, i32 16, i32 7, i32 9}
!105 = !{i32 6, !"fClipRange", i32 3, i32 28, i32 7, i32 9}
!106 = !{i32 6, !"vFalloff", i32 3, i32 32, i32 7, i32 9}
!107 = !{i32 6, !"vDirectionInView", i32 3, i32 48, i32 7, i32 9}
!108 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 60, i32 7, i32 9}
!109 = !{i32 6, !"mViewToShadowClip", i32 2, !110, i32 3, i32 64, i32 7, i32 9}
!110 = !{i32 4, i32 4, i32 2}
!111 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 128, i32 7, i32 9}
!112 = !{i32 6, !"fRadius", i32 3, i32 144, i32 7, i32 9}
!113 = !{i32 6, !"fBoundsRadius", i32 3, i32 148, i32 7, i32 9}
!114 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 152, i32 7, i32 9}
!115 = !{i32 6, !"uTechniqueProperties", i32 3, i32 156, i32 7, i32 5}
!116 = !{i32 16, !39}
!117 = !{i32 16, !102, !118}
!118 = !{i32 6, !"fBoundsRadius", i32 3, i32 12, i32 7, i32 9}
!119 = !{i32 44, !39}
!120 = !{i32 44, !121, !122, !123, !124, !125, !126}
!121 = !{i32 6, !"vMinInView", i32 3, i32 0, i32 7, i32 9}
!122 = !{i32 6, !"vMaxInView", i32 3, i32 16, i32 7, i32 9}
!123 = !{i32 6, !"uChildLHS", i32 3, i32 28, i32 7, i32 5}
!124 = !{i32 6, !"uChildRHS", i32 3, i32 32, i32 7, i32 5}
!125 = !{i32 6, !"uLightIndex", i32 3, i32 36, i32 7, i32 5}
!126 = !{i32 6, !"uCount", i32 3, i32 40, i32 7, i32 5}
!127 = !{i32 4, !34}
!128 = !{i32 336, !39}
!129 = !{i32 336, !102, !130, !131, !132, !133, !134, !135, !136, !137, !138, !139, !140, !141, !142, !143, !145, !146, !147, !148, !149, !150, !151, !152, !153}
!130 = !{i32 6, !"fNearDistance", i32 3, i32 12, i32 7, i32 9}
!131 = !{i32 6, !"vWedPositionInView", i32 3, i32 16, i32 7, i32 9}
!132 = !{i32 6, !"fFarDistance", i32 3, i32 28, i32 7, i32 9}
!133 = !{i32 6, !"vDirectionInView", i32 3, i32 32, i32 7, i32 9}
!134 = !{i32 6, !"fFarWidth", i32 3, i32 44, i32 7, i32 9}
!135 = !{i32 6, !"vColor", i32 3, i32 48, i32 7, i32 9}
!136 = !{i32 6, !"fClipRange", i32 3, i32 60, i32 7, i32 9}
!137 = !{i32 6, !"vFalloff", i32 3, i32 64, i32 7, i32 9}
!138 = !{i32 6, !"fRadius", i32 3, i32 72, i32 7, i32 9}
!139 = !{i32 6, !"fSinConeAnglePerTwo_sq", i32 3, i32 76, i32 7, i32 9}
!140 = !{i32 6, !"mViewToProjectionClip", i32 2, !110, i32 3, i32 80, i32 7, i32 9}
!141 = !{i32 6, !"mCameraPositionInCone", i32 3, i32 144, i32 7, i32 9}
!142 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 156, i32 7, i32 9}
!143 = !{i32 6, !"mViewToCone", i32 2, !144, i32 3, i32 160, i32 7, i32 9}
!144 = !{i32 3, i32 3, i32 2}
!145 = !{i32 6, !"fShadowPCFSize", i32 3, i32 204, i32 7, i32 9}
!146 = !{i32 6, !"fShadowLOD", i32 3, i32 208, i32 7, i32 9}
!147 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 212, i32 7, i32 9}
!148 = !{i32 6, !"mConeToView", i32 3, i32 224, i32 7, i32 9}
!149 = !{i32 6, !"vProjectionAtlasOffsetScale", i32 3, i32 272, i32 7, i32 9}
!150 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 288, i32 7, i32 9}
!151 = !{i32 6, !"vDynamicShadowAtlasOffsetScale", i32 3, i32 304, i32 7, i32 9}
!152 = !{i32 6, !"uTechniqueProperties", i32 3, i32 320, i32 7, i32 5}
!153 = !{i32 6, !"padTo16bytes", i32 3, i32 324, i32 7, i32 5}
!154 = !{i32 48, !39}
!155 = !{i32 48, !102, !130, !156, !132, !157, !158, !159}
!156 = !{i32 6, !"vDirectionInView", i32 3, i32 16, i32 7, i32 9}
!157 = !{i32 6, !"fFarWidth", i32 3, i32 32, i32 7, i32 9}
!158 = !{i32 6, !"uTechniqueProperties", i32 3, i32 36, i32 7, i32 5}
!159 = !{i32 6, !"padTo16bytes", i32 3, i32 40, i32 7, i32 5}
!160 = !{i32 708, !39}
!161 = !{i32 708, !162, !163, !104, !164, !165, !166, !167, !168, !169, !170, !171, !172, !173, !174}
!162 = !{i32 6, !"vDirectionInView", i32 3, i32 0, i32 7, i32 9}
!163 = !{i32 6, !"pad1", i32 3, i32 12, i32 7, i32 4}
!164 = !{i32 6, !"pad2", i32 3, i32 28, i32 7, i32 4}
!165 = !{i32 6, !"mViewToProjectionClip", i32 2, !110, i32 3, i32 32, i32 7, i32 9}
!166 = !{i32 6, !"fProjectionMapTransparency", i32 3, i32 96, i32 7, i32 9}
!167 = !{i32 6, !"fRadius", i32 3, i32 100, i32 7, i32 9}
!168 = !{i32 6, !"uTechniqueProperties", i32 3, i32 104, i32 7, i32 5}
!169 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 108, i32 7, i32 9}
!170 = !{i32 6, !"uCascadeCount", i32 3, i32 112, i32 7, i32 5}
!171 = !{i32 6, !"mCascadeViewToShadowClip", i32 2, !110, i32 3, i32 128, i32 7, i32 9}
!172 = !{i32 6, !"vCascadeRange", i32 3, i32 512, i32 7, i32 9}
!173 = !{i32 6, !"fCascadePCFWidth", i32 3, i32 608, i32 7, i32 9}
!174 = !{i32 6, !"pad3", i32 3, i32 704, i32 7, i32 4}
!175 = !{i32 92, !39}
!176 = !{i32 92, !177, !178, !179, !180, !181, !182, !183, !184, !185}
!177 = !{i32 6, !"mWorldToLocalRotationMatrix", i32 2, !144, i32 3, i32 0, i32 7, i32 9}
!178 = !{i32 6, !"vOneOverTopLevelFadeLength", i32 3, i32 48, i32 7, i32 9}
!179 = !{i32 6, !"uTopLevelNodeIndicesOffset", i32 3, i32 60, i32 7, i32 5}
!180 = !{i32 6, !"uInternalNodeIndexOffset", i32 3, i32 64, i32 7, i32 5}
!181 = !{i32 6, !"uBrickIndexOffset", i32 3, i32 68, i32 7, i32 5}
!182 = !{i32 6, !"fTopLevelTexelSizeInWorld", i32 3, i32 72, i32 7, i32 9}
!183 = !{i32 6, !"fWeightMultiplier", i32 3, i32 76, i32 7, i32 9}
!184 = !{i32 6, !"uAdditive", i32 3, i32 80, i32 7, i32 5}
!185 = !{i32 6, !"fpadTo16Bytes", i32 3, i32 84, i32 7, i32 9}
!186 = !{i32 16, !187, !188, !189, !190}
!187 = !{i32 6, !"uChildMask", i32 3, i32 0, i32 7, i32 5}
!188 = !{i32 6, !"uLeafChildMask", i32 3, i32 4, i32 7, i32 5}
!189 = !{i32 6, !"uChildBlockOffset", i32 3, i32 8, i32 7, i32 5}
!190 = !{i32 6, !"uLeafChildBlockOffset", i32 3, i32 12, i32 7, i32 5}
!191 = !{i32 16, !22, !192}
!192 = !{i32 6, !"mips", i32 3, i32 12}
!193 = !{i32 32, !194, !195, !196, !197}
!194 = !{i32 6, !"vAABBMinInView", i32 3, i32 0, i32 7, i32 9}
!195 = !{i32 6, !"fPad0", i32 3, i32 12, i32 7, i32 9}
!196 = !{i32 6, !"vAABBMaxInView", i32 3, i32 16, i32 7, i32 9}
!197 = !{i32 6, !"fPad1", i32 3, i32 28, i32 7, i32 9}
!198 = !{i32 64, !199, !200, !201}
!199 = !{i32 6, !"mWorldToTopLevelMatrixDecomp", i32 3, i32 0, i32 7, i32 9}
!200 = !{i32 6, !"vTopLevelSize", i32 3, i32 48, i32 7, i32 5}
!201 = !{i32 6, !"fPad0", i32 3, i32 60, i32 7, i32 9}
!202 = !{i32 28, !203, !204}
!203 = !{i32 6, !"vResultDiffuse", i32 3, i32 0, i32 7, i32 9}
!204 = !{i32 6, !"vResultSpecular", i32 3, i32 16, i32 7, i32 9}
!205 = !{i32 36, !102, !206, !207, !208}
!206 = !{i32 6, !"vNormalInView", i32 3, i32 16, i32 7, i32 9}
!207 = !{i32 6, !"fSmoothness", i32 3, i32 28, i32 7, i32 9}
!208 = !{i32 6, !"fSpecularAlbedoIntensity", i32 3, i32 32, i32 7, i32 9}
!209 = !{i32 44, !203, !210, !211}
!210 = !{i32 6, !"vResultDiffuseTransmission", i32 3, i32 16, i32 7, i32 9}
!211 = !{i32 6, !"vResultSpecular", i32 3, i32 32, i32 7, i32 9}
!212 = !{i32 28, !213, !214}
!213 = !{i32 6, !"vIntensityDiffuse", i32 3, i32 0, i32 7, i32 9}
!214 = !{i32 6, !"vIntensitySpecular", i32 3, i32 16, i32 7, i32 9}
!215 = !{i32 32, !216}
!216 = !{i32 6, !"d", i32 3, i32 0}
!217 = !{i32 12, !102}
!218 = !{i32 68, !219, !220, !221, !222, !223, !224}
!219 = !{i32 6, !"vGBufferTarget1", i32 3, i32 0, i32 7, i32 9}
!220 = !{i32 6, !"vGBufferTarget2", i32 3, i32 16, i32 7, i32 9}
!221 = !{i32 6, !"vPositionInView", i32 3, i32 32, i32 7, i32 9}
!222 = !{i32 6, !"vBentNormalInView", i32 3, i32 48, i32 7, i32 9}
!223 = !{i32 6, !"fDiffuseOcclusion", i32 3, i32 60, i32 7, i32 9}
!224 = !{i32 6, !"fSpecularOcclusion", i32 3, i32 64, i32 7, i32 9}
!225 = !{i32 356, !226, !227, !228, !229, !230, !231, !232, !233, !234, !235, !236, !237, !238, !239, !240, !241, !242, !243, !244, !245, !246, !247, !248, !249, !250}
!226 = !{i32 6, !"m_vMin", i32 3, i32 0, i32 7, i32 9}
!227 = !{i32 6, !"m_fBaseVoxelSize", i32 3, i32 12, i32 7, i32 9}
!228 = !{i32 6, !"m_vMax", i32 3, i32 16, i32 7, i32 9}
!229 = !{i32 6, !"m_fInvBaseVoxelSize", i32 3, i32 28, i32 7, i32 9}
!230 = !{i32 6, !"m_uVoxelResolution", i32 3, i32 32, i32 7, i32 5}
!231 = !{i32 6, !"m_uLog2VoxelResolution", i32 3, i32 36, i32 7, i32 5}
!232 = !{i32 6, !"m_uBaseVoxelResolutionX", i32 3, i32 40, i32 7, i32 5}
!233 = !{i32 6, !"m_uBaseVoxelResolutionXY", i32 3, i32 44, i32 7, i32 5}
!234 = !{i32 6, !"m_iPrimaryTreeOffset", i32 3, i32 48, i32 7, i32 4}
!235 = !{i32 6, !"m_iDualTreeOffset", i32 3, i32 52, i32 7, i32 4}
!236 = !{i32 6, !"m_iIrradianceProbeOffset", i32 3, i32 56, i32 7, i32 4}
!237 = !{i32 6, !"m_iDirectTransportProbeOffset", i32 3, i32 60, i32 7, i32 4}
!238 = !{i32 6, !"m_iIndirectTransportProbeOffset", i32 3, i32 64, i32 7, i32 4}
!239 = !{i32 6, !"m_iLightGatherOffset", i32 3, i32 68, i32 7, i32 4}
!240 = !{i32 6, !"m_iLightProbeRefOffset", i32 3, i32 72, i32 7, i32 4}
!241 = !{i32 6, !"m_iDualLightProbeRefOffset", i32 3, i32 76, i32 7, i32 4}
!242 = !{i32 6, !"m_iEnvironmentMapRefOffset", i32 3, i32 80, i32 7, i32 4}
!243 = !{i32 6, !"m_iEnvironmentMapDualRefOffset", i32 3, i32 84, i32 7, i32 4}
!244 = !{i32 6, !"m_iEnvironmentMapInfoOffset", i32 3, i32 88, i32 7, i32 4}
!245 = !{i32 6, !"m_fWorldToGrid", i32 3, i32 92, i32 7, i32 9}
!246 = !{i32 6, !"m_fGridToWorld", i32 3, i32 96, i32 7, i32 9}
!247 = !{i32 6, !"pad0", i32 3, i32 100, i32 7, i32 9}
!248 = !{i32 6, !"pad1", i32 3, i32 104, i32 7, i32 9}
!249 = !{i32 6, !"pad2", i32 3, i32 108, i32 7, i32 9}
!250 = !{i32 6, !"m_iRowTranslationTable", i32 3, i32 112, i32 7, i32 4}
!251 = !{i32 4, !252}
!252 = !{i32 6, !"m_fDiffuseC0", i32 3, i32 0, i32 7, i32 9}
!253 = !{i32 4, !254}
!254 = !{i32 6, !"m_data", i32 3, i32 0, i32 7, i32 5}
!255 = !{i32 356, !39}
!256 = !{i32 28, !39}
!257 = !{i32 28, !258, !259, !260}
!258 = !{i32 6, !"m_childMask", i32 3, i32 0}
!259 = !{i32 6, !"m_pointer", i32 3, i32 20}
!260 = !{i32 6, !"m_payload", i32 3, i32 24, i32 7, i32 5}
!261 = !{i32 20, !262}
!262 = !{i32 6, !"m_uMask", i32 3, i32 0, i32 7, i32 5}
!263 = !{i32 4, !264}
!264 = !{i32 6, !"m_flagAndPointer", i32 3, i32 0, i32 7, i32 5}
!265 = !{i32 36, !39}
!266 = !{i32 36, !267, !268, !269, !270, !271, !272, !273, !274, !275}
!267 = !{i32 6, !"m_vR00", i32 3, i32 0, i32 7, i32 5}
!268 = !{i32 6, !"m_vR01", i32 3, i32 4, i32 7, i32 5}
!269 = !{i32 6, !"m_vR02", i32 3, i32 8, i32 7, i32 5}
!270 = !{i32 6, !"m_vR10", i32 3, i32 12, i32 7, i32 5}
!271 = !{i32 6, !"m_vR11", i32 3, i32 16, i32 7, i32 5}
!272 = !{i32 6, !"m_vR12", i32 3, i32 20, i32 7, i32 5}
!273 = !{i32 6, !"m_vR20", i32 3, i32 24, i32 7, i32 5}
!274 = !{i32 6, !"m_vR21", i32 3, i32 28, i32 7, i32 5}
!275 = !{i32 6, !"m_vR22", i32 3, i32 32, i32 7, i32 5}
!276 = !{i32 116, !39}
!277 = !{i32 116, !278}
!278 = !{i32 6, !"m_mTransport", i32 3, i32 0, i32 7, i32 5}
!279 = !{i32 8, !280, !281}
!280 = !{i32 6, !"m_index1", i32 3, i32 0, i32 7, i32 5}
!281 = !{i32 6, !"m_index2", i32 3, i32 4, i32 7, i32 5}
!282 = !{i32 220, !39}
!283 = !{i32 220, !284, !285, !286, !287, !288}
!284 = !{i32 6, !"m_vDebugColor", i32 3, i32 0, i32 7, i32 9}
!285 = !{i32 6, !"m_fRadius", i32 3, i32 12, i32 7, i32 9}
!286 = !{i32 6, !"m_vCenter", i32 3, i32 16, i32 7, i32 9}
!287 = !{i32 6, !"m_vMin", i32 3, i32 32, i32 7, i32 9}
!288 = !{i32 6, !"m_vMax", i32 3, i32 128, i32 7, i32 9}
!289 = !{i32 44, !290, !291, !292, !293, !294}
!290 = !{i32 6, !"m_vVoxelMin", i32 3, i32 0, i32 7, i32 9}
!291 = !{i32 6, !"m_fVoxelSize", i32 3, i32 12, i32 7, i32 9}
!292 = !{i32 6, !"m_uPayload", i32 3, i32 16, i32 7, i32 5}
!293 = !{i32 6, !"m_uLinearBaseVoxelIndex", i32 3, i32 20, i32 7, i32 5}
!294 = !{i32 6, !"m_vBaseVoxelMin", i32 3, i32 32, i32 7, i32 9}
!295 = !{i32 44, !296, !297, !298, !299, !300, !301, !302}
!296 = !{i32 6, !"m_iParentNode", i32 3, i32 0, i32 7, i32 4}
!297 = !{i32 6, !"m_vParentVoxelMin", i32 3, i32 4, i32 7, i32 9}
!298 = !{i32 6, !"m_vLeafVoxelMin", i32 3, i32 16, i32 7, i32 9}
!299 = !{i32 6, !"m_fLeafVoxelSize", i32 3, i32 28, i32 7, i32 9}
!300 = !{i32 6, !"m_bIsTerminal", i32 3, i32 32, i32 7, i32 1}
!301 = !{i32 6, !"m_bIsSolid", i32 3, i32 36, i32 7, i32 1}
!302 = !{i32 6, !"m_iPayload", i32 3, i32 40, i32 7, i32 4}
!303 = !{i32 16, !304, !305, !306}
!304 = !{i32 6, !"m_vUVOffset", i32 3, i32 0, i32 7, i32 9}
!305 = !{i32 6, !"m_uEnvMapIndex", i32 3, i32 8, i32 7, i32 5}
!306 = !{i32 6, !"m_uEnvInfoIndex", i32 3, i32 12, i32 7, i32 5}
!307 = !{i32 152, !39}
!308 = !{i32 152, !309, !310, !311, !312, !313, !314, !315}
!309 = !{i32 6, !"g_fPerLODDissolve", i32 3, i32 0, i32 7, i32 9}
!310 = !{i32 6, !"g_vWindIndex_Base_Trunk", i32 3, i32 116, i32 7, i32 5}
!311 = !{i32 6, !"g_uMask", i32 3, i32 124, i32 7, i32 5}
!312 = !{i32 6, !"g_iCharacterLightRigIndex", i32 3, i32 128, i32 7, i32 4}
!313 = !{i32 6, !"g_fDepthMultiply", i32 3, i32 132, i32 7, i32 9}
!314 = !{i32 6, !"g_fExtraDepthMultiply", i32 3, i32 136, i32 7, i32 9}
!315 = !{i32 6, !"padTo16Bytes", i32 3, i32 144, i32 7, i32 4}
!316 = !{i32 40, !317, !318, !319, !320, !321, !322, !323, !324, !325, !326}
!317 = !{i32 6, !"uIndexOffset", i32 3, i32 0, i32 7, i32 5}
!318 = !{i32 6, !"uIndexStride", i32 3, i32 4, i32 7, i32 5}
!319 = !{i32 6, !"uVertexStride1", i32 3, i32 8, i32 7, i32 5}
!320 = !{i32 6, !"uMaterialID", i32 3, i32 12, i32 7, i32 5}
!321 = !{i32 6, !"uTangentOffset", i32 3, i32 16, i32 7, i32 5}
!322 = !{i32 6, !"uNormalOffset", i32 3, i32 20, i32 7, i32 5}
!323 = !{i32 6, !"uTexcoordOffset", i32 3, i32 24, i32 7, i32 5}
!324 = !{i32 6, !"uColorOffset", i32 3, i32 28, i32 7, i32 5}
!325 = !{i32 6, !"uIndexBufferSize", i32 3, i32 32, i32 7, i32 5}
!326 = !{i32 6, !"uVertexBuffer1Size", i32 3, i32 36, i32 7, i32 5}
!327 = !{i32 8, !328}
!328 = !{i32 6, !"vIntersection", i32 3, i32 0, i32 4, !"INTERSECTION", i32 7, i32 9}
!329 = !{i32 4, !330}
!330 = !{i32 6, !"uRayIndex", i32 3, i32 0, i32 4, !"RAY_INDEX", i32 7, i32 5}
!331 = !{i32 32, !332, !333, !334, !335}
!332 = !{i32 6, !"Origin", i32 3, i32 0, i32 7, i32 9}
!333 = !{i32 6, !"TMin", i32 3, i32 12, i32 7, i32 9}
!334 = !{i32 6, !"Direction", i32 3, i32 16, i32 7, i32 9}
!335 = !{i32 6, !"TMax", i32 3, i32 28, i32 7, i32 9}
!336 = !{i32 88, !337, !338, !339, !340, !341, !342, !343, !344, !345, !346, !347, !348, !349, !350, !351, !352, !353, !354, !355, !356, !357, !358}
!337 = !{i32 6, !"g_fDebug_h_Slider0", i32 3, i32 0, i32 7, i32 9}
!338 = !{i32 6, !"g_fDebug_h_Slider1", i32 3, i32 4, i32 7, i32 9}
!339 = !{i32 6, !"g_fDebug_h_Slider2", i32 3, i32 8, i32 7, i32 9}
!340 = !{i32 6, !"g_fDebug_h_Slider3", i32 3, i32 12, i32 7, i32 9}
!341 = !{i32 6, !"g_fDebug_h_Slider4", i32 3, i32 16, i32 7, i32 9}
!342 = !{i32 6, !"g_fDebug_h_Slider5", i32 3, i32 20, i32 7, i32 9}
!343 = !{i32 6, !"g_fDebug_h_Slider6", i32 3, i32 24, i32 7, i32 9}
!344 = !{i32 6, !"g_fDebug_h_Slider7", i32 3, i32 28, i32 7, i32 9}
!345 = !{i32 6, !"g_fDebug_h_Knob0", i32 3, i32 32, i32 7, i32 9}
!346 = !{i32 6, !"g_fDebug_h_Knob1", i32 3, i32 36, i32 7, i32 9}
!347 = !{i32 6, !"g_fDebug_h_Knob2", i32 3, i32 40, i32 7, i32 9}
!348 = !{i32 6, !"g_fDebug_h_Knob3", i32 3, i32 44, i32 7, i32 9}
!349 = !{i32 6, !"g_fDebug_h_Knob4", i32 3, i32 48, i32 7, i32 9}
!350 = !{i32 6, !"g_fDebug_h_Knob5", i32 3, i32 52, i32 7, i32 9}
!351 = !{i32 6, !"g_fDebug_h_Knob6", i32 3, i32 56, i32 7, i32 9}
!352 = !{i32 6, !"g_fDebug_h_Knob7", i32 3, i32 60, i32 7, i32 9}
!353 = !{i32 6, !"g_fDebug_h_Toggle0", i32 3, i32 64, i32 7, i32 9}
!354 = !{i32 6, !"g_fDebug_h_Toggle1", i32 3, i32 68, i32 7, i32 9}
!355 = !{i32 6, !"g_fDebug_h_Toggle2", i32 3, i32 72, i32 7, i32 9}
!356 = !{i32 6, !"g_fDebug_h_Toggle3", i32 3, i32 76, i32 7, i32 9}
!357 = !{i32 6, !"g_fDebug_h_Toggle4", i32 3, i32 80, i32 7, i32 9}
!358 = !{i32 6, !"g_fDebug_h_Toggle5", i32 3, i32 84, i32 7, i32 9}
!359 = !{i32 1128, !360, !361, !362, !363, !364, !365, !366, !367, !368, !369, !370, !371, !372, !373, !374, !375, !376, !377, !378, !379, !380, !381, !382, !383, !384, !385, !386, !387, !388, !389, !390, !391}
!360 = !{i32 6, !"g_vScreenRes", i32 3, i32 0, i32 7, i32 9}
!361 = !{i32 6, !"g_vInvScreenRes", i32 3, i32 8, i32 7, i32 9}
!362 = !{i32 6, !"g_vOutputRes", i32 3, i32 16, i32 7, i32 9}
!363 = !{i32 6, !"g_vInvOutputRes", i32 3, i32 24, i32 7, i32 9}
!364 = !{i32 6, !"g_mWorldToView", i32 2, !110, i32 3, i32 32, i32 7, i32 9}
!365 = !{i32 6, !"g_mViewToWorld", i32 2, !110, i32 3, i32 96, i32 7, i32 9}
!366 = !{i32 6, !"g_mViewToClip", i32 2, !110, i32 3, i32 160, i32 7, i32 9}
!367 = !{i32 6, !"g_mClipToView", i32 2, !110, i32 3, i32 224, i32 7, i32 9}
!368 = !{i32 6, !"g_mWorldToClip", i32 2, !110, i32 3, i32 288, i32 7, i32 9}
!369 = !{i32 6, !"g_mClipToWorld", i32 2, !110, i32 3, i32 352, i32 7, i32 9}
!370 = !{i32 6, !"g_mClipToPreviousClip", i32 2, !110, i32 3, i32 416, i32 7, i32 9}
!371 = !{i32 6, !"g_mViewToPreviousClip", i32 2, !110, i32 3, i32 480, i32 7, i32 9}
!372 = !{i32 6, !"g_mPreviousViewToView", i32 2, !110, i32 3, i32 544, i32 7, i32 9}
!373 = !{i32 6, !"g_mPreviousWorldToClip", i32 2, !110, i32 3, i32 608, i32 7, i32 9}
!374 = !{i32 6, !"g_mPreviousViewToClip", i32 2, !110, i32 3, i32 672, i32 7, i32 9}
!375 = !{i32 6, !"g_mClipToPreviousClipNoJitter", i32 2, !110, i32 3, i32 736, i32 7, i32 9}
!376 = !{i32 6, !"g_mPreviousClipToClipNoJitter", i32 2, !110, i32 3, i32 800, i32 7, i32 9}
!377 = !{i32 6, !"g_vViewPoint", i32 3, i32 864, i32 7, i32 9}
!378 = !{i32 6, !"g_fInvNear", i32 3, i32 880, i32 7, i32 9}
!379 = !{i32 6, !"g_mViewToCameraView", i32 2, !110, i32 3, i32 896, i32 7, i32 9}
!380 = !{i32 6, !"g_mCameraViewToCameraClip", i32 2, !110, i32 3, i32 960, i32 7, i32 9}
!381 = !{i32 6, !"g_mCameraClipToView", i32 2, !110, i32 3, i32 1024, i32 7, i32 9}
!382 = !{i32 6, !"g_fWorldTime", i32 3, i32 1088, i32 7, i32 9}
!383 = !{i32 6, !"g_fWorldTimeDelta", i32 3, i32 1092, i32 7, i32 9}
!384 = !{i32 6, !"g_fRealTime", i32 3, i32 1096, i32 7, i32 9}
!385 = !{i32 6, !"g_fRealTimeDelta", i32 3, i32 1100, i32 7, i32 9}
!386 = !{i32 6, !"g_fLastValidWorldTimeDelta", i32 3, i32 1104, i32 7, i32 9}
!387 = !{i32 6, !"g_uTemporalFrame", i32 3, i32 1108, i32 7, i32 5}
!388 = !{i32 6, !"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5}
!389 = !{i32 6, !"g_bHDR", i32 3, i32 1116, i32 7, i32 1}
!390 = !{i32 6, !"g_fHDRSDRSceneBrightnessMultiplier", i32 3, i32 1120, i32 7, i32 9}
!391 = !{i32 6, !"g_fHDRSDRUIBrightnessMultiplier", i32 3, i32 1124, i32 7, i32 9}
!392 = !{i32 32, !393, !394, !395, !396}
!393 = !{i32 6, !"g_vShadowMapRes", i32 3, i32 0, i32 7, i32 9}
!394 = !{i32 6, !"g_vShadowMapVSMRes", i32 3, i32 8, i32 7, i32 9}
!395 = !{i32 6, !"g_vJitterOffset", i32 3, i32 16, i32 7, i32 9}
!396 = !{i32 6, !"g_vSnapOffset", i32 3, i32 24, i32 7, i32 4}
!397 = !{i32 4, !398}
!398 = !{i32 6, !"g_fOnePerTranslucencyKernelCount", i32 3, i32 0, i32 7, i32 9}
!399 = !{i32 408, !400, !401, !402, !403, !404, !405, !406, !407, !408, !409, !410, !411, !412, !413, !414, !415, !416, !417, !418, !419, !420, !421, !422, !423, !424, !425, !426, !427, !428, !429, !430, !431, !432, !433, !434}
!400 = !{i32 6, !"g_fTileDepthClipRanges", i32 3, i32 0, i32 7, i32 9}
!401 = !{i32 6, !"g_fTileDepthRanges", i32 3, i32 80, i32 7, i32 9}
!402 = !{i32 6, !"g_vDepthTileResolve", i32 3, i32 160, i32 7, i32 9}
!403 = !{i32 6, !"g_uDepthTileCount", i32 3, i32 168, i32 7, i32 5}
!404 = !{i32 6, !"g_vTileResolution", i32 3, i32 176, i32 7, i32 5}
!405 = !{i32 6, !"g_vTileWidthHeightDepth", i32 3, i32 192, i32 7, i32 5}
!406 = !{i32 6, !"g_vTileResolutionPerScreenResolution", i32 3, i32 208, i32 7, i32 9}
!407 = !{i32 6, !"g_vTileDepthNearFar", i32 3, i32 216, i32 7, i32 9}
!408 = !{i32 6, !"g_uMaxPointLightsPerTile", i32 3, i32 224, i32 7, i32 5}
!409 = !{i32 6, !"g_uMaxSpotLightsPerTile", i32 3, i32 228, i32 7, i32 5}
!410 = !{i32 6, !"g_uAmbientLightTotalCount", i32 3, i32 232, i32 7, i32 5}
!411 = !{i32 6, !"g_fAmbientEnvDiffuseIntensity", i32 3, i32 236, i32 7, i32 9}
!412 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnTransparent", i32 3, i32 240, i32 7, i32 9}
!413 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnOpaque", i32 3, i32 244, i32 7, i32 9}
!414 = !{i32 6, !"g_fAmbientSkyIntensity", i32 3, i32 248, i32 7, i32 9}
!415 = !{i32 6, !"g_fAmbientLocalIntensity", i32 3, i32 252, i32 7, i32 9}
!416 = !{i32 6, !"g_uPointLightTotalCount", i32 3, i32 256, i32 7, i32 5}
!417 = !{i32 6, !"g_uSpotLightTotalCount", i32 3, i32 260, i32 7, i32 5}
!418 = !{i32 6, !"g_uSunLightTotalCount", i32 3, i32 264, i32 7, i32 5}
!419 = !{i32 6, !"g_uAmbientLightEnabled", i32 3, i32 268, i32 7, i32 5}
!420 = !{i32 6, !"g_fAmbientDepthRampDistance", i32 3, i32 272, i32 7, i32 9}
!421 = !{i32 6, !"g_fAmbientDepthRampFeather", i32 3, i32 276, i32 7, i32 9}
!422 = !{i32 6, !"g_fAmbientDepthRampIntensityNear", i32 3, i32 280, i32 7, i32 9}
!423 = !{i32 6, !"g_fAmbientDepthRampIntensityFar", i32 3, i32 284, i32 7, i32 9}
!424 = !{i32 6, !"g_vVolumeLightDimensions", i32 3, i32 288, i32 7, i32 5}
!425 = !{i32 6, !"g_vVolumeLightProjectionConstants", i32 3, i32 304, i32 7, i32 9}
!426 = !{i32 6, !"g_vHalfResVolumeLightProjectionConstants", i32 3, i32 320, i32 7, i32 9}
!427 = !{i32 6, !"g_vOnePerVolumeLightDimensions", i32 3, i32 336, i32 7, i32 9}
!428 = !{i32 6, !"g_vVolumeLightXYToTileXY", i32 3, i32 352, i32 7, i32 9}
!429 = !{i32 6, !"g_vVolumeLightDepthResolve", i32 3, i32 368, i32 7, i32 9}
!430 = !{i32 6, !"g_fVolumeLightOnePerDepthMinusOne", i32 3, i32 380, i32 7, i32 9}
!431 = !{i32 6, !"g_vVolumeLightNearSplit0Far", i32 3, i32 384, i32 7, i32 9}
!432 = !{i32 6, !"g_fVolumeLightKernelWidth", i32 3, i32 396, i32 7, i32 9}
!433 = !{i32 6, !"g_fVolumeLightSamplingBias", i32 3, i32 400, i32 7, i32 9}
!434 = !{i32 6, !"g_uRTContactShadowRayCount", i32 3, i32 404, i32 7, i32 5}
!435 = !{i32 8, !436, !437}
!436 = !{i32 6, !"g_fEnvReflectionEdgeLength", i32 3, i32 0, i32 7, i32 9}
!437 = !{i32 6, !"g_fEnvReflectionMipCount", i32 3, i32 4, i32 7, i32 9}
!438 = !{i32 188, !439, !440, !441, !442, !443, !444, !445, !446, !447, !448, !449, !450, !451, !452, !453, !454, !455, !456, !457, !458}
!439 = !{i32 6, !"g_atmosphere_vSunDir", i32 3, i32 0, i32 7, i32 9}
!440 = !{i32 6, !"g_atmosphere_vSunE", i32 3, i32 16, i32 7, i32 9}
!441 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants1", i32 3, i32 32, i32 7, i32 9}
!442 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants2", i32 3, i32 40, i32 7, i32 9}
!443 = !{i32 6, !"g_atmosphere_vPhaseBlend", i32 3, i32 48, i32 7, i32 9}
!444 = !{i32 6, !"g_atmosphere_vPhaseG", i32 3, i32 52, i32 7, i32 9}
!445 = !{i32 6, !"g_atmosphere_vFog", i32 3, i32 64, i32 7, i32 9}
!446 = !{i32 6, !"g_vFogColor", i32 3, i32 80, i32 7, i32 9}
!447 = !{i32 6, !"g_vFogColorOpposite", i32 3, i32 96, i32 7, i32 9}
!448 = !{i32 6, !"g_fFogExp", i32 3, i32 108, i32 7, i32 9}
!449 = !{i32 6, !"g_fFogGroundDensityAtViewer", i32 3, i32 112, i32 7, i32 9}
!450 = !{i32 6, !"g_fFogGroundHeight", i32 3, i32 116, i32 7, i32 9}
!451 = !{i32 6, !"g_fFogGroundFalloff", i32 3, i32 120, i32 7, i32 9}
!452 = !{i32 6, !"g_fFogGroundDensity", i32 3, i32 124, i32 7, i32 9}
!453 = !{i32 6, !"g_vFogGroundDensityMapRange", i32 3, i32 128, i32 7, i32 9}
!454 = !{i32 6, !"g_vFogGroundSimulationVelocityAndScale", i32 3, i32 144, i32 7, i32 9}
!455 = !{i32 6, !"g_fFogDepthRampDistance", i32 3, i32 156, i32 7, i32 9}
!456 = !{i32 6, !"g_fFogDepthRampFeather", i32 3, i32 160, i32 7, i32 9}
!457 = !{i32 6, !"g_fFogDepthRampIntensityNear", i32 3, i32 164, i32 7, i32 9}
!458 = !{i32 6, !"g_vFogAlbedo", i32 3, i32 176, i32 7, i32 9}
!459 = !{i32 40, !460, !461, !462, !463, !464, !465}
!460 = !{i32 6, !"g_vOnePerVolumeAtlasSize", i32 3, i32 0, i32 7, i32 9}
!461 = !{i32 6, !"g_uActiveVolumeCount", i32 3, i32 12, i32 7, i32 5}
!462 = !{i32 6, !"g_vAdditiveAmbient", i32 3, i32 16, i32 7, i32 9}
!463 = !{i32 6, !"g_uActiveDebugVolumeCount", i32 3, i32 28, i32 7, i32 5}
!464 = !{i32 6, !"g_fTransparentBalanceEnergy", i32 3, i32 32, i32 7, i32 9}
!465 = !{i32 6, !"g_fTransparentBalanceEnergyStrength", i32 3, i32 36, i32 7, i32 9}
!466 = !{i32 24576, !467}
!467 = !{i32 6, !"g_cbActiveVolumeTopLevelInfoArray", i32 3, i32 0}
!468 = !{i32 24576, !469}
!469 = !{i32 6, !"g_cbDebugVolumeTopLevelInfoArray", i32 3, i32 0}
!470 = !{i32 12, !471, !472, !473}
!471 = !{i32 6, !"g_fWorldSize", i32 3, i32 0, i32 7, i32 9}
!472 = !{i32 6, !"g_fCellSize", i32 3, i32 4, i32 7, i32 9}
!473 = !{i32 6, !"g_iCellCount", i32 3, i32 8, i32 7, i32 4}
!474 = !{i32 24, !475, !476, !477, !478, !479}
!475 = !{i32 6, !"g_fInvEnvironmentMapsPerRow", i32 3, i32 0, i32 7, i32 9}
!476 = !{i32 6, !"g_fEnvironmentMapsPerRow", i32 3, i32 4, i32 7, i32 9}
!477 = !{i32 6, !"g_fEnvironmentMapColSize", i32 3, i32 8, i32 7, i32 9}
!478 = !{i32 6, !"g_fEnvironmentMapRowSize", i32 3, i32 12, i32 7, i32 9}
!479 = !{i32 6, !"g_fInvEnvironmentMapAtlasSize", i32 3, i32 16, i32 7, i32 9}
!480 = !{i32 48, !481, !482, !483}
!481 = !{i32 6, !"g_uStride0_uStride1_uOffset0_uOffset1", i32 3, i32 0, i32 7, i32 5}
!482 = !{i32 6, !"g_Tangent_Normal_Texcoord_Color", i32 3, i32 16, i32 7, i32 5}
!483 = !{i32 6, !"g_BIndices_BWeights", i32 3, i32 32, i32 7, i32 5}
!484 = !{i32 40, !485}
!485 = !{i32 6, !"g_cbRaytracingHit", i32 3, i32 0}
!486 = !{i32 24, !487, !488, !489, !490, !491, !492}
!487 = !{i32 6, !"g_fClampReflectionIntensity", i32 3, i32 0, i32 7, i32 9}
!488 = !{i32 6, !"g_fRTReflectionAddRays", i32 3, i32 4, i32 7, i32 9}
!489 = !{i32 6, !"g_uRTReflectionRayCount", i32 3, i32 8, i32 7, i32 5}
!490 = !{i32 6, !"g_fRTZeroRayReflectance", i32 3, i32 12, i32 7, i32 9}
!491 = !{i32 6, !"g_fRTMaxRayReflectance", i32 3, i32 16, i32 7, i32 9}
!492 = !{i32 6, !"g_fRTMaxRayRoughness", i32 3, i32 20, i32 7, i32 9}
!493 = !{i32 1, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !494, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !494, void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !501, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !494, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !494, void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !501, void ()* @"\01?reflectionRayGeneration@@YAXXZ", !502}
!494 = !{!495, !497, !499}
!495 = !{i32 1, !496, !496}
!496 = !{}
!497 = !{i32 2, !498, !496}
!498 = !{i32 4, !"SV_RayPayload"}
!499 = !{i32 0, !500, !496}
!500 = !{i32 4, !"SV_IntersectionAttributes"}
!501 = !{!495, !497}
!502 = !{!495}
!503 = !{null, !"", null, !3, !504}
!504 = !{i32 0, i64 65808, i32 5, !505}
!505 = !{i32 0}
!506 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !507}
!507 = !{i32 8, i32 9, i32 6, i32 4, i32 7, i32 8, i32 5, !505}
!508 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !509}
!509 = !{i32 8, i32 10, i32 6, i32 4, i32 7, i32 8, i32 5, !505}
!510 = !{void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !"\01?reflectionMiss@@YAXUHitData@@@Z", null, null, !511}
!511 = !{i32 8, i32 11, i32 6, i32 4, i32 5, !505}
!512 = !{void ()* @"\01?reflectionRayGeneration@@YAXXZ", !"\01?reflectionRayGeneration@@YAXXZ", null, null, !513}
!513 = !{i32 8, i32 7, i32 5, !505}
!514 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !507}
!515 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !509}
!516 = !{void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !"\01?shadowMiss@@YAXUHitData@@@Z", null, null, !511}
!517 = !{!518, !518, i64 0}
!518 = !{!"omnipotent char", !519, i64 0}
!519 = !{!"Simple C/C++ TBAA"}
!520 = !{!521, !521, i64 0}
!521 = !{!"float", !518, i64 0}
!522 = !{!523, !523, i64 0}
!523 = !{!"int", !518, i64 0}

