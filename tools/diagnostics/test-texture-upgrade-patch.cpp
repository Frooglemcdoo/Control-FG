#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include "../../src/native_texture_upgrade_patch.h"
static void Check(bool ok,const char* label){if(!ok){printf("FAIL %s\n",label);fflush(stdout);ExitProcess(1);}}
int main(){
 using namespace native_texture_upgrade;
 auto* p=static_cast<unsigned char*>(VirtualAlloc(nullptr,8*1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check(p!=nullptr,"allocation");
 memset(p,0x90,128);p[0]=0x8b;p[1]=0xc1; // eax=first integer argument
 auto* site=p+8;memcpy(site,Original,8);
 const unsigned char yes[]={0xb8,1,0,0,0,0xc3},no[]={0x31,0xc0,0xc3};
 memcpy(site+8,yes,sizeof(yes));memcpy(site+8+0x2f,no,sizeof(no));int min=-5;memcpy(site+6+0x6a8172,&min,4);
 DWORD old;Check(VirtualProtect(p,8*1024*1024,PAGE_EXECUTE_READ,&old)!=FALSE,"RX protection");FlushInstructionCache(GetCurrentProcess(),p,128);
 using Fn=int(*)(int);auto fn=reinterpret_cast<Fn>(p);
 Check(fn(-5)==1&&fn(-1)==0&&fn(0)==0&&fn(3)==0,"original strict minimum");
 Patch patch;Check(!patch.Prepare(site+1)&&patch.Prepare(site),"alignment and exact bytes");
 Check(patch.Exchange(true)&&patch.changed&&patch.healthy,"patch installation");
 for(int delta=-8;delta<=8;++delta)Check(fn(delta)==int(delta<0),"only upgrades pass");
 Check(!patch.Exchange(true),"duplicate exchange rejected");
 Check(patch.Exchange(false)&&!patch.changed&&patch.healthy,"rollback");
 Check(fn(-5)==1&&fn(-1)==0&&!memcmp(site,Original,8),"original behavior restored");
 VirtualFree(p,0,MEM_RELEASE);puts("PASS R254 executable gate: strict original, negative-only upgrade eligibility, non-upgrade rejection, exact bytes, atomic exchange and rollback");
}
