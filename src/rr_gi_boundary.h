#pragma once
#include <atomic>
#include <cstdint>
#include "rr_gi_stage_policy.h"
namespace control_rr_gi {
struct Boundary {
 unsigned long long frame=0;
 std::uintptr_t color=0;
 StageMode stage=StageMode::ClampOnly;
 DiagnosticView view=DiagnosticView::Normal;
 unsigned spatialPasses=0;
 bool cleanup=false,applied=false,valid=false;
};
// Identity only: no borrowed resource is retained or dereferenced by the reader.
inline std::atomic<bool> boundaryRequested{false};
inline std::atomic_flag boundaryLock=ATOMIC_FLAG_INIT;
inline Boundary boundary{};
inline void PublishBoundary(const Boundary& value) noexcept {
 if(boundaryLock.test_and_set(std::memory_order_acquire))return;
 boundary=value;boundaryLock.clear(std::memory_order_release);
}
inline Boundary ReadBoundary(unsigned long long frame) noexcept {
 Boundary result{};
 if(boundaryLock.test_and_set(std::memory_order_acquire))return result;
 if(boundary.frame==frame)result=boundary;
 boundaryLock.clear(std::memory_order_release);return result;
}
}
