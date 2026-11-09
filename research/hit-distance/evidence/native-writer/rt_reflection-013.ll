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
; g_tStaticBlueNoiseRGBA_0          texture     f32          2d      T3             t3     1
; g_rtScene                         texture     i32         ras      T4             t4     1
; g_rwtShadow                           UAV     u32     2darray      U0             u0     1
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%"class.Texture2D<vector<float, 4> >" = type { <4 x float>, %"class.Texture2D<vector<float, 4> >::mips_type" }
%"class.Texture2D<vector<float, 4> >::mips_type" = type { i32 }
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
  %5 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %6 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %7 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", align 4
  %8 = load %rtreflection, %rtreflection* @rtreflection, align 4
  %9 = load %sys_constants, %sys_constants* @sys_constants, align 4
  %rtreflection = call %dx.types.Handle @dx.op.createHandleForLib.rtreflection(i32 160, %rtreflection %8)  ; CreateHandleForLib(Resource)
  %sys_constants = call %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32 160, %sys_constants %9)  ; CreateHandleForLib(Resource)
  %10 = alloca %struct.HitData, align 8
  %ray = alloca %struct.RayDesc, align 4
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex150 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %11 = uitofp i32 %DispatchRaysIndex to float
  %12 = uitofp i32 %DispatchRaysIndex150 to float
  %.i0 = fadd fast float %11, 5.000000e-01
  %.i1 = fadd fast float %12, 5.000000e-01
  %13 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %14 = extractvalue %dx.types.CBufRet.f32 %13, 2
  %15 = extractvalue %dx.types.CBufRet.f32 %13, 3
  %.i0152 = fmul fast float %14, %.i0
  %.i1153 = fmul fast float %.i1, %15
  %16 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %5)  ; CreateHandleForLib(Resource)
  %TextureLoad = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %16, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex150, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %17 = extractvalue %dx.types.ResRet.f32 %TextureLoad, 0
  %18 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %6)  ; CreateHandleForLib(Resource)
  %TextureLoad145 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %18, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex150, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %19 = extractvalue %dx.types.ResRet.f32 %TextureLoad145, 0
  %.i0154 = fmul fast float %.i0152, 2.000000e+00
  %.i1155 = fmul fast float %.i1153, 2.000000e+00
  %.i0156 = fadd fast float %.i0154, -1.000000e+00
  %.i1157433 = fsub fast float 1.000000e+00, %.i1155
  %20 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 14)  ; CBufferLoadLegacy(handle,regIndex)
  %21 = extractvalue %dx.types.CBufRet.f32 %20, 0
  %22 = extractvalue %dx.types.CBufRet.f32 %20, 1
  %23 = extractvalue %dx.types.CBufRet.f32 %20, 2
  %24 = extractvalue %dx.types.CBufRet.f32 %20, 3
  %25 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %26 = extractvalue %dx.types.CBufRet.f32 %25, 0
  %27 = extractvalue %dx.types.CBufRet.f32 %25, 1
  %28 = extractvalue %dx.types.CBufRet.f32 %25, 2
  %29 = extractvalue %dx.types.CBufRet.f32 %25, 3
  %30 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %31 = extractvalue %dx.types.CBufRet.f32 %30, 0
  %32 = extractvalue %dx.types.CBufRet.f32 %30, 1
  %33 = extractvalue %dx.types.CBufRet.f32 %30, 2
  %34 = extractvalue %dx.types.CBufRet.f32 %30, 3
  %35 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %36 = extractvalue %dx.types.CBufRet.f32 %35, 0
  %37 = extractvalue %dx.types.CBufRet.f32 %35, 1
  %38 = extractvalue %dx.types.CBufRet.f32 %35, 2
  %39 = extractvalue %dx.types.CBufRet.f32 %35, 3
  %40 = fmul fast float %21, %.i0156
  %FMad84 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %26, float %40)  ; FMad(a,b,c)
  %FMad83 = call float @dx.op.tertiary.f32(i32 46, float %17, float %31, float %FMad84)  ; FMad(a,b,c)
  %41 = fadd fast float %FMad83, %36
  %42 = fmul fast float %22, %.i0156
  %FMad81 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %27, float %42)  ; FMad(a,b,c)
  %FMad80 = call float @dx.op.tertiary.f32(i32 46, float %17, float %32, float %FMad81)  ; FMad(a,b,c)
  %43 = fadd fast float %FMad80, %37
  %44 = fmul fast float %23, %.i0156
  %FMad78 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %28, float %44)  ; FMad(a,b,c)
  %FMad77 = call float @dx.op.tertiary.f32(i32 46, float %17, float %33, float %FMad78)  ; FMad(a,b,c)
  %45 = fadd fast float %FMad77, %38
  %46 = fmul fast float %24, %.i0156
  %FMad75 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %29, float %46)  ; FMad(a,b,c)
  %FMad74 = call float @dx.op.tertiary.f32(i32 46, float %17, float %34, float %FMad75)  ; FMad(a,b,c)
  %47 = fadd fast float %FMad74, %39
  %48 = fdiv fast float 1.000000e+00, %47
  %.i0158 = fmul fast float %48, %41
  %.i1159 = fmul fast float %48, %43
  %.i2 = fmul fast float %48, %45
  %49 = fmul fast float %.i0158, %.i0158
  %50 = fmul fast float %.i1159, %.i1159
  %51 = fadd fast float %49, %50
  %52 = fmul fast float %.i2, %.i2
  %53 = fadd fast float %51, %52
  %Sqrt66 = call float @dx.op.unary.f32(i32 24, float %53)  ; Sqrt(value)
  %.i0160 = fdiv fast float %.i0158, %Sqrt66
  %.i1161 = fdiv fast float %.i1159, %Sqrt66
  %.i2162 = fdiv fast float %.i2, %Sqrt66
  %54 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 22)  ; CBufferLoadLegacy(handle,regIndex)
  %55 = extractvalue %dx.types.CBufRet.f32 %54, 0
  %56 = extractvalue %dx.types.CBufRet.f32 %54, 1
  %57 = extractvalue %dx.types.CBufRet.f32 %54, 2
  %58 = extractvalue %dx.types.CBufRet.f32 %54, 3
  %59 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 23)  ; CBufferLoadLegacy(handle,regIndex)
  %60 = extractvalue %dx.types.CBufRet.f32 %59, 0
  %61 = extractvalue %dx.types.CBufRet.f32 %59, 1
  %62 = extractvalue %dx.types.CBufRet.f32 %59, 2
  %63 = extractvalue %dx.types.CBufRet.f32 %59, 3
  %64 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 24)  ; CBufferLoadLegacy(handle,regIndex)
  %65 = extractvalue %dx.types.CBufRet.f32 %64, 0
  %66 = extractvalue %dx.types.CBufRet.f32 %64, 1
  %67 = extractvalue %dx.types.CBufRet.f32 %64, 2
  %68 = extractvalue %dx.types.CBufRet.f32 %64, 3
  %69 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 25)  ; CBufferLoadLegacy(handle,regIndex)
  %70 = extractvalue %dx.types.CBufRet.f32 %69, 0
  %71 = extractvalue %dx.types.CBufRet.f32 %69, 1
  %72 = extractvalue %dx.types.CBufRet.f32 %69, 2
  %73 = extractvalue %dx.types.CBufRet.f32 %69, 3
  %74 = fmul fast float %55, %.i0156
  %FMad96 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %60, float %74)  ; FMad(a,b,c)
  %FMad95 = call float @dx.op.tertiary.f32(i32 46, float %17, float %65, float %FMad96)  ; FMad(a,b,c)
  %75 = fadd fast float %FMad95, %70
  %76 = fmul fast float %56, %.i0156
  %FMad93 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %61, float %76)  ; FMad(a,b,c)
  %FMad92 = call float @dx.op.tertiary.f32(i32 46, float %17, float %66, float %FMad93)  ; FMad(a,b,c)
  %77 = fadd fast float %FMad92, %71
  %78 = fmul fast float %57, %.i0156
  %FMad90 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %62, float %78)  ; FMad(a,b,c)
  %FMad89 = call float @dx.op.tertiary.f32(i32 46, float %17, float %67, float %FMad90)  ; FMad(a,b,c)
  %79 = fadd fast float %FMad89, %72
  %80 = fmul fast float %58, %.i0156
  %FMad87 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %63, float %80)  ; FMad(a,b,c)
  %FMad86 = call float @dx.op.tertiary.f32(i32 46, float %17, float %68, float %FMad87)  ; FMad(a,b,c)
  %81 = fadd fast float %FMad86, %73
  %82 = fdiv fast float 1.000000e+00, %81
  %.i0167 = fmul fast float %82, %75
  %.i1168 = fmul fast float %82, %77
  %.i2169 = fmul fast float %82, %79
  %83 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 54)  ; CBufferLoadLegacy(handle,regIndex)
  %84 = extractvalue %dx.types.CBufRet.f32 %83, 0
  %85 = extractvalue %dx.types.CBufRet.f32 %83, 1
  %86 = extractvalue %dx.types.CBufRet.f32 %83, 2
  %.i0170 = fsub fast float %.i0167, %84
  %.i1171 = fsub fast float %.i1168, %85
  %.i2172 = fsub fast float %.i2169, %86
  %87 = fmul fast float %.i0170, %.i0170
  %88 = fmul fast float %.i1171, %.i1171
  %89 = fadd fast float %87, %88
  %90 = fmul fast float %.i2172, %.i2172
  %91 = fadd fast float %89, %90
  %Sqrt67 = call float @dx.op.unary.f32(i32 24, float %91)  ; Sqrt(value)
  %.i0173 = fdiv fast float %.i0170, %Sqrt67
  %.i1174 = fdiv fast float %.i1171, %Sqrt67
  %.i2175 = fdiv fast float %.i2172, %Sqrt67
  %92 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %7)  ; CreateHandleForLib(Resource)
  %TextureLoad146 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %92, i32 0, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex150, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %93 = extractvalue %dx.types.ResRet.f32 %TextureLoad146, 0
  %94 = extractvalue %dx.types.ResRet.f32 %TextureLoad146, 1
  %95 = extractvalue %dx.types.ResRet.f32 %TextureLoad146, 3
  %96 = fmul fast float %95, 2.550000e+02
  %97 = fadd fast float %96, 5.000000e-01
  %98 = fptoui float %97 to i32
  %99 = and i32 %98, 254
  %100 = uitofp i32 %99 to float
  %.i0176 = fmul fast float %93, 2.550000e+02
  %.i1177 = fmul fast float %94, 2.550000e+02
  %.i0179 = fadd fast float %.i0176, 5.000000e-01
  %.i1180 = fadd fast float %.i1177, 5.000000e-01
  %.i2181 = fadd fast float %100, 5.000000e-01
  %Round_ni60 = call float @dx.op.unary.f32(i32 27, float %.i0179)  ; Round_ni(value)
  %Round_ni61 = call float @dx.op.unary.f32(i32 27, float %.i1180)  ; Round_ni(value)
  %Round_ni62 = call float @dx.op.unary.f32(i32 27, float %.i2181)  ; Round_ni(value)
  %.i0182 = fptosi float %Round_ni60 to i32
  %.i1183 = fptosi float %Round_ni61 to i32
  %.i2184 = fptosi float %Round_ni62 to i32
  %.i0185.431 = lshr i32 %.i2184, 1
  %.i1186.432 = lshr i32 %.i2184, 4
  %.i0187 = and i32 %.i0185.431, 7
  %.i1188 = and i32 %.i1186.432, 15
  %.i0189 = shl i32 %.i0182, 3
  %.i1190 = shl i32 %.i1183, 4
  %.i0191 = or i32 %.i0187, %.i0189
  %.i1192 = or i32 %.i1188, %.i1190
  %101 = sitofp i32 %.i0191 to float
  %102 = sitofp i32 %.i1192 to float
  %103 = fmul fast float %101, 0x3F54CF66A0000000
  %.i0199 = fadd fast float %103, 0xBFF4CCCCC0000000
  %104 = fmul fast float %102, 0x3F44CE19C0000000
  %.i1200 = fadd fast float %104, 0xBFF4CCCCC0000000
  %105 = fmul fast float %.i0199, %.i0199
  %106 = fmul fast float %.i1200, %.i1200
  %107 = fadd fast float %105, 1.000000e+00
  %108 = fadd fast float %107, %106
  %109 = fmul fast float %101, 0x3F64CF66A0000000
  %.i0201 = fadd fast float %109, 0xC004CCCCC0000000
  %110 = fmul fast float %102, 0x3F54CE19C0000000
  %.i1202 = fadd fast float %110, 0xC004CCCCC0000000
  %111 = fadd fast float %106, -1.000000e+00
  %112 = fadd fast float %111, %105
  %.i0203 = fdiv fast float %.i0201, %108
  %.i1204 = fdiv fast float %.i1202, %108
  %.i2205 = fdiv fast float %112, %108
  %113 = fmul fast float %.i0203, %.i0203
  %114 = fmul fast float %.i1204, %.i1204
  %115 = fadd fast float %114, %113
  %116 = fmul fast float %.i2205, %.i2205
  %117 = fadd fast float %115, %116
  %Sqrt68 = call float @dx.op.unary.f32(i32 24, float %117)  ; Sqrt(value)
  %.i0206 = fdiv fast float %.i0203, %Sqrt68
  %.i1207 = fdiv fast float %.i1204, %Sqrt68
  %.i2208 = fdiv fast float %.i2205, %Sqrt68
  %118 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %119 = extractvalue %dx.types.CBufRet.f32 %118, 0
  %120 = extractvalue %dx.types.CBufRet.f32 %118, 1
  %121 = extractvalue %dx.types.CBufRet.f32 %118, 2
  %122 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %123 = extractvalue %dx.types.CBufRet.f32 %122, 0
  %124 = extractvalue %dx.types.CBufRet.f32 %122, 1
  %125 = extractvalue %dx.types.CBufRet.f32 %122, 2
  %126 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %127 = extractvalue %dx.types.CBufRet.f32 %126, 0
  %128 = extractvalue %dx.types.CBufRet.f32 %126, 1
  %129 = extractvalue %dx.types.CBufRet.f32 %126, 2
  %130 = fmul fast float %119, %.i0206
  %FMad144 = call float @dx.op.tertiary.f32(i32 46, float %.i1207, float %123, float %130)  ; FMad(a,b,c)
  %FMad143 = call float @dx.op.tertiary.f32(i32 46, float %.i2208, float %127, float %FMad144)  ; FMad(a,b,c)
  %131 = fmul fast float %120, %.i0206
  %FMad142 = call float @dx.op.tertiary.f32(i32 46, float %.i1207, float %124, float %131)  ; FMad(a,b,c)
  %FMad141 = call float @dx.op.tertiary.f32(i32 46, float %.i2208, float %128, float %FMad142)  ; FMad(a,b,c)
  %132 = fmul fast float %121, %.i0206
  %FMad140 = call float @dx.op.tertiary.f32(i32 46, float %.i1207, float %125, float %132)  ; FMad(a,b,c)
  %FMad139 = call float @dx.op.tertiary.f32(i32 46, float %.i2208, float %129, float %FMad140)  ; FMad(a,b,c)
  %.i0209 = fmul fast float %19, 0x3F50624DE0000000
  %.i0212 = fmul fast float %.i0209, %.i0173
  %.i1213 = fmul fast float %.i0209, %.i1174
  %.i2214 = fmul fast float %.i0209, %.i2175
  %.i0215 = fsub fast float %.i0167, %.i0212
  %.i1216 = fsub fast float %.i1168, %.i1213
  %.i2217 = fsub fast float %.i2169, %.i2214
  %.i0221 = fmul fast float %.i0209, %FMad143
  %.i1222 = fmul fast float %.i0209, %FMad141
  %.i2223 = fmul fast float %.i0209, %FMad139
  %.i0224 = fadd fast float %.i0215, %.i0221
  %.i1225 = fadd fast float %.i1216, %.i1222
  %.i2226 = fadd fast float %.i2217, %.i2223
  %.upto0377 = insertelement <3 x float> undef, float %.i0224, i32 0
  %.upto1378 = insertelement <3 x float> %.upto0377, float %.i1225, i32 1
  %133 = insertelement <3 x float> %.upto1378, float %.i2226, i32 2
  %134 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 0
  store <3 x float> %133, <3 x float>* %134, align 4, !tbaa !515
  %135 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 1
  store float 0.000000e+00, float* %135, align 4, !tbaa !518
  %136 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %rtreflection, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %137 = extractvalue %dx.types.CBufRet.i32 %136, 2
  %138 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 15)  ; CBufferLoadLegacy(handle,regIndex)
  %139 = extractvalue %dx.types.CBufRet.f32 %138, 1
  %140 = extractvalue %dx.types.CBufRet.f32 %138, 3
  %141 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %142 = extractvalue %dx.types.CBufRet.f32 %141, 1
  %143 = extractvalue %dx.types.CBufRet.f32 %141, 3
  %144 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %145 = extractvalue %dx.types.CBufRet.f32 %144, 1
  %146 = extractvalue %dx.types.CBufRet.f32 %144, 3
  %147 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 18)  ; CBufferLoadLegacy(handle,regIndex)
  %148 = extractvalue %dx.types.CBufRet.f32 %147, 0
  %149 = extractvalue %dx.types.CBufRet.f32 %147, 1
  %150 = extractvalue %dx.types.CBufRet.f32 %147, 3
  %151 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 19)  ; CBufferLoadLegacy(handle,regIndex)
  %152 = extractvalue %dx.types.CBufRet.f32 %151, 0
  %153 = extractvalue %dx.types.CBufRet.f32 %151, 1
  %154 = extractvalue %dx.types.CBufRet.f32 %151, 3
  %155 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 20)  ; CBufferLoadLegacy(handle,regIndex)
  %156 = extractvalue %dx.types.CBufRet.f32 %155, 0
  %157 = extractvalue %dx.types.CBufRet.f32 %155, 1
  %158 = extractvalue %dx.types.CBufRet.f32 %155, 3
  %159 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 21)  ; CBufferLoadLegacy(handle,regIndex)
  %160 = extractvalue %dx.types.CBufRet.f32 %159, 0
  %161 = extractvalue %dx.types.CBufRet.f32 %159, 1
  %162 = extractvalue %dx.types.CBufRet.f32 %159, 3
  %163 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %164 = extractvalue %dx.types.CBufRet.f32 %163, 0
  %165 = extractvalue %dx.types.CBufRet.f32 %163, 1
  %166 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 0
  store float %.i0167, float* %166, align 4
  %167 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 1
  store float %.i1168, float* %167, align 4
  %168 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 2
  store float %.i2169, float* %168, align 4
  %.i0231 = fmul fast float %FMad143, 4.000000e+00
  %.i1232 = fmul fast float %FMad141, 4.000000e+00
  %.i2233 = fmul fast float %FMad139, 4.000000e+00
  %Round_ne = call float @dx.op.unary.f32(i32 26, float %.i0231)  ; Round_ne(value)
  %Round_ne49 = call float @dx.op.unary.f32(i32 26, float %.i1232)  ; Round_ne(value)
  %Round_ne50 = call float @dx.op.unary.f32(i32 26, float %.i2233)  ; Round_ne(value)
  %169 = fmul fast float %Round_ne, %Round_ne
  %170 = fmul fast float %Round_ne49, %Round_ne49
  %171 = fadd fast float %170, %169
  %172 = fmul fast float %Round_ne50, %Round_ne50
  %173 = fadd fast float %171, %172
  %Sqrt48 = call float @dx.op.unary.f32(i32 24, float %173)  ; Sqrt(value)
  %.i0234 = fdiv fast float %Round_ne, %Sqrt48
  %.i1235 = fdiv fast float %Round_ne49, %Sqrt48
  %.i2236 = fdiv fast float %Round_ne50, %Sqrt48
  %FAbs57 = call float @dx.op.unary.f32(i32 6, float %.i0234)  ; FAbs(value)
  %FAbs58 = call float @dx.op.unary.f32(i32 6, float %.i1235)  ; FAbs(value)
  %FAbs59 = call float @dx.op.unary.f32(i32 6, float %.i2236)  ; FAbs(value)
  %FMax73 = call float @dx.op.binary.f32(i32 35, float %FAbs57, float %FAbs58)  ; FMax(a,b)
  %FMax72 = call float @dx.op.binary.f32(i32 35, float %FMax73, float %FAbs59)  ; FMax(a,b)
  %174 = fcmp fast oeq float %FMax72, %FAbs58
  %iMajorAxis.i.0 = zext i1 %174 to i32
  %175 = fcmp fast oeq float %FMax72, %FAbs59
  %iMajorAxis.i.1 = select i1 %175, i32 2, i32 %iMajorAxis.i.0
  %FMad134 = call float @dx.op.tertiary.f32(i32 46, float %17, float %142, float 0.000000e+00)  ; FMad(a,b,c)
  %176 = fadd fast float %FMad134, %145
  %FMad128 = call float @dx.op.tertiary.f32(i32 46, float %17, float %143, float 0.000000e+00)  ; FMad(a,b,c)
  %177 = fadd fast float %FMad128, %146
  %178 = fdiv fast float 1.000000e+00, %177
  %.i1238 = fmul fast float %178, %176
  %179 = fdiv fast float 2.000000e+00, %165
  %FMad105 = call float @dx.op.tertiary.f32(i32 46, float %179, float %139, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad104 = call float @dx.op.tertiary.f32(i32 46, float %17, float %142, float %FMad105)  ; FMad(a,b,c)
  %180 = fadd fast float %FMad104, %145
  %FMad99 = call float @dx.op.tertiary.f32(i32 46, float %179, float %140, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad98 = call float @dx.op.tertiary.f32(i32 46, float %17, float %143, float %FMad99)  ; FMad(a,b,c)
  %181 = fadd fast float %FMad98, %146
  %182 = fdiv fast float 1.000000e+00, %181
  %.i1241 = fmul fast float %182, %180
  %183 = fsub fast float %.i1241, %.i1238
  %FAbs = call float @dx.op.unary.f32(i32 6, float %183)  ; FAbs(value)
  %184 = fmul fast float %FAbs, 6.400000e+01
  %Log = call float @dx.op.unary.f32(i32 23, float %184)  ; Log(value)
  %Round_pi = call float @dx.op.unary.f32(i32 28, float %Log)  ; Round_pi(value)
  %Exp = call float @dx.op.unary.f32(i32 21, float %Round_pi)  ; Exp(value)
  %.i0243 = fdiv fast float %.i0167, %Exp
  %.i1244 = fdiv fast float %.i1168, %Exp
  %.i2245 = fdiv fast float %.i2169, %Exp
  %Round_ni51 = call float @dx.op.unary.f32(i32 27, float %.i0243)  ; Round_ni(value)
  %Round_ni52 = call float @dx.op.unary.f32(i32 27, float %.i1244)  ; Round_ni(value)
  %Round_ni53 = call float @dx.op.unary.f32(i32 27, float %.i2245)  ; Round_ni(value)
  %.i0246 = fptosi float %Round_ni51 to i32
  %.i1247 = fptosi float %Round_ni52 to i32
  %.i2248 = fptosi float %Round_ni53 to i32
  %.i0249 = sitofp i32 %.i0246 to float
  %.i1250 = sitofp i32 %.i1247 to float
  %.i2251 = sitofp i32 %.i2248 to float
  %.i0252 = fmul fast float %.i0249, %Exp
  %.i1253 = fmul fast float %.i1250, %Exp
  %.i2254 = fmul fast float %.i2251, %Exp
  %185 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 0
  store i32 %.i0246, i32* %185, align 4
  %186 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 1
  store i32 %.i1247, i32* %186, align 4
  %187 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 2
  store i32 %.i2248, i32* %187, align 4
  %188 = getelementptr [3 x i32], [3 x i32]* %1, i32 0, i32 %iMajorAxis.i.1
  store i32 0, i32* %188, align 4, !tbaa !520
  %189 = load i32, i32* %185, align 4
  %190 = load i32, i32* %186, align 4
  %191 = load i32, i32* %187, align 4
  %192 = shl i32 %190, 5
  %193 = xor i32 %192, %190
  %194 = shl i32 %191, 13
  %195 = xor i32 %194, %191
  %196 = add i32 %193, %189
  %197 = add i32 %196, %195
  %198 = xor i32 %197, 61
  %199 = lshr i32 %197, 16
  %200 = xor i32 %198, %199
  %201 = mul i32 %200, 9
  %202 = lshr i32 %201, 4
  %203 = xor i32 %202, %201
  %204 = mul i32 %203, 668265261
  %205 = lshr i32 %204, 15
  %206 = xor i32 %205, %204
  %207 = uitofp i32 %206 to float
  %208 = fmul fast float %207, 0x3DF0000000000000
  %209 = fdiv fast float %184, %Exp
  %210 = fmul fast float %209, 4.000000e+00
  %211 = fadd fast float %210, -2.000000e+00
  %212 = fsub fast float %211, %208
  %Saturate40 = call float @dx.op.unary.f32(i32 7, float %212)  ; Saturate(value)
  %213 = fmul fast float %Saturate40, 5.000000e-01
  %214 = fadd fast float %213, 5.000000e-01
  %215 = fmul fast float %214, %Exp
  %216 = load float, float* %166, align 4, !tbaa !518
  %217 = fsub fast float %216, %.i0252
  %218 = fcmp fast ogt float %217, %215
  %219 = fadd fast float %.i0249, 5.000000e-01
  %220 = fmul fast float %Exp, %219
  %221 = select i1 %218, float %220, float %.i0252
  %222 = load float, float* %167, align 4, !tbaa !518
  %223 = fsub fast float %222, %.i1253
  %224 = fcmp fast ogt float %223, %215
  %225 = fadd fast float %.i1250, 5.000000e-01
  %226 = fmul fast float %Exp, %225
  %227 = select i1 %224, float %226, float %.i1253
  %228 = load float, float* %168, align 4, !tbaa !518
  %229 = fsub fast float %228, %.i2254
  %230 = fcmp fast ogt float %229, %215
  %231 = fadd fast float %.i2251, 5.000000e-01
  %232 = fmul fast float %Exp, %231
  %233 = select i1 %230, float %232, float %.i2254
  %234 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 0
  store float %221, float* %234, align 4
  %235 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 1
  store float %227, float* %235, align 4
  %236 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 2
  store float %233, float* %236, align 4
  %237 = getelementptr [3 x float], [3 x float]* %2, i32 0, i32 %iMajorAxis.i.1
  store float 0.000000e+00, float* %237, align 4, !tbaa !518
  %238 = load float, float* %234, align 4
  %239 = load float, float* %235, align 4
  %240 = load float, float* %236, align 4
  %.i0327 = bitcast float %238 to i32
  %.i1328 = bitcast float %239 to i32
  %.i2329 = bitcast float %240 to i32
  %241 = shl i32 %.i1328, 5
  %242 = xor i32 %241, %.i1328
  %243 = shl i32 %.i2329, 13
  %244 = xor i32 %243, %.i2329
  %245 = add i32 %242, %.i0327
  %246 = add i32 %245, %244
  %247 = xor i32 %246, 61
  %248 = lshr i32 %246, 16
  %249 = xor i32 %247, %248
  %250 = mul i32 %249, 9
  %251 = lshr i32 %250, 4
  %252 = xor i32 %251, %250
  %253 = mul i32 %252, 668265261
  %254 = lshr i32 %253, 15
  %255 = xor i32 %254, %253
  %256 = lshr i32 %255, 16
  %.i0330 = fsub fast float %216, %221
  %.i1331 = fsub fast float %222, %227
  %.i2332 = fsub fast float %228, %233
  %257 = call float @dx.op.dot3.f32(i32 55, float %.i0330, float %.i1331, float %.i2332, float %.i0234, float %.i1235, float %.i2236)  ; Dot3(ax,ay,az,bx,by,bz)
  %.i0333 = fmul fast float %257, %.i0234
  %.i1334 = fmul fast float %257, %.i1235
  %.i2335 = fmul fast float %257, %.i2236
  %.i0336 = fadd fast float %.i0333, %221
  %.i1337 = fadd fast float %.i1334, %227
  %.i2338 = fadd fast float %.i2335, %233
  %258 = fmul fast float %.i0336, %148
  %FMad120 = call float @dx.op.tertiary.f32(i32 46, float %.i1337, float %152, float %258)  ; FMad(a,b,c)
  %FMad119 = call float @dx.op.tertiary.f32(i32 46, float %.i2338, float %156, float %FMad120)  ; FMad(a,b,c)
  %259 = fadd fast float %FMad119, %160
  %260 = fmul fast float %.i0336, %149
  %FMad117 = call float @dx.op.tertiary.f32(i32 46, float %.i1337, float %153, float %260)  ; FMad(a,b,c)
  %FMad116 = call float @dx.op.tertiary.f32(i32 46, float %.i2338, float %157, float %FMad117)  ; FMad(a,b,c)
  %261 = fadd fast float %FMad116, %161
  %262 = fmul fast float %.i0336, %150
  %FMad111 = call float @dx.op.tertiary.f32(i32 46, float %.i1337, float %154, float %262)  ; FMad(a,b,c)
  %FMad110 = call float @dx.op.tertiary.f32(i32 46, float %.i2338, float %158, float %FMad111)  ; FMad(a,b,c)
  %263 = fadd fast float %FMad110, %162
  %264 = fdiv fast float 1.000000e+00, %263
  %.i0339 = fmul fast float %259, 5.000000e-01
  %.i0342 = fmul fast float %.i0339, %264
  %.i1340 = fmul fast float %261, 5.000000e-01
  %.i1343 = fmul fast float %.i1340, %264
  %.i0344 = fadd fast float %.i0342, 5.000000e-01
  %.i1345435 = fsub fast float 5.000000e-01, %.i1343
  %.i0346 = fmul fast float %.i0344, %164
  %.i1347 = fmul fast float %.i1345435, %165
  %.i0229.neg = fsub fast float -5.000000e-01, %11
  %.i0348 = fadd fast float %.i0229.neg, %.i0346
  %.i1230.neg = fsub fast float -5.000000e-01, %12
  %.i1349 = fadd fast float %.i1230.neg, %.i1347
  %Round_ni = call float @dx.op.unary.f32(i32 27, float %.i0348)  ; Round_ni(value)
  %Round_ni46 = call float @dx.op.unary.f32(i32 27, float %.i1349)  ; Round_ni(value)
  %.i0350 = fptosi float %Round_ni to i32
  %.i1351 = fptosi float %Round_ni46 to i32
  %.i0352 = add i32 %.i0350, %256
  %.i1353 = add i32 %.i1351, %255
  %.i0354 = and i32 %.i0352, 255
  %.i1355 = and i32 %.i1353, 255
  %265 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %4)  ; CreateHandleForLib(Resource)
  %TextureLoad147 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %265, i32 0, i32 %.i0354, i32 %.i1355, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %.i2262 = extractvalue %dx.types.ResRet.f32 %TextureLoad147, 2
  %266 = icmp sgt i32 %137, 0
  br i1 %266, label %.lr.ph30.preheader, label %._crit_edge.31

.lr.ph30:                                         ; preds = %.lr.ph30.preheader, %._crit_edge
  %reflectionRayIndex.029 = phi i32 [ %291, %._crit_edge ], [ 0, %.lr.ph30.preheader ]
  %267 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)  ; CBufferLoadLegacy(handle,regIndex)
  %268 = extractvalue %dx.types.CBufRet.i32 %267, 1
  %269 = mul i32 %268, %137
  %270 = add i32 %269, %reflectionRayIndex.029
  %271 = uitofp i32 %270 to float
  %.i2257 = fmul fast float %271, 0x3FF9E377A0000000
  %.i2263 = fadd fast float %.i2257, %.i2262
  %Frc65 = call float @dx.op.unary.f32(i32 22, float %.i2263)  ; Frc(value)
  br i1 %311, label %.lr.ph.preheader, label %._crit_edge

