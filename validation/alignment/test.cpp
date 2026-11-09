#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include <cassert>
using UINT=unsigned int;using HRESULT=long;using SRWLOCK=int;
#define SRWLOCK_INIT 0
#define VK_F8 119
struct IDXGISwapChain {};
static void AcquireSRWLockExclusive(SRWLOCK*){} static void ReleaseSRWLockExclusive(SRWLOCK*){}
static void AcquireSRWLockShared(SRWLOCK*){} static void ReleaseSRWLockShared(SRWLOCK*){}
static short key=0;static short GetAsyncKeyState(int){return key;}
static std::atomic<unsigned long long> aaCount{1},beginCount{1};
static std::vector<std::string> logs;
static void Log(const char* f,...){char b[2048];va_list a;va_start(a,f);vsnprintf(b,sizeof(b),f,a);va_end(a);logs.emplace_back(b);}
#include "../../src/fg_alignment_trace.h"
int main(){
 int a=0,b=0;
 FGAlignProduced(10,&a,nullptr);FGAlignBridge(10,0,&a,&b,nullptr);
 assert(logs.back().find("target_matches=1")!=std::string::npos);
 FGAlignProduced(11,&b,nullptr);FGAlignBridge(11,0,&a,&b,nullptr);
 assert(logs.back().find("matched_target=10")!=std::string::npos);
 assert(logs.back().find("source_is_latest=0 target_matches=0")!=std::string::npos);
 ++fgAlignEpoch;FGAlignBridge(12,0,&a,&b,nullptr);
 assert(logs.back().find("epoch=1")!=std::string::npos&&logs.back().find("matched_epoch=0")!=std::string::npos);
 for(unsigned long long n=20;n<85;++n) FGAlignProduced(n,&b,nullptr);
 FGAlignBridge(85,0,&a,&b,nullptr);assert(logs.back().find("matched_sequence=0")!=std::string::npos);
 auto size=logs.size();FGAlignBridge(241,0,&a,&b,nullptr);assert(logs.size()==size);
 key=short(0x8000);FGAlignPoll(250);assert(fgAlignUntil==2050);size=logs.size();FGAlignPoll(251);assert(logs.size()==size);
 key=0;FGAlignPoll(252);key=short(0x8000);FGAlignPoll(253);assert(fgAlignUntil==2053);
 IDXGISwapChain swap;auto serial=FGAlignPresentEnter(254,&swap,"resize_quiesce",0);FGAlignPresentExit(serial,254,&swap,-1);
 assert(serial==1&&logs.back().find("ALIGN_DXGI_EXIT serial=1")!=std::string::npos);
 for(auto& line:logs) { puts(line.c_str()); }
 puts("PASS alignment provenance, expiry, epoch, hotkey and call pairing");
}
