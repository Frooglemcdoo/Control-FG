#include "../src/gi_stage_timing_policy.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace gi_stage_timing;
 Ledger l;
 assert(l.Begin(0,1,Stage::Evaluator)==~0u);
 assert(l.Begin(10,20,Stage::Evaluator)==0);
 ++l.ended;
 assert(l.Begin(11,21,Stage::Evaluator)==~0u);
 assert(!l.Complete(10));
 assert(l.Begin(10,20,Stage::TemporalDispatch)==1);++l.ended;
 assert(l.Begin(10,20,Stage::Reconstruction)==2);
 assert(!l.Complete(10));++l.ended;
 assert(!l.Complete(10));
 l.Batch(0,1,true);
 assert(l.Complete(10)&&!l.Complete(11));
 Ledger missing=l;missing.expectedBatches=2;assert(!missing.Complete(10));
 Ledger duplicate=l;duplicate.Batch(0,1,true);assert(!duplicate.Complete(10));
 Ledger changed=l;changed.Batch(1,2,true);assert(!changed.Complete(10));
 Ledger invalid=l;invalid.Batch(1,1,false);assert(!invalid.Complete(10));
 assert(l.Begin(10,20,Stage::Reconstruction)==3);++l.ended;
 assert(!l.Complete(10));
 Ledger full;
 for(unsigned i=0;i<capacity;++i)assert(full.Begin(10,20,Stage::Evaluator)==i);
 assert(full.Begin(10,20,Stage::Evaluator)==~0u&&full.overflow);
 double ms=0;
 assert(Milliseconds(100,250,1000000,ms)&&std::abs(ms-0.15)<1e-12);
 assert(!Milliseconds(250,100,1000000,ms));
 assert(!Milliseconds(100,250,0,ms));
 std::cout<<"PASS timestamp admission, required coverage, frame isolation, overflow and invalid durations\n";
}
