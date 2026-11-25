#pragma once
#include <atomic>
#include <memory>
namespace gi_motion {
// A GPU owner and its disk job share one reservation until both have released
// their resources. Declare this first so resource destructors run before it.
class ExportLease {
 struct Slot {
  std::atomic<unsigned>* count=nullptr;
  ~Slot(){if(count)count->fetch_sub(1);}
 };
 std::shared_ptr<Slot> slot;
public:
 bool Acquire(std::atomic<unsigned>& count,unsigned capacity) {
  if(slot)return false;
  auto ticket=std::make_shared<Slot>();
  unsigned used=count.load();
  while(used<capacity){
   if(count.compare_exchange_weak(used,used+1)){
    ticket->count=&count;slot=std::move(ticket);return true;
   }
  }
  return false;
 }
};
}
