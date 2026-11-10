#pragma once
#include "rt_frame_intervals.h"
// Called only at the existing engine present-entry boundary. These are engine
// intervals, not GPU execution times or generated/displayed frame durations.
static void RTPerformancePresent(unsigned long long present){
 static LONGLONG previous=0;static UINT64 reportUs=0;static rt_frame_intervals::Window window;
 const DWORD error=GetLastError();LARGE_INTEGER now{};QueryPerformanceCounter(&now);
 if(previous>0&&now.QuadPart>=previous&&frequency.QuadPart>0){
  const auto ticks=static_cast<UINT64>(now.QuadPart-previous),freq=static_cast<UINT64>(frequency.QuadPart);
  const auto us=ticks/freq*1000000+(ticks%freq)*1000000/freq;
  window.Add({static_cast<UINT64>(previous),static_cast<UINT64>(now.QuadPart),present,us,reportUs});reportUs=0;
  if(window.count==300){
   LARGE_INTEGER reportStart{},reportEnd{};QueryPerformanceCounter(&reportStart);
   const auto& a=window.top[0];const auto& b=window.top[1];const auto& c=window.top[2];const auto& d=window.top[3];
   Log("RT_FRAME_R410 present=%llu intervals=%llu total_us=%llu mean_us=%llu max_us=%llu over_25ms=%llu over_33ms=%llu over_50ms=%llu over_100ms=%llu observer_us=%llu aa_count=%llu top1_present=%llu top1_us=%llu top1_begin=%llu top1_end=%llu top1_observer_us=%llu top2_present=%llu top2_us=%llu top2_begin=%llu top2_end=%llu top2_observer_us=%llu top3_present=%llu top3_us=%llu top3_begin=%llu top3_end=%llu top3_observer_us=%llu top4_present=%llu top4_us=%llu top4_begin=%llu top4_end=%llu top4_observer_us=%llu engine_present_entry=1 gpu_duration=0 generated_frames=0 menus_loading_included=1",present,window.count,window.totalUs,window.totalUs/window.count,a.us,window.over25,window.over33,window.over50,window.over100,window.observerUs,aaCount.load(),a.present,a.us,a.begin,a.end,a.observerUs,b.present,b.us,b.begin,b.end,b.observerUs,c.present,c.us,c.begin,c.end,c.observerUs,d.present,d.us,d.begin,d.end,d.observerUs);
   QueryPerformanceCounter(&reportEnd);const auto reporting=static_cast<UINT64>(reportEnd.QuadPart-reportStart.QuadPart);
   reportUs=reporting/freq*1000000+(reporting%freq)*1000000/freq;window={};
  }
 }
 previous=now.QuadPart;SetLastError(error);
}
