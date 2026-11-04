#include "../src/gi_reuse/writer_handoff.h"
#include <iostream>
#include <stdexcept>
static void Require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
int main(){try {
 gi_reuse::GIWriterHandoff<int> handoff;
 const auto writer=std::make_shared<const int>(42);
 Require(handoff.Publish(1,10,writer),"publish failed");
 Require(handoff.Take(1,10)==writer,"list rotation lost invocation writer");
 Require(!handoff.Take(1,10),"writer reused twice");
 handoff.Publish(2,10,writer);
 Require(!handoff.Take(3,10)&&!handoff.Take(2,10),"stale invocation survived");
 handoff.Publish(4,10,writer);
 Require(!handoff.Take(4,11),"prior frame accepted");
 handoff.Publish(5,11,writer);handoff.Clear();
 Require(!handoff.Take(5,11),"cancelled publication survived");
 Require(!handoff.Publish(0,11,writer)&&!handoff.Publish(6,0,writer),"unknown identity accepted");
 gi_reuse::GIWriterSubmission producer(17);
 // A consumer visited before a producer cannot become admissible later when
 // the rest of the Execute batch is walked.
 const bool reversedBatch=producer.Precedes(2);
 Require(!reversedBatch,"unsubmitted producer accepted");
 Require(producer.Submit(2,17)&&producer.Precedes(2),"ordered producer rejected");
 Require(!reversedBatch&&!producer.Precedes(3),"reversed or independent queue accepted");
 Require(!producer.Submit(2,17)&&!producer.Precedes(2),"replay accepted");
 gi_reuse::GIWriterSubmission reset(18);
 Require(!reset.Submit(2,19)&&!reset.Precedes(2),"reset generation accepted");
 gi_reuse::GIWriterSubmission abandoned(20);
 Require(!abandoned.Precedes(2),"discarded recording accepted");
 // Receipt contains no graph cut, so compaction cannot turn it into a stale
 // index. Same-queue admission remains valid while the writer owner survives.
 gi_reuse::GIWriterSubmission retained(21);
 Require(retained.Submit(2,21),"retained submission failed");
 for(int i=0;i<2000;++i)Require(retained.Precedes(2),"receipt lifetime failed");
 std::cout<<"PASS single-use invocation/frame handoff, list rotation, queue ordering, reset, replay and abandonment\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
