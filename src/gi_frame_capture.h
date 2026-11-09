#pragma once
#include <windows.h>
#include <atomic>
#include <cstdio>
namespace control_gi_frames {
// Activated only by RECORD GI. One QPC per native rendered frame; bounded batches,
// no ETW, GPU waits, resource copies, or per-frame file writes.
inline std::atomic<ULONGLONG> until{0};
inline std::atomic<unsigned> generation{0};
inline void Start() noexcept {until.store(GetTickCount64()+120000);++generation;}
inline void Present() noexcept {
 if(!until.load(std::memory_order_relaxed))return;
 static unsigned seen=0,count=0;static LONGLONG previous=0,first=0;
 static char values[1536]{};static size_t used=0;
 LARGE_INTEGER now{},freq{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&freq);
 const auto gen=generation.load();
 if(gen!=seen){seen=gen;previous=now.QuadPart;first=previous;count=0;used=0;return;}
 const auto delta=now.QuadPart-previous;
 if(delta>0){
  const int n=snprintf(values+used,sizeof(values)-used,"%s%lld",count?",":"",delta);
  if(n>0&&size_t(n)<sizeof(values)-used){used+=size_t(n);++count;}
 }
 previous=now.QuadPart;
 const bool expired=GetTickCount64()>=until.load();
 if(count>=64||used>1400||now.QuadPart-first>=freq.QuadPart||expired){
  if(count)Log("GI_FRAME_TIMES start_qpc=%lld end_qpc=%lld frequency=%lld count=%u ticks=%s",first,now.QuadPart,freq.QuadPart,count,values);
  first=now.QuadPart;count=0;used=0;values[0]=0;
 }
 if(expired)until.store(0);
}
}
