#pragma once
#include <atomic>
#include "settings_check_state.h"
namespace control_diagnostics {
inline std::atomic<bool> captureActive{false};
inline bool Active() noexcept {
#ifdef CONTROL_ON_DEMAND_DIAGNOSTICS
 return captureActive.load(std::memory_order_relaxed);
#else
 return true;
#endif
}
}
