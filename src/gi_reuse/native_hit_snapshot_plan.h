#pragma once
#include "pt_native_binding.h"
#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <vector>
namespace gi_reuse {
// Plans one packed PRIVATE buffer for the bytes referenced by native hit roots.
// Does not grant source state/ordering/alias/lifetime proof: the host must copy
// at the original native ray boundary and retain the actual source allocations.
// Preserves native bounds, strides, material constants and geometry row order.
class NativeHitSnapshotPlan {
 using U64=std::uint64_t;
public:
 struct Allocation {U64 token=0,address=0,bytes=0;};
 struct Copy {Allocation source;U64 offset=0,bytes=0,destinationOffset=0;};
private:
 struct Reference {std::size_t allocation=0;U64 offset=0;};
 std::vector<Copy> copies_;
 std::vector<PTNativeBinding::Binding> bindings_;
 std::vector<std::array<Reference,2>> references_;
 U64 bytes_=0;
 static constexpr U64 Max=(std::numeric_limits<U64>::max)();
 static bool Range(U64 address,U64 bytes)noexcept{return address&&bytes&&bytes<=Max-address;}
public:
 bool Build(const std::vector<PTNativeBinding::Binding>& bindings,
            std::vector<Allocation> allocations,U64 budget)noexcept {
  *this={};
  if(bindings.empty()||bindings.size()>65536||allocations.empty()||allocations.size()>8192||!budget)return false;
  try {
   std::sort(allocations.begin(),allocations.end(),[](const auto& a,const auto& b){return a.address<b.address;});
   std::set<U64> tokens;
   for(std::size_t i=0;i<allocations.size();++i){const auto& a=allocations[i];
    if(!a.token||!Range(a.address,a.bytes)||a.address%256||!tokens.insert(a.token).second||
       (i&&allocations[i-1].address+allocations[i-1].bytes>a.address))return false;
   }
   NativeHitSnapshotPlan next;next.bindings_=bindings;next.references_.resize(bindings.size());
   std::vector<U64> begin(allocations.size(),Max),end(allocations.size(),0);
   for(std::size_t i=0;i<bindings.size();++i){const auto& b=bindings[i];
    if(b.type>1||(b.constants[1]!=2&&b.constants[1]!=4)||
       b.constants[0]%b.constants[1]||b.constants[0]>=b.constants[8])return false;
    const U64 addresses[]={b.indices,b.attributes};
    const U64 bounds[]={b.constants[8],b.constants[9]};
    if(b.indices%b.constants[1])return false;
    for(unsigned kind=0;kind<2;++kind){
     const U64 address=addresses[kind],length=bounds[kind];if(!Range(address,length))return false;
     auto found=std::upper_bound(allocations.begin(),allocations.end(),address,
       [](U64 value,const Allocation& a){return value<a.address;});
     if(found==allocations.begin())return false;
     --found;
     const auto n=static_cast<std::size_t>(found-allocations.begin());const auto offset=address-found->address;
     if(offset>found->bytes||length>found->bytes-offset)return false;
     next.references_[i][kind]={n,offset};
     begin[n]=(std::min)(begin[n],offset&~U64(255));end[n]=(std::max)(end[n],offset+length);
    }
   }
   std::vector<std::size_t> remap(allocations.size(),allocations.size());
   for(std::size_t n=0;n<allocations.size();++n)if(begin[n]!=Max){
    const U64 size=end[n]-begin[n];if(size>Max-255)return false;
    const U64 padded=(size+255)&~U64(255);
    if(next.bytes_>budget||padded>budget-next.bytes_)return false;
    remap[n]=next.copies_.size();next.copies_.push_back({allocations[n],begin[n],size,next.bytes_});next.bytes_+=padded;
   }
   for(auto& pair:next.references_)for(auto& r:pair){
    const auto original=r.allocation;r.offset-=begin[original];r.allocation=remap[original];
   }
   *this=std::move(next);return true;
  }catch(...){return false;}
 }
 // destination is an independently owned buffer. Reject address aliases even
 // if a caller supplies a different token; no GPU commands happen in this API.
 bool Rebase(U64 destination,U64 capacity,std::vector<PTNativeBinding::Binding>& out)const noexcept {
  out.clear();if(!bytes_||!Range(destination,capacity)||destination%256||capacity<bytes_)return false;
  for(const auto& c:copies_)if(destination<c.source.address+c.source.bytes&&c.source.address<destination+capacity)return false;
  try {auto next=bindings_;
   for(std::size_t i=0;i<next.size();++i){U64 values[2]{};
    for(unsigned kind=0;kind<2;++kind){const auto& r=references_[i][kind];
     values[kind]=destination+copies_[r.allocation].destinationOffset+r.offset;
    }
    next[i].indices=values[0];next[i].attributes=values[1];
   }
   out=std::move(next);return true;
  }catch(...){return false;}
 }
 U64 Bytes()const noexcept{return bytes_;} // Logical packed width; host budgets committed allocation too.
 const std::vector<Copy>& Copies()const noexcept{return copies_;}
};
}
