#pragma once
#include "rt_prepared_cache.h"
#include <mutex>
namespace RTPreparedCache {
inline std::mutex mutex;
inline rt_prepared_cache::Entry entries[4096]{};
inline std::atomic<UINT64> hits{0},misses{0},stored{0},busyLoops{0},activeLoops{0};
struct Session;
inline thread_local Session* current=nullptr;
struct Session {
 std::unique_lock<std::mutex> lock;Session* previous;void* context;UINT64 hits=0,misses=0,stored=0;
 explicit Session(void* ctx):lock(mutex,std::defer_lock),previous(current),context(ctx){
  // A recursive job never tries to lock a mutex already owned by this thread.
  if(!previous&&RTCacheRevision::installed)lock.try_lock();current=this;
  if(lock.owns_lock())activeLoops.fetch_add(1,std::memory_order_relaxed);else busyLoops.fetch_add(1,std::memory_order_relaxed);
 }
 ~Session(){RTPreparedCache::hits.fetch_add(hits,std::memory_order_relaxed);RTPreparedCache::misses.fetch_add(misses,std::memory_order_relaxed);RTPreparedCache::stored.fetch_add(stored,std::memory_order_relaxed);current=previous;}
 Session(const Session&)=delete;Session& operator=(const Session&)=delete;
};
inline rt_prepared_cache::Entry* Find(void* context,void* provider,void* owner,std::int32_t variant)noexcept{
 if(!current||!current->lock.owns_lock()||current->context!=context)return nullptr;
 return &entries[rt_prepared_cache::Slot(provider,owner,variant)];
}
inline void Report(){Log("RT_REUSE_R402 active=%u hits=%llu misses=%llu stored=%llu active_loops=%llu busy_loops=%llu provider_creates=%llu provider_destroys=%llu material_notifications=%llu exact_input_validation=1 hash_authorized_reuse=0 transforms_live=1 blas_addresses_live=1",unsigned(RTCacheRevision::installed),hits.load(),misses.load(),stored.load(),activeLoops.load(),busyLoops.load(),RTCacheRevision::revisions.creates.load(),RTCacheRevision::revisions.destroys.load(),RTCacheRevision::revisions.notifications.load());}
}
