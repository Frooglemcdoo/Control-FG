;
; Input signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; no parameters
;
; Output signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; no parameters
; shader hash: 6ed8de8a94b7c999c068978bb23967e6
;
; Pipeline Runtime Information: 
;
;PSVRuntimeInfo:
; Compute Shader
; NumThreads=(8,8,1)
; MinimumExpectedWaveLaneCount: 0
; MaximumExpectedWaveLaneCount: 4294967295
; UsesViewID: false
; SigInputElements: 0
; SigOutputElements: 0
; SigPatchConstOrPrimElements: 0
; SigInputVectors: 0
; SigOutputVectors[0]: 0
; SigOutputVectors[1]: 0
; SigOutputVectors[2]: 0
; SigOutputVectors[3]: 0
; EntryFunctionName: main
;
;
; Buffer Definitions:
;
; cbuffer Parameters
; {
;
;   struct Parameters
;   {
;
;       float4 WorldFromViewRow0;                     ; Offset:    0
;       float4 WorldFromViewRow1;                     ; Offset:   16
;       float4 WorldFromViewRow2;                     ; Offset:   32
;       uint Width;                                   ; Offset:   48
;       uint Height;                                  ; Offset:   52
;       uint RayCapacity;                             ; Offset:   56
;       uint AdaptiveRayCount;                        ; Offset:   60
;       float4 ClipToViewColumn0;                     ; Offset:   64
;       float4 ClipToViewColumn1;                     ; Offset:   80
;       float4 ClipToViewColumn2;                     ; Offset:   96
;       float4 ClipToViewColumn3;                     ; Offset:  112
;       float2 NativeInvOutputRes;                    ; Offset:  128
;       float2 Reserved;                              ; Offset:  136
;   
;   } Parameters;                                     ; Offset:    0 Size:   144
;
; }
;
;
; Resource Bindings:
;
; Name                                 Type  Format         Dim      ID      HLSL Bind  Count
; ------------------------------ ---------- ------- ----------- ------- -------------- ------
; Parameters                        cbuffer      NA          NA     CB0            cb0     1
; NativeMaterialId                  texture     u32     2darray      T0             t0     1
; NativePositionTexcoordY           texture     f32     2darray      T1             t1     1
; NativeClipDepth                   texture     f32          2d      T2             t2     1
; Distance                              UAV     f32     2darray      U0             u0     1
; Status                                UAV     u32     2darray      U1             u1     1
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%dx.types.Handle = type { i8* }
%dx.types.Dimensions = type { i32, i32, i32, i32 }
%dx.types.CBufRet.i32 = type { i32, i32, i32, i32 }
%dx.types.ResRet.i32 = type { i32, i32, i32, i32, i32 }
%dx.types.CBufRet.f32 = type { float, float, float, float }
%dx.types.ResRet.f32 = type { float, float, float, float, i32 }
%"class.Texture2DArray<unsigned int>" = type { i32, %"class.Texture2DArray<unsigned int>::mips_type" }
%"class.Texture2DArray<unsigned int>::mips_type" = type { i32 }
%"class.Texture2DArray<vector<float, 4> >" = type { <4 x float>, %"class.Texture2DArray<vector<float, 4> >::mips_type" }
%"class.Texture2DArray<vector<float, 4> >::mips_type" = type { i32 }
%"class.Texture2D<float>" = type { float, %"class.Texture2D<float>::mips_type" }
%"class.Texture2D<float>::mips_type" = type { i32 }
%"class.RWTexture2DArray<float>" = type { float }
%"class.RWTexture2DArray<unsigned int>" = type { i32 }
%Parameters = type { <4 x float>, <4 x float>, <4 x float>, i32, i32, i32, i32, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <2 x float>, <2 x float> }

