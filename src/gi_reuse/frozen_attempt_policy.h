#pragma once
#include <atomic>
namespace gi_reuse {
// Startup-only activation. A stopped session never retries implicitly.
class FrozenAttemptPolicy {
 std::atomic<unsigned> consecutive_{0};
 std::atomic<bool> stopped_{false};
public:
 bool Rejected()noexcept {
  if(stopped_.load())return true;
  if(consecutive_.fetch_add(1)+1>=8)stopped_.store(true);
  return stopped_.load();
 }
 void Succeeded()noexcept {consecutive_.store(0);}
 void Stop()noexcept {stopped_.store(true);}
 bool Stopped()const noexcept {return stopped_.load();}
};
}
