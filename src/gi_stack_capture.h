#pragma once
#include <atomic>
#include <cstdint>
// Shared coordination only. Owners retain their own resource/fence lifetimes.
namespace gi_stack_capture {
inline std::atomic<bool> enabled{false};
inline std::atomic<std::uint64_t> sourceFrame{0};
inline std::atomic<bool(*)() noexcept> request{nullptr};
inline std::atomic<int(*)() noexcept> status{nullptr}; // 0 pending, 1 complete, -1 failed
inline std::atomic<std::uint64_t(*)(void*) noexcept> readySource{nullptr};
}
