#include "../src/gi_reuse/split_recording_lease.h"
#include <cassert>
#include <iostream>
using Lease=gi_reuse::GISplitRecordingLease;
int main(){
 Lease p;assert(p.Begin(1,10,1));assert(p.BeforeExecute(1,100));assert(p.AfterExecute(100,1,true));
 assert(p.Seal(10,1));assert(p.ObserveCompletion(100,1));assert(!p.Retirable());
 // Producer list reset/completion is NOT the last consumer's completion.
 assert(p.AttachConsumer(1,20,1));assert(p.BeforeExecute(2,100));
 assert(p.Seal(20,1));assert(!p.Retirable());assert(p.AfterExecute(100,2,true));
 assert(!p.Retirable());assert(p.SubmittedBoth());assert(p.ObserveCompletion(100,2));assert(p.Retirable());
 Lease batch;assert(batch.Begin(1,10,1)&&batch.AttachConsumer(1,20,1));
 assert(batch.BeforeExecute(3,100)&&batch.AfterExecute(100,12,true));
 assert(batch.ObserveCompletion(100,50));assert(!batch.Retirable());
 assert(batch.Seal(10,1)&&batch.Seal(20,1)&&batch.Retirable());
 Lease reverse;assert(reverse.Begin(1,10,1)&&reverse.AttachConsumer(1,20,1));
 assert(!reverse.BeforeExecute(2,100)&&reverse.Quarantined());
 Lease replay;assert(replay.Begin(1,10,1)&&replay.BeforeExecute(1,100)&&replay.AfterExecute(100,1,true));
 assert(!replay.BeforeExecute(1,100)&&replay.Quarantined());
 Lease cross;assert(cross.Begin(1,10,1)&&cross.AttachConsumer(1,20,1));
 assert(cross.BeforeExecute(1,100)&&cross.AfterExecute(100,1,true));
 assert(!cross.BeforeExecute(2,200)&&cross.Quarantined());
 Lease overlap;assert(overlap.Begin(1,10,1)&&overlap.AttachConsumer(1,20,1)&&overlap.BeforeExecute(1,100));
 assert(!overlap.BeforeExecute(2,100)&&overlap.Quarantined());
 Lease failure;assert(failure.Begin(1,10,1)&&failure.BeforeExecute(1,100));
 assert(!failure.AfterExecute(100,1,false)&&failure.Quarantined()&&!failure.Retirable());
 Lease cancel;assert(cancel.Begin(1,10,1)&&cancel.CancelUnattachedConsumer());
 assert(!cancel.AttachConsumer(1,20,1));assert(cancel.Seal(10,1)&&cancel.Retirable());
 Lease inFlight;assert(inFlight.Begin(1,10,1)&&inFlight.BeforeExecute(1,100));
 assert(inFlight.CancelUnattachedConsumer()&&inFlight.Seal(10,1)&&!inFlight.Retirable());
 assert(inFlight.AfterExecute(100,9,true)&&!inFlight.Retirable());
 assert(inFlight.ObserveCompletion(100,9)&&inFlight.Retirable());
 Lease discarded;assert(discarded.Begin(1,10,1)&&discarded.AttachConsumer(1,20,1));
 assert(!discarded.CancelUnattachedConsumer());assert(discarded.Seal(10,1)&&discarded.Seal(20,1)&&discarded.Retirable());
 Lease identity;assert(identity.Begin(1,10,1));assert(!identity.AttachConsumer(2,20,1));
 assert(!identity.AttachConsumer(1,10,1));assert(!identity.Seal(10,2));assert(!identity.Retirable());
 assert(identity.AttachConsumer(1,10,2)); // Same pointer after real Reset is a different recording.
 assert(identity.Seal(10,1)&&!identity.Retirable());assert(identity.Seal(10,2)&&identity.Retirable());
 Lease removed;assert(removed.Begin(1,10,1)&&removed.BeforeExecute(1,100)&&removed.AfterExecute(100,1,true));
 assert(!removed.ObserveCompletion(100,UINT64_MAX)&&removed.Quarantined());
 // Scene admission release is distinct from allocation retirement.
 Lease scene;assert(scene.Begin(4,10,1)&&!scene.SceneReleaseReady());
 assert(scene.BeforeExecute(1,100)&&!scene.SceneReleaseReady());
 assert(scene.AfterExecute(100,1,true)&&!scene.SceneReleaseReady());
 assert(scene.AttachConsumer(4,20,1)&&scene.ClaimConsumerRecording(4,100,20,1));
 assert(!scene.SceneReleaseReady());
 assert(scene.BeforeExecute(2,100)&&!scene.SceneReleaseReady());
 assert(scene.AfterExecute(100,2,true)&&scene.SceneReleaseReady());
 assert(!scene.Retirable()); // Neither Reset nor GPU completion yet.
 assert(scene.Seal(10,1)&&scene.Seal(20,1)&&!scene.Retirable());
 assert(scene.ObserveCompletion(100,2)&&scene.Retirable());
 Lease noConsumer;assert(noConsumer.Begin(5,10,1)&&noConsumer.CancelUnattachedConsumer());
 assert(!noConsumer.SceneReleaseReady()); // Producer can still be submitted.
 assert(noConsumer.Seal(10,1)&&noConsumer.SceneReleaseReady());
 Lease discardedConsumer;assert(discardedConsumer.Begin(6,10,1));
 assert(discardedConsumer.BeforeExecute(1,100)&&discardedConsumer.AfterExecute(100,3,true));
 assert(discardedConsumer.AttachConsumer(6,20,1)&&!discardedConsumer.SceneReleaseReady());
 assert(discardedConsumer.Seal(20,1)&&discardedConsumer.SceneReleaseReady()&&!discardedConsumer.Retirable());
 assert(!failure.SceneReleaseReady()&&!cross.SceneReleaseReady()&&!removed.SceneReleaseReady());
 std::cout<<"PASS split recording lifetime: last consumer, reset vs completion, batch order, cancellation, replay, cross-queue, failed signal\n";
}
