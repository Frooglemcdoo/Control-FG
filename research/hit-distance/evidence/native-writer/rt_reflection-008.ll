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
; cbuffer g_cbRaytracingHit
; {
;
;   struct g_cbRaytracingHit
;   {
;
;       struct struct.RaytracingHitRootConstants
;       {
;
;           uint uIndexOffset;                        ; Offset:    0
;           uint uIndexStride;                        ; Offset:    4
;           uint uVertexStride1;                      ; Offset:    8
;           uint uMaterialID;                         ; Offset:   12
;           uint uTangentOffset;                      ; Offset:   16
;           uint uNormalOffset;                       ; Offset:   20
;           uint uTexcoordOffset;                     ; Offset:   24
;           uint uColorOffset;                        ; Offset:   28
;           uint uIndexBufferSize;                    ; Offset:   32
;           uint uVertexBuffer1Size;                  ; Offset:   36
;       
;       } g_cbRaytracingHit;                          ; Offset:    0
;
;   
;   } g_cbRaytracingHit;                              ; Offset:    0 Size:    40
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
; Resource bind info for g_sbMaterialDataPart3
; {
;
;   struct struct.MaterialDataPart3
;   {
;
;       float fClothDiffScatterAmount;                ; Offset:    0
;       float fClothFuzzWrap;                         ; Offset:    4
;       float3 vClothScatterColor;                    ; Offset:    8
;       float fHairSmoothness;                        ; Offset:   20
;       float3 vSpecularColor2;                       ; Offset:   24
;       float fHairSpecularShift;                     ; Offset:   36
;       float fHairDiffuseWrap;                       ; Offset:   40
;       float fPAD1;                                  ; Offset:   44
;       float fEmissionIntensity;                     ; Offset:   48
;       float4 vColorMultiplier;                      ; Offset:   52
;       float2 vSmoothnessRange;                      ; Offset:   68
;       float2 vShadowSoftnessRange;                  ; Offset:   76
;       float2 vScatterWidthRange;                    ; Offset:   84
;       float fHairDepthOffsetAmount;                 ; Offset:   92
;       float fHairNormalAmount;                      ; Offset:   96
;       float3 vEmissionMultiplier;                   ; Offset:  100
;       float4 vColorMultiplier2;                     ; Offset:  112
;       float2 vBaseDetailUVScale;                    ; Offset:  128
;       float2 vSmoothnessRange2;                     ; Offset:  136
;       float2 vBlendDetailUVScale;                   ; Offset:  144
;       float fBaseDetailAmount;                      ; Offset:  152
;       float fBlendUVMultiplier;                     ; Offset:  156
;       float fBlendDetailAmount;                     ; Offset:  160
;       float fFuzzUVMultiplier;                      ; Offset:  164
;       float fPupilDilation;                         ; Offset:  168
;       float fPupilOuterRadius;                      ; Offset:  172
;       float4 vIntensity;                            ; Offset:  176
;       float4 vWrinkleWeights0;                      ; Offset:  192
;       float4 vWrinkleWeights1;                      ; Offset:  208
;       float4 vWrinkleWeights2;                      ; Offset:  224
;       float4 vWrinkleWeights3;                      ; Offset:  240
;       float4 vWrinkleWeights4;                      ; Offset:  256
;       float4 vWrinkleWeights5;                      ; Offset:  272
;       uint uDisableBackfaceFlip;                    ; Offset:  288
;       float fEdgeSharpness;                         ; Offset:  292
;       float fTrunkBendFactor;                       ; Offset:  296
;       float fTrunkPivotOffset;                      ; Offset:  300
;       float3 vColorRgbMultiplier;                   ; Offset:  304
;       float fWindFactor;                            ; Offset:  316
;       float fDecalMaterialBlendFactor;              ; Offset:  320
;       float fDecalAlbedoBlendFactor;                ; Offset:  324
;       uint uStableHash;                             ; Offset:  328
;       uint pad;                                     ; Offset:  332
;   
;   } $Element;                                       ; Offset:    0 Size:   336
;
; }
;
; Resource bind info for g_rwbPerMaterialCount
; {
;
;   uint $Element;                                    ; Offset:    0 Size:     4
;
; }
;
;
; Resource Bindings:
;
; Name                                 Type  Format         Dim      ID      HLSL Bind  Count
; ------------------------------ ---------- ------- ----------- ------- -------------- ------
; sys_constants                     cbuffer      NA          NA     CB0            cb0     1
; g_cbRaytracingHit                 cbuffer      NA          NA     CB1     cb0,space3     1
; rtreflection                      cbuffer      NA          NA     CB2            cb1     1
; g_sLinearWrap                     sampler      NA          NA      S0      s5,space1     1
; g_tGBuffer1                       texture     f32          2d      T0             t0     1
; g_tLinearDepth                    texture     f32          2d      T1             t1     1
; g_tClipDepth                      texture     f32          2d      T2             t2     1
; g_sbMaterialDataPart3             texture  struct         r/o      T3             t3     1
; g_sMaterialTextureArray           texture     f32          2d      T4      t0,space1unbounded
; g_bRaytracingIndexBuffer          texture    byte         r/o      T5      t0,space3     1
; g_bRaytracingVertexBuffer1        texture    byte         r/o      T6      t1,space3     1
; g_tStaticBlueNoiseRGBA_0          texture     f32          2d      T7             t4     1
; g_rtScene                         texture     i32         ras      T8             t5     1
; g_rwtMaterialId                       UAV     u32     2darray      U0             u0     1
; g_rwtNormal_TexcoordX                 UAV     f32     2darray      U1             u1     1
; g_rwtPosition_TexcoordY               UAV     f32     2darray      U2             u2     1
; g_rwbPerMaterialCount                 UAV  struct         r/w      U3             u3     1
; g_rwtShadow                           UAV     u32     2darray      U4             u4     1
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%"class.Texture2D<vector<float, 4> >" = type { <4 x float>, %"class.Texture2D<vector<float, 4> >::mips_type" }
%"class.Texture2D<vector<float, 4> >::mips_type" = type { i32 }
%struct.SamplerState = type { i32 }
%"class.StructuredBuffer<MaterialDataPart3>" = type { %struct.MaterialDataPart3 }
%struct.MaterialDataPart3 = type { float, float, <3 x float>, float, <3 x float>, float, float, float, float, <4 x float>, <2 x float>, <2 x float>, <2 x float>, float, float, <3 x float>, <4 x float>, <2 x float>, <2 x float>, <2 x float>, float, float, float, float, float, float, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, i32, float, float, float, <3 x float>, float, float, float, i32, i32 }
%struct.ByteAddressBuffer = type { i32 }
%struct.RaytracingAccelerationStructure = type { i32 }
%"class.RWTexture2DArray<unsigned int>" = type { i32 }
%"class.RWTexture2DArray<vector<float, 4> >" = type { <4 x float> }
%"class.RWStructuredBuffer<unsigned int>" = type { i32 }
%sys_constants = type { <2 x float>, <2 x float>, <2 x float>, <2 x float>, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, <4 x float>, float, %class.matrix.float.4.4, %class.matrix.float.4.4, %class.matrix.float.4.4, float, float, float, float, float, i32, i32, i32, float, float }
%class.matrix.float.4.4 = type { [4 x <4 x float>] }
%g_cbRaytracingHit = type { %struct.RaytracingHitRootConstants }
%struct.RaytracingHitRootConstants = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, i32 }
%rtreflection = type { float, float, i32, float, float, float }
%struct.HitData = type { i32 }
%struct.IntersectionAttributes = type { <2 x float> }
%dx.types.Handle = type { i8* }
%dx.types.CBufRet.i32 = type { i32, i32, i32, i32 }
%dx.types.ResRet.i32 = type { i32, i32, i32, i32, i32 }
%dx.types.ResRet.f32 = type { float, float, float, float, i32 }
%dx.types.CBufRet.f32 = type { float, float, float, float }
%struct.RayDesc = type { <3 x float>, float, <3 x float>, float }
%struct.GBufferFormat = type { <4 x float>, <4 x float> }
%"class.Texture2D<float>" = type { float, %"class.Texture2D<float>::mips_type" }
%"class.Texture2D<float>::mips_type" = type { i32 }
%"class.Texture2D<unsigned int>" = type { i32, %"class.Texture2D<unsigned int>::mips_type" }
%"class.Texture2D<unsigned int>::mips_type" = type { i32 }
%"class.Texture2DMS<float, 0>" = type { float, %"class.Texture2DMS<float, 0>::sample_type" }
%"class.Texture2DMS<float, 0>::sample_type" = type { i32 }
%"class.StructuredBuffer<vector<unsigned int, 4> >" = type { <4 x i32> }
%"class.StructuredBuffer<MaterialDataPart1>" = type { %struct.MaterialDataPart1 }
%struct.MaterialDataPart1 = type { i32, i32 }
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

@"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_sLinearWrap@@3USamplerState@@A" = external constant %struct.SamplerState, align 4
@"\01?g_sbMaterialDataPart3@@3V?$StructuredBuffer@UMaterialDataPart3@@@@A" = external constant %"class.StructuredBuffer<MaterialDataPart3>", align 4
@"\01?g_sMaterialTextureArray@@3PAV?$Texture2D@V?$vector@M$03@@@@A" = external constant [0 x %"class.Texture2D<vector<float, 4> >"], align 4
@"\01?g_bRaytracingIndexBuffer@@3UByteAddressBuffer@@A" = external constant %struct.ByteAddressBuffer, align 4
@"\01?g_bRaytracingVertexBuffer1@@3UByteAddressBuffer@@A" = external constant %struct.ByteAddressBuffer, align 4
@"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_rtScene@@3URaytracingAccelerationStructure@@A" = external constant %struct.RaytracingAccelerationStructure, align 4
@"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A" = external constant %"class.RWTexture2DArray<unsigned int>", align 4
@"\01?g_rwtNormal_TexcoordX@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A" = external constant %"class.RWTexture2DArray<vector<float, 4> >", align 4
@"\01?g_rwtPosition_TexcoordY@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A" = external constant %"class.RWTexture2DArray<vector<float, 4> >", align 4
@"\01?g_rwbPerMaterialCount@@3V?$RWStructuredBuffer@I@@A" = external constant %"class.RWStructuredBuffer<unsigned int>", align 4
@"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A" = external constant %"class.RWTexture2DArray<unsigned int>", align 4
@sys_constants = external constant %sys_constants
@g_cbRaytracingHit = external constant %g_cbRaytracingHit
@rtreflection = external constant %rtreflection

; Function Attrs: nounwind
define void @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture readonly %ia) #0 {
  %1 = load %struct.SamplerState, %struct.SamplerState* @"\01?g_sLinearWrap@@3USamplerState@@A", align 4
  %2 = load %struct.ByteAddressBuffer, %struct.ByteAddressBuffer* @"\01?g_bRaytracingVertexBuffer1@@3UByteAddressBuffer@@A", align 4
  %3 = load %struct.ByteAddressBuffer, %struct.ByteAddressBuffer* @"\01?g_bRaytracingIndexBuffer@@3UByteAddressBuffer@@A", align 4
  %4 = load %"class.StructuredBuffer<MaterialDataPart3>", %"class.StructuredBuffer<MaterialDataPart3>"* @"\01?g_sbMaterialDataPart3@@3V?$StructuredBuffer@UMaterialDataPart3@@@@A", align 4
  %5 = load %g_cbRaytracingHit, %g_cbRaytracingHit* @g_cbRaytracingHit, align 4
  %g_cbRaytracingHit16 = call %dx.types.Handle @dx.op.createHandleForLib.g_cbRaytracingHit(i32 160, %g_cbRaytracingHit %5)  ; CreateHandleForLib(Resource)
  %6 = getelementptr inbounds %struct.HitData, %struct.HitData* %payload, i32 0, i32 0
  %7 = load i32, i32* %6, align 4
  %PrimitiveIndex = call i32 @dx.op.primitiveIndex.i32(i32 161)  ; PrimitiveIndex()
  %8 = getelementptr inbounds %struct.IntersectionAttributes, %struct.IntersectionAttributes* %ia, i32 0, i32 0
  %9 = load <2 x float>, <2 x float>* %8, align 4, !tbaa !528
  %10 = extractelement <2 x float> %9, i32 0
  %11 = fsub fast float 1.000000e+00, %10
  %12 = extractelement <2 x float> %9, i32 1
  %13 = fsub fast float %11, %12
  %14 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit16, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %15 = extractvalue %dx.types.CBufRet.i32 %14, 3
  %16 = mul i32 %PrimitiveIndex, 3
  %17 = extractvalue %dx.types.CBufRet.i32 %14, 1
  %18 = mul i32 %16, %17
  %19 = extractvalue %dx.types.CBufRet.i32 %14, 0
  %20 = add i32 %18, %19
  %21 = icmp eq i32 %17, 2
  br i1 %21, label %22, label %35

; <label>:22                                      ; preds = %0
  %23 = and i32 %20, -4
  %24 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit16, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %25 = extractvalue %dx.types.CBufRet.i32 %24, 0
  %26 = add i32 %25, -8
  %UMin11 = call i32 @dx.op.binary.i32(i32 40, i32 %23, i32 %26)  ; UMin(a,b)
  %27 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %3)  ; CreateHandleForLib(Resource)
  %RawBufferLoad4 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %27, i32 %UMin11, i32 undef, i8 3, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %28 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad4, 0
  %29 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad4, 1
  %30 = icmp eq i32 %UMin11, %20
  %31 = and i32 %28, 65535
  %32 = lshr i32 %28, 16
  %33 = and i32 %29, 65535
  %34 = lshr i32 %29, 16
  %.i017 = select i1 %30, i32 %31, i32 %32
  %.i118 = select i1 %30, i32 %32, i32 %33
  %.i2 = select i1 %30, i32 %33, i32 %34
  br label %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"

; <label>:35                                      ; preds = %0
  %36 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit16, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %37 = extractvalue %dx.types.CBufRet.i32 %36, 0
  %38 = add i32 %37, -12
  %UMin12 = call i32 @dx.op.binary.i32(i32 40, i32 %20, i32 %38)  ; UMin(a,b)
  %39 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %3)  ; CreateHandleForLib(Resource)
  %RawBufferLoad5 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %39, i32 %UMin12, i32 undef, i8 7, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %40 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad5, 0
  %41 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad5, 1
  %42 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad5, 2
  br label %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"

"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit": ; preds = %35, %22
  %.0.i0 = phi i32 [ %.i017, %22 ], [ %40, %35 ]
  %.0.i1 = phi i32 [ %.i118, %22 ], [ %41, %35 ]
  %.0.i2 = phi i32 [ %.i2, %22 ], [ %42, %35 ]
  %43 = extractvalue %dx.types.CBufRet.i32 %14, 2
  %44 = mul i32 %43, %.0.i2
  %45 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit16, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %46 = extractvalue %dx.types.CBufRet.i32 %45, 2
  %47 = add i32 %44, %46
  %48 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit16, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %49 = extractvalue %dx.types.CBufRet.i32 %48, 1
  %50 = add i32 %49, -4
  %UMin10 = call i32 @dx.op.binary.i32(i32 40, i32 %47, i32 %50)  ; UMin(a,b)
  %51 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %2)  ; CreateHandleForLib(Resource)
  %RawBufferLoad8 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %51, i32 %UMin10, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %52 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad8, 0
  %53 = shl i32 %52, 16
  %54 = ashr exact i32 %53, 16
  %55 = ashr i32 %52, 16
  %.i019 = sitofp i32 %54 to float
  %.i120 = sitofp i32 %55 to float
  %56 = mul i32 %43, %.0.i1
  %57 = add i32 %56, %46
  %UMin9 = call i32 @dx.op.binary.i32(i32 40, i32 %57, i32 %50)  ; UMin(a,b)
  %RawBufferLoad7 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %51, i32 %UMin9, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %58 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad7, 0
  %59 = shl i32 %58, 16
  %60 = ashr exact i32 %59, 16
  %61 = ashr i32 %58, 16
  %.i023 = sitofp i32 %60 to float
  %.i124 = sitofp i32 %61 to float
  %62 = mul i32 %43, %.0.i0
  %63 = add i32 %62, %46
  %UMin = call i32 @dx.op.binary.i32(i32 40, i32 %63, i32 %50)  ; UMin(a,b)
  %RawBufferLoad6 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %51, i32 %UMin, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %64 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad6, 0
  %65 = shl i32 %64, 16
  %66 = ashr exact i32 %65, 16
  %67 = ashr i32 %64, 16
  %.i027 = sitofp i32 %66 to float
  %.i128 = sitofp i32 %67 to float
  %.i031 = fmul fast float %.i027, %13
  %.i132 = fmul fast float %.i128, %13
  %.i033 = fmul fast float %.i023, %10
  %.i134 = fmul fast float %.i124, %10
  %.i037 = fmul fast float %.i019, %12
  %.i138 = fmul fast float %.i120, %12
  %tmp = fadd fast float %.i033, %.i037
  %tmp46 = fadd fast float %tmp, %.i031
  %tmp47 = fmul fast float %tmp46, 0x3F30010020000000
  %tmp48 = fadd fast float %.i134, %.i138
  %tmp49 = fadd fast float %tmp48, %.i132
  %tmp50 = fmul fast float %tmp49, 0x3F30010020000000
  %68 = mul i32 %15, 35
  %69 = getelementptr inbounds [0 x %"class.Texture2D<vector<float, 4> >"], [0 x %"class.Texture2D<vector<float, 4> >"]* @"\01?g_sMaterialTextureArray@@3PAV?$Texture2D@V?$vector@M$03@@@@A", i32 0, i32 %68
  %70 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* %69, align 4, !noalias !531
  %71 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %70)  ; CreateHandleForLib(Resource)
  %72 = call %dx.types.Handle @dx.op.createHandleForLib.struct.SamplerState(i32 160, %struct.SamplerState %1)  ; CreateHandleForLib(Resource)
  %73 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %71, %dx.types.Handle %72, float %tmp47, float %tmp50, float undef, float undef, i32 undef, i32 undef, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %74 = extractvalue %dx.types.ResRet.f32 %73, 3
  %75 = call %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<MaterialDataPart3>"(i32 160, %"class.StructuredBuffer<MaterialDataPart3>" %4)  ; CreateHandleForLib(Resource)
  %RawBufferLoad = call %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32 139, %dx.types.Handle %75, i32 %15, i32 52, i8 15, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %76 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad, 3
  %.i3 = fmul fast float %76, %74
  %77 = fcmp fast olt float %.i3, 5.000000e-01
  br i1 %77, label %78, label %79

