// CPU-only repeated validation workload, not GPU/frame-time evidence.
#ifndef ORDER_HEADER
#define ORDER_HEADER "../src/gi_reuse/ordered_as_metadata.h"
#define RANGE_HEADER "../src/gi_reuse/producer_backing_ranges.h"
#endif
#include ORDER_HEADER
#include RANGE_HEADER
#include <cassert>
#include <chrono>
#include <cstdio>
using namespace gi_reuse;
int main(){
 using Clock=std::chrono::steady_clock;
 OrderedASMetadata<> history;
 auto recording=std::make_shared<OrderedASRecording>();recording->generation=1;
 constexpr unsigned count=2048;
 for(unsigned i=0;i<count;++i){
  OrderedASEvent event;event.destinationToken=i+1;
  event.event.operation=ASOperation::Build;event.event.record.address=(i+1)*256ull;
  event.event.record.instances=1048576;event.event.record.count=1;event.event.record.topLevel=true;
  recording->events.push_back(event);
 }
 assert(history.Submit(1,{recording}));
 // New cut each iteration prevents reporting an already warm previous frame.
 double orderMS=0,rangeMS=0;std::uint64_t checksum=0;
 for(unsigned trial=0;trial<55;++trial){
  const auto cut=history.Mark(1);const auto start=Clock::now();
  for(unsigned pass=0;pass<4;++pass)for(unsigned i=0;i<count;++i){
   ASRecord record;assert(history.Select(cut,i+1,(i+1)*256ull,record)==PTSceneOrder::Status::ready);
   assert(record.revision==i+1);checksum+=record.revision;
  }
  const auto elapsed=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
  if(trial>=5)orderMS+=elapsed;
  ProducerBackingRanges ranges;
  for(unsigned i=0;i<count;++i)assert(ranges.Add(4096ull*(i+1),2048));
  const auto begin=Clock::now();ranges.Seal();
  for(unsigned i=0;i<11344;++i)for(unsigned view=0;view<3;++view){
   const auto address=4096ull*((i+view)%count+1);
   assert(ranges.Unique(address,2048));++checksum;
  }
  if(trial>=5)rangeMS+=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();
 }
 std::printf("selection_ms=%.3f backing_ranges_ms=%.3f checksum=%llu\n",orderMS/50,rangeMS/50,static_cast<unsigned long long>(checksum));
}
