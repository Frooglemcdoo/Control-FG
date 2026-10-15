#pragma once
#include <array>
#include <chrono>
namespace PTPreparationMetrics {
using Clock=std::chrono::steady_clock;
// handoff lock, scene preview, owner lock, plan validation, resource retention, proof
using Costs=std::array<double,6>;
inline double Milliseconds(Clock::time_point a,Clock::time_point b){
    return std::chrono::duration<double,std::milli>(b-a).count();
}
}