; <label>:78                                      ; preds = %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"
  store i32 %7, i32* %6, align 4
  call void @dx.op.ignoreHit(i32 155)  ; IgnoreHit()
  unreachable

; <label>:79                                      ; preds = %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"
  store i32 %7, i32* %6, align 4
  ret void
}

; Function Attrs: nounwind
define void @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture readonly %ia) #0 {
  %1 = load %struct.ByteAddressBuffer, %struct.ByteAddressBuffer* @"\01?g_bRaytracingVertexBuffer1@@3UByteAddressBuffer@@A", align 4
  %2 = load %struct.ByteAddressBuffer, %struct.ByteAddressBuffer* @"\01?g_bRaytracingIndexBuffer@@3UByteAddressBuffer@@A", align 4
  %3 = load %"class.RWStructuredBuffer<unsigned int>", %"class.RWStructuredBuffer<unsigned int>"* @"\01?g_rwbPerMaterialCount@@3V?$RWStructuredBuffer@I@@A", align 4
  %4 = load %"class.RWTexture2DArray<vector<float, 4> >", %"class.RWTexture2DArray<vector<float, 4> >"* @"\01?g_rwtPosition_TexcoordY@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A", align 4
  %5 = load %"class.RWTexture2DArray<vector<float, 4> >", %"class.RWTexture2DArray<vector<float, 4> >"* @"\01?g_rwtNormal_TexcoordX@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A", align 4
  %6 = load %"class.RWTexture2DArray<unsigned int>", %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", align 4
  %7 = load %g_cbRaytracingHit, %g_cbRaytracingHit* @g_cbRaytracingHit, align 4
  %8 = load %sys_constants, %sys_constants* @sys_constants, align 4
  %g_cbRaytracingHit82 = call %dx.types.Handle @dx.op.createHandleForLib.g_cbRaytracingHit(i32 160, %g_cbRaytracingHit %7)  ; CreateHandleForLib(Resource)
  %sys_constants = call %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32 160, %sys_constants %8)  ; CreateHandleForLib(Resource)
  %PrimitiveIndex = call i32 @dx.op.primitiveIndex.i32(i32 161)  ; PrimitiveIndex()
  %9 = getelementptr inbounds %struct.IntersectionAttributes, %struct.IntersectionAttributes* %ia, i32 0, i32 0
  %10 = load <2 x float>, <2 x float>* %9, align 4, !tbaa !528
  %11 = extractelement <2 x float> %10, i32 0
  %12 = fsub fast float 1.000000e+00, %11
  %13 = extractelement <2 x float> %10, i32 1
  %14 = fsub fast float %12, %13
  %WorldRayOrigin = call float @dx.op.worldRayOrigin.f32(i32 147, i8 0)  ; WorldRayOrigin(col)
  %WorldRayOrigin37 = call float @dx.op.worldRayOrigin.f32(i32 147, i8 1)  ; WorldRayOrigin(col)
  %WorldRayOrigin38 = call float @dx.op.worldRayOrigin.f32(i32 147, i8 2)  ; WorldRayOrigin(col)
  %WorldRayDirection = call float @dx.op.worldRayDirection.f32(i32 148, i8 0)  ; WorldRayDirection(col)
  %WorldRayDirection35 = call float @dx.op.worldRayDirection.f32(i32 148, i8 1)  ; WorldRayDirection(col)
  %WorldRayDirection36 = call float @dx.op.worldRayDirection.f32(i32 148, i8 2)  ; WorldRayDirection(col)
  %RayTCurrent = call float @dx.op.rayTCurrent.f32(i32 154)  ; RayTCurrent()
  %WorldToObject64 = call float @dx.op.worldToObject.f32(i32 152, i32 0, i8 0)  ; WorldToObject(row,col)
  %WorldToObject65 = call float @dx.op.worldToObject.f32(i32 152, i32 0, i8 1)  ; WorldToObject(row,col)
  %WorldToObject66 = call float @dx.op.worldToObject.f32(i32 152, i32 0, i8 2)  ; WorldToObject(row,col)
  %WorldToObject56 = call float @dx.op.worldToObject.f32(i32 152, i32 1, i8 0)  ; WorldToObject(row,col)
  %WorldToObject57 = call float @dx.op.worldToObject.f32(i32 152, i32 1, i8 1)  ; WorldToObject(row,col)
  %WorldToObject58 = call float @dx.op.worldToObject.f32(i32 152, i32 1, i8 2)  ; WorldToObject(row,col)
  %WorldToObject48 = call float @dx.op.worldToObject.f32(i32 152, i32 2, i8 0)  ; WorldToObject(row,col)
  %WorldToObject49 = call float @dx.op.worldToObject.f32(i32 152, i32 2, i8 1)  ; WorldToObject(row,col)
  %WorldToObject50 = call float @dx.op.worldToObject.f32(i32 152, i32 2, i8 2)  ; WorldToObject(row,col)
  %15 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit82, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %16 = extractvalue %dx.types.CBufRet.i32 %15, 3
  %17 = mul i32 %PrimitiveIndex, 3
  %18 = extractvalue %dx.types.CBufRet.i32 %15, 1
  %19 = mul i32 %17, %18
  %20 = extractvalue %dx.types.CBufRet.i32 %15, 0
  %21 = add i32 %19, %20
  %22 = icmp eq i32 %18, 2
  br i1 %22, label %23, label %36

; <label>:23                                      ; preds = %0
  %24 = and i32 %21, -4
  %25 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit82, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %26 = extractvalue %dx.types.CBufRet.i32 %25, 0
  %27 = add i32 %26, -8
  %UMin29 = call i32 @dx.op.binary.i32(i32 40, i32 %24, i32 %27)  ; UMin(a,b)
  %28 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %2)  ; CreateHandleForLib(Resource)
  %RawBufferLoad19 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %28, i32 %UMin29, i32 undef, i8 3, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %29 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad19, 0
  %30 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad19, 1
  %31 = icmp eq i32 %UMin29, %21
  %32 = and i32 %29, 65535
  %33 = lshr i32 %29, 16
  %34 = and i32 %30, 65535
  %35 = lshr i32 %30, 16
  %.i083 = select i1 %31, i32 %32, i32 %33
  %.i184 = select i1 %31, i32 %33, i32 %34
  %.i2 = select i1 %31, i32 %34, i32 %35
  br label %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"

; <label>:36                                      ; preds = %0
  %37 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit82, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %38 = extractvalue %dx.types.CBufRet.i32 %37, 0
  %39 = add i32 %38, -12
  %UMin30 = call i32 @dx.op.binary.i32(i32 40, i32 %21, i32 %39)  ; UMin(a,b)
  %40 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %2)  ; CreateHandleForLib(Resource)
  %RawBufferLoad20 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %40, i32 %UMin30, i32 undef, i8 7, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %41 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad20, 0
  %42 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad20, 1
  %43 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad20, 2
  br label %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"

"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit": ; preds = %36, %23
  %.0.i0 = phi i32 [ %.i083, %23 ], [ %41, %36 ]
  %.0.i1 = phi i32 [ %.i184, %23 ], [ %42, %36 ]
  %.0.i2 = phi i32 [ %.i2, %23 ], [ %43, %36 ]
  %44 = extractvalue %dx.types.CBufRet.i32 %15, 2
  %45 = mul i32 %44, %.0.i2
  %46 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit82, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %47 = extractvalue %dx.types.CBufRet.i32 %46, 2
  %48 = add i32 %45, %47
  %49 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit82, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %50 = extractvalue %dx.types.CBufRet.i32 %49, 1
  %51 = add i32 %50, -4
  %UMin28 = call i32 @dx.op.binary.i32(i32 40, i32 %48, i32 %51)  ; UMin(a,b)
  %52 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %1)  ; CreateHandleForLib(Resource)
  %RawBufferLoad23 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %52, i32 %UMin28, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %53 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad23, 0
  %54 = shl i32 %53, 16
  %55 = ashr exact i32 %54, 16
  %56 = ashr i32 %53, 16
  %.i085 = sitofp i32 %55 to float
  %.i186 = sitofp i32 %56 to float
  %57 = mul i32 %44, %.0.i1
  %58 = add i32 %57, %47
  %UMin27 = call i32 @dx.op.binary.i32(i32 40, i32 %58, i32 %51)  ; UMin(a,b)
  %RawBufferLoad22 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %52, i32 %UMin27, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %59 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad22, 0
  %60 = shl i32 %59, 16
  %61 = ashr exact i32 %60, 16
  %62 = ashr i32 %59, 16
  %.i089 = sitofp i32 %61 to float
  %.i190 = sitofp i32 %62 to float
  %63 = mul i32 %44, %.0.i0
  %64 = add i32 %63, %47
  %UMin26 = call i32 @dx.op.binary.i32(i32 40, i32 %64, i32 %51)  ; UMin(a,b)
  %RawBufferLoad21 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %52, i32 %UMin26, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %65 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad21, 0
  %66 = shl i32 %65, 16
  %67 = ashr exact i32 %66, 16
  %68 = ashr i32 %65, 16
  %.i093 = sitofp i32 %67 to float
  %.i194 = sitofp i32 %68 to float
  %.i097 = fmul fast float %.i093, %14
  %.i198 = fmul fast float %.i194, %14
  %.i099 = fmul fast float %.i089, %11
  %.i1100 = fmul fast float %.i190, %11
  %.i0103 = fmul fast float %.i085, %13
  %.i1104 = fmul fast float %.i186, %13
  %tmp = fadd fast float %.i099, %.i0103
  %tmp153 = fadd fast float %tmp, %.i097
  %tmp154 = fmul fast float %tmp153, 0x3F30010020000000
  %tmp155 = fadd fast float %.i1100, %.i1104
  %tmp156 = fadd fast float %tmp155, %.i198
  %tmp157 = fmul fast float %tmp156, 0x3F30010020000000
  %69 = extractvalue %dx.types.CBufRet.i32 %46, 1
  %70 = add i32 %45, %69
  %71 = add i32 %50, -8
  %UMin25 = call i32 @dx.op.binary.i32(i32 40, i32 %70, i32 %71)  ; UMin(a,b)
  %RawBufferLoad18 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %52, i32 %UMin25, i32 undef, i8 3, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %72 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad18, 0
  %73 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad18, 1
  %74 = shl i32 %72, 16
  %75 = ashr exact i32 %74, 16
  %76 = sitofp i32 %75 to float
  %77 = ashr i32 %72, 16
  %78 = sitofp i32 %77 to float
  %79 = shl i32 %73, 16
  %80 = ashr exact i32 %79, 16
  %81 = sitofp i32 %80 to float
  %82 = add i32 %57, %69
  %UMin24 = call i32 @dx.op.binary.i32(i32 40, i32 %82, i32 %71)  ; UMin(a,b)
  %RawBufferLoad17 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %52, i32 %UMin24, i32 undef, i8 3, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %83 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad17, 0
  %84 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad17, 1
  %85 = shl i32 %83, 16
  %86 = ashr exact i32 %85, 16
  %87 = sitofp i32 %86 to float
  %88 = ashr i32 %83, 16
  %89 = sitofp i32 %88 to float
  %90 = shl i32 %84, 16
  %91 = ashr exact i32 %90, 16
  %92 = sitofp i32 %91 to float
  %93 = add i32 %63, %69
  %UMin = call i32 @dx.op.binary.i32(i32 40, i32 %93, i32 %71)  ; UMin(a,b)
  %RawBufferLoad = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %52, i32 %UMin, i32 undef, i8 3, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %94 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad, 0
  %95 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad, 1
  %96 = shl i32 %94, 16
  %97 = ashr exact i32 %96, 16
  %98 = sitofp i32 %97 to float
  %99 = ashr i32 %94, 16
  %100 = sitofp i32 %99 to float
  %101 = shl i32 %95, 16
  %102 = ashr exact i32 %101, 16
  %103 = sitofp i32 %102 to float
  %.i0107 = fmul fast float %98, %14
  %.i1108 = fmul fast float %100, %14
  %.i2109 = fmul fast float %103, %14
  %.i0110 = fmul fast float %87, %11
  %.i1111 = fmul fast float %89, %11
  %.i2112 = fmul fast float %92, %11
  %.i0118 = fmul fast float %76, %13
  %.i1119 = fmul fast float %78, %13
  %.i2120 = fmul fast float %81, %13
  %tmp158 = fadd fast float %.i0110, %.i0118
  %tmp159 = fadd fast float %tmp158, %.i0107
  %tmp160 = fmul fast float %tmp159, 0x3F00002000000000
  %tmp161 = fadd fast float %.i1111, %.i1119
  %tmp162 = fadd fast float %tmp161, %.i1108
  %tmp163 = fmul fast float %tmp162, 0x3F00002000000000
  %tmp164 = fadd fast float %.i2112, %.i2120
  %tmp165 = fadd fast float %tmp164, %.i2109
  %tmp166 = fmul fast float %tmp165, 0x3F00002000000000
  %.i0126 = fmul fast float %RayTCurrent, %WorldRayDirection
  %.i1127 = fmul fast float %RayTCurrent, %WorldRayDirection35
  %.i2128 = fmul fast float %RayTCurrent, %WorldRayDirection36
  %.i0129 = fadd fast float %.i0126, %WorldRayOrigin
  %.i1130 = fadd fast float %.i1127, %WorldRayOrigin37
  %.i2131 = fadd fast float %.i2128, %WorldRayOrigin38
  %104 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %105 = extractvalue %dx.types.CBufRet.f32 %104, 0
  %106 = extractvalue %dx.types.CBufRet.f32 %104, 1
  %107 = extractvalue %dx.types.CBufRet.f32 %104, 2
  %108 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 3)  ; CBufferLoadLegacy(handle,regIndex)
  %109 = extractvalue %dx.types.CBufRet.f32 %108, 0
  %110 = extractvalue %dx.types.CBufRet.f32 %108, 1
  %111 = extractvalue %dx.types.CBufRet.f32 %108, 2
  %112 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 4)  ; CBufferLoadLegacy(handle,regIndex)
  %113 = extractvalue %dx.types.CBufRet.f32 %112, 0
  %114 = extractvalue %dx.types.CBufRet.f32 %112, 1
  %115 = extractvalue %dx.types.CBufRet.f32 %112, 2
  %116 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 5)  ; CBufferLoadLegacy(handle,regIndex)
  %117 = extractvalue %dx.types.CBufRet.f32 %116, 0
  %118 = extractvalue %dx.types.CBufRet.f32 %116, 1
  %119 = extractvalue %dx.types.CBufRet.f32 %116, 2
  %120 = fmul fast float %105, %.i0129
  %FMad16 = call float @dx.op.tertiary.f32(i32 46, float %.i1130, float %109, float %120)  ; FMad(a,b,c)
  %FMad15 = call float @dx.op.tertiary.f32(i32 46, float %.i2131, float %113, float %FMad16)  ; FMad(a,b,c)
  %121 = fadd fast float %FMad15, %117
  %122 = fmul fast float %106, %.i0129
  %FMad13 = call float @dx.op.tertiary.f32(i32 46, float %.i1130, float %110, float %122)  ; FMad(a,b,c)
  %FMad12 = call float @dx.op.tertiary.f32(i32 46, float %.i2131, float %114, float %FMad13)  ; FMad(a,b,c)
  %123 = fadd fast float %FMad12, %118
  %124 = fmul fast float %107, %.i0129
  %FMad10 = call float @dx.op.tertiary.f32(i32 46, float %.i1130, float %111, float %124)  ; FMad(a,b,c)
  %FMad9 = call float @dx.op.tertiary.f32(i32 46, float %.i2131, float %115, float %FMad10)  ; FMad(a,b,c)
  %125 = fadd fast float %FMad9, %119
  %.i0132 = fmul fast float %tmp160, %WorldToObject64
  %.i1133 = fmul fast float %tmp160, %WorldToObject65
  %.i2134 = fmul fast float %tmp160, %WorldToObject66
  %.i0135 = fmul fast float %tmp163, %WorldToObject56
  %.i1136 = fmul fast float %tmp163, %WorldToObject57
  %.i2137 = fmul fast float %tmp163, %WorldToObject58
  %.i0138 = fadd fast float %.i0132, %.i0135
  %.i1139 = fadd fast float %.i1133, %.i1136
  %.i2140 = fadd fast float %.i2134, %.i2137
  %.i0141 = fmul fast float %tmp166, %WorldToObject48
  %.i1142 = fmul fast float %tmp166, %WorldToObject49
  %.i2143 = fmul fast float %tmp166, %WorldToObject50
  %.i0144 = fadd fast float %.i0138, %.i0141
  %.i1145 = fadd fast float %.i1139, %.i1142
  %.i2146 = fadd fast float %.i2140, %.i2143
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex39 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %126 = getelementptr inbounds %struct.HitData, %struct.HitData* %payload, i32 0, i32 0
  %127 = load i32, i32* %126, align 4, !tbaa !536
  %128 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32 160, %"class.RWTexture2DArray<unsigned int>" %6)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %128, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex39, i32 %127, i32 %16, i32 %16, i32 %16, i32 %16, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %129 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<vector<float, 4> >"(i32 160, %"class.RWTexture2DArray<vector<float, 4> >" %5)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.f32(i32 67, %dx.types.Handle %129, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex39, i32 %127, float %.i0144, float %.i1145, float %.i2146, float %tmp154, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %130 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<vector<float, 4> >"(i32 160, %"class.RWTexture2DArray<vector<float, 4> >" %4)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.f32(i32 67, %dx.types.Handle %130, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex39, i32 %127, float %121, float %123, float %125, float %tmp157, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %131 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWStructuredBuffer<unsigned int>"(i32 160, %"class.RWStructuredBuffer<unsigned int>" %3)  ; CreateHandleForLib(Resource)
  %AtomicAdd = call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle %131, i32 0, i32 %16, i32 0, i32 undef, i32 1)  ; AtomicBinOp(handle,atomicOp,offset0,offset1,offset2,newValue)
  %132 = fmul fast float %RayTCurrent, 1.000000e+04
  %133 = fptoui float %132 to i32
  store i32 %133, i32* %126, align 4, !tbaa !536
  ret void
}

