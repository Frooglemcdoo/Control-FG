#pragma once
#include <cstdint>
namespace storage_asset_trace {
// Fixed metadata capture. Caller serializes access; no engine pointers or payloads.
struct Capture {
 using U64=std::uint64_t;
 static constexpr unsigned Capacity=512;
 struct Row {U64 token=0,requested=0,admitted=0,submitted=0,retired=0,published=0,bytes=0;unsigned level=0;int priority=0,result=0;bool truncated=false;char name[512]{};};
 Row rows[Capacity]{};unsigned count=0,overflow=0;U64 epoch=0,start=0,end=0,flushAt=0;bool active=false;
 bool Begin(U64 now){
  if(active)return false;
  ++epoch;count=overflow=0;start=now;end=now+15000000;flushAt=end+5000000;active=true;return true;
 }
 U64 Add(const char* name,U64 requested,U64 admitted,unsigned level,int priority,U64 bytes){
  if(!active||requested<start||requested>=end)return 0;
  if(count==Capacity){++overflow;return 0;}
  auto& r=rows[count];r={};r.token=epoch*Capacity+count+1;r.requested=requested;r.admitted=admitted;r.level=level;r.priority=priority;r.bytes=bytes;
  if(name){unsigned i=0;for(;i<511&&name[i];++i){const auto c=static_cast<unsigned char>(name[i]);r.name[i]=c<32||c==127?'_':static_cast<char>(c);}r.truncated=name[i]!=0;}
  ++count;return r.token;
 }
 Row* Find(U64 token){
  if(!active||!token)return nullptr;const auto index=(token-1)%Capacity;
  return index<count&&rows[index].token==token?&rows[index]:nullptr;
 }
 void Submit(U64 token,U64 now){if(auto* r=Find(token))r->submitted=now;}
 void Retire(U64 token,U64 now,int result){if(auto* r=Find(token)){r->retired=now;r->result=result;}}
 void Publish(U64 token,U64 now){if(auto* r=Find(token))r->published=now;}
 bool Finish(U64 now){if(!active||now<flushAt)return false;active=false;return true;}
};
}
