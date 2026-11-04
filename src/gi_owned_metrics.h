#pragma once
#include <windows.h>
#include <atomic>
#include <array>
namespace GIOwnedMetrics {
enum Stage {Geometry,Writer,Bindings,Prepare,Submission,Admission,Selection,Mapping,TableCommit,Controls,OwnerLock,MappingWork,OwnerLeases,DeferredResolve,Count};
struct Counter {std::atomic<UINT64> calls{0},ticks{0},maximum{0};};
inline std::array<Counter,Count> counters;
class Scope {
 Stage stage_;LARGE_INTEGER start_{};
public:
 explicit Scope(Stage stage) noexcept:stage_(stage){QueryPerformanceCounter(&start_);}
 ~Scope(){LARGE_INTEGER end{};QueryPerformanceCounter(&end);const auto ticks=UINT64(end.QuadPart-start_.QuadPart);auto& c=counters[stage_];
  ++c.calls;c.ticks+=ticks;auto old=c.maximum.load();while(old<ticks&&!c.maximum.compare_exchange_weak(old,ticks)){}
 }
};
inline void RecordMS(Stage stage,double ms) noexcept {
 LARGE_INTEGER f{};QueryPerformanceFrequency(&f);const auto ticks=UINT64(ms*double(f.QuadPart)/1000.0);
 auto& c=counters[stage];++c.calls;c.ticks+=ticks;
 auto old=c.maximum.load();while(old<ticks&&!c.maximum.compare_exchange_weak(old,ticks)){}
}
inline void Report() noexcept {
 static const char* names[]={"geometry_preservation","writer_snapshot","native_binding_snapshot","owned_prepare","queue_tracking","scene_admission","scene_selection","scene_mapping","table_commit","control_commit","owner_lock_wait","mapping_work","owner_leases","deferred_resolve"};
 LARGE_INTEGER f{};QueryPerformanceFrequency(&f);if(!f.QuadPart)return;
 for(unsigned i=0;i<Count;++i){auto& c=counters[i];const auto n=c.calls.exchange(0),t=c.ticks.exchange(0),m=c.maximum.exchange(0);
  if(n)Log("GI_OWNED_CPU stage=%s calls=%llu total_ms=%.3f max_ms=%.3f inclusive=1",names[i],n,double(t)*1000.0/double(f.QuadPart),double(m)*1000.0/double(f.QuadPart));
 }
}
}