; Function Attrs: nounwind
define void @"\01?reflectionMiss@@YAXUHitData@@@Z"(%struct.HitData* noalias nocapture %payload) #0 {
  %1 = load %"class.RWTexture2DArray<vector<float, 4> >", %"class.RWTexture2DArray<vector<float, 4> >"* @"\01?g_rwtPosition_TexcoordY@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A", align 4
  %2 = load %"class.RWTexture2DArray<unsigned int>", %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", align 4
  %3 = load %sys_constants, %sys_constants* @sys_constants, align 4
  %sys_constants = call %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32 160, %sys_constants %3)  ; CreateHandleForLib(Resource)
  %WorldRayOrigin = call float @dx.op.worldRayOrigin.f32(i32 147, i8 0)  ; WorldRayOrigin(col)
  %WorldRayOrigin14 = call float @dx.op.worldRayOrigin.f32(i32 147, i8 1)  ; WorldRayOrigin(col)
  %WorldRayOrigin15 = call float @dx.op.worldRayOrigin.f32(i32 147, i8 2)  ; WorldRayOrigin(col)
  %WorldRayDirection = call float @dx.op.worldRayDirection.f32(i32 148, i8 0)  ; WorldRayDirection(col)
  %WorldRayDirection12 = call float @dx.op.worldRayDirection.f32(i32 148, i8 1)  ; WorldRayDirection(col)
  %WorldRayDirection13 = call float @dx.op.worldRayDirection.f32(i32 148, i8 2)  ; WorldRayDirection(col)
  %RayTCurrent = call float @dx.op.rayTCurrent.f32(i32 154)  ; RayTCurrent()
  %.i0 = fmul fast float %RayTCurrent, %WorldRayDirection
  %.i1 = fmul fast float %RayTCurrent, %WorldRayDirection12
  %.i2 = fmul fast float %RayTCurrent, %WorldRayDirection13
  %.i021 = fadd fast float %.i0, %WorldRayOrigin
  %.i122 = fadd fast float %.i1, %WorldRayOrigin14
  %.i223 = fadd fast float %.i2, %WorldRayOrigin15
  %4 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %5 = extractvalue %dx.types.CBufRet.f32 %4, 0
  %6 = extractvalue %dx.types.CBufRet.f32 %4, 1
  %7 = extractvalue %dx.types.CBufRet.f32 %4, 2
  %8 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 3)  ; CBufferLoadLegacy(handle,regIndex)
  %9 = extractvalue %dx.types.CBufRet.f32 %8, 0
  %10 = extractvalue %dx.types.CBufRet.f32 %8, 1
  %11 = extractvalue %dx.types.CBufRet.f32 %8, 2
  %12 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 4)  ; CBufferLoadLegacy(handle,regIndex)
  %13 = extractvalue %dx.types.CBufRet.f32 %12, 0
  %14 = extractvalue %dx.types.CBufRet.f32 %12, 1
  %15 = extractvalue %dx.types.CBufRet.f32 %12, 2
  %16 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 5)  ; CBufferLoadLegacy(handle,regIndex)
  %17 = extractvalue %dx.types.CBufRet.f32 %16, 0
  %18 = extractvalue %dx.types.CBufRet.f32 %16, 1
  %19 = extractvalue %dx.types.CBufRet.f32 %16, 2
  %20 = fmul fast float %.i021, %5
  %FMad11 = call float @dx.op.tertiary.f32(i32 46, float %.i122, float %9, float %20)  ; FMad(a,b,c)
  %FMad10 = call float @dx.op.tertiary.f32(i32 46, float %.i223, float %13, float %FMad11)  ; FMad(a,b,c)
  %21 = fadd fast float %FMad10, %17
  %22 = fmul fast float %.i021, %6
  %FMad8 = call float @dx.op.tertiary.f32(i32 46, float %.i122, float %10, float %22)  ; FMad(a,b,c)
  %FMad7 = call float @dx.op.tertiary.f32(i32 46, float %.i223, float %14, float %FMad8)  ; FMad(a,b,c)
  %23 = fadd fast float %FMad7, %18
  %24 = fmul fast float %.i021, %7
  %FMad5 = call float @dx.op.tertiary.f32(i32 46, float %.i122, float %11, float %24)  ; FMad(a,b,c)
  %FMad4 = call float @dx.op.tertiary.f32(i32 46, float %.i223, float %15, float %FMad5)  ; FMad(a,b,c)
  %25 = fadd fast float %FMad4, %19
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex16 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %26 = getelementptr inbounds %struct.HitData, %struct.HitData* %payload, i32 0, i32 0
  %27 = load i32, i32* %26, align 4, !tbaa !536
  %28 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32 160, %"class.RWTexture2DArray<unsigned int>" %2)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %28, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex16, i32 %27, i32 65535, i32 65535, i32 65535, i32 65535, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %29 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<vector<float, 4> >"(i32 160, %"class.RWTexture2DArray<vector<float, 4> >" %1)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.f32(i32 67, %dx.types.Handle %29, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex16, i32 %27, float %21, float %23, float %25, float 0.000000e+00, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  store i32 0, i32* %26, align 4, !tbaa !536
  ret void
}

; Function Attrs: nounwind
define void @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture readnone %ia) #0 {
  %1 = load %"class.RWTexture2DArray<unsigned int>", %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", align 4
  %2 = getelementptr inbounds %struct.HitData, %struct.HitData* %payload, i32 0, i32 0
  %3 = load i32, i32* %2, align 4
  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)  ; DispatchRaysIndex(col)
  %DispatchRaysIndex1 = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)  ; DispatchRaysIndex(col)
  %4 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32 160, %"class.RWTexture2DArray<unsigned int>" %1)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %4, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex1, i32 %3, i32 0, i32 0, i32 0, i32 0, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  ret void
}

; Function Attrs: nounwind
define void @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z"(%struct.HitData* noalias nocapture %payload, %struct.IntersectionAttributes* nocapture readonly %ia) #0 {
  %1 = load %struct.SamplerState, %struct.SamplerState* @"\01?g_sLinearWrap@@3USamplerState@@A", align 4
  %2 = load %struct.ByteAddressBuffer, %struct.ByteAddressBuffer* @"\01?g_bRaytracingVertexBuffer1@@3UByteAddressBuffer@@A", align 4
  %3 = load %struct.ByteAddressBuffer, %struct.ByteAddressBuffer* @"\01?g_bRaytracingIndexBuffer@@3UByteAddressBuffer@@A", align 4
  %4 = load %"class.StructuredBuffer<MaterialDataPart3>", %"class.StructuredBuffer<MaterialDataPart3>"* @"\01?g_sbMaterialDataPart3@@3V?$StructuredBuffer@UMaterialDataPart3@@@@A", align 4
  %5 = load %g_cbRaytracingHit, %g_cbRaytracingHit* @g_cbRaytracingHit, align 4
  %g_cbRaytracingHit67 = call %dx.types.Handle @dx.op.createHandleForLib.g_cbRaytracingHit(i32 160, %g_cbRaytracingHit %5)  ; CreateHandleForLib(Resource)
  %6 = getelementptr inbounds %struct.HitData, %struct.HitData* %payload, i32 0, i32 0
  %7 = load i32, i32* %6, align 4
  %PrimitiveIndex = call i32 @dx.op.primitiveIndex.i32(i32 161)  ; PrimitiveIndex()
  %8 = getelementptr inbounds %struct.IntersectionAttributes, %struct.IntersectionAttributes* %ia, i32 0, i32 0
  %9 = load <2 x float>, <2 x float>* %8, align 4, !tbaa !528
  %10 = extractelement <2 x float> %9, i32 0
  %11 = fsub fast float 1.000000e+00, %10
  %12 = extractelement <2 x float> %9, i32 1
  %13 = fsub fast float %11, %12
  %14 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit67, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %15 = extractvalue %dx.types.CBufRet.i32 %14, 3
  %16 = mul i32 %PrimitiveIndex, 3
  %17 = extractvalue %dx.types.CBufRet.i32 %14, 1
  %18 = mul i32 %16, %17
  %19 = extractvalue %dx.types.CBufRet.i32 %14, 0
  %20 = add i32 %18, %19
  %21 = icmp eq i32 %17, 2
  br i1 %21, label %22, label %35

; <label>:22                                      ; preds = %0
  %23 = and i32 %20, -4
  %24 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit67, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %25 = extractvalue %dx.types.CBufRet.i32 %24, 0
  %26 = add i32 %25, -8
  %UMin62 = call i32 @dx.op.binary.i32(i32 40, i32 %23, i32 %26)  ; UMin(a,b)
  %27 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %3)  ; CreateHandleForLib(Resource)
  %RawBufferLoad55 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %27, i32 %UMin62, i32 undef, i8 3, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %28 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad55, 0
  %29 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad55, 1
  %30 = icmp eq i32 %UMin62, %20
  %31 = and i32 %28, 65535
  %32 = lshr i32 %28, 16
  %33 = and i32 %29, 65535
  %34 = lshr i32 %29, 16
  %.i068 = select i1 %30, i32 %31, i32 %32
  %.i169 = select i1 %30, i32 %32, i32 %33
  %.i2 = select i1 %30, i32 %33, i32 %34
  br label %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"

; <label>:35                                      ; preds = %0
  %36 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit67, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %37 = extractvalue %dx.types.CBufRet.i32 %36, 0
  %38 = add i32 %37, -12
  %UMin63 = call i32 @dx.op.binary.i32(i32 40, i32 %20, i32 %38)  ; UMin(a,b)
  %39 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %3)  ; CreateHandleForLib(Resource)
  %RawBufferLoad56 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %39, i32 %UMin63, i32 undef, i8 7, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %40 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad56, 0
  %41 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad56, 1
  %42 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad56, 2
  br label %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"

"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit": ; preds = %35, %22
  %.053.i0 = phi i32 [ %.i068, %22 ], [ %40, %35 ]
  %.053.i1 = phi i32 [ %.i169, %22 ], [ %41, %35 ]
  %.053.i2 = phi i32 [ %.i2, %22 ], [ %42, %35 ]
  %43 = extractvalue %dx.types.CBufRet.i32 %14, 2
  %44 = mul i32 %43, %.053.i2
  %45 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit67, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %46 = extractvalue %dx.types.CBufRet.i32 %45, 2
  %47 = add i32 %44, %46
  %48 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %g_cbRaytracingHit67, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %49 = extractvalue %dx.types.CBufRet.i32 %48, 1
  %50 = add i32 %49, -4
  %UMin61 = call i32 @dx.op.binary.i32(i32 40, i32 %47, i32 %50)  ; UMin(a,b)
  %51 = call %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32 160, %struct.ByteAddressBuffer %2)  ; CreateHandleForLib(Resource)
  %RawBufferLoad59 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %51, i32 %UMin61, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %52 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad59, 0
  %53 = shl i32 %52, 16
  %54 = ashr exact i32 %53, 16
  %55 = ashr i32 %52, 16
  %.i070 = sitofp i32 %54 to float
  %.i171 = sitofp i32 %55 to float
  %56 = mul i32 %43, %.053.i1
  %57 = add i32 %56, %46
  %UMin60 = call i32 @dx.op.binary.i32(i32 40, i32 %57, i32 %50)  ; UMin(a,b)
  %RawBufferLoad58 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %51, i32 %UMin60, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %58 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad58, 0
  %59 = shl i32 %58, 16
  %60 = ashr exact i32 %59, 16
  %61 = ashr i32 %58, 16
  %.i074 = sitofp i32 %60 to float
  %.i175 = sitofp i32 %61 to float
  %62 = mul i32 %43, %.053.i0
  %63 = add i32 %62, %46
  %UMin = call i32 @dx.op.binary.i32(i32 40, i32 %63, i32 %50)  ; UMin(a,b)
  %RawBufferLoad57 = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32 139, %dx.types.Handle %51, i32 %UMin, i32 undef, i8 1, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %64 = extractvalue %dx.types.ResRet.i32 %RawBufferLoad57, 0
  %65 = shl i32 %64, 16
  %66 = ashr exact i32 %65, 16
  %67 = ashr i32 %64, 16
  %.i078 = sitofp i32 %66 to float
  %.i179 = sitofp i32 %67 to float
  %.i082 = fmul fast float %.i078, %13
  %.i183 = fmul fast float %.i179, %13
  %.i084 = fmul fast float %.i074, %10
  %.i185 = fmul fast float %.i175, %10
  %.i088 = fmul fast float %.i070, %12
  %.i189 = fmul fast float %.i171, %12
  %tmp = fadd fast float %.i084, %.i088
  %tmp101 = fadd fast float %tmp, %.i082
  %tmp102 = fmul fast float %tmp101, 0x3F30010020000000
  %tmp103 = fadd fast float %.i185, %.i189
  %tmp104 = fadd fast float %tmp103, %.i183
  %tmp105 = fmul fast float %tmp104, 0x3F30010020000000
  %68 = mul i32 %15, 35
  %69 = getelementptr inbounds [0 x %"class.Texture2D<vector<float, 4> >"], [0 x %"class.Texture2D<vector<float, 4> >"]* @"\01?g_sMaterialTextureArray@@3PAV?$Texture2D@V?$vector@M$03@@@@A", i32 0, i32 %68
  %70 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* %69, align 4, !noalias !538
  %71 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %70)  ; CreateHandleForLib(Resource)
  %72 = call %dx.types.Handle @dx.op.createHandleForLib.struct.SamplerState(i32 160, %struct.SamplerState %1)  ; CreateHandleForLib(Resource)
  %73 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %71, %dx.types.Handle %72, float %tmp102, float %tmp105, float undef, float undef, i32 undef, i32 undef, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %74 = extractvalue %dx.types.ResRet.f32 %73, 3
  %75 = call %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<MaterialDataPart3>"(i32 160, %"class.StructuredBuffer<MaterialDataPart3>" %4)  ; CreateHandleForLib(Resource)
  %RawBufferLoad = call %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32 139, %dx.types.Handle %75, i32 %15, i32 52, i8 15, i32 4)  ; RawBufferLoad(srv,index,elementOffset,mask,alignment)
  %76 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad, 3
  %.i3 = fmul fast float %76, %74
  %.i398 = fcmp fast ogt float %.i3, 0.000000e+00
  %77 = select i1 %.i398, float %76, float 1.000000e+00
  %78 = fcmp fast olt float %77, 5.000000e-01
  br i1 %78, label %79, label %80

; <label>:79                                      ; preds = %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"
  store i32 %7, i32* %6, align 4
  call void @dx.op.ignoreHit(i32 155)  ; IgnoreHit()
  unreachable

; <label>:80                                      ; preds = %"\01?internal.getRaytracingVertexIndices@@YA?AV?$vector@I$02@@I@Z.exit"
  store i32 %7, i32* %6, align 4
  ret void
}

