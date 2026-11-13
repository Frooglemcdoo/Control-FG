#pragma once
#include <atomic>
namespace gi_reuse {inline std::atomic<bool> ownedRequested{false};
inline constexpr bool GuidedLiveEnabled() noexcept {
#ifdef CONTROL_GI_GUIDED_LIVE
 return true;
#else
 return false;
#endif
}
inline std::atomic<bool> guidedRequested{true},guidedRayApplied{false},guidedEvaluatorReady{false},guidedScopeReady{false};
// Both stages are recorded in the verified native GI callback. Never share
// their pairing flag with another recording thread or a later invocation.
inline thread_local bool guidedScopeActive=false,guidedScopeDraw=false;
}
#ifdef _WIN32
#include <windows.h>
namespace gi_reuse {
inline bool SingleProducerEnabled() noexcept {
    if(GuidedLiveEnabled())return true;
    static const bool enabled=[] {char v[4]{};const DWORD length=GetEnvironmentVariableA("CONTROLTOH_GI_SINGLE",v,4);
        if(length)return length==1&&v[0]=='1';
        // Explicit legacy diagnostic launches keep their original selector.
        return !GetEnvironmentVariableA("CONTROLTOH_GI_LIVE",v,4)
            && !GetEnvironmentVariableA("CONTROLTOH_GI_EVALUATOR",v,4)
            && !GetEnvironmentVariableA("CONTROLTOH_GI_OWNED",v,4);}();
    return enabled;
}
inline std::atomic<bool> singleEvaluatorReady{false},singleRayApplied{false};
inline std::atomic<unsigned long long> temporalDispatches{0},singleRayDispatches{0},singleEvaluatorDispatches{0};
}
#endif
