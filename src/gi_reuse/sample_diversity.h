#pragma once
#include <array>
#include <cstdint>
namespace gi_reuse {
struct SampleKey {std::uint64_t vertexOwner=0,indexOwner=0,vertexOffset=0,indexOffset=0,stride=0;std::uint32_t vertices=0,indices=0,width=0,geometry=0;
 bool operator==(const SampleKey& b)const noexcept{return vertexOwner==b.vertexOwner&&indexOwner==b.indexOwner&&vertexOffset==b.vertexOffset&&indexOffset==b.indexOffset&&stride==b.stride&&vertices==b.vertices&&indices==b.indices&&width==b.width&&geometry==b.geometry;}};
template<unsigned Capacity=48> class SampleDiversity {
 struct Entry{SampleKey key;unsigned count=0;};std::array<Entry,Capacity> entries_{};unsigned used_=0;
public:
 // Caller serializes access. No resource ownership or rendering decisions here.
 bool Reserve(const SampleKey& key) noexcept {
  if(!key.vertexOwner||!key.indexOwner)return false;
  for(unsigned i=0;i<used_;++i)if(entries_[i].key==key){if(entries_[i].count>=2)return false;++entries_[i].count;return true;}
  if(used_==Capacity){return false;}entries_[used_++]={key,1};return true;
 }
};
}
