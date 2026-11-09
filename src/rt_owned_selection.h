#pragma once
#include "rt_owned_assembly.h"
#include <smmintrin.h>
#include <cassert>
namespace rt_owned_selection {
using rt_owned_assembly::Read;
using rt_owned_assembly::Write;
using rt_owned_assembly::Method;
using Partial=void(*)(void*,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,void*,void*,const void*);
struct Api {
 void(*refresh)(void*,int,void*);
 void(*append)(void*,const void*);
 Partial partial[25]{};
};
struct Stats {std::uint64_t layers=0,nodes=0,refreshes=0,partial=0,accepted=0,rejected=0;};
inline bool Supported(void* grid,void* volume) {
 const auto planes=Read<std::int32_t>(volume,0x180),levels=Read<std::int32_t>(grid,0x64);
 return planes>=0&&planes<=24&&levels>=1&&levels<=16;
}
// The caller remains on the original preparation worker. No scene pointers survive
// this invocation. Native dirty-bound refresh and partial-node filters stay in use.
inline void Layer(const Api& api,void* grid,int layer,void* volume,std::uint32_t types,
                  std::uint32_t flags,std::uint32_t required,void* output,Stats& stats) {
 struct Entry {void* node;std::uint32_t mask;};
 Entry stack[1024];unsigned size=0;
 const auto planes=Read<std::int32_t>(volume,0x180);
 const auto initial=planes>0?((1u<<planes)-1u):0u;
 auto* root=Read<void*>(Read<void*>(grid,0x68+std::size_t(layer)*256));
 if(root&&Read<std::uint32_t>(root,0x30))stack[size++]={root,initial};
 ++stats.layers;
 while(size){
  const auto entry=stack[--size];auto* node=entry.node;auto mask=entry.mask;++stats.nodes;
  if(!Read<unsigned char>(node,0x39)){api.refresh(grid,layer,node);++stats.refreshes;}
  bool rejected=false;std::uint32_t intersect=0;
  if(mask){
   const auto center=_mm_loadu_ps(static_cast<const float*>(node));
   const auto extent=_mm_loadu_ps(reinterpret_cast<const float*>(static_cast<unsigned char*>(node)+16));
   for(unsigned index=0;index<24;++index){
    const auto bit=1u<<index;if(!(mask&bit))continue;
    const auto plane=_mm_loadu_ps(reinterpret_cast<const float*>(static_cast<unsigned char*>(volume)+index*16));
    const auto distance=_mm_add_ps(_mm_shuffle_ps(plane,plane,0xff),_mm_dp_ps(plane,center,0x7f));
    const auto radius=_mm_dp_ps(_mm_and_ps(plane,_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))),extent,0x7f);
    const auto negative=_mm_xor_ps(radius,_mm_castsi128_ps(_mm_set1_epi32(static_cast<int>(0x80000000u))));
    if(_mm_movemask_ps(_mm_cmplt_ps(distance,negative))==15){rejected=true;break;}
    if(_mm_movemask_ps(_mm_cmplt_ps(distance,radius))==15)intersect|=bit;
   }
  }
  if(rejected){++stats.rejected;continue;}
  mask=intersect;
  if(mask){
   alignas(16) unsigned char packed[24*16];unsigned count=0;
   for(unsigned index=0;index<24;++index)if(mask&(1u<<index)){std::memcpy(packed+count*16,static_cast<unsigned char*>(volume)+index*16,16);++count;}
   api.partial[count](volume,mask,types,flags,required,output,node,packed);++stats.partial;
  }else{
   auto* begin=Read<unsigned char*>(node,0x20);const auto count=Read<std::uint32_t>(node,0x28);
   for(std::uint32_t index=0;index<count;++index){
    auto* record=begin+std::size_t(index)*48;
    if(!(Read<std::uint32_t>(record,0x28)&types))continue;
    auto* object=Read<void*>(record,0x20);const auto objectFlags=Read<std::uint32_t>(object,0xc);
    if((objectFlags&flags)!=required)continue;
    if(objectFlags&4){using Test=unsigned char(*)(void*,void*,std::uint32_t);if(!Method<Test>(object,0x28)(object,volume,mask))continue;}
    api.append(output,&object);++stats.accepted;
   }
  }
  const auto depth=Read<unsigned char>(node,0x38);const unsigned next=unsigned(depth)+1;
  if(next>=Read<std::uint32_t>(grid,0x64))continue;
  const auto x=unsigned(Read<std::uint16_t>(node,0x34))*2u,y=unsigned(Read<std::uint16_t>(node,0x36))*2u;
  const auto offset=(std::size_t(layer)*32+depth+14)*8;
  for(unsigned child=0;child<4;++child){
   const auto index=((y+child/2)<<(next&31))+x+child%2;
   auto* candidate=Read<void*>(Read<unsigned char*>(grid,offset)+std::size_t(index)*8);
   if(candidate&&Read<std::uint32_t>(candidate,0x30)){
    // At <=16 levels the four-way DFS frontier needs <=46 entries.
    assert(size<1024);stack[size++]={candidate,mask};
   }
  }
 }
}
inline Stats Query(const Api& api,void* grid,void* volume,std::uint32_t types,std::uint32_t flags,std::uint32_t required,void* output){
 Stats stats;
 if(!(flags&(1u<<17))||(required&(1u<<17)))Layer(api,grid,0,volume,types,flags,required,output,stats);
 Layer(api,grid,1,volume,types,flags,required,output,stats);return stats;
}
}
