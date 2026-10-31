#pragma once
#include <cstdint>
namespace storage_publication {
// Identity metadata only; no retained or dereferenced engine pointers.
// Caller serializes access. Bounded probes keep observer work independent of backlog.
struct Tracker {
 using U64=std::uint64_t;
 struct Entry {U64 texture=0,container=0,start=0;unsigned level=0;};
 Entry entries[4096]{};
 static unsigned Index(U64 texture,unsigned level){return unsigned(((texture>>4)^(U64(level)*0x9e3779b9u))&4095);}
 bool Begin(U64 texture,unsigned level,U64 container,U64 now){
  if(!texture||!container||!now)return false;
  const unsigned first=Index(texture,level);Entry* free=nullptr;
  for(unsigned p=0;p<8;++p){auto& e=entries[(first+p)&4095];
   if(e.texture==texture&&e.level==level){e={texture,container,now,level};return true;}
   if(!free&&(!e.texture||(now>=e.start&&now-e.start>30000000)))free=&e;
  }
  if(!free)return false;*free={texture,container,now,level};return true;
 }
 U64 Take(U64 texture,unsigned level,U64 container,U64 now){
  const unsigned first=Index(texture,level);
  for(unsigned p=0;p<8;++p){auto& e=entries[(first+p)&4095];
   if(e.texture!=texture||e.level!=level)continue;
   const auto old=e;e={};
   return old.container==container&&now>=old.start&&now-old.start<=30000000?old.start:0;
  }return 0;
 }
};
}
