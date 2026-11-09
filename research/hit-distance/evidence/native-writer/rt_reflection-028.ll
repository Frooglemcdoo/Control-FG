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
; g_sLinearClamp                    sampler      NA          NA      S1      s6,space1     1
; g_tGBuffer1                       texture     f32          2d      T0             t0     1
; g_tGBuffer2                       texture     f32          2d      T1             t1     1
; g_tLinearDepth                    texture     f32          2d      T2             t2     1
; g_tClipDepth                      texture     f32          2d      T3             t3     1
; g_sbMaterialDataPart1             texture  struct         r/o      T4             t4     1
; g_sbMaterialDataPart3             texture  struct         r/o      T5             t5     1
; g_sMaterialTextureArray           texture     f32          2d      T6      t0,space1unbounded
; g_tEnvBRDF                        texture     f32          2d      T7             t6     1
; g_bRaytracingIndexBuffer          texture    byte         r/o      T8      t0,space3     1
; g_bRaytracingVertexBuffer1        texture    byte         r/o      T9      t1,space3     1
; g_tStaticBlueNoiseRGBA_0          texture     f32          2d     T10             t7     1
; g_rtScene                         texture     i32         ras     T11             t8     1
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
%"class.StructuredBuffer<MaterialDataPart1>" = type { %struct.MaterialDataPart1 }
%struct.MaterialDataPart1 = type { i32, i32 }
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
@"\01?g_tGBuffer2@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
@"\01?g_sLinearWrap@@3USamplerState@@A" = external constant %struct.SamplerState, align 4
@"\01?g_sLinearClamp@@3USamplerState@@A" = external constant %struct.SamplerState, align 4
@"\01?g_sbMaterialDataPart1@@3V?$StructuredBuffer@UMaterialDataPart1@@@@A" = external constant %"class.StructuredBuffer<MaterialDataPart1>", align 4
@"\01?g_sbMaterialDataPart3@@3V?$StructuredBuffer@UMaterialDataPart3@@@@A" = external constant %"class.StructuredBuffer<MaterialDataPart3>", align 4
@"\01?g_sMaterialTextureArray@@3PAV?$Texture2D@V?$vector@M$03@@@@A" = external constant [0 x %"class.Texture2D<vector<float, 4> >"], align 4
@"\01?g_tEnvBRDF@@3V?$Texture2D@V?$vector@M$03@@@@A" = external constant %"class.Texture2D<vector<float, 4> >", align 4
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
  %9 = load <2 x float>, <2 x float>* %8, align 4, !tbaa !533
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
  %70 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* %69, align 4, !noalias !536
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
  %10 = load <2 x float>, <2 x float>* %9, align 4, !tbaa !533
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
  %127 = load i32, i32* %126, align 4, !tbaa !541
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
  store i32 %133, i32* %126, align 4, !tbaa !541
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
  %27 = load i32, i32* %26, align 4, !tbaa !541
  %28 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<unsigned int>"(i32 160, %"class.RWTexture2DArray<unsigned int>" %2)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %28, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex16, i32 %27, i32 65535, i32 65535, i32 65535, i32 65535, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %29 = call %dx.types.Handle @"dx.op.createHandleForLib.class.RWTexture2DArray<vector<float, 4> >"(i32 160, %"class.RWTexture2DArray<vector<float, 4> >" %1)  ; CreateHandleForLib(Resource)
  call void @dx.op.textureStore.f32(i32 67, %dx.types.Handle %29, i32 %DispatchRaysIndex, i32 %DispatchRaysIndex16, i32 %27, float %21, float %23, float %25, float 0.000000e+00, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  store i32 0, i32* %26, align 4, !tbaa !541
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
  %9 = load <2 x float>, <2 x float>* %8, align 4, !tbaa !533
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
  %70 = load %"class.Texture2D<vector<float, 4> >", %"class.Texture2D<vector<float, 4> >"* %69, align 4, !noalias !543
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
  %FMad118 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %32, float %46)  ; FMad(a,b,c)
  %FMad117 = call float @dx.op.tertiary.f32(i32 46, float %23, float %37, float %FMad118)  ; FMad(a,b,c)
  %47 = fadd fast float %FMad117, %42
  %48 = fmul fast float %28, %.i0172
  %FMad115 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %33, float %48)  ; FMad(a,b,c)
  %FMad114 = call float @dx.op.tertiary.f32(i32 46, float %23, float %38, float %FMad115)  ; FMad(a,b,c)
  %49 = fadd fast float %FMad114, %43
  %50 = fmul fast float %29, %.i0172
  %FMad112 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %34, float %50)  ; FMad(a,b,c)
  %FMad111 = call float @dx.op.tertiary.f32(i32 46, float %23, float %39, float %FMad112)  ; FMad(a,b,c)
  %51 = fadd fast float %FMad111, %44
  %52 = fmul fast float %30, %.i0172
  %FMad109 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %35, float %52)  ; FMad(a,b,c)
  %FMad108 = call float @dx.op.tertiary.f32(i32 46, float %23, float %40, float %FMad109)  ; FMad(a,b,c)
  %53 = fadd fast float %FMad108, %45
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
  %FMad154 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %66, float %80)  ; FMad(a,b,c)
  %FMad153 = call float @dx.op.tertiary.f32(i32 46, float %23, float %71, float %FMad154)  ; FMad(a,b,c)
  %81 = fadd fast float %FMad153, %76
  %82 = fmul fast float %62, %.i0172
  %FMad151 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %67, float %82)  ; FMad(a,b,c)
  %FMad150 = call float @dx.op.tertiary.f32(i32 46, float %23, float %72, float %FMad151)  ; FMad(a,b,c)
  %83 = fadd fast float %FMad150, %77
  %84 = fmul fast float %63, %.i0172
  %FMad148 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %68, float %84)  ; FMad(a,b,c)
  %FMad147 = call float @dx.op.tertiary.f32(i32 46, float %23, float %73, float %FMad148)  ; FMad(a,b,c)
  %85 = fadd fast float %FMad147, %78
  %86 = fmul fast float %64, %.i0172
  %FMad145 = call float @dx.op.tertiary.f32(i32 46, float %.i1173487, float %69, float %86)  ; FMad(a,b,c)
  %FMad144 = call float @dx.op.tertiary.f32(i32 46, float %23, float %74, float %FMad145)  ; FMad(a,b,c)
  %87 = fadd fast float %FMad144, %79
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
  %FMad106 = call float @dx.op.tertiary.f32(i32 46, float %.i1223, float %129, float %136)  ; FMad(a,b,c)
  %FMad105 = call float @dx.op.tertiary.f32(i32 46, float %.i2224, float %133, float %FMad106)  ; FMad(a,b,c)
  %137 = fmul fast float %126, %.i0222
  %FMad104 = call float @dx.op.tertiary.f32(i32 46, float %.i1223, float %130, float %137)  ; FMad(a,b,c)
  %FMad103 = call float @dx.op.tertiary.f32(i32 46, float %.i2224, float %134, float %FMad104)  ; FMad(a,b,c)
  %138 = fmul fast float %127, %.i0222
  %FMad102 = call float @dx.op.tertiary.f32(i32 46, float %.i1223, float %131, float %138)  ; FMad(a,b,c)
  %FMad101 = call float @dx.op.tertiary.f32(i32 46, float %.i2224, float %135, float %FMad102)  ; FMad(a,b,c)
  %.i0225 = fmul fast float %25, 0x3F50624DE0000000
  %.i0228 = fmul fast float %.i0225, %.i0189
  %.i1229 = fmul fast float %.i0225, %.i1190
  %.i2230 = fmul fast float %.i0225, %.i2191
  %.i0231 = fsub fast float %.i0183, %.i0228
  %.i1232 = fsub fast float %.i1184, %.i1229
  %.i2233 = fsub fast float %.i2185, %.i2230
  %.i0237 = fmul fast float %.i0225, %FMad105
  %.i1238 = fmul fast float %.i0225, %FMad103
  %.i2239 = fmul fast float %.i0225, %FMad101
  %.i0240 = fadd fast float %.i0231, %.i0237
  %.i1241 = fadd fast float %.i1232, %.i1238
  %.i2242 = fadd fast float %.i2233, %.i2239
  %.upto0425 = insertelement <3 x float> undef, float %.i0240, i32 0
  %.upto1426 = insertelement <3 x float> %.upto0425, float %.i1241, i32 1
  %139 = insertelement <3 x float> %.upto1426, float %.i2242, i32 2
  %140 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 0
  store <3 x float> %139, <3 x float>* %140, align 4, !tbaa !533
  %141 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 1
  store float 0.000000e+00, float* %141, align 4, !tbaa !548
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
  %169 = call float @dx.op.dot3.f32(i32 55, float %FMad105, float %FMad103, float %FMad101, float %.i0261, float %.i1262, float %.i2263)  ; Dot3(ax,ay,az,bx,by,bz)
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
  %.i0277 = fmul fast float %FMad105, 4.000000e+00
  %.i1278 = fmul fast float %FMad103, 4.000000e+00
  %.i2279 = fmul fast float %FMad101, 4.000000e+00
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
  %FMad96 = call float @dx.op.tertiary.f32(i32 46, float %23, float %195, float 0.000000e+00)  ; FMad(a,b,c)
  %229 = fadd fast float %FMad96, %198
  %FMad90 = call float @dx.op.tertiary.f32(i32 46, float %23, float %196, float 0.000000e+00)  ; FMad(a,b,c)
  %230 = fadd fast float %FMad90, %199
  %231 = fdiv fast float 1.000000e+00, %230
  %.i1284 = fmul fast float %231, %229
  %232 = fdiv fast float 2.000000e+00, %218
  %FMad139 = call float @dx.op.tertiary.f32(i32 46, float %232, float %192, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad138 = call float @dx.op.tertiary.f32(i32 46, float %23, float %195, float %FMad139)  ; FMad(a,b,c)
  %233 = fadd fast float %FMad138, %198
  %FMad133 = call float @dx.op.tertiary.f32(i32 46, float %232, float %193, float 0.000000e+00)  ; FMad(a,b,c)
  %FMad132 = call float @dx.op.tertiary.f32(i32 46, float %23, float %196, float %FMad133)  ; FMad(a,b,c)
  %234 = fadd fast float %FMad132, %199
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
  store i32 0, i32* %241, align 4, !tbaa !541
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
  %269 = load float, float* %219, align 4, !tbaa !548
  %270 = fsub fast float %269, %.i0298
  %271 = fcmp fast ogt float %270, %268
  %272 = fadd fast float %.i0295, 5.000000e-01
  %273 = fmul fast float %Exp, %272
  %274 = select i1 %271, float %273, float %.i0298
  %275 = load float, float* %220, align 4, !tbaa !548
  %276 = fsub fast float %275, %.i1299
  %277 = fcmp fast ogt float %276, %268
  %278 = fadd fast float %.i1296, 5.000000e-01
  %279 = fmul fast float %Exp, %278
  %280 = select i1 %277, float %279, float %.i1299
  %281 = load float, float* %221, align 4, !tbaa !548
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
  store float 0.000000e+00, float* %290, align 4, !tbaa !548
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
  %FMad130 = call float @dx.op.tertiary.f32(i32 46, float %.i1383, float %205, float %311)  ; FMad(a,b,c)
  %FMad129 = call float @dx.op.tertiary.f32(i32 46, float %.i2384, float %209, float %FMad130)  ; FMad(a,b,c)
  %312 = fadd fast float %FMad129, %213
  %313 = fmul fast float %.i0382, %202
  %FMad127 = call float @dx.op.tertiary.f32(i32 46, float %.i1383, float %206, float %313)  ; FMad(a,b,c)
  %FMad126 = call float @dx.op.tertiary.f32(i32 46, float %.i2384, float %210, float %FMad127)  ; FMad(a,b,c)
  %314 = fadd fast float %FMad126, %214
  %315 = fmul fast float %.i0382, %203
  %FMad121 = call float @dx.op.tertiary.f32(i32 46, float %.i1383, float %207, float %315)  ; FMad(a,b,c)
  %FMad120 = call float @dx.op.tertiary.f32(i32 46, float %.i2384, float %211, float %FMad121)  ; FMad(a,b,c)
  %316 = fadd fast float %FMad120, %215
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
  %FMad88 = call float @dx.op.tertiary.f32(i32 46, float %.i1338, float %330, float %337)  ; FMad(a,b,c)
  %FMad87 = call float @dx.op.tertiary.f32(i32 46, float %.i2339, float %334, float %FMad88)  ; FMad(a,b,c)
  %338 = fmul fast float %327, %.i0337
  %FMad86 = call float @dx.op.tertiary.f32(i32 46, float %.i1338, float %331, float %338)  ; FMad(a,b,c)
  %FMad85 = call float @dx.op.tertiary.f32(i32 46, float %.i2339, float %335, float %FMad86)  ; FMad(a,b,c)
  %339 = fmul fast float %328, %.i0337
  %FMad84 = call float @dx.op.tertiary.f32(i32 46, float %.i1338, float %332, float %339)  ; FMad(a,b,c)
  %FMad = call float @dx.op.tertiary.f32(i32 46, float %.i2339, float %336, float %FMad84)  ; FMad(a,b,c)
  %340 = fadd fast float %Frc71, 5.000000e-01
  %341 = fmul fast float %340, 0x4066666660000000
  %342 = fadd fast float %341, 0x4066666660000000
  store i32 %reflectionRayIndex.030, i32* %350, align 8
  %343 = call %dx.types.Handle @dx.op.createHandleForLib.struct.RaytracingAccelerationStructure(i32 160, %struct.RaytracingAccelerationStructure %5)  ; CreateHandleForLib(Resource)
  call void @dx.op.traceRay.struct.HitData(i32 157, %dx.types.Handle %343, i32 0, i32 1, i32 0, i32 2, i32 0, float %352, float %353, float %354, float %355, float %FMad87, float %FMad85, float %FMad, float %342, %struct.HitData* nonnull %16)  ; TraceRay(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToShaderIndex,MissShaderIndex,Origin_X,Origin_Y,Origin_Z,TMin,Direction_X,Direction_Y,Direction_Z,TMax,payload)
  %344 = add nuw nsw i32 %reflectionRayIndex.030, 1
  %exitcond = icmp eq i32 %344, %IMin
  br i1 %exitcond, label %._crit_edge.32.loopexit, label %.lr.ph31

._crit_edge.32.loopexit:                          ; preds = %._crit_edge
  %.lcssa = phi float [ %342, %._crit_edge ]
  %FMad.lcssa = phi float [ %FMad, %._crit_edge ]
  %FMad85.lcssa = phi float [ %FMad85, %._crit_edge ]
  %FMad87.lcssa = phi float [ %FMad87, %._crit_edge ]
  %345 = getelementptr inbounds %struct.RayDesc, %struct.RayDesc* %ray, i32 0, i32 3
  %346 = insertelement <3 x float> undef, float %FMad87.lcssa, i64 0
  %347 = insertelement <3 x float> %346, float %FMad85.lcssa, i64 1
  %348 = insertelement <3 x float> %347, float %FMad.lcssa, i64 2
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

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32(i32, %dx.types.Handle, i32, i32, i8, i32) #1

; Function Attrs: nounwind
declare i32 @dx.op.atomicBinOp.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32) #3

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
declare %dx.types.Handle @"dx.op.createHandleForLib.class.StructuredBuffer<MaterialDataPart1>"(i32, %"class.StructuredBuffer<MaterialDataPart1>") #1

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
!dx.typeAnnotations = !{!36, !509}
!dx.entryPoints = !{!519, !522, !524, !526, !528, !530, !531, !532}