define void @main() {
  %1 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 1, i32 1, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %2 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %3 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 2, i32 2, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %4 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 1, i32 1, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %5 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 0, i32 0, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %6 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 2, i32 0, i32 0, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %7 = call i32 @dx.op.threadId.i32(i32 93, i32 0)  ; ThreadId(component)
  %8 = call i32 @dx.op.threadId.i32(i32 93, i32 1)  ; ThreadId(component)
  %9 = call i32 @dx.op.threadId.i32(i32 93, i32 2)  ; ThreadId(component)
  %10 = call %dx.types.Dimensions @dx.op.getDimensions(i32 72, %dx.types.Handle %2, i32 0)  ; GetDimensions(handle,mipLevel)
  %11 = extractvalue %dx.types.Dimensions %10, 0
  %12 = extractvalue %dx.types.Dimensions %10, 1
  %13 = extractvalue %dx.types.Dimensions %10, 2
  %14 = call %dx.types.Dimensions @dx.op.getDimensions(i32 72, %dx.types.Handle %1, i32 0)  ; GetDimensions(handle,mipLevel)
  %15 = extractvalue %dx.types.Dimensions %14, 0
  %16 = extractvalue %dx.types.Dimensions %14, 1
  %17 = extractvalue %dx.types.Dimensions %14, 2
  %18 = icmp uge i32 %7, %11
  %19 = icmp uge i32 %8, %12
  %20 = icmp uge i32 %9, %13
  %21 = or i1 %18, %19
  %22 = or i1 %20, %21
  br i1 %22, label %344, label %23

; <label>:23                                      ; preds = %0
  %24 = icmp uge i32 %7, %15
  %25 = icmp uge i32 %8, %16
  %26 = icmp uge i32 %9, %17
  %27 = or i1 %24, %25
  %28 = or i1 %26, %27
  br i1 %28, label %344, label %29

; <label>:29                                      ; preds = %23
  call void @dx.op.textureStore.f32(i32 67, %dx.types.Handle %2, i32 %7, i32 %8, i32 %9, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %1, i32 %7, i32 %8, i32 %9, i32 0, i32 0, i32 0, i32 0, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  %30 = call %dx.types.Dimensions @dx.op.getDimensions(i32 72, %dx.types.Handle %5, i32 0)  ; GetDimensions(handle,mipLevel)
  %31 = extractvalue %dx.types.Dimensions %30, 0
  %32 = extractvalue %dx.types.Dimensions %30, 1
  %33 = extractvalue %dx.types.Dimensions %30, 2
  %34 = call %dx.types.Dimensions @dx.op.getDimensions(i32 72, %dx.types.Handle %4, i32 0)  ; GetDimensions(handle,mipLevel)
  %35 = extractvalue %dx.types.Dimensions %34, 0
  %36 = extractvalue %dx.types.Dimensions %34, 1
  %37 = extractvalue %dx.types.Dimensions %34, 2
  %38 = call %dx.types.Dimensions @dx.op.getDimensions(i32 72, %dx.types.Handle %3, i32 0)  ; GetDimensions(handle,mipLevel)
  %39 = extractvalue %dx.types.Dimensions %38, 0
  %40 = extractvalue %dx.types.Dimensions %38, 1
  %41 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %6, i32 3)  ; CBufferLoadLegacy(handle,regIndex)
  %42 = extractvalue %dx.types.CBufRet.i32 %41, 0
  %43 = icmp eq i32 %42, 0
  br i1 %43, label %344, label %44

; <label>:44                                      ; preds = %29
  %45 = extractvalue %dx.types.CBufRet.i32 %41, 1
  %46 = icmp eq i32 %45, 0
  br i1 %46, label %344, label %47

; <label>:47                                      ; preds = %44
  %48 = extractvalue %dx.types.CBufRet.i32 %41, 2
  %49 = icmp ugt i32 %48, 32
  br i1 %49, label %344, label %50

; <label>:50                                      ; preds = %47
  %51 = extractvalue %dx.types.CBufRet.i32 %41, 3
  %52 = icmp ult i32 %51, 2
  %53 = icmp eq i32 %11, %42
  %54 = and i1 %52, %53
  %55 = icmp eq i32 %12, %45
  %56 = and i1 %54, %55
  %57 = icmp eq i32 %15, %11
  %58 = and i1 %56, %57
  br i1 %58, label %59, label %344

; <label>:59                                      ; preds = %50
  %60 = icmp eq i32 %16, %12
  %61 = icmp eq i32 %17, %13
  %62 = and i1 %61, %60
  %63 = icmp eq i32 %31, %11
  %64 = and i1 %62, %63
  %65 = icmp eq i32 %32, %12
  %66 = and i1 %64, %65
  %67 = icmp eq i32 %35, %11
  %68 = and i1 %66, %67
  %69 = icmp eq i32 %36, %12
  %70 = and i1 %68, %69
  %71 = icmp eq i32 %39, %11
  %72 = and i1 %70, %71
  %73 = icmp eq i32 %40, %12
  %74 = and i1 %72, %73
  %75 = xor i1 %74, true
  %76 = icmp ult i32 %33, %48
  %77 = or i1 %76, %75
  %78 = icmp ult i32 %37, %48
  %79 = or i1 %77, %78
  %80 = icmp ult i32 %13, %48
  %81 = or i1 %79, %80
  br i1 %81, label %344, label %82

; <label>:82                                      ; preds = %59
  %83 = icmp ult i32 %9, %48
  br i1 %83, label %85, label %84

; <label>:84                                      ; preds = %82
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %1, i32 %7, i32 %8, i32 %9, i32 3, i32 3, i32 3, i32 3, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  br label %344

; <label>:85                                      ; preds = %82
  %86 = icmp ne i32 %51, 0
  %87 = icmp ne i32 %9, 0
  %88 = and i1 %87, %86
  br i1 %88, label %89, label %102

; <label>:89                                      ; preds = %85
  br label %90

; <label>:90                                      ; preds = %98, %89
  %91 = phi i32 [ %99, %98 ], [ 0, %89 ]
  %92 = call %dx.types.ResRet.i32 @dx.op.textureLoad.i32(i32 66, %dx.types.Handle %5, i32 0, i32 %7, i32 %8, i32 %91, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %93 = extractvalue %dx.types.ResRet.i32 %92, 0
  %94 = icmp eq i32 %93, 65534
  br i1 %94, label %95, label %96

; <label>:95                                      ; preds = %90
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %1, i32 %7, i32 %8, i32 %9, i32 3, i32 3, i32 3, i32 3, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  br label %344

; <label>:96                                      ; preds = %90
  %97 = icmp ugt i32 %93, 65535
  br i1 %97, label %343, label %98

; <label>:98                                      ; preds = %96
  %99 = add nuw i32 %91, 1
  %100 = icmp ult i32 %99, %9
  br i1 %100, label %90, label %101

; <label>:101                                     ; preds = %98
  br label %102

; <label>:102                                     ; preds = %101, %85
  %103 = call %dx.types.ResRet.i32 @dx.op.textureLoad.i32(i32 66, %dx.types.Handle %5, i32 0, i32 %7, i32 %8, i32 %9, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %104 = extractvalue %dx.types.ResRet.i32 %103, 0
  %105 = icmp eq i32 %104, 65534
  br i1 %105, label %111, label %106

; <label>:106                                     ; preds = %102
  %107 = icmp eq i32 %104, 65535
  br i1 %107, label %111, label %108

; <label>:108                                     ; preds = %106
  %109 = icmp ult i32 %104, 65534
  %110 = zext i1 %109 to i32
  br label %111

; <label>:111                                     ; preds = %108, %106, %102
  %112 = phi i32 [ %110, %108 ], [ 3, %102 ], [ 2, %106 ]
  %113 = icmp eq i32 %112, 1
  br i1 %113, label %115, label %114

; <label>:114                                     ; preds = %111
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %1, i32 %7, i32 %8, i32 %9, i32 %112, i32 %112, i32 %112, i32 %112, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  br label %344

; <label>:115                                     ; preds = %111
  %116 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %117 = extractvalue %dx.types.CBufRet.f32 %116, 0
  %118 = extractvalue %dx.types.CBufRet.f32 %116, 1
  %119 = extractvalue %dx.types.CBufRet.f32 %116, 2
  %120 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %121 = extractvalue %dx.types.CBufRet.f32 %120, 0
  %122 = extractvalue %dx.types.CBufRet.f32 %120, 1
  %123 = extractvalue %dx.types.CBufRet.f32 %120, 2
  %124 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %125 = extractvalue %dx.types.CBufRet.f32 %124, 0
  %126 = extractvalue %dx.types.CBufRet.f32 %124, 1
  %127 = extractvalue %dx.types.CBufRet.f32 %124, 2
  %128 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 4)  ; CBufferLoadLegacy(handle,regIndex)
  %129 = extractvalue %dx.types.CBufRet.f32 %128, 0
  %130 = extractvalue %dx.types.CBufRet.f32 %128, 1
  %131 = extractvalue %dx.types.CBufRet.f32 %128, 2
  %132 = extractvalue %dx.types.CBufRet.f32 %128, 3
  %133 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 5)  ; CBufferLoadLegacy(handle,regIndex)
  %134 = extractvalue %dx.types.CBufRet.f32 %133, 0
  %135 = extractvalue %dx.types.CBufRet.f32 %133, 1
  %136 = extractvalue %dx.types.CBufRet.f32 %133, 2
  %137 = extractvalue %dx.types.CBufRet.f32 %133, 3
  %138 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %139 = extractvalue %dx.types.CBufRet.f32 %138, 0
  %140 = extractvalue %dx.types.CBufRet.f32 %138, 1
  %141 = extractvalue %dx.types.CBufRet.f32 %138, 2
  %142 = extractvalue %dx.types.CBufRet.f32 %138, 3
  %143 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %144 = extractvalue %dx.types.CBufRet.f32 %143, 0
  %145 = extractvalue %dx.types.CBufRet.f32 %143, 1
  %146 = extractvalue %dx.types.CBufRet.f32 %143, 2
  %147 = extractvalue %dx.types.CBufRet.f32 %143, 3
  %148 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %3, i32 0, i32 %7, i32 %8, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %149 = extractvalue %dx.types.ResRet.f32 %148, 0
  %150 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %6, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %151 = extractvalue %dx.types.CBufRet.f32 %150, 1
  %152 = extractvalue %dx.types.CBufRet.f32 %150, 0
  %153 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %152)  ; IsFinite(value)
  br i1 %153, label %154, label %261

