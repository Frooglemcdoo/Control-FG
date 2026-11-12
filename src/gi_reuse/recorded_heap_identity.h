#pragma once
#include <array>
#include <cstdint>
namespace gi_reuse {
// Exact recording-time identity, independent of the submission thread's TLS.
// This is a validity check, NOT a lock or a scene/descriptor-content lease.
// The host must exclude mutation while sampling and retain the native records.
struct GIRecordedHeapIdentity {
 using U64=std::uint64_t;
 struct Heap {
  U64 record=0,object=0,epoch=0,generation=0;
  std::uint32_t cursor=0;
 };
 struct Sample {
  U64 frame=0,deviceFenceRecord=0,deviceFenceObject=0,deviceEpoch=0;
  U64 completed=0;
  std::array<Heap,2> heaps{};
 };
private:
 Sample saved_{};
 bool captured_=false;
 static bool Valid(const Sample& s,const std::array<U64,2>& bound) noexcept {
  if(!s.frame||!s.deviceFenceRecord||!s.deviceFenceObject||!s.deviceEpoch||
     s.completed>=s.deviceEpoch)return false;
  for(unsigned i=0;i<2;++i){const auto& h=s.heaps[i];
   if(!h.record||!h.object||h.object!=bound[i]||h.epoch!=s.deviceEpoch)return false;
  }
  return s.heaps[0].record!=s.heaps[1].record&&s.heaps[0].object!=s.heaps[1].object;
 }
public:
 bool Capture(const Sample& s,const std::array<U64,2>& bound) noexcept {
  if(captured_||!Valid(s,bound))return false;
  saved_=s;captured_=true;return true;
 }
 // 'now' must be read from saved_.heaps[*].record, never the current provider.
 // Cursor advance appends disjoint native tables; rollback/recycling rejects.
 bool ValidateSaved(const Sample& now,const std::array<U64,2>& bound)const noexcept {
  if(!captured_||!Valid(now,bound)||now.frame!=saved_.frame||
     now.deviceFenceRecord!=saved_.deviceFenceRecord||now.deviceFenceObject!=saved_.deviceFenceObject||
     now.deviceEpoch!=saved_.deviceEpoch||now.completed<saved_.completed)return false;
  for(unsigned i=0;i<2;++i){const auto& a=saved_.heaps[i];const auto& b=now.heaps[i];
   if(a.record!=b.record||a.object!=b.object||a.epoch!=b.epoch||a.generation!=b.generation||b.cursor<a.cursor)return false;
  }
  return true;
 }
 const Sample* Saved()const noexcept{return captured_?&saved_:nullptr;}
};
}
