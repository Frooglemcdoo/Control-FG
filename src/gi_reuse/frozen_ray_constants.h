#pragma once
#include "compute_root_arguments.h"
#include <limits>

namespace gi_reuse {
struct FrozenRayConstants {
 static constexpr unsigned Bytes=1536,GIConstantOffset=1280;
 // Both replay CBVs must address the same immutable copy used by shading.
 // Address identity is necessary, but does not establish lifetime/ownership.
 static bool Matches(const ComputeRootArguments& arguments,std::uint64_t base) noexcept {
  if(!base||base%256||base>(std::numeric_limits<std::uint64_t>::max)()-Bytes)return false;
  struct Extract {
   std::uint64_t cbv[2]{};
   void Constants(unsigned,unsigned,unsigned,const std::uint32_t*) noexcept {}
   void Descriptor(unsigned index,RootArgumentKind kind,std::uint64_t address) noexcept {
    if(index>=2&&index<=3&&kind==RootArgumentKind::CBV)cbv[index-2]=address;
   }
  } input;
  return arguments.Replay(input)&&input.cbv[0]==base&&input.cbv[1]==base+GIConstantOffset;
 }
};
}
