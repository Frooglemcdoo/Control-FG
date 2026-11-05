#pragma once
#include <atomic>
namespace gi_reuse {inline std::atomic<bool> ownedRequested{true};}
#ifdef _WIN32
#include <windows.h>
namespace gi_reuse {
inline bool SingleProducerEnabled() noexcept {
    static const bool enabled=[] {char v[4]{};const DWORD length=GetEnvironmentVariableA("CONTROLTOH_GI_SINGLE",v,4);
        if(length)return length==1&&v[0]=='1';
        // Explicit legacy diagnostic launches keep their original selector.
        return !GetEnvironmentVariableA("CONTROLTOH_GI_LIVE",v,4)
            && !GetEnvironmentVariableA("CONTROLTOH_GI_EVALUATOR",v,4)
            && !GetEnvironmentVariableA("CONTROLTOH_GI_OWNED",v,4);}();
    return enabled;
}
inline std::atomic<bool> singleEvaluatorReady{false},singleRayApplied{false};
inline std::atomic<unsigned long long> singleRayDispatches{0},singleEvaluatorDispatches{0};
}
#endif
