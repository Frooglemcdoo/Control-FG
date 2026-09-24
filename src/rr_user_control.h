#pragma once
#include <atomic>
#include <cstring>
namespace control_rr {
// r21y production: native Control energy clamp is fixed in the public runtime.
// The r21w ReShade reference enum/helper remains only for archived source comparison; the
// public overlay does not expose it and the production temporal hooks do not request it.
// RAW remains an emergency fail-closed fallback. Historical material replay stays source-only.
inline constexpr bool RRGBufferGuideMode = true;
enum class RRUserStatus {Off,Waiting,Active,Stopped,Recovering,Resizing};
enum class RRUserMode : unsigned int { Off=0, Partial=1, Full=2 };
inline std::atomic<unsigned int> rrUserMode{static_cast<unsigned int>(RRUserMode::Off)};
enum class RRSpecularSignalMode : unsigned int { NativeClamp=0, ReferenceClamp=1 };
inline std::atomic<unsigned int> rrUserSpecularSignalMode{static_cast<unsigned int>(RRSpecularSignalMode::NativeClamp)};
inline std::atomic<RRUserStatus> rrUserStatus{RRUserStatus::Off};
enum class RRWaitReason {Guides,Unsupported,EnableReflections,OtherRT,Matrix,Resolution,Depth,Input,Handoff,Pipeline,CaptureBudget,CaptureBudgetQuery,CaptureAllocation};
inline std::atomic<RRWaitReason> rrUserWaitReason{RRWaitReason::Guides};
inline std::atomic<RRWaitReason> rrUserInputReason{RRWaitReason::Guides};
// Capture preparation failures are latched separately so repeated distance
// warmup attempts cannot overwrite their cause with reflection_unavailable.
inline std::atomic<RRWaitReason> rrUserCaptureReason{RRWaitReason::Guides};
inline std::atomic<unsigned int> rrUserSharpnessPercent{0};
inline std::atomic<bool> rrUserSharpnessOverride{false};
inline constexpr unsigned int RRRuntimePauseResize = 1u << 0;
inline constexpr unsigned int RRRuntimePausePreset = 1u << 1;
inline std::atomic<unsigned int> rrUserRuntimePauseReasons{0};
inline std::atomic<bool> rrUserPresetOptionsWindow{false};
inline constexpr unsigned int RRPresetE = 5;
inline constexpr unsigned int RRPresetF = 6;
inline constexpr unsigned int RRPresetK = 11;
inline constexpr unsigned int RRPresetL = 12;
inline constexpr unsigned int RRPresetM = 13;
inline std::atomic<unsigned int> rrUserPreset{RRPresetF};
inline std::atomic<unsigned long long> rrUserPresetSelectionGeneration{1};
inline std::atomic<bool> rrUserPresetRestartRequired{false};
inline std::atomic<unsigned int> rrUserPresetCreateConfirmed{0};
inline std::atomic<unsigned long long> rrUserPresetCreateGeneration{0};
enum class RRSkinMode : unsigned int { Off=0, Responsivity=1 };
inline std::atomic<unsigned int> rrUserSkinMode{static_cast<unsigned int>(RRSkinMode::Off)};
inline void RRUserCaptureFailure(RRWaitReason reason) noexcept {
 rrUserCaptureReason.store(reason,std::memory_order_release);
}
inline void RRUserDistanceInput(bool valid,const char* reason) noexcept {
 RRWaitReason value=RRWaitReason::Guides;
 if(!valid){
  value=RRWaitReason::Input;
  if(reason&&(std::strcmp(reason,"clip_to_view_provider")==0||std::strcmp(reason,"view_to_world_provider")==0))value=RRWaitReason::Matrix;
  else if(reason&&std::strcmp(reason,"inverse_resolution_provider")==0)value=RRWaitReason::Resolution;
  else if(reason&&std::strcmp(reason,"clip_depth_provider")==0)value=RRWaitReason::Depth;
 }
 rrUserInputReason.store(value,std::memory_order_release);
}
inline void RRUserDistanceDispatch(bool valid,const char* reason) noexcept {
 if(valid)return;
 auto value=RRWaitReason::Input;
 if(reason&&(std::strcmp(reason,"pipeline_prepare_requested")==0||std::strcmp(reason,"pipeline_not_ready_or_extent_changed")==0||std::strcmp(reason,"pipeline_resize_retirement_pending")==0))value=RRWaitReason::Pipeline;
 else if(reason&&std::strncmp(reason,"producer_",9)==0)value=RRWaitReason::Handoff;
 rrUserInputReason.store(value,std::memory_order_release);
}
inline void RRUserWaitingStatus(bool supported,bool configuration,unsigned effects) noexcept {
 const auto capture=rrUserCaptureReason.load(std::memory_order_acquire);
 const auto reason=!supported?RRWaitReason::Unsupported:
  !configuration?((effects&1)?RRWaitReason::OtherRT:RRWaitReason::EnableReflections):
  capture!=RRWaitReason::Guides?capture:rrUserInputReason.load(std::memory_order_acquire);
 rrUserWaitReason.store(reason,std::memory_order_release);
}
inline RRUserMode RRUserModeValue() noexcept {return static_cast<RRUserMode>(rrUserMode.load(std::memory_order_acquire));}
inline bool RRUserRequested() noexcept {return RRUserModeValue()==RRUserMode::Full;}
inline bool RRUserPartialRequested() noexcept {return RRUserModeValue()==RRUserMode::Partial;}
inline bool RRUserAnyRequested() noexcept {return RRUserModeValue()!=RRUserMode::Off;}
// Expensive guide/capture work exists only for FULL RR and is hard-paused
// across renderer-state epochs. Resize and live-preset transitions are tracked
// independently so one transition can never accidentally reopen work while the
// other is still draining.
inline unsigned int RRUserRuntimePauseReasons() noexcept {return rrUserRuntimePauseReasons.load(std::memory_order_acquire);}
inline bool RRUserRuntimePaused() noexcept {return RRUserRuntimePauseReasons()!=0;}
inline void RRUserSetRuntimePauseReason(unsigned int reason,bool paused) noexcept {
 if(!reason)return;
 if(paused)rrUserRuntimePauseReasons.fetch_or(reason,std::memory_order_acq_rel);
 else rrUserRuntimePauseReasons.fetch_and(~reason,std::memory_order_acq_rel);
}
// Compatibility name retained for the existing resize epoch.
inline void RRUserSetRuntimePaused(bool paused) noexcept {RRUserSetRuntimePauseReason(RRRuntimePauseResize,paused);}
inline void RRUserSetPresetTransitionPaused(bool paused) noexcept {RRUserSetRuntimePauseReason(RRRuntimePausePreset,paused);}
inline bool RRUserPresetOptionsWindow() noexcept {return rrUserPresetOptionsWindow.load(std::memory_order_acquire);}
inline void RRUserSetPresetOptionsWindow(bool enabled) noexcept {rrUserPresetOptionsWindow.store(enabled,std::memory_order_release);}
inline bool RRUserRuntimeWorkRequested() noexcept {return RRUserModeValue()==RRUserMode::Full&&!RRUserRuntimePaused();}
inline void RRUserSetMode(RRUserMode mode) noexcept {rrUserMode.store(static_cast<unsigned int>(mode),std::memory_order_release);}
inline void RRUserRequest(bool enabled) noexcept {RRUserSetMode(enabled?RRUserMode::Full:RRUserMode::Off);}
inline RRSpecularSignalMode RRUserSpecularSignalModeValue() noexcept {return static_cast<RRSpecularSignalMode>(rrUserSpecularSignalMode.load(std::memory_order_acquire));}
inline bool RRUserSpecularReferenceRequested() noexcept {return RRUserSpecularSignalModeValue()==RRSpecularSignalMode::ReferenceClamp;}
inline bool RRUserSpecularNativeClampRequested() noexcept {return RRUserSpecularSignalModeValue()==RRSpecularSignalMode::NativeClamp;}
inline void RRUserSetSpecularSignalMode(RRSpecularSignalMode mode) noexcept {rrUserSpecularSignalMode.store(static_cast<unsigned int>(mode),std::memory_order_release);}
inline const char* RRUserSpecularSignalLabel() noexcept {return RRUserSpecularReferenceRequested()?"renodx_reference":"native_control_clamp";}
inline const char* RRUserModeLabel() noexcept {switch(RRUserModeValue()){case RRUserMode::Partial:return "partial";case RRUserMode::Full:return "full";default:return "off";}}
inline void RRUserPublish(RRUserStatus status) noexcept {rrUserStatus.store(status,std::memory_order_release);}
inline unsigned int RRUserSharpnessPercent() noexcept {return rrUserSharpnessPercent.load(std::memory_order_acquire);}
inline float RRUserSharpnessValue() noexcept {return static_cast<float>(RRUserSharpnessPercent())/100.0f;}
inline void RRUserSetSharpnessPercent(unsigned int value) noexcept {if(value>100)value=100; rrUserSharpnessPercent.store(value,std::memory_order_release);}
inline bool RRUserPresetSupported(unsigned int value) noexcept {
 return value==RRPresetE||value==RRPresetF||value==RRPresetK||value==RRPresetL||value==RRPresetM;
}
inline unsigned int RRUserPresetValue() noexcept {return rrUserPreset.load(std::memory_order_acquire);}
inline bool RRUserPresetLiveSwitchable(unsigned int value) noexcept {return value==RRPresetE||value==RRPresetF;}
inline bool RRUserPresetLiveSwitchable(unsigned int from,unsigned int to) noexcept {return RRUserPresetLiveSwitchable(from)&&RRUserPresetLiveSwitchable(to);}
inline void RRUserSetPresetValue(unsigned int value) noexcept {
 if(!RRUserPresetSupported(value))return;
 const auto previous=rrUserPreset.exchange(value,std::memory_order_acq_rel);
 if(previous!=value)rrUserPresetSelectionGeneration.fetch_add(1,std::memory_order_acq_rel);
}
inline unsigned long long RRUserPresetSelectionGeneration() noexcept {return rrUserPresetSelectionGeneration.load(std::memory_order_acquire);}
inline bool RRUserPresetNeedsRestart() noexcept {return rrUserPresetRestartRequired.load(std::memory_order_acquire);}
inline void RRUserMarkPresetRestartRequired(bool required=true) noexcept {rrUserPresetRestartRequired.store(required,std::memory_order_release);if(required)rrUserPresetCreateConfirmed.store(0,std::memory_order_release);}
inline void RRUserConfirmPresetCreate(unsigned int value) noexcept {
 if(RRUserPresetSupported(value)){rrUserPresetCreateConfirmed.store(value,std::memory_order_release);rrUserPresetCreateGeneration.fetch_add(1,std::memory_order_acq_rel);if(value==RRUserPresetValue())rrUserPresetRestartRequired.store(false,std::memory_order_release);}
}
inline unsigned int RRUserConfirmedPresetValue() noexcept {return rrUserPresetCreateConfirmed.load(std::memory_order_acquire);}
inline unsigned long long RRUserPresetCreateGeneration() noexcept {return rrUserPresetCreateGeneration.load(std::memory_order_acquire);}
inline const wchar_t* RRUserPresetLabelW() noexcept {
 switch(RRUserPresetValue()){
 case RRPresetE:return L"E"; case RRPresetK:return L"K"; case RRPresetL:return L"L"; case RRPresetM:return L"M"; default:return L"F";
 }
}
inline const char* RRUserPresetLabel() noexcept {
 switch(RRUserPresetValue()){
 case RRPresetE:return "E"; case RRPresetK:return "K"; case RRPresetL:return "L"; case RRPresetM:return "M"; default:return "F";
 }
}
inline RRSkinMode RRUserSkinModeValue() noexcept {return static_cast<RRSkinMode>(rrUserSkinMode.load(std::memory_order_acquire));}
inline bool RRUserSkinResponsivity() noexcept {return RRUserSkinModeValue()==RRSkinMode::Responsivity;}
inline void RRUserSetSkinMode(RRSkinMode mode) noexcept {rrUserSkinMode.store(static_cast<unsigned int>(mode),std::memory_order_release);}
inline const char* RRUserSkinModeLabel() noexcept {return RRUserSkinResponsivity()?"responsivity":"off";}
inline bool RRUserSharpnessOverrideEnabled() noexcept {return rrUserSharpnessOverride.load(std::memory_order_acquire);}
inline void RRUserSetSharpnessOverrideEnabled(bool enabled) noexcept {rrUserSharpnessOverride.store(enabled,std::memory_order_release);}
inline const wchar_t* RRUserLabel() noexcept {
 const auto status=rrUserStatus.load(std::memory_order_acquire);
 if(RRUserPartialRequested())return L"Partial - native denoiser path, RR model OFF";
 if(status==RRUserStatus::Stopped)return L"Stopped - restart the game to retry";
 if(!RRUserRequested())return status==RRUserStatus::Off?L"Off":L"Turning off at the next frame";
 if(status==RRUserStatus::Resizing){
  const auto pauses=RRUserRuntimePauseReasons();
  if(pauses&RRRuntimePausePreset)return L"Switching RR preset - rebuilding RR state";
  const auto reason=rrUserWaitReason.load(std::memory_order_acquire);
  if(reason!=RRWaitReason::Unsupported&&reason!=RRWaitReason::EnableReflections&&reason!=RRWaitReason::OtherRT&&
     reason!=RRWaitReason::CaptureBudget&&reason!=RRWaitReason::CaptureBudgetQuery&&reason!=RRWaitReason::CaptureAllocation)
   return L"Resolution changed - rebuilding RR guides";
 }
 if(status==RRUserStatus::Recovering)return L"Paused - resumes when gameplay returns";
 if(status==RRUserStatus::Active){
  switch(RRUserPresetValue()){
  case RRPresetE:return L"Active - RR Preset E";
  case RRPresetK:return L"Active - RR Preset K";
  case RRPresetL:return L"Active - RR Preset L";
  case RRPresetM:return L"Active - RR Preset M";
  default:return L"Active - RR Preset F";
  }
 }
 switch(rrUserWaitReason.load(std::memory_order_acquire)){
 case RRWaitReason::Unsupported:return L"RR unavailable on this device/runtime";
 case RRWaitReason::EnableReflections:return L"Enable Ray Traced Reflections in Graphics";
 case RRWaitReason::OtherRT:return L"Blocked: ray tracing settings unavailable";
 case RRWaitReason::Handoff:return L"Blocked: reflection handoff; see log";
 case RRWaitReason::Pipeline:return L"Preparing reflection distance pipeline";
 case RRWaitReason::Matrix:return L"Blocked: reflection matrix snapshot failed";
 case RRWaitReason::Resolution:return L"Blocked: reflection resolution snapshot failed";
 case RRWaitReason::Depth:return L"Blocked: reflection depth snapshot failed";
 case RRWaitReason::Input:return L"Blocked: reflection inputs rejected; see log";
 case RRWaitReason::CaptureBudget:return L"Blocked: insufficient available GPU memory";
 case RRWaitReason::CaptureBudgetQuery:return L"Blocked: GPU memory budget unavailable";
 case RRWaitReason::CaptureAllocation:return L"Blocked: reflection allocation failed; see log";
 default:return L"Preparing current-frame guides";
 }
}
inline void RRUserFrameStatus(bool requested,bool selected,bool stopped,bool recovering=false,bool resizing=false) noexcept {
 if(stopped)RRUserPublish(RRUserStatus::Stopped);
 else if(!requested)RRUserPublish(RRUserStatus::Off);
 else if(resizing)RRUserPublish(RRUserStatus::Resizing);
 else if(recovering)RRUserPublish(RRUserStatus::Recovering);
 else if(!selected||rrUserStatus.load(std::memory_order_acquire)!=RRUserStatus::Active)RRUserPublish(RRUserStatus::Waiting);
}
}