.lr.ph.preheader:                                 ; preds = %.lr.ph30
  br label %.lr.ph

.lr.ph:                                           ; preds = %.lr.ph.preheader
  br label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph, %.lr.ph30
  %272 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %273 = extractvalue %dx.types.CBufRet.f32 %272, 0
  %274 = extractvalue %dx.types.CBufRet.f32 %272, 1
  %275 = extractvalue %dx.types.CBufRet.f32 %272, 2
  %276 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %277 = extractvalue %dx.types.CBufRet.f32 %276, 0
  %278 = extractvalue %dx.types.CBufRet.f32 %276, 1
  %279 = extractvalue %dx.types.CBufRet.f32 %276, 2
  %280 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %281 = extractvalue %dx.types.CBufRet.f32 %280, 0
  %282 = extractvalue %dx.types.CBufRet.f32 %280, 1
  %283 = extractvalue %dx.types.CBufRet.f32 %280, 2
  %284 = fmul fast float %273, %.i0291
  %FMad126 = call float @dx.op.tertiary.f32(i32 46, float %.i1292, float %277, float %284)  ; FMad(a,b,c)
  %FMad125 = call float @dx.op.tertiary.f32(i32 46, float %.i2293, float %281, float %FMad126)  ; FMad(a,b,c)
  %285 = fmul fast float %274, %.i0291
  %FMad124 = call float @dx.op.tertiary.f32(i32 46, float %.i1292, float %278, float %285)  ; FMad(a,b,c)
  %FMad123 = call float @dx.op.tertiary.f32(i32 46, float %.i2293, float %282, float %FMad124)  ; FMad(a,b,c)
  %286 = fmul fast float %275, %.i0291
  %FMad122 = call float @dx.op.tertiary.f32(i32 46, float %.i1292, float %279, float %286)  ; FMad(a,b,c)
  %FMad121 = call float @dx.op.tertiary.f32(i32 46, float %.i2293, float %283, float %FMad122)  ; FMad(a,b,c)
  %287 = fadd fast float %Frc65, 5.000000e-01
  %288 = fmul fast float %287, 0x4066666660000000
  %289 = fadd fast float %288, 0x4066666660000000
  store i32 %reflectionRayIndex.029, i32* %297, align 8
  %290 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %3)  ; CreateHandleForLib(Resource)
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %290, i32 0, i32 1, i32 0, i32 2, i32 0, float %299, float %300, float %301, float %302, float %FMad125, float %FMad123, float %FMad121, float %289, %struct.HitData* nonnull %10)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  %291 = add nuw nsw i32 %reflectionRayIndex.029, 1
  %exitcond = icmp eq i32 %291, %137
  br i1 %exitcond, label %._crit_edge.31.loopexit, label %.lr.ph30

