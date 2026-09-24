#pragma once
#include "rr_indirect_call.h"
// Initialization only, before native rendering callbacks are armed.
struct RRIndirectCallPatch {
 unsigned char* site=nullptr;
 control_rr::IndirectCall6 original{},replacement{};
 void* relay=nullptr;
 bool changed=false,healthy=false;
 bool Prepare(unsigned char* address,void** expectedSlot,void* originalFunction,void* hook) noexcept {
  if(!address||!expectedSlot||!originalFunction||!hook||*expectedSlot!=originalFunction)return false;
  site=address;memcpy(original.bytes,address,6);std::uintptr_t decoded=0;
  if(!control_rr::DecodeIndirectCall(original,reinterpret_cast<std::uintptr_t>(address),decoded)||decoded!=reinterpret_cast<std::uintptr_t>(expectedSlot))return false;
  relay=AllocateExecutableRelayNear(address);if(!relay)return false;
  if(!control_rr::ReplaceIndirectCall(original,reinterpret_cast<std::uintptr_t>(address),decoded,reinterpret_cast<std::uintptr_t>(relay),replacement))return false;
  unsigned char stub[14]{0xff,0x25,0,0,0,0};memcpy(stub+6,&hook,8);memcpy(relay,stub,14);
  DWORD previous=0;
  return VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&previous)&&FlushInstructionCache(GetCurrentProcess(),relay,14);
 }
 bool Exchange(bool install) noexcept {
  if(!site||!relay)return false;
  const auto& expected=install?original:replacement;const auto& next=install?replacement:original;
  if(memcmp(site,expected.bytes,6))return false;
  DWORD previous=0;if(!VirtualProtect(site,6,PAGE_EXECUTE_READWRITE,&previous))return false;
  memcpy(site,next.bytes,6);changed=install;
  const bool flush=FlushInstructionCache(GetCurrentProcess(),site,6)!=FALSE;
  DWORD ignored=0;const bool protect=VirtualProtect(site,6,previous,&ignored)!=FALSE;
  healthy=flush&&protect;return true;
 }
};
