#include "../../src/rr_input_capture_policy.h"
#include <cassert>
#include <cstdio>
int main(){using P=control_rr::InputCapturePolicy;P p;
 assert(!p.CanSubmit(10));assert(!p.Retire(1));
 for(unsigned i=0;i<4;++i){assert(p.Request());assert(!p.Request());assert(p.BeginPrepare());assert(!p.Record(9));p.Prepared(true);assert(p.Record(10));assert(!p.Record(11));assert(!p.CanSubmit(10));assert(p.CanSubmit(11));p.Submit(true);assert(!p.Retire(0));assert(p.Retire(1));assert(!p.Request());p.Exported();}
 assert(!p.Request());P lost;assert(lost.Request());assert(lost.BeginPrepare());lost.Prepared(true);assert(lost.Record(7));lost.Submit(true);assert(!lost.Retire(UINT64_MAX));assert(lost.phase==P::Failed);assert(!lost.Request());
 P failure;assert(failure.Request());assert(failure.BeginPrepare());failure.Prepared(false);assert(!failure.Record(3));assert(!failure.Request());
 P exporting;exporting.phase=P::Exporting;
 assert(!exporting.Request()&&!exporting.Record(20)&&!exporting.BeginPrepare());
 exporting.phase=P::Failed; // Render-side timeout while the disk worker owns the slot.
 exporting.Exported();assert(exporting.phase==P::Failed&&!exporting.Request());
 puts("PASS C1 capture lifecycle: capacity, no early submit/map/reuse, device loss, allocation failure and timeout during export");}
