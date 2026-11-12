#include "../src/rt_ptlas_submission.h"
#include <cassert>
#include <iostream>
using namespace rt_ptlas;
int main(){
 Instance a{};a.transform[0]=a.transform[5]=a.transform[10]=1;a.blas=256;a.idMask=0xff000007;
 SubmissionJournal j(4,1,10,20);
 auto first=j.Prepare({a},{0});assert(j.Prepared(first).initialize);
 // A partially recorded initialization is discarded, not committed.
 assert(j.DiscardAfterReset(first));
 auto t=j.Prepare({a},{0});assert(j.Prepared(t).initialize&&j.Prepared(t).writes.size()==4);
 assert(!j.BeginExecute(t,10,20,1));assert(j.Recorded(t));assert(!j.Recorded(t));assert(j.Closed(t));
 assert(!j.BeginExecute(t,11,20,1));assert(!j.BeginExecute(t,10,21,1));assert(!j.BeginExecute(t,10,20,0));
 assert(j.BeginExecute(t,10,20,1));assert(!j.DiscardAfterReset(t));assert(j.Submitted(t,true));assert(!j.Submitted(t,true));
 // No completion before the next update: it derives from submitted history.
 a.transform[3]=2;t=j.Prepare({a},{0});assert(!j.Prepared(t).initialize&&j.Prepared(t).writes.size()==1);
 assert(j.Recorded(t)&&j.Closed(t));assert(!j.BeginExecute(t,10,20,1));assert(j.BeginExecute(t,10,20,2)&&j.Submitted(t,true));
 assert(j.InFlight()==2&&j.LastSubmitted()==2);
 // An unchanged record has no build, but its consumer still has a submission.
 t=j.Prepare({a},{0});assert(j.Prepared(t).writes.empty());assert(j.Recorded(t)&&j.Closed(t));
 assert(j.BeginExecute(t,10,20,3)&&j.Submitted(t,true));
 auto removed=j.Prepare({},{});assert(j.Prepared(removed).writes.size()==1);assert(j.Recorded(removed)&&j.Closed(removed));
 assert(j.DiscardAfterReset(removed));t=j.Prepare({a},{0});assert(j.Prepared(t).writes.empty());assert(j.DiscardAfterReset(t));
 assert(!j.Completed(11,20,3)&&!j.Completed(10,21,3)&&!j.Completed(10,20,4)&&!j.Completed(10,20,UINT64_MAX));
 assert(j.Completed(10,20,1)&&j.InFlight()==2);assert(j.Completed(10,20,0)&&j.InFlight()==2);
 assert(j.Completed(10,20,3)&&j.InFlight()==0);
 // Queue failure quarantines the executed recording and refuses more planning.
 t=j.Prepare({},{});assert(j.Recorded(t)&&j.Closed(t)&&j.BeginExecute(t,10,20,4));assert(!j.Submitted(t,false));
 assert(j.Faulted()&&j.InFlight()==1&&!j.Completed(10,20,4)&&!j.DiscardAfterReset(t));
 bool rejected=false;try{j.Prepare({a},{0});}catch(const std::logic_error&){rejected=true;}assert(rejected);
 SubmissionJournal bounded(4,1,10,20);
 for(unsigned i=1;i<=128;++i){t=bounded.Prepare({a},{0});assert(bounded.Recorded(t)&&bounded.Closed(t)&&bounded.BeginExecute(t,10,20,i)&&bounded.Submitted(t,true));}
 rejected=false;try{bounded.Prepare({a},{0});}catch(const std::logic_error&){rejected=true;}assert(rejected);
 assert(bounded.Completed(10,20,128));t=bounded.Prepare({a},{0});assert(bounded.DiscardAfterReset(t));
 std::cout<<"PASS queued PTLAS planning: discard, 128 in-flight submissions, wrong timeline, stale ticket, failed Signal quarantine\n";
}
