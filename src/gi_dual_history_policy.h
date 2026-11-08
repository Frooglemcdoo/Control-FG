#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
// Single owning recording thread under the runtime's mutex. No D3D calls.
// Sidecars are reusable only after their last writer AND reader have retired.
namespace gi_dual_history {
using Id=std::uint64_t;
constexpr unsigned Slots=4,Invalid=Slots;
enum class Phase {Idle,Recording,Submitted,Poisoned};
struct Slot {Phase phase=Phase::Idle;Id serial=0,frame=0,epoch=0,queue=0,generation=0,lastUse=0;unsigned pins=0;};
struct Lease {unsigned output=Invalid,previous=Invalid;Id serial=0,previousSerial=0;explicit operator bool()const{return output<Slots;}};
class Policy {
 std::array<Slot,Slots> slots{};Id next=1,ownerQueue=0,lastFence=0;bool failed=false;
public:
 const Slot& Get(unsigned i)const{return slots.at(i);}
 bool Failed()const{return failed;}
 Lease Begin(Id frame,Id epoch,Id queue,Id generation,Id completed){
  if(failed||!frame||!epoch||!queue||!generation||(ownerQueue&&queue!=ownerQueue))return {};
  ownerQueue=queue;
  unsigned previous=Invalid;
  for(unsigned i=0;i<Slots;++i){const auto&s=slots[i];
   if(s.phase==Phase::Submitted&&s.frame+1==frame&&s.epoch==epoch&&s.queue==queue)previous=i;
   // Never record the same logical frame twice into the history chain.
   if(s.phase!=Phase::Idle&&s.frame==frame&&s.epoch==epoch&&s.queue==queue)return {};
  }
  for(unsigned i=0;i<Slots;++i){auto&s=slots[i];
   if(i==previous||s.pins||s.phase==Phase::Recording||s.phase==Phase::Poisoned||s.lastUse>completed)continue;
   const Id serial=next++;s={Phase::Recording,serial,frame,epoch,queue,generation,0,0};
   Lease l{i,previous,serial,previous<Slots?slots[previous].serial:0};
   if(previous<Slots)++slots[previous].pins;
   return l;
  }return {}; // Never wait or overwrite in-flight history.
 }
 bool Validate(const Lease&l,Id queue,Id generation)const{
  if(failed||!l)return false;
  const auto&s=slots[l.output];
  if(s.phase!=Phase::Recording||s.serial!=l.serial||s.queue!=queue||s.generation!=generation)return false;
  if(l.previous<Slots){const auto&p=slots[l.previous];if(p.phase!=Phase::Submitted||p.serial!=l.previousSerial||p.queue!=queue||!p.pins)return false;}
  return true;
 }
 bool Submitted(const Lease&l,Id queue,Id generation,Id fence){
  if(fence<=lastFence||!Validate(l,queue,generation))return false;
  lastFence=fence;
  auto&s=slots[l.output];if(l.previous<Slots){auto&p=slots[l.previous];p.lastUse=std::max(p.lastUse,fence);--p.pins;}
  s.phase=Phase::Submitted;s.lastUse=fence;return true;
 }
 // Call only after actual list reset/cancellation proves it cannot execute.
 bool Cancel(const Lease&l,Id generation){
  if(!l||slots[l.output].serial!=l.serial||slots[l.output].phase!=Phase::Recording||slots[l.output].generation!=generation)return false;
  if(l.previous<Slots){auto&p=slots[l.previous];if(p.serial!=l.previousSerial||!p.pins){Poison();return false;}--p.pins;}
  slots[l.output]={};return true;
 }
 // Ambiguous submission/signal failure: retain every allocation, disable reuse.
 void Poison(){failed=true;for(auto&s:slots)s.phase=Phase::Poisoned;}
};
}
