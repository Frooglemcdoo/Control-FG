#include "../src/gi_batch_capture_policy.h"
#include <cstdio>
#include <cstdlib>
static void Check(bool ok){if(!ok)std::abort();}
int main(){
 using gi_batch_capture::Ledger;
 for(unsigned batches=1;batches<=8;++batches){Ledger x;
  for(unsigned b=0;b<batches;++b){Check(x.Begin(10,b,batches,16));x.Recorded();}
  Check(x.Seal(10));Check(!x.Begin(10,batches,batches,16));
 }
 {Ledger x;Check(!x.Begin(10,1,2,8));}
 {Ledger x;Check(x.Begin(10,0,2,8));x.Recorded();Check(!x.Begin(10,0,2,8));}
 {Ledger x;Check(x.Begin(10,0,2,8));x.Recorded();Check(!x.Seal(10));}
 {Ledger x;Check(x.Begin(10,0,2,8));x.Recorded();Check(!x.Begin(11,1,2,8));}
 {Ledger x;Check(x.Begin(10,0,2,8));x.Recorded();Check(!x.Begin(10,1,2,16));}
 {Ledger x;Check(!x.Begin(10,0,9,8));}
 {Ledger x;Check(!x.Begin(10,0,1,17));}
 {Ledger x;Check(!x.Seal(10));}
 {Ledger x;Check(x.Begin(10,0,1,8));x.Recorded();Check(!x.Begin(10,1,1,8));}
 std::puts("PASS GI batch completeness: contiguous batches, missing/duplicate/frame/count/overflow rejection");
}
