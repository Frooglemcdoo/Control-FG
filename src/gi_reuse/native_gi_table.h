#pragma once
#include "pt_native_binding.h"
#include "pt_shader_table_adapter.h"
namespace gi_reuse {
// A CPU reconstruction of ACTIVE GI records, not a copy of unknown padding.
// Native writer 33a40 emits rays active rows then (2-rays) reserved rows per
// binding. GI TraceRay uses multiplier 2. Never compact the even row indices.
struct NativeGITable {
 std::uint64_t pipeline=0,version=0;
 std::vector<unsigned char> bytes;
 std::vector<unsigned char> active;
 bool Active(std::size_t row) const noexcept {return row<active.size()&&active[row]!=0;}
};
inline bool BuildNativeGITable(std::uint64_t pipeline,std::uint64_t version,int rays,
 const std::vector<PTNativeBinding::Binding>& bindings,
 const PTShaderTableAdapter::Identifier& opaque,const PTShaderTableAdapter::Identifier& alpha,
 NativeGITable& output) {
 output={};using namespace PTShaderTableAdapter;
 if(!pipeline||!version||rays!=1||bindings.empty()||bindings.size()>65536||
    opaque==Identifier{}||alpha==Identifier{}||opaque==alpha)return false;
 NativeGITable next;next.pipeline=pipeline;next.version=version;
 next.bytes.resize(bindings.size()*2*NativeStride);next.active.resize(bindings.size()*2);
 for(std::size_t n=0;n<bindings.size();++n){
  const auto& binding=bindings[n];if(binding.type>1)return false;
  const auto& identifier=binding.type?alpha:opaque;
  auto* row=next.bytes.data()+n*2*NativeStride;
  std::memcpy(row,identifier.data(),32);
  std::memcpy(row+32,reinterpret_cast<const unsigned char*>(&binding)+8,56);
  next.active[n*2]=1;
 }
 output=std::move(next);return true;
}
// Compare against the already captured writer table without constructing and
// zero-filling a second full table at the submission boundary. Equivalent to
// BuildNativeGITable(...).bytes/active equality, including all reserved bytes.
inline bool MatchesNativeGITable(const NativeGITable& table,
 const std::vector<PTNativeBinding::Binding>& bindings,
 const PTShaderTableAdapter::Identifier& opaque,const PTShaderTableAdapter::Identifier& alpha){
 using namespace PTShaderTableAdapter;
 if(!table.pipeline||!table.version||bindings.empty()||bindings.size()>65536||
    opaque==Identifier{}||alpha==Identifier{}||opaque==alpha||
    table.bytes.size()!=bindings.size()*2*NativeStride||table.active.size()!=bindings.size()*2)return false;
 const std::array<unsigned char,NativeStride> zero{};
 for(std::size_t n=0;n<bindings.size();++n){
  const auto& binding=bindings[n];if(binding.type>1)return false;
  const auto& identifier=binding.type?alpha:opaque;const auto* row=table.bytes.data()+n*2*NativeStride;
  if(table.active[n*2]!=1||table.active[n*2+1]!=0||
     std::memcmp(row,identifier.data(),32)||
     std::memcmp(row+32,reinterpret_cast<const unsigned char*>(&binding)+8,56)||
     std::memcmp(row+88,zero.data(),8)||std::memcmp(row+NativeStride,zero.data(),NativeStride))return false;
 }
 return true;
}

}
