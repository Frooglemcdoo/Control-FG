#pragma once
#include <atomic>
namespace control_reblur {
#ifdef CONTROL_REBLUR_ONLY
inline constexpr bool Enabled=true;
#else
inline constexpr bool Enabled=false;
#endif
enum class Status {Waiting,Recorded,Submitted,Active,Fallback};
inline std::atomic<Status> status{Status::Waiting};
inline std::atomic<const char*> reason{"waiting_for_gi"};
}
