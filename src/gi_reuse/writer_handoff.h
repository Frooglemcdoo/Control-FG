#pragma once
#include <cstdint>
#include <memory>
#include <utility>
namespace gi_reuse {
// One render-thread invocation, consumed once at GI6. Never a global latest
// writer: another invocation/frame or a failed Take discards the candidate.
template<class T> class GIWriterHandoff {
 std::shared_ptr<const T> value_;
 std::uint64_t invocation_=0,frame_=0;
public:
 void Clear() noexcept {value_.reset();invocation_=frame_=0;}
 bool Publish(std::uint64_t invocation,std::uint64_t frame,std::shared_ptr<const T> value) noexcept {
  Clear();if(!invocation||!frame||!value)return false;
  invocation_=invocation;frame_=frame;value_=std::move(value);return true;
 }
 std::shared_ptr<const T> Take(std::uint64_t invocation,std::uint64_t frame) noexcept {
  auto value=std::move(value_);const bool match=invocation&&frame&&invocation==invocation_&&frame==frame_;
  Clear();return match?std::move(value):nullptr;
 }
};
// Access only under the queue observer lock. Checked when visiting the consumer
// event, not after walking the batch (which could incorrectly accept a later
// producer). Queue IDs survive scene-history compaction; graph cut IDs do not.
class GIWriterSubmission {
 std::uint64_t generation_=0,queue_=0;
 bool valid_=true;
public:
 explicit GIWriterSubmission(std::uint64_t generation):generation_(generation){}
 bool Submit(std::uint64_t queue,std::uint64_t generation) noexcept {
  if(!valid_||!queue||!generation_||generation!=generation_||queue_){valid_=false;return false;}
  queue_=queue;return true;
 }
 bool Precedes(std::uint64_t queue) const noexcept {return valid_&&queue&&queue_==queue;}
};
}
