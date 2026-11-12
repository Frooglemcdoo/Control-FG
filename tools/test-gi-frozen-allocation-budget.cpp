#include "../src/gi_reuse/frozen_allocation_budget.h"
#include <cassert>
#include <thread>
#include <vector>
#include <iostream>
int main(){
 using namespace gi_reuse;
 auto budget=std::make_shared<FrozenAllocationBudget>(100);
 {
  FrozenAllocationReservation active,idle,preparing;
  assert(active.Peak(budget,60)&&idle.Peak(budget,30));
  assert(!preparing.Peak(budget,11)&&budget->Held()==90);
  assert(idle.Peak(budget,40)&&budget->Held()==100); // old+new growth peak
  assert(idle.Settle(20)&&budget->Held()==80); // actual retained idle allocation
  assert(preparing.Peak(budget,20)&&!preparing.Peak(budget,21));
  assert(preparing.Settle(7)&&budget->Held()==87); // partial preparation failure
  assert(!active.Peak(std::make_shared<FrozenAllocationBudget>(1000),1));
 }
 assert(budget->Held()==0);
 std::vector<std::thread> threads;
 for(unsigned n=0;n<8;++n)threads.emplace_back([&]{for(unsigned j=0;j<10000;++j){
  FrozenAllocationReservation owner;
  if(owner.Peak(budget,30)){assert(budget->Held()<=budget->Limit());assert(owner.Settle(12));}
 }});
 for(auto& thread:threads)thread.join();
 assert(budget->Held()==0);
 {
  FrozenAllocationReservation owner;assert(owner.Peak(budget,10));
  assert(!owner.Settle(11));FrozenAllocationReservation later;assert(!later.Peak(budget,1));
 }
 assert(budget->Held()==0);
 std::cout<<"PASS shared allocation budget: active+idle+growth, partial failure, concurrent reservations and fail-closed accounting.\n";
}
