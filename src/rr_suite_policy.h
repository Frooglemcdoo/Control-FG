#pragma once
#include <atomic>
#include <cstdint>
namespace rr_suite {
struct Step {const char* name;unsigned variant,samples;bool raw;};
inline constexpr Step steps[]={
 {"full-spec4",0,4,false},{"full-spec4-no-rr-clamp",0,4,true},
 {"local-specular",1,4,false},{"volume-specular",2,4,false},
 {"cubemap-specular",3,4,false},{"emission",4,4,false},
 {"reflected-diffuse",5,4,false},{"full-spec16",0,16,false}};
inline constexpr unsigned count=sizeof(steps)/sizeof(steps[0]);
inline constexpr unsigned maxPairs=2*count;
inline std::atomic<unsigned> requested{0},latched{0},session{0}; // zero means inactive; step+1 otherwise
inline std::atomic<unsigned long long> frame{0},dispatchFrame{0};
inline std::atomic<unsigned> dispatchStep{0},effectiveSamples{0};
inline std::atomic<unsigned long long> sampleFrame{0},clampFrame{0};
inline std::atomic<unsigned> clampPercent{~0u};
inline std::atomic<bool> cancelled{false};
inline std::atomic<unsigned long long> dispatches{0};
inline const Step* Current() noexcept {auto n=latched.load();return n&&n<=count?&steps[n-1]:nullptr;}
inline bool Latch(std::uint64_t f) noexcept {
 frame.store(f);const auto n=requested.load();return latched.exchange(n)!=n;
}
inline bool Verified(std::uint64_t f) noexcept {
 const auto n=latched.load();return n&&n<=count&&dispatchFrame.load()==f&&dispatchStep.load()==n&&sampleFrame.load()==f&&effectiveSamples.load()==steps[n-1].samples&&clampFrame.load()==f&&clampPercent.load()==(steps[n-1].raw?0u:60u);
}
}