!0 = !{!"dxc 1.2"}
!1 = !{i32 1, i32 3}
!2 = !{!"lib", i32 6, i32 3}
!3 = !{!4, !21, !29, !33}
!4 = !{!5, !7, !8, !9, !10, !12, !14, !15, !16, !17, !18, !19}
!5 = !{i32 0, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer1@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer1", i32 0, i32 0, i32 1, i32 2, i32 0, !6}
!6 = !{i32 0, i32 9}
!7 = !{i32 1, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tGBuffer2@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tGBuffer2", i32 0, i32 1, i32 1, i32 2, i32 0, !6}
!8 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tLinearDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tLinearDepth", i32 0, i32 2, i32 1, i32 2, i32 0, !6}
!9 = !{i32 3, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tClipDepth@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tClipDepth", i32 0, i32 3, i32 1, i32 2, i32 0, !6}
!10 = !{i32 4, %"class.StructuredBuffer<MaterialDataPart1>"* @"\01?g_sbMaterialDataPart1@@3V?$StructuredBuffer@UMaterialDataPart1@@@@A", !"g_sbMaterialDataPart1", i32 0, i32 4, i32 1, i32 12, i32 0, !11}
!11 = !{i32 1, i32 8}
!12 = !{i32 5, %"class.StructuredBuffer<MaterialDataPart3>"* @"\01?g_sbMaterialDataPart3@@3V?$StructuredBuffer@UMaterialDataPart3@@@@A", !"g_sbMaterialDataPart3", i32 0, i32 5, i32 1, i32 12, i32 0, !13}
!13 = !{i32 1, i32 336}
!14 = !{i32 6, [0 x %"class.Texture2D<vector<float, 4> >"]* @"\01?g_sMaterialTextureArray@@3PAV?$Texture2D@V?$vector@M$03@@@@A", !"g_sMaterialTextureArray", i32 1, i32 0, i32 -1, i32 2, i32 0, !6}
!15 = !{i32 7, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tEnvBRDF@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tEnvBRDF", i32 0, i32 6, i32 1, i32 2, i32 0, !6}
!16 = !{i32 8, %struct.ByteAddressBuffer* @"\01?g_bRaytracingIndexBuffer@@3UByteAddressBuffer@@A", !"g_bRaytracingIndexBuffer", i32 3, i32 0, i32 1, i32 11, i32 0, null}
!17 = !{i32 9, %struct.ByteAddressBuffer* @"\01?g_bRaytracingVertexBuffer1@@3UByteAddressBuffer@@A", !"g_bRaytracingVertexBuffer1", i32 3, i32 1, i32 1, i32 11, i32 0, null}
!18 = !{i32 10, %"class.Texture2D<vector<float, 4> >"* @"\01?g_tStaticBlueNoiseRGBA_0@@3V?$Texture2D@V?$vector@M$03@@@@A", !"g_tStaticBlueNoiseRGBA_0", i32 0, i32 7, i32 1, i32 2, i32 0, !6}
!19 = !{i32 11, %struct.RaytracingAccelerationStructure* @"\01?g_rtScene@@3URaytracingAccelerationStructure@@A", !"g_rtScene", i32 0, i32 8, i32 1, i32 16, i32 0, !20}
!20 = !{i32 0, i32 4}
!21 = !{!22, !24, !25, !26, !28}
!22 = !{i32 0, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtMaterialId@@3V?$RWTexture2DArray@I@@A", !"g_rwtMaterialId", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !23}
!23 = !{i32 0, i32 5}
!24 = !{i32 1, %"class.RWTexture2DArray<vector<float, 4> >"* @"\01?g_rwtNormal_TexcoordX@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A", !"g_rwtNormal_TexcoordX", i32 0, i32 1, i32 1, i32 7, i1 false, i1 false, i1 false, !6}
!25 = !{i32 2, %"class.RWTexture2DArray<vector<float, 4> >"* @"\01?g_rwtPosition_TexcoordY@@3V?$RWTexture2DArray@V?$vector@M$03@@@@A", !"g_rwtPosition_TexcoordY", i32 0, i32 2, i32 1, i32 7, i1 false, i1 false, i1 false, !6}
!26 = !{i32 3, %"class.RWStructuredBuffer<unsigned int>"* @"\01?g_rwbPerMaterialCount@@3V?$RWStructuredBuffer@I@@A", !"g_rwbPerMaterialCount", i32 0, i32 3, i32 1, i32 12, i1 false, i1 false, i1 false, !27}
!27 = !{i32 1, i32 4}
!28 = !{i32 4, %"class.RWTexture2DArray<unsigned int>"* @"\01?g_rwtShadow@@3V?$RWTexture2DArray@I@@A", !"g_rwtShadow", i32 0, i32 4, i32 1, i32 7, i1 false, i1 false, i1 false, !23}
!29 = !{!30, !31, !32}
!30 = !{i32 0, %sys_constants* @sys_constants, !"sys_constants", i32 0, i32 0, i32 1, i32 1128, null}
!31 = !{i32 1, %g_cbRaytracingHit* @g_cbRaytracingHit, !"g_cbRaytracingHit", i32 3, i32 0, i32 1, i32 40, null}
!32 = !{i32 2, %rtreflection* @rtreflection, !"rtreflection", i32 0, i32 1, i32 1, i32 24, null}
!33 = !{!34, !35}
!34 = !{i32 0, %struct.SamplerState* @"\01?g_sLinearWrap@@3USamplerState@@A", !"g_sLinearWrap", i32 1, i32 5, i32 1, i32 0, null}
!35 = !{i32 1, %struct.SamplerState* @"\01?g_sLinearClamp@@3USamplerState@@A", !"g_sLinearClamp", i32 1, i32 6, i32 1, i32 0, null}
!36 = !{i32 0, %"class.Texture2D<vector<float, 4> >" undef, !37, %"class.Texture2D<vector<float, 4> >::mips_type" undef, !40, %struct.GBufferFormat undef, !42, %struct.SamplerState undef, !45, %"class.Texture2D<float>" undef, !47, %"class.Texture2D<float>::mips_type" undef, !40, %"class.Texture2D<unsigned int>" undef, !49, %"class.Texture2D<unsigned int>::mips_type" undef, !40, %"class.Texture2DMS<float, 0>" undef, !51, %"class.Texture2DMS<float, 0>::sample_type" undef, !40, %"class.StructuredBuffer<vector<unsigned int, 4> >" undef, !53, %"class.StructuredBuffer<MaterialDataPart1>" undef, !54, %struct.MaterialDataPart1 undef, !56, %"class.StructuredBuffer<MaterialDataPart3>" undef, !59, %struct.MaterialDataPart3 undef, !60, %"class.StructuredBuffer<MaterialBindingData>" undef, !104, %struct.MaterialBindingData undef, !105, %struct.order2_sh undef, !107, %"class.RWTexture3D<vector<float, 4> >" undef, !112, %"class.Texture3D<vector<float, 4> >" undef, !37, %"class.Texture3D<vector<float, 4> >::mips_type" undef, !40, %struct.LightVolumeData undef, !113, %"class.TextureCube<vector<float, 4> >" undef, !112, %"class.StructuredBuffer<DeferredLightPoint>" undef, !116, %struct.DeferredLightPoint undef, !117, %"class.StructuredBuffer<DeferredLightPointClipping>" undef, !132, %struct.DeferredLightPointClipping undef, !133, %"class.StructuredBuffer<DeferredLightPointBVH>" undef, !135, %struct.DeferredLightPointBVH undef, !136, %"class.RWTexture2D<unsigned int>" undef, !143, %"class.StructuredBuffer<DeferredLightSpot>" undef, !144, %struct.DeferredLightSpot undef, !145, %"class.StructuredBuffer<DeferredLightSpotClipping>" undef, !170, %struct.DeferredLightSpotClipping undef, !171, %"class.StructuredBuffer<DeferredLightSpotBVH>" undef, !135, %struct.DeferredLightSpotBVH undef, !136, %"class.StructuredBuffer<DeferredLightSun>" undef, !176, %struct.DeferredLightSun undef, !177, %"class.StructuredBuffer<VolumeSamplingInfo>" undef, !191, %struct.VolumeSamplingInfo undef, !192, %"class.StructuredBuffer<unsigned int>" undef, !143, %"class.StructuredBuffer<InternalNode>" undef, !132, %struct.InternalNode undef, !202, %"class.Texture3D<vector<float, 3> >" undef, !207, %"class.Texture3D<vector<float, 3> >::mips_type" undef, !40, %struct.VolumeCullingInCB undef, !209, %struct.VolumeTopLevelInfo undef, !214, %struct.DeferredLightSpecularBRDF undef, !218, %struct.DeferredLightGenericSurface undef, !221, %struct.DeferredLightTransparentBRDF undef, !225, %struct.DeferredLightIntensity undef, !228, %struct.DeferredLightVolumeResult undef, !231, %struct.DeferredLightPointSurface undef, !233, %struct.DeferredLightGBufferData undef, !234, %struct.CellInfo undef, !241, %"class.StructuredBuffer<EnvironmentMapDynamicInfo>" undef, !104, %struct.EnvironmentMapDynamicInfo undef, !267, %"class.StructuredBuffer<EnvironmentMapReference>" undef, !104, %struct.EnvironmentMapReference undef, !269, %"class.StructuredBuffer<CellInfo>" undef, !271, %"class.StructuredBuffer<Node>" undef, !272, %struct.Node undef, !273, %struct.ChildMask undef, !277, %struct.NodePointer undef, !279, %"class.StructuredBuffer<IrradianceProbe>" undef, !281, %struct.IrradianceProbe undef, !282, %"class.StructuredBuffer<TransportProbe>" undef, !292, %struct.TransportProbe undef, !293, %"class.StructuredBuffer<vector<float, 4> >" undef, !112, %"class.StructuredBuffer<ProbeRef>" undef, !54, %struct.ProbeRef undef, !295, %"class.StructuredBuffer<EnvironmentMapInfo>" undef, !298, %struct.EnvironmentMapInfo undef, !299, %struct.VoxelInfo undef, !305, %struct.LeafInfo undef, !311, %"class.StructuredBuffer<PerInstanceLBData>" undef, !132, %struct.PerInstanceLBData undef, !319, %"class.StructuredBuffer<RenderInstanceData>" undef, !323, %struct.RenderInstanceData undef, !324, %struct.ByteAddressBuffer undef, !45, %struct.RaytracingHitRootConstants undef, !332, %struct.RaytracingAccelerationStructure undef, !45, %struct.IntersectionAttributes undef, !343, %struct.HitData undef, !345, %"class.RWTexture2DArray<unsigned int>" undef, !143, %"class.RWTexture2DArray<vector<float, 4> >" undef, !112, %"class.RWStructuredBuffer<unsigned int>" undef, !143, %struct.RayDesc undef, !347, %debug_general undef, !352, %sys_constants undef, !375, %shadow_general undef, !408, %mid_translucency undef, !413, %mid_general undef, !105, %deferredlight_constants undef, !415, %env_general undef, !451, %atmosphere_general undef, !454, %illuminationvolume undef, !475, %ManualUpdateCB_ActiveVolumeTopLevelInfoArray undef, !482, %ManualUpdateCB_DebugVolumeTopLevelInfoArray undef, !484, %WorldGridInfo undef, !486, %EnvironmentMapAtlas undef, !490, %vertex_binding undef, !496, %g_cbRaytracingHit undef, !500, %rtreflection undef, !502}
!37 = !{i32 20, !38, !39}
!38 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 9}
!39 = !{i32 6, !"mips", i32 3, i32 16}
!40 = !{i32 4, !41}
!41 = !{i32 6, !"handle", i32 3, i32 0, i32 7, i32 5}
!42 = !{i32 32, !43, !44}
!43 = !{i32 6, !"vTarget1", i32 3, i32 0, i32 4, !"SV_Target0", i32 7, i32 9}
!44 = !{i32 6, !"vTarget2", i32 3, i32 16, i32 4, !"SV_Target1", i32 7, i32 9}
!45 = !{i32 4, !46}
!46 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 4}
!47 = !{i32 8, !38, !48}
!48 = !{i32 6, !"mips", i32 3, i32 4}
!49 = !{i32 8, !50, !48}
!50 = !{i32 6, !"h", i32 3, i32 0, i32 7, i32 5}
!51 = !{i32 8, !38, !52}
!52 = !{i32 6, !"sample", i32 3, i32 4}
!53 = !{i32 16, !50}
!54 = !{i32 8, !55}
!55 = !{i32 6, !"h", i32 3, i32 0}
!56 = !{i32 8, !57, !58}
!57 = !{i32 6, !"vSpecularColor_uBRDF", i32 3, i32 0, i32 7, i32 5}
!58 = !{i32 6, !"uScatterIndex_vTranslucencyDepthRange", i32 3, i32 4, i32 7, i32 5}
!59 = !{i32 352, !55}
!60 = !{i32 352, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !87, !88, !89, !90, !91, !92, !93, !94, !95, !96, !97, !98, !99, !100, !101, !102, !103}
!61 = !{i32 6, !"fClothDiffScatterAmount", i32 3, i32 0, i32 7, i32 9}
!62 = !{i32 6, !"fClothFuzzWrap", i32 3, i32 4, i32 7, i32 9}
!63 = !{i32 6, !"vClothScatterColor", i32 3, i32 16, i32 7, i32 9}
!64 = !{i32 6, !"fHairSmoothness", i32 3, i32 28, i32 7, i32 9}
!65 = !{i32 6, !"vSpecularColor2", i32 3, i32 32, i32 7, i32 9}
!66 = !{i32 6, !"fHairSpecularShift", i32 3, i32 44, i32 7, i32 9}
!67 = !{i32 6, !"fHairDiffuseWrap", i32 3, i32 48, i32 7, i32 9}
!68 = !{i32 6, !"fPAD1", i32 3, i32 52, i32 7, i32 9}
!69 = !{i32 6, !"fEmissionIntensity", i32 3, i32 56, i32 7, i32 9}
!70 = !{i32 6, !"vColorMultiplier", i32 3, i32 64, i32 7, i32 9}
!71 = !{i32 6, !"vSmoothnessRange", i32 3, i32 80, i32 7, i32 9}
!72 = !{i32 6, !"vShadowSoftnessRange", i32 3, i32 88, i32 7, i32 9}
!73 = !{i32 6, !"vScatterWidthRange", i32 3, i32 96, i32 7, i32 9}
!74 = !{i32 6, !"fHairDepthOffsetAmount", i32 3, i32 104, i32 7, i32 9}
!75 = !{i32 6, !"fHairNormalAmount", i32 3, i32 108, i32 7, i32 9}
!76 = !{i32 6, !"vEmissionMultiplier", i32 3, i32 112, i32 7, i32 9}
!77 = !{i32 6, !"vColorMultiplier2", i32 3, i32 128, i32 7, i32 9}
!78 = !{i32 6, !"vBaseDetailUVScale", i32 3, i32 144, i32 7, i32 9}
!79 = !{i32 6, !"vSmoothnessRange2", i32 3, i32 152, i32 7, i32 9}
!80 = !{i32 6, !"vBlendDetailUVScale", i32 3, i32 160, i32 7, i32 9}
!81 = !{i32 6, !"fBaseDetailAmount", i32 3, i32 168, i32 7, i32 9}
!82 = !{i32 6, !"fBlendUVMultiplier", i32 3, i32 172, i32 7, i32 9}
!83 = !{i32 6, !"fBlendDetailAmount", i32 3, i32 176, i32 7, i32 9}
!84 = !{i32 6, !"fFuzzUVMultiplier", i32 3, i32 180, i32 7, i32 9}
!85 = !{i32 6, !"fPupilDilation", i32 3, i32 184, i32 7, i32 9}
!86 = !{i32 6, !"fPupilOuterRadius", i32 3, i32 188, i32 7, i32 9}
!87 = !{i32 6, !"vIntensity", i32 3, i32 192, i32 7, i32 9}
!88 = !{i32 6, !"vWrinkleWeights0", i32 3, i32 208, i32 7, i32 9}
!89 = !{i32 6, !"vWrinkleWeights1", i32 3, i32 224, i32 7, i32 9}
!90 = !{i32 6, !"vWrinkleWeights2", i32 3, i32 240, i32 7, i32 9}
!91 = !{i32 6, !"vWrinkleWeights3", i32 3, i32 256, i32 7, i32 9}
!92 = !{i32 6, !"vWrinkleWeights4", i32 3, i32 272, i32 7, i32 9}
!93 = !{i32 6, !"vWrinkleWeights5", i32 3, i32 288, i32 7, i32 9}
!94 = !{i32 6, !"uDisableBackfaceFlip", i32 3, i32 304, i32 7, i32 5}
!95 = !{i32 6, !"fEdgeSharpness", i32 3, i32 308, i32 7, i32 9}
!96 = !{i32 6, !"fTrunkBendFactor", i32 3, i32 312, i32 7, i32 9}
!97 = !{i32 6, !"fTrunkPivotOffset", i32 3, i32 316, i32 7, i32 9}
!98 = !{i32 6, !"vColorRgbMultiplier", i32 3, i32 320, i32 7, i32 9}
!99 = !{i32 6, !"fWindFactor", i32 3, i32 332, i32 7, i32 9}
!100 = !{i32 6, !"fDecalMaterialBlendFactor", i32 3, i32 336, i32 7, i32 9}
!101 = !{i32 6, !"fDecalAlbedoBlendFactor", i32 3, i32 340, i32 7, i32 9}
!102 = !{i32 6, !"uStableHash", i32 3, i32 344, i32 7, i32 5}
!103 = !{i32 6, !"pad", i32 3, i32 348, i32 7, i32 5}
!104 = !{i32 4, !55}
!105 = !{i32 4, !106}
!106 = !{i32 6, !"g_uMaterialID", i32 3, i32 0, i32 7, i32 5}
!107 = !{i32 60, !108, !109, !110, !111}
!108 = !{i32 6, !"c0", i32 3, i32 0, i32 7, i32 9}
!109 = !{i32 6, !"c11", i32 3, i32 16, i32 7, i32 9}
!110 = !{i32 6, !"c10", i32 3, i32 32, i32 7, i32 9}
!111 = !{i32 6, !"c1m1", i32 3, i32 48, i32 7, i32 9}
!112 = !{i32 16, !38}
!113 = !{i32 32, !114, !115}
!114 = !{i32 6, !"value0", i32 3, i32 0, i32 7, i32 9}
!115 = !{i32 6, !"value1", i32 3, i32 16, i32 7, i32 9}
!116 = !{i32 160, !55}
!117 = !{i32 160, !118, !119, !120, !121, !122, !123, !124, !125, !127, !128, !129, !130, !131}
!118 = !{i32 6, !"vPositionInView", i32 3, i32 0, i32 7, i32 9}
!119 = !{i32 6, !"fShadowMapShrink", i32 3, i32 12, i32 7, i32 9}
!120 = !{i32 6, !"vColor", i32 3, i32 16, i32 7, i32 9}
!121 = !{i32 6, !"fClipRange", i32 3, i32 28, i32 7, i32 9}
!122 = !{i32 6, !"vFalloff", i32 3, i32 32, i32 7, i32 9}
!123 = !{i32 6, !"vDirectionInView", i32 3, i32 48, i32 7, i32 9}
!124 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 60, i32 7, i32 9}
!125 = !{i32 6, !"mViewToShadowClip", i32 2, !126, i32 3, i32 64, i32 7, i32 9}
!126 = !{i32 4, i32 4, i32 2}
!127 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 128, i32 7, i32 9}
!128 = !{i32 6, !"fRadius", i32 3, i32 144, i32 7, i32 9}
!129 = !{i32 6, !"fBoundsRadius", i32 3, i32 148, i32 7, i32 9}
!130 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 152, i32 7, i32 9}
!131 = !{i32 6, !"uTechniqueProperties", i32 3, i32 156, i32 7, i32 5}
!132 = !{i32 16, !55}
!133 = !{i32 16, !118, !134}
!134 = !{i32 6, !"fBoundsRadius", i32 3, i32 12, i32 7, i32 9}
!135 = !{i32 44, !55}
!136 = !{i32 44, !137, !138, !139, !140, !141, !142}
!137 = !{i32 6, !"vMinInView", i32 3, i32 0, i32 7, i32 9}
!138 = !{i32 6, !"vMaxInView", i32 3, i32 16, i32 7, i32 9}
!139 = !{i32 6, !"uChildLHS", i32 3, i32 28, i32 7, i32 5}
!140 = !{i32 6, !"uChildRHS", i32 3, i32 32, i32 7, i32 5}
!141 = !{i32 6, !"uLightIndex", i32 3, i32 36, i32 7, i32 5}
!142 = !{i32 6, !"uCount", i32 3, i32 40, i32 7, i32 5}
!143 = !{i32 4, !50}
!144 = !{i32 336, !55}
!145 = !{i32 336, !118, !146, !147, !148, !149, !150, !151, !152, !153, !154, !155, !156, !157, !158, !159, !161, !162, !163, !164, !165, !166, !167, !168, !169}
!146 = !{i32 6, !"fNearDistance", i32 3, i32 12, i32 7, i32 9}
!147 = !{i32 6, !"vWedPositionInView", i32 3, i32 16, i32 7, i32 9}
!148 = !{i32 6, !"fFarDistance", i32 3, i32 28, i32 7, i32 9}
!149 = !{i32 6, !"vDirectionInView", i32 3, i32 32, i32 7, i32 9}
!150 = !{i32 6, !"fFarWidth", i32 3, i32 44, i32 7, i32 9}
!151 = !{i32 6, !"vColor", i32 3, i32 48, i32 7, i32 9}
!152 = !{i32 6, !"fClipRange", i32 3, i32 60, i32 7, i32 9}
!153 = !{i32 6, !"vFalloff", i32 3, i32 64, i32 7, i32 9}
!154 = !{i32 6, !"fRadius", i32 3, i32 72, i32 7, i32 9}
!155 = !{i32 6, !"fSinConeAnglePerTwo_sq", i32 3, i32 76, i32 7, i32 9}
!156 = !{i32 6, !"mViewToProjectionClip", i32 2, !126, i32 3, i32 80, i32 7, i32 9}
!157 = !{i32 6, !"mCameraPositionInCone", i32 3, i32 144, i32 7, i32 9}
!158 = !{i32 6, !"fInvShadowMapRange", i32 3, i32 156, i32 7, i32 9}
!159 = !{i32 6, !"mViewToCone", i32 2, !160, i32 3, i32 160, i32 7, i32 9}
!160 = !{i32 3, i32 3, i32 2}
!161 = !{i32 6, !"fShadowPCFSize", i32 3, i32 204, i32 7, i32 9}
!162 = !{i32 6, !"fShadowLOD", i32 3, i32 208, i32 7, i32 9}
!163 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 212, i32 7, i32 9}
!164 = !{i32 6, !"mConeToView", i32 3, i32 224, i32 7, i32 9}
!165 = !{i32 6, !"vProjectionAtlasOffsetScale", i32 3, i32 272, i32 7, i32 9}
!166 = !{i32 6, !"vShadowAtlasOffsetScale", i32 3, i32 288, i32 7, i32 9}
!167 = !{i32 6, !"vDynamicShadowAtlasOffsetScale", i32 3, i32 304, i32 7, i32 9}
!168 = !{i32 6, !"uTechniqueProperties", i32 3, i32 320, i32 7, i32 5}
!169 = !{i32 6, !"padTo16bytes", i32 3, i32 324, i32 7, i32 5}
!170 = !{i32 48, !55}
!171 = !{i32 48, !118, !146, !172, !148, !173, !174, !175}
!172 = !{i32 6, !"vDirectionInView", i32 3, i32 16, i32 7, i32 9}
!173 = !{i32 6, !"fFarWidth", i32 3, i32 32, i32 7, i32 9}
!174 = !{i32 6, !"uTechniqueProperties", i32 3, i32 36, i32 7, i32 5}
!175 = !{i32 6, !"padTo16bytes", i32 3, i32 40, i32 7, i32 5}
!176 = !{i32 708, !55}
!177 = !{i32 708, !178, !179, !120, !180, !181, !182, !183, !184, !185, !186, !187, !188, !189, !190}
!178 = !{i32 6, !"vDirectionInView", i32 3, i32 0, i32 7, i32 9}
!179 = !{i32 6, !"pad1", i32 3, i32 12, i32 7, i32 4}
!180 = !{i32 6, !"pad2", i32 3, i32 28, i32 7, i32 4}
!181 = !{i32 6, !"mViewToProjectionClip", i32 2, !126, i32 3, i32 32, i32 7, i32 9}
!182 = !{i32 6, !"fProjectionMapTransparency", i32 3, i32 96, i32 7, i32 9}
!183 = !{i32 6, !"fRadius", i32 3, i32 100, i32 7, i32 9}
!184 = !{i32 6, !"uTechniqueProperties", i32 3, i32 104, i32 7, i32 5}
!185 = !{i32 6, !"fScatterIntensityMul", i32 3, i32 108, i32 7, i32 9}
!186 = !{i32 6, !"uCascadeCount", i32 3, i32 112, i32 7, i32 5}
!187 = !{i32 6, !"mCascadeViewToShadowClip", i32 2, !126, i32 3, i32 128, i32 7, i32 9}
!188 = !{i32 6, !"vCascadeRange", i32 3, i32 512, i32 7, i32 9}
!189 = !{i32 6, !"fCascadePCFWidth", i32 3, i32 608, i32 7, i32 9}
!190 = !{i32 6, !"pad3", i32 3, i32 704, i32 7, i32 4}
!191 = !{i32 92, !55}
!192 = !{i32 92, !193, !194, !195, !196, !197, !198, !199, !200, !201}
!193 = !{i32 6, !"mWorldToLocalRotationMatrix", i32 2, !160, i32 3, i32 0, i32 7, i32 9}
!194 = !{i32 6, !"vOneOverTopLevelFadeLength", i32 3, i32 48, i32 7, i32 9}
!195 = !{i32 6, !"uTopLevelNodeIndicesOffset", i32 3, i32 60, i32 7, i32 5}
!196 = !{i32 6, !"uInternalNodeIndexOffset", i32 3, i32 64, i32 7, i32 5}
!197 = !{i32 6, !"uBrickIndexOffset", i32 3, i32 68, i32 7, i32 5}
!198 = !{i32 6, !"fTopLevelTexelSizeInWorld", i32 3, i32 72, i32 7, i32 9}
!199 = !{i32 6, !"fWeightMultiplier", i32 3, i32 76, i32 7, i32 9}
!200 = !{i32 6, !"uAdditive", i32 3, i32 80, i32 7, i32 5}
!201 = !{i32 6, !"fpadTo16Bytes", i32 3, i32 84, i32 7, i32 9}
!202 = !{i32 16, !203, !204, !205, !206}
!203 = !{i32 6, !"uChildMask", i32 3, i32 0, i32 7, i32 5}
!204 = !{i32 6, !"uLeafChildMask", i32 3, i32 4, i32 7, i32 5}
!205 = !{i32 6, !"uChildBlockOffset", i32 3, i32 8, i32 7, i32 5}
!206 = !{i32 6, !"uLeafChildBlockOffset", i32 3, i32 12, i32 7, i32 5}
!207 = !{i32 16, !38, !208}
!208 = !{i32 6, !"mips", i32 3, i32 12}
!209 = !{i32 32, !210, !211, !212, !213}
!210 = !{i32 6, !"vAABBMinInView", i32 3, i32 0, i32 7, i32 9}
!211 = !{i32 6, !"fPad0", i32 3, i32 12, i32 7, i32 9}
!212 = !{i32 6, !"vAABBMaxInView", i32 3, i32 16, i32 7, i32 9}
!213 = !{i32 6, !"fPad1", i32 3, i32 28, i32 7, i32 9}
!214 = !{i32 64, !215, !216, !217}
!215 = !{i32 6, !"mWorldToTopLevelMatrixDecomp", i32 3, i32 0, i32 7, i32 9}
!216 = !{i32 6, !"vTopLevelSize", i32 3, i32 48, i32 7, i32 5}
!217 = !{i32 6, !"fPad0", i32 3, i32 60, i32 7, i32 9}
!218 = !{i32 28, !219, !220}
!219 = !{i32 6, !"vResultDiffuse", i32 3, i32 0, i32 7, i32 9}
!220 = !{i32 6, !"vResultSpecular", i32 3, i32 16, i32 7, i32 9}
!221 = !{i32 36, !118, !222, !223, !224}
!222 = !{i32 6, !"vNormalInView", i32 3, i32 16, i32 7, i32 9}
!223 = !{i32 6, !"fSmoothness", i32 3, i32 28, i32 7, i32 9}
!224 = !{i32 6, !"fSpecularAlbedoIntensity", i32 3, i32 32, i32 7, i32 9}
!225 = !{i32 44, !219, !226, !227}
!226 = !{i32 6, !"vResultDiffuseTransmission", i32 3, i32 16, i32 7, i32 9}
!227 = !{i32 6, !"vResultSpecular", i32 3, i32 32, i32 7, i32 9}
!228 = !{i32 28, !229, !230}
!229 = !{i32 6, !"vIntensityDiffuse", i32 3, i32 0, i32 7, i32 9}
!230 = !{i32 6, !"vIntensitySpecular", i32 3, i32 16, i32 7, i32 9}
!231 = !{i32 32, !232}
!232 = !{i32 6, !"d", i32 3, i32 0}
!233 = !{i32 12, !118}
!234 = !{i32 68, !235, !236, !237, !238, !239, !240}
!235 = !{i32 6, !"vGBufferTarget1", i32 3, i32 0, i32 7, i32 9}
!236 = !{i32 6, !"vGBufferTarget2", i32 3, i32 16, i32 7, i32 9}
!237 = !{i32 6, !"vPositionInView", i32 3, i32 32, i32 7, i32 9}
!238 = !{i32 6, !"vBentNormalInView", i32 3, i32 48, i32 7, i32 9}
!239 = !{i32 6, !"fDiffuseOcclusion", i32 3, i32 60, i32 7, i32 9}
!240 = !{i32 6, !"fSpecularOcclusion", i32 3, i32 64, i32 7, i32 9}
!241 = !{i32 356, !242, !243, !244, !245, !246, !247, !248, !249, !250, !251, !252, !253, !254, !255, !256, !257, !258, !259, !260, !261, !262, !263, !264, !265, !266}
!242 = !{i32 6, !"m_vMin", i32 3, i32 0, i32 7, i32 9}
!243 = !{i32 6, !"m_fBaseVoxelSize", i32 3, i32 12, i32 7, i32 9}
!244 = !{i32 6, !"m_vMax", i32 3, i32 16, i32 7, i32 9}
!245 = !{i32 6, !"m_fInvBaseVoxelSize", i32 3, i32 28, i32 7, i32 9}
!246 = !{i32 6, !"m_uVoxelResolution", i32 3, i32 32, i32 7, i32 5}
!247 = !{i32 6, !"m_uLog2VoxelResolution", i32 3, i32 36, i32 7, i32 5}
!248 = !{i32 6, !"m_uBaseVoxelResolutionX", i32 3, i32 40, i32 7, i32 5}
!249 = !{i32 6, !"m_uBaseVoxelResolutionXY", i32 3, i32 44, i32 7, i32 5}
!250 = !{i32 6, !"m_iPrimaryTreeOffset", i32 3, i32 48, i32 7, i32 4}
!251 = !{i32 6, !"m_iDualTreeOffset", i32 3, i32 52, i32 7, i32 4}
!252 = !{i32 6, !"m_iIrradianceProbeOffset", i32 3, i32 56, i32 7, i32 4}
!253 = !{i32 6, !"m_iDirectTransportProbeOffset", i32 3, i32 60, i32 7, i32 4}
!254 = !{i32 6, !"m_iIndirectTransportProbeOffset", i32 3, i32 64, i32 7, i32 4}
!255 = !{i32 6, !"m_iLightGatherOffset", i32 3, i32 68, i32 7, i32 4}
!256 = !{i32 6, !"m_iLightProbeRefOffset", i32 3, i32 72, i32 7, i32 4}
!257 = !{i32 6, !"m_iDualLightProbeRefOffset", i32 3, i32 76, i32 7, i32 4}
!258 = !{i32 6, !"m_iEnvironmentMapRefOffset", i32 3, i32 80, i32 7, i32 4}
!259 = !{i32 6, !"m_iEnvironmentMapDualRefOffset", i32 3, i32 84, i32 7, i32 4}
!260 = !{i32 6, !"m_iEnvironmentMapInfoOffset", i32 3, i32 88, i32 7, i32 4}
!261 = !{i32 6, !"m_fWorldToGrid", i32 3, i32 92, i32 7, i32 9}
!262 = !{i32 6, !"m_fGridToWorld", i32 3, i32 96, i32 7, i32 9}
!263 = !{i32 6, !"pad0", i32 3, i32 100, i32 7, i32 9}
!264 = !{i32 6, !"pad1", i32 3, i32 104, i32 7, i32 9}
!265 = !{i32 6, !"pad2", i32 3, i32 108, i32 7, i32 9}
!266 = !{i32 6, !"m_iRowTranslationTable", i32 3, i32 112, i32 7, i32 4}
!267 = !{i32 4, !268}
!268 = !{i32 6, !"m_fDiffuseC0", i32 3, i32 0, i32 7, i32 9}
!269 = !{i32 4, !270}
!270 = !{i32 6, !"m_data", i32 3, i32 0, i32 7, i32 5}
!271 = !{i32 356, !55}
!272 = !{i32 28, !55}
!273 = !{i32 28, !274, !275, !276}
!274 = !{i32 6, !"m_childMask", i32 3, i32 0}
!275 = !{i32 6, !"m_pointer", i32 3, i32 20}
!276 = !{i32 6, !"m_payload", i32 3, i32 24, i32 7, i32 5}
!277 = !{i32 20, !278}
!278 = !{i32 6, !"m_uMask", i32 3, i32 0, i32 7, i32 5}
!279 = !{i32 4, !280}
!280 = !{i32 6, !"m_flagAndPointer", i32 3, i32 0, i32 7, i32 5}
!281 = !{i32 36, !55}
!282 = !{i32 36, !283, !284, !285, !286, !287, !288, !289, !290, !291}
!283 = !{i32 6, !"m_vR00", i32 3, i32 0, i32 7, i32 5}
!284 = !{i32 6, !"m_vR01", i32 3, i32 4, i32 7, i32 5}
!285 = !{i32 6, !"m_vR02", i32 3, i32 8, i32 7, i32 5}
!286 = !{i32 6, !"m_vR10", i32 3, i32 12, i32 7, i32 5}
!287 = !{i32 6, !"m_vR11", i32 3, i32 16, i32 7, i32 5}
!288 = !{i32 6, !"m_vR12", i32 3, i32 20, i32 7, i32 5}
!289 = !{i32 6, !"m_vR20", i32 3, i32 24, i32 7, i32 5}
!290 = !{i32 6, !"m_vR21", i32 3, i32 28, i32 7, i32 5}
!291 = !{i32 6, !"m_vR22", i32 3, i32 32, i32 7, i32 5}
!292 = !{i32 116, !55}
!293 = !{i32 116, !294}
!294 = !{i32 6, !"m_mTransport", i32 3, i32 0, i32 7, i32 5}
!295 = !{i32 8, !296, !297}
!296 = !{i32 6, !"m_index1", i32 3, i32 0, i32 7, i32 5}
!297 = !{i32 6, !"m_index2", i32 3, i32 4, i32 7, i32 5}
!298 = !{i32 220, !55}
!299 = !{i32 220, !300, !301, !302, !303, !304}
!300 = !{i32 6, !"m_vDebugColor", i32 3, i32 0, i32 7, i32 9}
!301 = !{i32 6, !"m_fRadius", i32 3, i32 12, i32 7, i32 9}
!302 = !{i32 6, !"m_vCenter", i32 3, i32 16, i32 7, i32 9}
!303 = !{i32 6, !"m_vMin", i32 3, i32 32, i32 7, i32 9}
!304 = !{i32 6, !"m_vMax", i32 3, i32 128, i32 7, i32 9}
!305 = !{i32 44, !306, !307, !308, !309, !310}
!306 = !{i32 6, !"m_vVoxelMin", i32 3, i32 0, i32 7, i32 9}
!307 = !{i32 6, !"m_fVoxelSize", i32 3, i32 12, i32 7, i32 9}
!308 = !{i32 6, !"m_uPayload", i32 3, i32 16, i32 7, i32 5}
!309 = !{i32 6, !"m_uLinearBaseVoxelIndex", i32 3, i32 20, i32 7, i32 5}
!310 = !{i32 6, !"m_vBaseVoxelMin", i32 3, i32 32, i32 7, i32 9}
!311 = !{i32 44, !312, !313, !314, !315, !316, !317, !318}
!312 = !{i32 6, !"m_iParentNode", i32 3, i32 0, i32 7, i32 4}
!313 = !{i32 6, !"m_vParentVoxelMin", i32 3, i32 4, i32 7, i32 9}
!314 = !{i32 6, !"m_vLeafVoxelMin", i32 3, i32 16, i32 7, i32 9}
!315 = !{i32 6, !"m_fLeafVoxelSize", i32 3, i32 28, i32 7, i32 9}
!316 = !{i32 6, !"m_bIsTerminal", i32 3, i32 32, i32 7, i32 1}
!317 = !{i32 6, !"m_bIsSolid", i32 3, i32 36, i32 7, i32 1}
!318 = !{i32 6, !"m_iPayload", i32 3, i32 40, i32 7, i32 4}
!319 = !{i32 16, !320, !321, !322}
!320 = !{i32 6, !"m_vUVOffset", i32 3, i32 0, i32 7, i32 9}
!321 = !{i32 6, !"m_uEnvMapIndex", i32 3, i32 8, i32 7, i32 5}
!322 = !{i32 6, !"m_uEnvInfoIndex", i32 3, i32 12, i32 7, i32 5}
!323 = !{i32 152, !55}
!324 = !{i32 152, !325, !326, !327, !328, !329, !330, !331}
!325 = !{i32 6, !"g_fPerLODDissolve", i32 3, i32 0, i32 7, i32 9}
!326 = !{i32 6, !"g_vWindIndex_Base_Trunk", i32 3, i32 116, i32 7, i32 5}
!327 = !{i32 6, !"g_uMask", i32 3, i32 124, i32 7, i32 5}
!328 = !{i32 6, !"g_iCharacterLightRigIndex", i32 3, i32 128, i32 7, i32 4}
!329 = !{i32 6, !"g_fDepthMultiply", i32 3, i32 132, i32 7, i32 9}
!330 = !{i32 6, !"g_fExtraDepthMultiply", i32 3, i32 136, i32 7, i32 9}
!331 = !{i32 6, !"padTo16Bytes", i32 3, i32 144, i32 7, i32 4}
!332 = !{i32 40, !333, !334, !335, !336, !337, !338, !339, !340, !341, !342}
!333 = !{i32 6, !"uIndexOffset", i32 3, i32 0, i32 7, i32 5}
!334 = !{i32 6, !"uIndexStride", i32 3, i32 4, i32 7, i32 5}
!335 = !{i32 6, !"uVertexStride1", i32 3, i32 8, i32 7, i32 5}
!336 = !{i32 6, !"uMaterialID", i32 3, i32 12, i32 7, i32 5}
!337 = !{i32 6, !"uTangentOffset", i32 3, i32 16, i32 7, i32 5}
!338 = !{i32 6, !"uNormalOffset", i32 3, i32 20, i32 7, i32 5}
!339 = !{i32 6, !"uTexcoordOffset", i32 3, i32 24, i32 7, i32 5}
!340 = !{i32 6, !"uColorOffset", i32 3, i32 28, i32 7, i32 5}
!341 = !{i32 6, !"uIndexBufferSize", i32 3, i32 32, i32 7, i32 5}
!342 = !{i32 6, !"uVertexBuffer1Size", i32 3, i32 36, i32 7, i32 5}
!343 = !{i32 8, !344}
!344 = !{i32 6, !"vIntersection", i32 3, i32 0, i32 4, !"INTERSECTION", i32 7, i32 9}
!345 = !{i32 4, !346}
!346 = !{i32 6, !"uRayIndex", i32 3, i32 0, i32 4, !"RAY_INDEX", i32 7, i32 5}
!347 = !{i32 32, !348, !349, !350, !351}
!348 = !{i32 6, !"Origin", i32 3, i32 0, i32 7, i32 9}
!349 = !{i32 6, !"TMin", i32 3, i32 12, i32 7, i32 9}
!350 = !{i32 6, !"Direction", i32 3, i32 16, i32 7, i32 9}
!351 = !{i32 6, !"TMax", i32 3, i32 28, i32 7, i32 9}
!352 = !{i32 88, !353, !354, !355, !356, !357, !358, !359, !360, !361, !362, !363, !364, !365, !366, !367, !368, !369, !370, !371, !372, !373, !374}
!353 = !{i32 6, !"g_fDebug_h_Slider0", i32 3, i32 0, i32 7, i32 9}
!354 = !{i32 6, !"g_fDebug_h_Slider1", i32 3, i32 4, i32 7, i32 9}
!355 = !{i32 6, !"g_fDebug_h_Slider2", i32 3, i32 8, i32 7, i32 9}
!356 = !{i32 6, !"g_fDebug_h_Slider3", i32 3, i32 12, i32 7, i32 9}
!357 = !{i32 6, !"g_fDebug_h_Slider4", i32 3, i32 16, i32 7, i32 9}
!358 = !{i32 6, !"g_fDebug_h_Slider5", i32 3, i32 20, i32 7, i32 9}
!359 = !{i32 6, !"g_fDebug_h_Slider6", i32 3, i32 24, i32 7, i32 9}
!360 = !{i32 6, !"g_fDebug_h_Slider7", i32 3, i32 28, i32 7, i32 9}
!361 = !{i32 6, !"g_fDebug_h_Knob0", i32 3, i32 32, i32 7, i32 9}
!362 = !{i32 6, !"g_fDebug_h_Knob1", i32 3, i32 36, i32 7, i32 9}
!363 = !{i32 6, !"g_fDebug_h_Knob2", i32 3, i32 40, i32 7, i32 9}
!364 = !{i32 6, !"g_fDebug_h_Knob3", i32 3, i32 44, i32 7, i32 9}
!365 = !{i32 6, !"g_fDebug_h_Knob4", i32 3, i32 48, i32 7, i32 9}
!366 = !{i32 6, !"g_fDebug_h_Knob5", i32 3, i32 52, i32 7, i32 9}
!367 = !{i32 6, !"g_fDebug_h_Knob6", i32 3, i32 56, i32 7, i32 9}
!368 = !{i32 6, !"g_fDebug_h_Knob7", i32 3, i32 60, i32 7, i32 9}
!369 = !{i32 6, !"g_fDebug_h_Toggle0", i32 3, i32 64, i32 7, i32 9}
!370 = !{i32 6, !"g_fDebug_h_Toggle1", i32 3, i32 68, i32 7, i32 9}
!371 = !{i32 6, !"g_fDebug_h_Toggle2", i32 3, i32 72, i32 7, i32 9}
!372 = !{i32 6, !"g_fDebug_h_Toggle3", i32 3, i32 76, i32 7, i32 9}
!373 = !{i32 6, !"g_fDebug_h_Toggle4", i32 3, i32 80, i32 7, i32 9}
!374 = !{i32 6, !"g_fDebug_h_Toggle5", i32 3, i32 84, i32 7, i32 9}
!375 = !{i32 1128, !376, !377, !378, !379, !380, !381, !382, !383, !384, !385, !386, !387, !388, !389, !390, !391, !392, !393, !394, !395, !396, !397, !398, !399, !400, !401, !402, !403, !404, !405, !406, !407}
!376 = !{i32 6, !"g_vScreenRes", i32 3, i32 0, i32 7, i32 9}
!377 = !{i32 6, !"g_vInvScreenRes", i32 3, i32 8, i32 7, i32 9}
!378 = !{i32 6, !"g_vOutputRes", i32 3, i32 16, i32 7, i32 9}
!379 = !{i32 6, !"g_vInvOutputRes", i32 3, i32 24, i32 7, i32 9}
!380 = !{i32 6, !"g_mWorldToView", i32 2, !126, i32 3, i32 32, i32 7, i32 9}
!381 = !{i32 6, !"g_mViewToWorld", i32 2, !126, i32 3, i32 96, i32 7, i32 9}
!382 = !{i32 6, !"g_mViewToClip", i32 2, !126, i32 3, i32 160, i32 7, i32 9}
!383 = !{i32 6, !"g_mClipToView", i32 2, !126, i32 3, i32 224, i32 7, i32 9}
!384 = !{i32 6, !"g_mWorldToClip", i32 2, !126, i32 3, i32 288, i32 7, i32 9}
!385 = !{i32 6, !"g_mClipToWorld", i32 2, !126, i32 3, i32 352, i32 7, i32 9}
!386 = !{i32 6, !"g_mClipToPreviousClip", i32 2, !126, i32 3, i32 416, i32 7, i32 9}
!387 = !{i32 6, !"g_mViewToPreviousClip", i32 2, !126, i32 3, i32 480, i32 7, i32 9}
!388 = !{i32 6, !"g_mPreviousViewToView", i32 2, !126, i32 3, i32 544, i32 7, i32 9}
!389 = !{i32 6, !"g_mPreviousWorldToClip", i32 2, !126, i32 3, i32 608, i32 7, i32 9}
!390 = !{i32 6, !"g_mPreviousViewToClip", i32 2, !126, i32 3, i32 672, i32 7, i32 9}
!391 = !{i32 6, !"g_mClipToPreviousClipNoJitter", i32 2, !126, i32 3, i32 736, i32 7, i32 9}
!392 = !{i32 6, !"g_mPreviousClipToClipNoJitter", i32 2, !126, i32 3, i32 800, i32 7, i32 9}
!393 = !{i32 6, !"g_vViewPoint", i32 3, i32 864, i32 7, i32 9}
!394 = !{i32 6, !"g_fInvNear", i32 3, i32 880, i32 7, i32 9}
!395 = !{i32 6, !"g_mViewToCameraView", i32 2, !126, i32 3, i32 896, i32 7, i32 9}
!396 = !{i32 6, !"g_mCameraViewToCameraClip", i32 2, !126, i32 3, i32 960, i32 7, i32 9}
!397 = !{i32 6, !"g_mCameraClipToView", i32 2, !126, i32 3, i32 1024, i32 7, i32 9}
!398 = !{i32 6, !"g_fWorldTime", i32 3, i32 1088, i32 7, i32 9}
!399 = !{i32 6, !"g_fWorldTimeDelta", i32 3, i32 1092, i32 7, i32 9}
!400 = !{i32 6, !"g_fRealTime", i32 3, i32 1096, i32 7, i32 9}
!401 = !{i32 6, !"g_fRealTimeDelta", i32 3, i32 1100, i32 7, i32 9}
!402 = !{i32 6, !"g_fLastValidWorldTimeDelta", i32 3, i32 1104, i32 7, i32 9}
!403 = !{i32 6, !"g_uTemporalFrame", i32 3, i32 1108, i32 7, i32 5}
!404 = !{i32 6, !"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5}
!405 = !{i32 6, !"g_bHDR", i32 3, i32 1116, i32 7, i32 1}
!406 = !{i32 6, !"g_fHDRSDRSceneBrightnessMultiplier", i32 3, i32 1120, i32 7, i32 9}
!407 = !{i32 6, !"g_fHDRSDRUIBrightnessMultiplier", i32 3, i32 1124, i32 7, i32 9}
!408 = !{i32 32, !409, !410, !411, !412}
!409 = !{i32 6, !"g_vShadowMapRes", i32 3, i32 0, i32 7, i32 9}
!410 = !{i32 6, !"g_vShadowMapVSMRes", i32 3, i32 8, i32 7, i32 9}
!411 = !{i32 6, !"g_vJitterOffset", i32 3, i32 16, i32 7, i32 9}
!412 = !{i32 6, !"g_vSnapOffset", i32 3, i32 24, i32 7, i32 4}
!413 = !{i32 4, !414}
!414 = !{i32 6, !"g_fOnePerTranslucencyKernelCount", i32 3, i32 0, i32 7, i32 9}
!415 = !{i32 408, !416, !417, !418, !419, !420, !421, !422, !423, !424, !425, !426, !427, !428, !429, !430, !431, !432, !433, !434, !435, !436, !437, !438, !439, !440, !441, !442, !443, !444, !445, !446, !447, !448, !449, !450}
!416 = !{i32 6, !"g_fTileDepthClipRanges", i32 3, i32 0, i32 7, i32 9}
!417 = !{i32 6, !"g_fTileDepthRanges", i32 3, i32 80, i32 7, i32 9}
!418 = !{i32 6, !"g_vDepthTileResolve", i32 3, i32 160, i32 7, i32 9}
!419 = !{i32 6, !"g_uDepthTileCount", i32 3, i32 168, i32 7, i32 5}
!420 = !{i32 6, !"g_vTileResolution", i32 3, i32 176, i32 7, i32 5}
!421 = !{i32 6, !"g_vTileWidthHeightDepth", i32 3, i32 192, i32 7, i32 5}
!422 = !{i32 6, !"g_vTileResolutionPerScreenResolution", i32 3, i32 208, i32 7, i32 9}
!423 = !{i32 6, !"g_vTileDepthNearFar", i32 3, i32 216, i32 7, i32 9}
!424 = !{i32 6, !"g_uMaxPointLightsPerTile", i32 3, i32 224, i32 7, i32 5}
!425 = !{i32 6, !"g_uMaxSpotLightsPerTile", i32 3, i32 228, i32 7, i32 5}
!426 = !{i32 6, !"g_uAmbientLightTotalCount", i32 3, i32 232, i32 7, i32 5}
!427 = !{i32 6, !"g_fAmbientEnvDiffuseIntensity", i32 3, i32 236, i32 7, i32 9}
!428 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnTransparent", i32 3, i32 240, i32 7, i32 9}
!429 = !{i32 6, !"g_fAmbientEnvSpecularIntensityOnOpaque", i32 3, i32 244, i32 7, i32 9}
!430 = !{i32 6, !"g_fAmbientSkyIntensity", i32 3, i32 248, i32 7, i32 9}
!431 = !{i32 6, !"g_fAmbientLocalIntensity", i32 3, i32 252, i32 7, i32 9}
!432 = !{i32 6, !"g_uPointLightTotalCount", i32 3, i32 256, i32 7, i32 5}
!433 = !{i32 6, !"g_uSpotLightTotalCount", i32 3, i32 260, i32 7, i32 5}
!434 = !{i32 6, !"g_uSunLightTotalCount", i32 3, i32 264, i32 7, i32 5}
!435 = !{i32 6, !"g_uAmbientLightEnabled", i32 3, i32 268, i32 7, i32 5}
!436 = !{i32 6, !"g_fAmbientDepthRampDistance", i32 3, i32 272, i32 7, i32 9}
!437 = !{i32 6, !"g_fAmbientDepthRampFeather", i32 3, i32 276, i32 7, i32 9}
!438 = !{i32 6, !"g_fAmbientDepthRampIntensityNear", i32 3, i32 280, i32 7, i32 9}
!439 = !{i32 6, !"g_fAmbientDepthRampIntensityFar", i32 3, i32 284, i32 7, i32 9}
!440 = !{i32 6, !"g_vVolumeLightDimensions", i32 3, i32 288, i32 7, i32 5}
!441 = !{i32 6, !"g_vVolumeLightProjectionConstants", i32 3, i32 304, i32 7, i32 9}
!442 = !{i32 6, !"g_vHalfResVolumeLightProjectionConstants", i32 3, i32 320, i32 7, i32 9}
!443 = !{i32 6, !"g_vOnePerVolumeLightDimensions", i32 3, i32 336, i32 7, i32 9}
!444 = !{i32 6, !"g_vVolumeLightXYToTileXY", i32 3, i32 352, i32 7, i32 9}
!445 = !{i32 6, !"g_vVolumeLightDepthResolve", i32 3, i32 368, i32 7, i32 9}
!446 = !{i32 6, !"g_fVolumeLightOnePerDepthMinusOne", i32 3, i32 380, i32 7, i32 9}
!447 = !{i32 6, !"g_vVolumeLightNearSplit0Far", i32 3, i32 384, i32 7, i32 9}
!448 = !{i32 6, !"g_fVolumeLightKernelWidth", i32 3, i32 396, i32 7, i32 9}
!449 = !{i32 6, !"g_fVolumeLightSamplingBias", i32 3, i32 400, i32 7, i32 9}
!450 = !{i32 6, !"g_uRTContactShadowRayCount", i32 3, i32 404, i32 7, i32 5}
!451 = !{i32 8, !452, !453}
!452 = !{i32 6, !"g_fEnvReflectionEdgeLength", i32 3, i32 0, i32 7, i32 9}
!453 = !{i32 6, !"g_fEnvReflectionMipCount", i32 3, i32 4, i32 7, i32 9}
!454 = !{i32 188, !455, !456, !457, !458, !459, !460, !461, !462, !463, !464, !465, !466, !467, !468, !469, !470, !471, !472, !473, !474}
!455 = !{i32 6, !"g_atmosphere_vSunDir", i32 3, i32 0, i32 7, i32 9}
!456 = !{i32 6, !"g_atmosphere_vSunE", i32 3, i32 16, i32 7, i32 9}
!457 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants1", i32 3, i32 32, i32 7, i32 9}
!458 = !{i32 6, !"g_atmosphere_vPhaseSchlickConstants2", i32 3, i32 40, i32 7, i32 9}
!459 = !{i32 6, !"g_atmosphere_vPhaseBlend", i32 3, i32 48, i32 7, i32 9}
!460 = !{i32 6, !"g_atmosphere_vPhaseG", i32 3, i32 52, i32 7, i32 9}
!461 = !{i32 6, !"g_atmosphere_vFog", i32 3, i32 64, i32 7, i32 9}
!462 = !{i32 6, !"g_vFogColor", i32 3, i32 80, i32 7, i32 9}
!463 = !{i32 6, !"g_vFogColorOpposite", i32 3, i32 96, i32 7, i32 9}
!464 = !{i32 6, !"g_fFogExp", i32 3, i32 108, i32 7, i32 9}
!465 = !{i32 6, !"g_fFogGroundDensityAtViewer", i32 3, i32 112, i32 7, i32 9}
!466 = !{i32 6, !"g_fFogGroundHeight", i32 3, i32 116, i32 7, i32 9}
!467 = !{i32 6, !"g_fFogGroundFalloff", i32 3, i32 120, i32 7, i32 9}
!468 = !{i32 6, !"g_fFogGroundDensity", i32 3, i32 124, i32 7, i32 9}
!469 = !{i32 6, !"g_vFogGroundDensityMapRange", i32 3, i32 128, i32 7, i32 9}
!470 = !{i32 6, !"g_vFogGroundSimulationVelocityAndScale", i32 3, i32 144, i32 7, i32 9}
!471 = !{i32 6, !"g_fFogDepthRampDistance", i32 3, i32 156, i32 7, i32 9}
!472 = !{i32 6, !"g_fFogDepthRampFeather", i32 3, i32 160, i32 7, i32 9}
!473 = !{i32 6, !"g_fFogDepthRampIntensityNear", i32 3, i32 164, i32 7, i32 9}
!474 = !{i32 6, !"g_vFogAlbedo", i32 3, i32 176, i32 7, i32 9}
!475 = !{i32 40, !476, !477, !478, !479, !480, !481}
!476 = !{i32 6, !"g_vOnePerVolumeAtlasSize", i32 3, i32 0, i32 7, i32 9}
!477 = !{i32 6, !"g_uActiveVolumeCount", i32 3, i32 12, i32 7, i32 5}
!478 = !{i32 6, !"g_vAdditiveAmbient", i32 3, i32 16, i32 7, i32 9}
!479 = !{i32 6, !"g_uActiveDebugVolumeCount", i32 3, i32 28, i32 7, i32 5}
!480 = !{i32 6, !"g_fTransparentBalanceEnergy", i32 3, i32 32, i32 7, i32 9}
!481 = !{i32 6, !"g_fTransparentBalanceEnergyStrength", i32 3, i32 36, i32 7, i32 9}
!482 = !{i32 24576, !483}
!483 = !{i32 6, !"g_cbActiveVolumeTopLevelInfoArray", i32 3, i32 0}
!484 = !{i32 24576, !485}
!485 = !{i32 6, !"g_cbDebugVolumeTopLevelInfoArray", i32 3, i32 0}
!486 = !{i32 12, !487, !488, !489}
!487 = !{i32 6, !"g_fWorldSize", i32 3, i32 0, i32 7, i32 9}
!488 = !{i32 6, !"g_fCellSize", i32 3, i32 4, i32 7, i32 9}
!489 = !{i32 6, !"g_iCellCount", i32 3, i32 8, i32 7, i32 4}
!490 = !{i32 24, !491, !492, !493, !494, !495}
!491 = !{i32 6, !"g_fInvEnvironmentMapsPerRow", i32 3, i32 0, i32 7, i32 9}
!492 = !{i32 6, !"g_fEnvironmentMapsPerRow", i32 3, i32 4, i32 7, i32 9}
!493 = !{i32 6, !"g_fEnvironmentMapColSize", i32 3, i32 8, i32 7, i32 9}
!494 = !{i32 6, !"g_fEnvironmentMapRowSize", i32 3, i32 12, i32 7, i32 9}
!495 = !{i32 6, !"g_fInvEnvironmentMapAtlasSize", i32 3, i32 16, i32 7, i32 9}
!496 = !{i32 48, !497, !498, !499}
!497 = !{i32 6, !"g_uStride0_uStride1_uOffset0_uOffset1", i32 3, i32 0, i32 7, i32 5}
!498 = !{i32 6, !"g_Tangent_Normal_Texcoord_Color", i32 3, i32 16, i32 7, i32 5}
!499 = !{i32 6, !"g_BIndices_BWeights", i32 3, i32 32, i32 7, i32 5}
!500 = !{i32 40, !501}
!501 = !{i32 6, !"g_cbRaytracingHit", i32 3, i32 0}
!502 = !{i32 24, !503, !504, !505, !506, !507, !508}
!503 = !{i32 6, !"g_fClampReflectionIntensity", i32 3, i32 0, i32 7, i32 9}
!504 = !{i32 6, !"g_fRTReflectionAddRays", i32 3, i32 4, i32 7, i32 9}
!505 = !{i32 6, !"g_uRTReflectionRayCount", i32 3, i32 8, i32 7, i32 5}
!506 = !{i32 6, !"g_fRTZeroRayReflectance", i32 3, i32 12, i32 7, i32 9}
!507 = !{i32 6, !"g_fRTMaxRayReflectance", i32 3, i32 16, i32 7, i32 9}
!508 = !{i32 6, !"g_fRTMaxRayRoughness", i32 3, i32 20, i32 7, i32 9}
!509 = !{i32 1, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !510, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !510, void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !517, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !510, void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !510, void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !517, void ()* @"\01?reflectionRayGeneration@@YAXXZ", !518}
!510 = !{!511, !513, !515}
!511 = !{i32 1, !512, !512}
!512 = !{}
!513 = !{i32 2, !514, !512}
!514 = !{i32 4, !"SV_RayPayload"}
!515 = !{i32 0, !516, !512}
!516 = !{i32 4, !"SV_IntersectionAttributes"}
!517 = !{!511, !513}
!518 = !{!511}
!519 = !{null, !"", null, !3, !520}
!520 = !{i32 0, i64 65808, i32 5, !521}
!521 = !{i32 0}
!522 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !523}
!523 = !{i32 8, i32 9, i32 6, i32 4, i32 7, i32 8, i32 5, !521}
!524 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?reflectionClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !525}
!525 = !{i32 8, i32 10, i32 6, i32 4, i32 7, i32 8, i32 5, !521}
!526 = !{void (%struct.HitData*)* @"\01?reflectionMiss@@YAXUHitData@@@Z", !"\01?reflectionMiss@@YAXUHitData@@@Z", null, null, !527}
!527 = !{i32 8, i32 11, i32 6, i32 4, i32 5, !521}
!528 = !{void ()* @"\01?reflectionRayGeneration@@YAXXZ", !"\01?reflectionRayGeneration@@YAXXZ", null, null, !529}
!529 = !{i32 8, i32 7, i32 5, !521}
!530 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowAlphaTestAnyHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !523}
!531 = !{void (%struct.HitData*, %struct.IntersectionAttributes*)* @"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", !"\01?shadowClosestHit@@YAXUHitData@@UIntersectionAttributes@@@Z", null, null, !525}
!532 = !{void (%struct.HitData*)* @"\01?shadowMiss@@YAXUHitData@@@Z", !"\01?shadowMiss@@YAXUHitData@@@Z", null, null, !527}
!533 = !{!534, !534, i64 0}
!534 = !{!"omnipotent char", !535, i64 0}
!535 = !{!"Simple C/C++ TBAA"}
!536 = !{!537, !539}
!537 = distinct !{!537, !538, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z: %agg.result"}
!538 = distinct !{!538, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z"}
!539 = distinct !{!539, !540, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z: %agg.result"}
!540 = distinct !{!540, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z"}
!541 = !{!542, !542, i64 0}
!542 = !{!"int", !534, i64 0}
!543 = !{!544, !546}
!544 = distinct !{!544, !545, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z: %agg.result"}
!545 = distinct !{!545, !"\01?internal.getMaterialTexture@@YA?AV?$Texture2D@V?$vector@M$03@@@@II@Z"}
!546 = distinct !{!546, !547, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z: %agg.result"}
!547 = distinct !{!547, !"\01?internal.getMaterialColorMap@@YA?AV?$Texture2D@V?$vector@M$03@@@@I@Z"}
!548 = !{!549, !549, i64 0}
!549 = !{!"float", !534, i64 0}

