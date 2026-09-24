#include <sys/mman.h>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <stdexcept>
#define __except(x) catch(...)
using DWORD=unsigned long;using HMODULE=void*;
static unsigned char* module=nullptr;static HMODULE verifiedD3d=nullptr;
static DWORD error=0;
static DWORD GetLastError(){return error;}
static void SetLastError(DWORD value){error=value;}
template<class... T> static void Log(const char*,T...){error=999;}
static std::map<std::string,unsigned> values;
static unsigned reads=0,sets=0,loseRead=0,loseSet=0,calls=0;
static void setter(void*,const char* key,unsigned value){if(++sets!=loseSet)values[key]=value;}
static unsigned getter(void*,const char* key,unsigned* value){*value=values[key];return ++reads==loseRead?0u:1u;}
static auto ngxGetUInt=&getter;
static unsigned char relays[4096];static unsigned writes=0;static bool failWrite=false;
static void* GetProcAddress(HMODULE,const char*){return module+0x528d0;}
static void* AllocateExecutableRelayNear(void*){return module+0x53000;}
constexpr unsigned PAGE_EXECUTE_READ=0x20;
static bool VirtualProtect(void*,unsigned,unsigned,DWORD* p){*p=4;return true;}
static void* GetCurrentProcess(){return nullptr;}
static bool FlushInstructionCache(void*,void*,std::size_t){return true;}
struct RRAlbedoCallPatch{unsigned char* site;unsigned char original[5],replacement[5];void* relay;bool writeHealthy;};
static bool RRAlbedoExchangeCall(RRAlbedoCallPatch* p,bool install){
 ++writes;if(failWrite)return false;
 if(std::memcmp(p->site,install?p->original:p->replacement,5))return false;
 std::memcpy(p->site,install?p->replacement:p->original,5);p->writeHealthy=true;return true;
}
#include "../../src/rr_native_preset.h"
static unsigned native(void* list,unsigned kind,void* params,void** out){
 assert(list==reinterpret_cast<void*>(1)&&kind==13&&params==reinterpret_cast<void*>(2)&&GetLastError()==17);
 for(const char* key:control_rr::RRPresetKeys)assert(values[key]==control_rr::RRUserPresetValue());
 ++calls;*out=reinterpret_cast<void*>(3);SetLastError(18);return 1;
}
int main(){
 (void)relays;
 module=static_cast<unsigned char*>(mmap(nullptr,0x54000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));assert(module!=MAP_FAILED);verifiedD3d=module;
 unsigned char stub[14]{0xff,0x25,0,0,0,0};auto fn=&setter;memcpy(stub+6,&fn,8);memcpy(module+0x51f50,stub,14);
 assert(!RRNativeInstallPreset(nullptr));assert(!RRNativeInstallPreset(module));assert(writes==0);
 control_rr::Jump5 jump{};assert(control_rr::RelativeJump(reinterpret_cast<std::uintptr_t>(module+0x1d457),reinterpret_cast<std::uintptr_t>(module+0x528d0),jump));
 memcpy(module+0x1d457,jump.data(),5);failWrite=true;assert(!RRNativeInstallPreset(module));failWrite=false;assert(RRNativeInstallPreset(module));
 rrNativeCreateOriginal=&native;
 const unsigned presets[]={control_rr::RRPresetE,control_rr::RRPresetF,control_rr::RRPresetK,control_rr::RRPresetL,control_rr::RRPresetM};
 for(unsigned preset:presets){
  control_rr::RRUserSetPresetValue(preset);assert(control_rr::RRUserPresetValue()==preset);
  for(unsigned mode=0;mode<3;++mode)for(unsigned failure=0;failure<=6;++failure){
   values.clear();reads=sets=0;loseRead=mode==1?failure:0;loseSet=mode==2?failure:0;
   void* out=nullptr;SetLastError(17);unsigned before=calls;
   unsigned result=RRNativeCreateWithPreset(reinterpret_cast<void*>(1),13,reinterpret_cast<void*>(2),&out);
   bool good=mode==0||failure==0;assert(result==(good?1u:0xBAD00001u));assert(calls==before+unsigned(good));assert(GetLastError()==(good?18:17));assert(out==(good?reinterpret_cast<void*>(3):nullptr));
  }
 }
 const auto before=calls;void* out=nullptr;assert(RRNativeCreateWithPreset(nullptr,1,nullptr,&out)==0xBAD00001u);assert(calls==before);
 munmap(module,0x54000);
 puts("PASS selectable native RR presets E/F/K/L/M: six modes set/read back before exactly one creation, all lost setters/getters reject, argument/result/error preservation and checked installation");
}