; <label>:154                                     ; preds = %115
  %155 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %151)  ; IsFinite(value)
  %156 = xor i1 %155, true
  %157 = fcmp fast ole float %152, 0.000000e+00
  %158 = or i1 %157, %156
  %159 = fcmp fast ole float %151, 0.000000e+00
  %160 = or i1 %159, %158
  br i1 %160, label %261, label %161

; <label>:161                                     ; preds = %154
  %162 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %149)  ; IsFinite(value)
  %163 = xor i1 %162, true
  %164 = fcmp fast olt float %149, 0.000000e+00
  %165 = or i1 %164, %163
  %166 = fcmp fast ogt float %149, 1.000000e+00
  %167 = or i1 %166, %165
  br i1 %167, label %261, label %168

; <label>:168                                     ; preds = %161
  %169 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %129)  ; IsFinite(value)
  br i1 %169, label %170, label %176

; <label>:170                                     ; preds = %168
  %171 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %130)  ; IsFinite(value)
  br i1 %171, label %172, label %176

; <label>:172                                     ; preds = %170
  %173 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %131)  ; IsFinite(value)
  br i1 %173, label %174, label %176

; <label>:174                                     ; preds = %172
  %175 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %132)  ; IsFinite(value)
  br label %176

; <label>:176                                     ; preds = %174, %172, %170, %168
  %177 = phi i1 [ false, %172 ], [ false, %170 ], [ false, %168 ], [ %175, %174 ]
  br i1 %177, label %178, label %261

