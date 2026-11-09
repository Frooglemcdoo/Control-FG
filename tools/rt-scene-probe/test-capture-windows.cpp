#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
static HANDLE saved=nullptr;
static void Log(const char* format,...){va_list a;va_start(a,format);std::vprintf(format,a);va_end(a);std::puts("");if(std::strstr(format,"saved=%u"))SetEvent(saved);}
static HMODULE verifiedRenderer=nullptr;
static const char* kRendererHash="test";
static bool HashMatches(HMODULE,const char*){return false;}
struct RRIndirectCallPatch{bool changed=false,healthy=false;bool Prepare(unsigned char*,void**,void*,void*){return false;}bool Exchange(bool){return false;}};
struct RRAlbedoCallPatch{unsigned char* site=nullptr;unsigned char replacement[5]{};bool writeHealthy=false;};
static bool RRAlbedoPrepareCallPatch(unsigned char*,void*,void*,RRAlbedoCallPatch*){return false;}
static bool RRAlbedoExchangeCall(RRAlbedoCallPatch*,bool){return false;}
#include "../../src/rt_instance_capture.h"
static unsigned locks=0,copies=0;
static unsigned char mapped[64]{};
static void Lock(void*,UINT64,UINT64,unsigned,void** out){++locks;*out=mapped;SetLastError(117);}
static void* Copy(void* to,const void* from,size_t bytes){++copies;if(GetLastError()!=117)throw std::runtime_error("LastError altered before native copy");SetLastError(118);return std::memcpy(to,from,bytes);}
static void Require(bool value){if(!value)throw std::runtime_error("Capture test assertion");}
int main(){try{
 saved=CreateEventW(nullptr,TRUE,FALSE,nullptr);Require(saved!=nullptr);
 RTInstanceCapture::originalLock=&Lock;RTInstanceCapture::originalCopy=&Copy;
 RTInstanceCapture::output=L"RT-instances-fixture.bin";DeleteFileW(RTInstanceCapture::output.c_str());
 RTInstanceCapture::ready=true;RTInstanceCapture::started=GetTickCount64()-6000;
 std::array<unsigned char,0x50> wrapper{};
 UINT64 resource=1,capacity=64,address=0x20000;
 std::memcpy(wrapper.data(),&resource,8);std::memcpy(wrapper.data()+0x30,&capacity,8);std::memcpy(wrapper.data()+0x48,&address,8);
 struct Instance{float matrix[12];unsigned id,flags;UINT64 blas;} instance{{1,0,0,0,0,1,0,0,0,0,1,0},0xff000007,0,0x10000};
 static_assert(sizeof(instance)==64);
 for(unsigned i=0;i<48;++i){
  RTInstanceCapture::last=0;instance.matrix[3]=float(i);void* out=nullptr;
  RTInstanceCapture::LockHook(wrapper.data(),0,64,2,&out);Require(GetLastError()==117);
  Require(RTInstanceCapture::CopyHook(out,&instance,64)==out);Require(GetLastError()==118);
  Require(std::memcmp(out,&instance,64)==0);
 }
 Require(WaitForSingleObject(saved,10000)==WAIT_OBJECT_0);
 {std::lock_guard<std::mutex> guard(RTInstanceCapture::mutex);Require(RTInstanceCapture::captured==48);}
 // After the cap, native forwarding continues without additional snapshots.
 void* out=nullptr;RTInstanceCapture::LockHook(wrapper.data(),0,64,2,&out);RTInstanceCapture::CopyHook(out,&instance,64);
 Require(locks==49&&copies==49&&RTInstanceCapture::captured==48);
 Require(!RTInstanceCapture::Install(nullptr));
 CloseHandle(saved);std::puts("PASS native forwarding, LastError, capture cap and real Windows file writer");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
