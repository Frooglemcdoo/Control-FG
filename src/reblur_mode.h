#pragma once
#include <atomic>
namespace control_reblur {
#ifdef CONTROL_REBLUR_ONLY
inline constexpr bool Enabled=true;
#else
inline constexpr bool Enabled=false;
#endif
inline std::atomic<unsigned> mode{0}, sharpening{0}, revision{0};
inline unsigned Mode() noexcept {return mode.load();}
inline bool Active() noexcept {return Enabled && Mode()!=2;}
inline void Select(unsigned value) noexcept {if(value>2)value=0;if(mode.exchange(value)!=value)++revision;}
inline void Sharpen(unsigned value) noexcept {value=value>100?100:value;sharpening=((value+2)/5)*5;}
inline float Sharpness() noexcept {return Active()?float(sharpening.load())*.01f:0.f;}
enum class Status {Waiting,Recorded,Submitted,Active,Fallback};
inline std::atomic<Status> status{Status::Waiting};
inline std::atomic<const char*> reason{"waiting_for_gi"};
}