; <label>:178                                     ; preds = %176
  %179 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %134)  ; IsFinite(value)
  br i1 %179, label %180, label %186

; <label>:180                                     ; preds = %178
  %181 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %135)  ; IsFinite(value)
  br i1 %181, label %182, label %186

; <label>:182                                     ; preds = %180
  %183 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %136)  ; IsFinite(value)
  br i1 %183, label %184, label %186

; <label>:184                                     ; preds = %182
  %185 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %137)  ; IsFinite(value)
  br label %186

; <label>:186                                     ; preds = %184, %182, %180, %178
  %187 = phi i1 [ false, %182 ], [ false, %180 ], [ false, %178 ], [ %185, %184 ]
  br i1 %187, label %188, label %261

; <label>:188                                     ; preds = %186
  %189 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %139)  ; IsFinite(value)
  br i1 %189, label %190, label %196

; <label>:190                                     ; preds = %188
  %191 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %140)  ; IsFinite(value)
  br i1 %191, label %192, label %196

; <label>:192                                     ; preds = %190
  %193 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %141)  ; IsFinite(value)
  br i1 %193, label %194, label %196

; <label>:194                                     ; preds = %192
  %195 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %142)  ; IsFinite(value)
  br label %196

; <label>:196                                     ; preds = %194, %192, %190, %188
  %197 = phi i1 [ false, %192 ], [ false, %190 ], [ false, %188 ], [ %195, %194 ]
  br i1 %197, label %198, label %261

; <label>:198                                     ; preds = %196
  %199 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %144)  ; IsFinite(value)
  br i1 %199, label %200, label %206