._crit_edge.31.loopexit:                          ; preds = %._crit_edge
  %.lcssa = phi float [ %289, %._crit_edge ]
  %FMad121.lcssa = phi float [ %FMad121, %._crit_edge ]
  %FMad123.lcssa = phi float [ %FMad123, %._crit_edge ]
  %FMad125.lcssa = phi float [ %FMad125, %._crit_edge ]
  %292 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 3
  %293 = insertelement <3 x float> undef, float %FMad125.lcssa, i64 0
  %294 = insertelement <3 x float> %293, float %FMad123.lcssa, i64 1
  %295 = insertelement <3 x float> %294, float %FMad121.lcssa, i64 2
  store <3 x float> %295, <3 x float>* %296, align 4
  store float %.lcssa, float* %292, align 4
  br label %._crit_edge.31

._crit_edge.31:                                   ; preds = %._crit_edge.31.loopexit, %"\01?internal.getWorldSpaceNoiseOffset@@YA?AV?$vector@I$01@@V?$vector@M$01@@V?$matrix@M$03$03@@10MV?$vector@M$02@@22M_N@Z.exit33"
  ret void

.lr.ph30.preheader:                               ; preds = %"\01?internal.getWorldSpaceNoiseOffset@@YA?AV?$vector@I$01@@V?$vector@M$01@@V?$matrix@M$03$03@@10MV?$vector@M$02@@22M_N@Z.exit33"
  %.i0267 = fsub fast float -0.000000e+00, %.i0160
  %.i1268 = fsub fast float -0.000000e+00, %.i1161
  %.i2269 = fsub fast float -0.000000e+00, %.i2162
  %296 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 2
  %297 = getelementptr inbounds %struct.HitData, %struct.HitData* %10, i32 0, i32 0
  %298 = load <3 x float>, <3 x float>* %134, align 4
  %299 = extractelement <3 x float> %298, i64 0
  %300 = extractelement <3 x float> %298, i64 1
  %301 = extractelement <3 x float> %298, i64 2
  %302 = load float, float* %135, align 4
  %303 = call float @dx.op.dot3.f32(i32 55, float %.i0267, float %.i1268, float %.i2269, float %.i0206, float %.i1207, float %.i2208)  ; Dot3(ax,ay,az,bx,by,bz)
  %304 = fmul fast float %303, 2.000000e+00
  %.i0285 = fmul fast float %304, %.i0206
  %.i1286 = fmul fast float %304, %.i1207
  %.i2287 = fmul fast float %.i2208, %304
  %.i0288 = fadd fast float %.i0285, %.i0160
  %.i1289 = fadd fast float %.i1286, %.i1161
  %.i2290 = fadd fast float %.i2287, %.i2162
  %305 = fmul fast float %.i0288, %.i0288
  %306 = fmul fast float %.i1289, %.i1289
  %307 = fadd fast float %305, %306
  %308 = fmul fast float %.i2290, %.i2290
  %309 = fadd fast float %307, %308
  %Sqrt69 = call float @dx.op.unary.f32(i32 24, float %309)  ; Sqrt(value)
  %.i0291 = fdiv fast float %.i0288, %Sqrt69
  %.i1292 = fdiv fast float %.i1289, %Sqrt69
  %.i2293 = fdiv fast float %.i2290, %Sqrt69
  %310 = call float @dx.op.dot3.f32(i32 55, float %.i0291, float %.i1292, float %.i2293, float %.i0206, float %.i1207, float %.i2208)  ; Dot3(ax,ay,az,bx,by,bz)
  %311 = fcmp fast olt float %310, 0x3F747AE140000000
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
!dx.typeAnnotations = !{!18, !491}
!dx.entryPoints = !{!501, !504, !506, !508, !510, !512, !513, !514}

