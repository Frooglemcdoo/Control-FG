#pragma once
#include <cstdint>
namespace gi_reuse {
// One invocation, owned by the recording thread. This is a recording protocol,
// not GPU completion or a resource lease. Resource/descriptor/SBT owners must
// be secured before Prepare; this planner cannot manufacture those guarantees.
class GISplitPhasePlan {
public:
 enum class Stage { Empty, Prepared, Training, TrainingShaded, Completion, Published };
 enum class Completion { None, NativeComplement, GuidedComplement };
private:
 Stage stage_=Stage::Empty;Completion completion_=Completion::None;
 std::uint64_t invocation_=0;std::uint32_t pixels_=0,parity_=0;
 bool resourcesReady_=false;
public:
 bool Prepare(std::uint64_t invocation,std::uint32_t pixels,std::uint32_t parity,
              bool completionResourcesOwned) noexcept {
  if(stage_!=Stage::Empty||!invocation||pixels<2||parity>1||!completionResourcesOwned)return false;
  invocation_=invocation;pixels_=pixels;parity_=parity;resourcesReady_=true;stage_=Stage::Prepared;return true;
 }
 bool BeginTraining(std::uint64_t invocation) noexcept {
  if(stage_!=Stage::Prepared||invocation!=invocation_||!resourcesReady_)return false;
  stage_=Stage::Training;return true;
 }
 bool TrainingShaded(std::uint64_t invocation) noexcept {
  if(stage_!=Stage::Training||invocation!=invocation_)return false;
  stage_=Stage::TrainingShaded;return true;
 }
 bool BeginCompletion(std::uint64_t invocation,bool guideReady) noexcept {
  if(invocation!=invocation_||!resourcesReady_||
     (stage_!=Stage::Training&&stage_!=Stage::TrainingShaded))return false;
  // An unavailable guide cannot send already traced receivers through native again.
  completion_=guideReady&&stage_==Stage::TrainingShaded?Completion::GuidedComplement:Completion::NativeComplement;
  stage_=Stage::Completion;return true;
 }
 bool Publish(std::uint64_t invocation,bool bothSubsetsShaded) noexcept {
  if(stage_!=Stage::Completion||invocation!=invocation_||!bothSubsetsShaded)return false;
  stage_=Stage::Published;return true;
 }
 // Compact dispatch mapping preserves the original receiver index/random stream.
 // Do not halve DispatchRays dimensions without restoring original coordinates
 // and dimensions in the raygen; that would change the native sample identity.
 std::uint32_t TrainingPixelAt(std::uint32_t sample)const noexcept {
  return sample<TrainingCount()?sample*2+parity_:UINT32_MAX;
 }
 std::uint32_t ComplementPixelAt(std::uint32_t sample)const noexcept {
  return sample<ComplementCount()?sample*2+(parity_^1u):UINT32_MAX;
 }
 bool CanRunFullNative()const noexcept{return stage_==Stage::Empty||stage_==Stage::Prepared;}
 bool TrainingPixel(std::uint32_t pixel)const noexcept{return pixel<pixels_&&(pixel&1u)==parity_;}
 bool ComplementPixel(std::uint32_t pixel)const noexcept{return pixel<pixels_&&!TrainingPixel(pixel);}
 std::uint32_t TrainingCount()const noexcept{return pixels_/2+((pixels_&1u)&&parity_==0?1u:0u);}
 std::uint32_t ComplementCount()const noexcept{return pixels_-TrainingCount();}
 Stage State()const noexcept{return stage_;}
 Completion SelectedCompletion()const noexcept{return completion_;}
};
}
