#pragma once
#include <cstdint>
namespace control_rr {
// One recording-thread coordinator. Begin belongs before feature reset/jitter;
// the selection cannot change at a later lighting or evaluation boundary.
struct RRFrameKey {
 std::uint64_t frame=0;
 std::uint32_t outputWidth=0,outputHeight=0,width=0,height=0;
 std::uint32_t rtEffects=0;
};
inline bool SameExtent(const RRFrameKey&a,const RRFrameKey&b) noexcept {
 return a.outputWidth==b.outputWidth&&a.outputHeight==b.outputHeight&&a.width==b.width&&a.height==b.height;
}
enum class RRFrameStage { Idle, Selected, Ready, Lighting, Evaluating, Complete, Failed };
class RRFrameCoordinator {
 RRFrameKey key_{},fixed_{};
 RRFrameStage stage_=RRFrameStage::Idle;
 bool selected_=false,stopped_=false,historyDirty_=false,reset_=false,recovering_=false,resizing_=false;
 const char* stopReason_="none";
public:
 bool Begin(const RRFrameKey&key,bool requested,bool ready) noexcept {
  // Menus/capture interruptions can run feature setup without any lighting or
  // AA. RR lighting/evaluation has not begun in Ready: return through a complete
  // SR frame. Candidate copies retain their own fence retirement. Abandonment
  // AFTER lighting stays terminal.
  if(selected_&&stage_!=RRFrameStage::Complete&&stage_!=RRFrameStage::Failed){
   if(stage_==RRFrameStage::Ready&&!stopped_)recovering_=true;
   else Fail("abandoned_committed_frame");
  }
  if((fixed_.frame||(requested&&ready))&&(!key.frame||key.frame<=key_.frame))Fail("nonmonotonic_frame");
  const bool previousRR=selected_;
  const bool effectsChanged=key_.frame&&key.rtEffects!=key_.rtEffects;
  const bool changed=key_.frame&&!SameExtent(key,key_);
  key_=key;reset_=false;
  const bool valid=key.width>=64&&key.height>=64&&key.width<=8192&&key.height<=8192&&
   key.outputWidth>=key.width&&key.outputHeight>=key.height&&key.outputWidth<=8192&&key.outputHeight<=8192;
  // Select SR before feature creation/jitter when either extent changes.
  // Existing terminal recording faults stay terminal. Complete native SR and
  // rebuild/warm the new guides before another RR selection.
  if(changed&&!stopped_){recovering_=true;resizing_=true;}
  selected_=requested&&ready&&valid&&!stopped_&&!recovering_;
  reset_=selected_&&(!previousRR||effectsChanged);
  stage_=RRFrameStage::Selected;
  return selected_;
 }
 bool FeatureResult(bool success,bool activeRR) noexcept {
  if(stage_!=RRFrameStage::Selected)return Fail();
  if(!success&&!selected_){stage_=RRFrameStage::Complete;return false;}
  if(!success||activeRR!=selected_)return Fail();
  stage_=RRFrameStage::Ready;
  if(selected_){if(!fixed_.frame||!SameExtent(key_,fixed_))reset_=true;fixed_=key_;resizing_=false;}
  return true;
 }
 bool NeedsHistoryReset() const noexcept {return historyDirty_&&!selected_;}
 bool HistoryResetResult(bool success) noexcept {
  if(stage_!=RRFrameStage::Ready||!NeedsHistoryReset()||!success)return Fail();
  historyDirty_=false;reset_=true;return true;
 }
 bool BeginLighting(std::uint64_t frame) noexcept {
  if(!selected_||stage_!=RRFrameStage::Ready||key_.frame!=frame)return Fail();
  // Mark before recording: a fault may follow a partially emitted command.
  historyDirty_=true;stage_=RRFrameStage::Lighting;return true;
 }
 bool BeginEvaluation(std::uint64_t frame,bool guidesMatch,bool lightingComplete) noexcept {
  if(!selected_||stage_!=RRFrameStage::Lighting||frame!=key_.frame||!guidesMatch||!lightingComplete)return Fail();
  stage_=RRFrameStage::Evaluating;return true;
 }
 bool EvaluationResult(bool success) noexcept {
  if(stage_!=RRFrameStage::Evaluating||!success)return Fail();
  stage_=RRFrameStage::Complete;return true;
 }
 bool CompleteSR(bool success=true,bool frameMatches=true) noexcept {
  if(selected_||stage_!=RRFrameStage::Ready||!success)return Fail("sr_evaluation_failed");
  if(recovering_&&!frameMatches)return Fail("recovery_sr_frame_mismatch");
  if(recovering_&&historyDirty_)return Fail("recovery_history_not_reset");
  stage_=RRFrameStage::Complete;recovering_=false;return true;
 }
 bool Fail(const char* reason="explicit_failure") noexcept {
  if(!stopped_)stopReason_=reason;
  stopped_=true;recovering_=false;stage_=RRFrameStage::Failed;return false;
 }
 bool Selected() const noexcept {return selected_;}
 bool Stopped() const noexcept {return stopped_;}
 bool Recovering() const noexcept {return recovering_;}
 bool Resizing() const noexcept {return resizing_;}
 const char* StopReason() const noexcept {return stopReason_;}
 bool Reset() const noexcept {return reset_;}
 RRFrameStage Stage() const noexcept {return stage_;}
 const RRFrameKey& Key() const noexcept {return key_;}
};
}
