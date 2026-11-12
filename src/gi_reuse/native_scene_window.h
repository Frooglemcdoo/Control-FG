#pragma once
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
namespace gi_reuse {
// One delayed GI reader versus admission of native preparation callbacks.
// Host must intercept the FULL preparation entry, before its graphics signal,
// and acquire only after the matching native scene job join. Holding this gate
// does not itself retain GPU objects or replace native queue/fence ordering.
class NativeSceneWindow {
 using U64=std::uint64_t;
 mutable std::mutex mutex_;
 std::condition_variable changed_;
 U64 epoch_=0,preparations_=0,waiting_=0,reader_=0;
public:
 NativeSceneWindow()=default;
 NativeSceneWindow(const NativeSceneWindow&)=delete;
 NativeSceneWindow& operator=(const NativeSceneWindow&)=delete;
 // Preparation executes outside the mutex. Multiple native writers are not
 // serialized with each other; only a delayed GI reader blocks admission.
 // Pinned callback contract forbids reentrant preparation on the reader's
 // recording thread before it submits/cancels its consumer.
 void EnterPreparation()noexcept {
  std::unique_lock<std::mutex> lock(mutex_);++waiting_;
  changed_.wait(lock,[&]{return !reader_;});--waiting_;++preparations_;
  if(epoch_!=(std::numeric_limits<U64>::max)())++epoch_;
 }
 bool LeavePreparation()noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if(!preparations_)return false;
  --preparations_;return true;
 }
 // Sample BEFORE native join; acquire afterward with this exact epoch. A new
 // preparation between join and acquisition invalidates the proof, even when
 // its callback has already returned and its jobs are still running.
 U64 BeforeJoin()const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return !preparations_&&!waiting_&&!reader_&&epoch_!=(std::numeric_limits<U64>::max)()?epoch_:0;
 }
 bool TryAcquire(U64 token,U64 joinedEpoch)noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if(!token||!joinedEpoch||joinedEpoch!=epoch_||epoch_==(std::numeric_limits<U64>::max)()||
     preparations_||waiting_||reader_)return false;
  reader_=token;return true;
 }
 // Release only after real consumer Execute, or a proven cancellation that
 // cannot execute delayed rays. Recording/Close/frame advance is insufficient.
 bool Release(U64 token)noexcept {
  {std::lock_guard<std::mutex> lock(mutex_);if(!token||reader_!=token)return false;reader_=0;}
  changed_.notify_all();return true;
 }
 struct State {U64 epoch,preparations,waiting,reader;};
 State Inspect()const noexcept {std::lock_guard<std::mutex> lock(mutex_);return {epoch_,preparations_,waiting_,reader_};}
};
}
