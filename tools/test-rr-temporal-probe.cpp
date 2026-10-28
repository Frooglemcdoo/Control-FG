#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#ifndef _MSC_VER
#define __try try
#define __except(x) catch(...)
#endif
#define CONTROL_RR_TEMPORAL_PROBE
using DWORD=unsigned long;
DWORD lastError=77;
DWORD GetLastError(){return lastError;}
void SetLastError(DWORD v){lastError=v;}
struct Frame {bool selected=false;bool Selected(){return selected;}} rrNativeFrame;
namespace control_rr_native_clamp {struct Entry {int offset=0,reserved=0,size=4,revisionIndex=0;};}
unsigned readCount=0,traceCount=0,attemptCount=0;bool readOK=true;
struct Api {
 bool ready=true;
 static void Resolve(void* raw,const char* name){auto* slot=static_cast<int*>(raw);slot[0]=!std::strcmp(name,"g_uTemporalFrame")?1:2;}
 void (*resolve)(void*,const char*)=&Resolve;
 bool ReadCurrent(int id,std::uint32_t& value,control_rr_native_clamp::Entry* entry=nullptr){++readCount;value=id==1?7:0;if(entry)*entry={};return readOK;}
} rrNativeClampApi;
namespace native_lighting {
void Trace(const char* format,...){++traceCount;va_list args;va_start(args,format);char line[1024];vsnprintf(line,sizeof(line),format,args);va_end(args);if(std::strstr(line,"stage=pre_reflection_filter")){++attemptCount;assert(std::strstr(line,"gpu_confirmed=0"));assert(std::strstr(line,readOK?"valid=1":"valid=0"));}}
}
#include "../src/rr_temporal_probe.h"
int main(){
 RRTemporalProbe(1,4);assert(readCount==0);
 rrNativeFrame.selected=true;RRTemporalProbe(1,16);assert(readCount==0);
 RRTemporalProbe(1,4);assert(readCount==4&&attemptCount==1&&lastError==77);
 RRTemporalProbe(1,4);assert(readCount==4&&attemptCount==1);
 readOK=false;RRTemporalProbe(2,4);assert(attemptCount==2);readOK=true;
 for(unsigned i=3;i<=1024;++i)RRTemporalProbe(i,4);
 assert(attemptCount==1024&&traceCount==1025);
 unsigned saved=readCount;RRTemporalProbe(1025,4);assert(readCount==saved&&attemptCount==1024);
 puts("PASS: actual probe admission, duplicate suppression, invalid reads, provenance and bounded work");
}
