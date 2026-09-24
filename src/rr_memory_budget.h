#pragma once
#include <cstdint>
namespace control_rr {
// Allocation admission uses the OS process budget on the rendering adapter.
// Leave 10% of that budget for subsequent game/RR/FG growth. This is a snapshot,
// not a reservation or a promise that later allocations will succeed.
struct MemoryBudget {
 std::uint64_t budget=0,usage=0;
 std::uint64_t Reserve() const noexcept {return budget/10;}
 std::uint64_t Allowance() const noexcept {
  if(usage>=budget)return 0;
  const auto free=budget-usage,reserve=Reserve();
  return free>reserve?free-reserve:0;
 }
};
}