!0 = !{!"dxc 1.2"}
!1 = !{i32 1, i32 3}
!2 = !{!"lib", i32 6, i32 3}
!3 = !{!4, !12, !15, null}
!4 = !{!5, !7, !8, !9, !10}
!5 = !{i32 0, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer1", i32 0, i32 0, i32 1, i32 2, i32 0, !6}
!6 = !{i32 0, i32 9}
!7 = !{i32 1, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tLinearDepth", i32 0, i32 1, i32 1, i32 2, i32 0, !6}
!8 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tClipDepth", i32 0, i32 2, i32 1, i32 2, i32 0, !6}
!9 = !{i32 3, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tStaticBlueNoiseRGBA_0", i32 0, i32 3, i32 1, i32 2, i32 0, !6}
!10 = !{i32 4, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", !"g_rtScene", i32 0, i32 4, i32 1, i32 16, i32 0, !11}
!11 = !{i32 0, i32 4}
!12 = !{!13}
!13 = !{i32 0, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", !"g_rwtShadow", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !14}
!14 = !{i32 0, i32 5}
!15 = !{!16, !17}
!16 = !{i32 0, %sys_constants* @sys_constants, !"sys_constants", i32 0, i32 0, i32 1, i32 1128, null}
!17 = !{i32 1, %rtreflection* @rtreflection, !"rtreflection", i32 0, i32 1, i32 1, i32 24, null}
!18 = !{i32 0, %"class.Texture2D<vector<float, 4> >" undef, !19, %"class.Texture2D<vector<float, 4> >::mips_type" undef, !22, %struct.GBufferFormat undef, !24, %struct.SamplerState undef, !27, %"class.Texture2D<float>" undef, !29, %"class.Texture2D<float>::mips_type" undef, !22, %"class.Texture2D<unsigned int>" undef, !31, %"class.Texture2D<unsigned int>::mips_type" undef, !22, %"class.Texture2DMS<float, 0>" undef, !33, %"class.Texture2DMS<float, 0>::sample_type" undef, !22, %"class.StructuredBuffer<vector<unsigned int, 4> >" undef, !35, %"class.StructuredBuffer<MaterialDataPart1>" undef, !36, %struct.MaterialDataPart1 undef, !38, %"class.StructuredBuffer<MaterialDataPart3>" undef, !41, %struct.MaterialDataPart3 undef, !42, %"class.StructuredBuffer<MaterialBindingData>" undef, !86, %struct.MaterialBindingData undef, !87, %struct.order2_sh undef, !89, %"class.RWTexture3D<vector<float, 4> >" undef, !94, %"class.Texture3D<vector<float, 4> >" undef, !19, %"class.Texture3D<vector<float, 4> >::mips_type" undef, !22, %struct.LightVolumeData undef, !95, %"class.TextureCube<vector<float, 4> >" undef, !94, %"class.StructuredBuffer<DeferredLightPoint>" undef, !98, %struct.DeferredLightPoint undef, !99, %"class.StructuredBuffer<DeferredLightPointClipping>" undef, !114, %struct.DeferredLightPointClipping undef, !115, %"class.StructuredBuffer<DeferredLightPointBVH>" undef, !117, %struct.DeferredLightPointBVH undef, !118, %"class.RWTexture2D<unsigned int>" undef, !125, %"class.StructuredBuffer<DeferredLightSpot>" undef, !126, %struct.DeferredLightSpot undef, !127, %"class.StructuredBuffer<DeferredLightSpotClipping>" undef, !152, %struct.DeferredLightSpotClipping undef, !153, %"class.StructuredBuffer<DeferredLightSpotBVH>" undef, !117, %struct.DeferredLightSpotBVH undef, !118, %"class.StructuredBuffer<DeferredLightSun>" undef, !158, %struct.DeferredLightSun undef, !159, %"class.StructuredBuffer<VolumeSamplingInfo>" undef, !173, %struct.VolumeSamplingInfo undef, !174, %"class.StructuredBuffer<unsigned int>" undef, !125, %"class.StructuredBuffer<InternalNode>" undef, !114, %struct.InternalNode undef, !184, %"class.Texture3D<vector<float, 3> >" undef, !189, %"class.Texture3D<vector<float, 3> >::mips_type" undef, !22, %struct.VolumeCullingInCB undef, !191, %struct.VolumeTopLevelInfo undef, !196, %struct.DeferredLightSpecularBRDF undef, !200, %struct.DeferredLightGenericSurface undef, !203, %struct.DeferredLightTransparentBRDF undef, !207, %struct.DeferredLightIntensity undef, !210, %struct.DeferredLightVolumeResult undef, !213, %struct.DeferredLightPointSurface undef, !215, %struct.DeferredLightGBufferData undef, !216, %struct.CellInfo undef, !223, %"class.StructuredBuffer<EnvironmentMapDynamicInfo>" undef, !86, %struct.EnvironmentMapDynamicInfo undef, !249, %"class.StructuredBuffer<EnvironmentMapReference>" undef, !86, %struct.EnvironmentMapReference undef, !251, %"class.StructuredBuffer<CellInfo>" undef, !253, %"class.StructuredBuffer<Node>" undef, !254, %struct.Node undef, !255, %struct.ChildMask undef, !259, %struct.NodePointer undef, !261, %"class.StructuredBuffer<IrradianceProbe>" undef, !263, %struct.IrradianceProbe undef, !264, %"class.StructuredBuffer<TransportProbe>" undef, !274, %struct.TransportProbe undef, !275, %"class.StructuredBuffer<vector<float, 4> >" undef, !94, %"class.StructuredBuffer<ProbeRef>" undef, !36, %struct.ProbeRef undef, !277, %"class.StructuredBuffer<EnvironmentMapInfo>" undef, !280, %struct.EnvironmentMapInfo undef, !281, %struct.VoxelInfo undef, !287, %struct.LeafInfo undef, !293, %"class.StructuredBuffer<PerInstanceLBData>" undef, !114, %struct.PerInstanceLBData undef, !301, %"class.StructuredBuffer<RenderInstanceData>" undef, !305, %struct.RenderInstanceData undef, !306, %struct.ByteAddressBuffer undef, !27, %struct.RaytracingHitRootConstants undef, !314, %struct.RaytracingAccelerationStructure undef, !27, %struct.IntersectionAttributes undef, !325, %struct.HitData undef, !327, %"class.RWTexture2DArray<unsigned int>" undef, !125, %"class.RWTexture2DArray<vector<float, 4> >" undef, !94, %"class.RWStructuredBuffer<unsigned int>" undef, !125, %struct.RayDesc undef, !329, %debug_general undef, !334, %sys_constants undef, !357, %shadow_general undef, !390, %mid_translucency undef, !395, %mid_general undef, !87, %deferredlight_constants undef, !397, %env_general undef, !433, %atmosphere_general undef, !436, %illuminationvolume undef, !457, %ManualUpdateCB_ActiveVolumeTopLevelInfoArray undef, !464, %ManualUpdateCB_DebugVolumeTopLevelInfoArray undef, !466, %WorldGridInfo undef, !468, %EnvironmentMapAtlas undef, !472, %vertex_binding undef, !478, %g_cbRaytracingHit undef, !482, %rtreflection undef, !484}
!19 = !{i32 20, !20, !21}
!20 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 9}
!21 = !{i32 6, !"mips", i32 3, i32 16}
!22 = !{i32 4, !23}
!23 = !{i32 6, !"handle", i32 3, i32 0, i32 7, i32 5}
!24 = !{i32 32, !25, !26}
!25 = !{i32 6, !"vTarget1", i32 3, i32 0, i32 4, !"SV_Target0", i32 7, i32 9}
!26 = !{i32 6, !"vTarget2", i32 3, i32 16, i32 4, !"SV_Target1", i32 7, i32 9}
!27 = !{i32 4, !28}
!28 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 4}
!29 = !{i32 8, !20, !30}
!30 = !{i32 6, !"mips", i32 3, i32 4}
!31 = !{i32 8, !32, !30}
!32 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 5}
!33 = !{i32 8, !20, !34}
!34 = !{i32 6, !"sample", i32 3, i32 4}
!35 = !{i32 16, !32}
!36 = !{i32 8, !37}
!37 = !{i32 6, !"h", i32 3, i32 0}
!38 = !{i32 8, !39, !40}
!39 = !{i32 6, !"vSpecularColor_uBRDF", i32 3, i32 0, i32 7, i32 5}
!40 = !{i32 6, !"uScatterIndex_vTranslucencyDepthRange", i32 3, i32 4, i32 7, i32 5}
!41 = !{i32 352, !37}
!42 = !{i32 352, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85}
!43 = !{i32 6, !"fClothDiffScatterAmount", i32 3, i32 0, i32 7, i32 9}
!44 = !{i32 6, !"fClothFuzzWrap", i32 3, i32 4, i32 7, i32 9}
!45 = !{i32 6, !"vClothScatterColor", i32 3, i32 16, i32 7, i32 9}
!46 = !{i32 6, !"fHairSmoothness", i32 3, i32 28, i32 7, i32 9}
!47 = !{i32 6, !"vSpecularColor2", i32 3, i32 32, i32 7, i32 9}
!48 = !{i32 6, !"fHairSpecularShift", i32 3, i32 44, i32 7, i32 9}
!49 = !{i32 6, !"fHairDiffuseWrap", i32 3, i32 48, i32 7, i32 9}
!50 = !{i32 6, !"fPAD1", i32 3, i32 52, i32 7, i32 9}
!51 = !{i32 6, !"fEmissionIntensity", i32 3, i32 56, i32 7, i32 9}
!52 = !{i32 6, !"vColorMultiplier", i32 3, i32 64, i32 7, i32 9}
!53 = !{i32 6, !"vSmoothnessRange", i32 3, i32 80, i32 7, i32 9}
!54 = !{i32 6, !"vShadowSoftnessRange", i32 3, i32 88, i32 7, i32 9}
!55 = !{i32 6, !"vScatterWidthRange", i32 3, i32 96, i32 7, i32 9}
!56 = !{i32 6, !"fHairDepthOffsetAmount", i32 3, i32 104, i32 7, i32 9}
!57 = !{i32 6, !"fHairNormalAmount", i32 3, i32 108, i32 7, i32 9}
!58 = !{i32 6, !"vEmissionMultiplier", i32 3, i32 112, i32 7, i32 9}
!59 = !{i32 6, !"vColorMultiplier2", i32 3, i32 128, i32 7, i32 9}
!60 = !{i32 6, !"vBaseDetailUVScale", i32 3, i32 144, i32 7, i32 9}
!61 = !{i32 6, !"vSmoothnessRange2", i32 3, i32 152, i32 7, i32 9}
!62 = !{i32 6, !"vBlendDetailUVScale", i32 3, i32 160, i32 7, i32 9}
!63 = !{i32 6, !"fBaseDetailAmount", i32 3, i32 168, i32 7, i32 9}
!64 = !{i32 6, !"fBlendUVMultiplier", i32 3, i32 172, i32 7, i32 9}
!65 = !{i32 6, !"fBlendDetailAmount", i32 3, i32 176, i32 7, i32 9}
!66 = !{i32 6, !"fFuzzUVMultiplier", i32 3, i32 180, i32 7, i32 9}
!67 = !{i32 6, !"fPupilDilation", i32 3, i32 184, i32 7, i32 9}
!68 = !{i32 6, !"fPupilOuterRadius", i32 3, i32 188, i32 7, i32 9}
!69 = !{i32 6, !"vIntensity", i32 3, i32 192, i32 7, i32 9}
!70 = !{i32 6, !"vWrinkleWeights0", i32 3, i32 208, i32 7, i32 9}
!71 = !{i32 6, !"vWrinkleWeights1", i32 3, i32 224, i32 7, i32 9}
!72 = !{i32 6, !"vWrinkleWeights2", i32 3, i32 240, i32 7, i32 9}
!73 = !{i32 6, !"vWrinkleWeights3", i32 3, i32 256, i32 7, i32 9}
!74 = !{i32 6, !"vWrinkleWeights4", i32 3, i32 272, i32 7, i32 9}
!75 = !{i32 6, !"vWrinkleWeights5", i32 3, i32 288, i32 7, i32 9}
!76 = !{i32 6, !"uDisableBackfaceFlip", i32 3, i32 304, i32 7, i32 5}
!77 = !{i32 6, !"fEdgeSharpness", i32 3, i32 308, i32 7, i32 9}
!78 = !{i32 6, !"fTrunkBendFactor", i32 3, i32 312, i32 7, i32 9}
!79 = !{i32 6, !"fTrunkPivotOffset", i32 3, i32 316, i32 7, i32 9}
!80 = !{i32 6, !"vColorRgbMultiplier", i32 3, i32 320, i32 7, i32 9}
!81 = !{i32 6, !"fWindFactor", i32 3, i32 332, i32 7, i32 9}
!82 = !{i32 6, !"fDecalMaterialBlendFactor", i32 3, i32 336, i32 7, i32 9}
!83 = !{i32 6, !"fDecalAlbedoBlendFactor", i32 3, i32 340, i32 7, i32 9}
!84 = !{i32 6, !"uStableHash", i32 3, i32 344, i32 7, i32 5}
!85 = !{i32 6, !"pad", i32 3, i32 348, i32 7, i32 5}
!86 = !{i32 4, !37}
!87 = !{i32 4, !88}
!88 = !{i32 6, !"g_uMaterialID", i32 3, i32 0, i32 7, i32 5}
!89 = !{i32 60, !90, !91, !92, !93}
!90 = !{i32 6, !"c0", i32 3, i32 0, i32 7, i32 9}
!91 = !{i32 6, !"c11", i32 3, i32 16, i32 7, i32 9}
!92 = !{i32 6, !"c10", i32 3, i32 32, i32 7, i32 9}
!93 = !{i32 6, !"c1m1", i32 3, i32 48, i32 7, i32 9}
!94 = !{i32 16, !20}
!95 = !{i32 32, !96, !97}
!96 = !{i32 6, !"value0", i32 3, i32 0, i32 7, i32 9}
!97 = !{i32 6, !"value1", i32 3, i32 16, i32 7, i32 9}
!98 = !{i32 160, !37}
!99 = !{i32 160, !100, !101, !102, !103, !104, !105, !106, !107, !109, !110, !111, !112, !113}
!100 = !{i32 6, !"vPositionInView", i32 3, i32 0, i32 7, i32 9}
!101 = !{i32 6, !"fShadowMapShrink", i32 3, i32 12, i32 7, i32 9}
!102 = !{i32 6, !"vColor", i32 3, i32 16, i32 7, i32 9}
!103 = !{i32 6, !"fClipRange", i32 3, i32 28, i32 7, i32 9}
!104 = !{i32 6, !"vFalloff", i32 3, i32 32, i32 7, i32 9}
!105 = !{i32 6, !"vDirectionInView", i32 3, i32 48, i32 7, i32 9}
!106 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 60, i32 7, i32 9}
!107 = !{i32 6, !"mViewToShadowClip", i32 2, !108, i32 3, i32 64, i32 7, i32 9}
!108 = !{i32 4, i32 4, i32 2}
!109 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 128, i32 7, i32 9}
!110 = !{i32 6, !"fRadius", i32 3, i32 144, i32 7, i32 9}
!111 = !{i32 6, !"fBoundsRadius", i32 3, i32 148, i32 7, i32 9}
!112 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 152, i32 7, i32 9}
!113 = !{i32 6, !"uTechniqueProperties", i32 3, i32 156, i32 7, i32 5}
!114 = !{i32 16, !37}
!115 = !{i32 16, !100, !116}
!116 = !{i32 6, !"fBoundsRadius", i32 3, i32 12, i32 7, i32 9}
!117 = !{i32 44, !37}
!118 = !{i32 44, !119, !120, !121, !122, !123, !124}
!119 = !{i32 6, !"vMinInView", i32 3, i32 0, i32 7, i32 9}
!120 = !{i32 6, !"vMaxInView", i32 3, i32 16, i32 7, i32 9}
!121 = !{i32 6, !"uChildLHS", i32 3, i32 28, i32 7, i32 5}
!122 = !{i32 6, !"uChildRHS", i32 3, i32 32, i32 7, i32 5}
!123 = !{i32 6, !"uLightIndex", i32 3, i32 36, i32 7, i32 5}
!124 = !{i32 6, !"uCount", i32 3, i32 40, i32 7, i32 5}
!125 = !{i32 4, !32}
!126 = !{i32 336, !37}
!127 = !{i32 336, !100, !128, !129, !130, !131, !132, !133, !134, !135, !136, !137, !138, !139, !140, !141, !143, !144, !145, !146, !147, !148, !149, !150, !151}
!128 = !{i32 6, !"fNearDistance", i32 3, i32 12, i32 7, i32 9}
!129 = !{i32 6, !"vWedPositionInView", i32 3, i32 16, i32 7, i32 9}
!130 = !{i32 6, !"fFarDistance", i32 3, i32 28, i32 7, i32 9}
!131 = !{i32 6, !"vDirectionInView", i32 3, i32 32, i32 7, i32 9}
!132 = !{i32 6, !"fFarWidth", i32 3, i32 44, i32 7, i32 9}
!133 = !{i32 6, !"vColor", i32 3, i32 48, i32 7, i32 9}
!134 = !{i32 6, !"fClipRange", i32 3, i32 60, i32 7, i32 9}
!135 = !{i32 6, !"vFalloff", i32 3, i32 64, i32 7, i32 9}
!136 = !{i32 6, !"fRadius", i32 3, i32 72, i32 7, i32 9}
!137 = !{i32 6, !"fSinConeAnglePerTwo_sq", i32 3, i32 76, i32 7, i32 9}
!138 = !{i32 6, !"mViewToProjectionClip", i32 2, !108, i32 3, i32 80, i32 7, i32 9}
!139 = !{i32 6, !"mCameraPositionInCone", i32 3, i32 144, i32 7, i32 9}
!140 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 156, i32 7, i32 9}
!141 = !{i32 6, !"mViewToCone", i32 2, !142, i32 3, i32 160, i32 7, i32 9}
!142 = !{i32 3, i32 3, i32 2}
!143 = !{i32 6, !"fShadowPCFSize", i32 3, i32 204, i32 7, i32 9}
!144 = !{i32 6, !"fShadowLOD", i32 3, i32 208, i32 7, i32 9}
!145 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 212, i32 7, i32 9}
!146 = !{i32 6, !"mConeToView", i32 3, i32 224, i32 7, i32 9}
!147 = !{i32 6, !"vProjectionAtlasOffsetScale", i32 3, i32 272, i32 7, i32 9}
!148 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 288, i32 7, i32 9}
!149 = !{i32 6, !"vDynamicShadowAtlasOffsetScale", i32 3, i32 304, i32 7, i32 9}
!150 = !{i32 6, !"uTechniqueProperties", i32 3, i32 320, i32 7, i32 5}
!151 = !{i32 6, !"padTo16bytes", i32 3, i32 324, i32 7, i32 5}
!152 = !{i32 48, !37}
!153 = !{i32 48, !100, !128, !154, !130, !155, !156, !157}
!154 = !{i32 6, !"vDirectionInView", i32 3, i32 16, i32 7, i32 9}
!155 = !{i32 6, !"fFarWidth", i32 3, i32 32, i32 7, i32 9}
!156 = !{i32 6, !"uTechniqueProperties", i32 3, i32 36, i32 7, i32 5}
!157 = !{i32 6, !"padTo16bytes", i32 3, i32 40, i32 7, i32 5}
!158 = !{i32 708, !37}
!159 = !{i32 708, !160, !161, !102, !162, !163, !164, !165, !166, !167, !168, !169, !170, !171, !172}
!160 = !{i32 6, !"vDirectionInView", i32 3, i32 0, i32 7, i32 9}
!161 = !{i32 6, !"pad1", i32 3, i32 12, i32 7, i32 4}
!162 = !{i32 6, !"pad2", i32 3, i32 28, i32 7, i32 4}
!163 = !{i32 6, !"mViewToProjectionClip", i32 2, !108, i32 3, i32 32, i32 7, i32 9}
!164 = !{i32 6, !"fProjectionMapTransparency", i32 3, i32 96, i32 7, i32 9}
!165 = !{i32 6, !"fRadius", i32 3, i32 100, i32 7, i32 9}
!166 = !{i32 6, !"uTechniqueProperties", i32 3, i32 104, i32 7, i32 5}
!167 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 108, i32 7, i32 9}
!168 = !{i32 6, !"uCascadeCount", i32 3, i32 112, i32 7, i32 5}
!169 = !{i32 6, !"mCascadeViewToShadowClip", i32 2, !108, i32 3, i32 128, i32 7, i32 9}
!170 = !{i32 6, !"vCascadeRange", i32 3, i32 512, i32 7, i32 9}
!171 = !{i32 6, !"fCascadePCFWidth", i32 3, i32 608, i32 7, i32 9}
!172 = !{i32 6, !"pad3", i32 3, i32 704, i32 7, i32 4}
!173 = !{i32 92, !37}
!174 = !{i32 92, !175, !176, !177, !178, !179, !180, !181, !182, !183}
!175 = !{i32 6, !"mWorldToLocalRotationMatrix", i32 2, !142, i32 3, i32 0, i32 7, i32 9}
!176 = !{i32 6, !"vOneOverTopLevelFadeLength", i32 3, i32 48, i32 7, i32 9}
!177 = !{i32 6, !"uTopLevelNodeIndicesOffset", i32 3, i32 60, i32 7, i32 5}
!178 = !{i32 6, !"uInternalNodeIndexOffset", i32 3, i32 64, i32 7, i32 5}
!179 = !{i32 6, !"uBrickIndexOffset", i32 3, i32 68, i32 7, i32 5}
!180 = !{i32 6, !"fTopLevelTexelSizeInWorld", i32 3, i32 72, i32 7, i32 9}
!181 = !{i32 6, !"fWeightMultiplier", i32 3, i32 76, i32 7, i32 9}
!182 = !{i32 6, !"uAdditive", i32 3, i32 80, i32 7, i32 5}
!183 = !{i32 6, !"fpadTo16Bytes", i32 3, i32 84, i32 7, i32 9}
!184 = !{i32 16, !185, !186, !187, !188}
!185 = !{i32 6, !"uChildMask", i32 3, i32 0, i32 7, i32 5}
!186 = !{i32 6, !"uLeafChildMask", i32 3, i32 4, i32 7, i32 5}
!187 = !{i32 6, !"uChildBlockOffset", i32 3, i32 8, i32 7, i32 5}
!188 = !{i32 6, !"uLeafChildBlockOffset", i32 3, i32 12, i32 7, i32 5}
!189 = !{i32 16, !20, !190}
!190 = !{i32 6, !"mips", i32 3, i32 12}
!191 = !{i32 32, !192, !193, !194, !195}
!192 = !{i32 6, !"vAABBMinInView", i32 3, i32 0, i32 7, i32 9}
!193 = !{i32 6, !"fPad0", i32 3, i32 12, i32 7, i32 9}
!194 = !{i32 6, !"vAABBMaxInView", i32 3, i32 16, i32 7, i32 9}
!195 = !{i32 6, !"fPad1", i32 3, i32 28, i32 7, i32 9}
!196 = !{i32 64, !197, !198, !199}
!197 = !{i32 6, !"mWorldToTopLevelMatrixDecomp", i32 3, i32 0, i32 7, i32 9}
!198 = !{i32 6, !"vTopLevelSize", i32 3, i32 48, i32 7, i32 5}
!199 = !{i32 6, !"fPad0", i32 3, i32 60, i32 7, i32 9}
!200 = !{i32 28, !201, !202}
!201 = !{i32 6, !"vResultDiffuse", i32 3, i32 0, i32 7, i32 9}
!202 = !{i32 6, !"vResultSpecular", i32 3, i32 16, i32 7, i32 9}
!203 = !{i32 36, !100, !204, !205, !206}
!204 = !{i32 6, !"vNormalInView", i32 3, i32 16, i32 7, i32 9}
!205 = !{i32 6, !"fSmoothness", i32 3, i32 28, i32 7, i32 9}
!206 = !{i32 6, !"fSpecularAlbedoIntensity", i32 3, i32 32, i32 7, i32 9}
!207 = !{i32 44, !201, !208, !209}
!208 = !{i32 6, !"vResultDiffuseTransmission", i32 3, i32 16, i32 7, i32 9}
!209 = !{i32 6, !"vResultSpecular", i32 3, i32 32, i32 7, i32 9}
!210 = !{i32 28, !211, !212}
!211 = !{i32 6, !"vIntensityDiffuse", i32 3, i32 0, i32 7, i32 9}
!212 = !{i32 6, !"vIntensitySpecular", i32 3, i32 16, i32 7, i32 9}
!213 = !{i32 32, !214}
!214 = !{i32 6, !"d", i32 3, i32 0}
!215 = !{i32 12, !100}
!216 = !{i32 68, !217, !218, !219, !220, !221, !222}
!217 = !{i32 6, !"vGBufferTarget1", i32 3, i32 0, i32 7, i32 9}
!218 = !{i32 6, !"vGBufferTarget2", i32 3, i32 16, i32 7, i32 9}
!219 = !{i32 6, !"vPositionInView", i32 3, i32 32, i32 7, i32 9}
!220 = !{i32 6, !"vBentNormalInView", i32 3, i32 48, i32 7, i32 9}
!221 = !{i32 6, !"fDiffuseOcclusion", i32 3, i32 60, i32 7, i32 9}
!222 = !{i32 6, !"fSpecularOcclusion", i32 3, i32 64, i32 7, i32 9}
!223 = !{i32 356, !224, !225, !226, !227, !228, !229, !230, !231, !232, !233, !234, !235, !236, !237, !238, !239, !240, !241, !242, !243, !244, !245, !246, !247, !248}
!224 = !{i32 6, !"m_vMin", i32 3, i32 0, i32 7, i32 9}
!225 = !{i32 6, !"m_fBaseVoxelSize", i32 3, i32 12, i32 7, i32 9}
!226 = !{i32 6, !"m_vMax", i32 3, i32 16, i32 7, i32 9}
!227 = !{i32 6, !"m_fInvBaseVoxelSize", i32 3, i32 28, i32 7, i32 9}
!228 = !{i32 6, !"m_uVoxelResolution", i32 3, i32 32, i32 7, i32 5}
!229 = !{i32 6, !"m_uLog2VoxelResolution", i32 3, i32 36, i32 7, i32 5}
!230 = !{i32 6, !"m_uBaseVoxelResolutionX", i32 3, i32 40, i32 7, i32 5}
!231 = !{i32 6, !"m_uBaseVoxelResolutionXY", i32 3, i32 44, i32 7, i32 5}
!232 = !{i32 6, !"m_iPrimaryTreeOffset", i32 3, i32 48, i32 7, i32 4}
!233 = !{i32 6, !"m_iDualTreeOffset", i32 3, i32 52, i32 7, i32 4}
!234 = !{i32 6, !"m_iIrradianceProbeOffset", i32 3, i32 56, i32 7, i32 4}
!235 = !{i32 6, !"m_iDirectTransportProbeOffset", i32 3, i32 60, i32 7, i32 4}
!236 = !{i32 6, !"m_iIndirectTransportProbeOffset", i32 3, i32 64, i32 7, i32 4}
!237 = !{i32 6, !"m_iLightGatherOffset", i32 3, i32 68, i32 7, i32 4}
!238 = !{i32 6, !"m_iLightProbeRefOffset", i32 3, i32 72, i32 7, i32 4}
!239 = !{i32 6, !"m_iDualLightProbeRefOffset", i32 3, i32 76, i32 7, i32 4}
!240 = !{i32 6, !"m_iEnvironmentMapRefOffset", i32 3, i32 80, i32 7, i32 4}
!241 = !{i32 6, !"m_iEnvironmentMapDualRefOffset", i32 3, i32 84, i32 7, i32 4}
!242 = !{i32 6, !"m_iEnvironmentMapInfoOffset", i32 3, i32 88, i32 7, i32 4}
!243 = !{i32 6, !"m_fWorldToGrid", i32 3, i32 92, i32 7, i32 9}
!244 = !{i32 6, !"m_fGridToWorld", i32 3, i32 96, i32 7, i32 9}
!245 = !{i32 6, !"pad0", i32 3, i32 100, i32 7, i32 9}
!246 = !{i32 6, !"pad1", i32 3, i32 104, i32 7, i32 9}
!247 = !{i32 6, !"pad2", i32 3, i32 108, i32 7, i32 9}
!248 = !{i32 6, !"m_iRowTranslationTable", i32 3, i32 112, i32 7, i32 4}
!249 = !{i32 4, !250}
!250 = !{i32 6, !"m_fDiffuseC0", i32 3, i32 0, i32 7, i32 9}
!251 = !{i32 4, !252}
!252 = !{i32 6, !"m_data", i32 3, i32 0, i32 7, i32 5}
!253 = !{i32 356, !37}
!254 = !{i32 28, !37}
!255 = !{i32 28, !256, !257, !258}
!256 = !{i32 6, !"m_childMask", i32 3, i32 0}
!257 = !{i32 6, !"m_pointer", i32 3, i32 20}
!258 = !{i32 6, !"m_payload", i32 3, i32 24, i32 7, i32 5}
!259 = !{i32 20, !260}
!260 = !{i32 6, !"m_uMask", i32 3, i32 0, i32 7, i32 5}
!261 = !{i32 4, !262}
!262 = !{i32 6, !"m_flagAndPointer", i32 3, i32 0, i32 7, i32 5}
!263 = !{i32 36, !37}
!264 = !{i32 36, !265, !266, !267, !268, !269, !270, !271, !272, !273}
!265 = !{i32 6, !"m_vR00", i32 3, i32 0, i32 7, i32 5}
!266 = !{i32 6, !"m_vR01", i32 3, i32 4, i32 7, i32 5}
!267 = !{i32 6, !"m_vR02", i32 3, i32 8, i32 7, i32 5}
!268 = !{i32 6, !"m_vR10", i32 3, i32 12, i32 7, i32 5}
!269 = !{i32 6, !"m_vR11", i32 3, i32 16, i32 7, i32 5}
!270 = !{i32 6, !"m_vR12", i32 3, i32 20, i32 7, i32 5}
!271 = !{i32 6, !"m_vR20", i32 3, i32 24, i32 7, i32 5}
!272 = !{i32 6, !"m_vR21", i32 3, i32 28, i32 7, i32 5}
!273 = !{i32 6, !"m_vR22", i32 3, i32 32, i32 7, i32 5}
!274 = !{i32 116, !37}
!275 = !{i32 116, !276}
!276 = !{i32 6, !"m_mTransport", i32 3, i32 0, i32 7, i32 5}
!277 = !{i32 8, !278, !279}
!278 = !{i32 6, !"m_index1", i32 3, i32 0, i32 7, i32 5}
!279 = !{i32 6, !"m_index2", i32 3, i32 4, i32 7, i32 5}
!280 = !{i32 220, !37}
!281 = !{i32 220, !282, !283, !284, !285, !286}
!282 = !{i32 6, !"m_vDebugColor", i32 3, i32 0, i32 7, i32 9}
!283 = !{i32 6, !"m_fRadius", i32 3, i32 12, i32 7, i32 9}
!284 = !{i32 6, !"m_vCenter", i32 3, i32 16, i32 7, i32 9}
!285 = !{i32 6, !"m_vMin", i32 3, i32 32, i32 7, i32 9}
!286 = !{i32 6, !"m_vMax", i32 3, i32 128, i32 7, i32 9}
!287 = !{i32 44, !288, !289, !290, !291, !292}
!288 = !{i32 6, !"m_vVoxelMin", i32 3, i32 0, i32 7, i32 9}
!289 = !{i32 6, !"m_fVoxelSize", i32 3, i32 12, i32 7, i32 9}
!290 = !{i32 6, !"m_uPayload", i32 3, i32 16, i32 7, i32 5}
!291 = !{i32 6, !"m_uLinearBaseVoxelIndex", i32 3, i32 20, i32 7, i32 5}
!292 = !{i32 6, !"m_vBaseVoxelMin", i32 3, i32 32, i32 7, i32 9}
!293 = !{i32 44, !294, !295, !296, !297, !298, !299, !300}
!294 = !{i32 6, !"m_iParentNode", i32 3, i32 0, i32 7, i32 4}
!295 = !{i32 6, !"m_vParentVoxelMin", i32 3, i32 4, i32 7, i32 9}
!296 = !{i32 6, !"m_vLeafVoxelMin", i32 3, i32 16, i32 7, i32 9}
!297 = !{i32 6, !"m_fLeafVoxelSize", i32 3, i32 28, i32 7, i32 9}
!298 = !{i32 6, !"m_bIsTerminal", i32 3, i32 32, i32 7, i32 1}
!299 = !{i32 6, !"m_bIsSolid", i32 3, i32 36, i32 7, i32 1}
!300 = !{i32 6, !"m_iPayload", i32 3, i32 40, i32 7, i32 4}
!301 = !{i32 16, !302, !303, !304}
!302 = !{i32 6, !"m_vUVOffset", i32 3, i32 0, i32 7, i32 9}
!303 = !{i32 6, !"m_uEnvMapIndex", i32 3, i32 8, i32 7, i32 5}
!304 = !{i32 6, !"m_uEnvInfoIndex", i32 3, i32 12, i32 7, i32 5}
!305 = !{i32 152, !37}
!306 = !{i32 152, !307, !308, !309, !310, !311, !312, !313}
!307 = !{i32 6, !"g_fPerLODDissolve", i32 3, i32 0, i32 7, i32 9}
!308 = !{i32 6, !"g_vWindIndex_Base_Trunk", i32 3, i32 116, i32 7, i32 5}
!309 = !{i32 6, !"g_uMask", i32 3, i32 124, i32 7, i32 5}
!310 = !{i32 6, !"g_iCharacterLightRigIndex", i32 3, i32 128, i32 7, i32 4}
!311 = !{i32 6, !"g_fDepthMultiply", i32 3, i32 132, i32 7, i32 9}
!312 = !{i32 6, !"g_fExtraDepthMultiply", i32 3, i32 136, i32 7, i32 9}
!313 = !{i32 6, !"padTo16Bytes", i32 3, i32 144, i32 7, i32 4}
!314 = !{i32 40, !315, !316, !317, !318, !319, !320, !321, !322, !323, !324}
!315 = !{i32 6, !"uIndexOffset", i32 3, i32 0, i32 7, i32 5}
!316 = !{i32 6, !"uIndexStride", i32 3, i32 4, i32 7, i32 5}
!317 = !{i32 6, !"uVertexStride1", i32 3, i32 8, i32 7, i32 5}
!318 = !{i32 6, !"uMaterialID", i32 3, i32 12, i32 7, i32 5}
!319 = !{i32 6, !"uTangentOffset", i32 3, i32 16, i32 7, i32 5}
!320 = !{i32 6, !"uNormalOffset", i32 3, i32 20, i32 7, i32 5}
!321 = !{i32 6, !"uTexcoordOffset", i32 3, i32 24, i32 7, i32 5}
!322 = !{i32 6, !"uColorOffset", i32 3, i32 28, i32 7, i32 5}
!323 = !{i32 6, !"uIndexBufferSize", i32 3, i32 32, i32 7, i32 5}
!324 = !{i32 6, !"uVertexBuffer1Size", i32 3, i32 36, i32 7, i32 5}
!325 = !{i32 8, !326}
!326 = !{i32 6, !"vIntersection", i32 3, i32 0, i32 4, !"INTERSECTION", i32 7, i32 9}
!327 = !{i32 4, !328}
!328 = !{i32 6, !"uRayIndex", i32 3, i32 0, i32 4, !"RAY_INDEX", i32 7, i32 5}
!329 = !{i32 32, !330, !331, !332, !333}
!330 = !{i32 6, !"Origin", i32 3, i32 0, i32 7, i32 9}
!331 = !{i32 6, !"TMin", i32 3, i32 12, i32 7, i32 9}
!332 = !{i32 6, !"Direction", i32 3, i32 16, i32 7, i32 9}
!333 = !{i32 6, !"TMax", i32 3, i32 28, i32 7, i32 9}
!334 = !{i32 88, !335, !336, !337, !338, !339, !340, !341, !342, !343, !344, !345, !346, !347, !348, !349, !350, !351, !352, !353, !354, !355, !356}
!335 = !{i32 6, !"g_fDebug_h_Slider0", i32 3, i32 0, i32 7, i32 9}
!336 = !{i32 6, !"g_fDebug_h_Slider1", i32 3, i32 4, i32 7, i32 9}
!337 = !{i32 6, !"g_fDebug_h_Slider2", i32 3, i32 8, i32 7, i32 9}
!338 = !{i32 6, !"g_fDebug_h_Slider3", i32 3, i32 12, i32 7, i32 9}
!339 = !{i32 6, !"g_fDebug_h_Slider4", i32 3, i32 16, i32 7, i32 9}
!340 = !{i32 6, !"g_fDebug_h_Slider5", i32 3, i32 20, i32 7, i32 9}
!341 = !{i32 6, !"g_fDebug_h_Slider6", i32 3, i32 24, i32 7, i32 9}
!342 = !{i32 6, !"g_fDebug_h_Slider7", i32 3, i32 28, i32 7, i32 9}
!343 = !{i32 6, !"g_fDebug_h_Knob0", i32 3, i32 32, i32 7, i32 9}
!344 = !{i32 6, !"g_fDebug_h_Knob1", i32 3, i32 36, i32 7, i32 9}
!345 = !{i32 6, !"g_fDebug_h_Knob2", i32 3, i32 40, i32 7, i32 9}
!346 = !{i32 6, !"g_fDebug_h_Knob3", i32 3, i32 44, i32 7, i32 9}
!347 = !{i32 6, !"g_fDebug_h_Knob4", i32 3, i32 48, i32 7, i32 9}
!348 = !{i32 6, !"g_fDebug_h_Knob5", i32 3, i32 52, i32 7, i32 9}
!349 = !{i32 6, !"g_fDebug_h_Knob6", i32 3, i32 56, i32 7, i32 9}
!350 = !{i32 6, !"g_fDebug_h_Knob7", i32 3, i32 60, i32 7, i32 9}
!351 = !{i32 6, !"g_fDebug_h_Toggle0", i32 3, i32 64, i32 7, i32 9}
!352 = !{i32 6, !"g_fDebug_h_Toggle1", i32 3, i32 68, i32 7, i32 9}
!353 = !{i32 6, !"g_fDebug_h_Toggle2", i32 3, i32 72, i32 7, i32 9}
!354 = !{i32 6, !"g_fDebug_h_Toggle3", i32 3, i32 76, i32 7, i32 9}
!355 = !{i32 6, !"g_fDebug_h_Toggle4", i32 3, i32 80, i32 7, i32 9}
!356 = !{i32 6, !"g_fDebug_h_Toggle5", i32 3, i32 84, i32 7, i32 9}
!357 = !{i32 1128, !358, !359, !360, !361, !362, !363, !364, !365, !366, !367, !368, !369, !370, !371, !372, !373, !374, !375, !376, !377, !378, !379, !380, !381, !382, !383, !384, !385, !386, !387, !388, !389}
!358 = !{i32 6, !"g_vScreenRes", i32 3, i32 0, i32 7, i32 9}
!359 = !{i32 6, !"g_vInvScreenRes", i32 3, i32 8, i32 7, i32 9}
!360 = !{i32 6, !"g_vOutputRes", i32 3, i32 16, i32 7, i32 9}
!361 = !{i32 6, !"g_vInvOutputRes", i32 3, i32 24, i32 7, i32 9}
!362 = !{i32 6, !"g_mWorldToView", i32 2, !108, i32 3, i32 32, i32 7, i32 9}
!363 = !{i32 6, !"g_mViewToWorld", i32 2, !108, i32 3, i32 96, i32 7, i32 9}
!364 = !{i32 6, !"g_mViewToClip", i32 2, !108, i32 3, i32 160, i32 7, i32 9}
!365 = !{i32 6, !"g_mClipToView", i32 2, !108, i32 3, i32 224, i32 7, i32 9}
!366 = !{i32 6, !"g_mWorldToClip", i32 2, !108, i32 3, i32 288, i32 7, i32 9}
!367 = !{i32 6, !"g_mClipToWorld", i32 2, !108, i32 3, i32 352, i32 7, i32 9}
!368 = !{i32 6, !"g_mClipToPreviousClip", i32 2, !108, i32 3, i32 416, i32 7, i32 9}
!369 = !{i32 6, !"g_mViewToPreviousClip", i32 2, !108, i32 3, i32 480, i32 7, i32 9}
!370 = !{i32 6, !"g_mPreviousViewToView", i32 2, !108, i32 3, i32 544, i32 7, i32 9}
!371 = !{i32 6, !"g_mPreviousWorldToClip", i32 2, !108, i32 3, i32 608, i32 7, i32 9}
!372 = !{i32 6, !"g_mPreviousViewToClip", i32 2, !108, i32 3, i32 672, i32 7, i32 9}
!373 = !{i32 6, !"g_mClipToPreviousClipNoJitter", i32 2, !108, i32 3, i32 736, i32 7, i32 9}
!374 = !{i32 6, !"g_mPreviousClipToClipNoJitter", i32 2, !108, i32 3, i32 800, i32 7, i32 9}
!375 = !{i32 6, !"g_vViewPoint", i32 3, i32 864, i32 7, i32 9}
!376 = !{i32 6, !"g_fInvNear", i32 3, i32 880, i32 7, i32 9}
!377 = !{i32 6, !"g_mViewToCameraView", i32 2, !108, i32 3, i32 896, i32 7, i32 9}
!378 = !{i32 6, !"g_mCameraViewToCameraClip", i32 2, !108, i32 3, i32 960, i32 7, i32 9}
!379 = !{i32 6, !"g_mCameraClipToView", i32 2, !108, i32 3, i32 1024, i32 7, i32 9}
!380 = !{i32 6, !"g_fWorldTime", i32 3, i32 1088, i32 7, i32 9}
!381 = !{i32 6, !"g_fWorldTimeDelta", i32 3, i32 1092, i32 7, i32 9}
!382 = !{i32 6, !"g_fRealTime", i32 3, i32 1096, i32 7, i32 9}
!383 = !{i32 6, !"g_fRealTimeDelta", i32 3, i32 1100, i32 7, i32 9}
!384 = !{i32 6, !"g_fLastValidWorldTimeDelta", i32 3, i32 1104, i32 7, i32 9}
!385 = !{i32 6, !"g_uTemporalFrame", i32 3, i32 1108, i32 7, i32 5}
!386 = !{i32 6, !"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5}
!387 = !{i32 6, !"g_bHDR", i32 3, i32 1116, i32 7, i32 1}
!388 = !{i32 6, !"g_fHDRSDRSceneBrightnessMultiplier", i32 3, i32 1120, i32 7, i32 9}
!389 = !{i32 6, !"g_fHDRSDRUIBrightnessMultiplier", i32 3, i32 1124, i32 7, i32 9}
!390 = !{i32 32, !391, !392, !393, !394}
!391 = !{i32 6, !"g_vShadowMapRes", i32 3, i32 0, i32 7, i32 9}
!392 = !{i32 6, !"g_vShadowMapVSMRes", i32 3, i32 8, i32 7, i32 9}
!393 = !{i32 6, !"g_vJitterOffset", i32 3, i32 16, i32 7, i32 9}
!394 = !{i32 6, !"g_vSnapOffset", i32 3, i32 24, i32 7, i32 4}
!395 = !{i32 4, !396}
!396 = !{i32 6, !"g_fOnePerTranslucencyKernelCount", i32 3, i32 0, i32 7, i32 9}
!397 = !{i32 408, !398, !399, !400, !401, !402, !403, !404, !405, !406, !407, !408, !409, !410, !411, !412, !413, !414, !415, !416, !417, !418, !419, !420, !421, !422, !423, !424, !425, !426, !427, !428, !429, !430, !431, !432}
!398 = !{i32 6, !"g_fTileDepthClipRanges", i32 3, i32 0, i32 7, i32 9}
!399 = !{i32 6, !"g_fTileDepthRanges", i32 3, i32 80, i32 7, i32 9}
!400 = !{i32 6, !"g_vDepthTileResolve", i32 3, i32 160, i32 7, i32 9}
!401 = !{i32 6, !"g_uDepthTileCount", i32 3, i32 168, i32 7, i32 5}
!402 = !{i32 6, !"g_vTileResolution", i32 3, i32 176, i32 7, i32 5}
!403 = !{i32 6, !"g_vTileWidthHeightDepth", i32 3, i32 192, i32 7, i32 5}
!404 = !{i32 6, !"g_vTileResolutionPerScreenResolution", i32 3, i32 208, i32 7, i32 9}
!405 = !{i32 6, !"g_vTileDepthNearFar", i32 3, i32 216, i32 7, i32 9}
!406 = !{i32 6, !"g_uMaxPointLightsPerTile", i32 3, i32 224, i32 7, i32 5}
!407 = !{i32 6, !"g_uMaxSpotLightsPerTile", i32 3, i32 228, i32 7, i32 5}
!408 = !{i32 6, !"g_uAmbientLightTotalCount", i32 3, i32 232, i32 7, i32 5}
!409 = !{i32 6, !"g_fAmbientEnvDiffuseIntensity", i32 3, i32 236, i32 7, i32 9}
!410 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnTransparent", i32 3, i32 240, i32 7, i32 9}
!411 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnOpaque", i32 3, i32 244, i32 7, i32 9}
!412 = !{i32 6, !"g_fAmbientSkyIntensity", i32 3, i32 248, i32 7, i32 9}
!413 = !{i32 6, !"g_fAmbientLocalIntensity", i32 3, i32 252, i32 7, i32 9}
!414 = !{i32 6, !"g_uPointLightTotalCount", i32 3, i32 256, i32 7, i32 5}
!415 = !{i32 6, !"g_uSpotLightTotalCount", i32 3, i32 260, i32 7, i32 5}
!416 = !{i32 6, !"g_uSunLightTotalCount", i32 3, i32 264, i32 7, i32 5}
!417 = !{i32 6, !"g_uAmbientLightEnabled", i32 3, i32 268, i32 7, i32 5}
!418 = !{i32 6, !"g_fAmbientDepthRampDistance", i32 3, i32 272, i32 7, i32 9}
!419 = !{i32 6, !"g_fAmbientDepthRampFeather", i32 3, i32 276, i32 7, i32 9}
!420 = !{i32 6, !"g_fAmbientDepthRampIntensityNear", i32 3, i32 280, i32 7, i32 9}
!421 = !{i32 6, !"g_fAmbientDepthRampIntensityFar", i32 3, i32 284, i32 7, i32 9}
!422 = !{i32 6, !"g_vVolumeLightDimensions", i32 3, i32 288, i32 7, i32 5}
!423 = !{i32 6, !"g_vVolumeLightProjectionConstants", i32 3, i32 304, i32 7, i32 9}
!424 = !{i32 6, !"g_vHalfResVolumeLightProjectionConstants", i32 3, i32 320, i32 7, i32 9}
!425 = !{i32 6, !"g_vOnePerVolumeLightDimensions", i32 3, i32 336, i32 7, i32 9}
!426 = !{i32 6, !"g_vVolumeLightXYToTileXY", i32 3, i32 352, i32 7, i32 9}
!427 = !{i32 6, !"g_vVolumeLightDepthResolve", i32 3, i32 368, i32 7, i32 9}
!428 = !{i32 6, !"g_fVolumeLightOnePerDepthMinusOne", i32 3, i32 380, i32 7, i32 9}
!429 = !{i32 6, !"g_vVolumeLightNearSplit0Far", i32 3, i32 384, i32 7, i32 9}
!430 = !{i32 6, !"g_fVolumeLightKernelWidth", i32 3, i32 396, i32 7, i32 9}
!431 = !{i32 6, !"g_fVolumeLightSamplingBias", i32 3, i32 400, i32 7, i32 9}
!432 = !{i32 6, !"g_uRTContactShadowRayCount", i32 3, i32 404, i32 7, i32 5}
!433 = !{i32 8, !434, !435}
!434 = !{i32 6, !"g_fEnvReflectionEdgeLength", i32 3, i32 0, i32 7, i32 9}
!435 = !{i32 6, !"g_fEnvReflectionMipCount", i32 3, i32 4, i32 7, i32 9}
!436 = !{i32 188, !437, !438, !439, !440, !441, !442, !443, !444, !445, !446, !447, !448, !449, !450, !451, !452, !453, !454, !455, !456}
!437 = !{i32 6, !"g_atmosphere_vSunDir", i32 3, i32 0, i32 7, i32 9}
!438 = !{i32 6, !"g_atmosphere_vSunE", i32 3, i32 16, i32 7, i32 9}
!439 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants1", i32 3, i32 32, i32 7, i32 9}
!440 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants2", i32 3, i32 40, i32 7, i32 9}
!441 = !{i32 6, !"g_atmosphere_vPhaseBlend", i32 3, i32 48, i32 7, i32 9}
!442 = !{i32 6, !"g_atmosphere_vPhaseG", i32 3, i32 52, i32 7, i32 9}
!443 = !{i32 6, !"g_atmosphere_vFog", i32 3, i32 64, i32 7, i32 9}
!444 = !{i32 6, !"g_vFogColor", i32 3, i32 80, i32 7, i32 9}
!445 = !{i32 6, !"g_vFogColorOpposite", i32 3, i32 96, i32 7, i32 9}
!446 = !{i32 6, !"g_fFogExp", i32 3, i32 108, i32 7, i32 9}
!447 = !{i32 6, !"g_fFogGroundDensityAtViewer", i32 3, i32 112, i32 7, i32 9}
!448 = !{i32 6, !"g_fFogGroundHeight", i32 3, i32 116, i32 7, i32 9}
!449 = !{i32 6, !"g_fFogGroundFalloff", i32 3, i32 120, i32 7, i32 9}
!450 = !{i32 6, !"g_fFogGroundDensity", i32 3, i32 124, i32 7, i32 9}
!451 = !{i32 6, !"g_vFogGroundDensityMapRange", i32 3, i32 128, i32 7, i32 9}
!452 = !{i32 6, !"g_vFogGroundSimulationVelocityAndScale", i32 3, i32 144, i32 7, i32 9}
!453 = !{i32 6, !"g_fFogDepthRampDistance", i32 3, i32 156, i32 7, i32 9}
!454 = !{i32 6, !"g_fFogDepthRampFeather", i32 3, i32 160, i32 7, i32 9}
!455 = !{i32 6, !"g_fFogDepthRampIntensityNear", i32 3, i32 164, i32 7, i32 9}
!456 = !{i32 6, !"g_vFogAlbedo", i32 3, i32 176, i32 7, i32 9}
!457 = !{i32 40, !458, !459, !460, !461, !462, !463}
!458 = !{i32 6, !"g_vOnePerVolumeAtlasSize", i32 3, i32 0, i32 7, i32 9}
!459 = !{i32 6, !"g_uActiveVolumeCount", i32 3, i32 12, i32 7, i32 5}
!460 = !{i32 6, !"g_vAdditiveAmbient", i32 3, i32 16, i32 7, i32 9}
!461 = !{i32 6, !"g_uActiveDebugVolumeCount", i32 3, i32 28, i32 7, i32 5}
!462 = !{i32 6, !"g_fTransparentBalanceEnergy", i32 3, i32 32, i32 7, i32 9}
!463 = !{i32 6, !"g_fTransparentBalanceEnergyStrength", i32 3, i32 36, i32 7, i32 9}
!464 = !{i32 24576, !465}
!465 = !{i32 6, !"g_cbActiveVolumeTopLevelInfoArray", i32 3, i32 0}
!466 = !{i32 24576, !467}
!467 = !{i32 6, !"g_cbDebugVolumeTopLevelInfoArray", i32 3, i32 0}
!468 = !{i32 12, !469, !470, !471}
!469 = !{i32 6, !"g_fWorldSize", i32 3, i32 0, i32 7, i32 9}
!470 = !{i32 6, !"g_fCellSize", i32 3, i32 4, i32 7, i32 9}
!471 = !{i32 6, !"g_iCellCount", i32 3, i32 8, i32 7, i32 4}
!472 = !{i32 24, !473, !474, !475, !476, !477}
!473 = !{i32 6, !"g_fInvEnvironmentMapsPerRow", i32 3, i32 0, i32 7, i32 9}
!474 = !{i32 6, !"g_fEnvironmentMapsPerRow", i32 3, i32 4, i32 7, i32 9}
!475 = !{i32 6, !"g_fEnvironmentMapColSize", i32 3, i32 8, i32 7, i32 9}
!476 = !{i32 6, !"g_fEnvironmentMapRowSize", i32 3, i32 12, i32 7, i32 9}
!477 = !{i32 6, !"g_fInvEnvironmentMapAtlasSize", i32 3, i32 16, i32 7, i32 9}
!478 = !{i32 48, !479, !480, !481}
!479 = !{i32 6, !"g_uStride0_uStride1_uOffset0_uOffset1", i32 3, i32 0, i32 7, i32 5}
!480 = !{i32 6, !"g_Tangent_Normal_Texcoord_Color", i32 3, i32 16, i32 7, i32 5}
!481 = !{i32 6, !"g_BIndices_BWeights", i32 3, i32 32, i32 7, i32 5}
!482 = !{i32 40, !483}
!483 = !{i32 6, !"g_cbRaytracingHit", i32 3, i32 0}
!484 = !{i32 24, !485, !486, !487, !488, !489, !490}
!485 = !{i32 6, !"g_fClampReflectionIntensity", i32 3, i32 0, i32 7, i32 9}
!486 = !{i32 6, !"g_fRTReflectionAddRays", i32 3, i32 4, i32 7, i32 9}
!487 = !{i32 6, !"g_uRTReflectionRayCount", i32 3, i32 8, i32 7, i32 5}
!488 = !{i32 6, !"g_fRTZeroRayReflectance", i32 3, i32 12, i32 7, i32 9}
!489 = !{i32 6, !"g_fRTMaxRayReflectance", i32 3, i32 16, i32 7, i32 9}
!490 = !{i32 6, !"g_fRTMaxRayRoughness", i32 3, i32 20, i32 7, i32 9}
!491 = !{i32 1, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !492, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !492, void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !499, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !492, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !492, void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !499, void ()* @"\01?reflectionRayGeneration@@YAXXZ", !500}
!492 = !{!493, !495, !497}
!493 = !{i32 1, !494, !494}
!494 = !{}
!495 = !{i32 2, !496, !494}
!496 = !{i32 4, !"SV_RayPayload"}
!497 = !{i32 0, !498, !494}
!498 = !{i32 4, !"SV_IntersectionAttributes"}
!499 = !{!493, !495}
!500 = !{!493}
!501 = !{null, !"", null, !3, !502}
!502 = !{i32 0, i64 65792, i32 5, !503}
!503 = !{i32 0}
!504 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !505}
!505 = !{i32 8, i32 9, i32 6, i32 4, i32 7, i32 8, i32 5, !503}
!506 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !507}
!507 = !{i32 8, i32 10, i32 6, i32 4, i32 7, i32 8, i32 5, !503}
!508 = !{void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !"\01?reflectionMiss@@YAXUHitData@@@Z", null, null, !509}
!509 = !{i32 8, i32 11, i32 6, i32 4, i32 5, !503}
!510 = !{void ()* @"\01?reflectionRayGeneration@@YAXXZ", !"\01?reflectionRayGeneration@@YAXXZ", null, null, !511}
!511 = !{i32 8, i32 7, i32 5, !503}
!512 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !505}
!513 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !507}
!514 = !{void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !"\01?shadowMiss@@YAXUHitData@@@Z", null, null, !509}
!515 = !{!516, !516, i64 0}
!516 = !{!"omnipotent char", !517, i64 0}
!517 = !{!"Simple C/C++ TBAA"}
!518 = !{!519, !519, i64 0}
!519 = !{!"float", !516, i64 0}
!520 = !{!521, !521, i64 0}
!521 = !{!"int", !516, i64 0}