; Function Attrs: nounwind
define void @"\01?shadowMiss@@YAXUHitData@@@Z"(%struct.HitData* noalias nocapture %payload) #0 {
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
define void @"\01?reflectionRayGeneration@@YAXXZ"() #0 {
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
  %FMad132 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %60, float %74)  ; FMad(a,b,c)
  %FMad131 = call float @dx.op.tertiary.f32(i32 46, float %17, float %65, float %FMad132)  ; FMad(a,b,c)
  %75 = fadd fast float %FMad131, %70
  %76 = fmul fast float %56, %.i0156
  %FMad129 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %61, float %76)  ; FMad(a,b,c)
  %FMad128 = call float @dx.op.tertiary.f32(i32 46, float %17, float %66, float %FMad129)  ; FMad(a,b,c)
  %77 = fadd fast float %FMad128, %71
  %78 = fmul fast float %57, %.i0156
  %FMad126 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %62, float %78)  ; FMad(a,b,c)
  %FMad125 = call float @dx.op.tertiary.f32(i32 46, float %17, float %67, float %FMad126)  ; FMad(a,b,c)
  %79 = fadd fast float %FMad125, %72
  %80 = fmul fast float %58, %.i0156
  %FMad123 = call float @dx.op.tertiary.f32(i32 46, float %.i1157433, float %63, float %80)  ; FMad(a,b,c)
  %FMad122 = call float @dx.op.tertiary.f32(i32 46, float %17, float %68, float %FMad123)  ; FMad(a,b,c)
  %81 = fadd fast float %FMad122, %73
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
  %95 = extractvalue %dx.types.ResRet.f32 %TextureLoad146, 2
  %96 = extractvalue %dx.types.ResRet.f32 %TextureLoad146, 3
  %97 = fmul fast float %96, 2.550000e+02
  %98 = fadd fast float %97, 5.000000e-01
  %99 = fptoui float %98 to i32
  %100 = and i32 %99, 254
  %101 = uitofp i32 %100 to float
  %.i0176 = fmul fast float %93, 2.550000e+02
  %.i1177 = fmul fast float %94, 2.550000e+02
  %.i0179 = fadd fast float %.i0176, 5.000000e-01
  %.i1180 = fadd fast float %.i1177, 5.000000e-01
  %.i2181 = fadd fast float %101, 5.000000e-01
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
  %102 = sitofp i32 %.i0191 to float
  %103 = sitofp i32 %.i1192 to float
  %104 = fmul fast float %102, 0x3F54CF66A0000000
  %.i0199 = fadd fast float %104, 0xBFF4CCCCC0000000
  %105 = fmul fast float %103, 0x3F44CE19C0000000
  %.i1200 = fadd fast float %105, 0xBFF4CCCCC0000000
  %106 = fmul fast float %.i0199, %.i0199
  %107 = fmul fast float %.i1200, %.i1200
  %108 = fadd fast float %106, 1.000000e+00
  %109 = fadd fast float %108, %107
  %110 = fmul fast float %102, 0x3F64CF66A0000000
  %.i0201 = fadd fast float %110, 0xC004CCCCC0000000
  %111 = fmul fast float %103, 0x3F54CE19C0000000
  %.i1202 = fadd fast float %111, 0xC004CCCCC0000000
  %112 = fadd fast float %107, -1.000000e+00
  %113 = fadd fast float %112, %106
  %.i0203 = fdiv fast float %.i0201, %109
  %.i1204 = fdiv fast float %.i1202, %109
  %.i2205 = fdiv fast float %113, %109
  %114 = fmul fast float %.i0203, %.i0203
  %115 = fmul fast float %.i1204, %.i1204
  %116 = fadd fast float %115, %114
  %117 = fmul fast float %.i2205, %.i2205
  %118 = fadd fast float %116, %117
  %Sqrt68 = call float @dx.op.unary.f32(i32 24, float %118)  ; Sqrt(value)
  %.i0206 = fdiv fast float %.i0203, %Sqrt68
  %.i1207 = fdiv fast float %.i1204, %Sqrt68
  %.i2208 = fdiv fast float %.i2205, %Sqrt68
  %119 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %120 = extractvalue %dx.types.CBufRet.f32 %119, 0
  %121 = extractvalue %dx.types.CBufRet.f32 %119, 1
  %122 = extractvalue %dx.types.CBufRet.f32 %119, 2
  %123 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %124 = extractvalue %dx.types.CBufRet.f32 %123, 0
  %125 = extractvalue %dx.types.CBufRet.f32 %123, 1
  %126 = extractvalue %dx.types.CBufRet.f32 %123, 2
  %127 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %128 = extractvalue %dx.types.CBufRet.f32 %127, 0
  %129 = extractvalue %dx.types.CBufRet.f32 %127, 1
  %130 = extractvalue %dx.types.CBufRet.f32 %127, 2
  %131 = fmul fast float %120, %.i0206
  %FMad120 = call float @dx.op.tertiary.f32(i32 46, float %.i1207, float %124, float %131)  ; FMad(a,b,c)
  %FMad119 = call float @dx.op.tertiary.f32(i32 46, float %.i2208, float %128, float %FMad120)  ; FMad(a,b,c)
  %132 = fmul fast float %121, %.i0206
  %FMad118 = call float @dx.op.tertiary.f32(i32 46, float %.i1207, float %125, float %132)  ; FMad(a,b,c)
  %FMad117 = call float @dx.op.tertiary.f32(i32 46, float %.i2208, float %129, float %FMad118)  ; FMad(a,b,c)
  %133 = fmul fast float %122, %.i0206
  %FMad116 = call float @dx.op.tertiary.f32(i32 46, float %.i1207, float %126, float %133)  ; FMad(a,b,c)
  %FMad115 = call float @dx.op.tertiary.f32(i32 46, float %.i2208, float %130, float %FMad116)  ; FMad(a,b,c)
  %134 = fsub fast float 1.000000e+00, %95
  %.i0209 = fmul fast float %19, 0x3F50624DE0000000
  %.i0212 = fmul fast float %.i0209, %.i0173
  %.i1213 = fmul fast float %.i0209, %.i1174
  %.i2214 = fmul fast float %.i0209, %.i2175
  %.i0215 = fsub fast float %.i0167, %.i0212
  %.i1216 = fsub fast float %.i1168, %.i1213
  %.i2217 = fsub fast float %.i2169, %.i2214
  %.i0221 = fmul fast float %.i0209, %FMad119
  %.i1222 = fmul fast float %.i0209, %FMad117
  %.i2223 = fmul fast float %.i0209, %FMad115
  %.i0224 = fadd fast float %.i0215, %.i0221
  %.i1225 = fadd fast float %.i1216, %.i1222
  %.i2226 = fadd fast float %.i2217, %.i2223
  %.upto0377 = insertelement <3 x float> undef, float %.i0224, i32 0
  %.upto1378 = insertelement <3 x float> %.upto0377, float %.i1225, i32 1
  %135 = insertelement <3 x float> %.upto1378, float %.i2226, i32 2
  %136 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 0
  store <3 x float> %135, <3 x float>* %136, align 4, !tbaa !528
  %137 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 1
  store float 0.000000e+00, float* %137, align 4, !tbaa !543
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
  store float %.i0167, float* %168, align 4
  %169 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 1
  store float %.i1168, float* %169, align 4
  %170 = getelementptr inbounds [3 x float], [3 x float]* %0, i32 0, i32 2
  store float %.i2169, float* %170, align 4
  %.i0231 = fmul fast float %FMad119, 4.000000e+00
  %.i1232 = fmul fast float %FMad117, 4.000000e+00
  %.i2233 = fmul fast float %FMad115, 4.000000e+00
  %Round_ne = call float @dx.op.unary.f32(i32 26, float %.i0231)  ; Round_ne(value)
  %Round_ne49 = call float @dx.op.unary.f32(i32 26, float %.i1232)  ; Round_ne(value)
  %Round_ne50 = call float @dx.op.unary.f32(i32 26, float %.i2233)  ; Round_ne(value)
  %171 = fmul fast float %Round_ne, %Round_ne
  %172 = fmul fast float %Round_ne49, %Round_ne49
  %173 = fadd fast float %172, %171
  %174 = fmul fast float %Round_ne50, %Round_ne50
  %175 = fadd fast float %173, %174
  %Sqrt48 = call float @dx.op.unary.f32(i32 24, float %175)  ; Sqrt(value)
  %.i0234 = fdiv fast float %Round_ne, %Sqrt48
  %.i1235 = fdiv fast float %Round_ne49, %Sqrt48
  %.i2236 = fdiv fast float %Round_ne50, %Sqrt48
  %FAbs57 = call float @dx.op.unary.f32(i32 6, float %.i0234)  ; FAbs(value)
  %FAbs58 = call float @dx.op.unary.f32(i32 6, float %.i1235)  ; FAbs(value)
  %FAbs59 = call float @dx.op.unary.f32(i32 6, float %.i2236)  ; FAbs(value)
  %FMax73 = call float @dx.op.binary.f32(i32 35, float %FAbs57, float %FAbs58)  ; FMax(a,b)
  %FMax72 = call float @dx.op.binary.f32(i32 35, float %FMax73, float %FAbs59)  ; FMax(a,b)
  %176 = fcmp fast oeq float %FMax72, %FAbs58
  %iMajorAxis.i.0 = zext i1 %176 to i32
  %177 = fcmp fast oeq float %FMax72, %FAbs59
  %iMajorAxis.i.1 = select i1 %177, i32 2, i32 %iMajorAxis.i.0
  %FMad110 = call float @dx.op.tertiary.f32(i32 46, float %17, float %144, float 0.000000e+00)  ; FMad(a,b,c)
  %178 = fadd fast float %FMad110, %147
  %FMad104 = call float @dx.op.tertiary.f32(i32 46, float %17, float %145, float 0.000000e+00)  ; FMad(a,b,c)
  %179 = fadd fast float %FMad104, %148
  %180 = fdiv fast float 1.000000e+00, %179
  %.i1238 = fmul fast float %180, %178
  %181 = fdiv fast float 2.000000e+00, %167
  %FMad141 = call float @dx.op.tertiary.f32(i32 46, float %181, float %141, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad140 = call float @dx.op.tertiary.f32(i32 46, float %17, float %144, float %FMad141)  ; FMad(a,b,c)
  %182 = fadd fast float %FMad140, %147
  %FMad135 = call float @dx.op.tertiary.f32(i32 46, float %181, float %142, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad134 = call float @dx.op.tertiary.f32(i32 46, float %17, float %145, float %FMad135)  ; FMad(a,b,c)
  %183 = fadd fast float %FMad134, %148
  %184 = fdiv fast float 1.000000e+00, %183
  %.i1241 = fmul fast float %184, %182
  %185 = fsub fast float %.i1241, %.i1238
  %FAbs = call float @dx.op.unary.f32(i32 6, float %185)  ; FAbs(value)
  %186 = fmul fast float %FAbs, 6.400000e+01
  %Log = call float @dx.op.unary.f32(i32 23, float %186)  ; Log(value)
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
  %187 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 0
  store i32 %.i0246, i32* %187, align 4
  %188 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 1
  store i32 %.i1247, i32* %188, align 4
  %189 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i32 0, i32 2
  store i32 %.i2248, i32* %189, align 4
  %190 = getelementptr [3 x i32], [3 x i32]* %1, i32 0, i32 %iMajorAxis.i.1
  store i32 0, i32* %190, align 4, !tbaa !536
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
  %218 = load float, float* %168, align 4, !tbaa !543
  %219 = fsub fast float %218, %.i0252
  %220 = fcmp fast ogt float %219, %217
  %221 = fadd fast float %.i0249, 5.000000e-01
  %222 = fmul fast float %Exp, %221
  %223 = select i1 %220, float %222, float %.i0252
  %224 = load float, float* %169, align 4, !tbaa !543
  %225 = fsub fast float %224, %.i1253
  %226 = fcmp fast ogt float %225, %217
  %227 = fadd fast float %.i1250, 5.000000e-01
  %228 = fmul fast float %Exp, %227
  %229 = select i1 %226, float %228, float %.i1253
  %230 = load float, float* %170, align 4, !tbaa !543
  %231 = fsub fast float %230, %.i2254
  %232 = fcmp fast ogt float %231, %217
  %233 = fadd fast float %.i2251, 5.000000e-01
  %234 = fmul fast float %Exp, %233
  %235 = select i1 %232, float %234, float %.i2254
  %236 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 0
  store float %223, float* %236, align 4
  %237 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 1
  store float %229, float* %237, align 4
  %238 = getelementptr inbounds [3 x float], [3 x float]* %2, i32 0, i32 2
  store float %235, float* %238, align 4
  %239 = getelementptr [3 x float], [3 x float]* %2, i32 0, i32 %iMajorAxis.i.1
  store float 0.000000e+00, float* %239, align 4, !tbaa !543
  %240 = load float, float* %236, align 4
  %241 = load float, float* %237, align 4
  %242 = load float, float* %238, align 4
  %.i0327 = bitcast float %240 to i32
  %.i1328 = bitcast float %241 to i32
  %.i2329 = bitcast float %242 to i32
  %243 = shl i32 %.i1328, 5
  %244 = xor i32 %243, %.i1328
  %245 = shl i32 %.i2329, 13
  %246 = xor i32 %245, %.i2329
  %247 = add i32 %244, %.i0327
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
  %.i0330 = fsub fast float %218, %223
  %.i1331 = fsub fast float %224, %229
  %.i2332 = fsub fast float %230, %235
  %259 = call float @dx.op.dot3.f32(i32 55, float %.i0330, float %.i1331, float %.i2332, float %.i0234, float %.i1235, float %.i2236)  ; Dot3(ax,ay,az,bx,by,bz)
  %.i0333 = fmul fast float %259, %.i0234
  %.i1334 = fmul fast float %259, %.i1235
  %.i2335 = fmul fast float %259, %.i2236
  %.i0336 = fadd fast float %.i0333, %223
  %.i1337 = fadd fast float %.i1334, %229
  %.i2338 = fadd fast float %.i2335, %235
  %260 = fmul fast float %.i0336, %150
  %FMad102 = call float @dx.op.tertiary.f32(i32 46, float %.i1337, float %154, float %260)  ; FMad(a,b,c)
  %FMad101 = call float @dx.op.tertiary.f32(i32 46, float %.i2338, float %158, float %FMad102)  ; FMad(a,b,c)
  %261 = fadd fast float %FMad101, %162
  %262 = fmul fast float %.i0336, %151
  %FMad99 = call float @dx.op.tertiary.f32(i32 46, float %.i1337, float %155, float %262)  ; FMad(a,b,c)
  %FMad98 = call float @dx.op.tertiary.f32(i32 46, float %.i2338, float %159, float %FMad99)  ; FMad(a,b,c)
  %263 = fadd fast float %FMad98, %163
  %264 = fmul fast float %.i0336, %152
  %FMad93 = call float @dx.op.tertiary.f32(i32 46, float %.i1337, float %156, float %264)  ; FMad(a,b,c)
  %FMad92 = call float @dx.op.tertiary.f32(i32 46, float %.i2338, float %160, float %FMad93)  ; FMad(a,b,c)
  %265 = fadd fast float %FMad92, %164
  %266 = fdiv fast float 1.000000e+00, %265
  %.i0339 = fmul fast float %261, 5.000000e-01
  %.i0342 = fmul fast float %.i0339, %266
  %.i1340 = fmul fast float %263, 5.000000e-01
  %.i1343 = fmul fast float %.i1340, %266
  %.i0344 = fadd fast float %.i0342, 5.000000e-01
  %.i1345435 = fsub fast float 5.000000e-01, %.i1343
  %.i0346 = fmul fast float %.i0344, %166
  %.i1347 = fmul fast float %.i1345435, %167
  %.i0229.neg = fsub fast float -5.000000e-01, %11
  %.i0348 = fadd fast float %.i0229.neg, %.i0346
  %.i1230.neg = fsub fast float -5.000000e-01, %12
  %.i1349 = fadd fast float %.i1230.neg, %.i1347
  %Round_ni = call float @dx.op.unary.f32(i32 27, float %.i0348)  ; Round_ni(value)
  %Round_ni46 = call float @dx.op.unary.f32(i32 27, float %.i1349)  ; Round_ni(value)
  %.i0350 = fptosi float %Round_ni to i32
  %.i1351 = fptosi float %Round_ni46 to i32
  %.i0352 = add i32 %.i0350, %258
  %.i1353 = add i32 %.i1351, %257
  %.i0354 = and i32 %.i0352, 255
  %.i1355 = and i32 %.i1353, 255
  %267 = call %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32 160, %"class.Texture2D<vector<float, 4> >" %4)  ; CreateHandleForLib(Resource)
  %TextureLoad147 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %267, i32 0, i32 %.i0354, i32 %.i1355, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %.i0258 = extractvalue %dx.types.ResRet.f32 %TextureLoad147, 0
  %.i1260 = extractvalue %dx.types.ResRet.f32 %TextureLoad147, 1
  %.i2262 = extractvalue %dx.types.ResRet.f32 %TextureLoad147, 2
  %268 = icmp sgt i32 %139, 0
  br i1 %268, label %.lr.ph30.preheader, label %._crit_edge.31

.lr.ph30:                                         ; preds = %.lr.ph30.preheader, %._crit_edge
  %reflectionRayIndex.029 = phi i32 [ %338, %._crit_edge ], [ 0, %.lr.ph30.preheader ]
  %269 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)  ; CBufferLoadLegacy(handle,regIndex)
  %270 = extractvalue %dx.types.CBufRet.i32 %269, 1
  %271 = mul i32 %270, %139
  %272 = add i32 %271, %reflectionRayIndex.029
  %273 = uitofp i32 %272 to float
  %.i0255 = fmul fast float %273, 0x3FE827F540000000
  %.i1256 = fmul fast float %273, 0x3FE23C21C0000000
  %.i2257 = fmul fast float %273, 0x3FF9E377A0000000
  %.i0259 = fadd fast float %.i0255, %.i0258
  %.i1261 = fadd fast float %.i1256, %.i1260
  %.i2263 = fadd fast float %.i2257, %.i2262
  %Frc63 = call float @dx.op.unary.f32(i32 22, float %.i0259)  ; Frc(value)
  %Frc64 = call float @dx.op.unary.f32(i32 22, float %.i1261)  ; Frc(value)
  %Frc65 = call float @dx.op.unary.f32(i32 22, float %.i2263)  ; Frc(value)
  %.i1265 = fmul fast float %Frc64, 0x3FD99999A0000000
  %274 = fmul fast float %Frc63, 0x401921FB60000000
  %275 = fsub fast float 1.000000e+00, %.i1265
  %276 = fmul fast float %.i1265, %346
  %277 = fadd fast float %276, 1.000000e+00
  %278 = fdiv fast float %275, %277
  %Saturate39 = call float @dx.op.unary.f32(i32 7, float %278)  ; Saturate(value)
  %Sqrt38 = call float @dx.op.unary.f32(i32 24, float %Saturate39)  ; Sqrt(value)
  %279 = fmul fast float %Sqrt38, %Sqrt38
  %280 = fsub fast float 1.000000e+00, %279
  %Sqrt37 = call float @dx.op.unary.f32(i32 24, float %280)  ; Sqrt(value)
  %Sin36 = call float @dx.op.unary.f32(i32 13, float %274)  ; Sin(value)
  %Cos35 = call float @dx.op.unary.f32(i32 12, float %274)  ; Cos(value)
  %281 = fmul fast float %Cos35, %Sqrt37
  %282 = fmul fast float %Sin36, %Sqrt37
  br i1 %347, label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit", label %283

; <label>:283                                     ; preds = %.lr.ph30
  br label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit"

"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit": ; preds = %283, %.lr.ph30
  %.025.i0 = phi float [ %360, %283 ], [ -1.000000e+00, %.lr.ph30 ]
  %.025.i1 = phi float [ %367, %283 ], [ 0.000000e+00, %.lr.ph30 ]
  %.025.i2 = phi float [ %368, %283 ], [ 0.000000e+00, %.lr.ph30 ]
  %.024.i0 = phi float [ %363, %283 ], [ 0.000000e+00, %.lr.ph30 ]
  %.024.i2 = phi float [ %364, %283 ], [ 0.000000e+00, %.lr.ph30 ]
  %.i0270 = fmul fast float %.024.i0, %281
  %.i1271 = fmul fast float %.025.i0, %281
  %.i2272 = fmul fast float %.024.i2, %281
  %.i0273 = fmul fast float %.025.i0, %282
  %.i1274 = fmul fast float %.025.i1, %282
  %.i2275 = fmul fast float %.025.i2, %282
  %.i0279 = fmul fast float %Sqrt38, %.i0206
  %.i1280 = fmul fast float %Sqrt38, %.i1207
  %.i2281 = fmul fast float %Sqrt38, %.i2208
  %.i0276 = fadd fast float %.i0273, %.i0279
  %.i0282 = fadd fast float %.i0276, %.i0270
  %.i1277 = fadd fast float %.i1271, %.i1280
  %.i1283 = fadd fast float %.i1277, %.i1274
  %.i2278 = fadd fast float %.i2275, %.i2281
  %.i2284 = fadd fast float %.i2278, %.i2272
  %284 = call float @dx.op.dot3.f32(i32 55, float %.i0267, float %.i1268, float %.i2269, float %.i0282, float %.i1283, float %.i2284)  ; Dot3(ax,ay,az,bx,by,bz)
  %285 = fmul fast float %284, 2.000000e+00
  %.i0285 = fmul fast float %285, %.i0282
  %.i1286 = fmul fast float %285, %.i1283
  %.i2287 = fmul fast float %.i2284, %285
  %.i0288 = fadd fast float %.i0285, %.i0160
  %.i1289 = fadd fast float %.i1286, %.i1161
  %.i2290 = fadd fast float %.i2287, %.i2162
  %286 = fmul fast float %.i0288, %.i0288
  %287 = fmul fast float %.i1289, %.i1289
  %288 = fadd fast float %286, %287
  %289 = fmul fast float %.i2290, %.i2290
  %290 = fadd fast float %288, %289
  %Sqrt69 = call float @dx.op.unary.f32(i32 24, float %290)  ; Sqrt(value)
  %.i0291 = fdiv fast float %.i0288, %Sqrt69
  %.i1292 = fdiv fast float %.i1289, %Sqrt69
  %.i2293 = fdiv fast float %.i2290, %Sqrt69
  %291 = call float @dx.op.dot3.f32(i32 55, float %.i0291, float %.i1292, float %.i2293, float %.i0206, float %.i1207, float %.i2208)  ; Dot3(ax,ay,az,bx,by,bz)
  %292 = fcmp fast olt float %291, 0x3F747AE140000000
  br i1 %292, label %.lr.ph.preheader, label %._crit_edge

.lr.ph.preheader:                                 ; preds = %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit"
  %293 = uitofp i32 %270 to float
  br label %.lr.ph

