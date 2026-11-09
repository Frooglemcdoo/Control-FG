#pragma once
#include <cstdint>
#include <unordered_map>
namespace gi_reuse {
enum class LocalStateKind:unsigned {Unknown,Explicit,Pending,Invalid};
struct LocalState {LocalStateKind kind{};std::uint32_t before=0,after=0;};
// Recording evidence only. Never infer initial, promoted, decayed or submitted
// states. Keys are owner-inventory generations, not reusable pointers/addresses.
template<std::size_t Capacity=8192> class LocalBufferStates {
 std::unordered_map<std::uint64_t,LocalState> states_;
 bool complete_=true;
public:
 void Clear() noexcept {states_.clear();} // aliasing invalidates prior evidence
 bool Complete() const noexcept {return complete_;}
 LocalState Find(std::uint64_t id) const noexcept {
  if(!complete_||!id)return {};
  const auto it=states_.find(id);return it==states_.end()?LocalState{}:it->second;
 }
 void Transition(std::uint64_t id,std::uint32_t before,std::uint32_t after,unsigned flags) noexcept {
  if(!complete_||!id)return;
  try {
   auto it=states_.find(id);
   if(it==states_.end()){
    if(states_.size()==Capacity){complete_=false;states_.clear();return;}
    it=states_.emplace(id,LocalState{}).first;
   }
   auto& s=it->second;if(s.kind==LocalStateKind::Invalid)return;
   if(flags==1){if(s.kind==LocalStateKind::Pending){s={LocalStateKind::Invalid,0,0};return;}s={LocalStateKind::Pending,before,after};return;}
   if(flags==2){
    if(s.kind==LocalStateKind::Pending&&s.before==before&&s.after==after)s={LocalStateKind::Explicit,before,after};
    else s={LocalStateKind::Invalid,0,0};
    return;
   }
   if(flags||s.kind==LocalStateKind::Pending){s={LocalStateKind::Invalid,0,0};return;}
   s={LocalStateKind::Explicit,before,after};
  }catch(...){complete_=false;states_.clear();}
 }
};
}
