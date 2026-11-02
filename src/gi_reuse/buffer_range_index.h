#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>
namespace gi_reuse {
enum class RangeMatch { Missing, Unique, Ambiguous, Invalid };
// CPU address metadata only. Prefix maxima keep overlapping/aliased ranges
// conservative while pruning ordinary disjoint-buffer lookups.
template<std::size_t Capacity> class BufferRangeIndex {
 struct Range {std::uint64_t begin=0,end=0,maximumEnd=0;std::size_t slot=0;};
 std::array<Range,Capacity> ranges_{};std::size_t size_=0;
 void Rebuild(std::size_t from) noexcept {
  auto maximum=from?ranges_[from-1].maximumEnd:0;
  for(auto i=from;i<size_;++i){maximum=(std::max)(maximum,ranges_[i].end);ranges_[i].maximumEnd=maximum;}
 }
public:
 bool Insert(std::uint64_t address,std::uint64_t bytes,std::size_t slot) noexcept {
  if(size_==Capacity||!address||!bytes||bytes>UINT64_MAX-address)return false;
  std::size_t pos=0;while(pos<size_&&ranges_[pos].begin<address)++pos;
  for(auto i=size_;i>pos;--i)ranges_[i]=ranges_[i-1];
  ranges_[pos]={address,address+bytes,0,slot};++size_;Rebuild(pos);return true;
 }
 bool Remove(std::size_t slot) noexcept {
  std::size_t pos=0;while(pos<size_&&ranges_[pos].slot!=slot)++pos;if(pos==size_)return false;
  for(auto i=pos+1;i<size_;++i){ranges_[i-1]=ranges_[i];}
  --size_;Rebuild(pos);return true;
 }
 RangeMatch Find(std::uint64_t address,std::uint64_t bytes,std::size_t& slot,std::size_t* examined=nullptr) const noexcept {
  slot=0;if(examined)*examined=0;if(!address||!bytes||bytes>UINT64_MAX-address)return RangeMatch::Invalid;
  const auto end=address+bytes;std::size_t low=0,high=size_;
  while(low<high){const auto mid=low+(high-low)/2;if(ranges_[mid].begin<=address)low=mid+1;else high=mid;}
  unsigned matches=0;
  while(low){const auto& r=ranges_[--low];if(r.maximumEnd<end)break;if(examined)++*examined;
   if(r.end>=end){if(++matches>1){slot=0;return RangeMatch::Ambiguous;}slot=r.slot;}
  }
  return matches?RangeMatch::Unique:RangeMatch::Missing;
 }
};
}
