#pragma once
#include <atomic>
#include "settings_check_state.h"
namespace control_diagnostics {
inline std::atomic<bool> captureActive{false};
inline std::atomic<bool> ownedSceneAllowed{false};
inline void PublishCaptureActivity(bool otherCapture,bool rrCapture,bool ownedScene) noexcept {
 ownedSceneAllowed.store(ownedScene&&rrCapture&&!otherCapture);
 captureActive.store(otherCapture||rrCapture);
}
inline bool Active() noexcept {
#ifdef CONTROL_ON_DEMAND_DIAGNOSTICS
 return captureActive.load(std::memory_order_relaxed);
#else
 return true;
#endif
}
inline bool BlocksOwnedGI(bool ownedScene) noexcept {
 return Active()&&!(ownedScene&&ownedSceneAllowed.load());
}
}