.lr.ph:                                           ; preds = %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13", %.lr.ph.preheader
  %i.028 = phi i32 [ %294, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ], [ 0, %.lr.ph.preheader ]
  %294 = add nuw nsw i32 %i.028, 1
  %295 = uitofp i32 %294 to float
  %296 = fmul fast float %295, 1.000000e+01
  %297 = fadd fast float %296, %293
  %.i0294 = fmul fast float %297, 0x3FE827F540000000
  %.i1295 = fmul fast float %297, 0x3FE23C21C0000000
  %.i0296 = fadd fast float %.i0294, %.i0258
  %.i1297 = fadd fast float %.i1295, %.i1260
  %Frc = call float @dx.op.unary.f32(i32 22, float %.i0296)  ; Frc(value)
  %Frc47 = call float @dx.op.unary.f32(i32 22, float %.i1297)  ; Frc(value)
  %.i1299 = fmul fast float %Frc47, 0x3FD99999A0000000
  %298 = fmul fast float %Frc, 0x401921FB60000000
  %299 = fsub fast float 1.000000e+00, %.i1299
  %300 = fmul fast float %.i1299, %346
  %301 = fadd fast float %300, 1.000000e+00
  %302 = fdiv fast float %299, %301
  %Saturate = call float @dx.op.unary.f32(i32 7, float %302)  ; Saturate(value)
  %Sqrt34 = call float @dx.op.unary.f32(i32 24, float %Saturate)  ; Sqrt(value)
  %303 = fmul fast float %Sqrt34, %Sqrt34
  %304 = fsub fast float 1.000000e+00, %303
  %Sqrt = call float @dx.op.unary.f32(i32 24, float %304)  ; Sqrt(value)
  %Sin = call float @dx.op.unary.f32(i32 13, float %298)  ; Sin(value)
  %Cos = call float @dx.op.unary.f32(i32 12, float %298)  ; Cos(value)
  %305 = fmul fast float %Cos, %Sqrt
  %306 = fmul fast float %Sin, %Sqrt
  br i1 %347, label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13", label %307

; <label>:307                                     ; preds = %.lr.ph
  br label %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13"

"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13": ; preds = %307, %.lr.ph
  %.027.i0 = phi float [ %360, %307 ], [ -1.000000e+00, %.lr.ph ]
  %.027.i1 = phi float [ %367, %307 ], [ 0.000000e+00, %.lr.ph ]
  %.027.i2 = phi float [ %368, %307 ], [ 0.000000e+00, %.lr.ph ]
  %.026.i0 = phi float [ %363, %307 ], [ 0.000000e+00, %.lr.ph ]
  %.026.i2 = phi float [ %364, %307 ], [ 0.000000e+00, %.lr.ph ]
  %.i0303 = fmul fast float %.026.i0, %305
  %.i1304 = fmul fast float %.027.i0, %305
  %.i2305 = fmul fast float %.026.i2, %305
  %.i0306 = fmul fast float %.027.i0, %306
  %.i1307 = fmul fast float %.027.i1, %306
  %.i2308 = fmul fast float %.027.i2, %306
  %.i0312 = fmul fast float %Sqrt34, %.i0206
  %.i1313 = fmul fast float %Sqrt34, %.i1207
  %.i2314 = fmul fast float %Sqrt34, %.i2208
  %.i0309 = fadd fast float %.i0306, %.i0312
  %.i0315 = fadd fast float %.i0309, %.i0303
  %.i1310 = fadd fast float %.i1304, %.i1313
  %.i1316 = fadd fast float %.i1310, %.i1307
  %.i2311 = fadd fast float %.i2308, %.i2314
  %.i2317 = fadd fast float %.i2311, %.i2305
  %308 = call float @dx.op.dot3.f32(i32 55, float %.i0267, float %.i1268, float %.i2269, float %.i0315, float %.i1316, float %.i2317)  ; Dot3(ax,ay,az,bx,by,bz)
  %309 = fmul fast float %308, 2.000000e+00
  %.i0318 = fmul fast float %309, %.i0315
  %.i1319 = fmul fast float %309, %.i1316
  %.i2320 = fmul fast float %.i2317, %309
  %.i0321 = fadd fast float %.i0318, %.i0160
  %.i1322 = fadd fast float %.i1319, %.i1161
  %.i2323 = fadd fast float %.i2320, %.i2162
  %310 = fmul fast float %.i0321, %.i0321
  %311 = fmul fast float %.i1322, %.i1322
  %312 = fadd fast float %310, %311
  %313 = fmul fast float %.i2323, %.i2323
  %314 = fadd fast float %312, %313
  %Sqrt70 = call float @dx.op.unary.f32(i32 24, float %314)  ; Sqrt(value)
  %.i0324 = fdiv fast float %.i0321, %Sqrt70
  %.i1325 = fdiv fast float %.i1322, %Sqrt70
  %.i2326 = fdiv fast float %.i2323, %Sqrt70
  %315 = call float @dx.op.dot3.f32(i32 55, float %.i0324, float %.i1325, float %.i2326, float %.i0206, float %.i1207, float %.i2208)  ; Dot3(ax,ay,az,bx,by,bz)
  %316 = fcmp fast olt float %315, 0x3F747AE140000000
  %317 = icmp ult i32 %294, 100
  %318 = and i1 %317, %316
  br i1 %318, label %.lr.ph, label %._crit_edge.loopexit

._crit_edge.loopexit:                             ; preds = %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13"
  %.i2326.lcssa = phi float [ %.i2326, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ]
  %.i1325.lcssa = phi float [ %.i1325, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ]
  %.i0324.lcssa = phi float [ %.i0324, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit13" ]
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge.loopexit, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit"
  %vReflectionDirectionInView.0.lcssa.i0 = phi float [ %.i0291, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit" ], [ %.i0324.lcssa, %._crit_edge.loopexit ]
  %vReflectionDirectionInView.0.lcssa.i1 = phi float [ %.i1292, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit" ], [ %.i1325.lcssa, %._crit_edge.loopexit ]
  %vReflectionDirectionInView.0.lcssa.i2 = phi float [ %.i2293, %"\01?internal.getImportanceRay@@YA?AV?$vector@M$02@@V1@0MV?$vector@M$01@@@Z.exit" ], [ %.i2326.lcssa, %._crit_edge.loopexit ]
  %319 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %320 = extractvalue %dx.types.CBufRet.f32 %319, 0
  %321 = extractvalue %dx.types.CBufRet.f32 %319, 1
  %322 = extractvalue %dx.types.CBufRet.f32 %319, 2
  %323 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %324 = extractvalue %dx.types.CBufRet.f32 %323, 0
  %325 = extractvalue %dx.types.CBufRet.f32 %323, 1
  %326 = extractvalue %dx.types.CBufRet.f32 %323, 2
  %327 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %sys_constants, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %328 = extractvalue %dx.types.CBufRet.f32 %327, 0
  %329 = extractvalue %dx.types.CBufRet.f32 %327, 1
  %330 = extractvalue %dx.types.CBufRet.f32 %327, 2
  %331 = fmul fast float %320, %vReflectionDirectionInView.0.lcssa.i0
  %FMad90 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i1, float %324, float %331)  ; FMad(a,b,c)
  %FMad89 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i2, float %328, float %FMad90)  ; FMad(a,b,c)
  %332 = fmul fast float %321, %vReflectionDirectionInView.0.lcssa.i0
  %FMad88 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i1, float %325, float %332)  ; FMad(a,b,c)
  %FMad87 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i2, float %329, float %FMad88)  ; FMad(a,b,c)
  %333 = fmul fast float %322, %vReflectionDirectionInView.0.lcssa.i0
  %FMad86 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i1, float %326, float %333)  ; FMad(a,b,c)
  %FMad85 = call float @dx.op.tertiary.f32(i32 46, float %vReflectionDirectionInView.0.lcssa.i2, float %330, float %FMad86)  ; FMad(a,b,c)
  %334 = fadd fast float %Frc65, 5.000000e-01
  %335 = fmul fast float %349, %334
  %336 = fadd fast float %335, %349
  store i32 %reflectionRayIndex.029, i32* %350, align 8
  %337 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %3)  ; CreateHandleForLib(Resource)
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %337, i32 0, i32 1, i32 0, i32 2, i32 0, float %352, float %353, float %354, float %355, float %FMad89, float %FMad87, float %FMad85, float %336, %struct.HitData* nonnull %10)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  %338 = add nuw nsw i32 %reflectionRayIndex.029, 1
  %exitcond = icmp eq i32 %338, %139
  br i1 %exitcond, label %._crit_edge.31.loopexit, label %.lr.ph30

._crit_edge.31.loopexit:                          ; preds = %._crit_edge
  %.lcssa = phi float [ %336, %._crit_edge ]
  %FMad85.lcssa = phi float [ %FMad85, %._crit_edge ]
  %FMad87.lcssa = phi float [ %FMad87, %._crit_edge ]
  %FMad89.lcssa = phi float [ %FMad89, %._crit_edge ]
  %339 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 2
  %340 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 3
  %341 = insertelement <3 x float> undef, float %FMad89.lcssa, i64 0
  %342 = insertelement <3 x float> %341, float %FMad87.lcssa, i64 1
  %343 = insertelement <3 x float> %342, float %FMad85.lcssa, i64 2
  store <3 x float> %343, <3 x float>* %339, align 4
  store float %.lcssa, float* %340, align 4
  br label %._crit_edge.31

._crit_edge.31:                                   ; preds = %._crit_edge.31.loopexit, %"\01?internal.getWorldSpaceNoiseOffset@@YA?AV?$vector@I$01@@V?$vector@M$01@@V?$matrix@M$03$03@@10MV?$vector@M$02@@22M_N@Z.exit33"
  ret void

.lr.ph30.preheader:                               ; preds = %"\01?internal.getWorldSpaceNoiseOffset@@YA?AV?$vector@I$01@@V?$vector@M$01@@V?$matrix@M$03$03@@10MV?$vector@M$02@@22M_N@Z.exit33"
  %.i0267 = fsub fast float -0.000000e+00, %.i0160
  %.i1268 = fsub fast float -0.000000e+00, %.i1161
  %.i2269 = fsub fast float -0.000000e+00, %.i2162
  %344 = fmul fast float %134, %134
  %345 = fmul fast float %344, %344
  %346 = fadd fast float %345, -1.000000e+00
  %347 = fcmp fast olt float %.i2208, 0xBFEFFFFFC0000000
  %Saturate44 = call float @dx.op.unary.f32(i32 7, float %134)  ; Saturate(value)
  %Sqrt45 = call float @dx.op.unary.f32(i32 24, float %Saturate44)  ; Sqrt(value)
  %348 = fmul fast float %Sqrt45, 6.000000e+00
  %Exp43 = call float @dx.op.unary.f32(i32 21, float %348)  ; Exp(value)
  %349 = fdiv fast float 0x4066666660000000, %Exp43
  %350 = getelementptr inbounds %struct.HitData, %struct.HitData* %10, i32 0, i32 0
  %351 = load <3 x float>, <3 x float>* %136, align 4
  %352 = extractelement <3 x float> %351, i64 0
  %353 = extractelement <3 x float> %351, i64 1
  %354 = extractelement <3 x float> %351, i64 2
  %355 = load float, float* %137, align 4
  %356 = fadd fast float %.i2208, 1.000000e+00
  %357 = fdiv fast float 1.000000e+00, %356
  %358 = fmul fast float %.i0206, %.i1207
  %359 = fmul fast float %358, %357
  %360 = fsub fast float -0.000000e+00, %359
  %361 = fmul fast float %.i0206, %.i0206
  %362 = fmul fast float %361, %357
  %363 = fsub fast float 1.000000e+00, %362
  %364 = fsub fast float -0.000000e+00, %.i0206
  %365 = fmul fast float %.i1207, %.i1207
  %366 = fmul fast float %365, %357
  %367 = fsub fast float 1.000000e+00, %366
  %368 = fsub fast float -0.000000e+00, %.i1207
  br label %.lr.ph30
}

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind readnone
declare float @dx.op.unary.f32(i32, float) #2

; Function Attrs: nounwind readnone
declare float @dx.op.dot3.f32(i32, float, float, float, float, float, float) #2

; Function Attrs: nounwind readnone
declare float @dx.op.binary.f32(i32, float, float) #2

; Function Attrs: nounwind readnone
declare float @dx.op.tertiary.f32(i32, float, float, float) #2

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32, %dx.types.Handle, %dx.types.Handle, float, float, float, float, i32, i32, i32, float) #1

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32, %dx.types.Handle, i32, i32, i8, i32) #1

; Function Attrs: nounwind
declare i32 @dx.op.atomicBinOp.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32) #3

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32, %dx.types.Handle, i32, i32, i8, i32) #1

; Function Attrs: nounwind readnone
declare i32 @dx.op.binary.i32(i32, i32, i32) #2

; Function Attrs: nounwind readnone
declare i32 @dx.op.primitiveIndex.i32(i32) #2

; Function Attrs: noreturn nounwind
declare void @dx.op.ignoreHit(i32) #4

; Function Attrs: nounwind readnone
declare float @dx.op.worldRayDirection.f32(i32, i8) #2

; Function Attrs: nounwind readnone
declare float @dx.op.worldRayOrigin.f32(i32, i8) #2

; Function Attrs: nounwind readonly
declare float @dx.op.rayTCurrent.f32(i32) #1

; Function Attrs: nounwind readnone
declare i32 @dx.op.dispatchRaysIndex.i32(i32, i8) #2

; Function Attrs: nounwind
declare void @dx.op.textureStore.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i32, i8) #3

; Function Attrs: nounwind
declare void @dx.op.textureStore.f32(i32, %dx.types.Handle, i32, i32, i32, float, float, float, float, i8) #3

; Function Attrs: nounwind
declare void @dx.op.traceRay.struct.HitData(i32, %dx.types.Handle, i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, %struct.HitData*) #3

; Function Attrs: nounwind readnone
declare float @dx.op.worldToObject.f32(i32, i32, i8) #2

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.sys_constants(i32, %sys_constants) #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.g_cbRaytracingHit(i32, %g_cbRaytracingHit) #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.rtreflection(i32, %rtreflection) #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.Texture2D<vector<float, 4> >"(i32, %"class.Texture2D<vector<float, 4> >") #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.struct.SamplerState(i32, %struct.SamplerState) #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<MaterialDataPart3>"(i32, %"class.StructuredBuffer<MaterialDataPart3>") #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.struct.ByteAddressBuffer(i32, %struct.ByteAddressBuffer) #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32, %"class.RWTexture2DArray<unsigned int>") #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<vector<float, 4> >"(i32, %"class.RWTexture2DArray<vector<float, 4> >") #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @"dx.op.createHandleForLib.class.RWStructuredBuffer<unsigned int>"(i32, %"class.RWStructuredBuffer<unsigned int>") #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32, %struct.RaytracingAccelerationStructure) #1

attributes #0 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="0" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }
attributes #4 = { noreturn nounwind }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!1}
!dx.shaderModel = !{!2}
!dx.resources = !{!3}
!dx.typeAnnotations = !{!31, !504}
!dx.entryPoints = !{!514, !517, !519, !521, !523, !525, !526, !527}

