#pragma once
#include <cstdint>
namespace gi_reuse {
// Owned target coordinates live for this retained device/queue session. Native
// recording heap/fence epochs are unrelated and must not version these buffers.
// Host retains both objects, serializes access, and rejects identity changes.
class GIAutoGuideSession {
 std::uintptr_t device_=0,queue_=0;
public:
 std::uint32_t Admit(std::uintptr_t device,std::uintptr_t queue) noexcept {
  if(!device||!queue)return 0;
  if(!device_){device_=device;queue_=queue;}
  return device==device_&&queue==queue_?1u:0u;
 }
};
inline bool GIAutoPriorCompatible(bool hasBuffer,std::uint32_t priorVersion,
 std::uint32_t priorFrame,std::uint32_t version,std::uint32_t frame)noexcept {
 return hasBuffer&&version!=0&&priorVersion==version&&priorFrame!=0&&priorFrame<frame;
}
}
