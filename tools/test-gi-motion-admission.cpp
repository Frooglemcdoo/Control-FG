#include "../src/gi_reuse/motion_capture_layout.h"
#include "../src/gi_reuse/split_owner_retirement.h"
#include <cassert>
#include <iostream>
struct Owner {};
using Pool=gi_reuse::GISplitOwnerRetirement<Owner,8,gi_motion::MaxOwnerBudget>;
int main(){
 const auto trace=gi_motion::Bytes(2560,1440,4);
 const auto bytes=2*trace+256ull*1024*1024;
 const auto budget=gi_motion::OwnerBudget(2560,1440,4);
 assert(2*bytes<gi_motion::NormalOwnerBudget&&3*bytes>gi_motion::NormalOwnerBudget);
 assert(budget<=gi_motion::MaxOwnerBudget);
 Pool old;for(unsigned i=1;i<=2;++i){auto p=std::make_unique<Owner>();assert(old.Admit(i,{i,1},bytes,p,gi_motion::NormalOwnerBudget));}
 auto third=std::make_unique<Owner>();assert(old.CheckAdmission(3,{3,1},bytes,true,gi_motion::NormalOwnerBudget)==Pool::Admission::RetainedBudget);
 assert(!old.Admit(3,{3,1},bytes,third,gi_motion::NormalOwnerBudget)&&third);old.DeviceStopped();
 Pool pool;
 for(unsigned i=1;i<=gi_motion::Frames;++i){
  // Two previous frames can still be retained when a third frame records.
  auto p=std::make_unique<Owner>();auto id=pool.Admit(i,{i*2,1},bytes,p,budget);assert(id);
  Pool::Recording a{i*2,1},b{i*2+1,1};auto batch=pool.Before(99,&a,1);assert(pool.After(batch,99,i*2,true));
  assert(pool.Attach(id,i,b));batch=pool.Before(99,&b,1);assert(pool.After(batch,99,i*2+1,true));
  pool.Complete(99,i*2+1);
  std::unique_ptr<Owner> out;assert(!pool.TakeReady(out)); // GPU completion alone must not release.
  if(i>=3){const unsigned done=i-2;assert(pool.Seal({done*2,1}));assert(pool.Seal({done*2+1,1}));assert(pool.TakeReady(out));}
 }
 pool.DeviceStopped();
 Pool slots;for(unsigned i=1;i<=8;++i){auto p=std::make_unique<Owner>();assert(slots.Admit(i,{i,1},1,p));}
 assert(slots.CheckAdmission(9,{9,1},1,true)==Pool::Admission::Slots);slots.DeviceStopped();
 // Four overlapping GPU trace buffers are covered, plus all sixteen CPU outputs.
 assert(gi_motion::Reservation(2560,1440,4,0)>=gi_motion::Frames*trace+gi_motion::InFlight*trace);
 assert(!gi_motion::Addressable(3840,2160,4,256ull*1024*1024));
 std::cout<<"PASS R490: reproduced third-frame budget failure; 16 consecutive frames with delayed seals; slot/budget reasons distinct; retention safe\n";
}
