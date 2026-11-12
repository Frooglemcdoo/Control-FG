#pragma once
#include "native_gi_job_route.h"
#include "native_gi_table.h"
#ifdef _WIN32
namespace gi_reuse {
// CPU writer inputs, copied either at matched pre-schedule writerReady or at
// the exact original dispatch under the acquired native scene window. Neither
// route proves GPU completion, source contents, or descriptor ownership.
struct NativeGIWriterInputs {std::vector<PTNativeBinding::Binding> bindings;NativeGITable table;};
namespace native_gi_detail {
struct Header {std::uint64_t buffer=0,bytes=0,pipeline=0;PTShaderTableAdapter::Identifier opaque{},alpha{};};
inline bool ReadHeader(const GINativeJobRoute::Writer& writer,Header& out) noexcept {
 out={};Header next;
 __try {
  if(!writer.buffer||!writer.state)return false;
  const auto* buffer=reinterpret_cast<const unsigned char*>(writer.buffer);
  const auto* state=reinterpret_cast<const unsigned char*>(writer.state);
  std::memcpy(&next.buffer,buffer+0x48,8);std::memcpy(&next.bytes,buffer+0x30,8);
  std::memcpy(&next.pipeline,state,8);
  const void* opaque=nullptr;const void* alpha=nullptr;
  std::memcpy(&opaque,state+0x20,8);std::memcpy(&alpha,state+0x30,8);
  if(!opaque||!alpha)return false;
  std::memcpy(next.opaque.data(),opaque,32);std::memcpy(next.alpha.data(),alpha,32);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
 out=next;return true;
}
}
inline bool SnapshotNativeGIWriter(const GINativeJobRoute::Ticket& ticket,NativeGIWriterInputs& output,const char** failure=nullptr) noexcept {
 if(failure)*failure="ticket";
 output={};const auto& writer=ticket.writer;
 if(!ticket.serial||!ticket.list||!ticket.pipeline||!ticket.counter||!ticket.engineFrame||writer.rays!=1||
    writer.count<=0||writer.count>65536||writer.bytes<=0||!writer.bindings)return false;
 if(failure)*failure="layout";
 std::uint64_t hit=0,size=0;
 if(!GINativeJobRoute::HitRange(ticket.layout,static_cast<std::size_t>(writer.count),hit,size)||
    hit!=ticket.hitTable||size!=ticket.hitBytes||static_cast<std::uint64_t>(writer.bytes)>ticket.layout.bytes)return false;
 const auto offset=hit-ticket.layout.buffer;
 const auto written=static_cast<std::uint64_t>(writer.bytes);
 if(offset>written||size>written-offset)return false;
 if(failure)*failure="native_header";
 native_gi_detail::Header header;
 if(!native_gi_detail::ReadHeader(writer,header)||header.pipeline!=ticket.pipeline||
    header.buffer!=ticket.layout.buffer||header.bytes!=ticket.layout.bytes)return false;
 try {
  if(failure)*failure="binding_copy";
  NativeGIWriterInputs next;next.bindings.resize(static_cast<std::size_t>(writer.count));
  for(unsigned i=0;i<next.bindings.size();++i)
   if(!PTNativeBinding::Read(reinterpret_cast<const void*>(writer.bindings),i,next.bindings[i]))return false;
  if(failure)*failure="identifiers_or_binding_types";
  if(!BuildNativeGITable(ticket.pipeline,ticket.serial,1,next.bindings,header.opaque,header.alpha,next.table))return false;
  output=std::move(next);if(failure)*failure="none";return true;
 }catch(...){if(failure)*failure="allocation";return false;}
}
}
#endif
