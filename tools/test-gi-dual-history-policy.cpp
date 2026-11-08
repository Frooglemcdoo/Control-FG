#include "../src/gi_dual_history_policy.h"
#include <cstdio>
#include <stdexcept>
using namespace gi_dual_history;
static void Check(bool b){if(!b)throw std::runtime_error("dual history ownership invariant");}
int main(){try{
 Policy p;auto a=p.Begin(1,1,1,1,0);Check(bool(a)&&a.previous==Invalid);
 Check(!p.Begin(1,1,1,1,0));Check(!p.Submitted(a,2,1,1));Check(!p.Submitted(a,1,2,1));
 Check(p.Submitted(a,1,1,1));auto b=p.Begin(2,1,1,2,0);Check(b.previous==a.output);Check(p.Get(a.output).pins==1);
 // A source remains pinned while its reader has not reached submission.
 auto unrelated=p.Begin(9,1,1,3,1);Check(bool(unrelated)&&unrelated.output!=a.output);
 Check(p.Cancel(unrelated,3));Check(!p.Cancel(b,3));Check(p.Submitted(b,1,2,2));Check(p.Get(a.output).lastUse==2);
 auto c=p.Begin(3,1,1,4,0);Check(c.previous==b.output);Check(p.Submitted(c,1,4,3));
 auto d=p.Begin(4,1,1,5,0);Check(d.previous==c.output);Check(p.Submitted(d,1,5,4));
 Check(!p.Begin(5,1,1,6,0));auto e=p.Begin(5,1,1,6,2);Check(bool(e)&&e.previous==d.output);Check(p.Cancel(e,6));
 auto resized=p.Begin(6,2,1,7,4);Check(bool(resized)&&resized.previous==Invalid);Check(p.Cancel(resized,7));
 auto otherQueue=p.Begin(5,1,2,8,4);Check(!otherQueue);
 // Completed GPU work does not release a recording that can still replay.
 Check(!p.Begin(7,2,1,10,100,0));
 auto masked=p.Begin(7,2,1,11,100,1u<<a.output);Check(bool(masked)&&masked.output==a.output);Check(p.Cancel(masked,11));
 p.Poison();Check(!p.Begin(7,2,1,9,100));
 Policy q;auto pending=q.Begin(1,1,1,1,0);auto next=q.Begin(2,1,1,2,0);Check(next.previous==Invalid);Check(q.Cancel(next,2));Check(q.Cancel(pending,1));
 puts("PASS dual-history leases: writer/reader retirement, frame/epoch/queue identity, cancellation, exhaustion and ambiguous failure");return 0;
}catch(const std::exception&e){puts(e.what());return 1;}}
