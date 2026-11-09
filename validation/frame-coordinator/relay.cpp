#include "../../src/rr_indirect_call.h"
#include <sys/mman.h>
#include <cassert>
#include <cstdio>
#define ABI __attribute__((ms_abi))
static unsigned originalCalls=0,hookCalls=0;
static bool ABI original(unsigned a,unsigned b,unsigned c,unsigned d,bool e,bool f,bool g,bool h,bool& reset){assert(a==3840&&b==2160&&c==2560&&d==1440&&e&&!f&&g&&!h);++originalCalls;reset=true;return true;}
static bool ABI hook(unsigned a,unsigned b,unsigned c,unsigned d,bool e,bool f,bool g,bool h,bool& reset){++hookCalls;return original(a,b,c,d,e,f,g,h,reset);}
int main(){
 auto* image=static_cast<unsigned char*>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));assert(image!=MAP_FAILED);
 unsigned char code[]={0x48,0x83,0xec,0x48,
  0x48,0x8b,0x44,0x24,0x70,0x48,0x89,0x44,0x24,0x20,
  0x48,0x8b,0x44,0x24,0x78,0x48,0x89,0x44,0x24,0x28,
  0x48,0x8b,0x84,0x24,0x80,0,0,0,0x48,0x89,0x44,0x24,0x30,
  0x48,0x8b,0x84,0x24,0x88,0,0,0,0x48,0x89,0x44,0x24,0x38,
  0x48,0x8b,0x84,0x24,0x90,0,0,0,0x48,0x89,0x44,0x24,0x40};
 std::memcpy(image,code,sizeof(code));auto*site=image+sizeof(code);auto*slot=image+256;auto*relay=image+512;
 control_rr::IndirectCall6 old{{0xff,0x15,0,0,0,0}};auto offset=static_cast<std::int32_t>(slot-site-6);std::memcpy(old.bytes+2,&offset,4);std::memcpy(site,old.bytes,6);
 unsigned char tail[]{0x48,0x83,0xc4,0x48,0xc3};std::memcpy(site+6,tail,5);void*target=reinterpret_cast<void*>(&original);std::memcpy(slot,&target,8);
 using Function=bool(ABI *)(unsigned,unsigned,unsigned,unsigned,bool,bool,bool,bool,bool&);auto call=reinterpret_cast<Function>(image);
 assert(!mprotect(image,4096,PROT_READ|PROT_EXEC));bool reset=false;assert(call(3840,2160,2560,1440,true,false,true,false,reset)&&reset&&originalCalls==1&&hookCalls==0);
 assert(!mprotect(image,4096,PROT_READ|PROT_WRITE));unsigned char jump[]{0xff,0x25,0,0,0,0};std::memcpy(relay,jump,6);target=reinterpret_cast<void*>(&hook);std::memcpy(relay+6,&target,8);
 control_rr::IndirectCall6 patch{};assert(control_rr::ReplaceIndirectCall(old,reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(slot),reinterpret_cast<std::uintptr_t>(relay),patch));std::memcpy(site,patch.bytes,6);assert(!mprotect(image,4096,PROT_READ|PROT_EXEC));
 for(unsigned i=0;i<10000;++i){reset=false;assert(call(3840,2160,2560,1440,true,false,true,false,reset)&&reset);}
 assert(originalCalls==10001&&hookCalls==10000);munmap(image,4096);puts("PASS executed six-byte indirect-call replacement: all nine Microsoft ABI arguments, stack reference and return preserved 10000 times");
}
