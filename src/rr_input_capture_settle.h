#pragma once
#include <cstdint>
namespace control_rr {
// A request may be clicked while the game menu has suspended RR. Count time
// only across consecutive evaluated gameplay frames, restarting after resets.
class InputCaptureSettle {
    std::uint64_t previous_=0, started_=0;
    bool observed_=false;
public:
    static constexpr std::uint64_t DurationMs=5000;
    void Reset() noexcept { observed_=false; }
    bool Ready(std::uint64_t frame,bool reset,std::uint64_t now) noexcept {
        if(!observed_ || reset || frame!=previous_+1 || now<started_){started_=now;observed_=true;}
        previous_=frame;
        return !reset && now-started_>=DurationMs;
    }
};
}
