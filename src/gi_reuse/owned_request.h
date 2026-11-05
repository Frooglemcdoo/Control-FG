#pragma once
#include <atomic>
namespace gi_reuse {inline std::atomic<bool> ownedRequested{true};}
#ifdef _WIN32
#include <windows.h>
namespace gi_reuse {
inline bool SingleProducerEnabled() noexcept {
    static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_SINGLE",v,4)==1&&v[0]=='1';}();
    return enabled;
}
inline std::atomic<bool> singleEvaluatorReady{false},singleRayApplied{false};
inline std::atomic<unsigned long long> singleRayDispatches{0},singleEvaluatorDispatches{0};
}
#endif
