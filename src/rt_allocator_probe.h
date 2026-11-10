#pragma once
#include <cstdint>
namespace rt_allocator_probe {
using U64=std::uint64_t;using U32=std::uint32_t;
struct Snapshot {void* heap=nullptr;U64 version=0,heapVersion=0;U32 cursor=0,pool=0;};
struct Sample {void* lock=nullptr;Snapshot before{},after{};U64 begin=0,acquired=0,end=0,wait=0,hold=0,release=0,requested=49;bool entered=false,left=false;};
struct Api {void(*enter)(void*);void(*leave)(void*);U64(*clock)();U32(*error)();void(*restore)(U32);Snapshot(*capture)(void*);};
inline void Enter(const Api& api,Sample* sample,void* lock){
 if(!sample||sample->lock){api.enter(lock);return;}
 sample->lock=lock;const auto before=api.error();sample->begin=api.clock();api.restore(before);api.enter(lock);
 const auto after=api.error();sample->acquired=api.clock();sample->wait=sample->acquired-sample->begin;sample->before=api.capture(lock);sample->entered=true;api.restore(after);
}
inline void Leave(const Api& api,Sample* sample,void* lock){
 if(!sample||sample->lock!=lock||!sample->entered||sample->left){api.leave(lock);return;}
 const auto before=api.error();sample->after=api.capture(lock);const auto start=api.clock();api.restore(before);api.leave(lock);
 const auto after=api.error();sample->end=api.clock();sample->hold=start-sample->acquired;sample->release=sample->end-start;sample->left=true;api.restore(after);
}
// 0 normal cursor advance; 1 observed refill/heap change; 2 other.
inline unsigned Classify(const Sample& s){
 const auto& a=s.before;const auto& b=s.after;
 if(!a.heap||a.heap!=b.heap||a.heapVersion!=a.version||b.cursor<a.cursor)return 1;
 if(a.version==b.version&&a.heapVersion==b.heapVersion&&b.heapVersion==b.version&&s.requested<=UINT32_MAX-a.cursor&&U64(b.cursor)==U64(a.cursor)+s.requested)return 0;
 return 2;
}
}