; <label>:200                                     ; preds = %198
  %201 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %145)  ; IsFinite(value)
  br i1 %201, label %202, label %206

; <label>:202                                     ; preds = %200
  %203 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %146)  ; IsFinite(value)
  br i1 %203, label %204, label %206

; <label>:204                                     ; preds = %202
  %205 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %147)  ; IsFinite(value)
  br label %206

; <label>:206                                     ; preds = %204, %202, %200, %198
  %207 = phi i1 [ false, %202 ], [ false, %200 ], [ false, %198 ], [ %205, %204 ]
  br i1 %207, label %208, label %261

; <label>:208                                     ; preds = %206
  %209 = uitofp i32 %7 to float
  %210 = fadd fast float %209, 5.000000e-01
  %211 = fmul fast float %210, 2.000000e+00
  %212 = fmul fast float %211, %152
  %213 = fadd fast float %212, -1.000000e+00
  %214 = uitofp i32 %8 to float
  %215 = fadd fast float %214, 5.000000e-01
  %216 = fmul fast float %215, 2.000000e+00
  %217 = fmul fast float %216, %151
  %218 = fsub fast float 1.000000e+00, %217
  %219 = fmul fast float %213, %132
  %220 = fmul fast float %218, %137
  %221 = fmul fast float %149, %142
  %222 = fadd fast float %221, %147
  %223 = fadd fast float %222, %220
  %224 = fadd fast float %223, %219
  %225 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %224)  ; IsFinite(value)
  %226 = xor i1 %225, true
  %227 = fcmp fast oeq float %224, 0.000000e+00
  %228 = or i1 %227, %226
  br i1 %228, label %261, label %229

; <label>:229                                     ; preds = %208
  %230 = fmul fast float %213, %131
  %231 = fmul fast float %218, %136
  %232 = fmul fast float %149, %141
  %233 = fadd fast float %232, %146
  %234 = fadd fast float %233, %231
  %235 = fadd fast float %234, %230
  %236 = fmul fast float %213, %130
  %237 = fmul fast float %218, %135
  %238 = fmul fast float %149, %140
  %239 = fadd fast float %238, %145
  %240 = fadd fast float %239, %237
  %241 = fadd fast float %240, %236
  %242 = fmul fast float %213, %129
  %243 = fmul fast float %218, %134
  %244 = fmul fast float %149, %139
  %245 = fadd fast float %244, %144
  %246 = fadd fast float %245, %243
  %247 = fadd fast float %246, %242
  %248 = fdiv fast float 1.000000e+00, %224
  %249 = fmul fast float %248, %247
  %250 = fmul fast float %248, %241
  %251 = fmul fast float %248, %235
  %252 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %249)  ; IsFinite(value)
  br i1 %252, label %253, label %261

; <label>:253                                     ; preds = %229
  %254 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %250)  ; IsFinite(value)
  br i1 %254, label %255, label %261

; <label>:255                                     ; preds = %253
  %256 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %251)  ; IsFinite(value)
  %257 = select i1 %256, float %249, float 0.000000e+00
  %258 = select i1 %256, float %250, float 0.000000e+00
  %259 = select i1 %256, float %251, float 0.000000e+00
  %260 = xor i1 %256, true
  br label %261

; <label>:261                                     ; preds = %255, %253, %229, %208, %206, %196, %186, %176, %161, %154, %115
  %262 = phi float [ 0.000000e+00, %253 ], [ 0.000000e+00, %229 ], [ 0.000000e+00, %208 ], [ 0.000000e+00, %206 ], [ 0.000000e+00, %196 ], [ 0.000000e+00, %186 ], [ 0.000000e+00, %176 ], [ 0.000000e+00, %161 ], [ 0.000000e+00, %154 ], [ 0.000000e+00, %115 ], [ %257, %255 ]
  %263 = phi float [ 0.000000e+00, %253 ], [ 0.000000e+00, %229 ], [ 0.000000e+00, %208 ], [ 0.000000e+00, %206 ], [ 0.000000e+00, %196 ], [ 0.000000e+00, %186 ], [ 0.000000e+00, %176 ], [ 0.000000e+00, %161 ], [ 0.000000e+00, %154 ], [ 0.000000e+00, %115 ], [ %258, %255 ]
  %264 = phi float [ 0.000000e+00, %253 ], [ 0.000000e+00, %229 ], [ 0.000000e+00, %208 ], [ 0.000000e+00, %206 ], [ 0.000000e+00, %196 ], [ 0.000000e+00, %186 ], [ 0.000000e+00, %176 ], [ 0.000000e+00, %161 ], [ 0.000000e+00, %154 ], [ 0.000000e+00, %115 ], [ %259, %255 ]
  %265 = phi i1 [ true, %253 ], [ true, %229 ], [ true, %208 ], [ true, %206 ], [ true, %196 ], [ true, %186 ], [ true, %176 ], [ true, %161 ], [ true, %154 ], [ true, %115 ], [ %260, %255 ]
  br i1 %265, label %344, label %266

