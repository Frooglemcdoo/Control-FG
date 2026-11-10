#include <windows.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <thread>
#include <atomic>
#include "../../src/rt_allocator_probe.h"
#include "../../src/rt_allocator_probe_patch.h"
namespace {
rt_allocator_probe::Api testApi{};
thread_local rt_allocator_probe::Sample* testSample=nullptr;
SRWLOCK testLock=SRWLOCK_INIT;
unsigned captures=0;
void Enter(void* lock){rt_allocator_probe::Enter(testApi,testSample,lock);}
void Leave(void* lock){rt_allocator_probe::Leave(testApi,testSample,lock);}
std::uint64_t Clock(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return static_cast<std::uint64_t>(value.QuadPart);}
std::uint32_t Error(){return GetLastError();}
void Restore(std::uint32_t value){SetLastError(value);}
rt_allocator_probe::Snapshot Capture(void*){++captures;return {&testLock,1,1,captures==1?0u:49u,1};}
}
int main(){
 testApi={reinterpret_cast<void(*)(void*)>(&AcquireSRWLockExclusive),reinterpret_cast<void(*)(void*)>(&ReleaseSRWLockExclusive),Clock,Error,Restore,Capture};
 auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(memory);
 // Win64: reserve shadow space, call through RIP-relative IAT, restore stack, return.
 const unsigned char body[]={0x48,0x83,0xec,0x28,0xff,0x15,0,0,0,0,0x48,0x83,0xc4,0x28,0xc3};
 for(unsigned i=0;i<2;++i){
  auto* fn=memory+i*128;std::memcpy(fn,body,sizeof(body));
  auto* target=i?reinterpret_cast<void*>(&ReleaseSRWLockExclusive):reinterpret_cast<void*>(&AcquireSRWLockExclusive);
  const std::int32_t displacement=64-10;std::memcpy(fn+6,&displacement,4);std::memcpy(fn+64,&target,8);
 }
 assert(FlushInstructionCache(GetCurrentProcess(),memory,4096));
 auto enter=reinterpret_cast<void(*)(void*)>(memory);auto leave=reinterpret_cast<void(*)(void*)>(memory+128);
 enter(&testLock);leave(&testLock);
 for(unsigned i=0;i<2;++i){
  auto* fn=memory+i*128;auto* relay=fn+80;auto* hook=i?reinterpret_cast<void*>(&Leave):reinterpret_cast<void*>(&Enter);
  const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(relay,jump,6);std::memcpy(relay+6,&hook,8);
  unsigned char patch[6]{};assert(rt_allocator_probe::Call6(reinterpret_cast<std::uintptr_t>(fn+4),reinterpret_cast<std::uintptr_t>(relay),patch));std::memcpy(fn+4,patch,6);
 }
 DWORD old=0;assert(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),memory,4096));
 enter(&testLock);leave(&testLock);assert(!captures);
 AcquireSRWLockExclusive(&testLock);
 std::atomic<bool> started{false},finished{false};
 rt_allocator_probe::Sample result;
 std::thread worker([&]{testSample=&result;started=true;enter(&testLock);leave(&testLock);testSample=nullptr;finished=true;});
 while(!started.load())SwitchToThread();
 assert(!finished.load());ReleaseSRWLockExclusive(&testLock);worker.join();
 assert(result.entered&&result.left&&captures==2&&rt_allocator_probe::Classify(result)==0);
 assert(TryAcquireSRWLockExclusive(&testLock));ReleaseSRWLockExclusive(&testLock);
 assert(VirtualFree(memory,0,MEM_RELEASE));
}
