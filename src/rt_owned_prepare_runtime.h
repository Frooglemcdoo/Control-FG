#pragma once
#include "rt_owned_prepare.h"
#include "rt_owned_prepare_patch.h"
#include "rt_owned_prepare_bytes.h"
namespace RTOwnedPrepare {
inline rt_owned_prepare::Api api{};
inline unsigned char* base=nullptr;
inline unsigned char* relay=nullptr;
inline RUNTIME_FUNCTION unwindFunction{};
inline unsigned char replacement[14]{};
inline constexpr auto& original=rt_owned_prepare::kLoopEntry;
inline std::atomic<UINT64> calls{0},ticks{0},objects{0},instances{0},updates{0},batchGroups{0},reusedInstances{0};
inline UINT64 frequency=0;
__declspec(noinline) inline void Hook(void* capture){
 const rt_owned_prepare::Options options{base[0x90e480],base[0x90e300],base[0x90e3c0],rt_owned_assembly::Read<float>(base,0x67ed44)};
 LARGE_INTEGER begin{},end{};QueryPerformanceCounter(&begin);
 const auto stats=rt_owned_prepare::Run(api,rt_owned_assembly::Read<void*>(capture),rt_owned_assembly::Read<unsigned char>(capture,0x18),options);
 QueryPerformanceCounter(&end);
 batchGroups.fetch_add(stats.batchGroups,std::memory_order_relaxed);reusedInstances.fetch_add(stats.reusedInstances,std::memory_order_relaxed);
 objects.fetch_add(stats.objects,std::memory_order_relaxed);instances.fetch_add(stats.instances,std::memory_order_relaxed);updates.fetch_add(stats.dynamicUpdates,std::memory_order_relaxed);ticks.fetch_add(static_cast<UINT64>(end.QuadPart-begin.QuadPart),std::memory_order_relaxed);
 const auto n=calls.fetch_add(1,std::memory_order_relaxed)+1;
 if(n==1||n%512==0){const DWORD error=GetLastError();const auto elapsed=ticks.load();const auto us=elapsed/frequency*1000000+(elapsed%frequency)*1000000/frequency;
  Log("RT_PREPARE_R402 active=1 loops=%llu objects=%llu instances=%llu dynamic_updates=%llu total_loop_us=%llu batch_groups=%llu reused_instances=%llu native_object_loop=0 native_dynamic_updates=1 native_readback=1 native_as_builds=1 ptlas=0",n,objects.load(),instances.load(),updates.load(),us,batchGroups.load(),reusedInstances.load());SetLastError(error);
 }
}
inline bool ExchangeLoop(bool install)noexcept{
 auto* site=base+0xa8459;const auto* expected=install?original:replacement;const auto* next=install?replacement:original;
 if(memcmp(site,expected,sizeof(original)))return false;
 DWORD old=0;if(!VirtualProtect(site,sizeof(original),PAGE_EXECUTE_READWRITE,&old))return false;
 memcpy(site,next,sizeof(original));const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,sizeof(original))!=FALSE;
 DWORD ignored=0;const bool protectedAgain=VirtualProtect(site,sizeof(original),old,&ignored)!=FALSE;
 return flushed&&protectedAgain;
}
inline bool Install(HMODULE renderer)noexcept{
 if(!renderer||renderer!=verifiedRenderer||!HashMatches(renderer,kRendererHash))return false;
 base=reinterpret_cast<unsigned char*>(renderer);
 if(memcmp(base+0xa8459,original,sizeof(original)))return false;
 for(const auto& p:RTOwnedAssembly::patches)if(!p.site||!p.writeHealthy||memcmp(p.site,p.replacement,5))return false;
 // Reject any additional in-memory edits in the displaced loop. The three
 // verified assembly calls are the only intentional differences from the image.
 for(std::size_t i=0;i<sizeof(nativePrepareOracle);++i){
  auto expected=nativePrepareOracle[i];auto* address=base+0xa8459+i;
  for(const auto& p:RTOwnedAssembly::patches)if(address>=p.site&&address<p.site+5)expected=p.replacement[address-p.site];
  if(*address!=expected)return false;
 }
 LARGE_INTEGER f{};if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0)return false;frequency=static_cast<UINT64>(f.QuadPart);
 api={reinterpret_cast<decltype(api.count)>(base+0x1d18a0),reinterpret_cast<decltype(api.markUpdated)>(base+0x1c7c30),&RTOwnedAssembly::Hook,&RTOwnedAssembly::HookBatch};
 relay=static_cast<unsigned char*>(AllocateExecutableRelayNear(base+0xa8459));if(!relay||!Rel32Fits(base+0xa845e,relay))return false;
 // R15 is the native immutable job capture. Recreate only the displaced RCX
 // load on return, then jump past the original loop to its sizing continuation.
 unsigned char code[27]{};
 if(!rt_owned_prepare::BuildLoopPatch(reinterpret_cast<std::uintptr_t>(base+0xa8459),reinterpret_cast<std::uintptr_t>(relay),reinterpret_cast<std::uintptr_t>(&Hook),code,replacement))return false;
 memcpy(relay,code,sizeof(code));
 const unsigned char unwind[]={1,4,1,0,4,0x42,0,0};memcpy(relay+0x100,unwind,sizeof(unwind));unwindFunction={0,sizeof(code),0x100};
 DWORD old=0;if(!VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),relay,sizeof(code)))return false;
 if(!RtlAddFunctionTable(&unwindFunction,1,reinterpret_cast<DWORD64>(relay)))return false;
 if(ExchangeLoop(true))return true;
 if(!memcmp(base+0xa8459,replacement,sizeof(replacement)))ExchangeLoop(false);
 return false;
}
}
