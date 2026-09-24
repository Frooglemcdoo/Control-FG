#include "fg_source_policy.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 using control_fg_source::Tracker;
 Tracker t;int owner=0,other=0,buffers[2]{},lists[260]{};bool observed=false;
 assert(t.Register(&owner,&buffers[0],0));assert(t.Register(&owner,&buffers[1],1));
 assert(!t.Register(&other,&buffers[0],0));assert(!t.Register(&owner,&buffers[0],8));
 // Recording alone must never select an unsubmitted buffer.
 t.Record(&lists[0],&buffers[1]);assert(t.Take(&owner,0,observed)==0&&!observed);
 t.Commit(t.Submitted(&lists[0]),true);assert(t.Take(&owner,0,observed)==1&&observed);
 assert(t.Take(&owner,0,observed)==0&&!observed); // No stale reuse on next Present.
 // R5 broken capture: native writer alternates opposite to DXGI.
 const unsigned destinations[]={0,1,0,1,1,0,1,0};
 const unsigned writers[]={1,0,1,0,0,1,0,1};
 for(unsigned i=0;i<8;++i){t.Record(&lists[0],&buffers[writers[i]]);t.Commit(t.Submitted(&lists[0]),true);assert(t.Take(&owner,destinations[i],observed)==writers[i]&&observed);}
 // Healthy R4 capture: same-index writes must remain same-index.
 for(unsigned index:{1u,0u,1u,0u}){t.Record(&lists[0],&buffers[index]);t.Commit(t.Submitted(&lists[0]),true);assert(t.Take(&owner,index,observed)==index&&observed);}
 t.Record(&lists[0],&buffers[1]);t.Reset(&lists[0]);t.Commit(t.Submitted(&lists[0]),true);assert(t.Take(&owner,0,observed)==0&&!observed);
 t.Record(&lists[0],&buffers[1]);t.Commit(t.Submitted(&lists[0]),false);assert(t.Take(&owner,0,observed)==0&&!observed);
 t.Record(&lists[0],&buffers[1]);auto stale=t.Submitted(&lists[0]);t.Clear(&owner);
 assert(t.Register(&owner,&buffers[0],0));t.Commit(stale,true);assert(t.Take(&owner,0,observed)==0&&!observed);
 assert(t.Register(&owner,&buffers[1],1));
 t.Record(&lists[0],&buffers[0]);t.Record(&lists[0],&buffers[1]);t.Commit(t.Submitted(&lists[0]),true);assert(t.Take(&owner,0,observed)==1&&observed);
 for(unsigned i=0;i<257;++i)t.Record(&lists[i],&buffers[1]);
 t.Commit(t.Submitted(&lists[0]),true);assert(t.Take(&owner,0,observed)==0&&!observed);
 std::puts("PASS submitted-source selection: healthy/reversed, no submission, reset, foreign queue, resize epoch, latest write, capacity fallback");
}
