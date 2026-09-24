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
; shader hash: cb0616adf4b2d3ea81ad310beb45cffa
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
;   
;   } Parameters;                                     ; Offset:    0 Size:    64
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
; PrimaryPositionView               texture     f32          2d      T2             t2     1
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
%"class.Texture2D<vector<float, 4> >" = type { <4 x float>, %"class.Texture2D<vector<float, 4> >::mips_type" }
%"class.Texture2D<vector<float, 4> >::mips_type" = type { i32 }
%"class.RWTexture2DArray<float>" = type { float }
%"class.RWTexture2DArray<unsigned int>" = type { i32 }
%Parameters = type { <4 x float>, <4 x float>, <4 x float>, i32, i32, i32, i32 }

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
  br i1 %22, label %209, label %23

; <label>:23                                      ; preds = %0
  %24 = icmp uge i32 %7, %15
  %25 = icmp uge i32 %8, %16
  %26 = icmp uge i32 %9, %17
  %27 = or i1 %24, %25
  %28 = or i1 %26, %27
  br i1 %28, label %209, label %29

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
  br i1 %43, label %209, label %44

; <label>:44                                      ; preds = %29
  %45 = extractvalue %dx.types.CBufRet.i32 %41, 1
  %46 = icmp eq i32 %45, 0
  br i1 %46, label %209, label %47

; <label>:47                                      ; preds = %44
  %48 = extractvalue %dx.types.CBufRet.i32 %41, 2
  %49 = icmp ugt i32 %48, 32
  br i1 %49, label %209, label %50

; <label>:50                                      ; preds = %47
  %51 = extractvalue %dx.types.CBufRet.i32 %41, 3
  %52 = icmp ult i32 %51, 2
  %53 = icmp eq i32 %11, %42
  %54 = and i1 %52, %53
  %55 = icmp eq i32 %12, %45
  %56 = and i1 %54, %55
  %57 = icmp eq i32 %15, %11
  %58 = and i1 %56, %57
  br i1 %58, label %59, label %209

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
  br i1 %81, label %209, label %82

; <label>:82                                      ; preds = %59
  %83 = icmp ult i32 %9, %48
  br i1 %83, label %85, label %84

; <label>:84                                      ; preds = %82
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %1, i32 %7, i32 %8, i32 %9, i32 3, i32 3, i32 3, i32 3, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  br label %209

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
  br label %209

; <label>:96                                      ; preds = %90
  %97 = icmp ugt i32 %93, 65535
  br i1 %97, label %208, label %98

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
  br label %209

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
  %128 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %3, i32 0, i32 %7, i32 %8, i32 undef, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %129 = extractvalue %dx.types.ResRet.f32 %128, 0
  %130 = extractvalue %dx.types.ResRet.f32 %128, 1
  %131 = extractvalue %dx.types.ResRet.f32 %128, 2
  %132 = call %dx.types.ResRet.f32 @dx.op.textureLoad.f32(i32 66, %dx.types.Handle %4, i32 0, i32 %7, i32 %8, i32 %9, i32 undef, i32 undef, i32 undef)  ; TextureLoad(srv,mipLevelOrSampleCount,coord0,coord1,coord2,offset0,offset1,offset2)
  %133 = extractvalue %dx.types.ResRet.f32 %132, 0
  %134 = extractvalue %dx.types.ResRet.f32 %132, 1
  %135 = extractvalue %dx.types.ResRet.f32 %132, 2
  br i1 %105, label %141, label %136

; <label>:136                                     ; preds = %115
  %137 = icmp eq i32 %104, 65535
  br i1 %137, label %141, label %138

; <label>:138                                     ; preds = %136
  %139 = icmp ult i32 %104, 65534
  %140 = zext i1 %139 to i32
  br label %141

; <label>:141                                     ; preds = %138, %136, %115
  %142 = phi i32 [ %140, %138 ], [ 3, %115 ], [ 2, %136 ]
  %143 = icmp eq i32 %142, 1
  br i1 %143, label %144, label %205

; <label>:144                                     ; preds = %141
  %145 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %129)  ; IsFinite(value)
  br i1 %145, label %146, label %205

; <label>:146                                     ; preds = %144
  %147 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %130)  ; IsFinite(value)
  br i1 %147, label %148, label %205

; <label>:148                                     ; preds = %146
  %149 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %131)  ; IsFinite(value)
  br i1 %149, label %150, label %205

; <label>:150                                     ; preds = %148
  %151 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %133)  ; IsFinite(value)
  br i1 %151, label %152, label %205

; <label>:152                                     ; preds = %150
  %153 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %134)  ; IsFinite(value)
  br i1 %153, label %154, label %205

; <label>:154                                     ; preds = %152
  %155 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %135)  ; IsFinite(value)
  br i1 %155, label %156, label %205

