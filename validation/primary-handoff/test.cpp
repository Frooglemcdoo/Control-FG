#include "fixture.h"
int main(){
 Api a;Backend<Api>b(a);Capture<Backend<Api>>c(b);PresentLedger presents;
 assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape)&&c.Bind(b.Key()));
 control_rr::Lease stale{};
 for(unsigned frame=1;frame<=3000;++frame){
  a.current.frame=frame;a.current.list=100+frame%5;auto producer=snapshot(a);control_rr::Lease lease{};
  assert(b.SetRecording(producer.context)&&c.Record(producer,1,b.Destinations(),lease));
  assert(presents.Record(lease,frame+17)&&presents.Matches(lease,frame+17));
  assert(!presents.Matches(lease,frame+18)&&!presents.Matches(stale,frame+17));
  auto consumer=producer.context;consumer.list+=10;
  assert(c.PrimaryHandoffReason(lease,consumer,1)&&!c.TakeOnPrimaryQueue(lease,consumer,1));
  assert(!c.ObservePrimaryQueued(stale,producer.context));
  auto wrong=producer.context;wrong.list+=1;assert(!c.ObservePrimaryQueued(lease,wrong));
  assert(c.ObservePrimaryQueued(lease,producer.context));assert(!c.ObservePrimaryQueued(lease,producer.context));
  assert(c.PrimaryHandoffReason(lease,producer.context,1)); // recycled pointer rejected
  for(unsigned k=0;k<5;++k){wrong=consumer;
   switch(k){case 0:++wrong.frame;break;case 1:++wrong.queue;break;case 2:++wrong.device;break;case 3:++wrong.view;break;case 4:wrong.list=0;break;}
   assert(c.PrimaryHandoffReason(lease,wrong,1)&&!c.TakeOnPrimaryQueue(lease,wrong,1));
  }
  assert(c.PrimaryHandoffReason(lease,consumer,2));
  assert(!c.Take(lease,consumer,1)); // old strictly same-list API stays strict
  assert(!c.PrimaryHandoffReason(lease,consumer,1)&&c.TakeOnPrimaryQueue(lease,consumer,1));
  assert(!c.TakeOnPrimaryQueue(lease,consumer,1));
  assert(c.SubmitCaptured(lease)&&c.Collect()&&!c.Idle());
  a.completed=a.signal;assert(c.Collect()&&c.Idle());stale=lease;
 }
 assert(b.Dispose(c.Idle()));
 puts("PASS observed enqueue required, 3000 handoffs/retirements, stale/recycled identities, frame/view/device/queue/epoch/Present guards, exactly-once consumption");
}
