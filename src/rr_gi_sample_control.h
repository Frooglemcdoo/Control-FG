#pragma once
#include <atomic>
#include "rr_gi_sample_policy.h"
namespace control_rr {
inline std::atomic<unsigned> rrGISampleRequested{4};
inline unsigned GISampleRequested() noexcept {return rrGISampleRequested.load(std::memory_order_acquire);}
inline void GISampleSelect(unsigned samples) noexcept {rrGISampleRequested.store(NormalizeGISampleRequest(samples),std::memory_order_release);}
// Render-thread owned snapshot. UI changes cannot alter a frame in progress.
struct GISampleFrameSelection {
 unsigned requested=0;
 bool Latch(unsigned next,bool fullRR) noexcept {
  if(!fullRR)return false;
  next=NormalizeGISampleRequest(next);
  if(next==requested)return false;
  requested=next;return true;
 }
};
}
