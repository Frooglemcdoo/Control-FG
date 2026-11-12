#include "../src/gi_reuse/recorded_heap_identity.h"
#include <cassert>
#include <cstdio>
#ifdef _WIN32
#include "../src/gi_reuse/d3d12_native_recorded_heaps.h"
#include "../src/gi_reuse/d3d12_native_gi_recording.h"
#include <vector>
#endif
int main(){
 using I=gi_reuse::GIRecordedHeapIdentity;
 const std::array<I::U64,2> bound{100,200};
 const I::Sample sample{1405,10,11,1406,1400,{{{20,100,1406,48020,1000},{30,200,1406,7,50}}}};
 I identity;assert(identity.Capture(sample,bound));assert(!identity.Capture(sample,bound));
 assert(identity.ValidateSaved(sample,bound));
 auto changed=sample;changed.heaps[0].cursor+=49;changed.completed=1404;
 assert(identity.ValidateSaved(changed,bound)); // Native append is not recycling.
 // Reproduce R427 v2: the submit thread's current context changes to a heap
 // carrying older material generation. Saved records still validate.
 auto unrelated=sample;unrelated.heaps[0]={900,901,1406,42,600};
 assert(!identity.ValidateSaved(unrelated,bound));assert(identity.ValidateSaved(sample,bound));
 for(unsigned test=0;test<11;++test){changed=sample;
  switch(test){
  case 0:++changed.frame;break;
  case 1:++changed.deviceEpoch;break;
  case 2:++changed.deviceFenceRecord;break;
  case 3:++changed.deviceFenceObject;break;
  case 4:changed.completed=changed.deviceEpoch;break;
  case 5:changed.completed=UINT64_MAX;break;
  case 6:++changed.heaps[0].generation;break;
  case 7:++changed.heaps[1].epoch;break;
  case 8:--changed.heaps[0].cursor;break;
  case 9:++changed.heaps[0].record;break;
  case 10:--changed.completed;break;
  }
  assert(!identity.ValidateSaved(changed,bound));
 }
 I empty;assert(!empty.ValidateSaved(sample,bound));assert(!empty.Capture(sample,{100,201}));
 assert(empty.Capture(sample,bound)); // Rejected capture is transactional.
#ifdef _WIN32
 // Exact native field widths; adjacent padding must not contaminate cursor.
 alignas(8) unsigned char records[2][0x40]{},fence[16]{};
 auto saved=sample;saved.deviceFenceRecord=reinterpret_cast<I::U64>(fence);
 std::memcpy(fence,&sample.deviceEpoch,8);std::memcpy(fence+8,&sample.deviceFenceObject,8);
 for(unsigned i=0;i<2;++i){saved.heaps[i].record=reinterpret_cast<I::U64>(records[i]);
  const auto& h=sample.heaps[i];std::memset(records[i],0xff,sizeof(records[i]));
  std::memcpy(records[i],&h.object,8);std::memcpy(records[i]+0x18,&h.cursor,4);
  std::memcpy(records[i]+0x20,&h.epoch,8);std::memcpy(records[i]+0x30,&h.generation,8);
 }
 I pinned;assert(pinned.Capture(saved,bound));I::Sample read{};
 assert(gi_reuse::ReadNativeGIRecordedHeaps(saved,saved.frame,saved.completed,&read));
 assert(pinned.ValidateSaved(read,bound));
 // Original-thread native context, with both direct TLS and audited fallback.
 std::vector<unsigned char> module(0x136d30);
 alignas(8) unsigned char tls[0x28]{},state[0x60]{},context[0x28]{},queue[0x18]{},device[0x228]{};
 auto pointer=[](unsigned char* dst,const void* value){std::memcpy(dst,&value,8);};
 auto value=[](unsigned char* dst,I::U64 v){std::memcpy(dst,&v,8);};
 value(module.data()+0x136c68,saved.frame);pointer(module.data()+0x136d28,device);
 pointer(tls+0x20,state);pointer(tls+8,context);pointer(tls+0x18,queue);
 pointer(state+0x50,records[0]);pointer(state+0x58,records[1]);
 auto* list=reinterpret_cast<ID3D12CommandList*>(0x1000);
 pointer(context+0x18,list);pointer(context+0x20,list);value(queue+0x10,0x2000);
 value(device+0x78,0x3000);pointer(device+0x88,fence);value(device+0x220,0x4000);
 gi_reuse::NativeGIRecording recording;
 assert(gi_reuse::ReadNativeGIRecording(module.data(),tls,list,saved.frame,&recording));
 assert(recording.heaps.heaps[0].object==bound[0]&&recording.heaps.heaps[1].cursor==sample.heaps[1].cursor);
 assert(recording.heaps.deviceFenceObject==sample.deviceFenceObject&&recording.list==list);
 assert(!gi_reuse::ReadNativeGIRecording(module.data(),tls,reinterpret_cast<ID3D12CommandList*>(0x1008),saved.frame,&recording));
 assert(!recording.list&&!recording.heaps.frame);
 assert(!gi_reuse::ReadNativeGIRecording(module.data(),tls,list,saved.frame+1,&recording));
 pointer(module.data()+0x111c10,state);pointer(module.data()+0x111c18,context);pointer(module.data()+0x111c38,queue);
 std::memset(tls,0,sizeof(tls));assert(gi_reuse::ReadNativeGIRecording(module.data(),tls,list,saved.frame,&recording));
 pointer(module.data()+0x111c38,nullptr);assert(!gi_reuse::ReadNativeGIRecording(module.data(),tls,list,saved.frame,&recording));
 assert(!gi_reuse::ReadNativeGIRecording(module.data(),reinterpret_cast<unsigned char*>(1),list,saved.frame,&recording));
 saved.heaps[0].record=1;assert(!gi_reuse::ReadNativeGIRecordedHeaps(saved,saved.frame,0,&read));
 assert(!read.frame&&!read.heaps[0].object); // No partial sample escapes a fault.
#endif
 std::puts("PASS recorded GI heaps: saved records, unrelated TLS switch, append, recycling, epoch/fence changes and device loss.");
}
