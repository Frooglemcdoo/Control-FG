#pragma once
#include "rt_owned_selection.h"
#include <intrin.h>
namespace RTOwnedSelection {
using QueryFn=void(*)(void*,void*,std::uint32_t,std::uint32_t,std::uint32_t,void*);
inline QueryFn original=nullptr;
inline rt_owned_selection::Api api{};
inline void* returnSite=nullptr;
inline Patch patch{};
inline std::atomic<UINT64> calls{0},fallbacks{0},nodes{0},refreshes{0},partial{0},ticks{0};
inline UINT64 frequency=0;
__declspec(noinline) inline void Hook(void* grid,void* volume,std::uint32_t types,std::uint32_t flags,std::uint32_t required,void* output){
 // The vtable is shared by raster/other queries. Only this exact native RT
 // preparation caller may enter the owned path; all other callers stay native.
 if(_ReturnAddress()!=returnSite){original(grid,volume,types,flags,required,output);return;}
 if(!rt_owned_selection::Supported(grid,volume)){
  const auto n=fallbacks.fetch_add(1,std::memory_order_relaxed)+1;
  original(grid,volume,types,flags,required,output);
  if(n==1)Log("RT_SELECTION_R401 fallback=1 reason=unsupported_shape");return;
 }
 LARGE_INTEGER begin{},end{};QueryPerformanceCounter(&begin);
 const auto stats=rt_owned_selection::Query(api,grid,volume,types,flags,required,output);
 QueryPerformanceCounter(&end);
 nodes.fetch_add(stats.nodes,std::memory_order_relaxed);refreshes.fetch_add(stats.refreshes,std::memory_order_relaxed);partial.fetch_add(stats.partial,std::memory_order_relaxed);
 ticks.fetch_add(static_cast<UINT64>(end.QuadPart-begin.QuadPart),std::memory_order_relaxed);
 const auto n=calls.fetch_add(1,std::memory_order_relaxed)+1;
 if(n==1||n%512==0){const DWORD error=GetLastError();const auto elapsed=ticks.load();const auto us=elapsed/frequency*1000000+(elapsed%frequency)*1000000/frequency;
  Log("RT_SELECTION_R401 active=1 queries=%llu nodes=%llu dirty_refreshes=%llu partial_filters=%llu total_query_us=%llu fallback=%llu owned_traversal=1 traversal_heap_allocations=0 native_bounds=1 native_partial_filters=1 native_object_updates=1 native_as_builds=1 ptlas=0",n,nodes.load(),refreshes.load(),partial.load(),us,fallbacks.load());SetLastError(error);
 }
}
inline bool Install(HMODULE renderer)noexcept{
 if(!renderer||renderer!=verifiedRenderer||!HashMatches(renderer,kRendererHash))return false;
 auto* base=reinterpret_cast<unsigned char*>(renderer);
 const unsigned char expected[]={0x41,0xff,0x52,0x50};
 if(memcmp(base+0xa83d9,expected,sizeof(expected)))return false;
 original=reinterpret_cast<QueryFn>(base+0x165520);returnSite=base+0xa83dd;
 api.refresh=reinterpret_cast<decltype(api.refresh)>(base+0x165dc0);api.append=reinterpret_cast<decltype(api.append)>(base+0x22730);
 api.partial[0]=reinterpret_cast<rt_owned_selection::Partial>(base+0x165f70);
 for(unsigned n=1;n<25;++n)api.partial[n]=reinterpret_cast<rt_owned_selection::Partial>(base+0x166030+(n-1)*0x150);
 LARGE_INTEGER f{};if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0)return false;frequency=static_cast<UINT64>(f.QuadPart);
 patch={reinterpret_cast<void**>(base+0x634cb8+0x50),reinterpret_cast<void*>(original),reinterpret_cast<void*>(&Hook),"RT owned selection"};
 if(*patch.slot!=patch.original)return false;
 return Exchange(patch,true);
}
}