; <label>:266                                     ; preds = %261
  %267 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %4, i32 0, i32 %7, i32 %8, i32 %9, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %268 = extractvalue %dx.types.ResRet.f32 %267, 0
  %269 = extractvalue %dx.types.ResRet.f32 %267, 1
  %270 = extractvalue %dx.types.ResRet.f32 %267, 2
  br i1 %105, label %276, label %271

; <label>:271                                     ; preds = %266
  %272 = icmp eq i32 %104, 65535
  br i1 %272, label %276, label %273

; <label>:273                                     ; preds = %271
  %274 = icmp ult i32 %104, 65534
  %275 = zext i1 %274 to i32
  br label %276

; <label>:276                                     ; preds = %273, %271, %266
  %277 = phi i32 [ %275, %273 ], [ 3, %266 ], [ 2, %271 ]
  %278 = icmp eq i32 %277, 1
  br i1 %278, label %279, label %340

; <label>:279                                     ; preds = %276
  %280 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %262)  ; IsFinite(value)
  br i1 %280, label %281, label %340

; <label>:281                                     ; preds = %279
  %282 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %263)  ; IsFinite(value)
  br i1 %282, label %283, label %340

; <label>:283                                     ; preds = %281
  %284 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %264)  ; IsFinite(value)
  br i1 %284, label %285, label %340

; <label>:285                                     ; preds = %283
  %286 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %268)  ; IsFinite(value)
  br i1 %286, label %287, label %340

; <label>:287                                     ; preds = %285
  %288 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %269)  ; IsFinite(value)
  br i1 %288, label %289, label %340

; <label>:289                                     ; preds = %287
  %290 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %270)  ; IsFinite(value)
  br i1 %290, label %291, label %340

; <label>:291                                     ; preds = %289
  %292 = fsub fast float %268, %262
  %293 = fsub fast float %269, %263
  %294 = fsub fast float %270, %264
  %295 = fmul fast float %292, %117
  %296 = fmul fast float %293, %118
  %297 = fadd fast float %295, %296
  %298 = fmul fast float %294, %119
  %299 = fadd fast float %297, %298
  %300 = fmul fast float %292, %121
  %301 = fmul fast float %293, %122
  %302 = fadd fast float %300, %301
  %303 = fmul fast float %294, %123
  %304 = fadd fast float %302, %303
  %305 = fmul fast float %292, %125
  %306 = fmul fast float %293, %126
  %307 = fadd fast float %305, %306
  %308 = fmul fast float %294, %127
  %309 = fadd fast float %307, %308
  %310 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %299)  ; IsFinite(value)
  br i1 %310, label %311, label %340

; <label>:311                                     ; preds = %291
  %312 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %304)  ; IsFinite(value)
  br i1 %312, label %313, label %340

; <label>:313                                     ; preds = %311
  %314 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %309)  ; IsFinite(value)
  br i1 %314, label %315, label %340

; <label>:315                                     ; preds = %313
  %316 = call float @dx.op.unary.f32(i32 6, float %299)  ; FAbs(value)
  %317 = call float @dx.op.unary.f32(i32 6, float %304)  ; FAbs(value)
  %318 = fcmp fast ogt float %317, %316
  %319 = select i1 %318, float %317, float %316
  %320 = call float @dx.op.unary.f32(i32 6, float %309)  ; FAbs(value)
  %321 = fcmp fast ogt float %320, %319
  %322 = select i1 %321, float %320, float %319
  %323 = fcmp fast ogt float %322, 0.000000e+00
  br i1 %323, label %324, label %335

