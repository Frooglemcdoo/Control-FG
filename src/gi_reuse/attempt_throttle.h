#pragma once
#include <atomic>
#include <cstdint>
namespace gi_reuse {
// Limit expensive retries after rejection. Never authorizes a scene or delays
// GPU work; native rendering and live metadata collection continue normally.
class AttemptThrottle {
 std::atomic<std::uint64_t> next_{0};
public:
 bool Allow(std::uint64_t now) const noexcept {return now>=next_.load();}
 void Rejected(std::uint64_t now) noexcept {next_=now>UINT64_MAX-250?UINT64_MAX:now+250;}
 void Succeeded() noexcept {next_=0;}
};
}
