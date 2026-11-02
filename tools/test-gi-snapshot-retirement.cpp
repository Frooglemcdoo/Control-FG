#include "../src/gi_reuse/snapshot_retirement.h"
#include <cassert>
#include <iostream>
struct Owner {int* destroyed;explicit Owner(int& n):destroyed(&n){}~Owner(){++*destroyed;}};
using Policy=gi_reuse::SnapshotRetirement<Owner,3,2,192>;
bool Release(Policy& policy,std::uint64_t id){
 std::unique_ptr<Owner> retired;
 if(!policy.Retire(id,retired))return false;
 // In the runtime this destruction belongs outside observation/queue locks.
 retired.reset();return true;
}
int main(){
 int destroyed=0;Policy p;assert(p.RegisterQueue(11)&&p.RegisterQueue(22));assert(!p.RegisterQueue(33));
 auto owner=std::make_unique<Owner>(destroyed);const auto a=p.Admit(1,64,owner);assert(a&&!owner);
 const auto first=p.BeforeExecute(a,1,11);assert(first.serial&&!p.View(a));
 assert(!Release(p,a));assert(p.AfterExecute(first,11,1,true));assert(p.ObserveCompletion(11,1));
 assert(!p.View(a)&&!Release(p,a)); // Completed closed lists can still replay.
 const auto replay=p.BeforeExecute(a,1,11);assert(replay.serial!=first.serial);
 assert(p.AfterExecute(replay,11,2,true));assert(p.Seal(a,1));assert(!p.View(a)&&!Release(p,a));
 assert(p.ObserveCompletion(11,2));assert(p.View(a));std::unique_ptr<Owner> detached;assert(p.Retire(a,detached)&&destroyed==0);
 detached.reset();assert(destroyed==1);
 assert(!p.View(a)&&!Release(p,a));
 // Reset before submission completion releases recording eligibility, not GPU ownership.
 owner=std::make_unique<Owner>(destroyed);const auto b=p.Admit(2,64,owner);
 const auto pending=p.BeforeExecute(b,2,11);assert(p.Seal(b,2));assert(!Release(p,b));
 assert(p.AfterExecute(pending,11,3,true));assert(!p.View(b));
 assert(p.ObserveCompletion(11,3)&&p.View(b));assert(Release(p,b)&&destroyed==2);
 // Unsubmitted/reset recordings have no GPU work and can be released, not published.
 owner=std::make_unique<Owner>(destroyed);const auto unused=p.Admit(3,64,owner);
 assert(p.Seal(unused,3)&&!p.View(unused)&&Release(p,unused)&&destroyed==3);
 // Budget refusal must occur before recording; ownership stays with the caller.
 owner=std::make_unique<Owner>(destroyed);assert(!p.Admit(4,193,owner)&&owner);
 const auto c=p.Admit(4,192,owner);auto refused=std::make_unique<Owner>(destroyed);
 assert(!p.Admit(5,1,refused)&&refused&&p.RetainedBytes()==192);refused.reset();
 auto failure=p.BeforeExecute(c,4,11);assert(!p.AfterExecute(failure,11,4,false));
 assert(p.Quarantined(c)&&!p.View(c)&&!Release(p,c));p.Seal(c,4);
 assert(!p.ObserveCompletion(11,UINT64_MAX)&&!Release(p,c));
 const int beforeShutdown=destroyed;p.DeviceStopped();assert(destroyed==beforeShutdown+1&&p.RetainedBytes()==0);
 // Independent queues, wrong queue proofs, and later cross-queue replay.
 Policy q;q.RegisterQueue(11);q.RegisterQueue(22);
 owner=std::make_unique<Owner>(destroyed);const auto d=q.Admit(6,64,owner);
 auto write=q.BeforeExecute(d,6,11);assert(!q.AfterExecute(write,22,1,true));assert(q.Quarantined(d));
 owner=std::make_unique<Owner>(destroyed);const auto e=q.Admit(7,64,owner);
 write=q.BeforeExecute(e,7,11);assert(q.AfterExecute(write,11,1,true));assert(q.ObserveCompletion(11,1));
 assert(!q.BeforeExecute(e,7,22).serial&&q.Quarantined(e));
 owner=std::make_unique<Owner>(destroyed);const auto f=q.Admit(8,64,owner);
 assert(!q.BeforeExecute(f,9,11).serial&&q.Quarantined(f));q.DeviceStopped();
 // Duplicate/concurrent submission proofs cannot overwrite a pending ticket.
 Policy r;r.RegisterQueue(11);owner=std::make_unique<Owner>(destroyed);const auto g=r.Admit(1,64,owner);
 write=r.BeforeExecute(g,1,11);assert(!r.BeforeExecute(g,1,11).serial&&r.Quarantined(g));
 assert(!r.AfterExecute(write,11,1,true));assert(r.ObserveCompletion(11,1));r.Seal(g,1);assert(!Release(r,g));r.DeviceStopped();
 // One real signal can cover a native batch containing several recordings.
 Policy batch;batch.RegisterQueue(11);
 owner=std::make_unique<Owner>(destroyed);const auto x=batch.Admit(1,64,owner);
 owner=std::make_unique<Owner>(destroyed);const auto y=batch.Admit(2,64,owner);
 auto tx=batch.BeforeExecute(x,1,11),ty=batch.BeforeExecute(y,2,11);
 assert(batch.AfterExecute(tx,11,1,true)&&batch.AfterExecute(ty,11,1,true));
 batch.Seal(x,1);batch.Seal(y,2);assert(!batch.View(x)&&!batch.View(y));
 assert(batch.ObserveCompletion(11,1)&&batch.View(x)&&batch.View(y));
 assert(!batch.ObserveCompletion(11,2)); // Completion cannot invent an unissued signal.
 assert(batch.Quarantined(x)&&batch.Quarantined(y));batch.DeviceStopped();
 Policy wrap;wrap.RegisterQueue(11);owner=std::make_unique<Owner>(destroyed);
 const auto w=wrap.Admit(1,64,owner);write=wrap.BeforeExecute(w,1,11);
 assert(wrap.AfterExecute(write,11,UINT64_MAX-1,true));
 assert(!wrap.BeforeExecute(w,1,11).serial && wrap.Quarantined(w));wrap.DeviceStopped();
 Policy unknown;owner=std::make_unique<Owner>(destroyed);const auto u=unknown.Admit(1,64,owner);
 assert(!unknown.BeforeExecute(u,1,99).serial&&unknown.Quarantined(u));unknown.DeviceStopped();
 Policy sealed;sealed.RegisterQueue(11);owner=std::make_unique<Owner>(destroyed);
 const auto closed=sealed.Admit(1,64,owner);sealed.Seal(closed,1);
 assert(!sealed.BeforeExecute(closed,1,11).serial&&sealed.Quarantined(closed));sealed.DeviceStopped();
 Policy regression;regression.RegisterQueue(11);owner=std::make_unique<Owner>(destroyed);
 const auto back=regression.Admit(1,64,owner);write=regression.BeforeExecute(back,1,11);
 assert(regression.AfterExecute(write,11,2,true)&&regression.ObserveCompletion(11,2));
 assert(!regression.ObserveCompletion(11,1)&&regression.Quarantined(back));regression.DeviceStopped();
 std::cout<<"PASS snapshot lifetime: replay/reset, batch fence cuts, budgets, stale/wrong-queue proofs, failures and quarantine\n";
}
