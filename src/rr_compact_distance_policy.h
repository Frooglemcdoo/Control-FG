#pragma once
#include <atomic>
#include <cstring>
namespace rr_compact_distance {
inline bool Parse(const char* value) noexcept {return value&&!std::strcmp(value,"on");}
inline std::atomic<bool> enabled{false},installed{false},lastBound{false};
inline std::atomic<unsigned long long> evaluations{0},boundEvaluations{0};
inline std::atomic<const char*> reason{"not_requested"},captureReason{"not_requested"};
inline std::atomic<unsigned long long> captureBytes{0};
}
