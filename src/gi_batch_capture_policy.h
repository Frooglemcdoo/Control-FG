#pragma once
#include <cstdint>
namespace gi_batch_capture {
inline constexpr unsigned MaxBatches=8, MaxSlices=16, RegionSize=128, Regions=2;
inline constexpr std::uint64_t Capacity=128ull*1024*1024;
// A missing/duplicate batch must never produce a successful capture.
struct Ledger {
 std::uint64_t frame=0;
 unsigned expected=0,recorded=0,rays=0;
 bool failed=false,sealed=false;
 bool Begin(std::uint64_t f,unsigned batch,unsigned batches,unsigned rayCount) noexcept {
  if(failed||sealed||!f||!batches||batches>MaxBatches||!rayCount||rayCount>MaxSlices||batch>=batches||batch!=recorded||
     (recorded&&(f!=frame||batches!=expected||rayCount!=rays))){failed=true;return false;}
  if(!recorded){frame=f;expected=batches;rays=rayCount;}
  return true;
 }
 void Recorded() noexcept {++recorded;}
 bool Seal(std::uint64_t f) noexcept {
  sealed=true;
  if(f!=frame||!recorded||recorded!=expected)failed=true;
  return !failed;
 }
};
}
