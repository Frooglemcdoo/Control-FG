#pragma once
#include <atomic>
#include <array>
#include <cstdint>
namespace control_compare {
struct Condition {const char* name;bool owned;unsigned rays;bool lighting,relax,character,volume;};
inline constexpr Condition conditions[]={
 {"native_rr_begin",false,0,false,false,false,false},
 {"native_rr_4_samples",false,4,false,false,false,false},
 {"owned_4_raw",true,4,false,false,false,false},
 {"owned_4_lighting_raw",true,4,true,false,false,false},
 {"owned_4_lighting_relax",true,4,true,true,false,false},
 {"plus_character",true,4,true,true,true,false},
 {"full_rr",true,4,true,true,true,true},
 {"full_rr_repeat",true,4,true,true,true,true},
 {"native_rr_end",false,0,false,false,false,false}};
inline constexpr unsigned phaseCount=sizeof(conditions)/sizeof(conditions[0]);
inline constexpr unsigned capacity=65536;
struct Cadence {std::uint64_t ticks=0,present=0;unsigned phase=0;std::atomic<bool> ready{false};};
inline std::array<Cadence,capacity> frames{};
inline std::atomic<bool> enabled{false},running{false};
inline std::atomic<std::uint64_t> geometry{0};
inline std::atomic<unsigned> phase{0},count{0},dropped{0};
inline std::atomic<std::uint64_t> cpuTicks[phaseCount][3]{},cpuCalls[phaseCount][3]{};
inline void Reset() noexcept {
 enabled=false;count=0;dropped=0;
 for(auto& f:frames)f.ready=false;
 for(unsigned p=0;p<phaseCount;++p)for(unsigned s=0;s<3;++s){cpuTicks[p][s]=0;cpuCalls[p][s]=0;}
}
inline void Present(std::uint64_t ticks,std::uint64_t present) noexcept {
 if(!enabled.load(std::memory_order_acquire))return;
 const auto p=phase.load();const auto i=count.fetch_add(1);
 if(i>=capacity){++dropped;return;}
 auto& f=frames[i];f.ticks=ticks;f.present=present;f.phase=p;f.ready.store(true,std::memory_order_release);
}
inline void CPU(unsigned p,unsigned stage,std::uint64_t ticks) noexcept {
 if(p<phaseCount&&stage<3){cpuTicks[p][stage].fetch_add(ticks,std::memory_order_relaxed);++cpuCalls[p][stage];}
}
}