; <label>:324                                     ; preds = %315
  %325 = fdiv fast float %299, %322
  %326 = fdiv fast float %304, %322
  %327 = fdiv fast float %309, %322
  %328 = fmul fast float %325, %325
  %329 = fmul fast float %326, %326
  %330 = fadd fast float %329, %328
  %331 = fmul fast float %327, %327
  %332 = fadd fast float %330, %331
  %333 = call float @dx.op.unary.f32(i32 24, float %332)  ; Sqrt(value)
  %334 = fmul fast float %333, %322
  br label %335

; <label>:335                                     ; preds = %324, %315
  %336 = phi float [ %334, %324 ], [ 0.000000e+00, %315 ]
  %337 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %336)  ; IsFinite(value)
  %338 = zext i1 %337 to i32
  %339 = select i1 %337, float %336, float 0.000000e+00
  br label %340

; <label>:340                                     ; preds = %335, %313, %311, %291, %289, %287, %285, %283, %281, %279, %276
  %341 = phi i32 [ %277, %276 ], [ 0, %313 ], [ 0, %311 ], [ 0, %291 ], [ 0, %289 ], [ 0, %287 ], [ 0, %285 ], [ 0, %283 ], [ 0, %281 ], [ 0, %279 ], [ %338, %335 ]
  %342 = phi float [ 0.000000e+00, %276 ], [ 0.000000e+00, %313 ], [ 0.000000e+00, %311 ], [ 0.000000e+00, %291 ], [ 0.000000e+00, %289 ], [ 0.000000e+00, %287 ], [ 0.000000e+00, %285 ], [ 0.000000e+00, %283 ], [ 0.000000e+00, %281 ], [ 0.000000e+00, %279 ], [ %339, %335 ]
  call void @dx.op.textureStore.f32(i32 67, %dx.types.Handle %2, i32 %7, i32 %8, i32 %9, float %342, float %342, float %342, float %342, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %1, i32 %7, i32 %8, i32 %9, i32 %341, i32 %341, i32 %341, i32 %341, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  br label %344

; <label>:343                                     ; preds = %96
  br label %344

; <label>:344                                     ; preds = %343, %340, %261, %114, %95, %84, %59, %50, %47, %44, %29, %23, %0
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @dx.op.threadId.i32(i32, i32) #0

; Function Attrs: nounwind readonly
declare %dx.types.Dimensions @dx.op.getDimensions(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind
declare void @dx.op.textureStore.f32(i32, %dx.types.Handle, i32, i32, i32, float, float, float, float, i8) #2

; Function Attrs: nounwind
declare void @dx.op.textureStore.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i32, i8) #2

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.i32 @dx.op.textureLoad.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i32) #1

; Function Attrs: nounwind readnone
declare i1 @dx.op.isSpecialFloat.f32(i32, float) #0

; Function Attrs: nounwind readnone
declare float @dx.op.unary.f32(i32, float) #0

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandle(i32, i8, i32, i32, i1) #1

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!2}
!dx.shaderModel = !{!3}
!dx.resources = !{!4}
!dx.entryPoints = !{!16}

!0 = !{!"dxc(private) 1.9.0.1 (0d3ee6b5)"}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 9}
!3 = !{!"cs", i32 6, i32 0}
!4 = !{!5, !11, !14, null}
!5 = !{!6, !8, !10}
!6 = !{i32 0, %"class.Texture2DArray<unsigned int>"* undef, !"", i32 0, i32 0, i32 1, i32 7, i32 0, !7}
!7 = !{i32 0, i32 5}
!8 = !{i32 1, %"class.Texture2DArray<vector<float, 4> >"* undef, !"", i32 0, i32 1, i32 1, i32 7, i32 0, !9}
!9 = !{i32 0, i32 9}
!10 = !{i32 2, %"class.Texture2D<float>"* undef, !"", i32 0, i32 2, i32 1, i32 2, i32 0, !9}
!11 = !{!12, !13}
!12 = !{i32 0, %"class.RWTexture2DArray<float>"* undef, !"", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !9}
!13 = !{i32 1, %"class.RWTexture2DArray<unsigned int>"* undef, !"", i32 0, i32 1, i32 1, i32 7, i1 false, i1 false, i1 false, !7}
!14 = !{!15}
!15 = !{i32 0, %Parameters* undef, !"", i32 0, i32 0, i32 1, i32 144, null}
!16 = !{void ()* @main, !"main", null, !4, !17}
!17 = !{i32 4, !18}
!18 = !{i32 8, i32 8, i32 1}
