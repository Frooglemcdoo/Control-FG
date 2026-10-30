#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
static void Log(const char*,...){}
static bool HashMatches(HMODULE,const char*){return false;}
static void* AllocateExecutableRelayNear(const void*){return nullptr;}
static bool Rel32Fits(const void*,const void*){return false;}
#include "../src/native_cpu_call_patch.h"
#include "../src/native_loader_trace_runtime.h"
static unsigned char record[0x188]{};
static void* expectedSelf=reinterpret_cast<void*>(0x1234);
static void Fill(void* arguments,void* destination){
    assert(arguments==reinterpret_cast<void*>(0x5678)&&destination==record);
    std::uint64_t id=42,size=4096;memcpy(record+0x178,&id,8);memcpy(record+0x150,&size,8);SetLastError(123);
}
static std::int64_t Submit(void* self,void* arguments){
    assert(self==expectedSelf);native_loader_trace::Construct(arguments,record);
    assert(GetLastError()==123);SetLastError(456);return 42;
}
static void Execute(void* self,void* input){assert(self==expectedSelf&&input==record&&GetLastError()==789);SetLastError(321);}
int main(){
    using namespace native_loader_trace;
    construct=&Fill;request=&Submit;worker=&Execute;
    assert(RequestHook(expectedSelf,reinterpret_cast<void*>(0x5678))==42);
    assert(GetLastError()==456&&requestOwner==0&&tracker.queued.load()==1);
    SetLastError(789);Worker(expectedSelf,record);
    assert(GetLastError()==321&&tracker.matched.load()==1&&tracker.bytes.load()==4096&&workerCount.load()==1);
    puts("PASS R235 runtime forwarding: original arguments, native request ID return, scoped owner, worker correlation and LastError preservation");
}
