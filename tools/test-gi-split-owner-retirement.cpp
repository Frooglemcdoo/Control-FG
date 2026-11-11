#include "../src/gi_reuse/split_owner_retirement.h"
#include <cassert>
#include <iostream>
struct Owner {int* deaths;explicit Owner(int& n):deaths(&n){}~Owner(){++*deaths;}};
using Pool=gi_reuse::GISplitOwnerRetirement<Owner,2,100>;
int main(){
 int deaths=0;Pool p;auto owner=std::make_unique<Owner>(deaths);
 const auto id=p.Admit(7,{10,1},60,owner);assert(id&&!owner&&p.RetainedBytes()==60);
 auto rejected=std::make_unique<Owner>(deaths);assert(!p.Admit(7,{11,1},41,rejected)&&rejected);
 Pool::Recording a{10,1},b{20,1};auto first=p.Before(99,&a,1);assert(first.count==1);
 assert(p.After(first,99,1,true));p.Complete(99,1);assert(p.Seal(a));
 std::unique_ptr<Owner> out;assert(!p.Take(id,out)&&p.View(id)&&deaths==0);
 assert(p.Attach(id,7,b));auto second=p.Before(99,&b,1);assert(second.count==1);
 assert(p.After(second,99,2,true));assert(p.Seal(b));assert(!p.Take(id,out));
 p.Complete(99,2);assert(p.Take(id,out)&&p.RetainedBytes()==0&&deaths==0);out.reset();assert(deaths==1);
 // Budget admission is transactional; failure retains caller ownership.
 rejected.reset();assert(deaths==2);
 for(unsigned scenario=0;scenario<6;++scenario){
  Pool pool;auto o=std::make_unique<Owner>(deaths);auto token=pool.Admit(1,a,50,o);assert(token&&pool.Attach(token,1,b));
  Pool::Batch batch;
  if(scenario==0){Pool::Recording reversed[]={b,a};batch=pool.Before(99,reversed,2);}
  if(scenario==1){Pool::Recording duplicate[]={a,a};batch=pool.Before(99,duplicate,2);}
  if(scenario==2){Pool::Recording together[]={a,b};batch=pool.Before(99,together,2);}
  if(scenario>=3){batch=pool.Before(99,&a,1);assert(batch.count==1);
   if(scenario==3)assert(!pool.After(batch,99,1,false));
   else {assert(pool.After(batch,99,1,true));
    if(scenario==4)batch=pool.Before(100,&b,1); // cross queue
    else batch=pool.Before(99,&a,1); // replay
   }
  }
  assert(pool.Quarantined(token));assert(pool.Seal(a)==false);pool.Seal(b);pool.Complete(99,100);
  assert(!pool.Take(token,out)&&pool.RetainedBytes()==50);const int before=deaths;
  pool.DeviceStopped();assert(deaths==before+1&&pool.RetainedBytes()==0);
 }
 // Same list pointer is reusable only AFTER the producer's successful Reset.
 Pool reuse;auto o=std::make_unique<Owner>(deaths);auto token=reuse.Admit(1,a,50,o);
 assert(!reuse.Attach(token,1,{10,2}));assert(reuse.Seal(a));assert(reuse.Attach(token,1,{10,2}));
 assert(reuse.Seal({10,2})&&reuse.Take(token,out));out.reset();
 std::cout<<"PASS shared invocation ownership: no early release, transactional budget, actual batch ordering, replay, failure pinning, Reset generations\n";
}
