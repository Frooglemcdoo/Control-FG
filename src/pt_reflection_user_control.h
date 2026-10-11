#pragma once
#include <atomic>

// One atomic snapshot couples the A/B switch and diagnostic mode. A UI toggle
// always returns to normal shading, even if F11 is pressed concurrently.
// Bit 0 = added PT reflection lighting enabled; bits 1..2 = F11 mode.
static std::atomic<unsigned int> ptReflectionUserState{1u};

static bool PTReflectionUserRequested() noexcept {
    return (ptReflectionUserState.load(std::memory_order_acquire)&1u)!=0u;
}
static void PTReflectionUserRequest(bool enabled) noexcept {
    ptReflectionUserState.store(enabled?1u:0u,std::memory_order_release);
}
static unsigned PTReflectionUserCycleDiagnostic() noexcept {
    unsigned state=ptReflectionUserState.load(std::memory_order_acquire);
    while(state&1u){
        const unsigned next=1u|((((state>>1u)+1u)%4u)<<1u);
        if(ptReflectionUserState.compare_exchange_weak(state,next,
            std::memory_order_acq_rel,std::memory_order_acquire))return next>>1u;
    }
    return 0u; // F11 cannot re-enable PT while the overlay switch is off.
}