; <label>:156                                     ; preds = %154
  %157 = fsub fast float %133, %129
  %158 = fsub fast float %134, %130
  %159 = fsub fast float %135, %131
  %160 = fmul fast float %157, %117
  %161 = fmul fast float %158, %118
  %162 = fadd fast float %160, %161
  %163 = fmul fast float %159, %119
  %164 = fadd fast float %162, %163
  %165 = fmul fast float %157, %121
  %166 = fmul fast float %158, %122
  %167 = fadd fast float %165, %166
  %168 = fmul fast float %159, %123
  %169 = fadd fast float %167, %168
  %170 = fmul fast float %157, %125
  %171 = fmul fast float %158, %126
  %172 = fadd fast float %170, %171
  %173 = fmul fast float %159, %127
  %174 = fadd fast float %172, %173
  %175 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %164)  ; IsFinite(value)
  br i1 %175, label %176, label %205

; <label>:176                                     ; preds = %156
  %177 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %169)  ; IsFinite(value)
  br i1 %177, label %178, label %205

; <label>:178                                     ; preds = %176
  %179 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %174)  ; IsFinite(value)
  br i1 %179, label %180, label %205

; <label>:180                                     ; preds = %178
  %181 = call float @dx.op.unary.f32(i32 6, float %164)  ; FAbs(value)
  %182 = call float @dx.op.unary.f32(i32 6, float %169)  ; FAbs(value)
  %183 = fcmp fast ogt float %182, %181
  %184 = select i1 %183, float %182, float %181
  %185 = call float @dx.op.unary.f32(i32 6, float %174)  ; FAbs(value)
  %186 = fcmp fast ogt float %185, %184
  %187 = select i1 %186, float %185, float %184
  %188 = fcmp fast ogt float %187, 0.000000e+00
  br i1 %188, label %189, label %200

; <label>:189                                     ; preds = %180
  %190 = fdiv fast float %164, %187
  %191 = fdiv fast float %169, %187
  %192 = fdiv fast float %174, %187
  %193 = fmul fast float %190, %190
  %194 = fmul fast float %191, %191
  %195 = fadd fast float %194, %193
  %196 = fmul fast float %192, %192
  %197 = fadd fast float %195, %196
  %198 = call float @dx.op.unary.f32(i32 24, float %197)  ; Sqrt(value)
  %199 = fmul fast float %198, %187
  br label %200

; <label>:200                                     ; preds = %189, %180
  %201 = phi float [ %199, %189 ], [ 0.000000e+00, %180 ]
  %202 = call i1 @dx.op.isSpecialFloat.f32(i32 10, float %201)  ; IsFinite(value)
  %203 = zext i1 %202 to i32
  %204 = select i1 %202, float %201, float 0.000000e+00
  br label %205

; <label>:205                                     ; preds = %200, %178, %176, %156, %154, %152, %150, %148, %146, %144, %141
  %206 = phi i32 [ %142, %141 ], [ 0, %178 ], [ 0, %176 ], [ 0, %156 ], [ 0, %154 ], [ 0, %152 ], [ 0, %150 ], [ 0, %148 ], [ 0, %146 ], [ 0, %144 ], [ %203, %200 ]
  %207 = phi float [ 0.000000e+00, %141 ], [ 0.000000e+00, %178 ], [ 0.000000e+00, %176 ], [ 0.000000e+00, %156 ], [ 0.000000e+00, %154 ], [ 0.000000e+00, %152 ], [ 0.000000e+00, %150 ], [ 0.000000e+00, %148 ], [ 0.000000e+00, %146 ], [ 0.000000e+00, %144 ], [ %204, %200 ]
  call void @dx.op.textureStore.f32(i32 67, %dx.types.Handle %2, i32 %7, i32 %8, i32 %9, float %207, float %207, float %207, float %207, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  call void @dx.op.textureStore.i32(i32 67, %dx.types.Handle %1, i32 %7, i32 %8, i32 %9, i32 %206, i32 %206, i32 %206, i32 %206, i8 15)  ; TextureStore(srv,coord0,coord1,coord2,value0,value1,value2,value3,mask)
  br label %209

; <label>:208                                     ; preds = %96
  br label %209

; <label>:209                                     ; preds = %208, %205, %114, %95, %84, %59, %50, %47, %44, %29, %23, %0
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
declare %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32, %dx.types.Handle, i32) #1

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
!10 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* undef, !"", i32 0, i32 2, i32 1, i32 2, i32 0, !9}
!11 = !{!12, !13}
!12 = !{i32 0, %"class.RWTexture2DArray<float>"* undef, !"", i32 0, i32 0, i32 1, i32 7, i1 false, i1 false, i1 false, !9}
!13 = !{i32 1, %"class.RWTexture2DArray<unsigned int>"* undef, !"", i32 0, i32 1, i32 1, i32 7, i1 false, i1 false, i1 false, !7}
!14 = !{!15}
!15 = !{i32 0, %Parameters* undef, !"", i32 0, i32 0, i32 1, i32 64, null}
!16 = !{void ()* @main, !"main", null, !4, !17}
!17 = !{i32 4, !18}
!18 = !{i32 8, i32 8, i32 1}
