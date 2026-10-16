#pragma once
#include <atomic>
#include <cstdint>
namespace control_rr {
// Publish only after native feature creation confirms this frame's RR route.
// PT recording threads consume a single atomic engine-frame identity, never UI
// request/status or a previous frame's successful evaluation.
class RRPTInputRoute {
    std::atomic<std::uint64_t> selectedFrame{0};
public:
    void Publish(std::uint64_t frame,bool selected) noexcept {
        selectedFrame.store(selected?frame:0,std::memory_order_release);
    }
    bool Fresh(std::uint64_t frame) const noexcept {
        return frame!=0&&selectedFrame.load(std::memory_order_acquire)==frame;
    }
};
inline RRPTInputRoute rrPTInputRoute;
}
