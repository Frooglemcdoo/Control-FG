#pragma once
#include <array>
#include <cstdint>
namespace gi_reuse {
// Only descriptor tables are invalidated by a temporary heap switch. Graphics
// roots/constants/descriptors, PSO and raster state are otherwise untouched.
struct GraphicsTableState {
 std::array<std::uint64_t,64> tables{};
 std::uint64_t root=0,mask=0;bool known=false;
 void Reset()noexcept{tables={};root=mask=0;known=true;}
 void Invalidate()noexcept{known=false;}
 void Root(std::uint64_t value)noexcept{if(root!=value){root=value;mask=0;}}
 void Table(unsigned index,std::uint64_t value)noexcept{
  if(!root||index>=64||!value){Invalidate();return;}tables[index]=value;mask|=std::uint64_t(1)<<index;
 }
 void HeapChange()noexcept{mask=0;}
 template<class F> void Replay(F&& bind)const noexcept{
  if(!known)return;for(unsigned i=0;i<64;++i)if(mask&(std::uint64_t(1)<<i))bind(i,tables[i]);
 }
};
}
