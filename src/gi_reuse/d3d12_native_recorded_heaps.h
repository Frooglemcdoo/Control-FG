#pragma once
#include "recorded_heap_identity.h"
#include <windows.h>
#include <cstring>
namespace gi_reuse {
// Build-locked cceb99cb... native layout. No TLS/global current-heap lookup.
// Reads only previously captured records under the host's native scope. SEH
// handles unreadable pointers; it does NOT make concurrent mutation safe.
inline bool ReadNativeGIRecordedHeaps(const GIRecordedHeapIdentity::Sample& saved,
                                     std::uint64_t frame,std::uint64_t completed,
                                     GIRecordedHeapIdentity::Sample* output)noexcept {
 if(!output)return false;*output={};
 if(!saved.deviceFenceRecord||!saved.heaps[0].record||!saved.heaps[1].record)return false;
 GIRecordedHeapIdentity::Sample next{};next.frame=frame;next.completed=completed;
 next.deviceFenceRecord=saved.deviceFenceRecord;
 __try {
  const auto* fence=reinterpret_cast<const unsigned char*>(saved.deviceFenceRecord);
  std::memcpy(&next.deviceEpoch,fence,8);std::memcpy(&next.deviceFenceObject,fence+8,8);
  for(unsigned i=0;i<2;++i){auto& h=next.heaps[i];h.record=saved.heaps[i].record;
   const auto* p=reinterpret_cast<const unsigned char*>(h.record);
   std::memcpy(&h.object,p,8);std::memcpy(&h.cursor,p+0x18,4);
   std::memcpy(&h.epoch,p+0x20,8);std::memcpy(&h.generation,p+0x30,8);
  }
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
 *output=next;return true;
}
}
