#pragma once
#include "rr_evaluation_inputs.h"
#include "rr_projection_validation.h"
struct RRNativeGuideBindings {
 ID3D12Resource* normal=nullptr;ID3D12Resource* diffuse=nullptr;ID3D12Resource* specular=nullptr;
 ID3D12Resource* responsivity=nullptr;
 bool projectionValid=false;
 float worldToView[16]{},viewToClip[16]{};
};
static bool RRNativeTakeGuides(const RREvaluationInputs& in,RRNativeGuideBindings& bindings) noexcept {
 if(!TryAcquireSRWLockExclusive(&rrLiveLock))return false;bool good=false;
 __try {
  auto* owner=rrLive;if(!owner||owner->stopped||!owner->prepared||owner->width!=in.width||owner->height!=in.height)__leave;
  for(std::size_t i=0;i<3;++i){
   const auto state=owner->policy.Inspect()[i];auto& slot=owner->slots[i];
   if(state.state!=control_rr::SlotState::Ready||state.key.frame!=in.frame||!slot.diffuseValid||slot.camera.engineFrame!=in.frame)continue;
   // The guide normal is already view-space. RenoDX default mode pairs that
   // with identity WorldToView AND identity ViewToClip matrices.
   for(unsigned row=0;row<4;++row)for(unsigned column=0;column<4;++column){
    const unsigned index=row*4+column;
    const float identity=(row==column)?1.0f:0.0f;
    bindings.worldToView[index]=identity;
    bindings.viewToClip[index]=identity;
   }
   // P1 isolates projection data on F. View-space normals retain identity WorldToView.
   // E and rejected snapshots keep the exact reference bindings above.
   if(control_rr::RRUserPresetValue()==control_rr::RRPresetF) {
    const bool validProjection=control_rr::CopyValidatedProjection(slot.camera.viewToClip,slot.camera.clipToView,bindings.viewToClip);
    bindings.projectionValid=validProjection;
    static unsigned long long projectionChecks=0;
    const auto check=++projectionChecks;
    if(check<=4||(check%240)==0||!validProjection)
     Log("RR_F_PROJECTION_P1 frame=%llu valid=%u mode=%s hit_distance=D1_gated",in.frame,unsigned(validProjection),validProjection?"native_projection_view_normals":"identity_fallback");
   }
   if(!owner->policy.TakeEvaluation({i,state.serial},state.key))__leave;
   bindings.normal=slot.normal;bindings.specular=slot.specular;bindings.diffuse=slot.diffuse;
   good=bindings.normal&&bindings.specular&&bindings.diffuse;break;
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){good=false;}
 ReleaseSRWLockExclusive(&rrLiveLock);return good;
}
#include "rr_guide_parameters.h"
struct RRNativeParameterApi {
 void* parameters;
 void Resource(const char* name,ID3D12Resource* value) const {
  using Fn=void (*)(void*,const char*,ID3D12Resource*);
  reinterpret_cast<Fn>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x51e40)(parameters,name,value);
 }
 void Pointer(const char* name,void* value) const {
  using Fn=void (*)(void*,const char*,void*);
  reinterpret_cast<Fn>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x52010)(parameters,name,value);
 }
 void Integer(const char* name,int value) const {
  using Fn=void (*)(void*,const char*,int);
  reinterpret_cast<Fn>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x51ef0)(parameters,name,value);
 }
 void Float(const char* name,float value) const {
  using Fn=void (*)(void*,const char*,float);
  reinterpret_cast<Fn>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x51ea0)(parameters,name,value);
 }
 bool ResourceEquals(const char* name,ID3D12Resource* expected) const {
  ID3D12Resource* actual=nullptr;const auto result=ngxGetResource(parameters,name,&actual);
  const bool okay=result==1&&actual==expected;
  if(!okay)Log("RR_PARAMETER_REJECT key=%s result=0x%08X expected=%p actual=%p",name,result,expected,actual);
  return okay;
 }
 bool PointerEquals(const char* name,const void* expected) const {
  using Fn=unsigned (*)(void*,const char*,void**);void* actual=nullptr;
  const auto result=reinterpret_cast<Fn>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x51d30)(parameters,name,&actual);
  const bool okay=result==1&&actual==expected;
  if(!okay)Log("RR_PARAMETER_REJECT key=%s result=0x%08X expected=%p actual=%p",name,result,expected,actual);
  return okay;
 }
 bool IntegerEquals(const char* name,int expected) const {
  int actual=0;const auto result=ngxGetInt(parameters,name,&actual);const bool okay=result==1&&actual==expected;
  if(!okay)Log("RR_PARAMETER_REJECT key=%s result=0x%08X expected=%d actual=%d",name,result,expected,actual);
  return okay;
 }
 bool FloatEquals(const char* name,float expected) const {
  float actual=0.0f;const auto result=ngxGetFloat(parameters,name,&actual);
  float delta=actual-expected;if(delta<0.0f)delta=-delta;
  const bool okay=result==1&&delta<=0.001f;
  if(!okay)Log("RR_PARAMETER_REJECT key=%s result=0x%08X expected=%.6f actual=%.6f",name,result,double(expected),double(actual));
  return okay;
 }
};
static bool RRNativeSetGuides(void* parameters,const RRNativeGuideBindings* bindings,ID3D12Resource* distance,bool reset) noexcept {
 __try {
  if(!parameters||!bindings)return false;
  RRNativeParameterApi api{parameters};
  if(!control_rr::SetGuideParameters(api,*bindings,distance,reset))return false;
  // Always write this optional parameter, including nullptr, so disabling the
  // experiment cannot leave a previous frame's mask on Control's persistent
  // NGX parameter object.
  api.Resource("DLSSD.ResponsivityMask",bindings->responsivity);
  return api.ResourceEquals("DLSSD.ResponsivityMask",bindings->responsivity);
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RRNativeSetFrameTime(void* parameters,float frameTimeMs) noexcept {
 __try {
  if(!parameters)return false;
  RRNativeParameterApi api{parameters};
  api.Float("FrameTimeDeltaInMsec",frameTimeMs);
  return api.FloatEquals("FrameTimeDeltaInMsec",frameTimeMs);
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
