#pragma once
#include <atomic>
#include <cstdint>
namespace storage_latency {
struct Timing {
 std::atomic<std::uint64_t> count{0},total{0},maximum{0},over16ms{0},over100ms{0};
 void Record(std::uint64_t us) noexcept {
  total.fetch_add(us,std::memory_order_relaxed);
  auto old=maximum.load(std::memory_order_relaxed);
  while(old<us&&!maximum.compare_exchange_weak(old,us,std::memory_order_relaxed)){}
  if(us>=16000)over16ms.fetch_add(1,std::memory_order_relaxed);
  if(us>=100000)over100ms.fetch_add(1,std::memory_order_relaxed);
  count.fetch_add(1,std::memory_order_relaxed);
 }
};
}