!0 = !{!"dxc 1.2"}
!1 = !{i32 1, i32 3}
!2 = !{!"lib", i32 6, i32 3}
!3 = !{!4, !17, !25, !29}
!4 = !{!5, !7, !8, !9, !11, !12, !13, !14, !15}
!5 = !{i32 0, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer1", i32 0, i32 0, i32 1, i32 2, i32 0, !6}
!6 = !{i32 0, i32 9}
!7 = !{i32 1, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tLinearDepth", i32 0, i32 1, i32 1, i32 2, i32 0, !6}
!8 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tClipDepth", i32 0, i32 2, i32 1, i32 2, i32 0, !6}
!9 = !{i32 3, %"class.StructuredBuffer<MaterialDataPart3>"* @"\01?g_sbMaterialDataPart3@@3V?$StructuredBuffer@UMaterialDataPart3@@@@A", !"g_sbMaterialDataPart3", i32 0, i32 3, i32 1, i32 12, i32 0, !10}
!10 = !{i32 1, i32 336}
!11 = !{i32 4, [0 x %"class.Texture2D<vector<float, 4> >"]* @"\01?g_sMaterialTextureArray@@3PAV?$Texture2D@V?$vector@M$03@@@@A", !"g_sMaterialTextureArray", i32 1, i32 0, i32 -1, i32 2, i32 0, !6}
!12 = !{i32 5, %struct.ByteAddressBuffer* @"\01?g_bRaytracingIndexBuffer@@3UByteAddressBuffer@@A", !"g_bRaytracingIndexBuffer", i32 3, i32 0, i32 1, i32 11, i32 0, null}
!13 = !{i32 6, %struct.ByteAddressBuffer* @"\01?g_bRaytracingVertexBuffer1@@3UByteAddressBuffer@@A", !"g_bRaytracingVertexBuffer1", i32 3, i32 1, i32 1, i32 11, i32 0, null}
!14 = !{i32 7, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tStaticBlueNoiseRGBA_0", i32 0, i32 4, i32 1, i32 2, i32 0, !6}
!15 = !{i32 8, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", !"g_rtScene", i32 0, i32 5, i32 1, i32 16, i32 0, !16}
!16 = !{i32 0, i32 4}
!17 = !{!18, !20, !21, !22, !24}
!18 = !{i32 0, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", !"g_rwtMaterialId", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !19}
!19 = !{i32 0, i32 5}
!20 = !{i32 1, %"class.RWTexture2DArray<vector<float, 4> >"* @"\01?g_rwtNormal_TexcoordX@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A", !"g_rwtNormal_TexcoordX", i32 0, i32 1, i32 1, i32 7, i1 false, i1 false, i1 false, !6}
!21 = !{i32 2, %"class.RWTexture2DArray<vector<float, 4> >"* @"\01?g_rwtPosition_TexcoordY@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A", !"g_rwtPosition_TexcoordY", i32 0, i32 2, i32 1, i32 7, i1 false, i1 false, i1 false, !6}
!22 = !{i32 3, %"class.RWStructuredBuffer<unsigned int>"* @"\01?g_rwbPerMaterialCount@@3V?$RWStructuredBuffer@I@@A", !"g_rwbPerMaterialCount", i32 0, i32 3, i32 1, i32 12, i1 false, i1 false, i1 false, !23}
!23 = !{i32 1, i32 4}
!24 = !{i32 4, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", !"g_rwtShadow", i32 0, i32 4, i32 1, i32 7, i1 false, i1 false, i1 false, !19}
!25 = !{!26, !27, !28}
!26 = !{i32 0, %sys_constants* @sys_constants, !"sys_constants", i32 0, i32 0, i32 1, i32 1128, null}
!27 = !{i32 1, %g_cbRaytracingHit* @g_cbRaytracingHit, !"g_cbRaytracingHit", i32 3, i32 0, i32 1, i32 40, null}
!28 = !{i32 2, %rtreflection* @rtreflection, !"rtreflection", i32 0, i32 1, i32 1, i32 24, null}
!29 = !{!30}
!30 = !{i32 0, %struct.SamplerState* @"\01?g_sLinearWrap@@3USamplerState@@A", !"g_sLinearWrap", i32 1, i32 5, i32 1, i32 0, null}
!31 = !{i32 0, %"class.Texture2D<vector<float, 4> >" undef, !32, %"class.Texture2D<vector<float, 4> >::mips_type" undef, !35, %struct.GBufferFormat undef, !37, %struct.SamplerState undef, !40, %"class.Texture2D<float>" undef, !42, %"class.Texture2D<float>::mips_type" undef, !35, %"class.Texture2D<unsigned int>" undef, !44, %"class.Texture2D<unsigned int>::mips_type" undef, !35, %"class.Texture2DMS<float, 0>" undef, !46, %"class.Texture2DMS<float, 0>::sample_type" undef, !35, %"class.StructuredBuffer<vector<unsigned int, 4> >" undef, !48, %"class.StructuredBuffer<MaterialDataPart1>" undef, !49, %struct.MaterialDataPart1 undef, !51, %"class.StructuredBuffer<MaterialDataPart3>" undef, !54, %struct.MaterialDataPart3 undef, !55, %"class.StructuredBuffer<MaterialBindingData>" undef, !99, %struct.MaterialBindingData undef, !100, %struct.order2_sh undef, !102, %"class.RWTexture3D<vector<float, 4> >" undef, !107, %"class.Texture3D<vector<float, 4> >" undef, !32, %"class.Texture3D<vector<float, 4> >::mips_type" undef, !35, %struct.LightVolumeData undef, !108, %"class.TextureCube<vector<float, 4> >" undef, !107, %"class.StructuredBuffer<DeferredLightPoint>" undef, !111, %struct.DeferredLightPoint undef, !112, %"class.StructuredBuffer<DeferredLightPointClipping>" undef, !127, %struct.DeferredLightPointClipping undef, !128, %"class.StructuredBuffer<DeferredLightPointBVH>" undef, !130, %struct.DeferredLightPointBVH undef, !131, %"class.RWTexture2D<unsigned int>" undef, !138, %"class.StructuredBuffer<DeferredLightSpot>" undef, !139, %struct.DeferredLightSpot undef, !140, %"class.StructuredBuffer<DeferredLightSpotClipping>" undef, !165, %struct.DeferredLightSpotClipping undef, !166, %"class.StructuredBuffer<DeferredLightSpotBVH>" undef, !130, %struct.DeferredLightSpotBVH undef, !131, %"class.StructuredBuffer<DeferredLightSun>" undef, !171, %struct.DeferredLightSun undef, !172, %"class.StructuredBuffer<VolumeSamplingInfo>" undef, !186, %struct.VolumeSamplingInfo undef, !187, %"class.StructuredBuffer<unsigned int>" undef, !138, %"class.StructuredBuffer<InternalNode>" undef, !127, %struct.InternalNode undef, !197, %"class.Texture3D<vector<float, 3> >" undef, !202, %"class.Texture3D<vector<float, 3> >::mips_type" undef, !35, %struct.VolumeCullingInCB undef, !204, %struct.VolumeTopLevelInfo undef, !209, %struct.DeferredLightSpecularBRDF undef, !213, %struct.DeferredLightGenericSurface undef, !216, %struct.DeferredLightTransparentBRDF undef, !220, %struct.DeferredLightIntensity undef, !223, %struct.DeferredLightVolumeResult undef, !226, %struct.DeferredLightPointSurface undef, !228, %struct.DeferredLightGBufferData undef, !229, %struct.CellInfo undef, !236, %"class.StructuredBuffer<EnvironmentMapDynamicInfo>" undef, !99, %struct.EnvironmentMapDynamicInfo undef, !262, %"class.StructuredBuffer<EnvironmentMapReference>" undef, !99, %struct.EnvironmentMapReference undef, !264, %"class.StructuredBuffer<CellInfo>" undef, !266, %"class.StructuredBuffer<Node>" undef, !267, %struct.Node undef, !268, %struct.ChildMask undef, !272, %struct.NodePointer undef, !274, %"class.StructuredBuffer<IrradianceProbe>" undef, !276, %struct.IrradianceProbe undef, !277, %"class.StructuredBuffer<TransportProbe>" undef, !287, %struct.TransportProbe undef, !288, %"class.StructuredBuffer<vector<float, 4> >" undef, !107, %"class.StructuredBuffer<ProbeRef>" undef, !49, %struct.ProbeRef undef, !290, %"class.StructuredBuffer<EnvironmentMapInfo>" undef, !293, %struct.EnvironmentMapInfo undef, !294, %struct.VoxelInfo undef, !300, %struct.LeafInfo undef, !306, %"class.StructuredBuffer<PerInstanceLBData>" undef, !127, %struct.PerInstanceLBData undef, !314, %"class.StructuredBuffer<RenderInstanceData>" undef, !318, %struct.RenderInstanceData undef, !319, %struct.ByteAddressBuffer undef, !40, %struct.RaytracingHitRootConstants undef, !327, %struct.RaytracingAccelerationStructure undef, !40, %struct.IntersectionAttributes undef, !338, %struct.HitData undef, !340, %"class.RWTexture2DArray<unsigned int>" undef, !138, %"class.RWTexture2DArray<vector<float, 4> >" undef, !107, %"class.RWStructuredBuffer<unsigned int>" undef, !138, %struct.RayDesc undef, !342, %debug_general undef, !347, %sys_constants undef, !370, %shadow_general undef, !403, %mid_translucency undef, !408, %mid_general undef, !100, %deferredlight_constants undef, !410, %env_general undef, !446, %atmosphere_general undef, !449, %illuminationvolume undef, !470, %ManualUpdateCB_ActiveVolumeTopLevelInfoArray undef, !477, %ManualUpdateCB_DebugVolumeTopLevelInfoArray undef, !479, %WorldGridInfo undef, !481, %EnvironmentMapAtlas undef, !485, %vertex_binding undef, !491, %g_cbRaytracingHit undef, !495, %rtreflection undef, !497}
!32 = !{i32 20, !33, !34}
!33 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 9}
!34 = !{i32 6, !"mips", i32 3, i32 16}
!35 = !{i32 4, !36}
!36 = !{i32 6, !"handle", i32 3, i32 0, i32 7, i32 5}
!37 = !{i32 32, !38, !39}
!38 = !{i32 6, !"vTarget1", i32 3, i32 0, i32 4, !"SV_Target0", i32 7, i32 9}
!39 = !{i32 6, !"vTarget2", i32 3, i32 16, i32 4, !"SV_Target1", i32 7, i32 9}
!40 = !{i32 4, !41}
!41 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 4}
!42 = !{i32 8, !33, !43}
!43 = !{i32 6, !"mips", i32 3, i32 4}
!44 = !{i32 8, !45, !43}
!45 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 5}
!46 = !{i32 8, !33, !47}
!47 = !{i32 6, !"sample", i32 3, i32 4}
!48 = !{i32 16, !45}
!49 = !{i32 8, !50}
!50 = !{i32 6, !"h", i32 3, i32 0}
!51 = !{i32 8, !52, !53}
!52 = !{i32 6, !"vSpecularColor_uBRDF", i32 3, i32 0, i32 7, i32 5}
!53 = !{i32 6, !"uScatterIndex_vTranslucencyDepthRange", i32 3, i32 4, i32 7, i32 5}
!54 = !{i32 352, !50}
!55 = !{i32 352, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !87, !88, !89, !90, !91, !92, !93, !94, !95, !96, !97, !98}
!56 = !{i32 6, !"fClothDiffScatterAmount", i32 3, i32 0, i32 7, i32 9}
!57 = !{i32 6, !"fClothFuzzWrap", i32 3, i32 4, i32 7, i32 9}
!58 = !{i32 6, !"vClothScatterColor", i32 3, i32 16, i32 7, i32 9}
!59 = !{i32 6, !"fHairSmoothness", i32 3, i32 28, i32 7, i32 9}
!60 = !{i32 6, !"vSpecularColor2", i32 3, i32 32, i32 7, i32 9}
!61 = !{i32 6, !"fHairSpecularShift", i32 3, i32 44, i32 7, i32 9}
!62 = !{i32 6, !"fHairDiffuseWrap", i32 3, i32 48, i32 7, i32 9}
!63 = !{i32 6, !"fPAD1", i32 3, i32 52, i32 7, i32 9}
!64 = !{i32 6, !"fEmissionIntensity", i32 3, i32 56, i32 7, i32 9}
!65 = !{i32 6, !"vColorMultiplier", i32 3, i32 64, i32 7, i32 9}
!66 = !{i32 6, !"vSmoothnessRange", i32 3, i32 80, i32 7, i32 9}
!67 = !{i32 6, !"vShadowSoftnessRange", i32 3, i32 88, i32 7, i32 9}
!68 = !{i32 6, !"vScatterWidthRange", i32 3, i32 96, i32 7, i32 9}
!69 = !{i32 6, !"fHairDepthOffsetAmount", i32 3, i32 104, i32 7, i32 9}
!70 = !{i32 6, !"fHairNormalAmount", i32 3, i32 108, i32 7, i32 9}
!71 = !{i32 6, !"vEmissionMultiplier", i32 3, i32 112, i32 7, i32 9}
!72 = !{i32 6, !"vColorMultiplier2", i32 3, i32 128, i32 7, i32 9}
!73 = !{i32 6, !"vBaseDetailUVScale", i32 3, i32 144, i32 7, i32 9}
!74 = !{i32 6, !"vSmoothnessRange2", i32 3, i32 152, i32 7, i32 9}
!75 = !{i32 6, !"vBlendDetailUVScale", i32 3, i32 160, i32 7, i32 9}
!76 = !{i32 6, !"fBaseDetailAmount", i32 3, i32 168, i32 7, i32 9}
!77 = !{i32 6, !"fBlendUVMultiplier", i32 3, i32 172, i32 7, i32 9}
!78 = !{i32 6, !"fBlendDetailAmount", i32 3, i32 176, i32 7, i32 9}
!79 = !{i32 6, !"fFuzzUVMultiplier", i32 3, i32 180, i32 7, i32 9}
!80 = !{i32 6, !"fPupilDilation", i32 3, i32 184, i32 7, i32 9}
!81 = !{i32 6, !"fPupilOuterRadius", i32 3, i32 188, i32 7, i32 9}
!82 = !{i32 6, !"vIntensity", i32 3, i32 192, i32 7, i32 9}
!83 = !{i32 6, !"vWrinkleWeights0", i32 3, i32 208, i32 7, i32 9}
!84 = !{i32 6, !"vWrinkleWeights1", i32 3, i32 224, i32 7, i32 9}
!85 = !{i32 6, !"vWrinkleWeights2", i32 3, i32 240, i32 7, i32 9}
!86 = !{i32 6, !"vWrinkleWeights3", i32 3, i32 256, i32 7, i32 9}
!87 = !{i32 6, !"vWrinkleWeights4", i32 3, i32 272, i32 7, i32 9}
!88 = !{i32 6, !"vWrinkleWeights5", i32 3, i32 288, i32 7, i32 9}
!89 = !{i32 6, !"uDisableBackfaceFlip", i32 3, i32 304, i32 7, i32 5}
!90 = !{i32 6, !"fEdgeSharpness", i32 3, i32 308, i32 7, i32 9}
!91 = !{i32 6, !"fTrunkBendFactor", i32 3, i32 312, i32 7, i32 9}
!92 = !{i32 6, !"fTrunkPivotOffset", i32 3, i32 316, i32 7, i32 9}
!93 = !{i32 6, !"vColorRgbMultiplier", i32 3, i32 320, i32 7, i32 9}
!94 = !{i32 6, !"fWindFactor", i32 3, i32 332, i32 7, i32 9}
!95 = !{i32 6, !"fDecalMaterialBlendFactor", i32 3, i32 336, i32 7, i32 9}
!96 = !{i32 6, !"fDecalAlbedoBlendFactor", i32 3, i32 340, i32 7, i32 9}
!97 = !{i32 6, !"uStableHash", i32 3, i32 344, i32 7, i32 5}
!98 = !{i32 6, !"pad", i32 3, i32 348, i32 7, i32 5}
!99 = !{i32 4, !50}
!100 = !{i32 4, !101}
!101 = !{i32 6, !"g_uMaterialID", i32 3, i32 0, i32 7, i32 5}
!102 = !{i32 60, !103, !104, !105, !106}
!103 = !{i32 6, !"c0", i32 3, i32 0, i32 7, i32 9}
!104 = !{i32 6, !"c11", i32 3, i32 16, i32 7, i32 9}
!105 = !{i32 6, !"c10", i32 3, i32 32, i32 7, i32 9}
!106 = !{i32 6, !"c1m1", i32 3, i32 48, i32 7, i32 9}
!107 = !{i32 16, !33}
!108 = !{i32 32, !109, !110}
!109 = !{i32 6, !"value0", i32 3, i32 0, i32 7, i32 9}
!110 = !{i32 6, !"value1", i32 3, i32 16, i32 7, i32 9}
!111 = !{i32 160, !50}
!112 = !{i32 160, !113, !114, !115, !116, !117, !118, !119, !120, !122, !123, !124, !125, !126}
!113 = !{i32 6, !"vPositionInView", i32 3, i32 0, i32 7, i32 9}
!114 = !{i32 6, !"fShadowMapShrink", i32 3, i32 12, i32 7, i32 9}
!115 = !{i32 6, !"vColor", i32 3, i32 16, i32 7, i32 9}
!116 = !{i32 6, !"fClipRange", i32 3, i32 28, i32 7, i32 9}
!117 = !{i32 6, !"vFalloff", i32 3, i32 32, i32 7, i32 9}
!118 = !{i32 6, !"vDirectionInView", i32 3, i32 48, i32 7, i32 9}
!119 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 60, i32 7, i32 9}
!120 = !{i32 6, !"mViewToShadowClip", i32 2, !121, i32 3, i32 64, i32 7, i32 9}
!121 = !{i32 4, i32 4, i32 2}
!122 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 128, i32 7, i32 9}
!123 = !{i32 6, !"fRadius", i32 3, i32 144, i32 7, i32 9}
!124 = !{i32 6, !"fBoundsRadius", i32 3, i32 148, i32 7, i32 9}
!125 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 152, i32 7, i32 9}
!126 = !{i32 6, !"uTechniqueProperties", i32 3, i32 156, i32 7, i32 5}
!127 = !{i32 16, !50}
!128 = !{i32 16, !113, !129}
!129 = !{i32 6, !"fBoundsRadius", i32 3, i32 12, i32 7, i32 9}
!130 = !{i32 44, !50}
!131 = !{i32 44, !132, !133, !134, !135, !136, !137}
!132 = !{i32 6, !"vMinInView", i32 3, i32 0, i32 7, i32 9}
!133 = !{i32 6, !"vMaxInView", i32 3, i32 16, i32 7, i32 9}
!134 = !{i32 6, !"uChildLHS", i32 3, i32 28, i32 7, i32 5}
!135 = !{i32 6, !"uChildRHS", i32 3, i32 32, i32 7, i32 5}
!136 = !{i32 6, !"uLightIndex", i32 3, i32 36, i32 7, i32 5}
!137 = !{i32 6, !"uCount", i32 3, i32 40, i32 7, i32 5}
!138 = !{i32 4, !45}
!139 = !{i32 336, !50}
!140 = !{i32 336, !113, !141, !142, !143, !144, !145, !146, !147, !148, !149, !150, !151, !152, !153, !154, !156, !157, !158, !159, !160, !161, !162, !163, !164}
!141 = !{i32 6, !"fNearDistance", i32 3, i32 12, i32 7, i32 9}
!142 = !{i32 6, !"vWedPositionInView", i32 3, i32 16, i32 7, i32 9}
!143 = !{i32 6, !"fFarDistance", i32 3, i32 28, i32 7, i32 9}
!144 = !{i32 6, !"vDirectionInView", i32 3, i32 32, i32 7, i32 9}
!145 = !{i32 6, !"fFarWidth", i32 3, i32 44, i32 7, i32 9}
!146 = !{i32 6, !"vColor", i32 3, i32 48, i32 7, i32 9}
!147 = !{i32 6, !"fClipRange", i32 3, i32 60, i32 7, i32 9}
!148 = !{i32 6, !"vFalloff", i32 3, i32 64, i32 7, i32 9}
!149 = !{i32 6, !"fRadius", i32 3, i32 72, i32 7, i32 9}
!150 = !{i32 6, !"fSinConeAnglePerTwo_sq", i32 3, i32 76, i32 7, i32 9}
!151 = !{i32 6, !"mViewToProjectionClip", i32 2, !121, i32 3, i32 80, i32 7, i32 9}
!152 = !{i32 6, !"mCameraPositionInCone", i32 3, i32 144, i32 7, i32 9}
!153 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 156, i32 7, i32 9}
!154 = !{i32 6, !"mViewToCone", i32 2, !155, i32 3, i32 160, i32 7, i32 9}
!155 = !{i32 3, i32 3, i32 2}
!156 = !{i32 6, !"fShadowPCFSize", i32 3, i32 204, i32 7, i32 9}
!157 = !{i32 6, !"fShadowLOD", i32 3, i32 208, i32 7, i32 9}
!158 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 212, i32 7, i32 9}
!159 = !{i32 6, !"mConeToView", i32 3, i32 224, i32 7, i32 9}
!160 = !{i32 6, !"vProjectionAtlasOffsetScale", i32 3, i32 272, i32 7, i32 9}
!161 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 288, i32 7, i32 9}
!162 = !{i32 6, !"vDynamicShadowAtlasOffsetScale", i32 3, i32 304, i32 7, i32 9}
!163 = !{i32 6, !"uTechniqueProperties", i32 3, i32 320, i32 7, i32 5}
!164 = !{i32 6, !"padTo16bytes", i32 3, i32 324, i32 7, i32 5}
!165 = !{i32 48, !50}
!166 = !{i32 48, !113, !141, !167, !143, !168, !169, !170}
!167 = !{i32 6, !"vDirectionInView", i32 3, i32 16, i32 7, i32 9}
!168 = !{i32 6, !"fFarWidth", i32 3, i32 32, i32 7, i32 9}
!169 = !{i32 6, !"uTechniqueProperties", i32 3, i32 36, i32 7, i32 5}
!170 = !{i32 6, !"padTo16bytes", i32 3, i32 40, i32 7, i32 5}
!171 = !{i32 708, !50}
!172 = !{i32 708, !173, !174, !115, !175, !176, !177, !178, !179, !180, !181, !182, !183, !184, !185}
!173 = !{i32 6, !"vDirectionInView", i32 3, i32 0, i32 7, i32 9}
!174 = !{i32 6, !"pad1", i32 3, i32 12, i32 7, i32 4}
!175 = !{i32 6, !"pad2", i32 3, i32 28, i32 7, i32 4}
!176 = !{i32 6, !"mViewToProjectionClip", i32 2, !121, i32 3, i32 32, i32 7, i32 9}
!177 = !{i32 6, !"fProjectionMapTransparency", i32 3, i32 96, i32 7, i32 9}
!178 = !{i32 6, !"fRadius", i32 3, i32 100, i32 7, i32 9}
!179 = !{i32 6, !"uTechniqueProperties", i32 3, i32 104, i32 7, i32 5}
!180 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 108, i32 7, i32 9}
!181 = !{i32 6, !"uCascadeCount", i32 3, i32 112, i32 7, i32 5}
!182 = !{i32 6, !"mCascadeViewToShadowClip", i32 2, !121, i32 3, i32 128, i32 7, i32 9}
!183 = !{i32 6, !"vCascadeRange", i32 3, i32 512, i32 7, i32 9}
!184 = !{i32 6, !"fCascadePCFWidth", i32 3, i32 608, i32 7, i32 9}
!185 = !{i32 6, !"pad3", i32 3, i32 704, i32 7, i32 4}
!186 = !{i32 92, !50}
!187 = !{i32 92, !188, !189, !190, !191, !192, !193, !194, !195, !196}
!188 = !{i32 6, !"mWorldToLocalRotationMatrix", i32 2, !155, i32 3, i32 0, i32 7, i32 9}
!189 = !{i32 6, !"vOneOverTopLevelFadeLength", i32 3, i32 48, i32 7, i32 9}
!190 = !{i32 6, !"uTopLevelNodeIndicesOffset", i32 3, i32 60, i32 7, i32 5}
!191 = !{i32 6, !"uInternalNodeIndexOffset", i32 3, i32 64, i32 7, i32 5}
!192 = !{i32 6, !"uBrickIndexOffset", i32 3, i32 68, i32 7, i32 5}
!193 = !{i32 6, !"fTopLevelTexelSizeInWorld", i32 3, i32 72, i32 7, i32 9}
!194 = !{i32 6, !"fWeightMultiplier", i32 3, i32 76, i32 7, i32 9}
!195 = !{i32 6, !"uAdditive", i32 3, i32 80, i32 7, i32 5}
!196 = !{i32 6, !"fpadTo16Bytes", i32 3, i32 84, i32 7, i32 9}
!197 = !{i32 16, !198, !199, !200, !201}
!198 = !{i32 6, !"uChildMask", i32 3, i32 0, i32 7, i32 5}
!199 = !{i32 6, !"uLeafChildMask", i32 3, i32 4, i32 7, i32 5}
!200 = !{i32 6, !"uChildBlockOffset", i32 3, i32 8, i32 7, i32 5}
!201 = !{i32 6, !"uLeafChildBlockOffset", i32 3, i32 12, i32 7, i32 5}
!202 = !{i32 16, !33, !203}
!203 = !{i32 6, !"mips", i32 3, i32 12}
!204 = !{i32 32, !205, !206, !207, !208}
!205 = !{i32 6, !"vAABBMinInView", i32 3, i32 0, i32 7, i32 9}
!206 = !{i32 6, !"fPad0", i32 3, i32 12, i32 7, i32 9}
!207 = !{i32 6, !"vAABBMaxInView", i32 3, i32 16, i32 7, i32 9}
!208 = !{i32 6, !"fPad1", i32 3, i32 28, i32 7, i32 9}
!209 = !{i32 64, !210, !211, !212}
!210 = !{i32 6, !"mWorldToTopLevelMatrixDecomp", i32 3, i32 0, i32 7, i32 9}
!211 = !{i32 6, !"vTopLevelSize", i32 3, i32 48, i32 7, i32 5}
!212 = !{i32 6, !"fPad0", i32 3, i32 60, i32 7, i32 9}
!213 = !{i32 28, !214, !215}
!214 = !{i32 6, !"vResultDiffuse", i32 3, i32 0, i32 7, i32 9}
!215 = !{i32 6, !"vResultSpecular", i32 3, i32 16, i32 7, i32 9}
!216 = !{i32 36, !113, !217, !218, !219}
!217 = !{i32 6, !"vNormalInView", i32 3, i32 16, i32 7, i32 9}
!218 = !{i32 6, !"fSmoothness", i32 3, i32 28, i32 7, i32 9}
!219 = !{i32 6, !"fSpecularAlbedoIntensity", i32 3, i32 32, i32 7, i32 9}
!220 = !{i32 44, !214, !221, !222}
!221 = !{i32 6, !"vResultDiffuseTransmission", i32 3, i32 16, i32 7, i32 9}
!222 = !{i32 6, !"vResultSpecular", i32 3, i32 32, i32 7, i32 9}
!223 = !{i32 28, !224, !225}
!224 = !{i32 6, !"vIntensityDiffuse", i32 3, i32 0, i32 7, i32 9}
!225 = !{i32 6, !"vIntensitySpecular", i32 3, i32 16, i32 7, i32 9}
!226 = !{i32 32, !227}
!227 = !{i32 6, !"d", i32 3, i32 0}
!228 = !{i32 12, !113}
!229 = !{i32 68, !230, !231, !232, !233, !234, !235}
!230 = !{i32 6, !"vGBufferTarget1", i32 3, i32 0, i32 7, i32 9}
!231 = !{i32 6, !"vGBufferTarget2", i32 3, i32 16, i32 7, i32 9}
!232 = !{i32 6, !"vPositionInView", i32 3, i32 32, i32 7, i32 9}
!233 = !{i32 6, !"vBentNormalInView", i32 3, i32 48, i32 7, i32 9}
!234 = !{i32 6, !"fDiffuseOcclusion", i32 3, i32 60, i32 7, i32 9}
!235 = !{i32 6, !"fSpecularOcclusion", i32 3, i32 64, i32 7, i32 9}
!236 = !{i32 356, !237, !238, !239, !240, !241, !242, !243, !244, !245, !246, !247, !248, !249, !250, !251, !252, !253, !254, !255, !256, !257, !258, !259, !260, !261}
!237 = !{i32 6, !"m_vMin", i32 3, i32 0, i32 7, i32 9}
!238 = !{i32 6, !"m_fBaseVoxelSize", i32 3, i32 12, i32 7, i32 9}
!239 = !{i32 6, !"m_vMax", i32 3, i32 16, i32 7, i32 9}
!240 = !{i32 6, !"m_fInvBaseVoxelSize", i32 3, i32 28, i32 7, i32 9}
!241 = !{i32 6, !"m_uVoxelResolution", i32 3, i32 32, i32 7, i32 5}
!242 = !{i32 6, !"m_uLog2VoxelResolution", i32 3, i32 36, i32 7, i32 5}
!243 = !{i32 6, !"m_uBaseVoxelResolutionX", i32 3, i32 40, i32 7, i32 5}
!244 = !{i32 6, !"m_uBaseVoxelResolutionXY", i32 3, i32 44, i32 7, i32 5}
!245 = !{i32 6, !"m_iPrimaryTreeOffset", i32 3, i32 48, i32 7, i32 4}
!246 = !{i32 6, !"m_iDualTreeOffset", i32 3, i32 52, i32 7, i32 4}
!247 = !{i32 6, !"m_iIrradianceProbeOffset", i32 3, i32 56, i32 7, i32 4}
!248 = !{i32 6, !"m_iDirectTransportProbeOffset", i32 3, i32 60, i32 7, i32 4}
!249 = !{i32 6, !"m_iIndirectTransportProbeOffset", i32 3, i32 64, i32 7, i32 4}
!250 = !{i32 6, !"m_iLightGatherOffset", i32 3, i32 68, i32 7, i32 4}
!251 = !{i32 6, !"m_iLightProbeRefOffset", i32 3, i32 72, i32 7, i32 4}
!252 = !{i32 6, !"m_iDualLightProbeRefOffset", i32 3, i32 76, i32 7, i32 4}
!253 = !{i32 6, !"m_iEnvironmentMapRefOffset", i32 3, i32 80, i32 7, i32 4}
!254 = !{i32 6, !"m_iEnvironmentMapDualRefOffset", i32 3, i32 84, i32 7, i32 4}
!255 = !{i32 6, !"m_iEnvironmentMapInfoOffset", i32 3, i32 88, i32 7, i32 4}
!256 = !{i32 6, !"m_fWorldToGrid", i32 3, i32 92, i32 7, i32 9}
!257 = !{i32 6, !"m_fGridToWorld", i32 3, i32 96, i32 7, i32 9}
!258 = !{i32 6, !"pad0", i32 3, i32 100, i32 7, i32 9}
!259 = !{i32 6, !"pad1", i32 3, i32 104, i32 7, i32 9}
!260 = !{i32 6, !"pad2", i32 3, i32 108, i32 7, i32 9}
!261 = !{i32 6, !"m_iRowTranslationTable", i32 3, i32 112, i32 7, i32 4}
!262 = !{i32 4, !263}
!263 = !{i32 6, !"m_fDiffuseC0", i32 3, i32 0, i32 7, i32 9}
!264 = !{i32 4, !265}
!265 = !{i32 6, !"m_data", i32 3, i32 0, i32 7, i32 5}
!266 = !{i32 356, !50}
!267 = !{i32 28, !50}
!268 = !{i32 28, !269, !270, !271}
!269 = !{i32 6, !"m_childMask", i32 3, i32 0}
!270 = !{i32 6, !"m_pointer", i32 3, i32 20}
!271 = !{i32 6, !"m_payload", i32 3, i32 24, i32 7, i32 5}
!272 = !{i32 20, !273}
!273 = !{i32 6, !"m_uMask", i32 3, i32 0, i32 7, i32 5}
!274 = !{i32 4, !275}
!275 = !{i32 6, !"m_flagAndPointer", i32 3, i32 0, i32 7, i32 5}
!276 = !{i32 36, !50}
!277 = !{i32 36, !278, !279, !280, !281, !282, !283, !284, !285, !286}
!278 = !{i32 6, !"m_vR00", i32 3, i32 0, i32 7, i32 5}
!279 = !{i32 6, !"m_vR01", i32 3, i32 4, i32 7, i32 5}
!280 = !{i32 6, !"m_vR02", i32 3, i32 8, i32 7, i32 5}
!281 = !{i32 6, !"m_vR10", i32 3, i32 12, i32 7, i32 5}
!282 = !{i32 6, !"m_vR11", i32 3, i32 16, i32 7, i32 5}
!283 = !{i32 6, !"m_vR12", i32 3, i32 20, i32 7, i32 5}
!284 = !{i32 6, !"m_vR20", i32 3, i32 24, i32 7, i32 5}
!285 = !{i32 6, !"m_vR21", i32 3, i32 28, i32 7, i32 5}
!286 = !{i32 6, !"m_vR22", i32 3, i32 32, i32 7, i32 5}
!287 = !{i32 116, !50}
!288 = !{i32 116, !289}
!289 = !{i32 6, !"m_mTransport", i32 3, i32 0, i32 7, i32 5}
!290 = !{i32 8, !291, !292}
!291 = !{i32 6, !"m_index1", i32 3, i32 0, i32 7, i32 5}
!292 = !{i32 6, !"m_index2", i32 3, i32 4, i32 7, i32 5}
!293 = !{i32 220, !50}
!294 = !{i32 220, !295, !296, !297, !298, !299}
!295 = !{i32 6, !"m_vDebugColor", i32 3, i32 0, i32 7, i32 9}
!296 = !{i32 6, !"m_fRadius", i32 3, i32 12, i32 7, i32 9}
!297 = !{i32 6, !"m_vCenter", i32 3, i32 16, i32 7, i32 9}
!298 = !{i32 6, !"m_vMin", i32 3, i32 32, i32 7, i32 9}
!299 = !{i32 6, !"m_vMax", i32 3, i32 128, i32 7, i32 9}
!300 = !{i32 44, !301, !302, !303, !304, !305}
!301 = !{i32 6, !"m_vVoxelMin", i32 3, i32 0, i32 7, i32 9}
!302 = !{i32 6, !"m_fVoxelSize", i32 3, i32 12, i32 7, i32 9}
!303 = !{i32 6, !"m_uPayload", i32 3, i32 16, i32 7, i32 5}
!304 = !{i32 6, !"m_uLinearBaseVoxelIndex", i32 3, i32 20, i32 7, i32 5}
!305 = !{i32 6, !"m_vBaseVoxelMin", i32 3, i32 32, i32 7, i32 9}
!306 = !{i32 44, !307, !308, !309, !310, !311, !312, !313}
!307 = !{i32 6, !"m_iParentNode", i32 3, i32 0, i32 7, i32 4}
!308 = !{i32 6, !"m_vParentVoxelMin", i32 3, i32 4, i32 7, i32 9}
!309 = !{i32 6, !"m_vLeafVoxelMin", i32 3, i32 16, i32 7, i32 9}
!310 = !{i32 6, !"m_fLeafVoxelSize", i32 3, i32 28, i32 7, i32 9}
!311 = !{i32 6, !"m_bIsTerminal", i32 3, i32 32, i32 7, i32 1}
!312 = !{i32 6, !"m_bIsSolid", i32 3, i32 36, i32 7, i32 1}
!313 = !{i32 6, !"m_iPayload", i32 3, i32 40, i32 7, i32 4}
!314 = !{i32 16, !315, !316, !317}
!315 = !{i32 6, !"m_vUVOffset", i32 3, i32 0, i32 7, i32 9}
!316 = !{i32 6, !"m_uEnvMapIndex", i32 3, i32 8, i32 7, i32 5}
!317 = !{i32 6, !"m_uEnvInfoIndex", i32 3, i32 12, i32 7, i32 5}
!318 = !{i32 152, !50}
!319 = !{i32 152, !320, !321, !322, !323, !324, !325, !326}
!320 = !{i32 6, !"g_fPerLODDissolve", i32 3, i32 0, i32 7, i32 9}
!321 = !{i32 6, !"g_vWindIndex_Base_Trunk", i32 3, i32 116, i32 7, i32 5}
!322 = !{i32 6, !"g_uMask", i32 3, i32 124, i32 7, i32 5}
!323 = !{i32 6, !"g_iCharacterLightRigIndex", i32 3, i32 128, i32 7, i32 4}
!324 = !{i32 6, !"g_fDepthMultiply", i32 3, i32 132, i32 7, i32 9}
!325 = !{i32 6, !"g_fExtraDepthMultiply", i32 3, i32 136, i32 7, i32 9}
!326 = !{i32 6, !"padTo16Bytes", i32 3, i32 144, i32 7, i32 4}
!327 = !{i32 40, !328, !329, !330, !331, !332, !333, !334, !335, !336, !337}
!328 = !{i32 6, !"uIndexOffset", i32 3, i32 0, i32 7, i32 5}
!329 = !{i32 6, !"uIndexStride", i32 3, i32 4, i32 7, i32 5}
!330 = !{i32 6, !"uVertexStride1", i32 3, i32 8, i32 7, i32 5}
!331 = !{i32 6, !"uMaterialID", i32 3, i32 12, i32 7, i32 5}
!332 = !{i32 6, !"uTangentOffset", i32 3, i32 16, i32 7, i32 5}
!333 = !{i32 6, !"uNormalOffset", i32 3, i32 20, i32 7, i32 5}
!334 = !{i32 6, !"uTexcoordOffset", i32 3, i32 24, i32 7, i32 5}
!335 = !{i32 6, !"uColorOffset", i32 3, i32 28, i32 7, i32 5}
!336 = !{i32 6, !"uIndexBufferSize", i32 3, i32 32, i32 7, i32 5}
!337 = !{i32 6, !"uVertexBuffer1Size", i32 3, i32 36, i32 7, i32 5}
!338 = !{i32 8, !339}
!339 = !{i32 6, !"vIntersection", i32 3, i32 0, i32 4, !"INTERSECTION", i32 7, i32 9}
!340 = !{i32 4, !341}
!341 = !{i32 6, !"uRayIndex", i32 3, i32 0, i32 4, !"RAY_INDEX", i32 7, i32 5}
!342 = !{i32 32, !343, !344, !345, !346}
!343 = !{i32 6, !"Origin", i32 3, i32 0, i32 7, i32 9}
!344 = !{i32 6, !"TMin", i32 3, i32 12, i32 7, i32 9}
!345 = !{i32 6, !"Direction", i32 3, i32 16, i32 7, i32 9}
!346 = !{i32 6, !"TMax", i32 3, i32 28, i32 7, i32 9}
!347 = !{i32 88, !348, !349, !350, !351, !352, !353, !354, !355, !356, !357, !358, !359, !360, !361, !362, !363, !364, !365, !366, !367, !368, !369}
!348 = !{i32 6, !"g_fDebug_h_Slider0", i32 3, i32 0, i32 7, i32 9}
!349 = !{i32 6, !"g_fDebug_h_Slider1", i32 3, i32 4, i32 7, i32 9}
!350 = !{i32 6, !"g_fDebug_h_Slider2", i32 3, i32 8, i32 7, i32 9}
!351 = !{i32 6, !"g_fDebug_h_Slider3", i32 3, i32 12, i32 7, i32 9}
!352 = !{i32 6, !"g_fDebug_h_Slider4", i32 3, i32 16, i32 7, i32 9}
!353 = !{i32 6, !"g_fDebug_h_Slider5", i32 3, i32 20, i32 7, i32 9}
!354 = !{i32 6, !"g_fDebug_h_Slider6", i32 3, i32 24, i32 7, i32 9}
!355 = !{i32 6, !"g_fDebug_h_Slider7", i32 3, i32 28, i32 7, i32 9}
!356 = !{i32 6, !"g_fDebug_h_Knob0", i32 3, i32 32, i32 7, i32 9}
!357 = !{i32 6, !"g_fDebug_h_Knob1", i32 3, i32 36, i32 7, i32 9}
!358 = !{i32 6, !"g_fDebug_h_Knob2", i32 3, i32 40, i32 7, i32 9}
!359 = !{i32 6, !"g_fDebug_h_Knob3", i32 3, i32 44, i32 7, i32 9}
!360 = !{i32 6, !"g_fDebug_h_Knob4", i32 3, i32 48, i32 7, i32 9}
!361 = !{i32 6, !"g_fDebug_h_Knob5", i32 3, i32 52, i32 7, i32 9}
!362 = !{i32 6, !"g_fDebug_h_Knob6", i32 3, i32 56, i32 7, i32 9}
!363 = !{i32 6, !"g_fDebug_h_Knob7", i32 3, i32 60, i32 7, i32 9}
!364 = !{i32 6, !"g_fDebug_h_Toggle0", i32 3, i32 64, i32 7, i32 9}
!365 = !{i32 6, !"g_fDebug_h_Toggle1", i32 3, i32 68, i32 7, i32 9}
!366 = !{i32 6, !"g_fDebug_h_Toggle2", i32 3, i32 72, i32 7, i32 9}
!367 = !{i32 6, !"g_fDebug_h_Toggle3", i32 3, i32 76, i32 7, i32 9}
!368 = !{i32 6, !"g_fDebug_h_Toggle4", i32 3, i32 80, i32 7, i32 9}
!369 = !{i32 6, !"g_fDebug_h_Toggle5", i32 3, i32 84, i32 7, i32 9}
!370 = !{i32 1128, !371, !372, !373, !374, !375, !376, !377, !378, !379, !380, !381, !382, !383, !384, !385, !386, !387, !388, !389, !390, !391, !392, !393, !394, !395, !396, !397, !398, !399, !400, !401, !402}
!371 = !{i32 6, !"g_vScreenRes", i32 3, i32 0, i32 7, i32 9}
!372 = !{i32 6, !"g_vInvScreenRes", i32 3, i32 8, i32 7, i32 9}
!373 = !{i32 6, !"g_vOutputRes", i32 3, i32 16, i32 7, i32 9}
!374 = !{i32 6, !"g_vInvOutputRes", i32 3, i32 24, i32 7, i32 9}
!375 = !{i32 6, !"g_mWorldToView", i32 2, !121, i32 3, i32 32, i32 7, i32 9}
!376 = !{i32 6, !"g_mViewToWorld", i32 2, !121, i32 3, i32 96, i32 7, i32 9}
!377 = !{i32 6, !"g_mViewToClip", i32 2, !121, i32 3, i32 160, i32 7, i32 9}
!378 = !{i32 6, !"g_mClipToView", i32 2, !121, i32 3, i32 224, i32 7, i32 9}
!379 = !{i32 6, !"g_mWorldToClip", i32 2, !121, i32 3, i32 288, i32 7, i32 9}
!380 = !{i32 6, !"g_mClipToWorld", i32 2, !121, i32 3, i32 352, i32 7, i32 9}
!381 = !{i32 6, !"g_mClipToPreviousClip", i32 2, !121, i32 3, i32 416, i32 7, i32 9}
!382 = !{i32 6, !"g_mViewToPreviousClip", i32 2, !121, i32 3, i32 480, i32 7, i32 9}
!383 = !{i32 6, !"g_mPreviousViewToView", i32 2, !121, i32 3, i32 544, i32 7, i32 9}
!384 = !{i32 6, !"g_mPreviousWorldToClip", i32 2, !121, i32 3, i32 608, i32 7, i32 9}
!385 = !{i32 6, !"g_mPreviousViewToClip", i32 2, !121, i32 3, i32 672, i32 7, i32 9}
!386 = !{i32 6, !"g_mClipToPreviousClipNoJitter", i32 2, !121, i32 3, i32 736, i32 7, i32 9}
!387 = !{i32 6, !"g_mPreviousClipToClipNoJitter", i32 2, !121, i32 3, i32 800, i32 7, i32 9}
!388 = !{i32 6, !"g_vViewPoint", i32 3, i32 864, i32 7, i32 9}
!389 = !{i32 6, !"g_fInvNear", i32 3, i32 880, i32 7, i32 9}
!390 = !{i32 6, !"g_mViewToCameraView", i32 2, !121, i32 3, i32 896, i32 7, i32 9}
!391 = !{i32 6, !"g_mCameraViewToCameraClip", i32 2, !121, i32 3, i32 960, i32 7, i32 9}
!392 = !{i32 6, !"g_mCameraClipToView", i32 2, !121, i32 3, i32 1024, i32 7, i32 9}
!393 = !{i32 6, !"g_fWorldTime", i32 3, i32 1088, i32 7, i32 9}
!394 = !{i32 6, !"g_fWorldTimeDelta", i32 3, i32 1092, i32 7, i32 9}
!395 = !{i32 6, !"g_fRealTime", i32 3, i32 1096, i32 7, i32 9}
!396 = !{i32 6, !"g_fRealTimeDelta", i32 3, i32 1100, i32 7, i32 9}
!397 = !{i32 6, !"g_fLastValidWorldTimeDelta", i32 3, i32 1104, i32 7, i32 9}
!398 = !{i32 6, !"g_uTemporalFrame", i32 3, i32 1108, i32 7, i32 5}
!399 = !{i32 6, !"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5}
!400 = !{i32 6, !"g_bHDR", i32 3, i32 1116, i32 7, i32 1}
!401 = !{i32 6, !"g_fHDRSDRSceneBrightnessMultiplier", i32 3, i32 1120, i32 7, i32 9}
!402 = !{i32 6, !"g_fHDRSDRUIBrightnessMultiplier", i32 3, i32 1124, i32 7, i32 9}
!403 = !{i32 32, !404, !405, !406, !407}
!404 = !{i32 6, !"g_vShadowMapRes", i32 3, i32 0, i32 7, i32 9}
!405 = !{i32 6, !"g_vShadowMapVSMRes", i32 3, i32 8, i32 7, i32 9}
!406 = !{i32 6, !"g_vJitterOffset", i32 3, i32 16, i32 7, i32 9}
!407 = !{i32 6, !"g_vSnapOffset", i32 3, i32 24, i32 7, i32 4}
!408 = !{i32 4, !409}
!409 = !{i32 6, !"g_fOnePerTranslucencyKernelCount", i32 3, i32 0, i32 7, i32 9}
!410 = !{i32 408, !411, !412, !413, !414, !415, !416, !417, !418, !419, !420, !421, !422, !423, !424, !425, !426, !427, !428, !429, !430, !431, !432, !433, !434, !435, !436, !437, !438, !439, !440, !441, !442, !443, !444, !445}
!411 = !{i32 6, !"g_fTileDepthClipRanges", i32 3, i32 0, i32 7, i32 9}
!412 = !{i32 6, !"g_fTileDepthRanges", i32 3, i32 80, i32 7, i32 9}
!413 = !{i32 6, !"g_vDepthTileResolve", i32 3, i32 160, i32 7, i32 9}
!414 = !{i32 6, !"g_uDepthTileCount", i32 3, i32 168, i32 7, i32 5}
!415 = !{i32 6, !"g_vTileResolution", i32 3, i32 176, i32 7, i32 5}
!416 = !{i32 6, !"g_vTileWidthHeightDepth", i32 3, i32 192, i32 7, i32 5}
!417 = !{i32 6, !"g_vTileResolutionPerScreenResolution", i32 3, i32 208, i32 7, i32 9}
!418 = !{i32 6, !"g_vTileDepthNearFar", i32 3, i32 216, i32 7, i32 9}
!419 = !{i32 6, !"g_uMaxPointLightsPerTile", i32 3, i32 224, i32 7, i32 5}
!420 = !{i32 6, !"g_uMaxSpotLightsPerTile", i32 3, i32 228, i32 7, i32 5}
!421 = !{i32 6, !"g_uAmbientLightTotalCount", i32 3, i32 232, i32 7, i32 5}
!422 = !{i32 6, !"g_fAmbientEnvDiffuseIntensity", i32 3, i32 236, i32 7, i32 9}
!423 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnTransparent", i32 3, i32 240, i32 7, i32 9}
!424 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnOpaque", i32 3, i32 244, i32 7, i32 9}
!425 = !{i32 6, !"g_fAmbientSkyIntensity", i32 3, i32 248, i32 7, i32 9}
!426 = !{i32 6, !"g_fAmbientLocalIntensity", i32 3, i32 252, i32 7, i32 9}
!427 = !{i32 6, !"g_uPointLightTotalCount", i32 3, i32 256, i32 7, i32 5}
!428 = !{i32 6, !"g_uSpotLightTotalCount", i32 3, i32 260, i32 7, i32 5}
!429 = !{i32 6, !"g_uSunLightTotalCount", i32 3, i32 264, i32 7, i32 5}
!430 = !{i32 6, !"g_uAmbientLightEnabled", i32 3, i32 268, i32 7, i32 5}
!431 = !{i32 6, !"g_fAmbientDepthRampDistance", i32 3, i32 272, i32 7, i32 9}
!432 = !{i32 6, !"g_fAmbientDepthRampFeather", i32 3, i32 276, i32 7, i32 9}
!433 = !{i32 6, !"g_fAmbientDepthRampIntensityNear", i32 3, i32 280, i32 7, i32 9}
!434 = !{i32 6, !"g_fAmbientDepthRampIntensityFar", i32 3, i32 284, i32 7, i32 9}
!435 = !{i32 6, !"g_vVolumeLightDimensions", i32 3, i32 288, i32 7, i32 5}
!436 = !{i32 6, !"g_vVolumeLightProjectionConstants", i32 3, i32 304, i32 7, i32 9}
!437 = !{i32 6, !"g_vHalfResVolumeLightProjectionConstants", i32 3, i32 320, i32 7, i32 9}
!438 = !{i32 6, !"g_vOnePerVolumeLightDimensions", i32 3, i32 336, i32 7, i32 9}
!439 = !{i32 6, !"g_vVolumeLightXYToTileXY", i32 3, i32 352, i32 7, i32 9}
!440 = !{i32 6, !"g_vVolumeLightDepthResolve", i32 3, i32 368, i32 7, i32 9}
!441 = !{i32 6, !"g_fVolumeLightOnePerDepthMinusOne", i32 3, i32 380, i32 7, i32 9}
!442 = !{i32 6, !"g_vVolumeLightNearSplit0Far", i32 3, i32 384, i32 7, i32 9}
!443 = !{i32 6, !"g_fVolumeLightKernelWidth", i32 3, i32 396, i32 7, i32 9}
!444 = !{i32 6, !"g_fVolumeLightSamplingBias", i32 3, i32 400, i32 7, i32 9}
!445 = !{i32 6, !"g_uRTContactShadowRayCount", i32 3, i32 404, i32 7, i32 5}
!446 = !{i32 8, !447, !448}
!447 = !{i32 6, !"g_fEnvReflectionEdgeLength", i32 3, i32 0, i32 7, i32 9}
!448 = !{i32 6, !"g_fEnvReflectionMipCount", i32 3, i32 4, i32 7, i32 9}
!449 = !{i32 188, !450, !451, !452, !453, !454, !455, !456, !457, !458, !459, !460, !461, !462, !463, !464, !465, !466, !467, !468, !469}
!450 = !{i32 6, !"g_atmosphere_vSunDir", i32 3, i32 0, i32 7, i32 9}
!451 = !{i32 6, !"g_atmosphere_vSunE", i32 3, i32 16, i32 7, i32 9}
!452 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants1", i32 3, i32 32, i32 7, i32 9}
!453 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants2", i32 3, i32 40, i32 7, i32 9}
!454 = !{i32 6, !"g_atmosphere_vPhaseBlend", i32 3, i32 48, i32 7, i32 9}
!455 = !{i32 6, !"g_atmosphere_vPhaseG", i32 3, i32 52, i32 7, i32 9}
!456 = !{i32 6, !"g_atmosphere_vFog", i32 3, i32 64, i32 7, i32 9}
!457 = !{i32 6, !"g_vFogColor", i32 3, i32 80, i32 7, i32 9}
!458 = !{i32 6, !"g_vFogColorOpposite", i32 3, i32 96, i32 7, i32 9}
!459 = !{i32 6, !"g_fFogExp", i32 3, i32 108, i32 7, i32 9}
!460 = !{i32 6, !"g_fFogGroundDensityAtViewer", i32 3, i32 112, i32 7, i32 9}
!461 = !{i32 6, !"g_fFogGroundHeight", i32 3, i32 116, i32 7, i32 9}
!462 = !{i32 6, !"g_fFogGroundFalloff", i32 3, i32 120, i32 7, i32 9}
!463 = !{i32 6, !"g_fFogGroundDensity", i32 3, i32 124, i32 7, i32 9}
!464 = !{i32 6, !"g_vFogGroundDensityMapRange", i32 3, i32 128, i32 7, i32 9}
!465 = !{i32 6, !"g_vFogGroundSimulationVelocityAndScale", i32 3, i32 144, i32 7, i32 9}
!466 = !{i32 6, !"g_fFogDepthRampDistance", i32 3, i32 156, i32 7, i32 9}
!467 = !{i32 6, !"g_fFogDepthRampFeather", i32 3, i32 160, i32 7, i32 9}
!468 = !{i32 6, !"g_fFogDepthRampIntensityNear", i32 3, i32 164, i32 7, i32 9}
!469 = !{i32 6, !"g_vFogAlbedo", i32 3, i32 176, i32 7, i32 9}
!470 = !{i32 40, !471, !472, !473, !474, !475, !476}
!471 = !{i32 6, !"g_vOnePerVolumeAtlasSize", i32 3, i32 0, i32 7, i32 9}
!472 = !{i32 6, !"g_uActiveVolumeCount", i32 3, i32 12, i32 7, i32 5}
!473 = !{i32 6, !"g_vAdditiveAmbient", i32 3, i32 16, i32 7, i32 9}
!474 = !{i32 6, !"g_uActiveDebugVolumeCount", i32 3, i32 28, i32 7, i32 5}
!475 = !{i32 6, !"g_fTransparentBalanceEnergy", i32 3, i32 32, i32 7, i32 9}
!476 = !{i32 6, !"g_fTransparentBalanceEnergyStrength", i32 3, i32 36, i32 7, i32 9}
!477 = !{i32 24576, !478}
!478 = !{i32 6, !"g_cbActiveVolumeTopLevelInfoArray", i32 3, i32 0}
!479 = !{i32 24576, !480}
!480 = !{i32 6, !"g_cbDebugVolumeTopLevelInfoArray", i32 3, i32 0}
!481 = !{i32 12, !482, !483, !484}
!482 = !{i32 6, !"g_fWorldSize", i32 3, i32 0, i32 7, i32 9}
!483 = !{i32 6, !"g_fCellSize", i32 3, i32 4, i32 7, i32 9}
!484 = !{i32 6, !"g_iCellCount", i32 3, i32 8, i32 7, i32 4}
!485 = !{i32 24, !486, !487, !488, !489, !490}
!486 = !{i32 6, !"g_fInvEnvironmentMapsPerRow", i32 3, i32 0, i32 7, i32 9}
!487 = !{i32 6, !"g_fEnvironmentMapsPerRow", i32 3, i32 4, i32 7, i32 9}
!488 = !{i32 6, !"g_fEnvironmentMapColSize", i32 3, i32 8, i32 7, i32 9}
!489 = !{i32 6, !"g_fEnvironmentMapRowSize", i32 3, i32 12, i32 7, i32 9}
!490 = !{i32 6, !"g_fInvEnvironmentMapAtlasSize", i32 3, i32 16, i32 7, i32 9}
!491 = !{i32 48, !492, !493, !494}
!492 = !{i32 6, !"g_uStride0_uStride1_uOffset0_uOffset1", i32 3, i32 0, i32 7, i32 5}
!493 = !{i32 6, !"g_Tangent_Normal_Texcoord_Color", i32 3, i32 16, i32 7, i32 5}
!494 = !{i32 6, !"g_BIndices_BWeights", i32 3, i32 32, i32 7, i32 5}
!495 = !{i32 40, !496}
!496 = !{i32 6, !"g_cbRaytracingHit", i32 3, i32 0}
!497 = !{i32 24, !498, !499, !500, !501, !502, !503}
!498 = !{i32 6, !"g_fClampReflectionIntensity", i32 3, i32 0, i32 7, i32 9}
!499 = !{i32 6, !"g_fRTReflectionAddRays", i32 3, i32 4, i32 7, i32 9}
!500 = !{i32 6, !"g_uRTReflectionRayCount", i32 3, i32 8, i32 7, i32 5}
!501 = !{i32 6, !"g_fRTZeroRayReflectance", i32 3, i32 12, i32 7, i32 9}
!502 = !{i32 6, !"g_fRTMaxRayReflectance", i32 3, i32 16, i32 7, i32 9}
!503 = !{i32 6, !"g_fRTMaxRayRoughness", i32 3, i32 20, i32 7, i32 9}
!504 = !{i32 1, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !505, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !505, void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !512, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !505, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !505, void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !512, void ()* @"\01?reflectionRayGeneration@@YAXXZ", !513}
!505 = !{!506, !508, !510}
!506 = !{i32 1, !507, !507}
!507 = !{}
!508 = !{i32 2, !509, !507}
!509 = !{i32 4, !"SV_RayPayload"}
!510 = !{i32 0, !511, !507}
!511 = !{i32 4, !"SV_IntersectionAttributes"}
!512 = !{!506, !508}
!513 = !{!506}
!514 = !{null, !"", null, !3, !515}
!515 = !{i32 0, i64 65808, i32 5, !516}
!516 = !{i32 0}
!517 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !518}
!518 = !{i32 8, i32 9, i32 6, i32 4, i32 7, i32 8, i32 5, !516}
!519 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !520}
!520 = !{i32 8, i32 10, i32 6, i32 4, i32 7, i32 8, i32 5, !516}
!521 = !{void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !"\01?reflectionMiss@@YAXUHitData@@@Z", null, null, !522}
!522 = !{i32 8, i32 11, i32 6, i32 4, i32 5, !516}
!523 = !{void ()* @"\01?reflectionRayGeneration@@YAXXZ", !"\01?reflectionRayGeneration@@YAXXZ", null, null, !524}
!524 = !{i32 8, i32 7, i32 5, !516}
!525 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !518}
!526 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !520}
!527 = !{void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !"\01?shadowMiss@@YAXUHitData@@@Z", null, null, !522}
!528 = !{!529, !529, i64 0}
!529 = !{!"omnipotent char", !530, i64 0}
!530 = !{!"Simple C/C++ TBAA"}
!531 = !{!532, !534}
!532 = distinct !{!532, !533, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z: %agg.result"}
!533 = distinct !{!533, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z"}
!534 = distinct !{!534, !535, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z: %agg.result"}
!535 = distinct !{!535, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z"}
!536 = !{!537, !537, i64 0}
!537 = !{!"int", !529, i64 0}
!538 = !{!539, !541}
!539 = distinct !{!539, !540, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z: %agg.result"}
!540 = distinct !{!540, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z"}
!541 = distinct !{!541, !542, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z: %agg.result"}
!542 = distinct !{!542, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z"}
!543 = !{!544, !544, i64 0}
!544 = !{!"float", !529, i64 0}

