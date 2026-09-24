#pragma once
#include <atomic>
// A normal successful Present with known FG Off can satisfy the next resize's
// Off-commit boundary. Enabling FG or consuming the proof retires it.
namespace control_fg_off_present {
class Proof {
    std::atomic_flag lock_=ATOMIC_FLAG_INIT;
    const void* chain_=nullptr;
    unsigned long long frame_=0;
    struct Guard {
        std::atomic_flag& lock;
        explicit Guard(std::atomic_flag& value) noexcept : lock(value) {
            while(lock.test_and_set(std::memory_order_acquire)) {}
        }
        ~Guard() { lock.clear(std::memory_order_release); }
    };
public:
    void Invalidate() noexcept { Guard guard(lock_);chain_=nullptr;frame_=0; }
    void Observe(const void* chain,unsigned long long frame,bool committedOff) noexcept {
        Guard guard(lock_);
        chain_=committedOff&&frame?chain:nullptr;
        frame_=chain_?frame:0;
    }
    unsigned long long Take(const void* chain) noexcept {
        Guard guard(lock_);
        const auto result=chain&&chain_==chain?frame_:0;
        chain_=nullptr;frame_=0;return result;
    }
};
}
