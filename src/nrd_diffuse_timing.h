#pragma once
// Optional diagnostics supplied by the boundary owner. No D3D or profiler
// dependency; null callbacks leave ordinary rendering without timestamp work.
namespace control_nrd {
enum class TimingStage { Prepare, Denoise, Dispatch, Finish };
struct Timing {
 void* context=nullptr;
 unsigned (*begin)(void*,TimingStage,unsigned,unsigned,unsigned,unsigned) noexcept=nullptr;
 void (*end)(void*,unsigned) noexcept=nullptr;
};
struct TimingScope {
 const Timing* timing=nullptr;unsigned ticket=~0u;
 TimingScope(const Timing* t,TimingStage s,unsigned ordinal=~0u,unsigned pipeline=~0u,unsigned x=0,unsigned y=0) noexcept :timing(t) {
  if(timing&&timing->begin&&timing->end)ticket=timing->begin(timing->context,s,ordinal,pipeline,x,y);
 }
 ~TimingScope(){if(ticket!=~0u)timing->end(timing->context,ticket);}
 TimingScope(const TimingScope&)=delete;TimingScope& operator=(const TimingScope&)=delete;
};
}
