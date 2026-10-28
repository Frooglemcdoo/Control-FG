#pragma once
#include <atomic>
#include <cstdint>

namespace control_free_camera {
enum class Result { Idle, Queued, Applied, NotGameplay, OtherMode, VerificationFailed, TimedOut };
inline bool SupportedMode(int mode) noexcept { return mode == 0 || mode == 1; }

// The UI queues commands. Only the native camera-update hook consumes them.
// 0=idle, 1=normal requested, 2=free requested, 3=executing.
class Commands {
    std::atomic<int> command_{0};
    std::atomic<int> mode_{-1};
    std::atomic<std::uint64_t> generation_{0};
    std::atomic<Result> result_{Result::Idle};
public:
    int Mode() const noexcept { return mode_.load(); }
    std::uint64_t Generation() const noexcept { return generation_.load(); }
    bool Pending() const noexcept { return command_.load() != 0; }
    Result Status() const noexcept { return result_.load(); }
    void Observe(int mode) noexcept {
        if (mode_.exchange(mode) != mode) ++generation_;
    }
    bool Request(int mode) noexcept {
        if (!SupportedMode(mode)) return false;
        int idle = 0;
        // Publish Queued before the request can be serviced. UI is the only producer.
        if (Pending()) return false;
        result_.store(Result::Queued);
        return command_.compare_exchange_strong(idle, mode + 1);
    }
    bool Timeout() noexcept {
        int request = command_.load();
        if (request != 1 && request != 2) return false;
        if (!command_.compare_exchange_strong(request, 0)) return false;
        result_.store(Result::TimedOut);
        return true;
    }
    template<class Read, class Ready, class Set, class Verify>
    void Service(Read read, Ready ready, Set set, Verify verify) noexcept {
        Observe(read());
        int request = command_.load();
        if (request != 1 && request != 2) return;
        if (!command_.compare_exchange_strong(request, 3)) return;
        const int target = request - 1;
        Result result;
        if (!SupportedMode(Mode())) result = Result::OtherMode;
        else if (!ready()) result = Result::NotGameplay;
        else {
            // Even an already-selected normal mode goes through the native setter:
            // it also restores the game's player-input ownership and camera history.
            set(target);
            Observe(read());
            result = Mode() == target && verify(target) ? Result::Applied : Result::VerificationFailed;
        }
        result_.store(result);
        command_.store(0);
    }
};
}
