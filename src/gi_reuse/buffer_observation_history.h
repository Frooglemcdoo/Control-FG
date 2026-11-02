#pragma once
#include <array>
#include <cstdint>
#include <unordered_map>
namespace gi_reuse {
enum class BufferEvent:unsigned {Unknown,Admitted,Evicted,Rejected,Excluded};
struct BufferEvidence {
 std::uint64_t address=0,bytes=0,sequence=0;
 BufferEvent event=BufferEvent::Unknown;
 unsigned rank=0;
};
// Bounded CPU metadata only, keyed by GPU base address, never a resource owner.
// Address reuse and overlapping allocations mean this is historical evidence,
// not proof of the lifetime or identity of a missing resource. Caller locks.
template<std::size_t Capacity> class BufferObservationHistory {
 static_assert(Capacity>0,"history needs storage");
 std::array<BufferEvidence,Capacity> entries_{};
 std::unordered_map<std::uint64_t,std::size_t> slots_;
 std::size_t cursor_=0;
 std::uint64_t sequence_=0,overwritten_=0,dropped_=0;
public:
 void Record(std::uint64_t address,std::uint64_t bytes,BufferEvent event,unsigned rank) noexcept {
  if(!address||!bytes||bytes>UINT64_MAX-address)return;
  const auto existing=slots_.find(address);
  if(existing!=slots_.end()){entries_[existing->second]={address,bytes,++sequence_,event,rank};return;}
  // Reserve first so allocation failure leaves the prior evidence intact.
  try{slots_.emplace(address,cursor_);}catch(...){++dropped_;return;}
  auto& e=entries_[cursor_];if(e.sequence){slots_.erase(e.address);++overwritten_;}
  e={address,bytes,++sequence_,event,rank};cursor_=(cursor_+1)%Capacity;
 }
 BufferEvidence Find(std::uint64_t address,std::uint64_t bytes) const noexcept {
  BufferEvidence found{};if(!address||!bytes||bytes>UINT64_MAX-address)return found;
  for(const auto& e:entries_)if(e.sequence>found.sequence&&e.address<=address&&address-e.address<=e.bytes&&bytes<=e.bytes-(address-e.address))found=e;
  return found;
 }
 void Stats(std::uint64_t& overwritten,std::uint64_t& dropped) const noexcept {overwritten=overwritten_;dropped=dropped_;}
};
}
