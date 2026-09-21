#include "input_policy.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace hdrguard;
int main(){
 unsigned checks=0;
 for(unsigned mask=0;mask<1024;++mask){InputReadiness r{};
  r.ready=(mask&1)!=0;r.foreground=(mask&2)!=0;r.busy=(mask&4)==0;r.frames=(mask&8)?10:0;
  r.frameAge=(mask&16)?20:500;r.gpuKnown=(mask&32)!=0;r.rtx40=(mask&64)==0;r.selection=(mask&128)?2:0;
  r.displayValid=(mask&256)!=0;r.displayAge=(mask&512)?20:500;
  bool accepted=inputAllowed(r);if(accepted!=(mask==1023) || accepted!=(std::strcmp(inputReason(r),"accepted")==0))return 1;
  ++checks;
 }
 for(uint32_t m:{0u,0x0312u,0x0100u}){for(uintptr_t id:{0u,0xc000u,0xc001u}){
   if(isControllerMessage(m,id,0xc000)!=(m==0x0312 && id==0xc000))return 2;++checks;
 }}
 if(isControllerMessage(0x0312,0,0))return 3;++checks;
 std::printf("PASS %u input/dispatch checks; every blocked readiness combination has an explicit reason\n",checks);return 0;
}
