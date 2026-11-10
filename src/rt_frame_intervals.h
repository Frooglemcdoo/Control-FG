#pragma once
#include <cstdint>
namespace rt_frame_intervals {
using U64=std::uint64_t;
struct Interval {U64 begin=0,end=0,present=0,us=0,observerUs=0;};
struct Window {
 U64 count=0,totalUs=0,observerUs=0,over25=0,over33=0,over50=0,over100=0;Interval top[4]{};
 void Add(Interval value){++count;totalUs+=value.us;observerUs+=value.observerUs;over25+=value.us>=25000;over33+=value.us>=33333;over50+=value.us>=50000;over100+=value.us>=100000;
  for(unsigned i=0;i<4;++i)if(value.us>top[i].us){for(unsigned j=3;j>i;--j)top[j]=top[j-1];top[i]=value;break;}
 }
};
}
